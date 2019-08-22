import argparse
import pytest

from differ import (
    main as differ,
    get_options,
    _compile_regex,
    _convert_to_int,
    _split_linenumber_sequence,
    _validate_path,
    _get_path_kind,
)
from test_utils import get_input_path


@pytest.mark.parametrize('v_arg', ['', 'v', 'vv', 'vvv'])
def test_log_level(v_arg):
    fp1, fp2 = get_input_path('file1'), get_input_path('file2')
    v_val = len(v_arg)
    arg = [] if not v_val else [f'-{v_arg}']
    opts = get_options([fp1, fp2] + arg)
    assert opts.verbose == v_val


@pytest.mark.parametrize('fname, result', [
    ('folder_copy', True),
    ('folder_mod_file', False),
    ('folder_less_file', False),
    ('folder_more_file', False),
])
@pytest.mark.parametrize('difmod', [[], ['-m', 'bin']])
def test_compare_dir_contents(fname, result, difmod):
    fp1, fp2 = get_input_path('folder'), get_input_path(fname)
    assert differ([fp1, fp2] + difmod) is result


@pytest.mark.parametrize('difmod', [[], ['-m', 'bin']])
def test_compare_files(difmod):
    fp1, fp2 = get_input_path('file1'), get_input_path('file2')
    assert differ([fp1, fp2] + difmod) is False


def test_file_encoding():
    fp1, fp2 = get_input_path('file1'), get_input_path('file2_macroman')
    with pytest.raises(SystemExit) as exc_info:
        differ([fp1, fp2])
    assert exc_info.value.code == 1
    assert differ([fp1, fp2, '-e', 'macroman']) is False


def test_paths_not_same_kind():
    fp1, fp2 = get_input_path('file1'), get_input_path('folder')
    with pytest.raises(SystemExit) as exc_info:
        differ([fp1, fp2])
    assert exc_info.value.code == 2


def test_validate_path():
    with pytest.raises(argparse.ArgumentTypeError):
        _validate_path(get_input_path('not_a_file_or_folder'))


@pytest.mark.parametrize('pth, result', [
    ('not_a_file_or_folder', 'invalid'),
    ([], 'invalid'),
])
def test_get_path_kind(pth, result):
    assert _get_path_kind(pth) == result


@pytest.mark.parametrize('regex', ['', '^(ab'])
def test_compile_regex(regex):
    with pytest.raises(argparse.ArgumentTypeError):
        _compile_regex(regex)


@pytest.mark.parametrize('string', ['', 'a'])
def test_convert_to_int(string):
    with pytest.raises(argparse.ArgumentTypeError):
        _convert_to_int(string)


@pytest.mark.parametrize('str_seq, tup_seq', [
    ('10', (10,)),
    ('11,13,19', (11, 13, 19)),
    ('13-16', (13, 14, 15, 16)),
    ('15+4', (15, 16, 17, 18, 19)),
    ('10-10,5+4,2,3-5', (2, 3, 4, 5, 6, 7, 8, 9, 10)),
])
def test_split_linenumber_sequence(str_seq, tup_seq):
    assert _split_linenumber_sequence(str_seq) == tup_seq


@pytest.mark.parametrize('str_seq', ['9*7', '7_9', '9-7'])
def test_split_linenumber_sequence_error(str_seq):
    with pytest.raises(argparse.ArgumentTypeError):
        _split_linenumber_sequence(str_seq)
