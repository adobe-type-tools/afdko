from __future__ import print_function, division, absolute_import

import subprocess32 as subprocess
import pytest

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path

TOOL = 'sfntdiff'


# -----
# Tests
# -----

def test_exit_no_option():
    # When ran by itself, 'sfntdiff' prints the usage
    assert subprocess.call([TOOL]) == 1


@pytest.mark.parametrize('arg', ['-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--bogus'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


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
        ['-t', TOOL, '-s', '-f', 'regular.otf', 'bold.otf'] + args)
    expected_path = get_expected_path(txt_filename)
    assert differ([expected_path, actual_path, '-l', '1-4'])
