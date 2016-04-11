#!/bin/env python

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
"""
__usage__ = """
   makeInstances program v1.32 March 17 2016
   makeInstances -h
   makeInstances -u
   makeInstances [-a] [-f <instance file path>] [-m <MM font path>] 
                 [-o <instance parent directory>] [-d <instance file name>]
                 [-uf] [-ui] [-a]

   -f <instance file path> .... Specifies alternate path to instance specification 
                                file. Default: instances
   -m <MM font path> .......... Specified alternate path to source MM Type1 font 
                                file. Default: mmfont.pfa
   -o <instance parent dir> ... Specifies alternate parent directory for instance 
                                directories. Default: parent dir of instances file
   -d <instance font file name> .... Specifies alternate name for instance font file. 
                                Default is font.pfa.

   -uf ... Use pre-existing font.pfa file's FontDict values for new instance font.
   -ui ... As '-uf', but also updates the instance file with the original font.pfa 
           values.
   -i <list of instance indices> Build only the listed instances, as 0-based instance index in the isntance file.
   -a ... do NOT autohint the instance font files. Default is to do so, as IS strips out all hints.
   -c ... do NOT run checkOutlines to remove overlaps from the instance font files. Default is to do so.
"""

__help__ = __usage__ + """
	makeInstances builds font.pfa instances from a multiple master font file.
	The tool is needed because FontLab does not do intelligent scaling when
	making MM instances. When run, it will create all the instances named in the
	instance file.

	makeInstances requires two input files: the MM font file, and the instances
	file. The instances file provides the information needed for each instance.
	By default, it is assumed to be in the current directory, and to be named
	'instances'. Another path may be specified with the option '-f'.

	The MM font file is assumed to be in the same directory as the instances
	file, and to be named "mmfont.pfa". Another path may be specified with the
	option '-m'.

	By default, the instances are created in sub-directories of the directory
	that contains the instances file. A different parent directory may be
	specified with the option '-o'.

	By default, the name of the instance font files is "font.pfa". This can be
	overridden with the '-d' option.

	The sub-directory names are derived by taking the portion of the font's
	PostScript name after the hyphen. If there is no hyphen, the "Regular" is
	used.

	The global values that are taken from the original font.pfa file with the
	'-ui' and '-uf' options are applied are:
   - FontBBox
   - BlueValues
   - OtherBlues
   - FamilyBlues
   - FamilyOtherBlues
   - StdHW
   - StdVW
   - StemSnapH
   - StemSnapV
	The '-ui' and -uf' options are useful when the previous instance fonts have
	been edited to have individual global alignment zones and stems. When this
	has been done, the results are much better than what you get by using the
	interpolated values from the MM font. In order to avoid losing the
	individuak instance values whenever the instances are regenerated from the
	parent MM font, it is useful to use the -ui and -uf options to get these
	values from the previous instance font, and optionally to store them in the
	instances file.

	The instance file is a flat text file. Each line represents a record
	describing a single instance. Each record consists of a series of
	tab-delimited fields. The first 6 fields are always, order:

   FamilyName  : This is the Preferred Family name.
   FontName    : This is the PostScript name.
   FullName    : This is the Preferred Full name.
   Weight      : This is the Weight name.
   Coords      : This is a single value, or a list of comma-separated integer values.
                Each integer value corresponds to an axis.
   IsBold      : This must be either 1 (True) or 0 (False). This will be translated into
                Postscript FontDict  ForceBold field with a value of
                'true' for instances where then font is bold, and will be
                copied to the fontinfo file
   
	If only these six fields are used, then there is no need for a header line.
	However, if any additional fields are used, then the file must contain a
	line starting with "#KEYS:", and continuing with tab-delimited field names.

	ExceptionSuffixes : A list of suffixes, used to identify MM exception
	glyphs. An MM exception glyph is one which is designed for use in only one
	instance, and is used by replacing every occurence of the glyphs that match
	the MM exception glyph's base name. The  MM exception glyph is written to no
	other instance. This allows the developer to fix problems with just a few
	glyphs in each instance. For example, if the record for HypatiaSansPro-Black
	in the 'instances' file specifies an ExceptionSuffix suffix list which
	contains the suffix ".black", and there is an MM exception glyph named
	"a.black", then the glyph "a" will be replaced by the glyph "a.black", and
	all composite glyphs that use "a" will be updated to use the contours from
	"a.black" instead.

	NOTE! In order for the MM Exception glyphs to be applied to what where
	originally composite glyphs in the MM FontLab font file, makeInstances
	requires a composite glyph data file. You  must generate this before running
	makeInstances. It is generated by the Adobe script for FontLab, "Instance
	Generator".

	ExtraGlyphs : A list of working glyph names, to be omitted from the
	instances. The may be a complete glyph name, or a wild-card pattern. A
	pattern may take two forms: "*<suffix>", which will  match any glyph ending
	with that suffix, or a regular expression which must match entire glyph
	names. The pattern must begin with "^" and ends with "$". You do not need to
	include glyph names which match an MM Exception glyph suffix: such glyphs
	will not be written to any instance.

	Any additional field names are assumed to be the names for Postscript
	FontDict keys. The script will modify the values in the instance's
	PostScript FontDict for the keys that have these names. The additional
	Postscript keys and  values must be formatted exactly as they are in the
	detype1 output.
   
   Example 1:
#KEYS:<tab>FamilyName<tab>FontName<tab>FullName<tab>Weight<tab>Coords<tab>IsBold<tab>FontBBox<tab>BlueValues
ArnoPro<tab><ArnoPro-Regular<tab>Arno Pro Regular<tab>Regular<tab>160,451<tab>0<tab>{-147 -376 1511 871}<tab>{[-18 0 395 410 439 453 596 608 615 633 672 678]
ArnoPro<tab><ArnoPro-Bold<tab>Arno Pro Bold<tab>Bold<tab>1000,451<tab>1<tab>{-148 -380 1515 880} <tab> [-18 0 400 414 439 453 584 596 603 621 653 664]

   Example 2:
#KEYS:<tab>FamilyName<tab>FontName(PSName)<tab>FullName<tab>Weight<tab>AxisValues<tab>isBold<tab>ExceptionSuffixes<tab>ExtraGlyphs
HypatiaSansPro<tab>HypatiaSansPro-ExtraLight<tab>Hypatia Sans Pro ExtraLight<tab>ExtraLight<tab>0<tab>0<tab><tab>
HypatiaSansPro<tab>HypatiaSansPro-Black<tab>Hypatia Sans Pro Black<tab>Black<tab>1000<tab>0<tab>[".black", "black"]<tab>['^serif.+$','^[Dd]escender.*$']

"""



import sys
import os
import re
import time
import copy
import copy
import traceback
import FDKUtils
from subprocess import PIPE, Popen

kInstanceName = "font.pfa"
kCompositeDataName = "temp.composite.dat"

kFieldsKey = "#KEYS:"
kFamilyName = "FamilyName"
kFontName = "FontName"
kFullName = "FullName"
kWeight = "Weight"
kCoordsKey = "Coords"
kIsBoldKey = "IsBold"
kForceBold = "ForceBold"
kIsItalicKey = "IsItalic"
kDefaultValue = "Default"
kExceptionSuffixes = "ExceptionSuffixes"
kExtraGlyphs = "ExtraGlyphs"

kFixedFieldKeys = {
		# field index: key name
		0:kFamilyName,
		1:kFontName,
		2:kFullName,
		3:kWeight,
		4:kCoordsKey,
		5:kIsBoldKey,
		}
kNumFixedFields = len(kFixedFieldKeys)
kDefaultInstancePath = "instances"
kDefaultMMFontPath = "mmfont.pfa"

kNonPostScriptKeys = [kCoordsKey, kIsBoldKey, kIsItalicKey, kExtraGlyphs, kExceptionSuffixes]
kPSNameKeys = [kFullName, kFamilyName, kForceBold]
kPSFontDictKeys = {
	"FontBBox": None,
 	"BlueValues": None,
 	"OtherBlues": None,
 	"FamilyBlues": None,
 	"FamilyOtherBlues": None,
 	kForceBold: None,
 	"StdHW": None,
 	"StdVW": None,
 	"StemSnapH": None,
 	"StemSnapV": None,
 	"BlueScale": None,
 	"BlueShift": None,
 	"BlueFuzz": None,
}
class OptError(ValueError):
	pass
class ParseError(ValueError):
	pass
class SnapShotError(ValueError):
	pass

class Options:
	def __init__(self, args):
		self.instancePath = kDefaultInstancePath
		self.mmFontPath = kDefaultMMFontPath
		self.instancePath = None
		self.mmFontPath = None
		self.parentDir = None
		self.InstanceFontName = None
		self.useExisting = 0
		self.updateInstances = 0
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
			elif arg == "-m":
				self.mmFontPath = args[i]
				i +=1
			elif arg == "-o":
				self.parentDir = args[i]
				i +=1
			elif arg == "-log":
				logMsg.sendTo( args[i])
				i +=1
			elif arg == "-d":
				self.InstanceFontName = args[i]
				i +=1
			elif arg == "-uf":
				self.useExisting = 1
			elif arg == "-a":
				self.doAutoHint = 0
			elif arg == "-c":
				self.doOverlapRemoval = 0
			elif arg == "-ui":
				self.useExisting = 1
				self.updateInstances = 1
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

		if self.instancePath == None:
			self.instancePath = kDefaultInstancePath
			
		if self.InstanceFontName == None:
			self.InstanceFontName = kInstanceName
			
		if self.mmFontPath == None:
			self.mmFontPath = os.path.dirname(self.instancePath)
			self.mmFontPath = os.path.join(self.mmFontPath, kDefaultMMFontPath)
	
		if self.parentDir == None:
			self.parentDir = os.path.dirname(self.instancePath)
		
		if not os.path.exists(self.instancePath):
			logMsg.log("Error: could not find instance file path:", self.instancePath)
			hadError = 1
			
		if not os.path.exists(self.mmFontPath):
			logMsg.log("Error: could not find MM font file path:", self.mmFontPath)
			hadError = 1
	
		if self.parentDir and not os.path.exists(self.parentDir):
			logMsg.log("Error: could not find parent directory for instance fonts.", self.parentDir)
			hadError = 1
		
		if os.path.exists(self.mmFontPath):
			self.emSquare = getEmSquare(self.mmFontPath)
		#print self.instancePath
		#print self.mmFontPath
		#print repr(self.parentDir)
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

def getEmSquare(mmFontPath):
	emSquare = "1000"
	command = "tx -dump -0 \"%s\" 2>&1" % (mmFontPath)
	report = FDKUtils.runShellCmd(command)
	
	m = re.search(r"sup.UnitsPerEm[ \t]+(\d+)", report)
	if m: # no match is OK, means the em aquare is the default 1000.
		emSquare = m.group(1)
	return emSquare
	
def readInstanceFile(options):

	f = open(options.instancePath, "rt")
	data = f.read()
	f.close()
	
	
	lines = data.splitlines()
	
	i = 0
	parseError = 0
	keyDict = copy.copy(kFixedFieldKeys)
	numKeys = kNumFixedFields
	numLines = len(lines)
	instancesList = []
	
	for i in range(numLines):
		line = lines[i]
		
		# Get rid of all comments. If we find a key definition comment line, parse it.
		commentIndex = line.find('#')
		if commentIndex >= 0:
			if line.startswith(kFieldsKey):
				if instancesList:
					logMsg.log("Error. Field header line must preceed an data line")
					raise ParseError
					
				# parse the line with the field names.
				line = line[len(kFieldsKey):]
				line = line.strip()
				keys = line.split('\t')
				keys = map(lambda name: name.strip(), keys)
				numKeys = len(keys)
				k = kNumFixedFields
				while k < numKeys:
					keyDict[k] = keys[k]
					k +=1
				continue
				
			else:
				line = line[:commentIndex]
				
		# get rid of blank lines
		line2 = line.strip()
		if not line2:
			continue # skip blank lines
		
		# Must be a data line.
			
		fields = line.split('\t')
		fields = map(lambda datum: datum.strip(), fields)
		numFields = len(fields)
		if (numFields != numKeys):
			logMsg.log("Error: at line %s, the number of fields %s do not match the number of key names %s." % (i, numFields, numKeys))
			parseError = 1
			continue
		
		instanceDict= {}
		#Build a dict from key to value. Some kinds of values needs special processing.
		for k in range(numFields):
			key = keyDict[k]
			field = fields[k]
			if not field:
				continue
			if field in ["Default", "None", "FontBBox"]:
				# FontBBox is no longer supported - I calculate the real
				# instance fontBBox from the glyph metrics instead,
				continue
			if key == kFontName:
				value = "/%s" % field
			elif key in [kExtraGlyphs, kExceptionSuffixes]:
				value = eval(field)
			elif key in [kIsBoldKey, kIsItalicKey, kCoordsKey]:
				value = eval(field) # this works for all three fields.
				
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
				elif key == kCoordsKey:
					if type(value) == type(0):
						value = (value,)
					
			elif field[0] in ["[","{"]: # it is a Type 1 array value. Pass as is, as a string.
				value = field
			else:
				# either a single number or a string.
				if re.match(r"^[-.\d]+$", field):
					value = field #it is a Type 1 number. Pass as is, as a string.
				else:
					# Wasn't a number. Format as Type 1 string, i.e add parens around it.
					value = "(%s)" % (field)
			instanceDict[key] = value
				
			
		instancesList.append(instanceDict)
		
	if parseError:
		raise(ParseError)
		
	return instancesList

class InstanceFontPaths:
	def __init__(self, options, instanceDict):
		""" Set the font directories and temp and final font paths for the current instance.
		"""
		psName = instanceDict[kFontName]
		
		# Figure out style name, for use in setting directory paths.
		parts = psName.split("-")
		if len(parts) > 1:
			style = parts[1]
		else:
			style = "Regular"
		
		# Set output directory path, and temp file names.
		dirPath = style
		if options.parentDir:
			dirPath = os.path.join(options.parentDir, style)
		
		if not os.path.exists(dirPath):
			logMsg.log("Warning: making new font instance path", dirPath)
			os.makedirs(dirPath)
		self.fontInstancePath = os.path.join(dirPath, options.InstanceFontName)
		self.tempInstance = self.fontInstancePath + ".tmp"
		self.textInstance = self.fontInstancePath + ".txt"
	
def getExistingValues(instanceDict, instanceFontPaths):
	""" Get the PostScript FontDict key/value pairs of interest
	from a pre-existing font instance.
	"""
	fontInstancePath = instanceFontPaths.fontInstancePath
	if not os.path.exists(fontInstancePath):
		logMsg.log("Error. Cannot open pre-existing instance file %s" % (fontInstancePath))
		raise(SnapShotError)

	#Decompile
	command = "detype1 \"%s\"" % (fontInstancePath)
	log = FDKUtils.runShellCmd(command)
	if not "cleartomark" in log[-200:]:
		logMsg.log("Error. Cannot decompile pre-existing instance file %s" % (fontInstancePath))
		logMsg.log(log)
		raise(SnapShotError)

	# Get key values
	updateDict = copy.copy(kPSFontDictKeys)
	missingList = []
	for key in updateDict:
		m1 = re.search(key + r"\s*", log)
		if not m1:
				logMsg.log("Warning: Failed to match key %s in original font" % (key))
				missingList.append(key)
				continue
		start = m1.end()
		limit = re.search(r"[\r\n]", log[start:]).start()
		m2 = re.search(r"(readonly)*(\s+def|\s*\|-)", log[start:start+limit])
		end = start + m2.start()
		value = log[start:end]
		updateDict[key] = value
		
	if missingList:
		# Remove from updateDict all the keys that
		# I did n't find in the pre-existing font.
		for key in missingList:
			del updateDict[key]
	return updateDict
		

def getFontBBox(fontPath):
	"""Get the real FontBox from the font instance. This often differs a lot
	from the interpolation of FontBBox in the parent MM font.
	"""
	command = "tx -mtx \"%s\" 2>&1" % (fontPath)
	log = FDKUtils.runShellCmd(command)
	mtxList = re.findall(r",\{([^,]+),([^,]+),([^,]+),([^}]+)", log)
	mtxList = mtxList[1:] # get rid of header line
	llxList = map(lambda entry: eval(entry[0]), mtxList)
	llyList = map(lambda entry: eval(entry[1]), mtxList)
	urxList = map(lambda entry: eval(entry[2]), mtxList)
	uryList = map(lambda entry: eval(entry[3]), mtxList)
	updateValue = "{ %s %s %s %s }" % ( min(llxList), min(llyList), max(urxList), max(uryList) )
	return updateValue


def doSnapshot(coords, emSquare, mmFontPath, tempInstance):
	coords = repr(coords)[1:-1] # get rid of parens
	if coords[-1] == ",":  # If it is a one-item list
		coords = coords[:-1]
	coords = re.sub(r"\s+", "", coords) # get rid of spaces after commas
	command = "IS -t1 -Z -U %s -z %s \"%s\" \"%s\" 2>&1" % (coords, emSquare, mmFontPath, tempInstance)
	log = FDKUtils.runShellCmd(command)
	if ("error" in log) or not os.path.exists(tempInstance):
		logMsg.log("Error in IS snapshotting to %s" % (tempInstance))
		logMsg.log("Command:", command)
		logMsg.log("log:", log)
		raise(SnapShotError)


def applyUpdateValues(log, instanceDict, updateDict, diffValueDict):
	""" For each {key,value} pairs in updateDict, check that it exists in
	the new instance. If so, then override the instanceDict value from the 
	updateDict value.
	"""
	for key,updateValue in updateDict.items():
		m1 = re.search(key + r"\s*", log)
		if not m1:
			value = None
		else:
			start = m1.end()
			limit = re.search(r"[\r\n]", log[start:]).start()
			m2 = re.search(r"(readonly)*(\s+def|\s*\|-)", log[start:start+limit])
			end = start + m2.start()
			value = log[start+1:end-1]
	
		try:
			if instanceDict[key]  != updateValue:
				instanceDict[key] = updateValue
				diffValueDict[key] = updateValue
			# else instanceDict has key and the update value is the same as the value from the instances file
		except KeyError:
			if value != updateValue:
				instanceDict[key] = updateValue
				diffValueDict[key] = updateValue
			# else don't need to add diff entry; since entry is not
				# in the instance receord, it is coming out right as a result

class FontParts:
	def __init__(self, log):
		# Parse text into prefix, suffix, and charstring dict.
		m1 = re.search(r"(/CharStrings\s+)\d+([^/]+)", log, re.DOTALL)
		if not m1:
			logMsg.log("Error. Was unable to parse decompiled font to extract glyphs.")
			raise ParseError
		charStart = m1.end(2)
		self.prefix = log[:m1.end(1)] # up to the end of /CharStrings\s+
		self.prefix2 = log[m1.start(2) : charStart]
		m2 = re.search(r"\send\s", log[charStart:], re.DOTALL)
		if not m2:
			logMsg.log("Error. Was unable to parse decompiled font to extract glyphs.")
			raise ParseError
		charEnd = m1.end(2) + m2.start()
		self.suffix = log[charEnd:]
		charBlock = log[charStart:charEnd]
		charList = re.findall(r"(/\S+)([^/]+)", charBlock)
		n = len(charList)
		self.charDict = {}
		charDict = self.charDict
		for i in range(n):
			entry = charList[i]
			try:
				charDict[entry[0][1:]] = Charstring(i, entry[0], entry[1])
			except CharParseError:
				print "Error: could not parse a component glyph in %s. Will skip merging exception glyph %s" % (glyphName, exceptionName )
				return

class CharParseError:
	pass

def rm(coords, x, y):
	x += coords[0]
	y += coords[1]
	return x,y, None
	
def hm(coords, x, y):
	x += coords[0]
	return x,y, [ [coords[0], 0], "rmoveto"]
	
def vm(coords, x, y):
	y += coords[0]
	return x,y, [ [0, coords[0] ], "rmoveto"]

def rt(coords, x, y):
	x += coords[0]
	y += coords[1]
	return x,y, None
	
def ht(coords, x, y):
	x += coords[0]
	return x,y, [ [ coords[0], 0], "rlineto"]
	
def vt(coords, x, y):
	y += coords[0]
	return x,y, [ [ 0, coords[0] ], "rlineto"]

def rc(coords, x, y):
	x += coords[0] + coords[2] + coords[4]
	y += coords[1] + coords[3] + coords[5]
	return x,y, None

def vhc(coords, x, y):
	x += coords[1] + coords[3]
	y += coords[0] + coords[2]
	return x,y, [ [0] + coords + [0], "rrcurveto"]

def hvc(coords, x, y):
	x += coords[0] + coords[1]
	y += coords[2] + coords[3]
	return x,y, [ [coords[0]] + [0] + coords[1:3] + [0] + [coords[3]], "rrcurveto"]

class Path:
	updateDict = {
		'rmoveto': rm,
		'hmoveto': hm,
		'vmoveto': vm,
		'rlineto': rt,
		'hlineto': ht,
		'vlineto': vt,
		'rrcurveto': rc,
		'vhcurveto': vhc,
		'hvcurveto': hvc,
			}
			
	def __init__(self):
		self.opList = []
		self.end = (0,0)
		self.start = (0,0)
		self.startIndex = 0
	
	def updateCurPoint(self, coords, op, x, y):
		return self.updateDict[op](coords, x, y)
		
	def __cmp__(self, otherPath):
		return cmp(self.getString(1), otherPath.getString(1))

	def __str__(self):
		return "Path start: %s. %s" % (self.start, self.getString())
		
	def __repr__(self):
		return "Path() # start: %s. %s" % (self.start, self.getString())
		
	def fuzzyMatch(self, otherPath, tolerance = 1):
		opList1 = self.opList[1:]
		opList2 = otherPath.opList[1:]
		numOps = len(opList1)
		match = cmp(numOps, len(opList2))
		if match != 0:
			return 0
			
		for i in range(numOps):
			entry1 = opList1[i]
			entry2 = opList2[i]
			if entry1[1] != entry2[1]:
				return 0
			coords1 = entry1[0]
			coords2 = entry2[0]
			numCoords = len(coords1)
			for j in range(numCoords):
				if abs(coords1[j] - coords2[j]) > tolerance:
					return 0
		return 1

	def getString(self, forCompare = 0):
		list = []
		if forCompare:
			opList = self.opList[1:] # ignore initial moveto
		else:
			opList = self.opList # ignore initial moveto
		for opEntry in opList:
			coords = map(str, opEntry[0])
			list.extend(coords)
			list.extend(opEntry[1])
		pathString = " ".join(list)
		return pathString
		
	def replacePath(self, otherPath):
		self.opList = otherPath.opList
	
	def adjustMoveTo(self):
		dx = self.start[0] - self.origin[0]
		dy = self.start[1] - self.origin[1]
		opEntry = self.opList[0] 
		if dx == 0:
			opEntry[0] = [dy]
			opEntry[1][0] = 'vmoveto'
		elif dy == 0:
			opEntry[0] = [dx]
			opEntry[1][0] = 'hmoveto'
		else:
			opEntry[0] = [dx, dy]
			opEntry[1][0] = 'rmoveto'

def pointDiff(first, second):
	diff =  [ first[0] - second[0], first[1] - second[1] ]
	return diff
	
def pointAdd(first, second):
	sum =  [ first[0] + second[0], first[1] + second[1] ]
	return sum
	
class Charstring:
	kMovetoPat = re.compile(r"(?:[-\d]+\s+)+[hvr]moveto")
	kOpPat = re.compile(r"((?:[-\d]+\s+)+)([^0-9\s]+)")
	def __init__(self, gid, glyphName, charstring):
		self.gid = gid
		self.glyphName = glyphName # this is the Type1 name, with a "/" prefix.
		self.origCharstring = charstring # Charstring after the glyph name.
		self.charstring = self.origCharstring
		self.parsed = 0
		self.beforeMove = ""
		self.paths = []
		
	def __cmp__(self, otherChar):
		return cmp(self.gid, otherChar.gid)
	
	def __str__(self):
		return "Char %s %s: %s" % (self.gid, self.glyphName, self.charstring)
		
	def __repr__(self):
		return "Charstring( %s,%s,%s)" % (self.gid, self.glyphName, self.charstring)
	
	def parsePaths(self, opList):
		paths = []
		numOps = len(opList)

		x = y = 0
		path = None
		for i in range(numOps): 
			opEntry = opList[i] # looks like ('coord 1-coordn' , 'op1 op2')
			coords = opEntry[0]
			ops = opEntry[1]
			if ops[0].endswith('moveto'):
				if path:
					oldPath = path
					oldPath.end = (x, y)
					oldPath.opList =  opList[startIndex:i]
				path = Path()
				paths.append(path)
				path.origin = (x,y)
				x, y, newEntry = path.updateCurPoint(coords, ops[0], x, y)
					
				startIndex = i
				path.start = (x, y)
			else:
				x, y, newEntry = path.updateCurPoint(coords, ops[0], x, y)
				
			# reduce optimization, e.g  "dy vlineto" -> "dx dy rlineto".
			# This so that rounding differences won't produce different path operators.
			if newEntry:
				opEntry[0] = newEntry[0] # replace expanded coords
				opEntry[1][0] = newEntry[1] # replace path op: e.g vlineto->rlineto.

		path.end = (x, y)
		path.opList =  opList[startIndex:]
		return paths
		
	def parse(self):
		if self.parsed:
			return
		self.parsed = 1
		self.paths = []
		self.movetos = []
		charstring = self.charstring
		self.seac = "seac" in charstring
		if self.seac:
			return
		m = self.kMovetoPat.search(charstring)
		if not m:
			print "Failed to find moveto in  %s." % (glyphName)
			#import pdb
			#pdb.set_trace()
			raise CharParseError
		start = m.start()
		self.prefix = charstring[:start]
		endCharIndex = charstring.find("endchar")
		self.suffix = charstring[endCharIndex:]
		charstring = charstring[start:endCharIndex]
		
		opList = []
		startPos = 0
		m = self.kOpPat.search(charstring[startPos:])
		while m:
			if m:
				coords = m.group(1).strip()
				op = m.group(2)
				startPos2 = startPos + m.end()
				m2 = self.kOpPat.search(charstring[startPos2:])
				if m2:
					ops = charstring[startPos + m.start(2): startPos2 + m2.start()]
					startPos = startPos2
				else:
					ops = charstring[startPos + m.start(2):]
				coords = coords.split()
				coords = map(int, coords)
				ops = ops.split()
				opList.append( [coords, ops] )
				m = m2
				
		self.paths = self.parsePaths(opList)
		
	def getCharString(self):
		return self.charstring
		
	def checkMatch(self, otherChar, pathIndex, compMetrics):
		if not self.parsed:
			self.parse()
		if not otherChar.parsed:
			otherChar.parse()
		if self.seac:
			return 1
			
		# compMetrics is [shift, scale]. Each of these contain [x, y].
		numPaths = len(otherChar.paths) 
		
		pathIndex
		ok = 1
			
		for i in range(numPaths):
			try:
				path1 = self.paths[pathIndex + i]
				path2 = otherChar.paths[i]
			except IndexError:
				#import pdb
				#pdb.set_trace()
				#print self
				raise CharParseError
				
			if not path1.fuzzyMatch(path2):
				print "\tWARNING: Paths don't match. Comp glyph %s ci %s, exception glyph %s ci %s. " % (self.glyphName, pathIndex + i, otherChar.glyphName, i)
				print "\t\tpath1: %s" % (path1)
				print "\t\tpath2: %s" % (path2)
		return ok
		
	def replace(self, otherChar, pathIndex, stdChar):
		if not self.parsed:
			self.parse()
		if not otherChar.parsed:
			otherChar.parse()
		if not stdChar.parsed:
			stdChar.parse()
		if self.seac:
			return
		
		newChar = copy.deepcopy(otherChar)
		# Note that the exception Glyph and can have a different number of paths than the stdChar.
		numPaths = len(stdChar.paths) # target paths in the curent glyph
		numOtherPaths = len(newChar.paths) # paths which will replace the target paths.
		
		if numPaths == numOtherPaths:
			for i in range(numPaths):
				self.paths[pathIndex + i].replacePath(newChar.paths[i])
		elif numPaths > numOtherPaths:
			for i in range(numOtherPaths):
				self.paths[pathIndex + i].replacePath(newChar.paths[i])
			i += 1
			del_i = pathIndex + i
			while i < numPaths:
				del self.paths[del_i]
				i += 1
		else: # numPaths < numOtherPaths
			for i in range(numPaths):
				self.paths[pathIndex + i].replacePath(newChar.paths[i])
			i += 1
			while i < numOtherPaths:
				self.paths.insert(pathIndex + i, newChar.paths[i])
				i += 1

		# we want to line up not the start points of the std and other chars,
		#  rather the origins. See if there is a delta that needs to be applied between the start point
		# of the stdChar and the otherChar.
		diffStart = pointDiff(newChar.paths[0].start, stdChar.paths[0].start)
		#if diffStart != [0,0]:
		#	print "Nonzero diffStart", diffStart, self.glyphName
		firstPath = self.paths[pathIndex]
		firstPath.start = pointAdd(firstPath.start, diffStart)
		self.paths[pathIndex].adjustMoveTo() # fixes moveto  coords to go from origin to start.

		componentShift = pointDiff(firstPath.start, newChar.paths[0].start)
		lastComponentPath = self.paths[pathIndex + numOtherPaths-1]
		lastOtherPath = newChar.paths[numOtherPaths-1]
		if numOtherPaths > 1: # don't need to fix the start of the first  == last component again.
			lastComponentPath.start = pointAdd(lastOtherPath.start, componentShift)
		lastComponentPath.end = pointAdd(lastOtherPath.end, componentShift)
		if len(self.paths) > pathIndex + numOtherPaths:
			nextPath = self.paths[pathIndex + numOtherPaths]
			nextPath.origin = lastComponentPath.end
			nextPath.adjustMoveTo()
		
		list = [self.prefix]
		for i in range(len(self.paths)):
			list.append(self.paths[i].getString())
		list.append(self.suffix)
		self.charstring = " ".join(list)
		return

	def replaceAllPaths(self, otherChar):
		self.paths = copy.deepcopy(otherChar.paths)
		self.charstring  = copy.copy(otherChar.charstring)
		
	def scale(self, scale):
		if not self.parsed:
			self.parse()
		scaledChar = copy.deepcopy(self)

		if (scale[0] != 1.0) or (scale[1] != 1.0):
			print "Help! I need to scale exception %s." % (self.glyphName)
			#import pdb
			#pdb.set_trace()
			raise CharParseError
		return scaledChar

def getExceptionEntries(charDict, exceptionSuffixList):
	glyphList = charDict.keys()
	exceptionList = []
	seenExceptionDict = {}
	for name in glyphList:
		for suffix in exceptionSuffixList:
			if name.endswith(suffix):
				stdName = name[:-len(suffix)]
				if charDict.has_key(stdName):
					exceptionList.append( [stdName, name])
				else:
					print"Error: standard glyph name %s is not in font (derived from exception glyph name %s.)" % (stdName, name)
				seenExceptionDict[suffix] = 1
				break
	
	# Check that all suffixes matched something.
	for suffix in exceptionSuffixList:
		try:
			seen = seenExceptionDict[suffix]
		except KeyError:
			logMsg.log("Error. There are no glyphs with the exception glyph suffx", suffix)
	
	return exceptionList

def replaceContour(glyphName, pathIndex, metrics, stdName, exceptionName, charDict):
	""" How this works:
	1) divide each glyph string up into a prefix, the paths, and a suffix. Get the start point for each path.
	Calculate the new start points for the first path in the std glyph. If this matches at the expected paths
	in the target glyph all is well, else report an error.
	
	2) For each path in the exception glyph, replace the corresponding path in the target glyph.
		2.A Split path into ops and operands
		2.B. Apply the shift, then scale to each coordinate e exception glyph path
		2.C reconstitute this into a string
		2.D replace the target path.
	"""
	targetChar = charDict[glyphName]
	stdChar = charDict[stdName]
	exceptionChar = charDict[exceptionName]

	ok = targetChar.checkMatch(stdChar, pathIndex, metrics)
	if not ok:
		print "Error: could not match %s in composite glyph %s. Will skip merging exception glyph %s" % (stdName, glyphName, exceptionName )
		return
	
	if metrics:
		scale = metrics[1]
		if scale: # if we have to scale the glyph
			srcChar = exceptionChar.scale(scale) # makes a new char - we can't alter the exceptionChar, as it is used more than once.
		# I don't need to apply the shift, as that is already handled in the Charstring.replace() function, which 
		# aligns starts points of the target paths and the replacement charString.
	else:
		srcChar = exceptionChar
	targetChar.replace(srcChar, pathIndex, stdChar)
	return

def blendMetrics(metrics, wgtVector):
	numMasters = len(wgtVector)
	shift = [0.0, 0.0]
	scale = [0.0, 0.0]
	for i in range(numMasters):
		metric = metrics[i]
		if metric == None:
			lshift = [0.0, 0.0]
			lscale = [1.0, 1.0]
		else:
			lshift = metrics[i][0]
			if not lshift:
				lshift = [0.0, 0.0]
			lscale =  metrics[i][1]
			if not lscale:
				lscale = [1.0, 1.0]
		wv = wgtVector[i]
		shift[0] += lshift[0]*wv
		shift[1] += lshift[1]*wv
		scale[0] += lscale[0]*wv
		scale[1] += lscale[1]*wv
	shift[0] = round(shift[0])
	shift[1] = round(shift[1])
	scale[0] = round(scale[0], 12)
	scale[1] = round(scale[1], 12)
	return (shift, scale)

def fixGlyphs(log, extraList, exceptionList, compositeDict, wgtVector):
	"""Determine the list of exception MM glyphs by finding all glyphs with the names suffixes.
	Then build the list of glyphs to be removed: this is the kExtraGlyphs list, plus all the glyphs
	whose name is the same as the MM exception glyphs, with their extension removed.
	
	Split out the charstring block, and convert the glyphs to a dict keyed by name, with value - [index, charstring]
	First delete the glyph names in the kExtraGlyphs list.
	Then, for each glyph name in the MM exception glyph list, replace the matching glyph name without the sufiix
	then delete the MM execption glyph.
	"""
	fontParts = FontParts(log)
	if exceptionList:
		# We don't need to decompose any compsite glyphs that will be wholly replaced by an exception glyph.
		for exceptionName, stdName in exceptionList:
			if compositeDict.has_key(stdName):
				compositeList = compositeDict[stdName]
				for cmpEntry in compositeList:
					glyphName = cmpEntry[0] # this is the name of the composite glyph in  which stdName is a component.
					if glyphName == stdName:
						compositeList.remove(cmpEntry) 

		print "Fixing composite glyphs by replacing std glyph components with exception glyphs."
		for exceptionName, stdName in exceptionList:
			# First  step through all the composite glyphs, and fix any which have the stdName as a composite.
			if not compositeDict.has_key(stdName):
				continue
			
			print "\nReplacing %s with %s:" % (stdName, exceptionName)
			compositeList = compositeDict[stdName]
			for cmpEntry in compositeList:
				glyphName = cmpEntry[0]
				pathIndex = cmpEntry[1]
				metrics = cmpEntry[2]
				if metrics:
					metrics = blendMetrics(metrics, wgtVector)
				replaceContour(glyphName, pathIndex, metrics, stdName, exceptionName, fontParts.charDict)
				print "\tFixed comp %s with %s." % (glyphName, exceptionName)
			# Then replace the base glyphs.
		print "\nReplacing glyph1 with glyph2:"
		for exceptionName, stdName in exceptionList:
			print "\t(%s, %s)" % (stdName, exceptionName)
			try:
				fontParts.charDict[stdName].replaceAllPaths(fontParts.charDict[exceptionName])
			except KeyError:
				print
				logMsg.log("Warning: either glyph %s or %s not in font!"  % (stdName, exceptionName))
		print

	if extraList:
		print "\nDeleting glyphs:"
		for glyphName in extraList:
			try:
				del fontParts.charDict[glyphName]
				print "\t%s" % glyphName
			except KeyError:
				print
				logMsg.log("Warning: extra glyph %s not in font!"  % (glyphName))
		print
			
	charList = fontParts.charDict.values()
	charList.sort() # sorts by index order, since that is first
	dataList = [fontParts.prefix, " %s " % len(charList), fontParts.prefix2]
	for char in charList:
		dataList.append(char.glyphName)
		dataList.append(char.charstring)
	dataList.append(fontParts.suffix)
	log = "".join(dataList)
	
	return log
	
def fixPostScriptFontDict(log, key, value):
	""" Search for the PostScript FontDict key, and update its value.
	"""
	m1 = re.search("/" + key + r"\s*", log)
	if not m1:
		# Will add a new key/value. We only do this for some fields.
		if key in kPSNameKeys:
			m1 = re.search("/FontName" + r"\s*", log)
			start = m1.end()
			limit = re.search(r"[\r\n]", log[start:]).start()
			m2 = re.search(r"(readonly)*(\s+def|\s*\|-)([\r\n ]+)", log[start:start+limit+1])
			end = start + m2.end()
			if m2.group(1) == None:
				readOnlyTxt = ""
			else:
				readOnlyTxt = m2.group(1)
			newEntry = "/%s %s %s%s%s" % (key, value,readOnlyTxt,m2.group(2),m2.group(3))
			log = log[:end] + newEntry + log[end:]
		elif key in kPSFontDictKeys.keys():
			# add after BlueValues.
			m1 = re.search("/BlueValues" + r"\s*", log)
			start = m1.end()
			limit = re.search(r"[\r\n]", log[start:]).start()
			m2 = re.search(r"(readonly)*(\s+def|\s*\|-)([\r\n ]+)", log[start:start+limit+1])
			end = start + m2.end()
			if m2.group(1) == None:
				readOnlyTxt = ""
			else:
				readOnlyTxt = m2.group(1)
			newEntry = "/%s %s %s%s%s" % (key, value,readOnlyTxt,m2.group(2),m2.group(3))
			log = log[:end] + newEntry + log[end:]
		else:
			editError = 1
			#import pdb
			#pdb.set_trace()
			logMsg.log("Error: Failed to match PostScript FontDict key '%s' in list of known PS Font Dict keys: %s." % (key, kPSNameKeys + kPSFontDictKeys.keys()))
	else:
		start = m1.end()
		limit = re.search(r"[\r\n]", log[start:]).start()
		m2 = re.search(r"(readonly)*(\s+def|\s*\|-)", log[start:start+limit])
		end = start + m2.start()
		log = log[:start] + value + log[end:]
	return log

def closeEnough(firstMetrics, secondMetrics, shift):
	num = len(firstMetrics)
	if not (num == len(secondMetrics)):
		return 0
	for i in range(num):
		val1 = firstMetrics[i]
		val2 = secondMetrics[i]
		if shift:
			val2 += shift[i]
		
		diff = abs(val1 - val2)
		if diff > 3:
			return 0
	return 1
	
def  calculateWeightVector( userCoords, mmFontPath):
	numCoords = len(userCoords)
	numMasters = 2**numCoords
	wghtVector  = [0]*numMasters
	if numCoords == 1:
		BCA1 = userCoords[0]/1000.0
		wghtVector[0] = (1-BCA1)
	elif numCoords == 2:
		BCA1 = userCoords[0]/1000.0
		BCA2 = userCoords[1]/1000.0
		wghtVector[0] = (1-BCA1)*(1-BCA2) 
		wghtVector[1] = (BCA1)*(1-BCA2) 
		wghtVector[2] = (1-BCA1)*(BCA2) 
	elif numCoords == 3:
		BCA1 = userCoords[0]/1000.0
		BCA2 = userCoords[1]/1000.0
		BCA3 = userCoords[2]/1000.0
		wghtVector[0] = (1-BCA1)*(1-BCA2)*(1-BCA3)
		wghtVector[1] = (BCA1)*(1-BCA2)*(1-BCA3)
		wghtVector[2] = (BCA1)*(BCA2)*(1-BCA3)
		wghtVector[3] = (1-BCA1)*(BCA2)*(1-BCA3)
		wghtVector[4] = (1-BCA1)*(1-BCA2)*(BCA3)
		wghtVector[5] = (BCA1)*(1-BCA2)*(BCA3)
		wghtVector[6] = (BCA1)*(BCA2)*(BCA3)
	else:
		print "Error: Weight vector calculation only support 3 axes"
		raise SnapShotError
	wghtVector[numMasters-1] = round( 1.0 - reduce(lambda x, y: x+y, wghtVector[0:-1]) , 12)

	return wghtVector


def makeInstance(instanceDict, updateDict, options, instanceFontPaths, extraGlyphList, exceptionList, compositeDict):

	editError = 0
	diffValueDict = {} # record which values in updateDict are new or differ from what is in instanceDict
						# Used only for debugging.

	coords = instanceDict[kCoordsKey]
	tempInstance = instanceFontPaths.tempInstance
	textInstance = instanceFontPaths.textInstance
	fontInstancePath = instanceFontPaths.fontInstancePath
	wgtVector = calculateWeightVector(instanceDict[kCoordsKey], options.mmFontPath)
	if os.path.exists(tempInstance):
		os.remove(tempInstance)
		
	# Do snapshot to make instance font.
	doSnapshot(coords, options.emSquare, options.mmFontPath, tempInstance)
	logMsg.log("Wrote instance font file '%s'." % (fontInstancePath))

	#Decompile font to text
	command = "detype1 \"%s\"" % (tempInstance)
	detype1Data = FDKUtils.runShellCmd(command)
	if not "cleartomark" in detype1Data[-200:]:
		logMsg.log("Error in IS snapshotting: can't decompile temp snapshot file %s." % (tempInstance))
		logMsg.log(log)
		raise(SnapShotError)

	# Set which values need to be updated from the original font.
	# Note that this only happens when user specifies option -ui else
	# the updateDict is empty
	if updateDict:
		applyUpdateValues(detype1Data, instanceDict, updateDict, diffValueDict)

	# Take care of the kExtraGlyphs and kExceptionSuffixes, if any.
	psName = instanceDict[kFontName]
	if extraGlyphList or exceptionList:
		detype1Data = fixGlyphs(detype1Data, extraGlyphList, exceptionList, compositeDict, wgtVector)
	
	# Update tempInstance (font.pfa.tmp) after the ExtraGlyphs
	# have been removed. This is necessary otherwise the FontBBox
	# values will be inaccurate
	fp = open(textInstance, "wt")
	fp.write(detype1Data)
	fp.close()
	command = "type1 \"%s\" 2>&1 1> \"%s\"" % (textInstance, tempInstance)
	FDKUtils.runShellCmd(command)
	
	# Get the real FontBBox, and stuff it into the instance value.
	updateValue = getFontBBox(tempInstance)
	instanceDict["FontBBox"] = updateValue
	itemList = instanceDict.items()
	for key,value in itemList:
		if key in  kNonPostScriptKeys:
			continue
		detype1Data = fixPostScriptFontDict(detype1Data, key, value)

	# compile decompiled text back into a Type 1 font.
	fp = open(textInstance, "wt")
	fp.write(detype1Data)
	fp.close()
	command = "type1 \"%s\" 2>&1 1>\"%s\"" % (textInstance, fontInstancePath)
	log = FDKUtils.runShellCmd(command)
	if log:
		logMsg.log("Error: type1 complained about decompiled font data in temp file %s. Log:" % (textInstance))
		logMsg.log(log)
		raise(SnapShotError)

	# Now remove all overlaps
	# compile decompiled text back into a Type 1 font.
	command = "tx -t1 -Z \"%s\" \"%s\"  2>&1" % (fontInstancePath, tempInstance)
	txLog = FDKUtils.runShellCmd(command)
	if ("invalid" in txLog):
		logMsg.log("Error: tx complained about converting font data to Type 1 font %s. Log:" % (fontInstancePath))
		logMsg.log(txLog)
		raise(SnapShotError)
	elif log:
		logMsg.log("Warning: tx complained about converting font data to Type 1 font %s. Log:" % (fontInstancePath))
		logMsg.log(txLog)
		
		
	try:
		if os.path.exists(fontInstancePath):
			os.remove(fontInstancePath)
		os.rename(tempInstance, fontInstancePath)
	except OSError:
		logMsg.log( "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
		logMsg.log(tempInstance, fontInstancePath)
	
	#DEBUG
	#if os.path.exists(tempInstance):
	#	os.remove(tempInstance)
	#if os.path.exists(textInstance):
	#	os.remove(textInstance)
		
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
	else:
		command = "tx -t1  \"%s\" \"%s\"  2>&1" % (fontInstancePath, tempInstance)
		log = FDKUtils.runShellCmd(command)
		try:
			if os.path.exists(fontInstancePath):
				os.remove(fontInstancePath)
			os.rename(tempInstance, fontInstancePath)
		except OSError:
			logMsg.log( "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
			logMsg.log(tempInstance, fontInstancePath)
		
	# Add creation date. Have to do this last, as the tx-based tools strip off PS comments.
	fp = open(fontInstancePath, "rt")
	log = fp.read()
	fp.close()
	m1 = re.search(r"(\s+)%%BeginResource", log)
	if not m1:
		logMsg.log("Error in IS snapshotting to %s: no %%BeginResource keyword in font." % (tempInstance))
		raise(SnapShotError)
	insertPos = m1.start()
	dateString = "%%CreationDate %s" % time.asctime()
	log = log[:insertPos] + m1.group(1) + dateString + log[insertPos:]
	fp = open(fontInstancePath, "wt")
	fp.write(log)
	fp.close()

	# Delete 'font.pfa.txt' file
	try:
		if os.path.exists(textInstance):
			os.remove(textInstance)
	except:
		pass

	logMsg.log("\tDone generating instance:", fontInstancePath)

	return diffValueDict
	
		
def writeNewInstancesFile(instancePath, instancesList):

	# The keys added may not have been the same for all instanceDicts. Compile the union of all keys.
	# Start with the required keys, to get them in the right order.
	keyDict = {}
	for item in kFixedFieldKeys.items():
		keyDict[item[1]] = item[0]
	i = len(keyDict)
	for instanceDict in instancesList:
		# Supress ForceBold.
		if instanceDict.has_key(kForceBold):
			del instanceDict[kForceBold] 
		instanceDict[kIsBoldKey] = "0"
		for key in instanceDict:
			try:
				j = keyDict[key]
			except KeyError:
				keyDict[key] = i
				i += 1
	# Turn 	keyDict into a list sorted by index.
	keyList = keyDict.items()
	keyList = map(lambda entry: [entry[1], entry[0]], keyList)
	# each entry s now [index, key]
	keyList.sort()
	keyList = map(lambda entry: entry[1], keyList)
	
	# sort the non-fixed fields alphabetically.
	newKeys = keyList[len(kFixedFieldKeys):]
	newKeys.sort()
	keyList =  keyList[:len(kFixedFieldKeys)] + newKeys
	keyList.remove("FontBBox")
	
	# Build the key field line.
	line = [kFieldsKey] + keyList
	line = "\t".join(line)
	lines = [line]
	for instanceDict in instancesList:
		line = []
		for key in keyList:
			if key == "FontBBox":
				continue # skip FontBBox.
			try:
				value = instanceDict[key]
				if type(value) != type(""):
					if key != kCoordsKey:
						value = repr(value)[1:-1]
					else:
						value = repr(value)
				elif value[0] == "(":
					value = value[1:-1] # trim off parens
				elif value[0] == "/":
					value = value[1:] # trim off parens
				elif value[0] == "[":
					value = re.sub("r\[\s+", "[", value)
					value = re.sub("r\s+\]", "]", value)
				line.append(value)
			except KeyError:
				line.append(kDefaultValue)
		
		lines.append("\t".join(line))
	data = os.linesep.join(lines) + os.linesep
	f = open(instancePath, "wt")
	f.write(data)
	f.close()
	
	return

def findExtraGlyphMatches(extraGlyphs, charDict):
	charList = charDict.keys()
	charList.sort()
	glyphList = []
	for line in extraGlyphs:
		if line.startswith('*'):
			matchList = filter(lambda name: name.endswith(line[1:]), charList)
			if matchList:
				glyphList.extend(matchList)
			else:
				print "Warning! Extra glyph suffix '%s' did not match in the char list." % (line)
		elif line.startswith('^'):
			matchList = filter(lambda name: re.search(line, name), charList)
			if matchList:
				glyphList.extend(matchList)
			else:
				print "Warning! Extra glyph search pattern  '%s' did not match in the char list." % (line)
		else:
			if charDict.has_key(line):
				glyphList.append(line)
			
	# We do not need to eliminate duplicates; that happens when these lists are added to extraDict.
	return glyphList
	
	
def findExceptionGlyphMatches(exceptionSuffixes, charDict):
	charList = charDict.keys()
	charList.sort()
	glyphList = []
	for suffix in exceptionSuffixes:
		matchList = filter(lambda name: name.endswith(suffix), charList)
		if not matchList:
			print "Warning! Exception glyph suffix '%s' did not match in the char list." % (suffix)
		else:
			matchList2 = []
			missingList = []
			for name in matchList:
				charList.remove(name)
				if charDict.has_key(name[:-len(suffix)]):
					matchList2.append([name, name[:-len(suffix)]] )
				else:
					missingList.append(name)
			if matchList2:
				glyphList.extend(matchList2)
			if missingList:
				print "Warning! Exception glyphs '%s' did not have alternates without suffix '%s', and were skipped." % (missingList, suffix)
	# We do not need to eliminate duplicates; that happens when these lists are added to extraDict.
	return glyphList
	
	
def fillSpecialGlyphLists(mmFontPath, instancesList):
	extraGlyphList = [] # Is not instance specific
	exceptionDict = {} # Lists are instance specific.

	command = "tx -mtx \"%s\" 2>&1" % mmFontPath
	log = FDKUtils.runShellCmd(command)
	charList = re.findall(r"glyph\[[^{]+\{([^,]+)",log)
	if not charList:
		print "Error. 'tx -mtx' failed on the mmfont.pfa file."
		print log
		raise ParseError
	charDict = {}
	for glyphName in charList:
		charDict[glyphName] = 1
		
	extraGlyphsDict = {}
	for instanceDict in instancesList:
		psName = instanceDict[kFontName]
		extraGlyphs = instanceDict.get(kExtraGlyphs, None)
		if extraGlyphs:
			glyphList = findExtraGlyphMatches(extraGlyphs, charDict)
			for glyphName in glyphList:
				extraGlyphsDict[glyphName] = 1
		
		exceptionSuffixes = instanceDict.get(kExceptionSuffixes, None)
		if exceptionSuffixes:
			localExceptionDict = {}
			glyphList = findExceptionGlyphMatches(exceptionSuffixes, charDict)
			for entry in glyphList:
				gName = entry[0]
				localExceptionDict[gName] = entry
				extraGlyphsDict[gName] = 1
			localExceptionList = localExceptionDict.values()
			localExceptionList.sort()
			exceptionDict[psName] = localExceptionList
	extraGlyphList = extraGlyphsDict.keys()
	extraGlyphList.sort()
	return extraGlyphList, exceptionDict

def fillCompositeDict(options):
	""" Return a dict of the composite glyphs with components that are replacement targets in the MM exception list.
	"""
	mmDir = os.path.dirname(os.path.abspath(options.mmFontPath))
	fpath = os.path.join(mmDir, kCompositeDataName)
	if os.path.exists(fpath):
		fp = open(fpath, "rt")
		data = fp.read()
		fp.close()
		compositeDict = eval(data)
	else:
		print "Warning: could not find the composite glyph data file that is written by the Adobe script for FontLab' Instance Generator'. Please run this to create the necessary file."
		print "Will not be able to decompose composite glyphs."
		newCompositeDict = {}
		return newCompositeDict
		
	# This dict has glyph names as keys. Each entry is a list of component entrries. Each component entry looks like:
	# [component Name, path number, None or [list of shifts and deltas for each master]
	# We want a dict keyed by component name, with a list of component entries
	# Each dict value will be a list of component, where th glyph name is the name of the composite glyph that
	# references the key glyph name as a component, and the metrics for the key glyph in the comnposite glyph.
	newCompositeDict = {}
	for glyphName, compositeEntryList in compositeDict.items():
		for compositeEntry in compositeEntryList:
			compName = compositeEntry[0]
			compositeEntry[0] = glyphName
			try:
				newCmpList = newCompositeDict[compName]
				newCmpList.append(compositeEntry)
			except KeyError:
				newCompositeDict[compName] = [compositeEntry]
	return newCompositeDict
	
def run(args):
	options = Options(args)
	instancesList = readInstanceFile(options)

	iRange = range(len(instancesList))
	
	haveUpdates = 0
	
	extraGlyphList, exceptionDict = fillSpecialGlyphLists(options.mmFontPath, instancesList)

	if exceptionDict:
		compositeDict = fillCompositeDict(options)
	else:
		compositeDict = {}
		
	instanceIndex = 0
	for instanceDict in instancesList:
		instanceIndex += 1
		if options.indexList and (not (instanceIndex -1) in options.indexList):
			continue
			
		instanceFontPaths = InstanceFontPaths(options, instanceDict)
		if options.useExisting:
			# Get PS FontDict values from the pre-existing font instance.
			updateDict = getExistingValues(instanceDict, instanceFontPaths)
		else:
			updateDict = {}
		# make new instance font.
		psName = instanceDict[kFontName]
		try:
			exceptionList = exceptionDict[psName]
		except KeyError:
			exceptionList = None
		haveChange = makeInstance(instanceDict, updateDict, options, instanceFontPaths, extraGlyphList, exceptionList, compositeDict)
		if haveChange:
			haveUpdates = 1 # 

	if options.updateInstances:
		# Update the instances file with values from the pre-existing
		# instance fonts.
		if haveUpdates:
			# Save the old instance to its same file name plus some number.
			if os.path.exists(options.instancePath):
				i = 0
				while 1:
					i += 1
					path = "%s%s" % (options.instancePath, i)
					if not os.path.exists(path):
						break
				if os.path.exists(path):
					os.remove(path)
				os.rename(options.instancePath, path)
				
			# Write the new instance file.
			writeNewInstancesFile(options.instancePath, instancesList)
			logMsg.log("Wrote new instance file to ", os.path.abspath(options.instancePath))
		else:
			logMsg.log("Note: No changes needed to be made to the instances files.")
		

if __name__=='__main__':
	try:
		run(sys.argv[1:])
	except (OptError, ParseError):
		logMsg.log("Quitting after error.")
		pass
	
	
