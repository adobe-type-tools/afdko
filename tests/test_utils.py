import inspect
import os
import tempfile

from fontTools.ttLib import TTFont


def get_data_dir():
    tests_dir, test_file = os.path.split(inspect.stack()[2][1])
    tool_name = test_file[:-8]  # trim '_test.py' portion
    return os.path.join(tests_dir, tool_name + '_data')


def get_expected_path(file_name):
    return os.path.join(get_data_dir(), 'expected_output', file_name)


def get_input_path(file_name):
    return os.path.join(get_data_dir(), 'input', file_name)


def get_temp_file_path(directory=None):
    file_descriptor, path = tempfile.mkstemp(dir=directory)
    os.close(file_descriptor)
    return path


def generate_ttx_dump(font_path, tables=None):
    with TTFont(font_path) as font:
        temp_path = get_temp_file_path()
        font.saveXML(temp_path, tables=tables)
        return temp_path


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
