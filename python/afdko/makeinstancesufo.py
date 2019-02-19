# Copyright 2015 Adobe. All rights reserved.

"""
Generates UFO font instances from a set of master UFO fonts.
It uses the mutatorMath library. The paths to the masters and
instances fonts are specified in the .designspace file.
"""

from __future__ import print_function, absolute_import, division

import argparse
import logging
import os
import shutil
import sys

from fontTools.misc.py23 import open, tobytes

from defcon import Font
from psautohint.__main__ import main as psautohint
from ufonormalizer import normalizeUFO
from ufoProcessor import build as ufoProcessorBuild

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

from afdko.checkoutlinesufo import run as checkoutlinesUFO
from afdko.ufotools import validateLayers


__version__ = '2.2.1'

logger = logging.getLogger(__name__)


DFLT_DESIGNSPACE_FILENAME = "font.designspace"


def readDesignSpaceFile(options):
    """ Read design space file.
    build a new instancesList with all the instances from the ds file

    - Promote all the source and instance filename attributes from relative
      to absolute paths
    - Write a temporary ds file
    - Return a path to the temporary ds file, and the current instances list.
    """

    instanceEntryList = []
    logger.info("Reading design space file '%s' ..." % options.dsPath)

    with open(options.dsPath, "r", encoding='utf-8') as f:
        data = f.read()

    ds = ET.XML(data)

    instances = ds.find("instances")

    # Remove any instances that are not in the specified list of instance
    # indices, from the option -i.
    if options.indexList:
        newInstanceXMLList = instances.findall("instance")
        numInstances = len(newInstanceXMLList)
        instanceIndex = numInstances
        while instanceIndex > 0:
            instanceIndex -= 1
            instanceXML = newInstanceXMLList[instanceIndex]
            if instanceIndex not in options.indexList:
                instances.remove(instanceXML)

    # We want to build all remaining instances.
    for instanceXML in instances:
        familyName = instanceXML.attrib["familyname"]
        styleName = instanceXML.attrib["stylename"]
        curPath = instanceXML.attrib["filename"]
        logger.info("Adding %s %s to build list." % (familyName, styleName))
        instanceEntryList.append(curPath)
        if os.path.exists(curPath):
            glyphDir = os.path.join(curPath, "glyphs")
            if os.path.exists(glyphDir):
                shutil.rmtree(glyphDir, ignore_errors=True)
    if not instanceEntryList:
        logger.error("Failed to find any instances in the ds file '%s' that "
                     "have the postscriptfilename attribute" % options.dsPath)
        return None, None

    dsPath = "{}.temp".format(options.dsPath)
    data = ET.tostring(ds)
    with open(dsPath, "wb") as f:
        f.write(tobytes(data, encoding='utf-8'))

    return dsPath, instanceEntryList


def updateInstance(options, fontInstancePath):
    """
    Run checkoutlinesufo and psautohint, unless explicitly suppressed.
    """
    if options.doOverlapRemoval:
        logger.info("Doing overlap removal with checkoutlinesufo on %s ..." %
                    fontInstancePath)
        co_args = ['-e', '-q', fontInstancePath]
        if options.no_round:
            co_args.insert(0, '-d')
        try:
            checkoutlinesUFO(co_args)
        except Exception:
            raise

    if options.doAutoHint:
        logger.info("Running psautohint on %s ..." % fontInstancePath)
        ah_args = ['--no-zones-stems', fontInstancePath]
        if options.no_round:
            ah_args.insert(0, '-d')
        try:
            psautohint(ah_args)
        except Exception:
            raise


def clearCustomLibs(dFont):
    for key in list(dFont.lib.keys()):
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
    if not options.no_round:
        roundSelectedValues(dFont)
    dFont.save()


def run(options):

    # Set the current dir to the design space dir, so that relative paths in
    # the design space file will work.
    dsDir, dsFile = os.path.split(os.path.abspath(options.dsPath))
    os.chdir(dsDir)
    options.dsPath = dsFile

    dsPath, newInstancesList = readDesignSpaceFile(options)
    if not dsPath:
        return

    if len(newInstancesList) == 1:
        logger.info("Building 1 instance...")
    else:
        logger.info("Building %s instances..." % len(newInstancesList))
    ufoProcessorBuild(documentPath=dsPath,
                      outputUFOFormatVersion=options.ufo_version,
                      roundGeometry=(not options.no_round),
                      logger=logger)
    if (dsPath != options.dsPath) and os.path.exists(dsPath):
        os.remove(dsPath)

    logger.info("Built %s instances." % len(newInstancesList))
    # Remove glyph.lib and font.lib (except for "public.glyphOrder")
    for instancePath in newInstancesList:
        postProcessInstance(instancePath, options)

    if options.doNormalize:
        logger.info("Applying UFO normalization...")
        for instancePath in newInstancesList:
            normalizeUFO(instancePath, outputPath=None, onlyModified=True,
                         writeModTimes=False)

    if options.doAutoHint or options.doOverlapRemoval:
        logger.info("Applying post-processing...")
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


def _validate_path(path_str):
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            "'{}' is not a valid path.".format(path_str))
    return valid_path


def _split_comma_sequence(comma_str):
    index_lst = [item.strip() for item in comma_str.split(',')]
    int_set = set()
    for item in index_lst:
        try:
            index = int(item)
        except ValueError:
            raise argparse.ArgumentTypeError(
                "Invalid integer value: '{}'".format(item))
        if index < 0:
            raise argparse.ArgumentTypeError(
                "Index values must be greater than zero: '{}'".format(index))
        int_set.add(index)
    return sorted(int_set)


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description=__doc__
    )
    parser.add_argument(
        '--version',
        action='version',
        version=__version__
    )
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=0,
        help='verbose mode\n'
             'Use -vv for debug mode'
    )
    parser.add_argument(
        '-d',
        '--designspace',
        metavar='PATH',
        dest='dsPath',
        type=_validate_path,
        default=DFLT_DESIGNSPACE_FILENAME,
        help='path to design space file\n'
             "Default file name is '%(default)s'"
    )
    parser.add_argument(
        '-a',
        '--no-autohint',
        dest='doAutoHint',
        action='store_false',
        help='do NOT autohint the instances'
    )
    parser.add_argument(
        '-c',
        '--no-remove-overlap',
        dest='doOverlapRemoval',
        action='store_false',
        help="do NOT remove the instances' outline overlaps"
    )
    parser.add_argument(
        '-n',
        '--no-normalize',
        dest='doNormalize',
        action='store_false',
        help='do NOT normalize the instances'
    )
    parser.add_argument(
        '-r',
        '--no-round',
        action='store_true',
        help='do NOT round coordinates to integer'
    )
    parser.add_argument(
        '-i',
        metavar='INDEX(ES)',
        dest='indexList',
        type=_split_comma_sequence,
        default=[],
        help='index of instance(s) to generate\n'
             'Zero-based comma-separated sequence of integers\n'
             'Example: -i 1,4 (generates 2nd & 5th instances)'
    )
    parser.add_argument(
        '--ufo-version',
        metavar='NUMBER',
        choices=(2, 3),
        default=3,
        type=int,
        help='specify the format version number of the generated UFOs\n'
             'Default: %(default)s'
    )
    options = parser.parse_args(args)

    if not options.verbose:
        level = "WARNING"
    elif options.verbose == 1:
        level = "INFO"
    else:
        level = "DEBUG"
    logging.basicConfig(level=level)

    return options


def main(args=None):
    opts = get_options(args)

    try:
        run(opts)
    except Exception:
        raise


if __name__ == "__main__":
    sys.exit(main())
