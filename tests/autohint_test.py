import os
import pytest
from shutil import copy2

from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
    get_expected_path,
    generalizeCFF,
    generate_ttx_dump,
    generate_ps_dump,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

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
    pytest.param(
        'ufo2.ufo', '',
        marks=pytest.mark.skip(reason='UFO2 non-default layer not supported')),
    pytest.param(
        'ufo2.ufo', 'wd',),
    ('ufo3.ufo', ''),
    ('ufo3.ufo', 'wd'),
    ('font.pfa', 'fi'),
    ('font.pfb', 'fi'),
    ('font.otf', 'fi'),
    ('font.cff', 'fi'),
    pytest.param(
        'ufo2.ufo', 'fi',
        marks=pytest.mark.skip(reason='UFO2 non-default layer not supported')),
    ('ufo3.ufo', 'fi'),
])
def test_basic_hinting(font_filename, opt):
    arg = []
    head, tail = os.path.splitext(font_filename)

    if opt:
        if opt == 'fi':
            _copy_fontinfo_file()
        else:
            arg = [opt]
        expected_filename = f'{head}-{opt}{tail}'
    else:
        expected_filename = f'{head}{tail}'

    if 'ufo' in font_filename:
        actual_path = get_temp_dir_path()
    else:
        actual_path = get_temp_file_path()

    diff_mode = []
    if tail in {'.cff', '.pfb'}:
        diff_mode = ['-m', 'bin']

    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', f'_{actual_path}'] + arg)

    skip = []
    if tail == '.otf':
        fi = ''
        if opt == 'fi':
            fi = '-' + opt
        expected_filename = os.path.splitext(font_filename)[0] + fi + '.ttx'
        actual_path = generate_ttx_dump(
            generalizeCFF(actual_path), ['CFF '])
        skip = ['-l', '2']  # <ttFont sfntVersion=
    elif tail in {'.pfa', '.pfb'}:
        skip = ['-s', r'%ADOt1write:']
    elif tail == '.ps':
        actual_path = generate_ps_dump(actual_path)
        skip = ['-s', r'%ADOt1write:']
    elif tail == '.cff':
        actual_path = generalizeCFF(actual_path, do_sfnt=False)

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

    actual_path = get_temp_dir_path()
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
    actual_path = get_temp_dir_path()
    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', f'_{actual_path}'])
    runner(CMD + ['-f', actual_path, '-o', 'wd', 'all'])
    expected_path = get_expected_path(font_filename)
    assert differ([expected_path, actual_path])
