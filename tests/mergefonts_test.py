import pytest
import subprocess

from runner import main as runner
from differ import main as differ
from test_utils import (get_expected_path, get_temp_file_path, get_input_path,
                        generate_ps_dump)
from afdko.fdkutils import (get_temp_dir_path)

TOOL = 'mergefonts'
CMD = ['-t', TOOL]


# -----
# Tests
# -----

@pytest.mark.parametrize('arg', ['-h', '-v', '-u'])
def test_exit_known_option(arg):
    assert subprocess.call([TOOL, arg]) == 0


@pytest.mark.parametrize('arg', ['-z', '-foo'])
def test_exit_unknown_option(arg):
    assert subprocess.call([TOOL, arg]) == 1


def test_convert_to_cid():
    font1_filename = 'font1.pfa'
    font2_filename = 'font2.pfa'
    font3_filename = 'font3.pfa'
    alias1_filename = 'alias1.txt'
    alias2_filename = 'alias2.txt'
    alias3_filename = 'alias3.txt'
    fontinfo_filename = 'cidfontinfo.txt'
    actual_path = get_temp_file_path()

    expected_path = get_expected_path('cidfont.ps')
    runner(CMD + ['-o', 'cid', '-f', fontinfo_filename, actual_path,
                  alias1_filename, font1_filename,
                  alias2_filename, font2_filename,
                  alias3_filename, font3_filename])

    actual_path = generate_ps_dump(actual_path)
    expected_path = generate_ps_dump(expected_path)

    assert differ([expected_path, actual_path, '-s', r'%ADOt1write:'])


def test_warnings_bug635():
    font1_path = get_input_path('bug635/cidfont.ps')
    font2_path = get_input_path('bug635/rotated.ps')
    actual_path = get_temp_file_path()
    expected_path = get_expected_path('bug635.txt')
    warnings_path = runner(
        CMD + ['-s', '-e', '-a', '-f', actual_path, font1_path, font2_path])
    # On Windows the messages start with 'mergefonts.exe:'
    with open(warnings_path, 'rb') as f:
        warnings = f.read().replace(b'mergefonts.exe:', b'mergefonts:')
    with open(warnings_path, 'wb') as f:
        f.write(warnings)
    assert differ([expected_path, warnings_path, '-l', '1,5-7,11-12'])


def test_camel_case():
    # confirm old 'mergeFonts' name in glyph alias files still works
    # (as opposed to the new 'mergefonts' name)
    font1_filename = 'font1.pfa'
    font2_filename = 'font2.pfa'
    font3_filename = 'font3.pfa'
    alias1_filename = 'camelCase/alias1.txt'
    alias2_filename = 'camelCase/alias2.txt'
    alias3_filename = 'camelCase/alias3.txt'
    fontinfo_filename = 'cidfontinfo.txt'
    actual_path = get_temp_file_path()

    expected_path = get_expected_path('cidfont.ps')
    runner(CMD + ['-o', 'cid', '-f', fontinfo_filename, actual_path,
                  alias1_filename, font1_filename,
                  alias2_filename, font2_filename,
                  alias3_filename, font3_filename])

    actual_path = generate_ps_dump(actual_path)
    expected_path = generate_ps_dump(expected_path)

    assert differ([expected_path, actual_path, '-s', r'%ADOt1write:'])


def test_cidkeyed_merge_ufos():
    """
    Test merging cid-keyed UFOs into one.
    """
    input_ufo_a = get_input_path("mergeufo/cidkeyed/" + 'ufo-1.ufo')
    input_alias_a = get_input_path("mergeufo/cidkeyed/" + 'alias1.txt')
    input_ufo_b = get_input_path("mergeufo/cidkeyed/" + 'ufo-2.ufo')
    input_alias_b = get_input_path("mergeufo/cidkeyed/" + 'alias2.txt')
    expected_path = get_expected_path("cidkeyed-merge.ufo")
    output_path = get_temp_dir_path("temp_cidkeyed-merge.ufo")
    subprocess.call([TOOL, output_path,
                     input_alias_a, input_ufo_a,
                     input_alias_b, input_ufo_b])
    assert differ([expected_path, output_path])


def test_namekeyed_merge_ufos():
    """
    Test merging name-keyed UFOs into one.
    """
    input_ufo_c = get_input_path("mergeufo/namekeyed/" + 'ufo-1.ufo')
    input_ufo_d = get_input_path("mergeufo/namekeyed/" + 'ufo-2.ufo')
    expected_path = get_expected_path("namekeyed-merge.ufo")
    output_path = get_temp_dir_path("temp_namekeyed-merge.ufo")
    subprocess.call([TOOL, output_path, input_ufo_c, input_ufo_d])
    assert differ([expected_path, output_path])
