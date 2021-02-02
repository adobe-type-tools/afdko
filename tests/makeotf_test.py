import os
import pytest
from shutil import copy2, copytree, rmtree
import subprocess
import sys

from afdko.makeotf import (
    checkIfVertInFeature,
    getOptions,
    MakeOTFParams,
    getSourceGOADBData,
    readOptionFile,
    writeOptionsFile,
    kMOTFOptions,
    kOptionNotSeen,
)
from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
    get_expected_path,
    get_font_revision,
    generate_ttx_dump,
    font_has_table,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

# The following ensures that the current working directory is consistently set.
# This is necessary to avoid a problem that was discovered when running tests
# with cibuildwheel, which initiates the pytest command from root, which causes
# some minor discrepancies in relative vs absolute paths.
test_dir = os.path.dirname(os.path.realpath(__file__))
afdko_dir = os.path.join(test_dir, "..")
os.chdir(afdko_dir)

TOOL = 'makeotf'
CMD = ['-t', TOOL]

T1PFA_NAME = 't1pfa.pfa'
UFO2_NAME = 'ufo2.ufo'
UFO3_NAME = 'ufo3.ufo'
CID_NAME = 'cidfont.ps'
TTF_NAME = 'font.ttf'
OTF_NAME = 'SourceSans-Test.otf'

DATA_DIR = os.path.join(os.path.split(__file__)[0], TOOL + '_data')
TEMP_DIR = os.path.join(DATA_DIR, 'temp_output')


xfail_win = pytest.mark.xfail(
    sys.platform == 'win32',
    reason="Console's encoding is not UTF-8 ?")


xfail_py3_win = pytest.mark.xfail(
    sys.version_info >= (3, 0) and sys.platform == 'win32',
    reason="?")


def setup_module():
    """
    Create the temporary output directory
    """
    rmtree(TEMP_DIR, ignore_errors=True)
    os.mkdir(TEMP_DIR)


def teardown_module():
    """
    teardown the temporary output directory
    """
    rmtree(TEMP_DIR)


# -----
# Tests
# -----

def test_exit_no_option():
    # It's valid to run 'makeotf' without using any options,
    # but if a default-named font file is NOT found in the
    # current directory, the tool exits with an error
    with pytest.raises(subprocess.CalledProcessError) as err:
        subprocess.check_call([TOOL])
    assert err.value.returncode == 2


@pytest.mark.parametrize('arg', ['-v', '-h', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-j', '--bogus'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 2


@pytest.mark.parametrize('arg, input_filename, ttx_filename', [
    ([], T1PFA_NAME, 't1pfa-dev.ttx'),
    ([], UFO2_NAME, 'ufo2-dev.ttx'),
    ([], UFO3_NAME, 'ufo3-dev.ttx'),
    ([], CID_NAME, 'cidfont-dev.ttx'),
    ([], TTF_NAME, 'ttf-dev.ttx'),
    (['r'], T1PFA_NAME, 't1pfa-rel.ttx'),
    (['r'], UFO2_NAME, 'ufo2-rel.ttx'),
    (['r'], UFO3_NAME, 'ufo3-rel.ttx'),
    (['r'], CID_NAME, 'cidfont-rel.ttx'),
    (['r', 'gf'], TTF_NAME, 'ttf-rel.ttx'),
])
def test_input_formats(arg, input_filename, ttx_filename):
    if 'gf' in arg:
        arg.append(f'_{get_input_path("GOADB.txt")}')
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'o', f'_{actual_path}'] + arg)
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])


@pytest.mark.parametrize('args, ttx_fname', [
    ([], 'font_dev'),
    (['r'], 'font_rel'),
])
def test_build_font_and_check_messages(args, ttx_fname):
    actual_path = get_temp_file_path()
    expected_msg_path = get_expected_path(f'{ttx_fname}_output.txt')
    ttx_filename = f'{ttx_fname}.ttx'
    stderr_path = runner(CMD + [
        '-s', '-e', '-o', 'f', f'_{get_input_path("font.pfa")}',
                          'o', f'_{actual_path}'] + args)
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])
    assert differ([expected_msg_path, stderr_path,
                   '-r', r'^Built (development|release) mode font'])


def test_getSourceGOADBData():
    ttf_path = get_input_path('font.ttf')
    assert getSourceGOADBData(ttf_path) == [['.notdef', '.notdef', ''],
                                            ['a', 'a', 'uni0041'],
                                            ['g2', 'g2', '']]


@xfail_win
@pytest.mark.parametrize('input_filename', [
    T1PFA_NAME, UFO2_NAME, UFO3_NAME, CID_NAME])
def test_path_with_non_ascii_chars_bug222(input_filename):
    temp_dir = get_temp_dir_path('á意ê  ï薨õ 巽ù')
    os.makedirs(temp_dir)
    assert os.path.isdir(temp_dir)
    input_path = get_input_path(input_filename)
    temp_path = os.path.join(temp_dir, input_filename)
    if os.path.isdir(input_path):
        copytree(input_path, temp_path)
    else:
        copy2(input_path, temp_path)
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.exists(expected_path) is False
    runner(CMD + ['-o', 'f', f'_{temp_path}'])
    assert os.path.isfile(expected_path)


def test_font_with_hash_bug239():
    input_path = get_input_path('bug239/font.ufo')
    output_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{input_path}',
                        'o', f'_{output_path}'])
    assert font_has_table(output_path, 'CFF ')


def test_font_with_outdated_hash_bug239():
    input_path = get_input_path('bug239/font_outdated_hash.ufo')
    output_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-o', 'f', f'_{input_path}',
                            'o', f'_{output_path}'])
    assert err.value.returncode == 2


@xfail_py3_win
@pytest.mark.parametrize('input_filename', [UFO2_NAME, UFO3_NAME])
def test_ufo_with_trailing_slash_bug280(input_filename):
    # makeotf will now save the OTF alongside the UFO instead of inside of it
    ufo_path = get_input_path(input_filename)
    temp_dir = get_temp_dir_path()
    tmp_ufo_path = os.path.join(temp_dir, input_filename)
    copytree(ufo_path, tmp_ufo_path)
    runner(CMD + ['-o', 'f', f'_{tmp_ufo_path}{os.sep}'])
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [
    T1PFA_NAME, UFO2_NAME, UFO3_NAME, CID_NAME])
def test_output_is_folder_only_bug281(input_filename):
    # makeotf will output a default-named font to the folder
    input_path = get_input_path(input_filename)
    temp_dir = get_temp_dir_path()
    expected_path = os.path.join(temp_dir, OTF_NAME)
    assert os.path.exists(expected_path) is False
    runner(CMD + ['-o', 'f', f'_{input_path}',
                        'o', f'_{temp_dir}'])
    assert os.path.isfile(expected_path)


@pytest.mark.parametrize('input_filename', [
    't1pfa-noPSname.pfa',
    'ufo2-noPSname.ufo',
    'ufo3-noPSname.ufo',
    'cidfont-noPSname.ps',
])
def test_no_postscript_name_bug282(input_filename):
    # makeotf will fail for both UFO and Type 1 inputs
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-o', 'f', f'_{input_filename}'])
    assert err.value.returncode == 2


@pytest.mark.parametrize('fea_filename, result', [
    (None, 0),  # No feature path at...
    ('missing_file', 0),  # No feature path at...
    ('feat0.fea', 0),
    ('feat1.fea', 1),
    ('feat2.fea', 0),  # Could not find the include file...
    ('feat3.fea', 0),  # Could not find the include file...
    ('feat4.fea', 1),
])
def test_find_vert_feature_bug148(fea_filename, result):
    fea_path = None
    if fea_filename:
        input_dir = get_input_path('bug148')
        fea_path = os.path.join(input_dir, fea_filename)
    assert checkIfVertInFeature(fea_path) == result


@pytest.mark.parametrize('args, result', [
    # 'result' corresponds to the values of the
    # options 'ReleaseMode' and 'SuppressHintWarnings'
    ([], (None, None)),
    (['-r'], ('true', None)),
    (['-shw'], (None, None)),
    (['-nshw'], (None, 'true')),
    (['-r', '-shw'], ('true', None)),
    (['-r', '-nshw'], ('true', 'true')),
    # makeotf option parsing has no mechanism for mutual exclusivity,
    # so the last option typed on the command line wins
    (['-shw', '-nshw'], (None, 'true')),
    (['-nshw', '-shw'], (None, None)),
])
def test_options_shw_nshw_bug457(args, result):
    params = MakeOTFParams()
    getOptions(params, args)
    assert params.opt_ReleaseMode == result[0]
    assert params.opt_SuppressHintWarnings == result[1]


@pytest.mark.parametrize('args, result', [
    # 'result' corresponds to the values of the
    # options 'MacCmapScriptID' and 'MacCmapLanguageID'
    # CJK MacCmapScriptIDs: Japan/1, CN/2, Korea/3, GB/25
    ([], (None, None)),
    (['-cs', '1'], ('1', None)),
    (['-cl', '2'], (None, '2')),
    (['-cs', '4', '-cl', '5'], ('4', '5')),
])
def test_options_cs_cl_bug459(args, result):
    params = MakeOTFParams()
    getOptions(params, args)
    assert params.opt_MacCmapScriptID == result[0]
    assert params.opt_MacCmapLanguageID == result[1]


def test_writeOptionsFile():
    actual_path = get_temp_file_path()
    expected_path = get_expected_path('proj_write.txt')
    params = MakeOTFParams()
    params.currentDir = os.path.dirname(actual_path)
    params.opt_InputFontPath = 'font.pfa'
    params.opt_OutputFontPath = 'fôñt.otf'
    params.opt_kSetfsSelectionBitsOn = [7]
    params.opt_kSetfsSelectionBitsOff = [9, 8]
    params.opt_ConvertToCID = 'true'
    params.opt_kSetOS2Version = 3
    params.verbose = True
    # set options's sort order
    kMOTFOptions['InputFontPath'][0] = 1
    kMOTFOptions['OutputFontPath'][0] = kOptionNotSeen + 1
    kMOTFOptions['kSetfsSelectionBitsOn'][0] = 30
    writeOptionsFile(params, actual_path)
    assert differ([expected_path, actual_path])


def test_readOptionFile():
    INPUT_FONT_NAME = 'font.pfa'
    OUTPUT_FONT_NAME = 'font.otf'
    proj_path = get_input_path('proj.txt')
    abs_font_dir_path = os.path.dirname(proj_path)
    params = MakeOTFParams()
    assert params.fontDirPath == '.'

    params.currentDir = os.path.dirname(proj_path)
    assert readOptionFile(proj_path, params, 1) == (True, 7)
    assert params.fontDirPath == '.'
    assert params.opt_InputFontPath == INPUT_FONT_NAME
    assert params.opt_OutputFontPath == OUTPUT_FONT_NAME
    assert params.opt_ConvertToCID == 'true'
    assert params.opt_kSetfsSelectionBitsOff == '[8, 9]'
    assert params.opt_kSetfsSelectionBitsOn == '[7]'
    assert params.seenOS2v4Bits == [1, 1, 1]
    assert params.opt_UseOldNameID4 is None

    params.currentDir = os.getcwd()
    font_dir_path = os.path.relpath(abs_font_dir_path, params.currentDir)
    input_font_path = os.path.join(font_dir_path, INPUT_FONT_NAME)
    output_font_path = os.path.join(font_dir_path, OUTPUT_FONT_NAME)
    assert readOptionFile(proj_path, params, 1) == (True, 7)
    assert params.fontDirPath == os.path.normpath(font_dir_path)
    assert params.opt_InputFontPath == os.path.normpath(input_font_path)
    assert params.opt_OutputFontPath == os.path.normpath(output_font_path)


def test_readOptionFile_abspath():
    proj_path = get_input_path('proj2.txt')
    params = MakeOTFParams()

    root = os.path.abspath(os.sep)
    params.currentDir = os.path.join(root, 'different_dir')
    assert readOptionFile(proj_path, params, 1) == (False, 3)
    assert params.fontDirPath.startswith(root)
    assert params.opt_InputFontPath.startswith(root)
    assert params.opt_OutputFontPath.startswith(root)


@pytest.mark.parametrize('cur_dir_str', [
    'different_dir',
    './different_dir',
    '../different_dir',
])
def test_readOptionFile_relpath(cur_dir_str):
    INPUT_FONT_NAME = 'font.pfa'
    OUTPUT_FONT_NAME = 'font.otf'
    proj_path = get_input_path('proj2.txt')
    abs_font_dir_path = os.path.dirname(proj_path)
    params = MakeOTFParams()

    if '/' in cur_dir_str:
        # flip the slashes used in the test's input string
        params.currentDir = os.path.normpath(cur_dir_str)
    else:
        params.currentDir = cur_dir_str

    font_dir_path = os.path.relpath(abs_font_dir_path, params.currentDir)

    if cur_dir_str.startswith('..') and os.path.dirname(os.getcwd()) == os.sep:
        # the project is inside a folder located at the root level;
        # remove the two dots at the start of the path, otherwise
        # testing input '../different_dir' will fail.
        font_dir_path = font_dir_path[2:]

    input_font_path = os.path.join(font_dir_path, INPUT_FONT_NAME)
    output_font_path = os.path.join(font_dir_path, OUTPUT_FONT_NAME)
    assert readOptionFile(proj_path, params, 1) == (False, 3)
    assert params.fontDirPath == os.path.normpath(font_dir_path)
    assert params.opt_InputFontPath == os.path.normpath(input_font_path)
    assert params.opt_OutputFontPath == os.path.normpath(output_font_path)


def test_readOptionFile_filenotfound():
    proj_path = get_input_path('notafile')
    params = MakeOTFParams()
    params.currentDir = os.getcwd()
    assert readOptionFile(proj_path, params, 0) == (True, 0)


@pytest.mark.parametrize('args, input_filename, ttx_filename', [
    (['r'], T1PFA_NAME, 't1pfa-cmap.ttx'),
    (['r', 'cs', '_1'], T1PFA_NAME, 't1pfa-cmap_cs1.ttx'),
    (['r', 'cl', '_2'], T1PFA_NAME, 't1pfa-cmap_cl2.ttx'),
    (['r'], UFO2_NAME, 'ufo2-cmap.ttx'),
    (['r', 'cs', '_4'], UFO2_NAME, 'ufo2-cmap_cs4.ttx'),
    (['r', 'cl', '_5'], UFO2_NAME, 'ufo2-cmap_cl5.ttx'),
    (['r'], UFO3_NAME, 'ufo3-cmap.ttx'),
    (['r', 'cs', '_4'], UFO3_NAME, 'ufo3-cmap_cs4.ttx'),
    (['r', 'cl', '_5'], UFO3_NAME, 'ufo3-cmap_cl5.ttx'),
    (['r'], CID_NAME, 'cidfont-cmap.ttx'),
    (['r', 'cs', '_2'], CID_NAME, 'cidfont-cmap_cs2.ttx'),
    (['r', 'cl', '_3'], CID_NAME, 'cidfont-cmap_cl3.ttx'),
])
def test_build_options_cs_cl_bug459(args, input_filename, ttx_filename):
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'o', f'_{actual_path}'] + args)
    actual_ttx = generate_ttx_dump(actual_path, ['cmap'])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


@pytest.mark.parametrize('args, font, fontinfo', [
    (['cn'], 'type1', ''),
    (['cn'], 'no_notdef', ''),
    (['cn'], 'blank_notdef', ''),
    (['cn'], 'notdef_not1st', ''),
    (['cn', 'fi'], 'type1', 'fi'),
    (['cn', 'fi'], 'type1', 'fi2'),
    (['cn', 'fi'], 'no_notdef', 'fi'),
    (['cn', 'fi'], 'no_notdef', 'fi2'),
    (['cn', 'fi'], 'blank_notdef', 'fi'),
    (['cn', 'fi'], 'blank_notdef', 'fi2'),
    (['cn', 'fi'], 'notdef_not1st', 'fi'),
    (['cn', 'fi'], 'notdef_not1st', 'fi2'),
])
def test_cid_keyed_cff_bug470(args, font, fontinfo):
    if 'fi' in args:
        fontinfo_file = f'bug470/{fontinfo}.txt'
        args.append(f'_{get_input_path(fontinfo_file)}')
        ttx_file = f'bug470/{font}-fi.ttx'
    else:
        ttx_file = f'bug470/{font}.ttx'
    font_file = f'bug470/{font}.pfa'
    # 'dir=TEMP_DIR' is used for guaranteeing that the temp data is on same
    # file system as other data; if it's not, a file rename step made by
    # sfntedit will NOT work.
    actual_path = get_temp_file_path(directory=TEMP_DIR)
    runner(CMD + ['-o', 'f', f'_{get_input_path(font_file)}',
                        'o', f'_{actual_path}'] + args)
    actual_ttx = generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = get_expected_path(ttx_file)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


@pytest.mark.parametrize('opts', [
    [],
    ['r'],
    ['ga'],
    ['r', 'ga'],
    ['nga'],
    ['r', 'nga'],
    ['gf'],
    ['r', 'gf'],
    ['gf', 'ga'],
    ['gf', 'ga', 'nga'],  # GOADB will be applied, because 'ga' is first
    ['gf', 'nga'],
    ['gf', 'nga', 'ga'],  # GOADB not applied, because 'nga' is first
    ['r', 'ga', 'gf'],
    ['bit7n', 'bit8n', 'bit9n'],
    ['bit7y', 'bit8y', 'bit9y'],
    ['bit7y', 'bit8n', 'bit9n'],
    ['bit7y', 'bit8n', 'bit7n', 'bit8y'],
])
def test_GOADB_options_bug497(opts):
    font_path = get_input_path(T1PFA_NAME)
    feat_path = get_input_path('bug497/feat.fea')
    fmndb_path = get_input_path('bug497/fmndb.txt')
    goadb_path = get_input_path('bug497/goadb.txt')
    actual_path = get_temp_file_path()
    ttx_filename = '-'.join(['opts'] + opts) + '.ttx'

    args = []
    for opt in opts:  # order of the opts is important
        if opt == 'r':
            args.append(opt)
        elif opt == 'ga':
            args.append(opt)
        elif opt == 'nga':
            args.append(opt)
        elif opt == 'gf':
            args.extend([opt, f'_{goadb_path}'])
        elif 'bit' in opt:
            bit_num = int(opt[3])
            bit_bol = 'osbOn' if opt[4] == 'y' else 'osbOff'
            args.extend([bit_bol, f'_{bit_num}'])

    runner(CMD + ['-o', 'f', f'_{font_path}',
                        'o', f'_{actual_path}',
                        'ff', f'_{feat_path}',
                        'mf', f'_{fmndb_path}'] + args)
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(f'bug497/{ttx_filename}')
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])


@pytest.mark.parametrize('feat_name, has_warn', [('v0005', False),
                                                 ('ADBE', True)])
def test_fetch_font_version_bug610(feat_name, has_warn):
    input_filename = 't1pfa.pfa'
    feat_filename = f'bug610/{feat_name}.fea'
    otf_path = get_temp_file_path()

    stdout_path = runner(
        CMD + ['-s', '-o', 'r',
               'f', f'_{get_input_path(input_filename)}',
               'ff', f'_{get_input_path(feat_filename)}',
               'o', f'_{otf_path}'])

    with open(stdout_path, 'rb') as f:
        output = f.read()
    assert b"Revision 0.005" in output
    assert (b"[Warning] Major version number not in "
            b"range 1 .. 255" in output) is has_warn


def test_update_cff_bbox_bug617():
    input_filename = "bug617/font.pfa"
    goadb_filename = "bug617/goadb.txt"
    actual_path = get_temp_file_path()
    ttx_filename = "bug617.ttx"
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'gf', f'_{get_input_path(goadb_filename)}',
                        'o', f'_{actual_path}', 'r', 'gs'])
    actual_ttx = generate_ttx_dump(actual_path, ['head', 'CFF '])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


@pytest.mark.parametrize('feat_filename', [
    'bug164/d1/d2/rel_to_main.fea',
    'bug164/d1/d2/rel_to_parent.fea',
])
def test_feature_includes_type1_bug164(feat_filename):
    input_filename = "bug164/d1/d2/font.pfa"
    otf_path = get_temp_file_path()

    runner(CMD + ['-o',
                  'f', f'_{get_input_path(input_filename)}',
                  'ff', f'_{get_input_path(feat_filename)}',
                  'o', f'_{otf_path}'])

    assert font_has_table(otf_path, 'head')


def test_feature_includes_ufo_bug164():
    input_filename = "bug164/d1/d2/font.ufo"
    otf_path = get_temp_file_path()

    runner(CMD + ['-o',
                  'f', f'_{get_input_path(input_filename)}',
                  'o', f'_{otf_path}'])

    assert font_has_table(otf_path, 'head')


def test_ttf_input_font_bug680():
    input_filename = 'bug680/font.ttf'
    feat_filename = 'bug680/features.fea'
    ttf_path = get_temp_file_path()

    runner(CMD + ['-o', 'r',
                  'f', f'_{get_input_path(input_filename)}',
                  'ff', f'_{get_input_path(feat_filename)}',
                  'o', f'_{ttf_path}'])

    for table_tag in ('head', 'hhea', 'maxp', 'OS/2', 'hmtx', 'cmap', 'fpgm',
                      'prep', 'cvt ', 'loca', 'glyf', 'name', 'post', 'gasp',
                      'BASE', 'GDEF', 'GPOS', 'GSUB'):
        assert font_has_table(ttf_path, table_tag)


def test_skip_ufo3_global_guides_bug700():
    input_filename = "bug700.ufo"
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'o', f'_{actual_path}'])
    assert font_has_table(actual_path, 'CFF ')


def test_outline_from_processed_layer_bug703():
    input_filename = 'bug703.ufo'
    ttx_filename = 'bug703.ttx'
    actual_path = get_temp_file_path()
    runner(CMD + ['-o', 'f', f'_{get_input_path(input_filename)}',
                        'o', f'_{actual_path}'])
    actual_ttx = generate_ttx_dump(actual_path, ['CFF '])
    expected_ttx = get_expected_path(ttx_filename)
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_unhandled_ufo_glif_token_bug705():
    input_filename = 'bug705.ufo'
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o',
               'f', f'_{get_input_path(input_filename)}',
               'o', f'_{otf_path}'])

    with open(stderr_path, 'rb') as f:
        output = f.read()
    assert b"unhandled token: <foo" in output


def test_delete_zero_kb_font_on_fail_bug736():
    input_filename = 'bug736/font.pfa'
    feat_filename = 'bug736/feat.fea'
    out_filename = 'bug736/SourceSans-Test.otf'

    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-o',
                      'f', f'_{get_input_path(input_filename)}',
                      'ff', f'_{get_input_path(feat_filename)}'])
    assert err.value.returncode == 2
    assert os.path.exists(get_input_path(out_filename)) is False


def test_duplicate_warning_messages_bug751():
    input_filename = 'bug751.ufo'
    expected_path = get_expected_path('bug751.txt')
    otf_path = get_temp_file_path()

    stderr_path = runner(
        CMD + ['-s', '-e', '-o',
               'f', f'_{get_input_path(input_filename)}',
               'o', f'_{otf_path}'])

    assert differ([expected_path, stderr_path, '-l', '1',
                   '-s', 'Built development mode font'])


@pytest.mark.parametrize('feat_filename, rev_value, expected', [
    ('fontrev/fr_1_000.fea', '1', '1.001'),
    ('fontrev/fr_1_000.fea', '2.345', '2.345'),
    ('fontrev/fr_1_000.fea', '40000.000', '1.000')])
def test_update_font_revision(feat_filename, rev_value, expected):
    input_filename = "fontrev/font.ufo"
    otf_path = get_temp_file_path()
    tmp_fea_path = get_temp_file_path()
    copy2(get_input_path(feat_filename), tmp_fea_path)

    runner(CMD + ['-o',
                  'f', f'_{get_input_path(input_filename)}',
                  'ff', f'_{tmp_fea_path}',
                  'o', f'_{otf_path}',
                  'rev', f'_{rev_value}',
                  ])
    frstr = f'{get_font_revision(otf_path):.3f}'
    assert frstr == expected


def test_cli_numerics():
    input_filename = "clinum/font.otf"
    fontinfo_filename = "clinum/fontinfo"
    expected_msg_path = get_expected_path("clinum_output.txt")
    out_font_path = get_temp_file_path()
    stderr_path = runner(CMD + [
        '-s', '-e', '-o',
        'f', f'_{get_input_path(input_filename)}',
        'o', f'_{out_font_path}',
        'fi', f'_{get_input_path(fontinfo_filename)}'])
    assert differ([expected_msg_path, stderr_path,
                   '-r', r'^Built (development|release) mode font'])


@pytest.mark.parametrize('explicit_fmndb', [False, True])
def test_check_psname_in_fmndb_bug1171(explicit_fmndb):
    input_path = get_input_path('bug1171/font.ufo')
    fmndb_path = get_input_path('bug1171/FontMenuNameDB')
    expected_ttx = get_expected_path('bug1171.ttx')
    actual_path = get_temp_file_path()
    opts = ['-o', 'f', f'_{input_path}', 'o', f'_{actual_path}']
    if explicit_fmndb is True:
        opts.extend(['mf', f'_{fmndb_path}'])
    runner(CMD + opts)
    actual_ttx = generate_ttx_dump(actual_path, ['name'])
    assert differ([expected_ttx, actual_ttx,
                   '-s', '<ttFont sfntVersion',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])
