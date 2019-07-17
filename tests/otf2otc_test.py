import os
import pytest

from runner import main as runner
from differ import main as differ
from test_utils import get_expected_path, get_temp_file_path

TOOL = 'otf2otc'
CMD = ['-t', TOOL]

REGULAR = 'SourceSansPro-Regular.otf'
ITALIC = 'SourceSansPro-It.otf'
BOLD = 'SourceSansPro-Bold.otf'

MSG_1 = (
    "Shared tables: "
    "['BASE', 'DSIG', 'GDEF', 'GSUB', 'cmap', 'maxp', 'post']%s"
    "Un-shared tables: "
    "['CFF ', 'GPOS', 'OS/2', 'head', 'hhea', 'hmtx', 'name']" %
    os.linesep).encode('ascii')

MSG_2 = (
    "Shared tables: ['BASE', 'DSIG']%s"
    "Un-shared tables: ['CFF ', 'GDEF', 'GPOS', 'GSUB', 'OS/2', 'cmap', "
    "'head', 'hhea', 'hmtx', 'maxp', 'name', 'post']" %
    os.linesep).encode('ascii')


# -----
# Tests
# -----

@pytest.mark.parametrize('otf_filenames, ttc_filename, tables_msg', [
    ([REGULAR, BOLD], 'RgBd.ttc', MSG_1),
    ([REGULAR, ITALIC], 'RgIt.ttc', MSG_2),
    ([REGULAR, ITALIC, BOLD], 'RgItBd.ttc', MSG_2),
])
def test_convert(otf_filenames, ttc_filename, tables_msg):
    actual_path = get_temp_file_path()
    expected_path = get_expected_path(ttc_filename)
    stdout_path = runner(CMD + ['-s', '-o', 'o', f'_{actual_path}',
                                '-f'] + otf_filenames)
    with open(stdout_path, 'rb') as f:
        output = f.read()
    assert tables_msg in output
    assert differ([expected_path, actual_path, '-m', 'bin'])
