from __future__ import print_function, division, absolute_import

import pytest
import subprocess32 as subprocess

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump)

TOOL = 'makeotfexe'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

def test_exit_no_option():
    # It's valid to run 'makeotfexe' without using any options,
    # but if a default-named font file ('font.ps') is NOT found
    # in the current directory, the tool exits with an error
    with pytest.raises(subprocess.CalledProcessError) as err:
        subprocess.check_call([TOOL])
    assert err.value.returncode == 1


@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--bogus'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


@pytest.mark.parametrize('caret_format', [
    'bypos', 'byindex', 'mixed', 'mixed2', 'double', 'double2'])
def test_GDEF_LigatureCaret_bug155(caret_format):
    input_filename = 'bug155/font.pfa'
    feat_filename = 'bug155/caret-{}.fea'.format(caret_format)
    ttx_filename = 'bug155/caret-{}.ttx'.format(caret_format)
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', '_{}'.format(get_input_path(input_filename)),
                        'ff', '_{}'.format(get_input_path(feat_filename)),
                        'o', '_{}'.format(actual_path)])
    actual_ttx = generate_ttx_dump(actual_path, ['GDEF'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-l', '2'])


def test_useMarkFilteringSet_flag_bug196():
    input_filename = "bug196/font.pfa"
    feat_filename = "bug196/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug196.ttx"
    runner(CMD + ['-o', 'f', '_{}'.format(get_input_path(input_filename)),
                        'ff', '_{}'.format(get_input_path(feat_filename)),
                        'o', '_{}'.format(actual_path)])
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_mark_refer_diff_classes_bug416():
    input_filename = "bug416/font.pfa"
    feat_filename = "bug416/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug416.ttx"
    runner(CMD + ['-o', 'f', '_{}'.format(get_input_path(input_filename)),
                        'ff', '_{}'.format(get_input_path(feat_filename)),
                        'o', '_{}'.format(actual_path)])
    actual_ttx = generate_ttx_dump(actual_path, ['GPOS'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_DFLT_script_with_any_lang_bug438():
    """ The feature file bug438/feat.fea contains languagesystem
    statements for a language other than 'dflt' with the 'DFLT' script
    tag. With the fix, makeotfexe will build an OTF which is identical to
    'bug438.ttx'. Without the fix, it will fail to build an OTF."""
    input_filename = 'bug438/font.pfa'
    feat_filename = 'bug438/feat.fea'
    ttx_filename = 'bug438.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', '_{}'.format(get_input_path(input_filename)),
                        'ff', '_{}'.format(get_input_path(feat_filename)),
                        'o', '_{}'.format(actual_path)])
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])


def test_version_warning_bug610():
    input_filename = 'bug610/font.pfa'
    feat_filename = 'bug610/v0005.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o',
               'f', '_{}'.format(get_input_path(input_filename)),
               'ff', '_{}'.format(get_input_path(feat_filename)),
               'o', '_{}'.format(otf_path)])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert (b"[WARNING] <SourceSans-Test> Major version number not in "
            b"range 1 .. 255") not in output


@pytest.mark.parametrize('feat_name, error_msg', [
    ('test_named_lookup',
        b"[FATAL] <SourceSans-Test> GPOS feature 'last' causes overflow of "
        b"offset to a subtable (0x100010020)"),
    ('test_singlepos_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GPOS feature 'sps1' causes overflow of "
        b"offset to a subtable (0x100010188)"),
    ('test_class_pair_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GPOS feature 'last' causes overflow of "
        b"offset to a subtable (0x100010020)"),
    ('test_class_pair_class_def_overflow',
        b"[FATAL] <SourceSans-Test> ClassDef offset overflow (0x1001a) in "
        b"pair positioning"),
    ('test_contextual_overflow',
        b"[FATAL] <SourceSans-Test> Chain contextual lookup subtable in "
        b"GPOS feature 'krn0' causes offset overflow."),
    ('test_cursive_subtable_overflow',
        b"[FATAL] <SourceSans-Test> Cursive Attach lookup subtable in "
        b"GPOS feature 'curs' causes offset overflow."),
    ('test_mark_to_base_coverage_overflow',
        b"[FATAL] <SourceSans-Test> base coverage offset overflow "
        b"(0x1002c) in MarkToBase positioning"),
    ('test_mark_to_base_subtable_overflow',
        b"[FATAL] <SourceSans-Test> MarkToBase lookup subtable in GPOS "
        b"feature 'mrk1' causes offset overflow."),
    ('test_mark_to_ligature_subtable_overflow',
        b"[FATAL] <SourceSans-Test> ligature coverage offset overflow "
        b"(0x1007a) in MarkToLigature positioning"),
    ('test_singlesub1_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature 'tss2' causes overflow "
        b"of offset to a subtable"),
    ('test_singlesub2_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature 'tss1' causes overflow "
        b"of offset to a subtable"),
    ('test_multiplesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature 'mts1' causes overflow "
        b"of offset to a subtable"),
    ('test_alternatesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature 'ats1' causes overflow "
        b"of offset to a subtable"),
    ('test_ligaturesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature 'lts1' causes overflow "
        b"of offset to a subtable"),
    ('test_chaincontextualsub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> GSUB feature '\xff\xff\xff\xff' causes "
        b"overflow of offset to a subtable"),
    ('test_reversechaincontextualsub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> Reverse Chain contextual lookup "
        b"subtable in GSUB feature 'rts1' causes offset overflow"),
])
def test_oveflow_report_bug313(feat_name, error_msg):
    input_filename = 'bug313/font.pfa'
    feat_filename = 'bug313/{}.fea'.format(feat_name)
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o', 'shw',
               'f', '_{}'.format(get_input_path(input_filename)),
               'ff', '_{}'.format(get_input_path(feat_filename)),
               'o', '_{}'.format(otf_path)])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert error_msg in output
