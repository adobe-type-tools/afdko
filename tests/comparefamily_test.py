from __future__ import print_function, division, absolute_import

import os
import pytest
import subprocess32 as subprocess

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path, get_expected_path, get_temp_file_path

TOOL = 'comparefamily'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('font_format', ['otf', 'ttf'])
@pytest.mark.parametrize('font_family', ['source-code-pro'])
def test_report(font_family, font_format):
    input_dir = os.path.join(get_input_path(font_family), font_format)
    log_path = get_temp_file_path()
    runner(CMD + ['-o', 'd', '_{}'.format(input_dir), 'tolerance', '_3',
                  'rm', 'rn', 'rp', 'l', '_{}'.format(log_path)])
    expected_path = get_expected_path('{}_{}.txt'.format(
                                      font_family, font_format))
    assert differ([expected_path, log_path, '-l', '1'])


def test_report2():
    input_dir = get_input_path('font-family')
    expected_path = get_expected_path('font-family.txt')
    log_path = get_temp_file_path()
    runner(CMD + ['-o', 'd', '_{}'.format(input_dir),
                  'rm', 'rn', 'rp', 'l', '_{}'.format(log_path)])
    assert differ([expected_path, log_path, '-l', '1'])


def test_st28_basic_cmap():
    input_dir = get_input_path('basic_cmap')
    expected_path = get_expected_path('st28_basic_cmap.txt')
    log_path = get_temp_file_path()
    runner(CMD + ['-o', 'st', '_28', 'd', '_{}'.format(input_dir),
                  'l', '_{}'.format(log_path)])
    assert differ([expected_path, log_path, '-l', '1'])


def test_font_with_no_cmap():
    # A tx fatal error happens: "OTF: can't find cmap"
    input_dir = get_input_path('no_cmap')
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-o', 'd', '_{}'.format(input_dir)])
    assert err.value.returncode == 1
