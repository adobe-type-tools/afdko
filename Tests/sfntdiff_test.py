from __future__ import print_function, division, absolute_import

import os
import pytest

from runner import main as runner
from differ import main as differ

TOOL = 'sfntdiff'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


# -----
# Tests
# -----

@pytest.mark.parametrize('args, txt_filename', [
    ([], 'dflt.txt'),
    (['T'], 'dflt.txt'),  # default diff with timestamp
    (['d', '_0'], 'dflt.txt'),  # level 0 diff
    (['d', '_1'], 'level1.txt'),
    (['d', '_2'], 'level2.txt'),
    (['d', '_3'], 'level3.txt'),
    (['d', '_4'], 'level4.txt'),
])
def test_diff(args, txt_filename):
    if args:
        args.insert(0, '-o')
    actual_path = runner(
        ['-t', TOOL, '-r', '-f', 'regular.otf', 'bold.otf'] + args)
    expected_path = _get_expected_path(txt_filename)
    assert differ([expected_path, actual_path, '-l', '1-4'])
