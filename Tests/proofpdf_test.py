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


def _get_filename_label(file_name):
    sep_index = file_name.find('_')
    if sep_index == -1:
        return ''
    return file_name.split('.')[0][sep_index:]


# -----
# Tests
# -----

@pytest.mark.parametrize('font_filename', [
    'cidfont.otf',
    'font.otf',
    'font.ttf',
])
@pytest.mark.parametrize('tool_name', [
    'charplot',
    'digiplot',
    'fontplot',
    'fontplot2',
    'hintplot',
    'waterfallplot',
])
def test_glyphs_2_7(tool_name, font_filename):
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


@pytest.mark.parametrize('font_filename', [
    'cidfont_noHints.otf',
    'cidfont_noStems.otf',
    'cidfont_noZones.otf',
    'font_noHints.otf',
    'font_noStems.otf',
    'font_noZones.otf',
])
@pytest.mark.parametrize('tool_name', [
    'hintplot',
    'waterfallplot',
])
def test_hinting_data(tool_name, font_filename):
    label = _get_filename_label(font_filename)
    if 'cid' in font_filename:
        font_format = 'cid'
    elif 'ttf' in font_filename:
        font_format = 'ttf'
    else:
        font_format = 'otf'
    pdf_filename = '{}_{}{}.pdf'.format(tool_name, font_format, label)
    font_path = _get_input_path(font_filename)
    save_path = tempfile.mkstemp()[1]
    runner(['-t', tool_name, '-o', 'o', '_{}'.format(save_path), 'g', '_2-7',
            'dno', '=pageIncludeTitle', '_0', '-f', font_path, '-n', '-a'])
    expected_path = _get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])


@pytest.mark.parametrize('font_filename, glyphs', [
    ('cidfont.otf', '_0-5,98-101'),
    ('font.otf', '_0-2,4,5'),
])
def test_fontplot2_lf_option(font_filename, glyphs):
    tool_name = 'fontplot2'
    if 'cid' in font_filename:
        font_format = 'cid'
    else:
        font_format = 'otf'
    layout_path = _get_input_path('CID_layout')
    font_path = _get_input_path(font_filename)
    pdf_filename = '{}_{}_lf_option.pdf'.format(tool_name, font_format)
    save_path = tempfile.mkstemp()[1]
    runner(['-t', tool_name, '-o', 'o', '_{}'.format(save_path), 'dno',
            'g', glyphs, 'lf', '_{}'.format(layout_path),
            '=pageIncludeTitle', '_0', '-f', font_path, '-n', '-a'])
    expected_path = _get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])
