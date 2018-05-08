from __future__ import print_function, division, absolute_import

import os

from runner import main as runner
from differ import main as differ

TOOL = 'spot'
CMD = ['-t', TOOL]

BLACK = 'black.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_list_tables():
    actual_path = runner(CMD + ['-r', '-o', 'T', '-f', BLACK])
    expected_path = _get_expected_path('list_tables.txt')
    assert differ([expected_path, actual_path]) is True


def test_list_features():
    actual_path = runner(CMD + ['-r', '-o', 'F', '-f', BLACK])
    expected_path = _get_expected_path('list_features.txt')
    assert differ([expected_path, actual_path]) is True


def test_glyph_proof():
    actual_path = runner(CMD + ['-r', '-o', 'G', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof.ps')
    assert differ([expected_path, actual_path,
                   '-s', '0 743.4 moveto (CFF:black.otf']) is True


def test_glyph_proof_no_header():
    actual_path = runner(CMD + ['-r', '-o', 'G', 'd', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof_no_header.ps')
    assert differ([expected_path, actual_path]) is True


def test_glyph_proof_no_header_multipage():
    actual_path = runner(CMD + ['-r', '-o', 'G', 'd', 'br', '-f', BLACK])
    expected_path = _get_expected_path('glyph_proof_no_header_multipage.ps')
    assert differ([expected_path, actual_path]) is True


def test_proof_all_features():
    actual_path = runner(CMD + ['-r', '-o', 'Proof', '-f', BLACK])
    expected_path = _get_expected_path('proof_all_features.ps')
    assert differ([expected_path, actual_path]) is True
