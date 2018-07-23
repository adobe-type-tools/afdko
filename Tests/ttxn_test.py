from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from .runner import main as runner
from .differ import main as differ

TOOL = 'ttxn'

OTF_FONT = 'SourceSansPro-Black_subset.otf'
TTF_FONT = 'SourceSansPro-Black_subset.ttf'
OTF2_FONT = 'SourceSansPro-ExtraLightIt.otf'
OTF3_FONT = 'SourceSansPro-Light.otf'
TTF2_FONT = 'NotoNaskhArabic-Regular.ttf'
TTF3_FONT = 'NotoNastaliqUrdu-Regular.ttf'

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
    (['tGPOS'], TTF2_FONT, 'TTF2_GPOS_only.ttx'),
    (['tGSUB'], TTF2_FONT, 'TTF2_GSUB_only.ttx'),
    (['tGPOS', 'se'], TTF3_FONT, 'TTF3_GPOS_only.ttx'),
    (['tGSUB', 'se'], TTF3_FONT, 'TTF3_GSUB_only.ttx'),
])
def test_dump(args, font_filename, exp_filename):
    save_path = tempfile.mkstemp()[1]
    runner(['-t', TOOL, '-n', '-f', font_filename, '-o',
            'o{}'.format(save_path)] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, save_path, '-s', '<ttFont sfntVersion'])
