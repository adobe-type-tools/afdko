"""
ufoTools.py v1.14 Nov 24 2104

This module supports using the Adobe FDK tools which operate on 'bez'
files with UFO fonts. It provides low level utilities to manipulate UFO
data without fully parsing and instantiating UFO objects, and without
requiring that the AFDKO contain the robofab libraries.

Modified in Nov 2014, when AFDKO acquired the robofab libraries. It can now
be used with UFO fonts only to support the hash mechanism. 

Developed in order to support checkOutlines and autohint, the code
supports two main functions:
- convert between UFO GLIF and bez formats
- keep a history of processing in a hash map, so that the (lengthy)
processing by autohint and checkOutlines can be avoided if the glyph has
already been processed, and the source data has not changed.

The basic model is:
 - read GLIF file
 - transform GLIF XML element to bez file
 - call FDK tool on bez file
 - transform new bez file to GLIF XML element with new data, and save in list

After all glyphs are done save all the new GLIF XML elements to GLIF
files, and update the hash map.

A complication in the Adobe UFO workflow comes from the fact we want to
make sure that checkOutlines and autohint have been run on each glyph in
a UFO font, when building an OTF font from the UFO font. We need to run
checkOutlines, because we no longer remove overlaps from source UFO font
data, because this can make revising a glyph much easier. We need to run
autohint, because the glyphs must be hinted after checkOutlines is run,
and in any case we want all glyphs to have been autohinted. The problem
with this is that it can take a minute or two to run autohint or
checkOutlines on a 2K-glyph font. The way we avoid this is to make and
keep a hash of the source glyph drawing operators for each tool. When
the tool is run, it calculates a hash of the source glyph, and compares
it to the saved hash. If these are the same, the tool can skip the
glyph. This saves a lot of time: if checkOutlines and autohint are run
on all glyphs in a font, then a second pass is under 2 seconds.

Another issue is that since we no longer remove overlaps from the source
glyph files, checkOutlines must write any edited glyph data to a
different layer in order to not destroy the source data. The ufoTools
defines an Adobe-specific glyph layer for processed glyphs, named
"glyphs.com.adobe.type.processedGlyphs".
checkOutlines  writes new glyph files to the processed glyphs layer only
when it makes a change.

When the autohint program is run, the ufoTools must be able to tell
whether checkOutlines has been run and has altered a glyph: if so, the
input file needs to be from the processed glyph layer, else it needs to
be from the default glyph layer.

The way the hashmap works is that we keep an entry for every glyph that
has been processed, identified by a hash of its marking path data. Each
entry contains:
- a hash of the glyph point coordinates and its type
- a flag 'editStatus' of whether the path data was altered by a program.
- a history list: a list of the names of each program that has been run, in order.

New GLIF data is always written to the Adobe processed glyph layer. The
program may or may not have altered the outline data. For example,
autohint adds private hint data, and adds names to points, but does not
change any paths.

The editStatus flag records whether any program has changed the path
data. If 0 (no change), a program should read its data from the default
glyph layer. Otherwise, a program should take the input  glyph file
from the processed layer.

If the stored hash for the glyph matches the hash for the current glyph
file in the default layer, and the current program name is in the
history list,then the ufoTools will return "skipped=1", and the calling
program will skip the glyph. If the hash differs between the hash map
entry and the current glyph in the default layer, then the ufoTools lib
will save the new hash in the hash map entry and reset the history list
to contain just the current program. If the old and new hash match, but
the program name is not in the history list, then the ufoTools will not
skip the glyph, and will add the program name to the history list.

If a program changes the path data -- which we can tell by whether the
convertBezToGLIF() function is called, or if the hash of the new data is
different from the source data -- then the editStatus flag is set, and
any program names following the current program name are removed from
the history list.

The only tools using this are, at the moment, checkOutlines and
autohint. checkOutlines uses the hash map to skip processing only when
being used to edit glyphs, not when reporting them. checkOutlines
necessarily flattens  any components in the source glyph file to actual
outlines. autohint adds point names, and saves the hint data as a
private data in the new GLIF file.

autohint saves the hint data in the GLIF private data area, /lib/dict,
as a key and data pair. See below for the format.

A <hintset> element identifies a specific point by its name, and 
describes a new set of stem hints which should be applied before the 
specific point. 

A <flex> element identifies a specific point by its name. The point is
the first point of a curve. The presence of the <flex> element is a
processing suggestion, that the curve and its successor curve should
be converted to a flex operator.

One challenge in applying the hintset and flex elements is that in the
GLIF format, there is no explicit start and end operator: the first path
operator is both the end and the start of the path. I have chosen to
convert this to T1 by taking the first path operator, and making it a
move-to. I then also use it as the last path operator. An exception is a
line-to; in T1, this is omitted, as it is implied by the need to close
the path. Hence, if a hintset references the first operator, there is a
potential ambiguity: should it be applied before the T1 move-to, or
before the final T1 path operator?  The logic here applies it before the
move-to only. 

"""

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

import ConvertFontToCID

_hintFormat_ = """

 <glyph>
...
	<lib>
		<dict>
			<key><com.adobe.type.autohint><key>
			<data>
				<hintSetList>
					<hintset pointTag="point name">
						(<hstem pos="<decimal value>" width="decimal value" />)*
						(<vstem pos="<decimal value>" width="decimal value" />)*
            			<hstem3 stem3List="n0,n1,n2,n3,n4,n5" />* # where n1-5 are decimal values
            			<vstem3 stem3List="n0,n1,n2,n3,n4,n5" />* # where n1-5 are decimal values
					</hintset>*
					(<hintSetList>*
						(<hintset pointIndex="positive integer">
							(<stemindex>positive integer</stemindex>)+
						</hintset>)+
					</hintSetList>)*
					  <flexList>
						<flex pointTag="point Name" />
					  </flexList>*
				</hintSetList>
			</data>
		</dict>
	</lib>
</glyph>

Example from "B" in SourceCodePro-Regular
<key><com.adobe.type.autohint><key>
<data>
<hintSetList id="64bf4987f05ced2a50195f971cd924984047eb1d79c8c43e6a0054f59cc85dea23a49deb20946a4ea84840534363f7a13cca31a81b1e7e33c832185173369086">
  <hintset pointTag="hintSet0000">
	<hstem pos="0" width="28" />
	<hstem pos="338" width="28" />
	<hstem pos="632" width="28" />
	<vstem pos="100" width="32" />
	<vstem pos="496" width="32" />
  </hintset>
  <hintset pointTag="hintSet0005">
	<hstem pos="0" width="28" />
	<hstem pos="338" width="28" />
	<hstem pos="632" width="28" />
	<vstem pos="100" width="32" />
	<vstem pos="454" width="32" />
	<vstem pos="496" width="32" />
  </hintset>
  <hintset pointTag="hintSet0016">
	<hstem pos="0" width="28" />
	<hstem pos="338" width="28" />
	<hstem pos="632" width="28" />
	<vstem pos="100" width="32" />
	<vstem pos="496" width="32" />
  </hintset>
</hintSetList>
</data>

"""
import sys
import re
import time
import os
import plistlib
import hashlib

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring
debug = 0

def debugMsg(*args):
	if debug:
		print args

#UFO names
kProcessedGlyphsLayerName = "AFDKO ProcessedGlyphs"
kProcessedGlyphsLayer = "glyphs.com.adobe.type.processedGlyphs"
kContentsName = "contents.plist"
kLibName = "lib.plist"
kPublicGlyphOrderKey = "public.glyphOrder"

kAdobeDomainPrefix = "com.adobe.type"
kAdobHashMapName = "%s.processedHashMap" % (kAdobeDomainPrefix)
kAutohintName = "autohint"
kCheckOutlineName = "checkOutlines"

kOutlinePattern = re.compile(r"<outline.+?outline>", re.DOTALL)

kStemHintsName = "stemhints"
kStemListName = "stemList"
kStemPosName = "pos"
kStemWidthName = "width"
kHStemName = "hstem"
kVStemName = "vstem"
kHStem3Name = "hstem3"
kVStem3Name = "vstem3"
kStem3Pos = "stem3List"
kHintSetListName = "hintSetList"
kHintSetName = "hintset"
kBaseFlexName = "flexCurve"
kPointTag = "pointTag"
kStemIndexName = "stemindex"
kFlexIndexListName = "flexList"
kHintDomainName = "com.adobe.type.autohint"
kPointName = "name"
# Hint stuff
kStackLimit = 46
kStemLimit = 96


class UFOParseError(ValueError):
	pass

class BezParseError(ValueError):
	pass

class UFOFontData:
	def __init__(self, parentPath, useHashMap, programName):
		self.parentPath = parentPath
		self.glyphMap = {}
		self.newGlyphMap = {}
		self.glyphList = []
		self.fontInfo = None
		self.useHashMap = useHashMap # if true, will skip building bez string when glyph matches current glyph name.
		self.hashMap = {} #  Used to skip getting glyph data when glyph hash matches hash of current glyph data.
		self.fontDict = None
		self.programName = programName
		self.curSrcDir = None
		self.hashMapChanged = 0
		
		self.glyphDefaultDir = os.path.join(parentPath, "glyphs")
		self.glyphLayerDir = os.path.join(parentPath, kProcessedGlyphsLayer)
		self.glyphWriteDir = self.glyphLayerDir
		
	def getUnitsPerEm(self):
		unitsPerEm = "1000"
		if self.fontInfo == None:
			self.loadFontInfo()
		if self.fontInfo:
			unitsPerEm = int(self.fontInfo["unitsPerEm"])
		unitsPerEm = int(unitsPerEm)
		return unitsPerEm

	def getPSName(self):
		psName = "PSName-Undefined"
		if self.fontInfo == None:
			self.loadFontInfo()
		if self.fontInfo:
			psName = self.fontInfo.get("postscriptFontName", psName)
		return psName

	def isCID(self):
		return 0
		
	def convertToBez(self, glyphName, removeHints, beVerbose):
		# convertGLIFToBez does not yet support hints - no need for removeHints arg.
		bezString, width = convertGLIFToBez(self, glyphName)
		return bezString, width

	def updateFromBez(self, bezData, glyphName, width, beVerbose):
		# For UFO font, we don't use the width parameter: it is carried over from the input glif file.
		glifXML = convertBezToGLIF(self, glyphName, bezData )
		self.newGlyphMap[glyphName] = glifXML
	
	def saveChanges(self):
		if self.hashMapChanged:
			self.writeHashMap()
		self.hashMapChanged = 0
		
		if not os.path.exists(self.glyphWriteDir):
			os.makedirs(self.glyphWriteDir)
			
		for glyphName, glifXML in self.newGlyphMap.items():
			glyphPath = self.getWriteGlyphPath(glyphName)
			#print "Saving file", glyphPath
			fp = open(glyphPath, "wt")
			et = ET.ElementTree(glifXML)
			et.write(fp, encoding="UTF-8", xml_declaration=True)
			fp.close()

		# Update the layer contents.plist file
		layerContentsFilePath = os.path.join(self.parentPath, "layercontents.plist")
		self.updateLayerContents(layerContentsFilePath)
		glyphContentsFilePath = os.path.join(self.glyphWriteDir, "contents.plist")
		self.updateLayerGlyphContents(glyphContentsFilePath, self.newGlyphMap)
	
	def getWriteGlyphPath(self, glyphName):
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
			
		glyphFileName = self.glyphMap[glyphName]
		glifFilePath = os.path.join(self.glyphWriteDir, glyphFileName)
		return glifFilePath
		
	def getGlyphMap(self):
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
		return self.glyphMap
		
	def readHashMap(self):
		hashPath = os.path.join(self.parentPath, "data", kAdobHashMapName)
		if os.path.exists(hashPath):
			fp = open(hashPath, "rt")
			data = fp.read()
			fp.close()
			newMap =  eval(data)
		else:
			newMap = {}
			
		self.hashMap = newMap
		return
		
	def writeHashMap(self):
		hashMap = self.hashMap
		if len(hashMap) == 0:
			return # no glyphs were processed.
			
		hashDir = os.path.join(self.parentPath, "data")
		if not os.path.exists(hashDir):
			os.makedirs(hashDir)
		hashPath = os.path.join(hashDir, kAdobHashMapName)
		
		hasMapKeys = hashMap.keys()
		hasMapKeys.sort()
		data = "{" + os.linesep
		for gName in hasMapKeys:
			data += "'%s': %s,%s" % (gName, hashMap[gName], os.linesep)
		data += "}"
		fp = open(hashPath, "wt")
		fp.write(data)
		fp.close()
		return
	
	def getCurGlyphPath(self, glyphName):
		if self.curSrcDir == None:
			self.curSrcDir = self.glyphDefaultDir

		# Get the glyph file name.
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
		glyphFileName = self.glyphMap[glyphName]
		path = os.path.join(self.curSrcDir, glyphFileName)
		return path
		
	def getGlyphSrcPath(self, glyphName):
		"""
		A program that uses this library will read from the default
		glyph layer if it is the first program to run, else it will read
		from the processed layer. It will also skip processing a glyph
		if the glyph has not changed since the last time the program was
		run.
		
		This is controlled by the hash map, which will have an entry for
		each glyph that has been processed by a program. Each hash entry
		contains:
		[hash of the glyph in the default glyph layer, edit status, edit history list]
		
		When a program reads a UFO glyph, this library will apply the
		following rules. 
		If there is no hash map entry for the glyph, the glyph will be
		read from the default layer. 
		Else if the edit status is 0 (not yet changed), then glyph will
		be read from the default layer.
		Else if  the program name is first in the edit history list,
		then glyph will be read from the default layer.
		Else the program will read the glyph data from the processed
		glyph layer.
		
		If the program changes the glyph data:
			If it has an entry in the edit history list, it will delete all
			subsequent entries.
			Else it will add its name to the end.
		
		If the program's name is in the edit history list, it will
		compare the first entry in the hash map entry with a hash of the
		glyphs data in the default glyph data. If these match, the
		program will skip processing of this glyph.

		If a program changes the glyph data, it will set the edit status
		in the hash map entry to 1.
			
		"""
		
		# Get the glyph file name.
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
		glyphFileName = self.glyphMap[glyphName]

		srcDir = self.glyphDefaultDir
		usingLayer = 0
		if len(self.hashMap) == 0:
			# Hash maps have not yet been read in. Get them.
			self.readHashMap()

		usingProcessedLayer = 0 # default.
		try:
			hash, editStatus, historyList = self.hashMap[glyphName]
			try:
				programHistoryIndex = historyList.index(self.programName)
			except ValueError:
				programHistoryIndex = -1
			if (editStatus == 1) and (programHistoryIndex != 0):
				usingProcessedLayer = 1
		except KeyError:
			pass # use default glyph layer

		glyphPath = os.path.join(self.glyphDefaultDir, glyphFileName) # default
		self.curSrcDir = self.glyphDefaultDir
		if usingProcessedLayer:
			self.curSrcDir = self.glyphLayerDir
			glyphPath = os.path.join(self.glyphLayerDir, glyphFileName)
			if not os.path.exists(glyphPath):
				# should never happen
				print "Warning! hash map says '%s' should be in the '%s' layer, but it isn't." % (glyphName, self.glyphLayerDir)
				glyphPath = os.path.join(self.glyphDefaultDir, glyphFileName)
				self.curSrcDir = self.glyphDefaultDir
			
		return glyphPath
	
	def checkSkipGlyph(self, glyphName, newSrcHash):
		hashEntry = None
		programHistoryIndex = -1 # not found in historyList
		usingProcessedLayer = False
		skip = False
		if not self.useHashMap:
			return usingProcessedLayer, skip
		
		try:
			hashEntry = self.hashMap[glyphName]
			srcHash, editStatus, historyList = hashEntry
			try:
				programHistoryIndex = historyList.index(self.programName)
			except ValueError:
				programHistoryIndex = -1
			if (editStatus == 1) and (programHistoryIndex != 0):
				usingProcessedLayer = 1
		except KeyError:
			# Glyph is as yet untouched by any program. Use default glyph layer for source
				pass

		if (hashEntry != None) and (srcHash == newSrcHash) and (programHistoryIndex >= 0):
			# The glyph has already been processed by this program, and there have been no changes since.
			skip = 1
			return usingProcessedLayer, skip
			
		skip = 0 # We need to process this glyph.
		self.hashMapChanged = 1
		
		# If there is no hash entry for the glyph, add one. If the hash did not match, then build a new entry.
		if (hashEntry == None) or (newSrcHash != srcHash):
			self.hashMap[glyphName] = [newSrcHash, 0, [self.programName] ]
		else:
			# newHash == srcHash
			# Add program name to the historyList, if it is not there already.
			if programHistoryIndex < 0:
				# autohint must be run last. Remove it, if it is in the list before this program.
				try:
					historyList.remove(kAutohintName)
				except ValueError:
					pass
				historyList.append(self.programName)
				
		return usingProcessedLayer, skip
		
	def getOrSkipGlyphXML(self, glyphName):
		# Get the glyph file name.
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
		glyphFileName = self.glyphMap[glyphName]

		self.curSrcDir = self.glyphDefaultDir
		usingLayer = 0
		if len(self.hashMap) == 0:
			# Hash maps have not yet been read in. Get them.
			self.readHashMap()

		usingProcessedLayer = 0 # default.
		if self.useHashMap:
			try:
				hashEntry = self.hashMap[glyphName]
				srcHash, editStatus, historyList = hashEntry
				try:
					programHistoryIndex = historyList.index(self.programName)
				except ValueError:
					programHistoryIndex = -1
				if (editStatus == 1) and (programHistoryIndex != 0):
					usingProcessedLayer = 1
			except KeyError:
				# Glyph is as yet untouched by any program. Use default glyph layer
				hashEntry = None
				programHistoryIndex = -1 # not found in historyList
		else:
			hashEntry = None
			programHistoryIndex = -1 # not found in historyList
		
		# Get default glyph layer data, so we can check if the glyph has been edited since this program was last run.
		# If the program name is in the history list, and the srcHash matches the default glyph layer data, we can skip.
		glyphPath = os.path.join(self.glyphDefaultDir, glyphFileName) # default
		etRoot = ET.ElementTree()
		glifXML = etRoot.parse(glyphPath)
		outlineXML = glifXML.find("outline")
		try:
			widthXML = glifXML.find("advance")
			if widthXML != None:
				width = int(eval(widthXML.get("width")))
			else:
				width = 1000
			newHash, dataList = self.buildGlyphHashValue(width, outlineXML, glyphName)
		except UFOParseError,e:
			print "Error. skipping glyph '%s' because of parse error: %s" % (glyphName, e.message)
			return None, None, 1
		if self.useHashMap and (hashEntry != None) and (srcHash == newHash) and (programHistoryIndex >= 0):
			# The glyph has already been processed by this program, and there have been no changes since.
			skip = 1
			return None, None, skip
		skip = 0 # We need to process this glyph.
		self.hashMapChanged = 1
		
		# If there is no hash entry for the glyph, add one. If the hash did not match, then build a new entry.
		if (hashEntry == None) or (newHash != srcHash):
			self.hashMap[glyphName] = [newHash, 0, [self.programName] ]
		else:
			# newHash == srcHash
			# Add program name to the historyList, if it is not there already.
			if programHistoryIndex < 0:
				# autohint must be run last. Remove it, if it is in the list before this program.
				try:
					historyList.remove(kAutohintName)
				except ValueError:
					pass
				historyList.append(self.programName)
				
		# If the glyph needs to be read from the processed layer instead of the default layer, we still need to get it.
		if usingProcessedLayer:
			self.curSrcDir = self.glyphLayerDir
			glyphPath = os.path.join(self.glyphLayerDir, glyphFileName)
			if not os.path.exists(glyphPath):
				# should never happen
				print "Warning! hash map says '%s' should be in the '%s' layer, but it isn't." % (glyphName, self.glyphLayerDir)
				glyphPath = os.path.join(self.glyphDefaultDir, glyphFileName)
			etRoot = ET.ElementTree()
			glifXML = etRoot.parse(glyphPath)
			outlineXML = glifXML.find("outline")
	
		return glifXML, outlineXML, skip
	
	def getGlyphList(self):
		if len(self.glyphMap) == 0:
			self.loadGlyphMap()
		return self.glyphList
		
	def loadGlyphMap(self):
		# Need to both get the list of glyphs from contents.plist, and also the glyph order.
		# the latter is take from the public.glyphOrder key in lib.py, if it exists,
		# else it is taken from the  contents.plist file. Any glyphs in  contents.plist
		# which are not named in the public.glyphOrder are sorted after all glyphs which are named
		# in the public.glyphOrder,, in the order that they occured in contents.plist.
		contentsPath = os.path.join(self.parentPath, "glyphs", kContentsName)
		self.glyphMap, self.glyphList = parsePList(contentsPath)
		orderPath = os.path.join(self.parentPath, kLibName)
		self.orderMap  = parseGlyphOrder(orderPath)
		if self.orderMap != None:
			orderIndex = len(self.orderMap)
			orderList = []
			
			# If there are glyphs in the font which are not named in the public.glyphOrder entry, add then in the order of the contents.plist file.
			for glyphName in self.glyphList:
				try:
					entry = [self.orderMap[glyphName], glyphName]
				except KeyError:
					entry = [orderIndex, glyphName]
					self.orderMap[glyphName] = orderIndex
					orderIndex += 1
				orderList.append(entry)
			orderList.sort()
			self.glyphList = []
			for entry in orderList:
				self.glyphList.append(entry[1])
		else:
			self.orderMap = {}
			numGlyphs = len(self.glyphList)
			for i in range(numGlyphs):
				self.orderMap[self.glyphList[i]] = i
		
		
				
	def loadFontInfo(self):
		fontInfoPath = os.path.join(self.parentPath, "fontinfo.plist")
		if not os.path.exists(fontInfoPath):
			return
		self.fontInfo, tempList = parsePList(fontInfoPath)

	def updateLayerContents(self, contentsFilePath):
		if os.path.exists(contentsFilePath):
			contentsList= plistlib.readPlist(contentsFilePath)
			# If the layer name already exists, don't add a new one, or change the path
			for layerName, layerPath in contentsList:
				if (layerPath == kProcessedGlyphsLayer):
					return
		else:
			contentsList = []
		contentsList.append([kProcessedGlyphsLayerName, kProcessedGlyphsLayer])
		plistlib.writePlist(contentsList, contentsFilePath)
			
	def updateLayerGlyphContents(self, contentsFilePath, newGlyphData):
		if os.path.exists(contentsFilePath):
			contentsDict = plistlib.readPlist(contentsFilePath)
		else:
			contentsDict = {}
		for glyphName in newGlyphData.keys():
			contentsDict[glyphName] = self.glyphMap[glyphName]
		plistlib.writePlist(contentsDict, contentsFilePath)

	def getFontInfo(self, fontPSName, inputPath, allow_no_blues, noFlex, vCounterGlyphs, hCounterGlyphs, fdIndex=0):
		if self.fontDict != None:
			return self.fontDict
			
		if self.fontInfo == None:
			self.loadFontInfo()

		fdDict = ConvertFontToCID.FDDict()
		fdDict.LanguageGroup = self.fontInfo.get("languagegroup", "0") # should be 1 if the glyphs are ideographic, else 0.
		fdDict.OrigEmSqUnits = self.getUnitsPerEm()
		fdDict.FontName = self.getPSName()
		upm = self.getUnitsPerEm()
		low =  min( -upm*0.25, float(self.fontInfo.get("openTypeOS2WinDescent", "0")) - 200)
		high =  max ( upm*1.25, float(self.fontInfo.get("openTypeOS2WinAscent", "0")) + 200)
		# Make a set of inactive alignment zones: zones outside of the font bbox so as not to affect hinting.
		# Used when src font has no BlueValues or has invalid BlueValues. Some fonts have bad BBox values, so
		# I don't let this be smaller than -upm*0.25, upm*1.25.
		inactiveAlignmentValues = [low, low, high, high]
		blueValues = self.fontInfo.get("postscriptBlueValues", [])
		numBlueValues = len(blueValues)
		if numBlueValues < 4:
			if allow_no_blues:
				blueValues = inactiveAlignmentValues
				numBlueValues = len(blueValues)
			else:
				raise	UFOParseError("ERROR: Font must have at least four values in its BlueValues array for AC to work!")
		blueValues.sort()
		# The first pair only is a bottom zone, where the first value is the overshoot position;
		# the rest are top zones, and second value of the pair is the overshoot position.
		blueValues[0] = blueValues[0] - blueValues[1]
		for i in range(3, numBlueValues,2):
			blueValues[i] = blueValues[i] - blueValues[i-1]
			
		blueValues = map(str, blueValues)
		numBlueValues = min(numBlueValues, len(ConvertFontToCID.kBlueValueKeys))
		for i in range(numBlueValues):
			key = ConvertFontToCID.kBlueValueKeys[i]
			value = blueValues[i]
			exec("fdDict.%s = %s" % (key, value))

		otherBlues = self.fontInfo.get("postscriptOtherBlues", [])
		numBlueValues = len(otherBlues)
		
		if len(otherBlues) > 0:
			i = 0
			numBlueValues = len(otherBlues)
			otherBlues.sort()
			for i in range(0, numBlueValues,2):
				otherBlues[i] = otherBlues[i] - otherBlues[i+1]
			otherBlues = map(str, otherBlues)
			numBlueValues = min(numBlueValues, len(ConvertFontToCID.kOtherBlueValueKeys))
			for i in range(numBlueValues):
				key = ConvertFontToCID.kOtherBlueValueKeys[i]
				value = otherBlues[i]
				exec("fdDict.%s = %s" % (key, value))
				
		vstems = self.fontInfo.get("postscriptStemSnapV", [])
		if len(vstems) == 0:
			if allow_no_blues:
				vstems =  [fdDict.OrigEmSqUnits] # dummy value. Needs to be larger than any hint will likely be,
				# as the autohint program strips out any hint wider than twice the largest global stem width.
			else:
				raise	UFOParseError("ERROR: Font does not have postscriptStemSnapV!")

		vstems.sort()
		if (len(vstems) == 0) or ((len(vstems) == 1) and (vstems[0] < 1) ):
			vstems =  [fdDict.OrigEmSqUnits] # dummy value that will allow PyAC to run
			logMsg("WARNING: There is no value or 0 value for DominantV.")
		vstems = repr(vstems)
		fdDict.DominantV = vstems
		
		hstems = self.fontInfo.get("postscriptStemSnapH", [])
		if len(hstems) == 0:
			if allow_no_blues:
				hstems =  [fdDict.OrigEmSqUnits] # dummy value. Needs to be larger than any hint will likely be,
				# as the autohint program strips out any hint wider than twice the largest global stem width.
			else:
				raise	UFOParseError("ERROR: Font does not have postscriptStemSnapH!")

		hstems.sort()
		if (len(hstems) == 0) or ((len(hstems) == 1) and (hstems[0] < 1) ):
			hstems =  [fdDict.OrigEmSqUnits] # dummy value that will allow PyAC to run
			logMsg("WARNING: There is no value or 0 value for DominantH.")
		hstems = repr(hstems)
		fdDict.DominantH = hstems
		
		if noFlex:
			fdDict.FlexOK = "false"
		else:
			fdDict.FlexOK = "true"

		# Add candidate lists for counter hints, if any.
		if vCounterGlyphs:
			temp = " ".join(vCounterGlyphs)
			fdDict.VCounterChars = "( %s )" % (temp)
		if hCounterGlyphs:
			temp = " ".join(hCounterGlyphs)
			fdDict.HCounterChars = "( %s )" % (temp)
			
		fdDict.BlueFuzz = self.fontInfo.get("postscriptBlueFuzz", 1)
		# postscriptBlueShift
		# postscriptBlueScale
		self.fontDict = fdDict
		return fdDict

	def getfdIndex(self, gid):
		fdIndex = 0
		return fdIndex
		
	def getfdInfo(self, psName, inputPath, allow_no_blues, noFlex, vCounterGlyphs, hCounterGlyphs, glyphList, fdIndex=0):
		fontDictList = []
		fdGlyphDict = None
		fdDict = self.getFontInfo(psName, inputPath, allow_no_blues, noFlex, vCounterGlyphs, hCounterGlyphs, fdIndex)
		fontDictList.append(fdDict)

		# Check the fontinfo file, and add any other font dicts
		srcFontInfo = os.path.dirname(inputPath)
		srcFontInfo = os.path.join(srcFontInfo, "fontinfo")
		maxX = self.getUnitsPerEm()*2
		maxY = maxX
		minY = -self.getUnitsPerEm()
		if os.path.exists(srcFontInfo):
			fi = open(srcFontInfo, "rU")
			fontInfoData = fi.read()
			fi.close()
			fontInfoData = re.sub(r"#[^\r\n]+", "", fontInfoData)
		
			if "FDDict" in fontInfoData:
		
				blueFuzz = ConvertFontToCID.getBlueFuzz(inputPath)
				fdGlyphDict, fontDictList, finalFDict = ConvertFontToCID.parseFontInfoFile(fontDictList, fontInfoData, glyphList, maxY, minY, psName, blueFuzz)
				if finalFDict == None:
					# If a font dict was not explicitly specified for the output font, use the first user-specified font dict.
					ConvertFontToCID.mergeFDDicts( fontDictList[1:], self.fontDict )
				else:
					ConvertFontToCID.mergeFDDicts( [finalFDict], topDict )
					
		return fdGlyphDict, fontDictList
	
	def getGlyphID(self, glyphName):
		try:
			gid = self.orderMap[glyphName]
		except IndexError:
			raise UFOParseError("Could not find glyph name '%s' in UFO font contents plist. '%s'. " % (glyphName, self.parentPath))
		return gid
		
	def buildGlyphHashValue(self, width, outlineXML, glyphName, level = 0):
		"""  glyphData must be the official <outline> XML from a GLIF.
		We skip contours with only one point.
		"""
		dataList = [str(width)]
		if level > 10:
			raise UFOParseError("In parsing component, exceeded 10 levels of reference. '%s'. " % (glyphName))
		# <outline> tag is optional per spec., e.g. space glyph does not necessarily have it.
		if outlineXML is None:
			raise UFOParseError("In parsing component, no outline component found. '%s'. " % (glyphName))
		for childContour in outlineXML:
			if childContour.tag == "contour":
				if len(childContour) < 2:
					continue
				else:
					for child in childContour:
						if child.tag == "point":
							dataList.append(child.attrib["x"])
							dataList.append(child.attrib["y"])
							try:
								dataList.append(child.attrib["type"][0])
							except KeyError:
								dataList.append("o") # for off-curve.
							#print dataList[-3:]
			elif childContour.tag == "component":
				# first append the key/value pairs
				for key in childContour.attrib.keys():
					if key in ["base", "xOffset", "yOffset"]:
						dataList.append(key) # for off-curve.
						dataList.append(childContour.attrib[key]) # for off-curve.
				# now append the component hash.
				try:
					compGlyphName = childContour.attrib["base"]
				except KeyError:
					raise UFOParseError("'%s' is missing the 'base' attribute in a component. glyph '%s'." % (glyphName))
				try:
					componentPath = self.getCurGlyphPath(compGlyphName)
				except KeyError:
					raise UFOParseError("'%s' component glyph is missing from contents.plist." % (compGlyphName))
				if not os.path.exists(componentPath):
					raise UFOParseError("'%s' component file is missing: '%s'." % (compGlyphName, componentPath))
				etRoot = ET.ElementTree()
				componentXML = etRoot.parse(componentPath)
				componentOutlineXML = componentXML.find("outline")
				componentHash, componentDataList = self.buildGlyphHashValue(width, componentOutlineXML, glyphName, level+1)
				dataList.extend(componentDataList)
				
		data = "".join(dataList)
		if len(data) < 128:
			hash = data
		else:
			hash = hashlib.sha512(data).hexdigest()
		return hash, dataList

	def getComponentOutline(self, componentItem):
		try:
			compGlyphName = componentItem.attrib["base"]
		except KeyError:
			raise UFOParseError("'%s' attribute missing from component '%s'." % ("base", xmlToString(componentXML)))
		
		try:
			compGlyphFilePath = self.getCurGlyphPath(compGlyphName)
		except KeyError:
			raise UFOParseError("'%s' compGlyphName missing from contents.plist." % (compGlyphName))
		if not os.path.exists(compGlyphFilePath):
			raise UFOParseError("'%s' compGlyphName file is missing: '%s'." % (compGlyphName, compGlyphFilePath))
		
		etRoot =  ET.ElementTree()
		glifXML = etRoot.parse(compGlyphFilePath)
		outlineXML = glifXML.find("outline")
		return outlineXML
		
	def copyTo(self, dstPath):
		""" Copy UFO font to target path"""
		return
		
	def close(self):
		if self.hashMapChanged:
			self.writeHashMap()
		self.hashMapChanged = 0
		return

def parseGlyphOrder(filePath):
	orderMap = None
	if os.path.exists(filePath):
		publicOrderDict, temp = parsePList(filePath, kPublicGlyphOrderKey)
		if publicOrderDict != None:
			orderMap = {}
			glyphList = publicOrderDict[kPublicGlyphOrderKey]
			numGlyphs = len(glyphList)
			for i in range(numGlyphs):
				orderMap[glyphList[i]] = i
	return orderMap
	
def	parsePList(filePath, dictKey = None):
	# if dictKey is defined, parse and return only the data for that key.
	# This helps avoid needing to parse all plist types. I need to support only data for known keys amd lists.
	plistDict = {}
	plistKeys = []

	# I uses this rather than the plistlib in order to get a list that allows preserving key order.
	fp = open(filePath, "r")
	data = fp.read()
	fp.close()
	contents = XML(data)
	dict = contents.find("dict")
	if dict == None:
		raise UFOParseError("In '%s', failed to find dict. '%s'." % (filePath))
	lastTag = "string"
	for child in dict:
		if child.tag == "key":
			if lastTag == "key":
				raise UFOParseError("In contents.plist, key name '%s' followed another key name '%s'." % (xmlToString(child), lastTag) )
			skipKeyData = False
			lastName = child.text
			lastTag = "key"
			if dictKey != None:
				if lastName != dictKey:
					skipKeyData = True
		elif  child.tag != "key":
			if lastTag != "key":
				raise UFOParseError("In contents.plist, key value '%s' followed a non-key tag '%s'." % (xmlToString(child), lastTag) )
			lastTag =  child.tag

			if skipKeyData:
				continue
				
			if plistDict.has_key(lastName):
				raise UFOParseError("Encountered duplicate key name '%s' in '%s'." % (lastName, filePath) )
			if child.tag == "array":
				list = []
				for listChild in child:
					val = listChild.text
					if listChild.tag == "integer":
						val= int(eval(val))
					elif listChild.tag == "real":
						val = float(eval(val))
					elif listChild.tag == "string":
						pass
					else:
						raise UFOParseError("In plist file, encountered unhandled key type '%s' in '%s' for parent key %s. %s." % (listChild.tag, child.tag, lastName, filePath))
					list.append(val)
				plistDict[lastName] = list
			elif child.tag == "integer":
				plistDict[lastName] = int(eval(child.text))
			elif child.tag == "real":
				plistDict[lastName] = float(eval(child.text))
			elif child.tag == "false":
				plistDict[lastName] = 0
			elif child.tag == "true":
				plistDict[lastName] = 1
			elif child.tag == "string":
				plistDict[lastName] = child.text
			else:
				raise UFOParseError("In plist file, encountered unhandled key type '%s' for key %s. %s." % (child.tag, lastName, filePath))
			plistKeys.append(lastName)
		else:
			raise UFOParseError("In plist file, encountered unexpected element '%s'. %s." % (xmlToString(child), filePath ))
	if len(plistDict) == 0:
		plistDict = None
	return plistDict, plistKeys

				
	

class UFOTransform:
	kTransformTagList = ["xScale", "xyScale", "yxScale", "yScale", "xOffset", "yOffset"]
	def __init__(self, componentXML):
		self.transformFactors = []
		self.isDefault = True
		self.isOffsetOnly = True
		hasScale = False
		hasOffset = False
		for tag in self.kTransformTagList:
			val = componentXML.attrib.get(tag, None)
			if tag in ["xScale", "yScale"]:
				if val == None:
					val = 1.0
				else:
					val = float(val)
					if val != 1.0:
						hasScale = True
			else:
				if val == None:
					val = 0
				else:
					val = float(val)
					if val != 0:
						self.isDefault = False
						if tag in ["xyScale", "yxScale"]:
							hasScale = True
						elif tag in ["xOffset", "yOffset"]:
							hasOffset = True
						else:
							raise UFOParseError("Unknown tag '%s' in component '%s'." % (tag, xmlToString(componentXML)))
							
			self.transformFactors.append(val)
		if  hasScale or hasOffset:
			self.isDefault = False
		if hasScale:
			self.isOffsetOnly = False
			
	def concat(self, transform):
		if transform == None:
			return
		if transform.isDefault:
			return
		tfCur = self.transformFactors
		tfPrev = transform.transformFactors
		if transform.isOffsetOnly:
			tfCur[4] += tfPrev[4]
			tfCur[5] += tfPrev[5]
			self.isDefault = False
		else:
			tfNew = [0.0]*6
			tfNew[0] = tfCur[0]*tfPrev[0] + tfCur[1]*tfPrev[2];
			tfNew[1] = tfCur[0]*tfPrev[1] + tfCur[1]*tfPrev[3];
			tfNew[2] = tfCur[2]*tfPrev[0] + tfCur[3]*tfPrev[2];
			tfNew[3] = tfCur[2]*tfPrev[1] + tfCur[3]*tfPrev[3];
			tfNew[4] = tfCur[4]*tfPrev[0] + tfCur[5]*tfPrev[2] + tfPrev[4];
			tfNew[5] = tfCur[4]*tfPrev[1] + tfCur[5]*tfPrev[3] + tfPrev[5];
			self.transformFactors = tfNew
			self.isOffsetOnly = self.isOffsetOnly and transform.isOffsetOnly
			self.isDefault = False

	def apply(self, x, y):
		tfCur = self.transformFactors
		if self.isOffsetOnly:
			x += tfCur[4] 
			y += tfCur[5]
		else:
			x, y = x*tfCur[0] + y*tfCur[2] + tfCur[4], \
				   x*tfCur[1] + y*tfCur[3] + tfCur[5]
		return x,y
		
	
				
	
def convertGlyphOutlineToBezString(outlineXML, ufoFontData, curX, curY, transform = None, level = 0):
	"""convert XML outline element containing contours and components to a bez string.
	Since xml.etree.CElementTree is compiled code, this
	will run faster than tokenizing and parsing in regular Python.
	
	glyphMap is a dict mapping glyph names to component file names.
	If it has a length of 1, it needs to be filled from the contents.plist file.
	It must have a key/value [UFO_FONTPATH] = path to parent UFO font.
	
	transformList is None, or a list of floats in the order:
	["xScale", "xyScale", "yxScale", "yScale", "xOffset", "yOffset"]
	
	Build a list of outlines and components.
	For each item:
		if is outline:
			get list of points
			for each point:
				if transform matrix:
					apply transform to coordinate
				if off line
					push coord on stack
				if online:
					add operator
			if is component:
				if any scale, skew, or offset factors:
					set transform
					call convertGlyphOutlineToBezString with transform
					add to bez string
	I don't bother adding in any hinting info, as this is used only for making 
	temp bez files as input to checkOutlines or autohint, which invalidate hinting data
	by changing the outlines data, or at best ignore hinting data.
	
	bez ops output: ["rrmt", "dt", "rct", "cp" , "ed"]
	"""
	
	if level > 10:
		raise UFOParseError("In parsing component, exceeded 10 levels of reference. '%s'. " % (outlineXML))

	bezStringList = []
	for outlineItem in outlineXML:

		if outlineItem.tag == "component":
			newTransform = UFOTransform(outlineItem)
			if newTransform.isDefault:
				if transform != None:
					newTransform.concat(transform)
				else:	
					newTransform = None
			else:
				if transform != None:
					newTransform.concat(transform)
			componentOutline = ufoFontData.getComponentOutline(outlineItem)
			outlineBezString, curX, curY = convertGlyphOutlineToBezString(componentOutline, ufoFontData, curX, curY, newTransform, level+1)
			bezStringList.append(outlineBezString)
			continue
			
		if outlineItem.tag != "contour":
			continue
		
		# May be an empty contour.
		if len(outlineItem) == 0:
			continue 
			
		# We have a regular contour. Iterate over points.
		argStack = []
		# Deal with setting up move-to.
		lastItem = outlineItem[0]
		try:
			type = lastItem.attrib["type"]
		except KeyError:
			type = "offcurve"
		if type in ["curve","line","qccurve"]:
			outlineItem = outlineItem[1:]
			if type != "line":
				outlineItem.append(lastItem) # I don't do this for line, as AC behaves differently when a final line-to is explicit.
			x = float(lastItem.attrib["x"])
			y = float(lastItem.attrib["y"])
			if transform != None:
				x,y = transform.apply(x,y)
			dx = int(round(x)) - curX
			dy = int(round(y)) - curY
			curX = curX + dx
			curY = curY + dy
			
			op = "%s %s rmt" % (dx, dy)
			bezStringList.append(op)
		elif type == "move":
			# first op is a move-to.
			if len(outlineItem) == 1:
				# this is a move, and is the only op in this outline. Don't pass it through. 
				# this is most likely left over from a GLIF format 1 anchor.
				argStack = []
				continue
		elif type == "offcurve":
			# We should only see an off curve point as the first point when the first op is  a curve and the last op is a line.
			# In this case, insert a move-to to the line's coords, then omit the line.
			# Breaking news! In rare cases, a first off-curve point can occur when the first AND last op is a curve.
			curLastItem = outlineItem[-1]
			# In this case, insert a move-to to the last op's end pos.
			try:
				lastType = curLastItem.attrib["type"]
				if lastType == "line":
					x = float(curLastItem.attrib["x"])
					y = float(curLastItem.attrib["y"])
					if transform != None:
						x,y = transform.apply(x,y)
					dx = int(round(x)) - curX
					dy = int(round(y)) - curY
					curX = curX + dx
					curY = curY + dy
					op = "%s %s rmt" % (dx, dy)
					bezStringList.append(op)
					outlineItem = outlineItem[:-1]
				elif lastType == "curve":
					x = float(curLastItem.attrib["x"])
					y = float(curLastItem.attrib["y"])
					if transform != None:
						x,y = transform.apply(x,y)
					dx = int(round(x)) - curX
					dy = int(round(y)) - curY
					curX = curX + dx
					curY = curY + dy
					op = "%s %s rmt" % (dx, dy)
					bezStringList.append(op)
					
			except KeyError:
				raise UFOParseError("Unhandled case for first and last points in outline '%s'." % (xmlToString(outlineItem)))
		else:
			raise UFOParseError("Unhandled case for first point in outline '%s'." % (xmlToString(outlineItem)))
		for contourItem in outlineItem:
			if contourItem.tag != "point":
				continue
			x = float(contourItem.attrib["x"])
			y = float(contourItem.attrib["y"])
			if transform != None:
				x,y = transform.apply(x,y)
			dx = int(round(x)) - curX
			dy = int(round(y)) - curY
			curX = curX + dx
			curY = curY + dy
			try:
				type = contourItem.attrib["type"]
			except KeyError:
				type = "offcurve"
			
			if type == "offcurve":
				argStack.append(dx)
				argStack.append(dy)
			elif type == "move":
				op = "%s %s rmt" % (dx, dy)
				bezStringList.append(op)
				argStack = []
			elif type == "line":
				op = "%s %s rdt" % (dx, dy)
				bezStringList.append(op)
				argStack = []
			elif type == "curve":
				if len(argStack) != 4:
					raise UFOParseError("Argument stack error seen for curve point '%s'." % (xmlToString(contourItem)))
				op = "%s %s %s %s %s %s rct" % ( argStack[0], argStack[1], argStack[2], argStack[3], dx, dy)
				argStack = []
				bezStringList.append(op)
			elif type == "qccurve":
				raise UFOParseError("Point type not supported '%s'." % (xmlToString(contourItem)))
			else:
				raise UFOParseError("Unknown Point type not supported '%s'." % (xmlToString(contourItem)))
				
		# we get here only if there was at least a move.
		bezStringList.append("cp" + os.linesep)

		
	bezstring = " ".join(bezStringList)
	return bezstring, curX, curY


def convertGLIFToBez(ufoFontData, glyphName):
	glifXML, outlineXML, skipped = ufoFontData.getOrSkipGlyphXML(glyphName)
	if skipped:
		return None, 0

	if outlineXML == None:
		print "Glyph '%s' has no outline data" % (glyphName)
		return None, 0

		
	widthXML = glifXML.find("advance")
	if widthXML != None:
		width = int(eval(widthXML.get("width")))
	else:
		width = 1000

	curX = curY = 0
	bezString, curX, curY = convertGlyphOutlineToBezString(outlineXML, ufoFontData, curX, curY)
	bezString  = (r"%%%s%ssc " % (glyphName, os.linesep)) + bezString + " ed"
	return bezString, width


class HintMask:
	# class used to collect hints for the current hint mask when converting bez to T2.
	def __init__(self, listPos):
		self.listPos = listPos # The index into the pointList is kept so we can quickly find them later.
		self.hList = [] # These contain the actual hint values.
		self.vList = []
		self.hstem3List = []
		self.vstem3List = []
		self.pointName = "hintSet" + str(listPos).zfill(4)	# The name attribute of the point which follows the new hint set.
		
	def addHintSet(self, hintSetList):
		# Add the hint set to hintSetList
		newHintSet = XMLElement(kHintSetName, {kPointTag: "%s" % (self.pointName) } )
		hintSetList.append(newHintSet)
		if (len(self.hList) > 0) or (len(self.vstem3List)):
			isH = 1
			addHintList(self.hList, self.hstem3List, newHintSet, isH)

		if (len(self.vList) > 0) or (len(self.vstem3List)):
			isH = 0
			addHintList(self.vList, self.vstem3List, newHintSet, isH)


def makeStemHintList(hintsStem3, stemList, isH):
	stem3Args = []
	lastPos = 0
	# In bez terms, the first coordinate in each pair is absolute, second is relative, and hence is the width.
	if isH:
		op = kHStem3Name
	else: 
		op = kVStem3Name
	newStem = XMLElement(op )
	posList = []
	for stem3 in hintsStem3:
		for pos, width in stem3:
			if (type(pos) == type(0.0)) and (int(pos) == pos):
				pos = int(pos)
			if (type(width) == type(0.0)) and (int(width) == width):
				width = int(width)
			posList.append("%s,%s" % (pos, width))
	posAttr = ",".join(posList)
	newStem.attrib[kStem3Pos] = posAttr
	stemList.append(newStem)
		
def makeHintList(hints, stemList, isH):
	# Add the list of  hint operators
	hintList = []
	lastPos = 0
	# In bez terms, the first coordinate in each pair is absolute, second is relative, and hence is the width.
	for hint in hints:
		if not hint:
			continue
		pos = hint[0]
		if (type(pos) == type(0.0)) and (int(pos) == pos):
			pos = int(pos)
		width = hint[1]
		if (type(width) == type(0.0)) and (int(width) == width):
			width = int(width)
		if isH:
			op = kHStemName
		else: 
			op = kVStemName
		newStem = XMLElement(op, {kStemPosName: "%s" % (pos), kStemWidthName: "%s" % (width) } )
		stemList.append(newStem)

def addFlexHint(flexList, stemHints):
	newFlexList = XMLElement(kFlexIndexListName)
	stemHints.append(newFlexList)
	for pointTag in flexList:
		newFlexTag =  XMLElement("flex", {"pointTag": "%s" % (pointTag) } )
		newFlexList.append(newFlexTag)

def fixStartPoint(outlineItem, opList):
	# For the GLIF format, the idea of first/last point is funky, because the format avoids identifying a 
	# a start point. This means there is no implied close-path line-to. If the last implied or explicit path-close
	# operator is a line-to, then replace the "mt" with linto, and remove the last explicit path-closing line-to, if any.
	# If the last op is a curve, then leave the first two point args on the stack at the end of the point list,
	# and move the last curveto to the first op, replacing the move-to.
	
	firstOp, firstX, firstY = opList[0]
	lastOp, lastX, lastY = opList[-1]
	firstPointElement = outlineItem[0]
	if (firstX == lastX) and (firstY == lastY):
		del outlineItem[-1]
		firstPointElement.set("type", lastOp)			
	else:
		# we have an implied final line to. All we need to do is convert the inital moveto to a lineto.
		firstPointElement.set("type", "line")


bezToUFOPoint = {
		"rmt" : 'move',
		 "hmt" : 'move',
		"vmt" : 'move',
		"rdt" : 'line', 
		"hdt" : "line", 
		"vdt" : "line", 
		"rct" : 'curve', 
		"rcv" : 'curve',  # Morisawa's alternate name for 'rct'.
		"vhct": 'curve',
		"hvct": 'curve', 
		}

def convertCoords(curX, curY):
	showX = int(curX)
	if showX != curX:
		showX = curX
	showY = int(curY)
	if showY != curY:
		showY = curY
	return showX, showY
	
def convertBezToOutline(ufoFontData, glyphName, bezString):
	""" Since the UFO outline element as no attributes to preserve, I can
	just make a new one.
	"""
	# convert bez data to a T2 outline program, a list of operator tokens.
	#
	# Convert all bez ops to simplest T2 equivalent
	# Add all hints to vertical and horizontal hint lists as encountered; insert a HintMask class whenever a
	# new set of hints is encountered
	# after all operators have been processed, convert HintMask items into hintmask ops and hintmask bytes
	# add all hints as prefix
	# review operator list to optimize T2 operators.
	# if useStem3 == 1, then any counter hints must be processed as stem3 hints, else the opposite.
	# counter hints are used only in LanguageGroup 1 glyphs, aka ideographs
	
	bezString = re.sub(r"%.+?\n", "", bezString) # supress comments
	bezList = re.findall(r"(\S+)", bezString)
	if not bezList:
		return "", None, None
	flexList = []
	hintMask = HintMask(0) # Create an initial hint mask. We use this if there is no explicit initial hint sub.
	hintMaskList = [hintMask]
	vStem3Args = []
	hStem3Args = []
	vStem3List = []
	hStem3List = []
	argList = []
	opList = []
	newHintMaskName = hintMask.pointName
	inPreFlex = False
	
	opIndex = 0
	lastPathOp = None
	curX = 0
	curY = 0
	newOutline = XMLElement("outline")
	outlineItem = None
	seenHints = 0
	
	for token in bezList:
		try:
			val = float(token)
			argList.append(val)
			continue
		except ValueError:
			pass
		if token == "newcolors":
			lastPathOp = token
			pass
		elif token in ["beginsubr", "endsubr"]:
			lastPathOp = token
			pass
		elif token in ["snc"]:
			lastPathOp = token
			hintMask = HintMask(opIndex)
			if opIndex == 0: # If the new colors precedes any marking operator, then we want throw away the initial hint mask we made, and use the new one as the first hint mask.
				hintMaskList = [hintMask]
			else:
				hintMaskList.append(hintMask)
			newHintMaskName = hintMask.pointName
		elif token in ["enc"]:
			lastPathOp = token
			pass
		elif token == "div":
			# I specifically do NOT set lastPathOp for this.
			value = argList[-2]/float(argList[-1])
			argList[-2:] =[value]
		elif token == "rb":
			lastPathOp = token
			hintMask.hList.append(argList)
			argList = []
			seenHints = 1	
		elif token == "ry":
			lastPathOp = token
			hintMask.vList.append(argList)
			argList = []
			seenHints = 1	
		elif token == "rm": # vstem3's are vhints
			seenHints = 1	
			vStem3Args.append(argList)
			argList = []
			lastPathOp = token
			if len(vStem3Args) == 3:
				hintMask.vstem3List.append(vStem3Args)
				vStem3Args = []

		elif token == "rv": # hstem3's are hhints
			seenHints = 1	
			hStem3Args.append(argList)
			argList = []
			lastPathOp = token
			if len(hStem3Args) == 3:
				hintMask.hstem3List.append(hStem3Args)
				hStem3Args = []

		elif token == "preflx1":
			# the preflx1/preflx2 sequence provides the same i as the flex sequence; the difference is that the 
			# preflx1/preflx2 sequence provides the argument values needed for building a Type1 string
			# while the flex sequence is simply the 6 rcurveto points. Both sequences are always provided.
			lastPathOp = token
			argList = []
			inPreFlex = True # need to skip all move-tos until we see the "flex" operator.
		elif token == "preflx2":
			lastPathOp = token
			argList = []
		elif token == "flx":
			inPreFlex = False
			flexPointName = kBaseFlexName + str(opIndex).zfill(4)
			flexList.append(flexPointName)
			curveCnt = 2
			i = 0
			# The first 12 args are the 6 args for each of the two curves that make up the flex feature.
			while i < curveCnt:
				curX += argList[0]
				curY += argList[1]
				showX, showY = convertCoords(curX, curY)
				newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
				outlineItem.append(newPoint)
				curX += argList[2]
				curY += argList[3]
				showX, showY = convertCoords(curX, curY)
				newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
				outlineItem.append(newPoint)
				curX += argList[4]
				curY += argList[5]
				showX, showY = convertCoords(curX, curY)
				newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY), "type": "%s" % ('curve') } )
				outlineItem.append(newPoint)
				opList.append([opName,curX,  curY])
				opIndex += 1
				if i == 0:
					argList = argList[6:12]
				i += 1
			outlineItem[-6].set(kPointName, flexPointName) # attach the point name to the first point of the first curve.
			lastPathOp = token
			argList = []
		elif token == "sc":
			lastPathOp = token
			pass
		elif token == "cp":
			pass
		elif token == "ed":
		 	pass
		else: 
			if inPreFlex and (token[-2:] == "mt"):
				continue
				
			if token[-2:] in ["mt", "dt", "ct", "cv"]:
				lastPathOp = token
				opIndex += 1
			else:
				print "Unhandled operation", argList, token
				raise BezParseError("Unhandled operation: '%s' '%s'.", argList, token)
			dx = dy = 0
			opName = bezToUFOPoint[token]
			if  token[-2:] in ["mt", "dt"]:
				if token in ["rmt", "rdt"]:
					dx = argList[0]
					dy = argList[1]
				elif token in ["hmt", "hdt"]:
					dx = argList[0]
				elif token in ["vmt", "vdt"]:
					dy = argList[0]
				curX += dx
				curY += dy
				showX, showY = convertCoords(curX, curY)
				newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY), "type": "%s" % (opName) } )
				
				if opName == "move":
					if outlineItem != None:
						if len(outlineItem) == 1:
							# Just in case we see two moves in a row, delete the previous outlineItem if it has only the move-to''
							print "Deleting moveto:", xmlToString(newOutline[-1]), "adding", xmlToString(outlineItem)
							del newOutline[-1]
						else:
							fixStartPoint(outlineItem, opList) # Fix the start/implied end path of the previous path.
					opList = []
		 			outlineItem = XMLElement('contour')
					newOutline.append(outlineItem)

				if (newHintMaskName != None):
					newPoint.set(kPointName, newHintMaskName)
					newHintMaskName = None
				outlineItem.append(newPoint)
				opList.append([opName,curX,  curY])
			else:
				if token in ["rct", "rcv"]:
					curX += argList[0]
					curY += argList[1]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curX += argList[2]
					curY += argList[3]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curX += argList[4]
					curY += argList[5]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY), "type": "%s" % (opName) } )
					outlineItem.append(newPoint)
				elif token == "vhct":
					curY += argList[0]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curX += argList[1]
					curY += argList[2]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curX += argList[3]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY), "type": "%s" % (opName) } )
					outlineItem.append(newPoint)
				elif token == "hvct":
					curX += argList[0]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curX += argList[1]
					curY += argList[2]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY) } )
					outlineItem.append(newPoint)
					curY += argList[3]
					showX, showY = convertCoords(curX, curY)
					newPoint = XMLElement("point", {"x": "%s" % (showX), "y": "%s" % (showY), "type": "%s" % (opName) } )
					outlineItem.append(newPoint)
				if (newHintMaskName != None):
					outlineItem[-3].set(kPointName, newHintMaskName) # attach the pointName to the first point of the curve.
					newHintMaskName = None
				opList.append([opName,curX,  curY])
			argList = []
		
	if outlineItem != None:
		if len(outlineItem) == 1:
			# Just in case we see two moves in a row, delete the previous outlineItem if it has zero length.
			del newOutline[-1]
		else:
			fixStartPoint(outlineItem, opList)

	# add hints, if any
	# Must be done at the end of op processing to make sure we have seen all the
	# hints in the bez string.
	# Note that the hintmasks are identified in the opList by the point name.
	# We will follow the T1 spec: a glyph may have stem3 counter hints or regular hints, but not both.
	
	hintSetList = None
	if (seenHints) or (len(flexList) > 0):
		hintSetList = XMLElement(kHintSetListName)
		
		# Convert the rest of the hint masks to a hintmask op and hintmask bytes.
		for hintMask in hintMaskList:
			hintMask.addHintSet(hintSetList)
				
		if len(flexList) > 0:
			addFlexHint(flexList, hintSetList)
	return newOutline, hintSetList

def addHintList(hints, hintsStem3, stemList, isH):
	# A charstring may have regular v stem hints or vstem3 hints, but not both.
	# Same for h stem hints and hstem3 hints.
	if len(hintsStem3) > 0:
		hintsStem3.sort()
		numHints = len(hintsStem3) 
		hintLimit = (kStackLimit-2)/2
		if numHints >=hintLimit:
			hintsStem3 = hintsStem3[:hintLimit]
			numHints = hintLimit
		makeStemHintList(hintsStem3, stemList, isH)
		
	else:
		hints.sort()
		numHints = len(hints) 
		hintLimit = (kStackLimit-2)/2
		if numHints >=hintLimit:
			hints = hints[:hintLimit]
			numHints = hintLimit
		makeHintList(hints, stemList, isH)


def addWhiteSpace(parent, level):
	child = None
	childIndent =  os.linesep + ("  "*(level +1))
	prentIndent =  os.linesep + ("  "*(level))
	#print "parent Tag", parent.tag, repr(parent.text), repr(parent.tail)
	for child in parent:
		child.tail = childIndent
		addWhiteSpace(child, level +1)
	if child != None:
		if parent.text == None:
			parent.text =  childIndent
		child.tail = prentIndent
		#print "lastChild Tag", child.tag, repr(child.text), repr(child.tail), "parent Tag", parent.tag
		
	
def convertBezToGLIF(ufoFontData, glyphName, bezString, hintsOnly = False):
	# I need to replace the contours with data from the bez string.
	glyphPath = ufoFontData.getGlyphSrcPath(glyphName)
	
	fp = open(glyphPath, "r")
	data = fp.read()
	fp.close()
	
	glifXML = XML(data)
	
	outlineItem = None
	libIndex = outlineIndex = -1
	outlineIndex = outlineIndex = -1
	childIndex = 0
	for childElement in glifXML:
		if childElement.tag == "outline":
			outlineItem = childElement
			outlineIndex = childIndex
		if childElement.tag == "lib":
			libIndex = childIndex
		childIndex += 1 
		

	newOutlineElement, stemHints = convertBezToOutline(ufoFontData, glyphName, bezString )
	#print xmlToString(stemHints)
	
	if not hintsOnly:
		if outlineItem == None:
			# need to add it. Add it before the lib item, if any.
			if libIndex > 0:
				glifXML.insert(libIndex, newOutlineElement)
			else:
				glifXML.append(newOutlineElement)
		else:
			# remove the old one and add the new one.
			glifXML.remove(outlineItem)
			glifXML.insert(outlineIndex, newOutlineElement)

	# convertBezToGLIF is called only if the GLIF has been edited by a tool. We need to update the edit status
	# in the has map entry.
	# I assume that convertGLIFToBez has ben run before, which will add an entry for this glyph.
	hashEntry = ufoFontData.hashMap[glyphName]
	hashEntry[1] = 1 # mark the glyph as having been edited; any other program should read from the processed layer.
	historyList = hashEntry[2] 
	try:
		programHistoryIndex = historyList.index(ufoFontData.programName)
	except ValueError:
		programHistoryIndex = -1
	if programHistoryIndex >= 0:
		# truncate the history entry after the current position.
		historyList = historyList[:programHistoryIndex+1]
		hashEntry[2] = historyList
	
	# Add the stem hints.
	if (stemHints != None):
		widthXML = glifXML.find("advance")
		if widthXML != None:
			width = int(eval(widthXML.get("width")))
		else:
			width = 1000
		newGlyphHash, dataList = ufoFontData.buildGlyphHashValue(width, newOutlineElement, glyphName)
		# We add this hash to the T1 data, as it is the hash which matches the output outline data.
		# This is not necessarily the same as the the hash of the source data - autohint can be used to change outlines.
		stemHints.set("id", newGlyphHash)
		if libIndex > 0:	
			libItem = glifXML[libIndex]
		else:
			libItem = XMLElement("lib")
			glifXML.append(libItem)
		
		dictItem = libItem.find("dict")
		if dictItem == None:
			dictItem = XMLElement("dict")
			libItem.append(dictItem)
	
		# Remove any existing hint data.
		i = 0
		childList = list(dictItem)
		for childItem in childList:
			i += 1
			if (childItem.tag == "key") and (childItem.text == kHintDomainName):
				dictItem.remove(childItem) # remove key
				dictItem.remove(childList[i]) # remove data item.
		
		key = XMLElement("key")
		key.text = kHintDomainName
		dictItem.append(key)
		dataItem = XMLElement("data")
		dataItem.append(stemHints)
		dictItem.append(dataItem)

	addWhiteSpace(glifXML, 0)
	return glifXML

def checkHashMaps(fontPath, doSync):
	"""
	This function checks that all the glyphs in the processed glyph
	layer have a src glyph hash which matches the hash of the
	corresponding default layer glyph. If not, it returns an error.
	
	If doSync is True, it will delete any glyph in the processed glyph
	layer directory which does not have a matching glyph in the default
	layer, or whose src glyph hash does not match. It will then update
	the contents.plist file for the processed glyph layer, and delete
	the program specific hash maps.
	"""
	msgList = []
	allMatch = True
	
	useHashMap = True
	programName = "CheckOutlines"
	ufoFontData = UFOFontData(fontPath, useHashMap, programName)
	gm = ufoFontData.getGlyphMap()
	gNameList = gm.keys()
	ufoFontData.readHashMap()
	# Don't need to check the glyph hashes if there aren't any.
	if len(ufoFontData.hashMap) > 0:
		for glyphName in gNameList:
			glyphFileName = gm[glyphName]
			glyphPath = os.path.join(ufoFontData.glyphDefaultDir, glyphFileName)
			etRoot = ET.ElementTree()
			glifXML = etRoot.parse(glyphPath)
			outlineXML = glifXML.find("outline")
			failedMatch = 0
			if outlineXML != None:
				hashMap = ufoFontData.hashMap
				try:
					widthXML = glifXML.find("advance")
					if widthXML != None:
						width = int(eval(widthXML.get("width")))
					else:
						width = 1000
					oldHash, oldEdited, historyList = hashMap[glyphName] 
					newHash, dataList = ufoFontData.buildGlyphHashValue(width, outlineXML, glyphName)
					#print "\toldHash", oldHash
					if oldHash != newHash:
						failedMatch = 1
				except KeyError:
					pass
			else:
				continue
		
			if failedMatch:
				allMatch = False
				if len(msgList) < 10:
					msgList.append("src glyph %s is newer than processed layer. programName: %s" % (glyphName, programName))
					#print "glyph differs", glyphName
					#print "\told hash map:", oldHash
					#print "\tnew hash map:", newHash
					
				elif len(msgList) == 10:
					msgList.append("More glyphs differ between source and processed layer.")
				else:
					continue
	if doSync:
	 	fileList = os.listdir(ufoFontData.glyphWriteDir)
	 	fileList = filter(lambda fileName: fileName.endswith(".glif"), fileList)
	 	
	 	# invert glyphMap
	 	fileMap = {}
	 	for glyphName, fileName in ufoFontData.glyphMap.items():
	 		fileMap[fileName] = glyphName
	 		
	 	for fileName in fileList:
	 		if  fileMap.has_key(fileName) and ufoFontData.hashMap.has_key(fileMap[fileName]):
	 			continue
	 		
	 		# Either not in glyphMap, or not in hashMap. Exterminate.
			try:
				glyphPath = os.path.join(ufoFontData.glyphWriteDir, fileName)
				os.remove(glyphPath)
				print "Removed outdate file:", glyphPath
			except OSError:
				print "Cannot delete outdated file:", glyphPath
	return allMatch, msgList
				
			
		

	if len(msgList) == 0:
		retVal = None
	else:
		retVal = msgList
	return msgList
	
def test1():
	import pprint
	ufoFontData = UFOFontData(sys.argv[1])

	if len(sys.argv) > 2:
		gNameList = sys.argv[2:]
	else:
		gm = ufoFontData.getGlyphMap()
		gNameList = gm.keys()
		
	for glyphName in gNameList:
		bezString = convertGLIFToBez(ufoFontData, glyphName)
		print bezString
		glifXML = convertBezToGLIF(ufoFontData, glyphName, bezString )
		#pprint.pprint(xmlToString(glifXML))
	print len(gNameList)
	
def test2():
	checkHashMaps(sys.argv[1], False)
	
if __name__=='__main__':
	test2()
