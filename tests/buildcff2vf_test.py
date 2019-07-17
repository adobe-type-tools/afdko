import os
from shutil import copytree
import tempfile

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path, get_expected_path, generate_ttx_dump

TOOL = 'buildcff2vf'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

def test_rvrn_vf():
    input_dir = get_input_path('GSUBVar')
    temp_dir = os.path.join(tempfile.mkdtemp(), 'GSUBVar')
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
    temp_dir = os.path.join(tempfile.mkdtemp(), 'CJKVar')
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
    temp_dir = os.path.join(tempfile.mkdtemp(), 'CJKSparseVar')
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
    temp_dir = os.path.join(tempfile.mkdtemp(), 'bug816var')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug816.designspace')
    runner(CMD + ['-o', 'c', 'd', f'_{ds_path}'])
    actual_path = os.path.join(temp_dir, 'bug816.otf')
    actual_ttx = generate_ttx_dump(actual_path, ['CFF2'])
    expected_ttx = get_expected_path('bug816.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])


def test_subset_vf():
    input_dir = get_input_path('bug817')
    temp_dir = os.path.join(tempfile.mkdtemp(), 'bug817var')
    copytree(input_dir, temp_dir)
    ds_path = os.path.join(temp_dir, 'bug817.designspace')
    subset_path = os.path.join(temp_dir, 'bug817.subset')
    runner(CMD + ['-o', 'd', f'_{ds_path}', 'i', f'_{subset_path}'])
    actual_path = os.path.join(temp_dir, 'bug817.otf')
    actual_ttx = generate_ttx_dump(actual_path, ['GSUB'])
    expected_ttx = get_expected_path('bug817.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
