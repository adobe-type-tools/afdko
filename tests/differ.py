# Copyright 2018 Adobe. All rights reserved.

"""
Helper script for diff'ing files.
Used as part of the integration tests.
"""

import argparse
import difflib
import filecmp
import logging
import os
import re
import sys

__version__ = '0.3.4'

logger = logging.getLogger('differ')


TXT_MODE = 'txt'
BIN_MODE = 'bin'
SPLIT_MARKER = '_+:+_'
DFLT_ENC = 'utf-8'


class Differ(object):

    def __init__(self, opts):
        self.mode = opts.mode
        self.path1 = opts.path1
        self.path2 = opts.path2
        # tuple of strings containing the starts of lines to skip
        self.skip_strings = opts.skip_strings
        # tuple of integers for the line numbers to skip
        self.skip_lines = opts.skip_lines
        # regex pattern matching the beginning of line
        self.skip_regex = opts.skip_regex
        self.encoding = opts.encoding

    def diff_paths(self):
        """
        Diffs the contents of two paths using the parameters provided.
        Returns True if the contents match, and False if they don't.
        """
        if os.path.isdir(self.path1):
            return self._diff_dirs()
        else:  # paths are files
            if self.mode == BIN_MODE:
                return filecmp.cmp(self.path1, self.path2)
            else:  # TXT_MODE
                return self._diff_txt_files(self.path1, self.path2)

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
        try:
            with open(path, "r", encoding=self.encoding) as f:
                for i, line in enumerate(f.readlines(), 1):
                    # Skip lines that change, such as timestamps
                    if self._line_to_skip(line):
                        logger.debug(f"Matched begin of line. "
                                     f"Skipped: {line.rstrip()}")
                        # Blank the line instead of actually skipping (via
                        # 'continue'); this way the difflib results show the
                        # correct line numbers
                        line = ''
                    # Skip specific lines, referenced by number
                    elif i in self.skip_lines:
                        logger.debug(f"Matched line #{i}. "
                                     f"Skipped: {line.rstrip()}")
                        line = ''
                    # Skip lines that match regex
                    elif self.skip_regex and self.skip_regex.match(line):
                        logger.debug(f"Matched regex begin of line. "
                                     f"Skipped: {line.rstrip()}")
                        line = ''
                    # Use os-native line separator to enable running difflib
                    lines.append(line.rstrip() + os.linesep)
        except UnicodeDecodeError:
            logger.error(f"Couldn't read text file using '{self.encoding}' "
                         f"encoding.\n      File path: {path}")
            sys.exit(1)
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
            assert os.path.exists(path1), f"Not a valid path1: {path1}"
            path2 = self.path2 + rel_file_path
            assert os.path.exists(path2), f"Not a valid path2: {path2}"
            if self.mode == BIN_MODE:
                diff_result = filecmp.cmp(path1, path2)
            else:  # TXT_MODE
                diff_result = self._diff_txt_files(path1, path2)
            if not diff_result:
                logger.debug(f"Non-matching file: {rel_file_path}")
                return False
        return True

    def _report_dir_diffs(self, all_paths1, all_paths2):
        """
        Returns a string listing the paths that exist in folder 1 but not in 2,
        and vice-versa.
        """
        diffs_str = ''
        set_1st = set(all_paths1)
        set_2nd = set(all_paths2)
        diff1 = sorted(set_1st - set_2nd)
        diff2 = sorted(set_2nd - set_1st)
        if diff1:
            dir1 = os.path.basename(self.path1)
            dj1 = '\n    '.join(diff1)
            diffs_str += (f"\n  In 1st folder ({dir1}) but not in 2nd:"
                          f"\n    {dj1}")
        if diff2:
            dir2 = os.path.basename(self.path2)
            dj2 = '\n    '.join(diff2)
            diffs_str += (f"\n  In 2nd folder ({dir2}) but not in 1st:"
                          f"\n    {dj2}")
        return diffs_str

    def _compare_dir_contents(self):
        """
        Checks if two directory trees have the same files and folders.
        Returns a list of relative paths to all files if the dirs' contents
        match, and None if they don't.
        """
        all_paths1 = self._get_all_file_paths_in_dir_tree(self.path1)
        all_paths2 = self._get_all_file_paths_in_dir_tree(self.path2)
        if all_paths1 != all_paths2:
            dd = self._report_dir_diffs(all_paths1, all_paths2)
            logger.info(f"Folders' contents don't match.{dd}")
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

        # Make the paths relative, and enforce order.
        all_paths = sorted(
            [path.replace(start_path, '') for path in all_paths])

        logger.debug(f"All paths: {all_paths}")

        return all_paths


def _get_path_kind(pth):
    """
    Returns a string describing the kind of path.
    Possible values are 'file', 'folder', and 'invalid'.
    """
    try:
        if os.path.isfile(pth):
            return 'file'
        elif os.path.isdir(pth):
            return 'folder'
        else:
            return 'invalid'
    except TypeError:
        return 'invalid'


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
            f"'{path_str}' is not a valid path.")
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
    elif num_str.isnumeric():
        return num_str
    else:
        raise argparse.ArgumentTypeError(
            f"Invalid number range or delta: {num_str}")


def _convert_to_int(num_str):
    try:
        return int(num_str)
    except ValueError:
        raise argparse.ArgumentTypeError(f"Not a number: {num_str}")


def _expand_num_range_or_delta(num_str_lst):
    start_num = _convert_to_int(num_str_lst[0])
    rng_dlt_num = _convert_to_int(num_str_lst[1])
    sign = num_str_lst[2]
    if sign == '+':
        return list(range(start_num, start_num + rng_dlt_num + 1))
    else:  # sign == '-'
        if not (rng_dlt_num >= start_num):
            raise argparse.ArgumentTypeError(
                f"The start of range value is larger than the end of range "
                f"value: {start_num}-{rng_dlt_num}")
        return list(range(start_num, rng_dlt_num + 1))


def _convert_seq_to_ints(num_seq):
    seq = []
    for item in num_seq:
        if isinstance(item, list):
            seq.extend(_expand_num_range_or_delta(item))
        else:
            seq.append(_convert_to_int(item))
    return sorted(set(seq))


def _split_linenumber_sequence(str_seq):
    num_seq = [_split_num_range_or_delta(item) for item in str_seq.split(',')]
    return tuple(_convert_seq_to_ints(num_seq))


def _compile_regex(str_seq):
    if not str_seq.startswith('^'):
        raise argparse.ArgumentTypeError(
            "The expression must start with the caret '^' character")
    try:
        return re.compile(str_seq)
    except re.error as err:
        raise argparse.ArgumentTypeError(
            f'The expression is invalid: {err}')


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
        help=f'string for matching the beginning of a line to skip\n'
             f'For multiple strings, separate them with {SPLIT_MARKER}'
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
        '-r',
        '--regex',
        dest='skip_regex',
        type=_compile_regex,
        help='regular expression matching the beginning of a line to skip\n'
             "The expression must start with the caret '^' character, "
             'and characters such as backslash, semicolon, and space need '
             'to be escaped.'
    )
    parser.add_argument(
        '-e',
        '--encoding',
        default=DFLT_ENC,
        choices=(DFLT_ENC, 'macroman'),
        help='encoding to use when opening text files (default: %(default)s)'
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
        kp1 = _get_path_kind(options.path1)
        kp2 = _get_path_kind(options.path2)
        parser.error(f"The paths are not of the same kind. "
                     f"Path1's kind is {kp1}. "
                     f"Path2's kind is {kp2}.")

    logger.debug(f"Line numbers: {options.skip_lines}")
    regexpat = getattr(options.skip_regex, 'pattern', None)
    logger.debug(f"Regular expression: {regexpat}")

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
