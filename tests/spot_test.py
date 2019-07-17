import pytest
import subprocess

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import get_expected_path

TOOL = 'spot'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


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
    (['t', '_GPOS=6'], 'dump_GPOS=6.txt'),
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
    (['t', '_name=3'], 'dump_name=3.txt'),
    (['t', '_name=4'], 'dump_name=4.txt'),
    (['t', '_CFF_=6'], 'dump_CFF_=6.ps'),
    (['t', '_CFF_=6', 'a'], 'dump_CFF_=6a.ps'),
    (['t', '_CFF_=6', 'c'], 'dump_CFF_=6c.ps'),
    (['t', '_CFF_=6', 'R'], 'dump_CFF_=6R.ps'),
    (['t', '_CFF_=6', 'g2,5-7'], 'dump_CFF_=6g.ps'),
    (['t', '_CFF_=6', 's1.5,0.5'], 'dump_CFF_=6s.ps'),
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
    actual_path = runner(CMD + ['-s', '-f', 'black.otf', '-o'] + args)
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('table', ['GSUB', 'GPOS'])
def test_feature_tags_bug691(table):
    exp_filename = f'SourceCodePro-Regular_{table}.ps'
    actual_path = runner(CMD + ['-s', '-f', 'SourceCodePro-Regular.otf',
                                '-o', 't', f'_{table}=8'])
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('font_format', ['otf', 'ttf'])
def test_long_glyph_name_bug373(font_format):
    file_name = 'long_glyph_name.' + font_format
    actual_path = runner(CMD + ['-s', '-o', 't', '_GSUB=7', '-f', file_name])
    expected_path = get_expected_path(f'bug373_{font_format}.txt')
    assert differ([expected_path, actual_path])


def test_buffer_overrun_bug465():
    """ Fix bug where a fixed length string buffer was overrun. The test
    font was built by building an otf from makeotf_data/input/t1pfa.pfa,
    dumping this to ttx, and then hand-editing the ttx file to have
    empty charstrings, and a ContextPos lookup type 7, format 2.
    Hand-editing the ttx was needed as makeotf does not build this
    lookup type, nor this post rule format. This test validates that the
    current spot code correctly reports the test font GPOS table."""
    file_name = "bug465/bug465.otf"
    expected_path = get_expected_path('bug465_otf.txt')
    actual_path = runner(
        CMD + ['-s', expected_path, '-o', 't', '_GPOS=7', '-f', file_name])
    assert differ([expected_path, actual_path])


def test_full_unicode_ranges_bug813():
    font_name = 'AdobeBlack2VF.otf'
    actual_path = runner(CMD + ['-s', '-o', 't', '_OS/2', '-f', font_name])
    expected_path = get_expected_path('AdobeBlack2VF_OS2.txt')
    assert differ([expected_path, actual_path])
