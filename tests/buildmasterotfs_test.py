from __future__ import print_function, division, absolute_import

import os
from shutil import copytree
import tempfile

from fontTools.ttLib import TTFont

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'buildmasterotfs'
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

def test_cjk_var():
    """
    Builds all OTFs for the 'CJKVar' project and then diffs two of them.
    """
    input_dir = _get_input_path('CJKVar')
    temp_dir = os.path.join(tempfile.mkdtemp(), 'CJKVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'CJKVar.designspace')
    runner(CMD + ['-n', '-o', '_{}'.format(ds_path)])

    otf1_path = os.path.join(
        temp_dir, 'Normal', 'Master_8', 'MasterSet_Kanji-w600.00.otf')
    otf2_path = os.path.join(
        temp_dir, 'Condensed', 'Master_8', 'MasterSet_Kanji_75-w600.00.otf')

    for otf_path in (otf1_path, otf2_path):
        actual_ttx = _generate_ttx_dump(otf_path)
        expected_ttx = _get_expected_path(
            os.path.basename(otf_path)[:-3] + 'ttx')
        assert differ([expected_ttx, actual_ttx,
                       '-s',
                       '<ttFont sfntVersion' + SPLIT_MARKER +
                       '    <checkSumAdjustment value=' + SPLIT_MARKER +
                       '    <created value=' + SPLIT_MARKER +
                       '    <modified value=',
                       '-r', '^\s+Version.*;hotconv.*;makeotfexe'])
