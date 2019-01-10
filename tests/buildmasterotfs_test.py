from __future__ import print_function, division, absolute_import

import os
from shutil import copytree
import tempfile

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import get_input_path, get_expected_path, generate_ttx_dump

TOOL = 'buildmasterotfs'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

def test_cjk_var():
    """
    Builds all OTFs for the 'CJKVar' project and then diffs two of them.
    """
    input_dir = get_input_path('CJKVar')
    temp_dir = os.path.join(tempfile.mkdtemp(), 'CJKVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'CJKVar.designspace')
    runner(CMD + ['-o', '_{}'.format(ds_path)])

    otf1_path = os.path.join(
        temp_dir, 'Normal', 'Master_8', 'MasterSet_Kanji-w600.00.otf')
    otf2_path = os.path.join(
        temp_dir, 'Condensed', 'Master_8', 'MasterSet_Kanji_75-w600.00.otf')

    for otf_path in (otf1_path, otf2_path):
        actual_ttx = generate_ttx_dump(otf_path)
        expected_ttx = get_expected_path(
            os.path.basename(otf_path)[:-3] + 'ttx')
        assert differ([expected_ttx, actual_ttx,
                       '-s',
                       '<ttFont sfntVersion' + SPLIT_MARKER +
                       '    <checkSumAdjustment value=' + SPLIT_MARKER +
                       '    <created value=' + SPLIT_MARKER +
                       '    <modified value=',
                       '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])
