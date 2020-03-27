import pytest
from shutil import copy2
import subprocess

from fontTools.ttLib import TTFont
from afdko import ttfdecomponentizer as ttfdecomp

from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
)
from runner import main as runner
from differ import main as differ

TOOL = 'ttfdecomponentizer'
CMD = ['-t', TOOL]

TEST_TTF_FILENAME = 'hascomponents.ttf'


def _get_test_ttf_path():
    return get_input_path(TEST_TTF_FILENAME)


def _read_file(file_path):
    with open(file_path, "rb") as f:
        return f.read()


def _write_file(file_path, data):
    with open(file_path, "wb") as f:
        f.write(data)


# -----
# Tests
# -----

def test_run_no_args():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main()
    assert exc_info.value.code == 2


def test_run_cli_no_args():
    with pytest.raises(subprocess.CalledProcessError) as exc_info:
        runner(CMD)
    assert exc_info.value.returncode == 2


def test_run_invalid_font_path():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main(['not_a_file'])
    assert exc_info.value.code == 2


def test_run_invalid_font_file():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main(['not_a_font.ttf'])
    assert exc_info.value.code == 2


def test_run_with_output_path():
    ttf_path = _get_test_ttf_path()
    save_path = get_temp_file_path()
    ttfdecomp.main(['-o', save_path, ttf_path])
    font = TTFont(save_path)
    gtable = font['glyf']
    composites = [gtable[gl].isComposite() for gl in font.glyphOrder]
    assert not any(composites)


def test_run_cli_with_output_path():
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'o', f'_{actual_path}',
                  f'_{get_input_path(TEST_TTF_FILENAME)}'])
    actual_ttx = generate_ttx_dump(actual_path, ['maxp', 'glyf'])
    expected_ttx = get_expected_path('nocomponents.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_run_without_output_path():
    ttf_path = _get_test_ttf_path()
    temp_dir = get_temp_dir_path()
    save_path = get_temp_file_path(directory=temp_dir)
    copy2(ttf_path, save_path)
    ttfdecomp.main([save_path])
    font = TTFont(save_path)
    gtable = font['glyf']

    composites = [gtable[gl].isComposite() for gl in font.glyphOrder]
    assert not any(composites)


def test_options_help():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main(['-h'])
    assert exc_info.value.code == 0


def test_options_version():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main(['--version'])
    assert exc_info.value.code == 0


def test_options_bogus_option():
    with pytest.raises(SystemExit) as exc_info:
        ttfdecomp.main(['--bogus'])
    assert exc_info.value.code == 2


def test_options_dir():
    input_dir = get_temp_dir_path()
    output_dir = get_temp_dir_path()
    font_path = _get_test_ttf_path()
    in_font_path = copy2(font_path, input_dir)
    ttfdecomp.main(['-d', input_dir, '-o', output_dir, '-v'])
    out_file = in_font_path.replace(input_dir, output_dir)
    font = TTFont(out_file)
    assert font['maxp'].maxComponentElements == 0


def test_options_bad_dir():
    with pytest.raises(SystemExit) as exc_info:
        input_dir = _get_test_ttf_path()
        ttfdecomp.main(['-d', input_dir])
    assert exc_info.value.code == 2


def test_options_bad_dir_output():
    with pytest.raises(SystemExit) as exc_info:
        input_dir = get_temp_dir_path()
        output_dir = _get_test_ttf_path()
        ttfdecomp.main(['-d', input_dir, '-o', output_dir])
    assert exc_info.value.code == 2
