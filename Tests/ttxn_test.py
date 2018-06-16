from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from runner import main as runner
from differ import main as differ

TOOL = 'ttxn'

OTF_FONT = 'OTF.otf'
OTF2_FONT = 'OTF2.otf'
OTF3_FONT = 'OTF3.otf'
TTF_FONT = 'TTF.ttf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


# -----
# Tests
# -----

@pytest.mark.parametrize('args, font_filename, exp_filename', [
    ([], OTF_FONT, 'OTF.ttx'),
    ([], TTF_FONT, 'TTF.ttx'),
    (['nh'], OTF_FONT, 'OTF_no_hints.ttx'),
    (['tGPOS'], OTF_FONT, 'OTF_GPOS_only.ttx'),
    (['tGSUB'], OTF_FONT, 'OTF_GSUB_only.ttx'),
    (['tGPOS'], OTF2_FONT, 'OTF2_GPOS_only.ttx'),
    (['tGSUB'], OTF2_FONT, 'OTF2_GSUB_only.ttx'),
    (['tGPOS'], OTF3_FONT, 'OTF3_GPOS_only.ttx'),
    (['tGSUB'], OTF3_FONT, 'OTF3_GSUB_only.ttx'),
])
def test_dump(args, font_filename, exp_filename):
    save_path = tempfile.mkstemp()[1]
    runner(['-t', TOOL, '-n', '-f', font_filename, '-o',
            'o{}'.format(save_path)] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, save_path, '-s', '<ttFont sfntVersion'])
