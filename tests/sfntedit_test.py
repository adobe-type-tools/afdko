import pytest
import subprocess

from runner import main as runner
from differ import main as differ
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump, font_has_table)

TOOL = 'sfntedit'
CMD = ['-t', TOOL]

LIGHT = 'light.otf'
ITALIC = 'italic.otf'


# -----
# Tests
# -----

def test_exit_no_option():
    # When ran by itself, 'sfntedit' prints the usage
    assert subprocess.call([TOOL]) == 1


@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--bogus'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


def test_no_options():
    actual_path = runner(CMD + ['-s', '-f', LIGHT])
    expected_path = get_expected_path('list_sfnt.txt')
    assert differ([expected_path, actual_path, '-l', '1'])


def test_extract_table():
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'x', f'_GDEF={actual_path}', '-f', LIGHT])
    expected_path = get_expected_path('GDEF_light.tb')
    assert differ([expected_path, actual_path, '-m', 'bin'])

# This test fails on Windows and passes on Mac
# https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.108
# https://travis-ci.org/adobe-type-tools/afdko/builds/372889493
# def test_delete_table():
#     actual_path = runner(CMD + ['-o', 'd', '_GDEF', '-f', LIGHT])
#     expected_path = get_expected_path('light_n_GDEF.otf')
#     assert differ([expected_path, actual_path, '-m', 'bin'])


def test_add_table():
    table_path = get_input_path('GDEF_italic.tb')
    font_path = get_input_path(ITALIC)
    actual_path = get_temp_file_path()
    assert font_has_table(font_path, 'GDEF') is False
    runner(CMD + ['-a', '-o', 'a', f'_GDEF={table_path}',
                  '-f', font_path, actual_path])
    expected_path = get_expected_path('italic_w_GDEF.otf')
    assert differ([expected_path, actual_path, '-m', 'bin']) is False
    assert font_has_table(actual_path, 'GDEF')
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = generate_ttx_dump(expected_path)
    assert differ([expected_ttx, actual_ttx, '-s', '    <checkSumAdjustment'])


def test_linux_ci_failure_bug570():
    table_path = get_input_path('1_fdict.cff')
    font_path = get_input_path('core.otf')
    actual_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'a', f'_CFF={table_path}',
                  '-f', font_path, actual_path])
    expected_path = get_expected_path('1_fdict.otf')
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_missing_table_delete_bug160():
    actual_path = get_temp_file_path()
    stderr_path = runner(CMD + ['-s', '-e', '-o', 'd', '_xyz,GSUB',
                                '-f', LIGHT, actual_path])
    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert b'[WARNING]: table missing (xyz )' in output
    assert font_has_table(get_input_path(LIGHT), 'GSUB')
    assert not font_has_table(actual_path, 'GSUB')


def test_missing_table_extract_bug160():
    actual_path = get_temp_file_path()
    stderr_path = runner(CMD + ['-s', '-e', '-f', LIGHT, actual_path, '-o',
                                'x', f'_xyz,head={actual_path}'])
    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert b'[WARNING]: table missing (xyz )' in output
    expected_path = get_expected_path('head_light.tb')
    assert differ([expected_path, actual_path, '-m', 'bin'])
