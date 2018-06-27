#!/bin/env python
# Copyright 2014 Adobe. All rights reserved.

from __future__ import print_function, absolute_import

import os
import re
import sys
import types

from . import FDKUtils

__doc__ = """
ConvertFontToCID.py. v 1.11.3 Apr 29 2018

Convert a Type 1 font to CID, given multiple hint dict defs in the
"fontinfo" file. See AC.py help, with the "-hfd" option, or the MakeOTF
user guide for details on this format. The output file produced by
convertFontToCID() is a CID Type 1 font, no matter what the input is.

The "fontinfo" file is expected to be in the same directory as the input
font file.

Note that this file makes a lot of temporary files, using the input font
path as the base file path, so the parent directory needs to be
read/write enabled.
"""

__methods__ = """
1) parse fontinfo file. Collect all font dicts and glyph sets.
2) define final glyph sets = all glyphs not in other glyph sets
3) build glyph sets such that all glyphs in a set are contiguous and
ordered by original GID, and belong to the same fdDict. This allows the
glyphs to be merged back into a single CID font while maintaining glyph
order.
4) for each glyph set:
    use mergeFonts to subset the font to the glyph set
    use detype1 to make it editable
    edit in the FontDict values
    use type1 to convert back to type1
5) Use mergeFonts to merge fonts as a CID font.
"""

debug = 0

# Tokens seen in font info file that are not part
# of a FDDict or GlyphSet definition.
kBeginToken = "begin"
kEndToken = "end"
kFDDictToken = "FDDict"
kGlyphSetToken = "GlyphSet"
kFinalDictName = "FinalFont"
kDefaultDictName = "No Alignment Zones"

kBaseStateTokens = [
    "FontName",
    "FullName",
    "IsBoldStyle",
    "IsItalicStyle",
    "ConvertToCID",
    "PreferOS/2TypoMetrics",
    "IsOS/2WidthWeigthSlopeOnly",
    "IsOS/2OBLIQUE",
    "UseOldNameID4",
    "LicenseCode"
]

kBlueValueKeys = [
    "BaselineOvershoot",  # 0
    "BaselineYCoord",  # 1
    "CapHeight",  # 2
    "CapOvershoot",  # 3
    "LcHeight",  # 4
    "LcOvershoot",  # 5
    "AscenderHeight",  # 6
    "AscenderOvershoot",  # 7
    "FigHeight",  # 8
    "FigOvershoot",  # 9
    "Height5",  # 10
    "Height5Overshoot",  # 11
    "Height6",  # 12
    "Height6Overshoot",  # 13
]

kOtherBlueValueKeys = [
    "Baseline5Overshoot",  # 0
    "Baseline5",  # 1
    "Baseline6Overshoot",  # 2
    "Baseline6",  # 3
    "SuperiorOvershoot",  # 4
    "SuperiorBaseline",  # 5
    "OrdinalOvershoot",  # 6
    "OrdinalBaseline",  # 7
    "DescenderOvershoot",  # 8
    "DescenderHeight",  # 9
]

kOtherFDDictKeys = [
    "FontName",
    "OrigEmSqUnits",
    "LanguageGroup",
    "DominantV",
    "DominantH",
    "FlexOK",
    "VCounterChars",
    "HCounterChars",
    "BlueFuzz"
]

# We keep this in the FDDict, as it is easier
# to sort and validate as a list of pairs
kFontDictBluePairsName = "BlueValuesPairs"
kFontDictOtherBluePairsName = "OtherBlueValuesPairs"
# Holds the actual string for the Type1 font dict
kFontDictBluesName = "BlueValues"
# Holds the actual string for the Type1 font dict
kFontDictOtherBluesName = "OtherBlues"
kRunTimeFDDictKeys = [
    "DictName",
    kFontDictBluePairsName,
    kFontDictOtherBluePairsName,
    kFontDictBluesName,
    kFontDictOtherBluesName
]
kFDDictKeys = (kOtherFDDictKeys + kBlueValueKeys + kOtherBlueValueKeys +
               kRunTimeFDDictKeys)


# CID stuff
kRequiredCIDFontInfoFields = [
    "FontName",
    "Registry",
    "Ordering",
    "Supplement",
]

kOptionalFields = [
    "FSType",
    "Weight",
    "Trademark",
    "FullName",
    "FamilyName",
    "version",
    "AdobeCopyright",
    "Trademark",
]
kCIDFontInfokeyList = kRequiredCIDFontInfoFields + kOptionalFields

txFields = [  # [field name in tx, key name in cidfontinfo, value]
    ["FontName", "FontName", ""],
    ["FullName", "FullName", ""],
    ["FamilyName", "FamilyName", ""],
    ["version", "version", ""],
    ["Notice", "AdobeCopyright", ""],
    ["Copyright", "AdobeCopyright", ""],
    ["Trademark", "Trademark", ""],
    ["FSType", "FSType", ""],
    ["Weight", "Weight", ""],
]


class FontInfoParseError(ValueError):
    pass


class FontParseError(KeyError):
    pass


class FDDict(object):
    def __init__(self):
        self.DictName = None
        for key in kFDDictKeys:
            exec("self.%s = None" % (key))
        self.FlexOK = "true"

    def getFontInfo(self):
        keys = dir(self)
        fiList = []
        for key in keys:
            if key.startswith("_") or (key in kRunTimeFDDictKeys):
                continue
            value = eval("self.%s" % (key))
            if isinstance(value, types.MethodType):
                continue

            if value is not None:
                fiList.append("%s %s" % (key, value))
        return " ".join(fiList)

    def buildBlueLists(self):
        if self.BaselineOvershoot is None:
            print("Error: FDDict definition %s is missing the "
                  "BaselineYCoord/BaselineOvershoot values. These are "
                  "required." % self.DictName)
        elif int(self.BaselineOvershoot) > 0:
            print("Error: The BaselineYCoord/BaselineOvershoot in FDDict "
                  "definition %s must be a bottom zone - the "
                  "BaselineOvershoot must be negative, not positive." %
                  self.DictName)

        blueKeyList = [kBlueValueKeys, kOtherBlueValueKeys]
        bluePairListNames = [kFontDictBluePairsName,
                             kFontDictOtherBluePairsName]
        blueFieldNames = [kFontDictBluesName, kFontDictOtherBluesName]
        for i in [0, 1]:
            keyList = blueKeyList[i]
            fieldName = blueFieldNames[i]
            pairFieldName = bluePairListNames[i]
            bluePairList = []
            for key in keyList:
                if key.endswith("Overshoot"):
                    width = eval("self.%s" % key)  # XXX this looks fishy!!
                    if width is not None:
                        width = int(width)
                        baseName = key[:-len("Overshoot")]
                        zonePos = None
                        if key == "BaselineOvershoot":
                            zonePos = eval("self.BaselineYCoord")
                            zonePos = int(zonePos)
                            tempKey = "BaselineYCoord"
                        else:
                            for posSuffix in ["", "Height", "Baseline"]:
                                tempKey = "%s%s" % (baseName, posSuffix)
                                try:
                                    zonePos = eval("self.%s" % tempKey)
                                    zonePos = int(zonePos)
                                    break
                                except AttributeError:
                                    continue
                        if zonePos is None:
                            raise FontInfoParseError(
                                "Failed to find fontinfo  FDDict %s "
                                "top/bottom zone name %s to match the zone "
                                "width key '%s'." % (self.DictName, tempKey,
                                                     key))
                        if width < 0:
                            topPos = zonePos
                            bottomPos = zonePos + width
                            isBottomZone = 1
                            if (i == 0) and (key != "BaselineOvershoot"):
                                print("Error: FontDict %s. Zone %s is a top "
                                      "zone, and the width (%s)  must be "
                                      "positive." % (self.DictName, tempKey,
                                                     width))
                        else:
                            bottomPos = zonePos
                            topPos = zonePos + width
                            isBottomZone = 0
                            if (i == 1):
                                print("Error: FontDict %s. Zone %s is a "
                                      "bottom zone, and so the width (%s) "
                                      "must be negative.." % (
                                          self.DictName, tempKey, width))
                        bluePairList.append((topPos, bottomPos, tempKey,
                                             self.DictName, isBottomZone))

            if bluePairList:
                bluePairList.sort()
                prevPair = bluePairList[0]
                zoneBuffer = 2 * self.BlueFuzz + 1
                for pair in bluePairList[1:]:
                    if prevPair[0] > pair[1]:
                        print("Error in FDDict %s. The top of zone %s at %s "
                              "overlaps zone %s with the bottom at %s." %
                              (self.DictName, prevPair[2], prevPair[0],
                               pair[2], pair[1]))
                    elif abs(pair[1] - prevPair[0]) <= zoneBuffer:
                        print("Error in FDDict %s. The top of zone %s at %s "
                              "is within the min spearation limit (%s units) "
                              "of zone %s with the bottom at %s." %
                              (self.DictName, prevPair[2], prevPair[0],
                               zoneBuffer, pair[2], pair[1]))
                    prevPair = pair
                exec("self.%s = %s" % (pairFieldName, bluePairList))
                bluesList = []
                for pairEntry in bluePairList:
                    bluesList.append(pairEntry[1])
                    bluesList.append(pairEntry[0])
                bluesList = map(str, bluesList)
                bluesList = "[%s]" % (" ".join(bluesList))
                # print(self.DictName, bluePairList)
                # print("\t", bluesList)
                exec("self.%s = \"%s\"" % (fieldName, bluesList))
        return

    def __repr__(self):
        printStr = []
        keys = dir(self)
        for key in keys:
            val = eval("self.%s" % (key))
            # print(key, type(val))
            if ((val is None) or isinstance(val, types.MethodType) or
               key.startswith("_")):
                continue
            printStr.append(key)
            printStr.append("%s" % (val))
        return " ".join(printStr)


def parseFontInfoFile(fontDictList, data, glyphList, maxY, minY, fontName,
                      blueFuzz):
    # fontDictList may or may not already contain a font dict
    # taken from the source font top FontDict.

    # The map of glyph names to font dict: the index into fontDictList.
    fdGlyphDict = {}
    # The user-specified set of blue values to write into the output font,
    # some sort of merge of the individual font dicts. May not be supplied.
    finalFDict = None

    # Get rid of comments.
    data = re.sub(r"#[^\r\n]+[\r\n]", "", data)

    # We assume that no items contain whitespace.
    tokenList = data.split()
    numTokens = len(tokenList)
    i = 0
    baseState = 0
    inValue = 1
    inDictValue = 2
    dictState = 3
    glyphSetState = 4
    fdIndexDict = {}
    lenSrcFontDictList = len(fontDictList)
    dictValueList = []
    dictKeyWord = None

    state = baseState

    while i < numTokens:
        token = tokenList[i]
        i += 1
        if state == baseState:
            if token == kBeginToken:
                token = tokenList[i]
                i += 1
                if token == kFDDictToken:
                    state = dictState
                    dictName = tokenList[i]
                    i += 1
                    fdDict = FDDict()
                    fdDict.DictName = dictName
                    if dictName == kFinalDictName:
                        # This is dict is NOT used to hint any glyphs;
                        # it is used to supply the merged alignment zones
                        # and stem widths for the final font.
                        finalFDict = fdDict
                    else:
                        # save dict and FDIndex.
                        fdIndexDict[dictName] = len(fontDictList)
                        fontDictList.append(fdDict)

                elif token == kGlyphSetToken:
                    state = glyphSetState
                    setName = tokenList[i]
                    i += 1
                else:
                    raise FontInfoParseError(
                        "Unrecognized token after \"begin\" keyword: %s" %
                        token)

            elif token in kBaseStateTokens:
                # Discard value for base token.
                token = tokenList[i]
                i += 1
                if (token[0] in ["[", "("]) and (token[-1] not in ["]", ")"]):
                    state = inValue
            else:
                raise FontInfoParseError(
                    "Unrecognized token in base state: %s" % token)

        elif state == inValue:
            # We are processing a list value for a base state token.
            if token[-1] in ["]", ")"]:
                # found the last token in the list value.
                state = baseState

        elif state == inDictValue:
            dictValueList.append(token)
            if token[-1] in ["]", ")"]:
                value = " ".join(dictValueList)
                exec("fdDict.%s = \"%s\"" % (dictKeyWord, value))
                # found the last token in the list value.
                state = dictState

        elif state == glyphSetState:
            # "end GlyphSet" marks end of set,
            # else we are adding a new glyph name.
            if (token == kEndToken) and tokenList[i] == kGlyphSetToken:
                if tokenList[i + 1] != setName:
                    raise FontInfoParseError(
                        "End glyph set name \"%s\" does not match begin "
                        "glyph set name \"%s\"." % (tokenList[i + 1], setName))
                state = baseState
                i += 2
                setName = None
            else:
                # Need to add matching glyphs.
                gi = 0
                for gname in glyphList:
                    if re.search(token, gname):
                        # fdIndex value
                        fdGlyphDict[gname] = [fdIndexDict[setName], gi]
                    gi += 1

        elif state == dictState:
            # "end FDDict" marks end of set,
            # else we are adding a new glyph name.
            if (token == kEndToken) and tokenList[i] == kFDDictToken:
                if tokenList[i + 1] != dictName:
                    raise FontInfoParseError(
                        "End FDDict  name \"%s\" does not match begin FDDict "
                        "name \"%s\"." % (tokenList[i + 1], dictName))
                if fdDict.DominantH is None:
                    print("Warning: the FDDict '%s' in fontinfo has no "
                          "DominantH value" % dictName)
                if fdDict.DominantV is None:
                    print("Warning: the FDDict '%s' in fontinfo has no "
                          "DominantV value" % dictName)
                if fdDict.BlueFuzz is None:
                    fdDict.BlueFuzz = blueFuzz
                fdDict.buildBlueLists()
                if fdDict.FontName is None:
                    fdDict.FontName = fontName
                state = baseState
                i += 2
                dictName = None
                fdDict = None
            else:
                if token in kFDDictKeys:
                    value = tokenList[i]
                    i += 1
                    if ((value[0] in ["[", "("]) and
                       (value[-1] not in ["]", ")"])):
                        state = inDictValue
                        dictValueList = [value]
                        dictKeyWord = token
                    else:
                        exec("fdDict.%s = \"%s\"" % (token, value))
                else:
                    raise FontInfoParseError(
                        "FDDict key \"%s\" in fdDict named \"%s\" is not "
                        "recognized." % (token, dictName))

    if lenSrcFontDictList != len(fontDictList):
        # There are some FDDict definitions. This means that we need
        # to fix the default fontDict, inherited from the source font,
        # so that it has blues zones that will not affect hinting,
        # e.g outside of the Font BBox. We do this becuase if there are
        # glyphs which are not assigned toa user specified font dict,
        # it is becuase it doesn't make sense to provide alignment zones
        # for the glyph. Since AC does require at least one bottom zone
        # and one top zone, we add one bottom and one top zone that are
        # outside the font BBox, so that hinting won't be affected by them.
        defaultFDDict = fontDictList[0]
        for key in kBlueValueKeys + kOtherBlueValueKeys:
            exec("defaultFDDict.%s = None" % (key))
        defaultFDDict.BaselineYCoord = minY - 100
        defaultFDDict.BaselineOvershoot = 0
        defaultFDDict.CapHeight = maxY + 100
        defaultFDDict.CapOvershoot = 0
        defaultFDDict.BlueFuzz = 0
        defaultFDDict.DictName = kDefaultDictName  # "No Alignment Zones"
        defaultFDDict.FontName = fontName
        defaultFDDict.buildBlueLists()
        gi = 0
        for gname in glyphList:
            if gname not in fdGlyphDict:
                fdGlyphDict[gname] = [0, gi]
            gi += 1

    return fdGlyphDict, fontDictList, finalFDict


def mergeFDDicts(prevDictList, privateDict):
    # Extract the union of the stem widths and zones from the list
    # of FDDicts, and replace the current values in the topDict.
    fdDictName = None
    prefDDict = None
    zoneName = None
    blueZoneDict = {}
    otherBlueZoneDict = {}
    dominantHDict = {}
    dominantVDict = {}
    bluePairListNames = [kFontDictBluePairsName, kFontDictOtherBluePairsName]
    zoneDictList = [blueZoneDict, otherBlueZoneDict]
    for prefDDict in prevDictList:
        for ki in [0, 1]:
            zoneDict = zoneDictList[ki]
            bluePairName = bluePairListNames[ki]
            bluePairList = eval("prefDDict.%s" % bluePairName)
            if not bluePairList:
                continue
            for topPos, bottomPos, zoneName, _, isBottomZone in bluePairList:
                zoneDict[(topPos, bottomPos)] = (isBottomZone, zoneName,
                                                 prefDDict.DictName)

        # Now for the stem widths.
        stemNameList = ["DominantH", "DominantV"]
        stemDictList = [dominantHDict, dominantVDict]
        for wi in (0, 1):
            stemFieldName = stemNameList[wi]
            dList = eval("prefDDict.%s" % stemFieldName)
            stemDict = stemDictList[wi]
            if dList is not None:
                dList = dList[1:-1]  # remove the braces
                dList = dList.split()
                dList = map(int, dList)
                for width in dList:
                    stemDict[width] = prefDDict.DictName

    # Now we have collected all the stem widths and zones
    # from all the dicts. See if we can merge them.
    goodBlueZoneList = []
    goodOtherBlueZoneList = []
    goodHStemList = []
    goodVStemList = []

    zoneDictList = [blueZoneDict, otherBlueZoneDict]
    goodZoneLists = [goodBlueZoneList, goodOtherBlueZoneList]
    stemDictList = [dominantHDict, dominantVDict]
    goodStemLists = [goodHStemList, goodVStemList]

    for ki in [0, 1]:
        zoneDict = zoneDictList[ki]
        goodZoneList = goodZoneLists[ki]
        stemDict = stemDictList[ki]
        goodStemList = goodStemLists[ki]

        # Zones first.
        zoneList = zoneDict.keys()
        if not zoneList:
            continue
        zoneList.sort()
        # Now check for conflicts.
        prevZone = zoneList[0]
        goodZoneList.append(prevZone[1])
        goodZoneList.append(prevZone[0])
        zoneBuffer = 2 * prefDDict.BlueFuzz + 1
        for zone in zoneList[1:]:
            if (ki == 0) and (len(zoneList) >= 14):
                print("Warning. For final FontDict, skipping BlueValues "
                      "alignment zone %s from FDDict %s because there are "
                      "already 7 zones." % (zoneName, fdDictName))
            elif (ki == 1) and (len(zoneList) >= 5):
                print("Warning. For final FontDict, skipping OtherBlues "
                      "alignment zone %s from FDDict %s because there are "
                      "already 5 zones." % (zoneName, fdDictName))
            if zone[1] < prevZone[0]:
                curEntry = blueZoneDict[zone]
                prevEntry = blueZoneDict[prevZone]
                zoneName = curEntry[1]
                fdDictName = curEntry[2]
                prevZoneName = prevEntry[1]
                prevFDictName = prevEntry[2]
                print("Warning. For final FontDict, skipping zone %s in "
                      "FDDict %s because it overlaps with zone %s in "
                      "FDDict %s." % (zoneName, fdDictName, prevZoneName,
                                      prevFDictName))
            elif abs(zone[1] - prevZone[0]) <= zoneBuffer:
                curEntry = blueZoneDict[zone]
                prevEntry = blueZoneDict[prevZone]
                zoneName = curEntry[1]
                fdDictName = curEntry[2]
                prevZoneName = prevEntry[1]
                prevFDictName = prevEntry[2]
                print("Warning. For final FontDict, skipping zone %s in "
                      "FDDict %s because it is within the minimum separation "
                      "allowed (%s units) of %s in FDDict %s." %
                      (zoneName, fdDictName, zoneBuffer, prevZoneName,
                       prevFDictName))
            else:
                goodZoneList.append(zone[1])
                goodZoneList.append(zone[0])

            prevZone = zone

        stemList = stemDict.keys()
        if not stemList:
            continue
        stemList.sort()
        # Now check for conflicts.
        prevStem = stemList[0]
        goodStemList.append(prevStem)
        for stem in stemList[1:]:
            if abs(stem - prevStem) < 2:
                fdDictName = stemDict[stem]
                prevFDictName = stemDict[prevStem]
                print("Warning. For final FontDict, skipping stem width %s "
                      "in FDDict %s because it overlaps in coverage with "
                      "stem width %s in FDDict %s." %
                      (stem, fdDictName, prevStem, prevFDictName))
            else:
                goodStemList.append(stem)
            prevStem = stem
    if goodBlueZoneList:
        privateDict.BlueValues = goodBlueZoneList
    if goodOtherBlueZoneList:
        privateDict.OtherBlues = goodOtherBlueZoneList
    else:
        privateDict.OtherBlues = None
    if goodHStemList:
        privateDict.StemSnapH = goodHStemList
    else:
        privateDict.StemSnapH = None
    if goodVStemList:
        privateDict.StemSnapV = goodVStemList
    else:
        privateDict.StemSnapV = None
    return


def getGlyphList(fPath, removeNotdef=0):
    command = "tx -dump -4 \"%s\" 2>&1" % fPath
    data = FDKUtils.runShellCmd(command)
    if not data:
        print("Error: Failed getting glyph names from %s with tx." % fPath)
        return []

    # initial newline keeps us from picking up the first column header line
    nameList = re.findall(r"[\r\n]glyph.+?{([^,]+),", data)
    if not nameList:
        print("Error: Failed getting glyph names from %s with tx." % fPath)
        print(data)
        return []
    if removeNotdef:
        nameList.remove(".notdef")
    return nameList


def getFontBBox(fPath):
    fontBBox = [-200, -200, 1000, 100]
    command = "tx -mtx -2  \"%s\" 2>&1" % fPath
    data = FDKUtils.runShellCmd(command)
    if not data:
        raise FontInfoParseError("Error: Failed getting log from tx from %s, "
                                 "when trying to get FontBBox." % fPath)

    m = re.search(r"bbox[^{]+{([^}]+)}", data)
    if not m:
        print(data)
        raise FontInfoParseError("Error: Failed finding FontBBox in tx log "
                                 "from %s." % fPath)
    fontBBox = m.group(1).split(",")
    fontBBox = map(int, fontBBox)
    return fontBBox


def getFontName(fPath):
    command = "tx -dump -0 \"%s\" 2>&1" % fPath
    data = FDKUtils.runShellCmd(command)
    if not data:
        raise FontInfoParseError("Error: Failed getting log from tx from %s, "
                                 "when trying to get FontName." % fPath)

    m = re.search(r"FontName\s+\"([^\"]+)", data)
    if not m:
        print(data)
        raise FontInfoParseError("Error: Failed finding FontName in tx log "
                                 "from %s." % fPath)
    return m.group(1)


def getBlueFuzz(fPath):
    blueFuzz = 1.0
    command = "tx -dump -0 \"%s\" 2>&1" % (fPath)
    data = FDKUtils.runShellCmd(command)
    if not data:
        raise FontInfoParseError("Error: Failed getting log from tx from %s, "
                                 "when trying to get FontName." % fPath)

    m = re.search(r"BlueFuzz\s+(\d+(?:\.\d+)*)", data)
    if m:
        blueFuzz = int(m.group(1))
    return blueFuzz


def makeSortedGlyphSets(glyphList, fdGlyphDict):
    """ Start a glyph set list. For each glyph in the font glyph,
    check the FD index. If it is different than the previous one,
    start a new glyphset."""
    glyphSet = []
    glyphSetList = [glyphSet]
    fdIndex = 0
    for glyphTag in glyphList:
        try:
            newFDIndex = fdGlyphDict[glyphTag][0]
        except KeyError:
            newFDIndex = 0
            # use the default dict.
            fdGlyphDict[glyphTag] = [newFDIndex, glyphTag]
        else:
            raise FontParseError("Program Error - call Read Roberts. "
                                 "In makeSortedGlyphSets")

        if newFDIndex == fdIndex:
            glyphSet.append(glyphTag)
        else:
            # fdIndex = newFDIndex
            glyphSet = [glyphTag]
            glyphSetList.append(glyphSet)

    if len(glyphSetList) < 2:
        print("Warning: There is only one hint dict. Maybe the fontinfo file "
              "has not been edited to add hint dict info.")
    return glyphSetList


def fixFontDict(tempPath, fdDict):
    txtPath = tempPath + ".txt"
    command = "detype1 \"%s\" \"%s\" 2>&1" % (tempPath, txtPath)
    log = FDKUtils.runShellCmd(command)
    if log:
        print(log)
    fp = open(txtPath, "rt")
    data = fp.read()
    fp.close()

    # fix font name. We always search of it, as it is always present,
    # and we can use the following white space to get the file new line.
    m = re.search(r"(/FontName\s+/\S+\s+def)(\s+)", data)
    newLine = m.group(2)
    if not m:
        raise FontParseError("Failed to find FontName in input font! "
                             "%s" % tempPath)
    if fdDict.FontName:
        target = "/FontName /%s def" % (fdDict.FontName)
        data = data[:m.start(1)] + target + data[m.end(1):]

    # fix em square
    if fdDict.OrigEmSqUnits:
        m = re.search(r"/FontMatrix\s+\[.+?\]\s+def", data)
        if not m:
            raise FontParseError("Failed to find FontMatrix in input font! "
                                 "%s" % tempPath)
        emUnits = eval(fdDict.OrigEmSqUnits)
        a = 1.0 / emUnits
        target = "/FontMatrix [%s 0 0 %s 0 0] def" % (a, a)
        data = data[:m.start()] + target + data[m.end():]

    # fix StemSnapH. Remove StemSnapH if fdDict.StemSnapH
    # is not defined, else set it.
    m = re.search(r"/StemSnapH\s+\[.+?\]\s+def", data)
    if fdDict.DominantH:
        target = "/StemSnapH %s def" % fdDict.DominantH
        data = data[:m.start()] + target + data[m.end():]
        insertIndex = m.start() + len(target)
    else:
        if m:
            data = data[:m.start()] + data[m.end():]

    # fix StemSnapV.  Remove StemSnapV entry if fdDict.StemSnapV
    # is not defined, else set it.
    m = re.search(r"/StemSnapV\s+\[.+?\]\s+def", data)
    if fdDict.DominantV:
        target = "/StemSnapV %s def" % (fdDict.DominantV)
        data = data[:m.start()] + target + data[m.end():]
        insertIndex = m.start() + len(target)
    else:
        if m:
            data = data[:m.start()] + data[m.end():]

    # LanguageGroup. Remove LanguageGroup entry if
    # fdDict.LanguageGroup is not defined, else set it.
    if fdDict.LanguageGroup:
        m = re.search(r"/LanguageGroup\s+\d+\s+def", data)
        if not m:
            target = "%s/LanguageGroup %s def" % (newLine,
                                                  fdDict.LanguageGroup)
            data = data[:insertIndex] + data[insertIndex] + target + \
                data[insertIndex:]
        else:
            data = data[:m.start()] + target + data[m.end():]
        target = "/LanguageGroup %s def" % fdDict.LanguageGroup
    else:
        m = re.search(r"/LanguageGroup\s+\d+\s+def", data)
        if m:
            data = data[:m.start()] + data[m.end():]

    # Fix BlueValues. Must be present.
    if fdDict.BlueValues:
        m = re.search(r"/BlueValues\s+\[.+?\]\s+def", data)
        if not m:
            raise FontParseError("Failed to find BlueValues in input font! "
                                 "%s" % tempPath)
        target = "/BlueValues %s def" % fdDict.BlueValues
        data = data[:m.start()] + target + data[m.end():]
        insertIndex = m.start() + len(target)

    # Fix OtherBlues, if present. Remove if there are no OtherBlues entry.
    m = re.search(r"/OtherBlues\s+\[.+?\]\s+def", data)
    if fdDict.OtherBlues:
        if not m:
            target = "%s/OtherBlues %s def" % (newLine, fdDict.OtherBlues)
            data = data[:insertIndex] + target + data[insertIndex:]
        else:
            target = "/OtherBlues %s def" % fdDict.OtherBlues
            data = data[:m.start()] + target + data[m.end():]
    else:
        if m:
            data = data[:m.start()] + data[m.end():]

    fp = open(txtPath, "wt")
    fp.write(data)
    fp.close()

    command = "type1 \"%s\" \"%s\" 2>&1" % (txtPath, tempPath)
    log = FDKUtils.runShellCmd(command)
    if log:
        print(log)

    if not debug:
        os.remove(txtPath)
    return


def makeTempFonts(fontDictList, glyphSetList, fdGlyphDict, inputPath):
    fontList = []
    setIndex = 0
    for glyphList in glyphSetList:
        if not glyphList:
            continue
        setIndex += 1
        arg = ",".join(glyphList)
        tempPath = "%s.temp.%s.pfa" % (inputPath, setIndex)
        command = "tx -t1 -g \"%s\" \"%s\" \"%s\" 2>&1" % (arg, inputPath,
                                                           tempPath)
        log = FDKUtils.runShellCmd(command)
        if log:
            print("Have log output in subsetting command for %s to %s with "
                  "%s glyphs." % (inputPath, tempPath, len(glyphList)))
            print(log)
        fdIndex = fdGlyphDict[glyphList[0]][0]

        fdDict = fontDictList[fdIndex]
        fixFontDict(tempPath, fdDict)
        fontList.append(tempPath)
        if debug:
            print(glyphList[0], len(glyphList), fdGlyphDict[glyphList[0]],
                  tempPath)
    return fontList


def makeCIDFontInfo(fontPath, cidfontinfoPath):
    cfiDict = {}
    for key in kRequiredCIDFontInfoFields + kOptionalFields:
        cfiDict[key] = None
    cfiDict["Weight"] = "(Regular)"
    cfiDict["AdobeCopyright"] = "0"

    # get regular FontDict.
    command = "tx -0 \"%s\" 2>&1" % fontPath
    report = FDKUtils.runShellCmd(command)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError("Failed to dump font dict using tx from "
                                 "font '%s'" % fontPath)

    for entry in txFields:
        match = re.search(entry[0] + "\s+(.+?)[\r\n]", report)
        if match:
            entry[2] = match.group(1)

    cfiDict["Registry"] = "Adobe"
    cfiDict["Ordering"] = "Identity"
    cfiDict["Supplement"] = "0"

    for entry in txFields:
        if entry[2]:
            cfiDict[entry[1]] = entry[2]
        elif entry[1] in kRequiredCIDFontInfoFields:
            print("Error: did not find required info '%s' in tx dump of "
                  "font '%s'." % (entry[1], fontPath))
    try:
        fp = open(cidfontinfoPath, "wt")
        for key in kCIDFontInfokeyList:
            value = cfiDict[key]
            if value is None:
                continue
            if value[0] == "\"":
                value = "(" + value[1:-1] + ")"
            string = "%s\t%s" % (key, value)
            fp.write(string + os.linesep)
            fp.write(os.linesep)
        fp.close()
    except (IOError, OSError):
        msg = "Error. Could not open and write file '%s'" % cidfontinfoPath
        raise FontInfoParseError(msg)
    return


def makeGAFile(gaPath, fontPath, glyphList, fontDictList, fdGlyphDict,
               removeNotdef):
    lineList = [""]
    setList = getGlyphList(fontPath, removeNotdef)
    if not setList:
        print(setList, fontPath, removeNotdef)
        raise FontParseError("Program Error - call Read Roberts. "
                             "In makeGAFile.")

    fdIndex = fdGlyphDict[setList[0]][0]  # [fdIndex value, gi]
    fdDict = fontDictList[fdIndex]
    langGroup = ""  # values for default dict is empty string.
    dictName = ""
    if fdDict:
        langGroup = fdDict.LanguageGroup
        if langGroup is None:
            langGroup = " 0"
        else:
            langGroup = " %s" % (langGroup)
        dictName = "%s_%s" % (fdDict.FontName, fdDict.DictName)
    for glyphTag in setList:
        gid = glyphList.index(glyphTag)
        lineList.append("%s\t%s" % (gid, glyphTag))
    lineList.append("")
    gaText = "mergeFonts %s%s%s" % (dictName, langGroup,
                                    os.linesep.join(lineList))
    gf = open(gaPath, "wb")
    gf.write(gaText)
    gf.close()
    return


def mergeFonts(inputFontPath, outputPath, fontList, glyphList, fontDictList,
               fdGlyphDict):
    cidfontinfoPath = "%s.temp.cidfontinfo" % (inputFontPath)
    makeCIDFontInfo(inputFontPath, cidfontinfoPath)
    lastFont = ""
    tempfileList = []
    i = 0
    if debug:
        print("Merging temp fonts:", end=' ')
    for fontPath in fontList:
        print(".", end=' ')
        sys.stdout.flush()
        gaPath = fontPath + ".ga"
        dstPath = "%s.temp.merge.%s" % (inputFontPath, i)
        removeNotdef = i != 0
        makeGAFile(gaPath, fontPath, glyphList, fontDictList, fdGlyphDict,
                   removeNotdef)
        if lastFont:
            command = 'mergeFonts -std -cid "%s" "%s" "%s" "%s" "%s" 2>&1' % (
                cidfontinfoPath, dstPath, lastFont, gaPath, fontPath)
        else:
            command = 'mergeFonts -std -cid "%s" "%s" "%s" "%s" 2>&1' % (
                cidfontinfoPath, dstPath, gaPath, fontPath)
        log = FDKUtils.runShellCmd(command)
        if debug:
            print(command)
            print(log)
        if "rror" in log:
            msg = "Error merging font %s. Log: %s." % (fontPath, log)
            raise FontInfoParseError(msg)
        i += 1
        lastFont = dstPath
        tempfileList.append(gaPath)
        tempfileList.append(dstPath)

    print("")
    os.rename(dstPath, outputPath)
    if not debug:
        for path in tempfileList:
            if os.path.exists(path):
                os.remove(path)
        os.remove(cidfontinfoPath)
    return


def convertFontToCID(inputPath, outputPath):
    # Check the fontinfo file, and add any other font dicts
    srcFontInfo = os.path.dirname(inputPath)
    srcFontInfo = os.path.join(srcFontInfo, "fontinfo")
    if os.path.exists(srcFontInfo):
        fi = open(srcFontInfo, "rU")
        fontInfoData = fi.read()
        fi.close()
    else:
        return

    # Start with a uninitialized entry for the default FD Dict 0.
    fontDictList = [FDDict()]
    glyphList = getGlyphList(inputPath)
    fontBBox = getFontBBox(inputPath)
    fontName = getFontName(inputPath)
    blueFuzz = getBlueFuzz(inputPath)
    maxY = fontBBox[3]
    minY = fontBBox[1]
    fdGlyphDict, fontDictList, _ = parseFontInfoFile(
        fontDictList, fontInfoData, glyphList, maxY, minY, fontName, blueFuzz)

    glyphSetList = makeSortedGlyphSets(glyphList, fdGlyphDict)
    fontList = makeTempFonts(fontDictList, glyphSetList, fdGlyphDict,
                             inputPath)
    mergeFonts(inputPath, outputPath, fontList, glyphList, fontDictList,
               fdGlyphDict)
    # print(fdGlyphDict)
    # print(fontDictList)
    # print(fontList)
    if not debug:
        for fontPath in fontList:
            os.remove(fontPath)


def mergeFontToCFF(srcPath, outputPath, doSubr):
    # We assume that srcPath is a type 1 font,and outputPath is an OTF font.

    # First, convert src font to cff, and subroutinize it if so requested.
    tempPath = srcPath + ".temp.cff"
    subrArg = ""
    if doSubr:
        subrArg = " +S"
    command = "tx -cff +b%s \"%s\" \"%s\" 2>&1" % (subrArg, srcPath, tempPath)
    report = FDKUtils.runShellCmd(command)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError(
            "Failed to convert font '%s' to CFF." % srcPath)

    # Now merge it into the output file.
    command = "sfntedit -a CFF=\"%s\" \"%s\" 2>&1" % (tempPath, outputPath)
    report = FDKUtils.runShellCmd(command)
    if not debug:
        if os.path.exists(tempPath):
            os.remove(tempPath)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError(
            "Failed to convert font '%s' to CFF." % srcPath)


def main():
    inputPath = sys.argv[1]
    outputPath = inputPath + ".temp.cid"
    convertFontToCID(inputPath, outputPath)


if __name__ == '__main__':
    main()
