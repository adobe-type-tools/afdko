from __future__ import print_function, division, absolute_import

import os

from runner import main as runner
from differ import main as differ
from differ import SPLIT_MARKER

TOOL = 'spot'
CMD = ['-t', TOOL]

BLACK = 'black.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_list_tables():
    actual_path = runner(CMD + ['-r', '-o', 'T', '-f', BLACK])
    expected_path = _get_expected_path('list_tables.txt')
    assert differ([expected_path, actual_path]) is True


def test_list_features():
    actual_path = runner(CMD + ['-r', '-o', 'F', '-f', BLACK])
    expected_path = _get_expected_path('list_features.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_base_equal_4():
    actual_path = runner(CMD + ['-r', '-o', 't', '_BASE=4', '-f', BLACK])
    expected_path = _get_expected_path('dump_BASE=4.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_base_equal_5():
    actual_path = runner(CMD + ['-r', '-o', 't', '_BASE=5', '-f', BLACK])
    expected_path = _get_expected_path('dump_BASE=5.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gdef_equal_7():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GDEF=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_GDEF=7.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gpos_equal_5():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GPOS=5', '-f', BLACK])
    expected_path = _get_expected_path('dump_GPOS=5.txt')
    assert differ([expected_path, actual_path]) is True


# Segmentation fault: 11
# https://github.com/adobe-type-tools/afdko/issues/371
# def test_dump_gpos_equal_6():
#     actual_path = runner(CMD + ['-r', '-o', 't', '_GPOS=6', '-f', BLACK])
#     expected_path = _get_expected_path('dump_GPOS=6.txt')
#     assert differ([expected_path, actual_path]) is True


def test_dump_gpos_equal_7():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GPOS=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_GPOS=7.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gpos_equal_8():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GPOS=8', '-f', BLACK])
    expected_path = _get_expected_path('dump_GPOS=8.ps')
    assert differ([expected_path, actual_path]) is True


def test_dump_gsub_equal_4():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=4', '-f', BLACK])
    expected_path = _get_expected_path('dump_GSUB=4.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gsub_equal_5():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=5', '-f', BLACK])
    expected_path = _get_expected_path('dump_GSUB=5.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gsub_equal_7():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_GSUB=7.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_gsub_equal_8():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=8', '-f', BLACK])
    expected_path = _get_expected_path('dump_GSUB=8.ps')
    assert differ([expected_path, actual_path]) is True


def test_bug373_dump_gsub_equal_7_long_glyph_name_otf():
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=7',
                                '-f', 'long_glyph_name.otf'])
    expected_path = _get_expected_path('bug373_otf.txt')
    assert differ([expected_path, actual_path]) is True


# Abort trap: 6
# https://github.com/adobe-type-tools/afdko/issues/373
# def test_bug373_dump_gsub_equal_7_long_glyph_name_ttf():
#     actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=7',
#                                 '-f', 'long_glyph_name.ttf'])
#     expected_path = _get_expected_path('bug373_ttf.txt')
#     assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_5():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=5', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=5.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_6():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=6', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=6.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_7():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=7.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_8():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=8', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=8.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_9():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=9', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=9.ps')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_10():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=10', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=10.ps')
    assert differ([expected_path, actual_path]) is True


def test_dump_cmap_equal_11():
    actual_path = runner(CMD + ['-r', '-o', 't', '_cmap=11', '-f', BLACK])
    expected_path = _get_expected_path('dump_cmap=11.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_cff_equal_6():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


def test_dump_cff_equal_6a():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 'a', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6a.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


def test_dump_cff_equal_6c():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 'c', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6c.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


def test_dump_cff_equal_6r():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 'R', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6R.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


def test_dump_cff_equal_6g():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 'g2,5-7',
                                '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6g.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


# Fails on Windows, passes on Mac
# https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.217
# https://travis-ci.org/adobe-type-tools/afdko/builds/378601819
# def test_dump_cff_equal_6s():
#     actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 's1.5,0.5',
#                                 '-f', BLACK])
#     expected_path = _get_expected_path('dump_CFF_=6s.ps')
#     assert differ([expected_path, actual_path,
#                    '-s',
#                    '318 764 moveto (' + SPLIT_MARKER +
#                    '72 750 moveto ('
#                    ]) is True


def test_dump_cff_equal_6b():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=6', 'b-10,-10,200,500',
                                'g4', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=6b.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '318 764 moveto (' + SPLIT_MARKER +
                   '72 750 moveto ('
                   ]) is True


def test_dump_cff_equal_7():  # same as test_glyph_proof
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=7.ps')
    assert differ([expected_path, actual_path,
                   '-s', '0 743.4 moveto (CFF:']) is True


def test_dump_cff_equal_7g():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=7', 'g2,5-7',
                                '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=7g.ps')
    assert differ([expected_path, actual_path,
                   '-s', '0 743.4 moveto (CFF:']) is True


def test_dump_cff_equal_8():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=8', '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=8.ps')
    assert differ([expected_path, actual_path]) is True


def test_dump_cff_equal_8g():
    actual_path = runner(CMD + ['-r', '-o', 't', '_CFF_=8', 'g2,5-7',
                                '-f', BLACK])
    expected_path = _get_expected_path('dump_CFF_=8g.ps')
    assert differ([expected_path, actual_path]) is True


def test_dump_hmtx_equal_5():
    actual_path = runner(CMD + ['-r', '-o', 't', '_hmtx=5', '-f', BLACK])
    expected_path = _get_expected_path('dump_hmtx=5.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_hmtx_equal_6():
    actual_path = runner(CMD + ['-r', '-o', 't', '_hmtx=6', '-f', BLACK])
    expected_path = _get_expected_path('dump_hmtx=6.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_hmtx_equal_7():
    actual_path = runner(CMD + ['-r', '-o', 't', '_hmtx=7', '-f', BLACK])
    expected_path = _get_expected_path('dump_hmtx=7.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_hmtx_equal_8():
    actual_path = runner(CMD + ['-r', '-o', 't', '_hmtx=8', '-f', BLACK])
    expected_path = _get_expected_path('dump_hmtx=8.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_name_equal_2():
    actual_path = runner(CMD + ['-r', '-o', 't', '_name=2', '-f', BLACK])
    expected_path = _get_expected_path('dump_name=2.txt')
    assert differ([expected_path, actual_path]) is True


# Fails on Windows, passes on Mac
# https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.217
# https://travis-ci.org/adobe-type-tools/afdko/builds/378601819
# def test_dump_name_equal_3():
#     actual_path = runner(CMD + ['-r', '-o', 't', '_name=3', '-f', BLACK])
#     expected_path = _get_expected_path('dump_name=3.txt')
#     assert differ([expected_path, actual_path]) is True


def test_dump_name_equal_4():
    actual_path = runner(CMD + ['-r', '-o', 't', '_name=4', '-f', BLACK])
    expected_path = _get_expected_path('dump_name=4.txt')
    assert differ([expected_path, actual_path]) is True


def test_glyph_proof():  # same as test_dump_cff_equal_7
    actual_path = runner(CMD + ['-r', '-o', 'G', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof.ps')
    assert differ([expected_path, actual_path,
                   '-s', '0 743.4 moveto (CFF:']) is True


def test_glyph_proof_no_header():
    actual_path = runner(CMD + ['-r', '-o', 'G', 'd', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof_no_header.ps')
    assert differ([expected_path, actual_path]) is True


def test_glyph_proof_no_header_multipage():
    actual_path = runner(CMD + ['-r', '-o', 'G', 'd', 'br', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof_no_header_multipage.ps')
    assert differ([expected_path, actual_path]) is True


def test_proof_all_features():
    actual_path = runner(CMD + ['-r', '-o', 'Proof', '-f', BLACK])
    expected_path = _get_expected_path('proof_all_features.ps')
    assert differ([expected_path, actual_path]) is True
