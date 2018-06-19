from __future__ import print_function, division, absolute_import

import os
import pytest

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'spot'
CMD = ['-t', TOOL]

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


# -----
# Tests
# -----

@pytest.mark.parametrize('args, exp_filename', [
    (['T'], 'list_tables.txt'),
    (['F'], 'list_features.txt'),
    (['G'], 'glyph_proof.ps'),  # same as dump_CFF_=7
    (['G', 'd'], 'glyph_proof_no_header.ps'),
    (['G', 'd', 'br'], 'glyph_proof_no_header_multipage.ps'),
    (['Proof'], 'proof_all_features.ps'),
    (['t', '_BASE=4'], 'dump_BASE=4.txt'),
    (['t', '_BASE=5'], 'dump_BASE=5.txt'),
    (['t', '_GDEF=7'], 'dump_GDEF=7.txt'),
    (['t', '_GPOS=5'], 'dump_GPOS=5.txt'),
    # Segmentation fault: 11
    # https://github.com/adobe-type-tools/afdko/issues/371
    # (['t', '_GPOS=6'], 'dump_GPOS=6.txt'),
    (['t', '_GPOS=7'], 'dump_GPOS=7.txt'),
    (['t', '_GPOS=8'], 'dump_GPOS=8.ps'),
    (['t', '_GSUB=4'], 'dump_GSUB=4.txt'),
    (['t', '_GSUB=5'], 'dump_GSUB=5.txt'),
    (['t', '_GSUB=7'], 'dump_GSUB=7.txt'),
    (['t', '_GSUB=8'], 'dump_GSUB=8.ps'),
    (['t', '_cmap=5'], 'dump_cmap=5.txt'),
    (['t', '_cmap=6'], 'dump_cmap=6.txt'),
    (['t', '_cmap=7'], 'dump_cmap=7.txt'),
    (['t', '_cmap=8'], 'dump_cmap=8.txt'),
    (['t', '_cmap=9'], 'dump_cmap=9.ps'),
    (['t', '_cmap=10'], 'dump_cmap=10.ps'),
    (['t', '_cmap=11'], 'dump_cmap=11.txt'),
    (['t', '_hmtx=5'], 'dump_hmtx=5.txt'),
    (['t', '_hmtx=6'], 'dump_hmtx=6.txt'),
    (['t', '_hmtx=7'], 'dump_hmtx=7.txt'),
    (['t', '_hmtx=8'], 'dump_hmtx=8.txt'),
    (['t', '_name=2'], 'dump_name=2.txt'),
    # Fails on Windows, passes on Mac
    # https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.217
    # https://travis-ci.org/adobe-type-tools/afdko/builds/378601819
    # (['t', '_name=3'], 'dump_name=3.txt'),
    (['t', '_name=4'], 'dump_name=4.txt'),
    (['t', '_CFF_=6'], 'dump_CFF_=6.ps'),
    (['t', '_CFF_=6', 'a'], 'dump_CFF_=6a.ps'),
    (['t', '_CFF_=6', 'c'], 'dump_CFF_=6c.ps'),
    (['t', '_CFF_=6', 'R'], 'dump_CFF_=6R.ps'),
    (['t', '_CFF_=6', 'g2,5-7'], 'dump_CFF_=6g.ps'),
    # Fails on Windows, passes on Mac
    # https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.217
    # https://travis-ci.org/adobe-type-tools/afdko/builds/378601819
    # (['t', '_CFF_=6', 's1.5,0.5'], 'dump_CFF_=6s.ps'),
    (['t', '_CFF_=6', 'b-10,-10,200,500', 'g4'], 'dump_CFF_=6b.ps'),
    (['t', '_CFF_=7'], 'dump_CFF_=7.ps'),
    (['t', '_CFF_=7', 'g2,5-7'], 'dump_CFF_=7g.ps'),
    (['t', '_CFF_=8'], 'dump_CFF_=8.ps'),
    (['t', '_CFF_=8', 'g2,5-7'], 'dump_CFF_=8g.ps'),
])
def test_options(args, exp_filename):
    if 'CFF_=7' in exp_filename or exp_filename == 'glyph_proof.ps':
        skip = ['0 743.4 moveto (CFF:']
    elif 'CFF_=6' in exp_filename:
        skip = ['318 764 moveto (' + SPLIT_MARKER + '72 750 moveto (']
    else:
        skip = []
    if skip:
        skip.insert(0, '-s')
    actual_path = runner(CMD + ['-r', '-f', 'black.otf', '-o'] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('font_format', ['otf', 'ttf'])
def test_bug373(font_format):
    file_name = 'long_glyph_name.' + font_format
    actual_path = runner(CMD + ['-r', '-o', 't', '_GSUB=7', '-f', file_name])
    expected_path = _get_expected_path('bug373_{}.txt'.format(font_format))
    assert differ([expected_path, actual_path])
