#!/bin/env python

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
"""
__usage__ = """
   makeInstancesUFO program v1.02 Sep 18 2014
   makeInstancesUFO -h
   makeInstancesUFO -u
   makeInstancesUFO [-a] [-f <instance file path>]  [-d <design space file name>]
                 [-c] [-a] [-i ,i0,i1..,in]

   -f <instance file path> .... Specifies alternate path to instance specification 
                                file. Default: instanceList.txt

   -f <design space file path> .... Specifies alternate path to design space file.
                                Default: font.designspace

   -i <list of instance indices> Build only the listed instances, i is a  0-based index of the instance records in the instance file.
   -a ... do NOT autohint the instance font files. Default is to do so, as master designs are unhinted.
   -c ... do NOT run checkOutlines to remove overlaps from the instance font files. Default is to do so.
"""

__help__ = __usage__ + """
	makeInstances builds UFO font instances from a set of master design UFO fonts.
	It uses the same font math library as the LettError Superpolator program.
	The  paths of the master and instances are specified in "design space" XML file.
	
	An optional separate "instanceslist.txt file, in the same directory, provides overriding values
	for the Type1 FontDict. Commonly overridden values are the global hint values: 
   - BlueValues
   - OtherBlues
   - FamilyBlues
   - FamilyOtherBlues
   - StdHW
   - StdVW
   - StemSnapH
   - StemSnapV
	This is useful when instance fonts have been edited to have individual
	global alignment zones and stems. When this has been done, the results are 
	better than what you get by using the interpolated values from the master
	design fonts.

	Other T12 FontDict values that can be overridden are:
		FullName
		SlantAngle
		UniqueID
		UnderlineThickness
		UnderlinePosition
		IsFixedPitch
		Weight
		
	'FontName' cannot be overridden, as this is specified as the 'postscriptFontName' attribute
	in the design space file.
	
	The instanceList.txt file is a flat text file. Each line represents a record
	describing a single instance. Each record consists of a series of
	tab-delimited fields. The first field must always be "PSFontName". This
	field provides the Postscript FontName for the instance. If only this field
	is used, then there is no need for a header line. However, if any additional
	fields are used, then the file must contain a line starting with
	'PSFontName', and continuing with tab-delimited field names. The logic
	which processes the header line will assume that a first non-comment line
	that starts with 'PSFontName' is a header line that provides the keys for the 
	fields in all following lines.

	Any additional field names are assumed to be the names for Postscript
	FontDict keys. The script will modify the values in the instance's
	PostScript FontDict for the keys that have these names. These must be
	formatted exactly as they are in the detype1 output.
   
   Example 1:
PSFontName<tab>Weight<tab>BlueValues
<ArnoPro-Regular<tab>Regular<tab>{-18,0,400,414,439,453,584,596,603,621,653,664}
<ArnoPro-Bold<tab>Bold<tab>{-18,0,395,410,439,453,596,608,615,633,672,678}
"""

import sys
import os
import re
import time
import traceback
import FDKUtils
from subprocess import PIPE, Popen
from mutatorMath import build as mutatorMathBuild
import robofab.world

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET
XML = ET.XML
xmlToString = ET.tostring

kFontName = "PSFontName"

kDefaultInstanceFilePath = "instanceList.txt"
kDefaultDesignSpaceFileName = "font.designspace"

kIsBoldKey = "IsBold"
kForceBold = "ForceBold"
kIsItalicKey = "IsItalic"

kPSFontDictKeys = {
	"FontBBox": None,
	"Weight": None,
 	"BlueValues": None,
 	"OtherBlues": None,
 	"FamilyBlues": None,
 	"FamilyOtherBlues": None,
 	 "ForceBold": None,
 	"StdHW": None,
 	"StdVW": None,
 	"StemSnapH": None,
 	"StemSnapV": None,
 	"BlueScale": None,
 	"BlueShift": None,
 	"BlueFuzz": None,
}
T1FontHint2RFont = {
	"FontBBox": "postscriptFontBBox",
 	"BlueValues": "blueValues",
 	"OtherBlues": "otherBlues",
 	"FamilyBlues": "familyBlues",
 	"FamilyOtherBlues": "familyOtherBlues",
 	 "ForceBold": "forceBold",
 	"StdHW": "postscriptStdHW",
 	"StdVW": "postscriptStdVW",
 	"StemSnapH": "hStems",
 	"StemSnapV": "vStems",
 	"BlueScale": "blueScale",
 	"BlueShift": "blueShift",
 	"BlueFuzz": "blueFuzz",
}

T1FontDict2RFont = {
	"FontName": "postscriptFontName",
	"FullName": "postscriptFullName",
	"SlantAngle": "postscriptSlantAngle",
	"UniqueID": "postscriptUniqueID",
	"UnderlineThickness": "postscriptUnderlineThickness",
	"UnderlinePosition": "postscriptUnderlinePosition",
	"IsFixedPitch": "postscriptIsFixedPitch",
	"Weight": "postscriptWeightName",
}

class OptError(ValueError):
	pass
class ParseError(ValueError):
	pass
class SnapShotError(ValueError):
	pass

class Options:
	def __init__(self, args):
		self.instancePath = kDefaultInstanceFilePath
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
			elif arg == "-f":
				self.instancePath = args[i]
				i +=1
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

		
		if not os.path.exists(self.instancePath):
			logMsg.log("Note: could not find instance file path:", self.instancePath)
			
		if not os.path.exists(self.dsPath):
			logMsg.log("Note: could not find design space file path:", self.dsPath)
			hadError = 1
			
		#print self.instancePath
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


def matchesInstanceList(instanceXML, instancesList):
	matches = False
	psName = instanceXML.attrib["postscriptfontname"]
	filteredList = filter(lambda entry: entry[kFontName] == ("/"+psName), instancesList)
	if filteredList:
		instanceDict = filteredList[0]
	else:
		instanceDict = None
	return instanceDict
	
def readDesignSpaceFile(options, instancesList):
	""" Read design space file. If instancesList is empty:
		build a new instancesList with all the instances from the ds file
	else:
	  filter the design space instances element by the list of instances in instancesList
	
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
	
	newInstanceXMLList = instances.findall("instance")
	if instancesList:
		# Remove any instances that are not in instancesList.
		newInstanceXMLList.reverse()
		for instanceXML in newInstanceXMLList:
			try:
				matchInstanceDict =  matchesInstanceList(instanceXML, instancesList)
				if matchInstanceDict == None:
					instances.remove(instanceXML)
				else:
					curPath = os.path.abspath(instanceXML.attrib["filename"])
					instanceXML.attrib["filename"] = curPath
					instanceEntryList.append([curPath, matchInstanceDict])
			except KeyError:
				print "Removing instance that does not have postscriptname attribute:", xmlToString(instanceXML)
				instances.remove(instanceXML)
	else:
		for instanceXML in newInstanceXMLList:
			try:
				curPath = os.path.abspath(instanceXML.attrib["filename"])
				instanceXML.attrib["filename"] = curPath
				psName = instanceXML.attrib["postscriptfontname"]
				instanceEntryList.append([curPath, {kFontName:psName}])
			except KeyError:
				print "Skipping instance that does not have postscriptname attribute:", xmlToString(instanceXML)
				continue
			
	if not instanceEntryList:
		print "Failed to find any instances in the ds file '%s' that have the postscriptfilename attribute" % (options.dsPath)
		return None, None
		
	# Now update file name attribute for the master designs.
	sources = ds.find("sources")
	newXMLList = sources.findall("source")
	for sourceXML in newXMLList:
		curPath = sourceXML.attrib["filename"]
		sourceXML.attrib["filename"] = os.path.abspath(curPath)
		
	dsPath = options.dsPath + ".temp"
	data = xmlToString(ds)
	f = open(dsPath, "wt")
	f.write(data)
	f.close()
	
	return dsPath, instanceEntryList
	
def readInstanceFile(options):

	if not os.path.exists(options.instancePath):
		return None
		
	
	print "Reading instance list file: %s ..." % (options.instancePath)
	f = open(options.instancePath, "rt")
	data = f.read()
	f.close()
	
	lines = data.splitlines()
	i = 0
	parseError = 0
	keyDict = {}
	numLines = len(lines)
	instancesList = []
	
	for i in range(numLines):
		line = lines[i]
		
		# Get rid of all comments. 
		commentIndex = line.find('#')
		if commentIndex >= 0:
			line = re.sub(r"#.+", "", line)
		# skip blank lines.
		line = line.strip()
		if len(line) == 0:
			continue
		if line.startswith(kFontName):
			if len(keyDict) > 0:
				logMsg.log("Error. Field header line must preceed an instance definition line")
				raise ParseError
					
			# parse the line with the field names.
			keys = line.split('\t')
			keys = map(lambda name: name.strip(), keys)
			numKeys = len(keys)
			k = 0
			while k < numKeys:
				keyDict[k] = keys[k]
				k +=1
			continue
		elif len(keyDict) == 0:
			keyDict = {0:kFontName}
			numKeys	= len(keyDict)
		# Must be an instance definition line.
			
		fields = line.split('\t')
		fields = map(lambda datum: datum.strip(), fields)
		numFields = len(fields)
		if (numFields != numKeys):
			logMsg.log("Warning: at line %s, the number of fields %s do not match the number of key names %s for %s." % (i, numFields, numKeys, fields[0]))
		
		instanceDict= {}
		#Build a dict from key to value. Some kinds of values needs special processing.
		for k in range(numFields):
			key = keyDict[k]
			field = fields[k]
			if (not field) or (field == ""): # A field may be empty, or an empty string.
				continue
			if key == kFontName:
				value = "/%s" % field

			elif key in [kIsBoldKey, kIsItalicKey]:
				value = eval(field) # this works for both fields.
				
				if key == kIsBoldKey: # need to convert to Type 1 field key.
					if value == 1:
						value = "true"
					else:
						value = "false"
				elif key == kIsItalicKey:
					if value == 1:
						value = "true"
					else:
						value = "false"
					
			elif field[0] in ["[","{"]: # it is a Type 1 array value. Pass as is, as a string.
				value = field
			else:
				# either a single number or a string.
				if re.match(r"^[-.\d]+$", field):
					value = field #it is a Type 1 number. Pass as is, as a string.
				else:
					# Wasn't a number. Format as Type 1 string, i.e add parens around it.
					if field[0] != "(":
						value = "(%s)" % (field)
			instanceDict[key] = value
		instancesList.append(instanceDict)
		
	if parseError:
		raise(ParseError)
		
	if options.indexList != None:
		newInstanceList = []
		for i in options.indexList:
			try:
				newInstanceList.append(instancesList[i])
			except IndexError:
				print "Error. instance index '%s' is out of range for the number of instances in '%s." % (i, options.instancePath)
				raise OptError()
		instancesList = newInstanceList
	return instancesList


def updateFontDictValues(instanceDict, fontInstancePath):
	rPSDict = {} # Build a dict of Robofab font PS hint keys and values.
	rfDict = {} # TopDict values for Robofab.
	changed = 0
	rFont = robofab.world.OpenFont(fontInstancePath)
	for key, value in instanceDict.items():
		if key == "PSFontName":
			continue
			
		if T1FontHint2RFont.has_key(key):
			rKey = T1FontHint2RFont[key]
			updatePSFontHint(rPSDict, rKey, value)
			changed = 1
			continue

		try:
			rKey = T1FontDict2RFont[key]
			newValue = formatFontValue(value)
			exec("rFont.info.%s = '%s'" % (rKey, newValue))
			changed = 1
		except KeyError:
			print "Warning: FontDict key '%s' not supported by this script." % (key)
			continue
	
	if changed:	
		rFont.psHints.fromDict(rPSDict)
		rFont.save()

def updatePSFontHint(rPSDict, rKey, value):
		if (value[0] in ["[", "{"]) and (value[-1] in ["]", "}"]):
			# It is a Type1 value list.
			listValue = value[1:-1]
			listValue = eval(listValue)
			if (rKey == "blueValues") or rKey.endswith("Blues"):
				# convert to list of pairs.
				pairValue = []
				while listValue:
					pairValue.append(listValue[:2])
					listValue = listValue[2:]
				rPSDict[rKey] = pairValue
		else:
			try:
				val = float(value)
				intVal = int(value)
				if (val - intVal) != 0:
					rPSDict[rKey] = intVal
				else:
					rPSDict[rKey] = val
			except ValueError:
				# treat as string.
				rPSDict[rKey] = value

def formatFontValue(value):
	if (value[0] in ["[", "{"]) and (value[0] in ["]", "}"]):
		listValue = value[1:-1]
		listValue = eval(listValue)
		newValue = listValue
	else:
		try:
			val = float(value)
			intVal = int(value)
			if (val - intVal) != 0:
				newValue = intVal
			else:
				newValue = val
		except ValueError:
			# treat as string.
			if (value[0] == "(") and (value[-1] == ")"): # It is formatted as a Type1 string. Remove braces.
				value = value[1:-1]
			newValue = value
	return newValue

def updateInstance(instanceDict, options, fontInstancePath):
	"""Update instance with Type1 FOntDict values
	Run checkOUtlines and autohint, unless explicitly suppressed.
	"""
	if len(instanceDict) > 1: # If there is more than the PostScript name, there are FontDict values to add.
			logMsg.log("\tUpdating instance with FontDict values: %s ..." % (fontInstancePath))
			updateFontDictValues(instanceDict, fontInstancePath)
		
	if options.doAutoHint or options.doOverlapRemoval:
		if options.doOverlapRemoval:
			logMsg.log("\tdoing overlap removal with checkOutlines %s ..." % (fontInstancePath))
			logList = []
			if os.name == "nt":
				proc = Popen(['checkOutlines.cmd','-I', "-O", "-e", fontInstancePath], stdout=PIPE)
			else:
				proc = Popen(['checkOutlines','-I', "-O", "-e", fontInstancePath], stdout=PIPE)
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
			if not (("Writing changed file" in log) or ("Done with font" in log)):
				print
				logMsg.log("Error in checkOutlines %s" % (fontInstancePath))
				logMsg.log(log)
				raise(SnapShotError)
			else:
				print
				print lastLine # print where log file went
				
		if options.doAutoHint:
			logMsg.log("\tautohinting %s ..." % (fontInstancePath))
			logList = []
			if os.name == "nt":
				proc = Popen(['autohint.cmd','-q', '-nb', fontInstancePath], stdout=PIPE)
			else:
				proc = Popen(['autohint','-q','-nb',fontInstancePath], stdout=PIPE)
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
			if not "Done with font" in log:
				print
				logMsg.log("Error in autohinting %s" % (fontInstancePath))
				logMsg.log(log)
				raise(SnapShotError)
			else:
				print
				print lastLine # print where log file went

	logMsg.log("\tDone generating instance:", fontInstancePath)

	return
	
		

def run(args):
	options = Options(args)
	instancesList = readInstanceFile(options)

	dsPath, newInstancesList = readDesignSpaceFile(options, instancesList)
	if not dsPath:
		return
	version = 2
	if len(newInstancesList) == 1:
		print "Building 1 instance.."
	else:
		print "Building %s instances.." % (len(newInstancesList))
	mutatorMathBuild(documentPath=dsPath, outputUFOFormatVersion=version)
	if (dsPath != options.dsPath) and os.path.exists(dsPath):
		os.remove(dsPath)

	# Update instance fonts with data from the instanceList.txt file. Apply autohint and checkoutlines, if requested.
	for instancePath, instanceDict in newInstancesList:
		# make new instance font.
		psName = instanceDict[kFontName]
		updateInstance(instanceDict, options, instancePath)


if __name__=='__main__':
	try:
		run(sys.argv[1:])
	except (OptError, ParseError):
		logMsg.log("Quitting after error.")
		pass
	
	
