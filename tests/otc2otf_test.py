import os
import pytest
from shutil import copy2, rmtree
import subprocess

from afdko.otc2otf import get_options
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

TOOL = 'otc2otf'
CMD = ['-t', TOOL]

REGULAR = 'SourceSansPro-Regular.otf'
ITALIC = 'SourceSansPro-It.otf'
BOLD = 'SourceSansPro-Bold.otf'
LIGHT = 'SourceSerifPro-LightIt.ttf'
SEMIBOLD = 'SourceSerifPro-SemiboldIt.ttf'

DATA_DIR = os.path.join(os.path.split(__file__)[0], TOOL + '_data')
TEMP_DIR = os.path.join(DATA_DIR, 'temp_output')


def setup_module():
    """
    Create the temporary output directory
    """
    rmtree(TEMP_DIR, ignore_errors=True)
    os.mkdir(TEMP_DIR)


def teardown_module():
    """
    teardown the temporary output directory
    """
    rmtree(TEMP_DIR)


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--foobar'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 2


def test_exit_no_font_provided():
    assert subprocess.call([TOOL]) == 2


def test_exit_invalid_path():
    assert subprocess.call([TOOL, 'foobar']) == 2


def test_exit_not_ttc_font():
    font_path = get_input_path('font.ttf')
    assert subprocess.call([TOOL, font_path]) == 2


@pytest.mark.parametrize('v_arg', ['', 'v', 'vv', 'vvv'])
def test_log_level(v_arg):
    in_path = get_input_path('RgIt.ttc')
    v_val = len(v_arg)
    arg = [] if not v_val else [f'-{v_arg}']
    opts = get_options([in_path] + arg)
    assert opts.verbose == v_val


@pytest.mark.parametrize('ttc_filename, input_filenames, diff_index', [
    ('RgBd.ttc', [REGULAR, BOLD], 0),
    ('RgIt.ttc', [REGULAR, ITALIC], 1),
    ('RgItBd.ttc', [REGULAR, ITALIC, BOLD], 2),
    ('LtSmbd.ttc', [LIGHT, SEMIBOLD], 1),
])
def test_convert(ttc_filename, input_filenames, diff_index):
    # 'otc2otf' doesn't have an output option. The test makes a temporary
    # directory and copies the input TTC font there. The output OTFs will
    # be saved to that directory as well.
    input_ttc_path = get_input_path(ttc_filename)
    temp_ttc_path = os.path.join(TEMP_DIR, ttc_filename)
    copy2(input_ttc_path, temp_ttc_path)

    runner(CMD + ['-a', '-f', temp_ttc_path])

    # diff only one of the OTFs
    input_filename = input_filenames[diff_index]
    actual_path = os.path.join(TEMP_DIR, input_filename)
    assert os.path.isfile(actual_path)
    actual_ttx = generate_ttx_dump(actual_path)
    in_ext = os.path.splitext(input_filename)[1]
    expected_ttx = get_expected_path(input_filename.replace(in_ext, '.ttx'))
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


@pytest.mark.parametrize('filename', ['RgBd.ttc', 'RgIt.ttc', 'RgItBd.ttc'])
def test_report(filename):
    expected_report = get_expected_path(filename.replace('.ttc', '.txt'))
    stdout_path = runner(CMD + ['-s', '-o', 'r', '-f', filename])
    assert differ([expected_report, stdout_path])


def test_psnames():
    runner(CMD + ['-f', 'no_psname-mac_psname-no_name_tb.ttc'])
    fpath1 = get_input_path('no_psname-mac_psname-no_name_tb-font0.ttf')
    fpath2 = get_input_path('mac_psname.ttf')
    fpath3 = get_input_path('no_psname-mac_psname-no_name_tb-font2.ttf')

    for fpth in (fpath1, fpath2, fpath3):
        assert os.path.exists(fpth)
        os.remove(fpth)


def test_exception():
    with pytest.raises(Exception):
        runner(CMD + ['-o', 'r', '-f', 'invalid_num_fonts.ttc'])
