#!/usr/bin/env python

# Copyright 2015 Adobe. All rights reserved.

from __future__ import print_function, absolute_import, division

import os
import shutil
import sys
from subprocess import PIPE, Popen

from fontTools.misc.py23 import open, tounicode, tobytes

from defcon import Font
from mutatorMath.ufo import build as mutatorMathBuild
from ufonormalizer import normalizeUFO

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

from afdko.ufotools import validateLayers

__usage__ = """
   makeinstancesufo v1.5.0 Aug 28 2018
   makeinstancesufo -h
   makeinstancesufo -u
   makeinstancesufo [-d <design space file name>] [-a] [-c] [-n] [-dec]
                    [-i 0,1,..n]

   -d <design space file path>
      Specifies alternate path to design space file.
      Default name is 'font.designspace'.

   -a ... do NOT autohint the instances. Default is to do so.
   -c ... do NOT remove the instances' outline overlaps. Default is to do so.
   -n ... do NOT normalize the instances. Default is to do so.
   -dec . do NOT round coordinates to integer. Default is to do so.

   -i <list of instance indices>
      Specify the instances to generate. 'i' is a 0-based index of the
      instance records in the design space file.
      Example: '-i 1,4,22' -> generates only the 2nd, 5th, and 23rd instances
"""

__help__ = __usage__ + """
    makeinstancesufo generates UFO font instances from a set of master design
    UFO fonts. It uses the mutatorMath library. The paths to the masters and
    instances fonts are specified in the .designspace file.

"""

DFLT_DESIGNSPACE_FILENAME = "font.designspace"


class OptError(ValueError):
    pass


class ParseError(ValueError):
    pass


class SnapShotError(ValueError):
    pass


class Options(object):
    def __init__(self, args):
        self.dsPath = DFLT_DESIGNSPACE_FILENAME
        self.doAutoHint = True
        self.doOverlapRemoval = True
        self.doNormalize = True
        self.logFile = None
        self.allowDecimalCoords = False
        self.indexList = []
        lenArgs = len(args)

        i = 0
        hadError = 0
        while i < lenArgs:
            arg = args[i]
            i += 1

            if arg == "-h":
                logMsg.log(__help__)
                sys.exit(0)
            elif arg == "-u":
                logMsg.log(__usage__)
                sys.exit(0)
            elif arg == "-d":
                self.dsPath = args[i]
                i += 1
            elif arg == "-log":
                logMsg.sendTo(args[i])
                i += 1
            elif arg == "-a":
                self.doAutoHint = False
            elif arg == "-c":
                self.doOverlapRemoval = False
            elif arg == "-n":
                self.doNormalize = False
            elif arg in ["-dec", "-decimal"]:
                self.allowDecimalCoords = True
            elif arg == "-i":
                ilist = args[i]
                i += 1
                self.indexList = eval(ilist)
                if isinstance(self.indexList, int):
                    self.indexList = (self.indexList,)
            else:
                logMsg.log("Error: unrecognized argument:", arg)
                hadError = 1

        if hadError:
            raise(OptError)

        if not os.path.exists(self.dsPath):
            logMsg.log("Note: could not find design space file path:",
                       self.dsPath)
            hadError = 1

        if hadError:
            raise(OptError)


class LogMsg(object):
    def __init__(self, logFilePath=None):
        self.logFilePath = logFilePath
        self.logFile = None
        if self.logFilePath:
            self.logFile = open(logFilePath, "w")

    def log(self, *args):
        txt = " ".join(args)
        print(txt)
        if self.logFilePath:
            self.logFile.write(txt + os.linesep)
            self.logFile.flush()

    def sendTo(self, logFilePath):
        self.logFilePath = logFilePath
        self.logFile = open(logFilePath, "w")


logMsg = LogMsg()


def readDesignSpaceFile(options):
    """ Read design space file.
    build a new instancesList with all the instances from the ds file

    - Promote all the source and instance filename attributes from relative
      to absolute paths
    - Write a temporary ds file
    - Return a path to the temporary ds file, and the current instances list.
    """

    instanceEntryList = []
    print("Reading design space file: %s ..." % options.dsPath)

    with open(options.dsPath, "r", encoding='utf-8') as f:
        data = f.read()

    ds = ET.XML(data)

    instances = ds.find("instances")

    # Remove any instances that are not in the specified list of instance
    # indices, from the option -i.
    if (len(options.indexList) > 0):
        newInstanceXMLList = instances.findall("instance")
        numInstances = len(newInstanceXMLList)
        instanceIndex = numInstances
        while instanceIndex > 0:
            instanceIndex -= 1
            instanceXML = newInstanceXMLList[instanceIndex]
            if not (instanceIndex in options.indexList):
                instances.remove(instanceXML)

    # We want to build all remaining instances.
    for instanceXML in instances:
        familyName = instanceXML.attrib["familyname"]
        styleName = instanceXML.attrib["stylename"]
        curPath = instanceXML.attrib["filename"]
        print("Adding %s %s to build list." % (familyName, styleName))
        instanceEntryList.append(curPath)
        if os.path.exists(curPath):
            glyphDir = os.path.join(curPath, "glyphs")
            if os.path.exists(glyphDir):
                shutil.rmtree(glyphDir, ignore_errors=True)
    if not instanceEntryList:
        print("Failed to find any instances in the ds file '%s' that have the "
              "postscriptfilename attribute" % options.dsPath)
        return None, None

    dsPath = "{}.temp".format(options.dsPath)
    data = ET.tostring(ds)
    with open(dsPath, "wb") as f:
        f.write(tobytes(data, encoding='utf-8'))

    return dsPath, instanceEntryList


def updateInstance(options, fontInstancePath):
    """
    Run checkoutlinesufo and autohint, unless explicitly suppressed.
    """
    if options.doOverlapRemoval:
        logMsg.log("Doing overlap removal with checkoutlinesufo %s ..." %
                   fontInstancePath)
        logList = []
        opList = ["-e", fontInstancePath]
        if options.allowDecimalCoords:
            opList.insert(0, "-d")
        if os.name == "nt":
            opList.insert(0, 'checkoutlinesufo.exe')
            proc = Popen(opList, stdout=PIPE)
        else:
            opList.insert(0, 'checkoutlinesufo')
            proc = Popen(opList, stdout=PIPE)
        while 1:
            output = tounicode(proc.stdout.readline())
            if output:
                print('.', end='')
                logList.append(output)
            if proc.poll() is not None:
                output = proc.stdout.readline()
                if output:
                    print(output, end='')
                    logList.append(output)
                break
        log = "".join(logList)
        if not ("Done with font" in log):
            print()
            logMsg.log(log)
            logMsg.log("Error in checkoutlinesufo %s" % (fontInstancePath))
            raise(SnapShotError)
        else:
            print()

    if options.doAutoHint:
        logMsg.log("Autohinting %s ..." % (fontInstancePath))
        logList = []
        opList = ['-q', '-nb', fontInstancePath]
        if options.allowDecimalCoords:
            opList.insert(0, "-dec")
        if os.name == "nt":
            opList.insert(0, 'autohint.exe')
            proc = Popen(opList, stdout=PIPE)
        else:
            opList.insert(0, 'autohint')
            proc = Popen(opList, stdout=PIPE)
        while 1:
            output = tounicode(proc.stdout.readline())
            if output:
                print(output, end='')
                logList.append(output)
            if proc.poll() is not None:
                output = proc.stdout.readline()
                if output:
                    print(output, end='')
                    logList.append(output)
                break
        log = "".join(logList)
        if not ("Done with font" in log):
            print()
            logMsg.log(log)
            logMsg.log("Error in autohinting %s" % (fontInstancePath))
            raise(SnapShotError)
        else:
            print()
    return


def clearCustomLibs(dFont):
    for key in dFont.lib.keys():
        if key not in ['public.glyphOrder', 'public.postscriptNames']:
            del(dFont.lib[key])

    libGlyphs = [g for g in dFont if len(g.lib)]
    for g in libGlyphs:
        g.lib.clear()


def roundSelectedFontInfo(fontInfo):
    """
    'roundGeometry' is false, however, most FontInfo values have to be
    integer, with the exception of:
      - any of the postscript Blue values;
      - postscriptStemSnapH/V;
      - postscriptSlantAngle;

    The Blue values should be rounded to 2 decimal places, with the
    exception of postscriptBlueScale.

    The float values get rounded because most Type1/Type2 rasterizers store
    point and stem coordinates as Fixed numbers with 8 bits; if you pass in
    relative values with more than 2 decimal places, you end up with
    cumulative rounding errors. Other FontInfo float values are stored as
    Fixed number with 16 bits, and can support 6 decimal places.
    """
    for prop_name in fontInfo._properties.keys():
        prop_val = getattr(fontInfo, prop_name)

        if isinstance(prop_val, float):

            if prop_name == "postscriptBlueScale":
                # round to 6 places
                round_val = round(prop_val * 1000000) / 1000000.0

            elif prop_name in ('postscriptBlueFuzz', 'postscriptBlueShift',
                               'postscriptSlantAngle'):
                # round to 2 places
                round_val = round(prop_val * 100) / 100.0

            else:
                # simple round, and convert to integer
                round_val = int(round(prop_val))

            if (round_val == prop_val) and (prop_val % 1 == 0):
                # defcon does a value comparison before setting a new value
                # on the property, and it will not modify the existing value
                # if the comparison returns True, like e.g. 1 == 1.0
                # Therefore, the existing value is modified by an increment
                # ahead of setting the desired (rounded) value.
                setattr(fontInfo, prop_name, prop_val + 1)
            setattr(fontInfo, prop_name, round_val)

        elif isinstance(prop_val, list) and prop_name.startswith('postscript'):

            for i, val in enumerate(prop_val):
                if isinstance(val, float):
                    # round to 2 places
                    prop_val[i] = round(val * 100) / 100.0


def roundSelectedValues(dFont):
    """
    Glyph widths, kern values, and selected FontInfo values must be rounded,
    as these are stored in the final OTF as ints.
    """
    roundSelectedFontInfo(dFont.info)

    # round widths
    for dGlyph in dFont:
        rval = dGlyph.width
        ival = int(round(rval))
        if ival != rval:
            dGlyph.width = ival

    # round kern values, if any
    if dFont.kerning:
        keys = dFont.kerning.keys()
        for key in keys:
            rval = dFont.kerning[key]
            ival = int(round(rval))
            if ival != rval:
                dFont.kerning[key] = ival


def postProcessInstance(fontPath, options):
    dFont = Font(fontPath)
    clearCustomLibs(dFont)
    if options.allowDecimalCoords:
        roundSelectedValues(dFont)
    dFont.save()


def run(args):
    options = Options(args)

    # Set the current dir to the design space dir, so that relative paths in
    # the design space file will work.
    dsDir, dsFile = os.path.split(os.path.abspath(options.dsPath))
    os.chdir(dsDir)
    options.dsPath = dsFile

    dsPath, newInstancesList = readDesignSpaceFile(options)
    if not dsPath:
        return

    version = 2
    if len(newInstancesList) == 1:
        logMsg.log("Building 1 instance...")
    else:
        logMsg.log("Building %s instances..." % (len(newInstancesList)))
    mutatorMathBuild(documentPath=dsPath, outputUFOFormatVersion=version,
                     roundGeometry=(not options.allowDecimalCoords))
    if (dsPath != options.dsPath) and os.path.exists(dsPath):
        os.remove(dsPath)

    logMsg.log("Built %s instances." % (len(newInstancesList)))
    # Remove glyph.lib and font.lib (except for "public.glyphOrder")
    for instancePath in newInstancesList:
        postProcessInstance(instancePath, options)

    if options.doNormalize:
        logMsg.log("Applying UFO normalization...")
        for instancePath in newInstancesList:
            normalizeUFO(instancePath, outputPath=None, onlyModified=True,
                         writeModTimes=False)

    if options.doAutoHint or options.doOverlapRemoval:
        logMsg.log("Applying post-processing...")
        # Apply autohint and checkoutlines, if requested.
        for instancePath in newInstancesList:
            # make new instance font.
            updateInstance(options, instancePath)

    # checkoutlinesufo does ufotools.validateLayers()
    if not options.doOverlapRemoval:
        for instancePath in newInstancesList:
            # make sure that that there are no old glyphs left in the
            # processed glyphs folder
            validateLayers(instancePath)

    # The defcon library renames glyphs. Need to fix them again
    if options.doOverlapRemoval or options.doAutoHint:
        for instancePath in newInstancesList:
            if options.doNormalize:
                normalizeUFO(instancePath, outputPath=None, onlyModified=False,
                             writeModTimes=False)


def main():
    try:
        run(sys.argv[1:])
    except (OptError, ParseError, SnapShotError):
        logMsg.log("Quitting after error.")
        pass


if __name__ == '__main__':
    main()
