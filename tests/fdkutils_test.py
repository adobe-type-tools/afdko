import argparse
import pytest
from os import linesep, path
import sys

from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
    get_resources_dir,
    get_font_format,
    validate_path,
    run_shell_command,
    run_shell_command_logging,
    get_shell_command_output,
    runShellCmd,
    runShellCmdLogging,
)
from test_utils import get_input_path

mac_linux_only = pytest.mark.skipif(
    sys.platform == 'win32', reason="Mac/Linux subprocess results")

win_only = pytest.mark.skipif(
    sys.platform != 'win32', reason="Windows subprocess results")


def test_validate_path_valid():
    assert path.exists(validate_path(get_temp_file_path())) is True


def test_validate_path_invalid():
    with pytest.raises(argparse.ArgumentTypeError):
        validate_path('not_a_file_or_folder')


@pytest.mark.parametrize('path_comp', [None, 'path_component'])
def test_get_temp_dir_path(path_comp):
    pth = get_temp_dir_path(path_comp)
    if path_comp:
        assert path.basename(pth) == path_comp
        assert path.isdir(path.dirname(pth))
    else:
        assert path.isdir(pth)


def test_get_resources_dir():
    assert path.isdir(get_resources_dir())


@pytest.mark.parametrize('filename, fontformat', [
    ('file.txt', None),
    ('font.ufo', 'UFO'),
    ('font.otf', 'OTF'),
    ('font.ttf', 'TTF'),
    ('font.ttc', 'TTC'),
    ('font.cff', 'CFF'),
    ('font.pfa', 'PFA'),
    ('font2.pfa', 'PFA'),
    ('font.pfb', 'PFB'),
    ('font.ps', 'PFC'),
])
def test_get_font_format(filename, fontformat):
    assert get_font_format(get_input_path(filename)) == fontformat


@pytest.mark.parametrize('cmd, rslt', [
    (['type1', '-h'], True),
    (['type1', 'foo'], False),
])
def test_run_shell_command(cmd, rslt):
    assert run_shell_command(cmd) is rslt


@pytest.mark.parametrize('cmd, rslt', [
    (['type1', get_input_path('type1.txt')], True),
    (['type1', get_input_path('type1.txt'), get_temp_file_path()], True),
    (['type1', 'foo'], False),
    ('type1 foo', False),
])
def test_run_shell_command_logging(cmd, rslt):
    assert run_shell_command_logging(cmd) is rslt


@pytest.mark.parametrize('cmd, rslt', [
    (['type1', '-h'], (True, f'usage: type1 [text [font]]{linesep}')),
    (['type1', 'foo'], (False, None)),  # goes thru the exception
])
def test_get_shell_command_output(cmd, rslt):
    assert get_shell_command_output(cmd) == rslt


def test_runShellCmd_timeout():
    assert runShellCmd('type1 -h', timeout=0) == ''


@mac_linux_only
@pytest.mark.parametrize('cmd, shell, str_out', [
    ('type1 -h', True, f'usage: type1 [text [font]]{linesep}'),
    ('type1 -h', False, ''),  # fails on Windows
    (['type1', '-h'], True, ''),  # fails on Windows
    (['type1', '-h'], False, f'usage: type1 [text [font]]{linesep}'),
])
def test_runShellCmd_mac_linux(cmd, shell, str_out):
    assert runShellCmd(cmd, shell=shell) == str_out


@win_only
@pytest.mark.parametrize('cmd, shell', [
    ('type1 -h', True),
    ('type1 -h', False),
    (['type1', '-h'], True),
    (['type1', '-h'], False),
])
def test_runShellCmd_windows(cmd, shell):
    # The shell command call always produces output on Windows,
    # regardless of the value of the shell (True/False) and
    # no matter if the command/args are a list or a string
    assert runShellCmd(cmd, shell=shell) == (
        f'usage: type1 [text [font]]{linesep}')


@pytest.mark.parametrize('cmd, shell, str_out', [
    ('type1 foo', True, ''),
    ('type1 foo', False, ''),
    (['type1', 'foo'], True, ''),
    (['type1', 'foo'], False, ''),
])
def test_runShellCmd_error(cmd, shell, str_out):
    assert runShellCmd(cmd, shell=shell) == str_out


@pytest.mark.parametrize('cmd, rtrn', [
    ('', 1),
    ('type1 foo', 1),
    ([''], 1),
    (['tx', '-h'], 0),
    (['tx', '-z'], 1),
    (['type1', get_input_path('type1.txt'), get_temp_file_path()], 0),
])
def test_runShellCmdLogging_shell_false(cmd, rtrn):
    assert runShellCmdLogging(cmd, False) == rtrn
