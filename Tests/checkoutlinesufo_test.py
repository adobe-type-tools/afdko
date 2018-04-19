from __future__ import print_function, division, absolute_import

import pytest

from booleanOperations.booleanGlyph import BooleanGlyph
from defcon import Glyph

from afdko.Tools.SharedData.FDKScripts.CheckOutlinesUFO import (
    remove_tiny_sub_paths)


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
