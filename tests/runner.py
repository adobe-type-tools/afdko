# Copyright 2018 Adobe. All rights reserved.

"""
Helper script for running a command line tool.
Used as part of the integration tests.
"""

import argparse
import logging
import os
import subprocess
import sys
import tempfile

__version__ = '0.6.3'

logger = logging.getLogger('runner')


TIMEOUT = 240  # seconds


def _write_file(file_path, data):
    with open(file_path, "wb") as f:
        f.write(data)


def _get_input_dir_path(tool_name):
    input_dir = os.path.join(
        os.path.split(__file__)[0], f'{tool_name}_data', 'input')
    return os.path.abspath(os.path.realpath(input_dir))


def run_tool(opts):
    """
    Runs the tool using the parameters provided.
    """
    input_dir = _get_input_dir_path(opts.tool)

    args = [opts.tool]
    for opt in opts.options:
        if opt.startswith('='):
            args.append(f'--{opt[1:]}')
        elif opt.startswith('*'):
            args.append(f'+{opt[1:]}')
        elif opt.startswith('_'):
            args.append(f'{opt[1:]}')
        else:
            args.append(f'-{opt}')

    if opts.files:
        if opts.abs_paths:
            files = opts.files
            # validate only the first path; the other paths may not exist yet
            # as they can be the paths to which the user wants the tool to
            # write its output to
            assert os.path.exists(files[0]), "Invalid input path found."
        else:
            files = [os.path.join(input_dir, fname) for fname in opts.files]
            assert all([os.path.exists(fpath) for fpath in files]), (
                "Invalid input path found.")
        args.extend(files)

    stderr = None
    if opts.std_error:
        stderr = subprocess.STDOUT

    logger.debug(
        f"About to run the command below\n==>{' '.join(args)}<==")
    try:
        if opts.save_path:
            output = subprocess.check_output(args, stderr=stderr,
                                             timeout=TIMEOUT)
            _write_file(opts.save_path, output)
            return opts.save_path
        else:
            return subprocess.check_call(args, timeout=TIMEOUT)
    except (subprocess.CalledProcessError, OSError) as err:
        if opts.save_path:
            _write_file(opts.save_path, err.output)
            return opts.save_path
        logger.error(err)
        raise


def _check_tool(tool_name):
    """
    Checks if the invoked tool produces a known command.
    Returns the tool's name if the check is successful,
    or a tuple with the tool's name and the error if it's not.
    """
    try:
        subprocess.check_output([tool_name, '-h'], timeout=TIMEOUT)
        return tool_name
    except (subprocess.CalledProcessError, OSError) as err:
        logger.error(err)
        return tool_name, err


def _check_save_path(path_str):
    check_path = os.path.abspath(os.path.realpath(path_str))
    del_test_file = True
    try:
        if os.path.exists(check_path):
            del_test_file = False
        open(check_path, 'a').close()
        if del_test_file:
            os.remove(check_path)
    except (OSError):
        raise argparse.ArgumentTypeError(
            f"{check_path} is not a valid path to write to.")
    return check_path


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
             "The files will be sourced from the 'input' folder inside the\n"
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
        '-s',
        '--save',
        dest='save_path',
        nargs='?',
        default=False,
        type=_check_save_path,
        metavar='blank or PATH',
        help="path to save the tool's standard output (stdout) to\n"
             'The default is to use a temporary location.'
    )
    parser.add_argument(
        '-e',
        '--std-error',
        action='store_true',
        help="capture stderr instead of stdout"
    )
    options = parser.parse_args(args)

    if not options.verbose:
        level = "WARNING"
    elif options.verbose == 1:
        level = "INFO"
    else:
        level = "DEBUG"
    logging.basicConfig(level=level)

    if options.save_path is None:
        # runner.py was called with '-s' but the option is NOT followed by
        # a command-line argument; this means a temp file must be created.
        # When option '-s' is NOT used, the value of options.save_path is False
        file_descriptor, temp_path = tempfile.mkstemp()
        os.close(file_descriptor)
        options.save_path = temp_path

    if isinstance(options.tool, tuple):
        parser.error(f"'{options.tool[0]}' is an unknown command.")

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
