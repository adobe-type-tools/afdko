from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from .runner import main as runner
from .differ import main as differ

TOOL = 'comparefamily'
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


# -----
# Tests
# -----

@pytest.mark.parametrize('font_format', ['otf', 'ttf'])
@pytest.mark.parametrize('font_family', ['source-code-pro'])
def test_report(font_family, font_format):
    input_dir = os.path.join(_get_input_path(font_family), font_format)
    log_path = _get_temp_file_path()
    runner(CMD + ['-n', '-o', 'd', '_{}'.format(input_dir), 'tolerance', '_3',
                  'rm', 'rn', 'rp', 'l', '_{}'.format(log_path)])
    expected_path = _get_expected_path('{}_{}.txt'.format(
                                       font_family, font_format))
    assert differ([expected_path, log_path, '-l', '1'])
