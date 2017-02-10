from __future__ import print_function, division, absolute_import

__copyright__ = """Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
buildCFF2VF.py  1.2 Jan 27 2017
Build a variable font from a Superpolator design space file and the UFO master source fonts.

python buildCFF2VF.py -h
python buildCFF2VF.py -u
python buildCFF2VF.py  <path to design space file> <path to output variable font> 
"""

__help__  = __usage__  + """
The script makes a number of assumptions.
1) all the master source fonts are blend compatible in all their data.
2) The source OTF files are in the same directory as the master source fonts, and have the same file name but with an extension of ".otf' rather than '.ufo'
3) The master source OTF fonts were built with the companion script 'buildMasterOTFs.py'. This does a first pass of compatibilization by using 'tx' with the '-no_opt' option to undo T2 charstring optimization applied by makeotf.

The variable font inherits all the OpenType Tables except CFF2 and GPOS from the default master source font. The default font is flagged in the design space file by having the element "<info copy="1" />" in the <source> element.

The width values and the GPOS positioning data are drawn from all the master source fonts, so each must be built with with a full set of GPOS features.

The companion script buildMasterOTFs.py will build the master source OTFs from the design space file.

Any python interpreter may be used to run the script, as long as it has installed the latest version of the fonttools module from https://github.com/fonttools/fonttools
"""

import os
import re
import sys
import traceback
import copy
import fontTools
import fontTools.varLib
from fontTools.varLib import designspace, models
from fontTools.misc import xmlWriter
from fontTools.ttLib import TTFont, getTableModule, newTable
from fontTools.cffLib import VarStoreData, buildOpcodeDict, privateDictOperators, CharStrings
from fontTools.misc.psCharStrings import T2OutlineExtractor, CFF2CharString
from fontTools.pens.t2CharStringPen import T2CharStringPen, makeRoundFunc
roundPoint = makeRoundFunc(0.5)
import collections
try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring

kMaxStack = 48
kTempCFFSuffix = ".temp.cff"
kTempCFF2File = "test.cff2"

class ACFontError(KeyError):
	pass
	
def logMsg(err):
	print(err)

def openOpenTypeFile(path):
	try:
		ttFont = TTFont(path)
	except:
		logMsg( "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
		logMsg("Attempted to read font %s  as CFF." % path)
		raise ACFontError("Error parsing font file <%s>." % path)

	fontData = CFF2FontData(ttFont, path)
	return fontData


class CFF2FontData:
	def __init__(self, ttFont, srcPath):
		self.ttFont = ttFont
		self.srcPath = srcPath
		try:
			self.cffTable = ttFont["CFF "]
			topDict =  self.cffTable.cff.topDictIndex[0]
		except KeyError:
			raise focusFontError("Error: font is not a CFF font <%s>." % fontFileName)

		#    for identifier in glyph-list:
		# 	Get charstring.
		self.topDict = topDict
		self.privateDict = topDict.Private
		self.nominalWidth = topDict.Private.nominalWidthX
		self.defaultWidth = topDict.Private.defaultWidthX
		self.charStrings = topDict.CharStrings
		self.charStringIndex = self.charStrings.charStringsIndex
		self.allowDecimalCoords = False

class OpListExtractor(T2OutlineExtractor):
	def __init__(self, pen, subrs, globalSubrs, nominalWidthX, defaultWidthX):
		super(OpListExtractor, self).__init__(pen, subrs,globalSubrs, nominalWidthX, defaultWidthX)
		self.seenHints = None
		self.hintmaskLen = 0
		self.hints = []
		
	def countHints(self):
		args = self.popallWidth()
		self.hintCount = self.hintCount + len(args) // 2
		return args

	def op_hstem(self, index):
		self.seenHints = 'h'
		args = self.countHints()
		self.pen.hstem(args)

	def op_vstem(self, index):
		self.seenHints = 'v'
		args = self.countHints()
		self.pen.vstem(args)

	def op_hstemhm(self, index):
		self.seenHints = 'h'
		args = self.countHints()
		self.pen.hstemhm(args)

	def op_vstemhm(self, index):
		self.seenHints = 'v'
		args = self.countHints()
		self.pen.vstemhm(args)

	def op_hintmask(self, index):
		if not self.sawMoveTo:
			# are processing the first hintmask.
			if not self.seenHints:
				args = self.countHints()
				if args:
					self.pen.op_hstemhm(args)
			elif self.seenHints == 'h':
				args = self.countHints()
				if args:
					# There may be hstem hints but no vstem hints.
					self.pen.vstemhm(args)
		if not self.hintmaskLen:
			self.countHints()
			self.hintmaskLen = (self.hintCount + 7) // 8
		hintMaskBytes, index = self.callingStack[-1].getBytes(index, self.hintmaskLen)
		self.pen.hintmask(hintMaskBytes)
		return hintMaskBytes, index

	def op_cntrmask(self, index):
		if not self.sawMoveTo:
			# are processing the first hintmask.
			if not self.seenHints:
				args = self.countHints()
				if args:
					self.pen.op_hstemhm(args)
			elif self.seenHints == 'h':
				args = self.countHints()
				if args:
					# There may be hstem hints but no vstem hints.
					self.pen.vstemhm(args)
		if not self.hintmaskLen:
			self.countHints()
			self.hintmaskLen = (self.hintCount + 7) // 8
		hintMaskBytes, index = self.callingStack[-1].getBytes(index, self.hintmaskLen)
		self.pen.cntrmask(hintMaskBytes)
		return hintMaskBytes, index

class OpListPen(T2CharStringPen):
    # We use this to de-optimize the T2Charstrings to just rmoveto, rlineto, and rrcurveto.
    def __init__(self, supportHints):
        super(OpListPen, self).__init__(0, {})
        self.opList = []
        self.absMovetoPt = [0,0]
        self.supportHints = supportHints
        
    def _moveTo(self, pt):
        super(OpListPen, self)._moveTo(pt)
        self.absMovetoPt = pt

    def _storeHeldMove(self):
        if self._heldMove is not None:
            self.opList.append(["rmoveto", self._heldMove[:2]])
            self._program.extend(self._heldMove)
            self._heldMove = None

    def _relativeLineTo(self, pt):
        self._storeHeldMove()
        pt = roundPoint(pt)
        x, y = pt
        self.opList.append(["rlineto", [x, y]])

    def _relativeCurveToOne(self, pt1, pt2, pt3):
        self._storeHeldMove()
        pt1 = roundPoint(pt1)
        pt2 = roundPoint(pt2)
        pt3 = roundPoint(pt3)
        x1, y1 = pt1
        x2, y2 = pt2
        x3, y3 = pt3
        self.opList.append(["rrcurveto", [x1, y1, x2, y2, x3, y3]])

    def _closePath(self):
        # Add closing lineto if start and end path are not the same.
        # We need this to compatibilize different master designs where
        # the the last path op in one design is a rlineto and is omitted, while
        # the last path op in another is an rrcurveto.
        if (self._lastX != self.absMovetoPt[0]) or (self._lastY != self.absMovetoPt[1]):
            self.opList.append(["rlineto", [self._lastX - self.absMovetoPt[0], self._lastY - self.absMovetoPt[1]]])

    def vstem(self, args):
        if self.supportHints:
            self.opList.append(["hstem", args])

    def hstem(self, args):
        if self.supportHints:
            self.opList.append(["vstem", args])

    def vstemhm(self, args):
        if self.supportHints:
            self.opList.append(["vstemhm", args])

    def hstemhm(self, args):
        if self.supportHints:
            self.opList.append(["hstemhm", args])
    
    def hintmask(self, hintMaskBytes):
        if self.supportHints:
            self.opList.append(["hintmask", [hintMaskBytes]] )

    def cntrmask(self, hintMaskBytes):
        if self.supportHints:
            self.opList.append(["cntrmask", [hintMaskBytes]] )


    def getCharString(self, private=None, globalSubrs=None):
         return self.opList
        
class CFF2GlyphData:
	hintOpList = ('hintmask', 'cntrmask', 'hstem',  'vstem',  'hstemhm', 'vstemhm')

	def __init__(self, glyphName, gid, masterFontList):
		self.glyphName = glyphName
		self.gid = gid
		self.masterFontList = masterFontList
		self.charstringList = []
		self.mmCharString = None
		
	def addCharString(self, t2CharString):
		self.charstringList.append(t2CharString)

	def fixAdvanceWidth(self, programList, fontIndex):
		# Find first op token. Discard weight value
		opIndex = 0
		while opIndex < len(programList):
			if type(programList[opIndex]) == type(''):
				if (opIndex % 2) == 1:
					del programList[0]
				break
			opIndex += 1
		return programList

	def buildOpList(self, t2Index, supportHints):
		t2String = self.charstringList[t2Index]
		t2Pen = OpListPen(supportHints)
		subrs = getattr(t2String.private, "Subrs", [])
		extractor = OpListExtractor(t2Pen, subrs, t2String.globalSubrs,
					t2String.private.nominalWidthX, t2String.private.defaultWidthX)
		extractor.execute(t2String)
		opList = t2Pen.getCharString(t2String.private, t2String.globalSubrs)
		
		if t2Index == 0:
			# For the master font path, promote all coordinates to a list.
			for opName, ptList in opList:
				for i in range(len(ptList)):
					ptList[i] = [ptList[i]]
			
		return opList
		
	def buildMMData(self):
		# Build MM charstring, and list of  points.
		# First, build a list of path entries for the default master.
		# Each entry is [opName,  [ [arg0], [arg1],..[argn] ]. 
		# Note that each arg is a list: this is so we can add the corresponding
		# arg from each successive master font design, and end up with 
		# [opName,  [ [arg.0.0,...,arg.0.k-1], [arg.1.0,...,arg.1.k-1],..,[arg.n-1.0,...,arg.n-1.k-1] ]
		# where n is the number of arguments for the opName, and k is the number of master designs.
		supportHints = True
		self.opList = opList = self.buildOpList(0, supportHints)
		numOps = len(opList)
		# For each other master in turn, 
		#  add the args for each opName to pointList
		# deal with incompatible path lists because:
		# converting from UFO to Type1 can convert flat curves to lines.
		blendError = 0
		i = 1
		while i < len(self.charstringList):
			opList2 = self.buildOpList(i, supportHints)
			i += 1
			numOps2 = len(opList2)
			opIndex = op2Index = 0
			while opIndex < numOps:
				masterOp, masterPointList = opList[opIndex]
				if (not supportHints) and masterOp in self.hintOpList:
					opIndex +=1 
					continue
					
				# Do minor work to compatibilize charstrings.
				try:
					token, pointList = opList2[op2Index]
				except IndexError:
					print("Path mismatch: different number of points. Glyph name: %s. font index: %s. master font path: %s. Delta font path: %s." % (self.glyphName, i-1, self.masterFontList[0].srcPath, self.masterFontList[i-1].srcPath))
					blendError = 1
					break

				if (not supportHints) and token in self.hintOpList:
					op2Index +=1 
					continue

				if token == 'rlineto' and masterOp == 'rrcurveto':
					self.updateLineToCurve(pointList)
				elif masterOp == 'rlineto' and token == 'rrcurveto':
					self.updateLineToCurve(masterPointList)
					opList[opIndex] = ['rrcurveto', masterPointList]
				elif token != masterOp:
					if supportHints and (masterOp in self.hintOpList or token in self.hintOpList):
						# restart with hint support off.
						supportHints = False
						self.opList = opList = self.buildOpList(0, supportHints)
						numOps = len(opList)
						i = 1
						break
						
					print("Path mismatch 1", self.glyphName, i, self.masterFontList[i].srcPath)
					blendError = 1
					break

				try:
					self.mergePointList(masterPointList, pointList, masterOp in ('hintmask', 'cntrmask'))
				except IndexError:
					if supportHints and masterOp in self.hintOpList or token in self.hintOpList:
						# restart with hint support off.
						supportHints = False
						self.opList = opList = self.buildOpList(0, supportHints)
						numOps = len(opList)
						i = 1
						break
					else:
						print("Path mismatch 2", self.glyphName, i, self.masterFontList[i].srcPath)
						blendError = 1
						break
				opIndex += 1
				op2Index += 1
					
		# Now pointList is the list of number lists' first master value, folowed by the other master values.
		if not supportHints:
			print("\t skipped incompatible hint data for", self.glyphName)
		return blendError
		
	def mergePointList(self, masterPointList, pointList, isHintMask):
		i = 0
		while i < len(masterPointList):
			if isHintMask:
				# can't blend hint masks. Just use the first one.
				masterPointList[i].append(masterPointList[i][0])
			else:
				masterPointList[i].append(pointList[i])
			i += 1
	
	def updateLineToCurve(self, pointList):
		# pointList has a lineto that needs to be a flat curve. Convert to curve by pre-pending [0,0], and appending [0,0]
		if type(pointList[0]) == type([]):
			numMasters = len(pointList[0])
			pointList.insert(0, [0]*numMasters)
			pointList.insert(0, [0]*numMasters)
			pointList.append([0]*numMasters)
			pointList.append([0]*numMasters)
		else:
			pointList.insert(0, 0)
			pointList.insert(0, 0)
			pointList.append(0)
			pointList.append(0)
		
		

def getInsertGID(origGID, fontGlyphNameList, mergedGlyphNameList):
	if origGID == 0:
		return origGID
	newGID = 0
	# try stepping down in GID.
	gid = origGID-1
	while gid >= 0:
		prevName = fontGlyphNameList[gid]
		newGID = mergedGlyphNameList.index(prevName)
		if newGID >= 0:
			newGID += 1
			return newGID
		gid -= 1
	
	# try stepping up in GID.
	numGlyphs = len(fontGlyphNameList)
	gid = origGID+1
	while gid < numGlyphs:
		prevName = fontGlyphNameList[gid]
		newGID = mergedGlyphNameList.index(prevName)
		if newGID >= 0:
			newGID -= 1
			return newGID
		gid += 1
	
	return None
	

def buildMasterList(inputPaths):
	blendError = 0
	cff2FontList = []
	# Collect all the charstrings.
	cff2GlyphList = collections.OrderedDict()
	print("Opening default font", inputPaths[0])
	baseFont = openOpenTypeFile(inputPaths[0])
	cff2FontList.append(baseFont)
	
	# Get the master source data for all the glyphs.
	fontGlyphList = baseFont.ttFont.getGlyphOrder()
	for glyphName in fontGlyphList:
		gid = baseFont.charStrings.charStrings[glyphName]
		cff2GlyphData = CFF2GlyphData(glyphName, gid, cff2FontList)
		cff2GlyphList[glyphName] = cff2GlyphData 

		t2CharString = baseFont.charStringIndex[gid]
		cff2GlyphData.addCharString(t2CharString)
	
	for fontPath in inputPaths[1:]:
		print("Opening", fontPath)
		masterFont = openOpenTypeFile(fontPath)
		cff2FontList.append(masterFont)
		for glyphName in fontGlyphList:
			cff2GlyphData = cff2GlyphList[glyphName]
			gid = masterFont.charStrings.charStrings[glyphName]
			if gid != cff2GlyphData.gid:
				raise ACFontError("GID in master font did not match GID in base font: %s." % (glyphName))
			t2CharString = masterFont.charStringIndex[gid]
			cff2GlyphData.addCharString(t2CharString)
		
	# Now build MM versions.
	print("Reading glyph data...")
	bcDictList = {}
	blendError = 0
	for glyphName in fontGlyphList:
		cff2GlyphData = cff2GlyphList[glyphName]
		blendError = cff2GlyphData.buildMMData()
		if blendError:
			break
	if blendError:
		print("Failed to blend master designs.")
		return None, None, None, None, blendError

	# Now get the MM data for the Private Dict.
	# Not all fonts will have the same keys.
	# The first time a key is encountered, we fill out the
	# value list for that key for all fonts, using the values from
	# the first font which has the key. When other fonts with the
	# key are encountered, the real valeus will overwrite the default values.
	numMasters = len(cff2FontList)
	for i in range(numMasters):
		curFont = cff2FontList[i]
		pd = curFont.privateDict
		for key,value in pd.rawDict.items():
			try:
				valList = bcDictList[key]
				if isinstance(valList[0], list):
					lenValueList = len(value)
					for j in range(lenValueList):
						val = value[j]
						valList[j][i] = val
				else:
					bcDictList[key][i] = value
			except KeyError:
				if isinstance(value, list):
					lenValueList = len(value)
					valList = [None]*lenValueList
					for j in range(lenValueList):
						vList = [value[j]]*numMasters
						valList[j] = vList
					bcDictList[key] = valList
				else:
					bcDictList[key] = [value]*numMasters
	
	return baseFont, bcDictList, cff2GlyphList, fontGlyphList, blendError
	

def pointsDiffer(pointList):
	val = False
	p0 = pointList[0]
	for p in pointList[1:]:
		if p != p0:
			val = True
			break
	return val

def appendBlendOp(op, pointList, deltaWeights):
	blendStack = []
	needBlend = False
	numBlends = len(pointList)
	# pointList is arranged as:
	# [
	#	[m0, m1..mn] # values for each master for arg 0
	#	[m0, m1..mn] # values for each master for arg 1
	#	...
	#	[m0, m1..mn] # values for each master for arg numBlends
	# ]
	# The CFF blend operator expects first the series of args 0-numBlends
	# from the first master
	blendList = []
	
	if op in ('hintmask', 'cntrmask'):
		blendStack.append(op)
		blendStack.append(pointList[0][0])
		return blendStack
		
	for argEntry in pointList:
		masterArg = argEntry[0]
		if pointsDiffer(argEntry):
			blendStack.append(masterArg)
			blendList.append(argEntry)
		else:
			if blendList:
				for masterValues in blendList:
					out = []
					for i,weights in enumerate(deltaWeights):
						m0 = masterValues[0]
						delta = masterValues[i]
						# there are no weight items for the default master (0)
						for j,weight in weights.items():
							delta -= out[j] * weight
						if int(delta) == delta:
							delta = int(delta)
						out.append(delta)
					# First item in out is the default master value;
					# for CFF2 data, that has already been written.
					blendStack.extend(out[1:])
				numBlends = len(blendList)
				blendStack.append(numBlends)
				blendStack.append('blend')
				blendList = []
			blendStack.append(masterArg)
	if blendList:
		for masterValues in blendList:
			out = []
			for i,weights in enumerate(deltaWeights):
				m0 = masterValues[0]
				delta = masterValues[i]
				# there are no weight items for the default master (0)
				for j,weight in weights.items():
					delta -= out[j] * weight
				if int(delta) == delta:
					delta = int(delta)
				out.append(delta)
			# First item in out is the default master value;
			# for CFF2 data, that has already been written.
			blendStack.extend(out[1:])
		numBlends = len(blendList)
		blendStack.append(numBlends)	
		blendStack.append('blend')
		blendList = []
	
	blendStack.append(op)
	return blendStack
			
		

def buildMMCFFTables(baseFont, bcDictList, cff2GlyphList, numMasters, varModel):

	pd = baseFont.privateDict
	opCodeDict = buildOpcodeDict(privateDictOperators)
	for key in bcDictList.keys():
		operator, argType = opCodeDict[key]
		valList = bcDictList[key]
		if argType == 'delta':
			# If all masters are the same for each entry in the list, then
			# save a non-blend version; else save the blend version
			needsBlend = False
			for blendList in valList:
				if pointsDiffer(blendList):
					needsBlend = True
					break
			dataList = []
			if needsBlend:
				blendCount = 0
				for blendList in valList:
					dataList.append(blendList)
			else:
				for blendList in valList:
					dataList.append(blendList[0])
		else:
			if pointsDiffer(valList):
				dataList = valList
			else:
				dataList = valList[0]
				
		pd.rawDict[key] = dataList
	
	
	# Now update all the charstrings.
	fontGlyphList = baseFont.ttFont.getGlyphOrder()
	for glyphName in fontGlyphList:
		gid = baseFont.charStrings.charStrings[glyphName]
		t2CharString = baseFont.charStringIndex[gid]
		cff2GlyphData = cff2GlyphList[glyphName]
		t2CharString.decompile()
		newProgram = []
		for op, pointList in cff2GlyphData.opList:
			blendStack = appendBlendOp(op, pointList, varModel.deltaWeights)
			newProgram.extend(blendStack)
		baseFont.charStringIndex[gid] = t2CharString = CFF2CharString(private = t2CharString.private, globalSubrs = t2CharString.globalSubrs)
		
		t2CharString.program = newProgram
		t2CharString.compile()
		
	baseFont.privateDict.defaultWidthX = 0
	baseFont.privateDict.nominalWidthX = 0

def addCFFVarStore(baseFont, varModel, varFont):
	supports = varModel.supports[1:]
	fvarTable = varFont['fvar']
	axisKeys = []
	for axisEntry in supports:
		for axisKey in axisEntry.keys():
			if axisKey in axisKeys:
				continue
			axisKeys.append(axisKey)
	import pprint
	print("CFF2 region list:")
	pprint.pprint(supports)
	varTupleList = fontTools.varLib.builder.buildVarRegionList(supports, axisKeys)
	varTupleIndexes = list(range(len(supports)))
	varDeltasCFFV = fontTools.varLib.builder.buildVarData(varTupleIndexes, None)
	varStoreCFFV = fontTools.varLib.builder.buildVarStore(varTupleList, [varDeltasCFFV])

	cffTable = baseFont.cffTable
	topDict =  cffTable.cff.topDictIndex[0]
	topDict.VarStore = VarStoreData(otVarStore=varStoreCFFV)
	
def addNamesToPost(ttFont, fontGlyphList):
	postTable = ttFont['post']
	postTable.glyphOrder = ttFont.glyphOrder = fontGlyphList
	postTable.formatType = 2.0
	postTable.extraNames = []
	postTable.mapping = {}
	postTable.compile(ttFont)

def convertCFFtoCFF2(baseFont, masterFont):
	# base font contains the CFF2 blend data, but with the table tag 'CFF '
	# all the CFF fields. Remove all the fields that were removed in the
	# CFF2 spec.
	
	cffTable = baseFont.ttFont['CFF ']
	del masterFont['CFF ']
	cffTable.cff.convertCFFToCFF2(masterFont)
	cffTable.tag = 'CFF2'
	data = cffTable.compile(masterFont)
	masterFont['CFF2'] = cffTable

def reorderMasters(mapping, masterPaths):
	numMasters = len(masterPaths)
	new_order = [0]*numMasters
	for i in range(numMasters):
		new_order[mapping[i]] = masterPaths[i]
	return new_order
	

def fixVerticalMetrics(masterFont, masterPaths):
	# Fix the head table bbox, and OS/ winAScent/descent.
	# Record the largest BBOX extents, and use these values.
	print("Updating vertical metrics to most extreme in source fonts.")
	print("\tname old new")
	
	xMax = yMax = ascent = -100000
	xMin = yMin = descent = 100000
	for fPath in masterPaths:
		srcFont = TTFont(fPath)
		headTable = srcFont['head']
		xMax = max(xMax, headTable.xMax)
		yMax = max(yMax, headTable.yMax)
		xMin = min(xMin, headTable.xMin)
		yMin = min(yMin, headTable.yMin)
		hheaTable = srcFont['hhea']
		ascent = max(ascent, hheaTable.ascent)
		descent = min(descent, hheaTable.descent)
	print("\theadTable.xMax", headTable.xMax, xMax)
	print("\theadTable.xMin", headTable.xMin, xMin)
	print("\theadTable.yMax", headTable.yMax, yMax)
	print("\theadTable.yMin", headTable.yMin, yMin)
	headTable = masterFont['head']
	headTable.xMax = xMax
	headTable.yMax = yMax
	headTable.xMin = xMin
	headTable.yMin = yMin
	OS2Table = masterFont['OS/2']
	print("\tOS2Table.usWinAscent", OS2Table.usWinAscent, yMax)
	print("\tOS2Table.usWinDescent", OS2Table.usWinDescent, -yMin)
	OS2Table.usWinAscent = yMax
	OS2Table.usWinDescent = -yMin
	print("\tOS2Table.sTypoAscender", OS2Table.sTypoAscender, ascent)
	print("\tOS2Table.sTypoDescender", OS2Table.sTypoDescender, descent)
	OS2Table.sTypoAscender = ascent
	OS2Table.sTypoDescender = descent
	hheaTable = masterFont['hhea']
	print("\thheaTable.ascent", hheaTable.ascent, ascent)
	print("\thheaTable.descent", hheaTable.descent, descent)
	hheaTable.ascent = ascent
	hheaTable.descent = descent

def fixNameTablePSName(varFont):
	# Split off the style part, if any, and add "-VF"
	nameTable = varFont['name']
	for namerecord in nameTable.names:
		if namerecord.nameID == 6:
			namerecord.string = namerecord.string.rsplit("-",1)[0] + "-VF"

def buildCFF2Font(varFontPath, varFont, varModel, masterPaths):

	inputPaths = reorderMasters(varModel.mapping, masterPaths)
	"""Build CFF2 font from the master designs. default font is first."""
	baseFont, bcDictList, cff2GlyphList, fontGlyphList, blendError = buildMasterList(inputPaths)
	if blendError:
		return blendError
	numMasters = len(inputPaths)
	buildMMCFFTables(baseFont, bcDictList, cff2GlyphList, numMasters, varModel)
	addCFFVarStore(baseFont, varModel, varFont)
	addNamesToPost(varFont, fontGlyphList)
	convertCFFtoCFF2(baseFont, varFont)
	fixVerticalMetrics(varFont, masterPaths)
	fixNameTablePSName(varFont)
	varFont.save(varFontPath)
	return blendError
	
def otfFinder(s):
	return s.replace('.ufo', '.otf')
	
def buildMergedGlyphList(master_fonts):
	# Build a merged glyph name list.
	
	# Start with the glyph name list from mater_fonts[0].
	mergedGlyphNameList = copy.deepcopy(master_fonts[0].getGlyphOrder())
	seenGlyphDict = {}
	for glyphName in mergedGlyphNameList:
		seenGlyphDict[glyphName] = None
		
	for masterFont in master_fonts[1:]:
		fontGlyphList = masterFont.getGlyphOrder()
		gid = -1
		for glyphName in fontGlyphList:
			gid += 1
			try:
				seen = seenGlyphDict[glyphName]
			except KeyError:
				seenGlyphDict[glyphName] = None
				newGID = getInsertGID(gid, fontGlyphList, mergedGlyphNameList)
				if newGID == None:
					mergedGlyphNameList.append(glyphName)
				else:
					mergedGlyphNameList.insert(newGID, glyphName)

	return mergedGlyphNameList
	
def compatibilizeCFF2Table(master_fonts, mergedGlyphNameList):
	# We allow master fonts to not all have the same glyph set.

	numGlyphs = len(mergedGlyphNameList)

	# Find a default charstring for each glyph
	fallbackCharDict = {}
	for glyphName in mergedGlyphNameList:
		mi = 0
		for masterFont in master_fonts:
			cffFont = masterFont['CFF '].cff
			TDCharStrings = cffFont.topDictIndex[0].CharStrings
			mi += 1
			try:
				gid = TDCharStrings.charStrings[glyphName]
				t2CharString = TDCharStrings.charStringsIndex[gid]
				fallbackCharDict[glyphName] = copy.deepcopy(t2CharString)
				break
			except KeyError:
				continue

	# Now add new charstrings, in the new order
	for masterFont in master_fonts:
		cffFont = masterFont['CFF '].cff
		topDict = cffFont.topDictIndex[0]
		TDCharStrings = topDict.CharStrings
		newCharStringDict = {}
		for glyphName in mergedGlyphNameList:
			try:
				t2CharString = TDCharStrings[glyphName]
			except KeyError:
				t2CharString = fallbackCharDict[glyphName]
			newCharStringDict[glyphName] = t2CharString
			
		topDict.charset = mergedGlyphNameList
		topDict.CharStrings.charStrings = newCharStringDict
		topDict.CharStrings.charStringsAreIndexed = 0
	return

def compatibilize_hmtx(master_fonts, mergedGlyphNameList):
	# Find a default advance width for each glyph
	fallbackDict = {}
	for glyphName in mergedGlyphNameList:
		for masterFont in master_fonts:
			metrics = masterFont["hmtx"].metrics
			try:
				metricEntry = metrics[glyphName]
				fallbackDict[glyphName] = copy.deepcopy(metricEntry)
				break
			except KeyError:
				continue
				
	# Now add widths
	for masterFont in master_fonts:
		metrics = masterFont["hmtx"].metrics
		for glyphName in mergedGlyphNameList:
			try:
				metricEntry = metrics[glyphName]
			except KeyError:
				 metrics[glyphName] = fallbackDict[glyphName]

def compatibilizeMasters(designspace_filename, master_finder):
	masters, instances = designspace.load(designspace_filename)
	basedir = os.path.dirname(designspace_filename)
	master_ttfs = [master_finder(os.path.join(basedir, m['filename'])) for m in masters]
	master_fonts = [TTFont(ttf_path) for ttf_path in master_ttfs]
	mergedGlyphNameList = buildMergedGlyphList(master_fonts)
	
	# We need to decompile hmtx to a dict[glyphName] = metricEntry
	# before we change CFF2. We will change glyph index and order for all 
	# the master fonts when updating the CFF table, and this would produce an 
	# incorrect mappingof glyph names to metrics if we decompiled
	# the hmtx after this change.
	for masterFont in master_fonts:
		metrics = masterFont["hmtx"].metrics
		gpod = masterFont["GPOS"].table.LookupList

	compatibilizeCFF2Table(master_fonts, mergedGlyphNameList)
	compatibilize_hmtx(master_fonts, mergedGlyphNameList)
	numMasters = len(master_fonts)
	for i in range(numMasters):
		masterFont = master_fonts[i]
		masterFont.setGlyphOrder(mergedGlyphNameList)
		fontPath = master_ttfs[i]
		masterFont.save(fontPath)

def run(args=None):
	if args is None:
		args = sys.argv[1:]
	if '-u' in args:
		print(__usage__)
		return
	if '-h' in args:
		print(__help__)
		return
		
	try:
		(designSpacePath,varFontPath) = args
	except:
		print(__usage_)
		return
	
	axisMap = {
		'weight':  ('wght', 'Weight'),
		'width':   ('wdth', 'Width'),
		'slant':   ('slnt', 'Slant'),
		'optical': ('opsz', 'Optical Size'),
		'contrast':  ('ctst', 'Contrast'),
		'italic':  ('ital', 'Italic'),
		'custom':  ('xxxx', 'Custom'),
	}
	
	if os.path.exists(varFontPath):
		os.remove(varFontPath)
	try:
		varFont, varModel, masterPaths = fontTools.varLib.build(designSpacePath, otfFinder, axisMap)
	except KeyError:
		# force fonts to have same glyph set.
		compatibilizeMasters(designSpacePath, otfFinder)
		varFont, varModel, masterPaths = fontTools.varLib.build(designSpacePath, otfFinder, axisMap)
		
	blendError = buildCFF2Font(varFontPath, varFont, varModel, masterPaths)
	if not blendError:	
		print("Built variable font '%s'" % (varFontPath))

if __name__=='__main__':
	run()
	

