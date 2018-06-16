from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import rmtree

from runner import main as runner
from differ import main as differ

TOOL = 'makeinstancesufo'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_output_path(file_name, dir_name):
    return os.path.join(data_dir_path, dir_name, file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def teardown_module(module):
    """
    teardown the temporary output directory that contains the UFO instances
    """
    rmtree(os.path.join(data_dir_path, 'temp_output'))


# -----
# Tests
# -----

@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'extralight.ufo'),  # hint/remove overlap/normalize/round
    (['_1', 'a'], 'light.ufo'),  # no hint
    (['_2', 'dec'], 'regular.ufo'),  # no round
    (['_3', 'c'], 'semibold.ufo'),  # no remove overlap
    (['_4', 'n'], 'bold.ufo'),  # no normalize
    (['_5', 'a', 'c', 'n'], 'black.ufo'),  # round only
])
def test_options(args, ufo_filename):
    runner(['-t', TOOL, '-n', '-o', 'd',
            '_{}'.format(_get_input_path('font.designspace')), 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])
