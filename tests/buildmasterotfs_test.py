import os
import pytest
from shutil import copytree

from afdko.buildmasterotfs import (
    main as bmotfs,
    get_options,
    _preparse_args,
    _split_makeotf_options,
    generalizeCFF,
)
from afdko.fdkutils import get_temp_dir_path
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

TOOL = 'buildmasterotfs'
CMD = ['-t', TOOL]


def _compare_results(otf_path):
    actual_ttx = generate_ttx_dump(otf_path)
    expected_ttx = get_expected_path(
        os.path.basename(otf_path)[:-3] + 'ttx')
    return differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value=',
                   '-r', r'^\s+Version.*;hotconv.*;makeotfexe'])


# -----
# Tests
# -----

@pytest.mark.parametrize('v_arg', ['', 'v', 'vv', 'vvv'])
def test_log_level(v_arg):
    dspath = get_input_path('font.designspace')
    v_val = len(v_arg)
    arg = [] if not v_val else [f'-{v_arg}']
    opts = get_options(['-d', dspath] + arg)
    assert opts.verbose == v_val


@pytest.mark.parametrize('filename', ['nosources', 'badsourcepath'])
def test_validate_designspace_doc_raise(filename):
    dspath = get_input_path(f'{filename}.designspace')
    assert bmotfs(['-d', dspath]) == 1


@pytest.mark.parametrize('args, result', [
    ([], []),
    (['--mkot'], ['--mkot']),
    (['--mkot', 'a'], ['--mkot', 'a']),
])
def test_preparse_args(args, result):
    assert _preparse_args(args) == result


@pytest.mark.parametrize('comma_str, result', [
    ('a', ['-a']),
    ('-a', ['-a']),
    ('--a', ['--a']),
    ('a,b', ['-a', 'b']),
    ('-a,-b', ['-a', '-b']),
    ('--a,-b', ['--a', '-b']),
])
def test_split_makeotf_options(comma_str, result):
    assert _split_makeotf_options(comma_str) == result


@pytest.mark.parametrize('font_path', [
    'foo',   # tx call
    get_input_path('font.pfa'),  # sfntedit call
])
def test_generalize_cff_raise(font_path):
    with pytest.raises(Exception):
        generalizeCFF(font_path)


def test_build_otfs():
    ds_path = get_input_path('font.designspace')
    runner(CMD + ['-o', 'd', f'_{ds_path}'])
    otf1_path = get_input_path('master0.otf')
    otf2_path = get_input_path('master1.otf')
    for otf_path in (otf1_path, otf2_path):
        assert _compare_results(otf_path)
        if os.path.exists(otf_path):
            os.remove(otf_path)


def test_build_otfs_failure():
    ds_path = get_input_path('font.designspace')
    with pytest.raises(Exception):
        runner(CMD + ['-o', 'd', f'_{ds_path}', '=mkot', 'foobar'])


def test_cjk_var():
    """
    Builds all OTFs for the 'CJKVar' project and then diffs two of them.
    """
    input_dir = get_input_path('CJKVar')
    temp_dir = get_temp_dir_path('CJKVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'CJKVar.designspace')
    runner(CMD + ['-o', 'd', f'_{ds_path}', '=mkot',
                  'omitDSIG,-omitMacNames'])

    otf1_path = os.path.join(
        temp_dir, 'Normal', 'Master_8', 'MasterSet_Kanji-w600.00.otf')
    otf2_path = os.path.join(
        temp_dir, 'Condensed', 'Master_8', 'MasterSet_Kanji_75-w600.00.otf')

    for otf_path in (otf1_path, otf2_path):
        assert _compare_results(otf_path)
