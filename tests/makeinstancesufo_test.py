import argparse
import os
import pytest
from shutil import rmtree, copytree

from afdko.makeinstancesufo import (
    main as mkinstufo,
    get_options,
    _split_comma_sequence,
    updateInstance,
)
from runner import main as runner
from differ import main as differ
from test_utils import get_input_path

TOOL = 'makeinstancesufo'

DATA_DIR = os.path.join(os.path.dirname(__file__), TOOL + '_data')
TEMP_DIR = os.path.join(DATA_DIR, "temp_output")


def _get_output_path(file_name, dir_name):
    return os.path.join(DATA_DIR, dir_name, file_name)


def setup_module():
    """
    Create the temporary output directory
    """
    rmtree(TEMP_DIR, ignore_errors=True)
    os.mkdir(TEMP_DIR)


def teardown_module():
    """
    teardown the temporary UFOs or the directory that holds them
    """
    rmtree(os.path.join(TEMP_DIR), True)
    rmtree(os.path.join(DATA_DIR, 'input', 'same_dir.ufo'), True)


# -----
# Tests
# -----

@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'extralight.ufo'),  # hint/remove overlap/normalize/round
    (['_1', 'a'], 'light.ufo'),  # no hint
    (['_2', 'r'], 'regular.ufo'),  # no round
    (['_3', 'r', 'n'], 'regular1.ufo'),  # no round & no normalize
    (['_4', 'c'], 'semibold.ufo'),  # no remove overlap
    (['_5', 'n'], 'bold.ufo'),  # no normalize
    (['_6', 'a', 'c', 'n'], 'black.ufo'),  # round only
    (['_7', 'a', 'c', 'n'], 'anisotropic.ufo'),
    (['_8', 'a', '=ufo-version', '_2'], 'ufo2.ufo'),  # no hint UFO v2
])
def test_options(args, ufo_filename):
    runner(['-t', TOOL, '-o', 'd',
            f'_{get_input_path("font.designspace")}', 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('v_arg', ['', 'v', 'vv', 'vvv'])
def test_log_level(v_arg):
    dspath = get_input_path('font.designspace')
    v_val = len(v_arg)
    arg = [] if not v_val else [f'-{v_arg}']
    opts = get_options(['-d', dspath] + arg)
    assert opts.verbose == v_val


@pytest.mark.parametrize('str_seq, lst_seq', [
    ('10', [10]),
    ('11,13,19', [11, 13, 19]),
    ('16,0,13', [0, 13, 16]),
])
def test_split_comma_sequence(str_seq, lst_seq):
    assert _split_comma_sequence(str_seq) == lst_seq


@pytest.mark.parametrize('str_seq', ['0,a', '1,-2', '10-10', '5+4', '3*5'])
def test_split_comma_sequence_error(str_seq):
    with pytest.raises(argparse.ArgumentTypeError):
        _split_comma_sequence(str_seq)


@pytest.mark.parametrize('filename', [
    'noaxes', 'nosources', 'nosourcepath', 'badsourcepath',
    'noinstances', 'noinstancepath', ('badinstanceindex', 2)
])
def test_validate_designspace_doc_raise(filename):
    args = []
    if isinstance(filename, tuple):
        filename, idx = filename
        args = ['-i', f'{idx}']
    dspath = get_input_path(f'{filename}.designspace')
    assert mkinstufo(['-d', dspath] + args) == 1


@pytest.mark.parametrize('args', [
    [],  # checkoutlinesufo call
    ['-c'],  # psautohint call
])
def test_update_instance_raise(args):
    dspath = get_input_path('font.designspace')
    ufopath = get_input_path('invalid.ufo')
    opts = get_options(['-d', dspath] + args)
    with pytest.raises(SystemExit):
        updateInstance(ufopath, opts)


def test_filename_without_dir():
    instance_path = get_input_path('same_dir.ufo')
    assert not os.path.exists(instance_path)
    runner(['-t', TOOL, '-o', 'd',
            f'_{get_input_path("font.designspace")}', 'i', '_9'])
    assert os.path.exists(instance_path)


@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'ufo3regular.ufo'),  # hint/remove overlap/normalize/round
    (['_1'], 'ufo3regular.ufo'),  # for testing instance removal
    (['_2', 'a', 'c', 'n'], 'ufo3medium.ufo'),  # round only
    (['_3', 'a', '=ufo-version', '_2'], 'ufo3semibold.ufo'),  # no hint UFO v2
])
def test_ufo3_masters(args, ufo_filename):
    runner(['-t', TOOL, '-o', 'd',
            f'_{get_input_path("ufo3.designspace")}', 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('filename', ['features_copy', 'features_nocopy'])
def test_features_copy(filename):
    # NOTE: This test was originally implemented without copying the expected
    # UFOs to a temp location, but the Windows64 build always modified the glif
    # files' line endings after the test was ran and this caused wheel problems
    # https://ci.appveyor.com/project/adobe-type-tools/afdko/builds/25459479
    # ---
    # First copy the expected UFOs into the temp folder. The UFOs need to
    # exist before makeinstancesufo is called because this test is about
    # checking that any existing features.fea files are preserved.
    paths = []
    for i in (1, 2):  # two instances
        ufo_filename = f'{filename}{i}.ufo'
        from_path = _get_output_path(ufo_filename, 'expected_output')
        to_path = os.path.join(TEMP_DIR, ufo_filename)
        copytree(from_path, to_path)
        paths.append((to_path, from_path))
    # run makeinstancesufo
    runner(['-t', TOOL, '-o', 'a', 'c', 'n', 'd',
            f'_{get_input_path(f"{filename}.designspace")}'])
    # assert the expected results
    for expected_path, actual_path in paths:
        assert differ([expected_path, actual_path])


@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'bend1.ufo'),
    (['_1'], 'bend2.ufo'),
    (['_2'], 'bend3.ufo'),
])
def test_bend_masters_mutator_math(args, ufo_filename):
    # MutatorMath 2.1.2 did not handle location bending properly which resulted
    # in incorrect interpolation outlines. This was fixed in 3.0.1.
    runner(['-t', TOOL, '-o', 'a', 'c', 'n', 'd',
            f'_{get_input_path("bend_test.designspace")}', 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])


@pytest.mark.parametrize('args, ufo_filename', [
    (['_0'], 'bend1.ufo'),
    (['_1'], 'bend2.ufo'),
    (['_2'], 'bend3.ufo'),
])
def test_bend_masters_varlib(args, ufo_filename):
    # We should get the same output passing through varLib
    runner(['-t', TOOL, '-o', 'a', 'c', 'n', '=use-varlib', 'd',
            f'_{get_input_path("bend_test.designspace")}', 'i'] + args)
    expected_path = _get_output_path(ufo_filename, 'expected_output')
    actual_path = _get_output_path(ufo_filename, 'temp_output')
    assert differ([expected_path, actual_path])
