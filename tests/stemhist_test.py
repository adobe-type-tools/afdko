from __future__ import print_function, division, absolute_import

import pytest
import subprocess32 as subprocess

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path, get_temp_file_path

TOOL = 'stemhist'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


@pytest.mark.parametrize('arg', ([], ['a'], ['all']))
@pytest.mark.parametrize('font_filename', [
    'font.pfa', 'font.ufo', 'font.cff', 'font.otf', 'cidfont.ps'])
def test_stems_and_zones(arg, font_filename):
    prefix = font_filename.split('.')[0]
    if 'a' in arg:
        suffixes = ['.top.txt', '.bot.txt']
    else:
        suffixes = ['.hstm.txt', '.vstm.txt']

    report_path = get_temp_file_path()
    runner(CMD + ['-f', font_filename, '-o',
                  'o', '_{}'.format(report_path)] + arg)
    for suffix in suffixes:
        actual_path = '{}{}'.format(report_path, suffix)
        exp_suffix = suffix
        if 'all' in arg:
            exp_suffix = '.all' + suffix
        expected_path = get_expected_path('{}{}'.format(prefix, exp_suffix))
        assert differ([expected_path, actual_path, '-l', '1'])
