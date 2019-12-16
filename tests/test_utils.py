import inspect
import os
import shutil
import subprocess

from fontTools.ttLib import TTCollection, TTFont, TTLibError
from afdko.fdkutils import (get_temp_file_path, run_shell_command)

TIMEOUT = 240  # seconds


def generalizeCFF(otfPath, do_sfnt=True):
    """
    Adapted from similar routine in 'buildmasterotfs'. This uses
    temp files for both tx output and sfntedit output instead of
    overwriting 'otfPath', and also provides an option to skip
    the sfntedit step (so 'otfPath' can be either a .otf file or
    a .cff file).
    """
    tmp_tx_path = get_temp_file_path()
    out_path = get_temp_file_path()
    shutil.copyfile(otfPath, out_path, follow_symlinks=True)

    if not run_shell_command(['tx', '-cff', '+b', '-std', '-no_opt',
                              otfPath, tmp_tx_path]):
        raise Exception

    if do_sfnt:

        if not run_shell_command(['sfntedit', '-a',
                                  f'CFF ={tmp_tx_path}', out_path]):
            raise Exception

        return out_path

    else:

        return tmp_tx_path


def get_data_dir():
    tests_dir, test_file = os.path.split(inspect.stack()[2][1])
    tool_name = test_file[:-8]  # trim '_test.py' portion
    return os.path.join(tests_dir, tool_name + '_data')


def get_expected_path(file_name):
    return os.path.join(get_data_dir(), 'expected_output', file_name)


def get_input_path(file_name):
    return os.path.join(get_data_dir(), 'input', file_name)


def get_bad_input_path(file_name):
    return os.path.join(get_data_dir(), 'input', 'bad', file_name)


def generate_ttx_dump(font_path, tables=None):
    try:
        font = TTFont(font_path)
    except TTLibError:
        font = TTCollection(font_path)

    with font:
        temp_path = get_temp_file_path()
        font.saveXML(temp_path, tables=tables)
        return temp_path


def generate_spot_dumptables(font_path, tables):
    tmp_txt_path = get_temp_file_path()
    myargs = ['spot', "-t" + ",".join(tables), font_path]
    spot_txt = subprocess.check_output(myargs, timeout=TIMEOUT)
    tf = open(tmp_txt_path, "wb")
    tf.write(spot_txt)
    tf.close()
    return tmp_txt_path


def generate_ps_dump(font_path):
    with open(font_path, 'rb') as ps_file:
        data = ps_file.read()
    line = ''
    i = 0
    in_binary_section = False
    bytes_read = 0
    byte_count = 0
    temp_path = get_temp_file_path()
    with open(temp_path, 'w') as temp_file:
        while i < len(data):
            x = data[i]
            if not in_binary_section:
                if x != 0x0A:
                    line += '%c' % x
                else:
                    if line.startswith('%%BeginData'):
                        in_binary_section = True
                        byte_count = int(line.split()[1])
                    line += '\n'
                    temp_file.write(line)
                    line = ''
            else:
                bytes_read += 1
                if bytes_read == byte_count:
                    in_binary_section = False
                    if bytes_read % 32 != 1:
                        temp_file.write('\n')
                else:
                    temp_file.write('%02X' % x)
                    if bytes_read % 32 == 0:
                        temp_file.write('\n')
            i += 1
    return temp_path


def font_has_table(font_path, table_tag):
    with TTFont(font_path) as font:
        return table_tag in font


def get_font_revision(font_path):
    with TTFont(font_path) as font:
        return font['head'].fontRevision
