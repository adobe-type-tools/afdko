# Copyright 2017 Adobe. All rights reserved.

from __future__ import print_function

import hashlib
import os
import plistlib
import re
import sys

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

import ConvertFontToCID

__doc__ = """
ufoTools.py v1.30.3 Feb 01 2018

This module supports using the Adobe FDK tools which operate on 'bez'
files with UFO fonts. It provides low level utilities to manipulate UFO
data without fully parsing and instantiating UFO objects, and without
requiring that the AFDKO contain the robofab libraries.

Modified in Nov 2014, when AFDKO added the robofab libraries. It can now
be used with UFO fonts only to support the hash mechanism.

Developed in order to support checkOutlines and autohint, the code
supports two main functions:
- convert between UFO GLIF and bez formats
- keep a history of processing in a hash map, so that the (lengthy)
processing by autohint and checkOutlines can be avoided if the glyph has
already been processed, and the source data has not changed.

The basic model is:
 - read GLIF file
 - transform GLIF XML element to bez file
 - call FDK tool on bez file
 - transform new bez file to GLIF XML element with new data, and save in list

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
different layer in order to not destroy the source data. The ufoTools
defines an Adobe-specific glyph layer for processed glyphs, named
"glyphs.com.adobe.type.processedGlyphs".
checkOutlines writes new glyph files to the processed glyphs layer only
when it makes a change to the glyph data.

When the autohint program is run, the ufoTools must be able to tell
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

If the stored hash for the glyph does not exist, the ufoTools lib will save the
new hash in the hash map entry and will set the history list to contain just
the current program. The program will read the glyph from the default layer.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is in the history list,then ufoTools
will return "skip=1", and the calling program may skip the glyph.

If the stored hash matches the hash for the current glyph file in the default
layer, and the current program name is not in the history list, then the
ufoTools will return "skip=0". If the font object field 'usedProcessedLayer' is
set True, the program will read the glyph from the from the Adobe processed
layer, if it exists, else it will always read from the default layer.

If the hash differs between the hash map entry and the current glyph in the
default layer, and usedProcessedLayer is False, then ufoTools will return
"skip=0". If usedProcessedLayer is True, then the program will consult the list
of required programs. If any of these are in the history list, then the program
will report an error and return skip =1, else it will return skip=1. The
program will then save the new hash in the hash map entry and reset the history
list to contain just the current program. If the old and new hash match, but
the program name is not in the history list, then the ufoTools will not skip
the glyph, and will add the program name to the history list.


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

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring
debug = 0


def debugMsg(*args):
    if debug:
        print(args)


# UFO names
kDefaultGlyphsLayerName = "public.default"
kDefaultGlyphsLayer = "glyphs"
kProcessedGlyphsLayerName = "AFDKO ProcessedGlyphs"
kProcessedGlyphsLayer = "glyphs.com.adobe.type.processedGlyphs"
kFontInfoName = "fontinfo.plist"
kContentsName = "contents.plist"
kLibName = "lib.plist"
kPublicGlyphOrderKey = "public.glyphOrder"

kAdobeDomainPrefix = "com.adobe.type"
kAdobHashMapName = "%s.processedHashMap" % (kAdobeDomainPrefix)
kAdobHashMapVersionName = "hashMapVersion"
kAdobHashMapVersion = (1, 0)  # If major version differs, do not use.
kAutohintName = "autohint"
kCheckOutlineName = "checkOutlines"
kCheckOutlineNameUFO = "checkOutlines"

kOutlinePattern = re.compile(r"<outline.+?outline>", re.DOTALL)

kStemHintsName = "stemhints"
kStemListName = "stemList"
kStemPosName = "pos"
kStemWidthName = "width"
kHStemName = "hstem"
kVStemName = "vstem"
kHStem3Name = "hstem3"
kVStem3Name = "vstem3"
kStem3Pos = "stem3List"
kHintSetListName = "hintSetList"
kFlexListName = "hintSetList"
kHintSetName = "hintset"
kBaseFlexName = "flexCurve"
kPointTag = "pointTag"
kStemIndexName = "stemindex"
kFlexIndexListName = "flexList"
kHintDomainName1 = "com.adobe.type.autohint"
kHintDomainName2 = "com.adobe.type.autohint.v2"
kPointName = "name"
# Hint stuff
kStackLimit = 46
kStemLimit = 96

kDefaultFMNDBPath = "FontMenuNameDB"
kDefaultGOADBPath = "GlyphOrderAndAliasDB"


class UFOParseError(ValueError):
    pass


class BezParseError(ValueError):
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
        self.hashMapChanged = 0
        self.glyphDefaultDir = os.path.join(parentPath, "glyphs")
        self.glyphLayerDir = os.path.join(parentPath, kProcessedGlyphsLayer)
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

    def getUnitsPerEm(self):
        unitsPerEm = "1000"
        if self.fontInfo is None:
            self.loadFontInfo()
        if self.fontInfo:
            unitsPerEm = int(self.fontInfo["unitsPerEm"])
        unitsPerEm = int(unitsPerEm)
        return unitsPerEm

    def getPSName(self):
        psName = "PSName-Undefined"
        if self.fontInfo is None:
            self.loadFontInfo()
        if self.fontInfo:
            psName = self.fontInfo.get("postscriptFontName", psName)
        return psName

    def isCID(self):
        return 0

    def checkForHints(self, glyphName):
        hasHints = 0
        glyphPath = self.getGlyphProcessedPath(glyphName)
        if glyphPath and os.path.exists(glyphPath):
            fp = open(glyphPath, "rt")
            data = fp.read()
            fp.close()
            if "hintSetList" in data:
                hasHints = 1
        return hasHints

    def convertToBez(self, glyphName, removeHints, beVerbose, doAll=0):
        # XXX unused args: removeHints, beVerbose
        # convertGLIFToBez does not yet support
        # hints - no need for removeHints arg.
        bezString, width = convertGLIFToBez(self, glyphName, doAll)
        hasHints = self.checkForHints(glyphName)
        return bezString, width, hasHints

    def updateFromBez(self, bezData, glyphName, width, beVerbose):
        # XXX unused args: width, beVerbose
        # For UFO font, we don't use the width parameter:
        # it is carried over from the input glif file.
        glifXML = convertBezToGLIF(self, glyphName, bezData)
        self.newGlyphMap[glyphName] = glifXML

    def saveChanges(self):
        if self.hashMapChanged:
            self.writeHashMap()
        self.hashMapChanged = 0

        if not os.path.exists(self.glyphWriteDir):
            os.makedirs(self.glyphWriteDir)

        for glyphName, glifXML in self.newGlyphMap.items():
            glyphPath = self.getWriteGlyphPath(glyphName)
            # print("Saving file", glyphPath)
            fp = open(glyphPath, "wt")
            et = ET.ElementTree(glifXML)
            et.write(fp, encoding="UTF-8", xml_declaration=True)
            fp.close()

        # Update the layer contents.plist file
        layerContentsFilePath = os.path.join(
            self.parentPath, "layercontents.plist")
        self.updateLayerContents(layerContentsFilePath)
        glyphContentsFilePath = os.path.join(
            self.glyphWriteDir, "contents.plist")
        self.updateLayerGlyphContents(glyphContentsFilePath, self.newGlyphMap)

    def getWriteGlyphPath(self, glyphName):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()

        glyphFileName = self.glyphMap[glyphName]
        if not self.writeToDefaultLayer:
            if self.processedLayerGlyphMap:
                try:
                    glyphFileName = self.processedLayerGlyphMap[glyphName]
                except KeyError:
                    pass

        glifFilePath = os.path.join(self.glyphWriteDir, glyphFileName)
        return glifFilePath

    def getGlyphMap(self):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        return self.glyphMap

    def readHashMap(self):
        hashPath = os.path.join(self.parentPath, "data", kAdobHashMapName)
        if os.path.exists(hashPath):
            fp = open(hashPath, "rt")
            data = fp.read()
            fp.close()
            newMap = eval(data)
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
        return

    def writeHashMap(self):
        hashMap = self.hashMap
        if len(hashMap) == 0:
            return  # no glyphs were processed.

        hashDir = os.path.join(self.parentPath, "data")
        if not os.path.exists(hashDir):
            os.makedirs(hashDir)
        hashPath = os.path.join(hashDir, kAdobHashMapName)

        hasMapKeys = hashMap.keys()
        hasMapKeys.sort()
        data = ["{"]
        for gName in hasMapKeys:
            data.append("'%s': %s," % (gName, hashMap[gName]))
        data.append("}")
        data.append("")
        data = os.linesep.join(data)
        fp = open(hashPath, "wt")
        fp.write(data)
        fp.close()
        return

    def getCurGlyphPath(self, glyphName):
        if self.curSrcDir is None:
            self.curSrcDir = self.glyphDefaultDir

        # Get the glyph file name.
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        glyphFileName = self.glyphMap[glyphName]
        path = os.path.join(self.curSrcDir, glyphFileName)
        return path

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

    def updateHashEntry(self, glyphName, changed):
        # srcHarsh has already been set: we are fixing the history list.
        if not self.useHashMap:
            return
        # Get hash entry for glyph
        try:
            hashEntry = self.hashMap[glyphName]
            srcHash, historyList = hashEntry
        except KeyError:
            hashEntry = None

        self.hashMapChanged = 1
        # If the program always reads data from the default layer,
        # and we have just created a new glyph in the processed layer,
        # then reset the history.
        if (not self.useProcessedLayer) and changed:
            self.hashMap[glyphName] = [srcHash, [self.programName]]
            return
        # If the program is not in the history list, add it.
        elif self.programName not in historyList:
            historyList.append(self.programName)

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
                    self.hashMapChanged = 1
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
            self.hashMapChanged = 1
            self.hashMap[glyphName] = [newSrcHash, [self.programName]]
            glyphPath = self.getGlyphProcessedPath(glyphName)
            if glyphPath and os.path.exists(glyphPath):
                os.remove(glyphPath)
                self.deletedGlyph = True

        return skip

    def getGlyphXML(self, glyphDir, glyphFileName):
        glyphPath = os.path.join(glyphDir, glyphFileName)  # default
        etRoot = ET.ElementTree()
        glifXML = etRoot.parse(glyphPath)
        outlineXML = glifXML.find("outline")
        try:
            widthXML = glifXML.find("advance")
            if widthXML is not None:
                width = int(eval(widthXML.get("width")))
            else:
                width = 1000
        except UFOParseError as e:
            print("Error. skipping glyph '%s' because of parse error: %s" %
                  (glyphFileName, e.message))
            return None, None, None
        return width, glifXML, outlineXML

    def getOrSkipGlyphXML(self, glyphName, doAll=0):
        # Get default glyph layer data, so we can check if the glyph
        # has been edited since this program was last run.
        # If the program name is in the history list, and the srcHash
        # matches the default glyph layer data, we can skip.
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        glyphFileName = self.glyphMap.get(glyphName)
        if not glyphFileName:
            skip = 1
            return None, None, skip
        width, glifXML, outlineXML = self.getGlyphXML(self.glyphDefaultDir,
                                                      glyphFileName)
        if glifXML is None:
            skip = 1
            return None, None, skip

        # Hash is always from the default glyph layer.
        useDefaultGlyphDir = True
        newHash, dataList = self.buildGlyphHashValue(
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
                width, glifXML, outlineXML = self.getGlyphXML(
                    self.glyphLayerDir, glyphFileName)
                if glifXML is None:
                    skip = 1
                    return None, None, skip

        return width, outlineXML, skip

    def getGlyphList(self):
        if len(self.glyphMap) == 0:
            self.loadGlyphMap()
        return self.glyphList

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
        # Because checkOutlinesUFO used the defcon library, it can write
        # glyph file names that differ from what is in the default glyph layer.
        contentsPath = os.path.join(self.glyphLayerDir, kContentsName)
        if os.path.exists(contentsPath):
            self.processedLayerGlyphMap, self.processedLayerGlyphList = \
                parsePList(contentsPath)

    def loadFontInfo(self):
        fontInfoPath = os.path.join(self.parentPath, "fontinfo.plist")
        if not os.path.exists(fontInfoPath):
            return
        self.fontInfo, tempList = parsePList(fontInfoPath)

    def updateLayerContents(self, contentsFilePath):
        print("Calling updateLayerContents")
        if os.path.exists(contentsFilePath):
            contentsList = plistlib.readPlist(contentsFilePath)
            # If the layer name already exists,
            # don't add a new one, or change the path
            seenPublicDefault = 0
            seenProcessedGlyph = 0
            for layerName, layerPath in contentsList:
                if (layerPath == kProcessedGlyphsLayer):
                    seenProcessedGlyph = 1
                if (layerPath == kDefaultGlyphsLayer):
                    seenPublicDefault = 1
            update = 0
            if not seenPublicDefault:
                update = 1
                contentsList = [[kDefaultGlyphsLayerName,
                                kDefaultGlyphsLayer]] + contentsList
            if not seenProcessedGlyph:
                update = 1
                contentsList.append([kProcessedGlyphsLayerName,
                                    kProcessedGlyphsLayer])
            if update:
                plistlib.writePlist(contentsList, contentsFilePath)
        else:
            contentsList = [[kDefaultGlyphsLayerName, kDefaultGlyphsLayer]]
            contentsList.append([kProcessedGlyphsLayerName,
                                kProcessedGlyphsLayer])
        plistlib.writePlist(contentsList, contentsFilePath)

    def updateLayerGlyphContents(self, contentsFilePath, newGlyphData):
        if os.path.exists(contentsFilePath):
            contentsDict = plistlib.readPlist(contentsFilePath)
        else:
            contentsDict = {}
        for glyphName in newGlyphData.keys():
            # Try for processed layer first
            if self.useProcessedLayer and self.processedLayerGlyphMap:
                try:
                    contentsDict[glyphName] = \
                        self.processedLayerGlyphMap[glyphName]
                except KeyError:
                    contentsDict[glyphName] = self.glyphMap[glyphName]
            else:
                contentsDict[glyphName] = self.glyphMap[glyphName]
        plistlib.writePlist(contentsDict, contentsFilePath)

    def getFontInfo(self, fontPSName, inputPath, allow_no_blues, noFlex,
                    vCounterGlyphs, hCounterGlyphs, fdIndex=0):
        if self.fontDict is not None:
            return self.fontDict

        if self.fontInfo is None:
            self.loadFontInfo()

        fdDict = ConvertFontToCID.FDDict()
        # should be 1 if the glyphs are ideographic, else 0.
        fdDict.LanguageGroup = self.fontInfo.get("languagegroup", "0")
        fdDict.OrigEmSqUnits = self.getUnitsPerEm()
        fdDict.FontName = self.getPSName()
        upm = self.getUnitsPerEm()
        low = min(-upm * 0.25,
                  float(self.fontInfo.get("openTypeOS2WinDescent", "0")) - 200)
        high = max(upm * 1.25,
                   float(self.fontInfo.get("openTypeOS2WinAscent", "0")) + 200)
        # Make a set of inactive alignment zones: zones outside
        # of the font bbox so as not to affect hinting.
        # Used when src font has no BlueValues or has invalid BlueValues.
        # Some fonts have bad BBox values, so we don't let this be smaller
        # than -upm*0.25, upm*1.25.
        inactiveAlignmentValues = [low, low, high, high]
        blueValues = self.fontInfo.get("postscriptBlueValues", [])
        numBlueValues = len(blueValues)
        if numBlueValues < 4:
            if allow_no_blues:
                blueValues = inactiveAlignmentValues
                numBlueValues = len(blueValues)
            else:
                raise UFOParseError(
                    "ERROR: Font must have at least four values in its "
                    "BlueValues array for AC to work!")

        blueValues.sort()
        # The first pair only is a bottom zone, where the first value
        # is the overshoot position; the rest are top zones, and second
        # value of the pair is the overshoot position.
        blueValues[0] = blueValues[0] - blueValues[1]
        for i in range(3, numBlueValues, 2):
            blueValues[i] = blueValues[i] - blueValues[i - 1]

        blueValues = map(str, blueValues)
        numBlueValues = min(
            numBlueValues, len(ConvertFontToCID.kBlueValueKeys))
        for i in range(numBlueValues):
            key = ConvertFontToCID.kBlueValueKeys[i]
            value = blueValues[i]
            exec("fdDict.%s = %s" % (key, value))

        otherBlues = self.fontInfo.get("postscriptOtherBlues", [])
        numBlueValues = len(otherBlues)

        if len(otherBlues) > 0:
            i = 0
            numBlueValues = len(otherBlues)
            otherBlues.sort()
            for i in range(0, numBlueValues, 2):
                otherBlues[i] = otherBlues[i] - otherBlues[i + 1]
            otherBlues = map(str, otherBlues)
            numBlueValues = min(
                numBlueValues, len(ConvertFontToCID.kOtherBlueValueKeys))
            for i in range(numBlueValues):
                key = ConvertFontToCID.kOtherBlueValueKeys[i]
                value = otherBlues[i]
                exec("fdDict.%s = %s" % (key, value))

        vstems = self.fontInfo.get("postscriptStemSnapV", [])
        if len(vstems) == 0:
            if allow_no_blues:
                # dummy value. Needs to be larger than any hint will
                # likely be, as the autohint program strips out any
                # hint wider than twice the largest global stem width.
                vstems = [fdDict.OrigEmSqUnits]
            else:
                raise UFOParseError(
                    "ERROR: Font does not have postscriptStemSnapV!")

        vstems.sort()
        if (len(vstems) == 0) or ((len(vstems) == 1) and (vstems[0] < 1)):
            # dummy value that will allow PyAC to run
            vstems = [fdDict.OrigEmSqUnits]
            print("WARNING: There is no value or 0 value for DominantV.")
        vstems = repr(vstems)
        fdDict.DominantV = vstems

        hstems = self.fontInfo.get("postscriptStemSnapH", [])
        if len(hstems) == 0:
            if allow_no_blues:
                # dummy value. Needs to be larger than any hint will
                # likely be, as the autohint program strips out any
                # hint wider than twice the largest global stem width.
                hstems = [fdDict.OrigEmSqUnits]
            else:
                raise UFOParseError(
                    "ERROR: Font does not have postscriptStemSnapH!")

        hstems.sort()
        if (len(hstems) == 0) or ((len(hstems) == 1) and (hstems[0] < 1)):
            # dummy value that will allow PyAC to run
            hstems = [fdDict.OrigEmSqUnits]
            print("WARNING: There is no value or 0 value for DominantH.")
        hstems = repr(hstems)
        fdDict.DominantH = hstems

        if noFlex:
            fdDict.FlexOK = "false"
        else:
            fdDict.FlexOK = "true"

        # Add candidate lists for counter hints, if any.
        if vCounterGlyphs:
            temp = " ".join(vCounterGlyphs)
            fdDict.VCounterChars = "( %s )" % (temp)
        if hCounterGlyphs:
            temp = " ".join(hCounterGlyphs)
            fdDict.HCounterChars = "( %s )" % (temp)

        fdDict.BlueFuzz = self.fontInfo.get("postscriptBlueFuzz", 1)
        # postscriptBlueShift
        # postscriptBlueScale
        self.fontDict = fdDict
        return fdDict

    def getfdInfo(self, psName, inputPath, allow_no_blues, noFlex,
                  vCounterGlyphs, hCounterGlyphs, glyphList, fdIndex=0):
        fontDictList = []
        fdGlyphDict = None
        fdDict = self.getFontInfo(
            psName, inputPath, allow_no_blues, noFlex, vCounterGlyphs,
            hCounterGlyphs, fdIndex)
        fontDictList.append(fdDict)

        # Check the fontinfo file, and add any other font dicts
        srcFontInfo = os.path.dirname(inputPath)
        srcFontInfo = os.path.join(srcFontInfo, "fontinfo")
        maxX = self.getUnitsPerEm() * 2
        maxY = maxX
        minY = -self.getUnitsPerEm()
        if os.path.exists(srcFontInfo):
            fi = open(srcFontInfo, "rU")
            fontInfoData = fi.read()
            fi.close()
            fontInfoData = re.sub(r"#[^\r\n]+", "", fontInfoData)

            if "FDDict" in fontInfoData:
                blueFuzz = ConvertFontToCID.getBlueFuzz(inputPath)
                fdGlyphDict, fontDictList, finalFDict = \
                    ConvertFontToCID.parseFontInfoFile(
                        fontDictList, fontInfoData, glyphList,
                        maxY, minY, psName, blueFuzz)
                if finalFDict is None:
                    # If a font dict was not explicitly specified for the
                    # output font, use the first user-specified font dict.
                    ConvertFontToCID.mergeFDDicts(
                        fontDictList[1:], self.fontDict)
                else:
                    ConvertFontToCID.mergeFDDicts(
                        [finalFDict], self.topDict)

        return fdGlyphDict, fontDictList

    def getGlyphID(self, glyphName):
        try:
            gid = self.orderMap[glyphName]
        except IndexError:
            raise UFOParseError(
                "Could not find glyph name '%s' in UFO font contents plist. "
                "'%s'. " % (glyphName, self.parentPath))
        return gid

    def buildGlyphHashValue(self, width, outlineXML, glyphName,
                            useDefaultGlyphDir, level=0):
        """
        glyphData must be the official <outline> XML from a GLIF.
        We skip contours with only one point.
        """
        dataList = ["w%s" % (str(width))]
        if level > 10:
            raise UFOParseError(
                "In parsing component, exceeded 10 levels of reference. "
                "'%s'. " % (glyphName))
        # <outline> tag is optional per spec., e.g. space glyph
        # does not necessarily have it.
        if outlineXML:
            for childContour in outlineXML:
                if childContour.tag == "contour":
                    if len(childContour) < 2:
                        continue
                    else:
                        for child in childContour:
                            if child.tag == "point":
                                try:
                                    pointType = child.attrib["type"][0]
                                except KeyError:
                                    pointType = ""
                                dataList.append(
                                    "%s%s%s" % (pointType, child.attrib["x"],
                                                child.attrib["y"]))
                                # print(dataList[-3:])
                elif childContour.tag == "component":
                    # append the component hash.
                    try:
                        compGlyphName = childContour.attrib["base"]
                        dataList.append("%s%s" % ("base:", compGlyphName))
                    except KeyError:
                        raise UFOParseError(
                            "'%s' is missing the 'base' attribute in a "
                            "component. glyph '%s'." % (glyphName))

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

                    # Collect transformm fields, if any.
                    for transformTag in ["xScale", "xyScale", "yxScale",
                                         "yScale", "xOffset", "yOffset"]:
                        try:
                            value = childContour.attrib[transformTag]
                            rval = eval(value)
                            if int(rval) == rval:
                                value = str(int(rval))
                            dataList.append(value)
                        except KeyError:
                            pass
                    componentXML = etRoot.parse(componentPath)
                    componentOutlineXML = componentXML.find("outline")
                    componentHash, componentDataList = \
                        self.buildGlyphHashValue(
                            width, componentOutlineXML, glyphName,
                            useDefaultGlyphDir, level + 1)
                    dataList.extend(componentDataList)
        data = "".join(dataList)
        if len(data) < 128:
            hash = data
        else:
            hash = hashlib.sha512(data).hexdigest()
        return hash, dataList

    def getComponentOutline(self, componentItem):
        try:
            compGlyphName = componentItem.attrib["base"]
        except KeyError:
            raise UFOParseError(
                "'%s' attribute missing from component '%s'." %
                ("base", xmlToString(componentItem)))

        if not self.useProcessedLayer:
            try:
                compGlyphFilePath = self.getGlyphDefaultPath(compGlyphName)
            except KeyError:
                raise UFOParseError(
                    "'%s' component glyph is missing from "
                    "contents.plist." % compGlyphName)
        else:
            # If we are not necessarily using the default layer for the main
            # glyph, then a missing component may not have been processed, and
            # may just be in the default layer. We need to look for component
            # glyphs in the src list first, then in the defualt layer.
            try:
                compGlyphFilePath = self.getGlyphSrcPath(compGlyphName)
                if not os.path.exists(compGlyphFilePath):
                    compGlyphFilePath = self.getGlyphDefaultPath(compGlyphName)
            except KeyError:
                try:
                    compGlyphFilePath = self.getGlyphDefaultPath(compGlyphName)
                except KeyError:
                    raise UFOParseError(
                        "'%s' component glyph is missing from "
                        "contents.plist." % compGlyphName)

        if not os.path.exists(compGlyphFilePath):
            raise UFOParseError(
                "'%s' component file is missing: '%s'." %
                (compGlyphName, compGlyphFilePath))

        etRoot = ET.ElementTree()
        glifXML = etRoot.parse(compGlyphFilePath)
        outlineXML = glifXML.find("outline")
        return outlineXML

    def close(self):
        if self.hashMapChanged:
            self.writeHashMap()
            self.hashMapChanged = 0
        return

    def clearHashMap(self):
        self.hashMap = {kAdobHashMapVersionName: kAdobHashMapVersion}
        hashDir = os.path.join(self.parentPath, "data")
        if not os.path.exists(hashDir):
            return

        hashPath = os.path.join(hashDir, kAdobHashMapName)
        if os.path.exists(hashPath):
            os.remove(hashPath)

    def setWriteToDefault(self):
        self.useProcessedLayer = False
        self.writeToDefaultLayer = True
        self.glyphWriteDir = self.glyphDefaultDir


def parseGlyphOrder(filePath):
    orderMap = None
    if os.path.exists(filePath):
        publicOrderDict, temp = parsePList(filePath, kPublicGlyphOrderKey)
        if publicOrderDict is not None:
            orderMap = {}
            glyphList = publicOrderDict[kPublicGlyphOrderKey]
            numGlyphs = len(glyphList)
            for i in range(numGlyphs):
                orderMap[glyphList[i]] = i
    return orderMap


def parsePList(filePath, dictKey=None):
    # If dictKey is defined, parse and return only the data for that key.
    # This helps avoid needing to parse all plist types. We need to support
    # only data for known keys amd lists.
    plistDict = {}
    plistKeys = []

    # We uses this rather than the plistlib in order to get a list that
    # allows preserving key order.
    fp = open(filePath, "r")
    data = fp.read()
    fp.close()
    contents = XML(data)
    contents_dict = contents.find("dict")
    if contents_dict is None:
        raise UFOParseError("In '%s', failed to find dict. '%s'." % (
            filePath, contents_dict))
    lastTag = "string"
    for child in contents_dict:
        if child.tag == "key":
            if lastTag == "key":
                raise UFOParseError(
                    "In contents.plist, key name '%s' followed another key "
                    "name '%s'." % (xmlToString(child), lastTag))
            skipKeyData = False
            lastName = child.text
            lastTag = "key"
            if dictKey is not None:
                if lastName != dictKey:
                    skipKeyData = True
        elif child.tag != "key":
            if lastTag != "key":
                raise UFOParseError(
                    "In contents.plist, key value '%s' followed a non-key "
                    "tag '%s'." % (xmlToString(child), lastTag))
            lastTag = child.tag

            if skipKeyData:
                continue

            if lastName in plistDict:
                raise UFOParseError(
                    "Encountered duplicate key name '%s' in '%s'." %
                    (lastName, filePath))
            if child.tag == "array":
                list = []
                for listChild in child:
                    val = listChild.text
                    if listChild.tag == "integer":
                        val = int(eval(val))
                    elif listChild.tag == "real":
                        val = float(eval(val))
                    elif listChild.tag == "string":
                        pass
                    else:
                        raise UFOParseError(
                            "In plist file, encountered unhandled key type "
                            "'%s' in '%s' for parent key %s. %s." %
                            (listChild.tag, child.tag, lastName, filePath))
                    list.append(val)
                plistDict[lastName] = list
            elif child.tag == "integer":
                plistDict[lastName] = int(eval(child.text))
            elif child.tag == "real":
                plistDict[lastName] = float(eval(child.text))
            elif child.tag == "false":
                plistDict[lastName] = 0
            elif child.tag == "true":
                plistDict[lastName] = 1
            elif child.tag == "string":
                plistDict[lastName] = child.text
            else:
                raise UFOParseError(
                    "In plist file, encountered unhandled key type '%s' "
                    "for key %s. %s." % (child.tag, lastName, filePath))
            plistKeys.append(lastName)
        else:
            raise UFOParseError(
                "In plist file, encountered unexpected element '%s'. %s." %
                (xmlToString(child), filePath))
    if len(plistDict) == 0:
        plistDict = None
    return plistDict, plistKeys


class UFOTransform:
    kTransformTagList = ["xScale", "xyScale", "yxScale", "yScale",
                         "xOffset", "yOffset"]

    def __init__(self, componentXML):
        self.transformFactors = []
        self.isDefault = True
        self.isOffsetOnly = True
        hasScale = False
        hasOffset = False
        for tag in self.kTransformTagList:
            val = componentXML.attrib.get(tag, None)
            if tag in ["xScale", "yScale"]:
                if val is None:
                    val = 1.0
                else:
                    val = float(val)
                    if val != 1.0:
                        hasScale = True
            else:
                if val is None:
                    val = 0
                else:
                    val = float(val)
                    if val != 0:
                        self.isDefault = False
                        if tag in ["xyScale", "yxScale"]:
                            hasScale = True
                        elif tag in ["xOffset", "yOffset"]:
                            hasOffset = True
                        else:
                            raise UFOParseError(
                                "Unknown tag '%s' in component '%s'." %
                                (tag, xmlToString(componentXML)))

            self.transformFactors.append(val)
        if hasScale or hasOffset:
            self.isDefault = False
        if hasScale:
            self.isOffsetOnly = False

    def concat(self, transform):
        if transform is None:
            return
        if transform.isDefault:
            return
        tfCur = self.transformFactors
        tfPrev = transform.transformFactors
        if transform.isOffsetOnly:
            tfCur[4] += tfPrev[4]
            tfCur[5] += tfPrev[5]
            self.isDefault = False
        else:
            tfNew = [0.0] * 6
            tfNew[0] = tfCur[0] * tfPrev[0] + tfCur[1] * tfPrev[2]
            tfNew[1] = tfCur[0] * tfPrev[1] + tfCur[1] * tfPrev[3]
            tfNew[2] = tfCur[2] * tfPrev[0] + tfCur[3] * tfPrev[2]
            tfNew[3] = tfCur[2] * tfPrev[1] + tfCur[3] * tfPrev[3]
            tfNew[4] = tfCur[4] * tfPrev[0] + tfCur[5] * tfPrev[2] + tfPrev[4]
            tfNew[5] = tfCur[4] * tfPrev[1] + tfCur[5] * tfPrev[3] + tfPrev[5]
            self.transformFactors = tfNew
            self.isOffsetOnly = self.isOffsetOnly and transform.isOffsetOnly
            self.isDefault = False

    def apply(self, x, y):
        tfCur = self.transformFactors
        if self.isOffsetOnly:
            x += tfCur[4]
            y += tfCur[5]
        else:
            x, y = (x * tfCur[0] + y * tfCur[2] + tfCur[4],
                    x * tfCur[1] + y * tfCur[3] + tfCur[5])
        return x, y


def convertGlyphOutlineToBezString(outlineXML, ufoFontData, curX, curY,
                                   transform=None, level=0):
    """convert XML outline element containing contours and components to
    a bez string. Since xml.etree.CElementTree is compiled code, this
    will run faster than tokenizing and parsing in regular Python.

    glyphMap is a dict mapping glyph names to component file names. If it
    has a length of 1, it needs to be filled from the contents.plist file.
    It must have a key/value [UFO_FONTPATH] = path to parent UFO font.

    transformList is None, or a list of floats in the order:
    ["xScale", "xyScale", "yxScale", "yScale", "xOffset", "yOffset"]

    Build a list of outlines and components.
    For each item:
        if is outline:
            get list of points
            for each point:
                if transform matrix:
                    apply transform to coordinate
                if off line
                    push coord on stack
                if online:
                    add operator
            if is component:
                if any scale, skew, or offset factors:
                    set transform
                    call convertGlyphOutlineToBezString with transform
                    add to bez string
    I don't bother adding in any hinting info, as this is used only for making
    temp bez files as input to checkOutlines or autohint, which invalidate
    hinting data by changing the outlines data, or at best ignore hinting data.

    bez ops output: ["rrmt", "dt", "rct", "cp" , "ed"]
    """

    if level > 10:
        raise UFOParseError(
            "In parsing component, exceeded 10 levels of reference. '%s'. " %
            outlineXML)

    allowDecimals = ufoFontData.allowDecimalCoords

    bezStringList = []
    if outlineXML is None:
        bezstring = ""
    else:
        for outlineItem in outlineXML:

            if outlineItem.tag == "component":
                newTransform = UFOTransform(outlineItem)
                if newTransform.isDefault:
                    if transform is not None:
                        newTransform.concat(transform)
                    else:
                        newTransform = None
                else:
                    if transform is not None:
                        newTransform.concat(transform)
                componentOutline = ufoFontData.getComponentOutline(outlineItem)
                if componentOutline:
                    outlineBezString, curX, curY = \
                        convertGlyphOutlineToBezString(
                            componentOutline, ufoFontData, curX, curY,
                            newTransform, level + 1)
                    bezStringList.append(outlineBezString)
                continue

            if outlineItem.tag != "contour":
                continue

            # May be an empty contour.
            if len(outlineItem) == 0:
                continue

            # We have a regular contour. Iterate over points.
            argStack = []
            # Deal with setting up move-to.
            lastItem = outlineItem[0]
            try:
                type = lastItem.attrib["type"]
            except KeyError:
                type = "offcurve"
            if type in ["curve", "line", "qccurve"]:
                outlineItem = outlineItem[1:]
                if type != "line":
                    # I don't do this for line, as AC behaves
                    # differently when a final line-to is explicit.
                    outlineItem.append(lastItem)
                x = float(lastItem.attrib["x"])
                y = float(lastItem.attrib["y"])
                if transform is not None:
                    x, y = transform.apply(x, y)

                if (not allowDecimals):
                    x = int(round(x))
                    y = int(round(y))
                curX = x
                curY = y

                if (allowDecimals):
                    op = "%.3f %.3f mt" % (x, y)
                else:
                    op = "%s %s mt" % (x, y)
                bezStringList.append(op)
            elif type == "move":
                # first op is a move-to.
                if len(outlineItem) == 1:
                    # this is a move, and is the only op in this outline.
                    # Don't pass it thru. This is most likely leftover
                    # from a GLIF format 1 anchor.
                    argStack = []
                    continue
            elif type == "offcurve":
                # We should only see an off curve point as the first point
                # when the first op is a curve and the last op is a line.
                # In this case, insert a move-to to the line's coords, then
                # omit the line.
                # Breaking news! In rare cases, a first off-curve point can
                # occur when the first AND last op is a curve.
                curLastItem = outlineItem[-1]
                # In this case, insert a move-to to the last op's end pos.
                try:
                    lastType = curLastItem.attrib["type"]
                    x = float(curLastItem.attrib["x"])
                    y = float(curLastItem.attrib["y"])
                    if transform is not None:
                        x, y = transform.apply(x, y)

                    if (not allowDecimals):
                        x = int(round(x))
                        y = int(round(y))
                    curX = x
                    curY = y

                    if lastType == "line":
                        if (allowDecimals):
                            op = "%.3f %.3f mt" % (x, y)
                        else:
                            op = "%s %s mt" % (x, y)
                        bezStringList.append(op)
                        outlineItem = outlineItem[:-1]

                    elif lastType == "curve":
                        if (allowDecimals):
                            op = "%.3f %.3f mt" % (x, y)
                        else:
                            op = "%s %s mt" % (x, y)
                        bezStringList.append(op)

                except KeyError:
                    raise UFOParseError(
                        "Unhandled case for first and last points in outline "
                        "'%s'." % xmlToString(outlineItem))
            else:
                raise UFOParseError(
                    "Unhandled case for first point in outline '%s'." %
                    xmlToString(outlineItem))

            for contourItem in outlineItem:
                if contourItem.tag != "point":
                    continue
                x = float(contourItem.attrib["x"])
                y = float(contourItem.attrib["y"])
                if transform is not None:
                    x, y = transform.apply(x, y)

                if not allowDecimals:
                    x = int(round(x))
                    y = int(round(y))
                curX = x
                curY = y

                try:
                    type = contourItem.attrib["type"]
                except KeyError:
                    type = "offcurve"

                if type == "offcurve":
                    argStack.append(x)
                    argStack.append(y)
                elif type == "move":
                    if (allowDecimals):
                        op = "%.3f %.3f mt" % (x, y)
                    else:
                        op = "%s %s mt" % (x, y)
                    bezStringList.append(op)
                    argStack = []
                elif type == "line":
                    if (allowDecimals):
                        op = "%.3f %.3f dt" % (x, y)
                    else:
                        op = "%s %s dt" % (x, y)
                    bezStringList.append(op)
                    argStack = []
                elif type == "curve":
                    if len(argStack) != 4:
                        raise UFOParseError(
                            "Argument stack error seen for curve point '%s'." %
                            xmlToString(contourItem))
                    if (allowDecimals):
                        op = "%.3f %.3f %.3f %.3f %.3f %.3f ct" % (
                            argStack[0], argStack[1], argStack[2], argStack[3],
                            x, y)
                    else:
                        op = "%s %s %s %s %s %s ct" % (
                            argStack[0], argStack[1], argStack[2], argStack[3],
                            x, y)
                    argStack = []
                    bezStringList.append(op)
                elif type == "qccurve":
                    raise UFOParseError(
                        "Point type not supported '%s'." %
                        xmlToString(contourItem))
                else:
                    raise UFOParseError(
                        "Unknown Point type not supported '%s'." %
                        xmlToString(contourItem))

            # we get here only if there was at least a move.
            bezStringList.append("cp" + os.linesep)
        bezstring = os.linesep.join(bezStringList)
    return bezstring, curX, curY


def convertGLIFToBez(ufoFontData, glyphName, doAll=0):
    width, outlineXML, skip = ufoFontData.getOrSkipGlyphXML(glyphName, doAll)
    if skip:
        return None, width

    if outlineXML is None:
        return None, width

    curX = curY = 0
    bezString, curX, curY = convertGlyphOutlineToBezString(
        outlineXML, ufoFontData, curX, curY)
    bezString = (r"%%%s%ssc " % (glyphName, os.linesep)) + bezString + " ed"
    return bezString, width


class HintMask:
    # class used to collect hints for the current
    # hint mask when converting bez to T2.
    def __init__(self, listPos):
        # The index into the pointList is kept
        # so we can quickly find them later.
        self.listPos = listPos
        # These contain the actual hint values.
        self.hList = []
        self.vList = []
        self.hstem3List = []
        self.vstem3List = []
        # The name attribute of the point which follows the new hint set.
        self.pointName = "hintSet" + str(listPos).zfill(4)

    def addHintSet(self, hintSetList):
        # Add the hint set to hintSetList
        newHintSetDict = XMLElement("dict")
        hintSetList.append(newHintSetDict)

        newHintSetKey = XMLElement("key")
        newHintSetKey.text = kPointTag
        newHintSetDict.append(newHintSetKey)

        newHintSetValue = XMLElement("string")
        newHintSetValue.text = self.pointName
        newHintSetDict.append(newHintSetValue)

        stemKey = XMLElement("key")
        stemKey.text = "stems"
        newHintSetDict.append(stemKey)

        newHintSetArray = XMLElement("array")
        newHintSetDict.append(newHintSetArray)

        if (len(self.hList) > 0) or (len(self.vstem3List)):
            isH = 1
            addHintList(self.hList, self.hstem3List, newHintSetArray, isH)

        if (len(self.vList) > 0) or (len(self.vstem3List)):
            isH = 0
            addHintList(self.vList, self.vstem3List, newHintSetArray, isH)


def makeStemHintList(hintsStem3, stemList, isH):
    # In bez terms, the first coordinate in each pair is absolute,
    # second is relative, and hence is the width.
    if isH:
        op = kHStem3Name
    else:
        op = kVStem3Name
    newStem = XMLElement("string")
    posList = [op]
    for stem3 in hintsStem3:
        for pos, width in stem3:
            if isinstance(pos, float) and (int(pos) == pos):
                pos = int(pos)
            if isinstance(width, float) and (int(width) == width):
                width = int(width)
            posList.append("%s %s" % (pos, width))
    posString = " ".join(posList)
    newStem.text = posString
    stemList.append(newStem)


def makeHintList(hints, newHintSetArray, isH):
    # Add the list of hint operators
    # In bez terms, the first coordinate in each pair is absolute,
    # second is relative, and hence is the width.
    for hint in hints:
        if not hint:
            continue
        pos = hint[0]
        if isinstance(pos, float) and (int(pos) == pos):
            pos = int(pos)
        width = hint[1]
        if isinstance(width, float) and (int(width) == width):
            width = int(width)
        if isH:
            op = kHStemName
        else:
            op = kVStemName
        newStem = XMLElement("string")
        newStem.text = "%s %s %s" % (op, pos, width)
        newHintSetArray.append(newStem)


def addFlexHint(flexList, flexArray):
    for pointTag in flexList:
        newFlexTag = XMLElement("string")
        newFlexTag.text = pointTag
        flexArray.append(newFlexTag)


def fixStartPoint(outlineItem, opList):
    # For the GLIF format, the idea of first/last point is funky, because
    # the format avoids identifying a start point. This means there is no
    # implied close-path line-to. If the last implied or explicit path-close
    # operator is a line-to, then replace the "mt" with linto, and remove the
    # last explicit path-closing line-to, if any. If the last op is a curve,
    # then leave the first two point args on the stack at the end of the point
    # list, and move the last curveto to the first op, replacing the move-to.

    firstOp, firstX, firstY = opList[0]
    lastOp, lastX, lastY = opList[-1]
    firstPointElement = outlineItem[0]
    if (firstX == lastX) and (firstY == lastY):
        del outlineItem[-1]
        firstPointElement.set("type", lastOp)
    else:
        # we have an implied final line to. All we need to do is convert
        # the inital moveto to a lineto.
        firstPointElement.set("type", "line")


bezToUFOPoint = {
    "mt": 'move',
    "rmt": 'move',
    "hmt": 'move',
    "vmt": 'move',
    "rdt": 'line',
    "dt": 'line',
    "hdt": "line",
    "vdt": "line",
    "rct": 'curve',
    "ct": 'curve',
    "rcv": 'curve',  # Morisawa's alternate name for 'rct'.
    "vhct": 'curve',
    "hvct": 'curve',
}


def convertCoords(curX, curY):
    showX = int(curX)
    if showX != curX:
        showX = curX
    showY = int(curY)
    if showY != curY:
        showY = curY
    return showX, showY


def convertBezToOutline(ufoFontData, glyphName, bezString):
    """ Since the UFO outline element has no attributes to preserve, I can
    just make a new one.
    """
    # convert bez data to a UFO glif XML representation
    #
    # Convert all bez ops to simplest UFO equivalent. Add all hints to vertical
    # and horizontal hint lists as encountered; insert a HintMask class
    # whenever a new set of hints is encountered after all operators have been
    # processed, convert HintMask items into hintmask ops and hintmask bytes
    # add all hints as prefix review operator list to optimize T2 operators.
    # if useStem3 == 1, then any counter hints must be processed as stem3
    # hints, else the opposite. Counter hints are used only in LanguageGroup 1
    # glyphs, aka ideographs

    bezString = re.sub(r"%.+?\n", "", bezString)  # supress comments
    bezList = re.findall(r"(\S+)", bezString)
    if not bezList:
        return "", None, None
    flexList = []
    # Create an initial hint mask. We use this if
    # there is no explicit initial hint sub.
    hintMask = HintMask(0)
    hintMaskList = [hintMask]
    vStem3Args = []
    hStem3Args = []
    argList = []
    opList = []
    newHintMaskName = None
    inPreFlex = False
    hintInfoDict = None
    opIndex = 0
    curX = 0
    curY = 0
    newOutline = XMLElement("outline")
    outlineItem = None
    seenHints = 0

    for token in bezList:
        try:
            val = float(token)
            argList.append(val)
            continue
        except ValueError:
            pass
        if token == "newcolors":
            pass
        elif token in ["beginsubr", "endsubr"]:
            pass
        elif token in ["snc"]:
            hintMask = HintMask(opIndex)
            # If the new colors precedes any marking operator,
            # then we want throw away the initial hint mask we
            # made, and use the new one as the first hint mask.
            if opIndex == 0:
                hintMaskList = [hintMask]
            else:
                hintMaskList.append(hintMask)
            newHintMaskName = hintMask.pointName
        elif token in ["enc"]:
            pass
        elif token == "div":
            value = argList[-2] / float(argList[-1])
            argList[-2:] = [value]
        elif token == "rb":
            if newHintMaskName is None:
                newHintMaskName = hintMask.pointName
            hintMask.hList.append(argList)
            argList = []
            seenHints = 1
        elif token == "ry":
            if newHintMaskName is None:
                newHintMaskName = hintMask.pointName
            hintMask.vList.append(argList)
            argList = []
            seenHints = 1
        elif token == "rm":  # vstem3's are vhints
            if newHintMaskName is None:
                newHintMaskName = hintMask.pointName
            seenHints = 1
            vStem3Args.append(argList)
            argList = []
            if len(vStem3Args) == 3:
                hintMask.vstem3List.append(vStem3Args)
                vStem3Args = []

        elif token == "rv":  # hstem3's are hhints
            seenHints = 1
            hStem3Args.append(argList)
            argList = []
            if len(hStem3Args) == 3:
                hintMask.hstem3List.append(hStem3Args)
                hStem3Args = []

        elif token == "preflx1":
            # the preflx1/preflx2 sequence provides the same i as the flex
            # sequence; the difference is that the preflx1/preflx2 sequence
            # provides the argument values needed for building a Type1 string
            # while the flex sequence is simply the 6 rcurveto points. Both
            # sequences are always provided.
            argList = []
            # need to skip all move-tos until we see the "flex" operator.
            inPreFlex = True
        elif token == "preflx2a":
            argList = []
        elif token == "preflx2":
            argList = []
        elif token == "flxa":  # flex with absolute coords.
            inPreFlex = False
            flexPointName = kBaseFlexName + str(opIndex).zfill(4)
            flexList.append(flexPointName)
            curveCnt = 2
            i = 0
            # The first 12 args are the 6 args for each of
            # the two curves that make up the flex feature.
            while i < curveCnt:
                curX = argList[0]
                curY = argList[1]
                showX, showY = convertCoords(curX, curY)
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY})
                outlineItem.append(newPoint)
                curX = argList[2]
                curY = argList[3]
                showX, showY = convertCoords(curX, curY)
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY})
                outlineItem.append(newPoint)
                curX = argList[4]
                curY = argList[5]
                showX, showY = convertCoords(curX, curY)
                opName = 'curve'
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY,
                              "type": opName})
                outlineItem.append(newPoint)
                opList.append([opName, curX, curY])
                opIndex += 1
                if i == 0:
                    argList = argList[6:12]
                i += 1
            # attach the point name to the first point of the first curve.
            outlineItem[-6].set(kPointName, flexPointName)
            if newHintMaskName is not None:
                # We have a hint mask that we want to attach to the first
                # point of the flex op. However, there is already a flex
                # name in that attribute. What we do is set the flex point
                # name into the hint mask.
                hintMask.pointName = flexPointName
                newHintMaskName = None
            argList = []
        elif token == "flx":
            inPreFlex = False
            flexPointName = kBaseFlexName + str(opIndex).zfill(4)
            flexList.append(flexPointName)
            curveCnt = 2
            i = 0
            # The first 12 args are the 6 args for each of the two curves
            # that make up the flex feature.
            while i < curveCnt:
                curX += argList[0]
                curY += argList[1]
                showX, showY = convertCoords(curX, curY)
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY})
                outlineItem.append(newPoint)
                curX += argList[2]
                curY += argList[3]
                showX, showY = convertCoords(curX, curY)
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY})
                outlineItem.append(newPoint)
                curX += argList[4]
                curY += argList[5]
                showX, showY = convertCoords(curX, curY)
                opName = 'curve'
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY,
                              "type": opName})
                outlineItem.append(newPoint)
                opList.append([opName, curX, curY])
                opIndex += 1
                if i == 0:
                    argList = argList[6:12]
                i += 1
            # attach the point name to the first point of the first curve.
            outlineItem[-6].set(kPointName, flexPointName)
            if newHintMaskName is not None:
                # We have a hint mask that we want to attach to the first
                # point of the flex op. However, there is already a flex name
                # in that attribute. What we do is set the flex point name
                # into the hint mask.
                hintMask.pointName = flexPointName
                newHintMaskName = None
            argList = []
        elif token == "sc":
            pass
        elif token == "cp":
            pass
        elif token == "ed":
            pass
        else:
            if inPreFlex and (token[-2:] == "mt"):
                continue

            if token[-2:] in ["mt", "dt", "ct", "cv"]:
                opIndex += 1
            else:
                print("Unhandled operation %s %s" % (argList, token))
                raise BezParseError(
                    "Unhandled operation: '%s' '%s'.", argList, token)
            dx = dy = 0
            opName = bezToUFOPoint[token]
            if token[-2:] in ["mt", "dt"]:
                if token in ["mt", "dt"]:
                    curX = argList[0]
                    curY = argList[1]
                else:
                    if token in ["rmt", "rdt"]:
                        dx = argList[0]
                        dy = argList[1]
                    elif token in ["hmt", "hdt"]:
                        dx = argList[0]
                    elif token in ["vmt", "vdt"]:
                        dy = argList[0]
                    curX += dx
                    curY += dy
                showX, showY = convertCoords(curX, curY)
                newPoint = XMLElement(
                    "point", {"x": "%s" % showX, "y": "%s" % showY,
                              "type": "%s" % (opName)})

                if opName == "move":
                    if outlineItem is not None:
                        if len(outlineItem) == 1:
                            # Just in case we see 2 moves in a row, delete the
                            # previous outlineItem if it has only the move-to
                            print("Deleting moveto: %s adding %s" % (
                                xmlToString(newOutline[-1]),
                                xmlToString(outlineItem)))
                            del newOutline[-1]
                        else:
                            # Fix the start/implied end path of the
                            # previous path.
                            fixStartPoint(outlineItem, opList)
                    opList = []
                    outlineItem = XMLElement('contour')
                    newOutline.append(outlineItem)

                if newHintMaskName is not None:
                    newPoint.set(kPointName, newHintMaskName)
                    newHintMaskName = None
                outlineItem.append(newPoint)
                opList.append([opName, curX, curY])
            else:
                if token in ["ct", "cv"]:
                    curX = argList[0]
                    curY = argList[1]
                    showX, showY = convertCoords(curX, curY)
                    newPoint = XMLElement(
                        "point", {"x": "%s" % showX, "y": "%s" % showY})
                    outlineItem.append(newPoint)
                    curX = argList[2]
                    curY = argList[3]
                    showX, showY = convertCoords(curX, curY)
                    newPoint = XMLElement(
                        "point", {"x": "%s" % showX, "y": "%s" % showY})
                    outlineItem.append(newPoint)
                    curX = argList[4]
                    curY = argList[5]
                    showX, showY = convertCoords(curX, curY)
                    newPoint = XMLElement(
                        "point", {"x": "%s" % showX, "y": "%s" % showY,
                                  "type": "%s" % (opName)})
                    outlineItem.append(newPoint)
                else:
                    if token in ["rct", "rcv"]:
                        curX += argList[0]
                        curY += argList[1]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curX += argList[2]
                        curY += argList[3]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curX += argList[4]
                        curY += argList[5]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY,
                                      "type": "%s" % (opName)})
                        outlineItem.append(newPoint)
                    elif token == "vhct":
                        curY += argList[0]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curX += argList[1]
                        curY += argList[2]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curX += argList[3]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY,
                                      "type": "%s" % (opName)})
                        outlineItem.append(newPoint)
                    elif token == "hvct":
                        curX += argList[0]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curX += argList[1]
                        curY += argList[2]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY})
                        outlineItem.append(newPoint)
                        curY += argList[3]
                        showX, showY = convertCoords(curX, curY)
                        newPoint = XMLElement(
                            "point", {"x": "%s" % showX, "y": "%s" % showY,
                                      "type": "%s" % (opName)})
                        outlineItem.append(newPoint)
                if newHintMaskName is not None:
                    # attach the pointName to the first point of the curve.
                    outlineItem[-3].set(kPointName, newHintMaskName)
                    newHintMaskName = None
                opList.append([opName, curX, curY])
            argList = []
    if outlineItem is not None:
        if len(outlineItem) == 1:
            # Just in case we see two moves in a row, delete the previous
            # outlineItem if it has zero length.
            del newOutline[-1]
        else:
            fixStartPoint(outlineItem, opList)

    # add hints, if any
    # Must be done at the end of op processing to make sure we have seen
    # all the hints in the bez string.
    # Note that the hintmasks are identified in the opList by the point name.
    # We will follow the T1 spec: a glyph may have stem3 counter hints or
    # regular hints, but not both.
    if (seenHints) or (len(flexList) > 0):
        hintInfoDict = XMLElement("dict")

        idItem = XMLElement("key")
        idItem.text = "id"
        hintInfoDict.append(idItem)

        idString = XMLElement("string")
        idString.text = "id"
        hintInfoDict.append(idString)

        hintSetListItem = XMLElement("key")
        hintSetListItem.text = kHintSetListName
        hintInfoDict.append(hintSetListItem)

        hintSetListArray = XMLElement("array")
        hintInfoDict.append(hintSetListArray)
        # Convert the rest of the hint masks to a hintmask op
        # and hintmask bytes.
        for hintMask in hintMaskList:
            hintMask.addHintSet(hintSetListArray)

        if len(flexList) > 0:
            hintSetListItem = XMLElement("key")
            hintSetListItem.text = kFlexIndexListName
            hintInfoDict.append(hintSetListItem)

            flexArray = XMLElement("array")
            hintInfoDict.append(flexArray)
            addFlexHint(flexList, flexArray)
    return newOutline, hintInfoDict


def addHintList(hints, hintsStem3, newHintSetArray, isH):
    # A charstring may have regular vstem hints or vstem3 hints, but not both.
    # Same for hstem hints and hstem3 hints.
    if len(hintsStem3) > 0:
        hintsStem3.sort()
        numHints = len(hintsStem3)
        hintLimit = (kStackLimit - 2) / 2
        if numHints >= hintLimit:
            hintsStem3 = hintsStem3[:hintLimit]
            numHints = hintLimit
        makeStemHintList(hintsStem3, newHintSetArray, isH)

    else:
        hints.sort()
        numHints = len(hints)
        hintLimit = (kStackLimit - 2) / 2
        if numHints >= hintLimit:
            hints = hints[:hintLimit]
            numHints = hintLimit
        makeHintList(hints, newHintSetArray, isH)


def addWhiteSpace(parent, level):
    child = None
    childIndent = os.linesep + ("  " * (level + 1))
    prentIndent = os.linesep + ("  " * (level))
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


def convertBezToGLIF(ufoFontData, glyphName, bezString, hintsOnly=False):
    # I need to replace the contours with data from the bez string.
    glyphPath = ufoFontData.getGlyphSrcPath(glyphName)

    fp = open(glyphPath, "r")
    data = fp.read()
    fp.close()

    glifXML = XML(data)

    outlineItem = None
    libIndex = outlineIndex = -1
    outlineIndex = outlineIndex = -1
    childIndex = 0
    for childElement in glifXML:
        if childElement.tag == "outline":
            outlineItem = childElement
            outlineIndex = childIndex
        if childElement.tag == "lib":
            libIndex = childIndex
        childIndex += 1

    newOutlineElement, hintInfoDict = convertBezToOutline(
        ufoFontData, glyphName, bezString)
    # print xmlToString(stemHints)

    if not hintsOnly:
        if outlineItem is None:
            # need to add it. Add it before the lib item, if any.
            if libIndex > 0:
                glifXML.insert(libIndex, newOutlineElement)
            else:
                glifXML.append(newOutlineElement)
        else:
            # remove the old one and add the new one.
            glifXML.remove(outlineItem)
            glifXML.insert(outlineIndex, newOutlineElement)

    # convertBezToGLIF is called only if the GLIF has been edited by a tool.
    # We need to update the edit status in the has map entry.
    # I assume that convertGLIFToBez has ben run before, which will add an
    # entry for this glyph.
    ufoFontData.updateHashEntry(glyphName, changed=True)
    # Add the stem hints.
    if hintInfoDict is not None:
        widthXML = glifXML.find("advance")
        if widthXML is not None:
            width = int(eval(widthXML.get("width")))
        else:
            width = 1000
        useDefaultGlyphDir = False
        newGlyphHash, dataList = ufoFontData.buildGlyphHashValue(
            width, newOutlineElement, glyphName, useDefaultGlyphDir)
        # We add this hash to the T1 data, as it is the hash which matches
        # the output outline data. This is not necessarily the same as the
        # hash of the source data - autohint can be used to change outlines.
        if libIndex > 0:
            libItem = glifXML[libIndex]
        else:
            libItem = XMLElement("lib")
            glifXML.append(libItem)

        dictItem = libItem.find("dict")
        if dictItem is None:
            dictItem = XMLElement("dict")
            libItem.append(dictItem)

        # Remove any existing hint data.
        i = 0
        childList = list(dictItem)
        for childItem in childList:
            i += 1
            if (childItem.tag == "key") and (
               (childItem.text == kHintDomainName1) or
               (childItem.text == kHintDomainName2)):
                dictItem.remove(childItem)  # remove key
                dictItem.remove(childList[i])  # remove data item.

        glyphDictItem = dictItem
        key = XMLElement("key")
        key.text = kHintDomainName2
        glyphDictItem.append(key)

        glyphDictItem.append(hintInfoDict)

        childList = list(hintInfoDict)
        idValue = childList[1]
        idValue.text = newGlyphHash

    addWhiteSpace(glifXML, 0)
    return glifXML


def checkHashMaps(fontPath, doSync):
    """
    This function checks that all the glyphs in the processed glyph
    layer have a src glyph hash which matches the hash of the
    corresponding default layer glyph. If not, it returns an error.

    If doSync is True, it will delete any glyph in the processed glyph
    layer directory which does not have a matching glyph in the default
    layer, or whose src glyph hash does not match. It will then update
    the contents.plist file for the processed glyph layer, and delete
    the program specific hash maps.
    """
    msgList = []
    allMatch = True

    useHashMap = True
    programName = "CheckOutlines"
    ufoFontData = UFOFontData(fontPath, useHashMap, programName)
    gm = ufoFontData.getGlyphMap()
    gNameList = gm.keys()
    ufoFontData.readHashMap()
    # Don't need to check the glyph hashes if there aren't any.
    if len(ufoFontData.hashMap) > 0:
        for glyphName in gNameList:
            glyphFileName = gm[glyphName]
            glyphPath = os.path.join(
                ufoFontData.glyphDefaultDir, glyphFileName)
            etRoot = ET.ElementTree()
            glifXML = etRoot.parse(glyphPath)
            outlineXML = glifXML.find("outline")
            failedMatch = 0
            if outlineXML is not None:
                hashMap = ufoFontData.hashMap
                try:
                    widthXML = glifXML.find("advance")
                    if widthXML is not None:
                        width = int(eval(widthXML.get("width")))
                    else:
                        width = 1000
                    oldHash, historyList = hashMap[glyphName]
                    useDefaultGlyphDir = True
                    newHash, dataList = ufoFontData.buildGlyphHashValue(
                        width, outlineXML, glyphName, useDefaultGlyphDir)
                    # print("\toldHash", oldHash)
                    if oldHash != newHash:
                        failedMatch = 1
                except KeyError:
                    pass
            else:
                continue

            if failedMatch:
                allMatch = False
                if len(msgList) < 10:
                    msgList.append(
                        "src glyph %s is newer than processed layer. "
                        "programName: %s" % (glyphName, programName))
                    # print("glyph differs", glyphName)
                    # print("\told hash map:", oldHash)
                    # print("\tnew hash map:", newHash)

                elif len(msgList) == 10:
                    msgList.append(
                        "More glyphs differ between source and processed "
                        "layer.")
                else:
                    continue
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
    contentsDict = plistlib.readPlist(contentsFilePath)
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
    defaultContentsDict = plistlib.readPlist(defaultContentsFilePath)
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
    contentsDict = plistlib.readPlist(contentsFilePath)
    fileDict = {}
    fileList = os.listdir(glyphDirPath)
    for fileName in fileList:
        fileDict[fileName] = 1
    changed = 0

    # now update and write the processed processedGlyphDirPath
    # contents.plist file.
    itemList = contentsDict.items()
    for glyphName, fileName in itemList:
        if fileName not in fileDict:
            del contentsDict[glyphName]
            changed = 1
            if doWarning:
                print("Removing contents.plist entry where glif was missing: "
                      "%s, %s, %s" % (glyphName, fileName, glyphDirPath))

    if changed:
        plistlib.writePlist(contentsDict, contentsFilePath)


def validateLayers(ufoFontPath, doWarning=True):
    # Read glyphs/contents.plist file.
    # Delete any glyphs on /glyphs or /processed glyphs which are not in
    # glyphs/contents.plist file. Delete any entries in the contents.plist
    # file which are not in the glyph files. Filter contents list with what's
    # in /processed glyphs: write to process/plist file.' The most common way
    # that this is needed in the AFDKO workflow is if someone kills
    # checkOutlines/checkOutlinesUFO or autohint while it is running. Since
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


def makeUFOGOADB(srcFontPath):
    # Make a GOADB file for a UFO font.
    # Use public.glyphOrder if it exists, else use the contents.plist file.
    ufoFont = UFOFontData(srcFontPath, False, "")
    try:
        ufoFont.loadGlyphMap()
    except UFOParseError:
        return None

    goadbList = [[i, glyphName] for glyphName, i in ufoFont.orderMap.items()]
    goadbList.sort()
    ufoGOADBPath = os.path.join(srcFontPath, kDefaultGOADBPath)  # default
    fp = open(ufoGOADBPath, "wt")
    for i, glyphName in goadbList:
        fp.write("%s\t%s%s" % (glyphName, glyphName, os.linesep))
    fp.close()
    return ufoGOADBPath


def makeUFOFMNDB(srcFontPath):
    fontInfoPath = os.path.join(srcFontPath, kFontInfoName)  # default
    fiMap, fiList = parsePList(fontInfoPath)
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
        print("[Warning] UFO font is missing PostScript Name")

    try:
        familyName = fiMap["openTypeNamePreferredFamilyName"]
    except KeyError:
        try:
            familyName = fiMap["familyName"]
        except KeyError:
            print("[Warning] UFO font is missing familyName")

    try:
        styleName = fiMap["openTypeNamePreferredSubfamilyName"]
    except KeyError:
        try:
            styleName = fiMap["styleName"]
        except KeyError:
            print("[Warning] UFO font is missing styleName")

    fmndbPath = os.path.join(srcFontPath, kDefaultFMNDBPath)
    parts = []
    parts.append("[%s]" % (psName))
    parts.append("\tf=%s" % (familyName))
    parts.append("\ts=%s" % (styleName))
    parts.append("")
    data = os.linesep.join(parts)
    fp = open(fmndbPath, "wt")
    fp.write(data)
    fp.close()
    return fmndbPath


def test1():
    import pprint
    ufoFontData = UFOFontData(sys.argv[1])

    if len(sys.argv) > 2:
        gNameList = sys.argv[2:]
    else:
        gm = ufoFontData.getGlyphMap()
        gNameList = gm.keys()
    doAll = 0
    for glyphName in gNameList:
        bezString, width = convertGLIFToBez(ufoFontData, glyphName, doAll)
        glifXML = convertBezToGLIF(ufoFontData, glyphName, bezString)
        pprint.pprint(xmlToString(glifXML))
    print(len(gNameList))


def test2():
    checkHashMaps(sys.argv[1], False)


if __name__ == '__main__':
    test2()
