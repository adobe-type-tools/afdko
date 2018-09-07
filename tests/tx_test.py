from __future__ import print_function, division, absolute_import

import os
import pytest
import tempfile

from .runner import main as runner
from .differ import main as differ
from .differ import SPLIT_MARKER

TOOL = 'tx'
CMD = ['-t', TOOL]

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_expected_path(file_name):
    return os.path.join(data_dir_path, 'expected_output', file_name)


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

PS_SKIP = [
    '0 740 moveto (Filename:' + SPLIT_MARKER +
    '560 (Date:' + SPLIT_MARKER +
    '560 (Time:'
]


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
        save_path = os.path.join(_get_temp_dir_path(), 'font.ufo')
        runn_args = ['-s', save_path]
    else:
        runn_args = []

    # diff mode
    if to_format == 'cff':
        diff_mode = ['-m', 'bin']
    else:
        diff_mode = []

    # skip items
    if to_format == 'afm':
        skip = ['Comment Creation Date:']
    elif to_format == 'pdf':
        skip = PDF_SKIP[:]
    elif to_format == 'ps':
        skip = PS_SKIP[:]
    else:
        skip = []
    if skip:
        skip.insert(0, '-s')

    # format arg fix
    if to_format in ('ufo2', 'ufo3'):
        format_arg = 'ufo'
    elif to_format == 'type1':
        format_arg = 't1'
    else:
        format_arg = to_format

    actual_path = runner(
        CMD + ['-f', from_filename, '-o', format_arg] + runn_args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip + diff_mode)


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

    actual_path = runner(CMD + ['-f', 'type1.pfa'] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path] + skip)


# ----------
# CFF2 tests
# ----------

@pytest.mark.parametrize('args, exp_filename', [
    ([], 'SourceCodeVar-Roman_CFF2'),
    # The result is not the same across environments
    # https://ci.appveyor.com/project/adobe-type-tools/afdko/build/1.0.721
    # https://travis-ci.org/adobe-type-tools/afdko/builds/405208971
    # The result file was generated on a MBP running macOS 10.13.6
    # (['*S', '*b', 'std'], 'SourceCodeVar-Roman_CFF2_subr'),  # subroutinize
])
def test_cff2_extract(args, exp_filename):
    # read CFF2 VF, write CFF2 table
    actual_path = runner(CMD + ['-f', 'SourceCodeVariable-Roman.otf',
                                '-o', 'cff2'] + args)
    expected_path = _get_expected_path(exp_filename)
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_varread_pr355():
    # read CFF2 VF, write Type1 snapshot
    actual_path = runner(CMD + ['-o', 't1', '-f', 'cff2_vf.otf'])
    expected_path = _get_expected_path('cff2_vf.pfa')
    assert differ([expected_path, actual_path])


def test_cff2_no_vf_bug353():
    # read CFF2 WITHOUT VF info, write a CFF2 out. 'regular_CFF2.otf'
    # is derived by taking the regular.otf file from the sfntdiff
    # 'input_data' directory, and converting the CFF table to CFF2.
    actual_path = runner(CMD + ['-o', 'cff2', '-f', 'regular_CFF2.otf'])
    expected_path = _get_expected_path('regular_CFF2.cff2')
    assert differ([expected_path, actual_path, '-m', 'bin'])


# -----------
# Other tests
# -----------

def test_trademark_string_pr425():
    # the copyright symbol used in the trademark field of a UFO is
    # converted to 'Copyright' and stored in Notice field of a Type1
    actual_path = runner(CMD + ['-o', 't1', '-f', 'trademark.ufo'])
    expected_path = _get_expected_path('trademark.pfa')
    assert differ([expected_path, actual_path])


def test_remove_hints_bug180():
    actual_path = runner(CMD + ['-o', 't1', 'n', '-f', 'cid.otf'])
    expected_path = _get_expected_path('cid_nohints.ps')
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_long_charstring_read_bug444():
    # read a CFF2 VF with a charstring longer that 65535, check output
    actual_path = runner(CMD + ['-o', '0', '-f', 'CJK-VarTest.otf'])
    expected_path = _get_expected_path('CJK-VarTest_read.txt')
    assert differ([expected_path, actual_path, '-s', '## Filename'])


def test_long_charstring_warning():
    # read a CFF2 VF with a charstring longer that 65535, check warning message
    # NOTE: can't diff the output against 'CJK-VarTest_warn.txt' because on
    # Windows the lines start with 'tx.exe:' instead of just 'tx:'
    actual_path = runner(
        CMD + ['-r', '-e', '-o', '5', '-f', 'CJK-VarTest.otf'])
    # expected_path = _get_expected_path('CJK-VarTest_warn.txt')
    with open(actual_path, 'rb') as f:
        output = f.read()
    assert b"(cfr) Warning: CharString of GID 1 is 71057 bytes long" in output


def test_long_charstring_write():
    # read a CFF2 VF with a charstring longer that 65535, write out CFF2 file
    # NOTE: the font 'CJK-VarTest.otf' cannot be used in this test because
    # once its long charstring is optimized (floats -> ints) it's no longer
    # over the 65535 bytes limit; the long charstring in 'CJK-VarTest2.otf' is
    # already as small as possible, so it will trigger the check in cffwrite.c
    actual_path = runner(CMD + ['-o', 'cff2', '-f', 'CJK-VarTest2.otf'])
    expected_path = _get_expected_path('CJK-VarTest2.cff2')
    assert differ([expected_path, actual_path, '-m', 'bin'])


def test_many_hints_string_bug354():
    # The glyph T@gid002 has 33 hstem hints. This tests a bug where
    # tx defined an array of only 6 operands.
    # This is encountered only when wrinting to a VF CFF2.
    cff2_path = runner(CMD + ['-o', 'cff2', '-f', 'cff2_vf.otf'])
    dcf_txt_path = runner(CMD + ['-a', '-f', cff2_path, '-o', 'dcf'])
    expected_path = _get_expected_path('cff2_vf.dcf.txt')
    assert differ([expected_path, dcf_txt_path])


def test_non_varying_glyphs_bug356():
    """A glyph which is non-varying in a variable font may be referenced by a
    VariationStore data item subtable which has a region count of 0. The VF
    support code assumed that this was an error, and issued a false warning.
    File 'bug356.otf' is a handcrafted modification of 'cff2_vf.otf'. The
    latter cannot be used as-is to validate the fix."""
    stderr_path = runner(CMD + ['-r', '-e', '-o', 'cff', '-f', 'bug356.otf'])
    expected_path = _get_expected_path('bug356.txt')
    assert differ([expected_path, stderr_path, '-l', '1'])


def test_bug473():
    save_path = os.path.join(_get_temp_dir_path(), 'bug473.ufo')
    ufo_path = runner(CMD + ['-o', 'ufo', '-f', 'bug473.ufo', '-s', save_path])
    expected_path = _get_expected_path('bug473.ufo')
    assert differ([expected_path, ufo_path])


def test_subroutine_sorting_bug494():
    """ The input file was made with the command:
    tx -t1 -g 0-5 \
        source-serif-pro/Roman/Instances/Regular/font.ufo bug494.pfa
    The bug is that two subroutines in the Windows CFF output are swapped in
    index order from the Mac version. This was because of an unstable
    'qsort' done on the subroutines in the final stage of selection."""
    cff_path = runner(
        CMD + ['-o', 'cff', '*S', 'std', '*b', '-f', 'bug494.pfa'])
    dcf_txt_path = runner(CMD + ['-a', '-f', cff_path, '-o', 'dcf'])
    expected_path = _get_expected_path('bug494.dcf.txt')
    assert differ([expected_path, dcf_txt_path])
