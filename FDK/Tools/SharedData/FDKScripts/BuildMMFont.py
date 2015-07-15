#!/public/bin/python

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""
 
__usage__ = """ BuildMMFont v1.6 Aug 27 2010

BuildMMFont [-u] [-h] 
BuildMMFont [-new]  [-srcEM <number>]  [-dstEM <number> ] [-v]

"""

__help__ = """ BuildMMFont

Build MM font from bez files.

First builds base design single master fonts from the bez file
directories, auto-hints them, then assembles a one axis MM font from the
single-master fonts.

Options

The program asumes that is is being run in the root directory of an MM
font source data tree; that is, the directory where the mmfont.pfa file
will be built, and where the mmfontinfo file already exists. This
directory will contain a single-master sub-directory for each base
design; that is, a directory containing the font.pfa file, and the
directory of bez files for the single-master font.

The program will first parse the mmfontinfo file, and determine the base
design font directory paths. It will then look in each of these for the
'bez' directory, and will build a new 'font.pfa' file in the each parent
directory of the 'bez' directories.

The program will set all glyphs widths 1- 1000.

-new   With this option, the script will derive new global hint data for
each base design from the sum of all the bez files, and will write  this
single set of global data to all the new 'font.pfa' file. Existing
'font.pfa' files will get overwritten.

Without this option, the program will use the existing font.pfa files.


-srcEM <number> This option specifies the em-square size of the original
bez files. Intelligent scaling is used to scale the bez files to the
em-square of the base font. Default value is 1000.

-dstEM <number> This option specifies the target em-square size. By
default, this is 1000, or the em-size of the final row font if it
already exists. If not specified, the default is 1000

Notes: Calls the AC, type1, detype1 and tx tools.

Some bez files will have the suffix ".uc" added when written by
BezierLab. This is because neither Mac nor Windows file systems can put
two files in the same directory that differ only by case of one or more
letters. This program will remove the suffix when adding the bez file to
the font, so that the glyh name will not end with '.uc. """

__methods__ = """ Overview: 1) Read mmfontinfo file to get single master
directory names, and determine base font.

2) Build and auto-hint single-master fonts.

2.1) Make temp font out of the bez files using arbitrary font dict
values based on the origEM. - use template OTF font, and  BezTools.py to
stuff bez files into temp OTF.  Save temp file

2.2) Set up BlueValues

- apply Align, Stem Hist to  the base font. Collect the data, use
heuristics.  open with FontTools, fix hint and font dict values.

- determine example stem for each alignment and global stem width. Get
coordinates of these points from the other fonts, and apply these as the
global alignment zones and stem widths.


2.3) Hint temp fonts

2.4) Convert temp.cff to final font.ps

3) Merge fonts into template MM font. 
	3.1) Fix up template MM font from mmfontinfo: axis, name, etc
	3.2) Use detype 1 to decompile singel-master fonts
	3.3) Collect global metrics and hint values, font BBox, etc
	3.4) Collect dict of charstrings. Normalize outlines for a
	single glyph.
	3.5) add MM charstring to text template
	3.6) recompile.
"""
 
import sys
import os
import re
import time
import glob
import shutil
import copy
import pprint
import math
import pdb
import pprint

try:
	import BezTools
except ImportError:
	selfPath = os.path.abspath(__file__)
	newPath = selfPath.split(os.sep)[:-3] + ["public", "libraries", "scripts"]
	newPath = os.sep.join(newPath)
	sys.path.append(newPath)
	import BezTools
	
from fontTools.ttLib import TTFont, getTableModule
from fontTools.misc.psCharStrings import T2CharString
from fontTools.pens.boundsPen import BoundsPen
kMMFontInfoPath = "mmfontinfo"
kDstFileName = "font.pfa"
kWidthsFileName = "widths"
kSrcBezDir = "bez"
kUCSuffix = ".uc" # Must be kept in accordance with the suffix added by BezierLab.
kLenUC = len(kUCSuffix)
import FDKUtils

gDebug = 1

class TempFont:
	xmlData = """<?xml version="1.0" encoding="ISO-8859-1"?>
<ttFont sfntVersion="OTTO" ttLibVersion="2.0b2">

  <GlyphOrder>
    <!-- The 'id' attribute is only for humans; it is ignored when parsed. -->
    <GlyphID id="0" name=".notdef"/>
    <GlyphID id="1" name="space"/>
  </GlyphOrder>

  <head>
    <!-- Most of this table will be recalculated by the compiler -->
    <tableVersion value="1.0"/>
    <fontRevision value="1.02899169922"/>
    <checkSumAdjustment value="0x291E4CCF"/>
    <magicNumber value="0x5F0F3CF5"/>
    <flags value="00000000 00000011"/>
    <unitsPerEm value="1000"/>
    <created value="Sun Jun  5 17:16:54 2005"/>
    <modified value="Fri Oct 14 15:50:04 2005"/>
    <xMin value="-166"/>
    <yMin value="-283"/>
    <xMax value="1021"/>
    <yMax value="927"/>
    <macStyle value="00000000 00000000"/>
    <lowestRecPPEM value="3"/>
    <fontDirectionHint value="2"/>
    <indexToLocFormat value="0"/>
    <glyphDataFormat value="0"/>
  </head>

  <hhea>
    <tableVersion value="1.0"/>
    <ascent value="726"/>
    <descent value="-274"/>
    <lineGap value="200"/>
    <advanceWidthMax value="1144"/>
    <minLeftSideBearing value="-166"/>
    <minRightSideBearing value="-170"/>
    <xMaxExtent value="1021"/>
    <caretSlopeRise value="1"/>
    <caretSlopeRun value="0"/>
    <caretOffset value="0"/>
    <reserved0 value="0"/>
    <reserved1 value="0"/>
    <reserved2 value="0"/>
    <reserved3 value="0"/>
    <metricDataFormat value="0"/>
    <numberOfHMetrics value="2"/>
  </hhea>

  <maxp>
    <tableVersion value="0x5000"/>
    <numGlyphs value="2"/>
  </maxp>

  <OS_2>
    <version value="2"/>
    <xAvgCharWidth value="532"/>
    <usWeightClass value="400"/>
    <usWidthClass value="5"/>
    <fsType value="00000000 00001000"/>
    <ySubscriptXSize value="650"/>
    <ySubscriptYSize value="600"/>
    <ySubscriptXOffset value="0"/>
    <ySubscriptYOffset value="75"/>
    <ySuperscriptXSize value="650"/>
    <ySuperscriptYSize value="600"/>
    <ySuperscriptXOffset value="0"/>
    <ySuperscriptYOffset value="350"/>
    <yStrikeoutSize value="50"/>
    <yStrikeoutPosition value="269"/>
    <sFamilyClass value="0"/>
    <panose>
      <bFamilyType value="2"/>
      <bSerifStyle value="4"/>
      <bWeight value="5"/>
      <bProportion value="2"/>
      <bContrast value="5"/>
      <bStrokeVariation value="5"/>
      <bArmStyle value="5"/>
      <bLetterForm value="3"/>
      <bMidline value="3"/>
      <bXHeight value="4"/>
    </panose>
    <ulUnicodeRange1 value="10000000 00000000 00000000 10101111"/>
    <ulUnicodeRange2 value="01010000 00000000 00100000 01001010"/>
    <ulUnicodeRange3 value="00000000 00000000 00000000 00000000"/>
    <ulUnicodeRange4 value="00000000 00000000 00000000 00000000"/>
    <achVendID value="ADBE"/>
    <fsSelection value="00000000 01000000"/>
    <fsFirstCharIndex value="32"/>
    <fsLastCharIndex value="64258"/>
    <sTypoAscender value="726"/>
    <sTypoDescender value="-274"/>
    <sTypoLineGap value="200"/>
    <usWinAscent value="927"/>
    <usWinDescent value="283"/>
    <ulCodePageRange1 value="00000000 00000000 00000000 00000001"/>
    <ulCodePageRange2 value="00000000 00000000 00000000 00000000"/>
    <sxHeight value="449"/>
    <sCapHeight value="689"/>
    <usDefaultChar value="32"/>
    <usBreakChar value="32"/>
    <usMaxContex value="4"/>
  </OS_2>

  <name>
    <namerecord nameID="0" platformID="1" platEncID="0" langID="0x0">
      Copyright &#169; 2005 Adobe Systems Incorporated.  All Rights Reserved.
    </namerecord>
    <namerecord nameID="1" platformID="1" platEncID="0" langID="0x0">
      Temp Font
    </namerecord>
    <namerecord nameID="2" platformID="1" platEncID="0" langID="0x0">
      Regular
    </namerecord>
    <namerecord nameID="3" platformID="1" platEncID="0" langID="0x0">
      1.001;ADBE;TempFont-Regular
    </namerecord>
    <namerecord nameID="4" platformID="1" platEncID="0" langID="0x0">
      Temp Font Regular
    </namerecord>
    <namerecord nameID="5" platformID="1" platEncID="0" langID="0x0">
      Version 1.00
    </namerecord>
    <namerecord nameID="6" platformID="1" platEncID="0" langID="0x0">
      TempFont-Regular
    </namerecord>
    <namerecord nameID="0" platformID="3" platEncID="1" langID="0x409">
      Copyright &#169; 2005 Adobe Systems Incorporated.  All Rights Reserved.
    </namerecord>
    <namerecord nameID="1" platformID="3" platEncID="1" langID="0x409">
      Temp Font
    </namerecord>
    <namerecord nameID="2" platformID="3" platEncID="1" langID="0x409">
      Regular
    </namerecord>
    <namerecord nameID="3" platformID="3" platEncID="1" langID="0x409">
      1.001;ADBE;TempFont-Regular
    </namerecord>
    <namerecord nameID="4" platformID="3" platEncID="1" langID="0x409">
      TempFont-Regular
    </namerecord>
    <namerecord nameID="5" platformID="3" platEncID="1" langID="0x409">
      Version 1.00
    </namerecord>
    <namerecord nameID="6" platformID="3" platEncID="1" langID="0x409">
      TempFont-Regular
    </namerecord>
  </name>

  <cmap>
    <tableVersion version="0"/>
    <cmap_format_4 platformID="0" platEncID="3" language="0">
      <map code="0x20" name="space"/><!-- SPACE -->
    </cmap_format_4>
    <cmap_format_6 platformID="1" platEncID="0" language="0">
      <map code="0x9" name="space"/>
    </cmap_format_6>
    <cmap_format_4 platformID="3" platEncID="1" language="0">
      <map code="0x20" name="space"/><!-- SPACE -->
    </cmap_format_4>
  </cmap>

  <post>
    <formatType value="3.0"/>
    <italicAngle value="0.0"/>
    <underlinePosition value="-75"/>
    <underlineThickness value="50"/>
    <isFixedPitch value="0"/>
    <minMemType42 value="0"/>
    <maxMemType42 value="0"/>
    <minMemType1 value="0"/>
    <maxMemType1 value="0"/>
  </post>

  <CFF>

    <CFFFont name="TempFont-Regular">
      <version value="001.001"/>
      <Notice value="Copyright 2005 Adobe Systems.  All Rights Reserved. This software is the property of Adobe Systems  Incorporated and its licensors, and may not be reproduced, used,   displayed, modified, disclosed or transferred without the express   written approval of Adobe. "/>
      <FullName value="Temp Font"/>
      <FamilyName value="TempFont"/>
      <Weight value="Regular"/>
      <isFixedPitch value="0"/>
      <ItalicAngle value="0"/>
      <UnderlineThickness value="50"/>
      <PaintType value="0"/>
      <CharstringType value="2"/>
      <FontMatrix value="0.001 0 0 0.001 0 0"/>
      <FontBBox value="-166 -283 1021 927"/>
      <StrokeWidth value="0"/>
      <!-- charset is dumped separately as the 'GlyphOrder' element -->
      <Encoding>
      </Encoding>
      <Private>
        <BlueValues value="-20 0 689 709 459 469 726 728"/>
        <BlueScale value="0.039625"/>
        <BlueShift value="7"/>
        <BlueFuzz value="1"/>
        <StdHW value="1"/>
        <StdVW value="1"/>
         <ForceBold value="0"/>
        <LanguageGroup value="0"/>
        <ExpansionFactor value="0.06"/>
        <initialRandomSeed value="0"/>
        <defaultWidthX value="0"/>
        <nominalWidthX value="0"/>
      </Private>
      <CharStrings>
        <CharString name=".notdef">
          1000 0 50 600 50 hstem
          0 50 400 50 vstem
          0 vmoveto
          500 700 -500 hlineto
          250 -305 rmoveto
          -170 255 rlineto
          340 hlineto
          -140 -300 rmoveto
          170 255 rlineto
          -510 vlineto
          -370 -45 rmoveto
          170 255 170 -255 rlineto
          -370 555 rmoveto
          170 -255 -170 -255 rlineto
          endchar
        </CharString>
        <CharString name="space">
          1000 endchar
        </CharString>
      </CharStrings>
    </CFFFont>

    <GlobalSubrs>
      <!-- The 'index' attribute is only for humans; it is ignored when parsed. -->
    </GlobalSubrs>

  </CFF>

  <hmtx>
    <mtx name=".notdef" width="500" lsb="0"/>
    <mtx name="space" width="250" lsb="0"/>
  </hmtx>

</ttFont>
"""

class CBOptions:
	def __init__(self):
		self.mmFontInfo = None
		self.makeNewFonts = 0
		self.srcEM = 1000
		self.dstEM = 1000

class FDKEnvironmentError(AttributeError):
	pass

class CBOptionParseError(KeyError):
	pass

class CBFontError(KeyError):
	pass

class CBError(KeyError):
	pass

def logMsg(*args):
	noNewLine = 0
	if args[-1] == "noNewLine":
		noNewLine = 1
		args = args[:-1]
		
	for s in args[:-1]:
		print s,
	
	if noNewLine:
		print args[-1],
	else:
		print args[-1]

class SupressMsg:
	def write(self, *args):
		pass

def makeTempFont(fontPath, supressMsg):
	ttFont = None
	tempFont = TempFont()
	try:
		xf = file(fontPath, "wt")
		xf.write(tempFont.xmlData)
		xf.close()
	except(IOError, OSError), e:
		logMsg(e)
		logMsg("Failed to open temp file '%s': please check directory permissions." % (fontPath))
		raise CBError
	try:
		ttFont = TTFont()
		# supress fontTools messages:
		stdout = sys.stdout
		stderr = sys.stderr
		sys.stdout =  supressMsg
		sys.stderr = supressMsg
		ttFont.importXML(fontPath)
		sys.stdout = stdout
		sys.stderr = stderr
		ttFont.save(fontPath) # This is now actually an OTF font file
		ttFont.close()
	except:
		import traceback
		traceback.print_exc()
	return

def getOptions():
	options = CBOptions()
	i = 1 # skip the program name.
	numOptions = len(sys.argv)
	try:
		while i < numOptions:
			arg = sys.argv[i]
	
			if arg == "-h":
				print __help__
				sys.exit(0)
			elif arg == "-u":
				print __help__
				sys.exit(0)
			elif arg == "-new":
				options.makeNewFonts = 1
			elif arg == "-srcEM":
				i += 1
				options.srcEM = eval(sys.argv[i])
			elif arg == "-dstEM":
				i += 1
				options.dstEM = eval(sys.argv[i])
			else:
				logMsg("Option Error: unknown option : '%s' ." % (arg))
				raise CBOptionParseError
			i  += 1
	except IndexError:
		logMsg("Option Error: argument '%s' must be followed by a value." % (arg))
		raise CBOptionParseError
		
	options.mmFontInfo = MMFontInfo(kMMFontInfoPath)

	return options

class MMFontInfo:
	def __init__(self, infoPath):
		try:
			fp = open(infoPath, "rt")
			data = fp.read()
			fp.close()
		except (IOError,OSError):
			logMsg( "Error: could not open file %s." % (infoPath))
			raise CBOptionParseError
		
		lines = data.splitlines()
		self.baseDirs = []
		numLines = len(lines)
		i = 0
		while i < numLines:
			line = lines[i]
			i += 1
			tokenList = line.split(None,1)
			key = tokenList[0]
			if key == "AxisLabels1":
				continue
			elif key == "PrimaryInstances":
				while 1:
					line = lines[i]
					i += 1
					tokenList = line.split(None, 1)
					key = tokenList[0]
					if key == "EndInstances":
						break
			elif key == "MasterDirs":
				while 1:
					line = lines[i]
					i += 1
					tokenList = line.split(None, 1)
					key = tokenList[0]
					if key == "EndDirs":
						break
					if key[0] != "%":
						continue
					baseDir = line.split("%")[-1].strip()
					self.baseDirs.append(baseDir)
				
			elif key:
				value = tokenList[1]
				if value.startswith("["):
					value = re.sub(r"\]\s*\[", "], [", value)
					value = re.sub(r"(-*\d+)\s+(-*\d+)", r"\1, \2", value)
				if value [0] == "(":
					value = value[1:-1]
				elif value == "false":
					value = 0
				elif value == "true":
					value = 1
				else:
					value = eval(value)
				exec("self.%s = value" % (key))

		foundAll = 1
		for bezDir in self.baseDirs:
			if not os.path.exists(bezDir):
				logMsg( "Error: Failed to find bez directory %s." % (os.path.baspath(bezDir)))
				foundAll = 0
				
		if not foundAll:
			raise CBOptionParseError


class FontEntry:
	def __init__(self, fontPath, localBezList):
		self.fontPath = fontPath
		self.bezDirs = localBezList
		self.widthsDict = None
		self.tempFontPath = None
		self.tempFontScaledPath = None
		self.ttFont = None
		self.mappingFilePath = None

class GlobalHints:
	def __init__(self):
		self.BlueValues = [-250, -250, 1100, 1100] # For Kanji glyphs, this is the default. we do NOT want x-height.capheight control/overshoot supression.
		self.OtherBlues = None
		self.StdHW = None
		self.StdVW = None
		self.StemSnapH = None
		self.StemSnapV = None

class ToolPaths:
	def __init__(self):
		try:
			self.exe_dir, fdkSharedDataDir = FDKUtils.findFDKDirs()
		except FDKUtils.FDKEnvError:
			raise FDKEnvironmentError
	
		if not os.path.exists(self.exe_dir ):
			logMsg("The FDK executable dir \"%s\" does not exist." % (self.exe_dir))
			logMsg("Please re-instal. Quitting.")
			raise FDKEnvironmentError

		toolList = ["tx", "autoHint", "IS", "mergeFonts", "stemHist"]
		missingTools = []
		for name in toolList:
			toolPath = name
			exec("self.%s = toolPath" % (name))
			command = "%s -u 2>&1" % toolPath
			pipe = os.popen(command)
			report = pipe.read()
			pipe.close()
			if ("options" not in report) and ("Option" not in report):
				print report
				print command, len(report), report
				missingTools.append(name)
		if missingTools:
			logMsg("Please re-install the FDK. The executable directory \"%s\" is missing the tools: < %s >." % (self.exe_dir, ", ". join(missingTools)))
			logMsg("or the files referenced by the shell scripts are missing.")
			raise FDKEnvironmentError


def mergeBezFiles(bezDir, widthsDict, fontPath, srcEM):
	""" 
	First, fix the ttFont's em-box, and put in useless but hopefully safe alignment zone values.
	Then, for each bez file, convert it to a T2 string, and add it to the font.
	"""
	# Collect list of bez files
	ttFont = TTFont(fontPath)
	bezFileList = []
	dirPath =  os.path.abspath(os.path.dirname(bezDir))
	list = os.listdir(bezDir)
	list = filter(lambda name: not name.endswith("BAK"), list)
	bezFileList += map(lambda name: os.path.join(bezDir, name), list)
	if not bezFileList:
		logMsg("Error. No bez files were found in '%s'. " % (bezDir))

	# Merge the bez files.
	glyphList  = ttFont.getGlyphOrder() #force loading of charstring index table
	ttFont.getGlyphID(glyphList[-1])
	cffTable =  ttFont['CFF ']
	pTopDict = cffTable.cff.topDictIndex[0]
	if srcEM == 1000:
		invEM = 0.001
		scaleFactor = 1.0
	else:
		invEM = 1.0/srcEM
		scaleFactor = srcEM/1000
	pTopDict.FontMatrix = [invEM, 0, 0, invEM, 0, 0];
	pTopDict.rawDict["FontMatrix"] = pTopDict.FontMatrix
	if not hasattr(pTopDict, "UnderlineThickness"):
		pTopDict.UnderlineThickness = 50
	pTopDict.UnderlineThickness = pTopDict.UnderlineThickness*scaleFactor
	pTopDict.rawDict["UnderlineThickness"] = pTopDict.UnderlineThickness
	if not hasattr(pTopDict, "UnderlinePosition"):
		pTopDict.UnderlinePosition = -100
	pTopDict.UnderlinePosition = pTopDict.UnderlinePosition*scaleFactor
	pTopDict.rawDict["UnderlinePosition"] = pTopDict.UnderlinePosition
	pChar = pTopDict.CharStrings
	pCharIndex = pChar.charStringsIndex
	pHmtx = ttFont['hmtx']

	for path in bezFileList:
		glyphName = os.path.basename(path)
		if (glyphName[0] == ".") and (glyphName !=" .notdef"):
			continue
		if glyphName[0]  < 32:
			continue
		# Check if glyph name has kUCSuffix. If so, we need to remove it
		# when inserting this glyph into the font. The suffix is added by BezierLab
		# to avoid conflict between UC/lc file names, which neither Mac nor Win can handle.
		finalName = glyphName
		if finalName[-kLenUC:] == kUCSuffix:
			finalName = finalName[:kLenUC]
		width = srcEM
		if widthsDict:
			try:
				width = widthsDict[glyphName] # widthsDict is keyed by bez file name, not finalName
			except KeyError:
				logMsg("Warning: bez file name %s not found in the widths.unscaled file: assigning default width.", glyphName)
		pHmtx.metrics[finalName] = [width,  0] # assign arbitrary LSB of 0
		bf = open(path, 'rb')
		bezData = bf.read()
		bf.close()
		if BezTools.needsDecryption(bezData):
			bezData = BezTools.bezDecrypt(bezData)
		t2Program = [width] + BezTools.convertBezToT2(bezData)
		newCharstring = T2CharString(program = t2Program)
		nameExists = pChar.has_key(finalName)
		# Note that since the ttFont was read from XML, its properties are somewhat different than
		# when decompiled from an OTF font file: topDict.charset is None, and the charstrings are not indexed.
		if nameExists:
			gid = pChar.charStrings[finalName]
			pCharIndex.items[gid] = newCharstring
		else:
			# update CFF
			pCharIndex.append(newCharstring)
			gid = len(pTopDict.charset)
			pChar.charStrings[finalName] = gid # haven't appended the name to charset yet.
			pTopDict.charset.append(finalName)
	# Now update the font's BBox.
	for key in ttFont.keys():
            table = ttFont[key]
	ttFont.save(fontPath) # Compile charstrings, so that charstrings array is all up to date.
	ttFont.close()

	ttFont = TTFont(fontPath)
	cffTable =  ttFont['CFF ']
	pTopDict = cffTable.cff.topDictIndex[0]
	bbox = [srcEM, srcEM, -srcEM, -srcEM ]
	glyphSet = ttFont.getGlyphSet()
	pen = BoundsPen(glyphSet)
	for glyphName in glyphSet.keys():
		glyph = glyphSet[glyphName]
		glyph.draw(pen)
		if  pen.bounds:
			x0, y0, x1,y1 = pen.bounds
			if x0 < bbox[0]:
				bbox[0] = x0
			if y0 < bbox[1]:
				bbox[1] = y0
			if x1 > bbox[2]:
				bbox[2] = x1
			if y1 > bbox[3]:
				bbox[3] = y1
	pTopDict.FontBBox = bbox
	pTopDict.rawDict["FontBBox"] = bbox
	for key in ttFont.keys():
            table = ttFont[key]
	ttFont.save(fontPath)
 	ttFont.close()
	return 

def getHintData(reportPath, dict):
	try:
		hf = file(reportPath, "rt")
		data = hf.read()
		hf.close()
	except (IOError, OSError), e:
		logMsg("Failed to open hint report '%s'. System error <%s>." % (reportPath, e))
		raise CBError
	hintList = re.findall(r"(\d+)\s+(-*\d+)\s+\[[^]\r\n]+\]", data)
	for entry in hintList:
		dict[ eval(entry[1]) ] = eval(entry[0])
	return

def cmpWidth(first, last):
	first = first[2]
	last = last[2]
	return cmp(first,last)
	
def getBestValues(stemDict, ptSizeRange, srcResolution, dstResolution):
	"""
	For each of a range of point sizes, a hint will cover +- a delta, which may include other hints.
		I want to  construct an optimal StemW hint by finding the hint which covers the largest hint count, for all
		the point sizes of intterest
		I keep the current count total in countDict[stemWdith] = currrentCount. I initially set all the values to 0.
		for each point size
			calculate the delta
			for each stem width
				add up all the counts for all the stem widths which are within +/-  delta of the stem width;
				add this to the  countDict[stemWdith]  value.
	Normalize the count values by the number of pt size we tried
	Pick the stem width the maximum count value.
	Pick all the stems with a count greater than 1 for the StemSnap array, up to a max of 12, that do not overlap with the
	main stem width
	"""
	countDict = {}
	widthList = stemDict.keys()
	for width in widthList:
		countDict[width] = 0

	maxDelta = 0
	for ptSize in ptSizeRange:
		delta =  (0.35*72*srcResolution)/(ptSize*dstResolution)
		if maxDelta < delta:
			maxDelta = delta
		for width in widthList:
			top =  int(round(width+delta))
			bottom = int(round(width-delta))
			if bottom == top:
				top +=1
			for i in range(bottom, top):
				if stemDict.has_key(i):
					countDict[width] = countDict[width] + stemDict[i]
	countList = countDict.items()			
	countList = map(lambda entry: [entry[1], stemDict[ entry[0]], entry[0]], countList)
	countList.sort()
	countList.reverse()
	# This is so  that it will sort in order by
	#	entry[0] =  total count for width, i.e. the sum of all the counts of the widths that it overlaps +/- the delta
	#	etnry[1] = the original count for the width
	#	entry[2] = the width
	bestStem = countList[0][2]
	bestStemList = [bestStem]
	numPtSizes = len(ptSizeRange)
	tempList = [countList[0]]
	for entry in countList[1:]:
		if not entry[0] > numPtSizes:
			break # Weed out any stem width that was represented only once in one glyph
		tempList.append(entry)
	# Now we need to weed out the entries that overlap
	#Sort by ascending width, and then save each succesive width only if does nto overlap with the last saved width.
	countList = tempList
	if len(tempList) > 1:
		tempList = [countList[0]] # save the most popular width
		countList = countList[1:] # remove it from the list
		countList.sort(cmpWidth)
		# Some of these may overlap  with the tolerance of +/- maxDelta. Weed out the ones that overlap
		lastWidth = countList[0][2]
		for entry in countList:
			width = entry[2]
			if ((float(width)*dstResolution)/srcResolution) < 1: # don't copy over any values that will be less than 1 in the final font.
				continue
			if width >  (lastWidth + maxDelta):
				tempList.append(entry)
				lastWidth = width
	tempList.sort() # sort them in order of decreasing popularity again.
	tempList.reverse()
	bestStemList = tempList[:12] # Allow a max of 12 StemSnap values
	bestStemList = map(lambda entry: entry[2], bestStemList)
	bestStemList.sort()
	return bestStem, bestStemList

def getNewHintInfo(toolPaths, options, fontPath, ptSizeRange, srcResolution, dstResolutiion):
	"""
	This is being called to collect new hint info from one or more fonts. From
	each temp font in turn, we collect the stem info,tracking the total count
	for each stem or zone., and extarct the best stem width and StemSnap list.
	"""
	globalHints = GlobalHints()
	hStemDict = {}
	vStemDict= {}
	extensionList = [ ".hstem.txt", ".vstem.txt"]

	logMsg( "\tDeriving new hints from temp font: ", fontPath)

	hstemPath = fontPath+".hstm.txt"
	vstemPath = fontPath+".vstm.txt"
	if os.path.exists(hstemPath):
		os.remove(hstemPath)
	if os.path.exists(vstemPath):
		os.remove(vstemPath)

	command = "%s  \"%s\" 2>&1" % ( toolPaths.stemHist, fontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	getHintData(hstemPath,  hStemDict)
	getHintData(vstemPath,  vStemDict)

	if not gDebug:
		if os.path.exists(hstemPath):
			os.remove(hstemPath)
		if os.path.exists(vstemPath):
			os.remove(vstemPath)
		
	if hStemDict:
		globalHints.StdHW, globalHints.StemSnapH = getBestValues(hStemDict, ptSizeRange, srcResolution, dstResolutiion)
	if vStemDict:
		globalHints.StdVW, globalHints.StemSnapV = getBestValues(vStemDict, ptSizeRange, srcResolution, dstResolutiion)
	return globalHints

def getOldHintInfo(toolPaths, options, fontPath):
	globalHints = GlobalHints()
	srcEM = float(options.srcEM)
	dstEM = float(options.dstEM)

	seenFont = 0
	if os.path.exists(fontPath):
		command = "%s -0 \"%s\"  2>&1" % ( toolPaths.tx, fontPath)
		pipe = os.popen(command)
		report = pipe.read()
		pipe.close()
		match = re.search(r"BlueValues\s+\{(.+?)\}", report)
		if match:
			globalHints.BlueValues = eval( "[" + match.group(1) + "]")
			globalHints.BlueValues  = map(lambda val: int(round(val*srcEM/dstEM)), globalHints.BlueValues)
		else:
			logMsg("Error. The row font '%s' did not have a BlueValues entry in its font dict." % (fontPath))
			raise CBError

		match = re.search(r"OtherBlues\s+\{(.+?)\}", report)
		if match:
			globalHints.OtherBlues = eval( "[" + match.group(1) + "]")
			globalHints.OtherBlues  = map(lambda val: int(round(val*srcEM/dstEM)), globalHints.OtherBlues)

		match = re.search(r"StdHW\s+(\d+)", report)
		if match:
			globalHints.StdHW = int(round(eval(match.group(1))) * srcEM/dstEM)

		match = re.search(r"StdVW\s+(\d+)", report)
		if match:
			globalHints.StdVW =  int(round(eval(match.group(1))) * srcEM/dstEM)

		match = re.search(r"StemSnapH\s+\{(.+?)\}", report)
		if match:
			globalHints.StemSnapH = eval( "[" + match.group(1) + "]")
			globalHints.StemSnapH  = map(lambda val: int(round(val*srcEM/dstEM)), globalHints.StemSnapH)

		match = re.search(r"StemSnapV\s+\{(.+?)\}", report)
		if match:
			globalHints.StemSnapV = eval( "[" + match.group(1) + "]")
			globalHints.StemSnapV  = map(lambda val: int(round(val*srcEM/dstEM)), globalHints.StemSnapV)
			

		seenFont = 1
		logMsg("\tTaking global hint metrics from font %s."  % (os.path.abspath(fontPath)))
		#fields = dir(globalHints)
		#fields = filter(lambda name: name[0] != "_", fields)
		#for name in fields:
		#	print "\t%s: %s" % (name, eval("globalHints.%s" % name))

	if not seenFont:
		logMsg("Could not find existing row font from which to take hint values.")
		raise CBError

	return globalHints

def openFileAsTTFont(path, txPath):
	# If input font is  CFF or PS, build a dummy ttFont in memory for use by AC.
	# return ttFont, and flag if is a real OTF font Return flag is 0 if OTF, 1 if CFF, and 2 if PS/
	fontType  = 0 # OTF
	tempPath = os.path.dirname(path)
	tempPathBase  = os.path.join(tempPath, "temp.autoHint")
	tempPathCFF = tempPathBase + ".cff"
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

		return ttFont, fontType

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
		command="%s  -cff \"%s\" \"%s\" 2>&1" % (txPath, path, tempPathCFF)
		pipe = os.popen(command)
		report = pipe.read()
		pipe.close()
		if "fatal" in report:
			logMsg("Attempted to convert font %s  from PS to a temporary CFF data file." % path)
			logMsg(report)
			raise ACFontError("Failed to convert PS font %s to a temp CFF font." % path)

	# now package the CFF font as an OTF font for use by AC.
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
		import traceback
		traceback.print_exc()
		logMsg("Attempted to read font %s  as CFF." % path)
		raise ACFontError("Error parsing font file <%s>." % fontFileName)
	return ttFont, fontType

def saveFileFromTTFont(ttFont, inputPath, outputPath, fontType, txPath):
	overwriteOriginal = 0
	if inputPath == outputPath:
		overwriteOriginal = 1
	tempPath = os.path.dirname(inputPath)
	tempPath = os.path.join(tempPath, "temp.autoHint")

	if fontType == 0: # OTF
		if overwriteOriginal:
			ttFont.save(tempPath)
			shutil.copyfile(tempPath, inputPath)
		else:
			ttFont.save(outputPath)
		ttFont.close()

	else:		
		data = ttFont["CFF "].compile(ttFont)
		if fontType == 1: # CFF
			if overwriteOriginal:
				tf = file(tempPath, "wb")
				tf.write(data)
				tf.close()
				shutil.copyfile(tempPath, inputPath)
			else:
				tf = file(outputPath, "wb")
				tf.write(data)
				tf.close()

		elif  fontType == 2: # PS.
			tf = file(tempPath, "wb")
			tf.write(data)
			tf.close()
			if overwriteOriginal:
				command="%s  -t1 \"%s\" \"%s\" 2>&1" % (txPath, tempPath, inputPath)
			else:
				command="%s  -t1 \"%s\" \"%s\" 2>&1" % (txPath, tempPath, outputPath)
			pipe = os.popen(command)
			report = pipe.read()
			pipe.close()
			logMsg(report)
			if "fatal" in report:
				raise IOError("Failed to convert hinted font temp file with tx %s" % tempPath)
			if overwriteOriginal:
				os.remove(tempPath)
			# remove temp file left over from openFile.
			os.remove(tempPath + ".cff")

def addHints(globalHints, fontPath, txPath):
	ttFont, fontType = openFileAsTTFont(fontPath, txPath)
	privateDict = ttFont['CFF '].cff.topDictIndex[0].Private
	fields = dir(globalHints)
	fields = filter(lambda name: name[0] != "_", fields)
	for name in fields:
		val =  eval("globalHints.%s" % name)
		#print name,val, eval("privateDict.%s" % (name)
		if val != None:
			exec("privateDict.%s = val" % (name))
		elif eval("hasattr(privateDict, \"%s\")" % name):
			exec("del privateDict.%s" % name)
	saveFileFromTTFont( ttFont, fontPath, fontPath, fontType, txPath)
	return 

def scaleFont(toolPaths,  tempFontPath, tempFontScaledPath, dstEM):
	logMsg("\tScaling font %s..." % (tempFontPath)) 
	command = "%s -t1 -z  %s  \"%s\" \"%s\" 2>&1" % (toolPaths.IS, dstEM, tempFontPath,  tempFontScaledPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	print report
	if  ("fatal" in report) or ("error" in report):
		logMsg(report)
		logMsg("Error  scaling temp font '%s'." % tempFontPath)
		raise CBError
	return 

def makeMergeGAFile(fontPath, toolPaths, bezFontPath):
	gaPath = fontPath + ".ga.txt"
	gaBezPath = bezFontPath + ".ga.txt"
	command = "%s -mtx \"%s\"  2>&1" % (toolPaths.tx, fontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	if  ("fatal" in report) or ("error" in report):
		logMsg(report)
		logMsg("Error in using tx program to extract glyph names from  font '%s'." % (fontPath))
		raise MergeFontError
	fontNameList = re.findall(r"glyph\[\d+\]\s+{([^,]+),", report)

	command = "%s -mtx \"%s\"  2>&1" % (toolPaths.tx, bezFontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	if  ("fatal" in report) or ("error" in report):
		logMsg(report)
		logMsg("Error in using tx program to extract glyph names from  font '%s'." % (bezFontPath))
		raise MergeFontError
	bezNameList = re.findall(r"glyph\[\d+\]\s+{([^,]+),", report)
	bezNameList = filter(lambda name: name != ".notdef", bezNameList)
	# Don't use the .notdef from the bez temp file; this ensures that the fontNameList will
	# have at least one glyph name - .notdef.
	fontNameList = filter(lambda name: not name in bezNameList, fontNameList)

	# Note that we omit the FontName and Language Group values from the header line;
	# the srcFontPath is a name-keyed font.
	lineList = map(lambda name: "%s\t%s" % (name,name), fontNameList)
	gaText = "mergeFonts" + os.linesep + os.linesep.join(lineList) + os.linesep
	gf = file(gaPath, "wb")
	gf.write(gaText)
	gf.close()

	lineList = map(lambda name: "%s\t%s" % (name,name), bezNameList)
	gaText = "mergeFonts" + os.linesep + os.linesep.join(lineList) + os.linesep
	gf = file(gaBezPath, "wb")
	gf.write(gaText)
	gf.close()
	
	return gaPath,gaBezPath

def mergeTempFont(toolPaths, makeNewFonts, tempFont, fontPath):

	if  makeNewFonts:
		logMsg("\tCopying temp font %s to %s..." % (tempFont, fontPath))
	else:
		logMsg("\tMerging temp font %s into %s..." % (tempFont, fontPath))
		
		# merge  temp font into existing font rather than just copying it.
		# First convert to t1.
		tempFont2 = tempFont + "2"
		command = "%s  -t1 \"%s\" \"%s\" 2>&1" % (toolPaths.tx,  tempFont,  tempFont2)
		pipe = os.popen(command)
		report = pipe.read()
		pipe.close()
		os.rename(tempFont2, tempFont)
		# We need to filter out from the existing font all the glyph names
		# that conflict with the new bez glyphs, as well as the .notdef.
		# For merging files, all input files must be of the same type.
		gaPath, gaBezPath = makeMergeGAFile(fontPath, toolPaths, tempFont)
		command = "%s  \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" 2>&1" % (toolPaths.mergeFonts, tempFont,
                                                    gaPath, fontPath,  gaBezPath, tempFont)
		pipe = os.popen(command)
		report = pipe.read()
		pipe.close()
		print report
		if  ("fatal" in report) or  ("error" in report):
			logMsg(report)
			logMsg("Error  merging  temp font '%s' into final font '%s'." %  (tempFont, fontPath))
			raise CBError
		if not gDebug:
                    os.remove(gaPath)
                    os.remove(gaBezPath)
                    
	# copy temp font to final output.
	command = "%s  -t1 \"%s\" \"%s\" 2>&1" % (toolPaths.tx,  tempFont,  fontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	logMsg( "Report for",  fontPath, report)
	if  ("fatal" in report) or ("error" in report):
		logMsg(report)
		logMsg("Error  merging  temp font '%s' into final font '%s'." %  (tempFont, fontPath))
		raise CBError
	if not os.path.exists(fontPath):
		logMsg( "Error making final font: ", fontPath, report)

	return 

def stripPathNameList(entry):
	# Each entry has as many items as there are faces.
	# each item is a PathEntry item.
	 
	 # I return an entry looking like [pathIndex, pathOpLists]
	 # where pathOpLists is list of pathOpList entries, one for each face in this path.
	pathList =  [entry[0].pathIndex, map(lambda pathEntry: pathEntry.pathOpList, entry)]
	return pathList
	
kHintOps = ["hstem","vstem", "vstem3", "hstem3", "hintmask", "callsubr", "div"]
khintBlockName = "hintBlock"
kCallSubrDict = { 1 : "5 callsubr",
				2 : "6 callsubr",
				4 : "8 callsubr",
				6 : "9 callsubr",
				}
				
def parseCharString(charEntry):
	glyphName = charEntry[0]
	data = charEntry[1]
	tokenList = data.split()
	numTokens = len(tokenList)
	i = 0
	opList = []
	argList = []
	hintBlock = []
	while i < numTokens:
		token = tokenList[i]
		i += 1
		if re.match(r"[-.0-9]+", token):
			argList.append(eval(token))
		elif token in kHintOps:
			hintBlock.append([argList, token])
			argList = []
		else:
			if hintBlock:
				opList.append([hintBlock, khintBlockName])
				hintBlock = []
			opList.append([argList, token])
			argList = []
	if argList:
		logMsg( "Error parsing charstring for %s." % (glyphName))
		raise CBError
	return [glyphName, opList]
	
class PathEntry:
	def __init__(self, startPoint, endPoint, pathOpList, pathNameList, glypBBox, glyphName, pathIndex):
		self.startPoint = startPoint
		self.endPoint = endPoint
		self.pathOpList = pathOpList
		self.opNameList = pathNameList
		self.pathIndex = pathIndex
		self.numOps = len(pathNameList)
		self.glypBBox = glypBBox
		self.scaledList = None
		self.glyphName = glyphName
		self.needsLineToFixList = [] # List of paths which match this if one unit line-to is removed.
		self.degenerateCurveList = [] # List of paths which match this if a degenerate curve is removed.
		self.needsFinalLineList = [] # List of paths which match this if final line to is removed
		self.assigned = None # whether this path has already been matched to another.
		
		if pathIndex != 0: # First path is always the hswb command
			if not pathNameList[0][1] != "rmoveto":
				self.opNameList = ["rmoveto"] + self.opNameList
				pathOpList = [[[0,0], "rmoveto"]] + self.pathOpList
				self.numOps = len(self.opNameList)
	
			if not pathNameList[-1][1] != "cp":
				self.opNameList.append("cp")
				self.pathOpList.append([[0,0], "cp"])
				self.numOps = len(self.opNameList)
			
		
	def getScaledCoords(self):
		if not self.scaledList:
			xDiff = self.glypBBox[2] - self.glypBBox[0]
			yDiff = self. glypBBox[3] - self.glypBBox[1]
			if xDiff == 0:
				xDiff = 1.0
			else:
				xDiff = float(xDiff)
			if yDiff == 0:
				yDiff = 1.0
			else:
				yDiff = float(yDiff)
			scaledList = []
			for entry in self.pathOpList:
				if not entry[0]:
					continue
				coords = entry[0][-2:]
				scaledList.append([ coords[0]/xDiff, coords[1]/yDiff])
			self.scaledList = scaledList
		return self.scaledList

	def __repr__(self):
		return "index %s, assigned %s, start %s, scaledStartPoint %s opNameList %s" % ( self.pathIndex, self.assigned, self.startPoint, self.scaledStartPoint, self.opNameList, )
	
def countNonHintOps(opList):
	opList = filter(lambda entry: entry[1] != khintBlockName, opList)
	return len(opList)
	
def fixPathEnd(glyphName, pathIndex, fontIndex, oldCurPos, curPos, pathOpList, pathNameList):
		
	"""
	If the first or last marking operation was a one unit line rlineto, then remove it.
	"""
	if pathOpList[-1][1] == "closepath":
		lastOpIndex = -2
	else:
		if pathOpList[-1][1] == "hsbw":
			return
		else:
			logMsg( "Error: Path without closepath: %s." % (pathOpList))
			raise CBFontError

	if pathOpList[0][1] == "rmoveto":
		firstOpIndex = 1
	else:
		firstOpIndex = 0


	lastOpEntry = pathOpList[lastOpIndex]
	firstOpEntry = pathOpList[firstOpIndex]
	if firstOpEntry[1] == "rlineto":
		p_args = firstOpEntry[0]
		if (abs(p_args[0]) < 2) and (abs(p_args[1]) < 2):
			if firstOpIndex == 0:
				pathNameList[0] = "rmoveto"
				pathOpList[0][1] = "rmoveto"
			else:
				del pathNameList[firstOpIndex]
				del pathOpList[firstOpIndex]
				args0 = pathOpList[0][0]
				args0[0] += p_args[0]
				args0[1] += p_args[1]
			logMsg( "Note: in glyph %s path %s face %s, at pos %s, removing one unit implicit rlineto at start of path" % (glyphName, pathIndex, fontIndex, curPos))
	elif lastOpEntry[1] == "rlineto":
		p_args = lastOpEntry[0]
		if (abs(p_args[0]) < 2) and (abs(p_args[1]) < 2):
			del pathNameList[lastOpIndex]
			del pathOpList[lastOpIndex]
			args0 = pathOpList[lastOpIndex-1][0]
			args0[0] += p_args[0]
			args0[1] += p_args[1]
			logMsg( "Note: in glyph %s path %s face %s, at pos %s, removing one unit implicit rlineto at end of path" % (glyphName, pathIndex, fontIndex, curPos))

def findOpNameMatches(pathEntry, targetPathList):
	matchPathOpNameList = pathEntry[1].markingOpNameList
	matchIndexList = []
	for tPathEntry in targetPathList:
		if tPathEntry[1].markingOpNameList == matchPathOpNameList:
			matchIndexList.append(tPathEntry[0])
	return tuple(matchIndexList)


def isDegenerateCurve(args):
	zeroArgsCount = 0
	largeArgsCount = 0
	
	for i in [0,2,4]:
		if (abs(args[i])< 2) and (abs(args[i+1]) < 2):
			if (args[i] == 0) and (args[i+1] == 0):
				zeroArgsCount += 1
		else:
			largeArgsCount += 1
			newArgs = [ args[i], args[i+1] ]
			
	if largeArgsCount == 1:
		return newArgs
	else:
		return 0

def matchPathToCurve(targetPath, cPath):
	if  targetPath.numOps != cPath.numOps:
		return 0
		
	matched = 1
	i1 = 0
	while i1 < targetPath.numOps:
		tOpEntry = targetPath.pathOpList[i1]
		cOpEntry = cPath.pathOpList[i1]
		i1 += 1
		tName = tOpEntry[1] 
		if tName == cOpEntry[1]:
			continue
			
		cName = cOpEntry[1]
		if (tName == "rlineto") and (cName == "rrcurveto"):
			tOpEntry[1] = "rrcurveto"
			tOpEntry[0] =  [0, 0] + tOpEntry[0] + [0, 0]
		elif (tName == "rrcurveto") and (cName == "rlineto"):
			cOpEntry[1] = "rrcurveto"
			#cOpEntry[0] = [ 0, 0, 0, 0] + tOpEntry[0]
			cOpEntry[0] = [0, 0] + cOpEntry[0] + [0, 0]
		else:
			continue
			
	return 	matched	

def matchPathDegnerateCurve(targetPath, cPath, doFix):
	if  targetPath.numOps != cPath.numOps:
		return 0

	return matchSubPathDegnerateCurve(targetPath, cPath, doFix, 0, 0,  targetPath.numOps)
	
def matchSubPathDegnerateCurve(targetPath, cPath, doFix, startIndex1, startIndex2, numOps):
	# In some cases, the ai artwork contains a degenerate cuve that looks like:
	# x y 0 0 0 0 rrcurveto, the corresponds to a line-to in the other base designs.
		
	matched = 1
	i1 = startIndex1
	i2 = startIndex2
	endIndex = i1 + numOps
	while i1 < endIndex:
		tOpEntry = targetPath.pathOpList[i1]
		cOpEntry = cPath.pathOpList[i2]
		i1 += 1
		i2 += 1
		tName = tOpEntry[1] 
		if tName == cOpEntry[1]:
			continue
			
		cName = cOpEntry[1]
		if (tName == "rlineto") and (cName == "rrcurveto"):
			args = cOpEntry[0]
			fixPath = cPath
			otherPath = targetPath
			iFix = i2-1
		elif (tName == "rrcurveto") and (cName == "rlineto"):
			args = tOpEntry[0]
			fixPath = targetPath
			otherPath = cPath
			iFix = i1-1
		else:
			continue
		
		newArgs = isDegenerateCurve(args)
		if newArgs:
			if doFix:
				fixPath.pathOpList[iFix] = [ newArgs, "rlineto"]
				matched = 2
			else:
				fixPath.degenerateCurveList.append(otherPath.pathIndex)
		else:
			matched = 0
			break
		
			
	return 	matched	
			
def matchReversedCurve(targetPath, 	cPath, doFix):
	# One path has one op more than the other. Check if the pahts are identical except for the last op,
	# and the last op is a lineto that closes the path.  When ai2bex reverses a contour, it does not
	# add a linto to close a path, but TWB does.
	
	if targetPath.numOps > cPath.numOps:
		longP = targetPath
		shortP = cPath
	else:
		longP = cPath
		shortP = targetPath
		
	if 	longP.startPoint != longP.endPoint:
		return 0

	shortStartIndex = 0
	longStartIndex = 0
	numOps = shortP.numOps -1
	if not matchSubPathDegnerateCurve(shortP, longP, doFix, shortStartIndex, longStartIndex, numOps):
		return 0
	
	# Last marking op must be a rlineto, or a degenerate curve.
	lastMarkingOpEntry = longP.pathOpList[longP.numOps-2]
	if lastMarkingOpEntry[1] != "rlineto":
		args = lastMarkingOpEntry[0]
		if not isDegenerateCurve(args):
			return 0
		
	if doFix:
		longP.endPoint[0] -= longP.pathOpList[longP.numOps-2][0][0]
		longP.endPoint[1] -= longP.pathOpList[longP.numOps-2][0][1]
		del longP.pathOpList[longP.numOps-2]
		del longP.opNameList[longP.numOps-2]
	else:
		longP.needsFinalLineList.append(shortP.pathIndex)
	return 1
		

def match1UnitLineto(tPathEntry, cPathEntry, doFix):
	if tPathEntry.numOps > cPathEntry.numOps:
		longP = tPathEntry
		shortP = cPathEntry
	else:
		longP = cPathEntry
		shortP = tPathEntry
	
	# Paths must match for all ops except the first or last marking op.
	if  matchSubPathDegnerateCurve(shortP, longP, doFix, 0, 0, shortP.numOps -1):
		opIndex = -2
		opEntry = longP.pathOpList[-2]
	elif   matchSubPathDegnerateCurve(shortP, longP, doFix, 1, 2, shortP.numOps -2):
		opIndex = 1
		opEntry = longP.pathOpList[1]
	else:
		return 0
	
	if 	longP.startPoint != longP.endPoint:
		return 0

	# Last/first marking op must be a rlineto, or a degenerate curve.
	if opEntry[1] != "rlineto":
		args = opEntry[0]
		args = isDegenerateCurve(args)
		if not args:
			return 0
	else:
		args = opEntry[0]
	
	if (abs(args[0]) > 1) or (abs(args[1]) > 1):
		return 0
	
	if doFix:
		del longP.pathOpList[opIndex]
		del longP.opNameList[opIndex]
		prevArgs = longP.pathOpList[opIndex-1][0]
		prevArgs[-2] += args[0]
		prevArgs[-1] += args[1]
		if opIndex == -2:
			longP.endPoint[0] -= args[0]
			longP.endPoint[1] -= args[0]

	else:
		longP.needsLineToFixList.append(shortP.pathIndex)
	return 1
		

def selectmatchFromCandidates(tPathEntry, candidateList, glyphName, targetPathIndex, faceIndex):
	# We have to choose between several candidates.For each candidate, step through the nodes, 
	# calculating and summing the distance between the target and candidate nodes. Node positions
	# are first scaled by glyph BBox.
	
	sortList = []
	targetList = tPathEntry.getScaledCoords()
	numTargetCoords = len(targetList)
	for pathEntry in candidateList:
		candidateList = pathEntry.getScaledCoords()
		numCoords = min(numTargetCoords, len(candidateList))
		dist = 0.0
		for i in range(numCoords):
			tPos = targetList[i]
			cPos = candidateList[i]
			dist += math.sqrt( (cPos[0] - tPos[0])**2 + (cPos[1] - tPos[1])**2 )
		sortList.append([dist, pathEntry.pathIndex, pathEntry])
	sortList.sort()
	
	# Weed out all candiates that are far apart from the closest
	# Anything that is close - I chose 5% as a very arbitrary definition of "close"
	# might actually be the correct one. instead of the closest - no way to tell for sure.
	matchDist = sortList[0][0]
	sortList = filter(lambda entry: abs(matchDist - entry[0]) < 0.05, sortList)
	if len(sortList) > 1:
		warnList = map(lambda entry: entry[1], sortList)
		logMsg("Warning: glyph %s. Path %s in base design 0 matched several paths %s in base design %s." %  (glyphName, targetPathIndex, warnList, faceIndex))
		
	pathEntry = sortList[0][-1]
	
	return pathEntry
	
def buildMatchList(pathLists, options, dirList, glyphName):

	targetPathList = pathLists[0]
	numPaths = len(targetPathList)
	numFaces = len(pathLists)
	matchPathLists = pathLists[1:]
	faceIndex = 0
	haveError = 0
	matchList = range(numPaths)
	matchList = map(lambda entry: [None]*numFaces, matchList)
	for targetPathIndex in range(numPaths):
		tPathEntry = targetPathList[targetPathIndex]
		targetPathMatchEntry = matchList[targetPathIndex]
		targetPathMatchEntry[0] = tPathEntry

		for faceIndex in range(1,numFaces):
			pathList = pathLists[faceIndex]
			pathEntry = pathList[targetPathIndex]
			
			if tPathEntry.numOps != pathEntry.numOps:
				pathEntry = tPathEntry
				logMsg("Error: glyph %s. Could not match  path %s. Using only path from base design 0." %  (glyphName, targetPathIndex))
			elif tPathEntry.opNameList != pathEntry.opNameList:	
				retVal =  matchPathToCurve(tPathEntry, pathEntry)
				if retVal == 0:
					pathEntry = tPathEntry
					logMsg("Error: glyph %s. Could not match  path %s. Using only path from base design 0." %  (glyphName, targetPathIndex))
				else:
					logMsg("Note: glyph %s. Had to convert line to curve in path %s." %  (glyphName, targetPathIndex))
				

			if tPathEntry.numOps != pathEntry.numOps:
				match1UnitLineto(tPathEntry, pathEntry, 1)
			targetPathMatchEntry[faceIndex] =  pathEntry
			pathEntry.assigned = tPathEntry.pathIndex
			
			continue
		
				
		# matchList now contains the matching set of paths.		
					
				
	if haveError:
		raise CBFontError

	return matchList
	
def buildCharString(charSetEntry, bboxDict, options, dirList):
	#  bboxDict is a dict by glyph name mapping to the list of bbox's for the charstrings in base design order.
	# charSetEntry is a list of charstrings in base design order. [glyphName, opList]
	# where each opList is a list of entries[ [argList, opName]
	
	glyphName = charSetEntry[0][0]
	bboxList = bboxDict[glyphName]
	logMsg( "Building charstring for", glyphName)
	
	numFaces = len(charSetEntry)
	hadError = 0
	for entry in charSetEntry[1:]:
		if entry[0] != glyphName:
			logMsg( "Error: Glyph names don't match at %s vs %s." % (glyphName, entry[0]))
			hadError =1
			continue
		opList = entry[1]
		
	if hadError:
		raise CBFontError
	opLists = map(lambda entry: entry[1], charSetEntry) 
	
	# We first need to standardiize the path order. Build a list of paths ["sort-key", pathOpList"
	# Since paths in a glyph can be in different order in the different faces, we need to match them up.
	# 1) convert ops to more std versions, as a vlineto in one may be a rlineto in another
	# 2) get rid of 1 unit vlineto and hlineto's from MM source, which may be present in one but not the other
	# 3) close all paths with a lineto
	# 4) sort the face paths. In order to make them match, each entry will consist of
	#     [number of marking ops, marking op list, initial moveto coords, actual oplist, initial path order]
	# Note that we keep just the absolute moveto coords at this point, and leave out the moveto op;
	# we add  this  bac kin  in later, after sorting by the absolute coords.
	#
	pathLists = []
	pathIndex = 0
	fontIndex = -1
	for opList in opLists:
		fontIndex += 1
		pathIndex = 0
		bbox = bboxList[fontIndex]
		xMax = float(bbox[2]) - float(bbox[0]) # These are iused to normalize the path start positions by the size of the BBox.
		yMax = float(bbox[3]) - float(bbox[1])
		if yMax == 0.0:
			yMax = 1.0
		if xMax == 0.0:
			xMax = 1.0
		pathList = []
		pathOpList = []
		pathNameList = []
		curPos = [0,0]
		startPoint = copy.copy(curPos)
		for entry in opList:
			opName = entry[1]
			argList = entry[0]
			# Start a new path withe each 'moveto. If the preceeding op was a hint block, remove it from the previous path op list add it to the new path op list.
			if opName in ["hmoveto", "vmoveto", "rmoveto"]:
				pathEntry = PathEntry(startPoint, copy.copy(curPos), pathOpList, pathNameList, bboxList[fontIndex], glyphName, pathIndex)
				pathList.append( pathEntry)
				pathIndex +=1 
				pathOpList = []
				pathNameList = []
				if opName == "hmoveto":
					curPos[0] += argList[-1]
					argList2 = [argList[-1], 0]
					#print "hmoveto", curPos, argList[-1]
				elif opName == "vmoveto":
					curPos[1] += argList[-1]
					argList2 = [0, argList[-1]]
					#print "vmoveto", curPos, argList[-1]
				elif opName == "rmoveto":
					curPos[0] += argList[-2]
					curPos[1] += argList[-1]
					argList2 = copy.copy(argList)
					#print "rmoveto", curPos, argList[-2:]
				startPoint = copy.copy(curPos)
				pathOpList.append([argList2, "rmoveto"])	
				pathNameList.append("rmoveto")
			elif opName == "hlineto":
				pathOpList.append( [argList + [0], "rlineto"])
				pathNameList.append("rlineto")
				curPos[0] += argList[-1]
				#print curPos, argList,opName,  pathOpList[-1]
			elif opName == "vlineto":
				pathOpList.append( [[0]+ argList, "rlineto"])
				pathNameList.append("rlineto")
				curPos[1] += argList[-1]
				#print curPos, argList,opName,  pathOpList[-1]
			elif  opName == "rlineto":
				pathOpList.append( [argList, "rlineto"])
				pathNameList.append("rlineto")
				curPos[0] += argList[-2]
				curPos[1] += argList[-1]
				#print curPos, argList,opName,  pathOpList[-1]
			elif opName == "vhcurveto":
				pathOpList.append( [ [0] + argList + [0], "rrcurveto"])
				pathNameList.append("rrcurveto")
				curPos[0] += argList[1] + argList[3]
				curPos[1] += argList[0] + argList[2]
				#print curPos, argList,opName,  pathOpList[-1]
			elif opName == "hvcurveto":
				pathOpList.append( [ [argList[0], 0] + argList[1:-1] + [0, argList[-1]], "rrcurveto"])
				pathNameList.append("rrcurveto")
				curPos[0] += argList[0] + argList[1] 
				curPos[1] += argList[2] + argList[3]
				#print curPos, argList,opName,  pathOpList[-1]
			elif  opName == "rrcurveto":
				pathOpList.append( [argList, opName])
				pathNameList.append("rrcurveto")
				curPos[0] += argList[0] + argList[2]  + argList[4] 
				curPos[1] += argList[1] + argList[3]  + argList[5] 
				#print curPos, argList,opName,  pathOpList[-1]
			elif  opName == "endchar":
				continue
			elif  opName == khintBlockName:
				continue
			elif  opName in ["moveto", "curveto", "lineto"]:
				logMsg( "Error: in glyphs %s, the absolute positioning op is not supprted: %s." % (glyphName, opName))
				raise CBFontError
			else:
				pathOpList.append( [argList, opName])
				if opName != khintBlockName:
					pathNameList.append(opName)
				
		if pathOpList:
			pathEntry = PathEntry(startPoint,  copy.copy(curPos), pathOpList, pathNameList, bboxList[fontIndex], glyphName, pathIndex)
			pathList.append(pathEntry)
			pathLists.append(pathList)
			
	
	numPaths0 = len(pathLists[0])
	for pathList in pathLists[1:]:
		if len(pathList) != numPaths0:
			logMsg( "Error: in glyphs %s, the number of paths differ in the different faces: %s." % (glyphName, [ len(pathList) for pathList in pathLists]))
			raise CBFontError
	# At this point, pathLists is a two entry list. Each entry is the list of path entries for one base design.
	matchedList = buildMatchList(pathLists, options, dirList, glyphName)

	# Let's group by path number, then face, and sort by the base designs curPos.
	pathLists = map(stripPathNameList, matchedList)
		
	
	# Each entry now has all the data for all the faces for a single path, so path entry has as many items as there are faces.
	# each path entry is [pathIindex for first face, pathOpList], Sort to restore the original path order.
	pathLists.sort()
	pathLists = map(lambda entry: entry[1], pathLists) # Reduce items from [pathIindex for first face, pathOpList] to just pathOpList.
	# pathLists[2][1] is the third path in the second face.
	
	# Now the opLists all match. Let's make the charstring data.
	pn = -1
	data = []
	for opLists in pathLists:
		pn += 1
		opIndex = -1
		while 1: # I don't want to index by oplist index, as I may be adding new hints to one or the other list.
			# Check if any of the faces have a hint block at this index.
			opIndex += 1
			argLists = []
			if opIndex >= len(opLists[0]):
				break
			for opList in opLists:
				opName = opList[opIndex][1]
				argList = opList[opIndex][0]
				if argList:
					argLists.append(argList)

			if argLists:
				argList0 = argLists[0]
				numArgs = len(argList0)
				opString = [ "%s" % (val) for val in argList0]
				allSame = 1
				for argList in argLists[1:]:
					if argList != argList0:
						allSame = 0
					opString.extend(["%s" % (argList[i] - argList0[i]) for i in range(numArgs)])
				if allSame:
					opString = [ "%s" % (val) for val in argList0]
					opString = "%s %s" % (" ".join(opString),  opName)
				else:
					opString = "%s %s %s" % (" ".join(opString),  kCallSubrDict[numArgs], opName)
			else:
				opString = " %s" % (opName)
			data.append(opString)
			
	data = " ".join(data)

	return [glyphName, data]
	

def getFontData(fontPath):
	command = "detype1  \"%s\"  2>&1" % (fontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	
	bboxMatch = re.search(r"/FontBBox\s+\{([^}]+)\}", report)
	if not bboxMatch:
		logMsg("Error: failed to get FontBBox info from base design font '%s'." % (fontPath))
		raise CBFontError
	fontBBox = bboxMatch.group(1).split()
	
	StdHWMatch =  re.search(r"/StdHW\s+\[([^\]]+)\]", report)
	if not StdHWMatch:
		logMsg("Error: failed to get StdHW info from base design font '%s'." % (fontPath))
		raise CBFontError
	stdHWValues = StdHWMatch.group(1).split()
	
	StdVWMatch =  re.search(r"/StdVW\s+\[([^\]]+)\]", report)
	if not StdVWMatch:
		logMsg("Error: failed to get StdVW info from base design font '%s'." % (fontPath))
		raise CBFontError
	stdVWValues = StdVWMatch.group(1).split()
	
	charList = re.findall(r"[\r\n]/(\S+) ## -\| \{([^}]+)\} \|-", report)
	charList = map(parseCharString, charList)
	
	command = "tx -mtx  \"%s\"  2>&1" % (fontPath)
	pipe = os.popen(command)
	report = pipe.read()
	pipe.close()
	if "glyph" not in report:
		logMsg("Error: failed to get bbox from tx mtx dump otf '%s'." % (fontPath))
		logMsg(report)
		raise CBFontError
	mtxList = re.findall(r"[\r\n]glyph[^{]+{([^,]+),[^{]+{([^,]+),([^,]+),([^,]+),([^}]+)}", report)
	
	
	
	return fontBBox, stdHWValues, stdVWValues, charList, mtxList

	
def run():
	global gDebug
	try:
		options = getOptions()
		paths = ToolPaths()
		# Get the list of input bez directories and output font files.
		baseDirList = options.mmFontInfo.baseDirs
		if not baseDirList:
			logMsg("Error. Failed to read base font dir names from mmfontinfo file.")
			raise CBError
			
		bezDirList = map(lambda dirName: os.path.join(dirName, kSrcBezDir), baseDirList)
		fontList = map(lambda dirName: os.path.join(dirName, kDstFileName), baseDirList)
		allFontsExist = 1
		if not options.makeNewFonts:
			for fontPath in fontList:
				if not os.path.exists(fontPath):
					allFontsExist = 0
					options.makeNewFonts = 1
					print "Missing font %s." % (fontPath)
		else:
			allFontsExist = 0
		
		numBaseDirs = len(baseDirList)
		
		supressMsg = SupressMsg()
		if not allFontsExist:
			logMsg( "Building single-master font.pfa files from the bez files...")
			logMsg( os.linesep + "Building  temporary fonts..." + time.asctime())
			for i in range(numBaseDirs):
				# Build temp OTF fonts to hold the bez file data
				bezDir = bezDirList[i]
				fontPath = fontList[i]
				tempFontPath = fontPath + ".tmp.otf"
				tempFontScaledPath =  fontPath + ".tmp.scaled.ps"
				makeTempFont(tempFontPath, supressMsg)
				widthsDict = {}
				mergeBezFiles(bezDir, widthsDict, tempFontPath, options.srcEM)
				logMsg("\tSaved temp font file '%s'..." % (tempFontPath))
	
	
				if options.dstEM != options.srcEM:
					useScaledFiles = 1
					# Before applying IS to scale the bez files,, we need at derive global hints and 
					# apply AC.
					
					# Get and add global hint info use StemHist to get
					# aligment and stem zones from all the new glyphs, and
					# build new global hints.
					logMsg("\tAnalyzing glyphs to derive global hinting metrics  for intelligent scaling...")
					ptSizeRange = [72]
					srcResolution = options.srcEM
					dstResolutiion = options.dstEM
					globalHints = getNewHintInfo(paths, options,  tempFontPath, ptSizeRange, srcResolution, dstResolutiion)
				
					addHints(globalHints, tempFontPath, paths.tx)
					
					# Hint the font with ACs, and use IS to scale them
					logMsg("\tScaling temp font...")
					scaleFont(paths, tempFontPath, tempFontScaledPath, options.dstEM)
					tempFontPath2 = tempFontScaledPath
				else:
					useScaledFiles = 0
					tempFontPath2 = tempFontPath
					
				# We now need to set the global hint info for the (possibly scaled) tempfont, and hint it.
				if options.makeNewFonts:
					# we need to derive noe global hint info. We do this a little differently than for IS.
					logMsg("\tAnalyzing glyphs to derive global hinting metric...")
					ptSizeRange = range(6,24)
					srcResolution = options.dstEM
					dstResolutiion = 600 # a typical printing dpi now-days.
					globalHints = getNewHintInfo(paths, options,  tempFontPath2, ptSizeRange, srcResolution, dstResolutiion)
				else:
					globalHints = getOldHintInfo(paths, options, fontPath)
				addHints(globalHints, tempFontPath2, paths.tx)
					
				# Merge the data into the font files.
				mergeTempFont(paths, options.makeNewFonts, tempFontPath2, fontPath)
	
				if not gDebug:
					if os.path.exists(tempFontScaledPath):
						os.remove(tempFontScaledPath)
					if os.path.exists(tempFontPath):
						os.remove(tempFontPath)

	except(FDKEnvironmentError, CBOptionParseError, CBError):
		logMsg("Fatal error - all done.")
		return
	
	# Base Designs are now all done. Convert to an MM font
	# Detype1 the single master fonts
	# Set the name stuff from mmfontinfo
	# Set the private dict stuff from the single-master fonts.
	# Truncate arrays to shortest length.
	fontDataList = []
	x1List = []
	y1List = []
	x2List = []
	y2List = []
	stdHWList = []
	stdVWList = []
	minStdHWLen = 1000
	minStdHVLen = 1000
	
	charSet = []
	bboxSet = []
	for fontPath in fontList:
		fontBBox, stdHWValues, stdVWValues, charList, bboxList = getFontData(fontPath)
		fontDataList.append([ fontBBox, stdHWValues, stdVWValues])
		charSet.append(charList)
		bboxSet.append(bboxList)
		x1List.append(fontBBox[0])
		y1List.append(fontBBox[1])
		x2List.append(fontBBox[2])
		y2List.append(fontBBox[3])
		minStdHWLen = min(minStdHWLen, len(stdHWValues))
		minStdHVLen = min(minStdHVLen, len(stdVWValues))
	for i in range(minStdHWLen):
		for entry in fontDataList:
			stdHWValues = entry[1][:minStdHWLen]
			stdHWList.append(stdHWValues[i])
			
	for i in range(minStdHWLen):
		for entry in fontDataList:
			stdVWValues = entry[2][:minStdHVLen]
			stdVWList.append(stdVWValues[i])



	lineList = []
	line = kHeader1 + options.mmFontInfo.FontName + kHeader2
	lineList.append(line)
	
	# Add font name info
	line = kTopDict % (options.mmFontInfo.FullName, # Full Name
					options.mmFontInfo.FamilyName, # FamilyName
					options.mmFontInfo.FontName # FontName
					)
	lineList.append(line)
	
	# Add FontBBox info
	lightBBox = fontDataList[0][0]
	line = kFontBox % (" ".join(lightBBox), " ".join(x1List), " ".join(y1List), " ".join(x2List), " ".join(y2List) )
	lineList.append(line)

	lineList.append(kStdDefs1)

	# Add hint info
	StdHW = fontDataList[0][1][:minStdHWLen]
	StdHV = fontDataList[0][2][:minStdHVLen]
	line = kHintValues % (" ".join(StdHW), " ".join(StdHV),  " ".join(stdHWList), " ".join(stdVWList) )
	lineList.append(line)
	
	lineList.append(kStdDef2)
	
	charSet = apply(zip, charSet)
	bboxSet =  apply(zip, bboxSet)
	bboxDict = {}
	for entry in bboxSet:
		bboxDataList = []
		bboxDict[ entry[0][0] ] = bboxDataList
		for faceEntry in entry:
			bboxValues = map(lambda value: eval(value), faceEntry[1:])
			bboxDataList.append(bboxValues)

	mmCharSet = []
	numBuilt = 0
	numFailed = 0
	for charSetEntry in charSet:
		try:
			#if charSetEntry[0][0] != "r6D.c2C":
			#	continue
				
			mmEntry = buildCharString(charSetEntry, bboxDict, options, baseDirList) # This is where the charstrings get compatibilized.
			mmCharSet.append(mmEntry)
			numBuilt += 1
		except CBFontError:
			logMsg( "Skipping glyph %s." % charSetEntry[0][0])
			numFailed +=1
	if numFailed:
		failedMsg = ", failed to build %s glyphs" % (numFailed)
	else:
		failedMsg = ""
	logMsg("Built %s glyphs%s. Total glyphs processed %s." % (numBuilt, failedMsg, numBuilt +  numFailed))

	numGlyphs = len(mmCharSet)
	lineList.append(kCharStringHeader % (numGlyphs))
	
	for entry in mmCharSet:
		charString =  """/%s ## RD { %s endchar } ND
	""" % (entry[0], entry[1])
		lineList.append(charString)
	lineList.append(kTail)
	data = os.linesep.join(lineList)
	fp = open("mmfont.txt", "wt")
	fp.write(data)
	fp.close()
	
	p = os.popen("type1 mmfont.txt mmfont.pfa")
	log = p.read()
	p.close()
	logMsg( log)
	
	
	# 5 callsubr for 1 arg hlineto, vlineto
	# 6 callsubr for 2 args rlineto
	# 7 callsubr for 3 args rlineto
	# 8 callsubr for 4 args
	# 9 callsubr for 6 args
	logMsg("All done. " + time.asctime())
	return
					
				
				
		
kHeader1 = "%!PS-AdobeFont-1.0:"
kHeader2 = """ 1.000
%%CreationDate: Thu May 31 02:04:59 2007
%%VMusage: 120000 150000"""

kTopDict = """17 dict begin
/FontInfo 15 dict dup begin
/FullName (%s) readonly def
/FamilyName (%s) readonly def
/ItalicAngle 0 def
/isFixedPitch false def
/UnderlinePosition -100 def
/UnderlineThickness 50 def
/BlendDesignPositions [[0 ][1 ]] def
/BlendDesignMap [[[ 0 0 ][1000 1 ]]] def
/BlendAxisTypes [/Weight ] def
end readonly def
/WeightVector [1 0 ] def
/FontName /%s def
/Encoding StandardEncoding def
/PaintType 0 def
/FontType 1 def
/$Blend { 0 mul add} bind def
/FontMatrix [ 0.001 0 0 0.001 0 0 ] readonly def
"""

kFontBox = """/FontBBox {%s } readonly def
/Blend 3 dict dup begin
/FontBBox {{ %s  } { %s } { %s } { %s }} def
/Private 14 dict def
end def
"""
kStdDefs1 = """/shareddict where
{ pop currentshared { setshared } true setshared shareddict }
{ { } userdict } ifelse dup
/makeblendedfont where { /makeblendedfont get dup type /operatortype eq {
pop false } { 0 get dup type /integertype ne
{ pop false } { 11 lt } ifelse } ifelse } { true } ifelse
{ /makeblendedfont {
11 pop
2 copy length exch /WeightVector get length eq
{ dup 0 exch { add } forall 1 sub abs .001 gt }
{ true } ifelse
{ /makeblendedfont cvx errordict /rangecheck get exec } if
exch dup dup maxlength dict begin {
false { /FID /UniqueID /XUID } { 3 index eq or } forall
{ pop pop } { def } ifelse
} forall
/XUID 2 copy known {
get dup length 2 index length sub dup 0 gt {
exch dup length array copy
exch 2 index { 65536 mul cvi 3 copy put pop 1 add } forall pop /XUID exch def
} { pop pop } ifelse
} { pop pop } ifelse
{ /Private /FontInfo } {
dup load dup maxlength dict begin {
false { /UniqueID /XUID } { 3 index eq or } forall
{ pop pop } { def } ifelse } forall currentdict end def
} forall
dup /WeightVector exch def
dup /$Blend exch [
exch false exch
dup length 1 sub -1 1 {
1 index dup length 3 -1 roll sub get
dup 0 eq {
pop 1 index { /exch load 3 1 roll } if
/pop load 3 1 roll
} { dup 1 eq { pop }
{ 2 index { /exch load 4 1 roll } if
3 1 roll /mul load 3 1 roll } ifelse
1 index { /add load 3 1 roll } if
exch pop true exch } ifelse
} for
pop { /add load } if
] cvx def
{ 2 copy length exch length ne { /makeblendedfont cvx errordict /typecheck get exec } if
0 0 1 3 index length 1 sub {
dup 4 index exch get exch 3 index exch get mul add
} for
exch pop exch pop }
{ { dup type dup dup /arraytype eq exch /packedarraytype eq or {
pop 1 index /ForceBold eq {
5 index 0 0 1 3 index length 1 sub {
dup 4 index exch get { 2 index exch get add } { pop } ifelse
} for exch pop exch pop
2 index /ForceBoldThreshold get gt 3 copy } {
{ length 1 index length ne { pop false } {
true exch { type dup /integertype eq exch /realtype eq exch or and } forall
} ifelse }
2 copy 8 index exch exec { pop 5 index 5 index exec }
{ exch dup length array 1 index xcheck { cvx } if
dup length 1 sub 0 exch 1 exch {
dup 3 index exch get dup type dup /arraytype eq exch /packedarraytype eq or {
dup 10 index 6 index exec {
9 index exch 9 index exec } if } if 2 index 3 1 roll put
} for exch pop exch pop
} ifelse 3 copy
1 index dup /StemSnapH eq exch /StemSnapV eq or {
dup length 1 sub { dup 0 le { exit } if
dup dup 1 sub 3 index exch get exch 3 index exch get 2 copy eq {
pop 2 index 2 index 0 put 0 } if le { 1 sub }
{ dup dup 1 sub 3 index exch get exch 3 index exch get
3 index exch 3 index 1 sub exch put
3 copy put pop
2 copy exch length 1 sub lt { 1 add } if } ifelse } loop pop
dup 0 get 0 le {
dup 0 exch { 0 gt { exit } if 1 add } forall
dup 2 index length exch sub getinterval } if } if } ifelse put }
{ /dicttype eq { 6 copy 3 1 roll get exch 2 index exec }
{ /makeblendedfont cvx errordict /typecheck get exec } ifelse
} ifelse pop pop } forall pop pop pop pop }
currentdict Blend 2 index exec
currentdict end
} bind put
/$fbf { FontDirectory counttomark 3 add -1 roll known {
cleartomark pop findfont } {
] exch findfont exch makeblendedfont
dup /Encoding currentfont /Encoding get put definefont
} ifelse currentfont /ScaleMatrix get makefont setfont
} bind put } { pop pop } ifelse exec
/NormalizeDesignVector {
450 sub 450 div
} bind def
/ConvertDesignVector {
dup 1 exch sub exch
} bind def
/$mmff_origfindfont where {
pop save { restore } { pop pop }
} { { } { def } } ifelse
/setshared where { pop true } { false } ifelse
/findfont where pop dup systemdict eq {
pop { currentshared { { } } { true setshared { false setshared } } ifelse shareddict
} { { } userdict } ifelse begin
} { begin { currentdict scheck } { false } ifelse {
currentshared { { } } { true setshared { false setshared } } ifelse
} { { } } ifelse } ifelse
/$mmff_origfindfont /findfont load 3 index exec
/findfont {
dup FontDirectory exch known
{ dup FontDirectory exch get /FontType get 3 ne }
{ dup SharedFontDirectory exch known
{ dup SharedFontDirectory exch get /FontType get 3 ne }
{ false } ifelse } ifelse
{ $mmff_origfindfont } { dup dup length string cvs (_) search {
cvn dup dup FontDirectory exch known exch SharedFontDirectory exch known or {
true } { dup length 7 add string dup 0 (%font%) putinterval
dup 2 index 6 exch dup length string cvs putinterval
{ status } stopped { pop false } if {
pop pop pop pop true } { false } ifelse } ifelse {
$mmff_origfindfont begin pop
[ exch { (_) search { { cvr } stopped { pop pop } {
exch pop exch } ifelse
} { pop exit } ifelse } loop false /FontInfo where {
pop FontInfo /BlendAxisTypes 2 copy known {
get length counttomark 2 sub eq exch pop
} { pop pop } ifelse } if {
NormalizeDesignVector
ConvertDesignVector
] currentdict exch makeblendedfont
2 copy exch /FontName exch put
definefont } { cleartomark $mmff_origfindfont } ifelse end
} { pop pop pop $mmff_origfindfont } ifelse
} { pop $mmff_origfindfont } ifelse } ifelse
} bind 3 index exec
/SharedFontDirectory dup where { pop pop } { 0 dict 3 index exec } ifelse
end exec pop exec
currentdict end
%currentfile eexec
dup /Private 22 dict dup begin
/RD { string currentfile exch readstring pop } executeonly def
/ND { noaccess def } executeonly def
/NP { noaccess put } executeonly def
"""
kHintValues = """/BlueValues [ -250 -250 1100 1100  ] ND
/BlueFuzz 0 def
/StdHW [ %s ] ND
/StdVW [ %s] ND
3 index /Blend get /Private get begin
/BlueValues [[ -250 -250 ] [ -250 -250 ] [ 1100 1100 ][ 1100 1100 ] ] def
/BlueFuzz [0 0  ] def
/StdHW [[ %s ]] def
/StdVW [[ %s ]] def
"""

kStdDef2 = """end
/OtherSubrs
[ { } { } { }
{
systemdict /internaldict known not
{ pop 3 }
{ 1183615869 systemdict /internaldict get exec
dup /startlock known
{ /startlock get exec }
{ dup /strtlck known
{ /strtlck get exec }
{ pop 3 }
ifelse }
ifelse }
ifelse
} executeonly
{ } { } { } { } { } { } { } { } { } { }
{ 2 1 roll $Blend } bind
{ exch 4 -1 roll $Blend exch 3 2 roll $Blend } bind
{ 3 -1 roll 6 -1 roll $Blend 3 -1 roll 5 -1 roll $Blend 3 -1 roll 4 3 roll $Blend } bind
{ 4 -1 roll 8 -1 roll $Blend 4 -1 roll 7 -1 roll $Blend 4 -1 roll 6 -1 roll $Blend 4 -1 roll 5 -1 roll $Blend } bind
{ 6 -1 roll 12 -1 roll $Blend 6 -1 roll 11 -1 roll $Blend 6 -1 roll 10 -1 roll $Blend 6 -1 roll 9 -1 roll $Blend 6 -1 roll 8 -1 roll $Blend 6 -1 roll 7 -1 roll $Blend } bind
] noaccess def
/Subrs 10 array
dup 0 ## RD { 3 0 callother pop pop setcurrentpoint return } NP
dup 1 ## RD { 0 1 callother return } NP
dup 2 ## RD { 0 2 callother return } NP
dup 3 ## RD { return } NP
dup 4 ## RD { 1 3 callother pop callsubr return } NP
dup 5 ## RD { 2 14 callother pop return } NP
dup 6 ## RD { 4 15 callother pop pop return } NP
dup 7 ## RD { 6 16 callother pop pop pop return } NP
dup 8 ## RD { 8 17 callother pop pop pop pop return } NP
dup 9 ## RD { 12 18 callother pop pop pop pop pop pop return } NP
ND
"""
kCharStringHeader = """2 index /CharStrings %s dict dup begin
"""

kTail = """end
end
readonly put
put
dup /FontName get exch definefont pop
mark %currentfile closefile
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000
cleartomark
"""

if __name__=='__main__':
	run()
