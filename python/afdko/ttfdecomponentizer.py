# Copyright 2020 Adobe. All rights reserved.

"""
Takes in a TrueType font and de-componentizes (decomposes) any composite
glyphs into simple glyphs.

** NOTE: this process leaves the resulting decomposed glyphs with no
instructions ("hints"). **
"""

import argparse
import os
import sys

from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen

from afdko.fdkutils import get_font_format

__version__ = "0.1.0"


def _validate_dir_path(path):
    return os.path.isdir(path)


def _validate_font_path(path_str):
    vpath = os.path.abspath(os.path.realpath(path_str))
    return os.path.isfile(vpath) and (get_font_format(vpath) == 'TTF')


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter, description=__doc__
    )
    parser.add_argument("--version", action="version", version=__version__)
    parser.add_argument(
        "-o",
        metavar="OUTPUT_PATH",
        dest="output_path",
        help="path to output TTF file OR directory if -d specified.",
    )
    parser.add_argument(
        "-d",
        "--directory",
        action="store_true",
        help="Indicates that the input is a directory, rather than a file. "
             "The program will process any valid TrueType files in the "
             "specified directory."
    )
    parser.add_argument(
        "input_path",
        metavar="INPUT",
        help="path to input TTF font file OR directory if -d specified.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print the name of each composite glyph processed.",
    )
    options = parser.parse_args(args)

    # validate the provided options
    if options.directory:
        if not _validate_dir_path(options.input_path):
            parser.error(f'{options.input_path} is not a directory.')
        if options.output_path:
            if not _validate_dir_path(options.output_path):
                parser.error(f'{options.output_path} is not a directory')
    else:
        if not _validate_font_path(options.input_path):
            parser.error(f'{options.input_path} is not a TTF font file.')

    return options


def process_font(input_path, output_path=None, verbose=False):
    """
    De-componentize a single font at input_path, saving to output_path (or
    input_path if None)
    """
    font = TTFont(input_path)
    output_path = output_path or input_path
    gs = font.getGlyphSet()
    drpen = DecomposingRecordingPen(gs)
    ttgpen = TTGlyphPen(gs)

    count = 0
    for glyphname in font.glyphOrder:
        glyph = font["glyf"][glyphname]
        if not glyph.isComposite():
            continue

        if verbose:
            print(f"    decomposing '{glyphname}'")

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

    return count


def main(args=None):
    opts = get_options(args)
    font_count = 0

    if opts.directory:
        input_files = []
        output_files = []
        for f in os.listdir(opts.input_path):
            if f.startswith("."):
                continue
            fp = os.path.join(opts.input_path, f)
            if _validate_font_path(fp):
                input_files.append(fp)
                output_files.append(os.path.join(opts.output_path, f))

    else:
        input_files = [opts.input_path]
        output_files = [opts.output_path]

    for idx, input_file in enumerate(input_files):
        output_file = output_files[idx]
        end = None if opts.verbose else ''
        print(f"Processing {input_file}...", end=end, flush=True)
        done = f'{"" if opts.directory else "Done! "}'
        count = process_font(input_file, output_file, opts.verbose)
        plural = f"{' was' if count == 1 else 's were'}"
        print(f"{done}{count} glyph{plural} decomposed.")
        font_count += 1

    if opts.directory:
        print(f"Done! {font_count} fonts in {opts.input_path} were processed.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
