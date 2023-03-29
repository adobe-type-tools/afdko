import glob
import logging
import os
import pytest
import subprocess

from fontTools.misc.xmlWriter import XMLWriter
from fontTools.cffLib import CFFFontSet
from fontTools.ttLib import TTFont

from differ import main as differ
from shutil import copy2, copytree  # rmtree
from afdko.fdkutils import get_temp_file_path, get_temp_dir_path
from test_utils import get_input_path, get_expected_path

from afdko.otfautohint import FontParseError
from afdko.otfautohint.__main__ import main as otfautohint, stemhist
from afdko.otfautohint.fdTools import (mergeFDDicts, parseFontInfoFile,
                                       FDDict, FontInfoParseError)
from afdko.otfautohint.autohint import ACOptions, hintFiles
from afdko.otfautohint.ufoFont import UFOFontData

DUMMYFONTS = glob.glob(get_input_path("dummy/font.[ocu][tf][fo]"))


def parse(path, glyphs=None):
    with open(path) as fp:
        data = fp.read()
    if glyphs is None:
        glyphs = []
    fddict = FDDict(0)
    fddict.setInfo("BlueFuzz", 0)
    dl = {0: fddict}
    sm, fd = parseFontInfoFile(dl, data, glyphs, 2000, -1000, "Foo")
    return sm, dl, fd


def autohint(args):
    return otfautohint(["--all", "--test"] + args)


class OTFOptions(ACOptions):
    def __init__(self, inpath, outpath, logOnly=False, readHints=True,
                 zones=None, stems=None, all_stems=None):
        super(OTFOptions, self).__init__()
        self.inputPaths = [inpath]
        self.outputPaths = [outpath]
        self.logOnly = logOnly
        self.hintAll = True
        self.verbose = False
        if zones is not None:
            self.report_zones = zones
        if stems is not None:
            self.report_stems = stems
        if all_stems is not None:
            self.report_all_stems = all_stems


class MMOptions(ACOptions):
    def __init__(self, reference, inpaths, outpaths, ref_out=None):
        super(MMOptions, self).__init__()
        self.inputPaths = inpaths
        self.outputPaths = outpaths
        self.referenceFont = reference
        self.referenceOutputPath = ref_out
        self.hintAll = True
        self.verbose = False
        self.verbose = False


# -------------
# fdTools tests
# -------------


def test_finalfont():
    path = get_input_path("fontinfo/finalfont")
    _, _, final_dict = parse(path)
    assert final_dict is not None


def test_base_token_value_is_a_list():
    path = get_input_path("fontinfo/base_token_list")
    _, font_dicts, _ = parse(path)
    assert len(font_dicts) > 0


# def test_no_dominant_h_or_v(caplog):
def test_no_dominant_h_or_v():
    path = get_input_path("fontinfo/no_dominant_h_v")
    _, font_dicts, _ = parse(path)
    assert len(font_dicts) > 0
    # msgs = [r.getMessage() for r in caplog.records]
    # assert "The FDDict UPPERCASE in fontinfo has no DominantH value" in msgs
    # assert "The FDDict UPPERCASE in fontinfo has no DominantV value" in msgs


def test_bluefuzz_and_fontname():
    path = get_input_path("fontinfo/bluefuzz_fontname")
    _, font_dicts, _ = parse(path, ["A", "B", "C", "D"])
    assert len(font_dicts) > 1
    assert font_dicts[1].BlueFuzz == 10
    assert font_dicts[1].FontName == "Bar"


@pytest.mark.parametrize('path', glob.glob(get_input_path("fontinfo/bad_*")))
def test_bad_fontinfo(path):
    with pytest.raises(FontInfoParseError):
        parse(path)


@pytest.mark.parametrize("attributes", [
    {"CapOvershoot": 5, "CapHeight": 5,
     "AscenderOvershoot": 0, "AscenderHeight": 8},
    {"BaselineOvershoot": None},
    {"BaselineOvershoot": 10},
    {"CapOvershoot": 0},
    {"AscenderHeight": -10, "AscenderOvershoot": 0},
    {"AscenderHeight": 0, "AscenderOvershoot": -10},
    {"Baseline5Overshoot": 10, "Baseline5": 0},
])
def test_fddict_bad_zone(attributes):
    fddict = FDDict(0)
    fddict.setInfo("BlueFuzz", 0)
    fddict.setInfo("BaselineYCoord", 0)
    fddict.setInfo("BaselineOvershoot", -10)

    for key in attributes:
        setattr(fddict, key, attributes[key])

    with pytest.raises(FontInfoParseError):
        fddict.buildBlueLists()


def test_merge_empty_fddicts():
    mergeFDDicts([FDDict(0), FDDict(1)])


@pytest.mark.parametrize("stemdict", ["DominantH", "DominantV"])
def test_merge_fddicts_with_stemdicts(stemdict):
    fddict1 = FDDict(1)
    fddict2 = FDDict(2)

    for i, fddict in enumerate([fddict1, fddict2]):
        fddict.setInfo("BlueFuzz", 0)
        fddict.setInfo("BaselineYCoord", 0)
        fddict.setInfo("BaselineOvershoot", -10)
        fddict.setInfo(stemdict, [str(i * 10), str(i * 20)])
        fddict.buildBlueLists()

    mergeFDDicts([fddict1, fddict2])

# ===
# CLI
# ---


@pytest.mark.parametrize("path", DUMMYFONTS)
def test_basic(path):
    # the input font is modified in-place, make a temp copy first
    if os.path.isdir(path):
        temp_font_path = os.path.join(get_temp_dir_path(),
                                      os.path.basename(path))
        copytree(path, temp_font_path)
    else:
        temp_font_path = get_temp_file_path()
        copy2(path, temp_font_path)
    autohint([temp_font_path])


@pytest.mark.parametrize("path", DUMMYFONTS)
def test_outpath(path):
    out_path = get_temp_file_path()
    autohint([path, '-o', out_path])


def test_multi_outpath(tmpdir):
    """Test handling multiple output paths."""
    paths = sorted(glob.glob(get_input_path("dummy/mm0/*.ufo")))
    # the reference font is modified in-place, make a temp copy first
    out_dir = get_temp_dir_path()
    out_paths = [os.path.join(out_dir, os.path.basename(p)) for p in paths]
    autohint(['-r', paths[0]] + paths[1:] +
             ['-b', out_paths[0], '-o'] + out_paths[1:])


def test_multi_outpath_unequal(tmpdir):
    """Test that we exit if output paths don't match number of input paths."""
    paths = sorted(glob.glob(get_input_path("dummy/mm0/*.ufo")))
    # the reference font is modified in-place, make a temp copy first
    out_dir = get_temp_dir_path()
    out_paths = [os.path.join(out_dir, os.path.basename(p)) for p in paths][1:]

    with pytest.raises(SystemExit):
        autohint(['-r', paths[0]] + paths[1:] +
                 ['-b', out_paths[0], '-o'] + out_paths[1:])


def test_multi_different_formats(tmpdir):
    """Test that we exit if input paths are of different formats."""
    ufos = sorted(glob.glob(get_input_path("dummy/mm0/*.ufo")))
    otfs = sorted(glob.glob(get_input_path("dummy/mm0/*.otf")))
    # the reference font is modified in-place, make a temp copy first
    out_dir = get_temp_dir_path()
    out_paths = [os.path.join(out_dir, os.path.basename(p)) for p in ufos]

    with pytest.raises(SystemExit):
        autohint(['-r', otfs[0]] + ufos[1:] +
                 ['-b', out_paths[0], '-o'] + out_paths[1:])


def test_multi_reference_is_input(tmpdir):
    """Test that we exit if reference font is also one of the input paths."""
    paths = sorted(glob.glob(get_input_path("dummy/mm0/*.ufo")))
    # the reference font is modified in-place, make a temp copy first
    out_dir = get_temp_dir_path()
    out_paths = [os.path.join(out_dir, os.path.basename(p)) for p in paths]
    with pytest.raises(SystemExit):
        autohint(['-r', paths[0]] + paths +
                 ['-b', out_paths[0], '-o'] + out_paths[1:])


def test_multi_reference_is_duplicated(tmpdir):
    """Test that we exit if one of the input paths is duplicated."""
    paths = sorted(glob.glob(get_input_path("dummy/mm0/*.ufo")))
    # the reference font is modified in-place, make a temp copy first
    out_dir = get_temp_dir_path()
    out_paths = [os.path.join(out_dir, os.path.basename(p)) for p in paths]
    with pytest.raises(SystemExit):
        autohint(['-r', paths[0]] + paths[1:] +
                 [paths[1], '-b', out_paths[0], '-o'] + out_paths[1:])


@pytest.mark.parametrize('path', glob.glob(get_input_path("dummy/font.p*")))
def test_type1_supported(path):
    out_path = get_temp_file_path()
    autohint([path, '-o', out_path])


@pytest.mark.parametrize("glyphs", [
    'a,b,c',      # Glyph List
    'a-z',        # Glyph range
    'FOO,BAR,a',  # Some glyphs in the list do not exist.
])
def test_glyph_list(glyphs):
    path = get_input_path("dummy/font.ufo")
    out_path = get_temp_dir_path()
    autohint([path, '-o', out_path, '-g', glyphs])


@pytest.mark.parametrize("glyphs", [
    '/0,/1,/2',
    '/0-/10',
    'cid0,cid1,cid2',
    'cid0-cid10',
])
def test_cid_glyph_list(glyphs):
    path = get_input_path("CID/font.otf")
    out_path = get_temp_file_path()
    autohint([path, '-o', out_path, '-g', glyphs])


@pytest.mark.parametrize("glyphs", [
    'a,b,c',
    'a-z',
])
def test_exclude_glyph_list(glyphs, tmpdir):
    path = get_input_path("dummy/font.ufo")
    out_path = get_temp_dir_path()
    autohint([path, '-o', out_path, '-x', glyphs])


@pytest.mark.parametrize("glyphs", [
    'FOO,BAR',
    'FOO-BAR',
    'FOO-a',
    'a-BAR',
])
def test_missing_glyph_list(glyphs, tmpdir):
    path = get_input_path("dummy/font.ufo")
    out_path = get_temp_dir_path()
    with pytest.raises(FontParseError):
        autohint([path, '--traceback', '-o', out_path, '-g', glyphs])


@pytest.mark.parametrize("path", [get_input_path("dummy/fontinfo"),
                                  get_input_path('')])
def test_unsupported_format(path):
    with pytest.raises(SystemExit):
        autohint([path])


def test_missing_cff_table1():
    path = get_input_path("dummy/nocff.otf")
    out_path = get_temp_file_path()
    assert autohint([path, '-o', out_path]) == 1


def test_missing_cff_table2():
    path = get_input_path("dummy/nocff.otf")
    out_path = get_temp_file_path()
    with pytest.raises(FontParseError):
        autohint([path, '-o', out_path, '--traceback'])


@pytest.mark.parametrize("option,argument", [
    ("--exclude-glyphs-file", "glyphs.txt"),
    ("--fontinfo-file", "fontinfo"),
    ("--glyphs-file", "glyphs.txt"),
])
@pytest.mark.parametrize("path", [get_input_path("dummy/font.ufo"),
                                  get_input_path("dummy/font.otf")])
def test_option(path, option, argument):
    out_path = get_temp_file_path()
    argument = get_input_path("dummy/%s" % argument)

    autohint([path, '-o', out_path, option, argument])


@pytest.mark.parametrize("option", [
    "--allow-changes",
    "--decimal",
    "--no-flex",
    "--no-hint-sub",
    "--no-zones-stems",
    "--print-list-fddict",
    "--report-only",
    "--verbose",
    "--write-to-default-layer",
    "-vv",
])
@pytest.mark.parametrize("path", [get_input_path("dummy/font.ufo"),
                                  get_input_path("dummy/font.otf")])
def test_argumentless_option(path, option, tmpdir):
    out_path = get_temp_file_path()
    autohint([path, '-o', out_path, option])


def test_print_fddict():
    fontpath = get_input_path("dummy/font.otf")
    fipath = get_input_path("dummy/fontinfo_fdd")
    exp_path = get_expected_path('dummy/print_dflt_fddict_expected.txt')

    with open(exp_path, 'r') as exp_f:
        expected = exp_f.read()
        with pytest.raises(SystemExit) as wrapped_exc:
            autohint([fontpath,
                      '--print-dflt-fddict',
                      '--fontinfo-file',
                      fipath])
            assert str(wrapped_exc) == expected


@pytest.mark.parametrize("option", [
    "--doc-fontinfo",
    "--help",
    "--info",
    "--version",
])
def test_doc_option(option):
    with pytest.raises(SystemExit) as e:
        autohint([option])
    assert e.type == SystemExit
    assert e.value.code == 0


def test_no_fddict():
    path = get_input_path("dummy/mm0/font0.ufo")
    out_path = get_temp_dir_path()

    autohint([path, '-o', out_path, "--print-list-fddict"])


@pytest.mark.parametrize("path", [get_input_path("dummy/font.ufo"),
                                  get_input_path("dummy/font.otf"),
                                  get_input_path("dummy/font.cff")])
def test_overwrite_font(path, tmpdir):
    out_path = get_temp_file_path()
    autohint([path, '-o', out_path, '-g', 'a,b,c'])
    autohint([path, '-o', out_path, '-g', 'a,b,c'])


def test_invalid_input_path():
    path = get_temp_file_path()
    with pytest.raises(SystemExit):
        autohint([path])


@pytest.mark.parametrize("args", [
    pytest.param(['-z'], id="report_zones"),
    pytest.param([], id="report_stems"),
    pytest.param(['-a'], id="report_stems,all_stems"),
    pytest.param(['-g', 'a-z,A-Z,zero-nine'], id="report_stems,glyphs"),
])
def test_stemhist(args):
    path = get_input_path("dummy/font.otf")
    exp_path = get_expected_path("dummy/font.otf")
    out_path = get_temp_file_path()

    stemhist([path, '-o', out_path] + args)

    if '-z' in args:
        suffixes = ['.top.txt', '.bot.txt']
    else:
        suffixes = ['.hstm.txt', '.vstm.txt']

    for suffix in suffixes:
        exp_suffix = suffix
        if '-a' in args:
            exp_suffix = '.all' + suffix
        if '-g' in args:
            g = args[args.index('-g') + 1]
            exp_suffix = '.' + g + exp_suffix
        assert differ([exp_path + exp_suffix, out_path + suffix, '-l', '1'])


@pytest.mark.parametrize("path", DUMMYFONTS)
def test_outpath_order(path):
    """ e.g. psautohint -o outfile infile"""
    out_path = get_temp_file_path()
    autohint(['-o', out_path, path])


def test_multi_order():
    """ e.g. psautohint -o outfile1 outfile2 infile1 infile2"""
    path1 = get_input_path("dummy/font.ufo")
    path2 = get_input_path("dummy/big_glyph.ufo")
    out_path1 = get_temp_file_path()
    out_path2 = get_temp_file_path()

    autohint(['-o', out_path1, out_path2, path1, path2])


def test_multi_order_unequal():
    """ e.g. psautohint -o outfile1 outfile2 infile"""
    path1 = get_input_path("dummy/font.ufo")
    out_path1 = get_temp_file_path()
    out_path2 = get_temp_file_path()

    with pytest.raises(SystemExit):
        autohint(['-o', out_path1, out_path2, path1])


def test_lack_of_input_raises():
    with pytest.raises(SystemExit):
        autohint(['--report-only'])


@pytest.mark.parametrize("zones,stems,all_stems", [
    pytest.param(True, False, False, id="report_zones"),
    pytest.param(False, True, False, id="report_stems"),
    pytest.param(False, True, True, id="report_stems,all_stems"),
])
def test_otf(zones, stems, all_stems):
    path = get_input_path("dummy/font.otf")
    exp_path = get_expected_path("dummy/font.otf")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path, True, False, zones, stems, all_stems)

    hintFiles(options)

    if zones:
        suffixes = ['.top.txt', '.bot.txt']
    else:
        suffixes = ['.hstm.txt', '.vstm.txt']

    for suffix in suffixes:
        exp_suffix = suffix
        if all_stems:
            exp_suffix = '.all' + suffix
        assert differ([exp_path + exp_suffix, out_path + suffix, '-l', '1'])


# ---
# UFO
# ---


def test_incomplete_glyphorder():
    path = get_input_path("dummy/incomplete_glyphorder.ufo")
    font = UFOFontData(path, False, True)

    assert len(font.getGlyphList()) == 95
    assert "ampersand" in font.getGlyphList()


# -----
# MM/VF
# -----


@pytest.mark.parametrize("otf", glob.glob(get_input_path("vf_tests/*.otf")))
def test_vfotf(otf):
    out_path = get_temp_file_path()
    options = MMOptions(None, [otf], [out_path])
    options.allow_no_blues = True
    hintFiles(options)

    xmlFiles = []

    for path in (otf, out_path):
        font = TTFont(path)
        assert "CFF2" in font
        oname = get_temp_file_path() + ".xml"
        writer = XMLWriter(oname)
        font["CFF2"].toXML(writer, font)
        writer.close()
        xmlFiles.append(oname)
    assert differ(xmlFiles)


def test_sparse_mmotf():
    paths = sorted(glob.glob(get_input_path("sparse_masters/*.otf")))
    exp_paths = sorted(glob.glob(get_expected_path("sparse_masters/*.otf")))
    # the reference font is modified in-place, make a temp copy first
    # MasterSet_Kanji-w0.00.otf has to be the reference font.
    tmpdir = get_temp_dir_path()
    reference = paths[0]
    inpaths = paths[1:]
    ref_out = str(os.path.join(tmpdir, os.path.basename(reference)) + ".out")
    outpaths = [str(os.path.join(tmpdir, os.path.basename(p)) + ".out")
                for p in inpaths]

    options = MMOptions(reference, inpaths, outpaths, ref_out)
    options.allow_no_blues = True
    hintFiles(options)

    for ref, out in zip(exp_paths, [ref_out] + outpaths):
        xmlFiles = []
        for path in (ref, out):
            font = TTFont(path)
            assert "CFF " in font
            oname = get_temp_file_path() + ".xml"
            writer = XMLWriter(oname)
            font["CFF "].toXML(writer, font)
            writer.close()
            xmlFiles.append(oname)

        assert differ(xmlFiles)

# -------
# hinting
# -------


def test_cff():
    cff = get_input_path("dummy/font.cff")
    out_path = get_temp_file_path()
    options = OTFOptions(cff, out_path)
    hintFiles(options)

    xmlFiles = []
    for path in (cff, out_path):
        font = CFFFontSet()
        opath = get_temp_file_path()
        writer = XMLWriter(opath)
        with open(path, "rb") as fp:
            font.decompile(fp, None)
            font.toXML(writer)
        xmlFiles.append(opath)
        writer.close()

    assert differ(xmlFiles)


@pytest.mark.parametrize("path", glob.glob(get_input_path("dummy/font.p*")))
def test_type1_supported_hint(path):
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)
    if os.path.basename(path) == 'font.ps':
        options.ignoreFontinfo = True
    hintFiles(options)

    path_dump = get_temp_file_path()
    out_dump = get_temp_file_path()
    subprocess.check_call(["tx", "-dump", "-6", path, path_dump])
    subprocess.check_call(["tx", "-dump", "-6", out_path, out_dump])
    assert differ([path_dump, out_dump])


def test_unsupported_format_hintfiles():
    path = get_input_path("dummy/fontinfo")
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


def test_missing_cff_table_hintfiles():
    path = get_input_path("dummy/nocff.otf")
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


def test_ufo_write_to_default_layer():
    path = get_input_path("dummy/defaultlayer.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.writeToDefaultLayer = True
    hintFiles(options)

    assert differ([path, out_path])


@pytest.mark.parametrize("path",
                         glob.glob(get_input_path("dummy/*_metainfo.ufo")))
def test_ufo_bad(path):
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


@pytest.mark.parametrize("path", glob.glob(get_input_path("dummy/bad_*.p*")))
def test_type1_bad(path):
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


def test_counter_glyphs():
    path = get_input_path("dummy/font.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.vCounterGlyphs = {"m": True, "M": True, "T": True}
    hintFiles(options)


def test_seac_op(caplog):
    path = get_input_path("dummy/seac.otf")
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    msgs = [r.getMessage() for r in caplog.records]
    assert "Skipping Aacute: can't process SEAC composite glyphs." in msgs


def test_mute_tx_msgs(capfd):
    path = get_input_path("dummy/nohints.pfa")
    out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    captured = capfd.readouterr()
    assert "(cfw) unhinted <.notdef>" not in captured.err


@pytest.mark.parametrize("path",
                         glob.glob(get_input_path("dummy/bad_privatedict_*")))
def test_bad_privatedict(path):
    if os.path.isdir(path):
        out_path = get_temp_dir_path()
    else:
        out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


@pytest.mark.parametrize("path",
                         glob.glob(get_input_path("dummy/bad_privatedict_*")))
def test_bad_privatedict_accept(path):
    """Same as above test, but PrivateDict is accepted because of
       `allow_no_blues` option."""
    if os.path.isdir(path):
        out_path = get_temp_dir_path()
    else:
        out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)
    options.allow_no_blues = True

    hintFiles(options)


@pytest.mark.parametrize("path",
                         glob.glob(get_input_path("dummy/ok_privatedict_*")))
def test_ok_privatedict_accept(path, caplog):
    if os.path.isdir(path):
        out_path = get_temp_dir_path()
    else:
        out_path = get_temp_file_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    msg = "There is no value or 0 value for Dominant"
    assert any(r.getMessage().startswith(msg) for r in caplog.records)


def test_hashmap_glyph_changed(caplog):
    path = get_input_path("dummy/hashmap_glyph_changed.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)
    msgs = [r.getMessage() for r in caplog.records]
    assert "Glyph 'a' has been edited. You must first run 'checkOutlines' " \
           "before running 'autohint'. Skipping." in msgs


def test_hashmap_processed_no_autohint(caplog):
    path = get_input_path("dummy/hashmap_processed_no_autohint.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    assert not differ([path, out_path])


def test_hashmap_no_version(caplog):
    caplog.set_level(logging.INFO)
    path = get_input_path("dummy/hashmap_no_version.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)
    msgs = [r.getMessage() for r in caplog.records]
    assert "Updating hash map: was older version" in msgs

    assert not differ([path, out_path])


@pytest.mark.skipif(True, reason="XXX This doesn't work when no_version runs")
def test_hashmap_old_version(caplog):
    caplog.set_level(logging.INFO)
    path = get_input_path("dummy/hashmap_old_version.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)
    msgs = [r.getMessage() for r in caplog.records]
    assert "Updating hash map: was older version" in msgs

    assert not differ([path, out_path])


def test_hashmap_new_version(caplog):
    path = get_input_path("dummy/hashmap_new_version.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    with pytest.raises(FontParseError):
        hintFiles(options)


def test_hashmap_advance():
    path = get_input_path("dummy/hashmap_advance.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    assert differ([path, out_path])


def test_hashmap_dflt_layer():
    path = get_input_path("dummy/hashmap_dflt_layer.ufo")
    rslt = get_expected_path("dummy/hashmap_dflt_layer_hinted.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.writeToDefaultLayer = True

    hintFiles(options)

    assert differ([rslt, out_path])


def test_hashmap_dflt_layer_rehint():
    path = get_input_path("dummy/hashmap_dflt_layer.ufo")
    rslt = get_expected_path("dummy/hashmap_dflt_layer_rehinted.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.writeToDefaultLayer = False
    hintFiles(options)
    options2 = OTFOptions(out_path, out_path)
    options2.writeToDefaultLayer = True
    hintFiles(options2)

    assert differ([rslt, out_path])


def test_hashmap_transform():
    path = get_input_path("dummy/hashmap_transform.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    assert differ([path, out_path])


def test_hashmap_unnormalized_floats():
    path = get_input_path("dummy/hashmap_unnormalized_floats.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)

    hintFiles(options)

    assert differ([path, out_path])


def test_decimals_ufo():
    path = get_input_path("dummy/decimals.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.roundCoords = False

    hintFiles(options)

    assert differ([path, out_path])


def test_decimals_otf():
    cff = get_input_path("dummy/decimals.otf")
    out_path = get_temp_file_path()
    options = OTFOptions(cff, out_path)
    options.roundCoords = False

    hintFiles(options)

    xmlFiles = []
    for path in (cff, out_path):
        font = TTFont(path)
        assert "CFF " in font
        opath = get_temp_file_path()
        writer = XMLWriter(opath)
        font["CFF "].toXML(writer, font)
        writer.close()
        xmlFiles.append(opath)

    assert differ(xmlFiles)


def test_layers():
    path = get_input_path("dummy/layers.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.allow_no_blues = True

    hintFiles(options)

    assert differ([path, out_path])


def test_big_glyph():
    path = get_input_path("dummy/big_glyph.ufo")
    out_path = get_temp_dir_path()
    options = OTFOptions(path, out_path)
    options.explicitGlyphs = True

    hintFiles(options)

    assert differ([path, out_path])
