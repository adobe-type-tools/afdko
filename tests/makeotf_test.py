# coding: utf-8

from __future__ import print_function, division, absolute_import

import os
import platform
import pytest
from shutil import copy2, copytree, rmtree
import subprocess32 as subprocess
import sys
import tempfile

from fontTools.ttLib import TTFont

from afdko.makeotf import (
    checkIfVertInFeature, getOptions, MakeOTFParams, getSourceGOADBData,
    readOptionFile, writeOptionsFile, kMOTFOptions, kOptionNotSeen)

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'makeotf'
CMD = ['-t', TOOL]

T1PFA_NAME = 't1pfa.pfa'
UFO2_NAME = 'ufo2.ufo'
UFO3_NAME = 'ufo3.ufo'
CID_NAME = 'cidfont.ps'
TTF_NAME = 'font.ttf'
OTF_NAME = 'SourceSans-Test.otf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')
temp_dir_path = os.path.join(data_dir_path, 'temp_output')


xfail_py36_win = pytest.mark.xfail(
    sys.version_info >= (3, 0) and sys.platform == 'win32',
    reason="Console's encoding is not UTF-8 ?")


def setup_module():
    """
    Create the temporary output directory
    """
    os.mkdir(temp_dir_path)


def teardown_module():
    """
    teardown the temporary output directory
    """
    rmtree(temp_dir_path)


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    # make sure temp data on same file system as other data for rename to work
    file_descriptor, path = tempfile.mkstemp(dir=temp_dir_path)
    os.close(file_descriptor)
    return path


def _generate_ttx_dump(font_path, tables=None):
    with TTFont(font_path) as font:
        temp_path = _get_temp_file_path()
        font.saveXML(temp_path, tables=tables)
        return temp_path


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-v', '-h', '-u'])
def test_exit_known_option(arg):
    if platform.system() == 'Windows':
        tool_name = TOOL + '.exe'
    else:
        tool_name = TOOL
    assert subprocess.check_call([tool_name, arg]) == 0


@pytest.mark.parametrize('arg', ['j', 'bogus'])
def test_exit_unknown_option(arg):
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-n', '-o', arg])
    assert err.value.returncode == 1


@pytest.mark.parametrize('arg, input_filename, ttx_filename', [
    ([], T1PFA_NAME, 't1pfa-dev.ttx'),
    ([], UFO2_NAME, 'ufo2-dev.ttx'),
    ([], UFO3_NAME, 'ufo3-dev.ttx'),
    ([], CID_NAME, 'cidfont-dev.ttx'),
    ([], TTF_NAME, 'ttf-dev.ttx'),
    (['r'], T1PFA_NAME, 't1pfa-rel.ttx'),
    (['r'], UFO2_NAME, 'ufo2-rel.ttx'),
    (['r'], UFO3_NAME, 'ufo3-rel.ttx'),
    (['r'], CID_NAME, 'cidfont-rel.ttx'),
    (['r', 'gf'], TTF_NAME, 'ttf-rel.ttx'),
])
def test_input_formats(arg, input_filename, ttx_filename):
    if 'gf' in arg:
        arg.append('_{}'.format(_get_input_path('GOADB.txt')))
    actual_path = _get_temp_file_path()
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(input_filename)),
                  'o', '_{}'.format(actual_path)] + arg)
    actual_ttx = _generate_ttx_dump(actual_path)
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', '^\s+Version.*;hotconv.*;makeotfexe'])


def test_getSourceGOADBData():
    ttf_path = _get_input_path('font.ttf')
    assert getSourceGOADBData(ttf_path) == [['.notdef', '.notdef', ''],
                                            ['a', 'a', 'uni0041'],
                                            ['g2', 'g2', '']]


@xfail_py36_win
@pytest.mark.parametrize('input_filename', [
    T1PFA_NAME, UFO2_NAME, UFO3_NAME, CID_NAME])
def test_path_with_non_ascii_chars_bug222(input_filename):
    temp_dir = os.path.join(tempfile.mkdtemp(), 'á意ê  ï薨õ 巽ù')
    os.makedirs(temp_dir)
    assert os.path.isdir(temp_dir)
    input_path = _get_input_path(input_filename)
    temp_path = os.path.join(temp_dir, input_filename)
    if os.path.isdir(input_path):
        copytree(input_path, temp_path)
    else:
        copy2(input_path, temp_path)
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.exists(expected_path) is False
    runner(CMD + ['-n', '-o', 'f', '_{}'.format(temp_path)])
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [UFO2_NAME, UFO3_NAME])
def test_ufo_with_trailing_slash_bug280(input_filename):
    # makeotf will now save the OTF alongside the UFO instead of inside of it
    ufo_path = _get_input_path(input_filename)
    temp_dir = tempfile.mkdtemp()
    tmp_ufo_path = os.path.join(temp_dir, input_filename)
    copytree(ufo_path, tmp_ufo_path)
    runner(CMD + ['-n', '-o', 'f', '_{}{}'.format(tmp_ufo_path, os.sep)])
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [
    T1PFA_NAME, UFO2_NAME, UFO3_NAME, CID_NAME])
def test_output_is_folder_only_bug281(input_filename):
    # makeotf will output a default-named font to the folder
    input_path = _get_input_path(input_filename)
    temp_dir = tempfile.mkdtemp()
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.exists(expected_path) is False
    runner(CMD + ['-n', '-o', 'f', '_{}'.format(input_path),
                              'o', '_{}'.format(temp_dir)])
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [
    't1pfa-noPSname.pfa',
    'ufo2-noPSname.ufo',
    'ufo3-noPSname.ufo',
    'cidfont-noPSname.ps',
])
def test_no_postscript_name_bug282(input_filename):
    # makeotf will fail for both UFO and Type 1 inputs
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-n', '-o', 'f', '_{}'.format(input_filename)])
    assert err.value.returncode == 1


@pytest.mark.parametrize('fea_filename, result', [
    (None, 0),  # No feature path at...
    ('missing_file', 0),  # No feature path at...
    ('feat0.fea', 0),
    ('feat1.fea', 1),
    ('feat2.fea', 0),  # Could not find the include file...
    ('feat3.fea', 0),  # Could not find the include file...
    ('feat4.fea', 1),
])
def test_find_vert_feature_bug148(fea_filename, result):
    fea_path = None
    if fea_filename:
        input_dir = _get_input_path('bug148')
        fea_path = os.path.join(input_dir, fea_filename)
    assert checkIfVertInFeature(fea_path) == result


@pytest.mark.parametrize('args, result', [
    # 'result' corresponds to the values of the
    # options 'ReleaseMode' and 'SuppressHintWarnings'
    ([], (None, None)),
    (['-r'], ('true', None)),
    (['-shw'], (None, None)),
    (['-nshw'], (None, 'true')),
    (['-r', '-shw'], ('true', None)),
    (['-r', '-nshw'], ('true', 'true')),
    # makeotf option parsing has no mechanism for mutual exclusivity,
    # so the last option typed on the command line wins
    (['-shw', '-nshw'], (None, 'true')),
    (['-nshw', '-shw'], (None, None)),
])
def test_options_shw_nshw_bug457(args, result):
    params = MakeOTFParams()
    getOptions(params, args)
    assert params.opt_ReleaseMode == result[0]
    assert params.opt_SuppressHintWarnings == result[1]


@pytest.mark.parametrize('args, result', [
    # 'result' corresponds to the values of the
    # options 'MacCmapScriptID' and 'MacCmapLanguageID'
    # CJK MacCmapScriptIDs: Japan/1, CN/2, Korea/3, GB/25
    ([], (None, None)),
    (['-cs', '1'], (1, None)),
    (['-cl', '2'], (None, 2)),
    (['-cs', '4', '-cl', '5'], (4, 5)),
])
def test_options_cs_cl_bug459(args, result):
    params = MakeOTFParams()
    getOptions(params, args)
    assert params.opt_MacCmapScriptID == result[0]
    assert params.opt_MacCmapLanguageID == result[1]


def test_writeOptionsFile():
    actual_path = _get_temp_file_path()
    expected_path = _get_expected_path('proj_write.txt')
    params = MakeOTFParams()
    params.currentDir = os.path.dirname(actual_path)
    params.opt_InputFontPath = 'font.pfa'
    params.opt_OutputFontPath = 'fôñt.otf'
    params.opt_kSetfsSelectionBitsOn = [7]
    params.opt_kSetfsSelectionBitsOff = [9, 8]
    params.opt_ConvertToCID = 'true'
    params.opt_kSetOS2Version = 3
    params.verbose = True
    # set options's sort order
    kMOTFOptions['InputFontPath'][0] = 1
    kMOTFOptions['OutputFontPath'][0] = kOptionNotSeen + 1
    kMOTFOptions['kSetfsSelectionBitsOn'][0] = 30
    writeOptionsFile(params, actual_path)
    assert differ([expected_path, actual_path])


def test_readOptionFile():
    INPUT_FONT_NAME = 'font.pfa'
    OUTPUT_FONT_NAME = 'font.otf'
    proj_path = _get_input_path('proj.txt')
    abs_font_dir_path = os.path.dirname(proj_path)
    params = MakeOTFParams()
    assert params.fontDirPath == '.'

    params.currentDir = os.path.dirname(proj_path)
    assert readOptionFile(proj_path, params, 1) == (True, 7)
    assert params.fontDirPath == '.'
    assert params.opt_InputFontPath == INPUT_FONT_NAME
    assert params.opt_OutputFontPath == OUTPUT_FONT_NAME
    assert params.opt_ConvertToCID == 'true'
    assert params.opt_kSetfsSelectionBitsOff == '[8, 9]'
    assert params.opt_kSetfsSelectionBitsOn == [7]
    assert params.seenOS2v4Bits == [1, 1, 1]
    assert params.opt_UseOldNameID4 is None

    params.currentDir = os.getcwd()
    font_dir_path = os.path.relpath(abs_font_dir_path, params.currentDir)
    input_font_path = os.path.join(font_dir_path, INPUT_FONT_NAME)
    output_font_path = os.path.join(font_dir_path, OUTPUT_FONT_NAME)
    assert readOptionFile(proj_path, params, 1) == (True, 7)
    assert params.fontDirPath == os.path.normpath(font_dir_path)
    assert params.opt_InputFontPath == os.path.normpath(input_font_path)
    assert params.opt_OutputFontPath == os.path.normpath(output_font_path)


def test_readOptionFile_abspath():
    proj_path = _get_input_path('proj2.txt')
    params = MakeOTFParams()

    root = os.path.abspath(os.sep)
    params.currentDir = os.path.join(root, 'different_dir')
    assert readOptionFile(proj_path, params, 1) == (False, 3)
    assert params.fontDirPath.startswith(root)
    assert params.opt_InputFontPath.startswith(root)
    assert params.opt_OutputFontPath.startswith(root)


@pytest.mark.parametrize('cur_dir_str', [
    'different_dir',
    './different_dir',
    '../different_dir',
])
def test_readOptionFile_relpath(cur_dir_str):
    INPUT_FONT_NAME = 'font.pfa'
    OUTPUT_FONT_NAME = 'font.otf'
    proj_path = _get_input_path('proj2.txt')
    abs_font_dir_path = os.path.dirname(proj_path)
    params = MakeOTFParams()

    if '/' in cur_dir_str:
        # flip the slashes used in the test's input string
        params.currentDir = os.path.normpath(cur_dir_str)
    else:
        params.currentDir = cur_dir_str

    font_dir_path = os.path.relpath(abs_font_dir_path, params.currentDir)

    if cur_dir_str.startswith('..') and os.path.dirname(os.getcwd()) == os.sep:
        # the project is inside a folder located at the root level;
        # remove the two dots at the start of the path, otherwise
        # testing input '../different_dir' will fail.
        font_dir_path = font_dir_path[2:]

    input_font_path = os.path.join(font_dir_path, INPUT_FONT_NAME)
    output_font_path = os.path.join(font_dir_path, OUTPUT_FONT_NAME)
    assert readOptionFile(proj_path, params, 1) == (False, 3)
    assert params.fontDirPath == os.path.normpath(font_dir_path)
    assert params.opt_InputFontPath == os.path.normpath(input_font_path)
    assert params.opt_OutputFontPath == os.path.normpath(output_font_path)


def test_readOptionFile_filenotfound():
    proj_path = _get_input_path('notafile')
    params = MakeOTFParams()
    params.currentDir = os.getcwd()
    assert readOptionFile(proj_path, params, 0) == (True, 0)


@pytest.mark.parametrize('args, input_filename, ttx_filename', [
    (['r'], T1PFA_NAME, 't1pfa-cmap.ttx'),
    (['r', 'cs', '_1'], T1PFA_NAME, 't1pfa-cmap_cs1.ttx'),
    (['r', 'cl', '_2'], T1PFA_NAME, 't1pfa-cmap_cl2.ttx'),
    (['r'], UFO2_NAME, 'ufo2-cmap.ttx'),
    (['r', 'cs', '_4'], UFO2_NAME, 'ufo2-cmap_cs4.ttx'),
    (['r', 'cl', '_5'], UFO2_NAME, 'ufo2-cmap_cl5.ttx'),
    (['r'], UFO3_NAME, 'ufo3-cmap.ttx'),
    (['r', 'cs', '_4'], UFO3_NAME, 'ufo3-cmap_cs4.ttx'),
    (['r', 'cl', '_5'], UFO3_NAME, 'ufo3-cmap_cl5.ttx'),
    (['r'], CID_NAME, 'cidfont-cmap.ttx'),
    (['r', 'cs', '_2'], CID_NAME, 'cidfont-cmap_cs2.ttx'),
    (['r', 'cl', '_3'], CID_NAME, 'cidfont-cmap_cl3.ttx'),
])
def test_build_options_cs_cl_bug459(args, input_filename, ttx_filename):
    actual_path = _get_temp_file_path()
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(input_filename)),
                  'o', '_{}'.format(actual_path)] + args)
    actual_ttx = _generate_ttx_dump(actual_path, ['cmap'])
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_bug416():
    input_filename = "bug416/bug416.pfa"
    input_featurename = "bug416/bug416.fea"
    actual_path = _get_temp_file_path()
    ttx_filename = "bug416.ttx"
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(input_filename)),
                  'ff', '_{}'.format(_get_input_path(input_featurename)),
                  'o', '_{}'.format(actual_path)])
    actual_ttx = _generate_ttx_dump(actual_path, ['GPOS'])
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_bug438():
    """ The feature file bug438/feat.fea contains languagesystem
    statements for a language other than 'dflt' with the 'DFLT' script
    tag. With the fix, makeotf will build an otf which is identical to
    't1pfa-dev.ttx'. Without the fix, it will fail to build an OTF."""
    input_filename = T1PFA_NAME
    feat_filename = 'bug438/feat.fea'
    ttx_filename = 't1pfa-dev.ttx'
    actual_path = _get_temp_file_path()
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(input_filename)),
                  'ff', '_{}'.format(_get_input_path(feat_filename)),
                  'o', '_{}'.format(actual_path)])
    actual_ttx = _generate_ttx_dump(actual_path)
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', '^\s+Version.*;hotconv.*;makeotfexe'])


@pytest.mark.parametrize('args, font, fontinfo', [
    (['cn'], 'type1', ''),
    (['cn'], 'no_notdef', ''),
    (['cn'], 'blank_notdef', ''),
    (['cn'], 'notdef_not1st', ''),
    (['cn', 'fi'], 'type1', 'fi'),
    (['cn', 'fi'], 'type1', 'fi2'),
    (['cn', 'fi'], 'no_notdef', 'fi'),
    (['cn', 'fi'], 'no_notdef', 'fi2'),
    (['cn', 'fi'], 'blank_notdef', 'fi'),
    (['cn', 'fi'], 'blank_notdef', 'fi2'),
    (['cn', 'fi'], 'notdef_not1st', 'fi'),
    (['cn', 'fi'], 'notdef_not1st', 'fi2'),
])
def test_cid_keyed_cff_bug470(args, font, fontinfo):
    if 'fi' in args:
        fontinfo_file = 'bug470/{}.txt'.format(fontinfo)
        args.append('_{}'.format(_get_input_path(fontinfo_file)))
        ttx_file = 'bug470/{}-{}.ttx'.format(font, 'fi')
    else:
        ttx_file = 'bug470/{}.ttx'.format(font)
    font_file = 'bug470/{}.pfa'.format(font)
    actual_path = _get_temp_file_path()
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(font_file)),
                  'o', '_{}'.format(actual_path)] + args)
    actual_ttx = _generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = _get_expected_path(ttx_file)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


@pytest.mark.parametrize('opts', [
    [],
    ['r'],
    ['ga'],
    ['r', 'ga'],
    ['nga'],
    ['r', 'nga'],
    ['gf'],
    ['r', 'gf'],
    ['gf', 'ga'],
    ['gf', 'ga', 'nga'],  # GOADB will be applied, because 'ga' is first
    ['gf', 'nga'],
    ['gf', 'nga', 'ga'],  # GOADB not applied, because 'nga' is first
    ['r', 'ga', 'gf'],
    ['bit7n', 'bit8n', 'bit9n'],
    ['bit7y', 'bit8y', 'bit9y'],
    ['bit7y', 'bit8n', 'bit9n'],
    ['bit7y', 'bit8n', 'bit7n', 'bit8y'],
])
def test_bug497(opts):
    font_path = _get_input_path(T1PFA_NAME)
    feat_path = _get_input_path('bug497/feat.fea')
    fmndb_path = _get_input_path('bug497/fmndb.txt')
    goadb_path = _get_input_path('bug497/goadb.txt')
    actual_path = _get_temp_file_path()
    ttx_filename = '-'.join(['opts'] + opts) + '.ttx'

    args = []
    for opt in opts:  # order of the opts is important
        if opt == 'r':
            args.append(opt)
        elif opt == 'ga':
            args.append(opt)
        elif opt == 'nga':
            args.append(opt)
        elif opt == 'gf':
            args.extend([opt, '_{}'.format(goadb_path)])
        elif 'bit' in opt:
            bit_num = int(opt[3])
            bit_bol = 'osbOn' if opt[4] == 'y' else 'osbOff'
            args.extend([bit_bol, '_{}'.format(bit_num)])

    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(font_path),
                  'o', '_{}'.format(actual_path),
                  'ff', '_{}'.format(feat_path),
                  'mf', '_{}'.format(fmndb_path)] + args)
    actual_ttx = _generate_ttx_dump(actual_path)
    expected_ttx = _get_expected_path('bug497/{}'.format(ttx_filename))
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', '^\s+Version.*;hotconv.*;makeotfexe'])


def test_useMarkFilteringSet_flag_bug196():
    input_filename = "bug196/bug196.pfa"
    input_featurename = "bug196/bug196.fea"
    actual_path = _get_temp_file_path()
    ttx_filename = "bug196.ttx"
    runner(CMD + ['-n', '-o',
                  'f', '_{}'.format(_get_input_path(input_filename)),
                  'ff', '_{}'.format(_get_input_path(input_featurename)),
                  'o', '_{}'.format(actual_path)])
    actual_ttx = _generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
