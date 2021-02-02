# Copyright 2014 Adobe. All rights reserved.

"""
convertfonttocid.py. v 1.13.2 Jul 30 2020

Convert a Type 1 font to CID, given multiple hint dict defs in the
"fontinfo" file. See psautohint help, with the "--doc-fddict" option,
or the makeotf user guide for details on this format. The output file
produced by convertFontToCID() is a CID Type 1 font, no matter what
the input is.

PROCEDURE:
1. convertFontToCID()
   - read 'fontinfo' file
   - getGlyphList(): get list of glyph names (via 'tx -dump -4')
   - getFontBBox(): get FontBBox (via 'tx -mtx -2')
   - getFontName(): get FontName (via 'tx -dump -0')
   - getBlueFuzz(): get BlueFuzz (via 'tx -dump -0')

2. parseFontInfoFile()
   - parse 'fontinfo' file
   - collect all FDDict and corresponding GlyphSet

3. makeSortedGlyphLists()
   - make a list containing a list of glyph names for each FDDict;
     the list is sorted by FDDict index.
     NOTE: all glyphs in a list are contiguous and ordered by original GID,
           and belong to the same FDDict. This allows the glyphs to be
           merged into a single CID font that will have the same glyph
           order as the original font.

4. makeTempFonts()
   - generate a Type1 font (via 'tx -t1 -g ...') for each FDDict;
     the fonts are subsets of the original font.
   - fixFontDict():
     - convert each name-keyed Type1 font to text (via 'detype1')
     - fix FontName
     - fix FontMatrix
     - fix StemSnapH
     - fix StemSnapV
     - fix LanguageGroup
     - fix BlueValues
     - fix OtherBlues
     - convert text font back to Type1 (via 'type1')

5. merge_fonts()
   - makeCIDFontInfo():
     - get the original font's FontDict (via 'tx -0')
     - assemble and save a temporary 'cidfontinfo' file
   - makeGAFile():
     - generate temporary glyph alias file for each FDDict
   - convert each name-keyed Type1 font to CID-keyed (via 'mergefonts')
   - merge all CID-keyed fonts (via 'mergefonts')

"""

import os
import re
import sys

from afdko import fdkutils

__version__ = "1.13.1"

# Tokens seen in font info file that are not part
# of a FDDict or GlyphSet definition.
kBeginToken = "begin"
kEndToken = "end"
kFDDictToken = "FDDict"
kGlyphSetToken = "GlyphSet"

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

TX_FIELDS = [  # [field name in tx, key name in cidfontinfo, value]
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


class FontInfoParseError(Exception):
    pass


class FontParseError(Exception):
    pass


class FDDict(object):

    def __init__(self):
        self.DictName = None
        for key in kFDDictKeys:
            setattr(self, key, None)
        self.FlexOK = "true"

    def __repr__(self):
        fddict = {}
        for key, val in vars(self).items():
            if val is None:
                continue
            fddict[key] = val
        return "<%s '%s' %s>" % (
            self.__class__.__name__, fddict.get('DictName', 'no name'), fddict)

    def getFontInfo(self):
        fiList = []
        for key, val in vars(self).items():
            if val is not None:
                fiList.append("%s %s" % (key, val))
        return "\n".join(fiList)

    def buildBlueLists(self):
        baseline_overshoot = getattr(self, 'BaselineOvershoot', None)
        if baseline_overshoot is None:
            print("Error: FDDict definition %s is missing the "
                  "BaselineYCoord/BaselineOvershoot values. These are "
                  "required." % self.DictName)
        elif int(baseline_overshoot) > 0:
            print("Error: The BaselineYCoord/BaselineOvershoot in FDDict "
                  "'%s' must be a bottom zone therefore BaselineOvershoot "
                  "must be negative, not positive." % self.DictName)

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
                    width = getattr(self, key)
                    if width is not None:
                        width = int(width)
                        baseName = key[:-len("Overshoot")]
                        if key == "BaselineOvershoot":
                            tempKey = "BaselineYCoord"
                            zonePos = int(getattr(self, tempKey))
                        else:
                            for posSuffix in ["", "Height", "Baseline"]:
                                tempKey = "%s%s" % (baseName, posSuffix)
                                try:
                                    zonePos = int(getattr(self, tempKey))
                                    break
                                except AttributeError:
                                    continue
                        if zonePos is None:
                            raise FontInfoParseError(
                                "Failed to find fontinfo FDDict %s "
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
                            if i == 1:
                                print("Error: FontDict %s. Zone %s is a "
                                      "bottom zone, and so the width (%s) "
                                      "must be negative.." % (
                                          self.DictName, tempKey, width))
                        bluePairList.append((topPos, bottomPos, tempKey,
                                             self.DictName, isBottomZone))

            if bluePairList:
                bluePairList.sort()
                prevPair = bluePairList[0]
                zoneBuffer = 2 * getattr(self, 'BlueFuzz', 0) + 1
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
                setattr(self, pairFieldName, bluePairList)
                bluesList = []
                for pairEntry in bluePairList:
                    bluesList.append(pairEntry[1])
                    bluesList.append(pairEntry[0])
                bluesList = [str(val) for val in bluesList]
                bluesList = "[%s]" % (" ".join(bluesList))
                setattr(self, fieldName, bluesList)


def parseFontInfoFile(fontDictList, fontInfoData, glyphList, maxY, minY,
                      fontName, blueFuzz):
    """
    fontDictList may or may not already contain a font dict taken from
    the source font top FontDict.

    Returns fdGlyphDict, fontDictList, finalFDict

    fdGlyphDict: { '.notdef': [0, 0],  # 'g_name': [FDDict_index, g_index]
                   'negative': [1, 2],
                   'a': [2, 1]}

    fontDictList: [<FDDict 'No Alignment Zones' {
                           'FontName': 'SourceSans-Test', 'BlueFuzz': 0,
                           'CapHeight': 760, 'CapOvershoot': 0,
                           'FlexOK': 'true',
                           'BlueValues': '[-112 -112 760 760]',
                           'BlueValuesPairs': [(-112, -112,
                                                'BaselineYCoord',
                                                'No Alignment Zones', 0),
                                               (760, 760,
                                                'CapHeight',
                                                'No Alignment Zones', 0)],
                           'BaselineOvershoot': 0,
                           'DictName': 'No Alignment Zones',
                           'BaselineYCoord': -112}>,
                   <FDDict 'OTHER' {
                           'FontName': 'SourceSans-Test', 'BlueFuzz': 0,
                           'DominantH': '[68]', 'CapHeight': '656',
                           'DominantV': '[86]', 'CapOvershoot': '12',
                           'BlueValues': '[-12 0 656 668]', 'FlexOK': 'false',
                           'BlueValuesPairs': [(0, -12,
                                                'BaselineYCoord','OTHER', 1),
                                               (668, 656,
                                                'CapHeight', 'OTHER', 0)],
                           'BaselineOvershoot': '-12', 'DictName': 'OTHER',
                           'BaselineYCoord': '0'}>,
                   <FDDict 'LOWERCASE' {
                           'FontName': 'SourceSans-Test', 'BlueFuzz': 0,
                           'AscenderHeight': '712', 'DominantH': '[68]',
                           'DescenderOvershoot': '-12', 'DominantV': '[82]',
                           'BlueValues': '[-12 0 486 498 712 724]',
                           'DescenderHeight': '-205', 'LcHeight': '486',
                           'FlexOK': 'false', 'AscenderOvershoot': '12',
                           'LcOvershoot': '12',
                           'BaselineOvershoot': '-12',
                           'OtherBlueValuesPairs': [(-205, -217,
                                                     'DescenderHeight',
                                                     'LOWERCASE', 1)],
                           'BlueValuesPairs': [(0, -12,
                                                'BaselineYCoord',
                                                'LOWERCASE', 1),
                                               (498, 486,
                                                'LcHeight', 'LOWERCASE', 0),
                                               (724, 712,
                                                'AscenderHeight',
                                                'LOWERCASE', 0)],
                           'DictName': 'LOWERCASE',
                           'OtherBlues': '[-217 -205]',
                           'BaselineYCoord': '0'}>]
    """
    # The map of glyph names to font dict: the index into fontDictList.
    fdGlyphDict = {}

    # The user-specified set of blue values to write into the output font,
    # some sort of merge of the individual font dicts. May not be supplied.
    finalFDict = None

    # Get rid of comments.
    data = re.sub(r"#[^\r\n]+[\r\n]", "", fontInfoData)

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
                    if dictName == "FinalFont":
                        # This dict is NOT used to hint any glyphs;
                        # it is used to supply the merged alignment zones
                        # and stem widths for the final font.
                        finalFDict = fdDict
                    else:
                        # save dict and FDIndex
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
                setattr(fdDict, dictKeyWord, value)
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
                        setattr(fdDict, token, value)
                else:
                    raise FontInfoParseError(
                        "FDDict key \"%s\" in fdDict named \"%s\" is not "
                        "recognized." % (token, dictName))

    if lenSrcFontDictList != len(fontDictList):
        # There are some FDDict definitions. This means that we need
        # to fix the default fontDict, inherited from the source font,
        # so that it has blues zones that will not affect hinting,
        # e.g outside of the Font BBox. We do this because if there are
        # glyphs which are not assigned to a user specified font dict,
        # it is because it doesn't make sense to provide alignment zones
        # for the glyph. Since psautohint does require at least one bottom zone
        # and one top zone, we add one bottom and one top zone that are
        # outside the font BBox, so that hinting won't be affected by them.
        # NOTE: The FDDict receives a special name "No Alignment Zones" which
        # psautohint recognizes.
        defaultFDDict = fontDictList[0]

        for key in kBlueValueKeys + kOtherBlueValueKeys:
            setattr(defaultFDDict, key, None)

        defaultFDDict.BaselineYCoord = minY - 100
        defaultFDDict.BaselineOvershoot = 0
        defaultFDDict.CapHeight = maxY + 100
        defaultFDDict.CapOvershoot = 0
        defaultFDDict.BlueFuzz = 0
        defaultFDDict.DictName = "No Alignment Zones"
        defaultFDDict.FontName = fontName
        defaultFDDict.buildBlueLists()

        for gi, gname in enumerate(glyphList):
            if gname not in fdGlyphDict:
                fdGlyphDict[gname] = [0, gi]

    # hard code the '.notdef' to the default FDDict;
    # this is only necessary when fdGlyphDict is not empty.
    # NOTE: at this point it's guaranteed that the font has a glyph
    #       named '.notdef' at GID 0.
    #       If the '.notdef' was included in one of the GlyphSet definitions
    #       it will be associated with an FDDict other than the default.
    if fdGlyphDict:
        fdGlyphDict['.notdef'] = [0, 0]

    return fdGlyphDict, fontDictList, finalFDict


def mergeFDDicts(prevDictList, privateDict):
    """
    Used by beztools & ufotools.
    """
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
            bluePairList = getattr(prefDDict, bluePairName)
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
            dList = getattr(prefDDict, stemFieldName)
            stemDict = stemDictList[wi]
            if dList is not None:
                dList = dList[1:-1]  # remove the braces
                dList = dList.split()
                dList = [int(val) for val in dList]
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
        zoneList = sorted(zoneDict.keys())
        if not zoneList:
            continue

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

        stemList = sorted(stemDict.keys())
        if not stemList:
            continue

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


def getGlyphList(fPath, removeNotdef=False, original_font=False):
    command = "tx -dump -4 \"%s\" 2>&1" % fPath
    data = fdkutils.runShellCmd(command)
    if not data:
        raise FontParseError("Error: Failed running 'tx -dump -4' on file "
                             "%s" % fPath)

    # initial newline keeps us from picking up the first column header line
    nameList = re.findall(r"[\r\n]glyph.+?{([^,]+),", data)
    if not nameList:
        raise FontParseError("Error: Failed getting glyph names from file %s "
                             "using tx." % fPath)

    if original_font:
        # make sure the original font has a .notdef glyph
        # and that it's the first glyph
        if '.notdef' not in nameList:
            raise FontParseError("Error: The font file %s does not have a "
                                 "'.notdef'." % fPath)
        elif nameList[0] != '.notdef':
            raise FontParseError("Error: '.notdef' is not the first glyph of "
                                 "font file %s" % fPath)

    if removeNotdef:
        nameList.remove(".notdef")

    return nameList


def getFontBBox(fPath):
    command = "tx -mtx -2  \"%s\" 2>&1" % fPath
    data = fdkutils.runShellCmd(command)
    if not data:
        raise FontInfoParseError("Error: Failed getting log from tx from %s, "
                                 "when trying to get FontBBox." % fPath)

    m = re.search(r"bbox[^{]+{([^}]+)}", data)
    if not m:
        print(data)
        raise FontInfoParseError("Error: Failed finding FontBBox in tx log "
                                 "from %s." % fPath)
    fontBBox = m.group(1).split(",")
    fontBBox = [int(val) for val in fontBBox]
    return fontBBox


def getFontName(fPath):
    command = "tx -dump -0 \"%s\" 2>&1" % fPath
    data = fdkutils.runShellCmd(command)
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
    command = "tx -dump -0 \"%s\" 2>&1" % fPath
    data = fdkutils.runShellCmd(command)
    if not data:
        raise FontInfoParseError("Error: Failed getting log from tx from %s, "
                                 "when trying to get FontName." % fPath)

    m = re.search(r"BlueFuzz\s+(\d+(?:\.\d+)*)", data)
    if m:
        blueFuzz = int(m.group(1))
    return blueFuzz


def makeSortedGlyphLists(glyphList, fdGlyphDict):
    """
    Returns a list containing lists of glyph names (one for each FDDict).

    glyphList: list of all glyph names in the font

    fdGlyphDict: {'a': [2, 1], 'negative': [1, 2], '.notdef': [0, 0]}
                 keys: glyph names
                 values: [FDDict_index, glyph_index]
    """
    glyphSetList = [[]]
    for glyph_name in glyphList:
        try:
            fddict_index = fdGlyphDict[glyph_name][0]
            while len(glyphSetList) <= fddict_index:
                # glyphSetList does not have enough empty containers/lists
                glyphSetList.append([])
            glyphSetList[fddict_index].append(glyph_name)
        except KeyError:
            # the glyph name is not in the FDDict;
            # assign that glyph to the default FDDict (i.e. index 0)
            glyphSetList[0].append(glyph_name)

    if len(glyphSetList) < 2:
        print("Warning: There is only one hint dict. Maybe the fontinfo file "
              "has not been edited to add hint dict info.")

    return glyphSetList


def fixFontDict(tempPath, fdDict):
    txtPath = fdkutils.get_temp_file_path()
    command = "detype1 \"%s\" \"%s\" 2>&1" % (tempPath, txtPath)
    log = fdkutils.runShellCmd(command)
    if log:
        print(log)

    with open(txtPath, "r", encoding='utf-8') as fp:
        data = fp.read()

    # fix font name. We always search for it, as it is always present,
    # and we can use the following white space to get the file new line.
    m = re.search(r"(/FontName\s+/\S+\s+def)(\s+)", data)
    newLine = m.group(2)
    if not m:
        raise FontParseError("Failed to find FontName in input font! "
                             "%s" % tempPath)
    if fdDict.FontName:
        target = "/FontName /%s def" % fdDict.FontName
        data = data[:m.start(1)] + target + data[m.end(1):]

    # fix em square
    if fdDict.OrigEmSqUnits:
        m = re.search(r"/FontMatrix\s+\[.+?\]\s+def", data)
        if not m:
            raise FontParseError("Failed to find FontMatrix in input font! "
                                 "%s" % tempPath)
        emUnits = fdDict.OrigEmSqUnits
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
        target = "/StemSnapV %s def" % fdDict.DominantV
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
            target = "/LanguageGroup %s def" % fdDict.LanguageGroup
            data = data[:m.start()] + target + data[m.end():]
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

    with open(txtPath, "w") as fp:
        fp.write(data)

    command = "type1 \"%s\" \"%s\" 2>&1" % (txtPath, tempPath)
    log = fdkutils.runShellCmd(command)
    if log:
        print(log)


def makeTempFonts(fontDictList, glyphSetList, fdGlyphDict, inputPath):
    fontList = []
    for glyphList in glyphSetList:
        arg = ",".join(glyphList)
        tempPath = fdkutils.get_temp_file_path()
        command = "tx -t1 -g \"%s\" \"%s\" \"%s\" 2>&1" % (
            arg, inputPath, tempPath)
        log = fdkutils.runShellCmd(command)
        if log:
            print("Have log output in subsetting command for %s to %s with "
                  "%s glyphs." % (inputPath, tempPath, len(glyphList)))
            print(log)

        try:
            fdIndex = fdGlyphDict[glyphList[0]][0]
        except KeyError:
            # 'fontinfo' file was not provided;
            # assign list of glyphs to default FDDict
            fdIndex = 0

        fdDict = fontDictList[fdIndex]
        fixFontDict(tempPath, fdDict)
        fontList.append(tempPath)

    return fontList


def makeCIDFontInfo(fontPath, cidfontinfoPath):
    cfiDict = {}
    for key in kCIDFontInfokeyList:
        cfiDict[key] = None
    cfiDict["Weight"] = "(Regular)"
    cfiDict["AdobeCopyright"] = "0"

    # get regular FontDict.
    command = "tx -0 \"%s\" 2>&1" % fontPath
    report = fdkutils.runShellCmd(command)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError("Failed to dump font dict using tx from "
                                 "font '%s'" % fontPath)

    for entry in TX_FIELDS:
        match = re.search(entry[0] + r"\s+(.+?)[\r\n]", report)
        if match:
            entry[2] = match.group(1)

    cfiDict["Registry"] = "Adobe"
    cfiDict["Ordering"] = "Identity"
    cfiDict["Supplement"] = "0"

    for entry in TX_FIELDS:
        if entry[2]:
            cfiDict[entry[1]] = entry[2]
        elif entry[1] in kRequiredCIDFontInfoFields:
            print("Error: did not find required info '%s' in tx dump of "
                  "font '%s'." % (entry[1], fontPath))
    try:
        with open(cidfontinfoPath, "w") as fp:
            for key in kCIDFontInfokeyList:
                value = cfiDict[key]
                if value is None:
                    continue
                if value[0] == "\"":
                    value = "(" + value[1:-1] + ")"
                string = "%s\t%s\n" % (key, value)
                fp.write(string)
    except (OSError):
        raise FontInfoParseError(
            "Error. Could not open and write file '%s'" % cidfontinfoPath)


def makeGAFile(gaPath, fontPath, glyphList, fontDictList, fdGlyphDict,
               removeNotdef):
    """
    Creates a glyph alias file for each FDDict.
    These files will be used by 'mergefonts' tool.
    For documentation on the format of this file, run 'mergefonts -h'.
    """
    glyph_list = getGlyphList(fontPath, removeNotdef)

    try:
        fdIndex = fdGlyphDict[glyph_list[0]][0]  # [fdIndex value, gi]
    except KeyError:
        fdIndex = 0

    fdDict = fontDictList[fdIndex]
    lineList = [""]

    lang_group = fdDict.LanguageGroup
    if lang_group is None:
        langGroup = " 0"
    else:
        langGroup = " %s" % lang_group

    dictName = "%s_%s" % (fdDict.FontName, fdDict.DictName)

    for glyph_name in glyph_list:
        gid = glyphList.index(glyph_name)
        lineList.append("%s\t%s" % (gid, glyph_name))

    lineList.append("")
    gaText = "mergefonts %s%s%s" % (dictName, langGroup, '\n'.join(lineList))

    with open(gaPath, "w") as gf:
        gf.write(gaText)


def merge_fonts(inputFontPath, outputPath, fontList, glyphList, fontDictList,
                fdGlyphDict):
    cidfontinfoPath = fdkutils.get_temp_file_path()
    makeCIDFontInfo(inputFontPath, cidfontinfoPath)
    lastFont = ""
    dstPath = ''

    for i, fontPath in enumerate(fontList):
        gaPath = fdkutils.get_temp_file_path()
        dstPath = fdkutils.get_temp_file_path()
        removeNotdef = i != 0
        makeGAFile(gaPath, fontPath, glyphList, fontDictList, fdGlyphDict,
                   removeNotdef)
        if lastFont:
            command = 'mergefonts -std -cid "%s" "%s" "%s" "%s" "%s" 2>&1' % (
                cidfontinfoPath, dstPath, lastFont, gaPath, fontPath)
        else:
            command = 'mergefonts -std -cid "%s" "%s" "%s" "%s" 2>&1' % (
                cidfontinfoPath, dstPath, gaPath, fontPath)
        log = fdkutils.runShellCmd(command)
        if "rror" in log:
            raise FontInfoParseError(
                "Error running command '%s'\nLog: %s" % (command, log))

        lastFont = dstPath

    if os.path.exists(outputPath):
        os.remove(outputPath)
        os.rename(dstPath, outputPath)


def convertFontToCID(inputPath, outputPath, fontinfoPath=None):
    """
    Takes in a path to the font file to convert, a path to save the result,
    and an optional path to a '(cid)fontinfo' file.
    """
    if fontinfoPath and os.path.exists(fontinfoPath):
        with open(fontinfoPath, "r", encoding='utf-8') as fi:
            fontInfoData = fi.read()
    else:
        fontInfoData = ''

    glyphList = getGlyphList(inputPath, False, True)
    fontBBox = getFontBBox(inputPath)
    fontName = getFontName(inputPath)
    blueFuzz = getBlueFuzz(inputPath)
    maxY = fontBBox[3]
    minY = fontBBox[1]

    # Start with an uninitialized entry for the default FDDict 0
    fontDictList = [FDDict()]

    fdGlyphDict, fontDictList, _ = parseFontInfoFile(
        fontDictList, fontInfoData, glyphList, maxY, minY, fontName, blueFuzz)

    glyphSetList = makeSortedGlyphLists(glyphList, fdGlyphDict)

    fontList = makeTempFonts(fontDictList, glyphSetList, fdGlyphDict,
                             inputPath)

    merge_fonts(inputPath, outputPath, fontList, glyphList, fontDictList,
                fdGlyphDict)


def mergeFontToCFF(srcPath, outputPath, doSubr):
    """
    Used by makeotf.
    Assumes srcPath is a type 1 font,and outputPath is an OTF font.
    """
    # First, convert src font to cff, and subroutinize it if so requested.
    tempPath = fdkutils.get_temp_file_path()
    subrArg = ""
    if doSubr:
        subrArg = " +S"
    command = "tx -cff +b%s \"%s\" \"%s\" 2>&1" % (subrArg, srcPath, tempPath)
    report = fdkutils.runShellCmd(command)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError(
            "Failed to run 'tx -cff +b' on file %s" % srcPath)

    # Now merge it into the output file.
    command = "sfntedit -a CFF=\"%s\" \"%s\" 2>&1" % (tempPath, outputPath)
    report = fdkutils.runShellCmd(command)
    if ("fatal" in report) or ("error" in report):
        print(report)
        raise FontInfoParseError(
            "Failed to run 'sfntedit -a CFF=' on file %s" % srcPath)


def main():
    args = sys.argv

    if len(args) >= 2:
        inputPath = args[1]
        outputPath = fdkutils.get_temp_file_path()

        fontinfoPath = None
        if len(args) == 3:
            fontinfoPath = args[2]

        convertFontToCID(inputPath, outputPath, fontinfoPath)

        save_path = f'{os.path.splitext(inputPath)[0]}-CID.ps'

        if os.path.isfile(outputPath):
            os.rename(outputPath, save_path)
    else:
        print('ERROR: Missing path to font to convert.')


if __name__ == '__main__':
    main()
