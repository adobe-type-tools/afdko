import os
import pytest
import subprocess
import tempfile

from runner import main as runner
from differ import main as differ, SPLIT_MARKER
from test_utils import (get_input_path, get_expected_path, get_temp_file_path,
                        generate_ps_dump)

TOOL = 'tx'
CMD = ['-t', TOOL]


def _get_temp_dir_path():
    return tempfile.mkdtemp()


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
    '%ADOt1write:'
]


# -----------
# Basic tests
# -----------

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


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
    # 'cff',
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
        save_path = os.path.join(_get_temp_dir_path(), 'font.ufo')
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


# ----------
# Dump tests
# ----------

@pytest.mark.parametrize('args, exp_filename', [
    ([], 'type1.dump1.txt'),
    (['0'], 'type1.dump0.txt'),
    (['dump', '0'], 'type1.dump0.txt'),
    (['1'], 'type1.dump1.txt'),
    (['2'], 'type1.dump2.txt'),
    (['3'], 'type1.dump3.txt'),
    (['4'], 'type1.dump4.txt'),
    (['5'], 'type1.dump5.txt'),
    (['6'], 'type1.dump6.txt'),
    (['6', 'd'], 'type1.dump6d.txt'),
    (['6', 'n'], 'type1.dump6n.txt'),
])
def test_dump(args, exp_filename):
    if args:
        args.insert(0, '-o')

    if any([arg in args for arg in ('4', '5', '6')]):
        skip = []
    else:
        skip = ['-s', '## Filename']

    actual_path = runner(CMD + ['-s', '-f', 'type1.pfa'] + args)
    expected_path = get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


# ----------
# CFF2 tests
# ----------

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
    save_path = os.path.join(
        _get_temp_dir_path(), f'{font_format}.ufo')

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
    save_path = os.path.join(_get_temp_dir_path(), 'bug473.ufo')
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
    skip = ['-s'] + PFA_SKIP[:]
    assert differ([expected_path, actual_path] + skip)


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
    font_path = get_input_path('subr_test_font_infinite_recursion.otf')
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
def test_fdselect_format_4(option):
    font_name = 'fdselect4.otf'
    font_path = get_input_path(font_name)
    dump_path = get_temp_file_path()
    runner(CMD + ['-a', '-o', option, '-f', font_path, dump_path])
    expected_path = get_expected_path(font_name + '.' + option)
    assert differ([expected_path, dump_path, '-s', '## Filename'])
