from __future__ import print_function, division, absolute_import

import os
import pytest
from shutil import copy2, copytree
import tempfile

from fontTools.misc.py23 import open
from fontTools.ttLib import TTFont

from afdko.Tools.SharedData.FDKScripts.ttfcomponentizer import (
    main, get_options, get_ufo_path, get_goadb_path, GOADB_FILENAME,
    get_goadb_names_mapping, get_glyph_names_mapping, get_composites_data,
    assemble_components)

TEST_TTF_FILENAME = 'ttfcomponentizer.ttf'
DESIGN_NAMES_LIST = ['acaron', 'acutecmb', 'caroncmb', 'design_name',
                     'gravecmb', 'tildecmb']
PRODCT_NAMES_LIST = ['production_name', 'uni01CE', 'uni0300', 'uni0301',
                     'uni0303', 'uni030C']
EMPTY_LIB = u"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
"http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
    <dict/>
</plist>
"""


class Object(object):
    pass


def _get_test_file_path(file_name):
    return os.path.join(os.path.split(__file__)[0], 'data', file_name)


def _get_test_ttf_path():
    return _get_test_file_path(TEST_TTF_FILENAME)


def _get_test_ufo_path():
    return _get_test_ttf_path()[:-3] + 'ufo'


def _read_txt_file(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        return f.read()


def _write_file(file_path, data):
    with open(file_path, "w") as f:
        f.write(data)


# -----
# Tests
# -----

def test_run_no_args():
    with pytest.raises(SystemExit) as exc_info:
        main()
    assert exc_info.value.code == 2


def test_run_invalid_font():
    assert main(['not_a_file']) == 1


def test_run_ufo_not_found():
    ttf_path = _get_test_ttf_path()
    temp_dir = tempfile.mkdtemp()
    with tempfile.NamedTemporaryFile(delete=False, dir=temp_dir) as tmp:
        copy2(ttf_path, tmp.name)
        assert main([tmp.name]) == 1


def test_run_invalid_ufo():
    ttf_path = _get_test_ttf_path()
    temp_dir = tempfile.mkdtemp()
    with tempfile.NamedTemporaryFile(delete=False, dir=temp_dir) as tmp:
        ufo_path = tmp.name + '.ufo'
        copy2(ttf_path, tmp.name)
        copy2(ttf_path, ufo_path)
        assert main([tmp.name]) == 1


def test_run_with_output_path():
    ttf_path = _get_test_ttf_path()
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        main(['-o', tmp.name, ttf_path])
        gtable = TTFont(tmp.name)['glyf']
        composites = [gname for gname in gtable.glyphs if (
            gtable[gname].isComposite())]
        assert sorted(composites) == ['aacute', 'uni01CE']


def test_run_without_output_path():
    ttf_path = _get_test_ttf_path()
    ufo_path = _get_test_ufo_path()
    temp_dir = tempfile.mkdtemp()
    tmp_ufo_path = os.path.join(temp_dir, os.path.basename(ufo_path))
    with tempfile.NamedTemporaryFile(delete=False, dir=temp_dir) as tmp:
        copy2(ttf_path, tmp.name)
        copytree(ufo_path, tmp_ufo_path)
        main([tmp.name])
        gtable = TTFont(tmp.name)['glyf']
        assert gtable['agrave'].isComposite() is False
        assert gtable['aacute'].isComposite() is True


def test_options_help():
    with pytest.raises(SystemExit) as exc_info:
        main(['-h'])
    assert exc_info.value.code == 0


def test_options_version():
    with pytest.raises(SystemExit) as exc_info:
        main(['--version'])
    assert exc_info.value.code == 0


def test_options_bogus_option():
    with pytest.raises(SystemExit) as exc_info:
        main(['--bogus'])
    assert exc_info.value.code == 2


def test_options_invalid_font_path():
    assert get_options(['not_a_file']).font_path is None


def test_options_invalid_font():
    path = _get_test_file_path('not_a_font.ttf')
    assert get_options([path]).font_path is None


def test_options_valid_font():
    path = _get_test_ttf_path()
    assert os.path.basename(get_options([path]).font_path) == TEST_TTF_FILENAME


def test_get_ufo_path_found():
    path = _get_test_ttf_path()
    assert get_ufo_path(path) == _get_test_ufo_path()


def test_get_ufo_path_not_found():
    # the UFO font is intentionally used as a test folder
    path = os.path.join(_get_test_ufo_path(), 'lib.plist')
    assert get_ufo_path(path) is None


def test_get_goadb_path_found_same_folder():
    path = _get_test_ttf_path()
    assert os.path.basename(get_goadb_path(path)) == GOADB_FILENAME


def test_get_goadb_path_found_3_folders_up():
    # the UFO's glif file is intentionally used as a test file
    path = os.path.join(_get_test_ufo_path(), 'glyphs', 'a.glif')
    assert os.path.basename(get_goadb_path(path)) == GOADB_FILENAME


def test_get_goadb_path_not_found():
    path = os.path.dirname(_get_test_ttf_path())
    assert get_goadb_path(path) is None


def test_get_goadb_names_mapping_goadb_not_found():
    path = os.path.dirname(_get_test_ttf_path())
    assert get_goadb_names_mapping(path) == {}


def test_get_glyph_names_mapping_invalid_ufo():
    path = _get_test_ttf_path()
    assert get_glyph_names_mapping(path) == (None, None)


def test_get_glyph_names_mapping_names_from_lib():
    result = get_glyph_names_mapping(_get_test_ufo_path())
    assert sorted(result[1].keys()) == DESIGN_NAMES_LIST
    assert sorted(result[1].values()) == PRODCT_NAMES_LIST


def test_get_glyph_names_mapping_names_from_goadb():
    # the test UFO has a 'public.postscriptNames' in its lib;
    # empty the lib temporarily to force the reading of the GOADB
    ufo_path = _get_test_ufo_path()
    lib_path = os.path.join(ufo_path, 'lib.plist')
    lib_data = _read_txt_file(lib_path)
    _write_file(lib_path, EMPTY_LIB)
    result = get_glyph_names_mapping(ufo_path)
    _write_file(lib_path, lib_data)
    assert sorted(result[1].keys()) == DESIGN_NAMES_LIST
    assert sorted(result[1].values()) == PRODCT_NAMES_LIST


def test_get_composites_data():
    ufo, ps_names = get_glyph_names_mapping(_get_test_ufo_path())
    comps_data = get_composites_data(ufo, ps_names)
    comps_name_list = sorted(list(comps_data.keys()))
    comps_comp_list = [comps_data[gname] for gname in comps_name_list]
    assert comps_name_list == ['aacute', 'adieresis', 'atilde', 'uni01CE']
    assert comps_comp_list[0].names == ('a', 'uni0301')
    assert comps_comp_list[3].names == ('a', 'uni030C')
    assert comps_comp_list[0].positions == ((0, 0), (263.35, 0))
    assert comps_comp_list[3].positions == ((0, 0), (263, 0))


def test_assemble_components():
    comps_data = Object()
    setattr(comps_data, 'names', ('a', 'uni01CE'))
    setattr(comps_data, 'positions', ((0, 0), (263, 0)))
    comp_one, comp_two = assemble_components(comps_data)
    assert comp_one.glyphName == 'a'
    assert comp_two.glyphName == 'uni01CE'
    assert (comp_one.x, comp_one.y) == (0, 0)
    assert (comp_two.x, comp_two.y) == (263, 0)
    assert comp_one.flags == 0x204
    assert comp_two.flags == 0x4


if __name__ == "__main__":
    import sys

    sys.exit(pytest.main(sys.argv))
