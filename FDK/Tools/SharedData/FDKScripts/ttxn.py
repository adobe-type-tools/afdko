#! /usr/bin/env python


"""
How to build a feature-file format.
1) Traverse script->language->feature tree, building up list as we go.

These are all expected to be in alphabetic order in the font.

Add each  feature entry as FeatDict[tag][lang_sys].append(index]), and as  FeatDict[index] .append(lang_sys)

If there is no DFLT script, then step through all entries of  FeatDict[index]. Add  FeatDict[tag]["DFLT_dflt"].append(index) for
all FeatDict[index] where the number of lang_sys entries is the same as the number of lang_sys values.

Write out the language systems.

Collect and write the class defs.
For each lookup in lookup list: for each class def in the lookup:
	add entry to ClassesByNameDict[<glyph list as tuple>].append(lookup index, subtable_index, class index)

Get list of keys for ClassesByNameDict. Sort. MakeClassNameDict.
For each  keys for ClassesByNameDict:
	make class name from <firstglyph_lastglyph>. 
	while class name is in ClassesByNameDict:
		find glyph in current class that is not in previous class, and make name firstglyph_diff-glyph_last-glyph
	add entry to ClassesByLookup[lookup index, subtable_index, class index)] = class name.

Same for mark classes.

Write out class defs.

Write out the feature list.
Make lookupDict = {}
Get feat tag list. For each tag:
	note if DLFT dflt lang sys has any lookups.
	for each lang_sys ( by definition, starts with DFLT, then the rest in alpha order).
		If script is DFLT:
			don't write script/lang
			build list of lookups for DLFT script.
		else:
			get union of lookup indicies in all feat indicies
			Remove all the default lookups from the lookup list:
			if no lookups left, continue
			if there was no intersection with default lookups, set exclude keyword
			if the script is same as last one, write language only,
			else write script and language.

		if lookupIndex is in lookupDict:
			just write lookup references
		else:
			write lookup definition with name "feat_script_lang_new_index"
					
		
"""

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""



__help__ = """
ttxn v1.17 Jan 12 2016

Based on the ttx tool, with the same options, except that it is limited to
dumping, and cannot compile. Makes a normalized dump of the font, or of
selected tables.

Adds additional options:
	-nh	: suppresses hints in the glyph outline dump.
	-nv	: suppress version info in outpiut: head.fontRevision, and name ID's 3 and 5.
	-se	: show if lookup is wrapped in an Extension lookup type;
			by default, this information is suppressed.

"""

def _dummy():
	from encodings import latin_1, utf_8, utf_16_be


import sys
import os
from fontTools import ttx
from fontTools.ttLib import TTFont, tagToIdentifier
from fontTools.ttLib.tables.otBase import BaseTTXConverter
from fontTools.ttLib.tables.otTables import fixLookupOverFlows, fixSubTableOverFlows
from fontTools.misc.xmlWriter import XMLWriter
import copy
import subprocess
import re
import collections
import textwrap
import string
import platform
import getopt

curSystem = platform.system()
INDENT = "   "
ChainINDENT = INDENT + " C "
kMaxLineLength = 80
gtextWrapper = textwrap.TextWrapper(width = kMaxLineLength, subsequent_indent = INDENT + INDENT)


def getValueRecord(a,b,c,d):
	return "<%s %s %s %s>" % (a,b,c,d)
kNullValueRecord = getValueRecord(0,0,0,0)

def getAnchorString(anchorTable):
	if not anchorTable:
		return "<anchor NULL>"
	
	tokenList = ["<anchor", "%d" % (anchorTable.XCoordinate),"%d" % (anchorTable.YCoordinate)]
	if anchorTable.Format == 2:
		tokenList.append("%d" % (anchorTable.AnchorPoint))
	elif anchorTable.Format == 3:
		tokenList.append("%d" % (anchorTable.XDeviceTable))
		tokenList.append("%d" % (anchorTable.YDeviceTable))
	return " ".join(tokenList) + ">"


class ClassRecord:

	def __init__(self, lookupIndex, subtableIndex, classIndex, type = "", anchor = None):
		self.lookupIndex = lookupIndex
		self.subtableIndex = subtableIndex
		self.classIndex = classIndex
		self.type = type # None if not a kern pair, else "Left" or "Right"
		self.anchor = anchor


	def __repr__(self):
		return "(lookup %s subtable %s class %s type %s anchor %s)" % (self.lookupIndex, self.subtableIndex, self.classIndex, self.type, self.anchor)
		
def addClassDef(otlConv, classDefs, coverage, type = None, anchor = None):
	classDict = {}
	glyphDict = copy.deepcopy(otlConv.glyphDict)
	
	for name, classIndex in classDefs.items():
		del glyphDict[name]
		try:
			classDict[classIndex].append(name)
		except KeyError:
			classDict[classIndex] = [name]
	classZeroList = None
	if coverage:
		if not classDict.has_key(0):
			classZeroList = glyphDict.keys()
			classZeroList.sort()
			i = len(classZeroList)
			while i > 0:
				i -= 1
				name = classZeroList[i]
				if not (name in coverage.glyphs):
					classZeroList.remove(name)
	if classZeroList:
		classDict[0] = classZeroList
	else:
		classDict[0] = []
		
		
	
	for classIndex, nameList in classDict.items():
		nameList.sort()
		key = tuple(nameList)
		classRec = ClassRecord(otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, type)
		otlConv.classesByNameList[key].append(classRec)
	return


def AddMarkClassDef(otlConv, markCoverage, markArray, type):
	markGlyphList = markCoverage.glyphs
	markClass = collections.defaultdict(list)
	
	for m in range(len(markGlyphList)):
		markRec = markArray.MarkRecord[m]
		markClass[markRec.Class].append( (getAnchorString(markRec.MarkAnchor),markGlyphList[m]) )
		
	# Get the mark class names from the global dict
	for c in markClass.keys():
		valueList = markClass[c]
		anchorDict = collections.defaultdict(list)
		for anchor, glyph in valueList:
			anchorDict[anchor].append(glyph)
		defList = []
		for anchor, glyphList in anchorDict.items():
			glyphList.sort()
			defList.append((tuple(glyphList), anchor) )
		defList.sort()
		classRec = ClassRecord(otlConv.curLookupIndex, otlConv.curSubTableIndex, c, type)
		otlConv.markClassesByDefList[tuple(defList)].append( classRec)


def classPairGPOS(subtable, otlConv=None):
	addClassDef(otlConv, subtable.ClassDef1.classDefs, subtable.Coverage, otlConv.leftSideTag )
	addClassDef(otlConv, subtable.ClassDef2.classDefs, None, otlConv.rightSideTag )
	return
	
def markClassPOS(subtable, otlConv=None):
	AddMarkClassDef(otlConv, subtable.MarkCoverage, subtable.MarkArray, otlConv.markTag)
	
def markLigClassPOS(subtable, otlConv=None):
	AddMarkClassDef(otlConv, subtable.MarkCoverage, subtable.MarkArray, otlConv.markTag)
	
def markMarkClassPOS(subtable, otlConv=None):
	AddMarkClassDef(otlConv, subtable.Mark1Coverage, subtable.Mark1Array, otlConv.mark1Tag)
	
def classContexGPOS(subtable, otlConv):
	addClassDef(otlConv, subtable.ClassDef.classDefs )
	
def classChainContextGPOS(subtable, otlConv):
	addClassDef(otlConv, subtable.BacktrackClassDef.classDefs, None, otlConv.backtrackTag)
	addClassDef(otlConv, subtable.InputClassDef.classDefs, None, otlConv.InputTag)
	addClassDef(otlConv, subtable.LookAheadClassDef.classDefs, None,  otlConv.lookAheadTag)

def classExtPOS(subtable, otlConv):
	handler = otlConv.classHandlers.get( (subtable.ExtensionLookupType,  subtable.ExtSubTable.Format), None)
	if handler:
		handler( subtable.ExtSubTable, otlConv)
	
def classContextGSUB(subtable, otlConv):
	addClassDef(otlConv, subtable.ClassDef.classDefs )
	
def classChainContextGSUB(subtable, otlConv):
	addClassDef(otlConv, subtable.BacktrackClassDef.classDefs, None, otlConv.backtrackTag)
	addClassDef(otlConv, subtable.InputClassDef.classDefs, None, otlConv.InputTag)
	addClassDef(otlConv, subtable.LookAheadClassDef.classDefs, None,  otlConv.lookAheadTag)
	
def classExtSUB(subtable, otlConv):
	handler = otlConv.classHandlers.get( (subtable.ExtensionLookupType, subtable.ExtSubTable.Format), None)
	if handler:
		handler( subtable.ExtSubTable, otlConv)


	
class ContextRecord:
	def __init__(self, inputList, sequenceIndex):
		self.inputList = inputList
		self.sequenceIndex = sequenceIndex
		self.glyphsUsed = 0
		self.result = ""
		
def checkGlyphInSequence(glyphName, contextSequence, i):
	retVal = 0
	note = ""
	try:
		input = contextSequence[i]
		if type(input) == type([]):
			if glyphName  in input:
				retVal = 1
		elif type(input) == type(""):
			if glyphName == input:
				retVal = 1
	except IndexError:
		pass
		
	return retVal
	

def ruleSinglePos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	if subtable.Format == 1:
		valueString = getValRec(subtable.ValueFormat, subtable.Value)
		if valueString and valueString != "0" and valueString != kNullValueRecord:
			for i in range(len(subtable.Coverage.glyphs)):
				rule = note = None
				tglyph = subtable.Coverage.glyphs[i]
				if context and (not checkGlyphInSequence(tglyph, inputSeqList, 0)):
					note = " # Note! Not in input sequence"
				else:
					note = ""
				rule = "%spos %s %s;%s" % (indent, tglyph, valueString, note)
				if context and note:
					noMatchRules.append(rule)
				else:
					rules.append(rule)
			
	elif subtable.Format == 2:
		for i in range(subtable.ValueCount):
			rule = note = None
			tglyph = subtable.Coverage.glyphs[i]
			valueString = getValRec(subtable.ValueFormat, subtable.Value[i])
			if valueString and valueString != "0" and valueString != kNullValueRecord:
				if context and (not tglyph in inputSeqList):
					note = " # Note! Not in input sequence"
				else:
					note = ""
				rule = "%spos %s %s;%s" % (indent, tglyph, valueString, note)
				if context and note:
					noMatchRules.append(rule)
				else:
					rules.append(rule)
	else:
		raise AttributeError("Unknown Single Pos format")

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
		
	
def rulePairPos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	if subtable.Format == 1:
		firstGlyphList = subtable.Coverage.glyphs
		for i in range(subtable.PairSetCount):
			g1 = firstGlyphList[i]
			pairSet = subtable.PairSet[i]
			for j in range(pairSet.PairValueCount):
				rule = note = None
				pairValueRec = pairSet.PairValueRecord[j]
				g2 = pairValueRec.SecondGlyph
				note = ""
				if context:
					note = " # Note! Not in input sequence"
				if pairValueRec.Value1:
					valueString = getValRec(subtable.ValueFormat1, pairValueRec.Value1)
					if valueString and valueString != "0" and valueString != kNullValueRecord:
						rule = "%spos %s %s %s%s;" % (indent, g1, g2, valueString, note)
				if 	pairValueRec.Value2:
					valueString = getValRec(subtable.ValueFormat1, pairValueRec.Value2)
					if valueString and valueString != "0" and  valueString != kNullValueRecord:
						note = "%s # Warning: non-zero pos value record for second glyph %s" % (note)
						rule ="%spos %s %s %s;%s" % (indent, g1, g2, valueString, note)
				if rule:
					if context and note:
						noMatchRules.append(rule)
					else:
						rules.append(rule)
						

	elif subtable.Format == 2:
		for i in range(subtable.Class1Count):
			classRec1 = subtable.Class1Record[i]
			# if this class reference exists it has to be in classesByLookup.
			g1 = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, i, otlConv.leftSideTag]
			for j in range(subtable.Class2Count):
				rule = note = None
				classRec2 = classRec1.Class2Record[j]
				g2 = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, j, otlConv.rightSideTag]
				if context:
					if checkGlyphInSequence(g1, inputSeqList, 0) and (checkGlyphInSequence(g2, inputSeqList, 1)):
						note = ""
					else:
						note = " # Note! Not in input sequence"
				else:
					note = ""
				if 	classRec2.Value1:
					valueString = getValRec(subtable.ValueFormat1, classRec2.Value1)
					if valueString and valueString != "0":
						rule = "%spos %s %s %s;%s" % (indent, g1, g2, valueString, note)
					
				if 	classRec2.Value2:
					valueString = getValRec(subtable.ValueFormat1, classRec2.Value2)
					if valueString and valueString != "0":
						note = "%s # Warning: non-zero pos value record for second glyph %s" % (note)
						rule = "%spos %s %s %s;%s" % (indent, g1, g2, valueString, note)
				if rule:
					if context and note:
						noMatchRules.append(rule)
					else:
						rules.append(rule)
	else:
		raise AttributeError("Unknown Pair Pos format")

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
def ruleCursivePos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	cursivelyphList = subtable.Coverage.glyphs
	for i in range(subtable.EntryExitCount):
		rule =  note = None
		tglyph = cursivelyphList[i]
		if context and (not checkGlyphInSequence(tglyph, inputSeqList, 0)):
			note = " # Note! Not in input sequence"
		else:
			note = ""
		eRec = subtable.EntryExitRecord[i]
		anchor1 = getAnchorString(eRec.EntryAnchor)
		anchor2 = getAnchorString(eRec.ExitAnchor)
		rule = "%spos cursive %s %s %s;%s" % (indent, tglyph, anchor1, anchor2, note)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
def ruleMarkBasePos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""
	subsequentIndent = indent + INDENT
	
	markGlyphList = subtable.MarkCoverage.glyphs
	baseGlyphList = subtable.BaseCoverage.glyphs
	markClass = collections.defaultdict(list)
	classCount = subtable.ClassCount
	
	for m in range(len(markGlyphList)):
		markRec = subtable.MarkArray.MarkRecord[m]
		markClass[markRec.Class].append( (getAnchorString(markRec.MarkAnchor),markGlyphList[m]) )
		
	# Get the mark class names from the global dict
	markClassNameList = []
	for c in markClass.keys():
		markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, c, otlConv.markTag]
		markClassNameList.append(markClassName)
	
	# build a dict of [common set of anchors] -> glyph list.
	baseAnchorSets = collections.defaultdict(list)
	for b in range(len(baseGlyphList)):
		baseRec = subtable.BaseArray.BaseRecord[b]
		anchorList = []
		for c in range(classCount):
			anchorList.append(getAnchorString(baseRec.BaseAnchor[c]))
		anchorKey = tuple(anchorList)
		baseAnchorSets[anchorKey].append(baseGlyphList[b])
		
	
	for anchorKey, glyphList in baseAnchorSets.items():
		note = None
		glyphList.sort() 
		if context:
			if type(inputSeqList[0]) == type([]):
				inputList = inputSeqList[0]
			else:
				inputList = [inputSeqList[0]]
			overlapList = set(glyphList).intersection(inputList)
			if not overlapList:
				note = " # Note! Not in input sequence"
				glyphTxt = " ".join(glyphList)
			else:
				overlapList.sort()
				note = ""
				glyphTxt = " ".join(overlapList)
		else:
			note = ""
			glyphTxt = " ".join(glyphList)
		rule = ["%spos base [%s]" % (indent, glyphTxt)]
		for cl in range(classCount):
			rule.append(" %s mark %s" % (anchorKey[cl], markClassNameList[cl]) )
			if (cl +1 ) < classCount: # if it is not the last one
				rule.append("\n%s" % (subsequentIndent)	)
		rule.append(";")
		if note:
			rule.append(note)
		rule = "".join(rule)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)
		
	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
def ruleMarkLigPos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context
	
	markGlyphList = subtable.MarkCoverage.glyphs
	ligGlyphList = subtable.LigatureCoverage.glyphs
	markClass = collections.defaultdict(list)
	classCount = subtable.ClassCount
	
	for m in range(len(markGlyphList)):
		markRec = subtable.MarkArray.MarkRecord[m]
		markClass[markRec.Class].append( (getAnchorString(markRec.MarkAnchor),markGlyphList[m]) )
		
	# Get the mark class names from the global dict
	markClassNameList = []
	for c in markClass.keys():
		markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, c, otlConv.markTag]
		markClassNameList.append(markClassName)
	
	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""
	subsequentIndent = indent + INDENT
	ligArray = subtable.LigatureArray
	for l in range(ligArray.LigatureCount):
		ligGlyph = ligGlyphList[l]
		tokenList = ["%spos ligature " % (indent), ligGlyph ]
		if context and (not checkGlyphInSequence(ligGlyph, inputSeqList, 0)):
			note = " # Note! Not in input sequence"
		else:
			note = ""
		ligAttach = ligArray.LigatureAttach[l]
		for cmpIndex in range(ligAttach.ComponentCount):
			if cmpIndex > 0:
				tokenList.append("\n%sligComponent\n%s"% (subsequentIndent, subsequentIndent))
			componentRec = ligAttach.ComponentRecord[cmpIndex]
			for cl in range(classCount):
				tokenList.append( " %s mark %s" % (getAnchorString(componentRec.LigatureAnchor[cl]), markClassNameList[cl]) )
				if (cl +1 ) < classCount:
					tokenList.append("\n%s" % (subsequentIndent) )
		tokenList.append(";")
		if note:
			tokenList.append(note)
		rule = "".join(tokenList)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)
		
	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
def ruleMarkMarkPos(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""
	subsequentIndent = indent + INDENT
	markGlyphList = subtable.Mark1Coverage.glyphs
	mark2GlyphList = subtable.Mark2Coverage.glyphs
	markClass = collections.defaultdict(list)
	classCount = subtable.ClassCount
	
	for m in range(len(markGlyphList)):
		markRec = subtable.Mark1Array.MarkRecord[m]
		markClass[markRec.Class].append( (getAnchorString(markRec.MarkAnchor),markGlyphList[m]) )
		
	# Get the mark class names from the global dict
	markClassNameList = []
	for c in markClass.keys():
		markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, c, otlConv.mark1Tag]
		markClassNameList.append(markClassName)
	
	mark2AnchorSets = collections.defaultdict(list)
	for b in range(len(mark2GlyphList)):
		mark2Rec = subtable.Mark2Array.Mark2Record[b]
		anchorList = []
		for c in range(classCount):
			anchorList.append(getAnchorString(mark2Rec.Mark2Anchor[c]))
		anchorKey = tuple(anchorList)
		mark2AnchorSets[anchorKey].append(mark2GlyphList[b])
		
	for anchorKey, glyphList in mark2AnchorSets.items():
		note = None
		glyphList.sort() # This sort will lead the rules to be sorted by base glyph names.
		if context:
			if type(inputSeqList[0]) == type([]):
				inputList = inputSeqList[0]
			else:
				inputList = [inputSeqList[0]]
			overlapList = set(glyphList).intersection(inputList)
			if not overlapList:
				note = " # Note! Not in input sequence"
				glyphTxt = " ".join(glyphList)
			else:
				note = ""
				glyphTxt = " ".join(overlapList)
		else:
			note = ""
			glyphTxt = " ".join(glyphList)
			
		rule = ["%spos mark [%s]" % (indent, glyphTxt)]
		for cl in range(classCount):
			rule.append(" %s mark %s" % (anchorKey[cl], markClassNameList[cl]) )
			if (cl +1 ) < classCount: # if it is not the last one
				rule.append("\n%s" % (subsequentIndent)	)
		rule.append(";")
		if note:
			rule.append(note)
		rule = "".join(rule)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)
		
	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
	
def ruleContextPOS(subtable, otlConv, context = None):
	chainRules = []
	pLookupList =  otlConv.table.LookupList.Lookup

	if subtable.Format == 3:

		inputList = []
		inputList2 = []
		for i in range(subtable.GlyphCount):
			glyphList = subtable.Coverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			inputList2.append(glyphList)
			inputList.append("[" + (" ".join(glyphList)) + "]")
		inputTxt = "' ".join(inputList) + "'"

		rule = "pos %s;" % (inputTxt)
		pLookupList =  otlConv.table.LookupList.Lookup
		posRules = []
		for i in range(subtable.PosCount):
			subRec = subtable.PosLookupRecord[i]
			if not subRec:
				continue
			lookup = pLookupList[subRec.LookupListIndex]
			lookupType = lookup.LookupType
			curLI = otlConv.curLookupIndex 
			otlConv.curLookupIndex = subRec.LookupListIndex
			handler = otlConv.ruleHandlers[(lookupType)]
			contextRec = ContextRecord(inputList2, subRec.SequenceIndex)
			for si in range(lookup.SubTableCount):
				curSI = otlConv.curSubTableIndex
				otlConv.curSubTableIndex = si
				subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
				otlConv.curSubTableIndex = curSI
				posRules.extend(subtablerules)
			otlConv.curLookupIndex = curLI
		chainRules.append([rule, posRules])

	elif subtable.Format == 2:
		subRules = []
		for i in range(subtable.PosClassSetCount):
			ctxClassSet = subtable.PosClassSet[i]
			if not ctxClassSet:
				continue
			for j in range(ctxClassSet.PosClassRuleCount):
				ctxClassRule = ctxClassSet.PosClassRule[j]

				inputList = []
				className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, i, otlConv.InputTag]
				inputList = [className]
				inputList2 = [ otlConv.classesByClassName[className] ]
				for c in range(ctxClassRule.InputGlyphCount -1):
					classIndex = ctxClassRule.Input[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.InputTag]
					inputList.append(className)
					inputList2.append(otlConv.classesByClassName[className])
				inputTxt = "' ".join(inputList) + "'"
				rule = "pos %s;" % (inputTxt)
				subRules = []
				for k in range(ctxClassRule.PosCount):
					subsRec = ctxClassRule.PosLookupRecord[k]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])

	elif subtable.Format == 1:
		firstGlyphList = subtable.Coverage.glyphs
		for si in range(subtable.PosRuleSetCount): # for each glyph in the coverage table
			firstGlyph = firstGlyphList[si]
			subRuleSet = subtable.PosRuleSet[si]
			for ri in range(subRuleSet.PosRuleCount):
				ctxRuleRec = subRuleSet.PosRule[ri]
				inputList = [firstGlyph] + ctxRuleRec.Input
				inputTxt = "' ".join(inputList) + "'"
				rule = "pos %s" % (inputTxt)
				subRules = []
				for i in range(ctxRuleRec.PosCount):
					subsRec = ctxRuleRec.PosLookupRecord[i]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])
	else:
		raise AttributeError("Unknown  Chain Context Sub format %s" % (subtable.Format))

	chainRules.sort()
	rules = []
	for chainRule, subRules in chainRules:
		rules.append(chainRule)
		rules.extend(subRules)
	return rules	
	
def ruleChainContextPOS(subtable, otlConv, context = None):
	chainRules = []
	if subtable.Format == 3:
		backtrackList = []
		for i in range(subtable.BacktrackGlyphCount):
			glyphList = subtable.BacktrackCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			backtrackList.append("[" + (" ".join(glyphList)) + "]")
		backtrackList.reverse()
		backTxt = " ".join(backtrackList)

		inputList = []
		inputList2 = []
		for i in range(subtable.InputGlyphCount):
			glyphList = subtable.InputCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			inputList2.append(glyphList)
			inputList.append("[" + (" ".join(glyphList)) + "]")
		inputTxt = "' ".join(inputList) + "'"

		lookAheadList = []
		for i in range(subtable.LookAheadGlyphCount):
			glyphList = subtable.LookAheadCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			lookAheadList.append("[" + (" ".join(glyphList)) + "]")
		lookAheadTxt = " ".join(lookAheadList)
		
		rule = "pos %s %s %s;" % (backTxt, inputTxt, lookAheadTxt)
		pLookupList =  otlConv.table.LookupList.Lookup
		posRules = []
		for i in range(subtable.PosCount):
			subRec = subtable.PosLookupRecord[i]
			if not subRec:
				continue
			lookup = pLookupList[subRec.LookupListIndex]
			lookupType = lookup.LookupType
			curLI = otlConv.curLookupIndex 
			otlConv.curLookupIndex = subRec.LookupListIndex
			handler = otlConv.ruleHandlers[(lookupType)]
			contextRec = ContextRecord(inputList2, subRec.SequenceIndex)
			for si in range(lookup.SubTableCount):
				curSI = otlConv.curSubTableIndex
				otlConv.curSubTableIndex = si
				subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
				otlConv.curSubTableIndex = curSI
				posRules.extend(subtablerules)
			otlConv.curLookupIndex = curLI
		chainRules.append([rule, posRules])

	elif subtable.Format == 2:
		subRules = []
		for i in range(subtable.ChainPosRuleSetCount):
			ctxClassSet = subtable.ChainSubClassSet[i]
			if not ctxClassSet:
				continue
			for j in range(ctxClassSet.ChainSubClassRuleCount):
				ctxClassRule = ctxClassSet.ChainPosRuleSet[j]
				backTrackList = []
				for c in range(ctxClassRule.BacktrackGlyphCount):
					classIndex = ctxClassRule.Backtrack[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.backtrackTag]
					backTrackList.append(className)
				backTxt = " ".join(backTrackList)

				className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, i, otlConv.InputTag]
				inputList = [className]
				inputList2 = [ otlConv.classesByClassName[className] ]
				for c in range(ctxClassRule.InputGlyphCount -1):
					classIndex = ctxClassRule.Input[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.InputTag]
					inputList.append(className)
					inputList2.append(otlConv.classesByClassName[className])
				inputTxt = "' ".join(inputList) + "'"

				lookAheadList = []
				for c in range(ctxClassRule.LookAheadGlyphCount):
					classIndex = ctxClassRule.LookAhead[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.lookAheadTag]
					lookAheadList.append(className)
				lookTxt = " ".join(lookAheadList)

				rule = "sub %s %s %s;" % (backTxt, inputTxt, lookTxt)
				subRules = []
				for k in range(ctxClassRule.SubstCount):
					subsRec = ctxClassRule.SubstLookupRecord[k]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])

	elif subtable.Format == 1:
		firstGlyphList = subtable.Coverage.glyphs
		for si in range(subtable.ChainPosRuleSetCount): # for each glyph in the coverage table
			firstGlyph = firstGlyphList[si]
			subRuleSet = subtable.ChainPosRuleSet[si]
			for ri in range(subRuleSet.ChainPosRuleCount):
				ctxRuleRec = subRuleSet.ChainPosRule[ri]
				backList = ctxRuleRec.Backtrack
				backList.reverse()
				backTxt = " ".join(backList)
				inputList = [firstGlyph] + ctxRuleRec.Input
				inputTxt = "' ".join(inputList) + "'"
				lookAheadTxt = " ".join(ctxRuleRec.LookAhead)
				rule = "sub %s %s %s" % (backTxt, inputTxt, lookAheadTxt)
				subRules = []
				for i in range(ctxRuleRec.SubstCount):
					subsRec = ctxRuleRec.SubstLookupRecord[i]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])
	else:
		raise AttributeError("Unknown  Chain Context Sub format %s" % (subtable.Format))

	chainRules.sort()
	rules = []
	for chainRule, subRules in chainRules:
		rules.append(chainRule)
		rules.extend(subRules)
	return rules	
	
def ruleExtPOS(subtable, otlConv, context = None):
	handler = otlConv.ruleHandlers[(subtable.ExtensionLookupType)]
	rules = handler(subtable.ExtSubTable, otlConv, context)
	return rules

def ruleSingleSub(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	items = subtable.mapping.items()
	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	for g1, g2 in items:
		rule = note = None
		if context and (not checkGlyphInSequence(g1, inputSeqList, 0)):
			note = " # Note! Not in input sequence"
		else:
			note = ""
		rule = "%ssub %s by %s;%s" % (indent, g1, g2, note)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
		
	
def ruleMultipleSub(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""
	for i in range(subtable.SequenceCount):
		rule = note = None
		g1 = subtable.Coverage.glyphs[i]
		if context and (not checkGlyphInSequence(g1, inputSeqList, 0)):
			note = " # Note! Not in input sequence"
		else:
			note = ""
		substGlyphList = []
		subRec = subtable.Sequence[i]
		for j in range(subRec.GlyphCount):
			substGlyphList.append(subRec.Substitute[j])
		substGlyphList.sort()
		subTxt = " ".join(substGlyphList)
		rule = "%ssub %s by [%s];%s" % (indent, g1, subTxt, note)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules
		

def ruleAltSub(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context
	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	items = subtable.alternates.items()
	for g1, alts in items:
		rule = note = None
		if context and (not checkGlyphInSequence(g1, inputSeqList, 0)):
			note = " # Note! Not in input sequence"
		else:
			note = ""
		alts.sort()
		altText = " ".join(alts) 
		rule = "%ssub %s from [%s];%s" % (indent, g1, altText, note)
		if context and note:
			noMatchRules.append(rule)
		else:
			rules.append(rule)

	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules			
	
def ruleLigatureSub(subtable, otlConv, context = None):
	rules = []
	noMatchRules = [] # contains the rules that don't match the context

	if context:
		indent = ChainINDENT
		inputSeqList = context.inputList[context.sequenceIndex:]
	else:
		indent = ""

	items = subtable.ligatures.items()
	for item in items:
		ligs = item[1]
		firstGlyph = item[0]
		for lig in ligs:
			rule = note = None
			tokenList = [firstGlyph]
			missing = []
			foundAll = 1
			if context:
				try:
					if not checkGlyphInSequence(firstGlyph, inputSeqList, 0):
						raise ValueError

					# make sure everything else is also present with the same index.
					for i in range(len(lig.Component)):
						gname = lig.Component[i]
						if not checkGlyphInSequence(gname, inputSeqList, i+1):
							foundAll = 0
							missing.append(gname)

					if not foundAll:
						note = " # Note! lig comonents %s are not in input sequence with same index." % (" ".join(missing))
					else:
						note = ""
				except ValueError:
						note = " # Note! first glyph %s is not in input sequence." % (firstGlyph)
			else:
				note = ""

			tokenList.extend(lig.Component)
			glyphTxt = " ".join(tokenList)
			rule = "%ssub %s by %s;%s" % (indent, glyphTxt, lig.LigGlyph, note)
			if context and note:
				noMatchRules.append(rule)
			else:
				rules.append(rule)
	if rules:
		rules.sort()
		return rules
	else:
		noMatchRules.sort()
		return noMatchRules

	
def ruleContextSUB(subtable, otlConv, context = None):
	chainRules = []
	pLookupList =  otlConv.table.LookupList.Lookup

	if subtable.Format == 3:

		inputList = []
		inputList2 = []
		for i in range(subtable.GlyphCount):
			glyphList = subtable.Coverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			inputList2.append(glyphList)
			inputList.append("[" + (" ".join(glyphList)) + "]'")
		inputTxt = "' ".join(inputList) + "'"

		rule = "sub %s;" % (inputTxt)
		pLookupList =  otlConv.table.LookupList.Lookup
		posRules = []
		for i in range(subtable.SubstCount):
			subRec = subtable.SubstLookupRecord[i]
			if not subRec:
				continue
			lookup = pLookupList[subRec.LookupListIndex]
			lookupType = lookup.LookupType
			curLI = otlConv.curLookupIndex 
			otlConv.curLookupIndex = subRec.LookupListIndex
			handler = otlConv.ruleHandlers[(lookupType)]
			contextRec = ContextRecord(inputList2, subRec.SequenceIndex)
			for si in range(lookup.SubTableCount):
				curSI = otlConv.curSubTableIndex
				otlConv.curSubTableIndex = si
				subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
				otlConv.curSubTableIndex = curSI
				posRules.extend(subtablerules)
			otlConv.curLookupIndex = curLI
		chainRules.append([rule, posRules])

	elif subtable.Format == 2:
		subRules = []
		for i in range(subtable.SubClassSetCount):
			ctxClassSet = subtable.SubClassSet[i]
			if not ctxClassSet:
				continue
			for j in range(ctxClassSet.SubClassRuleCount):
				ctxClassRule = ctxClassSet.SubClassRule[j]

				inputList = []
				className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, i, otlConv.InputTag]
				inputList = [className]
				inputList2 = [ otlConv.classesByClassName[className] ]
				for c in range(ctxClassRule.InputGlyphCount -1):
					classIndex = ctxClassRule.Input[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.InputTag]
					inputList.append(className)
					inputList2.append(otlConv.classesByClassName[className])
				inputTxt = "' ".join(inputList) + "'"


				rule = "sub %s;" % (inputTxt)
				subRules = []
				for k in range(ctxClassRule.SubstCount):
					subsRec = ctxClassRule.SubstLookupRecord[k]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])

	elif subtable.Format == 1:
		firstGlyphList = subtable.Coverage.glyphs
		for si in range(subtable.SubRuleSetCount): # for each glyph in the coverage table
			firstGlyph = firstGlyphList[si]
			subRuleSet = subtable.SubRuleSet[si]
			for ri in range(subRuleSet.SubRuleCount):
				ctxRuleRec = subRuleSet.SubRule[ri]
				inputList = [firstGlyph] + ctxRuleRec.Input
				inputTxt = "' ".join(inputList) + "'"
				rule = "sub %s" % (inputTxt)
				subRules = []
				for i in range(ctxRuleRec.SubstCount):
					subsRec = ctxRuleRec.SubstLookupRecord[i]
					if not subsRec:
						continue
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])
	else:
		raise AttributeError("Unknown  Chain Context Sub format %s" % (subtable.Format))

	chainRules.sort()
	rules = []
	for chainRule, subRules in chainRules:
		rules.append(chainRule)
		rules.extend(subRules)
	return rules	
	
def ruleChainContextSUB(subtable, otlConv, context = None):
	chainRules = []
	pLookupList =  otlConv.table.LookupList.Lookup
	
	if subtable.Format == 3:
		backtrackList = []
		for i in range(subtable.BacktrackGlyphCount):
			glyphList = subtable.BacktrackCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			backtrackList.append("[" + (" ".join(glyphList)) + "]")
		backtrackList.reverse()
		backTxt = " ".join(backtrackList)

		inputList = []
		inputList2 = []
		for i in range(subtable.InputGlyphCount):
			glyphList = subtable.InputCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			inputList2.append(glyphList)
			inputList.append("[" + (" ".join(glyphList)) + "]'")
		inputTxt = " ".join(inputList)

		lookAheadList = []
		for i in range(subtable.LookAheadGlyphCount):
			glyphList = subtable.LookAheadCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			lookAheadList.append("[" + (" ".join(glyphList)) + "]")
		lookAheadTxt = " ".join(lookAheadList)
		rule = "sub %s %s %s;" % (backTxt, inputTxt, lookAheadTxt)
		subRules = []
		for i in range(subtable.SubstCount):
			subsRec = subtable.SubstLookupRecord[i]
			lookup = pLookupList[subsRec.LookupListIndex]
			lookupType = lookup.LookupType
			curLI = otlConv.curLookupIndex 
			otlConv.curLookupIndex = subsRec.LookupListIndex
			handler = otlConv.ruleHandlers[(lookupType)]
			contextRec = ContextRecord(inputList2, subsRec.SequenceIndex)
			for si in range(lookup.SubTableCount):
				curSI = otlConv.curSubTableIndex
				otlConv.curSubTableIndex = si
				subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
				otlConv.curSubTableIndex = curSI
				subRules.extend(subtablerules)
			otlConv.curLookupIndex = curLI
		chainRules.append([rule, subRules])

	elif subtable.Format == 2:
		subRules = []
		for i in range(subtable.ChainSubClassSetCount):
			ctxClassSet = subtable.ChainSubClassSet[i]
			if not ctxClassSet:
				continue
			for j in range(ctxClassSet.ChainSubClassRuleCount):
				ctxClassRule = ctxClassSet.ChainSubClassRule[j]
				inputList = []
				backTrackList = []
				for c in range(ctxClassRule.BacktrackGlyphCount):
					classIndex = ctxClassRule.Backtrack[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.backtrackTag]
					backTrackList.append(className)
				backTxt = " ".join(backTrackList)

				className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, i, otlConv.InputTag]
				inputList = [className]
				inputList2 = [ otlConv.classesByClassName[className] ]
				for c in range(ctxClassRule.InputGlyphCount -1):
					classIndex = ctxClassRule.Input[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.InputTag]
					inputList.append(className)
					inputList2.append(otlConv.classesByClassName[className])
				inputTxt = " ".join(inputList)

				lookAheadList = []
				for c in range(ctxClassRule.LookAheadGlyphCount):
					classIndex = ctxClassRule.LookAhead[c]
					className = otlConv.classesByLookup[otlConv.curLookupIndex, otlConv.curSubTableIndex, classIndex, otlConv.lookAheadTag]
					lookAheadList.append(className)
				lookTxt = " ".join(lookAheadList)

				rule = "sub %s %s' %s;" % (backTxt, inputTxt, lookTxt)
				subRules = []
				for k in range(ctxClassRule.SubstCount):
					subsRec = ctxClassRule.SubstLookupRecord[k]
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])

	elif subtable.Format == 1:
		firstGlyphList = subtable.Coverage.glyphs
		for si in range(subtable.ChainSubRuleSetCount): # for each glyph in the coverage table
			firstGlyph = firstGlyphList[si]
			subRuleSet = subtable.ChainSubRuleSet[si]
			for ri in range(subRuleSet.ChainSubRuleCount):
				ctxRuleRec = subRuleSet.ChainSubRule[ri]
				backList = ctxRuleRec.Backtrack
				backList.reverse()
				backTxt = " ".join(backList)
				inputList = [firstGlyph] + ctxRuleRec.Input
				inputTxt = " ".join(inputList)
				lookAheadTxt = " ".join(ctxRuleRec.LookAhead)
				rule = "sub %s %s' %s by " % (backTxt, inputTxt, lookAheadTxt)
				subRules = []
				for i in range(ctxRuleRec.SubstCount):
					subsRec = ctxRuleRec.SubstLookupRecord[i]
					lookup = pLookupList[subsRec.LookupListIndex]
					lookupType = lookup.LookupType
					curLI = otlConv.curLookupIndex 
					otlConv.curLookupIndex = subsRec.LookupListIndex
					handler = otlConv.ruleHandlers[(lookupType)]
					contextRec = ContextRecord(inputList, subsRec.SequenceIndex)
					for si in range(lookup.SubTableCount):
						curSI = otlConv.curSubTableIndex
						otlConv.curSubTableIndex = si
						subtablerules = handler(lookup.SubTable[si], otlConv, contextRec)
						otlConv.curSubTableIndex = curSI
						subRules.extend(subtablerules)
					otlConv.curLookupIndex = curLI
				chainRules.append([rule, subRules])
	else:
		raise AttributeError("Unknown  Chain Context Sub format %s" % (subtable.Format))
				
	chainRules.sort()
	rules = []
	for chainRule, subRules in chainRules:
		rules.append(chainRule)
		rules.extend(subRules)
	return rules	

def ruleExtSub(subtable, otlConv, context = None):
	handler = otlConv.ruleHandlers[(subtable.ExtensionLookupType)]
	rules = handler(subtable.ExtSubTable, otlConv, context)
	return rules

def ruleReverseChainSub(subtable, otlConv, context = None):
	rules = []
	if subtable.Format == 1:
		backtrackList = []
		for i in range(subtable.BacktrackGlyphCount):
			glyphList = subtable.BacktrackCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			backtrackTxt = "[ %s ] " % (" ".join(glyphList))
			backtrackList.append(backtrackTxt)
		backtrackList.reverse()
		backTxt = " ".join(backtrackList)


		glyphList = subtable.Coverage.glyphs
		# Since the substitute list is in the same order as the 
		# coverage table, I need to sort the substitution array
		# in the same order as the input list.
		
		glyphList = [ [glyphList[i], i] for i in range(len(glyphList))]
		glyphList.sort()
		if subtable.Substitute:
			substituteList = []
			for entry in glyphList:
				substituteList.append(subtable.Substitute[entry[1]])
			
		rg = range(1, len(glyphList))
		rg.reverse()
		for i in rg:
			if glyphList[i-1] == glyphList[i]:
				del glyphList[i]
				del substituteList[i]
				
		glyphList = [ entry[0] for entry in glyphList]
		
		inputTxt = "[ %s ]" % (" ".join(glyphList))

		lookAheadList = []
		for i in range(subtable.LookAheadGlyphCount):
			glyphList = subtable.LookAheadCoverage[i].glyphs
			glyphList.sort()
			rg = range(1, len(glyphList))
			rg.reverse()
			for i in rg:
				if glyphList[i-1] == glyphList[i]:
					del glyphList[i]
			lookAheadTxt = " [ %s ]" % (" ".join(glyphList))
			lookAheadList.append(lookAheadTxt)
		lookAheadTxt = " ".join(lookAheadList)
		
		if subtable.GlyphCount:
			replText = " by [ %s ]" % (" ".join(substituteList))
		else:
			replText = ""
		rule = "sub %s%s'%s%s;" % (backTxt, inputTxt, lookAheadTxt, replText)
		rules.append(rule)	
	return rules	


def getValRec(valueFormat, valueRec):
	if valueFormat == 4:
		return "%d" % (valueRec.XAdvance)
	else:
		xPos = yPos = xAdv = yAdv = 0
		if valueFormat & 1:
			xPos = valueRec.XPlacement
		if valueFormat & 2:
			yPos = valueRec.YPlacement
		if valueFormat & 4:
			xAdv = valueRec.XAdvance
		if valueFormat & 8:
			yAdv = valueRec.YAdvance
			
		if xPos == yPos == xAdv == yAdv == 0:
			return "0"
			
		return getValueRecord(xPos, yPos, xAdv,  yAdv)

gLookupFlagMap = {
				1: " RightToLeft",
				2: " IgnoreBaseGlyphs",
				4: " IgnoreLigatures",
				8: " IgnoreMarks",
				0xFF00: " MarkAttachmentTypedef",
				}
gLookupFlagKeys = gLookupFlagMap.keys()
gLookupFlagKeys.sort()

def getLookupFlagTag(lookupFlag):
	retList = []
	for lookupVal in gLookupFlagKeys:
		if lookupVal & lookupFlag:
			retList.append(gLookupFlagMap[lookupVal])
			if lookupVal == 0xFF00:
				classIndex = (lookupVal & lookupFlag)>>8
				retList.append(" %d" % (classIndex))
	return " ".join(retList)
	
class OTLConverter:
	leftSideTag = "Left"
	rightSideTag = "Right"
	backtrackTag = "BacktrackClassDef"
	InputTag = "InputClassDef"
	lookAheadTag = "LookaheadClassDef"
	markTag = "MarkClass"
	mark1Tag = "MarkMarkClass1"
	mark2Tag = "MarkMarkClass2"
	
	def __init__(self, table, ttFont):
		self.table = table.table
		self.classesByNameList = collections.defaultdict(list)
		self.classesByLookup = {}
		self.classesByClassName = {}
		self.lookupIndex = -1
		self.seenLookup = {}
		self.glyphDict = {}
		self.markClassesByDefList = collections.defaultdict(list)
		self.markClassesByLookup = collections.defaultdict(list)
		self.markClassesByClassName = {}
		self.curLookupName = ""
		self.showExtension = ttFont.showExtensionFlag
		
		for name in ttFont.getGlyphOrder():
			self.glyphDict[name] = 0
		
		if table.tableTag == "GPOS":
			self.ExtensionIndex = 9
			self.classHandlers = {
				(2,2) : classPairGPOS,
				(4,1) : markClassPOS,
				(5,1) : markLigClassPOS,
				(6,1) : markMarkClassPOS,
				(7,2) : classContexGPOS,
				(8,2) : classChainContextGPOS,
				(9,1) : classExtPOS,
				}
			self.ruleHandlers = {
				(1) : ruleSinglePos,
				(2) : rulePairPos,
				(3) : ruleCursivePos,
				(4) : ruleMarkBasePos,
				(5) : ruleMarkLigPos,
				(6) : ruleMarkMarkPos,
				(7) : ruleContextPOS,
				(8) : ruleChainContextPOS,
				(9) : ruleExtPOS,
				}
				
		elif table.tableTag == "GSUB":
			self.ExtensionIndex = 7
			self.classHandlers = {
				(5,2) : classContextGSUB,
				(6,2) : classChainContextGSUB,
				(7,2) : classExtSUB,
				}
			self.ruleHandlers = {
				(1) : ruleSingleSub,
				(2) : ruleMultipleSub,
				(3) : ruleAltSub,
				(4) : ruleLigatureSub,
				(5) : ruleContextSUB,
				(6) : ruleChainContextSUB,
				(7) : ruleExtSub,
				(8) : ruleReverseChainSub,
				}
		else:
			raise KeyError("OTLConverter can only be called for GPOS and GSUB tables")
				
	def otlFeatureFormat(self, writer):
		writer.newline()
		writer.indent()
		
		# get and write language systems
		lsList, featDictByLangSys, featDictByIndex = self.buildLangSys()
		self.writeLangSys(writer, lsList)
		
		writer.newline()

		# get and write class defs.
		self.buildClasses()
		self.writeClasses(writer)
		self.writeMarkClasses(writer)

		# do feature defs
		self.doFeatures(writer, featDictByLangSys)
		
		# write feature defs.
		
		writer.dedent()
		writer.newline()
		
	def addLangSysEntry(self, pfeatList, scriptTag, langTag, langSys, featDictByLangSys, featDictByIndex):
		for featIndex in langSys.FeatureIndex:
			try:
				featRecord = pfeatList[featIndex]
			except IndexError:
				print "FeatList does not contain index %s from current langsys (%s, %s)." % (featIndex,scriptTag,langTag )
				print "FeaturListLen %s,  langsys FeatIndex %s." % (len(pfeatList),  langSys.FeatureIndex)
				continue
			featTag = featRecord.FeatureTag
			lang_sys = (scriptTag, langTag)
			try:
				featDictByIndex[featIndex].append(lang_sys)
			except KeyError:
				featDictByIndex[featIndex] = [lang_sys]
			
			langsysDict = featDictByLangSys.get(featTag, {})
			try:
				langsysDict[lang_sys].append(featIndex)
			except KeyError:
				langsysDict[lang_sys] = [featIndex]
			featDictByLangSys[featTag] = langsysDict
	
	def buildLangSys(self):
		pfeatList = self.table.FeatureList.FeatureRecord
		featDictByLangSys = {}
		featDictByIndex = {}
		haveDFLT = 0
		lsList = []
		for scriptRecord in self.table.ScriptList.ScriptRecord:
			scriptTag = scriptRecord.ScriptTag
			if scriptTag == "DFLT":
				haveDFLT = 1
			if scriptRecord.Script.DefaultLangSys:
				langTag = "dflt"
				lsList.insert(0, (scriptTag, langTag, scriptRecord.Script.DefaultLangSys.ReqFeatureIndex))
				self.addLangSysEntry(pfeatList, scriptTag, langTag, scriptRecord.Script.DefaultLangSys, featDictByLangSys, featDictByIndex)
	
			for langSysRecord in scriptRecord.Script.LangSysRecord:
				langTag = langSysRecord.LangSysTag
				lsList.append( (scriptTag, langTag, langSysRecord.LangSys.ReqFeatureIndex) )
				self.addLangSysEntry(pfeatList, scriptTag, langTag, langSysRecord.LangSys, featDictByLangSys, featDictByIndex)
		
		if not haveDFLT:
			# Synthesize a DFLT dflt entry.
			addedDFLT = 0
			scriptCount = self.table.ScriptList.ScriptCount
			for langsysList in featDictByIndex.values():
				if len(langsysList) == scriptCount:
					langsysList.insert(0, ("DFLT", "dflt", 0xFFFF))
					addedDFLT = 1
					
			if addedDFLT:
				lsList.insert(0, ("DFLT", "dflt", 0xFFFF))
				
			
		return lsList, featDictByLangSys, featDictByIndex


	def writeLangSys(self, writer, lsList):
		# lsList is alphabetic, because of font order, except that I moved (DFLT, dflt) script to the top.
		for lang_sys in lsList:
			if lang_sys[2] != 0xFFFF:
				rfTxt = "  Required Feature Index %s" % (lang_sys[2])
			else:
				rfTxt = ""
			writer.write("languagesystem %s %s%s;" %  (lang_sys[0], lang_sys[1], rfTxt))
			writer.newline()
		
	def buildClasses(self):
		lookupIndex = -1
		for lookup in self.table.LookupList.Lookup:
			lookupIndex += 1
			self.curLookupIndex = lookupIndex
			subtableIndex = -1
			subtableIndex = -1
			for subtable in lookup.SubTable:
				subtableIndex += 1
				self.curSubTableIndex = subtableIndex
				try:
					format = subtable.Format
				except AttributeError:
					continue
				handler = self.classHandlers.get( (lookup.LookupType, format), None)
				if handler:
					handler(subtable, self)

		classNameLists = self.classesByNameList.keys()
		classNameLists.sort()
		for nameList in classNameLists:
			classRecList = self.classesByNameList[nameList]
			lenList = len(nameList)
			if lenList == 0:
				className = "@empty"
			elif lenList == 1:
				className = "[%s]" % (nameList[0])
			elif lenList == 2:
				className = "@%s_%s" % (nameList[0], nameList[1])
			else:
				className = "@%s_%d_%s" % (nameList[0], len(nameList), nameList[-1])
				i = 1
				while( (i < lenList) and self.classesByClassName.has_key(className)):
					className = "@%s_%s_%d_%s" % (nameList[0], nameList[i], len(nameList), nameList[-1])
			self.classesByClassName[className]= nameList
			for classRec in classRecList:
				key = (classRec.lookupIndex, classRec.subtableIndex, classRec.classIndex, classRec.type)
				self.classesByLookup[key] = className
			
		classDefLists = self.markClassesByDefList.keys()
		classDefLists.sort()
		for defList in classDefLists:
			classRecList = self.markClassesByDefList[defList]
			defNameList = []
			for nameList,anchor in defList:
				defNameList.extend(list(nameList))
			lenList = len(defNameList)
			if lenList == 0:
				className = "@empty_mark"
			elif lenList == 1:
				className = "%s_mark" % (defNameList[0])
			elif lenList == 2:
				className = "@%s_%s_mark" % (defNameList[0], defNameList[1])
			else:
				className = "@%s_%d_%s_mark" % (defNameList[0], len(defNameList), defNameList[-1])
				i = 1
				while( (i < lenList) and self.classesByClassName.has_key(className)):
					className = "@%s_%s_%d_%s_mark" % (defNameList[0], defNameList[i], len(defNameList), defNameList[-1])

			self.markClassesByClassName[className] = defList

			for classRec in classRecList:
				key = (classRec.lookupIndex, classRec.subtableIndex, classRec.classIndex, classRec.type)
				self.markClassesByLookup[key] = className
			
		return
		
	def writeClasses(self, writer):
		classNames = self.classesByClassName.keys()
		if classNames:
			writer.newline()
			writer.write("# Class definitions *********************************" )
			writer.newline()
		classNames.sort()
		for className in classNames:
			if className[0] == "[":
				continue # we don't write out the single glyph class names, as they are ued in -line as class lists.
			writer.write("%s = [" % (className) )
			nameList = self.classesByClassName[className]
			defString = " ".join(nameList[:10])
			writer.write( " %s" % (defString) )
			nameList = nameList[10:]
			writer.indent()
			while nameList:	
				writer.newline()
				defString = " ".join(nameList[:10])
				writer.write( " %s" % (defString) )
				nameList = nameList[10:]
			writer.dedent()
			writer.write( " ];")
			writer.newline()
		writer.newline()
		

	def writeMarkClasses(self, writer):
		classNames = self.markClassesByClassName.keys()
		if classNames:
			writer.newline()
			writer.write("# Mark Class definitions *********************************" )
			writer.newline()
		classNames.sort()
		for className in classNames:
			classDef = self.markClassesByClassName[className]
			for nameList, anchor in classDef:
				nameTxt = " ".join(nameList[:10])
				writer._writeraw("mark [%s" % (nameTxt))
				writer.indent()
				nameList = nameList[10:]
				while nameList:	
					writer.newline()
					defString = " ".join(nameList[:10])
					writer._writeraw( " %s" % (defString) )
					nameList = nameList[10:]
				writer.dedent()
				writer._writeraw( "] %s %s;" % (anchor, className))
				writer.newline()
		writer.newline()
		

	def doFeatures(self, writer, featDictByLangSys):
		featTagList = featDictByLangSys.keys()
		featTagList.sort()
		for featTag in featTagList:
			writer.write( "feature %s {" % (featTag))
			writer.newline()
			writer.indent()
			langSysDict = featDictByLangSys[featTag]
			dfltLangSysKey = ("DFLT", "dflt")
			langSysTagList = langSysDict.keys()
			langSysTagList.sort()
			try:
				dfltFeatIndexList = langSysDict[dfltLangSysKey]
				langSysTagList.remove(dfltLangSysKey)
			except KeyError:
				dfltFeatIndexList = None
			
			if dfltFeatIndexList:
				dfltLookupIndexDict = self.writeDfltLangSysFeat(writer, dfltLangSysKey, dfltFeatIndexList)
			else:
				dfltLookupIndexDict = None
			prevLangSysKey = dfltLangSysKey
			for langSysKey in langSysTagList:
				self.writeLangSysFeat(writer, langSysKey, prevLangSysKey, langSysDict[langSysKey], dfltLookupIndexDict)
				prevLangSysKey = langSysKey
			writer.dedent()
			writer.write( "} %s;" % (featTag))
			writer.newline()
			writer.newline()
			
	def writeDfltLangSysFeat(self, writer, langSysKey, featIndexList):
		pfeatList = self.table.FeatureList.FeatureRecord
		pLookupList =  self.table.LookupList.Lookup
		lookupIndexDict = {}
		for featIndex in featIndexList:
			featRecord = pfeatList[featIndex]
			for lookupIndex in featRecord.Feature.LookupListIndex:
				lookupIndexDict[lookupIndex] = pLookupList[lookupIndex]
		
		liList = lookupIndexDict.keys()
		liList.sort()
		excludeDLFTtxt = ""
		nameIndex = 0
		for li in liList:
			self.curLookupIndex = li
			lookup =  pLookupList[li]
			lookupFlagTxt = getLookupFlagTag(lookup.LookupFlag)
			if self.showExtension and lookup.LookupType == self.ExtensionIndex:
				useExtension = " useExtension"
			else:
				useExtension = ""
			if self.seenLookup.has_key(li):
				lookupName = self.seenLookup[li]
				writer.write("lookup %s%s%s;" % (lookupName, lookupFlagTxt, excludeDLFTtxt) )
				writer.newline()
			else:
				nameIndex +=1
				lookupName = "%s_%s_%s_%s" % (featRecord.FeatureTag, langSysKey[0], langSysKey[1], nameIndex)
				self.seenLookup[li] = lookupName
				writer.write("lookup %s%s%s {" % (lookupName, excludeDLFTtxt, useExtension) )
				excludeDLFTtxt = "" # Only need to write it once.
				writer.newline()
				writer.indent()
				self.curLookupIndex = li
				self.writeLookup(writer, lookup)
				writer.dedent()
				writer.write("} %s;" % (lookupName) )
				writer.newline()
				writer.newline()
				
		return lookupIndexDict
	
	def writeLangSysFeat(self, writer, langSysKey, prevLangSysKey, featIndexList, dfltLookupIndexDict):
		pfeatList = self.table.FeatureList.FeatureRecord
		pLookupList =  self.table.LookupList.Lookup
		lookupIndexDict = {}
		for featIndex in featIndexList:
			featRecord = pfeatList[featIndex]
			for lookupIndex in featRecord.Feature.LookupListIndex:
				lookupIndexDict[lookupIndex] = pLookupList[lookupIndex]
		
		# Remove all lookups shared with the DFLT dflt script.
		# Note if there were any; if not, then we need to use the exclude keyword with the lookup.
		haveAnyDflt = 0
		excludeDLFT = 0
		if dfltLookupIndexDict:
			lookupIndexList = lookupIndexDict.keys()
			lookupIndexList.sort()
			for lookupIndex in lookupIndexList:
				if dfltLookupIndexDict.has_key(lookupIndex):
					del lookupIndexDict[lookupIndex]
					haveAnyDflt =1
			if not haveAnyDflt:
				excludeDLFT = 1
		liList = lookupIndexDict.keys()
		liList.sort()
		if excludeDLFT:
			excludeDLFTtxt = " excludeDLFT"
		else:
			excludeDLFTtxt = ""
		if liList: # If all the lookups were shared with DFLt dflt, no need to write anything.
			nameIndex = 0
			if prevLangSysKey[0] != langSysKey[0]:
				writer.write("script %s;" % (langSysKey[0]) )
				writer.newline()
				writer.write("language %s;" % (langSysKey[1]) )
				writer.newline()
			elif prevLangSysKey[1] != langSysKey[1]:
				writer.write("language %s;" % (langSysKey[1]) )
				writer.newline()
				
			for li in liList:
				self.curLookupIndex = li
				lookup =  pLookupList[li]
				lookupFlagTxt = getLookupFlagTag(lookup.LookupFlag)
				if self.showExtension and lookup.LookupType == self.ExtensionIndex:
					useExtension = " useExtension"
				else:
					useExtension = ""
			
				if self.seenLookup.has_key(li):
					lookupName = self.seenLookup[li]
					writer.write("lookup %s%s;" % (lookupName, excludeDLFTtxt) )
					excludeDLFTtxt = "" # Only need to write it once.
					writer.newline()
				else:
					nameIndex +=1
					lookupName = "%s_%s_%s_%s" % (featRecord.FeatureTag, langSysKey[0], langSysKey[1], nameIndex)
					self.seenLookup[li] = lookupName
					writer.write("lookup %s%s%s;" % (lookupName, lookupFlagTxt, excludeDLFTtxt) )
					excludeDLFTtxt = "" # Only need to write it once.
					writer.newline()
					writer.indent()
					self.curLookupIndex = li
					self.writeLookup(writer, lookup)
					writer.dedent()
					writer.write("} %s;" % (lookupName) )
					writer.newline()
					writer.newline()
				
	def writeLookup(self, writer, lookup):
		lookupType = lookup.LookupType
		try:
			handler = self.ruleHandlers[(lookupType)]
		except KeyError:
			msg = "Error. Unknown lookup type %s. Skipping lookup." % (lookupType)
			print msg
			writer._writeraw(msg)
			writer.newline()
			return
			
		for si in range(lookup.SubTableCount):
			self.curSubTableIndex = si
			rules = handler(lookup.SubTable[si], self)
			for rule in rules:
				if len(rule) > kMaxLineLength:
					# I had to use a named group because re didn't work with the "\1" when the escape included just a single  quote.
					m = re.search(r"(^\s+)", rule)
					if m:
						indent = m.group(1) + INDENT
					else:
						indent = INDENT
					rule1 = re.sub(r"(?P<endBracket>]'*)\s+(?!;)", "\g<endBracket>\n" + indent, rule)
					ruleList2 = rule1.split("\n")
					for rule2 in ruleList2:
						ruleList3 = gtextWrapper.wrap(rule2)
						for rule3 in ruleList3:
							writer._writeraw(rule3)
							writer.newline()
				else:
					writer._writeraw(rule)
					writer.newline()
		
			
			
		
				
def dumpOTLAsFeatureFile(tableTag, writer, ttf):
	try:
		table = ttf[tableTag]
	except:
		print "Font does not have %s. Skipping." % (tableTag)
		return

	otlConverter = OTLConverter(table, ttf)
	otlConverter.otlFeatureFormat(writer)


def debugmsg(msg):
	import time
	print(msg + time.strftime("  (%H:%M:%S)", time.localtime(time.time())))

class  TTXNWriter(XMLWriter):
	def __init__(self, fileOrPath, indentwhite=INDENT, idlefunc=None, encoding='utf8', indentLevel=0):
		XMLWriter. __init__(self, fileOrPath, indentwhite, idlefunc, encoding)

class  TTXNTTFont(TTFont):
	def __init__(self, file=None, res_name_or_index=None,
			sfntVersion="\000\001\000\000", checkChecksums=0,
			verbose=0, recalcBBoxes=1, allowVID=0, ignoreDecompileErrors=False,
			fontNumber=-1, supressHints = 0, lazy=False, quiet=False):
			
		self.filePath = file
		self.supressHints = supressHints
		TTFont. __init__(self, file, res_name_or_index,sfntVersion, checkChecksums,
			verbose, recalcBBoxes, allowVID, ignoreDecompileErrors,
			fontNumber)

	def _tableToXML(self, writer, tag, progress, quiet):
		if self.has_key(tag):
			table = self[tag]
			report = "Dumping '%s' table..." % tag
		else:
			report = "No '%s' table found." % tag
		if progress:
			progress.setLabel(report)
		elif self.verbose:
			debugmsg(report)
		else:
			if not quiet:
				print report
		if not self.has_key(tag):
			return
		xmlTag = tagToXML(tag)
		if hasattr(table, "ERROR"):
			writer.begintag(xmlTag, ERROR="decompilation error")
		else:
			writer.begintag(xmlTag)
			writer.newline()
			if tag in ("glyf", "CFF "):
				ttxnWriter = TTXNWriter(writer.file, indentLevel=writer.indentlevel)
				print "Dumping '%s' table ..." % (tag)
				dumpFont(ttxnWriter, self.filePath, self.supressHints)
				ttxnWriter.newline()
			elif tag in ("GSUB", "GPOS"):
				ttxnWriter = TTXNWriter(writer.file,  indentLevel=writer.indentlevel)
				print "Dumping '%s' table ..." % (tag)
				dumpOTLAsFeatureFile(tag, ttxnWriter, self)
				ttxnWriter.newline()
			else:
				table.toXML(writer, self)
		writer.endtag(xmlTag)
		writer.newline()
		writer.newline()

def tagToXML(tag):
	"""Similarly to tagToIdentifier(), this converts a TT tag
	to a valid XML element name. Since XML element names are
	case sensitive, this is a fairly simple/readable translation.
	"""
	import re
	if tag == "OS/2":
		return "OS_2"
	elif tag == "GlyphOrder":
		return tag
	if re.match("[A-Za-z_][A-Za-z_0-9]* *$", tag):
		return string.strip(tag)
	else:
		return tagToIdentifier(tag)


def xmlToTag(tag):
	"""The opposite of tagToXML()"""
	if tag == "OS_2":
		return "OS/2"
	if len(tag) == 8:
		return identifierToTag(tag)
	else:
		return tag + " " * (4 - len(tag))
	return tag

def shellcmd(cmdList):
	tempFile = os.tmpfile() # I use this because tx -dump -6 can be very large.
	p = subprocess.Popen(cmdList,stdout=tempFile, stderr=subprocess.STDOUT)
	retCode = p.poll()
	while None == retCode:
		retCode = p.poll()
	tempFile.seek(0)
	log = tempFile.read()
	tempFile.close()
	return log
	
def dumpFont(writer, fontPath, supressHints=0):
	dictTxt = shellcmd(["tx", "-dump", "-0", fontPath])
	if curSystem == "Windows":
		dictTxt = re.sub(r"[\r\n]+", "\n", dictTxt)
	dictTxt = re.sub(r"##[^\r\n]*Filename[^\r\n]+", "", dictTxt, 1)
	lines = dictTxt.splitlines()
	dictTxt = "\n".join(lines)
	writer._writeraw("<FontTopDict>")
	writer.newline()
	writer.indent()
	writer._writeraw(dictTxt)
	writer.newline()
	writer.dedent()
	writer._writeraw("</FontTopDict>")
	writer.newline()
	
	if supressHints:
		charData= shellcmd(["tx", "-dump", "-6", "-n", fontPath])
	else:
		charData= shellcmd(["tx", "-dump", "-6", fontPath])
	
	if curSystem == "Windows":
		charData = re.sub(r"[\r\n]+", "\n", charData)
	charList = re.findall(r"[^ ]glyph\[[^]]+\] \{([^,]+),[^\r\n]+,([^}]+)", charData)
	if "cid.CIDFontName" in dictTxt:
		# fix glyph names to sort
		charList = map(lambda entry: ("cid%s" % (entry[0]).zfill(5), entry[1]) , charList)

	charList = map(lambda entry: entry[0] + entry[1], charList)
	charList.sort()
	charTxt = "\n".join(charList)
	writer._writeraw("<FontOutlines>")
	writer.newline()
	writer.indent()
	writer._writeraw(charTxt)
	writer.newline()
	writer.dedent()
	writer._writeraw("</FontOutlines>")
	writer.newline()
	
	return
	
def ttnDump(input, output, options, showExtensionFlag, supressHints = 0, supressVersions = 0, supressTTFDiffs = 0):
	print 'Dumping "%s" to "%s"...' % (input, output)
	ttf = TTXNTTFont(input, 0, verbose=options.verbose, allowVID=options.allowVID,
			ignoreDecompileErrors=options.ignoreDecompileErrors, supressHints = supressHints)
	splitTables=options.splitTables
	ttf.showExtensionFlag = showExtensionFlag
	
	kDoNotDumpTagList = ["GlyphOrder", "DSIG"]
	if options.onlyTables:
		onlyTables = copy.copy(options.onlyTables)
	else:
		onlyTables = ttf.keys()
		
	if options.skipTables:
		for tag in options.skipTables:
			if tag in onlyTables:
			 	onlyTables.remove(tag)

	for tag in kDoNotDumpTagList:
		if tag in onlyTables:
			onlyTables.remove(tag)
			
	# Zero values that always differ.
	if 'head' in onlyTables:
		head = ttf["head"]
		temp = head.created
		head.created = 0
		head.modified = 0
		head.magicNumber = 0
		head.checkSumAdjustment = 0
		if supressVersions:
			head.fontRevision = 0
	
	if 'hmtx' in onlyTables:
		hmtx = ttf["hmtx"]		
		temp = hmtx.metrics # hmtx must be decompiled *before* we zero the  hhea.numberOfHMetrics value
		if supressTTFDiffs:
			try:
				del temp["CR"]
			except KeyError:
				pass
			try:
				del temp["NULL"]
			except KeyError:
				pass
				
		
	if 'hhea' in onlyTables:
		hhea = ttf["hhea"]
		temp = hhea.numberOfHMetrics
		hhea.numberOfHMetrics = 0
		
	if 'vmtx' in onlyTables:
		vmtx = ttf["vmtx"]
		temp = vmtx.metrics # vmtx must be decompiled *before* we zero the  vhea.numberOfHMetrics value
		if supressTTFDiffs:
			try:
				del temp["CR"]
			except KeyError:
				pass
			try:
				del temp["NULL"]
			except KeyError:
				pass
		
	if 'vhea' in onlyTables:
		vhea = ttf["vhea"]
		temp = vhea.numberOfVMetrics
		vhea.numberOfVMetrics = 0
	
	if supressVersions:
		if 'name' in onlyTables:
			name_table = ttf["name"]
			for namerecord in name_table.names:
				if namerecord.nameID in [3,5]:
					namerecord.string = "VERSION SUPPRSESSED"
		
	if 'GDEF' in onlyTables:
		GDEF = ttf["GDEF"]
		gt = GDEF.table
		if gt.GlyphClassDef:
			gtc = gt.GlyphClassDef.classDefs
			gt.GlyphClassDef.Format = 0
		if gt.MarkAttachClassDef:
			gtc = gt.MarkAttachClassDef.classDefs
			gt.MarkAttachClassDef.Format = 0
		if gt.AttachList:
			if not gt.AttachList.Coverage.glyphs:
				gt.AttachList = None
			
		if gt.LigCaretList:
			if not gt.LigCaretList.Coverage.glyphs:
				gt.LigCaretList = None
				
		
	onlyTables.sort() # this is why I always use the onlyTables option - it allows me to sort the table list.
	if 'cmap' in onlyTables:
		# remove mappings to notdef.
		cmapTable = ttf["cmap"]
		""" Force shared cmap tables to get separately decompiled.
		The _c_m_a_p.py logic will decompile a cmap from source data if an attempt is made to 
		access a field which is not (yet) defined. Unicode ( format 4) subtables
		are usually identical, and thus shared. When intially decompiled, the first gets
		fully decompiled, and then the second gets a reference to the 'cmap' dict of the first.
		WHen entries are removed from the cmap dict of the first subtable, that is the same as
		removing them from the cmap dict of the second. However, when later an attempt is made
		to reference the 'nGroups' field - which doesn't exist  in format 4 - the second table
		gets fully decompiled, and its cmap dict is rebuilt from the original data.
		"""
		for cmapSubtable in cmapTable.tables:
			#if cmapSubtable.format != 4:
			#	continue
			delList = []
			items = cmapSubtable.cmap.items()
			for mapping in items:
				if mapping[1] == ".notdef":
					delList.append(mapping)
				if supressTTFDiffs:
					if mapping[1] == "CR":
						delList.append(mapping)
					if mapping[1] == "NULL":
						delList.append(mapping)
			if delList:
				for charCode, glyphName in delList:
					try:
						del cmapSubtable.cmap[charCode]
					except KeyError:
						pass
			
			if (cmapSubtable.format in [12,14]) and hasattr(cmapSubtable, "nGroups"):
				cmapSubtable.nGroups = 0
			if hasattr(cmapSubtable, "length"):
				cmapSubtable.length = 0		
	if ('OS/2' in onlyTables) and supressTTFDiffs:
		os2Table = ttf["OS/2"]
		
	if onlyTables:
		ttf.saveXML(output,
			tables=onlyTables,
			skipTables=options.skipTables, 
			splitTables=options.splitTables,
			disassembleInstructions=options.disassembleInstructions)

	if 0:
		writer = None
		specialTables = []
		path, ext = os.path.splitext(output)
		for tag in [ "GSUB", "GPOS"]:
			if tag in specialTables:
				if not (tag in options.skipTables):
					if options.splitTables:
						outputPath = "%s.%s%s" % (path,tagToIdentifier(tag),ext)
					else:
						outputPath = output
					writer = TTXNWriter(outputPath, splitTables=options.splitTables)
					print "Dumping '%s' table ..." % (tag)
					writer._writeraw("<%s>" % (tag))
					writer.newline()
					writer.indent()
					dumpOTLAsFeatureFile(tag, writer, ttf)
					writer.newline()
					writer.dedent()
					writer._writeraw("</%s>" % (tag))
					writer.newline()
					writer.close()
	
		for tag in [ "CFF ", "glyf"]:
			if tag in specialTables:
				if not ( tag in options.skipTables):
					if options.splitTables:
						outputPath = "%s.%s%s" % (path,tagToIdentifier(tag),ext)
					else:
						outputPath = output
					writer = TTXNWriter(outputPath, splitTables=options.splitTables)
					print "Dumping '%s' table ..." % (tag)
					writer._writeraw("<%s>" % (tag))
					writer.newline()
					writer.indent()
					dumpFont(writer, input, supressHints)
					writer.newline()
					writer.dedent()
					writer._writeraw("</%s>" % (tag))
					writer.newline()
					writer.close()
	ttf.close()
	return ttf



def run(args):
	if ("-h" in args) or ("-u" in args):
		print __help__
		sys.exit(0)
	
	if "-a" not in args:
		args.insert(0, "-a") # allow virtual GIDS.

	if "-nh" in args:
		args.remove("-nh")
		supressHints = 1
	else:
		supressHints = 0

	if "-se" in args:
		args.remove("-se")
		showExtensionFlag = 1
	else:
		showExtensionFlag = 0
		
	if "-nv" in args:
		args.remove("-nv")
		supressVersions = 1
	else:
		supressVersions = 0
		
	if "-supressTTFDiffs" in args:
		args.remove("-supressTTFDiffs")
		supressTTFDiffs = 1
	else:
		supressTTFDiffs = 0

	try:
		jobs, options = ttx.parseOptions(args)
	except getopt.GetoptError as e:
		print "ERROR:", e
		sys.exit(2)

	if not jobs:
		return
		
	action, input, output = jobs[0]

	if action != ttx.ttDump:
		print sys.argv[0],"can only dump font files."
	try:
		ttf = ttnDump(input, output, options, showExtensionFlag, supressHints, supressVersions, supressTTFDiffs)
		
	except KeyboardInterrupt:
		print "(Cancelled.)"
	
	

if __name__ == "__main__":
	run(sys.argv[1:])
