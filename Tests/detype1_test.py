from __future__ import print_function, division, absolute_import

import os

from runner import main as runner
from differ import main as differ

TOOL = 'detype1'
CMD = ['-t', TOOL]

TYPE1_PFA = 'type1.pfa'
TYPE1_TXT = 'type1.txt'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_run_on_pfa_data():
    actual_path = runner(CMD + ['-f', TYPE1_PFA])
    expected_path = _get_expected_path('type1_from_pfa.txt')
    assert differ([expected_path, actual_path]) is True


def test_run_on_txt_data():
    # type1.txt: EOF in eexec section
    # command 'detype1 type1.txt' returned non-zero exit status 1
    assert runner(CMD + ['-f', TYPE1_TXT]) is None
