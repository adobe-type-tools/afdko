# Copyright 2014 Adobe. All rights reserved.

"""
This module provides means of reading glyph data from and writing data
to UFO fonts, partly in virtue of the fontTools UFO library.

Developed in order to support checkOutlines and autohint, the code
supports two main functions:
- convert between UFO GLIF and GlyphData representations
- Calculate a hash value for a glyph path used to maintain a map of what
  glyphs have already been processed.

The basic model is:
 - read GLIF file
 - transform GLIF XML element to a GlyphData structure
 - operate on the GlyphData structure
 - transform the altered GlyphData back to a GLIF XML element,
   and save in list
-
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
different layer in order to not destroy the source data. The ufoFont
defines an Adobe-specific glyph layer for processed glyphs, named
"glyphs.com.adobe.type.processedGlyphs".
checkOutlines writes new glyph files to the processed glyphs layer only
when it makes a change to the glyph data.

When the autohint program is run, the ufoFont must be able to tell
whether checkOutlines has been run and has altered a glyph: if so, the
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

If the stored hash for the glyph does not exist, the ufoFont lib will save the
new hash in the hash map entry and will set the history list to contain just
the current program. The program will read the glyph from the default layer.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is in the history list,then ufoFont
will return "skip=1", and the calling program may skip the glyph.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is not in the history list, then the
ufoFont will return "skip=0". If the font object field 'usedProcessedLayer' is
set True, the program will read the glyph from the from the Adobe processed
layer, if it exists, else it will always read from the default layer.

If the hash differs between the hash map entry and the current glyph in the
default layer, and usedProcessedLayer is False, then ufoFont will return
"skip=0". If usedProcessedLayer is True, then the program will consult the
list of required programs. If any of these are in the history list, then the
program will report an error and return skip =1, else it will return skip=1.
The program will then save the new hash in the hash map entry and reset the
history list to contain just the current program. If the old and new hash
match, but the program name is not in the history list, then the ufoFont will
not skip the glyph, and will add the program name to the history list.


The only tools using this are, at the moment, checkOutlines, checkOutlinesUFO
and autohint. checkOutlines and checkOutlinesUFO use the hash map to skip
processing only when being used to edit glyphs, not when reporting them.
checkOutlines necessarily flattens any components in the source glyph file to
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

import ast
import hashlib
import logging
import os
import shutil

from types import SimpleNamespace

# from fontTools.pens.basePen import BasePen
from fontTools.pens.pointPen import AbstractPointPen
from fontTools.ufoLib import UFOReader, UFOWriter
from fontTools.ufoLib.errors import UFOLibError

from . import fdTools, FontParseError
from .glyphData import glyphData, norm_float


log = logging.getLogger(__name__)

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
PUBLIC_GLYPH_ORDER = "public.glyphOrder"

ADOBE_DOMAIN_PREFIX = "com.adobe.type"

PROCESSED_LAYER_NAME = "%s.processedglyphs" % ADOBE_DOMAIN_PREFIX
PROCESSED_GLYPHS_DIRNAME = "glyphs.%s" % PROCESSED_LAYER_NAME

HASHMAP_NAME = "%s.processedHashMap" % ADOBE_DOMAIN_PREFIX
HASHMAP_VERSION_NAME = "hashMapVersion"
HASHMAP_VERSION = (1, 0)  # If major version differs, do not use.
AUTOHINT_NAME = "autohint"
CHECKOUTLINE_NAME = "checkOutlines"

POINT_NAME_PATTERN = "hintRef%04d"
HINT_DOMAIN_NAME1 = "com.adobe.type.autohint"
HINT_DOMAIN_NAME2 = "com.adobe.type.autohint.v2"
FLEX_INDEX_LIST_NAME = "flexList"
HINT_SET_LIST_NAME = "hintSetList"
HSTEM3_NAME = "hstem3"
HSTEM_NAME = "hstem"
POINT_NAME = "name"
POINT_TAG = "pointTag"
STEMS_NAME = "stems"
VSTEM3_NAME = "vstem3"
VSTEM_NAME = "vstem"
STACK_LIMIT = 46


class UFOFontData:
    def __init__(self, path, log_only, write_to_default_layer):
        self._reader = UFOReader(path, validate=False)

        self.path = path
        self._glyphmap = None
        self._processed_layer_glyphmap = None
        self.newGlyphMap = {}
        self._fontInfo = None
        self._glyphsets = {}
        # If True, we are running in report mode and not doing any changes, so
        # we skip the hash map and process all glyphs.
        self.log_only = log_only
        # Used to store the hash of glyph data of already processed glyphs. If
        # the stored hash matches the calculated one, we skip the glyph.
        self._hashmap = None
        self.fontDict = None
        self.hashMapChanged = False
        # If True, then write data to the default layer
        self.writeToDefaultLayer = write_to_default_layer
        self.desc = None

    def getUnitsPerEm(self):
        return self.fontInfo.get("unitsPerEm", 1000)

    def getPSName(self):
        return self.fontInfo.get("postscriptFontName", "PSName-Undefined")

    def getInputPath(self):
        return self.path

    @staticmethod
    def isCID():
        return False

    def convertToGlyphData(self, name, readStems, readFlex, roundCoords,
                           doAll=False):
        glyph, skip = self._get_or_skip_glyph(name, readStems, readFlex,
                                              roundCoords, doAll)
        if skip:
            return None
        return glyph

    def updateFromGlyph(self, glyph, name):
        if glyph is None:
            try:
                self.removeHashEntry(name)
            except ValueError:
                pass
            return
        layer = None
        if name in self.processedLayerGlyphMap:
            layer = PROCESSED_LAYER_NAME
        glyphset = self._get_glyphset(layer)

        gdwrap = GlyphDataWrapper(glyph)
        glyphset.readGlyph(name, gdwrap)
        self.newGlyphMap[name] = gdwrap

        # updateFromGlyph is called only if the glyph has been autohinted
        # which might also change its outline data. We need to update the edit
        # status in the hash map entry. We assume that convertToGlyphData has
        # been run before, which will add an entry for this glyph.
        self.updateHashEntry(name)

    def save(self, path):
        if path is None:
            path = self.path

        if os.path.abspath(self.path) != os.path.abspath(path):
            # If user has specified a path other than the source font path,
            # then copy the entire UFO font, and operate on the copy.
            log.info("Copying from source UFO font to output UFO font before "
                     "processing...")
            if os.path.exists(path):
                shutil.rmtree(path)
            shutil.copytree(self.path, path)

        writer = UFOWriter(path,
                           self._reader.formatVersionTuple,
                           validate=False)

        layer = PROCESSED_LAYER_NAME
        if self.writeToDefaultLayer:
            layer = None

        # Write layer contents.
        layers = writer.layerContents.copy()
        if self.writeToDefaultLayer and PROCESSED_LAYER_NAME in layers:
            # Delete processed glyphs directory
            writer.deleteGlyphSet(PROCESSED_LAYER_NAME)
            # Remove entry from 'layercontents.plist' file
            del layers[PROCESSED_LAYER_NAME]
        elif self.processedLayerGlyphMap or not self.writeToDefaultLayer:
            layers[PROCESSED_LAYER_NAME] = PROCESSED_GLYPHS_DIRNAME
        writer.layerContents.update(layers)
        writer.writeLayerContents()

        # Write glyphs.
        glyphset = writer.getGlyphSet(layer, defaultLayer=layer is None)
        for name, glyph in self.newGlyphMap.items():
            filename = self.glyphMap[name]
            if not self.writeToDefaultLayer and \
                    name in self.processedLayerGlyphMap:
                filename = self.processedLayerGlyphMap[name]
            # Recalculate glyph hashes
            if self.writeToDefaultLayer:
                self.recalcHashEntry(name, glyph)
            glyphset.contents[name] = filename
            glyphset.writeGlyph(name, glyph, glyph.drawPoints)
        glyphset.writeContents()

        # Write hashmap
        if self.hashMapChanged:
            self.writeHashMap(writer)

    @property
    def hashMap(self):
        if self._hashmap is None:
            try:
                data = self._reader.readData(HASHMAP_NAME)
            except UFOLibError:
                data = None
            if data:
                hashmap = ast.literal_eval(data.decode("utf-8"))
            else:
                hashmap = {HASHMAP_VERSION_NAME: HASHMAP_VERSION}

            version = (0, 0)
            if HASHMAP_VERSION_NAME in hashmap:
                version = hashmap[HASHMAP_VERSION_NAME]

            if version[0] > HASHMAP_VERSION[0]:
                raise FontParseError("Hash map version is newer than "
                                     "otfautohint. Please update.")
            elif version[0] < HASHMAP_VERSION[0]:
                log.info("Updating hash map: was older version")
                hashmap = {HASHMAP_VERSION_NAME: HASHMAP_VERSION}

            self._hashmap = hashmap
        return self._hashmap

    def writeHashMap(self, writer):
        hashMap = self.hashMap
        if not hashMap:
            return  # no glyphs were processed.

        data = ["{"]
        for gName in sorted(hashMap.keys()):
            data.append("'%s': %s," % (gName, hashMap[gName]))
        data.append("}")
        data.append("")
        data = "\n".join(data)

        writer.writeData(HASHMAP_NAME, data.encode("utf-8"))

    def updateHashEntry(self, glyphName):
        # srcHash has already been set: we are fixing the history list.

        # Get hash entry for glyph
        srcHash, historyList = self.hashMap[glyphName]

        self.hashMapChanged = True
        # If the program is not in the history list, add it.
        if AUTOHINT_NAME not in historyList:
            historyList.append(AUTOHINT_NAME)

    def removeHashEntry(self, glyphName):
        del self.hashMap[glyphName]
        self.hashMapChanged = True

    def recalcHashEntry(self, glyphName, glyph):
        hashBefore, historyList = self.hashMap[glyphName]

        hash_pen = HashPointPen(glyph)
        glyph.drawPoints(hash_pen, ufoHintLib=False)
        hashAfter = hash_pen.getHash()

        if hashAfter != hashBefore:
            self.hashMap[glyphName] = [hashAfter, historyList]
            self.hashMapChanged = True

    def checkSkipGlyph(self, glyphName, newSrcHash, doAll):
        skip = False
        if self.log_only:
            return skip

        srcHash = None
        historyList = []

        # Get hash entry for glyph
        if glyphName in self.hashMap:
            srcHash, historyList = self.hashMap[glyphName]

        if srcHash == newSrcHash:
            if AUTOHINT_NAME in historyList:
                # The glyph has already been autohinted, and there have been no
                # changes since.
                skip = not doAll
            if not skip and AUTOHINT_NAME not in historyList:
                historyList.append(AUTOHINT_NAME)
        else:
            if CHECKOUTLINE_NAME in historyList:
                log.error("Glyph '%s' has been edited. You must first "
                          "run '%s' before running '%s'. Skipping.",
                          glyphName, CHECKOUTLINE_NAME, AUTOHINT_NAME)
                skip = True

            # If the source hash has changed, we need to delete the processed
            # layer glyph.
            self.hashMapChanged = True
            self.hashMap[glyphName] = [newSrcHash, [AUTOHINT_NAME]]
            if glyphName in self.processedLayerGlyphMap:
                del self.processedLayerGlyphMap[glyphName]

        return skip

    def _get_glyphset(self, layer_name=None):
        if layer_name not in self._glyphsets:
            glyphset = None
            try:
                glyphset = self._reader.getGlyphSet(layer_name)
            except UFOLibError:
                pass
            self._glyphsets[layer_name] = glyphset
        return self._glyphsets[layer_name]

    @staticmethod
    def get_glyph_data(glyph, name, readStems, readFlex, roundCoords,
                       glyphset):
        gl = glyphData(roundCoords=roundCoords, name=name)
        gl.glyphSet = glyphset  # To support addComponent in read phase
        glyph.draw(gl)
        del gl.glyphSet
        if not hasattr(glyph, "width"):
            glyph.width = 0
        gl.setWidth(glyph.width)
        return gl

    def _get_or_skip_glyph(self, name, readStems, readFlex, roundCoords,
                           doAll):
        # Get default glyph layer data, so we can check if the glyph
        # has been edited since this program was last run.
        # If the program name is in the history list, and the srcHash
        # matches the default glyph layer data, we can skip.
        glyphset = self._get_glyphset()
        glyph = glyphset[name]
        glyph_data = self.get_glyph_data(glyph, name, readStems, readFlex,
                                         roundCoords, glyphset)

        # Hash is always from the default glyph layer.
        hash_pen = HashPointPen(glyph, glyphset)
        glyph.drawPoints(hash_pen)
        skip = self.checkSkipGlyph(name, hash_pen.getHash(), doAll)

        # If there is a glyph in the processed layer, get the outline from it.
        if name in self.processedLayerGlyphMap:
            glyphset = self._get_glyphset(PROCESSED_LAYER_NAME)
            glyph = glyphset[name]
            glyph_data = self.get_glyph_data(glyph, name, readStems, readFlex,
                                             roundCoords, glyphset)
        return glyph_data, skip

    def getGlyphList(self):
        glyphOrder = self._reader.readLib().get(PUBLIC_GLYPH_ORDER, [])
        glyphList = list(self._get_glyphset().keys())

        # Sort the returned glyph list by the glyph order as we depend in the
        # order for expanding glyph ranges.
        def key_fn(v):
            if v in glyphOrder:
                return glyphOrder.index(v)
            return len(glyphOrder)
        return sorted(glyphList, key=key_fn)

    @property
    def glyphMap(self):
        if self._glyphmap is None:
            glyphset = self._get_glyphset()
            self._glyphmap = glyphset.contents
        return self._glyphmap

    @property
    def processedLayerGlyphMap(self):
        if self._processed_layer_glyphmap is None:
            self._processed_layer_glyphmap = {}
            glyphset = self._get_glyphset(PROCESSED_LAYER_NAME)
            if glyphset is not None:
                self._processed_layer_glyphmap = glyphset.contents
        return self._processed_layer_glyphmap

    @property
    def fontInfo(self):
        if self._fontInfo is None:
            info = SimpleNamespace()
            self._reader.readInfo(info)
            self._fontInfo = vars(info)
        return self._fontInfo

    def getPrivateFDDict(self, allow_no_blues, noFlex, vCounterGlyphs,
                         hCounterGlyphs, desc, fdIndex=None):
        if self.fontDict is not None:
            return self.fontDict

        assert fdIndex is None or fdIndex == 0

        fdDict = fdTools.FDDict(fdIndex, fontName=desc)
        # should be 1 if the glyphs are ideographic, else 0.
        fdDict.setInfo('LanguageGroup',
                       self.fontInfo.get("languagegroup", "0"))
        upm = self.getUnitsPerEm()
        fdDict.setInfo('OrigEmSqUnits', upm)
        low = min(-upm * 0.25,
                  self.fontInfo.get("openTypeOS2WinDescent", 0) - 200)
        high = max(upm * 1.25,
                   self.fontInfo.get("openTypeOS2WinAscent", 0) + 200)
        # Make a set of inactive alignment zones: zones outside of the font
        # bbox so as not to affect hinting. Used when src font has no
        # BlueValues or has invalid BlueValues. Some fonts have bad BBox
        # values, so I don't let this be smaller than -upm*0.25, upm*1.25.
        inactiveAlignmentValues = [low, low, high, high]
        blueValues = self.fontInfo.get("postscriptBlueValues", [])
        numBlueValues = len(blueValues)
        if numBlueValues < 4:
            if allow_no_blues:
                blueValues = inactiveAlignmentValues
                numBlueValues = len(blueValues)
            else:
                raise FontParseError(
                    "Font must have at least four values in its "
                    "BlueValues array for PSAutoHint to work!")
        blueValues.sort()
        # The first pair only is a bottom zone, where the first value is the
        # overshoot position; the rest are top zones, and second value of the
        # pair is the overshoot position.
        blueValues[0] = blueValues[0] - blueValues[1]
        for i in range(3, numBlueValues, 2):
            blueValues[i] = blueValues[i] - blueValues[i - 1]

        numBlueValues = min(numBlueValues, len(fdTools.kBlueValueKeys))
        for i in range(numBlueValues):
            key = fdTools.kBlueValueKeys[i]
            value = blueValues[i]
            fdDict.setInfo(key, value)

        otherBlues = self.fontInfo.get("postscriptOtherBlues", [])

        if len(otherBlues) > 0:
            i = 0
            numBlueValues = len(otherBlues)
            otherBlues.sort()
            for i in range(0, numBlueValues, 2):
                otherBlues[i] = otherBlues[i] - otherBlues[i + 1]
            numBlueValues = min(numBlueValues,
                                len(fdTools.kOtherBlueValueKeys))
            for i in range(numBlueValues):
                key = fdTools.kOtherBlueValueKeys[i]
                value = otherBlues[i]
                fdDict.setInfo(key, value)

        vstems = self.fontInfo.get("postscriptStemSnapV", [])
        if not vstems:
            if allow_no_blues:
                # dummy value. Needs to be larger than any hint will likely be,
                # as the autohint program strips out any hint wider than twice
                # the largest global stem width.
                vstems = [fdDict.OrigEmSqUnits]
            else:
                raise FontParseError("Font does not have postscriptStemSnapV!")

        vstems.sort()
        if not vstems or (len(vstems) == 1 and vstems[0] < 1):
            # dummy value that will allow PyAC to run
            vstems = [fdDict.OrigEmSqUnits]
            log.warning("There is no value or 0 value for DominantV.")
        fdDict.setInfo('DominantV', vstems)

        hstems = self.fontInfo.get("postscriptStemSnapH", [])
        if not hstems:
            if allow_no_blues:
                # dummy value. Needs to be larger than any hint will likely be,
                # as the autohint program strips out any hint wider than twice
                # the largest global stem width.
                hstems = [fdDict.OrigEmSqUnits]
            else:
                raise FontParseError("Font does not have postscriptStemSnapH!")

        hstems.sort()
        if not hstems or (len(hstems) == 1 and hstems[0] < 1):
            # dummy value that will allow PyAC to run
            hstems = [fdDict.OrigEmSqUnits]
            log.warning("There is no value or 0 value for DominantH.")
        fdDict.setInfo('DominantH', hstems)

        if noFlex:
            fdDict.setInfo('FlexOK', False)
        else:
            fdDict.setInfo('FlexOK', True)

        # Add candidate lists for counter hints, if any.
        if vCounterGlyphs:
            fdDict.setInfo('VCounterChars', vCounterGlyphs)
        if hCounterGlyphs:
            fdDict.setInfo('HCounterChars', hCounterGlyphs)

        fdDict.setInfo('BlueFuzz', self.fontInfo.get("postscriptBlueFuzz", 1))
        fdDict.buildBlueLists()
        # postscriptBlueShift
        # postscriptBlueScale
        self.fontDict = fdDict
        return fdDict

    def getfdIndex(self, name):
        # XXX update for postscriptFDArray in lib.plist
        return 0

    def isVF(self):
        return False

    def get_min_max(self, upm):
        return -upm, 2 * upm

    def mergePrivateMap(self, privateMap):
        for k, v in privateMap.items():
            setattr(self.fontDict, k, v)

    @staticmethod
    def close():
        return


class HashPointPen(AbstractPointPen):

    def __init__(self, glyph, glyphset=None):
        self.glyphset = glyphset
        self.width = norm_float(round(getattr(glyph, "width", 0), 9))
        self.data = ["w%s" % self.width]

    def getHash(self):
        data = "".join(self.data)
        if len(data) >= 128:
            data = hashlib.sha512(data.encode("ascii")).hexdigest()
        return data

    def beginPath(self, identifier=None, **kwargs):
        pass

    def endPath(self):
        pass

    def addPoint(self, pt, segmentType=None, smooth=False, name=None,
                 identifier=None, **kwargs):
        if segmentType is None:
            pt_type = ""
        else:
            pt_type = segmentType[0]
        self.data.append("%s%s%s" % (pt_type,
                                     repr(norm_float(round(pt[0], 9))),
                                     repr(norm_float(round(pt[1], 9)))))

    def addComponent(self, baseGlyphName, transformation, identifier=None,
                     **kwargs):
        # UFO glif files can reference other glifs as "components", so the
        # hash algorithm needs to deal with this. However, hinted glyphs
        # do not have components, so there is no need to track and
        # use the associated glyphset when calculating hinted hashes. As
        # that is the only circumstance where glyphset should be None, we
        # should never reach this condition.
        if self.glyphset is None:
            raise FontParseError("Internal error: addComponent called " +
                                 "when UFO glyphset is not set")

        self.data.append("base:%s" % baseGlyphName)

        for v in transformation:
            self.data.append(str(norm_float(round(v, 9))))

        self.data.append("w%s" % self.width)
        glyph = self.glyphset[baseGlyphName]
        glyph.drawPoints(self)


class GlyphDataWrapper(object):
    """
    Wraps a glyphData object while storing the properties set by readGlyph
    to aid output of hint data in Adobe's "hint format 2" for UFO.
    """
    def __init__(self, glyph):
        self._glyph = glyph
        self.lib = {}
        if hasattr(glyph, 'width'):
            self.width = norm_float(glyph.width)

    def addUfoFlex(self, uhl, pointname):
        """Mark the named point as starting a flex hint"""
        if uhl.get(FLEX_INDEX_LIST_NAME, None) is None:
            uhl[FLEX_INDEX_LIST_NAME] = []
        uhl[FLEX_INDEX_LIST_NAME].append(pointname)

    def addUfoMask(self, uhl, masks, pointname):
        """Associates the hint set represented by masks with the named point"""
        if uhl.get(HINT_SET_LIST_NAME, None) is None:
            uhl[HINT_SET_LIST_NAME] = []

        glyph = self._glyph
        iscntr = [False, False]
        opname = [HSTEM_NAME, VSTEM_NAME]
        cntropname = [HSTEM3_NAME, VSTEM3_NAME]
        if glyph.cntr and glyph.cntr[0]:
            for i in range(2):
                iscntr[i] = True in glyph.cntr[0][i]

        ustems = []
        for i, stems in enumerate([glyph.hstems, glyph.vstems]):
            if masks and masks[i]:
                sl = [s for (s, b) in zip(stems, masks[i]) if b]
            else:
                sl = stems
            if iscntr[i]:
                pl = [cntropname[i]]
                for s in sl:
                    pl.extend((str(norm_float(v)) for v in s.UFOVals()))
                ustems.append(" ".join(pl))
            else:
                for s in sl:
                    p, w = s.UFOVals()
                    ustems.append("%s %s %s" % (opname[i], norm_float(p),
                                                norm_float(w)))
        hintset = {}
        hintset[POINT_TAG] = pointname
        hintset[STEMS_NAME] = ustems
        uhl[HINT_SET_LIST_NAME].append(hintset)

    def addUfoHints(self, uhl, pe, labelnum, startSubpath=False):
        """Adds hints to the pathElement, naming points as necessary"""
        pn = POINT_NAME_PATTERN % labelnum
        if uhl is None:
            # Not recording hints
            return labelnum, None
        if not startSubpath or labelnum != 0:
            # No hint data to record
            if pe.flex != 1 and not pe.masks:
                return labelnum, None
            if pe.flex == 1:
                self.addUfoFlex(uhl, pn)
            if pe.masks:
                self.addUfoMask(uhl, pe.masks, pn)
        else:
            # First call, record top-level mask (if any)
            self.addUfoMask(uhl, self._glyph.startmasks, pn)
        return labelnum + 1, pn

    def drawPoints(self, pen, ufoHintLib=True):
        """
        Calls pointPen commands on pen to draw the glyph, optionally naming
        some points and building a library of hint annotations
        """
        if ufoHintLib is not None:
            uhl = {}
            ufoH = lambda pe, lm, ss=False: self.addUfoHints(uhl, pe, lm, ss)
        else:
            ufoH = None

        self._glyph.drawPoints(pen, ufoH)

        if ufoHintLib is not None:
            # Add this hash to the glyph data, as it is the hash which matches
            # the output outline data. This is not necessarily the same as the
            # hash of the source data; autohint can be used to change outlines.
            hash_pen = HashPointPen(self)
            self._glyph.drawPoints(hash_pen, None)
            uhl["id"] = hash_pen.getHash()

            # Remove any existing hint data.
            for key in (HINT_DOMAIN_NAME1, HINT_DOMAIN_NAME2):
                if key in self.lib:
                    del self.lib[key]

            self.lib[HINT_DOMAIN_NAME2] = uhl
