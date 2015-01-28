#!/bin/env python

__copyright__ = """Copyright 2015 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
"""
__usage__ = """
   makeInstancesUFO program v1.04 Jan 28 2015
   makeInstancesUFO -h
   makeInstancesUFO -u
   makeInstancesUFO [-a]  [-d <design space file name>]
                 [-c] [-a] [-i ,i0,i1..,in]

   -d <design space file path> .... Specifies alternate path to design space file.
                                Default: font.designspace

   -i <list of instance indices> Build only the listed instances, i is a  0-based index of the instance records in the instance file.
   -a ... do NOT autohint the instance font files. Default is to do so, as master designs are unhinted.
   -c ... do NOT run checkOutlines to remove overlaps from the instance font files. Default is to do so.
"""

__help__ = __usage__ + """
	makeInstances builds UFO font instances from a set of master design UFO fonts.
	It uses the same font math library as the LettError Superpolator program.
	The  paths of the master and instances are specified in "design space" XML file.
	
"""

import sys
import os
import re
import time
import traceback
import FDKUtils
from subprocess import PIPE, Popen
from mutatorMath.ufo import build as mutatorMathBuild
import robofab.world

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET
XML = ET.XML
xmlToString = ET.tostring

kFontName = "PSFontName"

kDefaultDesignSpaceFileName = "font.designspace"


class OptError(ValueError):
	pass
class ParseError(ValueError):
	pass
class SnapShotError(ValueError):
	pass

class Options:
	def __init__(self, args):
		self.dsPath = kDefaultDesignSpaceFileName
		self.doAutoHint = 1
		self.doOverlapRemoval = 1
		self.logFile = None
		self.indexList = []
		lenArgs = len(args)
		
		i = 0
		hadError = 0
		while i < lenArgs:
			arg = args[i]
			i += 1
			
			if arg == "-h":
				logMsg.log(__help__)
				sys.exit(0)
			elif arg == "-u":
				logMsg.log(__usage__)
				sys.exit(0)
			elif arg == "-d":
				self.dsPath = args[i]
				i +=1
			elif arg == "-log":
				logMsg.sendTo( args[i])
				i +=1
			elif arg == "-a":
				self.doAutoHint = 0
			elif arg == "-c":
				self.doOverlapRemoval = 0
			elif arg == "-i":
				ilist = args[i]
				i +=1
				self.indexList =eval(ilist)
				if type(self.indexList) == type(0):
					self.indexList = (self.indexList,)
			else:
				logMsg.log("Error: unrcognized argument:", arg)
				hadError = 1
	
		if hadError:
			raise(OptError)

		
		if not os.path.exists(self.dsPath):
			logMsg.log("Note: could not find design space file path:", self.dsPath)
			hadError = 1
			
		if hadError:
			raise(OptError)


class LogMsg:
	def __init__(self, logFilePath = None):
		self.logFilePath = logFilePath
		self.logFile = None
		if  self.logFilePath:
			self.logFile = open(logFilePath, "wt")

	def log(self, *args):
		txt = " ".join(args)
		print txt
		if  self.logFilePath:
			self.logFile.write(txt + os.linesep)
			self.logFile.flush()
			
	def sendTo(self, logFilePath):
		self.logFilePath = logFilePath
		self.logFile = open(logFilePath, "wt")

logMsg = LogMsg()


def readDesignSpaceFile(options):
	""" Read design space file.
	build a new instancesList with all the instances from the ds file
	
	Promote all the source and instance filename attributes from relative to absolute paths
	write a temporary ds file
	return a path to the temporary ds file, and the current instances list.
	"""
	
	instanceEntryList = []
	print "Reading design space file: %s ..." % (options.dsPath)
	
	f = open(options.dsPath, "rt")
	data = f.read()
	f.close()
	ds = XML(data)
	
	instances = ds.find("instances")
	
	# Remove any instances that are not in the specified list of instance indices, from the option -i.
	if (len(options.indexList) > 0):
		newInstanceXMLList = instances.findall("instance")
		numInstances =  len(newInstanceXMLList)
		instanceIndex = numInstances
		while instanceIndex > 0:
			instanceIndex -= 1
			instanceXML = newInstanceXMLList[instanceIndex]
			if not (instanceIndex in options.indexList):
				instances.remove(instanceXML)

	# We want to build all remaining instances. 
	for instanceXML in instances:
		try:
			curPath = instanceXML.attrib["filename"]
			psName = instanceXML.attrib["postscriptfontname"]
			print "adding %s to build list." % (psName)
			instanceEntryList.append(curPath)
		except KeyError:
			print "Skipping instance that does not have postscriptname attribute:", xmlToString(instanceXML)
			continue
	if not instanceEntryList:
		print "Failed to find any instances in the ds file '%s' that have the postscriptfilename attribute" % (options.dsPath)
		return None, None
				
	dsPath = options.dsPath + ".temp"
	data = xmlToString(ds)
	f = open(dsPath, "wt")
	f.write(data)
	f.close()
	
	return dsPath, instanceEntryList
	


def updateInstance(options, fontInstancePath):
	"""
	Run checkOutlinesUFO and autohint, unless explicitly suppressed.
	"""
	if options.doOverlapRemoval:
		logMsg.log("\tdoing overlap removal with checkOutlinesUFO %s ..." % (fontInstancePath))
		logList = []
		if os.name == "nt":
			proc = Popen(['checkOutlinesUFO.cmd', "-e", fontInstancePath], stdout=PIPE)
		else:
			proc = Popen(['checkOutlinesUFO', "-e", fontInstancePath], stdout=PIPE)
		lastLine = ""
		while 1:
			output = proc.stdout.readline()
			if output:
				sys.stdout.write(".")
				logList.append(output)
				lastLine = output
			if proc.poll() != None:
				output = proc.stdout.readline()
				if output:
					print output,
					logList.append(output)
					lastLine = output
				break
		log = "".join(logList)
		if not ("Done with font" in log):
			print
			logMsg.log("Error in checkOutlinesUFO %s" % (fontInstancePath))
			logMsg.log(log)
			raise(SnapShotError)
		else:
			print
			
	if options.doAutoHint:
		logMsg.log("\tautohinting %s ..." % (fontInstancePath))
		logList = []
		if os.name == "nt":
			proc = Popen(['autohint.cmd','-q', '-nb', fontInstancePath], stdout=PIPE)
		else:
			proc = Popen(['autohint', '-q','-nb',fontInstancePath], stdout=PIPE)
		lastLine = ""
		while 1:
			output = proc.stdout.readline()
			if output:
				sys.stdout.write(".")
				logList.append(output)
				lastLine = output
			if proc.poll() != None:
				output = proc.stdout.readline()
				if output:
					print output,
					logList.append(output)
					lastLine = output
				break
		log = "".join(logList)
		if not ("Done with font" in log):
			print
			logMsg.log("Error in autohinting %s" % (fontInstancePath))
			logMsg.log(log)
			raise(SnapShotError)
		else:
			print

	return
	
		

def run(args):
	options = Options(args)

	dsPath, newInstancesList = readDesignSpaceFile(options)
	if not dsPath:
		return
	version = 2
	if len(newInstancesList) == 1:
		logMsg.log( "Building 1 instance..")
	else:
		logMsg.log("Building %s instances.." % (len(newInstancesList)))
	mutatorMathBuild(documentPath=dsPath, outputUFOFormatVersion=version)
	if (dsPath != options.dsPath) and os.path.exists(dsPath):
		os.remove(dsPath)

	logMsg.log("Built %s instances.." % (len(newInstancesList)))
	if options.doAutoHint or options.doOverlapRemoval:
		logMsg.log( "Applying post-processing")
		# Apply autohint and checkoutlines, if requested.
		for instancePath in newInstancesList:
			# make new instance font.
			updateInstance(options, instancePath)


if __name__=='__main__':
	try:
		run(sys.argv[1:])
	except (OptError, ParseError):
		logMsg.log("Quitting after error.")
		pass
	
	
