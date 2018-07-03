# coding: utf-8

from __future__ import print_function, division, absolute_import

import os
import platform
import pytest
from shutil import copy2, copytree
import subprocess32 as subprocess
import tempfile

from fontTools.ttLib import TTFont

from afdko.Tools.SharedData.FDKScripts.MakeOTF import checkIfVertInFeature

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'makeotf'
CMD = ['-t', TOOL]

T1PFA_NAME = 't1pfa.pfa'
UFO2_NAME = 'ufo2.ufo'
CID_NAME = 'cidfont.ps'
OTF_NAME = 'SourceSans-Test.otf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    return tempfile.mkstemp()[1]


def _generate_ttx_dump(font_path):
    font = TTFont(font_path)
    temp_path = _get_temp_file_path()
    font.saveXML(temp_path)
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
    ([], CID_NAME, 'cidfont-dev.ttx'),
    (['r'], T1PFA_NAME, 't1pfa-rel.ttx'),
    (['r'], UFO2_NAME, 'ufo2-rel.ttx'),
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
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


@pytest.mark.parametrize('input_filename', [T1PFA_NAME, UFO2_NAME, CID_NAME])
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


def test_ufo_with_trailing_slash_bug280():
    # makeotf will now save the OTF alongside the UFO instead of inside of it
    ufo_path = _get_input_path(UFO2_NAME)
    temp_dir = tempfile.mkdtemp()
    tmp_ufo_path = os.path.join(temp_dir, UFO2_NAME)
    copytree(ufo_path, tmp_ufo_path)
    runner(CMD + ['-n', '-o', 'f', '_{}{}'.format(tmp_ufo_path, os.sep)])
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [T1PFA_NAME, UFO2_NAME, CID_NAME])
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
