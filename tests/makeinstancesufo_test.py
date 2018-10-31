from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import rmtree

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path

TOOL = 'makeinstancesufo'

DATA_DIR = os.path.join(os.path.dirname(__file__), TOOL + '_data')


def _get_output_path(file_name, dir_name):
    return os.path.join(DATA_DIR, dir_name, file_name)


def teardown_module():
    """
    teardown the temporary output directory that contains the UFO instances
    """
    rmtree(os.path.join(DATA_DIR, 'temp_output'))


# -----
# Tests
# -----

@pytest.mark.parametrize('args, ufo_filename, num', [
    (['_0'], 'extralight.ufo', 0),  # hint/remove overlap/normalize/round
    (['_1', 'a'], 'light.ufo', 0),  # no hint
    (['_2', 'r'], 'regular.ufo', 0),  # no round
    (['_2', 'r', 'n'], 'regular.ufo', 1),  # no round & no normalize
    (['_3', 'c'], 'semibold.ufo', 0),  # no remove overlap
    (['_4', 'n'], 'bold.ufo', 0),  # no normalize
    (['_5', 'a', 'c', 'n'], 'black.ufo', 0),  # round only
])
def test_options(args, ufo_filename, num):
    runner(['-t', TOOL, '-o', 'd',
            '_{}'.format(get_input_path('font.designspace')), 'i'] + args)
    if num:
        expct_filename = '{}{}.ufo'.format(ufo_filename[:-4], num)
    else:
        expct_filename = ufo_filename
    expected_path = _get_output_path(expct_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])
