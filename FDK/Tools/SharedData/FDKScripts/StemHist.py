#!/bin/env python
__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
stemHist. Wrapper for the Adobe auto-hinting C library. The AC C library
works on only one glyph at a time, expressed in the old 'bez' format, a
bezier-curve language much like PostScript Type 1, but with text
operators and coordinates.

The stemHist script uses Just von Rossum's fontTools library to read and write a
font file. It extracts the PostScript T2 charstring program for each selected
glyph in turn. It converts this into a 'bez language file, and calls the
autohintexe program to work on this file. stemHist returns a returns a list of
either stem hints or alignment zones.


In order to work on Type 1 font files as well as the OpenType/CFF fonts
supported directly by the fontTools library, stemHist uses the 'tx' tool
to convert the font to a 'CFF' font program, and then builds a partial
OpenType font for the fontTools to work with.

"""
__usage__ = """
stemHist program v1.26 April 27 2016
stemHist -h
stemHist -u
stemHist [-g <glyph list>] [-gf <filename>] [-xg <glyph list>] [-xgf <filename>] [-all] [-a] [-new] -q font-path1 font-path2...

Stem and Alignment zone report for OT/CFF fonts.
Copyright (c) 2006 Adobe Systems Incorporated
"""

__help__ = __usage__ + """
Examines the specified glyphs, and generates a report of either vertical
and horizontal stems, or alignment zones,  with a count of how many
glyphs have a stem with that zone, and the list of glyphs which have the
zone.

These reports are then used to select alignment zones and horizontal and
vertical stems for use in the global font hinting info. The idea is that
you should either pick the most popular alignment zones for the font
global alignment zone settings, or fix your font so that the important
alignment zones (Cap height, X height, Overshoot, etc), show up as the
most popular alignment zones. Same for stem hints; you should either use
the several most frequent stem widths for the font global stem hint
lists, or fix your font so that the most frequent stem widths are the
important ones.

Note that you can specify a set of glyphs from which to derive your
report. This allows you to get the stem report from just a few glyphs
where the main stems should all be the same, and to compare this with
the report from all the glyphs. If you have done a good job of digital
design, the main stems in all glyphs will lead to a report of a fairly
small set of frequently occurring different stem widths.

Options:

-h	Print help

-u	Print help

-g <glyphID1>,<glyphID2>,...,<glyphIDn>   Report on only the specified
	list of glyphs. The list must be comma-delimited. The glyph ID's may
	be glyphID's, glyph names, or glyph CID's. If the latter, the CID
	value must be prefixed with the string "cid". There must be no
	white-space in the glyph list. Examples: stemHist -g A,B,C,69 myFont
	stemHist -g cid1030,cid34,cid3455,69 myCIDFont

A range of glyphs may be specified by providing two names separated only
	by a hyphen: AC -g zero-nine,onehalf myFont Note that the range will
	be resolved by expanding the glyph indices (GID)'s, not by
	alphabetic names.

-gf <file name>   Report on only the list of glyphs contained in the
	specified file, The file must contain a comma-delimited list of
	glyph identifiers. Any number of space, tab, and new-line characters
	are permitted between glyph names and commas.

-xg, -xgf  Same as -g and -gf, but will exclude the specified glyphs from hinting.

-a   Print alignment zone report rather than stem report

-all Include stems formed by curved line segments; by default, includes
	only stems formed by straight line segments.

-new Skip processing a font if the report files already exist. This
	allows you to use the command: stemHist -new */*/*ps, and apply the
	tool only to fonts where the report has not yet been run.

-o report path 
	When this is specified, the path argument is used as the base
	path for the reports. Otherwise, the font file path is used as the
	base path. Several reports are written, using the base path
	name plus an extension.
	Without the -a option, the report files are:
	   <base path >.hstm.txt   The horizontal stems.
	   <base path >.vstm.txt  The vertical stems.
	With the -a option, the report files are:
	   <base path>.top.txt   The top zones.
	  <base path >.bot.txt   The bottom zones.

-q  Don't print glyph names - just print a dot occasionally to show progress. 

"""



import sys
import os
import re
import time
from fontTools.ttLib import TTFont, getTableModule
from BezTools import *
import warnings
import FDKUtils
import ufoTools
import traceback

kTempCFFSuffix = ".temp.ac.cff"

warnings.simplefilter("ignore", RuntimeWarning) # supress waring about use of os.tempnam().


rawData = []


kFontPlistSuffix  = ".plist"
kBlueValueKeys = [
			"BaselineOvershoot", # 0
			"BaselineYCoord", #1
			"LcHeight", #2
			"LcOvershoot", #3
			"CapHeight", #4
			"CapOvershoot", #5
			"AscenderHeight", #6
			"AscenderOvershoot", #7
			"FigHeight", #8
			"FigOvershoot", #9
			]

kOtherBlueValueKeys = [
			"DescenderHeight", # 0
			"DescenderOvershoot", #1
			"Baseline5", #2
			"Baseline5Overshoot", #3
			"SuperiorBaseline", #4
			"SuperiorOvershoot", #5
			]


class ACOptions:
	def __init__(self):
		self.fileList = []
		self.glyphList = []
		self.excludeGlyphList = 0
		self.allStems = 0
		self.doAlign = 0
		self.reportPath = None
		self.new = 0
		self.noFlex = 1
		self.allow_no_blues = 1
		self.vCounterGlyphs = []
		self.hCounterGlyphs = []
		self.verbose = 1
		self.debug = 0

class FDKEnvironmentError(AttributeError):
	pass

class ACOptionParseError(KeyError):
	pass

class ACFontError(KeyError):
	pass

class ACHintError(KeyError):
	pass

def logMsg(*args):
	for s in args:
		print s

def ACreport(*args):
	# long function used by the hinting library
	for arg in args:
		print arg,
	if arg[-1] != os.linesep:
		print

def CheckEnvironment():
	txPath = 'tx'
	txError = 0
	command = "%s -u 2>&1" % (txPath)
	report = FDKUtils.runShellCmd(command)
	if "options" not in report:
			txError = 1
	
	if  txError:
		logMsg("Please re-install the FDK. The executable directory \"%s\" is missing the tool: < %s >." % (exe_dir, txPath ))
		logMsg("or the files referenced by the shell script is missing.")
		raise FDKEnvironmentError

	return txPath

def expandNames(glyphName):
	glyphRange = glyphName.split("-")
	if len(glyphRange) > 1:
		g1 = expandNames(glyphRange[0])
		g2 =  expandNames(glyphRange[1])
		glyphName =  "%s-%s" % (g1, g2)

	elif glyphName[0] == "/":
		glyphName = "cid" + glyphName[1:].zfill(5)

	elif glyphName.startswith("cid") and (len(glyphName) < 8):
		return "cid" + glyphName[3:].zfill(5)

	return glyphName

def parseGlyphListArg(glyphString):
	glyphString = re.sub(r"[ \t\r\n,]+",  ",",  glyphString)
	glyphList = glyphString.split(",")
	glyphList = map(expandNames, glyphList)
	glyphList =  filter(None, glyphList)
	return glyphList


def getOptions():
	options = ACOptions()
	i = 1
	numOptions = len(sys.argv)
	while i < numOptions:
		arg = sys.argv[i]
		if options.fileList and arg[0] == "-":
			raise ACOptionParseError("Option Error: All file names must follow all other options <%s>." % arg) 

		if arg == "-h":
			logMsg(__help__)
			raise ACOptionParseError
		elif arg == "-u":
			logMsg(__usage__)
			raise ACOptionParseError
		elif arg == "-all":
			options.allStems = 1
		elif arg == "-a":
			options.doAlign = 1
		elif arg == "-d":
			options.debug = 1
		elif arg == "-q":
			options.verbose = 0
		elif arg == "-o":
			i = i +1
			options.reportPath = sys.argv[i]
		elif arg == "-new":
			options.new = 1
		elif arg in ["-xg", "-g"]:
			if arg == "-xg":
				options.excludeGlyphList = 1
			i = i +1
			glyphString = sys.argv[i]
			if glyphString[0] == "-":
				raise ACOptionParseError("Option Error: it looks like the first item in the glyph list following '-g' is another option.") 
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg in ["-xgf", "-gf"]:
			if arg == "-xgf":
				options.excludeGlyphList = 1
			i = i +1
			filePath = sys.argv[i]
			if filePath[0] == "-":
				raise ACOptionParseError("Option Error: it looks like the the glyph list file following '-gf' is another option.") 
			try:
				gf = file(filePath, "rt")
				glyphString = gf.read()
				gf.close()
			except (IOError,OSError):
				raise ACOptionParseError("Option Error: could not open glyph list file <%s>." %  filePath) 
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg[0] == "-":
			raise ACOptionParseError("Option Error: Unknown option <%s>." %  arg) 
		else:
			arg = arg.rstrip(os.sep) # might be a UFO font. auto completion in some shells adds a dir separator, which then causes problems with os.path.dirname().
			options.fileList += [arg]
		i  += 1
		
	if not options.fileList:
		raise ACOptionParseError("Option Error: You must provide at least one font file path.") 
		
	return options


def getGlyphID(glyphTag, fontGlyphList):
	glyphID = None
	try:
		glyphID = int(glyphTag)
		glyphName = fontGlyphList[glyphID]
	except IndexError:
		pass
	except ValueError:
		try:
			glyphID = fontGlyphList.index(glyphTag)
		except IndexError:
			pass
		except ValueError:
			pass
	return glyphID

def getGlyphNames(glyphTag, fontGlyphList, fontFileName):
	glyphNameList = []
	rangeList = glyphTag.split("-")
	prevGID = getGlyphID(rangeList[0], fontGlyphList)
	if prevGID == None:
		if len(rangeList) > 1:
			logMsg( "\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (rangeList[0], glyphTag, fontFileName))
		else:
			logMsg( "\tWarning: glyph ID <%s> from glyph selection list option is not in font. <%s>." % (rangeList[0], fontFileName))
		return None
	glyphNameList.append(fontGlyphList[prevGID])

	for glyphTag2 in rangeList[1:]:
		gid = getGlyphID(glyphTag2, fontGlyphList)
		if gid == None:
			logMsg( "\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (glyphTag2, glyphTag, fontFileName))
			return None
		for i in range(prevGID+1, gid+1):
			glyphNameList.append(fontGlyphList[i])
		prevGID = gid

	return glyphNameList

def filterGlyphList(options, fontGlyphList, fontFileName):
	# Return the list of glyphs which are in the intersection of the argument list and the glyphs in the font
	# Complain about glyphs in the argument list which are not in the font.
	if not options.glyphList:
		glyphList = fontGlyphList
	else:
		# expand ranges:
		glyphList = []
		for glyphTag in options.glyphList:
			glyphNames = getGlyphNames(glyphTag, fontGlyphList, fontFileName)
			if glyphNames != None:
				glyphList.extend(glyphNames)
		if options.excludeGlyphList:
			newList = filter(lambda name: name not in glyphList, fontGlyphList)
			glyphList = newList
	return glyphList


def getFontInfo(pTopDict, fontPSName, options, fdIndex = 0):
	# The AC library needs the global font hint zones and standard stem widths. Format them
	# into a single  text string.
	# The text format is arbitrary, inherited from very old software, but there is no real need to change it.
	# Assign arbitrary values that will prevent the alignment zones from affecting AC.
	if  hasattr(pTopDict, "FDArray"):
		pDict = pTopDict.FDArray[fdIndex]
	else:
		pDict = pTopDict
	privateDict = pDict.Private

  	fontinfo=[]
	fontinfo.append("OrigEmSqUnits")
	upm = int(1/pDict.FontMatrix[0])
	fontinfo.append(str(upm) ) # OrigEmSqUnits

	fontinfo.append("FontName")
	if  hasattr(pTopDict, "FontName"):
		fontinfo.append(pDict.FontName ) # FontName
	else:
		fontinfo.append(fontPSName )

	low =  min( -upm*0.25, pTopDict.FontBBox[1] - 200)
	high =  max ( upm*1.25, pTopDict.FontBBox[3] + 200)
	# Make a set of inactive alignment zones: zones outside of the font bbox so as not to affect stem collection.
	# Some fonts have bad BBox values, so I don't let this be smaller than -upm*0.25, upm*1.25.
	inactiveAlignmentValues = [low, low, high, high]
	blueValues = inactiveAlignmentValues
	numBlueValues = len(blueValues)
	numBlueValues = min(numBlueValues, len(kBlueValueKeys))
	blueValues = map(str, blueValues)
		
	for i in range(numBlueValues):
		fontinfo.append(kBlueValueKeys[i])
		fontinfo.append(blueValues[i])

	i = 0
	if numBlueValues < 5:
		fontinfo.append("CapHeight")
		fontinfo.append('0' )  #CapOvershoot = 0 always needs a value!

	
	vstems = [upm] # dummy value. Needs to be larger than any hint will likely be,
			# as the autohint program strips out any hint wider than twice the largest global stem width..
	vstems = repr(vstems)
	fontinfo.append("DominantV")
	fontinfo.append(vstems)
	fontinfo.append("StemSnapV")
	fontinfo.append(vstems)

	hstems = [upm] # dummy value. Needs to be larger than any hint will likely be,
			# as the autohint program strips out any hint wider than twice the largest global stem width.
	hstems = repr(hstems)
	fontinfo.append("DominantH")
	fontinfo.append(hstems)
	fontinfo.append("StemSnapH")
	fontinfo.append(hstems)

	fontinfo.append("FlexOK")
	fontinfo.append("false")
	fontinfo.append(" ")

	fontinfoStr = " ".join([str(x) for x in fontinfo])
	return fontinfoStr


def openFile(path):
	if os.path.isfile(path):
		font =  openOpenTypeFile(path, None)
	else:
		# maybe it is a a UFO font.
		# We always use the hash map to skip glyphs that have been previously processed, unless the user has said to do all.
		font =  openUFOFile(path, None, 0)
	return font

def openUFOFile(path, outFilePath, useHashMap):
	# Check if has glyphs/contents.plist
	contentsPath = os.path.join(path, "glyphs", "contents.plist")
	if not os.path.exists(contentsPath):
		msg = "Font file must be a PS, CFF, OTF, or ufo font file: %s." % (path)
		logMsg(msg)
		raise ACFontError(msg)
	
	# If user has specified a path other than the source font path, then copy the entire
	# ufo font, and operate on the copy.
	if (outFilePath != None) and (os.path.abspath(path) != os.path.abspath(outFilePath)):
		msg = "Copying from source UFO font to output UFO fot before processing..."
		logMsg(msg)
		path = outFilePath
	font = ufoTools.UFOFontData(path, useHashMap, ufoTools.kAutohintName)
	return font
	
def openOpenTypeFile(path, outFilePath):
	# If input font is  CFF or PS, build a dummy ttFont in memory..
	# return ttFont, and flag if is a real OTF font Return flag is 0 if OTF, 1 if CFF, and 2 if PS/
	fontType  = 0 # OTF
	tempPathCFF = path + kTempCFFSuffix
	try:
		ff = file(path, "rb")
		data = ff.read(10)
		ff.close()
	except (IOError, OSError):
		logMsg("Failed to open and read font file %s." % path)

	if data[:4] == "OTTO": # it is an OTF font, can process file directly
		try:
			ttFont = TTFont(path)
		except (IOError, OSError):
			raise ACFontError("Error opening or reading from font file <%s>." % path)
		except TTLibError:
			raise ACFontError("Error parsing font file <%s>." % path)

		try:
			cffTable = ttFont["CFF "]
		except KeyError:
			raise ACFontError("Error: font is not a CFF font <%s>." % fontFileName)

	else:
	
		# It is not an OTF file.
		if (data[0] == '\1') and (data[1] == '\0'): # CFF file
			fontType = 1
			tempPathCFF = path
		elif not "%" in data:
			#not a PS file either
			logMsg("Font file must be a PS, CFF or OTF  fontfile: %s." % path)
			raise ACFontError("Font file must be PS, CFF or OTF file: %s." % path)
	
		else:  # It is a PS file. Convert to CFF.	
			fontType =  2
			print "Converting Type1 font to temp CFF font file..."
			command="tx  -cff +b -std \"%s\" \"%s\" 2>&1" % (path, tempPathCFF)
			report = FDKUtils.runShellCmd(command)
			if "fatal" in report:
				logMsg("Attempted to convert font %s  from PS to a temporary CFF data file." % path)
				logMsg(report)
				raise ACFontError("Failed to convert PS font %s to a temp CFF font." % path)
		
		# now package the CFF font as an OTF font.
		ff = file(tempPathCFF, "rb")
		data = ff.read()
		ff.close()
		try:
			ttFont = TTFont()
			cffModule = getTableModule('CFF ')
			cffTable = cffModule.table_C_F_F_('CFF ')
			ttFont['CFF '] = cffTable
			cffTable.decompile(data, ttFont)
		except:
			logMsg( "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
			logMsg("Attempted to read font %s  as CFF." % path)
			raise ACFontError("Error parsing font file <%s>." % path)

	fontData = CFFFontData(ttFont, path, outFilePath, fontType, logMsg)
	return fontData




kCharZone = "charZone"
kStemZone = "stemZone"
kHStem = "HStem"
kVStem = "VStem"
class GlyphReports:
	def __init__(self):
		self.glyphName = None
		self.hStemList = {}
		self.vStemList = {}
		self.hStemPosList = {}
		self.vStemPosList = {}
		self.charZoneList = {}
		self.stemZoneStemList = {}
		self.glyphs = {}

	def startGlyphName(self, glyphName):
		self.hStemList = {}
		self.vStemList = {}
		self.hStemPosList = {}
		self.vStemPosList = {}
		self.charZoneList = {}
		self.stemZoneStemList = {}
		self.glyphs[glyphName] = [self.hStemList, self.vStemList, self.charZoneList, self.stemZoneStemList]
		self.glyphName = glyphName
	
	def addGlyphReport(self, reportString):
		lines = reportString.splitlines()
		for line in lines:
			tokenList = line.split()
			key = tokenList[0]
			x = eval(tokenList[3])
			y = eval(tokenList[5])
			hintpos = "%s %s" % (x ,y)
			if key == kCharZone:
				self.charZoneList[hintpos] = (x, y)
			elif key == kStemZone:
				self.stemZoneStemList[hintpos] = (x, y)
			elif key == kHStem:
				width = x - y
				if not self.hStemPosList.has_key(hintpos): # avoid counting duplicates
					count = self.hStemList.get(width, 0)
					self.hStemList[width] = count+1
					self.hStemPosList[hintpos] = width
			elif key == kVStem:
				width = x - y
				if not self.vStemPosList.has_key(hintpos): # avoid counting duplicates
					count = self.vStemList.get(width, 0)
					self.vStemList[width] = count+1
					self.vStemPosList[hintpos] = width
			else:
				raise ACFontError("Error: Found unknown keyword %s in report file for glyph %s." % (key, self.glyphName))
				
	
	def getReportDicts(self):
		hStemDict = {}
		vStemDict = {}
		topZoneDict = {}
		bottomZoneDict = {}
		for item in  self.glyphs.items():
			gName = item[0]
			hStemList, vStemList, charZoneList, stemZoneStemList = item[1]

			for item in hStemList.items():
				width, gcount = item
				if width >= 0:
					width = int(width + 0.5)
				else:
					width = int(width - 0.5)
				try:
					count, glyphList = hStemDict[width]
					hStemDict[width][0] = count + gcount
					hStemDict[width][1].append(gName)
				except KeyError:
					hStemDict[width] = [gcount,[gName]]
					
			for item in vStemList.items():
				width, gcount = item
				if width >= 0:
					width = int(width + 0.5)
				else:
					width = int(width - 0.5)
				try:
					count, glyphList = vStemDict[width]
					vStemDict[width][0] = count + gcount
					vStemDict[width][1].append(gName)
				except KeyError:
					vStemDict[width] = [gcount,[gName]]
					
			for btval in charZoneList.values():
				top, bottom = btval
				if top >= 0:
					top = int(top + 0.5)
				else:
					top = int(top - 0.5)
				if bottom >= 0:
					bottom = int(bottom + 0.5)
				else:
					bottom = int(bottom - 0.5)
				try:
					count, glyphList = topZoneDict[top]
					topZoneDict[top][0] = count + 1
					topZoneDict[top][1].append(gName)
				except KeyError:
					topZoneDict[top] = [1,[gName]]
				try:
					count, glyphList = bottomZoneDict[bottom]
					bottomZoneDict[bottom][0] = count + 1
					bottomZoneDict[bottom][1].append(gName)
				except KeyError:
					bottomZoneDict[bottom] = [1,[gName]]
					
			for btval in stemZoneStemList.values():
				top, bottom = btval
				if top >= 0:
					top = int(top + 0.5)
				else:
					top = int(top - 0.5)
				if bottom >= 0:
					bottom = int(bottom + 0.5)
				else:
					bottom = int(bottom - 0.5)
				try:
					count, glyphList = topZoneDict[top]
					topZoneDict[top][0] = count + 1
					topZoneDict[top][1].append(gName)
				except KeyError:
					topZoneDict[top] = [1,[gName]]
				try:
					count, glyphList = bottomZoneDict[bottom]
					bottomZoneDict[bottom][0] = count + 1
					bottomZoneDict[bottom][1].append(gName)
				except KeyError:
					bottomZoneDict[bottom] = [1,[gName]]
					
	
		return hStemDict, vStemDict,topZoneDict, bottomZoneDict


def srtCnt(first, second):
	first = first[1][0]
	second = second[1][0]
	return cmp(second, first)


def srtVal(first, second):
	first = first[0]
	second = second[0]
	return cmp(first, second)


def srtRevVal(first, second):
	first = first[0]
	second = second[0]
	return cmp(second, first)


def  formatReport(rDict, sortFnc):
	items = rDict.items()
	items.sort(sortFnc)
	# report in order count, value, glyph list.
	items = [ [item[1][0], item[0], item[1][1]] for item in items]
	# sort the glyph lists.
	for item in items:
		item[2].sort()
	reportLines = [ "%s\t%s\t%s" % (item[0], item[1], item[2]) for item in items]
	return reportLines


def checkReportExists(path, doAlign):
	if doAlign:
		suffixes = (".top.txt", ".bot.txt")
	else:
		suffixes = (".hstm.txt", ".vstm.txt")
	foundOne = 0
	for i in range(len(suffixes)):
		fName = path + suffixes[i]
		if os.path.exists(fName):
			foundOne = 1
			break
	return foundOne


def PrintReports(path, hStemDict, vStemDict,topZoneDict, bottomZoneDict):
	items = ([hStemDict,srtCnt], [vStemDict, srtCnt],[topZoneDict, srtRevVal], [bottomZoneDict, srtVal])
	atime = time.asctime()
	suffixes = (".hstm.txt", ".vstm.txt", ".top.txt", ".bot.txt")
	titles = ("Horizontal Stem List for %s on %s" % (path, atime), 
				"Vertical Stem List for %s on %s" % (path, atime),
				"Top Zone List for %s on %s" % (path, atime),
				"Bottom Zone List for %s on %s" % (path, atime),
			)
	headers = ("Count\tWidth\tGlyph List",
			"Count\tWidth\tGlyph List",
			"Count\tTop Zone\tGlyph List",
			"Count\tBottom Zone\tGlyph List",
			)
	sawData = 0
	for i in range(len(items)):
		rDict = items[i][0]
		sortFunc = items[i][1]
		fName = path + suffixes[i]
		title = titles[i]
		header = headers[i]
		try:
			if not rDict:
				continue
			sawData = 1
			fp = file(fName, "wt")
			fp.write(title + os.linesep)
			fp.write(header + os.linesep)
			reportLines = formatReport(rDict, sortFunc)
			for line in reportLines:
				fp.write(line + os.linesep)
			fp.close()
			print "\tWrote  %s" % (fName)
		except (IOError, OSError):
			print "Has some error opening the file %s!" % fName
	if not sawData:
		print "\tWarning. No reports printed - no stem data was collected."

def collectStemsFont(path, options, txPath):
	#    use fontTools library to open font and extract CFF table. 
	#    If error, skip font and report error.
	fontFileName = os.path.basename(path)
	logMsg("")
	if options.doAlign:
		logMsg( "Collecting alignment zones for font %s. Start time: %s." % (path, time.asctime()))
	else:
		logMsg( "Collecting stems for font %s. Start time: %s." % (path, time.asctime()))

	try:
		fontData = openFile(path)
	except (IOError, OSError):
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise ACFontError("Error opening or reading from font file <%s>." % fontFileName)
	except:
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise ACFontError("Error parsing font file <%s>." % fontFileName)

	#   filter specified list, if any, with font list.
	fontGlyphList = fontData.getGlyphList()
	glyphList = filterGlyphList(options, fontGlyphList, fontFileName)
	if not glyphList:
		raise ACFontError("Error: selected glyph list is empty for font <%s>." % fontFileName)

	tempBaseName = os.tempnam()
	tempBez = tempBaseName + ".bez"
	tempReport = tempBez + ".rpt"
	tempFI = tempBaseName + ".fi"
	
	#    open font plist file, if any. If not, create empty font plist.
	psName = fontData.getPSName()
	
	#    build alignment zone string
	allow_no_blues =  1
	noFlex = 0
	vCounterGlyphs = hCounterGlyphs = []
	fdGlyphDict, fontDictList = fontData.getfdInfo(psName, path, allow_no_blues, noFlex, vCounterGlyphs, hCounterGlyphs, glyphList)

	if fdGlyphDict == None:
		fdDict = fontDictList[0]
		fp = open(tempFI, "wt")
		fp.write(fdDict.getFontInfo())
		fp.close()
	else:
		if not options.verbose:
			logMsg("Note: Using alternate FDDict global values from fontinfo file for some glyphs. Remove option '-q' to see which dict is used for which glyphs.")

	removeHints = 1
	isCID = fontData.isCID()
	lastFDIndex = None
	glyphReports = GlyphReports()

	if not options.verbose:
		dotCount = 0
		curTime = time.time()

	for name in glyphList:
		if name == ".notdef":
			continue

		if options.verbose:
			logMsg("Checking %s." %name)
		else:
			newTime = time.time()
			if (newTime - curTime) > 1:
				print ".",
				sys.stdout.flush()
				curTime = newTime
				dotCount +=1
			if dotCount > 40:
				dotCount = 0
				print ""

		# 	Convert to bez format
		bezString, width, hasHints = fontData.convertToBez(name, removeHints, options.verbose)
		if bezString == None:
			continue
		if "mt" not in bezString:
			# skip empty glyphs.
			continue

		# get new fontinfo string if FD array index has changed, as
		# as each FontDict has different alignment zones.
		gid = fontData.getGlyphID(name)
		if isCID: #
			fdIndex = fontData.getfdIndex(gid)
			if not fdIndex == lastFDIndex:
				lastFDIndex = fdIndex
				fdDict = fontData.getFontInfo(psName, path, options.allow_no_blues, options.noFlex, options.vCounterGlyphs, options.hCounterGlyphs, fdIndex)
				fp = open(tempFI, "wt")
				fp.write(fdDict.getFontInfo())
				fp.close()
		else:
			if (fdGlyphDict != None):
				try:
					fdIndex = fdGlyphDict[name][0]
				except KeyError:
					# use default dict.
					fdIndex = 0
				if lastFDIndex != fdIndex:
					lastFDIndex = fdIndex
					fdDict = fontDictList[fdIndex]
					fp = open(tempFI, "wt")
					fp.write(fdDict.getFontInfo())
					fp.close()
		
		glyphReports.startGlyphName(name)
		

		# 	Call auto-hint library on bez string.
		bp = open(tempBez, "wt")
		bp.write(bezString)
		bp.close()

		if os.path.exists(tempReport):
			os.remove(tempReport)
		if options.doAlign:
			doAlign = "-ra"
		else:
			doAlign = "-rs"
			
		if options.allStems:
			allStems = "-a"
		else:
			allStems = ""
			
		command = "autohintexe -q %s %s  -f \"%s\" \"%s\" 2>&1" % (doAlign, allStems, tempFI, tempBez)
		if options.debug:
			print command
		log = FDKUtils.runShellCmd(command)
		if log:
			print log
			if "number terminator while" in log:
				print tempBez
				sys.exit()
 
		if os.path.exists(tempReport):
			bp = open(tempReport, "rt")
			report = bp.read()
			bp.close()
			print "report <%s>" % (report)
			if options.debug:
				print "Wrote AC fontinfo data file to", tempFI
				print "Wrote AC output rpt file to", tempReport
			else:
				os.remove(tempReport)
			report.strip()
			if report:
				glyphReports.addGlyphReport(report)
				if options.debug:
					rawData.append(report)
		else:
			print "Error - failure in processing outline data"
			report = None
			
	hStemDict, vStemDict,topZoneDict, bottomZoneDict = glyphReports.getReportDicts()
	if options.reportPath:
		reportPath = options.reportPath
	else:
		reportPath = path
	PrintReports(reportPath, hStemDict, vStemDict,topZoneDict, bottomZoneDict)
	fontData.close()
	logMsg( "Done with font %s. End time: %s." % (path, time.asctime()))


def main():
	try:
		txPath = CheckEnvironment()
	except FDKEnvironmentError,e:
		logMsg(e)
		return

	try:
		options = getOptions()
	except ACOptionParseError,e:
		logMsg(e)
		return
		
	# verify that all files exist.
	haveFiles = 1
	for path in options.fileList:
		if not os.path.exists(path):
			logMsg( "File does not exist: <%s>." % path)
			haveFiles = 0
	if not haveFiles:
		return

	for path in options.fileList:
		if options.new:
			if options.reportPath:
				reportPath = options.reportPath
			else:
				reportPath = path
			foundOne = checkReportExists(reportPath, options.doAlign)
			if foundOne:
				logMsg( "Skipping %s, as a report already exists." % (path))
				continue
		try:
			collectStemsFont(path, options, txPath)
		except (ACFontError, ufoTools.UFOParseError),e:
			logMsg( "\t%s" % e)
		if options.debug:
			fp = open("rawdata.txt", "wt")
			fp.write(os.linesep.join(rawData))
			fp.close()
	return


if __name__=='__main__':
	main()
	
	
