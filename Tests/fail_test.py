from __future__ import print_function, division, absolute_import

import os
import subprocess32 as subprocess

from runner import main as runner

TOOL = 'tx'
CMD = ['-t', TOOL]

def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], 'fail_data',
                        'expected_output', file_name)

def test_fail():
    # Test read CCF2 VF, write snapshot.
    try:
        actual_path = runner(CMD + ['-n', '-o', 'bad_opt'])
    except subprocess.CalledProcessError:
        pass

