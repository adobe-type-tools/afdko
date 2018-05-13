from __future__ import print_function, division, absolute_import

import os
import tempfile

from runner import main as runner
from differ import main as differ

TOOL = 'ttxn'
CMD = ['-t', TOOL]

OTF_FONT = 'OTF.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_dump_otf():
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        runner(CMD + ['-o', 'o{}'.format(tmp.name),
                      '-f', OTF_FONT, '-n'])
        expected_path = _get_expected_path('OTF.ttx')
        assert differ([expected_path, tmp.name,
                       '-s', '<ttFont sfntVersion']) is True


def test_dump_otf_no_hints():
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        runner(CMD + ['-o', 'nh', 'o{}'.format(tmp.name),
                      '-f', OTF_FONT, '-n'])
        expected_path = _get_expected_path('OTF_no_hints.ttx')
        assert differ([expected_path, tmp.name,
                       '-s', '<ttFont sfntVersion']) is True


def test_dump_otf_gpos_only():
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        runner(CMD + ['-o', 'tGPOS', 'o{}'.format(tmp.name),
                      '-f', OTF_FONT, '-n'])
        expected_path = _get_expected_path('OTF_GPOS_only.ttx')
        assert differ([expected_path, tmp.name,
                       '-s', '<ttFont sfntVersion']) is True


def test_dump_otf_gsub_only():
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        runner(CMD + ['-o', 'tGSUB', 'o{}'.format(tmp.name),
                      '-f', OTF_FONT, '-n'])
        expected_path = _get_expected_path('OTF_GSUB_only.ttx')
        assert differ([expected_path, tmp.name,
                       '-s', '<ttFont sfntVersion']) is True
