import os
import pytest
import subprocess

from test_utils import (
    get_input_path,
    get_expected_path,
    get_temp_file_path,
)
from runner import main as runner
from differ import main as differ

TOOL = 'otf2otc'
CMD = ['-t', TOOL]

REGULAR = 'SourceSansPro-Regular.otf'
ITALIC = 'SourceSansPro-It.otf'
BOLD = 'SourceSansPro-Bold.otf'
TTC = 'font.ttc'
FONT0 = 'font0.ttf'
FONT1 = 'font1.ttf'
FONT2 = 'font2.ttf'

MSG_1 = (
    "Shared tables: "
    f"['BASE', 'DSIG', 'GDEF', 'GSUB', 'cmap', 'maxp', 'post']{os.linesep}"
    "Un-shared tables: "
    "['CFF ', 'GPOS', 'OS/2', 'head', 'hhea', 'hmtx', 'name']").encode('ascii')

MSG_2 = (
    f"Shared tables: ['BASE', 'DSIG']{os.linesep}"
    "Un-shared tables: ['CFF ', 'GDEF', 'GPOS', 'GSUB', 'OS/2', 'cmap', "
    "'head', 'hhea', 'hmtx', 'maxp', 'name', 'post']").encode('ascii')

MSG_3 = (
    f"No tables are shared{os.linesep}"
    "Un-shared tables: ['OS/2', 'cmap', 'glyf', 'head', 'hhea', 'hmtx', "
    "'loca', 'maxp', 'name', 'post']").encode('ascii')

MSG_4 = (
    "Shared tables: ['OS/2', 'cmap', 'glyf', 'head', 'hhea', 'hmtx', "
    f"'loca', 'maxp', 'name', 'post']{os.linesep}"
    "All tables are shared").encode('ascii')


MSG_5 = (
    f"Shared tables: ['glyf']{os.linesep}"
    "Un-shared tables: ['OS/2', 'cmap', 'head', 'hhea', 'hmtx', 'loca', "
    "'maxp', 'name', 'post']").encode('ascii')


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--foobar'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


def test_exit_malformed_t_option():
    assert subprocess.call([TOOL, '-t', 'abc-2']) == 0


def test_exit_no_font_provided():
    assert subprocess.call([TOOL]) == 0


def test_exit_invalid_path():
    assert subprocess.call([TOOL, 'foobar']) == 0


def test_exit_not_a_font():
    font_path = get_input_path('file.txt')
    assert subprocess.call([TOOL, font_path]) == 0


@pytest.mark.parametrize('otf_filenames, ttc_filename, tables_msg', [
    pytest.param([REGULAR, BOLD], 'RgBd.ttc', MSG_1, id='regular-bold'),
    pytest.param([REGULAR, ITALIC], 'RgIt.ttc', MSG_2, id='regular-italic'),
    pytest.param([REGULAR, ITALIC, BOLD], 'RgItBd.ttc', MSG_2,
                 id='regular-italic-bold'),
    pytest.param([FONT0, FONT1], 'none_shared.ttc', MSG_3, id='none_shared'),
    pytest.param([FONT0, FONT0], 'all_shared.ttc', MSG_4, id='all_shared'),
    pytest.param([FONT0], 'single_font.ttc', MSG_4, id='single_font'),
    pytest.param([TTC, FONT2], 'ttc_input.ttc', MSG_3, id='ttc_input'),
])
def test_convert(otf_filenames, ttc_filename, tables_msg):
    actual_path = get_temp_file_path()
    expected_path = get_expected_path(ttc_filename)
    stdout_path = runner(CMD + ['-s', '-o', 'o', f'_{actual_path}',
                                '-f'] + otf_filenames)
    with open(stdout_path, 'rb') as f:
        output = f.read()
    assert tables_msg in output
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_t_option():
    actual_path = get_temp_file_path()
    expected_path = get_expected_path('shared_glyf.ttc')
    stdout_path = runner(CMD + ['-s', '-o', 'o', f'_{actual_path}',
                                't', '_glyf=0', '-f', FONT0, FONT1])
    with open(stdout_path, 'rb') as f:
        output = f.read()
    assert MSG_5 in output
    assert differ([expected_path, actual_path, '-m', 'bin'])
