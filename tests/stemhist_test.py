import pytest
import subprocess

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path, get_temp_file_path

TOOL = 'stemhist'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-b', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 2


@pytest.mark.parametrize('arg', (['z'], ['a']))
@pytest.mark.parametrize('font_filename', [
    'font.pfa', 'font.ufo', 'font.cff', 'font.otf', 'cidfont.ps'])
def test_stems_and_zones(arg, font_filename):
    prefix, ext = font_filename.split('.')

    # some gymnastics to workaround different glyph orders
    # and hence different sorted glyph name output in the test set.
    if ext in ("cff", "otf"):
        ext = ".otf"
    else:
        ext = ""

    if 'z' in arg:
        suffixes = ['.top.txt', '.bot.txt']
    else:
        suffixes = ['.hstm.txt', '.vstm.txt']

    report_path = get_temp_file_path()
    runner(CMD + ['-f', font_filename,
                  '-o', 'o', f'_{report_path}'] + arg)
    for suffix in suffixes:
        actual_path = f'{report_path}{suffix}'
        exp_suffix = suffix
        if 'a' in arg:
            exp_suffix = '.all' + suffix
        expected_path = get_expected_path(f'{prefix}{ext}{exp_suffix}')
        assert differ([expected_path, actual_path, '-l', '1'])


def test_cross_environment_results_bug210():
    filename = 'bug210'
    report_path = get_temp_file_path()
    actual_top_path = f'{report_path}.top.txt'
    actual_bot_path = f'{report_path}.bot.txt'
    expect_top_path = get_expected_path(f'{filename}.top.txt')
    expect_bot_path = get_expected_path(f'{filename}.bot.txt')

    runner(CMD + ['-f', f'{filename}.ufo',
                  '-o', 'o', f'_{report_path}', 'z', 'g', '_a-z,A-Z,zero-nine',
                  ])

    assert differ([expect_top_path, actual_top_path, '-l', '1'])
    assert differ([expect_bot_path, actual_bot_path, '-l', '1'])


def test_start_point_not_oncurve_bug715():
    filename = 'bug715'
    report_path = get_temp_file_path()
    actual_hstm_path = f'{report_path}.hstm.txt'
    actual_vstm_path = f'{report_path}.vstm.txt'
    expect_hstm_path = get_expected_path(f'{filename}.hstm.txt')
    expect_vstm_path = get_expected_path(f'{filename}.vstm.txt')

    runner(CMD + ['-f', f'{filename}.ufo',
                  '-o', 'o', f'_{report_path}', 'a'])

    assert differ([expect_hstm_path, actual_hstm_path, '-l', '1'])
    assert differ([expect_vstm_path, actual_vstm_path, '-l', '1'])
