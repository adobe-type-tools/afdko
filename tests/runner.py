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
import subprocess32 as subprocess
import sys
import tempfile

__version__ = '0.5.4'

logger = logging.getLogger('runner')


TIMEOUT = 240  # seconds


def _write_file(file_path, data):
    with open(file_path, "wb") as f:
        f.write(data)


def _get_input_dir_path(tool_path):
    # Windows tool name contains '.exe'
    tool_name = os.path.splitext(os.path.basename(tool_path))[0]
    input_dir = os.path.join(
        os.path.split(__file__)[0], '{}_data'.format(tool_name), 'input')
    return os.path.abspath(os.path.realpath(input_dir))


def run_tool(opts):
    """
    Runs the tool using the parameters provided.
    """
    input_dir = _get_input_dir_path(opts.tool)

    args = [opts.tool]
    for opt in opts.options:
        if opt.startswith('='):
            args.append('--{}'.format(opt[1:]))
        elif opt.startswith('*'):
            args.append('+{}'.format(opt[1:]))
        elif opt.startswith('_'):
            args.append('{}'.format(opt[1:]))
        else:
            args.append('-{}'.format(opt))

    if opts.files:
        if opts.abs_paths:
            files = opts.files
        else:
            files = [os.path.join(input_dir, fname) for fname in opts.files]
        assert all([os.path.exists(fpath) for fpath in files]), (
            "Invalid input path found.")
        args.extend(files)

    if not opts.no_save_path:
        # NOTE: the handling of 'opts.save_path' is nested under
        # 'opts.no_save_path' to avoid creating unnecessary temporary files
        if opts.save_path:
            # '--save' option was used. Use the path provided
            save_loc = opts.save_path
        else:
            # '--save' option was NOT used. Create a temporary file
            file_descriptor, save_loc = tempfile.mkstemp()
            os.close(file_descriptor)
        args.append(save_loc)

    stderr = None
    if opts.std_error:
        stderr = subprocess.STDOUT

    logger.debug(
        "About to run the command below\n==>{}<==".format(' '.join(args)))
    try:
        if opts.no_save_path:
            return subprocess.check_call(args, timeout=TIMEOUT)
        else:
            output = subprocess.check_output(args, stderr=stderr,
                                             timeout=TIMEOUT)
            if opts.redirect:
                _write_file(save_loc, output)
            return save_loc
    except (subprocess.CalledProcessError, OSError) as err:
        logger.error(err)
        raise


def _check_tool(tool_name):
    """
    Checks if the invoked tool produces a known command.
    Returns the tool's name if the check is successful,
    or a tuple with the tool's name and the error if it's not.
    """
    if platform.system() == 'Windows':
        tool_name += '.exe'
    # XXX start hack to bypass this issue
    # https://github.com/adobe-type-tools/afdko/issues/348
    if tool_name.split('.')[0] in ('sfntdiff', 'sfntedit', 'makeotfexe'):
        return tool_name
    # XXX end hack
    try:
        subprocess.check_output([tool_name, '-h'], timeout=TIMEOUT)
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
        metavar='FILE_NAME/PATH',
        help='names or full paths of the files to provide to the tool\n'
             "The files will be sourced from the 'input' folder inside the "
             "'{tool_name}_data' directory, unless the option '--abs-paths' "
             "is used."
    )
    parser.add_argument(
        '-a',
        '--abs-paths',
        action='store_true',
        help="treat the input paths as absolute\n"
             "(instead of deriving them from the tool's name)"
    )
    parser.add_argument(
        '-r',
        '--redirect',
        action='store_true',
        help="redirect the tool's output to a file"
    )
    parser.add_argument(
        '-e',
        '--std-error',
        action='store_true',
        help="capture stderr instead of stdout"
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
    and an exception if it isn't.
    """
    opts = get_options(args)

    return run_tool(opts)


if __name__ == "__main__":
    sys.exit(main())
