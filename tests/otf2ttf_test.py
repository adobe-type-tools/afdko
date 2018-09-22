from __future__ import print_function, division, absolute_import

import pytest

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ttx_dump)

TOOL = 'otf2ttf'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('filename', ['sans', 'serif', 'latincid', 'kanjicid'])
def test_convert(filename):
    input_path = get_input_path('{}.otf'.format(filename))
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'o', '_{}'.format(actual_path),
                  '-f', input_path])
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path('{}.ttx'.format(filename))
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])
