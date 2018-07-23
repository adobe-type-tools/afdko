from __future__ import print_function, division, absolute_import

import os

from fontTools.ttLib import TTFont

from afdko.Tools.SharedData.FDKScripts.fontPDF import (doTitle, FontPDFParams)
from afdko.Tools.SharedData.FDKScripts.otfPDF import txPDFFont
from afdko.Tools.SharedData.FDKScripts.pdfgen import Canvas


TOOL = 'fontpdf'
OTF_FONT = 'OTF.otf'

data_dir_path = os.path.join(os.path.split(__file__)[0], TOOL + '_data')


def _get_input_path(file_name):
    return os.path.join(data_dir_path, 'input', file_name)


# -----
# Tests
# -----

def test_doTitle_pageIncludeTitle_1():
    with TTFont(_get_input_path(OTF_FONT)) as otfont:
        params = FontPDFParams()
        assert params.pageIncludeTitle == 1
        pdfFont = txPDFFont(otfont, params)
        rt_canvas = Canvas("pdf_file_path")
        assert rt_canvas._code == []
        doTitle(rt_canvas, pdfFont, params, 1)
        assert len(rt_canvas._code)
        assert 'SourceSansPro-Black' in rt_canvas._code[1]


def test_doTitle_pageIncludeTitle_0():
    with TTFont(_get_input_path(OTF_FONT)) as otfont:
        params = FontPDFParams()
        params.pageIncludeTitle = 0
        pdfFont = txPDFFont(otfont, params)
        rt_canvas = Canvas("pdf_file_path")
        assert rt_canvas._code == []
        doTitle(rt_canvas, pdfFont, params, 1)
        assert rt_canvas._code == []
