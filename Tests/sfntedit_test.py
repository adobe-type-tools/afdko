from __future__ import print_function, division, absolute_import

import os
import tempfile

from fontTools.ttLib import TTFont

from runner import main as runner
from differ import main as differ

TOOL = 'sfntedit'
CMD = ['-t', TOOL]

LIGHT = 'light.otf'
ITALIC = 'italic.otf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_test_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    return tempfile.mkstemp()[1]


def _font_has_table(font_path, table_tag):
    font = TTFont(font_path)
    return table_tag in font


def _generate_ttx_dump(font_path):
    font = TTFont(font_path)
    temp_path = _get_temp_file_path()
    font.saveXML(temp_path)
    return temp_path


# -----
# Tests
# -----

# This test is commented because sfntedit does not exit with 0
# def test_no_options():
#     actual_path = runner(CMD + ['-r', '-f', LIGHT])
#     expected_path = _get_expected_path('list_sfnt.txt')
#     assert differ([expected_path, actual_path, '-l', '1'])


def test_extract_table():
    save_path = _get_temp_file_path()
    actual_path = runner(CMD + ['-o', 'x', '_GDEF={}'.format(save_path),
                                '-f', LIGHT, '-s', save_path])
    expected_path = _get_expected_path('GDEF_light.tb')
    assert differ([expected_path, actual_path, '-m', 'bin'])

# This test fails on Windows and passes on Mac
# https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.108
# https://travis-ci.org/adobe-type-tools/afdko/builds/372889493
# def test_delete_table():
#     actual_path = runner(CMD + ['-o', 'd', '_GDEF', '-f', LIGHT])
#     expected_path = _get_expected_path('light_n_GDEF.otf')
#     assert differ([expected_path, actual_path, '-m', 'bin'])


def test_add_table():
    test_path = _get_test_path('GDEF_italic.tb')
    assert _font_has_table(_get_test_path(ITALIC), 'GDEF') is False
    actual_path = runner(CMD + ['-o', 'a', '_GDEF={}'.format(test_path),
                                '-f', ITALIC])
    expected_path = _get_expected_path('italic_w_GDEF.otf')
    assert differ([expected_path, actual_path, '-m', 'bin']) is False
    assert _font_has_table(actual_path, 'GDEF')
    actual_ttx = _generate_ttx_dump(actual_path)
    expected_ttx = _generate_ttx_dump(expected_path)
    assert differ([expected_ttx, actual_ttx,
                   '-s', '    <checkSumAdjustment'])
