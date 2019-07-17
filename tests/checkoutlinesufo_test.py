import os
import pytest
from shutil import copytree
import tempfile

from booleanOperations.booleanGlyph import BooleanGlyph
from defcon import Glyph

from afdko.checkoutlinesufo import remove_tiny_sub_paths

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path, get_expected_path

TOOL = 'checkoutlinesufo'
CMD = ['-t', TOOL]

UFO2_NAME = 'ufo2.ufo'
UFO3_NAME = 'ufo3.ufo'


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


@pytest.mark.parametrize('ufo_filename', [UFO2_NAME, UFO3_NAME])
@pytest.mark.parametrize('args, expct_label', [
    (['e', 'w', 'q'], 'dflt-layer.ufo'),
    (['e', 'q'], 'proc-layer.ufo'),
])
def test_remove_overlap(args, ufo_filename, expct_label):
    actual_path = os.path.join(tempfile.mkdtemp(), ufo_filename)
    copytree(get_input_path(ufo_filename), actual_path)
    runner(CMD + ['-f', actual_path, '-o'] + args)
    expct_filename = f'{ufo_filename[:-4]}-{expct_label}'
    expected_path = get_expected_path(expct_filename)
    assert differ([expected_path, actual_path])
