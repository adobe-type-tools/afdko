import pytest
import tempfile

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path

TOOL = 'ttxn'

OTF_FONT = 'SourceSansPro-Black_subset.otf'
TTF_FONT = 'SourceSansPro-Black_subset.ttf'
OTF2_FONT = 'SourceSansPro-ExtraLightIt.otf'
OTF3_FONT = 'SourceSansPro-Light.otf'
TTF2_FONT = 'NotoNaskhArabic-Regular.ttf'
TTF3_FONT = 'NotoNastaliqUrdu-Regular.ttf'


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
    runner(['-t', TOOL, '-f', font_filename, '-o',
            f'o{save_path}'] + args)
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, save_path, '-s', '<ttFont sfntVersion'])
