import pytest

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path, get_expected_path, get_temp_file_path


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
    pdf_filename = f'{tool_name}_{font_format}_glyphs_2-7.pdf'
    font_path = get_input_path(font_filename)
    save_path = get_temp_file_path()
    runner(['-t', tool_name, '-o', 'o', f'_{save_path}', 'g', '_2-7',
            'dno', '=pageIncludeTitle', '_0', '-f', font_path, '-a'])
    expected_path = get_expected_path(pdf_filename)
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
    pdf_filename = f'{tool_name}_{font_format}{label}.pdf'
    font_path = get_input_path(font_filename)
    save_path = get_temp_file_path()
    runner(['-t', tool_name, '-o', 'o', f'_{save_path}', 'g', '_2-7',
            'dno', '=pageIncludeTitle', '_0', '-f', font_path, '-a'])
    expected_path = get_expected_path(pdf_filename)
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
    layout_path = get_input_path('CID_layout')
    font_path = get_input_path(font_filename)
    pdf_filename = f'{tool_name}_{font_format}_lf_option.pdf'
    save_path = get_temp_file_path()
    runner(['-t', tool_name, '-o', 'o', f'_{save_path}', 'dno',
            'g', glyphs, 'lf', f'_{layout_path}',
            '=pageIncludeTitle', '_0', '-f', font_path, '-a'])
    expected_path = get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])


def test_fontsetplot():
    f1 = 'SourceSansPro-Black.otf'
    f2 = 'SourceSansPro-BlackIt.otf'
    pdf_filename = "fontsetplot_otf_glyphs_2-7.pdf"
    fp1 = get_input_path(f1)
    fp2 = get_input_path(f2)
    save_path = get_temp_file_path()
    runner(['-t', 'fontsetplot', '-o', 'o', f'_{save_path}', 'dno',
            'g', '_2-7', '=pageIncludeTitle', '_0', f'_{fp1}', f'_{fp2}'])
    expected_path = get_expected_path(pdf_filename)

    assert(differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman']))


@pytest.mark.parametrize('filename', ['SourceSansPro-Black',
                                      'SourceSansPro-BlackIt'])
def test_waterfallplot(filename):
    font_filename = f'{filename}.otf'
    pdf_filename = f'{filename}.pdf'
    font_path = get_input_path(font_filename)
    save_path = get_temp_file_path()
    expected_path = get_expected_path(pdf_filename)
    runner(['-t', 'waterfallplot',
            '-o', 'o', f'_{save_path}', 'dno', '-f', font_path, '-a'])
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate',
                   '-r', r'^BT 1 0 0 1 \d{3}\.\d+ 742\.0000 Tm',  # timestamp
                   '-e', 'macroman'])


@pytest.mark.parametrize('tool_name', [
    'charplot',
    'digiplot',
    'fontplot',
    'fontplot2',
    'hintplot',
    'waterfallplot',
])
def test_seac_in_charstring_bug125(tool_name):
    pdf_filename = f'bug125_{tool_name}.pdf'
    font_path = get_input_path('seac.otf')
    save_path = get_temp_file_path()
    runner(['-t', tool_name, '-o', 'o', f'_{save_path}', 'dno',
            '=pageIncludeTitle', '_0', '-f', font_path, '-a'])
    expected_path = get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])


@pytest.mark.parametrize('font_format', ['otf', 'ttf'])
def test_round_glyph_bounds_values_bug128(font_format):
    bug_numb = 'bug128'
    pdf_filename = f'{bug_numb}_{font_format}.pdf'
    font_path = get_input_path(f'{bug_numb}/font.{font_format}')
    save_path = get_temp_file_path()
    runner(['-t', 'charplot', '-o', 'o', f'_{save_path}', 'g', '_o',
            'dno', '=pageIncludeTitle', '_0', '-f', font_path, '-a'])
    expected_path = get_expected_path(pdf_filename)
    assert differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman'])


def test_fontsetplot_ttf_with_components_bug1125():
    pdf_filename = "bug1125.pdf"
    font_path = get_input_path('bug1125.ttf')
    save_path = get_temp_file_path()
    runner(['-t', 'fontsetplot', '-o', 'o', f'_{save_path}', 'dno',
            '=pageIncludeTitle', '_0', f'_{font_path}'])
    expected_path = get_expected_path(pdf_filename)

    assert(differ([expected_path, save_path,
                   '-s', '/CreationDate', '-e', 'macroman']))
