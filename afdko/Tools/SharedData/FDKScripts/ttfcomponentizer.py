#!/usr/bin/env python

# Copyright 2018 Adobe. All rights reserved.

"""
Takes in a TrueType font and looks for a UFO font stored in the same folder.
Uses the UFO's components data to componentize matching TrueType glyphs.
The script only supports components that are not scaled, rotated or flipped.
"""

from __future__ import print_function

import argparse
import os
import sys

from fontTools.ttLib import TTFont, getTableModule
from fontTools.misc.py23 import open
from defcon import Font
from ufoLib import UFOLibError


__version__ = '0.1.0'


PUBLIC_PSNAMES = "public.postscriptNames"
GOADB_FILENAME = "GlyphOrderAndAliasDB"


def get_font_format(font_file_path):
    with open(font_file_path, "rb") as f:
        head = f.read(4).decode()
        if head in ("\0\1\0\0", "true"):
            return "TTF"
        return None


def validate_font_path(path):
    path = os.path.realpath(path)
    if not (os.path.isfile(path) and get_font_format(path) == 'TTF'):
        print("ERROR: {} is not a valid TrueType font file path.".format(path),
              file=sys.stderr)
        return None
    return path


def get_ufo_path(ttf_path):
    """
    Find a UFO font in the same folder as the TT font.
    Returns the UFO's path or None.
    """
    folder_path = os.path.dirname(ttf_path)
    for file_name in sorted(os.listdir(folder_path)):
        if file_name.lower().endswith('.ufo'):
            return os.path.join(folder_path, file_name)
    return None


def get_goadb_path(font_path):
    """
    Find a file named GlyphOrderAndAliasDB. This file can be in the same
    folder as the font, or up-to 3 folders above.
    Returns the GOADB's path or None.
    """
    level = 0
    folder = os.path.dirname(font_path)

    while level <= 3:
        path = os.path.join(folder, GOADB_FILENAME)
        if os.path.exists(path):
            return path
        folder = os.path.dirname(folder)
        level += 1

    return None


def read_txt_file_lines(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        return f.read().splitlines()


def process_goadb(goadb_path):
    """
    Read a GOAD file and return a dictionary mapping glyph design names to
    glyph production names. The returned mapping may be empty as it will only
    contain entries for glyph names that change from design to production.
    The sctructure of each line of a GOADB file is:
    production_name<tab>design_name<tab>unicode_overrides
    Blank lines and comment lines are alowed in a GOADB file.
    """
    gnames_mapping = {}
    dsgn_names_seen = set()
    prod_names_seen = set()

    for i, line in enumerate(read_txt_file_lines(goadb_path)):
        line = line.strip()
        # skip blank lines
        if not line:
            continue
        # skip comments
        if line.startswith('#'):
            continue

        line_cols = line.split()

        if len(line_cols) < 2:
            print("WARNING: Skipped invalid line #{}: {}".format(i + 1, line),
                  file=sys.stderr)
            continue

        prod_name, dsgn_name = line_cols[:2]

        if dsgn_name in dsgn_names_seen:
            print("WARNING: Skipped duplicate design glyph name '{}' at "
                  "line #{}".format(dsgn_name, i + 1), file=sys.stderr)
            continue

        if prod_name in prod_names_seen:
            print("WARNING: Skipped duplicate production glyph name '{}' at "
                  "line #{}".format(prod_name, i + 1), file=sys.stderr)
            continue

        dsgn_names_seen.add(dsgn_name)
        prod_names_seen.add(prod_name)

        # The mapping only needs to contain names that change
        if dsgn_name != prod_name:
            gnames_mapping[dsgn_name] = prod_name

    return gnames_mapping


def get_goadb_names_mapping(ufo_path):
    """
    Assemble a glyph names' mapping dictionary from a GOADB file.
    Returns a dictionary, which can be empty.
    """
    goadb_path = get_goadb_path(ufo_path)

    if not goadb_path:
        return {}

    return process_goadb(goadb_path)


def get_glyph_names_mapping(ufo_path):
    """
    Return a dictionary mapping glyphs' design names to production names.
    This mapping is necessary because the glyph names of the input TTF will
    be production (a.k.a. final) glyph names. In many cases the glyph's design
    name and the production name will be the same, but they may also differ.

    The method will first try to obtain the mapping from the UFO's lib key
    named 'public.postscriptNames'. If this key is not present, the method
    will try to find (and later process) a GlyphOrderAndAliasDB file in the
    folder tree.

    The returned dictionary may be empty, in which case it's assumed that
    there's no difference between the UFO's and the TTF's glyphs' names.

    The UFO object is also returned.
    """
    try:
        ufo = Font(ufo_path)
    except UFOLibError:
        print("ERROR: Not a valid UFO font at {}".format(ufo_path),
              file=sys.stderr)
        return None, None

    if PUBLIC_PSNAMES in ufo.lib:
        return ufo, ufo.lib[PUBLIC_PSNAMES]

    return ufo, get_goadb_names_mapping(ufo_path)


class ComponentsData(object):
    def __init__(self):
        self.names = ()
        self.positions = ()

    def add_component(self, name, pos):
        self.names += (name,)
        self.positions += (pos,)


def get_composites_data(ufo, ps_names):
    """
    Iterate thru each glyph of a UFO and collect the names and positions of
    all components that make up a composite glyph. The process will only
    collect data for composites that are strictly made with components (i.e.
    no mixed contours-components composites), and whose components only have
    x and y transformations (thus composites with scaled, rotated, or flipped
    componentes will NOT be considered).

    Returns a dictionary whose keys are composite names and whose values are
    ComponentsData objects.

    NOTE: All glyph names in the returned dictionary are production names.
    """
    composites_data = {}
    for glyph in ufo:
        if glyph.components and not len(glyph):
            components = ComponentsData()
            all_comps_are_basic = True
            for comp in glyph.components:
                if comp.transformation[:4] != (1, 0, 0, 1):
                    all_comps_are_basic = False
                    break
                comp_name = ps_names.get(comp.baseGlyph, comp.baseGlyph)
                components.add_component(comp_name, comp.transformation[4:])
            if all_comps_are_basic:
                glyf_name = ps_names.get(glyph.name, glyph.name)
                composites_data[glyf_name] = components
    return composites_data


def assemble_components(comps_data):
    """
    Assemble and return a list of GlyphComponent objects.
    """
    components = []
    for i, cname in enumerate(comps_data.names):
        component = getTableModule('glyf').GlyphComponent()
        component.glyphName = cname
        component.x, component.y = comps_data.positions[i]
        component.flags = 0x204 if i == 0 else 0x4
        components.append(component)
    return components


def componentize_ttf(font_path, composites_data, output_path):
    """
    Load a TrueType font and iterate thru a dictionary of composites data.
    Remakes some glyphs in the glyf table from being made of countours to
    being made of components.

    Saves the modified font in a new location if an output path was provided,
    otherwise overwrites the original one.

    Returns a count of the glyphs that got componentized.
    """
    font = TTFont(font_path)
    glyf_table = font['glyf']
    comp_count = 0

    for gname in composites_data:
        if gname not in glyf_table:
            continue
        if not all(
           [cname in glyf_table for cname in composites_data[gname].names]):
            continue

        components = assemble_components(composites_data[gname])

        glyph = glyf_table[gname]
        glyph.__dict__.clear()
        setattr(glyph, "components", components)
        glyph.numberOfContours = -1
        comp_count += 1

    if output_path:
        font.save(os.path.realpath(output_path))
    else:
        font.save(font_path)

    return comp_count


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
        '-o',
        metavar='OUTPUT_PATH',
        dest='output_path',
        help='path to output the componentized TTF to.'
    )
    parser.add_argument(
        'input_path',
        metavar='FONT',
        help='TTF font file.',
    )
    options = parser.parse_args(args)
    options.font_path = validate_font_path(options.input_path)
    return options


def main(args=None):
    opts = get_options(args)

    if not opts.font_path:
        return 1

    # Find UFO file in the same directory
    ufo_path = get_ufo_path(opts.font_path)
    if not ufo_path:
        print("ERROR: No UFO font was found for {}".format(opts.input_path),
              file=sys.stderr)
        return 1

    # Get the design->production glyph names mapping, and the UFO
    ufo, ps_names = get_glyph_names_mapping(ufo_path)
    if not ufo:
        return 1

    # Get the composites' info from processing the UFO
    composites_data = get_composites_data(ufo, ps_names)

    comp_count = componentize_ttf(opts.font_path, composites_data,
                                  opts.output_path)

    plural = "s were" if comp_count != 1 else " was"
    print("Done! {} glyph{} componentized.".format(comp_count, plural),
          file=sys.stdout)


if __name__ == "__main__":
    sys.exit(main())
