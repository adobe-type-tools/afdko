#!/usr/bin/env python

# Copyright 2018 Adobe. All rights reserved.

"""
Helper script for running a command line tool.
Used as part of the integration tests.
"""

from __future__ import print_function, division, absolute_import

import argparse
import logging
import os
import platform
import subprocess
import sys
import tempfile

__version__ = '0.1.2'

logger = logging.getLogger(__file__)


def _write_file(file_path, data):
    with open(file_path, "wb") as f:
        f.write(data)


def _get_input_dir_path(tool_name):
    tool_name = tool_name.split('.')[0]  # Windows tool name contains '.exe'
    input_dir = os.path.join(os.path.split(__file__)[0], tool_name + '_data',
                             'input')
    return os.path.abspath(os.path.realpath(input_dir))


def _get_save_location(save_path):
    """
    If save_path is not provided ('--save' option wasn't used),
    create a temporary location.
    """
    if not save_path:
        save_path = tempfile.NamedTemporaryFile(delete=False).name
    return save_path


def run_tool(opts):
    """
    Runs the tool using the parameters provided.
    """
    input_dir = _get_input_dir_path(opts.tool)
    save_loc = _get_save_location(opts.save_path)

    args = []
    for opt in opts.options:
        if opt.startswith('='):
            args.append('--{}'.format(opt[1:]))
        elif opt.startswith('*'):
            args.append('+{}'.format(opt[1:]))
        elif opt.startswith('_'):
            args.append('{}'.format(opt[1:]))
        else:
            args.append('-{}'.format(opt))

    args.insert(0, opts.tool)

    if opts.files:
        files = [os.path.join(input_dir, fname) for fname in opts.files]
        assert all([os.path.exists(fpath) for fpath in files]), (
            "Invalid input path found.")
        args.extend(files)

    if not opts.no_save_path:
        args.append(save_loc)

    logger.debug(
        "About to run the command below\n==>{}<==".format(' '.join(args)))
    try:
        if opts.no_save_path:
            return subprocess.check_call(args)
        else:
            output = subprocess.check_output(args)
            if opts.redirect:
                _write_file(save_loc, output)
            return save_loc
    except (subprocess.CalledProcessError, OSError) as err:
        logger.error(err)
        return None


def _check_tool(tool_name):
    """
    Checks if the invoked tool produces a known command.
    Returns the tool's name if the check is successful,
    or a tuple with the tool's name and the error if it's not.
    """
    if platform.system() == 'Windows':
        tool_name += '.exe'
    # XXX start hack to bypass these issues
    # https://github.com/adobe-type-tools/afdko/issues/347
    # https://github.com/adobe-type-tools/afdko/issues/348
    if tool_name.split('.')[0] in ('sfntdiff', 'sfntedit', 'makeotfexe',
                                   'type1', 'detype1'):
        return tool_name
    # XXX end hack
    try:
        subprocess.check_output([tool_name, '-h'])
        return tool_name
    except (subprocess.CalledProcessError, OSError) as err:
        logger.error(err)
        return tool_name, err


def _check_save_path(path_str):
    test_path = os.path.abspath(os.path.realpath(path_str))
    del_test_file = True
    try:
        if os.path.exists(test_path):
            del_test_file = False
        open(test_path, 'a').close()
        if del_test_file:
            os.remove(test_path)
    except (IOError, OSError):
        raise argparse.ArgumentTypeError(
            "{} is not a valid path to write to.".format(test_path))
    return test_path


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
        '-t',
        '--tool',
        required=True,
        type=_check_tool,
        metavar='NAME',
        help='name of the tool to run'
    )
    parser.add_argument(
        '-o',
        '--options',
        nargs='+',
        metavar='OPTION',
        default=[],
        help='options to run the tool with\n'
             'By default the option will be prepended with one minus (-).\n'
             'To request the use of two minus (--), prepend the option with '
             'an equal (=).\n'
             'To request the use of a plus (+), prepend the option with an '
             'asterisk (*).\n'
             'To request the use of nothing, prepend the option with an '
             'underscore (_).'
    )
    parser.add_argument(
        '-f',
        '--files',
        nargs='+',
        metavar='FILE_NAME',
        help='names of the files to provide to the tool\n'
             "The files will be sourced from the 'input' folder inside the "
             "'{tool_name}_data' directory."
    )
    parser.add_argument(
        '-r',
        '--redirect',
        action='store_true',
        help="redirect the tool's output to a file"
    )
    save_parser = parser.add_mutually_exclusive_group()
    save_parser.add_argument(
        '-s',
        '--save',
        dest='save_path',
        type=_check_save_path,
        metavar='PATH',
        help="path to save the tool's output to\n"
             'The default is to use a temporary location.'
    )
    save_parser.add_argument(
        '-n',
        '--no-save',
        dest='no_save_path',
        action='store_true',
        help="don't set a path to save the tool's output to"
    )
    options = parser.parse_args(args)

    if not options.verbose:
        level = "WARNING"
    elif options.verbose == 1:
        level = "INFO"
    else:
        level = "DEBUG"
    logging.basicConfig(level=level)

    if isinstance(options.tool, tuple):
        parser.error("'{}' is an unknown command.".format(options.tool[0]))

    return options


def main(args=None):
    """
    Returns the path to the result/output file if the command is successful,
    and None if it isn't.
    """
    opts = get_options(args)

    return run_tool(opts)


if __name__ == "__main__":
    sys.exit(1 if main() in [None, 1] else 0)
