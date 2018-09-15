from __future__ import print_function, division, absolute_import

import os
import pytest
import subprocess32 as subprocess

from .runner import main as runner
from .differ import main as differ

TOOL = 'type1'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-v', '-u'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


def test_run_on_txt_data():
    actual_path = runner(['-t', TOOL, '-s', '-f', 'type1.txt'])
    expected_path = _get_expected_path('type1.pfa')
    assert differ([expected_path, actual_path])
