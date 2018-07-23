# coding: utf-8

from __future__ import print_function, division, absolute_import

import os
import platform
import pytest
from shutil import copy2, copytree
import subprocess32 as subprocess
import tempfile

from fontTools.ttLib import TTFont

from afdko.MakeOTF import (
    checkIfVertInFeature, getOptions, MakeOTFParams)

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'makeotf'
CMD = ['-t', TOOL]

T1PFA_NAME = 't1pfa.pfa'
UFO2_NAME = 'ufo2.ufo'
UFO3_NAME = 'ufo3.ufo'
CID_NAME = 'cidfont.ps'
OTF_NAME = 'SourceSans-Test.otf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    return tempfile.mkstemp()[1]


def _generate_ttx_dump(font_path, tables=None):
    font = TTFont(font_path)
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
    (['r'], T1PFA_NAME, 't1pfa-rel.ttx'),
    (['r'], UFO2_NAME, 'ufo2-rel.ttx'),
    (['r'], UFO3_NAME, 'ufo3-rel.ttx'),
    (['r'], CID_NAME, 'cidfont-rel.ttx'),
])
def test_input_formats(arg, input_filename, ttx_filename):
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
                   '    <modified value='])


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
                   '    <modified value='])
