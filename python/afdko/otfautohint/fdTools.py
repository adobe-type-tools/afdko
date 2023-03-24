# Copyright 2014 Adobe. All rights reserved.

"""
Tools for processing "fontinfo" files. See "otfautohint --doc-fddict", or the
MakeOTF user guide for details on this format.

The "fontinfo" file is expected to be in the same directory as the input
font file.
"""

import logging
import re
import numbers
import sys
import os


log = logging.getLogger(__name__)

# Tokens seen in font info file that are not
# part of a FDDict or GlyphSet definition.
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

kFamilyValueKeys = ['Family' + i for i in kBlueValueKeys]

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

kOtherFamilyValueKeys = ['Family' + i for i in kOtherBlueValueKeys]

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

kFontDictBluePairsName = "BlueValuesPairs"
kFontDictOtherBluePairsName = "OtherBlueValuesPairs"
kFontDictFamilyPairsName = "FamilyValuesPairs"
kFontDictOtherFamilyPairsName = "OtherFamilyValuesPairs"

kRunTimeFDDictKeys = [
    "DictName",
    kFontDictBluePairsName,
    kFontDictOtherBluePairsName,
    kFontDictFamilyPairsName,
    kFontDictOtherFamilyPairsName,
]

kFDDictKeys = (kOtherFDDictKeys +
               kBlueValueKeys +
               kOtherBlueValueKeys +
               kFamilyValueKeys +
               kOtherFamilyValueKeys +
               kRunTimeFDDictKeys)
kFontInfoKeys = (kOtherFDDictKeys +
                 kBlueValueKeys +
                 kOtherBlueValueKeys +
                 kFamilyValueKeys +
                 kOtherFamilyValueKeys +
                 ["StemSnapH", "StemSnapV"])


class FontInfoParseError(ValueError):
    pass


class FDDict:
    def __init__(self, fdIndex, dictName=None, fontName=None):
        self.fdIndex = fdIndex
        for key in kFDDictKeys:
            setattr(self, key, None)
        if dictName is not None:
            self.DictName = dictName
        else:
            self.DictName = "FDArray[%s]" % fdIndex
        self.FontName = fontName
        self.FlexOK = True
        setattr(self, kFontDictBluePairsName, [])
        setattr(self, kFontDictOtherBluePairsName, [])
        setattr(self, kFontDictFamilyPairsName, [])
        setattr(self, kFontDictOtherFamilyPairsName, [])

    def __str__(self):
        a = ''
        for k, v in vars(self).items():
            if k not in kFontInfoKeys or v is None:
                continue
            if k == 'FontName':
                continue
            if isinstance(v, list):
                v = '[%s]' % ' '.join((str(i) for i in v))
            elif v is False:
                v = 'false'
            elif v is True:
                v = 'true'
            elif not isinstance(v, str):
                v = str(v)
            a += "%s %s\n" % (k, v)
        return a

    def setInfo(self, key, value):
        if key == 'LanguageGroup':
            if ((isinstance(value, numbers.Number) and value == 1) or
                    value == '1'):
                value = 1
            else:
                value = 0
        elif key in ('DominantV', 'DominantH'):
            if type(value) == list:
                value = [int(v) for v in value]
            else:
                value = [int(value)]
        elif key == 'FlexOK':
            if value is None:
                value = True        # default
            elif value == 'false':
                value = False
            else:
                value = bool(value)
        elif key in ('VCounterChars', 'HCounterChars'):
            pass  # already formatted in __main__::_parse_fontinfo_file
        elif key in ('FontName', 'DictName'):
            pass  # keep the string
        elif value is not None:
            value = int(value)

        setattr(self, key, value)

    def buildBlueLists(self):
        log.info("Building BlueValue lists for FDDict %s" % self.DictName)
        if self.BaselineOvershoot is None:
            raise FontInfoParseError(
                "FDDict definition %s is missing the BaselineYCoord/"
                "BaselineOvershoot values. These are required." %
                self.DictName)
        elif int(self.BaselineOvershoot) > 0:
            raise FontInfoParseError(
                "The BaselineYCoord/BaselineOvershoot in FDDict definition %s "
                "must be a bottom zone - the BaselineOvershoot must be "
                "negative, not positive." % self.DictName)

        blueKeyList = [kBlueValueKeys,
                       kOtherBlueValueKeys,
                       kFamilyValueKeys,
                       kOtherFamilyValueKeys]
        bluePairListNames = [kFontDictBluePairsName,
                             kFontDictOtherBluePairsName,
                             kFontDictFamilyPairsName,
                             kFontDictOtherFamilyPairsName]
        for i in range(4):
            keyList = blueKeyList[i]
            pairFieldName = bluePairListNames[i]
            bluePairList = []
            for key in keyList:
                if key.endswith("Overshoot"):
                    width = getattr(self, key)
                    if width is not None:
                        baseName = key[:-len("Overshoot")]
                        zonePos = None
                        if key == "BaselineOvershoot":
                            zonePos = self.BaselineYCoord
                            tempKey = "BaselineYCoord"
                        elif key == "FamilyBaselineOvershoot":
                            zonePos = self.FamilyBaselineYCoord
                            tempKey = "FamilyBaselineYCoord"
                        else:
                            for posSuffix in ["", "Height", "Baseline"]:
                                tempKey = "%s%s" % (baseName, posSuffix)
                                value = getattr(self, tempKey, None)
                                if value is not None:
                                    zonePos = value
                                    break
                        if zonePos is None:
                            raise FontInfoParseError(
                                "Failed to find fontinfo FDDict %s top/bottom "
                                "zone name %s to match the zone width key '%s'"
                                "." % (self.DictName, tempKey, key))
                        if width < 0:
                            topPos = zonePos
                            bottomPos = zonePos + width
                            isBottomZone = 1
                            if (i == 0) and (key != "BaselineOvershoot"):
                                raise FontInfoParseError(
                                    "FontDict %s. Zone %s is a top zone, and "
                                    "the width (%s) must be positive." %
                                    (self.DictName, tempKey, width))
                            elif ((i == 2) and
                                  (key != "FamilyBaselineOvershoot")):
                                raise FontInfoParseError(
                                    "FontDict %s. Zone %s is a top zone, and "
                                    "the width (%s) must be positive." %
                                    (self.DictName, tempKey, width))
                        else:
                            bottomPos = zonePos
                            topPos = zonePos + width
                            isBottomZone = 0
                            if i == 1 and width > 0:
                                raise FontInfoParseError(
                                    "FontDict %s. Zone %s is a bottom zone, "
                                    "and so the width (%s) must be negative." %
                                    (self.DictName, tempKey, width))
                        bluePairList.append((topPos, bottomPos, tempKey,
                                            self.DictName, isBottomZone))
                        log.debug("%s BlueValue %s: (%g, %g)" %
                                  ('Bottom' if isBottomZone else 'Top',
                                   tempKey, bottomPos, topPos))
            if bluePairList:
                bluePairList = sorted(bluePairList)
                prevPair = bluePairList[0]
                zoneBuffer = 2 * self.BlueFuzz + 1
                for pair in bluePairList[1:]:
                    if prevPair[0] > pair[1]:
                        raise FontInfoParseError(
                            "In FDDict %s. The top of zone %s at %s overlaps "
                            "zone %s with the bottom at %s." %
                            (self.DictName, prevPair[2], prevPair[0], pair[2],
                             pair[1]))
                    elif abs(pair[1] - prevPair[0]) <= zoneBuffer:
                        raise FontInfoParseError(
                            "In FDDict %s. The top of zone %s at %s is within "
                            "the min separation limit (%s units) of zone %s "
                            "with the bottom at %s." %
                            (self.DictName, prevPair[2], prevPair[0],
                             zoneBuffer, pair[2], pair[1]))
                    prevPair = pair
                setattr(self, pairFieldName, bluePairList)
        return

    def __repr__(self):
        fddict = {k: v for k, v in vars(self).items() if v is not None}
        return "<%s '%s' %s>" % (
            self.__class__.__name__, fddict.get('DictName', 'no name'), fddict)


def parseFontInfoFile(fdArrayMap, data, glyphList, maxY, minY, fontName):
    # fontDictList may or may not already contain a
    # font dict taken from the source font top FontDict.
    # The map of glyph names to font dict: the index into fontDictList.
    fdSelectMap = {}
    # The user-specified set of blue values to write into the output font,
    # some sort of merge of the individual font dicts. May not be supplied.
    finalFDict = None
    setName = None

    glyphSetSet = set()

    blueFuzz = fdArrayMap[0].BlueFuzz

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
    assert tuple(fdArrayMap.keys()) == (0,)
    maxfdIndex = max(fdArrayMap.keys())
    dictValueList = []
    dictKeyWord = ''

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
                    if dictName == kFinalDictName:
                        # This is dict is NOT used to hint any glyphs; it is
                        # used to supply the merged alignment zones and stem
                        # widths for the final font.
                        if finalFDict is None:
                            finalFDict = FDDict(maxfdIndex + 1,
                                                dictName=dictName)
                        fdDict = finalFDict
                    else:
                        if dictName not in fdIndexDict:
                            fdDict = FDDict(maxfdIndex + 1, dictName=dictName)
                            # save dict and FDIndex.
                            maxfdIndex += 1
                            fdIndexDict[dictName] = maxfdIndex
                            fdArrayMap[maxfdIndex] = fdDict
                        else:
                            fdDict = fdArrayMap[fdIndexDict[dictName]]

                elif token == kGlyphSetToken:
                    state = glyphSetState
                    setName = tokenList[i]
                    if setName in glyphSetSet:
                        raise FontInfoParseError(
                            "Duplicate GlyphSet \"%s\"" % setName)
                    else:
                        glyphSetSet.add(setName)
                    i += 1
                else:
                    raise FontInfoParseError(
                        "Unrecognized token after \"begin\" keyword: %s" %
                        token)

            elif token in kBaseStateTokens:
                # Discard value for base token.
                token = tokenList[i]
                i += 1
                if (token[0] in ["[", "("]) and (not token[-1] in ["]", ")"]):
                    state = inValue
            else:
                raise FontInfoParseError(
                    "Unrecognized token in base state: %s" % token)

        elif state == inValue:
            # We are processing a list value for a base state token.
            if token[-1] in ["]", ")"]:
                state = baseState  # found the last token in the list value.

        elif state == inDictValue:
            if token[-1] in ["]", ")"]:
                dictValueList.append(token[:-1])
                fdDict.setInfo(dictKeyWord, dictValueList)
                state = dictState  # found the last token in the list value.
            else:
                dictValueList.append(token)

        elif state == glyphSetState:
            # "end GlyphSet" marks end of set,
            # else we are adding a new glyph name.
            if (token == kEndToken) and tokenList[i] == kGlyphSetToken:
                if tokenList[i + 1] != setName:
                    raise FontInfoParseError(
                        "End glyph set name \"%s\" does not match begin glyph "
                        "set name \"%s\"." % (tokenList[i + 1], setName))
                state = baseState
                i += 2
                setName = None
            else:
                # Need to add matching glyphs.
                gi = 0
                for gname in glyphList:
                    if re.search(token, gname):
                        # fdIndex value
                        fdSelectMap[gname] = fdIndexDict[setName]
                    gi += 1

        elif state == dictState:
            # "end FDDict" marks end of set,
            # else we are adding a new glyph name.
            if (token == kEndToken) and tokenList[i] == kFDDictToken:
                if tokenList[i + 1] != dictName:
                    raise FontInfoParseError(
                        "End FDDict  name \"%s\" does not match begin FDDict "
                        "name \"%s\"." % (tokenList[i + 1], dictName))
                state = baseState
                i += 2
                dictName = None
                fdDict = None
            else:
                if token in kFDDictKeys:
                    value = tokenList[i]
                    i += 1
                    if value[0] in ["[", "("]:
                        if not value[-1] in ["]", ")"]:
                            state = inDictValue
                            dictValueList = [value[1:]]
                            dictKeyWord = token
                        else:
                            fdDict.setInfo(token, value[1:-1])
                    else:
                        fdDict.setInfo(token, value)
                else:
                    raise FontInfoParseError(
                        "FDDict key \"%s\" in fdDict named \"%s\" is not "
                        "recognized." % (token, dictName))

    for dictName, fdIndex in fdIndexDict.items():
        fdDict = fdArrayMap[fdIndex]
        if fdDict.DominantH is None:
            log.warning("The FDDict '%s' in fontinfo has no "
                        "DominantH value", dictName)
        if fdDict.DominantV is None:
            log.warning("The FDDict '%s' in fontinfo has no "
                        "DominantV value", dictName)
        if fdDict.BlueFuzz is None:
            fdDict.BlueFuzz = blueFuzz
        fdDict.buildBlueLists()
        if fdDict.FontName is None:
            fdDict.FontName = fontName

    if fdIndexDict:
        # There are some FDDict definitions. This means that we need
        # to fix the default fontDict, inherited from the source font,
        # so that it has blues zones that will not affect hinting,
        # e.g outside of the Font BBox. We do this becuase if there are
        # glyphs which are not assigned toa user specified font dict,
        # it is becuase it doesn't make sense to provide alignment zones
        # for the glyph. Since otfautohint does require at least one bottom
        # zone and one top zone, we add one bottom and one top zone that are
        # outside the font BBox, so that hinting won't be affected by them.
        for gname in glyphList:
            if gname not in fdSelectMap:
                fdSelectMap[gname] = 0
        defaultFDDict = fdArrayMap[0]
        for key in kBlueValueKeys + kOtherBlueValueKeys:
            defaultFDDict.setInfo(key, None)
        defaultFDDict.BaselineYCoord = minY - 100
        defaultFDDict.BaselineOvershoot = 0
        defaultFDDict.CapHeight = maxY + 100
        defaultFDDict.CapOvershoot = 0
        defaultFDDict.BlueFuzz = 0
        defaultFDDict.DictName = kDefaultDictName  # "No Alignment Zones"
        defaultFDDict.FontName = fontName
        defaultFDDict.buildBlueLists()

    return fdSelectMap, finalFDict


def mergeFDDicts(prevDictList):
    # Extract the union of the stem widths and zones from the list
    # of FDDicts, and replace the current values in the topDict.
    blueZoneDict = {}
    otherBlueZoneDict = {}
    dominantHDict = {}
    dominantVDict = {}
    bluePairListNames = [kFontDictBluePairsName, kFontDictOtherBluePairsName]
    zoneDictList = [blueZoneDict, otherBlueZoneDict]
    for prefDDict in prevDictList:
        if prefDDict is None:
            continue
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
        zoneList = sorted(zoneList)
        # Now check for conflicts.
        prevZone = zoneList[0]
        goodZoneList.append(prevZone[1])
        goodZoneList.append(prevZone[0])
        zoneBuffer = 2 * prefDDict.BlueFuzz + 1
        for zone in zoneList[1:]:
            curEntry = zoneDict[zone]
            prevEntry = zoneDict[prevZone]
            zoneName = curEntry[1]
            fdDictName = curEntry[2]
            prevZoneName = prevEntry[1]
            prevFDictName = prevEntry[2]

            if zone[1] < prevZone[0]:
                log.warning("For final FontDict, skipping zone %s in FDDict %s"
                            " because it overlaps with zone %s in FDDict %s.",
                            zoneName, fdDictName, prevZoneName, prevFDictName)
            elif abs(zone[1] - prevZone[0]) <= zoneBuffer:
                log.warning("For final FontDict, skipping zone %s in FDDict %s"
                            " because it is within the minimum separation "
                            "allowed (%s units) of %s in FDDict %s.",
                            zoneName, fdDictName, zoneBuffer, prevZoneName,
                            prevFDictName)
            else:
                goodZoneList.append(zone[1])
                goodZoneList.append(zone[0])

            prevZone = zone

        stemList = stemDict.keys()
        if not stemList:
            continue
        stemList = sorted(stemList)
        # Now check for conflicts.
        prevStem = stemList[0]
        goodStemList.append(prevStem)
        for stem in stemList[1:]:
            if abs(stem - prevStem) < 2:
                log.warning("For final FontDict, skipping stem width %s in "
                            "FDDict %s because it overlaps in coverage with "
                            "stem width %s in FDDict %s.",
                            stem, stemDict[stem], prevStem, stemDict[prevStem])
            else:
                goodStemList.append(stem)
            prevStem = stem
    privateMap = {}
    if goodBlueZoneList:
        privateMap['BlueValues'] = goodBlueZoneList
    if goodOtherBlueZoneList:
        privateMap['OtherBlues'] = goodOtherBlueZoneList
    else:
        privateMap['OtherBlues'] = None
    if goodHStemList:
        privateMap['StemSnapH'] = goodHStemList
    else:
        privateMap['StemSnapH'] = None
    if goodVStemList:
        privateMap['StemSnapV'] = goodVStemList
    else:
        privateMap['StemSnapV'] = None
    return privateMap


def fontinfoIncludeData(fdir, idir, match):
    incrpath = match.group(1)
    incpath = os.path.join(fdir, incrpath)
    if os.path.isfile(incpath):
        with open(incpath, "r", encoding="utf-8") as ii:
            return ii.read()
    if idir is not None:
        incpath = os.path.join(idir, incrpath)
        if os.path.isfile(incpath):
            with open(incpath, "r", encoding="utf-8") as ii:
                return ii.read()
    raise FontInfoParseError("fontinfo include file \"%s\" not found" %
                             incrpath)


def fontinfoFileData(options, font):
    # Don't use fontinfo file for a variable font
    if not font.isVF():
        fdir = os.path.dirname(font.getInputPath())
        if options.fontinfoPath is not None:
            idir = os.path.dirname(options.fontinfoPath)
            srcFontInfo = options.fontinfoPath
        else:
            idir = None
            srcFontInfo = os.path.join(fdir, "fontinfo")
        if os.path.isfile(srcFontInfo):
            with open(srcFontInfo, "r", encoding="utf-8") as fi:
                fontInfoData = fi.read()
            # Process includes
            subsMade = 1
            while subsMade > 0:
                fontInfoData, subsMade = re.subn(
                    r"^include (.*)$",
                    lambda m: fontinfoIncludeData(fdir, idir, m),
                    fontInfoData, 0, re.M)
            fontInfoData = re.sub(r"#[^\r\n]+", "", fontInfoData)
            return fontInfoData, 'FDDict' in fontInfoData
    return None, None


def getFDInfo(font, desc, options, glyphList, isVF):
    privateMap = None
    # Check the fontinfo file, and add any other font dicts
    srcFontinfoData, hasDict = fontinfoFileData(options, font)
    if hasDict and not options.ignoreFontinfo:
        # Get the default fontinfo from the font's top dict.
        fdDict = font.getPrivateFDDict(options.allow_no_blues,
                                       options.noFlex,
                                       options.vCounterGlyphs,
                                       options.hCounterGlyphs, desc)
        fdArrayMap = {0: fdDict}
        minY, maxY = font.get_min_max(fdDict.OrigEmSqUnits)
        fdSelectMap, finalFDict = parseFontInfoFile(
            fdArrayMap, srcFontinfoData, glyphList, maxY, minY, desc)
        if finalFDict is None:
            # If a font dict was not explicitly specified for the
            # output font, use the first user-specified font dict.
            fontDictList = [v for k, v in sorted(fdArrayMap.items())]
            privateMap = mergeFDDicts(fontDictList[1:])
        else:
            privateMap = mergeFDDicts([finalFDict])
    elif isVF:
        fdSelectMap = {}
        dictRecord = {}
        fdArraySet = set()
        for name in glyphList:
            fdIndex = font.getfdIndex(name)
            if fdIndex is None:
                continue
            if fdIndex not in fdArraySet:
                fddict, model = font.getPrivateFDDict(options.allow_no_blues,
                                                      options.noFlex,
                                                      options.vCounterGlyphs,
                                                      options.hCounterGlyphs,
                                                      desc, fdIndex)
                fdArraySet.add(fdIndex)
                if model not in dictRecord:
                    dictRecord[model] = {}
                dictRecord[model][fdIndex] = fddict
            fdSelectMap[name] = fdIndex
        return fdSelectMap, dictRecord, None
    else:
        fdSelectMap = {}
        fdArrayMap = {}
        for name in glyphList:
            fdIndex = font.getfdIndex(name)
            if fdIndex is None:
                continue
            if fdIndex not in fdArrayMap:
                fddict = font.getPrivateFDDict(options.allow_no_blues,
                                               options.noFlex,
                                               options.vCounterGlyphs,
                                               options.hCounterGlyphs,
                                               desc, fdIndex)
                fdArrayMap[fdIndex] = fddict
            fdSelectMap[name] = fdIndex

    return fdSelectMap, fdArrayMap, privateMap


class FDDictManager:
    def __init__(self, options, fontInstances, glyphList, isVF=False):
        self.options = options
        self.fontInstances = fontInstances
        self.glyphList = glyphList
        self.isVF = isVF
        self.fdSelectMap = None
        self.auxRecord = {}
        refI = fontInstances[0]
        fdArrayCompat = True
        fdArrays = []
        assert not isVF or len(fontInstances) == 1

        for i in fontInstances:
            (fdSelectMap, fdArrayMap,
             privateMap) = getFDInfo(i.font, i.font.desc, options,
                                     glyphList, isVF)
            if isVF:
                self.fdSelectMap = fdSelectMap
                self.dictRecord = fdArrayMap
            else:
                fdArrays.append(fdArrayMap)
                if self.fdSelectMap is None:
                    self.fdSelectMap = fdSelectMap
                elif not self.addDict(self.fdSelectMap, fdSelectMap):
                    log.error("FDDict indexes for font %s " % i.font.desc +
                              ("different from those in font %s" %
                               refI.font.desc))
                    fdArrayCompat = False
                if privateMap is not None:
                    # XXX make this decision more selective
                    i.font.mergePrivateMap(privateMap)

        if not isVF:
            if fdArrayCompat:
                fdmap = {}
                for fdIndex in fdArrays[0].keys():
                    fdmap[fdIndex] = [x[fdIndex] if fdIndex in x else None
                                      for x in fdArrays]
                self.dictRecord = {0: fdmap}
            else:
                log.error("Cannot continue")
                sys.exit()

        if options.printFDDictList or options.printAllFDDict:
            # Print the user defined FontDicts, and exit.
            print("Private Dictionaries:\n")
            for _model, fda in self.dictRecord.items():
                for fdIndex, fdl in fda.items():
                    for fontDict in fdl:
                        if fontDict is None:
                            continue
                        print(fontDict.FontName)
                        print(fontDict.DictName)
                        print(str(fontDict))
                        gnameList = [gn for gn, fi in self.fdSelectMap.items()
                                     if fdIndex == fi]
                        print("%d glyphs:" % len(gnameList))
                        if len(gnameList) > 0:
                            gTxt = " ".join(gnameList)
                        else:
                            gTxt = "None"
                        print(gTxt + "\n")
                        if not options.printAllFDDict:
                            break
        elif len(set(self.fdSelectMap.keys())) > 1:
            log.info("Using different FDDicts for some glyphs.")

        self.checkGlyphList()

    def getDictRecord(self):
        return self.dictRecord

    def getRecKey(self, gname, vsindex):
        fdIndex = self.fdSelectMap[gname]
        if vsindex in self.dictRecord and fdIndex in self.dictRecord[vsindex]:
            return vsindex, fdIndex
        assert self.isVF
        if vsindex not in self.auxRecord:
            self.auxRecord[vsindex] = {}
        elif fdIndex in self.auxRecord[vsindex]:
            return self.auxRecord[vsindex][fdIndex]
        f = self.fontInstances[0]
        fddict, _ = f.font.getPrivateFDDict(self.options.allow_no_blues,
                                            self.options.noFlex,
                                            self.options.vCounterGlyphs,
                                            self.options.hCounterGlyphs,
                                            f.font.desc, fdIndex, vsindex)
        self.auxRecord[vsindex][fdIndex] = fddict
        return fddict

    def checkGlyphList(self):
        options = self.options
        glyphSet = set(self.glyphList)
        # Check for missing glyphs explicitly added via fontinfo or cmd line
        for label, charDict in [("hCounterGlyphs", options.hCounterGlyphs),
                                ("vCounterGlyphs", options.vCounterGlyphs),
                                ("upperSpecials", options.upperSpecials),
                                ("lowerSpecials", options.lowerSpecials),
                                ("noBlues", options.noBlues)]:
            for name in (n for n, w in charDict.items()
                         if w and n not in glyphSet):
                log.warning("%s glyph named in fontinfo is " % label +
                            "not in font: %s" % name)

    def addDict(self, dict1, dict2):
        # This allows for sparse masters, just verifying that if a glyph name
        # is in both dictionaries it maps to the same index.
        good = True
        for k, v in dict2.items():
            if k not in dict1:
                dict1[k] = v
            elif v != dict1[k]:
                good = False
        return good
