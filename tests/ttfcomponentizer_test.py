import os
import pytest
from shutil import copy2, copytree
import subprocess

from fontTools.ttLib import TTFont
from afdko import ttfcomponentizer as ttfcomp

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

TOOL = 'ttfcomponentizer'
CMD = ['-t', TOOL]

TEST_TTF_FILENAME = 'ttfcomponentizer.ttf'
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
EMPTY_PSKEY = b"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
"http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
    <dict>
    <key>public.postscriptNames</key>
    <dict/>
    </dict>
</plist>
"""


def _get_test_ttf_path():
    return get_input_path(TEST_TTF_FILENAME)


def _get_test_ufo_path():
    return _get_test_ttf_path()[:-3] + 'ufo'


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
        ttfcomp.main()
    assert exc_info.value.code == 2


def test_run_cli_no_args():
    with pytest.raises(subprocess.CalledProcessError) as exc_info:
        runner(CMD)
    assert exc_info.value.returncode == 2


def test_run_invalid_font_path():
    with pytest.raises(SystemExit) as exc_info:
        ttfcomp.main(['not_a_file'])
    assert exc_info.value.code == 2


def test_run_invalid_font_file():
    with pytest.raises(SystemExit) as exc_info:
        ttfcomp.main(['not_a_font.ttf'])
    assert exc_info.value.code == 2


def test_run_ufo_not_found():
    ttf_path = _get_test_ttf_path()
    temp_dir = get_temp_dir_path()
    save_path = get_temp_file_path(directory=temp_dir)
    copy2(ttf_path, save_path)
    assert ttfcomp.main([save_path]) == 1


def test_run_invalid_ufo():
    ttf_path = _get_test_ttf_path()
    temp_dir = get_temp_dir_path()
    save_path = get_temp_file_path(directory=temp_dir)
    ufo_path = save_path + '.ufo'
    copy2(ttf_path, save_path)
    copy2(ttf_path, ufo_path)
    assert ttfcomp.main([save_path]) == 1


def test_run_with_output_path():
    ttf_path = _get_test_ttf_path()
    save_path = get_temp_file_path()
    ttfcomp.main(['-o', save_path, ttf_path])
    gtable = TTFont(save_path)['glyf']
    composites = [gname for gname in gtable.glyphs if (
        gtable[gname].isComposite())]
    assert sorted(composites) == ['aa', 'aacute', 'uni01CE']


def test_run_cli_with_output_path():
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'o', f'_{actual_path}',
                  f'_{get_input_path(TEST_TTF_FILENAME)}'])
    actual_ttx = generate_ttx_dump(actual_path, ['maxp', 'glyf'])
    expected_ttx = get_expected_path('ttfcomponentizer.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_run_without_output_path():
    ttf_path = _get_test_ttf_path()
    ufo_path = _get_test_ufo_path()
    temp_dir = get_temp_dir_path()
    tmp_ufo_path = os.path.join(temp_dir, os.path.basename(ufo_path))
    save_path = get_temp_file_path(directory=temp_dir)
    copy2(ttf_path, save_path)
    copytree(ufo_path, tmp_ufo_path)
    ttfcomp.main([save_path])
    gtable = TTFont(save_path)['glyf']
    assert gtable['agrave'].isComposite() is False
    assert gtable['aacute'].isComposite() is True


def test_options_help():
    with pytest.raises(SystemExit) as exc_info:
        ttfcomp.main(['-h'])
    assert exc_info.value.code == 0


def test_options_version():
    with pytest.raises(SystemExit) as exc_info:
        ttfcomp.main(['--version'])
    assert exc_info.value.code == 0


def test_options_bogus_option():
    with pytest.raises(SystemExit) as exc_info:
        ttfcomp.main(['--bogus'])
    assert exc_info.value.code == 2


def test_get_ufo_path_found():
    path = _get_test_ttf_path()
    assert ttfcomp.get_ufo_path(path) == _get_test_ufo_path()


def test_get_ufo_path_not_found():
    # the UFO font is intentionally used as a test folder
    path = os.path.join(_get_test_ufo_path(), 'lib.plist')
    assert ttfcomp.get_ufo_path(path) is None


def test_get_goadb_path_found_same_folder():
    path = _get_test_ttf_path()
    assert os.path.basename(
        ttfcomp.get_goadb_path(path)) == ttfcomp.GOADB_FILENAME


def test_get_goadb_path_found_3_folders_up():
    # the UFO's glif file is intentionally used as a test file
    path = os.path.join(_get_test_ufo_path(), 'glyphs', 'a.glif')
    assert os.path.basename(
        ttfcomp.get_goadb_path(path)) == ttfcomp.GOADB_FILENAME


def test_get_goadb_path_not_found():
    path = os.path.dirname(_get_test_ttf_path())
    assert ttfcomp.get_goadb_path(path) is None


def test_get_goadb_names_mapping_goadb_not_found():
    path = os.path.dirname(_get_test_ttf_path())
    assert ttfcomp.get_goadb_names_mapping(path) == {}


def test_get_glyph_names_mapping_invalid_ufo():
    path = _get_test_ttf_path()
    assert ttfcomp.get_glyph_names_mapping(path) == (None, None)


def test_get_glyph_names_mapping_names_from_lib():
    result = ttfcomp.get_glyph_names_mapping(_get_test_ufo_path())
    assert sorted(result[1].keys()) == DESIGN_NAMES_LIST
    assert sorted(result[1].values()) == PRODCT_NAMES_LIST


def test_get_glyph_names_mapping_names_from_goadb():
    # the test UFO has a 'public.postscriptNames' in its lib;
    # empty the lib temporarily to force the reading of the GOADB
    ufo_path = _get_test_ufo_path()
    lib_path = os.path.join(ufo_path, 'lib.plist')
    lib_data = _read_file(lib_path)
    _write_file(lib_path, EMPTY_LIB)
    result = ttfcomp.get_glyph_names_mapping(ufo_path)
    _write_file(lib_path, lib_data)
    assert sorted(result[1].keys()) == DESIGN_NAMES_LIST
    assert sorted(result[1].values()) == PRODCT_NAMES_LIST


def test_warn_empty_psnames_key(capsys):
    # the test UFO has a 'public.postscriptNames' in its lib;
    # set the key to empty to ensure we get a warning
    ufo_path = _get_test_ufo_path()
    lib_path = os.path.join(ufo_path, 'lib.plist')
    lib_data = _read_file(lib_path)
    _write_file(lib_path, EMPTY_PSKEY)
    result = ttfcomp.get_glyph_names_mapping(ufo_path)
    _write_file(lib_path, lib_data)
    assert len(result[1]) == 0
    captured = capsys.readouterr()
    # check (partial) warning message in stdout:
    assert "WARNING: The contents of public.postscriptNames is empty." in captured.out  # noqa: E501


def test_componentize():
    ttf_path = _get_test_ttf_path()
    save_path = get_temp_file_path()
    ufo, ps_names = ttfcomp.get_glyph_names_mapping(_get_test_ufo_path())
    ttcomp_obj = ttfcomp.TTComponentizer(ufo, ps_names, ttf_path, save_path)
    ttcomp_obj.componentize()

    # 'get_composites_data' method
    comps_data = ttcomp_obj.composites_data
    comps_name_list = sorted(comps_data.keys())
    comps_comp_list = [comps_data[gname] for gname in comps_name_list]
    assert comps_name_list == ['aa', 'aacute', 'adieresis', 'atilde',
                               'uni01CE']
    assert comps_comp_list[1].names == ('a', 'uni0301')
    assert comps_comp_list[4].names == ('a', 'uni030C')
    assert comps_comp_list[1].positions == ((0, 0), (263.35, 0))
    assert comps_comp_list[4].positions == ((0, 0), (263, 0))

    # 'assemble_components' method
    comps_data = ttfcomp.ComponentsData()
    comps_data.names = ('a', 'uni01CE')
    comps_data.positions = ((0, 0), (263, 0))
    comps_data.same_advwidth = True
    comp_one, comp_two = ttcomp_obj.assemble_components(comps_data)
    assert comp_one.glyphName == 'a'
    assert comp_two.glyphName == 'uni01CE'
    assert (comp_one.x, comp_one.y) == (0, 0)
    assert (comp_two.x, comp_two.y) == (263, 0)
    assert comp_one.flags == 0x204
    assert comp_two.flags == 0x4
