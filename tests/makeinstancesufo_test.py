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
    teardown the temporary UFOs or the directory that holds them
    """
    rmtree(os.path.join(DATA_DIR, 'temp_output'), True)
    rmtree(os.path.join(DATA_DIR, 'input', 'same_dir.ufo'), True)


# -----
# Tests
# -----

@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'extralight.ufo'),  # hint/remove overlap/normalize/round
    (['_1', 'a'], 'light.ufo'),  # no hint
    (['_2', 'r'], 'regular.ufo'),  # no round
    (['_3', 'r', 'n'], 'regular1.ufo'),  # no round & no normalize
    (['_4', 'c'], 'semibold.ufo'),  # no remove overlap
    (['_5', 'n'], 'bold.ufo'),  # no normalize
    (['_6', 'a', 'c', 'n'], 'black.ufo'),  # round only
    (['_7', 'a', 'c', 'n'], 'anisotropic.ufo'),
    (['_8', 'a', '=ufo-version', '_2'], 'ufo2.ufo'),  # no hint UFO v2
])
def test_options(args, ufo_filename):
    runner(['-t', TOOL, '-o', 'd',
            '_{}'.format(get_input_path('font.designspace')), 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])


def test_filename_without_dir():
    instance_path = get_input_path('same_dir.ufo')
    assert not os.path.exists(instance_path)
    runner(['-t', TOOL, '-o', 'd',
            '_{}'.format(get_input_path('font.designspace')), 'i', '_9'])
    assert os.path.exists(instance_path)


@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'ufo3regular.ufo'),  # hint/remove overlap/normalize/round
    (['_1', 'a', 'c', 'n'], 'ufo3medium.ufo'),  # round only
    (['_2', 'a', '=ufo-version', '_2'], 'ufo3semibold.ufo'),  # no hint UFO v2
])
def test_ufo3_masters(args, ufo_filename):
    runner(['-t', TOOL, '-o', 'd',
            '_{}'.format(get_input_path('ufo3.designspace')), 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])
