#!/usr/bin/env python

# Copyright 2018 Adobe. All rights reserved.

"""
Helper script for diff'ing files.
Used as part of the integration tests.
"""

from __future__ import print_function, division, absolute_import

import argparse
import difflib
import filecmp
import logging
import os
import sys

from fontTools.misc.py23 import open

__version__ = '0.2.0'

logger = logging.getLogger(__file__)


TXT_MODE = 'txt'
BIN_MODE = 'bin'
SPLIT_MARKER = '_+:+_'


class Differ(object):

    def __init__(self, opts):
        self.mode = opts.mode
        self.path1 = opts.path1
        self.path2 = opts.path2
        # tuple of strings containing the starts of lines to skip
        self.skip_strings = opts.skip_strings
        # tuple of integers for the line numbers to skip
        self.skip_lines = opts.skip_lines

    def diff_paths(self):
        """
        Diffs the contents of two paths using the parameters provided.
        Returns True if the contents match, and False if they don't.
        """
        if self.mode == TXT_MODE:
            if os.path.isfile(self.path1):
                return self._diff_txt_files(self.path1, self.path2)
            elif os.path.isdir(self.path1):
                return self._diff_dirs()
        elif self.mode == BIN_MODE:
            if os.path.isfile(self.path1):
                return filecmp.cmp(self.path1, self.path2)
            elif os.path.isdir(self.path1):
                return self._diff_dirs()

    def _diff_txt_files(self, path1, path2):
        """
        Diffs two text-based files using difflib.
        Returns True if the contents match, and False if they don't.

        NOTE: This method CANNOT use self.path1 and self.path2 because it
        gets called from _diff_dirs().
        """
        expected = self._read_txt_file(path1)
        actual = self._read_txt_file(path2)
        if actual != expected:
            for line in difflib.unified_diff(expected, actual,
                                             fromfile=path1,
                                             tofile=path2, n=1):
                sys.stdout.write(line)
            return False
        return True

    def _read_txt_file(self, path):
        """
        Reads a text file and returns a list of its lines.
        """
        # Hard code a first line; this way the difflib results start
        # from 1 instead of zero, thus matching the file's line numbers
        lines = ['']
        with open(path, "r", encoding="utf-8") as f:
            for i, line in enumerate(f.readlines(), 1):
                # Skip lines that change, such as timestamps
                if self._line_to_skip(line):
                    logger.debug("Matched begin of line. Skipped: {}"
                                 "".format(line.rstrip()))
                    # Blank the line instead of actually skipping (via
                    # 'continue'); this way the difflib results show the
                    # correct line numbers
                    line = ''
                # Skip specific lines, referenced by number
                elif i in self.skip_lines:
                    logger.debug("Matched line #{}. Skipped: {}"
                                 "".format(i, line.rstrip()))
                    # Blank the line instead of actually skipping (via
                    # 'continue'); this way the difflib results show the
                    # correct line numbers
                    line = ''
                # Use os-native line separator to enable running difflib
                lines.append(line.rstrip() + os.linesep)
        return lines

    def _line_to_skip(self, line):
        """
        Loops over the skip items.
        Returns True if the beginning of the line matches a skip item.
        """
        for item in self.skip_strings:
            if line.startswith(item):
                return True
        return False

    def _diff_dirs(self):
        """
        Diffs two folders containing files.
        Returns True if all files match. Returns False if the folders' contents
        don't match, or as soon as one non-matching file is found.
        """
        all_rel_file_paths = self._compare_dir_contents()
        if all_rel_file_paths is None:
            return False
        for rel_file_path in all_rel_file_paths:
            path1 = self.path1 + rel_file_path
            assert os.path.exists(path1), "Not a valid path1: {}".format(path1)
            path2 = self.path2 + rel_file_path
            assert os.path.exists(path2), "Not a valid path2: {}".format(path2)
            if self.mode == BIN_MODE:
                diff_result = filecmp.cmp(path1, path2)
            else:
                diff_result = self._diff_txt_files(path1, path2)
            if not diff_result:
                logger.debug("Non-matching file: {}".format(rel_file_path))
                return False
        return True

    def _compare_dir_contents(self):
        """
        Checks if two directory trees have the same files and folders.
        Returns a list of relative paths to all files if the dirs' contents
        match, and None if they don't.
        """
        all_paths1 = self._get_all_file_paths_in_dir_tree(self.path1)
        all_paths2 = self._get_all_file_paths_in_dir_tree(self.path2)
        if all_paths1 != all_paths2:
            logger.debug("Folders' contents don't match.")
            return None
        return all_paths1

    @staticmethod
    def _get_all_file_paths_in_dir_tree(start_path):
        """
        Returns a list of relative paths of all files in a directory tree.
        The list's items are ordered top-down according to the tree.
        """
        all_paths = []
        for dir_name, _, file_names in os.walk(start_path):
            all_paths.extend(
                [os.path.join(dir_name, f_name) for f_name in file_names])

        # Make the paths relative
        all_paths = [path.replace(start_path, '') for path in all_paths]

        logger.debug("All paths: {}".format(all_paths))

        return all_paths


def _get_path_kind(pth):
    """
    Returns a string describing the kind of path.
    String values are file, folder, invalid, and unknown.
    """
    if not os.path.exists(pth):
        return 'invalid'
    elif os.path.isfile(pth):
        return 'file'
    elif os.path.isdir(pth):
        return 'folder'
    return 'unknown'


def _paths_are_same_kind(path1, path2):
    """
    Checks that both paths are either files or folders.
    Returns boolean.
    """
    if all([os.path.isfile(path) for path in (path1, path2)]):
        return True
    elif all([os.path.isdir(path) for path in (path1, path2)]):
        return True
    return False


def _validate_path(path_str):
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            "{} is not a valid path.".format(path_str))
    return valid_path


def _split_string_sequence(str_seq):
    return tuple(str_seq.split(SPLIT_MARKER))


def _split_num_range_or_delta(num_str):
    num_range = num_str.split('-') + ['-']
    num_delta = num_str.split('+') + ['+']
    if len(num_range) == 3:
        return num_range
    elif len(num_delta) == 3:
        return num_delta
    return num_str


def _convert_to_int(num_str):
    try:
        return int(num_str)
    except ValueError:
        raise argparse.ArgumentTypeError("Not a number: {}".format(num_str))


def _expand_num_range_or_delta(num_str_lst):
    start_num = _convert_to_int(num_str_lst[0])
    rng_dlt_num = _convert_to_int(num_str_lst[1])
    plus_minus = num_str_lst[2]
    if plus_minus == '+':
        return list(range(start_num, start_num + rng_dlt_num + 1))
    elif plus_minus == '-':
        if not (rng_dlt_num >= start_num):
            raise argparse.ArgumentTypeError(
                "The start of range value is larger than the end of range "
                "value: {}-{}".format(start_num, rng_dlt_num))
        return list(range(start_num, rng_dlt_num + 1))
    else:
        raise argparse.ArgumentTypeError(
            "Unrecognized number separator: {}".format(plus_minus))


def _convert_seq_to_ints(num_seq):
    seq = []
    for item in num_seq:
        if isinstance(item, list):
            seq.extend(_expand_num_range_or_delta(item))
        else:
            seq.append(_convert_to_int(item))
    return seq


def _split_linenumber_sequence(str_seq):
    num_seq = [_split_num_range_or_delta(item) for item in str_seq.split(',')]
    return tuple(_convert_seq_to_ints(num_seq))


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
        '-m',
        '--mode',
        default=TXT_MODE,
        choices=(TXT_MODE, BIN_MODE),
        help='diff mode (default: %(default)s)'
    )
    parser.add_argument(
        '-s',
        '--string',
        dest='skip_strings',
        type=_split_string_sequence,
        default=(),
        help='string for matching the beginning of a line to skip\n'
             'For multiple strings, separate them with {}'.format(SPLIT_MARKER)
    )
    parser.add_argument(
        '-l',
        '--line',
        dest='skip_lines',
        type=_split_linenumber_sequence,
        default=(),
        help='number of a line to skip\n'
             'For multiple line numbers, separate them with a comma (,).\n'
             'For ranges of lines, use a minus (-) between two numbers.\n'
             'For a line delta, use a plus (+) between two numbers.'
    )
    parser.add_argument(
        'path1',
        metavar='PATH1',
        type=_validate_path,
        help='1st path for comparison'
    )
    parser.add_argument(
        'path2',
        metavar='PATH2',
        type=_validate_path,
        help='2nd path for comparison'
    )
    options = parser.parse_args(args)

    if not options.verbose:
        level = "WARNING"
    elif options.verbose == 1:
        level = "INFO"
    else:
        level = "DEBUG"
    logging.basicConfig(level=level)

    if not _paths_are_same_kind(options.path1, options.path2):
        parser.error("The paths are not of the same kind. Path1's kind is {}. "
                     "Path2's kind is {}.".format(
                         _get_path_kind(options.path1),
                         _get_path_kind(options.path2)))

    logger.debug("Line numbers: {}".format(options.skip_lines))

    return options


def main(args=None):
    """
    Returns True if the inputs match, and False if they don't.
    """
    opts = get_options(args)

    differ = Differ(opts)
    return differ.diff_paths()


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
