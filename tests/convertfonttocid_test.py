from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import copy2, rmtree
import tempfile

from fontTools.ttLib import TTFont

from afdko.convertfonttocid import mergeFontToCFF

from .differ import main as differ

MODULE = 'convertfonttocid'

data_dir_path = os.path.join(os.path.split(__file__)[0], MODULE + '_data')
temp_dir_path = os.path.join(data_dir_path, 'temp_output')


def setup_module():
    """
    Create the temporary output directory
    """
    os.mkdir(temp_dir_path)


def teardown_module():
    """
    teardown the temporary output directory
    """
    rmtree(temp_dir_path)


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


def _get_temp_file_path():
    # make sure temp data on same file system as other data for rename to work
    file_descriptor, path = tempfile.mkstemp(dir=temp_dir_path)
    os.close(file_descriptor)
    return path


def _generate_ttx_dump(font_path, tables=None):
    with TTFont(font_path) as font:
        temp_path = _get_temp_file_path()
        font.saveXML(temp_path, tables=tables)
        return temp_path


# -----
# Tests
# -----

@pytest.mark.parametrize('subroutinize', [True, False])
@pytest.mark.parametrize('font_filename', ['1_fdict.ps', '3_fdict.ps'])
def test_mergeFontToCFF_bug570(font_filename, subroutinize):
    actual_path = _get_temp_file_path()
    subr_str = 'subr' if subroutinize else 'no_subr'
    ttx_filename = '{}-{}.ttx'.format(font_filename.split('.')[0], subr_str)
    source_path = _get_input_path(font_filename)
    output_path = _get_input_path('core.otf')
    copy2(output_path, actual_path)

    mergeFontToCFF(source_path, actual_path, subroutinize)

    actual_ttx = _generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = _get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-l', '2'])
