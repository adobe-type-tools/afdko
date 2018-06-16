from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from runner import main as runner
from differ import main as differ
from differ import SPLIT_MARKER

TOOL = 'tx'
CMD = ['-t', TOOL]

CFF2_SIMPLE_VF_NAME = 'test_simple_cff2_vf.otf'
data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_temp_dir_path():
    return tempfile.mkdtemp()


def _get_extension(in_format):
    if 'ufo' in in_format:
        return '.ufo'
    elif in_format == 'type1':
        return '.pfa'
    return '.' + in_format


PDF_SKIP = [
    '/Creator' + SPLIT_MARKER +
    '/Producer' + SPLIT_MARKER +
    '/CreationDate' + SPLIT_MARKER +
    '/ModDate' + SPLIT_MARKER +
    '<< /Length' + SPLIT_MARKER +
    '(Filename:' + SPLIT_MARKER +
    '(Date:' + SPLIT_MARKER +
    '(Time:',
    '-l', '36,38,240-254,262'
]

PS_SKIP = [
    '0 740 moveto (Filename:' + SPLIT_MARKER +
    '560 (Date:' + SPLIT_MARKER +
    '560 (Time:'
]


# -------------
# Convert tests
# -------------

@pytest.mark.parametrize('to_format', [
    'ufo2',
    'type1',
    'svg',
    'mtx',
    'afm',
    'pdf',
    'ps',
    'cff',
])
@pytest.mark.parametrize('from_format', [
    'ufo2',
    'type1'
])
def test_convert(from_format, to_format):
    from_ext = _get_extension(from_format)
    to_ext = _get_extension(to_format)

    # input filename
    from_filename = from_format + from_ext

    # expected filename
    exp_filename = from_format + to_ext

    # runner args
    if 'ufo' in to_format:
        save_path = os.path.join(_get_temp_dir_path(), 'font.ufo')
        runn_args = ['-s', save_path]
    else:
        runn_args = []

    # diff mode
    if to_format == 'cff':
        diff_mode = ['-m', 'bin']
    else:
        diff_mode = []

    # skip items
    if to_format == 'afm':
        skip = ['Comment Creation Date:']
    elif to_format == 'pdf':
        skip = PDF_SKIP[:]
    elif to_format == 'ps':
        skip = PS_SKIP[:]
    else:
        skip = []
    if skip:
        skip.insert(0, '-s')

    # format arg fix
    if to_format == 'ufo2':
        format_arg = 'ufo'
    elif to_format == 'type1':
        format_arg = 't1'
    else:
        format_arg = to_format

    actual_path = runner(
        CMD + ['-f', from_filename, '-o', format_arg] + runn_args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip + diff_mode)


# ----------
# Dump tests
# ----------

@pytest.mark.parametrize('args, exp_filename', [
    ([], 'type1.dump1.txt'),
    (['0'], 'type1.dump0.txt'),
    (['dump', '0'], 'type1.dump0.txt'),
    (['1'], 'type1.dump1.txt'),
    (['2'], 'type1.dump2.txt'),
    (['3'], 'type1.dump3.txt'),
    (['4'], 'type1.dump4.txt'),
    (['5'], 'type1.dump5.txt'),
    (['6'], 'type1.dump6.txt'),
    (['6', 'd'], 'type1.dump6d.txt'),
    (['6', 'n'], 'type1.dump6n.txt'),
])
def test_dump(args, exp_filename):
    if args:
        args.insert(0, '-o')

    if any([arg in args for arg in ('4', '5', '6')]):
        skip = []
    else:
        skip = ['-s', '## Filename']

    actual_path = runner(CMD + ['-f', 'type1.pfa'] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


# ----------
# Test CFF2 font functions
# ----------

def test_varread():
    # Test read CCF2 VF, write snapshot.
    actual_path = runner(CMD + ['-o', 't1', '-f', CFF2_SIMPLE_VF_NAME])
    new_file = CFF2_SIMPLE_VF_NAME.rsplit('.', 1)[0] + '.pfa'
    expected_path = _get_expected_path(new_file)
    assert differ([expected_path, actual_path]) is True
