# Copyright 2017 Adobe. All rights reserved.

from __future__ import print_function, division, absolute_import

import argparse
import logging
import os
from pkg_resources import parse_version
import sys

from fontTools import varLib, version as fontToolsVersion
from fontTools.ttLib import TTFont

__version__ = '1.15.0'


# set up for printing progress notes
def progress(self, message, *args, **kws):
    # Note: message must contain the format specifiers for any strings in args.
    level = self.getEffectiveLevel()
    self._log(level, message, args, **kws)


PROGRESS_LEVEL = logging.INFO+5
PROGESS_NAME = "progress"
logging.addLevelName(PROGRESS_LEVEL, PROGESS_NAME)
logger = logging.getLogger(__name__)
logging.Logger.progress = progress


def _validate_path(path_str):
    # used for paths passed to get_options.
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            "'{}' is not a valid path.".format(path_str))
    return valid_path


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
        dest='design_space_path',
        type=_validate_path,
        help='path to design space file\n',
        required=True
    )
    parser.add_argument(
        '-o',
        '--out',
        metavar='PATH',
        dest='var_font_path',
        help='path to output variable font file. Default is base name\n'
        'of the design space file.\n',
        default=None,
    )
    parser.add_argument(
        '-k',
        '--keep_glyph_names',
        dest='keep_glyph_names',
        action='store_true',
        help='Preserve glyph names in output var font, with a post table\n'
        'format 2.\n',
        default=False,
    )
    options = parser.parse_args(args)
    if not options.var_font_path:
        var_font_path = os.path.splitext(options.design_space_path)[0] + '.otf'
        options.var_font_path = var_font_path

    if not options.verbose:
        level = PROGRESS_LEVEL
        logging.basicConfig(level=level, format="%(message)s")
    else:
        if options.verbose:
            level = logging.INFO
        logging.basicConfig(level=level)
    logger.setLevel(level)

    return options


def otfFinder(s):
    return s.replace('.ufo', '.otf')


def add_glyph_names(tt_font, glyph_order):
    postTable = tt_font['post']
    postTable.glyphOrder = tt_font.glyphOrder = glyph_order
    postTable.formatType = 2.0
    postTable.extraNames = []
    postTable.mapping = {}
    postTable.compile(tt_font)


def run(args=None):

    if args is None:
        args = sys.argv[1:]

    options = get_options(args)

    if parse_version(fontToolsVersion) < parse_version("3.19"):
        logger.error("Quitting. The Python fonttools module "
                     "must be at least version 3.41.0")
        return

    if os.path.exists(options.var_font_path):
        os.remove(options.var_font_path)

    logger.progress("Building VF font...")
    varFont, varModel, masterPaths = varLib.build(
        options.design_space_path, otfFinder)

    if options.keep_glyph_names:
        default_font = TTFont(masterPaths[0])  # Any source font will do.
        add_glyph_names(varFont, default_font.glyphOrder)

    varFont.save(options.var_font_path)
    logger.progress("Built variable font '%s'" % (options.var_font_path))


if __name__ == '__main__':
    run()
