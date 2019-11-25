# Copyright 2014 Adobe. All rights reserved.
"""
otfpdf v1.6.4 Nov 25 2019
provides support for the proofpdf script, for working with OpenType/CFF
fonts. Provides an implementation of the fontpdf font object. Cannot be
run alone.
"""
from fontTools.pens.boundsPen import BoundsPen
from fontTools.misc.psCharStrings import T2OutlineExtractor

from afdko.pdflib.fontpdf import FontPDFGlyph, FontPDFFont, FontPDFPen

__version__ = "1.6.4"


class txPDFFont(FontPDFFont):

    def __init__(self, clientFont, params):
        self.clientFont = clientFont
        if params.userBaseLine is not None:
            self.baseLine = params.userBaseLine
        else:
            self.baseLine = None
        self.path = params.rt_filePath
        self.isCID = 0
        self.psName = None
        self.OTVersion = None
        self.emSquare = None
        self.bbox = None
        self.ascent = None
        self.descent = None
        self.blueZones = None
        self.getEmSquare()
        self.getBaseLine()
        self.getBBox()
        self.GetBlueZones()
        self.AscentDescent()

    def __lt__(self, other):
        return self.clientGetPSName() < other.clientGetPSName()

    def clientGetPSName(self):
        psName = self.clientFont['CFF '].cff.fontNames[0]
        topDict = self.clientFont['CFF '].cff.topDictIndex[0]
        if hasattr(topDict, "ROS"):
            self.isCID = 1
        return psName

    def clientGetOTVersion(self):
        try:
            version = self.clientFont['head'].fontRevision
        except KeyError:
            # Maybe its a dummy OTF made from a Type 1 font,
            # and has only a CFF table.
            try:
                topDict = self.clientFont['CFF '].cff.topDictIndex[0]
                if hasattr(topDict, "ROS"):
                    version = topDict.CIDFontVersion  # a number
                else:
                    version = topDict.version  # a string
                    version = eval(version)
            except AttributeError:
                version = 1.0

        majorVersion = int(version)
        minorVersion = str(
            int(1000 * (0.0005 + version - majorVersion))).zfill(3)
        versionString = "%s.%s" % (majorVersion, minorVersion)
        return versionString

    def clientGetGlyph(self, glyphName):
        return txPDFGlyph(self, glyphName)

    def clientGetEmSquare(self):
        try:
            emSquare = self.clientFont['head'].unitsPerEm
        except KeyError:
            # Maybe its a dummy OTF made from a Type 1 font,
            # and has only a CFF table.
            topDict = self.clientFont['CFF '].cff.topDictIndex[0]
            scale = topDict.FontMatrix[0]
            emSquare = int(0.5 + 1 / scale)
        return emSquare

    def clientGetBaseline(self):
        baseLine = 0
        txFont = self.clientFont
        try:
            unicodeRange2 = txFont["OS/2"].ulUnicodeRange2
            if unicodeRange2 & 0x10000:  # supports CJK ideographs
                baseTag = "ideo"
            else:
                baseTag = "romn"
            horiz_axis = self.clientFont['BASE'].table.HorizAxis
            baseTagIndex = horiz_axis.BaseTagList.BaselineTag.index(baseTag)
            for baseRecord in horiz_axis.BaseScriptList.BaseScriptRecord:
                if baseRecord.BaseScriptTag == "latn":
                    baseValues = baseRecord.BaseScript.BaseValues
                    baseLine = baseValues.BaseCoord[baseTagIndex].Coordinate
                    break
        except (KeyError, AttributeError):
            topDict = txFont['CFF '].cff.topDictIndex[0]
            if hasattr(topDict, "ROS"):
                baseLine = -120
            else:
                baseLine = 0

        return baseLine

    def clientGetBBox(self):
        try:
            headTable = self.clientFont['head']
            return [headTable.xMin, headTable.yMin,
                    headTable.xMax, headTable.yMax]
        except KeyError:
            # Maybe it's a dummy OTF made from a Type 1 font,
            # and has only a CFF table.
            topDict = self.clientFont['CFF '].cff.topDictIndex[0]
            bbox = topDict.FontBBox
            return [bbox[0], bbox[1], bbox[2], bbox[3]]

    def clientGetBlueZones(self):
        blueValues = []
        txFont = self.clientFont
        topDict = txFont['CFF '].cff.topDictIndex[0]
        if hasattr(topDict, "ROS"):
            FDArray = topDict.FDArray
            self.isCID = 1
        else:
            FDArray = [topDict]
        for fontDict in FDArray:
            try:
                rawBlueList = fontDict.Private.BlueValues
                blueList = []
                for i in list(range(0, len(rawBlueList), 2)):
                    blueList.append((rawBlueList[i], rawBlueList[i + 1]))
                blueValues.append(blueList)
            except AttributeError:
                # the font has no BlueValues
                pass
        return blueValues

    def clientGetAscentDescent(self):
        try:
            os2Table = self.clientFont['OS/2']
            return os2Table.sTypoAscender, os2Table.sTypoDescender
        except KeyError:
            return None, None


def hintOn(i, hintMaskBytes):
    # used to add the active hints to the bez string,
    # when a T2 hintmask operator is encountered.
    byteIndex = i // 8
    byteValue = hintMaskBytes[byteIndex]
    offset = 7 - (i % 8)
    return ((2 ** offset) & byteValue) > 0


class FontPDFT2OutlineExtractor(T2OutlineExtractor):

    def __init__(self, pen, localSubrs, globalSubrs, nominalWidthX,
                 defaultWidthX):
        T2OutlineExtractor.__init__(self, pen, localSubrs, globalSubrs,
                                    nominalWidthX, defaultWidthX)
        self.hintTable = []
        self.vhints = []
        self.hhints = []

    def op_hstem(self, index):
        args = self.popallWidth()
        self.updateHints(args, self.hhints)

    def op_vstem(self, index):
        args = self.popallWidth()
        self.updateHints(args, self.vhints)

    def op_hstemhm(self, index):
        args = self.popallWidth()
        self.hhints = []
        self.updateHints(args, self.hhints)

    def op_vstemhm(self, index):
        args = self.popallWidth()
        self.vhints = []
        self.updateHints(args, self.vhints)

    def op_hintmask(self, index):
        hintMaskString, index = self.doMask(index, "hintmask")
        self.firstOpSeen = 1
        return hintMaskString, index

    def op_cntrmask(self, index):
        hintMaskString, index = self.doMask(index, "cntrmask")
        self.firstOpSeen = 1
        return hintMaskString, index

    def updateHints(self, args, hintList):
        self.countHints(args)

        # first hint value is absolute hint coordinate, second is hint width
        lastval = args[0]
        arg = str(lastval)
        hint1 = arg

        for i in list(range(len(args))[1:]):
            val = args[i]
            newVal = lastval + val
            lastval = newVal

            if i % 2:
                hint2 = str(val)
                hintList.append((hint1, hint2))
            else:
                hint1 = str(newVal)

    def getCurHints(self, hintMaskBytes):
        curhhints = []
        curvhints = []
        numhhints = len(self.hhints)

        for i in list(range(numhhints)):
            if hintOn(i, hintMaskBytes):
                curhhints.append(i)
        numvhints = len(self.vhints)
        for i in list(range(numvhints)):
            if hintOn(i + numhhints, hintMaskBytes):
                curvhints.append(i)
        return curhhints, curvhints

    def doMask(self, index, maskCommand):
        if not self.hintMaskBytes:
            args = self.popallWidth()
            if args:
                self.vhints = []
                self.updateHints(args, self.vhints)
            self.hintMaskBytes = (self.hintCount + 7) // 8

        self.hintMaskString, index = (
            self.callingStack[-1].getBytes(index, self.hintMaskBytes))

        curhhints, curvhints = self.getCurHints(self.hintMaskString)

        self.hintTable.append([curhhints, curvhints, self.pen.total])

        return self.hintMaskString, index

    def countHints(self, args):
        self.hintCount = self.hintCount + len(args) // 2


def drawCharString(charString, pen):
    subrs = getattr(charString.private, "Subrs", [])
    extractor = FontPDFT2OutlineExtractor(
        pen, subrs, charString.globalSubrs, charString.private.nominalWidthX,
        charString.private.defaultWidthX)
    extractor.execute(charString)
    charString.width = extractor.width
    charString.hintTable = extractor.hintTable
    charString.hhints = extractor.hhints
    charString.vhints = extractor.vhints


class txPDFGlyph(FontPDFGlyph):

    def clientInitData(self):
        txFont = self.parentFont.clientFont

        try:
            glyph_set = txFont.getGlyphSet()
        except KeyError:
            glyph_set = None

        if not hasattr(txFont, 'vmetrics'):
            try:
                txFont.vmetrics = txFont['vmtx'].metrics
            except KeyError:
                txFont.vmetrics = None
            try:
                txFont.vorg = txFont['VORG']
            except KeyError:
                txFont.vorg = None

        fTopDict = txFont['CFF '].cff.topDictIndex[0]
        self.isCID = self.parentFont.isCID
        charstring = fTopDict.CharStrings[self.name]

        # Get the list of points
        pen = FontPDFPen(glyph_set)
        drawCharString(charstring, pen)
        self.hintTable = charstring.hintTable

        self.hhints = charstring.hhints
        self.vhints = charstring.vhints
        self.numMT = pen.numMT
        self.numLT = pen.numLT
        self.numCT = pen.numCT
        self.numPaths = pen.numPaths
        self.pathList = pen.pathList
        for path in self.pathList:
            lenPath = len(path)
            path[-1].next = path[0]
            path[0].last = path[-1]
            if lenPath > 1:
                path[0].next = path[1]
                path[-1].last = path[-2]
                for i in list(range(lenPath)[1:-1]):
                    pt = path[i]
                    pt.next = path[i + 1]
                    pt.last = path[i - 1]

        assert len(self.pathList) == self.numPaths, (
            "Path lengths don't match %s %s" % (len(self.pathList),
                                                self.numPaths))
        # get the bbox and width.
        pen = BoundsPen(glyph_set)
        charstring.draw(pen)
        self.xAdvance = charstring.width
        glyph_bounds = pen.bounds
        if not glyph_bounds:
            self.BBox = [0, 0, 0, 0]
        else:
            self.BBox = [round(item) for item in glyph_bounds]

        self.yOrigin = self.parentFont.emSquare + self.parentFont.getBaseLine()
        if txFont.vorg:
            try:
                self.yOrigin = txFont.vorg[self.name]
            except KeyError:
                if txFont.vmetrics:
                    try:
                        mtx = txFont.vmetrics[self.name]
                        self.yOrigin = mtx[1] + self.BBox[3]
                    except KeyError:
                        pass

        haveVMTX = 0
        if txFont.vmetrics:
            try:
                mtx = txFont.vmetrics[self.name]
                self.yAdvance = mtx[0]
                self.tsb = mtx[1]
                haveVMTX = 1
            except KeyError:
                pass
        if not haveVMTX:
            self.yAdvance = self.parentFont.getEmSquare()
            self.tsb = (
                self.yOrigin - self.BBox[3] + self.parentFont.getBaseLine())

        # Get the fdIndex, so we can later determine
        # which set of blue values to use.
        self.fdIndex = 0
        if hasattr(fTopDict, "ROS"):
            gid = fTopDict.CharStrings.charStrings[self.name]
            self.fdIndex = fTopDict.FDSelect[gid]
