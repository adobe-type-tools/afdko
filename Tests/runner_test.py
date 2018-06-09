from __future__ import print_function, division, absolute_import

import os
import subprocess32 as subprocess

from runner import main as runner


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], 'fail_data',
                        'expected_output', file_name)


def test_bad_tx_cmd():
    # Trigger a fatal error from a command line tool, to make sure
    # is is handled correctly.
    try:
        runner(['-t', 'tx', '-n', '-o', 'bad_opt'])
    except subprocess.CalledProcessError:
        pass
    else:
     raise OSError("test_bad_tx_cmd() should fail, but did not do so.")
