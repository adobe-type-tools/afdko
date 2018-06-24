from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import copytree
import tempfile

from booleanOperations.booleanGlyph import BooleanGlyph
from defcon import Glyph

from afdko.Tools.SharedData.FDKScripts.CheckOutlinesUFO import (
    remove_tiny_sub_paths)

from .runner import main as runner
from .differ import main as differ

TOOL = 'checkoutlinesufo'
CMD = ['-t', TOOL]

UFO2_NAME = 'font.ufo'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


# -----
# Tests
# -----

def test_remove_tiny_sub_paths_large_contour():
    g = Glyph()
    p = g.getPen()
    p.moveTo((100, 100))
    p.lineTo((200, 200))
    p.lineTo((0, 100))
    p.closePath()
    assert len(g[0]) == 3
    assert g.bounds == (0, 100, 200, 200)
    bg = BooleanGlyph(g)
    assert remove_tiny_sub_paths(bg, 25, []) == []


def test_remove_tiny_sub_paths_small_contour():
    g = Glyph()
    p = g.getPen()
    p.moveTo((1, 1))
    p.lineTo((2, 2))
    p.lineTo((0, 1))
    p.closePath()
    assert len(g[0]) == 3
    assert g.bounds == (0, 1, 2, 2)
    bg = BooleanGlyph(g)
    assert remove_tiny_sub_paths(bg, 25, []) == \
        ['Contour 0 is too small: bounding box is less than minimum area. '
         'Start point: ((1, 1)).']


@pytest.mark.parametrize('args, expct_filename', [
    (['e', 'w', 'q'], 'dflt_layer.ufo'),
    (['e', 'q'], 'proc_layer.ufo'),
])
def test_remove_overlap(args, expct_filename):
    actual_path = os.path.join(tempfile.mkdtemp(), UFO2_NAME)
    copytree(_get_input_path(UFO2_NAME), actual_path)
    runner(CMD + ['-n', '-f', actual_path, '-o'] + args)
    expected_path = _get_expected_path(expct_filename)
    assert differ([expected_path, actual_path])
