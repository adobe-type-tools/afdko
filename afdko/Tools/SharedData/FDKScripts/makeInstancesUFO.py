#!/bin/env python

__copyright__ = """Copyright 2015 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
"""
__usage__ = """
   makeInstancesUFO program v1.3.2 Jun 12 2018
   makeInstancesUFO -h
   makeInstancesUFO -u
   makeInstancesUFO [-d <design space file name>] [-a] [-c] [-n] [-dec] [-i 0,1,..n]

   -d <design space file path>
   Specifies alternate path to design space file. Default name is 'font.designspace'.

   -a ... do NOT autohint the instances. Default is to do so.
   -c ... do NOT remove the outline overlaps in the instances. Default is to do so.
   -n ... do NOT normalize the instances. Default is to do so.
   -dec . do NOT round coordinates to integer. Default is to do so.

   -i <list of instance indices>
   Specify the instances to generate. 'i' is a 0-based index of the instance records
   in the design space file.
      Example: '-i 1,4,22' -> generates only the 2nd, 5th, and 23rd instances
"""

__help__ = __usage__ + """
	makeInstancesUFO generates UFO font instances from a set of master design UFO fonts.
	It uses the same font math library as the LettError Superpolator program.
	The paths of the masters and instances fonts are specified in "design space" XML file.

"""

import os
import shutil
import sys
from subprocess import PIPE, Popen

import defcon
from mutatorMath.ufo import build as mutatorMathBuild

import ufoTools

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET
XML = ET.XML
xmlToString = ET.tostring

haveUfONormalizer = True
try:
	import ufoLib.ufonormalizer as ufonormalizer
except ImportError:
	try:
		import ufonormalizer
	except ImportError:
		haveUfONormalizer = False
		print "Failed to find UFO normalizer, Pleas re-install the AFDKO."
		sys.exit()

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
		self.doAutoHint = True
		self.doOverlapRemoval = True
		self.doNormalize = True
		self.logFile = None
		self.allowDecimalCoords = False
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
				self.doAutoHint = False
			elif arg == "-c":
				self.doOverlapRemoval = False
			elif arg == "-n":
				self.doNormalize = False
			elif arg in ["-dec", "-decimal"]:
				self.allowDecimalCoords = True
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

		if self.doNormalize and not haveUfONormalizer:
			logMsg.log("Warning: skipping UFO normalization, as I cannot find the ufonormalizer module:", self.dsPath)


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
		familyName = instanceXML.attrib["familyname"]
		styleName = instanceXML.attrib["stylename"]
		curPath = instanceXML.attrib["filename"]
		print "adding %s %s to build list." % (familyName, styleName)
		instanceEntryList.append(curPath)
		if os.path.exists(curPath):
			glyphDir = os.path.join(curPath, "glyphs")
			if os.path.exists(glyphDir):
				shutil.rmtree(glyphDir, ignore_errors=True)
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
		opList = ["-e", fontInstancePath]
		if options.allowDecimalCoords:
			opList.insert(0, "-d")
		if os.name == "nt":
			opList.insert(0, 'checkOutlinesUFO.exe')
			proc = Popen(opList, stdout=PIPE)
		else:
			opList.insert(0, 'checkOutlinesUFO')
			proc = Popen(opList, stdout=PIPE)
		while 1:
			output = proc.stdout.readline()
			if output:
				print ".",
				logList.append(output)
			if proc.poll() != None:
				output = proc.stdout.readline()
				if output:
					print output,
					logList.append(output)
				break
		log = "".join(logList)
		if not ("Done with font" in log):
			print
			logMsg.log(log)
			logMsg.log("Error in checkOutlinesUFO %s" % (fontInstancePath))
			raise(SnapShotError)
		else:
			print

	if options.doAutoHint:
		logMsg.log("\tautohinting %s ..." % (fontInstancePath))
		logList = []
		opList = ['-q', '-nb', fontInstancePath]
		if options.allowDecimalCoords:
			opList.insert(0, "-dec")
		if os.name == "nt":
			opList.insert(0, 'autohint.exe')
			proc = Popen(opList, stdout=PIPE)
		else:
			opList.insert(0, 'autohint')
			proc = Popen(opList, stdout=PIPE)
		while 1:
			output = proc.stdout.readline()
			if output:
				print output,
				logList.append(output)
			if proc.poll() != None:
				output = proc.stdout.readline()
				if output:
					print output,
					logList.append(output)
				break
		log = "".join(logList)
		if not ("Done with font" in log):
			print
			logMsg.log(log)
			logMsg.log("Error in autohinting %s" % (fontInstancePath))
			raise(SnapShotError)
		else:
			print

	return

def clearCustomLibs(dFont):
	for key in dFont.lib.keys():
		if key not in ['public.glyphOrder', 'public.postscriptNames']:
			del(dFont.lib[key])

	libGlyphs = [g for g in dFont if len(g.lib)]
	for g in libGlyphs:
		g.lib.clear()

def roundSelectedFontInfo(fontInfo):
		"""self.roundGeometry is false. However, most FontInfo fields have to be integer.
		Exceptions are:
			any of the PostScript Blue values.
			postscriptStemSnapH, postscriptStemSnapV, postscriptSlantAngle
		The Blue values should be rounded to 2 decimal places, with the exception
		of postscriptBlueScale.
		I round the float values because most Type1/Type2 rasterizers store
		point and stem coordinates as Fixed numbers with 8 bits. You end up
		cumulative rounding errors if you pass in relative values with more
		than 2 decimal places. Other FontInfo float values are stored as Fixed
		number with 16 bits,and can support 6 decimal places.
		"""
		listType = type([])
		strType = type("")
		realType = type(0.1)

		fiKeys = dir(fontInfo)
		for fieldName in fiKeys:
			if fieldName.startswith("_"):
				continue
			srcValue = getattr(fontInfo, fieldName)
			if type(srcValue) != listType:
				try:
					realValue = float(srcValue)
				except (TypeError, ValueError):
					continue
				intValue = int(realValue)
				if intValue == realValue:
					if type(srcValue) == realType:
						# Convert to int value. I do the following because setattr() will actually set the value only
						# if it compares differently, and (intValue == realValue), so we first
						# have to set it to a different value.
						setattr(fontInfo, fieldName, intValue-1)
						setattr(fontInfo, fieldName, intValue)
					continue

				if (fieldName.startswith("postscript") and ("Blue" in fieldName)) or (fieldName == "postscriptSlantAngle"):
					if fieldName == "postscriptBlueScale":
						# round to 6 places
						rndValue = round(realValue*1000000)/1000000.0
						if rndValue != realValue:
							setattr(fontInfo, fieldName, rndValue)
					else:
						# round to 2 places
						rndValue = round(realValue*100)/100.0
						if rndValue != realValue:
							setattr(fontInfo, fieldName, rndValue)
				else:
					intValue = int(round(realValue))
					setattr(fontInfo, fieldName, intValue)

			elif (fieldName.startswith("postscript") and ("Blue" in fieldName)) or fieldName.startswith("postscriptStemSnap"):
				# It is a list.
				for i in range(len(srcValue)):
					realValue = float(srcValue[i])
					intVal = int(realValue)
					if intValue == realValue:
						continue
					# round to 2 places
					rndValue = round(realValue*100)/100.0
					if rndValue != realValue:
						srcValue[i] = rndValue

def roundSelectedValues(dFont):
	""" Glyph widths, kern values, and selected FontInfo values must be rounded,
	as these are stored in the final OTF as ints.
	"""
	roundSelectedFontInfo(dFont.info)

	# round widths.
	for dGlyph in dFont:
		rval = dGlyph.width
		ival = int(round(rval))
		if ival != rval:
			dGlyph.width = ival

	# round kern values, if any.
	if dFont.kerning:
		keys = dFont.kerning.keys()
		for key in keys:
			rval = dFont.kerning[key]
			ival = int(round(rval))
			if ival != rval:
				dFont.kerning[key] = ival

def postProcessInstance(fontPath, options):
	dFont = defcon.Font(fontPath)
	clearCustomLibs(dFont)
	if options.allowDecimalCoords:
		roundSelectedValues(dFont)
	dFont.save()

def run(args):
	options = Options(args)

	# Set the current dir to the design space dir, so that relative paths in
	# the design space file will work.
	dsDir,dsFile = os.path.split(os.path.abspath(options.dsPath))
	os.chdir(dsDir)
	options.dsPath = dsFile

	dsPath, newInstancesList = readDesignSpaceFile(options)
	if not dsPath:
		return

	version = 2
	if len(newInstancesList) == 1:
		logMsg.log("Building 1 instance...")
	else:
		logMsg.log("Building %s instances..." % (len(newInstancesList)))
	mutatorMathBuild(documentPath=dsPath, outputUFOFormatVersion=version, roundGeometry=(not options.allowDecimalCoords))
	if (dsPath != options.dsPath) and os.path.exists(dsPath):
		os.remove(dsPath)

	logMsg.log("Built %s instances." % (len(newInstancesList)))
	# Remove glyph.lib and font.lib (except for "public.glyphOrder")
	for instancePath in newInstancesList:
		postProcessInstance(instancePath, options)

	if options.doNormalize and haveUfONormalizer:
		logMsg.log("Applying UFO normalization...")
		for instancePath in newInstancesList:
				ufonormalizer.normalizeUFO(instancePath, outputPath=None, onlyModified=True, writeModTimes=False)

	if options.doAutoHint or options.doOverlapRemoval:
		logMsg.log("Applying post-processing...")
		# Apply autohint and checkoutlines, if requested.
		for instancePath in newInstancesList:
			# make new instance font.
			updateInstance(options, instancePath)

	if not options.doOverlapRemoval: # checkOutlinesUFO does ufoTools.validateLayers()
		for instancePath in newInstancesList:
			# make sure that that there are no old glyphs left in the processed glyphs folder.
			ufoTools.validateLayers(instancePath)

	if options.doOverlapRemoval or options.doAutoHint: # The defcon library renames glyphs. Need to fix them again.
		for instancePath in newInstancesList:
			if haveUfONormalizer and options.doNormalize:
				ufonormalizer.normalizeUFO(instancePath, outputPath=None, onlyModified=False, writeModTimes=False)

def main():
	try:
		run(sys.argv[1:])
	except (OptError, ParseError, SnapShotError):
		logMsg.log("Quitting after error.")
		pass

if __name__=='__main__':
	main()

