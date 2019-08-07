import argparse
import os
import pytest
import subprocess

from runner import (
    main as runner,
    get_options,
    _check_tool,
    _check_save_path,
)

TESTS_DIR = os.path.split(__file__)[0]


def test_bad_tx_cmd():
    # Trigger a fatal error from a command line tool, to make sure
    # it is handled correctly.
    with pytest.raises(subprocess.CalledProcessError):
        runner(['-t', 'tx', '-o', 'bad_opt'])


def test_check_tool_error():
    assert isinstance(_check_tool('not_a_tool'), tuple)


def test_check_tool_error_cli():
    with pytest.raises(SystemExit) as exc_info:
        runner(['-t', 'not_a_tool'])
    assert exc_info.value.code == 2


@pytest.mark.parametrize('tool_name', [
    'detype1', 'type1', 'sfntedit', 'sfntdiff', 'makeotfexe'])
def test_check_tool_unhacked(tool_name):
    assert _check_tool(tool_name) == tool_name


def test_capture_error_message():
    output_path = runner(['-t', 'makeotfexe', '-s', '-e'])
    with open(output_path, 'rb') as f:
        output = f.read()
    assert b"[FATAL] Source font file not found: font.ps" in output


@pytest.mark.parametrize('v_arg', ['', 'v', 'vv', 'vvv'])
def test_log_level(v_arg):
    v_val = len(v_arg)
    arg = [] if not v_val else [f'-{v_arg}']
    opts = get_options(['-t', 'tx'] + arg)
    assert opts.verbose == v_val


def test_check_save_path__path_does_not_exist():
    tmp_fpath = os.path.join(TESTS_DIR, 'foobar')
    abs_tmp_fpath = os.path.abspath(os.path.realpath(tmp_fpath))
    assert os.path.exists(tmp_fpath) is False
    assert _check_save_path(tmp_fpath) == abs_tmp_fpath
    assert os.path.exists(tmp_fpath) is False


def test_check_save_path__path_exists():
    tmp_fpath = os.path.join(TESTS_DIR, 'runner_data', 'input', 'a_file')
    abs_tmp_fpath = os.path.abspath(os.path.realpath(tmp_fpath))
    assert os.path.exists(tmp_fpath) is True
    assert _check_save_path(tmp_fpath) == abs_tmp_fpath
    assert os.path.exists(tmp_fpath) is True


def test_check_save_path__path_is_folder():
    tmp_fpath = os.path.join(TESTS_DIR, 'runner_data', 'input')
    assert os.path.exists(tmp_fpath) is True
    with pytest.raises(argparse.ArgumentTypeError):
        _check_save_path(tmp_fpath)
