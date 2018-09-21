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
        b"[FATAL] <SourceSans-Test> In feature 'last' positioning rules "
        b"cause an offset overflow (0x10020) to a lookup subtable"),
    ('test_singlepos_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'sps1' lookup 'lkup40' "
        b"positioning rules cause an offset overflow (0x10188) to a "
        b"lookup subtable"),
    ('test_class_pair_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'last' positioning rules "
        b"cause an offset overflow (0x10020) to a lookup subtable"),
    ('test_class_pair_class_def_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'krn1' lookup 'l1' pair "
        b"positioning rules cause an offset overflow (0x1001a) to a "
        b"class 2 definition table"),
    ('test_contextual_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'krn0' lookup 'lkup40' "
        b"chain contextual positioning rules cause an offset overflow "
        b"(0x10002) to a lookup subtable"),
    ('test_cursive_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'curs' lookup 'lk20' "
        b"cursive positioning rules cause an offset overflow "
        b"(0x1006e) to a cursive attach table"),
    ('test_mark_to_base_coverage_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'mrk1' mark to base "
        b"positioning rules cause an offset overflow (0x1002c) to a "
        b"base coverage table"),
    ('test_mark_to_base_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'mrk1' mark to base "
        b"positioning rules cause an offset overflow (0x10230) to a "
        b"lookup subtable"),
    ('test_mark_to_ligature_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'lig1' lookup 'lk0' mark "
        b"to ligature positioning rules cause an offset overflow (0x1053e) "
        b"to a lookup subtable"),
    ('test_singlesub1_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'tss2' lookup 'lkup258' "
        b"substitution rules cause an offset overflow (0x10002) to a "
        b"lookup subtable"),
    ('test_singlesub2_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'tss1' lookup 'lkup237' "
        b"substitution rules cause an offset overflow (0x10098) to a "
        b"lookup subtable"),
    ('test_multiplesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'mts1' lookup 'lkup238' "
        b"substitution rules cause an offset overflow (0x10056) to a "
        b"lookup subtable"),
    ('test_alternatesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'ats1' lookup 'lkup238' "
        b"substitution rules cause an offset overflow (0x1009c) to a "
        b"lookup subtable"),
    ('test_ligaturesub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'lts1' lookup 'lkup238' "
        b"substitution rules cause an offset overflow (0x10016) to a "
        b"lookup subtable"),
    ('test_chaincontextualsub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'cts1' substitution rules "
        b"cause an offset overflow (0x100ac) to a lookup subtable"),
    ('test_reversechaincontextualsub_subtable_overflow',
        b"[FATAL] <SourceSans-Test> In feature 'rts1' lookup 'lkup238' "
        b"reverse chain contextual substitution rules cause an offset "
        b"overflow (0x100a0) to a lookup subtable"),
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
