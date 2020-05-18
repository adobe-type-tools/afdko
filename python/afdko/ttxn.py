# Copyright 2014 Adobe. All Rights Reserved.

"""
How to build a feature-file format.
1) Traverse script->language->feature tree, building up list as we go.

These are all expected to be in alphabetic order in the font.

Add each  feature entry as FeatDict[tag][lang_sys].append(index]), and as
FeatDict[index] .append(lang_sys)

If there is no DFLT script, then step through all entries of  FeatDict[index].
Add FeatDict[tag]["DFLT_dflt"].append(index) for all FeatDict[index] where the
number of lang_sys entries is the same as the number of lang_sys values.

Write out the language systems.

Collect and write the class defs.
For each lookup in lookup list: for each class def in the lookup:
    add entry to ClassesByNameDict[<glyph list as tuple>].append(lookup index,
        subtable_index, class index)

Get list of keys for ClassesByNameDict. Sort. MakeClassNameDict.
For each  keys for ClassesByNameDict:
    make class name from <firstglyph_lastglyph>.
    while class name is in ClassesByNameDict:
        find glyph in current class that is not in previous class, and
            make name firstglyph_diff-glyph_last-glyph
    add entry to ClassesByLookup[lookup index, subtable_index, class index)] =
        class name.

Same for mark classes.

Write out class defs.

Write out the feature list.
Make lookupDict = {}
Get feat tag list. For each tag:
    note if DFLT dflt lang sys has any lookups.
    for each lang_sys ( by definition, starts with DFLT, then the rest in
      alpha order).
        If script is DFLT:
            don't write script/lang
            build list of lookups for DFLT script.
        else:
            get union of lookup indicies in all feat indicies
            Remove all the default lookups from the lookup list:
            if no lookups left, continue
            if there was no intersection with default lookups, set exclude
                keyword
            if the script is same as last one, write language only,
            else write script and language.

        if lookupIndex is in lookupDict:
            just write lookup references
        else:
            write lookup definition with name "feat_script_lang_new_index"


"""

__help__ = """
ttxn v1.21.2 Jul 15 2019

Based on the ttx tool, with the same options, except that it is limited to
dumping, and cannot compile. Makes a normalized dump of the font, or of
selected tables.

Adds additional options:
    -nh : suppresses hints in the glyph outline dump.
    -nv : suppress version info in output: head.fontRevision and name ID 3.
    -se : show if lookup is wrapped in an Extension lookup type;
            by default, this information is suppressed.

"""

import sys
from fontTools import ttx
from fontTools.ttLib import TTFont, tagToXML, TTLibError
from fontTools.ttLib.tables.DefaultTable import DefaultTable
from fontTools.misc.loggingTools import Timer
import copy
import subprocess
import re
import collections
import textwrap
import platform
import getopt
import logging

from afdko.fdkutils import get_temp_file_path

__version__ = "1.21.2"

log = logging.getLogger('fontTools.ttx')


curSystem = platform.system()

TX_TOOL = 'tx'


INDENT = "  "
ChainINDENT = INDENT + "C "
MAX_LINE_LENGTH = 80
gtextWrapper = textwrap.TextWrapper(width=MAX_LINE_LENGTH,
                                    subsequent_indent=INDENT + INDENT)

NULL_VALUE_RECORD = "<0 0 0 0>"


def getValueRecord(a, b, c, d):
    return "<%s %s %s %s>" % (a, b, c, d)


def getAnchorString(anchorTable):
    if not anchorTable:
        return "<anchor NULL>"

    tokenList = ["<anchor", "%d" % anchorTable.XCoordinate,
                 "%d" % anchorTable.YCoordinate]
    if anchorTable.Format == 2:
        tokenList.append("%d" % anchorTable.AnchorPoint)
    elif anchorTable.Format == 3:
        tokenList.append("%d" % anchorTable.XDeviceTable)
        tokenList.append("%d" % anchorTable.YDeviceTable)
    return " ".join(tokenList) + ">"


class ClassRecord:

    def __init__(self, lookupIndex, subtableIndex, classIndex, side="",
                 anchor=None):
        self.lookupIndex = lookupIndex
        self.subtableIndex = subtableIndex
        self.classIndex = classIndex
        self.side = side  # None if not a kern pair, else "Left" or "Right"
        self.anchor = anchor

    def __repr__(self):
        return "(lookup %s subtable %s class %s side %s anchor %s)" % (
            self.lookupIndex, self.subtableIndex, self.classIndex, self.side,
            self.anchor)


def addClassDef(otlConv, classDefs, coverage, side=None, anchor=None):
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
        if 0 not in classDict:
            classZeroList = sorted(glyphDict.keys())
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
        key = tuple(sorted(nameList))
        classRec = ClassRecord(otlConv.curLookupIndex,
                               otlConv.curSubTableIndex, classIndex, side)
        otlConv.classesByNameList[key].append(classRec)


def AddMarkClassDef(otlConv, markCoverage, markArray, tag):
    markGlyphList = markCoverage.glyphs
    markClass = collections.defaultdict(list)

    for m, markGlyph in enumerate(markGlyphList):
        markRec = markArray.MarkRecord[m]
        markClass[markRec.Class].append(
            (getAnchorString(markRec.MarkAnchor), markGlyph))

    # Get the mark class names from the global dict
    for c, valueList in markClass.items():
        anchorDict = collections.defaultdict(list)
        for anchor, glyph in valueList:
            anchorDict[anchor].append(glyph)
        defList = []
        for anchor, glyphList in anchorDict.items():
            defList.append((tuple(sorted(glyphList)), anchor))
        classRec = ClassRecord(otlConv.curLookupIndex,
                               otlConv.curSubTableIndex, c, tag)
        otlConv.markClassesByDefList[tuple(sorted(defList))].append(classRec)


def classPairGPOS(subtable, otlConv=None):
    addClassDef(otlConv, subtable.ClassDef1.classDefs, subtable.Coverage,
                otlConv.leftSideTag)
    addClassDef(otlConv, subtable.ClassDef2.classDefs, None,
                otlConv.rightSideTag)


def markClassPOS(subtable, otlConv=None):
    AddMarkClassDef(otlConv, subtable.MarkCoverage, subtable.MarkArray,
                    otlConv.markTag)


def markLigClassPOS(subtable, otlConv=None):
    AddMarkClassDef(otlConv, subtable.MarkCoverage, subtable.MarkArray,
                    otlConv.markTag)


def markMarkClassPOS(subtable, otlConv=None):
    AddMarkClassDef(otlConv, subtable.Mark1Coverage, subtable.Mark1Array,
                    otlConv.mark1Tag)


def classContext(subtable, otlConv):
    addClassDef(otlConv, subtable.ClassDef.classDefs, None)


def classChainContext(subtable, otlConv):
    for class_def_name in ('BacktrackClassDef', 'InputClassDef',
                           'LookAheadClassDef'):
        class_def = getattr(subtable, class_def_name)
        if class_def:
            addClassDef(otlConv, class_def.classDefs, None, class_def_name)


def classExt(subtable, otlConv):
    handler = otlConv.classHandlers.get(
        (subtable.ExtensionLookupType, subtable.ExtSubTable.Format), None)
    if handler:
        handler(subtable.ExtSubTable, otlConv)


class ContextRecord:
    def __init__(self, inputList, sequenceIndex):
        self.inputList = inputList
        self.sequenceIndex = sequenceIndex
        self.glyphsUsed = 0
        self.result = ""


def checkGlyphInSequence(glyphName, contextSequence, i):
    retVal = 0
    try:
        input_seq = contextSequence[i]
        if isinstance(input_seq, list):
            if glyphName in input_seq:
                retVal = 1
        elif isinstance(input_seq, str):
            if glyphName == input_seq:
                retVal = 1
    except IndexError:
        pass

    return retVal


def ruleSinglePos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    if subtable.Format == 1:
        valueString = getValRec(subtable.ValueFormat, subtable.Value)
        if valueString and valueString != "0" and (
                valueString != NULL_VALUE_RECORD):
            for tglyph in subtable.Coverage.glyphs:
                rule = note = None
                if context and (not checkGlyphInSequence(
                        tglyph, inputSeqList, 0)):
                    note = " # Note! Not in input sequence"
                else:
                    note = ""
                rule = "%spos %s %s;%s" % (indent, tglyph, valueString, note)
                if context and note:
                    noMatchRules.append(rule)
                else:
                    rules.append(rule)

    elif subtable.Format == 2:
        for i, subtable_value in enumerate(subtable.Value):
            rule = note = None
            tglyph = subtable.Coverage.glyphs[i]
            valueString = getValRec(subtable.ValueFormat, subtable_value)
            if valueString and valueString != "0" and (
                    valueString != NULL_VALUE_RECORD):
                if context and (tglyph not in inputSeqList):
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
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def rulePairPos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    if subtable.Format == 1:
        firstGlyphList = subtable.Coverage.glyphs
        for i, pairSet in enumerate(subtable.PairSet):
            g1 = firstGlyphList[i]
            for pairValueRec in pairSet.PairValueRecord:
                rule = note = None
                g2 = pairValueRec.SecondGlyph
                note = ""
                if context:
                    note = " # Note! Not in input sequence"
                if pairValueRec.Value1:
                    valueString = getValRec(subtable.ValueFormat1,
                                            pairValueRec.Value1)
                    if valueString and valueString != "0" and (
                            valueString != NULL_VALUE_RECORD):
                        rule = "%spos %s %s %s%s;" % (indent, g1, g2,
                                                      valueString, note)
                if pairValueRec.Value2:
                    valueString = getValRec(subtable.ValueFormat1,
                                            pairValueRec.Value2)
                    if valueString and valueString != "0" and (
                            valueString != NULL_VALUE_RECORD):
                        note = ("%s # Warning: non-zero pos value record for "
                                "second glyph" % note)
                        rule = "%spos %s %s %s;%s" % (indent, g1, g2,
                                                      valueString, note)
                if rule:
                    if context and note:
                        noMatchRules.append(rule)
                    else:
                        rules.append(rule)

    elif subtable.Format == 2:
        for i, classRec1 in enumerate(subtable.Class1Record):
            # if this class reference exists it has to be in classesByLookup.
            g1 = otlConv.classesByLookup[
                otlConv.curLookupIndex, otlConv.curSubTableIndex, i,
                otlConv.leftSideTag]
            for j, classRec2 in enumerate(classRec1.Class2Record):
                rule = note = None
                g2 = otlConv.classesByLookup[
                    otlConv.curLookupIndex, otlConv.curSubTableIndex, j,
                    otlConv.rightSideTag]
                if context:
                    if checkGlyphInSequence(g1, inputSeqList, 0) and (
                            checkGlyphInSequence(g2, inputSeqList, 1)):
                        note = ""
                    else:
                        note = " # Note! Not in input sequence"
                else:
                    note = ""
                if classRec2.Value1:
                    valueString = getValRec(subtable.ValueFormat1,
                                            classRec2.Value1)
                    if valueString and valueString != "0":
                        rule = "%spos %s %s %s;%s" % (indent, g1, g2,
                                                      valueString, note)

                if classRec2.Value2:
                    valueString = getValRec(subtable.ValueFormat1,
                                            classRec2.Value2)
                    if valueString and valueString != "0":
                        note = ("%s # Warning: non-zero pos value record for "
                                "second glyph" % note)
                        rule = "%spos %s %s %s;%s" % (indent, g1, g2,
                                                      valueString, note)
                if rule:
                    if context and note:
                        noMatchRules.append(rule)
                    else:
                        rules.append(rule)
    else:
        raise AttributeError("Unknown Pair Pos format")

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleCursivePos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    cursiveGlyphList = subtable.Coverage.glyphs
    for i, eRec in enumerate(subtable.EntryExitRecord):
        rule = note = None
        tglyph = cursiveGlyphList[i]
        if context and (not checkGlyphInSequence(tglyph, inputSeqList, 0)):
            note = " # Note! Not in input sequence"
        else:
            note = ""
        anchor1 = getAnchorString(eRec.EntryAnchor)
        anchor2 = getAnchorString(eRec.ExitAnchor)
        rule = "%spos cursive %s %s %s;%s" % (indent, tglyph, anchor1, anchor2,
                                              note)
        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleMarkBasePos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    subsequentIndent = indent + INDENT

    markGlyphList = subtable.MarkCoverage.glyphs
    baseGlyphList = subtable.BaseCoverage.glyphs
    markClass = collections.defaultdict(list)
    classCount = len(set(v.Class for v in subtable.MarkArray.MarkRecord))

    for m, markGlyph in enumerate(markGlyphList):
        markRec = subtable.MarkArray.MarkRecord[m]
        markClass[markRec.Class].append((getAnchorString(markRec.MarkAnchor),
                                         markGlyph))

    # Get the mark class names from the global dict
    markClassNameList = []
    for c in markClass.keys():
        markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex,
                                                    otlConv.curSubTableIndex,
                                                    c, otlConv.markTag]
        markClassNameList.append(markClassName)

    # build a dict of [common set of anchors] -> glyph list.
    baseAnchorSets = collections.defaultdict(list)
    for b, baseGlyph in enumerate(baseGlyphList):
        baseRec = subtable.BaseArray.BaseRecord[b]
        anchorList = []
        for c in range(classCount):
            anchorList.append(getAnchorString(baseRec.BaseAnchor[c]))
        anchorKey = tuple(anchorList)
        baseAnchorSets[anchorKey].append(baseGlyph)

    for anchorKey, glyphList in baseAnchorSets.items():
        note = ""
        glyphList.sort()
        if context:
            if isinstance(inputSeqList[0], list):
                inputList = inputSeqList[0]
            else:
                inputList = [inputSeqList[0]]
            overlapList = set(glyphList).intersection(inputList)
            if not overlapList:
                note = " # Note! Not in input sequence"
                glyphTxt = " ".join(glyphList)
            else:
                glyphTxt = " ".join(sorted(overlapList))
        else:
            glyphTxt = " ".join(glyphList)

        pos_ind = '\n' + INDENT * 4
        rule = ["pos base [%s]" % pos_ind.join(gtextWrapper.wrap(glyphTxt))]

        for cl in range(classCount):
            rule.append(" %s mark %s" % (anchorKey[cl], markClassNameList[cl]))
            if (cl + 1) < classCount:  # if it is not the last one
                rule.append("\n%s" % subsequentIndent)

        rule.append(";")

        if note:
            rule.append(note)
        rule = "".join(rule)

        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleMarkLigPos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    markGlyphList = subtable.MarkCoverage.glyphs
    ligGlyphList = subtable.LigatureCoverage.glyphs
    markClass = collections.defaultdict(list)
    classCount = len(set(v.Class for v in subtable.MarkArray.MarkRecord))

    for m, markGlyph in enumerate(markGlyphList):
        markRec = subtable.MarkArray.MarkRecord[m]
        markClass[markRec.Class].append((getAnchorString(markRec.MarkAnchor),
                                         markGlyph))

    # Get the mark class names from the global dict
    markClassNameList = []
    for c in markClass.keys():
        markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex,
                                                    otlConv.curSubTableIndex,
                                                    c, otlConv.markTag]
        markClassNameList.append(markClassName)

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    subsequentIndent = indent + INDENT

    for lidx, ligAttach in enumerate(subtable.LigatureArray.LigatureAttach):
        ligGlyph = ligGlyphList[lidx]
        tokenList = ["pos ligature ", ligGlyph]
        if context and (not checkGlyphInSequence(ligGlyph, inputSeqList, 0)):
            note = " # Note! Not in input sequence"
        else:
            note = ""

        for cmpIndex, componentRec in enumerate(ligAttach.ComponentRecord):
            if cmpIndex > 0:
                tokenList.append("\n%sligComponent" % (subsequentIndent * 5))
            for cl in range(classCount):
                tokenList.append(" %s mark %s" % (
                    getAnchorString(componentRec.LigatureAnchor[cl]),
                    markClassNameList[cl]))
                if (cl + 1) < classCount:
                    tokenList.append("\n%s" % subsequentIndent)

        tokenList.append(";")

        if note:
            tokenList.append(note)

        rule = "".join(tokenList)

        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleMarkMarkPos(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    subsequentIndent = indent + INDENT

    markGlyphList = subtable.Mark1Coverage.glyphs
    mark2GlyphList = subtable.Mark2Coverage.glyphs
    markClass = collections.defaultdict(list)
    classCount = len(set(v.Class for v in subtable.Mark1Array.MarkRecord))

    for m, markGlyph in enumerate(markGlyphList):
        markRec = subtable.Mark1Array.MarkRecord[m]
        markClass[markRec.Class].append((getAnchorString(markRec.MarkAnchor),
                                        markGlyph))

    # Get the mark class names from the global dict
    markClassNameList = []
    for c in markClass.keys():
        markClassName = otlConv.markClassesByLookup[otlConv.curLookupIndex,
                                                    otlConv.curSubTableIndex,
                                                    c, otlConv.mark1Tag]
        markClassNameList.append(markClassName)

    mark2AnchorSets = collections.defaultdict(list)
    for b, mark2Glyph in enumerate(mark2GlyphList):
        mark2Rec = subtable.Mark2Array.Mark2Record[b]
        anchorList = []
        for c in range(classCount):
            anchorList.append(getAnchorString(mark2Rec.Mark2Anchor[c]))
        anchorKey = tuple(anchorList)
        mark2AnchorSets[anchorKey].append(mark2Glyph)

    for anchorKey, glyphList in mark2AnchorSets.items():
        note = None
        # This sort will lead the rules to be sorted by base glyph names.
        glyphList.sort()
        if context:
            if isinstance(inputSeqList[0], list):
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
            rule.append(" %s mark %s" % (anchorKey[cl], markClassNameList[cl]))
            if (cl + 1) < classCount:  # if it is not the last one
                rule.append("\n%s" % subsequentIndent)
        rule.append(";")
        if note:
            rule.append(note)
        rule = "".join(rule)
        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleContextPOS(subtable, otlConv, context=None):
    chainRules = []
    # pLookupList = otlConv.table.LookupList.Lookup

    # ruleContextPOS 7-3
    # XXX support is incomplete
    if subtable.Format == 3:
        log.warn('Support for GPOS LookupType 7, Format 3 is incomplete')
        inputList = []
        inputList2 = []
        for input_coverage in subtable.Coverage:
            glyphList = sorted(input_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            inputList2.append(glyphList)
            inputList.append("[" + " ".join(glyphList) + "]")
        inputTxt = "' ".join(inputList) + "'"
        rule = "pos %s;" % inputTxt

        posRules = []
        # for subRec in subtable.PosLookupRecord:
        #     lookup = pLookupList[subRec.LookupListIndex]
        #     lookupType = lookup.LookupType
        #     curLI = otlConv.curLookupIndex
        #     otlConv.curLookupIndex = subRec.LookupListIndex
        #     handler = otlConv.ruleHandlers[lookupType]
        #     contextRec = ContextRecord(inputList2, subRec.SequenceIndex)

        #     for si, sub_table in enumerate(lookup.SubTable):
        #         curSI = otlConv.curSubTableIndex
        #         otlConv.curSubTableIndex = si
        #         subtablerules = handler(sub_table, otlConv, contextRec)
        #         otlConv.curSubTableIndex = curSI
        #         posRules.extend(subtablerules)

        #     otlConv.curLookupIndex = curLI

        chainRules.append([rule, posRules])

    # ruleContextPOS 7-2
    # XXX support is incomplete
    elif subtable.Format == 2:
        log.warn('Support for GPOS LookupType 7, Format 2 is incomplete')
        for i, ctxClassSet in enumerate(subtable.PosClassSet):
            if not ctxClassSet:
                continue

            for ctxClassRule in ctxClassSet.PosClassRule:
                className = otlConv.classesByLookup[
                    otlConv.curLookupIndex, otlConv.curSubTableIndex, i, None]
                inputList = [className]
                inputList2 = [otlConv.classesByClassName[className]]

                for classIndex in ctxClassRule.Class:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, None]
                    inputList.append(className)
                    inputList2.append(otlConv.classesByClassName[className])
                inputTxt = "' ".join(inputList) + "'"
                rule = "pos %s;" % inputTxt

                posRules = []
                # for subsRec in ctxClassRule.PosLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(
                #         inputList, subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         posRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, posRules])

    # ruleContextPOS 7-1
    # XXX support is incomplete
    elif subtable.Format == 1:
        log.warn('Support for GPOS LookupType 7, Format 1 is incomplete')
        firstGlyphList = subtable.Coverage.glyphs
        # for each glyph in the coverage table
        for ri, subRuleSet in enumerate(subtable.PosRuleSet):
            firstGlyph = firstGlyphList[ri]

            for ctxRuleRec in subRuleSet.PosRule:
                inputList = [firstGlyph] + ctxRuleRec.Input
                inputTxt = "' ".join(inputList) + "'"
                rule = "pos %s" % inputTxt

                posRules = []
                # for subsRec in ctxRuleRec.PosLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(inputList,
                #                                subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         posRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, posRules])
    else:
        raise AttributeError("Unknown Context Pos format %s" %
                             subtable.Format)

    rules = []
    for chainRule, posRules in sorted(chainRules):
        rules.append(chainRule)
        rules.extend(posRules)
    return rules


def ruleChainContextPOS(subtable, otlConv, context=None):
    chainRules = []
    # pLookupList = otlConv.table.LookupList.Lookup

    # ruleChainContextPOS 8-3
    # XXX support is incomplete
    if subtable.Format == 3:
        log.warn('Support for GPOS LookupType 8, Format 3 is incomplete')
        backtrackList = []
        for backtrack_coverage in subtable.BacktrackCoverage:
            glyphList = sorted(backtrack_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            backtrackList.append("[" + " ".join(glyphList) + "]")
        backtrackList.reverse()
        backTxt = " ".join(backtrackList)

        inputList = []
        inputList2 = []
        for input_coverage in subtable.InputCoverage:
            glyphList = sorted(input_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            inputList2.append(glyphList)
            inputList.append("[" + " ".join(glyphList) + "]")
        inputTxt = "' ".join(inputList) + "'"

        lookAheadList = []
        for lookahead_coverage in subtable.LookAheadCoverage:
            glyphList = sorted(lookahead_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            lookAheadList.append("[" + " ".join(glyphList) + "]")
        lookAheadTxt = " ".join(lookAheadList)
        rule = "pos %s %s %s;" % (backTxt, inputTxt, lookAheadTxt)

        posRules = []
        # for subRec in subtable.PosLookupRecord:
        #     lookup = pLookupList[subRec.LookupListIndex]
        #     lookupType = lookup.LookupType
        #     curLI = otlConv.curLookupIndex
        #     otlConv.curLookupIndex = subRec.LookupListIndex
        #     handler = otlConv.ruleHandlers[lookupType]
        #     contextRec = ContextRecord(inputList2, subRec.SequenceIndex)

        #     for si, sub_table in enumerate(lookup.SubTable):
        #         curSI = otlConv.curSubTableIndex
        #         otlConv.curSubTableIndex = si
        #         subtablerules = handler(sub_table, otlConv, contextRec)
        #         otlConv.curSubTableIndex = curSI
        #         posRules.extend(subtablerules)

        #     otlConv.curLookupIndex = curLI

        chainRules.append([rule, posRules])

    # ruleChainContextPOS 8-2
    # XXX support is incomplete
    elif subtable.Format == 2:
        log.warn('Support for GPOS LookupType 8, Format 2 is incomplete')
        for i, ctxClassSet in enumerate(subtable.ChainSubClassSet):
            if not ctxClassSet:
                continue

            for ctxClassRule in ctxClassSet.ChainPosRuleSet:
                backTrackList = []
                for classIndex in ctxClassRule.Backtrack:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.backtrackTag]
                    backTrackList.append(className)
                backTxt = " ".join(backTrackList)

                className = otlConv.classesByLookup[
                    otlConv.curLookupIndex, otlConv.curSubTableIndex, i,
                    otlConv.InputTag]
                inputList = [className]
                inputList2 = [otlConv.classesByClassName[className]]

                for classIndex in ctxClassRule.Input:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.InputTag]
                    inputList.append(className)
                    inputList2.append(otlConv.classesByClassName[className])
                inputTxt = "' ".join(inputList) + "'"

                lookAheadList = []
                for classIndex in ctxClassRule.LookAhead:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.lookAheadTag]
                    lookAheadList.append(className)
                lookTxt = " ".join(lookAheadList)
                rule = "sub %s %s %s;" % (backTxt, inputTxt, lookTxt)

                posRules = []
                # for subsRec in ctxClassRule.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(inputList,
                #                                subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         posRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, posRules])

    # ruleChainContextPOS 8-1
    # XXX support is incomplete
    elif subtable.Format == 1:
        log.warn('Support for GPOS LookupType 8, Format 1 is incomplete')
        firstGlyphList = subtable.Coverage.glyphs
        # for each glyph in the coverage table
        for ri, subRuleSet in enumerate(subtable.ChainPosRuleSet):
            firstGlyph = firstGlyphList[ri]

            for ctxRuleRec in subRuleSet.ChainPosRule:
                backList = ctxRuleRec.Backtrack
                backList.reverse()
                backTxt = " ".join(backList)
                inputList = [firstGlyph] + ctxRuleRec.Input
                inputTxt = "' ".join(inputList) + "'"
                lookAheadTxt = " ".join(ctxRuleRec.LookAhead)
                rule = "sub %s %s %s" % (backTxt, inputTxt, lookAheadTxt)

                posRules = []
                # for subsRec in ctxRuleRec.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(inputList,
                #                                subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         posRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, posRules])
    else:
        raise AttributeError("Unknown Chain Context Pos format %s" % (
            subtable.Format))

    rules = []
    for chainRule, posRules in sorted(chainRules):
        rules.append(chainRule)
        rules.extend(posRules)
    return rules


def ruleExt(subtable, otlConv, context=None):
    handler = otlConv.ruleHandlers[subtable.ExtensionLookupType]
    rules = handler(subtable.ExtSubTable, otlConv, context)
    return rules


def ruleSingleSub(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    for g1, g2 in subtable.mapping.items():
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
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleMultipleSub(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    for g1, substGlyphList in subtable.mapping.items():
        rule = note = None
        if context and (not checkGlyphInSequence(g1, inputSeqList, 0)):
            note = " # Note! Not in input sequence"
        else:
            note = ""
        subTxt = " ".join(substGlyphList)
        rule = "%ssub %s by [%s];%s" % (indent, g1, subTxt, note)
        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleAltSub(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context
    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    for g1, alts in subtable.alternates.items():
        rule = note = None
        if context and (not checkGlyphInSequence(g1, inputSeqList, 0)):
            note = " # Note! Not in input sequence"
        else:
            note = ""
        altText = " ".join(sorted(alts))
        rule = "%ssub %s from [%s];%s" % (indent, g1, altText, note)
        if context and note:
            noMatchRules.append(rule)
        else:
            rules.append(rule)

    if rules:
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleLigatureSub(subtable, otlConv, context=None):
    rules = []
    noMatchRules = []  # contains the rules that don't match the context

    if context:
        indent = ChainINDENT
        inputSeqList = context.inputList[context.sequenceIndex:]
    else:
        indent = ""
        inputSeqList = []

    for item in subtable.ligatures.items():
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

                    # make sure everything else is also present with
                    # the same index.
                    for i, gname in enumerate(lig.Component):
                        if not checkGlyphInSequence(gname, inputSeqList,
                                                    i + 1):
                            foundAll = 0
                            missing.append(gname)

                    if not foundAll:
                        note = (" # Note! lig components %s are not in input "
                                "sequence with same index."
                                % " ".join(missing))
                    else:
                        note = ""
                except ValueError:
                    note = (" # Note! first glyph %s is not in input "
                            "sequence." % firstGlyph)
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
        return sorted(rules)
    else:
        return sorted(noMatchRules)


def ruleContextSUB(subtable, otlConv, context=None):
    chainRules = []
    # pLookupList = otlConv.table.LookupList.Lookup

    # ruleContextSUB 5-3
    # XXX support is incomplete
    if subtable.Format == 3:
        log.warn('Support for GSUB LookupType 5, Format 3 is incomplete')
        inputList = []
        inputList2 = []

        for input_coverage in subtable.Coverage:
            glyphList = sorted(input_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            inputList2.append(glyphList)
            inputList.append("[" + " ".join(glyphList) + "]'")
        inputTxt = "' ".join(inputList) + "'"
        rule = "sub %s;" % inputTxt

        subRules = []
        # for subRec in subtable.SubstLookupRecord:
        #     lookup = pLookupList[subRec.LookupListIndex]
        #     lookupType = lookup.LookupType
        #     curLI = otlConv.curLookupIndex
        #     otlConv.curLookupIndex = subRec.LookupListIndex
        #     handler = otlConv.ruleHandlers[lookupType]
        #     contextRec = ContextRecord(inputList2, subRec.SequenceIndex)

        #     for si, sub_table in enumerate(lookup.SubTable):
        #         curSI = otlConv.curSubTableIndex
        #         otlConv.curSubTableIndex = si
        #         subtablerules = handler(sub_table, otlConv, contextRec)
        #         otlConv.curSubTableIndex = curSI
        #         subRules.extend(subtablerules)

        #     otlConv.curLookupIndex = curLI

        chainRules.append([rule, subRules])

    # ruleContextSUB 5-2
    # XXX support is incomplete
    elif subtable.Format == 2:
        log.warn('Support for GSUB LookupType 5, Format 2 is incomplete')
        for i, ctxClassSet in enumerate(subtable.SubClassSet):
            if not ctxClassSet:
                continue

            for ctxClassRule in ctxClassSet.SubClassRule:
                className = otlConv.classesByLookup[
                    otlConv.curLookupIndex, otlConv.curSubTableIndex, i, None]
                inputList = [className]
                inputList2 = [otlConv.classesByClassName[className]]

                for classIndex in ctxClassRule.Class:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, None]
                    inputList.append(className)
                    inputList2.append(otlConv.classesByClassName[className])
                inputTxt = "' ".join(inputList) + "'"
                rule = "sub %s;" % inputTxt

                subRules = []
                # for subsRec in ctxClassRule.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(
                #         inputList, subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         subRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, subRules])

    # ruleContextSUB 5-1
    # XXX support is incomplete
    elif subtable.Format == 1:
        log.warn('Support for GSUB LookupType 5, Format 1 is incomplete')
        firstGlyphList = subtable.Coverage.glyphs
        # for each glyph in the coverage table
        for ri, subRuleSet in enumerate(subtable.SubRuleSet):
            firstGlyph = firstGlyphList[ri]

            for ctxRuleRec in subRuleSet.SubRule:
                inputList = [firstGlyph] + ctxRuleRec.Input
                inputTxt = "' ".join(inputList) + "'"
                rule = "sub %s" % inputTxt

                subRules = []
                # for subsRec in ctxRuleRec.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(inputList,
                #                                subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         subRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, subRules])
    else:
        raise AttributeError("Unknown Context Sub format %s" % (
            subtable.Format))

    rules = []
    for chainRule, subRules in sorted(chainRules):
        rules.append(chainRule)
        rules.extend(subRules)
    return rules


def wrap_statement(token_str):
    """
    Wraps a long string of space-separated tokens
    or a list of tokens.
    """
    if isinstance(token_str, list):
        token_str = ' '.join(token_str)
    wrap_ind = '\n' + INDENT * 4
    return wrap_ind.join(gtextWrapper.wrap(token_str))


def ruleChainContextSUB(subtable, otlConv, context=None):
    chainRules = []
    # pLookupList = otlConv.table.LookupList.Lookup

    # ruleChainContextSUB 6-3
    # XXX support is incomplete
    if subtable.Format == 3:
        log.warn('Support for GSUB LookupType 6, Format 3 is incomplete')
        backtrackList = []
        for backtrack_coverage in subtable.BacktrackCoverage:
            glyphList = sorted(backtrack_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]

            backtrackList.append("[" + " ".join(glyphList) + "]")
        backtrackList.reverse()
        backTxt = " ".join(backtrackList)

        inputList = []
        inputList2 = []
        for input_coverage in subtable.InputCoverage:
            glyphList = sorted(input_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            inputList2.append(glyphList)
            inputList.append("[" + " ".join(glyphList) + "]'")
        inputTxt = " ".join(inputList)

        lookAheadList = []
        for lookahead_coverage in subtable.LookAheadCoverage:
            glyphList = sorted(lookahead_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            lookAheadList.append("[" + " ".join(glyphList) + "]")
        lookAheadTxt = " ".join(lookAheadList)
        rule = wrap_statement(
            "sub %s %s %s;" % (backTxt, inputTxt, lookAheadTxt))

        subRules = []
        # for subsRec in subtable.SubstLookupRecord:
        #     lookup = pLookupList[subsRec.LookupListIndex]
        #     lookupType = lookup.LookupType
        #     curLI = otlConv.curLookupIndex
        #     otlConv.curLookupIndex = subsRec.LookupListIndex
        #     handler = otlConv.ruleHandlers[lookupType]
        #     contextRec = ContextRecord(inputList2, subsRec.SequenceIndex)

        #     for si, sub_table in enumerate(lookup.SubTable):
        #         curSI = otlConv.curSubTableIndex
        #         otlConv.curSubTableIndex = si
        #         subtablerules = handler(sub_table, otlConv, contextRec)
        #         otlConv.curSubTableIndex = curSI
        #         subRules.extend(subtablerules)

        #     otlConv.curLookupIndex = curLI

        chainRules.append([rule, subRules])

    # ruleChainContextSUB 6-2
    # XXX support is incomplete
    elif subtable.Format == 2:
        log.warn('Support for GSUB LookupType 6, Format 2 is incomplete')
        for i, ctxClassSet in enumerate(subtable.ChainSubClassSet):
            if not ctxClassSet:
                continue

            for ctxClassRule in ctxClassSet.ChainSubClassRule:
                backTrackList = []

                for classIndex in ctxClassRule.Backtrack:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.backtrackTag]
                    backTrackList.append(className)
                backTxt = " ".join(backTrackList)

                className = otlConv.classesByLookup[
                    otlConv.curLookupIndex, otlConv.curSubTableIndex, i,
                    otlConv.InputTag]
                inputList = [className]
                inputList2 = [otlConv.classesByClassName[className]]

                for classIndex in ctxClassRule.Input:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.InputTag]
                    inputList.append(className)
                    inputList2.append(otlConv.classesByClassName[className])
                inputTxt = " ".join(inputList)

                lookAheadList = []
                for classIndex in ctxClassRule.LookAhead:
                    className = otlConv.classesByLookup[
                        otlConv.curLookupIndex, otlConv.curSubTableIndex,
                        classIndex, otlConv.lookAheadTag]
                    lookAheadList.append(className)
                lookTxt = " ".join(lookAheadList)
                rule = "sub %s %s' %s;" % (backTxt, inputTxt, lookTxt)

                subRules = []
                # for subsRec in ctxClassRule.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(
                #         inputList, subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         subRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, subRules])

    # ruleChainContextSUB 6-1
    # XXX support is incomplete
    elif subtable.Format == 1:
        log.warn('Support for GSUB LookupType 6, Format 1 is incomplete')
        firstGlyphList = subtable.Coverage.glyphs
        # for each glyph in the coverage table
        for ri, subRuleSet in enumerate(subtable.ChainSubRuleSet):
            firstGlyph = firstGlyphList[ri]

            for ctxRuleRec in subRuleSet.ChainSubRule:
                backList = ctxRuleRec.Backtrack
                backList.reverse()
                backTxt = " ".join(backList)
                inputList = [firstGlyph] + ctxRuleRec.Input
                inputTxt = " ".join(inputList)
                lookAheadTxt = " ".join(ctxRuleRec.LookAhead)
                rule = "sub %s %s' %s by " % (backTxt, inputTxt, lookAheadTxt)

                subRules = []
                # for subsRec in ctxRuleRec.SubstLookupRecord:
                #     lookup = pLookupList[subsRec.LookupListIndex]
                #     lookupType = lookup.LookupType
                #     curLI = otlConv.curLookupIndex
                #     otlConv.curLookupIndex = subsRec.LookupListIndex
                #     handler = otlConv.ruleHandlers[lookupType]
                #     contextRec = ContextRecord(inputList,
                #                                subsRec.SequenceIndex)

                #     for si, sub_table in enumerate(lookup.SubTable):
                #         curSI = otlConv.curSubTableIndex
                #         otlConv.curSubTableIndex = si
                #         subtablerules = handler(
                #             sub_table, otlConv, contextRec)
                #         otlConv.curSubTableIndex = curSI
                #         subRules.extend(subtablerules)

                #     otlConv.curLookupIndex = curLI

                chainRules.append([rule, subRules])
    else:
        raise AttributeError("Unknown Chain Context Sub format %s" % (
            subtable.Format))

    rules = []
    for chainRule, subRules in sorted(chainRules):
        rules.append(chainRule)
        rules.extend(subRules)
    return rules


def ruleReverseChainSub(subtable, otlConv, context=None):
    rules = []
    if subtable.Format == 1:
        backtrackList = []
        for backtrack_coverage in subtable.BacktrackCoverage:
            glyphList = sorted(backtrack_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            backtrackTxt = "[ %s ] " % " ".join(glyphList)
            backtrackList.append(backtrackTxt)
        backtrackList.reverse()
        backTxt = " ".join(backtrackList)

        glyphList = subtable.Coverage.glyphs
        # Since the substitute list is in the same order as the
        # coverage table, I need to sort the substitution array
        # in the same order as the input list.

        glyphList = sorted([[glyph, i] for i, glyph in enumerate(glyphList)])
        substituteList = []
        if subtable.Substitute:
            for entry in glyphList:
                substituteList.append(subtable.Substitute[entry[1]])

        for i in reversed(range(1, len(glyphList))):
            if glyphList[i - 1] == glyphList[i]:
                del glyphList[i]
                del substituteList[i]

        glyphList = [entry[0] for entry in glyphList]

        inputTxt = "[ %s ]" % " ".join(glyphList)

        lookAheadList = []
        for lookahead_coverage in subtable.LookAheadCoverage:
            glyphList = sorted(lookahead_coverage.glyphs)
            for i in reversed(range(1, len(glyphList))):
                if glyphList[i - 1] == glyphList[i]:
                    del glyphList[i]
            lookAheadTxt = " [ %s ]" % " ".join(glyphList)
            lookAheadList.append(lookAheadTxt)
        lookAheadTxt = " ".join(lookAheadList)

        if subtable.Substitute:
            replText = " by [ %s ]" % (" ".join(substituteList))
        else:
            replText = ""
        rule = "sub %s%s'%s%s;" % (backTxt, inputTxt, lookAheadTxt, replText)
        rules.append(rule)
    return rules


def getValRec(valueFormat, valueRec):
    if valueFormat == 4:
        return "%d" % valueRec.XAdvance
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

        return getValueRecord(xPos, yPos, xAdv, yAdv)


gLookupFlagMap = {
    1: "RightToLeft",
    2: "IgnoreBaseGlyphs",
    4: "IgnoreLigatures",
    8: "IgnoreMarks",
    0xFF00: "MarkAttachmentTypedef",
}

gLookupFlagKeys = sorted(gLookupFlagMap.keys())


def getLookupFlagTag(lookupFlag):
    retList = []
    for lookupVal in gLookupFlagKeys:
        if lookupVal & lookupFlag:
            retList.append(gLookupFlagMap[lookupVal])
            if lookupVal == 0xFF00:
                classIndex = (lookupVal & lookupFlag) >> 8
                retList.append("%d" % classIndex)
    return " ".join(retList)


DFLT_LANGSYS_KEYS = (("DFLT", "dflt"), ("dflt", "dflt"))


class OTLConverter(object):
    leftSideTag = "Left"
    rightSideTag = "Right"
    backtrackTag = "BacktrackClassDef"
    InputTag = "InputClassDef"
    lookAheadTag = "LookAheadClassDef"
    markTag = "MarkClass"
    mark1Tag = "MarkMarkClass1"
    mark2Tag = "MarkMarkClass2"

    def __init__(self, writer, ttFont, table_tag):
        self.writer = writer
        self.table = ttFont[table_tag].table
        self.classesByNameList = collections.defaultdict(list)
        self.classesByLookup = {}
        self.classesByClassName = {}
        self.lookupIndex = -1
        self.seenLookup = {}
        self.glyphDict = {}
        self.markClassesByDefList = collections.defaultdict(list)
        self.markClassesByLookup = collections.defaultdict(list)
        self.markClassesByClassName = {}
        self.curLookupIndex = -1
        self.curSubTableIndex = -1
        self.curLookupName = ""
        self.showExtension = ttFont.showExtensionFlag

        for name in ttFont.getGlyphOrder():
            self.glyphDict[name] = 0

        if table_tag == "GPOS":
            self.ExtensionIndex = 9
            self.classHandlers = {
                (2, 2): classPairGPOS,
                (4, 1): markClassPOS,
                (5, 1): markLigClassPOS,
                (6, 1): markMarkClassPOS,
                (7, 2): classContext,
                (8, 2): classChainContext,
                (9, 1): classExt,
            }
            self.ruleHandlers = {
                1: ruleSinglePos,
                2: rulePairPos,
                3: ruleCursivePos,
                4: ruleMarkBasePos,
                5: ruleMarkLigPos,
                6: ruleMarkMarkPos,
                7: ruleContextPOS,
                8: ruleChainContextPOS,
                9: ruleExt,
            }

        elif table_tag == "GSUB":
            self.ExtensionIndex = 7
            self.classHandlers = {
                (5, 2): classContext,
                (6, 2): classChainContext,
                (7, 1): classExt,
            }
            self.ruleHandlers = {
                1: ruleSingleSub,
                2: ruleMultipleSub,
                3: ruleAltSub,
                4: ruleLigatureSub,
                5: ruleContextSUB,
                6: ruleChainContextSUB,
                7: ruleExt,
                8: ruleReverseChainSub,
            }
        else:
            raise KeyError("OTLConverter can only be called for GPOS and "
                           "GSUB tables")

    def otlFeatureFormat(self):
        # get and write language systems
        lsList, featDictByLangSys, featDictByIndex = self.buildLangSys()
        self.writeLangSys(lsList)

        self.writer.newline()

        # get and write class defs.
        self.buildClasses()
        self.writeClasses()
        self.writeMarkClasses()

        # do feature defs
        self.doFeatures(featDictByLangSys)

    @staticmethod
    def addLangSysEntry(pfeatList, scriptTag, langTag, langSys,
                        featDictByLangSys, featDictByIndex):
        for featIndex in langSys.FeatureIndex:
            try:
                featRecord = pfeatList[featIndex]
            except IndexError:
                log.error("FeatList does not contain index %s from current "
                          "langsys (%s, %s).", featIndex, scriptTag, langTag)
                log.error("FeaturListLen %s, langsys FeatIndex %s.",
                          len(pfeatList), langSys.FeatureIndex)
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
        haveDFLT = False
        lsList = []
        for scriptRecord in self.table.ScriptList.ScriptRecord:
            scriptTag = scriptRecord.ScriptTag
            if scriptTag in ("DFLT", "dflt"):
                haveDFLT = True
            if scriptRecord.Script.DefaultLangSys:
                langTag = "dflt"
                lsList.insert(0, (
                    scriptTag, langTag,
                    scriptRecord.Script.DefaultLangSys.ReqFeatureIndex))
                self.addLangSysEntry(pfeatList, scriptTag, langTag,
                                     scriptRecord.Script.DefaultLangSys,
                                     featDictByLangSys, featDictByIndex)

            for langSysRecord in scriptRecord.Script.LangSysRecord:
                langTag = langSysRecord.LangSysTag
                lsList.append((scriptTag, langTag,
                              langSysRecord.LangSys.ReqFeatureIndex))
                self.addLangSysEntry(pfeatList, scriptTag, langTag,
                                     langSysRecord.LangSys, featDictByLangSys,
                                     featDictByIndex)

        if not haveDFLT:
            # Synthesize a DFLT dflt entry.
            addedDFLT = False
            scriptCount = len(self.table.ScriptList.ScriptRecord)
            for langsysList in featDictByIndex.values():
                if len(langsysList) == scriptCount:
                    langsysList.insert(0, ("DFLT", "dflt", 0xFFFF))
                    addedDFLT = True

            if addedDFLT:
                lsList.insert(0, ("DFLT", "dflt", 0xFFFF))

        return lsList, featDictByLangSys, featDictByIndex

    def writeLangSys(self, lsList):
        # lsList is alphabetic, because of font order, except that
        # I moved (DFLT, dflt) script to the top.
        for lang_sys in lsList:
            if lang_sys[2] != 0xFFFF:
                rfTxt = "  Required Feature Index %s" % (lang_sys[2])
            else:
                rfTxt = ""
            self.writer.write("languagesystem %s %s%s;" % (
                lang_sys[0], lang_sys[1], rfTxt))
            self.writer.newline()

    def buildClasses(self):
        for lookupIndex, lookup in enumerate(self.table.LookupList.Lookup):
            self.curLookupIndex = lookupIndex
            for subtableIndex, subtable in enumerate(lookup.SubTable):
                self.curSubTableIndex = subtableIndex
                handler = self.classHandlers.get(
                    (lookup.LookupType, subtable.Format))
                if handler:
                    handler(subtable, self)

        for nameList in sorted(self.classesByNameList.keys()):
            classRecList = self.classesByNameList[nameList]
            lenList = len(nameList)
            if lenList == 0:
                className = "@empty"
            elif lenList == 1:
                className = "[%s]" % (nameList[0])
            elif lenList == 2:
                className = "@%s_%s" % (nameList[0], nameList[1])
            else:
                className = "@%s_%d_%s" % (nameList[0], len(nameList),
                                           nameList[-1])
                i = 1
                while (i < lenList) and className in self.classesByClassName:
                    className = "@%s_%s_%d_%s" % (nameList[0], nameList[i],
                                                  len(nameList), nameList[-1])
            self.classesByClassName[className] = nameList
            for classRec in classRecList:
                key = (classRec.lookupIndex, classRec.subtableIndex,
                       classRec.classIndex, classRec.side)
                self.classesByLookup[key] = className

        for defList in sorted(self.markClassesByDefList.keys()):
            classRecList = self.markClassesByDefList[defList]
            defNameList = []
            for nameList, anchor in defList:
                defNameList.extend(list(nameList))
            lenList = len(defNameList)
            if lenList == 0:
                className = "@empty_mark"
            elif lenList == 1:
                className = "%s_mark" % (defNameList[0])
            elif lenList == 2:
                className = "@%s_%s_mark" % (defNameList[0], defNameList[1])
            else:
                className = "@%s_%d_%s_mark" % (
                    defNameList[0], len(defNameList), defNameList[-1])
                i = 1
                while (i < lenList) and className in self.classesByClassName:
                    className = "@%s_%s_%d_%s_mark" % (
                        defNameList[0], defNameList[i], len(defNameList),
                        defNameList[-1])

            self.markClassesByClassName[className] = defList

            for classRec in classRecList:
                key = (classRec.lookupIndex, classRec.subtableIndex,
                       classRec.classIndex, classRec.side)
                self.markClassesByLookup[key] = className

    def writeClasses(self):
        classNames = list(self.classesByClassName.keys())
        if classNames:
            self.writer.newline()
            self.writer.write(
                "# Class definitions *********************************")
            self.writer.newline()
        for className in sorted(classNames):
            if className[0] == "[":
                # we don't write out the single glyph class names,
                # as they are ued in -line as class lists.
                continue
            class_glyphs = " ".join(self.classesByClassName[className])
            class_statement = "%s = [ %s ];" % (className, class_glyphs)
            class_lines = gtextWrapper.wrap(class_statement)
            for wrapped_line in class_lines:
                self.writer._writeraw(wrapped_line)
                self.writer.newline()
        self.writer.newline()

    def writeMarkClasses(self):
        classNames = list(self.markClassesByClassName.keys())
        if classNames:
            self.writer.newline()
            self.writer.write(
                "# Mark Class definitions *********************************")
            self.writer.newline()
        for className in sorted(classNames):
            classDef = self.markClassesByClassName[className]
            for gnameList, anchor in classDef:
                self.writer._writeraw("mark [")
                class_lines = gtextWrapper.wrap(" ".join(gnameList))
                num_lines = len(class_lines)
                for ln, wrapped_line in enumerate(class_lines):
                    self.writer._writeraw(wrapped_line)
                    if ln < num_lines - 1:
                        self.writer.newline()
                self.writer._writeraw("] %s %s;" % (anchor, className))
                self.writer.newline()
        self.writer.newline()

    def doFeatures(self, featDictByLangSys):
        for featTag in sorted(featDictByLangSys.keys()):
            self.writer.write("feature %s {" % featTag)
            self.writer.newline()
            self.writer.indent()
            langSysDict = featDictByLangSys[featTag]
            langSysTagList = sorted(langSysDict.keys())

            dflt_langsys_key = None
            dfltFeatIndexList = None
            for key in DFLT_LANGSYS_KEYS:
                if key in langSysDict:
                    dfltFeatIndexList = langSysDict[key]
                    langSysTagList.remove(key)
                    dflt_langsys_key = key
                    break

            dfltLookupIndexDict = None
            if dfltFeatIndexList:
                dfltLookupIndexDict = self.writeDfltLangSysFeat(
                    dflt_langsys_key, dfltFeatIndexList)

            if dflt_langsys_key:
                prevLangSysKey = dflt_langsys_key
            else:
                prevLangSysKey = DFLT_LANGSYS_KEYS[0]

            for langSysKey in langSysTagList:
                self.writeLangSysFeat(langSysKey, prevLangSysKey,
                                      langSysDict[langSysKey],
                                      dfltLookupIndexDict)
                prevLangSysKey = langSysKey

            self.writer.dedent()
            self.writer.write("} %s;" % featTag)
            self.writer.newline()
            self.writer.newline()

    def writeDfltLangSysFeat(self, langSysKey, featIndexList):
        # XXX similar to writeLangSysFeat()
        pfeatList = self.table.FeatureList.FeatureRecord
        pLookupList = self.table.LookupList.Lookup
        featRecord = None

        lookupIndexDict = {}
        # key: lookup index (int)
        # val: fontTools.ttLib.tables.otTables.Lookup (object)
        for featIndex in featIndexList:
            featRecord = pfeatList[featIndex]
            for lookupIndex in featRecord.Feature.LookupListIndex:
                lookupIndexDict[lookupIndex] = pLookupList[lookupIndex]

        excludeDFLTtxt = ""
        nameIndex = 0
        for li in sorted(lookupIndexDict.keys()):
            self.curLookupIndex = li
            lookup = pLookupList[li]
            lookupFlagTxt = getLookupFlagTag(lookup.LookupFlag)

            useExtension = ""
            if self.showExtension and lookup.LookupType == self.ExtensionIndex:
                useExtension = " useExtension"

            if li in self.seenLookup:
                lookupName = self.seenLookup[li]
                self.writer.write("lookup %s%s;" % (
                    lookupName, excludeDFLTtxt))
                self.writer.newline()
            else:
                lookupName = "%s_%s_%s_%s" % (
                    featRecord.FeatureTag, langSysKey[0], langSysKey[1],
                    nameIndex)
                self.seenLookup[li] = lookupName
                self.writer.write("lookup %s%s {" % (lookupName,
                                                     useExtension))
                excludeDFLTtxt = ""  # Only need to write it once.

                self.writer.newline()
                self.writer.indent()

                if lookupFlagTxt:
                    self.writer.write("lookupflag %s;" % lookupFlagTxt)
                    self.writer.newline()

                self.curLookupIndex = li
                self.writeLookup(lookup)
                self.writer.dedent()
                self.writer.write("} %s;" % lookupName)
                self.writer.newline()
                self.writer.newline()
                nameIndex += 1

        return lookupIndexDict

    def writeLangSysFeat(self, langSysKey, prevLangSysKey,
                         featIndexList, dfltLookupIndexDict):
        pfeatList = self.table.FeatureList.FeatureRecord
        pLookupList = self.table.LookupList.Lookup
        featRecord = None

        lookupIndexDict = {}
        # key: lookup index (int)
        # val: fontTools.ttLib.tables.otTables.Lookup (object)
        for featIndex in featIndexList:
            featRecord = pfeatList[featIndex]
            for lookupIndex in featRecord.Feature.LookupListIndex:
                lookupIndexDict[lookupIndex] = pLookupList[lookupIndex]

        # Remove all lookups shared with the DFLT/dflt script.
        # Note if there were any; if not, then we need to use the
        # exclude keyword with the lookup.
        haveAnyDflt = False
        excludeDFLT = False
        if dfltLookupIndexDict:
            for lookupIndex in sorted(lookupIndexDict.keys()):
                if lookupIndex in dfltLookupIndexDict:
                    del lookupIndexDict[lookupIndex]
                    haveAnyDflt = True
            if not haveAnyDflt:
                excludeDFLT = True
        liList = sorted(lookupIndexDict.keys())

        excludeDFLTtxt = ""
        if excludeDFLT:
            excludeDFLTtxt = " excludeDFLT"

        # If all the lookups were shared with DFLt dflt,
        # no need to write anything.
        if liList:
            nameIndex = 0
            if prevLangSysKey[0] != langSysKey[0]:
                self.writer.write("script %s;" % (langSysKey[0]))
                self.writer.newline()
                self.writer.write("language %s;" % (langSysKey[1]))
                self.writer.newline()
            elif prevLangSysKey[1] != langSysKey[1]:
                self.writer.write("language %s;" % (langSysKey[1]))
                self.writer.newline()

            for li in liList:
                self.curLookupIndex = li
                lookup = pLookupList[li]
                lookupFlagTxt = getLookupFlagTag(lookup.LookupFlag)

                useExtension = ""
                if self.showExtension and (
                        lookup.LookupType == self.ExtensionIndex):
                    useExtension = " useExtension"

                if li in self.seenLookup:
                    lookupName = self.seenLookup[li]
                    self.writer.write("lookup %s%s;" % (
                        lookupName, excludeDFLTtxt))
                    excludeDFLTtxt = ""  # Only need to write it once.
                    self.writer.newline()
                else:
                    lookupName = "%s_%s_%s_%s" % (
                        featRecord.FeatureTag, langSysKey[0].strip(),
                        langSysKey[1].strip(), nameIndex)
                    self.seenLookup[li] = lookupName
                    self.writer.write("lookup %s%s {" % (lookupName,
                                                         useExtension))
                    excludeDFLTtxt = ""  # Only need to write it once.

                    self.writer.newline()
                    self.writer.indent()

                    if lookupFlagTxt:
                        self.writer.write("lookupflag %s;" % lookupFlagTxt)
                        self.writer.newline()

                    self.curLookupIndex = li
                    self.writeLookup(lookup)
                    self.writer.dedent()
                    self.writer.write("} %s;" % lookupName)
                    self.writer.newline()
                    self.writer.newline()
                    nameIndex += 1

    def writeLookup(self, lookup):
        lookupType = lookup.LookupType
        handler = self.ruleHandlers[lookupType]

        for si, sub_table in enumerate(lookup.SubTable):
            self.curSubTableIndex = si
            rules = handler(sub_table, self)
            for rule in rules:
                self.writer._writeraw(rule)
                self.writer.newline()


def dumpOTLAsFeatureFile(writer, ttFont, table_tag):
    otlConverter = OTLConverter(writer, ttFont, table_tag)
    otlConverter.otlFeatureFormat()


class TTXNTTFont(TTFont):
    def __init__(self, file=None, res_name_or_index=None,
                 sfntVersion="\000\001\000\000", flavor=None,
                 checkChecksums=False, verbose=None, recalcBBoxes=True,
                 allowVID=False, ignoreDecompileErrors=False,
                 recalcTimestamp=True, fontNumber=-1, lazy=None, quiet=None,
                 supressHints=False, showExtensionFlag=False):

        self.filePath = file
        self.supressHints = supressHints
        self.showExtensionFlag = showExtensionFlag
        TTFont. __init__(self, file, res_name_or_index=res_name_or_index,
                         sfntVersion=sfntVersion, flavor=flavor,
                         checkChecksums=checkChecksums, verbose=verbose,
                         recalcBBoxes=recalcBBoxes, allowVID=allowVID,
                         ignoreDecompileErrors=ignoreDecompileErrors,
                         recalcTimestamp=recalcTimestamp,
                         fontNumber=fontNumber, lazy=lazy, quiet=quiet)

    def _tableToXML(self, writer, tag, quiet=None, splitGlyphs=False):
        table = None
        if quiet is not None:
            from fontTools.misc.loggingTools import deprecateArgument
            deprecateArgument("quiet", "configure logging instead")
        if tag in self:
            table = self[tag]
            report = "Dumping '%s' table..." % tag
        else:
            report = "No '%s' table found." % tag
        log.info(report)
        if tag not in self:
            return
        xmlTag = tagToXML(tag)
        attrs = dict()
        if hasattr(table, "ERROR"):
            attrs['ERROR'] = "decompilation error"
        if table.__class__ == DefaultTable:
            attrs['raw'] = True
        writer.begintag(xmlTag, **attrs)
        writer.newline()
        if tag in ("glyf", "CFF "):
            dumpFont(writer, self.filePath, self.supressHints)
        elif tag in ("GSUB", "GPOS"):
            dumpOTLAsFeatureFile(writer, self, tag)
        else:
            table.toXML(writer, self)
        writer.endtag(xmlTag)
        writer.newline()
        writer.newline()


def shellcmd(cmdList):
    # In all cases, assume that cmdList does NOT specify the output file.
    # I use this because tx -dump -6 can be very large.
    tempPath = get_temp_file_path()
    cmdList.append(tempPath)
    subprocess.check_call(cmdList)
    with open(tempPath, "r", encoding="utf-8") as fp:
        data = fp.read()
    return data


def dumpFont(writer, fontPath, supressHints=False):
    dictTxt = shellcmd([TX_TOOL, "-dump", "-0", fontPath])
    if curSystem == "Windows":
        dictTxt = re.sub(r"[\r\n]+", "\n", dictTxt)
    dictTxt = re.sub(r"##[^\r\n]*Filename[^\r\n]+", "", dictTxt, 1).strip()
    dictLines = dictTxt.splitlines()
    writer.begintag("FontTopDict")
    writer.newline()
    for line in dictLines:
        writer._writeraw(line)
        writer.newline()
    writer.endtag("FontTopDict")
    writer.newline()

    if supressHints:
        charData = shellcmd([TX_TOOL, "-dump", "-6", "-n", fontPath])
    else:
        charData = shellcmd([TX_TOOL, "-dump", "-6", fontPath])

    if curSystem == "Windows":
        charData = re.sub(r"[\r\n]+", "\n", charData)
    charList = re.findall(r"[^ ]glyph\[[^]]+\] {([^,]+),[^\r\n]+,([^}]+)",
                          charData)
    if "cid.CIDFontName" in dictTxt:
        # fix glyph names to sort
        charList = [("cid%s" % (entry[0]).zfill(5),
                     entry[1]) for entry in charList]

    charList = sorted([entry[0] + entry[1] for entry in charList])
    charTxt = "\n".join(charList)
    writer.begintag("FontOutlines")
    writer.newline()
    for line in charTxt.splitlines():
        writer._writeraw(line)
        writer.newline()
    writer.endtag("FontOutlines")
    writer.newline()


@Timer(log, 'Done dumping TTX in %(time).3f seconds')
def ttnDump(input_file, output, options, showExtensionFlag, supressHints=False,
            supressVersions=False, supressTTFDiffs=False):
    log.info('Dumping "%s" to "%s"...', input_file, output)
    if options.unicodedata:
        from fontTools.unicode import setUnicodeData
        setUnicodeData(options.unicodedata)
    ttf = TTXNTTFont(input_file, 0, allowVID=options.allowVID,
                     ignoreDecompileErrors=options.ignoreDecompileErrors,
                     fontNumber=options.fontNumber, supressHints=supressHints,
                     showExtensionFlag=showExtensionFlag)

    kDoNotDumpTagList = ["GlyphOrder", "DSIG"]
    if options.onlyTables:
        onlyTables = sorted(options.onlyTables)
    else:
        onlyTables = sorted(ttf.keys())

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
        # hmtx must be decompiled *before* we zero
        # the hhea.numberOfHMetrics value
        temp = hmtx.metrics
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
        # vmtx must be decompiled *before* we zero
        # the vhea.numberOfHMetrics value
        temp = vmtx.metrics
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
                if namerecord.nameID == 3:
                    if namerecord.isUnicode():
                        namerecord.string = "VERSION SUPPRESSED".encode(
                            "utf-16be")
                    else:
                        namerecord.string = "VERSION SUPPRESSED"
                elif namerecord.nameID == 5:
                    if namerecord.platformID == 3:
                        namerecord.string = namerecord.string.split(';')[0]
                    else:
                        namerecord.string = "VERSION SUPPRESSED"

    if 'GDEF' in onlyTables:
        GDEF = ttf["GDEF"]
        gt = GDEF.table
        if gt.GlyphClassDef:
            gt.GlyphClassDef.Format = 0
        if gt.MarkAttachClassDef:
            gt.MarkAttachClassDef.Format = 0
        if gt.AttachList:
            if not gt.AttachList.Coverage.glyphs:
                gt.AttachList = None

        if gt.LigCaretList:
            if not gt.LigCaretList.Coverage.glyphs:
                gt.LigCaretList = None

    if 'cmap' in onlyTables:
        # remove mappings to notdef.
        cmapTable = ttf["cmap"]
        """ Force shared cmap tables to get separately decompiled.
        The _c_m_a_p.py logic will decompile a cmap from source data if an
        attempt is made to access a field which is not (yet) defined. Unicode
        (format 4) subtables are usually identical, and thus shared. When
        intially decompiled, the first gets fully decompiled, and then the
        second gets a reference to the 'cmap' dict of the first.
        When entries are removed from the cmap dict of the first subtable,
        that is the same as removing them from the cmap dict of the second.
        However, when later an attempt is made to reference the 'nGroups'
        field - which doesn't exist  in format 4 - the second table gets
        fully decompiled, and its cmap dict is rebuilt from the original data.
        """
        for cmapSubtable in cmapTable.tables:
            # if cmapSubtable.format != 4:
            #    continue
            delList = []
            for mapping in cmapSubtable.cmap.items():
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

            if (cmapSubtable.format in [12, 14]) and hasattr(cmapSubtable,
                                                             "nGroups"):
                cmapSubtable.nGroups = 0
            if hasattr(cmapSubtable, "length"):
                cmapSubtable.length = 0

    if onlyTables:
        ttf.saveXML(output,
                    tables=onlyTables,
                    skipTables=options.skipTables,
                    splitTables=options.splitTables,
                    splitGlyphs=options.splitGlyphs,
                    disassembleInstructions=options.disassembleInstructions,
                    bitmapGlyphDataFormat=options.bitmapGlyphDataFormat)
    ttf.close()
    return ttf


def main(args=None):
    from fontTools import configLogger

    if args is None:
        args = sys.argv[1:]

    if ("-h" in args) or ("-u" in args):
        print(__help__)
        return 0

    if "-a" not in args:
        args.insert(0, "-a")  # allow virtual GIDS.

    if "-nh" in args:
        args.remove("-nh")
        supressHints = True
    else:
        supressHints = False

    if "-se" in args:
        args.remove("-se")
        showExtensionFlag = True
    else:
        showExtensionFlag = False

    if "-nv" in args:
        args.remove("-nv")
        supressVersions = True
    else:
        supressVersions = False

    if "-supressTTFDiffs" in args:
        args.remove("-supressTTFDiffs")
        supressTTFDiffs = True
    else:
        supressTTFDiffs = False

    try:
        jobs, options = ttx.parseOptions(args)
    except getopt.GetoptError as e:
        print("ERROR:", e, file=sys.stderr)
        return 2

    configLogger(level=options.logLevel)

    try:
        for action, input_file, output in jobs:
            if action != ttx.ttDump:
                log.error("ttxn can only dump font files.")
                return 1
            ttnDump(input_file, output, options, showExtensionFlag,
                    supressHints, supressVersions, supressTTFDiffs)
    except SystemExit:
        raise
    except TTLibError as e:
        log.error(e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
