# Copyright 2015 Adobe. All rights reserved.

"""
Tool that performs outline quality checks and can remove path overlaps.
"""

__version__ = '2.5.0'

import argparse
from functools import cmp_to_key
import re
import shutil
import sys
import textwrap
from tqdm import tqdm, trange
import warnings

import booleanOperations.booleanGlyph
import defcon
from fontPens.digestPointPen import DigestPointPen
from fontTools.ufoLib import UFOWriter, UFOLibError, UFOFormatVersion

from afdko import ufotools
from afdko.ufotools import (
    thresholdAttrGlyph,
    kProcessedGlyphsLayer as PROCD_GLYPHS_LAYER,
    kProcessedGlyphsLayerName as PROCD_GLYPHS_LAYER_NAME,
)
from afdko.fdkutils import (
    get_temp_file_path,
    get_temp_dir_path,
    get_font_format,
    run_shell_command,
    validate_path,
)


UFO_FONT_TYPE = 1
TYPE1_FONT_TYPE = 2
CFF_FONT_TYPE = 3
OTF_FONT_TYPE = 4


class FocusOptionParseError(Exception):
    pass


class FocusFontError(Exception):
    pass


class FontFile(object):
    def __init__(self, font_path, font_format):
        self.font_path = font_path
        self.font_format = font_format
        self.temp_ufo_path = None
        self.font_type = None
        self.defcon_font = None
        self.use_hash_map = False
        self.ufo_font_hash_data = None
        self.save_to_default_layer = False

    def open(self, use_hash_map):
        font_path = self.font_path

        if self.font_format == 'UFO':
            self.font_type = UFO_FONT_TYPE
            ufotools.validateLayers(font_path)
            self.defcon_font = defcon.Font(font_path)
            self.ufo_format = self.defcon_font.ufoFormatVersionTuple
            self.use_hash_map = use_hash_map
            self.ufo_font_hash_data = ufotools.UFOFontData(
                font_path, self.use_hash_map,
                programName=ufotools.kCheckOutlineName)
            self.ufo_font_hash_data.readHashMap()

        else:
            print("Converting to temp UFO font...")
            self.temp_ufo_path = temp_path = get_temp_dir_path('font.ufo')

            if not run_shell_command([
                    'tx', '-ufo', font_path, temp_path]):
                raise FocusFontError(
                    'Failed to convert input font to UFO.')

            try:
                self.defcon_font = defcon.Font(temp_path)
            except UFOLibError:
                raise

            if self.font_format == 'OTF':
                self.font_type = OTF_FONT_TYPE
            elif self.font_format == 'CFF':
                self.font_type = CFF_FONT_TYPE
            else:
                self.font_type = TYPE1_FONT_TYPE

        return self.defcon_font

    def close(self):
        # Differs from save in that it saves just the hash file.
        # To be used when the glif files have not been changed by this program,
        # but the hash file has changed.
        if self.font_type == UFO_FONT_TYPE:
            # Write the hash data, if it has changed.
            self.ufo_font_hash_data.close()

    def save(self):
        if self.save_to_default_layer:
            self.defcon_font.save()
        else:
            if self.ufo_format <= UFOFormatVersion.FORMAT_2_0:
                # Up-convert the UFO to format 3
                warnings.warn("The UFO was up-converted to format 3.")
                self.ufo_format = UFOFormatVersion.FORMAT_3_0

                with UFOWriter(self.defcon_font.path,
                               formatVersion=self.ufo_format) as writer:
                    writer.getGlyphSet()
                    writer.writeLayerContents()

            writer = UFOWriter(
                self.defcon_font.path, formatVersion=self.ufo_format)
            writer.layerContents[
                PROCD_GLYPHS_LAYER_NAME] = PROCD_GLYPHS_LAYER
            layers = self.defcon_font.layers
            layer = layers[PROCD_GLYPHS_LAYER_NAME]
            glyph_set = writer.getGlyphSet(
                layerName=PROCD_GLYPHS_LAYER_NAME, defaultLayer=False)
            writer.writeLayerContents(layers.layerOrder)
            layer.save(glyph_set)

        if self.font_type == UFO_FONT_TYPE:
            ufotools.regenerate_glyph_hashes(self.ufo_font_hash_data)
            # Write the hash data, if it has changed.
            self.ufo_font_hash_data.close()

        elif self.font_type == TYPE1_FONT_TYPE:
            args = ['tx', '-t1']
            if self.font_format == 'PFB':
                args.append('-pfb')
            if not run_shell_command(
                    args + [self.temp_ufo_path, self.font_path]):
                raise FocusFontError('Failed to convert UFO font to Type 1.')

        else:
            temp_cff_path = get_temp_file_path()
            if not run_shell_command([
                    'tx', '-cff', '+S', '+b', '-std',
                    self.temp_ufo_path, temp_cff_path], suppress_output=True):
                raise FocusFontError('Failed to convert UFO font to CFF.')

            if self.font_type == CFF_FONT_TYPE:
                shutil.copy2(temp_cff_path, self.font_path)

            else:  # OTF_FONT_TYPE
                if not run_shell_command([
                        'sfntedit', '-a',
                        f'CFF={temp_cff_path}', self.font_path]):
                    raise FocusFontError('Failed to add CFF table to OTF.')

    def check_skip_glyph(self, glyph_name, do_all):
        skip = False
        if self.ufo_font_hash_data and self.use_hash_map:
            _, skip = \
                self.ufo_font_hash_data.getOrSkipGlyph(glyph_name, do_all)
        return skip

    def clear_hash_map(self):
        if self.ufo_font_hash_data is not None:
            self.ufo_font_hash_data.clearHashMap()


class COOptions(object):
    def __init__(self):
        self.file_path = None
        self.out_file_path = None
        self.log_file_path = None
        self.glyph_list = []
        self.allow_changes = False
        self.write_to_default_layer = False
        self.allow_decimal_coords = False
        self.min_area = 25
        self.tolerance = 0

        # forces all glyphs to be processed even if src hasn't changed.
        self.check_all = False

        # processing state flag, used to not repeat coincident point removal.
        self.remove_coincident_points_done = False

        # processing state flag, used to not repeat flat curve point removal.
        self.remove_flat_curves_done = False

        self.clear_hash_map = False
        self.quiet_mode = False

        # do_overlap_removal must come first in the list,
        # since it may cause problems, like co-linear lines,
        # that need to be checked/fixed by later tests.
        self.test_list = [do_overlap_removal, do_cleanup]


def parse_glyph_list_arg(glyph_string):
    glyph_string = re.sub(r"[ \t\r\n,]+", ",", glyph_string)
    glyph_list = glyph_string.split(",")
    glyph_list = map(expand_names, glyph_list)
    glyph_list = filter(None, glyph_list)
    return glyph_list


class InlineHelpFormatter(argparse.RawDescriptionHelpFormatter):

    @staticmethod
    def __add_whitespace(indent_index, indent_space, arg):
        if indent_index == 0:
            return arg
        return (" " * indent_space) + arg

    def _split_lines(self, arg, width):
        arg_rows = arg.splitlines()
        for line_index, text_line in enumerate(arg_rows):
            search = re.search(r'\s*[0-9\-]{0,}\.?\s*', text_line)
            if text_line.strip() == "":
                arg_rows[line_index] = " "
            elif search:
                indent_space = search.end()
                lines = [self.__add_whitespace(i, indent_space, x) for i, x in
                         enumerate(textwrap.wrap(text_line, width))]
                arg_rows[line_index] = lines

        # The  [''] adds an empty line between arguments.
        return [item for sublist in arg_rows for item in sublist] + ['']


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=InlineHelpFormatter,
        description=__doc__
    )
    parser.add_argument(
        '--version',
        action='version',
        version=__version__
    )
    parser.add_argument(
        '-q',
        '--quiet-mode',
        action='store_true',
        help='run in quiet mode'
    )
    parser.add_argument(
        '--no-overlap-checks',
        action='store_true',
        help='turn off path overlap checks'
    )
    parser.add_argument(
        '--no-basic-checks',
        action='store_true',
        help='turn off the following checks:\n'
             '- coincident points\n'
             '- colinear line segments\n'
             '- flat curves (straight segments with unnecessary control '
             'points)'
    )
    parser.add_argument(
        '--min-area',
        metavar='NUMBER',
        type=int,
        default=25,
        help='minimum area for a tiny outline\n'
             'Default is %(default)s square units. Subpaths with a bounding '
             'box less than this will be reported, and deleted if the -e '
             'option is used.'
    )
    parser.add_argument(
        '--tolerance',
        metavar='NUMBER',
        type=int,
        default=0,
        help='maximum allowed deviation from a straight line\n'
             'Default is %(default)s design space units. This is used to '
             'test whether the control points of a curve define a flat '
             'curve, and whether colinear line segments define a '
             'straight line.'
    )
    parser.add_argument(
        '-g',
        '--glyph-list',
        help='specify a list of glyphs to check\n'
             'Check only the specified list of glyphs. The list must be '
             'comma-delimited. The glyph IDs may be glyph indexes '
             'or glyph names. There must be no white-space in the '
             'glyph list.\n'
             'Example:\n'
             '    checkoutlinesufo -g A,B,C,69 MyFont.ufo'
    )
    parser.add_argument(
        '-f',
        '--glyph-file',
        metavar='FILE_PATH',
        type=validate_path,
        help='specify a file containing a list of glyphs to check\n'
             'Check only specific glyphs listed in a file. The '
             'file must contain a comma-delimited list of glyph '
             'identifiers. Any number of space, tab, and new-line '
             'characters are permitted between glyph names and commas.'
    )
    parser.add_argument(
        '-d',
        '--decimal',
        action='store_true',
        help='do NOT round point coordinates to integer'
    )
    parser.add_argument(
        '-e',
        '--error-correction-mode',
        action='store_true',
        help='correct reported problems\n'
             'The original font is modified. If the font is UFO, the outlines '
             'in the default layer are left untouched and a layer containing '
             'the modified version of the glyphs is added. The new layer is '
             f"named: '{PROCD_GLYPHS_LAYER}'"
    )
    parser.add_argument(
        '-o',
        '--output-file',
        metavar='FILE_PATH',
        nargs='?',
        help='Specify an output file to save to. RECOMMENDED for non-UFO '
             'inputs since the default behavior will overwrite the input.'
    )

    ufo_opts = parser.add_argument_group('UFO-only options')
    ufo_opts.add_argument(
        '-w',
        '--write-to-default-layer',
        action='store_true',
        help='write the modified glyphs to the default layer instead of '
             f"'{PROCD_GLYPHS_LAYER}'"
    )
    ufo_opts.add_argument(
        '--clear-hash-map',
        action='store_true',
        help='delete the hashes file\n'
             'By default, a file containing compact descriptions '
             '(a.k.a. hashes) of the glyphs is saved inside the '
             'UFO on the first time the font is checked. Storing '
             'these hashes allows checkoutlinesufo to be much faster '
             'on successive checks of the same UFO (because the '
             'tool will skip processing glyphs that were not '
             'modified since the last time the font was checked).'
    )
    ufo_opts.add_argument(
        '--all',
        action='store_true',
        help='force all glyphs to be processed\n'
             'Makes the tool ignore the stored hashes thus checking all the '
             'glyphs, even if they have already been processed.'
    )
    parser.add_argument(
        'font_path',
        metavar='FONT_PATH',
        type=validate_path,
        help='Path to UFO, OTF, CFF, or Type 1 font'
    )

    parsed_args = parser.parse_args(args)

    font_format = get_font_format(parsed_args.font_path)
    if font_format not in ('UFO', 'OTF', 'CFF', 'PFA', 'PFB', 'PFC'):
        parser.error('Font format is not supported.')

    if parsed_args.output_file:
        src = parsed_args.font_path
        dst = parsed_args.output_file
        if font_format == 'UFO':
            shutil.rmtree(dst, ignore_errors=True)
            shutil.copytree(src, dst)
        else:
            shutil.copy(src, dst)
        parsed_args.font_path = dst

    options = COOptions()

    if parsed_args.glyph_list:
        options.glyph_list += parse_glyph_list_arg(parsed_args.glyph_list)

    if parsed_args.glyph_file:
        gf = open(parsed_args.glyph_file)
        glyph_string = gf.read()
        glyph_string = glyph_string.strip()
        gf.close()
        options.glyph_list += parse_glyph_list_arg(glyph_string)

    if parsed_args.no_overlap_checks:
        options.test_list.remove(do_overlap_removal)
    if parsed_args.no_basic_checks:
        options.test_list.remove(do_cleanup)

    options.font_path = parsed_args.font_path
    options.font_format = font_format
    options.allow_changes = parsed_args.error_correction_mode
    options.quiet_mode = parsed_args.quiet_mode
    options.min_area = parsed_args.min_area
    options.tolerance = parsed_args.tolerance
    options.allow_decimal_coords = parsed_args.decimal
    options.check_all = parsed_args.all
    options.clear_hash_map = parsed_args.clear_hash_map
    options.write_to_default_layer = parsed_args.write_to_default_layer

    return options


def get_glyph_id(glyph_tag, font_glyph_list):
    glyph_id = None
    try:
        glyph_id = int(glyph_tag)
    except IndexError:
        pass
    except ValueError:
        try:
            glyph_id = font_glyph_list.index(glyph_tag)
        except IndexError:
            pass
        except ValueError:
            pass
    return glyph_id


def expand_names(glyph_name):
    glyph_range = glyph_name.split("-")
    if len(glyph_range) > 1:
        g1 = expand_names(glyph_range[0])
        g2 = expand_names(glyph_range[1])
        glyph_name = "%s-%s" % (g1, g2)

    elif glyph_name[0] == "/":
        glyph_name = "cid" + glyph_name[1:].zfill(5)
        if glyph_name == "cid00000":
            glyph_name = ".notdef"

    elif glyph_name.startswith("cid") and (len(glyph_name) < 8):
        glyph_name = "cid" + glyph_name[3:].zfill(5)
        if glyph_name == "cid00000":
            glyph_name = ".notdef"

    return glyph_name


def get_glyph_names(glyph_tag, font_glyph_list, font_file_name):
    glyph_name_list = []
    range_list = glyph_tag.split("-")
    prev_gid = get_glyph_id(range_list[0], font_glyph_list)
    if prev_gid is None:
        if len(range_list) > 1:
            print("\tWarning: glyph ID <%s> in range %s from glyph selection "
                  "list option is not in font. <%s>." %
                  (range_list[0], glyph_tag, font_file_name))
        else:
            print("\tWarning: glyph ID <%s> from glyph selection list option "
                  "is not in font. <%s>." % (range_list[0], font_file_name))
        return None
    try:
        glyph_name = font_glyph_list[prev_gid]
    except IndexError:
        print("\tWarning: glyph ID <%s> from glyph selection list option is "
              "not in font. <%s>." % (prev_gid, font_file_name))
        return None
    glyph_name_list.append(glyph_name)

    for glyph_tag_2 in range_list[1:]:
        gid = get_glyph_id(glyph_tag_2, font_glyph_list)
        if gid is None:
            print("\tWarning: glyph ID <%s> in range %s from glyph selection "
                  "list option is not in font. <%s>." %
                  (glyph_tag_2, glyph_tag, font_file_name))
            return None
        for i in range(prev_gid + 1, gid + 1):
            glyph_name_list.append(font_glyph_list[i])
        prev_gid = gid

    return glyph_name_list


def filter_glyph_list(options_glyph_list, font_glyph_list, font_file_name):
    # Return the list of glyphs which are in the intersection of the argument
    # list and the glyphs in the font.
    # Complain about glyphs in the argument list which are not in the font.
    glyph_list = []
    for glyph_tag in options_glyph_list:
        glyph_names = \
            get_glyph_names(glyph_tag, font_glyph_list, font_file_name)
        if glyph_names is not None:
            glyph_list.extend(glyph_names)
    return glyph_list


def get_digest(digest_glyph):
    """copied from robofab ObjectsBase.py.
    """
    mp = DigestPointPen()
    digest_glyph.drawPoints(mp)
    digest = list(mp.getDigestPointsOnly(needSort=False))
    digest.append((len(digest_glyph.contours), 0))
    return digest


def remove_coincident_points(bool_glyph, changed, msg):
    """ Remove coincident points.
    # a point is (segment_type, pt, smooth, name).
    # e.g. (u'curve', (138, -92), False, None)
    """
    for contour in bool_glyph.contours:
        op = contour._points[0]
        i = 1
        num_ops = len(contour._points)
        while i < num_ops:
            point = contour._points[i]
            segment_type = point[0]
            if segment_type is not None:
                last_op = op
                op = point
                if last_op[1] == op[1]:
                    # Coincident point. Remove it.
                    msg.append("Coincident points at (%s,%s)." % (op[1]))
                    changed = True
                    del contour._points[i]
                    num_ops -= 1
                    if segment_type == "curve":
                        # Need to delete the previous two points as well.
                        i -= 1
                        del contour._points[i]
                        i -= 1
                        del contour._points[i]
                        num_ops -= 2
                    # Note that we do not increment i - this will
                    # now index the next point.
                    continue
            i += 1
    return changed, msg


def remove_tiny_sub_paths(bool_glyph, min_area, msg):
    """
    Removes tiny subpaths that are created by overlap removal when the start
    and end path segments cross each other, rather than meet.
    """
    num_contours = len(bool_glyph.contours)
    ci = 0
    while ci < num_contours:
        contour = bool_glyph.contours[ci]
        ci += 1
        on_line_pts = filter(lambda pt: pt[0] is not None, contour._points)
        num_pts = len(list(on_line_pts))
        if num_pts <= 4:
            c_bounds = contour.bounds
            c_area = (c_bounds[2] - c_bounds[0]) * (c_bounds[3] - c_bounds[1])
            if c_area < min_area:
                ci -= 1
                num_contours -= 1
                msg.append(
                    "Contour %s is too small: bounding box is less than "
                    "minimum area. Start point: (%s)." %
                    (ci, contour._points[0][1]))
                del bool_glyph.contours[ci]
    return msg


def is_colinear_line(b3, b2, b1, tolerance=0):
    # b1-b3 are three points  [x, y] to test.
    # b1 = start point of first line,
    # b2 is start point of second,
    # b3 is end point of second line.
    # tolerance is how tight the match must be in design units, default 0.
    x1 = b1[-2]
    y1 = b1[-1]
    x2 = b2[-2]
    y2 = b2[-1]
    x3 = b3[-2]
    y3 = b3[-1]

    dx2 = x2 - x1
    dy2 = y2 - y1
    if (dx2 == 0) and (dy2 == 0):
        return True

    if (x3 - x1) == 0:
        if abs(dx2) > tolerance:
            return False
        else:
            return True

    if (y3 - y1) == 0:
        if abs(dy2) > tolerance:
            return False
        else:
            return True

    dx3 = x3 - x2
    dy3 = y3 - y2
    if (dx3 == 0) and (dy3 == 0):
        return False

    if dx2 == 0:
        if abs(x3 - x1) > tolerance:
            return False
        else:
            return True

    if dy2 == 0:
        if abs(y3 - y1) > tolerance:
            return False
        else:
            return True

    a = float(dy2) / dx2
    b = y1 - a * x1
    if abs(a) <= 1:
        y3t = a * x3 + b
        if abs(y3t - y3) > tolerance:
            return False
        else:
            return True
    else:
        x3t = float(y3 - b) / a
        if abs(x3t - x3) > tolerance:
            return False
        else:
            return True


def remove_flat_curves(new_glyph, changed, msg, options):
    """ Remove flat curves.
    # a point is (segment_type, pt, smooth, name).
    # e.g. (u'curve', (138, -92), False, None)
    """
    for contour in new_glyph.contours:
        num_ops = len(contour._points)
        i = 0
        while i < num_ops:
            p0 = contour._points[i]
            segment_type = p0[0]

            if segment_type == "curve":
                p3 = contour._points[i - 3]
                p2 = contour._points[i - 2]
                p1 = contour._points[i - 1]
                # fix flat curve.
                if is_colinear_line(
                        p3[1], p2[1], p0[1], options.tolerance):
                    if is_colinear_line(
                            p3[1], p1[1], p0[1], options.tolerance):
                        msg.append("Flat curve at (%s, %s)." % (p0[1]))
                        changed = True
                        p0 = list(p0)
                        p0[0] = "line"  # segment_type
                        contour._points[i] = tuple(p0)
                        num_ops -= 2
                        if i > 0:
                            del contour._points[i - 1]
                            del contour._points[i - 2]
                            i -= 2
                        else:
                            del contour._points[i - 1]
                            del contour._points[i - 1]
            i += 1
    return changed, msg


def remove_colinear_lines(new_glyph, changed, msg, options):
    """ Remove colinear line- curves.
    # a point is (segment_type, pt, smooth, name).
    # e.g. (u'curve', (138, -92), False, None)
    """
    for contour in new_glyph.contours:
        num_ops = len(contour._points)
        if num_ops < 3:
            # Need at least two line segments to have a co-linear line.
            continue
        i = 0
        while i < num_ops:
            p0 = contour._points[i]
            contour_type = p0[0]
            # Check for co-linear line.
            if contour_type == "line":
                p1 = contour._points[i - 1]
                # if p1 is a curve  point, no need to do colinear check!
                if p1[0] == "line":
                    p2 = contour._points[i - 2]
                    if is_colinear_line(
                            p2[1], p1[1], p0[1], options.tolerance):
                        msg.append("Colinear line at (%s, %s)." % (p1[1]))
                        changed = True
                        del contour._points[i - 1]
                        num_ops -= 1
                        if num_ops < 3:
                            # Need at least two line segments
                            # to have a co-linear line.
                            break
                        i -= 1
            i += 1
    return changed, msg


def split_touching_paths(new_glyph):
    """ This hack fixes a design difference between the Adobe checkoutlines
    logic and booleanGlyph, and is used only when comparing the two. With
    checkoutlines, if only a single point on a contour lines is coincident
    with the path of the another contour, the paths are NOT merged. With
    booleanGlyph, they are merged. An example is the vertex of a diamond
    shape having the same y coordinate as a horizontal line in another path,
    but no other overlap. However, this works only when a single point on one
    contour is coincident with another contour, with no other overlap. If
    there is more than one point of contact, then result is separate inner
    (unfilled) contour. This logic works only when the touching contour is
    merged with the other contour.
    """
    num_paths = len(new_glyph.contours)
    i = 0
    while i < num_paths:
        contour = new_glyph.contours[i]
        num_pts = len(contour._points)
        sort_list = [[contour._points[j], j] for j in range(num_pts)]
        sort_list = filter(lambda entry: entry[0][0] is not None, sort_list)
        sort_list.sort()
        j = 1
        len_sort_list = len(sort_list)
        have_new_contour = False
        while j < len_sort_list:
            coords1, j1 = sort_list[j][0][1], sort_list[j][1]
            coords0, j0 = sort_list[j - 1][0][1], sort_list[j - 1][1]
            if (num_pts != (j1 + 1)) and \
                    ((j1 - j0) > 1) and \
                    (coords0 == coords1):
                # If the contour has two non-contiguous points that are
                # coincident, and are not the start and end points, then we
                # have  contour that can be pinched off. I don't bother to
                # test that j0 is the start point, as UFO paths are always
                # closed.
                new_contour_pts = contour._points[j0:j1 + 1]
                np0 = new_contour_pts[0]
                if np0[0] == 'curve':
                    new_contour_pts[0] = ('line', np0[1], np0[2], np0[3])
                new_contour = booleanOperations.booleanGlyph.BooleanContour()
                new_contour._points = new_contour_pts
                new_contour._clockwise = new_contour._get_clockwise()
                # print("i", i)
                # print("coords1, j1", coords1, j1)
                # print("coords0, j0 ", coords0, j0)
                # print("new_contour._clockwise", repr(new_contour._clockwise))
                if 1:  # new_contour.clockwise == contour.clockwise:
                    # print("un-merging", new_contour._points,
                    #       repr(new_contour._clockwise))
                    del contour._points[j0 + 1:j1 + 1]
                    new_glyph.contours.insert(i + 1, new_contour)
                    have_new_contour = True
                    num_paths += 1
                    break
            j += 1
        if have_new_contour:
            continue
        i += 1


def round_point(pt):
    return int(pt[0]), int(pt[1])


def do_overlap_removal(bool_glyph, changed, msg, options):
    changed, msg = remove_coincident_points(bool_glyph, changed, msg)
    options.remove_coincident_points_done = True
    changed, msg = remove_flat_curves(bool_glyph, changed, msg, options)
    changed, msg = remove_colinear_lines(bool_glyph, changed, msg, options)
    options.remove_flat_curves_done = True
    # I need to fix these in the source, or the old vs new digests will differ,
    # as BooleanOperations removes these even if it does not do overlap
    # removal.
    old_digest = sorted(get_digest(bool_glyph))
    old_digest = [round_point(pt) for pt in old_digest]
    new_digest = []
    prev_digest = old_digest
    new_glyph = bool_glyph

    while new_digest != prev_digest:
        # This is a hack to get around a bug in booleanGlyph. Consider an M
        # sitting on a separate rectangular crossbar contour, the bottom of
        # the M legs being co-linear with the top of the cross bar. pyClipper
        # will merge only one of the co-linear lines with each call to
        # removeOverlap(). I suspect that this bug is in pyClipper, but
        # haven't yet looked.
        prev_digest = new_digest
        new_glyph = new_glyph.removeOverlap()
        new_digest = sorted(get_digest(new_glyph))
        # The new path points sometimes come back with very small
        # fractional parts to to rounding issues.
        new_digest = [round_point(pt) for pt in new_digest]

    # Can't use change in path number to see if something has changed
    # - overlap removal can add and subtract paths.
    if str(old_digest) != str(new_digest):
        changed = True
        msg.append("There is an overlap.")
    # Don't need to remove coincident points again.
    # These are not added by overlap removal.
    # Tiny subpaths are added by overlap removal.
    msg = remove_tiny_sub_paths(new_glyph, options.min_area, msg)
    return new_glyph, changed, msg


def do_cleanup(new_glyph, changed, msg, options):

    if new_glyph is None:
        return new_glyph, changed, msg

    # Note that these remove_coincident_points_done and remove_flat_curves_done
    # get called only if do_overlap_removal is NOT called.
    if not options.remove_coincident_points_done:
        changed, msg = remove_coincident_points(new_glyph, changed, msg)
        options.remove_coincident_points_done = True
    if not options.remove_flat_curves_done:
        changed, msg = remove_flat_curves(new_glyph, changed, msg, options)
    # I call remove_colinear_lines even when do_overlap_removal is called,
    # as the latter can introduce new co-linear points.
    changed, msg = remove_colinear_lines(new_glyph, changed, msg, options)

    return new_glyph, changed, msg


def set_max_p(contour):
    # Find the topmost point
    # If more than one point has the same Y, choose the leftmost.
    # Used to sort contours.
    max_p = contour[0]
    for point in contour:
        if point.segmentType is None:
            continue
        if max_p.y < point.y:
            max_p = point
        elif max_p.y == point.y:
            if max_p.x < point.y:
                max_p = point
    contour.maxP = max_p


def sort_contours(c1, c2):
    if not hasattr(c1, 'maxP'):
        set_max_p(c1)
    if not hasattr(c2, 'maxP'):
        set_max_p(c2)

    if c1.maxP.y > c2.maxP.y:
        return 1
    elif c1.maxP.y < c2.maxP.y:
        return -1

    if c1.maxP.x > c2.maxP.x:
        return 1
    elif c1.maxP.x < c2.maxP.x:
        return -1

    lc1 = len(c1)
    lc2 = len(c2)
    if lc1 > lc2:
        return 1
    elif lc1 < lc2:
        return -1
    return 0


def restore_contour_order(fixed_glyph, original_contours):
    """ The pyClipper library first sorts all the outlines by x position,
    then y position. I try to undo that, so that un-touched contours will end
    up in the same order as the in the original, and any conbined contours
    will end up in a similar order. The reason I try to match new contours
    to the old is to reduce arbitraryness in the new contour order between
    similar fonts. I can't completely avoid this, but I can reduce how often
    it happens.
    """
    if len(fixed_glyph) == 0:
        return

    new_contours = list(fixed_glyph)
    if len(new_contours) > 1:
        new_contours.sort(key=cmp_to_key(sort_contours))
    else:
        set_max_p(new_contours[0])
    new_index_list = range(len(new_contours))
    new_list = [[i, new_contours[i]] for i in new_index_list]
    old_index_list = range(len(original_contours))
    old_list = [[i, original_contours[i]] for i in old_index_list]
    order_list = []

    # Match contours that have not changed.
    # This will fix the order of the contours that have not been touched.
    num_contours = len(new_list)
    if num_contours > 0:  # If the new contours aren't already all matched..
        for i in range(num_contours):
            ci, contour = new_list[i]
            for j in old_index_list:
                ci2, old_contour = old_list[j]
                # skip if is not the same set of values.
                if len(old_contour) != len(contour):
                    continue
                if old_contour[0] != old_contour[0]:
                    continue
                order_list.append([ci2, ci])
                new_list[i] = None
                break

    new_list = [item for item in new_list if item]
    num_contours = len(new_list)
    # Check each extreme for a match.
    if num_contours > 0:
        # ctr_starts tracks start points per contour
        ctr_starts = {n: [] for n in range(len(fixed_glyph))}
        for i in range(num_contours):
            ci, contour = new_list[i]
            max_p = contour.maxP
            # Now search the old contour list.
            for j in old_index_list:
                ci2, old_contour = old_list[j]
                matched = False
                for old_point in old_contour:
                    if old_point.segmentType is None:
                        continue
                    if (max_p.x == old_point.x) and (max_p.y == old_point.y):
                        new_list[i] = None
                        order_list.append([ci2, ci])
                        matched = True
                        # See if we can set the start point in the new contour
                        # to match the old one.
                        if not ((old_contour[0].x == contour[0].x) and
                                (old_contour[0].y == contour[0].y)):
                            old_start_point = old_contour[0]
                            for pi, point in enumerate(contour):
                                if (point.x == old_start_point.x) \
                                        and (point.y == old_start_point.y) \
                                        and point.segmentType is not None:
                                    try:
                                        msg = ("Failed assertion: duplicated "
                                               f"start point on contour {ci} "
                                               f"at {point.x}, {point.y} of "
                                               f"glyph {fixed_glyph.name}.")
                                        assert not ctr_starts[ci], msg
                                    except KeyError:
                                        msg = (f"Contour index {ci} out "
                                               "of range on updated glyph "
                                               f"{fixed_glyph.name}")
                                        raise KeyError(msg)
                                    contour.setStartPoint(pi)
                                    ctr_starts[ci].append(pi)

                        break
                if matched:
                    break
        new_list = [item for item in new_list if item]
        num_contours = len(new_list)

    # If the algorithm didn't work for some contours,
    # just add them on the end.
    if num_contours != 0:
        ci2 = len(new_contours)
        for ci, _contour in new_list:
            order_list.append([ci2, ci])

    # Now re-order the new list
    order_list.sort()
    new_contour_list = []
    for _ci2, ci in order_list:
        new_contour_list.append(new_contours[ci])

    fixed_glyph.clearContours()
    for contour in new_contour_list:
        fixed_glyph.appendContour(contour)


RE_SPACE_PATTERN = re.compile(
    r"space|uni(00A0|1680|180E|202F|205F|3000|FEFF|200[0-9AB])")


def run(args=None):
    options = get_options(args)
    font_path = options.font_path
    font_format = options.font_format
    font_file = FontFile(font_path, font_format)
    use_hash_map = True if font_format == 'UFO' else False
    defcon_font = font_file.open(use_hash_map)

    # We allow use of a hash map to skip glyphs only if fixing glyphs
    if options.clear_hash_map:
        font_file.clear_hash_map()
        return

    if defcon_font is None:
        print("Could not open file: %s." % font_path)
        return

    if not options.glyph_list:
        glyph_list = list(defcon_font.keys())
    else:
        if not defcon_font.glyphOrder:
            raise FocusFontError(
                "Error: public.glyphOrder is empty or missing "
                "from lib.plist file of %s" % font_path)
        else:
            glyph_list = filter_glyph_list(
                options.glyph_list, defcon_font.glyphOrder, font_path)
    if not glyph_list:
        raise FocusFontError(
            "Error: selected glyph list is empty for font <%s>." % font_path)

    if (font_format == 'UFO') and (not options.write_to_default_layer):
        try:
            processed_layer = defcon_font.layers[PROCD_GLYPHS_LAYER_NAME]
        except KeyError:
            processed_layer = defcon_font.newLayer(PROCD_GLYPHS_LAYER_NAME)
    else:
        processed_layer = None
        font_file.save_to_default_layer = True

    font_changed = False
    seen_glyph_count = 0
    processed_glyph_count = 0

    max_length = max([len(g) for g in glyph_list])
    fmt = '{:<%d}' % max_length
    with trange(len(glyph_list)) as t:
        for i in t:
            glyph_name = sorted(glyph_list)[i]
            t.set_description('Checking outlines for %s' %
                              fmt.format(glyph_name))
            changed = False
            seen_glyph_count += 1
            msg = []

            if glyph_name not in defcon_font:
                continue

            # font_file.check_skip_glyph updates the hash map for the glyph,
            # so we call it even when the  '-all' option is used.
            skip = font_file.check_skip_glyph(glyph_name, options.check_all)
            # Note: this will delete glyphs from the processed layer,
            #       if the glyph hash has changed.
            if skip:
                continue
            processed_glyph_count += 1

            defcon_glyph = defcon_font[glyph_name]
            if defcon_glyph.components:
                defcon_glyph.decomposeAllComponents()
            new_glyph = booleanOperations.booleanGlyph.BooleanGlyph(
                defcon_glyph)
            if len(new_glyph) == 0:
                # Complain about empty glyph only if it is not a space glyph.
                if not RE_SPACE_PATTERN.search(glyph_name):
                    msg = ["has no contours"]
                else:
                    msg = []
            else:
                for test in options.test_list:
                    if test is not None:
                        new_glyph, changed, msg = \
                            test(new_glyph, changed, msg, options)

            if not options.quiet_mode:
                if msg:
                    tqdm.write('%s %s' % (glyph_name, ' '.join(msg)))
            if changed and options.allow_changes:
                font_changed = True
                original_contours = list(defcon_glyph)
                if font_file.save_to_default_layer:
                    fixed_glyph = defcon_glyph
                    fixed_glyph.clearContours()
                else:
                    # this will replace any pre-existing glyph:
                    processed_layer.newGlyph(glyph_name)
                    fixed_glyph = processed_layer[glyph_name]
                    fixed_glyph.width = defcon_glyph.width
                    fixed_glyph.height = defcon_glyph.height
                    fixed_glyph.unicodes = defcon_glyph.unicodes
                point_pen = fixed_glyph.getPointPen()
                new_glyph.drawPoints(point_pen)
                if options.allow_decimal_coords:
                    for contour in fixed_glyph:
                        for point in contour:
                            point.x = round(point.x, 3)
                            point.y = round(point.y, 3)
                else:
                    for contour in fixed_glyph:
                        for point in contour:
                            point.x = int(round(point.x))
                            point.y = int(round(point.y))

                # JH May 2020: remove overlap can leave some coincident points
                # we use thresholdAttrGlyph (modified thresholdPen) to remove
                # them prior to restore_contour_order.
                thresholdAttrGlyph(fixed_glyph, 1)

                restore_contour_order(fixed_glyph, original_contours)

            # The following is needed when the script is called from another
            # script with Popen():
            sys.stdout.flush()
            if i == len(glyph_list) - 1:
                t.set_description("Finished checkoutlinesufo")

    # update layer plist: the hash check call may have deleted processed layer
    # glyphs because the default layer glyph is newer.

    # At this point, we may have deleted glyphs in the
    # processed layer.writer.getGlyphSet()
    # will fail unless we update the contents.plist file to match.
    if options.allow_changes:
        ufotools.validateLayers(font_path, False)

    if not font_changed:
        # Even if the program didn't change any glyphs,
        # we should still save updates to the src glyph hash file.
        font_file.close()
    else:
        font_file.save()

    if processed_glyph_count != seen_glyph_count:
        print("Skipped %s of %s glyphs." %
              (seen_glyph_count - processed_glyph_count, seen_glyph_count))
    if not options.quiet_mode:
        print("Done with font")


def main():
    try:
        run(sys.argv[1:])
    except (FocusOptionParseError, FocusFontError) as focus_error:
        print("Quitting after error.", focus_error)


if __name__ == '__main__':
    main()
