# Copyright 2014 Adobe. All rights reserved.

"""
Extract all OpenType fonts from the parent OpenType Collection font.
"""

import argparse
import logging
import os
import sys

from fontTools.ttLib import sfnt, TTFont

from afdko.fdkutils import (
    get_font_format,
    validate_path,
)

__version__ = '2.0.0'

logger = logging.getLogger(__name__)


def get_psname(font):
    """
    Returns the font's nameID 6 (a.k.a. PostScript name).
    Returns None if the font has no nameID 6, if nameID 6 is an empty
    string, or if the font has no 'name' table.
    """
    if 'name' in font:
        psname = font['name'].getDebugName(6)
        if psname:
            return psname


def run(options):
    ttc_path = options.ttc_path

    with open(ttc_path, 'rb') as fp:
        num_fonts = sfnt.SFNTReader(fp, fontNumber=0).numFonts

    if options.report:
        unique_tables = set()
        print(f'Input font: {os.path.basename(ttc_path)}\n')

    for ft_idx in range(num_fonts):
        font = TTFont(ttc_path, fontNumber=ft_idx, lazy=True)

        psname = get_psname(font)
        if psname is None:
            ttc_name = os.path.splitext(os.path.basename(ttc_path))[0]
            psname = f'{ttc_name}-font{ft_idx}'

        ext = '.otf' if font.sfntVersion == 'OTTO' else '.ttf'
        font_filename = f'{psname}{ext}'

        if options.report:
            # this code is based on fontTools.ttx.ttList
            reader = font.reader
            tags = sorted(reader.keys())
            print(f'Font {ft_idx}: {font_filename}')
            print('    tag ', '   checksum', '   length', '   offset')
            print('    ----', ' ----------', ' --------', ' --------')
            for tag in tags:
                entry = reader.tables[tag]
                checksum = int(entry.checkSum)
                if checksum < 0:
                    checksum += 0x100000000
                checksum = f'0x{checksum:08X}'
                tb_info = (tag, checksum, entry.length, entry.offset)
                if tb_info not in unique_tables:
                    print('    {:4s}  {:10s}  {:8d}  {:8d}'.format(*tb_info))
                    unique_tables.add(tb_info)
                else:
                    print(f'    {tag:4s}  - shared -')
            print()
        else:
            save_path = os.path.join(os.path.dirname(ttc_path), font_filename)
            font.save(save_path)
            logger.info(f'Saved {save_path}')

        font.close()


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description=__doc__
    )
    parser.add_argument(
        '--version',
        action='version',
        version=__version__
    )
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=0,
        help='verbose mode\n'
             'Use -vv for debug mode'
    )
    parser.add_argument(
        '-r',
        '--report',
        action='store_true',
        help="report the TTC's fonts and tables (no files are written)",
    )
    parser.add_argument(
        'ttc_path',
        metavar='TTC FONT',
        type=validate_path,
        help='path to TTC font',
    )
    options = parser.parse_args(args)

    if not options.verbose:
        level = "WARNING"
    elif options.verbose == 1:
        level = "INFO"
    else:
        level = "DEBUG"
    logging.basicConfig(level=level)

    if get_font_format(options.ttc_path) != 'TTC':
        parser.error('The input file is not an OpenType Collection font.')

    return options


def main(args=None):
    opts = get_options(args)

    try:
        run(opts)
    except Exception as exc:
        logger.error(exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())
