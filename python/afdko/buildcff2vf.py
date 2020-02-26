# Copyright 2017 Adobe. All rights reserved.

"""
Builds a CFF2 variable font from a designspace file and its UFO masters.
"""

import argparse
from ast import literal_eval
from copy import deepcopy
import logging
import os
import re
import sys

from fontTools import varLib
from fontTools.cffLib.specializer import commandsToProgram
from fontTools.designspaceLib import DesignSpaceDocument
from fontTools.misc.fixedTools import otRound
from fontTools.misc.psCharStrings import T2OutlineExtractor, T2CharString
from fontTools.ttLib import TTFont
from fontTools.varLib.cff import (CFF2CharStringMergePen,
                                  VarLibCFFPointTypeMergeError)

from afdko.fdkutils import validate_path

__version__ = '2.0.2'

STAT_FILENAME = 'override.STAT.ttx'


# set up for printing progress notes
def progress(self, message, *args, **kws):
    # Note: message must contain the format specifiers for any strings in args.
    level = self.getEffectiveLevel()
    self._log(level, message, args, **kws)


PROGRESS_LEVEL = logging.INFO + 5
PROGESS_NAME = "progress"
logging.addLevelName(PROGRESS_LEVEL, PROGESS_NAME)
logger = logging.getLogger(__name__)
logging.Logger.progress = progress


def getSubset(subset_Path):
    with open(subset_Path, "rt") as fp:
        text_lines = fp.readlines()
    locationDict = {}
    cur_key_list = None
    for li, line in enumerate(text_lines):
        idx = line.find('#')
        if idx >= 0:
            line = line[:idx]
        line = line.strip()
        if not line:
            continue
        if line[0] == "(":
            cur_key_list = []
            location_list = literal_eval(line)
            for location_entry in location_list:
                cur_key_list.append(location_entry)
                if location_entry not in locationDict:
                    locationDict[location_entry] = []
        else:
            m = re.match(r"(\S+)", line)
            if m:
                if cur_key_list is None:
                    logger.error(
                        "Error parsing subset file. "
                        "Seeing a glyph name record before "
                        "seeing a location record.")
                    logger.error(f'Line number: {li}.')
                    logger.error(f'Line text: {line}.')
                for key in cur_key_list:
                    locationDict[key].append(m.group(1))
    return locationDict


def subset_masters(designspace, subsetDict):
    from fontTools import subset
    subset_options = subset.Options(notdef_outline=True, layout_features='*')
    for ds_source in designspace.sources:
        key = tuple(ds_source.location.items())
        included = set(subsetDict[key])
        ttf_font = ds_source.font
        subsetter = subset.Subsetter(options=subset_options)
        subsetter.populate(glyphs=included)
        subsetter.subset(ttf_font)
        subset_path = f'{os.path.splitext(ds_source.path)[0]}.subset.otf'
        logger.progress(f'Saving subset font {subset_path}')
        ttf_font.save(subset_path)
        ds_source.font = TTFont(subset_path)


class CompatibilityPen(CFF2CharStringMergePen):
    def __init__(self, default_commands,
                 glyphName, num_masters, master_idx, roundTolerance=0.5):
        super(CompatibilityPen, self).__init__(
            default_commands, glyphName, num_masters, master_idx,
            roundTolerance=0.5)
        self.fixed = False

    def add_point(self, point_type, pt_coords):
        if self.m_index == 0:
            self._commands.append([point_type, [pt_coords]])
        else:
            cmd = self._commands[self.pt_index]
            if cmd[0] != point_type:
                # Fix some issues that show up in some
                # CFF workflows, even when fonts are
                # topologically merge compatible.
                success, new_pt_coords = self.check_and_fix_flat_curve(
                    cmd, point_type, pt_coords)
                if success:
                    logger.progress("Converted between line and curve in "
                                    "source font index '%s' glyph '%s', "
                                    "point index '%s'at '%s'. "
                                    "Please check correction." % (
                                        self.m_index, self.glyphName,
                                        self.pt_index, pt_coords))
                    pt_coords = new_pt_coords
                else:
                    success = self.check_and_fix_closepath(
                        cmd, point_type, pt_coords)
                    if success:
                        # We may have incremented self.pt_index
                        cmd = self._commands[self.pt_index]
                        if cmd[0] != point_type:
                            success = False
                if not success:
                    raise VarLibCFFPointTypeMergeError(
                        point_type, self.pt_index, self.m_index, cmd[0],
                        self.glyphName)
                self.fixed = True
            cmd[1].append(pt_coords)
        self.pt_index += 1

    def make_flat_curve(self, cur_coords):
        # Convert line coords to curve coords.
        dx = self.roundNumber(cur_coords[0] / 3.0)
        dy = self.roundNumber(cur_coords[1] / 3.0)
        new_coords = [dx, dy, dx, dy,
                      cur_coords[0] - 2 * dx,
                      cur_coords[1] - 2 * dy]
        return new_coords

    def make_curve_coords(self, coords, is_default):
        # Convert line coords to curve coords.
        if is_default:
            new_coords = []
            for cur_coords in coords:
                master_coords = self.make_flat_curve(cur_coords)
                new_coords.append(master_coords)
        else:
            cur_coords = coords
            new_coords = self.make_flat_curve(cur_coords)
        return new_coords

    def check_and_fix_flat_curve(self, cmd, point_type, pt_coords):
        success = False
        if (point_type == 'rlineto') and (cmd[0] == 'rrcurveto'):
            is_default = False  # the line is in the master font we are adding
            pt_coords = self.make_curve_coords(pt_coords, is_default)
            success = True
        elif (point_type == 'rrcurveto') and (cmd[0] == 'rlineto'):
            is_default = True  # the line is in the default font commands
            expanded_coords = self.make_curve_coords(cmd[1], is_default)
            cmd[1] = expanded_coords
            cmd[0] = point_type
            success = True
        return success, pt_coords

    def check_and_fix_closepath(self, cmd, point_type, pt_coords):
        """ Some workflows drop a lineto which closes a path.
        Also, if the last segment is a curve in one master,
        and a flat curve in another, the flat curve can get
        converted to a closing lineto, and then dropped.
        Test if:
        1) one master op is a moveto,
        2) the previous op for this master does not close the path
        3) in the other master the current op is not a moveto
        4) the current op in the otehr master closes the current path

        If the default font is missing the closing lineto, insert it,
        then proceed with merging the current op and pt_coords.

        If the current region is missing the closing lineto
        and therefore the current op is a moveto,
        then add closing coordinates to self._commands,
        and increment self.pt_index.

        Note that if this may insert a point in the default font list,
        so after using it, 'cmd' needs to be reset.

        return True if we can fix this issue.
        """
        if point_type == 'rmoveto':
            # If this is the case, we know that cmd[0] != 'rmoveto'

            # The previous op must not close the path for this region font.
            prev_moveto_coords = self._commands[self.prev_move_idx][1][-1]
            prv_coords = self._commands[self.pt_index - 1][1][-1]
            if prev_moveto_coords == prv_coords[-2:]:
                return False

            # The current op must close the path for the default font.
            prev_moveto_coords2 = self._commands[self.prev_move_idx][1][0]
            prv_coords = self._commands[self.pt_index][1][0]
            if prev_moveto_coords2 != prv_coords[-2:]:
                return False

            # Add the closing line coords for this region
            # so self._commands, then increment self.pt_index
            # so that the current region op will get merged
            # with the next default font moveto.
            if cmd[0] == 'rrcurveto':
                new_coords = self.make_curve_coords(prev_moveto_coords, False)
                cmd[1].append(new_coords)
            self.pt_index += 1
            return True

        if cmd[0] == 'rmoveto':
            # The previous op must not close the path for the default font.
            prev_moveto_coords = self._commands[self.prev_move_idx][1][0]
            prv_coords = self._commands[self.pt_index - 1][1][0]
            if prev_moveto_coords == prv_coords[-2:]:
                return False

            # The current op must close the path for this region font.
            prev_moveto_coords2 = self._commands[self.prev_move_idx][1][-1]
            if prev_moveto_coords2 != pt_coords[-2:]:
                return False

            # Insert the close path segment in the default font.
            # We omit the last coords from the previous moveto
            # is it will be supplied by the current region point.
            # after this function returns.
            new_cmd = [point_type, None]
            prev_move_coords = self._commands[self.prev_move_idx][1][:-1]
            # Note that we omit the last region's coord from prev_move_coords,
            # as that is from the current region, and we will add the
            # current pts' coords from the current region in its place.
            if point_type == 'rlineto':
                new_cmd[1] = prev_move_coords
            else:
                # We omit the last set of coords from the
                # previous moveto, as it will be supplied by the coords
                # for the current region pt.
                new_cmd[1] = self.make_curve_coords(prev_move_coords, True)
            self._commands.insert(self.pt_index, new_cmd)
            return True
        return False

    def getCharStrings(self, num_masters, private=None, globalSubrs=None,
                       default_idx=0):
        """ A command looks like:
        [op_name, [
            [source 0 arglist for op],
            [source 1 arglist for op],
            ...
            [source n arglist for op],
        I am not optimizing this there, as that will be done when
        the CFF2 Charstring is created in fontTools.varLib.build().

        If I did, I would have to rearrange the arguments to:
        [
        [arg 0 for source 0 ... arg 0 for source n]
        [arg 1 for source 0 ... arg 1 for source n]
        ...
        [arg M for source 0 ... arg M for source n]
        ]
        before calling specialize.
        """
        t2List = []
        merged_commands = self._commands
        for i in range(num_masters):
            commands = []
            for op in merged_commands:
                source_op = [op[0], op[1][i]]
                commands.append(source_op)
            program = commandsToProgram(commands)
            if self._width is not None:
                assert not self._CFF2, (
                    "CFF2 does not allow encoding glyph width in CharString.")
                program.insert(0, otRound(self._width))
            if not self._CFF2:
                program.append('endchar')
            charString = T2CharString(
                program=program, private=private, globalSubrs=globalSubrs)
            t2List.append(charString)
        # if default_idx is not 0, we need to move it to the right index.
        if default_idx:
            default_font_cs = t2List.pop(0)
            t2List.insert(default_idx, default_font_cs)
        return t2List


def _get_cs(charstrings, glyphName):
    if glyphName not in charstrings:
        return None
    return charstrings[glyphName]


def do_compatibility(vf, master_fonts, default_idx):
    default_font = vf
    default_charStrings = default_font['CFF '].cff.topDictIndex[0].CharStrings
    glyphOrder = default_font.getGlyphOrder()
    charStrings = [
        font['CFF '].cff.topDictIndex[0].CharStrings for font in master_fonts]

    for gname in glyphOrder:
        all_cs = [_get_cs(cs, gname) for cs in charStrings]
        if len([gs for gs in all_cs if gs is not None]) < 2:
            continue
        # remove the None's from the list.
        cs_list = [cs for cs in all_cs if cs]
        num_masters = len(cs_list)
        default_charstring = default_charStrings[gname]
        compat_pen = CompatibilityPen([], gname, num_masters, 0)
        default_charstring.outlineExtractor = T2OutlineExtractor
        default_charstring.draw(compat_pen)

        # Add the coordinates from all the other regions to the
        # blend lists in the CFF2 charstring.
        region_cs = cs_list[:]
        del region_cs[default_idx]
        for region_idx, region_charstring in enumerate(region_cs, start=1):
            compat_pen.restart(region_idx)
            region_charstring.draw(compat_pen)
        if compat_pen.fixed:
            fixed_cs_list = compat_pen.getCharStrings(
                num_masters, private=default_charstring.private,
                globalSubrs=default_charstring.globalSubrs,
                default_idx=default_idx)
            cs_list = list(cs_list)
            for i, cs in enumerate(cs_list):
                mi = all_cs.index(cs)
                charStrings[mi][gname] = fixed_cs_list[i]


def otfFinder(s):
    return s.replace('.ufo', '.otf')


def suppress_glyph_names(tt_font):
    postTable = tt_font['post']
    postTable.formatType = 3.0
    postTable.compile(tt_font)


def remove_mac_names(tt_font):
    name_tb = tt_font['name']
    name_tb.names = [nr for nr in name_tb.names if nr.platformID != 1]


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
        '-d',
        '--designspace',
        metavar='PATH',
        dest='design_space_path',
        type=validate_path,
        help='path to design space file',
        required=True
    )
    parser.add_argument(
        '-o',
        '--output',
        dest='var_font_path',
        metavar='PATH',
        help='path to output variable font file. Default is base name\n'
        'of the design space file.',
    )
    parser.add_argument(
        '-k',
        '--keep-glyph-names',
        action='store_true',
        help='Preserve glyph names in output variable font\n'
        "(using 'post' table format 2).",
    )
    parser.add_argument(
        '--omit-mac-names',
        action='store_true',
        help="Omit Macintosh strings from 'name' table.",
    )
    parser.add_argument(
        '-c',
        '--check-compat',
        dest='check_compatibility',
        action='store_true',
        help='Check outline compatibility in source fonts, and fix flat\n'
        'curves.',
    )
    parser.add_argument(
        '-i',
        '--include-glyphs',
        dest='include_glyphs_path',
        metavar='PATH',
        type=validate_path,
        help='Path to file containing a python dict specifying which\n'
        'glyph names should be included from which source fonts.'
    )
    options = parser.parse_args(args)

    if not options.var_font_path:
        var_font_path = f'{os.path.splitext(options.design_space_path)[0]}.otf'
        options.var_font_path = var_font_path

    if not options.verbose:
        level = PROGRESS_LEVEL
        logging.basicConfig(level=level, format="%(message)s")
    else:
        level = logging.INFO
        logging.basicConfig(level=level)
    logger.setLevel(level)

    return options


def main(args=None):
    options = get_options(args)

    if os.path.exists(options.var_font_path):
        os.remove(options.var_font_path)

    designspace = DesignSpaceDocument.fromfile(options.design_space_path)
    ds_data = varLib.load_designspace(designspace)
    master_fonts = varLib.load_masters(designspace, otfFinder)
    logger.progress("Reading source fonts...")
    for i, master_font in enumerate(master_fonts):
        designspace.sources[i].font = master_font

    # Subset source fonts
    if options.include_glyphs_path:
        logger.progress("Subsetting source fonts...")
        subsetDict = getSubset(options.include_glyphs_path)
        subset_masters(designspace, subsetDict)

    if options.check_compatibility:
        logger.progress("Checking outline compatibility in source fonts...")
        font_list = [src.font for src in designspace.sources]
        default_font = designspace.sources[ds_data.base_idx].font
        vf = deepcopy(default_font)
        # We copy vf from default_font, because we use VF to hold
        # merged arguments from each source font charstring - this alters
        # the font, which we don't want to do to the default font.
        do_compatibility(vf, font_list, ds_data.base_idx)

    logger.progress("Building VF font...")
    # Note that we now pass in the design space object, rather than a path to
    # the design space file, in order to pass in the modified source fonts
    # fonts without having to recompile and save them.
    try:
        varFont, _, _ = varLib.build(designspace, otfFinder)
    except VarLibCFFPointTypeMergeError:
        logger.error("The input set requires compatibilization. Please try "
                     "again with the -c (--check-compat) option.")
        return 0

    if not options.keep_glyph_names:
        suppress_glyph_names(varFont)

    if options.omit_mac_names:
        remove_mac_names(varFont)

    stat_file_path = os.path.join(
        os.path.dirname(options.var_font_path), STAT_FILENAME)
    if os.path.exists(stat_file_path):
        logger.progress("Importing STAT table override...")
        varFont.importXML(stat_file_path)

    varFont.save(options.var_font_path)
    logger.progress("Built variable font '%s'" % (options.var_font_path))


if __name__ == '__main__':
    sys.exit(main())
