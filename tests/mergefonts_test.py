from __future__ import print_function, division, absolute_import

import pytest
import subprocess32 as subprocess

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path, get_temp_file_path, get_input_path

TOOL = 'mergefonts'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


def test_convert_to_cid():
    font1_filename = 'font1.pfa'
    font2_filename = 'font2.pfa'
    font3_filename = 'font3.pfa'
    alias1_filename = 'alias1.txt'
    alias2_filename = 'alias2.txt'
    alias3_filename = 'alias3.txt'
    fontinfo_filename = 'cidfontinfo.txt'
    actual_path = get_temp_file_path()
    expected_path = get_expected_path('cidfont.ps')
    runner(CMD + ['-o', 'cid', '-f', fontinfo_filename, actual_path,
                  alias1_filename, font1_filename,
                  alias2_filename, font2_filename,
                  alias3_filename, font3_filename])
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_warnings_bug635():
    font1_path = get_input_path('bug635/cidfont.ps')
    font2_path = get_input_path('bug635/rotated.ps')
    actual_path = get_temp_file_path()
    expected_path = get_expected_path('bug635.txt')
    warnings_path = runner(
        CMD + ['-s', '-e', '-a', '-f', actual_path, font1_path, font2_path])
    # On Windows the messages start with 'mergefonts.exe:'
    with open(warnings_path, 'rb') as f:
        warnings = f.read().replace(b'mergefonts.exe:', b'mergefonts:')
    with open(warnings_path, 'wb') as f:
        f.write(warnings)
    assert differ([expected_path, warnings_path, '-l', '1,5-7,11-12'])
