from __future__ import print_function, division, absolute_import

import os
import platform
import pytest
import subprocess32 as subprocess
import tempfile

from .runner import main as runner
from .differ import main as differ

TOOL = 'mergefonts'
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

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
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


def test_convert_to_cid():
    font1_filename = 'font1.pfa'
    font2_filename = 'font2.pfa'
    font3_filename = 'font3.pfa'
    alias1_filename = 'alias1.txt'
    alias2_filename = 'alias2.txt'
    alias3_filename = 'alias3.txt'
    fontinfo_filename = 'cidfontinfo.txt'
    actual_path = _get_temp_file_path()
    expected_path = _get_expected_path('cidfont.ps')
    runner(CMD + ['-n', '-o', 'cid', '-f', fontinfo_filename, actual_path,
                  alias1_filename, font1_filename,
                  alias2_filename, font2_filename,
                  alias3_filename, font3_filename])
    assert differ([expected_path, actual_path, '-m', 'bin'])
