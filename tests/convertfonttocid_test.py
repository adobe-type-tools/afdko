import os
import pytest
from shutil import copy2, rmtree

from afdko.convertfonttocid import mergeFontToCFF

from differ import main as differ
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump)

MODULE = 'convertfonttocid'

DATA_DIR = os.path.join(os.path.dirname(__file__), MODULE + '_data')
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

@pytest.mark.parametrize('subroutinize', [True, False])
@pytest.mark.parametrize('font_filename', ['1_fdict.ps', '3_fdict.ps'])
def test_mergeFontToCFF_bug570(font_filename, subroutinize):
    # 'dir=TEMP_DIR' is used for guaranteeing that the temp data is on same
    # file system as other data; if it's not, a file rename step made by
    # sfntedit will NOT work.
    actual_path = get_temp_file_path(directory=TEMP_DIR)
    subr_str = 'subr' if subroutinize else 'no_subr'
    font_base = os.path.splitext(font_filename)[0]
    ttx_filename = f'{font_base}-{subr_str}.ttx'
    source_path = get_input_path(font_filename)
    output_path = get_input_path('core.otf')
    copy2(output_path, actual_path)

    mergeFontToCFF(source_path, actual_path, subroutinize)

    actual_ttx = generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-l', '2'])
