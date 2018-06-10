from __future__ import print_function, division, absolute_import

import os
import tempfile

from runner import main as runner
from differ import main as differ
from differ import SPLIT_MARKER

TOOL = 'tx'
CMD = ['-t', TOOL]

UFO2_NAME = 'ufo2.ufo'
TYPE1_NAME = 'type1.pfa'
CFF2_SIMPLE_VF_NAME = 'test_simple_cff2_vf.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


def _get_temp_dir_path():
    return tempfile.mkdtemp()


# -------------------
# Convert UFO2 to ...
# -------------------

def test_convert_ufo2_to_ufo2():
    # tx can't overwrite an existing file, so use 'save' option
    save_path = os.path.join(_get_temp_dir_path(), 'font.ufo')
    actual_path = runner(
        CMD + ['-o', 'ufo', '-f', UFO2_NAME, '-s', save_path])
    expected_path = _get_expected_path(UFO2_NAME)
    assert differ([expected_path, actual_path]) is True


def test_convert_ufo2_to_type1():
    actual_path = runner(CMD + ['-o', 't1', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.pfa')
    assert differ([expected_path, actual_path]) is True


def test_convert_ufo2_to_svg():
    actual_path = runner(CMD + ['-o', 'svg', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.svg')
    assert differ([expected_path, actual_path]) is True


def test_convert_ufo2_to_mtx():
    actual_path = runner(CMD + ['-o', 'mtx', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.mtx')
    assert differ([expected_path, actual_path]) is True


def test_convert_ufo2_to_afm():
    actual_path = runner(CMD + ['-o', 'afm', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.afm')
    assert differ([expected_path, actual_path,
                   '-s', 'Comment Creation Date:']) is True


def test_convert_ufo2_to_pdf():
    actual_path = runner(CMD + ['-o', 'pdf', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.pdf')
    assert differ([expected_path, actual_path,
                   '-s',
                   '/Creator' + SPLIT_MARKER +
                   '/Producer' + SPLIT_MARKER +
                   '/CreationDate' + SPLIT_MARKER +
                   '/ModDate' + SPLIT_MARKER +
                   '<< /Length' + SPLIT_MARKER +
                   '(Filename:' + SPLIT_MARKER +
                   '(Date:' + SPLIT_MARKER +
                   '(Time:',
                   '-l', '36,38,240-254,262'
                   ]) is True


def test_convert_ufo2_to_ps():
    actual_path = runner(CMD + ['-o', 'ps', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '0 740 moveto (Filename:' + SPLIT_MARKER +
                   '560 (Date:' + SPLIT_MARKER +
                   '560 (Time:'
                   ]) is True


def test_convert_ufo2_to_cff():
    actual_path = runner(CMD + ['-o', 'cff', '-f', UFO2_NAME])
    expected_path = _get_expected_path('ufo2.cff')
    assert differ([expected_path, actual_path, '-m', 'bin']) is True


# --------------------
# Convert Type1 to ...
# --------------------

def test_convert_type1_to_type1():
    actual_path = runner(CMD + ['-o', 't1', '-f', TYPE1_NAME])
    expected_path = _get_expected_path(TYPE1_NAME)
    assert differ([expected_path, actual_path]) is True


def test_convert_type1_to_ufo2():
    # tx can't overwrite an existing file, so use 'save' option
    save_path = os.path.join(_get_temp_dir_path(), 'font.ufo')
    actual_path = runner(
        CMD + ['-o', 'ufo', '-f', TYPE1_NAME, '-s', save_path])
    expected_path = _get_expected_path('type1.ufo')
    assert differ([expected_path, actual_path]) is True


def test_convert_type1_to_svg():
    actual_path = runner(CMD + ['-o', 'svg', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.svg')
    assert differ([expected_path, actual_path]) is True


def test_convert_type1_to_mtx():
    actual_path = runner(CMD + ['-o', 'mtx', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.mtx')
    assert differ([expected_path, actual_path]) is True


def test_convert_type1_to_afm():
    actual_path = runner(CMD + ['-o', 'afm', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.afm')
    assert differ([expected_path, actual_path,
                   '-s', 'Comment Creation Date:']) is True


def test_convert_type1_to_pdf():
    actual_path = runner(CMD + ['-o', 'pdf', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.pdf')
    assert differ([expected_path, actual_path,
                   '-s',
                   '/Creator' + SPLIT_MARKER +
                   '/Producer' + SPLIT_MARKER +
                   '/CreationDate' + SPLIT_MARKER +
                   '/ModDate' + SPLIT_MARKER +
                   '<< /Length' + SPLIT_MARKER +
                   '(Filename:' + SPLIT_MARKER +
                   '(Date:' + SPLIT_MARKER +
                   '(Time:',
                   '-l', '36,38,240-254,262'
                   ]) is True


def test_convert_type1_to_ps():
    actual_path = runner(CMD + ['-o', 'ps', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.ps')
    assert differ([expected_path, actual_path,
                   '-s',
                   '0 740 moveto (Filename:' + SPLIT_MARKER +
                   '560 (Date:' + SPLIT_MARKER +
                   '560 (Time:'
                   ]) is True


def test_convert_type1_to_cff():
    actual_path = runner(CMD + ['-o', 'cff', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.cff')
    assert differ([expected_path, actual_path, '-m', 'bin']) is True


# ----------
# Dump Type1
# ----------

def test_dump_dflt_type1():
    actual_path = runner(CMD + ['-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump1.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_0_type1():
    actual_path = runner(CMD + ['-o', '0', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump0.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_0_type1_explicit_options():
    actual_path = runner(CMD + ['-o', 'dump', '0', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump0.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_1_type1():
    actual_path = runner(CMD + ['-o', '1', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump1.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_2_type1():
    actual_path = runner(CMD + ['-o', '2', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump2.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_3_type1():
    actual_path = runner(CMD + ['-o', '3', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump3.txt')
    assert differ([expected_path, actual_path,
                   '-s', '## Filename']) is True


def test_dump_4_type1():
    actual_path = runner(CMD + ['-o', '4', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump4.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_5_type1():
    actual_path = runner(CMD + ['-o', '5', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump5.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_6_type1():
    actual_path = runner(CMD + ['-o', '6', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump6.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_6_d_type1():
    actual_path = runner(CMD + ['-o', '6', 'd', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump6d.txt')
    assert differ([expected_path, actual_path]) is True


def test_dump_6_n_type1():
    actual_path = runner(CMD + ['-o', '6', 'n', '-f', TYPE1_NAME])
    expected_path = _get_expected_path('type1.dump6n.txt')
    assert differ([expected_path, actual_path]) is True


# ----------
# Test CFF2 font functions
# ----------

def test_varread():
    # Test read CCF2 VF, write snapshot.
    actual_path = runner(CMD + ['-o', 't1', '-f', CFF2_SIMPLE_VF_NAME])
    new_file = CFF2_SIMPLE_VF_NAME.rsplit('.', 1)[0] + '.pfa'
    expected_path = _get_expected_path(new_file)
    assert differ([expected_path, actual_path]) is True
