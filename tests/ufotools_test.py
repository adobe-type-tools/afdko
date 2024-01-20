import plistlib
from shutil import copytree
from pathlib import Path

from afdko import ufotools as ut

from afdko.fdkutils import (
    # get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
)


TEST_UFO_FILENAME = 'ufotools_basic.ufo'


def _get_test_ufo_path():
    return get_input_path(TEST_UFO_FILENAME)


def _dict2plist(my_dict, plist_file):
    with open(plist_file, 'wb') as pl_blob:
        plistlib.dump(my_dict, pl_blob)


def _plist2dict(plist_file):
    with open(plist_file, 'rb') as pl_blob:
        pl_dict = plistlib.load(pl_blob)
    return pl_dict


def _add_entry_to_plist(kv_entry, plist_file):
    '''
    add a simple key: value pair to an existing plist file
    '''
    key, value = kv_entry
    plist_dict = _plist2dict(plist_file)
    plist_dict[key] = value
    _dict2plist(plist_dict, plist_file)


def test_parseGlyphOrder():
    ufo_path = Path(_get_test_ufo_path())
    glyph_order = ut.parseGlyphOrder(ufo_path / 'lib.plist')
    assert glyph_order == {'.notdef': 0, 'space': 1, 'a': 2, 'b': 3, 'c': 4}


def test_UFOFontData():
    ufo_path = Path(_get_test_ufo_path())
    ufd = ut.UFOFontData(ufo_path, False, '')
    assert ufd.getGlyphSrcPath('a') == str(ufo_path / 'glyphs' / 'a.glif')


def test_cleanupContentsList_missing_glyph(capsys):
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)

    # delete a glyph from glyphs directory
    glif_path = tmp_ufo_path / 'glyphs' / 'a.glif'
    glif_path.unlink()
    ut.cleanupContentsList(tmp_ufo_path / 'glyphs')
    out, err = capsys.readouterr()
    assert 'glif was missing: a, a.glif' in out


def test_cleanupContentsList_no_plist():
    temp_dir = Path(get_temp_dir_path())
    # make dummy glyph dir
    glyph_dir = temp_dir / 'glyphs.com.adobe.type.processedglyphs'
    glyph_dir.mkdir()
    # make some glif files but no contents.plist
    for glyph_name in 'xyz':
        glyph_path = glyph_dir / f'{glyph_name}.glif'
        glyph_path.touch()

    # although there is no contents.plist, this should not fail (#1606)
    ut.cleanupContentsList(glyph_dir)


def test_cleanUpGLIFFiles_extraneous_glyph(capsys):
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)
    glyphs_dir = tmp_ufo_path / 'glyphs'
    contents_plist = glyphs_dir / 'contents.plist'
    # add an unexpected .glif file
    unexpected_glif = glyphs_dir / 'x.glif'
    unexpected_glif.touch()

    changed = ut.cleanUpGLIFFiles(contents_plist, glyphs_dir)
    assert changed == 1

    out, err = capsys.readouterr()
    assert f'Removing glif file x.glif' in out


def test_cleanUpGLIFFiles_other_layer(capsys):
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)
    glyphs_dir = tmp_ufo_path / 'glyphs'
    contents_plist = glyphs_dir / 'contents.plist'

    other_glyphs_dir = tmp_ufo_path / 'otherglyphs'
    other_contents_plist = other_glyphs_dir / 'contents.plist'
    copytree(glyphs_dir, other_glyphs_dir)

    # add a glyph to the other directory which shouldn’t be there
    unexpected_glif = other_glyphs_dir / 'x.glif'
    unexpected_glif.touch()
    _add_entry_to_plist(('x', 'x.glif'), other_contents_plist)

    changed = ut.cleanUpGLIFFiles(contents_plist, other_glyphs_dir)
    assert changed == 1

    out, err = capsys.readouterr()
    assert 'Removing glif x' in out


def test_cleanUpGLIFFiles_no_plist():
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)
    glyphs_dir = tmp_ufo_path / 'glyphs'
    contents_plist = glyphs_dir / 'contents.plist'

    other_glyphs_dir = tmp_ufo_path / 'otherglyphs'
    other_contents_plist = other_glyphs_dir / 'contents.plist'
    copytree(glyphs_dir, other_glyphs_dir)

    # remove contents plist
    other_contents_plist.unlink()
    # nothing should happen
    changed = ut.cleanUpGLIFFiles(contents_plist, other_glyphs_dir)
    assert changed == 0


def test_cleanUpGLIFFiles_empty_folder():
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)
    glyphs_dir = tmp_ufo_path / 'glyphs'
    contents_plist = glyphs_dir / 'contents.plist'

    other_glyphs_dir = tmp_ufo_path / 'otherglyphs'
    other_glyphs_dir.mkdir()

    # nothing should happen
    changed = ut.cleanUpGLIFFiles(contents_plist, other_glyphs_dir)
    assert changed == 0


def test_validateLayers_empty_folder(capsys):
    ufo_path = Path(_get_test_ufo_path())
    temp_dir = Path(get_temp_dir_path())
    tmp_ufo_path = temp_dir / ufo_path.name
    copytree(ufo_path, tmp_ufo_path)
    processed_glyphs_dir = tmp_ufo_path / 'glyphs.com.adobe.type.processedglyphs'
    processed_glyphs_dir.mkdir()

    ut.validateLayers(tmp_ufo_path)
    out, err = capsys.readouterr()
    print(out)


def test_makeUFOFMNDB_empty_UFO(capsys):
    temp_dir = Path(get_temp_dir_path())
    dummy_ufo = temp_dir / 'dummy.ufo'
    fontinto_plist = dummy_ufo / 'fontinfo.plist'
    dummy_ufo.mkdir()
    _dict2plist({}, fontinto_plist)

    fmndb_path = ut.makeUFOFMNDB(dummy_ufo)
    out, err = capsys.readouterr()

    assert "UFO font is missing 'postscriptFontName'" in out
    assert "UFO font is missing 'familyName'" in out
    assert "UFO font is missing 'styleName'" in out

    with open(fmndb_path, 'r') as fmndb_blob:
        fmndb_data = fmndb_blob.read().splitlines()
    assert fmndb_data == [
        '[NoFamilyName-Regular]',
        '\tf=NoFamilyName',
        '\ts=Regular']


def test_makeUFOFMNDB_psname(capsys):
    temp_dir = Path(get_temp_dir_path())
    dummy_ufo = temp_dir / 'dummy.ufo'
    fontinto_plist = dummy_ufo / 'fontinfo.plist'
    dummy_ufo.mkdir()
    fontinfo_dict = {
        'postscriptFontName': 'Dummy-Italic'
    }
    _dict2plist(fontinfo_dict, fontinto_plist)

    fmndb_path = ut.makeUFOFMNDB(dummy_ufo)
    out, err = capsys.readouterr()

    assert "UFO font is missing 'familyName'" in out
    assert "UFO font is missing 'styleName'" in out

    with open(fmndb_path, 'r') as fmndb_blob:
        fmndb_data = fmndb_blob.read().splitlines()
    assert fmndb_data == [
        '[Dummy-Italic]',
        '\tf=Dummy',
        '\ts=Italic']


def test_makeUFOFMNDB_wholesome():
    temp_dir = Path(get_temp_dir_path())
    dummy_ufo = temp_dir / 'dummy.ufo'
    fontinto_plist = dummy_ufo / 'fontinfo.plist'
    dummy_ufo.mkdir()
    fontinfo_dict = {
        'postscriptFontName': 'Whats-Up',
        'familyName': 'Addams Family',
        'styleName': 'Narrow Extended'
    }
    _dict2plist(fontinfo_dict, fontinto_plist)

    fmndb_path = ut.makeUFOFMNDB(dummy_ufo)

    with open(fmndb_path, 'r') as fmndb_blob:
        fmndb_data = fmndb_blob.read().splitlines()
    assert fmndb_data == [
        '[Whats-Up]',
        '\tf=Addams Family',
        '\ts=Narrow Extended']
