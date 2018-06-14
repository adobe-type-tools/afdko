from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from runner import main as runner
from differ import main as differ

TOOL = 'proofpdf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


# -----
# Tests
# -----

@pytest.mark.parametrize('tool_name', [
    'charplot',
    'digiplot',
    'fontplot',
    'fontplot2',
    'hintplot',
    'waterfallplot',
])
@pytest.mark.parametrize('font_filename', [
    'cidfont.otf',
    'font.otf',
    'font.ttf',
])
def test_font_glyphs_2_7(tool_name, font_filename):
    if 'cid' in font_filename:
        font_format = 'cid'
    elif 'ttf' in font_filename:
        font_format = 'ttf'
    else:
        font_format = 'otf'
    pdf_filename = '{}_{}_glyphs_2-7.pdf'.format(tool_name, font_format)
    font_path = _get_input_path(font_filename)
    save_path = tempfile.mkstemp()[1]
    runner(['-t', tool_name, '-o', 'o', '_{}'.format(save_path), 'g', '_2-7',
            'dno', '=pageIncludeTitle', '_0', '-f', font_path, '-n', '-a'])
    expected_path = _get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])
