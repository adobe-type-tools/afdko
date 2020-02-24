# Copyright 2020 Adobe. All rights reserved.

"""
Takes in a TrueType font and de-componentizes (decomposes) any composite
glyphs into simple glyphs.

** NOTE: this process leaves the resulting decomposed glyphs with no
instructions ("hints"). **
"""

import argparse
import sys

from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen

from afdko.ttfcomponentizer import _validate_font_path

__version__ = "0.1.0"


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter, description=__doc__
    )
    parser.add_argument("--version", action="version", version=__version__)
    parser.add_argument(
        "-o",
        metavar="OUTPUT_PATH",
        dest="output_path",
        help="path to output the decomponentized TTF to.",
    )
    parser.add_argument(
        "input_path",
        metavar="FONT",
        type=_validate_font_path,
        help="TTF font file.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print the name of each composite glyph processed.",
    )
    options = parser.parse_args(args)
    return options


def main(args=None):
    opts = get_options(args)
    output_path = opts.output_path or opts.input_path
    count = 0

    font = TTFont(opts.input_path)
    if "glyf" not in font:
        return 1

    gs = font.getGlyphSet()
    drpen = DecomposingRecordingPen(gs)
    ttgpen = TTGlyphPen(gs)

    for glyphname in font.glyphOrder:
        glyph = font["glyf"][glyphname]
        if not glyph.isComposite():
            continue

        if opts.verbose:
            print(f"decomposing '{glyphname}'")

        # reset the pens
        drpen.value = []
        ttgpen.init()

        # draw the composite glyph into the decomposing pen
        glyph.draw(drpen, font["glyf"])
        # replay the recorded decomposed glyph into the TTGlyphPen
        drpen.replay(ttgpen)
        # store the decomposed glyph in the 'glyf' table
        font["glyf"][glyphname] = ttgpen.glyph()

        count += 1

    font.save(output_path)
    plural = f"{' was' if count == 1 else 's were'}"
    print(f"Done! {count} glyph{plural} decomposed.")


if __name__ == "__main__":
    sys.exit(main())
