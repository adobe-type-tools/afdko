# Copyright 2015 Adobe. All rights reserved.

"""
Generates UFO font instances from a set of master UFO fonts.
It uses the mutatorMath library. The paths to the masters and
instances fonts are specified in the .designspace file.
"""

import argparse
import logging
import os
import shutil
import sys

from fontTools.designspaceLib import (
    DesignSpaceDocument,
    DesignSpaceDocumentError,
)

from defcon import Font
from psautohint.__main__ import main as psautohint
from ufonormalizer import normalizeUFO
from ufoProcessor import build as ufoProcessorBuild

from afdko.checkoutlinesufo import run as checkoutlinesUFO
from afdko.fdkutils import (
    validate_path,
)
from afdko.ufotools import validateLayers


__version__ = '2.4.4'

logger = logging.getLogger(__name__)


DFLT_DESIGNSPACE_FILENAME = "font.designspace"
TEMP_DESIGNSPACE_FILENAME = ".designspace"  # the '.' prefix is intentional
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
            shutil.rmtree(instance.path, ignore_errors=True)

        filteredInstances.append(instance)

    dsDoc.instances = filteredInstances
    # Cannot use a proper tempfile here because on Windows the temporary file
    # may end up being created in a different drive than the original file,
    # which will cause a ValueError when fontTools.designspaceLib tries to
    # compute the relative paths.
    # See https://github.com/fonttools/fonttools/issues/1735
    tmpPath = os.path.join(
        os.path.dirname(origDSPath), TEMP_DESIGNSPACE_FILENAME)
    dsDoc.write(tmpPath)

    return tmpPath


def updateInstance(fontInstancePath, options):
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
        except (Exception, SystemExit):
            raise

    if options.doAutoHint:
        logger.info("Running psautohint on %s ..." % fontInstancePath)
        ah_args = ['--no-zones-stems', fontInstancePath]
        if options.no_round:
            ah_args.insert(0, '-d')
        try:
            psautohint(ah_args)
        except (Exception, SystemExit):
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

            if prop_name == 'postscriptBlueScale':
                round_val = round(prop_val, 6)

            elif prop_name in ('postscriptBlueFuzz', 'postscriptBlueShift',
                               'postscriptSlantAngle'):
                round_val = round(prop_val, 2)
            else:
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
                    prop_val[i] = round(val, 2)


def roundPostscriptBlueScale(fontInfo):
    psbs_str = 'postscriptBlueScale'
    psbs_val = getattr(fontInfo, psbs_str, None)
    if psbs_val:
        setattr(fontInfo, psbs_str, round(psbs_val, 6))


def roundGlyphWidths(dFont):
    for dGlyph in dFont:
        dGlyph.width = int(round(dGlyph.width))


def roundKerningValues(dFont):
    if dFont.kerning:
        keys = dFont.kerning.keys()
        for key in keys:
            dFont.kerning[key] = int(round(dFont.kerning[key]))


def postProcessInstance(fontPath, options):
    dFont = Font(fontPath)
    clearCustomLibs(dFont)

    if options.no_round:
        # '-r/--no-round' option was used but certain values (glyph widths,
        # kerning values, font info values) still need to be rounded because
        # of how they are stored in the final OTF
        roundSelectedFontInfo(dFont.info)
        roundGlyphWidths(dFont)
        roundKerningValues(dFont)
    else:
        # ufoProcessor does not round kerning values nor postscriptBlueScale
        # when 'roundGeometry' = False
        roundPostscriptBlueScale(dFont.info)
        roundKerningValues(dFont)

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
                    f"Source file {src.path} does not exist")
    else:
        raise DesignSpaceDocumentError(
            "Designspace file contains no sources."
        )

    if dsDoc.instances:
        if dsoptions.indexList:
            # bounds check on indexList
            maxinstidx = max(dsoptions.indexList)
            if maxinstidx >= len(dsDoc.instances):
                raise IndexError(f"Instance index {maxinstidx} out-of-range")
    else:
        raise DesignSpaceDocumentError(
            "Designspace file contains no instances."
        )

    if dsoptions.useVarlib:
        axis_map = dict()
        for axis in dsDoc.axes:
            ad = axis.asdict()
            axis_map[ad['name']] = axis

    for i, inst in enumerate(dsDoc.instances):
        if dsoptions.indexList and i not in dsoptions.indexList:
            continue
        for attr_name in ('familyName', 'postScriptFontName', 'styleName'):
            if getattr(inst, attr_name, None) is None:
                logger.warning(
                    f"Instance at index {i} has no "
                    f"'{attr_name.lower()}' attribute.")
        if inst.path is None:
            raise DesignSpaceDocumentError(
                f"Instance at index {i} has no 'filename' attribute.")

        if dsoptions.useVarlib:
            # check for extrapolation, which is not supported by varLib
            # for these only warn, do not raise exception
            for dim, val in inst.location.items():
                axis = axis_map[dim]
                mval = axis.map_backward(val)
                if mval < axis.minimum or mval > axis.maximum:
                    logger.warning("Extrapolation is not supported with varlib"
                                   f" ({inst.familyName} {inst.styleName} "
                                   f"{dim}: {val})")


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
        ufo_pth = os.path.abspath(os.path.realpath(ufo_pth))
        fea_pth = os.path.join(ufo_pth, FEATURES_FILENAME)
        if os.path.isfile(fea_pth):
            with open(fea_pth, 'r') as fp:
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

    icnt_str = f'instance{"" if newInstancesCount == 1 else "s"}'
    tool_str = "fontTools.varlib" if options.useVarlib else "MutatorMath"
    info_str = f"Building {newInstancesCount} {icnt_str} with {tool_str}..."
    logger.info(info_str)
    ufoProcessorBuild(documentPath=dsPath,
                      outputUFOFormatVersion=options.ufo_version,
                      roundGeometry=(not options.no_round),
                      logger=logger, useVarlib=options.useVarlib)

    # Remove temporary designspace file
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
            updateInstance(instancePath, options)

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
        with open(fea_pth, 'w') as fp:
            fp.write(fea_cntnts)


def _split_comma_sequence(comma_str):
    index_lst = [item.strip() for item in comma_str.split(',')]
    int_set = set()
    for item in index_lst:
        try:
            index = int(item)
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"Invalid integer value: '{item}'")
        if index < 0:
            raise argparse.ArgumentTypeError(
                f"Index values must be greater than zero: '{index}'")
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
        type=validate_path,
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
        '--use-varlib',
        dest='useVarlib',
        action='store_true',
        help='Use varLib instead of MutatorMath'
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
