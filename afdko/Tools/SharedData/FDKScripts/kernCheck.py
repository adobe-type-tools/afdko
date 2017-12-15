#!/bin/env python

__copyright__ = """Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
kernCheck performs several (lengthy) checks on the GPOS table kern
feature in an OpenType font.

It first looks for glyph pairs that collide/overlap. It uses the 'tx'
tool to rasterize glyphs at 200 pts, and uses the 'spot -t GPOS=7
<font>" command to get the kerning data. It then checks every glyph 
pair by overlapping the bitmaps at the  (advance width + kern
adjustment) to see if any pixels overlap. By default, it checks only
kerned glyph pairs, but with an option it will check all possible glyph
pairs.

If the doAll option is used, kernCheck will check all the
possible glyph pairs for collisions.

if the -doMore option is used, kernCheck will check all the kern
pairs for overlap, and it will check all the glyph pairs that do not
meet any of the following criteria:

	- either left or right glyph is a swash glyph: suffix is ".swsh"
	or ".swash".

	- the right and left glyphs belong to different scripts. This is
	tested by trying to extract a Unicode value for both glyphs, and
	assigning a script according to groups of Unicode block ranges.

	- the right glyphs is intended only to begin a word (suffix
	ends in '.init' or '.begin'), and the left glyph is not punctuation.

	- the left glyph is lowercase, and the right glyph is uppercase
	or smallcaps

The kernCheck script then looks to see if any class kern statements block
any other kern statements. This is an issue only when subtable breaks
are used, or when regular kerning and contextual kerning is used. The
script can only deal with contextual kern pos statements that use a
single marked glyph.

NOTE! the time needed is related to the square of the number of glyphs.
This script will run in a few minutes for a font with 300 glyphs, but can
take more than hour for a font with 3000 glyphs.

For those interested in programming, this script also contains a
disabled method for automatically calculating line-spacing. My idea was
to use the calculated line-spacing to detect bad kern values. However,
the method has so  many bad results that it generated far too many false
positives to be useful. I still have hope that the methods can be
improved enough to be useful, but that will have to wait. This is not
the InDesign method, which actually works less well.
"""
__usage__ = """kernCheck v 1.6 January 16 2014
kernCheck [-u] [-h]
kernCheck [-ppEM <integer>] [-doAll] [-log <path to output log file>] -limit <number> -sortByName -sortByArea <path to font file>
kernCheck  -ptSize <number> -limit <number> -sortByName -sortByArea -makePDF <font file path> [<log file path>] [<pdf file path>]
kernCheck  -subtableCheck [-log <path to output log file>] <path to font file>

Note that with the -makePDf option, kernCheck requires as input the
log file which is made by running kernCheck previously with the -log
option.

-u                : write usage

-h                : show help: explains logic for ignoring some glyph pairs.

-ppEM <integer>   : Sets the  size of the bitmaps used for the glyph overlap checks. Default value is 100 pixels per em-square.

-doMore           : The glyph overlap check will check most of the possible glyph pairs; by default, only kerned pairs are checked.

-doAll            : The glyph overlap check will check all possible glyph pairs; by default, only kerned pairs are checked.

-log< path to output log file>
                  : Write output to the named log file. By default, the log file has same name and location as font file, but with suffix ".kc.txt".

-limit <integer>  : Omit all glyph pairs with an overlap less than the limit.

-sortByArea       : Sort output by size of overlap area; default is the log file order. Affects log file and PDF, but not messages during processing.
 
-sortByName       : Sort output by left and right glyph names; default is the log file order. Affects log file and PDF, but not messages during processing.

-subtableCheck    : Do only the check of subtables to see if any GPOS table kerning rules mask other rules.

-makePDF  <font file path>  [<log file path>] [<pdf file path>]
                  : Write a PDF file of the glyph overlaps. This option
                  works on data from the log file in order to allow the
                  user to edit and sort the data before making a PDF. If
                  the log file path is omitted, then a default path is
                  formed by adding the suffix 'kc.txt" to the font file
                  path. If the path to the output PDF file is omitted,
                  it is constructed by adding ".pdf" to the log file
                  path.

-ptSize <number> : Set the point size of the glyph pair outline in the PDF report.
"""

__help__ = __doc__


__methods__ = """

"""

import sys
import os
import re
import FDKUtils
import math
import agd
import traceback

kDefaultppEM = 100
kLogFileExt = ".kc.txt"
kPDFExt = ".pdf"

gAGDDict = {}

kAllGlyphs = "AllGlyphNames" # A key in seenLeftSideDict indicating that the left side was a regular class name, and will mask all subsequent kerns
								# in the same lookup with the same left-side glyphs.

class Reporter:
	def __init__(self,logFileName = None):
		self.fp = None
		self.fileName = logFileName
		if logFileName:
			try:
				self.fp = open(logFileName, "wt")
			except (OSError,IOError):
				print "Error: could not open file <%s>. Will not save log." % (logFileName)
				print "\t%s" % (traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
				
	def write(self, *args):
		hasFP = self.fp
		for arg in args:
			print arg
				
	def close(self, fontPath, sortType, overlapList, conflictMsgList):
		fontName = os.path.basename(fontPath)
		if self.fp:
			if sortType == 1:
				overlapList = map(lambda entry: (entry[2:] + entry[:2]), overlapList)
				overlapList.sort()
				overlapList = map(lambda entry: (entry[3:] + entry[:3]), overlapList)
			elif sortType == 2:
				overlapList.sort()
				overlapList.reverse()
			textList = map(lambda entry: "\tError: %s pixels overlap at %s ppem In glyph pair '%s %s with kern %s'." % (entry[0], entry[1], entry[2],  entry[3], entry[4]), overlapList)
			data = os.linesep.join(textList)
			self.fp.write("# kernCheck report for font: %s. %s" % (fontName, os.linesep))
			self.fp.write("%s### Glyph pairs with overlapping contours.%s" % (os.linesep, os.linesep))
			if not data.strip():
				data = "\tNone" + os.linesep
			self.fp.write(data)
			self.fp.write("%s### Conflicting GPOS kern rules.%s" % (os.linesep, os.linesep))
			data = os.linesep.join(conflictMsgList)
			if not data.strip():
				data = "\tNone" + os.linesep
			self.fp.write(data)
			self.fp.close()
			print "Log saved to file <%s>." % (self.fileName)

logMsg = None

class KernSubtable:
	def __init__(self, leftClassDict, rightClassDict, kernPairDict, backTrack = None, lookAhead = None):
		self.leftClassDict = leftClassDict
		self.rightClassDict = rightClassDict
		self.backTrack = backTrack
		self.lookAhead = lookAhead
		self.kernPairDict = kernPairDict
		
	def __repr__(self):
		return os.linesep.join(["", "Kern Subtable:", "Left Classes: " + repr(self.leftClassDict), "Right Classes: " + repr(self.rightClassDict), "Kern Dict: " + repr(self.kernPairDict)])
		
def parseContextPos(line):
	backTrack = lookAhead = leftSide = rightSide = valueRecord = None
	
	tokens = line.split()
	tokens = tokens[1:] # get rid of initial pos.
	if 0: # len(tokens) == 133:
		import pdb
		pdb.set_trace()
		
	numTokens = len(tokens)
	i = 0
	runRecords = [] # Each run record is a [ [glyph list}, value-record]
	
	classList = None
	while  i < numTokens:
		token = tokens[i]
		i += 1
		glyphRun = None
		tokenIsMarked = 0
		if token[-1] == "'":
			tokenIsMarked = 1
			token = token[:-1]

		if token[0] == "[":
			if classList != None:
				raise ValueError("Error: saw beginning of class without ending previous class.")
				
			token = token[1:]
			classList = []
			if not token:
				continue
			if len(token) > 1:
				if token[-1] == "]":
					token = token[:-1]
					classList.append(token)
					glyphRun = classList
					classList = None
			else:
				classList.append(token)
		elif  token[-1] == "]":
			if classList == None:
				raise ValueError("Error: saw end of class without beginning  class.")
			if len(token) > 1:
				token = token[:-1]
				classList.append(token)
			glyphRun = classList
			classList = None
		elif classList != None:
			classList.append(token)
		else:
			glyphRun = token
			
		if tokenIsMarked and not glyphRun:
			raise ValueError("Error: saw marked token without defining a glyph run!.")
		if not glyphRun:
			continue
		if not tokenIsMarked:
			if not runRecords:
				if not backTrack:
					backTrack = []
				backTrack.append(glyphRun)
			else:
				if token == "lookup":  # Just skip this, and the following looup[ index.
					token = tokens[i]
					i += 1
					continue
					
				if not lookAhead:
					lookAhead = []
				lookAhead.append(glyphRun)
			glyphRun = None
		else:
			# we have a marked record
			if lookAhead:
				raise ValueError("Error: seeing a seond marked glyph run.")
			
			# Next token may  be the value record, either an integer or a value record in the form < a b c d >
			token = tokens[i]
			i += 1
			valueRecord = None
			if not token[0] == "<":
				try:
					# maybe it is an integer.
					valueRecord = eval(token)
				except (ValueError, NameError, SyntaxError):
					# it must be a new glyph/class token
					i -= 1
					runRecords.append([glyphRun, [0,0,0,0]])
					glyphRun = None
					continue
			else:
				# token begins with "<".
				valueList = []
				numValues = 0
				if len(token) > 1:
					valueList.append(token[1:])
					numValues = 1
				while numValues < 4:
					numValues +=1
					token = tokens[i]
					valueList.append(token)
					i +=1
				if token[-1] == ">":
					token = token[:-1]
					valueList[-1] = token
				else:
					token = tokens[i]
					i += 1
					if not token == ">":
						logMsg("\tError:value record < a b c d > is not terminated! '%s'." % (line))
						return None, None, None, None, None
				
				for j in range(4):
					try:
						valueList[j] = eval(valueList[j])
					except (ValueError, NameError):
						logMsg("\tError:value record < a b c d > has value which is not an integer! '%s'." % (line))
						return None, None, None, None, None
				valueRecord = valueList
			runRecords.append([glyphRun, valueRecord])
			glyphRun = None

	if not runRecords:
		logMsg("\tError:Failed to find marked runs in contextual pos statement! '%s'." % (line))
		raise ValueError("Hello")
		return None, None, None, None, None
		
	if backTrack and (len(runRecords) == 1):
		# usual format  pos A B' value;
		leftSide = backTrack[-1]
		backTrack = backTrack[:-1]
		if not backTrack:
			backTrack = None
		if type(leftSide) != type(""):
			leftSide = tuple(leftSide)
		rightSide = runRecords[0][0]
		if type(rightSide) != type(""):
			rightSide = tuple(rightSide)
		valueRecord = runRecords[0][1]
		
	elif len(runRecords) == 2:
		leftSide = runRecords[0][0]
		if type(leftSide) != type(""):
			leftSide = tuple(leftSide)
		rightSide = runRecords[1][0]
		if type(rightSide) != type(""):
			rightSide = tuple(rightSide)
		if runRecords[1][1]:
			valueRecord = runRecords[1][1]
		if (runRecords[0][1]):
			logMsg("\tError: Can deal only with value record at end of marked glyph run. ! '%s'." % (line))
	return  backTrack, lookAhead, leftSide, rightSide, valueRecord 
	
	
def parseKernLookup(featureText, fontFlatKernTable, lookup):
	# leftTupleDict is keyed by glyph class defintion, and the value is a class name. Same for rightTupleDict
	# leftDict is keyed by unique class name, and the value is the class glyph name list as a tuple.
	# kernPairDict is keyed by left-side names, class or otherwise. Value is rightSideDict. This is keyed by 
	# right side names, class or otherwise, and values are a value record.
	# fontFlatKernTable is the same idea as kernPairDict, but with classes flattened, every left-side glyph in any class def
	# is a key, as is any right-side glyph.
	
	textBlocks = featureText.split("# -------- SubTable")
	subtableList = []
	if textBlocks:
		sti = -1
		for textBlock in textBlocks:
			sti +=1
			isClassPair = 0
			contextEntry = None
			if re.search(r"pos\s+[^;]+?'", textBlock):
				# it is a contextual positioning rule
				isContextual = 1
				posList = re.findall(r"\s(pos\s+[^;]+);", textBlock)
				leftDict = {}
				leftTupleDict = {}
				rightDict = {}
				rightTupleDict = {}
				kernPairs = []
				for line in posList:
					backTrack, lookAhead, leftSide, rightSide, valueRecord = parseContextPos(line)
					if leftSide == None:
						continue

					if type(leftSide) == type(()):
						try:
							leftName = leftTupleDict[leftSide]
						except KeyError:
							leftName = "@CONTEXT_LEFT_CLASS_%s" % (len(leftDict))
							leftDict[leftName] = leftSide
							leftTupleDict[leftSide] = leftName
					else:
						leftName = leftSide
						leftSide = None
						
					if type(rightSide) == type(()):
						try:
							rightName = rightTupleDict[rightSide]
						except KeyError:
							rightName = "@CONTEXT_RIGHT_CLASS_%s" % (len(rightDict))
							rightDict[rightName] = rightSide
							rightTupleDict[rightSide] = rightName
					else:
						rightName = rightSide
						rightSide = None
					
					kernPairs.append( [leftName, rightName, str(valueRecord)])
					contextEntry = [backTrack, lookAhead, leftSide, rightSide]
				
			elif re.search(r"@", textBlock):
				# it is a class kern subtable.
				isClassPair = 1
				glyphLeftClasses = re.findall(r"[\r\n](@LEFT\S+)\s*=\s*\[([^]]+)\]", textBlock)
				leftDict = {}
				for classEntry in glyphLeftClasses:
					leftDict[classEntry[0] ] = classEntry[1].split()
				
				rightDict = {}
				glyphRightClasses = re.findall(r"[\r\n](@RIGHT\S+)\s*=\s*\[([^]]+)\]", textBlock)
				for classEntry in glyphRightClasses:
					rightDict[classEntry[0] ] = classEntry[1].split()
				kernPairs = re.findall(r"pos\s+(\S+)\s+(\S+)\s+(-*\d+)", textBlock)
				kernPairs = filter(lambda kernPair: kernPair[2] != "0", kernPairs)
			else:
				# it is a non-class kern subtable.
				leftDict = rightDict = None
				kernPairs = re.findall(r"pos\s+(\S+)\s+(\S+)\s+(-*\d+)", textBlock)
				kernPairs = filter(lambda kernPair: kernPair[2] != "0", kernPairs)
				
			kernPairDict = {}
			for pairEntry in kernPairs:
				leftName = pairEntry[0]
				rightName = pairEntry[1]
				value = eval(pairEntry[2])
				if type(value) != type(0):
					#value = value[0] + value[2]
					temp = 0
					for v in value:
						temp += abs(v)
					if temp:
						value = str(value)
					else:
						value = 0
				rightSideDict = kernPairDict.get(leftName, {})
				rightSideDict[rightName] = value
				kernPairDict[leftName] = rightSideDict
				
				# Now add define fontFlatKernTable, using the full expansion of the classes.
				leftClassName = rightClassName = None
				try:
					leftList = leftDict[leftName]
					leftClassName = leftName
				except (TypeError,KeyError):
					leftList = [leftName]
				try:
					rightList = rightDict[rightName]
					rightClassName = rightName
				except (TypeError,KeyError):
					rightList = [rightName]

				for leftGlyphName in leftList:
					try:
						rightSideGlyphDict = fontFlatKernTable[leftGlyphName]
					except KeyError:
						rightSideGlyphDict = {}
						fontFlatKernTable[leftGlyphName] = rightSideGlyphDict
						
					for rightGlyphName in rightList:
						try:
							valueList = rightSideGlyphDict[rightGlyphName]
							valueList.append((sti, lookup, leftClassName, rightClassName, value, contextEntry))
							# Don't overwrite a value we have already seen.
						except KeyError:
							rightSideGlyphDict[rightGlyphName] = [(sti, lookup, leftClassName, rightClassName, value, contextEntry)]
					if  isClassPair:
							try:
								# Rather than specifying an entry for every other glyph in the font
								# with a kern value of 0, I add one entry with this name.
								valueList = rightSideGlyphDict[kAllGlyphs]
								valueList.append((sti, lookup, leftClassName, rightClassName, 0, None))
								# Don't overwrite a value we have already seen.
							except KeyError:
								rightSideGlyphDict[kAllGlyphs] = [(sti, lookup, leftClassName, rightClassName, 0, None)]
					
				
					
			if kernPairDict:
				subtable = KernSubtable(leftDict, rightDict, kernPairDict)
				subtableList.append(subtable)
				
	return subtableList, fontFlatKernTable


def collectKernData(fontpath):
	""" return:
		nameDict[leftGlyphName][rightGlyphName] = kern value
		tableList[classDicts], where classDict[leftClassNameList][rightClassNameList] = value
	"""
	scriptDict = {}
	lookupIndexDict = {}
	command = "spot -t GPOS=7 \"%s\"" % (fontpath)
	report = FDKUtils.runShellCmd(command)
	
	featureText = ""
	fontFlatKernTable = {}

	# pick out the feature blocks
	while 1:
		m = re.search("# (Printing|Skipping) lookup (\S+) in feature 'kern' for script '([^']+)' language '([^']+).*(\s+# because already dumped in script '([^']+)', language '([^']+)')*", report)
		if m:
			report = report[m.end(0):]
			lookup = m.group(2)
			script = m.group(3)
			lang = m.group(4)
			langDict = scriptDict.get(script, {})
			lookupDict = langDict.get(lang, {})
			lookupEntry = lookupDict.get(lookup, None)
			lookupDict[lookup] = lookupEntry
			langDict[lang] = lookupDict
			scriptDict[script] = langDict
			if m.group(1) == "Skipping":
				srcScript =  m.group(6)
				scrLang = m.group(7)
				lookupDict[lookup] = scriptDict[srcScript][scrLang][lookup]
				if not lookupDict[lookup]:
					logMsg("\tProgram Error: already seen lookup is empty.")
				continue
				
			# m.group(1) == "Printing" we collect real feature data.
			endm = re.search("# (Printing|Skipping) lookup", report)
			if endm:
				endmPos = endm.start(0)
				featureText = report[:endmPos]
			else:
				featureText = report
			subtableList, fontFlatKernTable = parseKernLookup(featureText, fontFlatKernTable, lookup)
			# since spot is printing out the definition here, it has not done so before.
			lookupDict[lookup] = lookup
			if lookupIndexDict.has_key(lookup):
				logMsg("\tProgram Error: lookupIndexDict already has new lookup index.")
			lookupIndexDict[lookup] = subtableList
			continue
		else:
			break
			
	if not scriptDict:
		logMsg("Error: did not find any kern feature text in the output of the command '%s'." % (command))
		return None, None, None
	
	# Now build the dict mapping unique lookup sequences to script/language pairs.
	lookupSequenceDict = {}
	for script in scriptDict.keys():
		langDict = scriptDict[script]
		for langTag in langDict.keys():
			lookupDict = langDict[langTag]
			lookupSequence = tuple(lookupDict.keys())
			slList = lookupSequenceDict.get(lookupSequence, [])
			slList.append( (script, langTag) )
			lookupSequenceDict[lookupSequence] = slList
	return lookupIndexDict, lookupSequenceDict, fontFlatKernTable

class TXBitMap:
	kIndexPat = re.compile(r"(-*\d+)")
	kAdvWidthPath = re.compile(r"(-*\d+),(-*\d+)")
	def __init__(self,glyphName, txReport, ppEM):
		"""
		A tx bit map is a series of scan lines that covers the vertical
		extent of the glyph bounding box (BBox).

		Each scan line is the same length.

		The length of the scan line is the advance width, plus the
		number of pixels by which the glyph BBox extends beyond the
		origin to the left, and plus the amount by which the glyph Bbox
		extends beyond the end of the advance width to the right. Within
		each scan line, pixels that are on are represented by the
		character '#'. Off pixels between the origin and the advance
		width are rpresented by '.'; off pixels outside this range are
		represented by spaces.

		Each scan line in the  output  from 'tx'is terminated by a space
		and the line number, counting up from the base-line in dex of 0.
		The base-line has an additional suffix, an integer pair
		representing the left side bearing and the advance width.
		"""
		# Since the lines in the report are indexed up from 0 at the base-line, the index
		# in the report of the top line is the lineindex of the base-line
		self.baseLineIndex = eval(self.kIndexPat.search(txReport).group(1))
		m = self.kAdvWidthPath.search(txReport)
		
		self.lsb = eval(m.group(1))
		self.advanceWidth = eval(m.group(2))
		
		self.ppEM = ppEM
		self.glyphName = glyphName
		self.center = None
		
		# get scan lines, remove indices from end, find x pos of bbox right.
		bboxRight = 0
		self.scanLines = scanLines = re.findall(r"[\r\n]([ .#]+)", txReport)
		lineLen = min(len(scanLines[-1]), len(scanLines[0]))
		# The lines have different numbers of spaces, depending on the number and sign of the line index integer.
		for scanLine in scanLines:
			i = lineLen-1
			while (i > bboxRight) and (scanLine[i] != "#"):
				i -= 1
			bboxRight =  max(bboxRight, i)

		self.bboxHeight = len(self.scanLines)
		self.bboxTopIndex = self.baseLineIndex # This is the index of the top line, if you start indexing  with the base line as line 0.
		self.bboxBottomIndex = self.bboxTopIndex + 1 -  self.bboxHeight
		self.rsb = self.advanceWidth - (bboxRight + 1)
		if self.lsb < 0:
			self.rsb -= self.lsb
			# This is because we want the rsb to be relative to (0,0) and the advance width.
			# However, when the lsb is negative, the scan-line starts with abs(lsb) space chars before the (0,0) origin point.
			# so that the bboxRight index is actually at BBox.right + abs(lsb)
		# get rid of extra spaces and line indices at end of lines.
		if self.rsb < 0:
			lineLength = bboxRight+1
		else:
			lineLength = self.advanceWidth
			if self.lsb < 0:
				lineLength -= self.lsb
		
		self.scanLines = map(lambda line: line[:lineLength], scanLines) # get rid of the final spaces and line indices.
		setGlyphClasses(self)
	
	def getCenter(self):
		if self.center == None:
			center = 0
			numI = 0.0
			for scanLine in self.scanLines:
				lineLen = len(scanLine)
				for i in range(lineLen):
					if scanLine[i] == "#":
						center += i
						numI += 1
			if numI == 0.0:
				center = self.advanceWidth/2.0 + self.lsb
			else:
				center = center/numI
		 	self.center = int(0.5 + center)
		 	self.area = numI
			logMsg(self.glyphName, self.lsb, self.advanceWidth, self.rsb, center)
		return self.center
					
				
kClassInfoDict = { # glyph name ; (case value, needs extra space)
					# 1 means is upper case, 0 means it is lowercase, -1 means unclassified.
				"a" : (0, 0),
				"A" : (1, 0),
				"aacute" : (0, 0),
				"Aacute" : (1, 0),
				"acircumflex" : (0, 0),
				"Acircumflex" : (1, 0),
				"adieresis" : (0, 0),
				"Adieresis" : (1, 0),
				"ae" : (0, 0),
				"AE" : (1, 0),
				"agrave" : (0, 0),
				"Agrave" : (1, 0),
				"ampersand" : (-1,1),
				"aring" : (0, 0),
				"Aring" : (1, 0),
				"at" : (-1,1),
				"atilde" : (0, 0),
				"Atilde" : (1, 0),
				"b" : (0, 0),
				"B" : (1, 0),
				"backslash" : (-1,1),
				"braceleft" : (-1,1),
				"braceright" : (-1,1),
				"bracketleft" : (-1,1),
				"bracketright" : (-1,1),
				"bullet" : (-1,1),
				"c" : (0, 0),
				"C" : (1, 0),
				"ccedilla" : (0, 0),
				"Ccedilla" : (1, 0),
				"cent" : (0, 1),
				"colon" : (-1,1),
				"currency" : (-1,1),
				"d" : (0, 0),
				"D" : (1, 0),
				"dagger" : (-1,1),
				"daggerdbl" : (-1,1),
				"divide" : (-1,1),
				"dollar" : (1, 1),
				"e" : (0, 0),
				"E" : (1, 0),
				"eacute" : (0, 0),
				"Eacute" : (1, 0),
				"ecircumflex" : (0, 0),
				"Ecircumflex" : (1, 0),
				"edieresis" : (0, 0),
				"Edieresis" : (1, 0),
				"egrave" : (0, 0),
				"Egrave" : (1, 0),
				"eight" : (1, 0),
				"ellipsis" : (-1,1),
				"equal" : (-1,1),
				"eth" : (0, 0),
				"Eth" : (1, 0),
				"euro" : (1, 1),
				"Euro" : (1, 1),
				"exclam" : (1,1),
				"exclamdown" : (1, 1),
				"f" : (0, 0),
				"F" : (1, 0),
				"five" : (1, 0),
				"florin" : (1, 1),
				"four" : (1, 0),
				"g" : (0, 0),
				"G" : (1, 0),
				"germandbls" : (0, 0),
				"greater" : (-1,1),
				"h" : (0, 0),
				"H" : (1, 0),
				"i" : (0, 0),
				"I" : (1, 0),
				"iacute" : (0, 0),
				"Iacute" : (1, 0),
				"icircumflex" : (0, 0),
				"Icircumflex" : (1, 0),
				"idieresis" : (0, 0),
				"Idieresis" : (1, 0),
				"igrave" : (0, 0),
				"Igrave" : (1, 0),
				"j" : (0, 0),
				"J" : (1, 0),
				"k" : (0, 0),
				"K" : (1, 0),
				"l" : (0, 0),
				"L" : (1, 0),
				"less" : (-1,1),
				"m" : (0, 0),
				"M" : (1, 0),
				"multiply" : (-1,1),
				"n" : (0, 0),
				"N" : (1, 0),
				"nine" : (1, 0),
				"ntilde" : (0, 0),
				"Ntilde" : (1, 0),
				"numbersign" : (-1,1),
				"o" : (0, 0),
				"O" : (1, 0),
				"oacute" : (0, 0),
				"Oacute" : (1, 0),
				"ocircumflex" : (0, 0),
				"Ocircumflex" : (1, 0),
				"odieresis" : (0, 0),
				"Odieresis" : (1, 0),
				"oe" : (0, 0),
				"OE" : (1, 0),
				"ograve" : (0, 0),
				"Ograve" : (1, 0),
				"one" : (1, 0),
				"oslash" : (0, 0),
				"Oslash" : (1, 0),
				"otilde" : (0, 0),
				"Otilde" : (1, 0),
				"p" : (0, 0),
				"P" : (1, 0),
				"paragraph" : (-1,1),
				"parenleft" : (-1,1),
				"parenright" : (-1,1),
				"percent" : (1, 1),
				"periodcentered" : (-1,1),
				"perthousand" : (1, 1),
				"plus" : (-1,1),
				"plusminus" : (-1,1),
				"q" : (0, 0),
				"Q" : (1, 0),
				"question" : (1, 1),
				"questiondown" : (1, 1),
				"r" : (0, 0),
				"R" : (1, 0),
				"s" : (0, 0),
				"S" : (1, 0),
				"scaron" : (0, 0),
				"Scaron" : (1, 0),
				"section" : (-1,1),
				"semicolon" : (-1,1),
				"seven" : (1, 0),
				"six" : (1, 0),
				"slash" : (-1,1),
				"sterling" : (1, 1),
				"t" : (0, 0),
				"T" : (1, 0),
				"thorn" : (0, 0),
				"Thorn" : (1, 0),
				"three" : (1, 0),
				"two" : (1, 0),
				"u" : (0, 0),
				"U" : (1, 0),
				"uacute" : (0, 0),
				"Uacute" : (1, 0),
				"ucircumflex" : (0, 0),
				"Ucircumflex" : (1, 0),
				"udieresis" : (0, 0),
				"Udieresis" : (1, 0),
				"ugrave" : (0, 0),
				"Ugrave" : (1, 0),
				"v" : (0, 0),
				"V" : (1, 0),
				"w" : (0, 0),
				"W" : (1, 0),
				"x" : (0, 0),
				"X" : (1, 0),
				"y" : (0, 0),
				"Y" : (1, 0),
				"yacute" : (0, 0),
				"Yacute" : (1, 0),
				"ydieresis" : (0, 0),
				"Ydieresis" : (1, 0),
				"yen" : (1, 1),
				"z" : (0, 0),
				"Z" : (1, 0),
				"zcaron" : (0, 0),
				"Zcaron" : (1, 0),
				"zero" : (1, 0),
					}

def setGlyphClasses(bitMap):
	glyphName = bitMap.glyphName
	parts = glyphName.split(".", 1)
	baseName = parts[0]
	try:
		suffix = parts[1]
	except IndexError:
		suffix = None
		
	bitMap.needsExtraSpace = 0 # used in  auto-kerning algorithms, not yet done.
	if suffix:
		bitMap.isBegin = suffix.startswith("init") or  suffix.startswith("begin")
		bitMap.isEnd = suffix.startswith("falt") or  suffix.startswith("fin")
		bitMap.isSwash = suffix.startswith("swsh") or  suffix.startswith("swash")
		bitMap.isSC = suffix.startswith("sc") or  suffix.startswith("smcp") or  suffix.startswith("c2sc")
	else:
		bitMap.isBegin =0
		bitMap.isEnd =0
		bitMap.isSwash =0
		bitMap.isSC =0
	
	bitMap.isLig = bitMap.isUC = bitMap.isLC = 0
	if not bitMap.isSC: # if glyph is Sc, assume isUC and isLC is 0, and doesn't matter if it is a ligature.
		# before doing upper/lower case, have to figure out if it is a ligature.
		# as ligatures may have different case on each side.
		ligParts = baseName.split("_")
		numLigParts = len(ligParts)
		if (numLigParts == 1):
			if  baseName.startswith("uni"):
				hexString = baseName[3:]
				numLigParts = len(hexString)/4
				if numLigParts > 1:
					leftLigName = "uni" + hexString[:4]
					rightLigName = "uni" + hexString[-4:]
		else:
			leftLigName = ligParts[0]
			rightLigName = ligParts[1]
			
		if numLigParts > 1:
			bitMap.isLig = 1
			bitMap.LeftisUC, bitMap.LeftisLC = getCase(leftLigName)
			bitMap.RightisUC, bitMap.RightisLC = getCase(leftLigName)
		else:
			bitMap.isLig = 0
			bitMap.isUC, bitMap.isLC = getCase(baseName)
		
	# Now set the script value.
	uv = None
	m = agd.re_uni.match(baseName)
	if m:
		uv = eval("0x" + m.group(1))
	else:
		m = agd.re_u.match(baseName)
		if m:
			uv = eval("0x" +m.group(1))
			
	if uv == None:
		dictGlyph = gAGDDict.glyph(baseName)
		if dictGlyph and dictGlyph.uni:
			uv = eval("0x" + dictGlyph.uni)

	if uv:
		bitMap.script = agd.getscript(uv)
		bitMap.isPunc = agd.kPunctuation.has_key(uv)
		if not bitMap.isPunc:
			bitMap.isPunc = bitMap.script == "Punctuation"
	else:
		bitMap.isPunc =0
		bitMap.script = agd.kUnknownScript
		

		
def getCase(glyphName):
	isUC = isLC = 0
	# try the hard-coded list.
	if kClassInfoDict.has_key(glyphName):
		isUpper, needsExtraSpace = kClassInfoDict[glyphName]
		if isUpper == 1:
			isUC = 1
		elif isUpper == 0:
			isLC = 1
		return isUC, isLC
		
	#Refer to the AGD.
	dictGlyph = gAGDDict.glyph(glyphName)
	if dictGlyph:
		if dictGlyph.maj:
			isUC = 1
		elif dictGlyph.min:
			isLC = 1
	return isUC, isLC


		
			
def checkGlyphOverlap(leftBitMap, rightBitMap, value, limitVal):
	"""
	If the sum of the left RSB and the right bitmap LSB  and the kern
	value are greater than zero, there cannot be an overlap.

	If this is  not the case, then we need to examine the region of
	overlap in each line. On the right side, we start at the edge of the
	BBox left, and index to the left. On the left side, we start at the
	Bbox right in the overlap, and index to the left. If at each index,
	a pixel is on in both  bitmaps, there is overlap.
	"""
	
	ppEM = leftBitMap.ppEM
	origValue = value
	if value >= 0:
		value = int(0.5 + value*ppEM/1000.0)
	else:
		value = int(-0.5 + value*ppEM/1000.0)
	overlapRange = leftBitMap.rsb + rightBitMap.lsb + value
	if overlapRange >= 0:
		return None # Can't have an overlap - there is space between the glyph bounding boxes
		
	bboxTopIndex = min(leftBitMap.bboxTopIndex, rightBitMap.bboxTopIndex)
	bboxBottomIndex = max(leftBitMap.bboxBottomIndex, rightBitMap.bboxBottomIndex)
	bboxHeight = 1 + bboxTopIndex - bboxBottomIndex
	if bboxHeight <=0:
		return None # Can't have an overlap - the top of one glyph's BBox is below the bottom of hte other glyph's BBox.
	leftLineIndex = leftBitMap.bboxTopIndex - bboxTopIndex
	rightLineIndex = rightBitMap.bboxTopIndex - bboxTopIndex
	
	# Get the start of the overlap region in the left and right side lines.
	if rightBitMap.lsb > 0:
		rsStart = rightBitMap.lsb # the lsb is the width of the lsb, so it is also the index of the first 'on' pixel in the line.
		rsEnd = rightBitMap.advanceWidth -1
	else:
		rsStart = 0 # if the rsb is negative, then the first pixel is on.
		rsEnd = rightBitMap.advanceWidth -1 - rightBitMap.lsb

	lsStart = leftBitMap.advanceWidth + overlapRange - (1 + leftBitMap.rsb)
	lsEnd = leftBitMap.advanceWidth - leftBitMap.rsb
	if leftBitMap.lsb < 0:
		lsStart -= leftBitMap.lsb
		lsEnd -= leftBitMap.lsb 
		
	if lsStart < 0:   # very long swashes on the right side can extend further than the left side of the left bitmap.
		lsStart = 0
		# this is because leftBBoxRight needs to be char index in the line,
		# and when lsb < 0, the line starts with the lsb, not at (0,0).

	testRange = -overlapRange
	if testRange >= rsEnd - rsStart: # very long swashes on the left side can extend further than the right side of the right bitmap.
		testRange = rsEnd - rsStart
	if testRange >= lsEnd - lsStart: # very long swashes on the left side can extend further than the right side of the right bitmap.
		testRange = lsEnd - lsStart
	overlapArea = 0
	lineCount = 0
	while lineCount < bboxHeight:
		leftLine = leftBitMap.scanLines[leftLineIndex]
		rightLine = rightBitMap.scanLines[rightLineIndex]
		leftLineIndex += 1
		rightLineIndex += 1
		lineCount += 1
		lsi = lsStart
		rsi = rsStart
		for i in range(testRange):
			if (leftLine[lsi] == "#") and (rightLine[rsi] == "#"):
				overlapArea  +=1
			lsi += 1
			rsi +=1
			
	if overlapArea >= limitVal:
		logMsg("\tError: %s pixels overlap at %s ppem In glyph pair '%s %s with kern %s'." % (overlapArea, ppEM, leftBitMap.glyphName, rightBitMap.glyphName, origValue))
		return overlapArea, ppEM, leftBitMap.glyphName, rightBitMap.glyphName, origValue
	return None

def skipGlyphPair(leftBitMap, rightBitMap):
	skip = 0
	if (rightBitMap.script !=  leftBitMap.script):
		return 1
	if rightBitMap.isBegin and  not leftBitMap.isPunc:
		return 1
		
	if leftBitMap.isLig:
		leftLC = leftBitMap.LeftisLC
		leftUC = leftBitMap.LeftisUC
	else:
		leftLC = leftBitMap.isLC
		leftUC = leftBitMap.isUC
		
	if rightBitMap.isLig:
		rightLC = rightBitMap.LeftisLC
		rightUC = rightBitMap.LeftisUC
	else:
		rightLC = rightBitMap.isLC
		rightUC = rightBitMap.isUC
		
	if 	leftLC and  (rightUC or rightBitMap.isSC) and (leftBitMap.glyphName[0] != 'n') and (rightBitMap.glyphName[0] != 'T'):
		return 1
		
	return 0
	
def checkForOverlap(fontFlatKernTable, fontPath, ppEM, glyphBitMapDict, doAll, limitVal):
	overlapList = []
	glyphNameList = glyphBitMapDict.keys()
	if doAll:
		doSome = (doAll == 1) # If doAll ==1, we skip some glyph pairs.
		leftCaseValue = 0
		glyphNameList.sort()
		for leftName in glyphNameList:
			rightSideDict = fontFlatKernTable.get(leftName, None)
			if rightSideDict == None:
				continue
				
			leftBitMap = glyphBitMapDict[leftName]
			if  doSome and leftBitMap.isSwash:
				skipLeft = 1
			else:
				skipLeft = 0 # I don't skip it here, as I do need to check the pair if it is kerned.
			for rightName in glyphNameList:
				if rightSideDict:
					 valueList = rightSideDict.get(rightName, [(0, 0, None, None, 0, None)])
				else:
					value = 0
				seenValue = {}
				for entry in valueList:
					sti, lookup, leftClassName, rightClassName, value, contextEntry = entry
					# With contextual rules, there may be several values in different contexts
					# We only need to report overlap for a given kern value once.
					seenVal = seenValue.get(value, 0)
					if seenVal:
						continue
					else:
						seenValue[value] = 1
					rightBitMap = glyphBitMapDict[rightName]
					if not value:
						if doSome and (skipLeft or rightBitMap.isSwash):
							continue
					if doSome and skipGlyphPair(leftBitMap, rightBitMap):
						continue
					overlapEntry = checkGlyphOverlap(leftBitMap, rightBitMap, value, limitVal)
					if overlapEntry:
						overlapList.append(overlapEntry)
	else:
		# check all kerned glyph pairs
		leftNames = fontFlatKernTable.keys()
		leftNames.sort()
		for leftName in leftNames:
			rightSideDict = fontFlatKernTable.get(leftName, None)
			if not rightSideDict:
				continue
			rightNames = rightSideDict.keys()
			leftBitMap = glyphBitMapDict[leftName]
			if kAllGlyphs in rightNames:
				rightNames = glyphNameList
				rightNames.sort()
			for rightName in rightNames:
				valueList = rightSideDict.get(rightName, [(0, 0, None, None, 0, None)])
				seenValue = {}
				for entry in valueList:
					sti, lookup, leftClassName, rightClassName, value, contextEntry = entry
					seenVal = seenValue.get(value, 0)
					if seenVal:
						continue
					else:
						seenValue[value] = 1
					rightBitMap = glyphBitMapDict[rightName]
					overlapEntry = checkGlyphOverlap(leftBitMap, rightBitMap, value, limitVal)
					if overlapEntry:
						overlapList.append(overlapEntry)
		
	return overlapList
	
def sequenceMatches(prev_seq, seq, sameLookup):
	# If they are in the same lookup, if the first lookup is less restrictive, then the second
	# doesn't get processed. If the first lookup is more restrictive, we assume that any overlap is by intent.
	# If they are in different lookups, then they can add if either context is a subset of the other.
	if sameLookup:
		if (prev_seq == None):
			return 1
		else:
			return 0
	else:
		# If they are in different lookups, values will add if there is any overlap in context.
		if (prev_seq == None) or (seq == None):
			return 1

	if type(prev_seq) == type(""): # it's a single glyph name, not a list
		prev_seq = [prev_seq]
	if type(seq) == type(""): # it's a single glyph name, not a list
		seq = [seq]
	
	# Now both are lists. Each position is either a glyph name or list of glyph names.
	# They match if there is a common 
	prevLen = len(prev_seq)
	curLen = len(seq)
	if sameLookup and prevLen > curLen:
		return 0
		
	listLen = min(prevLen, curLen)
	for i in range(listLen):
		item = seq[i]
		prev_item = prev_seq[i]
	
		if type(prev_item) == type(""): # it's a single glyph name, not a list
			if type(item) == type(""):
				return prev_item == item
			else:
				return prev_seq in item
		else:
			if type(item) == type(""):
				return item in prev_item
			else:
				# both are a list of glyph names.
				for gname in item:
					if gname in prev_item:
						return 1
				return 0

	return 0
	
def contextsMatch(prev_contextEntry, contextEntry, sameLookup):
	backtrack, lookAhead,leftClassDef, rightClassDef = contextEntry
	prev_backtrack, prev_lookAhead,prev_eftClassDef, prev_rightClassDef = prev_contextEntry
	# For the backtrack, we want to compare from the end towards the beginning.
	if prev_backtrack:
		prev_backtrack.reverse()
	if backtrack:
		backtrack.reverse()
	if sequenceMatches(prev_backtrack, backtrack, sameLookup):
		return sequenceMatches(prev_lookAhead, lookAhead, sameLookup)
	return 0
	
def checkForSubtableConflict(fontFlatKernTable):
	# for each lookup in ordeer
	# for each subtable in the lookup
	#  for each left side class
	#     if is class pair
	#       for each glyph in the left side class
	#        if haven't seen this glyph name before, add it as a key to seenkernDict, with the value {kAllGlyphs : lookupEntry}
	#        check if seenkernDict has this glyph name as a key. If so:
	#           if is in the same lookup, report that it will be masked
	#            else report that it will be added. !!! Bug: it will add only if right side classes overlap.
	#     else is not class pair:
	#        for left side name in kern pair dict:
	#          if not in seenkernDict:
	#             add as key to seenkernDict. Value is a dict of all the right side glyph names for the left side glyph
	#          else
	#             if not seenkernDict[left side glyph] has key kAllGlyphs:
	#                 for glyph name in the right-side name list for this left-side glyph;
	#                      if it is in the list of right side names in seenkernDict[left side glyph]:
	#                          get the  lookup and and subtable index for the previous occurence
	#                          report that it is masked by this lookup and STI.
	#                          !!! Bug - is additive if in a different lookup.
	#                      else add right side name to list of right side names in seenkernDict[left side glyph], with lookupEntry as value.
	#             else:
	#               This left side glyph was previously seen as part of a class pair.
	#               Report it as masked by class pair. !!! Bug: it is masked only if it is in the same lookup.
	# 
	errorList = []
	seenConflictDict = {}
	seenContextClassDict = {}
	for leftGlyphName in fontFlatKernTable.keys():
		leftFlatKernDict = fontFlatKernTable[leftGlyphName]
		classEntryList = leftFlatKernDict.get(kAllGlyphs, [])
		for rightGlyphName in leftFlatKernDict.keys():
			entryList = classEntryList + leftFlatKernDict[rightGlyphName]
			entryList.sort()
			if len(entryList) == 1:
				# no conflict
				continue

			for i in range(1, len(entryList)):
				sti, lookup, leftClassName, rightClassName, value, contextEntry = entryList[i]
				for prev_entry in entryList[:i]:
					prev_sti, prev_lookup, prev_leftClassName, prev_rightClassName, prev_value, prev_contextEntry = prev_entry
					msg = None
					if prev_lookup == lookup:
						# Note: context subttables are always in a different lookup than class and single kern pairs.
						if contextEntry:
							sameLookup = 1
							# Both must be contextual positioning statements. There is an conflict only if the previous context is a subset of the current context.
							if not contextsMatch(prev_contextEntry, contextEntry, sameLookup):
								continue
								
						if (not prev_leftClassName) and leftClassName:
							continue # an exception pair, is legal.
							
						elif prev_leftClassName and leftClassName:
								if sti == prev_sti:
									continue # just another class pair with the same left-side class.
									
								if seenConflictDict.get(leftClassName + lookup, 0):
									continue
									
								seenConflictDict[leftClassName + lookup] = 1
								msg = "\tError: Glyphs in class pairs with left side class '%s' in subtable %s are masked by class pairs with the left-side class '%s' in the previous subtable %s of lookup %s." % (leftClassName, sti, prev_leftClassName, prev_sti, lookup)
									# Since we are checking every kern pair, we need to suppress complaints about a left side class after seeing the first one.
						elif  prev_leftClassName and (not leftClassName):
								if seenConflictDict.get(leftGlyphName + prev_leftClassName + lookup, 0):
									continue

								msg = "\tError: kern pairs with left side glyph '%s' in subtable %s are masked by class pairs with the left-side class '%s' in the previous subtable %s of lookup %s." % (leftGlyphName, sti, prev_leftClassName, prev_sti, lookup)
								seenConflictDict[leftGlyphName + leftClassName + lookup] = 1
						elif  (not prev_leftClassName) and (not leftClassName):
								msg = "\tError: the kern pairs '%s %s' in subtable %s are masked by the same kern pair in the previous subtable %s of lookup %s." % (leftGlyphName, rightGlyphName, sti, prev_sti, lookup)
						
						if msg:
							if contextEntry:
								if leftClassName:
									try:
										seenContextClassDef = seenContextClassDict[leftClassName]
									except KeyError:
										msg = msg + "%s%s = %s" % (os.linesep, leftClassName, contextEntry[2])
										seenContextClassDict[leftClassName] = 1
								if rightClassName:
									try:
										seenContextClassDef = seenContextClassDict[rightClassName]
									except KeyError:
										msg = msg + "%s%s = %s" % (os.linesep, rightClassName, contextEntry[3])
										seenContextClassDict[rightClassName] = 1
							logMsg(msg)
							errorList.append(msg)
							
					else:
						# Lookups differ. One could be contextual positioning, and the other not.
						if contextEntry and prev_contextEntry:
							sameLookup = 0
							if not contextsMatch(prev_contextEntry, contextEntry, sameLookup):
								continue
								
						if (value != 0) and (prev_value != 0):
							if prev_leftClassName and leftClassName:
								if  seenConflictDict.get(prev_leftClassName + leftClassName + lookup + prev_lookup, 0):
									continue

								seenConflictDict[prev_leftClassName + leftClassName + lookup + prev_lookup] = 1
								first = "Class pair '%s %s %s' in subtable %s of lookup %s" % (leftClassName, rightClassName, value, sti, lookup)
								second = "previous Class pair '%s %s %s' in subtable %s of lookup %s" % (prev_leftClassName, prev_rightClassName, prev_value, prev_sti, prev_lookup)
							elif  prev_leftClassName and (not leftClassName):
								if rightClassName:
									rname = rightClassName
								else:
									rname = rightGlyphName
								if prev_rightClassName:
									prname = prev_rightClassName
								else:
									prname = rightGlyphName

								if  seenConflictDict.get(prev_leftClassName + leftGlyphName + rname + lookup + prev_lookup, 0):
									continue

								seenConflictDict[prev_leftClassName + leftGlyphName + rname + lookup + prev_lookup] = 1
								first = "kern pair '%s %s %s' in subtable %s of lookup %s" % (leftGlyphName, rname, value, sti, lookup)
								second = "previous Class pair '%s %s %s' in subtable %s of lookup %s" % (prev_leftClassName, prname, prev_value, prev_sti, prev_lookup)
							elif  (not prev_leftClassName) and (not leftClassName):
								# We only skip future matches if the right side is a class name.
								if rightClassName:
									if  seenConflictDict.get(leftGlyphName + rightClassName + lookup + prev_lookup, 0):
										continue
									seenConflictDict[leftGlyphName + rightClassName + lookup + prev_lookup] = 1
									rname = rightClassName
								else:
									rname = rightGlyphName
								if prev_rightClassName:
									prname = prev_rightClassName
								else:
									prname = rightGlyphName
								first = "kern pair '%s %s %s' in subtable %s of lookup %s" % (leftGlyphName, rname, value, sti, lookup)
								second = "previous kern pair '%s %s %s' in subtable %s of lookup %s" % (leftGlyphName, prname, prev_value, prev_sti, prev_lookup)

							msg = "\tError: %s will add to %s." % (first, second)
							if contextEntry:
								if leftClassName:
									try:
										seenContextClassDef = seenContextClassDict[leftClassName]
									except KeyError:
										msg = msg + "%s%s = %s" % (os.linesep, leftClassName, contextEntry[2])
										seenContextClassDict[leftClassName] = 1
								if rightClassName:
									try:
										seenContextClassDef = seenContextClassDict[rightClassName]
									except KeyError:
										msg = msg + "%s%s = %s" % (os.linesep, rightClassName, contextEntry[3])
										seenContextClassDict[rightClassName] = 1
							logMsg(msg)
							errorList.append(msg)
	return errorList

class AdjustmentMedthod_Base:
					
	def __init__(self, fontPath, ppEM, glyphBitMapDict):
		self.glyphBitMapDict = glyphBitMapDict
		self.fontPath = fontPath
		self.ppEM = ppEM

		gname = "space"
		bitmap_space = glyphBitMapDict.get(gname, None)

		if bitmap_space:
			self.specAdd = bitmap_space.advanceWidth/4
		else:
			self.specAdd = ppEM/10
		
		gname = "O"
		self.bitMap_O = glyphBitMapDict.get(gname, None)

		gname = "o"
		self.bitMap_o = glyphBitMapDict.get(gname, None)
			
		self.minDistanceBetween = self.bitMap_o.lsb + self.bitMap_o.rsb # need this to be defined before calcOpticalDistance is called.

		if self.bitMap_O:
			self.dist_O_O, minDist = self.calcOpticalDistance(self.bitMap_O, self.bitMap_O, 0)
			self.multiSpace_O = self.dist_O_O + self.bitMap_O.lsb +  self.bitMap_O.rsb
			self.upperCaseCorrection = self.bitMap_O.lsb +  self.bitMap_O.rsb + self.specAdd/5.0
		else:
			self.dist_O_O = ppEM;
			self.upperCaseCorrection = 0
			self.multiSpace_O = 0
			
		
		if self.bitMap_o:
			self.dist_o_o, minDist = self.calcOpticalDistance(self.bitMap_o, self.bitMap_o, 0)
			self.lowercaseCaseCorrection = self.bitMap_o.lsb +  self.bitMap_o.rsb
			self.multiSpace_o = self.dist_o_o + self.bitMap_o.lsb +  self.bitMap_o.rsb + self.specAdd/5
			self.multiMinDistBetween = self.bitMap_o.lsb +  self.bitMap_o.rsb
			self.multiMinDistBetween = self.multiMinDistBetween*2.0/3.0
		else:
			self.dist_o_o = ppEM;
			self.lowercaseCaseCorrection = 0
			self.multiSpace_o = 0
			self.multiMinDistBetween = self.specAdd/2.0
		
		self.dist_mixed = (self.dist_O_O + self.dist_o_o)/2.0
		return
		
	def getStdDistance(self, leftBitMap, rightBitMap):
		leftIsUpper = leftBitMap.isUC
		rightIsUpper = rightBitMap.isUC
		leftLC = leftBitMap.isLC
		rightLC = rightBitMap.isLC
		
		referenceDist = self.dist_o_o
		if (leftIsUpper or rightIsUpper) and  (leftLC or rightLC):
			referenceDist =  self.dist_mixed
			#logMsg("Mixed ", leftIsUpper, rightIsUpper,)
		elif rightIsUpper and leftIsUpper:
			referenceDist =  self.dist_O_O # use O-O spacing for both are eiher uppercase or unknown  
			#logMsg("Upper ", leftIsUpper, rightIsUpper,)
		else:
			pass
			#logMsg("Lower ", leftIsUpper, rightIsUpper,)
		
		return referenceDist
		
			
	def calcAdjustment(self, leftName, rightName, value):
		glyphBitMapDict = self.glyphBitMapDict
		leftBitMap = glyphBitMapDict[leftName]
		rightBitMap = glyphBitMapDict[rightName]
		dist, minDist  = self.calcOpticalDistance(leftBitMap, rightBitMap, value)
		referenceDist = self.getStdDistance(leftBitMap, rightBitMap)

		delta = referenceDist - dist
		if (leftBitMap.needsExtraSpace or rightBitMap.needsExtraSpace):
			delta += self.specAdd # Add 1/4 of the space width.
		
		
		# Avoid collision.
		if self.multiMinDistBetween > (minDist + delta): #would cause a collision!
			#logMsg(" Adding adjustment to avoid collision. Was ", int(0.5 + delta), ".", )
			diff = self.multiMinDistBetween - (minDist + delta)
			delta  = 0
			logMsg("\tSetting min distance %s with optkern %s, overlap %s." % (self.multiMinDistBetween,value,diff))
			value += diff

		
		return int(delta), int(value)

class AdjustmentMedthod_RMS(AdjustmentMedthod_Base):

	
	def calcOpticalDistance(self, leftBitMap, rightBitMap, value):
		opticalDistance = 0
		linearDistance = 0
		kMinNotSet = 9999999
		minDistance = kMinNotSet

		# imagine the right bitmap postioned right after the left bitmap.
		ppEM = leftBitMap.ppEM
		bboxTopIndex = min(leftBitMap.bboxTopIndex, rightBitMap.bboxTopIndex)
		bboxBottomIndex = max(leftBitMap.bboxBottomIndex, rightBitMap.bboxBottomIndex)
		bboxHeight = 1 + bboxTopIndex - bboxBottomIndex
		if bboxHeight < 0:
			return None, None
			
		leftLineIndex = leftBitMap.bboxTopIndex - bboxTopIndex
		rightLineIndex = rightBitMap.bboxTopIndex - bboxTopIndex
		leftOffset = 0
		if leftBitMap.rsb < 0:
			leftOffset = -leftBitMap.rsb
		if leftBitMap.lsb < 0:
			leftOffset = leftOffset-leftBitMap.lsb
		rightOffset = 0
		if rightBitMap.lsb < 0:
			rightOffset = -rightBitMap.lsb

		lineCount = 0
		opticalLineCount = 0
		#logMsg("leftBitMap.bboxTopIndex %s rightBitMap.bboxTopIndex %s." % (leftBitMap.bboxTopIndex, rightBitMap.bboxTopIndex))
		#logMsg("leftBitMap.bboxBottomIndex %s rightBitMap.bboxBottomIndex %s." % (leftBitMap.bboxBottomIndex, rightBitMap.bboxBottomIndex))
		#logMsg("leftLineIndex %s rightLineIndex %s." % (leftLineIndex, rightLineIndex))
		while lineCount < bboxHeight:
				
			leftLine = leftBitMap.scanLines[leftLineIndex]
			rightLine = rightBitMap.scanLines[rightLineIndex]
			leftLineIndex += 1
			rightLineIndex += 1
			lineCount += 1
			
			# find first black pixel in left image, searching from 
			advw = leftBitMap.advanceWidth
			lLimit = max(0, advw/2)
			lsi = len(leftLine)-1
			while (leftLine[lsi] != "#") and (lsi > lLimit):
				lsi -= 1
			lDist = advw - (lsi+ leftOffset + 1)
			
			# find first black pixel in right image
			rsi = 0 
			rLimit = min( rightBitMap.advanceWidth/2, rightBitMap.advanceWidth - rightBitMap.rsb)
			while (rightLine[rsi] != "#") and (rsi < rLimit):
				rsi += 1
			rDist = rsi - rightOffset
			
			#if (lsi == lLimit) and (rsi == rLimit):
			#	continue
			opticalLineCount +=1
			dist = rDist + lDist + value
			#logMsg(lineCount, dist, dist**2, leftLine[lsi-1:], rightLine[:rsi+1])
			linearDistance += dist
			if dist < minDistance:
				minDistance = dist
			if dist > 0:
				opticalDistance += math.sqrt(dist)
			elif dist < 0:
				opticalDistance -= math.sqrt(-dist)
			
			
		if opticalLineCount:
			opticalDistance = opticalDistance/float(bboxHeight)
			if opticalDistance > 0:
				opticalDistance  = opticalDistance * opticalDistance
			elif opticalDistance < 0:
				opticalDistance = - opticalDistance * opticalDistance
			
			linearDistance = linearDistance/float(bboxHeight)
			
		if minDistance == kMinNotSet:
			minDistance = 0
		return opticalDistance, minDistance


class AdjustmentMedthod_Lookup(AdjustmentMedthod_Base):

	def __init__(self, fontPath, ppEM, glyphBitMapDict):
		""" I want a curve that for any input distance X will return a scaled distance Y such that:
		- for X = 0 to some midpoint area, Y will be nearly equal to X
		- for X from the mid-point area on, Y will rise increasingly slowly to a maximum value of 'maxOut'.
		I will use a bezier curve to do this. Since I want the initial curve to rise such that Y== X, 
		I need the first BCP to be at (X1== Y1), and will set X1  =  frac1*maxIn
		I will set the second BCP and third BCP such that:
		Y2 = Y3 = maxOut
		X2 = frac2*maxIn, x3 = maxIn
		Any value passed in which is greater than maxIn will return maxOut.
		"""
		fp = open("lookup.py")
		data = fp.read()
		fp.close()
		exec(data)
		x0 = x0*ppEM/1000.0
		x1 = x1*ppEM/1000.0
		x2 = x2*ppEM/1000.0
		x3 = x3*ppEM/1000.0
		y0 = y0*ppEM/1000.0
		y1 = y1*ppEM/1000.0
		y2 = y2*ppEM/1000.0
		y3 = y3*ppEM/1000.0
		
		self.maxIn = maxIn = int(x3)
		self.maxOut = maxOut = int(y3)
		# compute the lookup function for all integers frmomn 0 to maxIn.
		# I first compute the (x,y) pairs at 100 intervals betweem t = 0->1.0,
		# and then build the lookup by linear interpolation between the x-y pairs.
		lookup = [] # for x = 0, return value is 0
		fMaxIn = float(maxIn)
		for i in range(maxIn):
			t = i/fMaxIn
			x = x0*(1-t)**3 + 3*x1*t*(1-t)**2 + 3*x2*(t**2)*(1-t) + x3*t**3
			y = y0*(1-t)**3 +3*y1*t*(1-t)**2 + 3*y2*(t**2)*(1-t) + y3*t**3
			#logMsg("i: %s, x: %s, y: %s" % (i, x,y))
			if x > (i+0.01):
				while x > (i+0.01):
					t = t - 0.001
					x = 3*x1*t*(1-t)**2 + 3*x2*(t**2)*(1-t) + x3*t**3
					y = 3*y1*t*(1-t)**2 + 3*y2*(t**2)*(1-t) + y3*t**3
			elif x < (i - 0.01):
				while x < (i - 0.01):
					t = t + 0.001
					x = 3*x1*t*(1-t)**2 + 3*x2*(t**2)*(1-t) + x3*t**3
					y = 3*y1*t*(1-t)**2 + 3*y2*(t**2)*(1-t) + y3*t**3
			#logMsg("i: %s, x: %s, y: %s" % (i, x,y))
			lookup.append(y)
		self.lookup = lookup
		#logMsg("LookupList")
		#for entry in lookup:
		#	logMsg(entry)
		#logMsg("End LookupList")
		self.maxIn = int(self.maxIn)
		AdjustmentMedthod_Base.__init__(self, fontPath, ppEM, glyphBitMapDict)
		

	def calcOpticalDistance(self, leftBitMap, rightBitMap, value):
		# bitmap.lsb
		# bitmap.rsb
		# bitmap.advanceWidth
		# bitmap.bboxTopIndex
		# bitmap.bboxBottomIndex
		# bitmap.bboxHeight
		# bitmap.glyphName
		opticalDistance = 0
		linearDistance = 0
		kMinNotSet = 9999999
		minDistance = kMinNotSet
	
		# imagine the right bitmap postioned right after the left bitmap.
		ppEM = leftBitMap.ppEM
		bboxTopIndex = max(leftBitMap.bboxTopIndex, rightBitMap.bboxTopIndex, 134)
		bboxBottomIndex = min(leftBitMap.bboxBottomIndex, rightBitMap.bboxBottomIndex, 0)
		bboxHeight = 1 + bboxTopIndex - bboxBottomIndex
		leftLineIndex = leftBitMap.bboxTopIndex - bboxTopIndex
		rightLineIndex = rightBitMap.bboxTopIndex - bboxTopIndex
		lineCount = 0
		opticalLineCount = 0
		leftOffset = 0
		if leftBitMap.rsb < 0:
			leftOffset = -leftBitMap.rsb
		if leftBitMap.lsb < 0:
			leftOffset = leftOffset-leftBitMap.lsb
		rightOffset = 0
		if rightBitMap.lsb < 0:
			rightOffset = -rightBitMap.lsb
			
		while lineCount < bboxHeight:
			if (leftLineIndex < 0) or (leftLineIndex >= leftBitMap.bboxHeight):
				rightLineIndex +=1
				leftLineIndex +=1
				lineCount += 1
				dist = self.minDistanceBetween + value
				linearDistance += dist
				opticalDistance += dist
				continue
			if (rightLineIndex < 0) or (rightLineIndex >= rightBitMap.bboxHeight):
				rightLineIndex +=1
				leftLineIndex +=1
				lineCount += 1
				dist = self.minDistanceBetween + value
				linearDistance += dist
				opticalDistance += dist
				continue
				
			leftLine = leftBitMap.scanLines[leftLineIndex]
			rightLine = rightBitMap.scanLines[rightLineIndex]
			leftLineIndex += 1
			rightLineIndex += 1
			lineCount += 1
			
			# find first black pixel in left image, searching from 
			advw = leftBitMap.advanceWidth
			lLimit = max(0, advw - self.maxIn)
			lsi = len(leftLine)-1
			while (leftLine[lsi] != "#") and (lsi > lLimit):
				lsi -= 1
			lDist = advw - (lsi+ leftOffset + 1)
			
			# find first black pixel in right image
			rsi = 0 
			rLimit = min(self.maxIn, rightBitMap.advanceWidth - rightBitMap.rsb)
			while (rightLine[rsi] != "#") and (rsi < rLimit):
				rsi += 1
			rDist = rsi - rightOffset
			
			#if (lsi == lLimit) and (rsi == rLimit):
			#	continue
			opticalLineCount +=1
			dist = rDist + lDist + value
			if dist < minDistance:
				minDistance = dist
			linearDistance += dist
			dist = self.getScaledDistance(dist)
			opticalDistance += dist
			
		if opticalLineCount:
			opticalDistance = (opticalDistance)/float(bboxHeight)
			linearDistance = linearDistance/float(bboxHeight)
		if minDistance == kMinNotSet:
			minDistance = 0
		return opticalDistance, minDistance

	def getScaledDistance(self, x):

		if x >= self.maxIn:
			return self.maxOut
		if x <= 0:
			return 0
		return self.lookup[x]
	
def compareKernValues(lookupIndexDict, fontPath, ppEM, glyphBitMapDict = {}):
	""" For each kernpair, calculate the optical distance, and report if it is
	more than 10 units different than the actual value.
	"""
	adjustmentMethod = AdjustmentMedthod_Lookup(fontPath, ppEM, glyphBitMapDict)
	#adjustmentMethod = AdjustmentMedthod_RMS(fontPath, ppEM, glyphBitMapDict)
	
	indexList = lookupIndexDict.keys()
	indexList.sort()
	if 0:
		for leftName in ["O", "o", "P", "J", "N", "n"]:
			for rightName in ["O", "o", "n"]:
				delta = adjustmentMethod.calcAdjustment(leftName, rightName, 0)
				logMsg("%s %s: delta %s." % (leftName, rightName, delta))
		return
	diffDict = {}
	debug = 1
	debugList = ["H", "J", "O", "P", "T", "V", "o", "n", "f", "braceright"]
	for lookupIndex in indexList:
		subtableList = lookupIndexDict[lookupIndex]
		for kernTable in subtableList:
			leftClassDict = kernTable.leftClassDict
			rightClassDict = kernTable.rightClassDict
			kernPairDict = kernTable.kernPairDict
			for leftName in kernPairDict.keys():
				rightDict = kernPairDict[leftName]
				if leftName[0] == "@":
					if debug:
						leftList = kernTable.leftClassDict[leftName]
						leftList = filter(lambda name: name in debugList, leftList)
						if not leftList:
							continue
						#leftList = [kernTable.leftClassDict[leftName][0]]
					else:
						leftList = kernTable.leftClassDict[leftName]
				else:
					leftList = [leftName]
					continue
				for rightName in rightDict.keys():
					value  = rightDict[rightName]
					origValue = value
					if type(value) != type(0):
						value = value[0] + value[2]
					if value >= 0:
						value = int(0.5 + value*ppEM/1000.0)
					else:
						value = int(-0.5 + value*ppEM/1000.0)
					if rightName[0] == "@":
						if debug:
							rightList = kernTable.rightClassDict[rightName]
							rightList = filter(lambda name: name in debugList, rightList)
							if not rightList:
								continue
							#rightList = [kernTable.rightClassDict[rightName][0]]
						else:
							rightList = kernTable.rightClassDict[rightName]
					else:
						rightList = [rightName]
						continue
					# Now compare evry possible kern pair
					for leftName in leftList:
						for rightName in rightList:
							tries = 0
							optical_kern = 0
							delta, adjustedKernValue = adjustmentMethod.calcAdjustment(leftName, rightName, optical_kern)
							optical_kern = delta
							if adjustedKernValue != optical_kern:
								optical_kern = adjustedKernValue
							while (abs(delta) != 0) and (tries < 10):
								delta, adjustedKernValue = adjustmentMethod.calcAdjustment(leftName, rightName, optical_kern)
								if adjustedKernValue != optical_kern:
									optical_kern = adjustedKernValue
									break
								optical_kern += delta
								tries += 1
							logMsg("%s %s: delta %s optical_kern %s kern %s diff %s." % (leftName, rightName, delta, optical_kern, value, optical_kern - value))
							diffCnt = diffDict.get(optical_kern - value, 0)
							diffCnt += 1
							diffDict[optical_kern - value] = diffCnt
		
	diffList = diffDict.items()
	total = 0
	for entry in diffList:
		total += abs(entry[1] * entry[0])
	logMsg("Total diff:", total)
	diffList.sort()
	for entry in diffList:
		print entry[0], "#"*entry[1]
		
def getBitMaps(fontPath, ppEM, doAll):
	glyphBitMapDict = {}
	command = "tx -bc -z %s  \"%s\"" % (ppEM, fontPath)
	report = FDKUtils.runShellCmd(command)


	# split bitmap text blocks apart
	bitmapList = re.findall(r"glyph (\S+)([^g]+)", report)

	for entry in bitmapList:
		glyphName = entry[0]
		if glyphName == ".notdef":
			continue
		bitMap = TXBitMap(glyphName, entry[1], ppEM)
		glyphBitMapDict[glyphName] = bitMap
		
	return glyphBitMapDict

def getOptions(argv):
	ppEM = kDefaultppEM
	doAll = 0
	doMore = 0
	doPDF = 0
	doCollisionCheck = 1
	doSubtableCheck = 1
	fontPath = None
	logPath = None
	pdfPath = None
	limitVal = 1
	ptSize = 24
	sortType = 1
	i = 0
	while i < len(argv):
		arg = argv[i]
		i += 1

		if arg == "-ppEM":
			value = argv[i]
			i += 1
			try:
				ppEM = eval(value)
			except:
				print "Error: option %s must be followed by an integer value." % (arg)
				sys.exit(1)
		elif arg == "-ptSize":
			value = argv[i]
			i += 1
			try:
				ptSize = eval(value)
			except:
				print "Error: option %s must be followed by an integer value." % (arg)
				sys.exit(1)
		elif arg == "-limit":
			value = argv[i]
			i += 1
			try:
				limitVal = eval(value)
			except:
				print "Error: option %s must be followed by an integer value." % (arg)
				sys.exit(1)
		elif arg == "-doAll":
			doAll = 2
		elif arg == "-doMore":
			doAll = 1
			doMore = 1
		elif arg == "-log":
			try:
				logPath = arg = argv[i]
				i += 1
			except IndexError:
				print "Error: option %s must be followed by the path to the output log file." % (arg)
				sys.exit(1)
		elif arg == "-subtableCheck":
			doPDF = 0
			doCollisionCheck = 0
			doSubtableCheck = 1
		elif arg == "-makePDF":
			doPDF = 1
			doCollisionCheck = 0
			doSubtableCheck = 0
			try:
				fontPath = arg = argv[i]
				i += 1
				if not os.path.exists(fontPath):
					print "Error: font file path <%s> does not exist." % (fontPath)
					sys.exit(1)
			except IndexError:
					print "Error: The option -makePDf requires a following argument which is the path to the font file, and optionally another path to the input log file." %s (fontPath)
					sys.exit(1)

			try:
				logPath = arg = argv[i]
				i += 1
				if not os.path.exists(logPath):
					print "Error: log file path <%s> does not exist." % (logPath)
					sys.exit(1)
			except IndexError:
				logPath = fontPath + kLogFileExt
				if not os.path.exists(logPath):
					print "Error: The default log file path <%s> does not exist." % (logPath)
					sys.exit(1)

			try:
				pdfPath = arg = argv[i]
				i += 1
			except IndexError:
				pass
				
		elif arg == "-sortByName":
			sortType = 1
		elif arg == "-sortByArea":
			sortType = 2
		elif arg[0] == '-':
			print "Error: do not recognize option %s." % (arg)
			sys.exit(1)
		else:
			if fontPath:
				print "Error: only one font path is allowed! Paths: %s, %s." % (fontPath, arg)
				sys.exit(1)
			fontPath = arg
			if not os.path.isfile(fontPath):
				print "\tError: fontpath '%s' does not exist." % (fontPath)
				sys.exit(0)

	if fontPath == None:
		print "\tError: fontpath must be specified."
		sys.exit(0)

	if logPath == None:
		logPath = fontPath + kLogFileExt

	if pdfPath == None:
		pdfPath = logPath + kPDFExt
		
		
	return ppEM, ptSize, doAll, sortType, limitVal, doCollisionCheck, doPDF, doSubtableCheck, fontPath, logPath, pdfPath

def parseKernLog(logFilePath):
	kernOverlapList = None
	try:
		fp = open(logFilePath)
	except (IOError, OSError):
		print "Error: could not open kern log file <%s>. Will not save pdf." % (logFilePath)
		print "\t%s" % (traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		return kernOverlapList
	data = fp.read()
	fp.close()
	kernOverlapList = re.findall("(\d+)\s+pixels overlap at (\d+) ppem In glyph pair '(\S+) (\S+) with kern (-*\d+)", data)
	kernOverlapList = map(lambda entry: (entry[2], entry[3], eval(entry[4]), eval(entry[0]), entry[1]), kernOverlapList)
	return kernOverlapList

def setPDFVariables(params, kDrawTag, kDrawPointTag, kShowMetaTag):
	exec("params." + kDrawTag + "HHints = 0")
	exec("params." + kDrawTag + "VHints = 0")
	exec("params." + kDrawTag + "BlueZones = 0")
	exec("params." + kDrawTag + "Baseline = 0")
	exec("params." + kDrawTag + "EMBox = 0")
	exec("params." + kDrawTag + "XAdvance = 0")
	exec("params." + kDrawTag + "ContourLabels = 0")
	exec("params." + kDrawTag + "Outline = 1")
	exec("params." + kDrawPointTag + "PointMarks = 0")
	exec("params." + kDrawPointTag + "BCPMarks = 0")
	exec("params." + kDrawPointTag + "PointLabels = 0")
	exec("params." + kShowMetaTag + "Outline = 0") # shows filled outline in  lower right of meta data area under glyph outline.
	exec("params." + kShowMetaTag + "Name = 0")
	exec("params." + kShowMetaTag + "BBox = 0")
	exec("params." + kShowMetaTag + "SideBearings = 0")
	exec("params." + kShowMetaTag + "V_SideBearings = 0")
	exec("params." + kShowMetaTag + "Parts = 0")
	exec("params." + kShowMetaTag + "Paths = 0")
	exec("params." + kShowMetaTag + "Hints = 0")
	return 
	
def makeOverlapPDF(fontPath, logPath, pdfPath, ptSize, sortType, limitVal):
	""" The layout is:
	As many lines per page as fits
	on each line, glyph1, glyph2, text " overlap of x pixels at y dpi with kern of z.
	"""
	from  fontPDF import FontPDFParams, makeKernPairPDF, kDrawTag, kDrawPointTag, kShowMetaTag
	import ProofPDF
	import otfPDF
	import ttfPDF
	
	txPath = ProofPDF.CheckEnvironment()
	kernPairPtPtSize = ptSize
	texPtSize = 12
	params = FontPDFParams()
	setPDFVariables(params, kDrawTag, kDrawPointTag, kShowMetaTag)
	params.DrawFilledOutline = 1
	params.userPtSize = kernPairPtPtSize
	params.rt_pdfFileName = pdfPath
	params.rt_reporter = logMsg
	fontFileName = os.path.basename(fontPath)
	params.rt_filePath = os.path.abspath(fontPath)
	logMsg("Building kern overlap proof for font %s." % (fontPath))
	try:
		ttFont, tempPathCFF = ProofPDF.openFile(fontPath, txPath)
	except ProofPDF.FontError:
		print traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]
		return

	if ttFont.has_key("CFF "):
		pdfFont = otfPDF.txPDFFont(ttFont, params)
	elif ttFont.has_key("glyf"):
		pdfFont = ttfPDF.txPDFFont(ttFont, params)
	else:
		logMsg( "Quitting. Font type is not recognized. %s.." % (path))
		return
	kernOverlapList = parseKernLog(logPath)
	if not kernOverlapList:
		return
	# Now sort and threshold the list.
	if limitVal > 1:
		kernOverlapList = filter(lambda entry: entry[3] >= limitVal, kernOverlapList)
	if sortType == 1:
		kernOverlapList.sort()
	elif sortType == 2:
		kernOverlapList = map(lambda entry: (entry[3:] + entry[:3]), kernOverlapList)
		kernOverlapList.sort()
		kernOverlapList.reverse()
		kernOverlapList = map(lambda entry: (entry[2:] + entry[:2]), kernOverlapList)
		
	pdfFilePath = makeKernPairPDF(pdfFont, kernOverlapList, params)
	if tempPathCFF:
		os.remove(tempPathCFF)
		
	logMsg( "Wrote proof file %s." % (pdfFilePath))
	
	return
	
def run():
	global gAGDDict
	global logMsg
	
	if "-u" in sys.argv:
		print __usage__ 
		return
		
	if "-h" in sys.argv:
		print __help__ 
		return

	ppEM, ptSize, doAll, sortType, limitVal, doCollisionCheck, doPDF, doSubtableCheck, fontPath, logPath, pdfPath = getOptions(sys.argv[1:])

	# kerncheck has two very different modes. With the option -makePDF, it processes a log file to make a PDF. 
	# Without the option -makePDF, it makes the log file.
	if doPDF:
		reporter = Reporter()
		logMsg = reporter.write
		logMsg( "Building PDF...")
		makeOverlapPDF(fontPath, logPath, pdfPath, ptSize, sortType, limitVal)
		logMsg( "All done.")
		return
	
	reporter = Reporter(logPath)
	logMsg = reporter.write
	print "Loading Adobe Glyph Dict..."
	fdkToolsDir, fdkSharedDataDir = FDKUtils.findFDKDirs()
	sys.path.append(fdkSharedDataDir)
	kAGD_TXTPath = os.path.join(fdkSharedDataDir, "AGD.txt")
	fp = open(kAGD_TXTPath, "rU")
	agdTextPath = fp.read()
	fp.close()
	gAGDDict = agd.dictionary(agdTextPath)
	overlapList = conflictMsgList = []
	print "Collecting font kern data..."
	lookupIndexDict, lookupSequenceDict, fontFlatKernTable = collectKernData(fontPath)

	if lookupIndexDict:
		if doCollisionCheck:
			print "Building bitmaps for font..."
		
			glyphBitMapDict = getBitMaps(fontPath, ppEM, doAll)
			
			print "Checking for glyph overlap in all glyph pairs..."
			overlapList = checkForOverlap(fontFlatKernTable, fontPath, ppEM, glyphBitMapDict, doAll, limitVal)
		
		if doSubtableCheck:
			print "Checking for subtable conflicts in kern feature..."
			conflictMsgList = checkForSubtableConflict(fontFlatKernTable)
		#print "Comparing font kern vs calculated values..."
		#compareKernValues(lookupIndexDict, fontPath, ppEM, glyphBitMapDict)

	reporter.close(fontPath, sortType, overlapList , conflictMsgList)

	print "All done."
	
if __name__=='__main__':
	run()
	
	
