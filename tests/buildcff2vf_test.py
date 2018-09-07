from __future__ import print_function, division, absolute_import

import os
from shutil import copytree
import tempfile

from fontTools.ttLib import TTFont

from .runner import main as runner
from .differ import main as differ

TOOL = 'buildcff2vf'
CMD = ['-t', TOOL]

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _generate_ttx_dump(font_path, tables=None):
    font = TTFont(font_path)
    temp_path = tempfile.mkstemp()[1]
    font.saveXML(temp_path, tables=tables)
    return temp_path


# -----
# Tests
# -----

def test_cjk_vf():
    input_dir = _get_input_path('CJKVar')
    temp_dir = os.path.join(tempfile.mkdtemp(), 'CJKVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'CJKVar.designspace')
    runner(CMD + ['-n', '-o', 'p', '_{}'.format(ds_path)])
    actual_path = os.path.join(temp_dir, 'CJKVar.otf')
    actual_ttx = _generate_ttx_dump(actual_path,
                                    ['CFF2', 'HVAR', 'avar', 'fvar'])
    expected_ttx = _get_expected_path('CJKVar.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
