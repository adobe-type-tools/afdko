# Copyright 2017 Adobe. All rights reserved.
"""
Builds master source OpenType/CFF fonts from a designspace file
and UFO master source fonts.
"""

# The script makes the following assumptions:
# 1. The AFDKO is installed; the script calls 'tx', 'sfntedit' and 'makeotf').
# 2. Each master source font is in a separate directory from the other master
#    fonts, and each of these directories provides the default metadata files
#    expected by the 'makeotf' command:
#    - features/features.fea
#    - font.ufo
#    - FontMenuNameDB within the directory or no more than 4 parent
#      directories up.
#    - GlyphAliasAndOrderDB within the directory or no more than 4 parent
#      directories up.
# The script will build a master source OpenType/CFF font for each master
# source UFO font, using the same file name, but with the extension '.otf'.

import argparse
import logging
import os
import sys

from fontTools.designspaceLib import (
    DesignSpaceDocument,
    DesignSpaceDocumentError,
)

from afdko.fdkutils import run_shell_command, get_temp_file_path
from afdko.makeotf import main as makeotf

__version__ = '1.9.2'

logger = logging.getLogger(__name__)


class ShellCommandError(Exception):
    pass


def generalizeCFF(otfPath):
    tempFilePath = get_temp_file_path()

    if not run_shell_command(['tx', '-cff', '+b', '-std', '-no_opt',
                              otfPath, tempFilePath]):
        raise ShellCommandError

    if not run_shell_command(['sfntedit', '-a',
                              f'CFF ={tempFilePath}', otfPath]):
        raise ShellCommandError


def _validate_path(path_str):
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            f"'{path_str}' is not a valid path.")
    return valid_path


def _split_makeotf_options(comma_str):
    """Converts a comma-separated string into a list.
       Adds back leading hyphen."""
    if not comma_str.startswith('-'):
        comma_str = '-' + comma_str
    return comma_str.split(',')


MKOT_OPT = '--mkot'


def _preparse_args(args):
    """Removes the leading hyphen from the '--mkot' option argument"""
    if args is None:
        args = sys.argv[1:]
    if MKOT_OPT not in args:
        return args
    opt_i = args.index(MKOT_OPT)
    try:
        arg_i = opt_i + 1
        mkot_arg = args[arg_i]
        if mkot_arg.startswith('-'):
            args[arg_i] = mkot_arg[1:]
    except IndexError:
        pass
    return args


def build_masters(opts):
    """
    Build master OTFs using supplied options.
    """
    logger.info("Reading designspace file...")
    ds = DesignSpaceDocument.fromfile(opts.dsPath)
    validateDesignspaceDoc(ds)
    master_paths = [s.path for s in ds.sources]

    logger.info("Building local OTFs for master font paths...")
    curDir = os.getcwd()
    dsDir = os.path.dirname(opts.dsPath)

    for master_path in master_paths:
        master_path = os.path.join(dsDir, master_path)
        masterDir = os.path.dirname(master_path)
        ufoName = os.path.basename(master_path)
        otfName = os.path.splitext(ufoName)[0]
        otfName = f"{otfName}.otf"

        if masterDir:
            os.chdir(masterDir)

        makeotf(['-nshw', '-f', ufoName, '-o', otfName,
                 '-r', '-nS'] + opts.mkot)
        logger.info(f"Built OTF font for {master_path}")
        generalizeCFF(otfName)
        os.chdir(curDir)


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
        help='path to design space file',
        required=True
    )
    parser.add_argument(
        MKOT_OPT,
        metavar='OPTIONS',
        help="comma-separated set of 'makeotf' options",
        type=_split_makeotf_options,
        default=[]
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


def validateDesignspaceDoc(dsDoc, **kwArgs):
    """
    Validate the dsDoc DesignSpaceDocument object. Raises Exceptions if
    certain criteria are not met. These are above and beyond the basic
    validations in fontTools.designspaceLib and are specific to
    buildmasterotfs.
    """
    if dsDoc.sources:
        for src in dsDoc.sources:
            if not os.path.exists(src.path):
                raise DesignSpaceDocumentError(
                    f"Source file {src.path} does not exist")
    else:
        raise DesignSpaceDocumentError("Designspace file contains no sources.")


def main(args=None):
    args = _preparse_args(args)
    opts = get_options(args)

    try:
        build_masters(opts)
    except Exception as exc:
        logger.error(exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())
