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

from fontTools.designspaceLib import (
    DesignSpaceDocument,
    DesignSpaceDocumentError,
)
from fontTools.misc.py23 import open

from defcon import Font
from psautohint.__main__ import main as psautohint
from ufonormalizer import normalizeUFO
from ufoProcessor import build as ufoProcessorBuild

from afdko.checkoutlinesufo import run as checkoutlinesUFO
from afdko.fdkutils import get_temp_file_path
from afdko.ufotools import validateLayers


__version__ = '2.3.1'

logger = logging.getLogger(__name__)


DFLT_DESIGNSPACE_FILENAME = "font.designspace"
FEATURES_FILENAME = "features.fea"


def filterDesignspaceInstances(dsDoc, options):
    """
    - Filter unwanted instances out of dsDoc as specified by -i option
      (options.indexList), which has already been validated.
    - Promote dsDoc.instance.paths to absolute, referring to original
      dsDoc's location.
    - Remove any existing instance
    - Write the modified doc to a proper temp file
    - Return the path to the temp DS file.
    """

    origDSPath = options.dsPath
    filteredInstances = []

    for idx in sorted(options.indexList):
        instance = dsDoc.instances[idx]
        instance.path = os.path.abspath(
            os.path.realpath(
                os.path.join(
                    origDSPath,
                    instance.path,
                )
            )
        )

        if os.path.exists(instance.path):
            glyphDir = os.path.join(instance.path, "glyphs")
            if os.path.exists(glyphDir):
                shutil.rmtree(glyphDir, ignore_errors=True)

        filteredInstances.append(instance)

    dsDoc.instances = filteredInstances
    tmpPath = get_temp_file_path()
    dsDoc.write(tmpPath)

    return tmpPath


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


def validateDesignspaceDoc(dsDoc, dsoptions, **kwArgs):
    """
    Validate the dsDoc DesignSpaceDocument object, using supplied dsoptions
    and kwArgs. Raises Exceptions if certain criteria are not met. These
    are above and beyond the basic validations in fontTools.designspaceLib
    and are specific to makeinstancesufo.
    """
    if dsDoc.sources:
        for src in dsDoc.sources:
            if not os.path.exists(src.path):
                raise DesignSpaceDocumentError(
                    "Source file {} does not exist".format(
                        src.path,
                    ))
    else:
        raise DesignSpaceDocumentError(
            "Designspace file contains no sources."
        )

    if dsDoc.instances:
        if dsoptions.indexList:
            # bounds check on indexList
            maxinstidx = max(dsoptions.indexList)
            if maxinstidx >= len(dsDoc.instances):
                raise IndexError("Instance index {} out-of-range".format(
                    maxinstidx,
                ))
    else:
        raise DesignSpaceDocumentError(
            "Designspace file contains no instances."
        )


def collect_features_content(instances, inst_idx_lst):
    """
    Returns a dictionary whose keys are 'features.fea' file paths, and the
    values are the contents of the corresponding file.
    """
    fea_dict = {}
    for i, inst_dscrpt in enumerate(instances):
        if inst_idx_lst and i not in inst_idx_lst:
            continue
        ufo_pth = inst_dscrpt.path
        if ufo_pth is None:
            continue
        ufo_pth = os.path.abspath(os.path.realpath(ufo_pth))
        fea_pth = os.path.join(ufo_pth, FEATURES_FILENAME)
        if os.path.isfile(fea_pth):
            with open(fea_pth, 'rb') as fp:
                fea_cntnts = fp.read()
            fea_dict[fea_pth] = fea_cntnts
    return fea_dict


def run(options):

    ds_doc = DesignSpaceDocument.fromfile(options.dsPath)

    # can still have a successful read but useless DSD (no sources, no
    # instances, sources non-existent, other conditions
    validateDesignspaceDoc(ds_doc, options)

    copy_features = any(src.copyFeatures for src in ds_doc.sources)
    features_store = {}
    if not copy_features:
        # '<features copy="1"/>' is NOT set in any of masters.
        # Collect the contents of 'features.fea' of any existing
        # instances so that they can be restored later.
        features_store = collect_features_content(ds_doc.instances,
                                                  options.indexList)

    # Set the current dir to the design space dir, so that relative paths in
    # the design space file will work.
    dsDir, dsFile = os.path.split(os.path.abspath(options.dsPath))
    os.chdir(dsDir)
    options.dsPath = dsFile

    if options.indexList:
        dsPath = filterDesignspaceInstances(ds_doc, options)
    else:
        dsPath = dsFile

    newInstancesList = [inst.path for inst in ds_doc.instances]
    newInstancesCount = len(newInstancesList)

    if newInstancesCount == 1:
        logger.info("Building 1 instance...")
    else:
        logger.info("Building %s instances..." % newInstancesCount)
    ufoProcessorBuild(documentPath=dsPath,
                      outputUFOFormatVersion=options.ufo_version,
                      roundGeometry=(not options.no_round),
                      logger=logger)
    if (dsPath != options.dsPath) and os.path.exists(dsPath):
        os.remove(dsPath)

    logger.info("Built %s instances." % newInstancesCount)
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
            # make sure that there are no old glyphs left in the
            # processed glyphs folder
            validateLayers(instancePath)

    # The defcon library renames glyphs. Need to fix them again
    if options.doOverlapRemoval or options.doAutoHint:
        for instancePath in newInstancesList:
            if options.doNormalize:
                normalizeUFO(instancePath, outputPath=None, onlyModified=False,
                             writeModTimes=False)

    # Restore the contents of the instances' 'features.fea' files
    for fea_pth, fea_cntnts in features_store.items():
        with open(fea_pth, 'wb') as fp:
            fp.write(fea_cntnts)


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
    except Exception as exc:
        logger.error(exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())
