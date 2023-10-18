# Copyright 2016, 2022 Adobe. All rights reserved.

# Methods:

# Parse args. If glyphlist is from file, read in entire file as single string,
# and remove all white space, then parse out glyph-names and GID's.

import logging
import time
from collections import defaultdict
from typing import Dict, Tuple

from . import Number

log = logging.getLogger(__name__)


class GlyphReport:
    """Class to store stem and zone data from a particular glyph"""

    def __init__(self, name: str = "", all_stems=False):
        self.name = name
        self.hstems: Dict[float, int] = {}
        self.vstems: Dict[float, int] = {}
        self.hstems_pos: set[Tuple[Number, Number]] = set()
        self.vstems_pos: set[Tuple[Number, Number]] = set()
        self.char_zones: set[Tuple[Number, Number]] = set()
        self.stem_zone_stems: set[Tuple[Number, Number]] = set()
        self.all_stems = all_stems

    def charZone(self, l: Number, u: Number):
        self.char_zones.add((l, u))

    def stemZone(self, l: Number, u: Number):
        self.stem_zone_stems.add((l, u))

    def stem(self, l: Number, u: Number, isLine: bool, isV=False):
        if not isLine and not self.all_stems:
            return
        if isV:
            stems, stems_pos = self.vstems, self.vstems_pos
        else:
            stems, stems_pos = self.hstems, self.hstems_pos
        pair = (l, u)
        if pair not in stems_pos:
            width = pair[1] - pair[0]
            stems[width] = stems.get(width, 0) + 1
            stems_pos.add(pair)


class Report:
    def __init__(self):
        self.glyphs: Dict[str, GlyphReport] = {}

    @staticmethod
    def round_value(val: float) -> int:
        if val >= 0:
            return int(val + 0.5)
        else:
            return int(val - 0.5)

    def parse_stem_dict(self, stem_dict: Dict[float, int]) -> Dict[float, int]:
        """
        stem_dict: {45.5: 1, 47.0: 2}
        """
        # key: stem width
        # value: stem count
        width_dict: Dict[float, int] = defaultdict(int)
        for width, count in stem_dict.items():
            width = self.round_value(width)
            width_dict[width] += count
        return width_dict

    def parse_zone_dicts(self, char_dict, stem_dict):
        all_zones_dict = char_dict.copy()
        all_zones_dict.update(stem_dict)
        # key: zone height
        # value: zone count
        top_dict: Dict[Number, int] = defaultdict(int)
        bot_dict: Dict[Number, int] = defaultdict(int)
        for bot, top in all_zones_dict:
            top = self.round_value(top)
            top_dict[top] += 1
            bot = self.round_value(bot)
            bot_dict[bot] += 1
        return top_dict, bot_dict

    def assemble_rep_list(self, items_dict, count_dict):
        # item 0: stem/zone count
        # item 1: stem width/zone height
        # item 2: list of glyph names
        gorder = list(self.glyphs.keys())
        rep_list = []
        for item in items_dict:
            gnames = list(items_dict[item])
            # sort the names by the font's glyph order
            if len(gnames) > 1:
                gindexes = [gorder.index(gname) for gname in gnames]
                gnames = [x for _, x in sorted(zip(gindexes, gnames))]
            rep_list.append((count_dict[item], item, gnames))
        return rep_list

    def _get_lists(self, options):
        """
        self.glyphs is a dictionary:
            key: glyph name
            value: Reports.Glyph object
        """
        if not (options.report_stems or options.report_zones):
            return [], [], [], []

        h_stem_items_dict: Dict[Number, set[str]] = defaultdict(set)
        h_stem_count_dict: Dict[Number, int] = defaultdict(int)
        v_stem_items_dict: Dict[Number, set[str]] = defaultdict(set)
        v_stem_count_dict: Dict[Number, int] = defaultdict(int)

        top_zone_items_dict: Dict[Number, set[str]] = defaultdict(set)
        top_zone_count_dict: Dict[Number, int] = defaultdict(int)
        bot_zone_items_dict: Dict[Number, set[str]] = defaultdict(set)
        bot_zone_count_dict: Dict[Number, int] = defaultdict(int)

        for gName, gr in self.glyphs.items():
            if options.report_stems:
                glyph_h_stem_dict = self.parse_stem_dict(gr.hstems)
                glyph_v_stem_dict = self.parse_stem_dict(gr.vstems)

                for stem_width, stem_count in glyph_h_stem_dict.items():
                    h_stem_items_dict[stem_width].add(gName)
                    h_stem_count_dict[stem_width] += stem_count

                for stem_width, stem_count in glyph_v_stem_dict.items():
                    v_stem_items_dict[stem_width].add(gName)
                    v_stem_count_dict[stem_width] += stem_count

            if options.report_zones:
                tmp = self.parse_zone_dicts(gr.char_zones, gr.stem_zone_stems)
                glyph_top_zone_dict, glyph_bot_zone_dict = tmp

                for zone_height, zone_count in glyph_top_zone_dict.items():
                    top_zone_items_dict[zone_height].add(gName)
                    top_zone_count_dict[zone_height] += zone_count

                for zone_height, zone_count in glyph_bot_zone_dict.items():
                    bot_zone_items_dict[zone_height].add(gName)
                    bot_zone_count_dict[zone_height] += zone_count

        # item 0: stem count
        # item 1: stem width
        # item 2: list of glyph names
        h_stem_list = self.assemble_rep_list(h_stem_items_dict,
                                             h_stem_count_dict)

        v_stem_list = self.assemble_rep_list(v_stem_items_dict,
                                             v_stem_count_dict)

        # item 0: zone count
        # item 1: zone height
        # item 2: list of glyph names
        top_zone_list = self.assemble_rep_list(top_zone_items_dict,
                                               top_zone_count_dict)

        bot_zone_list = self.assemble_rep_list(bot_zone_items_dict,
                                               bot_zone_count_dict)

        return h_stem_list, v_stem_list, top_zone_list, bot_zone_list

    @staticmethod
    def _sort_count(t):
        """
        sort by: count (1st item), value (2nd item), list of glyph names (3rd
        item)
        """
        return (-t[0], -t[1], t[2])

    @staticmethod
    def _sort_val(t):
        """
        sort by: value (2nd item), count (1st item), list of glyph names (3rd
        item)
        """
        return (t[1], -t[0], t[2])

    @staticmethod
    def _sort_val_reversed(t):
        """
        sort by: value (2nd item), count (1st item), list of glyph names (3rd
        item)
        """
        return (-t[1], -t[0], t[2])

    def save(self, path, options):
        h_stems, v_stems, top_zones, bot_zones = self._get_lists(options)
        items = (
            [h_stems, self._sort_count],
            [v_stems, self._sort_count],
            [top_zones, self._sort_val_reversed],
            [bot_zones, self._sort_val],
        )
        atime = time.asctime()
        suffixes = (".hstm.txt", ".vstm.txt", ".top.txt", ".bot.txt")
        titles = (
            "Horizontal Stem List for %s on %s\n" % (path, atime),
            "Vertical Stem List for %s on %s\n" % (path, atime),
            "Top Zone List for %s on %s\n" % (path, atime),
            "Bottom Zone List for %s on %s\n" % (path, atime),
        )
        headers = ["count    width    glyphs\n"] * 2 + [
            "count   height    glyphs\n"
        ] * 2

        for i, item in enumerate(items):
            reps, sortFunc = item
            if not reps:
                continue
            fName = f"{path}{suffixes[i]}"
            title = titles[i]
            header = headers[i]
            with open(fName, "w") as fp:
                fp.write(title)
                fp.write(header)
                reps.sort(key=sortFunc)
                for rep in reps:
                    gnames = " ".join(rep[2])
                    fp.write(f"{rep[0]:5}    {rep[1]:5}    [{gnames}]\n")
                log.info("Wrote %s" % fName)
