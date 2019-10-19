import os
from shutil import copytree

from afdko.fdkutils import get_temp_dir_path
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
)
from runner import main as runner
from differ import main as differ

TOOL = 'buildcff2vf'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

def test_rvrn_vf():
    input_dir = get_input_path('GSUBVar')
    temp_dir = get_temp_dir_path('GSUBVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'GSUBVar.designspace')
    runner(CMD + ['-o', 'd', f'_{ds_path}'])
    actual_path = os.path.join(temp_dir, 'GSUBVar.otf')
    actual_ttx = generate_ttx_dump(actual_path,
                                   ['CFF2', 'GSUB', 'avar', 'fvar'])
    expected_ttx = get_expected_path('GSUBVar.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_cjk_vf():
    input_dir = get_input_path('CJKVar')
    temp_dir = get_temp_dir_path('CJKVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'CJKVar.designspace')
    runner(CMD + ['-o', 'd', f'_{ds_path}'])
    actual_path = os.path.join(temp_dir, 'CJKVar.otf')
    actual_ttx = generate_ttx_dump(actual_path,
                                   ['CFF2', 'HVAR', 'avar', 'fvar'])
    expected_ttx = get_expected_path('CJKVar.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_sparse_cjk_vf():
    # subset fonts to lists in 'SHSansJPVFTest.subset.txt'
    # Check compatibility. Issue is in cid00089.
    # keep PostScript names
    input_dir = get_input_path('CJKSparseVar')
    temp_dir = get_temp_dir_path('CJKSparseVar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'SHSansJPVFTest.designspace')
    subset_path = os.path.join(temp_dir, 'SHSansJPVFTest.subset.txt')
    runner(CMD + ['-o', 'd', f'_{ds_path}', 'i', f'_{subset_path}', 'c', 'k'])
    actual_path = os.path.join(temp_dir, 'SHSansJPVFTest.otf')
    actual_ttx = generate_ttx_dump(actual_path,
                                   ['CFF2', 'HVAR', 'avar', 'fvar'])
    expected_ttx = get_expected_path('SHSansJPVFTest.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_compatibility_vf():
    input_dir = get_input_path('bug816')
    temp_dir = get_temp_dir_path('bug816var')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug816.designspace')
    runner(CMD + ['-o', 'c', 'd', f'_{ds_path}'])
    actual_path = os.path.join(temp_dir, 'bug816.otf')
    actual_ttx = generate_ttx_dump(actual_path, ['CFF2'])
    expected_ttx = get_expected_path('bug816.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_subset_vf():
    input_dir = get_input_path('bug817')
    temp_dir = get_temp_dir_path('bug817var')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug817.designspace')
    subset_path = os.path.join(temp_dir, 'bug817.subset')
    runner(CMD + ['-o', 'd', f'_{ds_path}', 'i', f'_{subset_path}'])
    actual_path = os.path.join(temp_dir, 'bug817.otf')
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path('bug817.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_bug1003(capfd):
    """
    Check that "the input set requires compatibilization" message is in
    stderr when run without '-c' option.
    """
    input_dir = get_input_path('bug1003')
    temp_dir = get_temp_dir_path('bug1003var')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug1003.designspace')
    runner(CMD + ['-o', 'd', f'_{ds_path}'])
    captured = capfd.readouterr()
    assert "The input set requires compatibilization" in captured.err


def test_bug1003_compat():
    """
    Check that file is properly built when '-c' is specified.
    """
    input_dir = get_input_path('bug1003')
    temp_dir = get_temp_dir_path('bug1003cvar')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug1003.designspace')
    runner(CMD + ['-o', 'c' 'd', f'_{ds_path}'])
    actual_path = os.path.join(temp_dir, 'bug1003.otf')
    actual_ttx = generate_ttx_dump(actual_path, ['CFF2'])
    expected_ttx = get_expected_path('bug1003.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
