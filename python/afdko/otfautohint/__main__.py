# Copyright 2014,2022 Adobe. All rights reserved.

"""
Auto-hinting program for OpenType/CFF and cubic spline UFO fonts.
"""

import argparse
import logging
import os
import re
import subprocess
import sys
import textwrap

from . import get_font_format
from .logging import logging_conf
from .autohint import ACOptions, hintFiles

__version__ = """\
otfautohint v3.0.0 May 26 2023
"""

FONTINFO_FILE_NAME = 'fontinfo'

GENERAL_INFO = """
otfautohint is a tool for hinting fonts. It supports the following formats:
CFF (a.k.a. Type 2), OTF (name- and CID-keyed), and UFO.

By default, otfautohint will hint all glyphs in the input font. Options are
available to specify a subset of the glyphs for hinting and other settings.

Note that the hinting results are better if the font's alignment zones are set
properly. For a font supporting a bicameral script (e.g. Latin, Greek, or
Cyrillic) the zones should capture the baseline, the x-height and the capital
height, and respective overshoots of the glyphs, at the very least.
Additionally, ascender, descender, small cap, and figures heights may also be
considered.

The reports provided by the stemHist tool can be useful for choosing alignment
zone and stem width values.
"""

FDDICT_DOC = r"""
By default, otfautohint uses the font's global alignment zones and stem widths
when hinting each glyph. However, if there is a file named 'fontinfo' in the
same directory as the input font file, otfautohint will search it for
definitions of sets of alignment zones (a.k.a 'FDDict'), and for matching
lists of glyphs to which each FDDict should be applied to. This approach
allows a group of glyphs to be hinted using a different set of zones and stem
widths than other glyphs.

If FDDict definitions are used, then the global alignment zones and stem widths
in the source font will be ignored. For any glyphs not covered by an explicit
FDDict definition, otfautohint will synthesize a dummy FDDict, where the zones
are set outside of the font's bounding box, so not to influence the hinting.
This is desirable for glyphs that have no features that need to be aligned.

To use a FDDict, one must define both the values of alignment zones and stem
widths, and the set of glyphs to apply it to. The FDDict must be defined in
the file before the set of glyphs which belong to it. Both the FDDict and the
glyph set define a name; an FDDict is applied to the glyph set with the same
name.

A fontinfo file can contain multiple FDDict definitions with the same name.
The values from each definition will be merged, with later values overriding
earlier ones in the case of duplicates. Defining multiple GlyphSets with
the same name is not allowed.

Running otfautohint with the option --print-dflt-fddict will provide the list
of default FDDict values for the source font. These can be used as a starting
point for alternate FDDict definitions.

Running otfautohint with the option --print-list-fddict will provide all the
user-defined FDDict defintions, as well as the list of glyphs associated with
each of them. One can use these to confirm the values, and to check which
glyphs are assigned to which FDDict. In particular, one should check the glyph
list of the FDDict named 'No Alignment Zones'; this list contains the glyphs
that did not match any of the search terms of the user-defined FDDicts.

The definitions use the following syntax:

begin FDDict <name>
    <key-1> <value-1>
    <key-2> <value-2>
    ...
    <key-n> <value-n>
end FDDict <name>

begin GlyphSet <name>
    <glyphname-1> <glyphname-2> ...
    <glyphname-n>
end GlyphSet <name>

The glyph names may be either a real glyph name, or a regular expression
designed to match several names.

An abbreviated regex primer:
^ ..... Matches at the start of the glyph name
$ ..... Matches at the end
[aABb]  Matches any one character in the set a, A, b, B
[A-Z]   Matches any one character in the set comprising the range from A-Z
[abA-Z] Matches any one character in the set a, b, and the characters
          in the range from A-Z
. ..... Matches any single character
+ ..... Maches whatever preceded it one or more times
* ..... Matches whatever preceded it none or more times.
\ ..... An escape character that includes the following character without
          the second one being interpreted as a regex special character

Examples:
^[A-Z]$    Matches names with one character in the range from A-Z.
^[A-Z].+   Matches any name where the first character is in the range A-Z,
             and it is followed by one or more characters.
[A-Z].+    Matches any name with a character that is in the range A-Z and
             which is followed by one or more characters.
^[A-Z].*   Matches any name with one or more characters, and
             the first character is in the range A-Z
^.+\.smallcaps   Matches any name that contains ".smallcaps"
^.+\.smallcaps$  Matches any name that ends with ".smallcaps"
^.+\.s[0-24]0$   Matches any name that ends with ".s00",".s10",".s20" or ".s40"


Example FDDict and GlyphSet definitions
=======================================

begin FDDict ST_Smallcaps
    # It's good practice to put the non-hint stuff first
    OrigEmSqUnits 1000
    FlexOK true
    # This gets used as the hint dict name if the font
    # is eventually built as a CID font.
    FontName MyFont-Bold

    # Alignment zones
    # The first is a bottom zone, the rest are top zones
    BaselineOvershoot -20
    BaselineYCoord 0
    CapHeight 900
    CapOvershoot 20
    LcHeight 700
    LcOvershoot 15

    # Stem widths
    DominantV [236 267]
    DominantH [141 152]
end FDDict ST_Smallcaps


begin FDDict LM_Smallcaps
    OrigEmSqUnits 1000
    FlexOK true
    FontName MyFont-Bold
    BaselineOvershoot -25
    BaselineYCoord 0
    CapHeight 950
    CapOvershoot 25
    LcHeight 750
    LcOvershoot 21
    DominantV [236 267]
    DominantH [141 152]
end FDDict LM_Smallcaps


begin GlyphSet LM_Smallcaps
    [Ll]\S+\.smallcap  [Mm]\S+\.smallcap
end GlyphSet LM_Smallcaps


begin GlyphSet ST_Smallcaps
    [Tt]\S+\.smallcap  [Ss]\S+\.smallcap
end GlyphSet ST_Smallcaps

***************************************

Note that whitespace must exist between keywords and values, but is otherwise
ignored. '#' marks the start of a comment; all the text after this character
on a line is ignored. GlyphSet and FDDict definitions may be intermixed,
as long as any FDDict is defined before the GlyphSet which refers to it.

At least two BlueValue pairs (the 'BaselineYCoord' bottom zone and any top
zone), and DominantH and DominantV values must be provided. All other keywords
are optional.

The full set of recognized FDDict keywords is:

BlueValue pairs:
    # BaselineOvershoot is a bottom zone, the rest are top zones.
    BaselineYCoord
    BaselineOvershoot

    CapHeight
    CapOvershoot

    LcHeight
    LcOvershoot

    AscenderHeight
    AscenderOvershoot

    FigHeight
    FigOvershoot

    Height5
    Height5Overshoot

    Height6
    Height6Overshoot

OtherBlues pairs:
    Baseline5Overshoot
    Baseline5

    Baseline6Overshoot
    Baseline6

    SuperiorOvershoot
    SuperiorBaseline

    OrdinalOvershoot
    OrdinalBaseline

    DescenderOvershoot
    DescenderHeight

FamilyBlueValue pairs:
    # Any BlueValue keyword preceded by "Family"

FamilyOtherBlueValue pairs:
    # Any OtherBlueValue keyword preceded by "Family"

For zones which capture the bottom of a feature in a glyph --BaselineYCoord
and all OtherBlues values-- the value specifies the top of the zone, and the
'Overshoot' is a negative value which specifes the offset to the bottom of
the zone, e.g.
    BaselineYCoord 0
    BaselineOvershoot -12

For zones which capture the top of a feature in a glyph --all BlueValue values
except BaselineYCoord-- the value specifies the bottom of the zone, and the
'Overshoot' is a positive value which specifes the offset to the top of the
zone, e.g.
    Height6 800
    Height6Overshoot 20

Note also that there is no implied sequential order of values. Height6 may have
a value less than or equal to CapHeight.

The values for keywords in one FontDict definiton are completely independent
from the values used in another FontDict. There is no inheritance from one
definition to the next.

Miscellaneous values:
  FontName ..... PostScript font name.
                 Only used by makeotf when building a CID font.
  OrigEmSqUnits  Single value: size of em-square.
                 Only used by makeotf when building a CID font.
  LanguageGroup  0 or 1. Specifies whether counter hints for ideographic
                 glyphs should be applied.
                 Only used by makeotf when building a CID font.
  DominantV .... List of dominant vertical stems, in the form
                 [<stem-value-1> <stem-value-2> ...]
  DominantH .... List of dominant horizontal stems, in the form
                 [<stem-value-1> <stem-value-2> ...]
  FlexOK ....... true or false.
  VCounterChars  List of glyphs to which counter hints may be applied,
                 in the form [<glyph-name-1> <glyph-name-2> ...]
  HCounterChars  List of glyphs to which counter hints may be applied,
                 in the form [<glyph-name-1> <glyph-name-2> ...]

Counter hints help to keep the space between stems open and equal in size.
otfautohint will try and add counter hints to only a short hard-coded list
of glyphs,
    V-counters: m, M, T, ellipsis
    H-counters: element, equivalence, notelement, divide

To extend this list, use the VCounterChars and HCounterChars keywords.
A maximum of 64 glyph names can be added to either the vertical or
horizontal list.

---
Note for cognoscenti: the otfautohint program code ignores StdHW and StdVW
entries if DominantV and DominantH entries are present, so it omits writing the
Std[HV]W keywords to fontinfo file. Also, otfautohint will add anynon-duplicate
stem width values for StemSnap[HV] to the Dominant[HV] stem width list, but the
StemSnap[HV] entries are not necessary if the full list of stem widths are
supplied as values for the Dominant[HV] keywords. Hence it also writes the full
stem list for the Dominant[HV] keywords, and does not write the StemSnap[HV]
keywords, to the fontinfo file. This is technically not right, as Dominant[HV]
array is supposed to hold only two values, but the otfautohint program is not
affected, and it can write fewer entries this way.
"""


def _read_txt_file(file_path):
    with open(file_path, encoding='utf-8') as f:
        return f.read()


def _expand_cid_name(glyph_name, name_aliases):
    glyphRange = glyph_name.split("-")
    if len(glyphRange) > 1:
        g1 = _expand_cid_name(glyphRange[0], name_aliases)
        g2 = _expand_cid_name(glyphRange[1], name_aliases)
        glyph_name = "%s-%s" % (g1, g2)

    elif glyph_name[0] == "/":
        glyph_name = "cid" + glyph_name[1:].zfill(5)
        if glyph_name == "cid00000":
            glyph_name = ".notdef"
            name_aliases[glyph_name] = "cid00000"

    elif glyph_name.startswith("cid") and (len(glyph_name) < 8):
        glyph_name = "cid" + glyph_name[3:].zfill(5)
        if glyph_name == "cid00000":
            glyph_name = ".notdef"
            name_aliases[glyph_name] = "cid00000"

    return glyph_name


def _process_glyph_list_arg(glyph_list, name_aliases):
    return [_expand_cid_name(n, name_aliases) for n in glyph_list]


class HintOptions(ACOptions):
    def __init__(self, pargs):
        super(HintOptions, self).__init__()
        self.inputPaths = pargs.font_paths
        self.outputPaths = pargs.output_paths
        self.referenceFont = pargs.reference_font
        self.referenceOutputPath = pargs.reference_out
        self.hintAll = pargs.hint_all_ufo
        self.allowChanges = pargs.allow_changes
        self.noFlex = pargs.no_flex
        self.noHintSub = pargs.no_hint_sub
        self.allow_no_blues = pargs.no_zones_stems
        self.ignoreFontinfo = pargs.ignore_fontinfo
        self.logOnly = pargs.report_only
        self.removeConflicts = not pargs.keep_conflicts
        self.printFDDictList = pargs.print_list_fddict
        self.printAllFDDict = pargs.print_all_fddict
        self.roundCoords = not pargs.decimal
        self.writeToDefaultLayer = pargs.write_to_default_layer
        self.maxSegments = pargs.max_segments
        self.verbose = pargs.verbose
        if pargs.force_overlap:
            self.overlapForcing = True
        elif pargs.force_no_overlap:
            self.overlapForcing = False
        if pargs.processes:
            self.process_count = pargs.processes


class _CustomHelpFormatter(argparse.RawDescriptionHelpFormatter):
    """
    Adds extra line between options
    """
    @staticmethod
    def __add_whitespace(i, i_wtsp, arg):
        if i == 0:
            return arg
        return (" " * i_wtsp) + arg

    def _split_lines(self, arg, width):
        arg_rows = arg.splitlines()
        for i, line in enumerate(arg_rows):
            search = re.search(r'\s*[0-9\-]{0,}\.?\s*', line)
            if line.strip() == "":
                arg_rows[i] = " "
            elif search:
                line_wtsp = search.end()
                lines = [self.__add_whitespace(j, line_wtsp, x)
                         for j, x in enumerate(textwrap.wrap(line, width))]
                arg_rows[i] = lines

        # [''] adds the extra line between args
        return [item for sublist in arg_rows for item in sublist] + ['']


class _AdditionalHelpAction(argparse.Action):
    """
    Based on argparse's _HelpAction and _VersionAction.
    """

    def __init__(self,
                 option_strings,
                 addl_help=None,
                 dest=argparse.SUPPRESS,
                 default=argparse.SUPPRESS,
                 help=None):
        super(_AdditionalHelpAction, self).__init__(
            option_strings=option_strings,
            dest=dest,
            default=default,
            nargs=0,
            help=help)
        self.addl_help = addl_help

    def __call__(self, parser, namespace, values, option_string=None):
        formatter = parser._get_formatter()
        formatter.add_text(self.addl_help)
        parser.exit(message=formatter.format_help())


def _split_comma_sequence(comma_str):
    return [item.strip() for item in comma_str.split(',')]


def _check_save_path(path_str):
    test_path = os.path.abspath(os.path.realpath(path_str))
    del_test_file = True
    check_path = test_path
    if os.path.isdir(test_path):
        check_path = os.path.join(test_path, "dummy")
    try:
        if os.path.exists(check_path):
            del_test_file = False
        open(check_path, 'a').close()
        if del_test_file:
            os.remove(check_path)
    except (OSError):
        raise argparse.ArgumentTypeError(
            f"{test_path} is not a valid path to write to.")
    return test_path


def _check_tx():
    try:
        subprocess.check_output(["tx", "-h"], stderr=subprocess.STDOUT)
        return True
    except (subprocess.CalledProcessError, OSError):
        return False


def _validate_font_paths(path_lst, parser):
    """
    Checks that all input paths are fonts, and that all are the same format.
    Returns the font format string.
    """
    has_tx = None
    format_set = set()
    for path in path_lst:
        font_format = get_font_format(path)
        base_name = os.path.basename(path)
        if font_format is None:
            parser.error(f"{base_name} is not a supported font format")
        if font_format in ("PFA", "PFB", "PFC"):
            if has_tx is None:
                has_tx = _check_tx()
            if not has_tx:
                parser.error(f"{base_name} font format requires 'tx'. "
                             "Please install 'afdko'.")
        format_set.add(font_format)
    if len(format_set) > 1:
        parser.error("the input fonts must be all of the same format")
    # get the set's single-item via tuple unpacking
    ft_format_str, = format_set
    return ft_format_str


def _validate_path(path_str):
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            f"{path_str} is not a valid path.")
    return valid_path


RE_COUNTER_CHARS = re.compile(
    r"([VH]CounterChars)\s+[\(\[]\s*([^\)\r\n]+)[\)\]]")
RE_COMMENT_LINE = re.compile(r"#[^\r\n]+")


def _parse_fontinfo_file(options, fontinfo_path):
    """
    Collects VCounterChars and HCounterChars values from a fontinfo file,
    and updates the ACOptions class.
    """
    data = _read_txt_file(fontinfo_path)
    data = re.sub(RE_COMMENT_LINE, "", data)
    counterGlyphLists = RE_COUNTER_CHARS.findall(data)
    for glname, glist in counterGlyphLists:
        # True indicates there should be an error if the glyph is missing
        if glist:
            glyphList = {k: True for k in glist.split()}
            if glname[0] == "V":
                options.vCounterGlyphs.update(glyphList)
            else:
                options.hCounterGlyphs.update(glyphList)


def add_common_options(parser, term):
    parser_fonts = parser.add_argument(
        'font_paths',
        metavar='FONT',
        nargs='*',
        type=_validate_path,
        help='Type1/CFF/OTF/UFO font files'
    )
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=0,
        help='verbose mode\n'
             'Use -vv for extra-verbose mode.'
    )
    glyphs_parser = parser.add_mutually_exclusive_group()
    glyphs_parser.add_argument(
        '-g',
        '--glyphs',
        metavar='GLYPH_LIST',
        dest='glyphs_to_hint',
        type=_split_comma_sequence,
        default=[],
        help='comma-separated sequence of glyphs to %s\n' % term +
             'The glyph identifiers may be glyph indexes, glyph names, or '
             'glyph CIDs. CID values must be prefixed with a forward slash.\n'
             'Examples:\n'
             '    otfautohint -g A,B,C,69 MyFont.ufo\n'
             '    otfautohint -g /103,/434,68 MyCIDFont'
    )
    glyphs_parser.add_argument(
        '--glyphs-file',
        metavar='PATH',
        dest='glyphs_to_hint_file',
        type=_validate_path,
        help='file containing a list of glyphs to %s\n' % term +
             'The file must contain a comma-separated sequence of glyph '
             'identifiers.'
    )
    glyphs_parser.add_argument(
        '-x',
        '--exclude-glyphs',
        metavar='GLYPH_LIST',
        dest='glyphs_to_not_hint',
        type=_split_comma_sequence,
        default=[],
        help='comma-separated sequence of glyphs to NOT %s\n' % term +
             "Counterpart to '--glyphs' option."
    )
    glyphs_parser.add_argument(
        '--exclude-glyphs-file',
        metavar='PATH',
        dest='glyphs_to_not_hint_file',
        type=_validate_path,
        help='file containing a list of glyphs to NOT %s\n' % term +
             "Counterpart to '--glyphs-file' option."
    )
    overlap_parser = parser.add_mutually_exclusive_group()
    overlap_parser.add_argument(
        '-m',
        '--overlap-glyphs',
        metavar='GLYPH_LIST',
        dest='overlaps_to_hint',
        type=_split_comma_sequence,
        default=[],
        help='comma-separated sequence of glyphs to be corrected for '
             'overlap\nSame convention as --glyphs flag'
    )
    overlap_parser.add_argument(
        '--overlaps-file',
        metavar='PATH',
        dest='overlaps_to_hint_file',
        type=_validate_path,
        help='file containing a list of glyphs to be corrected for overlap\n'
             'The file must contain a comma-separated sequence of glyph '
             'identifiers.'
    )
    overlap_parser.add_argument(
        '--force-overlap',
        action='store_true',
        help='Correct for potential overlap on all glyphs (the default when '
             'using -r or hinting a variable font and not supplying an '
             'overlap list)'
    )
    overlap_parser.add_argument(
        '--force-no-overlap',
        action='store_true',
        help='Do not correct for potential overlap on any glyphs (the '
             'default when hinting individual, non-variable fonts and not '
             'supplying an overlap list)'
    )
    parser.add_argument(
        '--log',
        metavar='PATH',
        dest='log_path',
        type=_check_save_path,
        help='write output messages to a file'
    )
    parser.add_argument(
        '-p',
        '--processes',
        type=int,
        help="The number of glyph-%sing processes (default is " % term +
             "os.cpu_count(), which is typically the number of CPU cores "
             "in the computer. If negative the number will be subtracted "
             "from the core count (with a minimum result of 1)"
    )
    parser.add_argument(
        '--version',
        action='version',
        version=__version__
    )
    parser.add_argument(
        '--traceback',
        action='store_true',
        help="show traceback for exceptions.",
    )
    return parser_fonts


def handle_glyph_lists(options, parsed_args):
    if parsed_args.glyphs_to_hint:
        options.explicitGlyphs = True
        options.glyphList = _process_glyph_list_arg(
            parsed_args.glyphs_to_hint, options.nameAliases)
    elif parsed_args.glyphs_to_not_hint:
        options.excludeGlyphList = True
        options.glyphList = _process_glyph_list_arg(
            parsed_args.glyphs_to_not_hint, options.nameAliases)
    elif parsed_args.glyphs_to_hint_file:
        options.explicitGlyphs = True
        options.glyphList = _process_glyph_list_arg(
            _read_txt_file(parsed_args.glyphs_to_hint_file),
            options.nameAliases)
    elif parsed_args.glyphs_to_not_hint_file:
        options.excludeGlyphList = True
        options.glyphList = _process_glyph_list_arg(
            _read_txt_file(parsed_args.glyphs_to_not_hint_file),
            options.nameAliases)

    if parsed_args.overlaps_to_hint:
        options.overlapList = _process_glyph_list_arg(
            parsed_args.overlaps_to_hint, options.nameAliases)
    elif parsed_args.overlaps_to_hint_file:
        options.overlapList = _process_glyph_list_arg(
            _read_txt_file(parsed_args.overlaps_to_hint_file),
            options.nameAliases)


def get_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=_CustomHelpFormatter,
        description=__doc__
    )
    parser_fonts = add_common_options(parser, 'hint')
    parser.add_argument(
        '-o',
        '--output',
        metavar='PATH',
        nargs='+',
        dest='output_paths',
        type=_check_save_path,
        help='specify a file to write the hints to\n'
             'The hints are written to a new font instead of modifying the '
             'original one.'
    )
    parser.add_argument(
        '-r',
        '--reference-font',
        metavar='PATH',
        type=_validate_path,
        help='reference font\n'
             'Font to be used as reference, when hinting multiple fonts '
             'compatibily.'
    )
    parser.add_argument(
        '-b',
        '--reference-out',
        metavar='PATH',
        type=_check_save_path,
        help='reference out\n'
             'Output path for the reference font, when hinting multiple '
             'fonts compatibily.'
    )
    parser.add_argument(
        '-a',
        '--all',
        action='store_true',
        dest='hint_all_ufo',
        help='hint all glyphs\n'
             'All glyphs get hinted even if:\n'
             '- they have been hinted before, or\n'
             '- the outlines in the default layer have not changed\n'
             'This is a UFO-only option.'
    )
    parser.add_argument(
        '-w',
        '--write-to-default-layer',
        action='store_true',
        help='write hints to default layer. This is a UFO-only option.'
    )
    parser.add_argument(
        '-k',
        '--keep-conflicts',
        action='store_true',
        help='When hinting a variable font keep a stem hint even when its '
             'order inverts compared with another stem.'
    )
    report_parser = parser.add_mutually_exclusive_group()
    report_parser.add_argument(
        '-c',
        '--allow-changes',
        action='store_true',
        help='allow changes to the glyph outlines\n'
             'Paths are reordered to reduce hint substitution, and nearly '
             'straight curves are flattened.'
    )
    report_parser.add_argument(
        '--report-only',
        action='store_true',
        help='process the font without modifying it'
    )
    parser.add_argument(
        '-d',
        '--decimal',
        action='store_true',
        help='use decimal coordinates'
    )
    parser.add_argument(
        '--no-flex',
        action='store_true',
        help='suppress generation of flex commands'
    )
    parser.add_argument(
        '--no-hint-sub',
        action='store_true',
        help='suppress hint substitution'
    )
    parser.add_argument(
        '--no-zones-stems',
        action='store_true',
        help='allow the font to have no alignment zones nor stem widths'
    )
    parser.add_argument(
        '--fontinfo-file',
        metavar='PATH',
        type=_validate_path,
        help='file containing hinting parameters\n'
             f"Default: '{FONTINFO_FILE_NAME}'"
    )
    parser.add_argument(
        '--ignore-fontinfo',
        action='store_true',
        help='Ignore fontinfo files in the same directory as the font'
    )
    fddict_parser = parser.add_mutually_exclusive_group()
    fddict_parser.add_argument(
        '--print-list-fddict',
        action='store_true',
        help='print the list of private dictionaries in the single font, '
             'reference font, or default instance, whether defined in the '
             'font or using fontinfo files, and the glyphs associated with '
             'each'
    )
    fddict_parser.add_argument(
        '--print-all-fddict',
        action='store_true',
        help='print the list of private dictionaries for all fonts or '
             'instances, whether defined in the font or using fontinfo '
             'files, and the glyphs associated with each'
    )
    parser.add_argument(
        '--doc-fontinfo',
        action=_AdditionalHelpAction,
        help="show a description of the format for defining sets of "
             "alternate alignment zones in a 'fontinfo' file and exit",
        addl_help=FDDICT_DOC
    )
    parser.add_argument(
        '--info',
        action=_AdditionalHelpAction,
        help="show program's general info and exit",
        addl_help=GENERAL_INFO
    )
    parser.add_argument(
        '--max-segments',
        type=int,
        default=100,
        help="Don't hint glyph in dimension when more than this number "
             "of segments are generated"
    )
    parser.add_argument(
        '--test',
        action='store_true',
        help="toggle settings needed for testing",
    )

    parsed_args = parser.parse_args(args)

    logging_conf(parsed_args.verbose, parsed_args.log_path)

    if (not len(parsed_args.font_paths or []) and
            len(parsed_args.output_paths or [])):
        # allow "otfautohint -o outputpath inputpath"
        # see https://github.com/adobe-type-tools/psautohint/issues/129
        half = len(parsed_args.output_paths) // 2
        parsed_args.font_paths = [
            parsed_args.output_paths.pop(half) for _ in range(half)]

    if not len(parsed_args.font_paths):
        parser.error(
            f"the following arguments are required: {parser_fonts.metavar}")

    if (parsed_args.output_paths and
            len(parsed_args.font_paths) != len(parsed_args.output_paths)):
        parser.error("number of input and output fonts differ")

    if parsed_args.reference_font in parsed_args.font_paths:
        parser.error("the reference font cannot also be a font input")

    if len(parsed_args.font_paths) != len(set(parsed_args.font_paths)):
        parser.error("found duplicate font inputs")

    if parsed_args.reference_font:
        all_font_paths = parsed_args.font_paths + [parsed_args.reference_font]
    else:
        all_font_paths = parsed_args.font_paths

    options = HintOptions(parsed_args)

    options.font_format = _validate_font_paths(all_font_paths, parser)

    handle_glyph_lists(options, parsed_args)

    if options.ignoreFontinfo:
        pass
    elif not parsed_args.fontinfo_file:
        fontinfo_path = os.path.join(os.path.dirname(all_font_paths[0]),
                                     FONTINFO_FILE_NAME)
        if os.path.isfile(fontinfo_path):
            _parse_fontinfo_file(options, fontinfo_path)
    else:
        options.fontinfoPath = parsed_args.fontinfo_file
        _parse_fontinfo_file(options, parsed_args.fontinfo_file)

    return options, parsed_args


def main(args=None):
    options, pargs = get_options(args)

    try:
        hintFiles(options)
    except Exception as ex:
        if pargs.traceback:
            raise
        logging.error(ex)
        return 1


class ReportOptions(ACOptions):
    def __init__(self, pargs):
        super(ReportOptions, self).__init__()
        self.hintAll = True
        self.noFlex = True
        self.allow_no_blues = True
        self.logOnly = True
        self.inputPaths = pargs.font_paths
        self.outputPaths = pargs.output_paths
        self.report_stems = not pargs.alignment_zones
        self.report_zones = pargs.alignment_zones
        self.report_all_stems = pargs.all_stems
        self.verbose = pargs.verbose
        if pargs.force_overlap:
            self.overlapForcing = True
        elif pargs.force_no_overlap:
            self.overlapForcing = False
        if pargs.processes:
            self.process_count = pargs.processes


def get_stemhist_options(args):
    parser = argparse.ArgumentParser(
        formatter_class=_CustomHelpFormatter,
        description='Stem and Alignment zones report for PostScript, '
                    'OpenType/CFF and UFO fonts.'
    )
    add_common_options(parser, 'report')
    parser.add_argument(
        '-o',
        '--output',
        metavar='PATH',
        nargs='+',
        dest='output_paths',
        type=_check_save_path,
        help='When this is specified, the path argument is used as the base '
             'path for the reports. Otherwise, the font file path is used as '
             'the base path. Several reports are written, using the base path '
             'name plus an extension.\n'
             'Without the -z option, the report files are:\n'
             '   <base path>.hstm.txt  The horizontal stems.\n'
             '   <base path>.vstm.txt  The vertical stems.\n'
             'With the -z option, the report files are:\n'
             '   <base path>.top.txt   The top zones.\n'
             '   <base path>.bot.txt   The bottom zones.'
    )
    parser.add_argument(
        '-z',
        '--alignment-zones',
        action='store_true',
        dest='alignment_zones',
        help='Print alignment zone report rather than stem report.'
    )
    parser.add_argument(
        '-a',
        '--all',
        action='store_true',
        dest='all_stems',
        help='Include stems formed by curved line segments; by default, '
             'includes only stems formed by straight line segments.'
    )
    parsed_args = parser.parse_args(args)

    logging_conf(parsed_args.verbose, parsed_args.log_path)

    if (parsed_args.output_paths and
            len(parsed_args.font_paths) != len(parsed_args.output_paths)):
        parser.error("number of input and output fonts differ")

    if len(parsed_args.font_paths) != len(set(parsed_args.font_paths)):
        parser.error("found duplicate font inputs")

    options = ReportOptions(parsed_args)

    options.font_format = _validate_font_paths(parsed_args.font_paths, parser)

    handle_glyph_lists(options, parsed_args)

    return options, parsed_args


def stemhist(args=None):
    options, pargs = get_stemhist_options(args)

    try:
        hintFiles(options)
    except Exception as ex:
        if pargs.traceback:
            raise
        logging.error(ex)
        return 1


if __name__ == '__main__':
    sys.exit(main())
