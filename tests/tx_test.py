import os
import pytest
import re
import subprocess
import time

from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
    get_bad_input_path,
    get_expected_path,
    generate_ps_dump,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

TOOL = 'tx'
CMD = ['-t', TOOL]


def _get_extension(in_format):
    if 'ufo' in in_format:
        return '.ufo'
    elif in_format == 'type1':
        return '.pfa'
    return '.' + in_format


PDF_SKIP = [
    '/Creator' + SPLIT_MARKER +
    '/Producer' + SPLIT_MARKER +
    '/CreationDate' + SPLIT_MARKER +
    '/ModDate' + SPLIT_MARKER +
    '(Date:' + SPLIT_MARKER +
    '(Time:',
]

PDF_SKIP_REGEX = [
    '^.+30.00 Td',
    '^.+0.00 Td',
]

PS_SKIP = [
    '0 740 moveto (Filename:' + SPLIT_MARKER +
    '560 (Date:' + SPLIT_MARKER +
    '560 (Time:'
]

PS_SKIP2 = [
    '%ADOt1write:'
]

PFA_SKIP = [
    '%ADOt1write:' + SPLIT_MARKER +
    '%%Copyright:' + SPLIT_MARKER
]


# -----------
# Basic tests
# -----------

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-bar', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


@pytest.mark.parametrize('pth', [
    ['invalid_path'],  # no such file or directory
    [get_temp_file_path()],  # end of file (not a font)
    [get_input_path('type1.pfa'), 'a', 'b'],  # too many file args
])
def test_exit_invalid_path_or_font(pth):
    assert subprocess.call([TOOL] + pth) == 1


# -------------
# Options tests
# -------------

@pytest.mark.parametrize('args', [
    ['-s', '-t1'],  # '-s' option must be last
    ['-t1', '-g', '0', '-gx', '1'],  # options are mutually exclusive
    ['-dcf'],  # non-CFF font
    ['-ps', '-1'],  # must specify an all-glyph range
    ['-ufo'], ['-t1', '-pfb'],  # must specify a destination path
    ['-t1', '-usefd'],  # bad arg
    ['-t1', '-decid'],  # input font is non-CID
])
def test_option_error_type1_input(args):
    font_path = get_input_path('type1.pfa')
    assert subprocess.call([TOOL] + args + [font_path]) == 1


@pytest.mark.parametrize('arg', ['-e', '-q', '+q', '-w', '+w', '-lf', '-cr',
                                 '-crlf', '-decid', '-LWFN', '-pfb'])
def test_option_error_type1_clash(arg):
    # options -pfb or -LWFN may not be used with other options
    pfb = '-pfb' if arg != '-pfb' else '-LWFN'
    assert subprocess.call([TOOL, '-t1', pfb, arg]) == 1


@pytest.mark.parametrize('args', [
    ['-cff', '-l'], ['-cff', '-0'], ['-cff', '-1'], ['-cff', '-2'],
    ['-cff', '-3'], ['-cff', '-4'], ['-cff', '-5'], ['-cff', '-6'],
    ['-cff', '-q'], ['-cff', '+q'], ['-cff', '-w'], ['-cff', '+w'],
    ['-cff', '-pfb'], ['-cff', '-usefd'], ['-cff', '-decid'],
    ['-cff', '-lf'], ['-cff', '-cr'], ['-cff', '-crlf'], ['-cff', '-LWFN'],
    ['-t1', '-gn0'], ['-t1', '-gn1'], ['-t1', '-gn2'], ['-t1', '-sa'],
    ['-t1', '-abs'], ['-t1', '-cefsvg'],
    ['-t1', '-no_futile'], ['-t1', '-no_opt'], ['-t1', '-d'], ['-t1', '+d'],
    ['-dcf', '-n'], ['-dcf', '-c'],
    ['-dump', '-E'], ['-dump', '+E'], ['-dump', '-F'], ['-dump', '+F'],
    ['-dump', '-O'], ['-dump', '+O'], ['-dump', '-S'], ['-dump', '+S'],
    ['-dump', '-T'], ['-dump', '+T'], ['-dump', '-V'], ['-dump', '+V'],
    ['-dump', '-b'], ['-dump', '+b'], ['-dump', '-e'], ['-dump', '+e'],
    ['-dump', '-Z'], ['-dump', '+Z'],
])
def test_option_error_wrong_mode(args):
    assert subprocess.call([TOOL] + args) == 1


@pytest.mark.parametrize('arg', [
    '-a', '-e', '-f', '-g', '-i', '-m', '-o', '-p', '-A', '-P', '-U', '-maxs',
    '-usefd', '-fd', '-dd', '-sd', '-sr', ['-cef', '-F'], ['-dcf', '-T']
])
def test_option_error_no_args_left(arg):
    if isinstance(arg, list):
        arg_lst = [TOOL] + arg
    else:
        arg_lst = [TOOL, '-t1', arg]
    assert subprocess.call(arg_lst) == 1


@pytest.mark.parametrize('args', [
    ['-maxs', 'X'], ['-m', 'X'], ['-e', 'X'], ['-e', '5'],
    ['-usefd', 'X'], ['-usefd', '-1']
])
def test_option_error_bad_arg(args):
    assert subprocess.call([TOOL, '-t1'] + args) == 1


@pytest.mark.parametrize('arg2', ['-sd', '-sr', '-dd'])
@pytest.mark.parametrize('arg1', ['-a', '-f', '-A'])
def test_option_error_no_args_left2(arg1, arg2):
    assert subprocess.call([TOOL, '-t1', arg1, arg2]) == 1


@pytest.mark.parametrize('arg2', ['-sd', '-sr', '-dd'])
@pytest.mark.parametrize('arg1', ['-a', '-f'])
def test_option_error_empty_list(arg1, arg2):
    empty_dir = get_temp_dir_path()
    assert subprocess.call([TOOL, '-t1', arg1, arg2, empty_dir]) == 1


@pytest.mark.parametrize('arg', ['-bc', '-z', '-cmp', '-sha1'])
def test_gone_options_bc(arg):
    assert subprocess.call([TOOL, arg]) == 1


@pytest.mark.parametrize('mode, msg', [
    ('-h', b'tx (Type eXchange) is a test harness'),
    ('-u', b'tx {[mode][mode options][shared options][files]}*'),
    ('-afm', b'[-afm options: default none]'),
    ('-cef', b'[-cef options: default none]'),
    ('-cff', b'[-cff options: defaults -E, -F, -O, -S, +T, -V, -Z, -b, -d]'),
    ('-cff2', b'[-cff2 options: defaults -S, -b]'),
    ('-dcf', b'[-dcf options: defaults -T all, -5]'),
    ('-dump', b'[-dump options: default -1]'),
    ('-mtx', b'[-mtx options: default -0]'),
    ('-path', b'[-path options: default -0]'),
    ('-pdf', b'[-pdf options: default -0]'),
    ('-ps', b'[-ps options: default -0]'),
    ('-svg', b'[-svg options: defaults -lf, -gn0]'),
    ('-t1',
        b'[-t1 options: defaults -0, -l, -E, -S, +T, -V, +q, -w, -e 4, -lf]'),
    ('-ufo', b'[-ufo options: default none]'),
])
def test_mode_help(mode, msg):
    output = subprocess.check_output([TOOL, mode, '-h'])
    assert msg in output


@pytest.mark.parametrize('dcf_dump_level', ['0', '1', '5'])
def test_script_file(dcf_dump_level):
    font_path = get_input_path('cid.otf')
    opts_path = get_temp_file_path()
    opts_file_content = f'\n# foo\n # bar\r -{dcf_dump_level}\t"{font_path}"'
    with open(opts_path, 'a') as fp:
        fp.write(opts_file_content)
    actual_path = runner(CMD + ['-s', '-a', '-o', 'dcf', 's', '-f', opts_path])
    expected_path = get_expected_path(f'cid_dcf_{dcf_dump_level}.txt')
    assert differ([expected_path, actual_path])


def test_nested_script():
    # nested scripts not allowed
    temp_path = get_temp_file_path()
    assert subprocess.call([TOOL, '-s', 'foobar', '-s', temp_path]) == 1


@pytest.mark.parametrize('layer_name', ['', 'None', 'background', 'foobar'])
def test_ufo_altlayer(layer_name):
    if not layer_name:
        fname = 'processed'
        args = []
    else:
        fname = 'foreground' if layer_name == 'None' else layer_name
        args = ['altLayer', f'_{fname}']
    actual_path = runner(CMD + ['-s', '-f', 'altlayer.ufo', '-o', '6'] + args)
    expected_path = get_expected_path(f'altlayer_{fname}.txt')
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('arg, filename', [
    ('-a', 'ufo3.t1'),
    ('-A', 'SourceSansPro-Regular.t1'),
])
def test_a_options(arg, filename):
    input_path = get_input_path('ufo3.ufo')
    output_path = os.path.join(os.getcwd(), filename)
    assert os.path.exists(output_path) is False
    subprocess.call([TOOL, '-t1', arg, input_path])
    assert os.path.exists(output_path) is True
    os.remove(output_path)


def test_o_option():
    input_path = get_input_path('ufo3.ufo')
    expected_path = get_expected_path('ufo3.pfa')
    output_path = get_temp_file_path()
    subprocess.call([TOOL, '-t1', '-o', output_path, input_path])
    assert differ([expected_path, output_path, '-s', PFA_SKIP[0]])


def test_f_option():
    fpath1 = get_input_path('type1.pfa')
    fpath2 = get_input_path('cff2_vf.otf')
    actual_path = runner(CMD + ['-s', '-o', 'mtx', '3',
                                'f', f'_{fpath1}', f'_{fpath2}'])
    expected_path = get_expected_path('mtx_f_options.txt')
    assert differ([expected_path, actual_path])


def test_stdin():
    input_path = get_input_path('type1.pfa')
    expected_path = get_expected_path('stdin.txt')
    output_path = get_temp_file_path()
    with open(input_path) as fp:
        output = subprocess.check_output([TOOL], stdin=fp)
    with open(output_path, 'wb') as fp:
        fp.write(output)
    assert differ([expected_path, output_path])


@pytest.mark.parametrize('arg', ['0', '-16'])
def test_m_option_success(arg):
    # mem_manage() is called 16 times with the command 'tx -m 0 type1.pfa'
    input_path = get_input_path('type1.pfa')
    assert subprocess.call([TOOL, '-m', arg, input_path]) == 0


# Disabled because of https://github.com/adobe-type-tools/afdko/issues/933
# @pytest.mark.parametrize('arg', range(1, 16))
# def test_m_option_fail(arg):
#     input_path = get_input_path('type1.pfa')
#     assert subprocess.call([TOOL, '-m', f'-{arg}', input_path]) != 0


@pytest.mark.parametrize('arg, exp_filename', [(None, 'not_removed'),
                                               ('-V', 'not_removed'),
                                               ('+V', 'removed')])
def test_V_option(arg, exp_filename):
    input_path = get_input_path('overlap.pfa')
    expected_path = get_expected_path(f'overlap_{exp_filename}.pfa')
    output_path = get_temp_file_path()
    args = [TOOL, '-t1', '-o', output_path, input_path]
    if arg:
        args.insert(2, arg)
    subprocess.call(args)
    assert differ([expected_path, output_path] + ['-s'] + PFA_SKIP)


# -------------
# Convert tests
# -------------

@pytest.mark.parametrize('to_format', [
    'ufo2',
    'ufo3',
    'type1',
    'svg',
    'mtx',
    'afm',
    'pdf',
    'ps',
    'cff',
])
@pytest.mark.parametrize('from_format', [
    'ufo2',
    'ufo3',
    'type1',
])
def test_convert(from_format, to_format):
    from_ext = _get_extension(from_format)
    to_ext = _get_extension(to_format)

    # input filename
    from_filename = from_format + from_ext

    # expected filename
    exp_filename = from_format + to_ext

    # runner args
    if 'ufo' in to_format:
        save_path = get_temp_dir_path('font.ufo')
    else:
        save_path = get_temp_file_path()

    # diff mode
    if to_format == 'cff':
        diff_mode = ['-m', 'bin']
    else:
        diff_mode = []

    # skip items
    regex_skip = []
    skip = []
    if to_format == 'afm':
        skip = ['Comment Creation Date:' + SPLIT_MARKER + 'Comment Copyright']
    elif to_format == 'pdf':
        skip = PDF_SKIP[:]
        regex_skip = PDF_SKIP_REGEX[:]
    elif to_format == 'ps':
        skip = PS_SKIP[:]
    elif to_format == 'type1':
        skip = PFA_SKIP[:]
    if skip:
        skip.insert(0, '-s')
    if regex_skip:
        for regex in regex_skip:
            skip.append('-r')
            skip.append(regex)

    # format arg fix
    if to_format in ('ufo2', 'ufo3'):
        format_arg = 'ufo'
    elif to_format == 'type1':
        format_arg = 't1'
    else:
        format_arg = to_format

    runner(CMD + ['-a', '-f', get_input_path(from_filename), save_path,
                  '-o', format_arg])
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, save_path] + skip + diff_mode)


def test_cef_cefsvg():
    font_path = get_input_path('cff2_vf.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cef', 'cefsvg', 'cr', 'gn1', 'abs', 'sa',
                  '-f', font_path, output_path])
    expected_path = get_expected_path('cef_cefsvg_cr.svg')
    assert differ([expected_path, output_path])


@pytest.mark.parametrize('file_ext', [
    'pfa', 'pfabin', 'pfb', 'lwfn', 'bidf'])  # TODO: 'bidf85'
def test_type1_inputs(file_ext):
    bidf = '.bidf' if 'bidf' in file_ext else ''
    actual_path = runner(CMD + ['-s', '-o', '2', '-f', f'type1.{file_ext}'])
    expected_path = get_expected_path(f'type1.dump2{bidf}.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


@pytest.mark.parametrize('args', [[], ['U', '_500,500'], ['U', '_0,0', 'n']])
@pytest.mark.parametrize('fname', ['zx', 'zy'])
def test_type1mm_inputs(fname, args):
    fname2 = f'.{"".join(args)}' if args else ''
    actual_path = runner(CMD + ['-s', '-f', f'{fname}.pfb', '-o', '2'] + args)
    expected_path = get_expected_path(f'{fname}.dump2{fname2}.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


@pytest.mark.parametrize('fext', ['otf', 'ttf', 'cff', 'cef', 'ttc'])
def test_other_input_formats(fext):
    arg = ['y'] if fext == 'ttc' else []
    actual_path = runner(CMD + ['-s', '-f', f'font.{fext}', '-o', '3'] + arg)
    expected_path = get_expected_path(f'font.{fext}.dump3.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


# ----------
# Dump tests
# ----------

@pytest.mark.parametrize('args', [
    [],
    ['0'],
    ['dump', '0'],
    ['1'],
    ['2'],
    ['3'],
    ['4'],
    ['4', 'N'],
    ['5'],
    ['6'],
    ['6', 'd'],
    ['6', 'n'],
])
@pytest.mark.parametrize('font_filename', ['type1.pfa', 'svg.svg'])
def test_dump_option(args, font_filename):
    if any([arg in args for arg in ('4', '5', '6')]):
        skip = []
    else:
        skip = ['-s', '## Filename']

    head = font_filename.split('.')[0]
    midl = ''.join(args) if args else 'dump1'
    if 'dump' not in midl:
        midl = f'dump{midl}'
    exp_filename = f'{head}.{midl}.txt'

    opts = ['-o'] + args if args else []

    actual_path = runner(CMD + ['-s', '-f', font_filename] + opts)
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('fext', ['pfa', 'ufo'])
def test_dump_flex_op(fext):
    fname = 'flex'
    actual_path = runner(CMD + ['-s', '-o', '6', '-f', f'{fname}.{fext}'])
    expected_path = get_expected_path(f'{fname}.txt')
    assert differ([expected_path, actual_path])


# ----------
# CFF2 tests
# ----------

@pytest.mark.parametrize('filename, msg', [
    ('avar_invalid_table_version',
        b'(cfr) invalid avar table version'),
    ('fvar_invalid_table_version',
        b'(cfr) invalid fvar table version'),
    ('avar_invalid_table_size',
        b'(cfr) invalid avar table size'),
    ('fvar_invalid_table_size',
        b'(cfr) invalid fvar table size'),
    ('fvar_invalid_table_header',
        b'(cfr) invalid values in fvar table header'),
    ('avar_invalid_axis-instance_count-size',
        b'(cfr) invalid avar table size or axis/instance count/size'),
    ('fvar_invalid_axis-instance_count-size',
        b'(cfr) invalid fvar table size or axis/instance count/size'),
    ('avar_axis_value_map_out_of_bounds',
        b'(cfr) avar axis value map out of bounds'),
    ('avar_fvar_axis_mismatch',
        b'(cfr) mismatching axis counts in fvar and avar'),
])
def test_varread_errors(filename, msg):
    font_path = get_bad_input_path(f'vf_{filename}.otf')
    output = subprocess.check_output([TOOL, '-dcf', '-0', font_path],
                                     stderr=subprocess.STDOUT)
    assert msg in output


@pytest.mark.parametrize('args, exp_filename', [
    ([], 'SourceCodeVar-Roman_CFF2'),
    (['*S', '*b', 'std'], 'SourceCodeVar-Roman_CFF2_subr'),  # subroutinize
])
def test_cff2_extract(args, exp_filename):
    # read CFF2 VF, write CFF2 table
    font_path = get_input_path('SourceCodeVariable-Roman.otf')
    cff2_path = get_temp_file_path()
    runner(CMD + ['-a', '-f', font_path, cff2_path, '-o', 'cff2'] + args)
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, cff2_path, '-m', 'bin'])


def test_cff2_sub_dump():
    # Dump a subroutinized CFF2 font. This is a J font with 64K glyphs,
    # and almost every subr and charstring is a single subr call.
    # A good test for problems with charstrings with no endchar operator.
    actual_path = runner(CMD + ['-s', '-o', 'dump', '6', 'g', '_21847',
                                '-f', 'CFF2-serif-sub.cff2'])
    expected_path = get_expected_path('CFF2-serif-sub.cff2.txt')
    assert differ([expected_path, actual_path])


def test_varread_pr355():
    # read CFF2 VF, write Type1 snapshot
    # Note that cff2_vf is built from the sources at:
    #    afdko/tests/buildmasterotfs_data/input/cff2_vf.
    actual_path = runner(CMD + ['-s', '-o', 't1', '-f', 'cff2_vf.otf'])
    expected_path = get_expected_path('cff2_vf.pfa')
    skip = ['-s'] + PFA_SKIP[:]
    assert differ([expected_path, actual_path] + skip)


def test_cff2_no_vf_bug353():
    # read CFF2 WITHOUT VF info, write a CFF2 out. 'regular_CFF2.otf'
    # is derived by taking the regular.otf file from the sfntdiff
    # 'input_data' directory, and converting the CFF table to CFF2.
    font_path = get_input_path('regular_CFF2.otf')
    cff2_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cff2', '-f', font_path, cff2_path])
    expected_path = get_expected_path('regular_CFF2.cff2')
    assert differ([expected_path, cff2_path, '-m', 'bin'])


def test_cff2_with_spare_masters_pr835():
    # SetNumMasters was incorrectly passing the number of region indices to
    # var_getIVSRegionIndices for the regionListCount. With PR #835 it now
    # passes the total region count for regionListCount.
    #
    # Example of the bug -- this command:
    #   tx -cff2 +S +b -std SHSansJPVFTest.otf SHSansJPVFTest.cff2
    # Would produce the following warning & error:
    #   inconsistent region indices detected in item variation store subtable 1
    #   memory error
    font_path = get_input_path('SHSansJPVFTest.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o',
                  'cff2', '*S', '*b', 'std',
                  '-f', font_path, output_path])
    expected_path = get_expected_path('SHSansJPVFTest.cff2')
    assert differ([expected_path, output_path, '-m', 'bin'])


@pytest.mark.parametrize('vector, exp_filename', [
    ('9999,9999,9999,9999,999,9', 'psname_last_resort_no.txt'),
    ('9999,9999,9999,9999,999,99', 'psname_last_resort_yes.txt'),
])
def test_last_resort_instance_psname(vector, exp_filename):
    font_path = get_input_path('cff2_vf_many_axes.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-o', '0', 'U', f'_{vector}', '-f', font_path, output_path])
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, output_path, '-s', '## Filename'])


# -----------
# Other tests
# -----------

def test_trademark_string_pr425():
    # the copyright symbol used in the trademark field of a UFO is
    # converted to 'Copyright' and stored in Notice field of a Type1
    actual_path = runner(CMD + ['-s', '-o', 't1', '-f', 'trademark.ufo'])
    expected_path = get_expected_path('trademark.pfa')
    skip = ['-s'] + PFA_SKIP[:]
    assert differ([expected_path, actual_path] + skip)


def test_remove_hints_bug180():
    font_path = get_input_path('cid.otf')
    cid_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 't1', 'n', '-f', font_path, cid_path])
    expected_path = get_expected_path('cid_nohints.ps')
    expected_path = generate_ps_dump(expected_path)
    actual_path = generate_ps_dump(cid_path)
    skip = ['-s'] + PS_SKIP2
    assert differ([expected_path, actual_path] + skip)


def test_long_charstring_read_bug444():
    # read a CFF2 VF with a charstring longer that 65535, check output
    actual_path = runner(CMD + ['-s', '-o', '0', '-f', 'CJK-VarTest.otf'])
    expected_path = get_expected_path('CJK-VarTest_read.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


def test_long_charstring_warning():
    # read a CFF2 VF with a charstring longer that 65535, check warning message
    # NOTE: can't diff the output against 'CJK-VarTest_warn.txt' because on
    # Windows the lines start with 'tx.exe:' instead of just 'tx:'
    actual_path = runner(
        CMD + ['-s', '-e', '-o', '5', '-f', 'CJK-VarTest.otf'])
    # expected_path = get_expected_path('CJK-VarTest_warn.txt')
    with open(actual_path, 'rb') as f:
        output = f.read()
    assert b"(cfr) Warning: CharString of GID 1 is 71057 bytes long" in output


def test_long_charstring_write():
    # read a CFF2 VF with a charstring longer that 65535, write out CFF2 file
    # NOTE: the font 'CJK-VarTest.otf' cannot be used in this test because
    # once its long charstring is optimized (floats -> ints) it's no longer
    # over the 65535 bytes limit; the long charstring in 'CJK-VarTest2.otf' is
    # already as small as possible, so it will trigger the check in cffwrite.c
    font_path = get_input_path('CJK-VarTest2.otf')
    cff2_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cff2', '-f', font_path, cff2_path])
    expected_path = get_expected_path('CJK-VarTest2.cff2')
    assert differ([expected_path, cff2_path, '-m', 'bin'])


def test_many_hints_string_bug354():
    # The glyph T@gid002 has 33 hstem hints. This tests a bug where
    # tx defined an array of only 6 operands.
    # This is encountered only when wrinting to a VF CFF2.
    font_path = get_input_path('cff2_vf.otf')
    cff2_path = get_temp_file_path()
    dcf_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cff2', '-f', font_path, cff2_path])
    runner(CMD + ['-a', '-o', 'dcf', '-f', cff2_path, dcf_path])
    expected_path = get_expected_path('cff2_vf.dcf.txt')
    assert differ([expected_path, dcf_path])


def test_non_varying_glyphs_bug356():
    """A glyph which is non-varying in a variable font may be referenced by a
    VariationStore data item subtable which has a region count of 0. The VF
    support code assumed that this was an error, and issued a false warning.
    File 'bug356.otf' is a handcrafted modification of 'cff2_vf.otf'. The
    latter cannot be used as-is to validate the fix."""
    actual_path = get_temp_file_path()
    font_path = get_input_path('bug356.otf')
    stderr_path = runner(CMD + ['-s', '-e', '-a', '-o', 'cff',
                                '-f', font_path, actual_path])
    expected_path = get_expected_path('bug356.txt')
    assert differ([expected_path, stderr_path, '-l', '1'])


@pytest.mark.parametrize('font_format', ['type1', 'cidfont', 'ufo2', 'ufo3'])
def test_no_psname_dump_bug437(font_format):
    if 'cid' in font_format:
        file_ext = 'ps'
    elif 'ufo' in font_format:
        file_ext = 'ufo'
    else:
        file_ext = 'pfa'

    filename = f'{font_format}-noPSname.{file_ext}'
    expected_path = get_expected_path(f'bug437/dump-{font_format}.txt')

    actual_path = runner(CMD + ['-s', '-o', 'dump', '0', '-f', filename])
    assert differ([expected_path, actual_path, '-l', '1'])


@pytest.mark.parametrize('font_format', ['type1', 'cidfont', 'ufo2', 'ufo3'])
def test_no_psname_convert_to_ufo_bug437(font_format):
    if 'cid' in font_format:
        file_ext = 'ps'
    elif 'ufo' in font_format:
        file_ext = 'ufo'
    else:
        file_ext = 'pfa'

    font_path = get_input_path(f'{font_format}-noPSname.{file_ext}')
    expected_path = get_expected_path(f'bug437/{font_format}.ufo')
    save_path = get_temp_dir_path(f'{font_format}.ufo')

    runner(CMD + ['-a', '-o', 'ufo', '-f', font_path, save_path])
    assert differ([expected_path, save_path])


@pytest.mark.parametrize('font_format', ['type1', 'cidfont', 'ufo2', 'ufo3'])
def test_no_psname_convert_to_type1_bug437(font_format):
    if 'cid' in font_format:
        file_ext = 'ps'
    elif 'ufo' in font_format:
        file_ext = 'ufo'
    else:
        file_ext = 'pfa'

    filename = f'{font_format}-noPSname.{file_ext}'
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-o', 't1', '-f', filename])
    assert err.value.returncode in (5, 6)


def test_illegal_chars_in_glyph_name_bug473():
    font_path = get_input_path('bug473.ufo')
    save_path = get_temp_dir_path('bug473.ufo')
    runner(CMD + ['-a', '-o', 'ufo', '-f', font_path, save_path])
    expected_path = get_expected_path('bug473.ufo')
    assert differ([expected_path, save_path])


def test_subroutine_sorting_bug494():
    """ The input file was made with the command:
    tx -t1 -g 0-5 \
        source-serif-pro/Roman/Instances/Regular/font.ufo bug494.pfa
    The bug is that two subroutines in the Windows CFF output are swapped in
    index order from the Mac version. This was because of an unstable
    'qsort' done on the subroutines in the final stage of selection."""
    font_path = get_input_path('bug494.pfa')
    cff_path = get_temp_file_path()
    dcf_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cff', '*S', 'std', '*b',
                  '-f', font_path, cff_path])
    runner(CMD + ['-a', '-o', 'dcf', '-f', cff_path, dcf_path])
    expected_path = get_expected_path('bug494.dcf.txt')
    assert differ([expected_path, dcf_path])


@pytest.mark.parametrize('args, exp_filename', [([], 'roundtrip'),
                                                (['g', '_0-1'], 'subset')])
@pytest.mark.parametrize('to_format', ['t1', 'cff', 'afm'])
def test_recalculate_font_bbox_bug618(to_format, args, exp_filename):
    font_path = get_input_path('bug618.pfa')
    save_path = get_temp_file_path()

    runner(CMD + ['-f', font_path, save_path, '-o', to_format] + args)

    file_ext = to_format
    if to_format == 't1':
        file_ext = 'pfa'
    elif to_format == 'afm':
        file_ext = 'txt'

    expected_path = get_expected_path(
        f'bug618/{exp_filename}.{file_ext}')

    diff_mode = []
    if to_format == 'cff':
        diff_mode = ['-m', 'bin']

    skip = []
    if to_format == 'afm':
        skip = ['-s', 'Comment Creation Date:' + SPLIT_MARKER +
                'Comment Copyright']
    elif to_format == 't1':
        skip = ['-s'] + PFA_SKIP[:]

    assert differ([expected_path, save_path] + diff_mode + skip)


def test_glyph_bboxes_bug655():
    actual_path = runner(CMD + ['-s', '-o', 'mtx', '2', '-f', 'bug655.ufo'])
    expected_path = get_expected_path('bug655.txt')
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('filename', ['SHSVF_9b3b', 'bug684'])
def test_cs_opt_bug684(filename):
    """ The input CFF2 variable font contains a long single charstring
    making the maximum use of the operand stack.
    tx was generating a bad CFF2 charstring that would overflow
    the operand stack of the standard size (513) after re-converted
    to CFF2 unless -no_opt option is specified."""
    font_path = get_input_path(f'{filename}.otf')
    result_path = get_temp_file_path()
    expected_path = get_expected_path(f'{filename}.cff2')
    runner(CMD + ['-a', '-o', 'cff2', '-f', font_path, result_path])
    assert differ([expected_path, result_path, '-m', 'bin'])


def test_standard_apple_glyph_names():
    actual_path = runner(CMD + ['-s', '-o', 'dump', '4', '-f', 'post-v2.ttf'])
    expected_path = get_expected_path('post-v2.txt')
    assert differ([expected_path, actual_path])


def test_ufo_self_closing_dict_element_bug701():
    actual_path = runner(CMD + ['-s', '-o', 'dump', '0', '-f', 'bug701.ufo'])
    expected_path = get_expected_path('bug701.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


def test_ufo3_guideline_bug705():
    actual_path = runner(CMD + ['-s', '-o', 't1', '-f', 'bug705.ufo'])
    expected_path = get_expected_path('bug705.pfa')
    assert differ([expected_path, actual_path] + ['-s'] + PFA_SKIP)


def test_ufo_vertical_advance_bug786():
    actual_path = runner(CMD + ['-s', '-o', 't1', '-f', 'bug786.ufo'])
    expected_path = get_expected_path('bug786.pfa')
    skip = ['-s'] + PFA_SKIP[:]
    assert differ([expected_path, actual_path] + skip)


@pytest.mark.parametrize('filename', [
    'a',  # AE glyph in both default and processed layers
    'b',  # AE glyph in default layer only
    'c',  # AE glyph in processed layer only
])
def test_ufo_read_processed_contents_plist_bug740(filename):
    actual_path = runner(CMD + ['-s', '-o', 'dump', '6', 'g', '_AE',
                                '-f', f'bug740/{filename}.ufo'])
    expected_path = get_expected_path(f'bug740/{filename}.txt')
    assert differ([expected_path, actual_path])


def test_dcf_with_infinite_recursion_bug775():
    font_path = get_bad_input_path('subr_test_font_infinite_recursion.otf')
    dcf_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-a', '-o', 'dcf', '-f', font_path, dcf_path])
    assert(err.value.returncode == 1)  # exit code of 1, not segfault of -11
    expected_path = get_expected_path(
        'subr_test_font_infinite_recursion.dcf.txt')
    assert differ([expected_path, dcf_path])


def test_dcf_call_depth_with_many_calls_bug846():
    # This font was getting an invalid subroutine count because tx wasn't
    # decrementing the subroutine call depth after the subroutine calls,
    # so it was effectively just counting the total number of calls,
    # not the call depth.
    font_path = get_input_path('SHSansJPVFTest_SUBR.otf')
    dcf_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'dcf', '-f', font_path, dcf_path])
    expected_path = get_expected_path('SHSansJPVFTest_SUBR.dcf.txt')
    assert differ([expected_path, dcf_path])


def test_svg_with_cid_font_bug822():
    font_path = get_input_path('cid.otf')
    cid_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'svg', '-f', font_path, cid_path])
    expected_path = get_expected_path('cid.svg')
    assert differ([expected_path, cid_path])


@pytest.mark.parametrize('filename',
                         ['type1-noPSname.pfa', 'cidfont-noPSname.ps'])
def test_svg_missing_fontname_bug883(filename):
    font_path = get_input_path(filename)
    svg_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-a', '-o', 'svg', '-f', font_path, svg_path])
    assert(err.value.returncode == 6)  # exit code of 6, not segfault of -11


@pytest.mark.parametrize('option', ['dump', 'dcf'])
def test_read_fdselect_format_4(option):
    font_name = 'fdselect4.otf'
    input_path = get_input_path(font_name)
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', option, '-f', input_path, output_path])
    expected_path = get_expected_path(font_name + '.' + option)
    assert differ([expected_path, output_path, '-s', '## Filename'])


def test_write_fdselect_format_4():
    font_name = 'FDArrayTest257FontDicts.otf'
    input_path = get_input_path(font_name)
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'cff2', '-f', input_path, output_path])
    expected_path = get_expected_path('FDArrayTest257FontDicts.cff2')
    assert differ([expected_path, output_path, '-m', 'bin'])


@pytest.mark.parametrize('option', ['cff', 'dcf'])
@pytest.mark.parametrize('font_name',
                         ['bug895_charstring.otf', 'bug895_private_dict.otf'])
def test_read_short_charstring_bug895(option, font_name):
    input_path = get_bad_input_path(font_name)
    output_path = runner(CMD + ['-s', '-e', '-a', '-o', option,
                                '-f', input_path])
    expected_path = get_expected_path(font_name + '.' + option)
    skip = ['-s', 'tx: ---']  # skip line with filename
    assert differ([expected_path, output_path] + skip)


@pytest.mark.parametrize('option', ['cff2', 'cff'])
def test_drop_defaultwidthx_when_writing_cff2_bug897(option):
    input_path = get_bad_input_path('bug897.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', option, '-f', input_path, output_path])
    dcf_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'dcf', '-f', output_path, dcf_path])
    expected_path = get_expected_path('bug897.' + option + '.dcf')
    assert differ([expected_path, dcf_path])


@pytest.mark.parametrize('option', ['afm', 'dump', 'svg'])
def test_missing_glyph_names_pr905(option):
    input_path = get_bad_input_path('pr905.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', option, '-f', input_path, output_path])
    expected_path = get_expected_path('pr905' + '.' + option)
    if option == 'afm':
        skip = ['-s',
                'Comment Creation Date:' + SPLIT_MARKER + 'Comment Copyright']
    elif option == 'dump':
        skip = ['-s', '## Filename']
    else:
        skip = []
    assert differ([expected_path, output_path] + skip)


def test_missing_glyph_names_pr905_cef():
    input_path = get_bad_input_path('pr905.otf')
    output_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-a', '-o', 'cef', '-f', input_path, output_path])
    assert(err.value.returncode > 0)  # error code, not segfault of -11


def test_var_bug_913():
    # AdobeVFPrototype_mod.otf is a modified copy of AdobeVFPrototype.otf 1.003
    # so that the region indexes in HVAR are listed in a different order from
    # those in CFF2. Also MVAR table has been modified to contain (dummy)
    # deltas for underline offset and underline thickness just to exercize
    # MVAR lookup code.
    font_path = get_input_path('AdobeVFPrototype_mod.otf')
    save_path = get_temp_file_path()
    runner(CMD + ['-a', '-o',
                  '3', 'g', '_A,W,y', 'U', '_900,0',
                  '-f', font_path, save_path])
    expected_path = get_expected_path('bug913.txt')
    assert differ([expected_path, save_path, '-s', '## Filename'])


def test_bad_charset():
    font_path = get_bad_input_path('bad_charset.otf')
    save_path = get_temp_file_path()
    runner(CMD + ['-a', '-f', font_path, save_path])
    expected_path = get_expected_path('bad_charset.txt')
    assert differ([expected_path, save_path, '-s', '## Filename'])


def test_bug_940():
    input_path = get_bad_input_path('bug940_private_blend.otf')
    output_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-a', '-o', 'cff2', '-f', input_path, output_path])
    assert(err.value.returncode > 0)  # error code, not segfault or success


def test_too_many_glyphs_pr955():
    input_path = get_bad_input_path('TooManyGlyphsCFF2.otf')
    output_path = get_temp_file_path()
    with pytest.raises(subprocess.CalledProcessError) as err:
        runner(CMD + ['-a', '-o', 'cff', '-f', input_path, output_path])
    assert(err.value.returncode > 0)  # error code, not hang or success


def test_ttread_varinst():
    font_path = get_input_path('AdobeVFPrototype.ttf')
    save_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', '3', 'g', '_A', 'U', '_500,800',
                  '-f', font_path, save_path])
    expected_path = get_expected_path('vfproto_tt_inst500_800.txt')
    assert differ([expected_path, save_path, '-s', '## Filename'])


def test_unused_post2_names():
    font_path = get_input_path('SourceSansPro-Regular-cff2-unused-post.otf')
    save_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', '1', '-f', font_path, save_path])
    expected_path = get_expected_path('ssr-cff2-unused-post.txt')
    assert differ([expected_path, save_path, '-s', '## Filename'])


def test_seac_reporting():
    # This test aims to show that the SEAC operator
    # is not reported by all tx modes
    font_path = get_input_path('seac.otf')
    save_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', '6', '-f', font_path, save_path])
    expected_path = get_expected_path('seac.dump.txt')
    assert differ([expected_path, save_path])
    runner(CMD + ['-a', '-o', 'dcf', '5', 'T', '_c',
                  '-f', font_path, save_path])
    expected_path = get_expected_path('seac.dcf.txt')
    assert differ([expected_path, save_path])


def test_date_and_time_afm():
    """
    test the use of date and time functions in absfont_afm.c
    """
    input_path = get_input_path('font.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'afm', '-f', input_path, output_path])
    now = time.time()
    year = '%s' % time.localtime().tm_year
    with open(output_path) as output_file:
        lines = output_file.readlines()
        file_year = lines[1].split()[2]
        assert year == file_year
        file_time_str = lines[2].split(': ')[1].strip()
        file_time = time.mktime(
            time.strptime(file_time_str, '%a %b %d %H:%M:%S %Y'))
        hours_diff = abs(now - file_time) / 3600
        assert(hours_diff < 1)


def test_date_and_time_ps():
    """
    test the use of date and time functions in absfont_draw.c
    """
    input_path = get_input_path('font.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'ps', '-f', input_path, output_path])
    now = time.time()
    with open(output_path) as output_file:
        lines = output_file.readlines()
        date_str = re.split(r'[()]', lines[5])[1]
        date_str = date_str.split(': ')[1]
        time_str = re.split(r'[()]', lines[7])[1]
        time_str = time_str.split(': ')[1]
        file_date_and_time_str = date_str + ' ' + time_str
        file_time = time.mktime(
            time.strptime(file_date_and_time_str, '%m/%d/%y %H:%M'))
        hours_diff = abs(now - file_time) / 3600
        assert(hours_diff < 1)


def test_date_and_time_pdf():
    """
    test the use of date and time functions in pdfwrite.c
    """
    input_path = get_input_path('font.otf')
    output_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', 'pdf', '-f', input_path, output_path])
    now = time.time()
    tz = time.timezone
    tz_hr = abs(int(tz / 3600))  # ignore sign since we're splitting on +/-
    tz_min = (tz % 3600) // 60
    with open(output_path) as output_file:
        lines = output_file.readlines()
        creation_date_str = re.split(r'[()]', lines[13])[1]
        mod_date_str = re.split(r'[()]', lines[14])[1]
        assert(creation_date_str == mod_date_str)
        (date_time_str, tz_hr_str, tz_min_str) = \
            re.split(r"[:+\-Z']", creation_date_str)[1:4]
        creation_time = time.mktime(
            time.strptime(date_time_str, '%Y%m%d%H%M%S'))
        hours_diff = abs(now - creation_time) / 3600
        assert(hours_diff < 1)
        creation_tz_hr = int(tz_hr_str)
        assert(creation_tz_hr == tz_hr)
        creation_tz_min = int(tz_min_str)
        assert(creation_tz_min == tz_min)
        file_date_str = re.split(r"[():]", lines[36])[2].strip()
        file_time_str = re.split(r"[() ]", lines[38])[3]
        file_date_time_str = file_date_str + ' ' + file_time_str
        file_time = time.mktime(
            time.strptime(file_date_time_str, "%d %b %y %H:%M"))
        hours_diff = abs(now - file_time) / 3600
        assert(hours_diff < 1)


def test_overlap_removal():
    input_path = get_input_path('overlaps.ufo')
    expected_path = get_expected_path('overlaps.pfa')
    output_path = get_temp_file_path()
    args = [TOOL, '-t1', '+V', '-o', output_path, input_path]
    subprocess.call(args)
    assert differ([expected_path, output_path, '-s', PFA_SKIP[0]])


@pytest.mark.parametrize("fmt", [
    "cff",
    "cff2",
])
def test_nonstd_fontmatrix(fmt):
    input_path = get_input_path("nonstdfmtx.otf")
    txt_filename = f"nonstdfmtx_{fmt}.txt"
    expected_path = get_expected_path(txt_filename)
    output_dir = get_temp_dir_path()
    bin_output = os.path.join(output_dir, f"nonstdfmtx.{fmt}")
    output_path = os.path.join(output_dir, txt_filename)
    runner(CMD + ['-a', '-o', fmt, '*S', '*b', '-f', input_path, bin_output])
    runner(CMD + ['-a', '-o', 'dump', '-f', bin_output, output_path])
    skip = "## Filename "
    assert differ([expected_path, output_path, '-s', skip])


def test_pdf_single_glyph():
    input_path = get_input_path("bug1218.otf")
    pdf_filename = "bug1218.pdf"
    expected_path = get_expected_path(pdf_filename)
    output_dir = get_temp_dir_path()
    output_path = os.path.join(output_dir, pdf_filename)

    runner(CMD + ['-a', '-o', 'pdf', '1', '-f', input_path, output_path])

    skip = PDF_SKIP[:]
    skip.insert(0, '-s')
    regex_skip = PDF_SKIP_REGEX[:]
    for regex in regex_skip:
        skip.append('-r')
        skip.append(regex)

    assert differ([expected_path, output_path] + skip)
