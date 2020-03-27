# Copyright 2017 Adobe. All rights reserved.

import ast
import hashlib
import os
import re
from collections import OrderedDict

from fontTools.misc import etree as ET
from fontTools.misc import plistlib
from fontTools.ufoLib import UFOReader

from psautohint.ufoFont import norm_float

from afdko import fdkutils

__version__ = '1.35.0'

__doc__ = """
ufotools.py v1.35.0 Mar 03 2020

Originally developed to work with 'bez' files and UFO fonts in support of
the autohint tool, ufotools.py is now only used in checkoutlinesufo (since
autohint has been dropped in favor of psautohint). Some references to "bez"
and "autohint" remain in the tool its documentation.

Users should *NOT* rely on long-term support of methods or classes within
this module directly. It is intended for AFDKO-internal use and not as a
general-purpose library outside of AFDKO.

The basic model is:
 - read GLIF file
 - transform GLIF XML element to bez file
 - call FDK tool on bez file
 - transform new bez file to GLIF XML element with new data, and save in list

After all glyphs are done save all the new GLIF XML elements to GLIF
files, and update the hash map.

A complication in the Adobe UFO workflow comes from the fact we want to
make sure that checkoutlines and autohint have been run on each glyph in
a UFO font, when building an OTF font from the UFO font. We need to run
checkoutlines, because we no longer remove overlaps from source UFO font
data, because this can make revising a glyph much easier. We need to run
autohint, because the glyphs must be hinted after checkoutlines is run,
and in any case we want all glyphs to have been autohinted. The problem
with this is that it can take a minute or two to run autohint or
checkoutlines on a 2K-glyph font. The way we avoid this is to make and
keep a hash of the source glyph drawing operators for each tool. When
the tool is run, it calculates a hash of the source glyph, and compares
it to the saved hash. If these are the same, the tool can skip the
glyph. This saves a lot of time: if checkoutlines and autohint are run
on all glyphs in a font, then a second pass is under 2 seconds.

Another issue is that since we no longer remove overlaps from the source
glyph files, checkoutlines must write any edited glyph data to a
different layer in order to not destroy the source data. The ufotools
defines an Adobe-specific glyph layer for processed glyphs, named
"glyphs.com.adobe.type.processedGlyphs".
checkoutlines writes new glyph files to the processed glyphs layer only
when it makes a change to the glyph data.

When the autohint program is run, the ufotools must be able to tell
whether checkoutlines has been run and has altered a glyph: if so, the
input file needs to be from the processed glyph layer, else it needs to
be from the default glyph layer.

The way the hashmap works is that we keep an entry for every glyph that
has been processed, identified by a hash of its marking path data. Each
entry contains:
- a hash of the glyph point coordinates, from the default layer.
This is set after a program has been run.
- a history list: a list of the names of each program that has been run,
  in order.
- an editStatus flag.
Altered GLIF data is always written to the Adobe processed glyph layer. The
program may or may not have altered the outline data. For example, autohint
adds private hint data, and adds names to points, but does not change any
paths.

If the stored hash for the glyph does not exist, the ufotools lib will save the
new hash in the hash map entry and will set the history list to contain just
the current program. The program will read the glyph from the default layer.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is in the history list,then ufotools
will return "skip=1", and the calling program may skip the glyph.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is not in the history list, then the
ufotools will return "skip=0". If the font object field 'usedProcessedLayer' is
set True, the program will read the glyph from the from the Adobe processed
layer, if it exists, else it will always read from the default layer.

If the hash differs between the hash map entry and the current glyph in the
default layer, and usedProcessedLayer is False, then ufotools will return
"skip=0". If usedProcessedLayer is True, then the program will consult the list
of required programs. If any of these are in the history list, then the program
will report an error and return skip =1, else it will return skip=1. The
program will then save the new hash in the hash map entry and reset the history
list to contain just the current program. If the old and new hash match, but
the program name is not in the history list, then the ufotools will not skip
the glyph, and will add the program name to the history list.


The only tools using this are, at the moment, checkoutlines, checkoutlinesufo
and autohint. checkoutlines and checkoutlinesufo use the hash map to skip
processing only when being used to edit glyphs, not when reporting them.
checkoutlines necessarily flattens any components in the source glyph file to
actual outlines. autohint adds point names, and saves the hint data as a
private data in the new GLIF file.

autohint saves the hint data in the GLIF private data area, /lib/dict,
as a key and data pair. See below for the format.

autohint started with _hintFormat1_, a reasonably compact XML representation of
the data. In Sep 2105, autohhint switched to _hintFormat2_ in order to be plist
compatible. This was necessary in order to be compatible with the UFO spec, by
was driven more immediately by the fact the the UFO font file normalization
tools stripped out the _hintFormat1_ hint data as invalid elements.
"""

_hintFormat1_ = """

Deprecated. See _hintFormat2_ below.

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
before the final T1 path operator? The logic here applies it before the
move-to only.

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
                      <!-- where n1-5 are decimal values -->
                      <hstem3 stem3List="n0,n1,n2,n3,n4,n5" />*
                      <!-- where n1-5 are decimal values -->
                      <vstem3 stem3List="n0,n1,n2,n3,n4,n5" />*
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
<hintSetList id="64bf4987f05ced2a50195f971cd924984047eb1d79c8c43e6a0054f59cc85
dea23a49deb20946a4ea84840534363f7a13cca31a81b1e7e33c832185173369086">
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

_hintFormat2_ = """

A <dict> element in the hintSetList array identifies a specific point by its
name, and describes a new set of stem hints which should be applied before the
specific point.

A <string> element in the flexList identifies a specific point by its name.
The point is the first point of a curve. The presence of the element is a
processing suggestion, that the curve and its successor curve should be
converted to a flex operator.

One challenge in applying the hintSetList and flexList elements is that in
the GLIF format, there is no explicit start and end operator: the first path
operator is both the end and the start of the path. I have chosen to convert
this to T1 by taking the first path operator, and making it a move-to. I then
also use it as the last path operator. An exception is a line-to; in T1, this
is omitted, as it is implied by the need to close the path. Hence, if a hintset
references the first operator, there is a potential ambiguity: should it be
applied before the T1 move-to, or before the final T1 path operator? The logic
here applies it before the move-to only.
 <glyph>
...
    <lib>
        <dict>
            <key><com.adobe.type.autohint></key>
            <dict>
                <key>id</key>
                <string> <fingerprint for glyph> </string>
                <key>hintSetList</key>
                <array>
                    <dict>
                      <key>pointTag</key>
                      <string> <point name> </string>
                      <key>stems</key>
                      <array>
                        <string>hstem <position value> <width value></string>*
                        <string>vstem <position value> <width value></string>*
                        <string>hstem3 <position value 0>...<position value 5>
                        </string>*
                        <string>vstem3 <position value 0>...<position value 5>
                        </string>*
                      </array>
                    </dict>*
                </array>

                <key>flexList</key>*
                <array>
                    <string><point name></string>+
                </array>
            </dict>
        </dict>
    </lib>
</glyph>

Example from "B" in SourceCodePro-Regular
<key><com.adobe.type.autohint><key>
<dict>
    <key>id</key>
    <string>64bf4987f05ced2a50195f971cd924984047eb1d79c8c43e6a0054f59cc85dea23
    a49deb20946a4ea84840534363f7a13cca31a81b1e7e33c832185173369086</string>
    <key>hintSetList</key>
    <array>
        <dict>
            <key>pointTag</key>
            <string>hintSet0000</string>
            <key>stems</key>
            <array>
                <string>hstem 338 28</string>
                <string>hstem 632 28</string>
                <string>hstem 100 32</string>
                <string>hstem 496 32</string>
            </array>
        </dict>
        <dict>
            <key>pointTag</key>
            <string>hintSet0005</string>
            <key>stems</key>
            <array>
                <string>hstem 0 28</string>
                <string>hstem 338 28</string>
                <string>hstem 632 28</string>
                <string>hstem 100 32</string>
                <string>hstem 454 32</string>
                <string>hstem 496 32</string>
            </array>
        </dict>
        <dict>
            <key>pointTag</key>
            <string>hintSet0016</string>
            <key>stems</key>
            <array>
                <string>hstem 0 28</string>
                <string>hstem 338 28</string>
                <string>hstem 632 28</string>
                <string>hstem 100 32</string>
                <string>hstem 496 32</string>
            </array>
        </dict>
    </array>
<dict>

"""

# UFO names
kDefaultGlyphsLayerName = "public.default"
kDefaultGlyphsLayer = "glyphs"
kProcessedGlyphsLayerName = "com.adobe.type.processedglyphs"
kProcessedGlyphsLayer = "glyphs.%s" % kProcessedGlyphsLayerName
DEFAULT_LAYER_ENTRY = [kDefaultGlyphsLayerName, kDefaultGlyphsLayer]
PROCESSED_LAYER_ENTRY = [kProcessedGlyphsLayerName, kProcessedGlyphsLayer]

kFontInfoName = "fontinfo.plist"
kContentsName = "contents.plist"
kLibName = "lib.plist"
kPublicGlyphOrderKey = "public.glyphOrder"

kAdobeDomainPrefix = "com.adobe.type"
kAdobHashMapName = "%s.processedHashMap" % kAdobeDomainPrefix
kAdobHashMapVersionName = "hashMapVersion"
kAdobHashMapVersion = (1, 0)  # If major version differs, do not use.
kAutohintName = "autohint"
kCheckOutlineName = "checkOutlines"
kCheckOutlineNameUFO = "checkOutlines"

kOutlinePattern = re.compile(r"<outline.+?outline>", re.DOTALL)

COMP_TRANSFORM = OrderedDict([
    ('xScale', '1'),
    ('xyScale', '0'),
    ('yxScale', '0'),
    ('yScale', '1'),
    ('xOffset', '0'),
    ('yOffset', '0')
])


class UFOParseError(Exception):
    pass


class UFOFontData(object):
    def __init__(self, parentPath, useHashMap, programName):
        self.parentPath = parentPath
        self.glyphMap = {}
        self.processedLayerGlyphMap = {}
        self.newGlyphMap = {}
        self.glyphList = []
        self.fontInfo = None
        # If False, will skip reading hashmap and
        # testing to see if glyph can be skipped.
        # Should be used only when calling program is
        # running in report mode only, and not changing
        # any glyph data.
        self.useHashMap = useHashMap
        # Used to skip getting glyph data when glyph
        # hash matches hash of current glyph data.
        self.hashMap = {}
        self.fontDict = None
        self.programName = programName
        self.curSrcDir = None
        self.hashMapChanged = False
        self.glyphDefaultDir = os.path.join(self.parentPath, "glyphs")
        self.glyphLayerDir = os.path.join(self.parentPath,
                                          kProcessedGlyphsLayer)
        self.glyphWriteDir = self.glyphLayerDir
        self.historyList = []
        self.requiredHistory = []  # See documentation above.
        # If False, then read data only from the default layer;
        # else read glyphs from processed layer, if it exists.
        self.useProcessedLayer = False
        # If True, then write data to the default layer
        self.writeToDefaultLayer = False
        # If True, then do not skip any glyphs.
        self.doAll = False
        # track whether checkSkipGLyph has deleted an
        # out-of-date glyph from the processed glyph layer
        self.deletedGlyph = False
        # If true, do NOT round x,y values when processing
        self.allowDecimalCoords = False

        self.glyphSet = UFOReader(self.parentPath,
                                  validate=False).getGlyphSet(None)

    def getGlyphMap(self):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        return self.glyphMap

    def readHashMap(self):
        hashPath = os.path.join(self.parentPath, "data", kAdobHashMapName)
        if os.path.exists(hashPath):
            with open(hashPath, "r", encoding='utf-8') as fp:
                data = fp.read()
            newMap = ast.literal_eval(data)
        else:
            newMap = {kAdobHashMapVersionName: kAdobHashMapVersion}

        try:
            version = newMap[kAdobHashMapVersionName]
            if version[0] > kAdobHashMapVersion[0]:
                raise UFOParseError("Hash map version is newer than program. "
                                    "Please update the FDK")
            elif version[0] < kAdobHashMapVersion[0]:
                print("Updating hash map: was older version")
                newMap = {kAdobHashMapVersionName: kAdobHashMapVersion}
        except KeyError:
            print("Updating hash map: was older version")
            newMap = {kAdobHashMapVersionName: kAdobHashMapVersion}
        self.hashMap = newMap

    def writeHashMap(self):
        hashMap = self.hashMap
        if len(hashMap) == 0:
            return  # no glyphs were processed.

        hashDir = os.path.join(self.parentPath, "data")
        if not os.path.exists(hashDir):
            os.makedirs(hashDir)
        hashPath = os.path.join(hashDir, kAdobHashMapName)

        hasMapKeys = sorted(hashMap.keys())
        data = ["{"]
        for gName in hasMapKeys:
            data.append("'%s': %s," % (gName, hashMap[gName]))
        data.append("}")
        data.append("")
        data = '\n'.join(data)
        with open(hashPath, "w") as fp:
            fp.write(data)

    def getGlyphSrcPath(self, glyphName):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        glyphFileName = self.glyphMap[glyphName]

        # Try for processed layer first
        if self.useProcessedLayer and self.processedLayerGlyphMap:
            try:
                glyphFileName = self.processedLayerGlyphMap[glyphName]
                self.curSrcDir = self.glyphLayerDir
                glyphPath = os.path.join(self.glyphLayerDir, glyphFileName)
                if os.path.exists(glyphPath):
                    return glyphPath
            except KeyError:
                pass

        self.curSrcDir = self.glyphDefaultDir
        glyphPath = os.path.join(self.curSrcDir, glyphFileName)

        return glyphPath

    def getGlyphDefaultPath(self, glyphName):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        glyphFileName = self.glyphMap[glyphName]
        glyphPath = os.path.join(self.glyphDefaultDir, glyphFileName)
        return glyphPath

    def getGlyphProcessedPath(self, glyphName):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        if not self.processedLayerGlyphMap:
            return None

        try:
            glyphFileName = self.processedLayerGlyphMap[glyphName]
            glyphPath = os.path.join(self.glyphLayerDir, glyphFileName)
        except KeyError:
            glyphPath = None
        return glyphPath

    def checkSkipGlyph(self, glyphName, newSrcHash, doAll):
        skip = False
        if not self.useHashMap:
            return skip

        if len(self.hashMap) == 0:
            # Hash maps have not yet been read in. Get them.
            self.readHashMap()

        hashEntry = srcHash = None
        historyList = []
        programHistoryIndex = -1  # not found in historyList

        # Get hash entry for glyph
        try:
            hashEntry = self.hashMap[glyphName]
            srcHash, historyList = hashEntry
            try:
                programHistoryIndex = historyList.index(self.programName)
            except ValueError:
                pass
        except KeyError:
            # Glyph is as yet untouched by any program.
            pass

        if (srcHash == newSrcHash):
            if (programHistoryIndex >= 0):
                # The glyph has already been processed by this program,
                # and there have been no changes since.
                skip = True and (not doAll)
            if not skip:
                # case for Checkoutlines
                if not self.useProcessedLayer:
                    self.hashMapChanged = True
                    self.hashMap[glyphName] = [newSrcHash, [self.programName]]
                    glyphPath = self.getGlyphProcessedPath(glyphName)
                    if glyphPath and os.path.exists(glyphPath):
                        os.remove(glyphPath)
                else:
                    if (programHistoryIndex < 0):
                        historyList.append(self.programName)
        else:
            # case for autohint
            if self.useProcessedLayer:
                # Default layer glyph and stored glyph hash differ, and
                # useProcessedLayer is True. If any of the programs in
                # requiredHistory in are in the historyList, we need to
                # complain and skip.
                foundMatch = False
                if len(historyList) > 0:
                    for programName in self.requiredHistory:
                        if programName in historyList:
                            foundMatch = True
                if foundMatch:
                    print("Error. Glyph '%s' has been edited. You must first "
                          "run '%s' before running '%s'. Skipping." %
                          (glyphName, self.requiredHistory, self.programName))
                    skip = True

            # If the source hash has changed, we need to
            # delete the processed layer glyph.
            self.hashMapChanged = True
            self.hashMap[glyphName] = [newSrcHash, [self.programName]]
            glyphPath = self.getGlyphProcessedPath(glyphName)
            if glyphPath and os.path.exists(glyphPath):
                os.remove(glyphPath)
                self.deletedGlyph = True

        return skip

    @staticmethod
    def getGlyphXML(glyphDir, glyphFileName):
        glyphPath = os.path.join(glyphDir, glyphFileName)  # default
        etRoot = ET.ElementTree()
        glifXML = etRoot.parse(glyphPath)
        outlineXML = glifXML.find("outline")
        try:
            widthXML = glifXML.find("advance")
            if widthXML is not None:
                width = round(ast.literal_eval(widthXML.get("width", '0')), 9)
            else:
                width = 0
        except UFOParseError as e:
            print("Error. skipping glyph '%s' because of parse error: %s" %
                  (glyphFileName, e.message))
            return None, None, None
        return width, glifXML, outlineXML

    def getOrSkipGlyph(self, glyphName, doAll=0):
        # Get default glyph layer data, so we can check if the glyph
        # has been edited since this program was last run.
        # If the program name is in the history list, and the srcHash
        # matches the default glyph layer data, we can skip.
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        glyphFileName = self.glyphMap.get(glyphName)
        if not glyphFileName:
            return None, True  # skip

        width, glifXML, outlineXML = self.getGlyphXML(self.glyphDefaultDir,
                                                      glyphFileName)
        if glifXML is None:
            return None, True  # skip

        # Hash is always from the default glyph layer.
        useDefaultGlyphDir = True
        newHash, _ = self.buildGlyphHashValue(
            width, outlineXML, glyphName, useDefaultGlyphDir)
        skip = self.checkSkipGlyph(glyphName, newHash, doAll)

        # If self.useProcessedLayer and there is a glyph in the
        # processed layer, get the outline from that.
        if self.useProcessedLayer and self.processedLayerGlyphMap:
            try:
                glyphFileName = self.processedLayerGlyphMap[glyphName]
            except KeyError:
                pass
            glyphPath = os.path.join(self.glyphLayerDir, glyphFileName)
            if os.path.exists(glyphPath):
                width, glifXML, _ = self.getGlyphXML(
                    self.glyphLayerDir, glyphFileName)
                if glifXML is None:
                    return None, True  # skip

        return width, skip

    def loadGlyphMap(self):
        # Need to both get the list of glyphs from contents.plist, and also
        # the glyph order. The latter is take from the public.glyphOrder key
        # in lib.plist, if it exists, else it is taken from the contents.plist
        # file. Any glyphs in contents.plist which are not named in the
        # public.glyphOrder are sorted after all glyphs which are named in the
        # public.glyphOrder,, in the order that they occured in contents.plist.
        contentsPath = os.path.join(self.parentPath, "glyphs", kContentsName)
        self.glyphMap, self.glyphList = parsePList(contentsPath)
        orderPath = os.path.join(self.parentPath, kLibName)
        self.orderMap = parseGlyphOrder(orderPath)
        if self.orderMap is not None:
            orderIndex = len(self.orderMap)
            orderList = []

            # If there are glyphs in the font which are not named in the
            # public.glyphOrder entry, add then in the order of the
            # contents.plist file.
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

        # We also need to get the glyph map for the processed layer,
        # and use this when the glyph is read from the processed layer.
        # Because checkoutlinesufo used the defcon library, it can write
        # glyph file names that differ from what is in the default glyph layer.
        contentsPath = os.path.join(self.glyphLayerDir, kContentsName)
        if os.path.exists(contentsPath):
            self.processedLayerGlyphMap, self.processedLayerGlyphList = \
                parsePList(contentsPath)

    @staticmethod
    def _rd_val(str_val):
        """Round and normalize a (string) GLIF value"""
        return repr(norm_float(round(ast.literal_eval(str_val), 9)))

    def buildGlyphHashValue(self, width, outlineXML, glyphName,
                            useDefaultGlyphDir, level=0):
        """
        glyphData must be the official <outline> XML from a GLIF.
        We skip contours with only one point.
        """
        dataList = ["w%s" % norm_float(round(width, 9))]
        if level > 10:
            raise UFOParseError(
                "In parsing component, exceeded 10 levels of reference. "
                "'%s'. " % (glyphName))
        # <outline> tag is optional per spec., e.g. space glyph
        # does not necessarily have it.
        if outlineXML is not None:
            for childContour in outlineXML:
                if childContour.tag == "contour":
                    if len(childContour) < 2:
                        continue
                    for child in childContour:
                        if child.tag == "point":
                            ptType = child.get("type")
                            pointType = '' if ptType is None else ptType[0]
                            x = self._rd_val(child.get("x"))
                            y = self._rd_val(child.get("y"))
                            dataList.append("%s%s%s" % (pointType, x, y))

                elif childContour.tag == "component":
                    # append the component hash.
                    compGlyphName = childContour.get("base")

                    if compGlyphName is None:
                        raise UFOParseError(
                            "'%s' is missing the 'base' attribute in a "
                            "component." % glyphName)

                    dataList.append("%s%s" % ("base:", compGlyphName))

                    if useDefaultGlyphDir:
                        try:
                            componentPath = self.getGlyphDefaultPath(
                                compGlyphName)
                        except KeyError:
                            raise UFOParseError(
                                "'%s' component glyph is missing from "
                                "contents.plist." % (compGlyphName))
                    else:
                        # If we are not necessarily using the default layer
                        # for the main glyph, then a missing component may not
                        # have been processed, and may just be in the default
                        # layer. We need to look for component glyphs in the
                        # src list first, then in the defualt layer.
                        try:
                            componentPath = self.getGlyphSrcPath(compGlyphName)
                            if not os.path.exists(componentPath):
                                componentPath = self.getGlyphDefaultPath(
                                    compGlyphName)
                        except KeyError:
                            try:
                                componentPath = self.getGlyphDefaultPath(
                                    compGlyphName)
                            except KeyError:
                                raise UFOParseError(
                                    "'%s' component glyph is missing from "
                                    "contents.plist." % (compGlyphName))

                    if not os.path.exists(componentPath):
                        raise UFOParseError(
                            "'%s' component file is missing: '%s'." %
                            (compGlyphName, componentPath))

                    etRoot = ET.ElementTree()

                    # Collect transform values
                    for trans_key, flbk_val in COMP_TRANSFORM.items():
                        value = childContour.get(trans_key, flbk_val)
                        dataList.append(self._rd_val(value))

                    componentXML = etRoot.parse(componentPath)
                    componentOutlineXML = componentXML.find("outline")
                    _, componentDataList = self.buildGlyphHashValue(
                        width, componentOutlineXML, glyphName,
                        useDefaultGlyphDir, level + 1)
                    dataList.extend(componentDataList)
        data = "".join(dataList)
        if len(data) >= 128:
            data = hashlib.sha512(data.encode("ascii")).hexdigest()
        return data, dataList

    def close(self):
        if self.hashMapChanged:
            self.writeHashMap()
            self.hashMapChanged = False


def parseGlyphOrder(filePath):
    orderMap = None
    if os.path.exists(filePath):
        publicOrderDict, _ = parsePList(filePath, kPublicGlyphOrderKey)
        if publicOrderDict is not None:
            orderMap = {}
            glyphList = publicOrderDict[kPublicGlyphOrderKey]
            numGlyphs = len(glyphList)
            for i in range(numGlyphs):
                orderMap[glyphList[i]] = i
    return orderMap


def parsePList(filePath, dictKey=None):
    # If dictKey is defined, parse and return only the data for that key.
    #
    # Updates July 2019:
    #  - use fontTools.misc.plistlib instead of ET to parse
    #  - use built-in OrderedDict as the dict_type to preserve ordering
    #  - use simpler filtering for non-None dictKey

    plistDict = {}
    plistKeys = []

    with open(filePath, 'r', encoding='utf-8') as fp:
        plistDict = plistlib.load(fp, dict_type=OrderedDict)

    if dictKey is not None:
        if dictKey in plistDict:
            plistDict = {dictKey: plistDict[dictKey]}
        else:
            plistDict = None

    if plistDict is not None:
        plistKeys = list(plistDict.keys())

    return plistDict, plistKeys


def addWhiteSpace(parent, level):
    child = None
    childIndent = '\n' + ("  " * (level + 1))
    prentIndent = '\n' + ("  " * (level))
    # print("parent Tag", parent.tag, repr(parent.text), repr(parent.tail))
    for child in parent:
        child.tail = childIndent
        addWhiteSpace(child, level + 1)
    if child is not None:
        if parent.text is None:
            parent.text = childIndent
        child.tail = prentIndent
        # print("lastChild Tag", child.tag, repr(child.text),
        #       repr(child.tail), "parent Tag", parent.tag)


def regenerate_glyph_hashes(ufo_font_data):
    """
    The handling of the glyph hashes is super convoluted.
    This method fixes https://github.com/adobe-type-tools/afdko/issues/349
    """
    for gname, gfilename in ufo_font_data.getGlyphMap().items():
        gwidth, _, outline_xml = ufo_font_data.getGlyphXML(
            ufo_font_data.glyphDefaultDir, gfilename)
        hash_entry = ufo_font_data.hashMap.get(gname, None)
        if not hash_entry:
            continue
        ghash, _ = ufo_font_data.buildGlyphHashValue(
            gwidth, outline_xml, gname, True)
        hash_entry[0] = ghash


def checkHashMaps(fontPath, doSync):
    """
    Checks if the hashes of the glyphs in the default layer match the hash
    values stored in the UFO's 'data/com.adobe.type.processedHashMap' file.

    Returns a tuple of a boolean and a list. The boolean is True if all glyph
    hashes matched. The list contains strings that report the glyph names
    whose hash did not match.

    If doSync is True, it will delete any glyph in the processed glyph
    layer directory which does not have a matching glyph in the default
    layer, or whose source glyph hash does not match. It will then update
    the contents.plist file for the processed glyph layer, and delete
    the program specific hash maps.
    """
    msgList = []
    allMatch = True

    ufoFontData = UFOFontData(fontPath, True, '')
    ufoFontData.readHashMap()

    # Don't need to check the glyph hashes if there aren't any.
    if not ufoFontData.hashMap:
        return allMatch, msgList

    for glyphName, glyphFileName in ufoFontData.getGlyphMap().items():
        hash_entry = ufoFontData.hashMap.get(glyphName, None)
        if not hash_entry:
            continue
        else:
            oldHash = hash_entry[0]

        width, _, outlineXML = ufoFontData.getGlyphXML(
            ufoFontData.glyphDefaultDir, glyphFileName)
        if outlineXML is None:
            continue

        newHash, _ = ufoFontData.buildGlyphHashValue(
            width, outlineXML, glyphName, True)

        if oldHash != newHash:
            allMatch = False
            if len(msgList) < 10:
                msgList.append("Glyph %s seems to have been modified since "
                               "last time checkoutlinesufo processed this "
                               "font." % glyphName)
            elif len(msgList) == 10:
                msgList.append("(additional messages omitted)")

    if doSync:
        fileList = os.listdir(ufoFontData.glyphWriteDir)
        fileList = filter(lambda fileName: fileName.endswith(".glif"),
                          fileList)

        # invert glyphMap
        fileMap = {}
        for glyphName, fileName in ufoFontData.glyphMap.items():
            fileMap[fileName] = glyphName

        for fileName in fileList:
            if fileName in fileMap and (
               fileMap[fileName] in ufoFontData.hashMap):
                continue

            # Either not in glyphMap, or not in hashMap. Exterminate.
            try:
                glyphPath = os.path.join(ufoFontData.glyphWriteDir, fileName)
                os.remove(glyphPath)
                print("Removed outdated file: %s" % glyphPath)
            except OSError:
                print("Cannot delete outdated file: %s" % glyphPath)

    return allMatch, msgList


kAdobeLCALtSuffix = ".adobe.lc.altsuffix"


def cleanUpGLIFFiles(defaultContentsFilePath, glyphDirPath, doWarning=True):
    changed = 0
    contentsFilePath = os.path.join(glyphDirPath, kContentsName)
    # maps glyph names to files.

    with open(contentsFilePath, 'r', encoding='utf-8') as fp:
        contentsDict = plistlib.load(fp)

    # First, delete glyph files that are not in the contents.plist file in
    # the glyphDirPath. In some UFOfont files, we end up with case errors,
    # so we need to check for a lower-case version of the file name.
    fileDict = {}
    for glyphName, fileName in contentsDict.items():
        fileDict[fileName] = glyphName
        lcFileName = fileName.lower()
        if lcFileName != fileName:
            fileDict[lcFileName + kAdobeLCALtSuffix] = glyphName

    fileList = os.listdir(glyphDirPath)
    for fileName in fileList:
        if not fileName.endswith(".glif"):
            continue
        if fileName in fileDict:
            continue
        lcFileName = fileName.lower()
        if (lcFileName + kAdobeLCALtSuffix) in fileDict:
            # glif file exists which has a case-insensitive match to file name
            # entry in the contents.plist file; assume latter is intended, and
            # change the file name to match.
            glyphFilePathOld = os.path.join(glyphDirPath, fileName)
            glyphFilePathNew = os.path.join(glyphDirPath, lcFileName)
            os.rename(glyphFilePathOld, glyphFilePathNew)
            continue

        glyphFilePath = os.path.join(glyphDirPath, fileName)
        os.remove(glyphFilePath)
        if doWarning:
            print("Removing glif file %s that was not in the contents.plist "
                  "file: %s" % (glyphDirPath, contentsFilePath))
        changed = 1

    if defaultContentsFilePath == contentsFilePath:
        return changed

    # Now remove glyphs that are not referenced in the defaultContentsFilePath.
    # Since the processed glyph layer is written with the defcon module,
    # and the default layer may be written by anything, the actual glyph file
    # names may be different for the same UFO glyph. We need to compare by UFO
    # glyph name, not file name.

    with open(defaultContentsFilePath, 'r', encoding='utf-8') as fp:
        defaultContentsDict = plistlib.load(fp)

    fileList = os.listdir(glyphDirPath)
    for fileName in fileList:
        if not fileName.endswith(".glif"):
            continue
        try:
            glyphName = fileDict[fileName]
            if glyphName not in defaultContentsDict:
                glyphFilePath = os.path.join(glyphDirPath, fileName)
                os.remove(glyphFilePath)
                if doWarning:
                    print("Removing glif %s that was not in the "
                          "contents.plist file: %s" % (
                              glyphName, defaultContentsFilePath))
                changed = 1

        except KeyError:
            print("Shouldn't happen %s %s" % (
                glyphName, defaultContentsFilePath))

    return changed


def cleanupContentsList(glyphDirPath, doWarning=True):
    contentsFilePath = os.path.join(glyphDirPath, kContentsName)
    # maps glyph names to files.

    with open(contentsFilePath, 'r', encoding='utf-8') as fp:
        contentsDict = plistlib.load(fp)

    fileDict = {}
    fileList = os.listdir(glyphDirPath)
    for fileName in fileList:
        fileDict[fileName] = 1
    changed = 0

    # now update and write the processed processedGlyphDirPath
    # contents.plist file.
    itemList = list(contentsDict.items())
    for glyphName, fileName in itemList:
        if fileName not in fileDict:
            del contentsDict[glyphName]
            changed = 1
            if doWarning:
                print("Removing contents.plist entry where glif was missing: "
                      "%s, %s, %s" % (glyphName, fileName, glyphDirPath))

    if changed:
        with open(contentsFilePath, 'wb') as fp:
            plistlib.dump(contentsDict, fp)


def validateLayers(ufoFontPath, doWarning=True):
    # Read glyphs/contents.plist file.
    # Delete any glyphs on /glyphs or /processed glyphs which are not in
    # glyphs/contents.plist file. Delete any entries in the contents.plist
    # file which are not in the glyph files. Filter contents list with what's
    # in /processed glyphs: write to process/plist file.' The most common way
    # that this is needed in the AFDKO workflow is if someone kills
    # checkoutlines/checkoutlinesufo or autohint while it is running. Since
    # the program may delete glyphs from the processed layer while running,
    # and the contents.plist file is updated only when the changed font is
    # saved, the contents.plist file in the processed layer ends up referencing
    # glyphs that aren't there anymore. You can also get extra glyphs not in
    # the contents.plist file by several editing workflows.

    # First, clean up the default layer.
    glyphDirPath = os.path.join(ufoFontPath, "glyphs")
    defaultContentsFilePath = os.path.join(
        ufoFontPath, "glyphs", kContentsName)
    # Happens when called on a font which is not a UFO font.
    if not os.path.exists(defaultContentsFilePath):
        return

    # remove glif files not in contents.plist
    cleanUpGLIFFiles(defaultContentsFilePath, glyphDirPath, doWarning)
    # remove entries for glif files that don't exist
    cleanupContentsList(glyphDirPath, doWarning)

    # now for the processed dir.
    glyphDirPath = os.path.join(ufoFontPath, kProcessedGlyphsLayer)
    if not os.path.exists(glyphDirPath):
        return

    # Remove any glif files that are not in both the processed glif directory
    # contents.plist file and the default contents .plist file.
    # This will happen pretty often, as glif files are deleted from the
    # processed glyph layer is their hash differs from the current hash for
    # the glyph in the default layer.
    cleanUpGLIFFiles(defaultContentsFilePath, glyphDirPath, doWarning)
    cleanupContentsList(glyphDirPath, doWarning)


def makeUFOFMNDB(srcFontPath):
    fontInfoPath = os.path.join(srcFontPath, kFontInfoName)  # default
    fiMap, _ = parsePList(fontInfoPath)
    psName = "NoFamilyName-Regular"
    familyName = "NoFamilyName"
    styleName = "Regular"
    try:
        psName = fiMap["postscriptFontName"]
        parts = psName.split("-")
        familyName = parts[0]
        if len(parts) > 1:
            styleName = parts[1]
    except KeyError:
        print("ufotools [Warning] UFO font is missing 'postscriptFontName'")

    try:
        familyName = fiMap["openTypeNamePreferredFamilyName"]
    except KeyError:
        try:
            familyName = fiMap["familyName"]
        except KeyError:
            print("ufotools [Warning] UFO font is missing 'familyName'")

    try:
        styleName = fiMap["openTypeNamePreferredSubfamilyName"]
    except KeyError:
        try:
            styleName = fiMap["styleName"]
        except KeyError:
            print("ufotools [Warning] UFO font is missing 'styleName'")

    fmndbPath = fdkutils.get_temp_file_path()
    parts = []
    parts.append("[%s]" % (psName))
    parts.append("\tf=%s" % (familyName))
    parts.append("\ts=%s" % (styleName))
    parts.append("")
    data = '\n'.join(parts)
    with open(fmndbPath, "w") as fp:
        fp.write(data)
    return fmndbPath
