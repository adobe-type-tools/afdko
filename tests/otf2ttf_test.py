from os import path, remove
import pytest
from time import sleep

from afdko.otf2ttf import main as otf2ttf
from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
)
from test_utils import (
    get_input_path,
    get_expected_path,
    generate_ttx_dump,
    font_has_table,
)
from runner import main as runner
from differ import main as differ, SPLIT_MARKER

TOOL = 'otf2ttf'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

def test_multiple_inputs_fail():
    path1 = get_input_path('latincid.otf')
    path2 = get_input_path('sans.otf')
    out_path = get_temp_file_path()
    with pytest.raises(SystemExit) as exc_info:
        otf2ttf(['-o', out_path, path1, path2])
    assert exc_info.value.code == 2


def test_wildcard_inputs_fail():
    paths = map(get_input_path, ['latincid.otf*', 'sans.ot?', 'serif.ot[cf]'])
    out_path = get_temp_file_path()
    with pytest.raises(SystemExit) as exc_info:
        otf2ttf(['-o', out_path, *paths])
    assert exc_info.value.code == 2


def test_multiple_inputs():
    fname1 = 'latincid'
    fname2 = 'sans'
    path1 = get_input_path(f'{fname1}.otf')
    path2 = get_input_path(f'{fname2}.otf')
    out_dir = get_temp_dir_path()
    out_path1 = path.join(out_dir, f'{fname1}.ttf')
    out_path2 = path.join(out_dir, f'{fname2}.ttf')
    assert all([path.exists(out_path1), path.exists(out_path2)]) is False
    otf2ttf(['-o', out_dir, path1, path2])
    assert all([path.exists(out_path1), path.exists(out_path2)]) is True


def test_wildcard_inputs():
    fnames = ['latincid', 'sans', 'serif']
    paths = map(get_input_path, ['latincid.otf*', 'sans.ot?', 'serif.ot[cf]'])
    out_dir = get_temp_dir_path()
    out_paths = [path.join(out_dir, f'{fname}.ttf') for fname in fnames]
    assert all(map(path.exists, out_paths)) is False
    otf2ttf(['-o', out_dir, *paths])
    assert all(map(path.exists, out_paths)) is True


def test_overwrite():
    in_path = get_input_path('sans.otf')
    in_head = path.splitext(in_path)[0]
    out_path1 = f'{in_head}.ttf'
    out_path2 = f'{in_head}#1.ttf'
    assert all([path.exists(out_path1), path.exists(out_path2)]) is False
    otf2ttf([in_path])  # 1st run; creates 'sans.ttf'
    assert font_has_table(out_path1, 'glyf')
    timestamp_run1 = path.getmtime(out_path1)
    sleep(1)
    otf2ttf(['--overwrite', in_path])  # 2nd run; overwrites 'sans.ttf'
    timestamp_run2 = path.getmtime(out_path1)
    assert timestamp_run1 != timestamp_run2  # 'sans.ttf' was modified
    sleep(1)
    otf2ttf([in_path])  # 3rd run; doesn't overwrite 'sans.ttf' (default)
    timestamp_run3 = path.getmtime(out_path1)
    assert path.exists(out_path2)  # 'sans#1.ttf' exists
    assert timestamp_run2 == timestamp_run3  # 'sans.ttf' was not modified
    for pth in (out_path1, out_path2):
        remove(pth)


def test_post_overflow():
    filename = 'blankcid'
    input_path = f'{filename}.otf'
    actual_path = get_temp_file_path()
    msg_path = runner(CMD + ['-s', '-e', '-o', 'o', f'_{actual_path}',
                             '=post-format', '_2', '-f', input_path])
    actual_ttx = generate_ttx_dump(actual_path, ['post'])
    expected_ttx = get_expected_path(f'{filename}.ttx')
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
    with open(msg_path, 'rb') as f:
        output = f.read()
    assert (b"WARNING: Dropping glyph names, they do not fit in 'post' "
            b"table.") in output


@pytest.mark.parametrize('filename', ['sans', 'serif', 'latincid', 'kanjicid'])
def test_convert(filename):
    input_path = get_input_path(f'{filename}.otf')
    actual_path = get_temp_file_path()
    otf2ttf(['-o', actual_path, input_path])
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(f'{filename}.ttx')
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttFont sfntVersion' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <checkSumAdjustment value=' + SPLIT_MARKER +
                   '    <created value=' + SPLIT_MARKER +
                   '    <modified value='])


@pytest.mark.parametrize('filename', ['otf_otf', 'otf_ttf', 'ttf_otf'])
def test_convert_ttc(filename):
    input_path = get_input_path(f'{filename}.ttc')
    actual_path = get_temp_file_path()
    otf2ttf(['-o', actual_path, input_path])
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path('ttf_ttf.ttx')
    assert differ([expected_ttx, actual_ttx,
                   '-s',
                   '<ttCollection ttLibVersion=' + SPLIT_MARKER +
                   '      <checkSumAdjustment value=' + SPLIT_MARKER +
                   '      <checkSumAdjustment value=' + SPLIT_MARKER +
                   '      <created value=' + SPLIT_MARKER +
                   '      <modified value='])


@pytest.mark.parametrize('file', ['font.ttf', 'ttf_ttf.ttc'])
def test_skip_ttf(file):
    warn = 'Not a OpenType font' if path.splitext(file)[1] != '.ttc' \
        else 'a Font Collection that has Not a OpenType font'
    input_path = get_input_path(file)
    out_path = path.join(get_temp_dir_path(), file)
    assert path.exists(out_path) is False
    msg_path = runner(CMD + ['-s', '-e', '-o', 'o', f'_{out_path}',
                             '=post-format', '_2', '-f', input_path])
    assert path.exists(out_path) is False
    with open(msg_path, 'rb') as f:
        output = f.read()
    assert (f'WARNING: "{input_path}" cannot be converted '
            f'since it is {warn}').encode() in output


@pytest.mark.parametrize('filename', ['otf_ttf', 'ttf_otf', 'ttf_ttf'])
def test_skip_ttf_in_ttc(filename):
    input_path = get_input_path(f'{filename}.ttc')
    out_path = path.join(get_temp_dir_path(), f'{filename}.ttc')
    msg_path = runner(CMD + ['-s', '-e', '-o', 'o', f'_{out_path}',
                             '=post-format', '_2', '-f', input_path])
    with open(msg_path, 'rb') as f:
        output = f.read()
    assert b'WARNING: Not a OpenType font (bad sfntVersion)' in output
