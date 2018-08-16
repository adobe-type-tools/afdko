from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import copy2
import tempfile

from fontTools.ttLib import TTFont

from .runner import main as runner
from .differ import main as differ

TOOL = 'autohint'
CMD = ['-t', TOOL]

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    file_descriptor, path = tempfile.mkstemp()
    os.close(file_descriptor)
    return path


def _generate_ttx_dump(font_path, tables=None):
    with TTFont(font_path) as font:
        temp_path = _get_temp_file_path()
        font.saveXML(temp_path, tables=tables)
        return temp_path


def _copy_fontinfo_file():
    """
    Copies 'fontinfo.txt' to 'fontinfo'
    """
    fontinfo_source = _get_input_path('fontinfo.txt')
    fontinfo_copy = fontinfo_source[:-4]
    if not os.path.exists(fontinfo_copy):
        copy2(fontinfo_source, fontinfo_copy)


def teardown_module():
    """
    Deletes the temporary 'fontinfo' file
    """
    try:
        os.remove(_get_input_path('fontinfo'))
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
        actual_path = _get_temp_file_path()

    diff_mode = []
    for font_format in ('.pfb', '.cff', '.ps'):
        if font_filename.endswith(font_format):
            diff_mode = ['-m', 'bin']
            break

    runner(CMD + ['-n', '-f', _get_input_path(font_filename),
                  '-o', 'o', '_{}'.format(actual_path)] + arg)

    skip = []
    if 'otf' in font_filename:
        fi = ''
        if opt == 'fi':
            fi = '-' + opt
        expected_filename = os.path.splitext(font_filename)[0] + fi + '.ttx'
        actual_path = _generate_ttx_dump(actual_path, ['CFF '])
        skip = ['-l', '2']  # <ttFont sfntVersion=

    expected_path = _get_expected_path(expected_filename)
    assert differ([expected_path, actual_path] + diff_mode + skip)
