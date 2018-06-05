from __future__ import print_function, division, absolute_import

import os

from runner import main as runner
from differ import main as differ

TOOL = 'sfntdiff'
CMD = ['-t', TOOL]

REGULAR = 'regular.otf'
BOLD = 'bold.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_default_diff():
    actual_path = runner(CMD + ['-r', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('dflt.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_default_with_timestamp_diff():
    actual_path = runner(CMD + ['-r', '-o', 'T', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('dflt.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_level_0_diff():
    actual_path = runner(CMD + ['-r', '-o', 'd', '_0', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('dflt.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_level_1_diff():
    actual_path = runner(CMD + ['-r', '-o', 'd', '_1', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('level1.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_level_2_diff():
    actual_path = runner(CMD + ['-r', '-o', 'd', '_2', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('level2.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_level_3_diff():
    actual_path = runner(CMD + ['-r', '-o', 'd', '_3', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('level3.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True


def test_level_4_diff():
    actual_path = runner(CMD + ['-r', '-o', 'd', '_4', '-f', REGULAR, BOLD])
    expected_path = _get_expected_path('level4.txt')
    assert differ([expected_path, actual_path, '-l', '1-4']) is True
