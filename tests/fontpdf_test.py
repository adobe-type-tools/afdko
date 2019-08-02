from fontTools.ttLib import TTFont

from afdko.pdflib.fontpdf import (doTitle, FontPDFParams)
from afdko.pdflib.otfpdf import txPDFFont
from afdko.pdflib.pdfgen import Canvas

from test_utils import get_input_path

OTF_FONT = 'OTF.otf'


# -----
# Tests
# -----

def test_doTitle_pageIncludeTitle_1():
    with TTFont(get_input_path(OTF_FONT)) as otfont:
        params = FontPDFParams()
        assert params.pageIncludeTitle == 1
        pdfFont = txPDFFont(otfont, params)
        rt_canvas = Canvas("pdf_file_path")
        assert rt_canvas._code == []
        doTitle(rt_canvas, pdfFont, params, 1)
        assert len(rt_canvas._code)
        assert 'SourceSansPro-Black' in rt_canvas._code[1]


def test_doTitle_pageIncludeTitle_0():
    with TTFont(get_input_path(OTF_FONT)) as otfont:
        params = FontPDFParams()
        params.pageIncludeTitle = 0
        pdfFont = txPDFFont(otfont, params)
        rt_canvas = Canvas("pdf_file_path")
        assert rt_canvas._code == []
        doTitle(rt_canvas, pdfFont, params, 1)
        assert rt_canvas._code == []
