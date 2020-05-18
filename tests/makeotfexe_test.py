import glob
import os
import pytest
import subprocess

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump, generate_spot_dumptables)

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


@pytest.mark.parametrize('args, ttx_fname', [
    ([], 'font_dev'),
    (['r'], 'font_rel'),
])
def test_build_font_and_check_messages(args, ttx_fname):
    actual_path = get_temp_file_path()
    expected_msg_path = get_expected_path(f'{ttx_fname}_output.txt')
    ttx_filename = f'{ttx_fname}.ttx'
    stderr_path = runner(CMD + [
        '-s', '-e', '-o', 'f', f'_{get_input_path("font.pfa")}',
                          'o', f'_{actual_path}'] + args)
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
    assert differ([expected_msg_path, stderr_path])


@pytest.mark.parametrize('caret_format', [
    'bypos', 'byindex', 'mixed', 'mixed2', 'double', 'double2'])
def test_GDEF_LigatureCaret_bug155(caret_format):
    input_filename = 'bug155/font.pfa'
    feat_filename = f'bug155/caret-{caret_format}.fea'
    ttx_filename = f'bug155/caret-{caret_format}.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GDEF'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-l', '2'])


def test_useMarkFilteringSet_flag_bug196():
    input_filename = "bug196/font.pfa"
    feat_filename = "bug196/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug196.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_useException_bug321():
    input_filename = "bug321/font.pfa"
    feat_filename = "bug321/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug321.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB', 'GPOS'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_mark_refer_diff_classes_bug416():
    input_filename = "bug416/font.pfa"
    feat_filename = "bug416/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug416.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
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
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
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


def test_glyph_not_in_font_bug492():
    input_filename = 'bug492/font.pfa'
    feat_filename = 'bug492/features.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert (b'[ERROR] <SourceSans-Test> Glyph "glyph_not_found" not in font. '
            b'[') in output
    assert (b'syntax error at "a" [') not in output


def test_version_warning_bug610():
    input_filename = 'bug610/font.pfa'
    feat_filename = 'bug610/v0005.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

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
    ('test_duplicate_single_sub',
        b"[NOTE] <SourceSans-Test> Removing duplicate single substitution "
        b"in standalone lookup 'test2': glyph1, glyph1"),
    ('test_conflicting_single_sub',
        b"[FATAL] <SourceSans-Test> Duplicate target glyph for single "
        b"substitution in standalone lookup 'test2': glyph1"),
    ('test_duplicate_alternate_sub',
        b"[FATAL] <SourceSans-Test> Duplicate target glyph for alternate "
        b"substitution in standalone lookup 'test2': glyph1"),
    ('test_duplicate_multiple_sub',
        b"[FATAL] <SourceSans-Test> Duplicate target glyph for multiple "
        b"substitution in standalone lookup 'test2': glyph1"),
    ('test_duplicate_ligature_sub',
        b"[NOTE] <SourceSans-Test> Removing duplicate ligature substitution "
        b"in standalone lookup 'test2'"),
    ('test_conflicting_ligature_sub',
        b"[FATAL] <SourceSans-Test> Duplicate target sequence but different "
        b"replacement glyphs in ligature substitutions in standalone "
        b"lookup 'test2'"),
    ('test_duplicate_single_pos',
        b"[NOTE] <SourceSans-Test> Removing duplicate single positioning "
        b"in standalone lookup 'test2'"),
    ('test_conflicting_single_pos',
        b"[FATAL] <SourceSans-Test> Duplicate single positioning glyph "
        b"with different values in standalone lookup 'test2'"),
    ('test_features_name_missing_win_dflt_sub',
        b"[FATAL] <SourceSans-Test> Missing Windows default name for "
        b"'featureNames' nameid 256 in feature 'ss01'"),
    ('test_cv_params_not_in_cvxx_sub',
        b"[FATAL] <SourceSans-Test> A 'cvParameters' block is only allowed "
        b"in Character Variant (cvXX) features; it is being used in "
        b"feature 'tst1'"),
    ('test_cv_params_missing_win_dflt_sub',
        b"[FATAL] <SourceSans-Test> Missing Windows default name for "
        b"'cvParameters' nameid 256 in feature 'cv01'"),
    ('test_size_withfamilyID_0_pos',
        b"[FATAL] <SourceSans-Test> 'size' feature must have 4 parameters if "
        b"sub family ID code is non-zero!"),
    ('test_size_withfamilyID_3_pos',
        b"[FATAL] <SourceSans-Test> 'size' feature must have 4 or 2 "
        b"parameters if sub family code is zero!"),
    ('test_sizemenuname_missing_win_dflt_pos',
        b"[FATAL] <SourceSans-Test> Missing Windows default name for "
        b"'sizemenuname' nameid 256 in 'size' feature"),
    ('test_kernpair_warnings_pos',
        b"[WARNING] <SourceSans-Test> Single kern pair occurring after class "
        b"kern pair in feature 'tst1'"),
    ('test_kernpair_warnings_pos',
        b"[WARNING] <SourceSans-Test> Start of new pair positioning subtable "
        b"forced by overlapping glyph classes in feature 'tst1'; some pairs "
        b"may never be accessed"),
    ('test_kernpair_warnings_pos',
        b"[NOTE] <SourceSans-Test> Removing duplicate pair positioning in "
        b"feature 'tst1': glyph3 glyph3"),
    ('test_kernpair_warnings_pos',
        b"[WARNING] <SourceSans-Test> Pair positioning has conflicting "
        b"statements in feature 'tst1'"),
    ('test_base_glyph_conflict_pos',
        b"[ERROR] <SourceSans-Test> MarkToBase or MarkToMark error in "
        b"feature 'tst1'. Another statement has already defined the anchors "
        b"and marks on glyph 'glyph5'."),
    ('test_base_glyph_conflict_pos',
        b"[WARNING] <SourceSans-Test> MarkToBase or MarkToMark error in "
        b"feature 'tst1'. Glyph 'glyph5' does not have an anchor point "
        b"for a mark class that was used in a previous statement in the "
        b"same lookup table. Setting the anchor point offset to 0."),
    ('test_mark_ligature_conflict_pos',
        b"[ERROR] <SourceSans-Test> MarkToLigature error in feature 'tst1'. "
        b"Two different statements referencing the ligature glyph 'glyph6' "
        b"have assigned the same mark class to the same ligature component."),
    ('test_mark_ligature_conflict_pos',
        b"[ERROR] <SourceSans-Test> MarkToLigature statement error in feature "
        b"'tst1'. Glyph 'glyph6' contains a duplicate mark class assignment "
        b"for one of the ligature components."),
    ('test_mark_class_glyph_conflict',
        b"[ERROR] <SourceSans-Test> In feature 'tst1', glyph 'glyph2' "
        b"is repeated in the current class definition. Mark class: "
        b"@TOP_MARKS."),
    ('test_mark_class_glyph_conflict',
        b"[ERROR] <SourceSans-Test> In feature 'tst1', glyph 'glyph1' "
        b"occurs in two different mark classes. Previous mark class: "
        b"@TOP_MARKS. Current mark class: @BOTTOM_MARKS."),
    ('test_base_anchor_errors_pos',
        b"[ERROR] <SourceSans-Test> MarkToBase or MarkToMark error in "
        b"feature 'tst1'. Another statement has already assigned the current "
        b"mark class to another anchor point on glyph 'glyph7'"),
    ('test_base_anchor_errors_pos',
        b"[WARNING] <SourceSans-Test> MarkToBase or MarkToMark error in "
        b"feature 'tst1'. Glyph 'glyph5' does not have an anchor point "
        b"for a mark class that was used in a previous statement in the "
        b"same lookup table"),
    ('test_cursive_duplicate_glyph_pos',
        b"[ERROR] <SourceSans-Test> Cursive statement error in feature "
        b"'tst1'. A previous statement has already referenced glyph "
        b"'glyph1'."),
])
def test_overflow_report_bug313(feat_name, error_msg):
    input_filename = 'bug313/font.pfa'
    feat_filename = f'bug313/{feat_name}.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o', 'shw',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert error_msg in output


def test_feature_recursion_bug628():
    input_filename = 'bug628/font.pfa'
    feat_filename = 'bug628/feat.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o', 'shw',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert(b"[FATAL] <SourceSans-Test> Can't include [feat.fea]; maximum "
           b"include levels <50> reached") in output


@pytest.mark.parametrize('arg', [[], ['gs']])
def test_recalculate_bbox_bug617(arg):
    input_filename = "bug617/font.pfa"
    goadb_filename = "bug617/goadb.txt"
    ttx_filename = f"bug617/{'no_' if not arg else ''}gs_opt.ttx"
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'gf', f'_{get_input_path(goadb_filename)}',
                        'o', f'_{actual_path}', 'r'] + arg)
    actual_ttx = generate_ttx_dump(actual_path, ['head', 'CFF '])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


def test_contextual_multiple_substitutions_bug725():
    input_filename = "bug725/font.pfa"
    feat_filename = "bug725/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug725.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_notdef_in_glyph_class_bug726():
    input_filename = "bug726/font.pfa"
    feat_filename = "bug726/feat.fea"
    actual_path = get_temp_file_path()
    ttx_filename = "bug726.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GDEF'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_overflow_bug731():
    input_filename = 'bug731/font.pfa'
    feat_filename = 'bug731/feat.fea'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o', 'shw',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert(b"[FATAL] <SourceSans-Test> subtable offset too large (1003c) in "
           b"lookup 0 type 3") in output


def test_base_anchor_bug811():
    input_filename = 'bug811/font.pfa'
    feat_filename = 'bug811/feat.fea'
    ttx_filename = 'bug811.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GPOS'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_max_revision_bug876():
    input_filename = 'bug876/font.pfa'
    feat_filename = 'bug876/feat.fea'
    ttx_filename = 'bug876.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['head'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


def test_infinite_loop_with_dflt_lookups_bug965():
    input_filename = 'bug965/font.pfa'
    feat_filename = 'bug965/feat.fea'
    ttx_filename = 'bug965.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


def test_bug993():
    input_filename = 'fealib/font.pfa'
    feat_filename = 'bug993/feat.fea'
    ttx_filename = 'bug993.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GPOS'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


def test_bug1006():
    input_filename = 'fealib/font.pfa'
    feat_filename = 'bug1006/feat.fea'
    ttx_filename = 'bug1006.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'ff', f'_{get_input_path(feat_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['GPOS'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


@pytest.mark.parametrize(
    'cmap_filename, cs, expectedOS2',
    [('UniCN-UTF32-H', '25', 'expected_OS2_SC.ttx'),
     ('UniJP-UTF32-H', '1', 'expected_OS2_JP.ttx'),
     ('UniKR-UTF32-H', '3', 'expected_OS2_KR.ttx'),
     ('UniTW-UTF32-H', '2', 'expected_OS2_TC.ttx')])
def test_bug1040(cmap_filename, cs, expectedOS2):
    out_path = get_temp_file_path()
    cmd = CMD + ['-o', 'f', f'_{get_input_path("bug1040/cidfont.ps")}',
                       'cs', f'_{cs}',
                       'o', f'_{out_path}',
                       'ch', f'_{get_input_path("bug1040/" + cmap_filename)}']
    runner(cmd)

    actual_ttx = generate_ttx_dump(out_path, ["OS/2"])
    expected_ttx = get_expected_path("bug1040/" + expectedOS2)

    assert differ([expected_ttx, actual_ttx, '-l', '2'])


def test_parameter_offset_overflow_bug1017():
    # Since the fix for issue 1017, it is not practical to actually cause
    # this overflow, which used to be tested as bug746.  What we do
    # instead is to verify that the FeatParams blocks are written right
    # after the FeatureList, and before the LookupList, by comparing with
    # the expected low level spot GSUB dump.
    input_filename = 'bug1017/font.pfa'
    feat_filename = 'bug1017/feat.fea'
    txt_filename = 'bug1017.GSUB.txt'
    otf_path = get_temp_file_path()

    runner(
        CMD + ['-s', '-e', '-o', 'shw',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    actual_txt = generate_spot_dumptables(otf_path, ['GSUB'])
    expected_txt = get_expected_path(txt_filename)
    assert differ([expected_txt, actual_txt])


TEST_FEATURE_FILES = [
    "Attach", "enum", "markClass", "language_required", "GlyphClassDef",
    "LigatureCaretByIndex", "LigatureCaretByPos", "lookup", "lookupflag",
    "feature_aalt", "ignore_pos", "GPOS_1", "GPOS_1_zero", "GPOS_2", "GPOS_2b",
    "GPOS_3", "GPOS_4", "GPOS_5", "GPOS_6", "GPOS_8", "GSUB_2", "GSUB_3",
    "GSUB_6", "GSUB_8", "spec4h1", "spec4h2", "spec5d1", "spec5d2", "spec5fi1",
    "spec5fi2", "spec5fi3", "spec5fi4", "spec5f_ii_1", "spec5f_ii_2",
    "spec5f_ii_3", "spec5f_ii_4", "spec5h1", "spec6b_ii", "spec6d2", "spec6e",
    "spec6f", "spec6h_ii", "spec6h_iii_1", "spec6h_iii_3d", "spec8a", "spec8b",
    "spec8c", "spec8d", "spec9a", "spec9b", "spec9c1", "spec9c2", "spec9c3",
    "spec9d", "spec9e", "spec9f", "spec9g", "spec10", "bug453", "bug457",
    "bug463", "bug501", "bug502", "bug504", "bug505", "bug506", "bug509",
    "bug512", "bug514", "bug568", "bug633", "bug1307", "bug1459", "name",
    "size", "size2", "multiple_feature_blocks", "omitted_GlyphClassDef",
    "ZeroValue_SinglePos_horizontal", "ZeroValue_SinglePos_vertical",
    "ZeroValue_PairPos_horizontal", "ZeroValue_PairPos_vertical",
    "ZeroValue_ChainSinglePos_horizontal", "ZeroValue_ChainSinglePos_vertical",
    "PairPosSubtable", "MultipleLookupsPerGlyph", "MultipleLookupsPerGlyph2"
]

TEST_FEATURE_FILES_XFAIL = [
    "GPOS_1",            # <device>
    "GSUB_6",            # Contextual alternate rule not yet supported
    "bug1459",           # <NULL>
    "language_required",  # required
    "PairPosSubtable",   # https://github.com/adobe-type-tools/afdko/issues/971
    "bug1307",           # https://github.com/adobe-type-tools/afdko/issues/966
]

TEST_FEATURE_FILES_TABLES = {
    "name": ["name"],
    "spec8b": ["name", "GPOS"],
    "spec8c": ["name", "GSUB"],
    "spec8d": ["name", "GSUB"],
    "spec9c1": ["head"],
    "spec9c2": ["head"],
    "spec9c3": ["head"],
    "spec9d": ["hhea"],
    "spec9e": ["name"],
    "spec9f": ["OS/2"],
    "spec9g": ["vhea"],
}

TEST_TABLES = ['BASE', 'GDEF', 'GSUB', 'GPOS']


@pytest.mark.parametrize('name', TEST_FEATURE_FILES)
def test_feature_file(name):
    try:
        input_filename = "fealib/font.pfa"
        feat_filename = f"fealib/{name}.fea"
        ttx_filename = f"fealib/{name}.ttx"
        actual_path = get_temp_file_path()

        runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                            'ff', f'_{get_input_path(feat_filename)}',
                            'o', f'_{actual_path}'])

        tables = TEST_FEATURE_FILES_TABLES.get(name, TEST_TABLES)
        actual_ttx = generate_ttx_dump(actual_path, tables)
        expected_ttx = get_expected_path(ttx_filename)
        assert differ([expected_ttx, actual_ttx, '-l', '2',
                       '-s',
                       '    <checkSumAdjustment value=' + SPLIT_MARKER +
                       '    <checkSumAdjustment value=' + SPLIT_MARKER +
                       '    <created value=' + SPLIT_MARKER +
                       '    <modified value=',
                       '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])
    except Exception:
        if name in TEST_FEATURE_FILES_XFAIL:
            pytest.xfail()
        raise


@pytest.mark.parametrize('path', glob.glob(get_input_path("spec/*.fea")))
def test_spec(path):
    name = os.path.splitext(os.path.basename(path))[0]
    if ".cid" in name:
        input_filename = "spec/fontcid.ps"
    else:
        input_filename = "spec/font.pfa"
    feat_filename = f"spec/{name}.fea"
    ttx_filename = f"spec/{name}.ttx"
    cmap_filename = "spec/Identity-H"
    actual_path = get_temp_file_path()

    cmd = CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                       'ff', f'_{get_input_path(feat_filename)}',
                       'o', f'_{actual_path}',
                       'ch', f'_{get_input_path(cmap_filename)}']

    if name.endswith(".bad"):
        with pytest.raises(subprocess.CalledProcessError):
            runner(cmd)
        return
    runner(cmd)

    tables = TEST_TABLES
    with open(get_input_path(feat_filename)) as fp:
        line = fp.readline()
        if line.startswith("#") and "TABLES" in line:
            tables = line.split("# TABLES:")[1].strip().split(",")

    actual_ttx = generate_ttx_dump(actual_path, tables)
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-l', '2',
                   '-s',
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])
