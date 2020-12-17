import pytest
from shutil import copy2, copytree
import os

from booleanOperations.booleanGlyph import BooleanGlyph
from defcon import Glyph

from afdko.checkoutlinesufo import remove_tiny_sub_paths

from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
    get_font_format,
)
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
)
from runner import main as runner
from differ import main as differ

TOOL = 'checkoutlinesufo'
CMD = ['-t', TOOL]


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


@pytest.mark.parametrize('ufo_filename', ['ufo2.ufo', 'ufo3.ufo'])
@pytest.mark.parametrize('args, expct_label', [
    (['e', 'w', 'q'], 'dflt-layer.ufo'),
    (['e', 'q'], 'proc-layer.ufo'),
])
def test_remove_overlap_ufo(args, ufo_filename, expct_label):
    actual_path = get_temp_dir_path(ufo_filename)
    copytree(get_input_path(ufo_filename), actual_path)
    runner(CMD + ['-f', actual_path, '-o'] + args)
    expct_filename = f'{ufo_filename[:-4]}-{expct_label}'
    expected_path = get_expected_path(expct_filename)
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('filename, diffmode', [
    ('font.pfa', []),
    ('font.pfb', ['-m', 'bin']),
    ('font.cff', ['-m', 'bin']),
])
def test_remove_overlap_type1_cff(filename, diffmode):
    actual_path = get_temp_file_path()
    copy2(get_input_path(filename), actual_path)
    runner(CMD + ['-f', actual_path, '-o', 'e', 'q'])
    expected_path = get_expected_path(filename)
    assert differ([expected_path, actual_path] + diffmode)


def test_remove_overlap_otf():
    actual_path = get_temp_file_path()
    copy2(get_input_path('font.otf'), actual_path)
    runner(CMD + ['-f', actual_path, '-o', 'e', 'q'])
    actual_ttx = generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = get_expected_path('font.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_bug790():
    """
    Test case where the result of overlap removal resulted in coincident points
    at contour start. Previously caused a crash when attempting to set start
    point on the second point.
    """
    ufoname = 'bug790.ufo'
    actual_path = get_temp_dir_path(ufoname)
    copytree(get_input_path(ufoname), actual_path)
    runner(CMD + ['-f', actual_path, '-o', 'e'])
    expected_path = get_expected_path(ufoname)
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('filename, diffmode', [
    ('cidfont.subset', ['-m', 'bin'])
])
def test_cidkeyed_remove_overlap(filename, diffmode):
    actual_path = get_temp_file_path()
    copy2(get_input_path(filename), actual_path)
    runner(CMD + ['-f', actual_path, '-o', 'e', 'q', '=no-overlap-checks'])
    expected_path = get_expected_path('cidfont.subset.checked')
    assert differ([expected_path, actual_path] + diffmode)


@pytest.mark.parametrize('input_font, expected_font', [
    ('ufo3.ufo', 'ufo3-proc-layer.ufo'),
    ('font.pfa', 'font.pfa'),
])
def test_output_file_option(input_font, expected_font):
    """
    Test the '-o' (output file) option.
    """
    in_path = get_input_path(input_font)
    out_path = os.path.join(get_temp_dir_path(), input_font)
    expected_path = get_expected_path(expected_font)
    runner(CMD + ['-f', in_path, '-o', 'e', 'o', '_' + out_path])

    assert get_font_format(out_path) == get_font_format(in_path)
    assert differ([expected_path, out_path])
