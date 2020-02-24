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
DESIGN_NAMES_LIST = ['acaron', 'acutecmb', 'caroncmb', 'design_name',
                     'gravecmb', 'tildecmb']
PRODCT_NAMES_LIST = ['production_name', 'uni01CE', 'uni0300', 'uni0301',
                     'uni0303', 'uni030C']
EMPTY_LIB = b"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
"http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
    <dict/>
</plist>
"""


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
    print(save_path)
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
