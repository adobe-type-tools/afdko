import os
import pytest
from shutil import copy2
import tempfile

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump, generate_ps_dump)

TOOL = 'autohint'
CMD = ['-t', TOOL]


def _copy_fontinfo_file():
    """
    Copies 'fontinfo.txt' to 'fontinfo'
    """
    fontinfo_source = get_input_path('fontinfo.txt')
    fontinfo_copy = fontinfo_source[:-4]
    if not os.path.exists(fontinfo_copy):
        copy2(fontinfo_source, fontinfo_copy)


def teardown_function():
    """
    Deletes the temporary 'fontinfo' file
    """
    try:
        os.remove(get_input_path('fontinfo'))
    except OSError:
        pass


# -----
# Tests
# -----

@pytest.mark.parametrize('font_filename, opt', [
    ('font.pfa', ''),
    ('font.pfb', ''),
    ('font.otf', ''),
    ('font.cff', ''),
    ('cidfont.otf', ''),
    ('cidfont.ps', ''),
    ('ufo2.ufo', ''),
    ('ufo2.ufo', 'wd'),
    ('ufo3.ufo', ''),
    ('ufo3.ufo', 'wd'),
    ('font.pfa', 'fi'),
    ('font.pfb', 'fi'),
    ('font.otf', 'fi'),
    ('font.cff', 'fi'),
    ('ufo2.ufo', 'fi'),
    ('ufo3.ufo', 'fi'),
])
def test_basic_hinting(font_filename, opt):
    arg = []
    head, tail = os.path.splitext(font_filename)
    if tail == '.pfb':
        tail = '.pfa'  # the input is PFB, but the output will be PFA
    if opt:
        if opt == 'fi':
            _copy_fontinfo_file()
        else:
            arg = [opt]
        expected_filename = f'{head}-{opt}{tail}'
    else:
        expected_filename = f'{head}{tail}'

    if 'ufo' in font_filename:
        actual_path = tempfile.mkdtemp()
    else:
        actual_path = get_temp_file_path()

    diff_mode = []
    if tail == '.cff':
        diff_mode = ['-m', 'bin']

    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', f'_{actual_path}'] + arg)

    skip = []
    if tail == '.otf':
        fi = ''
        if opt == 'fi':
            fi = '-' + opt
        expected_filename = os.path.splitext(font_filename)[0] + fi + '.ttx'
        actual_path = generate_ttx_dump(actual_path, ['CFF '])
        skip = ['-l', '2']  # <ttFont sfntVersion=
    elif tail in {'.pfa', '.pfb'}:
        skip = ['-s', r'%ADOt1write:']
    elif tail == '.ps':
        actual_path = generate_ps_dump(actual_path)
        skip = ['-s', r'%ADOt1write:']

    expected_path = get_expected_path(expected_filename)
    if tail == '.ps':
        expected_path = generate_ps_dump(expected_path)

    assert differ([expected_path, actual_path] + diff_mode + skip)


def test_beztools_hhint_over_limit_bug629():
    test_filename = 'bug629.pfa'
    actual_path = get_temp_file_path()
    expected_path = get_expected_path(test_filename)
    runner(CMD + ['-o', 'nb', 'o', f'_{actual_path}',
                  '-f', test_filename])
    assert differ([expected_path, actual_path,
                   '-s',
                   r'%ADOt1write' + SPLIT_MARKER + r'%%Copyright: Copyright'])


@pytest.mark.parametrize('font_filename, opt', [
    ('decimals.ufo', ''),
    ('decimals.ufo', 'decimal'),
    ('hashmap_advance.ufo', ''),
    ('hashmap_unnormalized_floats.ufo', ''),
    ('hashmap_transform.ufo', ''),
    ('hashmap_dflt_layer.ufo', 'wd'),
])
def test_ufo_hashmap(font_filename, opt):
    arg = []
    expected_filename = font_filename
    if opt:
        arg = [opt]
        head, tail = os.path.splitext(font_filename)
        expected_filename = f'{head}-{opt}{tail}'

    actual_path = tempfile.mkdtemp()
    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', f'_{actual_path}'] + arg)
    expected_path = get_expected_path(expected_filename)
    assert differ([expected_path, actual_path])


def test_ufo_hashmap_rehint():
    """
    The font is first hinted with no options, and then it's rehinted with -wd
    option. This test serves to test that the processed layer added in the
    first run gets removed in the second.
    """
    font_filename = 'hashmap_dflt_layer.ufo'
    actual_path = tempfile.mkdtemp()
    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', f'_{actual_path}'])
    runner(CMD + ['-f', actual_path, '-o', 'wd', 'all'])
    expected_path = get_expected_path(font_filename)
    assert differ([expected_path, actual_path])
