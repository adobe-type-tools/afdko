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
