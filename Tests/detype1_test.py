from __future__ import print_function, division, absolute_import

import os

from runner import main as runner
from differ import main as differ

TOOL = 'detype1'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


# -----
# Tests
# -----

def test_run_on_pfa_data():
    actual_path = runner(['-t', TOOL, '-f', 'type1.pfa'])
    expected_path = _get_expected_path('type1.txt')
    assert differ([expected_path, actual_path])
