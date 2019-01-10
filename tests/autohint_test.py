from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import copy2
import tempfile

from runner import main as runner
from differ import main as differ
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump)

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
    expected_filename = font_filename
    if opt:
        if opt == 'fi':
            _copy_fontinfo_file()
        else:
            arg = [opt]
        head, tail = os.path.splitext(font_filename)
        expected_filename = '{}-{}{}'.format(head, opt, tail)

    if 'ufo' in font_filename:
        actual_path = tempfile.mkdtemp()
    else:
        actual_path = get_temp_file_path()

    diff_mode = []
    for font_format in ('.pfb', '.cff', '.ps'):
        if font_filename.endswith(font_format):
            diff_mode = ['-m', 'bin']
            break

    runner(CMD + ['-f', get_input_path(font_filename),
                  '-o', 'o', '_{}'.format(actual_path)] + arg)

    skip = []
    if 'otf' in font_filename:
        fi = ''
        if opt == 'fi':
            fi = '-' + opt
        expected_filename = os.path.splitext(font_filename)[0] + fi + '.ttx'
        actual_path = generate_ttx_dump(actual_path, ['CFF '])
        skip = ['-l', '2']  # <ttFont sfntVersion=

    expected_path = get_expected_path(expected_filename)
    assert differ([expected_path, actual_path] + diff_mode + skip)


def test_beztools_hhint_over_limit_bug629():
    test_filename = 'bug629.pfa'
    actual_path = get_temp_file_path()
    expected_path = get_expected_path(test_filename)
    runner(CMD + ['-o', 'nb', 'o', '_{}'.format(actual_path),
                  '-f', test_filename])
    assert differ([expected_path, actual_path,
                   '-s', r'%%Copyright: Copyright'])
