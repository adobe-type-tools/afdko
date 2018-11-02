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


def font_has_table(font_path, table_tag):
    with TTFont(font_path) as font:
        return table_tag in font
