from __future__ import print_function, division, absolute_import

import os
import tempfile

from runner import main as runner
from differ import main as differ

TOOL = 'digiplot'
CMD = ['-t', TOOL]

CID_FONT = 'cidfont.otf'


def _get_expected_path(file_name):
    return os.path.join(os.path.split(__file__)[0], TOOL + '_data',
                        'expected_output', file_name)


# -----
# Tests
# -----

def test_cid_font_glyphs_2_3():
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        runner(CMD + ['-o', 'o', '_{}'.format(tmp.name), 'g', '_2,3',
                      '=pageIncludeTitle', '_0', '-f', CID_FONT, '-n'])
        expected_path = _get_expected_path('cid_glyphs_2_3.pdf')
        assert differ([expected_path, tmp.name,
                       '-s', '/CreationDate', '-e', 'macroman'])
