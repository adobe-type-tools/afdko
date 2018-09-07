from __future__ import print_function, division, absolute_import

import os
import platform
import pytest
import subprocess32 as subprocess
import tempfile

from .runner import main as runner
from .differ import main as differ

TOOL = 'stemhist'
CMD = ['-t', TOOL]

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    file_descriptor, path = tempfile.mkstemp()
    os.close(file_descriptor)
    return path


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    if platform.system() == 'Windows':
        tool_name = TOOL + '.exe'
    else:
        tool_name = TOOL
    assert subprocess.call([tool_name, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    if platform.system() == 'Windows':
        tool_name = TOOL + '.exe'
    else:
        tool_name = TOOL
    assert subprocess.call([tool_name, arg]) == 1


@pytest.mark.parametrize('arg', ([], ['a'], ['all']))
@pytest.mark.parametrize('font_filename', [
    'font.pfa', 'font.ufo', 'font.cff', 'font.otf', 'cidfont.ps'])
def test_stems_and_zones(arg, font_filename):
    prefix = font_filename.split('.')[0]
    if 'a' in arg:
        suffixes = ['.top.txt', '.bot.txt']
    else:
        suffixes = ['.hstm.txt', '.vstm.txt']

    report_path = _get_temp_file_path()
    runner(CMD + ['-n', '-f', font_filename, '-o',
                  'o', '_{}'.format(report_path)] + arg)
    for suffix in suffixes:
        actual_path = '{}{}'.format(report_path, suffix)
        exp_suffix = suffix
        if 'all' in arg:
            exp_suffix = '.all' + suffix
        expected_path = _get_expected_path('{}{}'.format(prefix, exp_suffix))
        assert differ([expected_path, actual_path, '-l', '1'])
