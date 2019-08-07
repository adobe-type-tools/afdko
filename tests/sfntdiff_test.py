import subprocess
import pytest

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path

TOOL = 'sfntdiff'
CMD = ['-t', TOOL]


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
    actual_path = runner(CMD + ['-s', '-f', 'regular.otf', 'bold.otf'] + args)
    expected_path = get_expected_path(txt_filename)
    assert differ([expected_path, actual_path, '-l', '1-4'])


def test_diff_otf_vs_ttf_bug626():
    actual_path = runner(CMD + ['-s', '-f', 'SourceSerifPro-It.otf',
                                            'SourceSerifPro-It.ttf'])
    expected_path = get_expected_path('bug626.txt')
    assert differ([expected_path, actual_path, '-l', '1-4'])


def test_not_font_files():
    stderr_path = runner(CMD + ['-s', '-e', '-f', 'not_a_font_1.otf',
                                                  'not_a_font_2.otf'])
    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert b'[WARNING]: unsupported/bad file' in output
    assert b'not_a_font_1.otf] (ignored)' in output
    assert b'not_a_font_2.otf] (ignored)' in output
