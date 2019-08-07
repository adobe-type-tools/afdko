import pytest
import subprocess

from runner import main as runner
from differ import main as differ
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ps_dump)

TOOL = 'rotatefont'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


@pytest.mark.parametrize('i, matrix', [
    # scale glyphs horizontally by a factor of 2,
    # condense glyphs vertically by a factor three-quarters,
    # move glyphs left by 100, and up by 200.
    (1, '2.0 0 0 0.75 100 200'),

    # rotate glyphs clockwise 90 degrees around origin point (0,0)
    # move glyphs up by 1000
    (2, '0 -1.0 1.0 0 0 1000'),

    # oblique glyphs, slanting at a slope of 2 units horizontally for every
    # 10 units of height
    (3, '1.0 0 .2 1.0 0 0'),
])
@pytest.mark.parametrize('font_filename', ['font.pfa', 'cidfont.ps'])
def test_matrix(font_filename, i, matrix):
    matrix_values = [f'_{val}' for val in matrix.split()]

    font_path = get_input_path(font_filename)
    actual_path = get_temp_file_path()
    runner(CMD + ['-a', '-f', font_path, actual_path,
                  '-o', 't1', 'matrix'] + matrix_values)
    ext = 'pfa'
    skip = ['-s', '%ADOt1write:']
    if 'cid' in font_filename:
        ext = 'ps'
    expected_path = get_expected_path(f'matrix{i}.{ext}')
    if ext == 'ps':
        actual_path = generate_ps_dump(actual_path)
        expected_path = generate_ps_dump(expected_path)

    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('i, rt_args', [
    (1, '26 105 -77'),
    (2, '-11 -56 98'),
])
@pytest.mark.parametrize('font_filename', ['font.pfa', 'cidfont.ps'])
def test_rt_fd_options(font_filename, i, rt_args):
    fd_args = []
    ext = 'pfa'
    skip = ['-s', '%ADOt1write:']
    if 'cid' in font_filename:
        fd_args = ['fd', f'_{i}']
        ext = 'ps'

    rt_values = [f'_{val}' for val in rt_args.split()]

    font_path = get_input_path(font_filename)
    actual_path = get_temp_file_path()
    runner(CMD + ['-a', '-f', font_path, actual_path,
                  '-o', 't1', 'rt'] + rt_values + fd_args)
    expected_path = get_expected_path(f'rotatrans{i}.{ext}')
    if ext == 'ps':
        actual_path = generate_ps_dump(actual_path)
        expected_path = generate_ps_dump(expected_path)
    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('font_filename', ['font.pfa', 'cidfont.ps'])
def test_rtf_option(font_filename):
    ext = 'pfa'
    rtf_filename = 'rotate.txt'
    skip = ['-s', '%ADOt1write:']
    if 'cid' in font_filename:
        ext = 'ps'
        rtf_filename = 'cidrotate.txt'

    rtf_path = get_input_path(rtf_filename)

    font_path = get_input_path(font_filename)
    actual_path = get_temp_file_path()
    runner(CMD + ['-a', '-f', font_path, actual_path,
                  '-o', 't1', 'rt', '_45', '_0', '_0',
                  'rtf', f'_{rtf_path}'])
    expected_path = get_expected_path(f'rotafile.{ext}')
    if ext == 'ps':
        actual_path = generate_ps_dump(actual_path)
        expected_path = generate_ps_dump(expected_path)
    assert differ([expected_path, actual_path] + skip)
