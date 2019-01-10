from __future__ import print_function, division, absolute_import

import pytest
import subprocess32 as subprocess

from runner import main as runner, _check_tool


def test_bad_tx_cmd():
    # Trigger a fatal error from a command line tool, to make sure
    # it is handled correctly.
    with pytest.raises(subprocess.CalledProcessError):
        runner(['-t', 'tx', '-o', 'bad_opt'])


@pytest.mark.parametrize('tool_name', ['not_a_tool'])
def test_check_tool_error(tool_name):
    assert isinstance(_check_tool(tool_name), tuple)


@pytest.mark.parametrize('tool_name', [
    'detype1', 'type1', 'sfntedit', 'sfntdiff', 'makeotfexe'])
def test_check_tool_unhacked(tool_name):
    assert _check_tool(tool_name) == tool_name


def test_capture_error_message():
    output_path = runner(['-t', 'makeotfexe', '-s', '-e'])
    with open(output_path, 'rb') as f:
        output = f.read()
    assert b"[FATAL] Source font file not found: font.ps" in output
