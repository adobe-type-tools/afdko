import os
import pytest
from shutil import copy2, rmtree

from runner import main as runner
from differ import main as differ
from test_utils import get_input_path, get_expected_path, generate_ttx_dump

TOOL = 'otc2otf'
CMD = ['-t', TOOL]

REGULAR = 'SourceSansPro-Regular.otf'
ITALIC = 'SourceSansPro-It.otf'
BOLD = 'SourceSansPro-Bold.otf'

DATA_DIR = os.path.join(os.path.split(__file__)[0], TOOL + '_data')
TEMP_DIR = os.path.join(DATA_DIR, 'temp_output')


def setup_module():
    """
    Create the temporary output directory
    """
    rmtree(TEMP_DIR, ignore_errors=True)
    os.mkdir(TEMP_DIR)


def teardown_module():
    """
    teardown the temporary output directory
    """
    rmtree(TEMP_DIR)


# -----
# Tests
# -----

@pytest.mark.parametrize('ttc_filename, otf_filenames, diff_index', [
    ('RgBd.ttc', [REGULAR, BOLD], 0),
    ('RgIt.ttc', [REGULAR, ITALIC], 1),
    ('RgItBd.ttc', [REGULAR, ITALIC, BOLD], 2),
])
def test_convert(ttc_filename, otf_filenames, diff_index):
    # 'otc2otf' doesn't have an output option. The test makes a temporary
    # directory and copies the input TTC font there. The output OTFs will
    # be saved to that directory as well.
    input_ttc_path = get_input_path(ttc_filename)
    temp_ttc_path = os.path.join(TEMP_DIR, ttc_filename)
    copy2(input_ttc_path, temp_ttc_path)

    fonts_msg = os.linesep.join(
        [f'Output font: {fname}' for fname in otf_filenames])

    stdout_path = runner(CMD + ['-s', '-a', '-f', temp_ttc_path])

    with open(stdout_path, 'rb') as f:
        output = f.read()
    assert fonts_msg.encode('ascii') in output

    # diff only one of the OTFs
    otf_filename = otf_filenames[diff_index]
    actual_path = os.path.join(TEMP_DIR, otf_filename)
    assert os.path.isfile(actual_path)
    actual_ttx = generate_ttx_dump(actual_path)
    expected_ttx = get_expected_path(otf_filename.replace('.otf', '.ttx'))
    assert differ([expected_ttx, actual_ttx, '-s', '<ttFont sfntVersion'])
