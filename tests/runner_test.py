from __future__ import print_function, division, absolute_import

import platform
import pytest
import subprocess32 as subprocess

from .runner import main as runner
from .runner import _check_tool


def test_bad_tx_cmd():
    # Trigger a fatal error from a command line tool, to make sure
    # it is handled correctly.
    with pytest.raises(subprocess.CalledProcessError):
        runner(['-t', 'tx', '-n', '-o', 'bad_opt'])


@pytest.mark.parametrize('tool_name', ['not_a_tool'])
def test_check_tool_error(tool_name):
    assert isinstance(_check_tool(tool_name), tuple)


@pytest.mark.parametrize('tool_name', ['detype1', 'type1'])
def test_check_tool_unhacked(tool_name):
    expected_name = tool_name
    if platform.system() == 'Windows':
        expected_name += '.exe'
    assert _check_tool(tool_name) == expected_name
