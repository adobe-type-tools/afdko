# Copyright 2021 Adobe. All rights reserved.

"""
Storage for intermediate state built by the hinter object
"""

import weakref
import logging
import bisect
from enum import IntEnum


from . import Number
from .glyphData import feq, pathElement, stem
from _weakref import ReferenceType
from typing import Any, Dict, List, Optional, Set, Tuple, Type, Self, Protocol


log: logging.Logger = logging.getLogger(__name__)


class AHinter(Protocol):
    def inBand(self, loc, isBottom=False) -> bool:
        ...


class hintSegment:
    """Represents a hint "segment" (one side of a potential stem)"""

    class sType(IntEnum):
        LINE = 1
        BEND = 2
        CURVE = 3
        LSBBOX = 4  # Calculated from the lower bound of a segment
        USBBOX = 5  # Calculated from the upper bound of a segment
        LGBBOX = 6  # Calculated from the lower bound of the whole glyph
        UGBBOX = 7  # Calcualted from the upper bound of the whole glyph
        GHOST = 8

    pe: Optional[ReferenceType[pathElement]]


    def __init__(self, aloc: float, oMin: float, oMax: float,
                 pe: pathElement, typ, bonus, isV,
                 isInc, desc) -> None:
        """
        Initializes the object

        self.loc is the segment location in the aligned dimension
        (x for horizontal, y for vertical)

        self.min and self.max are the extent of the segment in the
        opposite dimension

        self.bonus is 0 for normal segments and more for "special"
        segments (e.g. those within a BlueValue)

        self.type indicates the source of the segment (with GHOST
        being specially important)

        self.isV records the dimension (False for horizontal, etc.)

        self.desc is a string with a more detailed indication of
        how the segment was derived)

        self.pe is a weak reference to the pathElement (spline) from
        which the segment was derived

        self.hintval, self.replacedBy, and self.deleted are state used
        by the hinter
        """
        self.loc = aloc
        self.min = oMin
        self.max = oMax
        self.bonus = bonus
        self.type = typ
        self.isV = isV
        self.isInc = isInc
        self.desc = desc
        if pe:
            self.pe = weakref.ref(pe)
        else:
            self.pe = None
        self.hintval: Optional[stemValue] = None
        self.replacedBy: Optional[Self] = None
        self.deleted = False
        self.suppressed = False

    def __eq__(self, other) -> bool:
        return feq(self.loc, other.loc)

    def __lt__(self, other) -> bool:
        return self.loc < other.loc

    def isBend(self) -> bool:
        return self.type == self.sType.BEND

    def isCurve(self) -> bool:
        return self.type == self.sType.CURVE

    def isLine(self) -> bool:
        return self.type == self.sType.LINE or self.isBBox()

    def isBBox(self) -> bool:
        return self.type in (self.sType.LSBBOX, self.sType.USBBOX,
                             self.sType.LGBBOX, self.sType.UGBBOX)

    def isGBBox(self) -> bool:
        return self.type in (self.sType.LGBBOX, self.sType.UGBBOX)

    def isUBBox(self) -> bool:
        return self.type in (self.sType.USBBOX, self.sType.UGBBOX)

    def isGhost(self) -> bool:
        return self.type == self.sType.GHOST

    def current(self, orig=None) -> Self:
        """
        Returns the object corresponding to this object relative to
        self.replacedBy
        """
        if self.replacedBy is None:
            return self
        if self is orig:
            log.error("Cycle in hint segment replacement")
            return self
        if orig is None:
            orig = self
        return self.replacedBy.current(orig=self)

    def show(self, label) -> None:
        """Logs a debug message about the segment"""
        if self.isV:
            pp = (label, 'v', self.loc, self.min, self.loc, self.max,
                  self.desc)
        else:
            pp = (label, 'h', self.min, self.loc, self.max, self.loc,
                  self.desc)
        log.debug("%s %sseg %g %g to %g %g %s" % pp)


HintSegListWithType = Tuple[str, list[hintSegment]]


class stemValue:
    """Represents a potential hint stem"""

    def __init__(
        self,
        lloc: Number,
        uloc: Number,
        val: Number,
        spc,
        lseg: hintSegment,
        useg: hintSegment,
        isGhost=False,
    ) -> None:
        assert lloc <= uloc
        self.val = val
        self.spc = spc
        self.lloc = lloc
        self.uloc = uloc
        self.isGhost = isGhost
        self.pruned = False
        self.merge = False
        self.lseg = lseg
        self.useg = useg
        self.best = None
        self.initialVal = val
        self.idx: Optional[int] = None

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, stemValue):
            return False
        slloc, suloc = self.ghosted()
        olloc, ouloc = other.ghosted()
        return slloc == olloc and suloc == ouloc

    def __lt__(self, other: Self) -> bool:
        """Orders values by lower and then upper location"""
        slloc, suloc = self.ghosted()
        olloc, ouloc = other.ghosted()
        return (slloc < olloc or (slloc == olloc and suloc < ouloc))

    # c.f. page 22 of Adobe TN #5177 "The Type 2 Charstring Format"
    def ghosted(self) -> Tuple[Number, Number]:
        """Return the stem range but with ghost stems normalized"""
        lloc, uloc = self.lloc, self.uloc
        if self.isGhost:
            if self.lseg.type == hintSegment.sType.GHOST:
                lloc = uloc
                uloc = lloc - 20
            else:
                uloc = lloc
                lloc = uloc + 21
        return (lloc, uloc)

    def compVal(self, spcFactor=1, ghostFactor=1) -> Tuple[Any, Any]:
        """Represent self.val and self.spc as a comparable 2-tuple"""
        v = self.val
        if self.isGhost:
            v *= ghostFactor
        if self.spc > 0:
            v *= spcFactor
        return (v, self.initialVal)

    def show(self, isV, typ) -> None:
        """Add a log message with the content of the object"""
        tags = ('v', 'l', 'r', 'b', 't') if isV else ('h', 'b', 't', 'l', 'r')
        start = "%s %sval %s %g %s %g v %g s %g %s" % (typ, tags[0], tags[1],
                                                       self.lloc, tags[2],
                                                       self.uloc, self.val,
                                                       self.spc,
                                                       'G' if self.isGhost
                                                       else '')
        if self.lseg is None:
            log.debug(start)
            return
        log.debug("%s %s1 %g %s1 %g  %s2 %g %s2 %g" %
                  (start, tags[3], self.lseg.min, tags[4], self.lseg.max,
                   tags[3], self.useg.min, tags[4], self.useg.max))


class pathElementHintState:
    """Stores the intermediate hint state of a pathElement"""

    def __init__(self) -> None:
        self.s_segs: List[hintSegment] = []
        self.m_segs: List[hintSegment] = []
        self.e_segs: List[hintSegment] = []
        self.mask: List[bool] = []

    def cleanup(self) -> None:
        """Updates and deletes segments according to deleted and replacedBy"""
        for l in (self.s_segs, self.m_segs, self.e_segs):
            l[:] = [x for x in (s.current() for s in l) if not x.deleted]

    def pruneHintSegs(self) -> None:
        """Deletes segments with no assigned hintval"""
        for l in (self.s_segs, self.m_segs, self.e_segs):
            l[:] = [s for s in l if s.hintval is not None]

    def segments(self) -> list:
        return [s for s in self.s_segs + self.m_segs + self.e_segs]

    def segLists(
        self, first=None
    ) -> Tuple[HintSegListWithType, HintSegListWithType, HintSegListWithType]:
        if first is None or first == 's':
            return (('s', self.s_segs), ('m', self.m_segs), ('e', self.e_segs))
        elif first == 'm':
            return (('m', self.m_segs), ('s', self.s_segs), ('e', self.e_segs))
        elif first == 'e':
            return (('e', self.e_segs), ('m', self.m_segs), ('s', self.s_segs))
        else:
            raise ValueError('First must be s, m, or e')


class glyphHintState:
    """
    Stores the intermediate hint state (for one dimension) of a glyphData
    object

    peStates: A hash of pathElementHintState objects with the pathElement as
              key
    increasingSegs: Segments with endpoints (in the opposite dimension) greater
                    than their start points
    decreasingSegs: Segments with endpoints (in the opposite dimension) less
                    than their start points
    stemValues: List of stemValue objects in increasing order of position
    mainValues: List of non-overlapping "main" stemValues in decreasing order
                of value
    rejectValues: The set of stemValues - mainValues
    counterHinted: True if the glyph is counter hinted in this dimension
    stems: stemValue stems represented in glyphData 'stem' object format
    weights: weights corresponding to 'stems', for resolving conflicts
    keepHints: If true, keep already defined hints and masks in this dimension
               (XXX only partially implemented)
    hasOverlaps: True when some stemValues overlap
    stemOverlaps: 2d boolean array. Stem n "overlaps" with stem m <=>
                  stemOverlaps[n][m] == True
    ghostCompat: if stem m is a ghost stem, ghostCompat[m] is a boolean
                 array where ghostCompat[m][n] is True <=> n can substitute
                 for m (n has the same location on the relevant side)
    mainMask: glyphData hintmask-like representation of mainValues
    """

    def __init__(self) -> None:
        self.peStates = {}
        self.overlapRemoved = None
        self.increasingSegs: List[hintSegment] = []
        self.decreasingSegs: List[hintSegment] = []
        self.stemValues: List[stemValue] = []
        self.mainValues: List[stemValue] = []
        self.rejectValues: List[stemValue] = []
        self.counterHinted = False
        self.stems: Optional[List[stem]] = None  # in sorted glyphData format
        self.weights = None
        self.keepHints = None
        self.hasOverlaps: bool = False
        self.stemOverlaps: List[List[bool]] = []
        self.ghostCompat: List[Optional[List[bool]]] = []
        self.goodMask: List[bool] = []
        self.mainMask: List[bool] = []

    def getPEState(self, pe, make=False) -> Optional[pathElementHintState]:
        """
        Returns the pathElementHintState object for pe, allocating the object
        if necessary
        """
        s = self.peStates.get(pe, None)
        if s:
            return s
        if make:
            return self.getPEStateForced(pe)
        else:
            return None

    def getPEStateForced(self, pe) -> pathElementHintState:
        """
        Returns the pathElementHintState object for pe, allocating the object
        if necessary
        """
        return self.peStates.setdefault(pe, pathElementHintState())

    def addSegment(
        self,
        fr,
        to,
        loc,
        pe1: Optional[pathElement],
        pe2: Optional[pathElement],
        typ: hintSegment.sType,
        bonus,
        isV: bool,
        mid1,
        mid2,
        desc: str,
    ) -> None:
        """Adds a new segment associated with pathElements pe1 and pe2"""
        if isV:
            pp = ("v", loc, fr, loc, to, desc)
        else:
            pp = ("h", fr, loc, to, loc, desc)
        log.debug("add %sseg %g %g to %g %g %s" % pp)
        if fr > to:
            mn, mx = to, fr
            isInc = False
            lst = self.decreasingSegs
        else:
            mn, mx = fr, to
            isInc = True
            lst = self.increasingSegs

        aSeg = pe2 or pe1
        assert aSeg
        s = hintSegment(loc, mn, mx, aSeg, typ, bonus, isV, isInc, desc)
        # Segments derived from the first point in a path c are typically
        # also added to the previous spline p, with p passed as pe1 and c
        # passed as pe2. Segments derived from the last point in a path
        # are only added to that path, passed as pe1. pe1 is therefore the
        # "end point position" and pe2 the "start point position" except in
        # the special case of MCURVE, which indicates the point was derived
        # from the middle of the spline.
        if pe1:
            if mid1:
                self.getPEStateForced(pe1).m_segs.append(s)
            else:
                self.getPEStateForced(pe1).e_segs.append(s)
        if pe2:
            if mid2:
                self.getPEStateForced(pe2).m_segs.append(s)
            else:
                self.getPEStateForced(pe2).s_segs.append(s)

        bisect.insort(lst, s)

    def compactList(self, l: List[hintSegment]) -> None:
        """
        Compacts overlapping segments with the same location by picking
        one segment to represent the pair, adjusting its values, and
        removing the other segment
        """
        i = 0
        while i < len(l):
            j = i + 1
            while j < len(l) and feq(l[j].loc, l[i].loc):
                if l[i].max >= l[j].min and l[i].min <= l[j].max:
                    if abs(l[i].max - l[i].min) > abs(l[j].max - l[j].min):
                        l[i].min = min(l[i].min, l[j].min)
                        l[i].max = max(l[i].max, l[j].max)
                        l[i].bonus = max(l[i].bonus, l[j].bonus)
                        l[j].replacedBy = l[i]
                        del l[j]
                        continue
                    else:
                        l[j].min = min(l[i].min, l[j].min)
                        l[j].max = max(l[i].max, l[j].max)
                        l[j].bonus = max(l[i].bonus, l[j].bonus)
                        l[i].replacedBy = l[j]
                        del l[i]
                        i -= 1
                        break
                j += 1
            i += 1

    def compactLists(self) -> None:
        """Compacts both segment lists"""
        self.compactList(self.decreasingSegs)
        self.compactList(self.increasingSegs)

    def remExtraBends(self) -> None:
        """
        Delete BEND segment x when there is another segment y:
           1. At the same location
           2. but in the other direction
           2. that is not of type BEND or GHOST and
           3. that overlaps with x and
           4. is at least three times longer
        """
        for hsi in self.increasingSegs:
            lo = bisect.bisect_left(self.decreasingSegs, hsi)
            hi = bisect.bisect_right(self.decreasingSegs, hsi, lo=lo)
            for hsd in self.decreasingSegs[lo:hi]:
                assert hsd.loc == hsi.loc
                if hsd.min < hsi.max and hsd.max > hsi.min:
                    if (
                        hsi.type == hintSegment.sType.BEND
                        and hsd.type != hintSegment.sType.BEND
                        and hsd.type != hintSegment.sType.GHOST
                        and (hsd.max - hsd.min) > (hsi.max - hsi.min) * 3
                    ):
                        hsi.deleted = True
                        log.debug(
                            "rem seg loc %g from %g to %g" % (hsi.loc, hsi.min, hsi.max)
                        )
                        break
                    elif (
                        hsd.type == hintSegment.sType.BEND
                        and hsi.type != hintSegment.sType.BEND
                        and hsi.type != hintSegment.sType.GHOST
                        and (hsi.max - hsi.min) > (hsd.max - hsd.min) * 3
                    ):
                        hsd.deleted = True
                        log.debug(
                            "rem seg loc %g from %g to %g" % (hsd.loc, hsd.min, hsd.max)
                        )

    def deleteSegments(self) -> None:
        for s in self.increasingSegs:
            s.deleted = True
        for s in self.decreasingSegs:
            s.deleted = True
        self.increasingSegs = []
        self.decreasingSegs = []
        self.cleanup()

    def cleanup(self) -> None:
        """Runs cleanup on all pathElementHintState objects"""
        self.increasingSegs = [s for s in self.increasingSegs if not s.deleted]
        self.decreasingSegs = [s for s in self.decreasingSegs if not s.deleted]
        for pes in self.peStates.values():
            pes.cleanup()

    def pruneHintSegs(self) -> None:
        """Runs pruneHintSegs on all pathElementHintState objects"""
        for pes in self.peStates.values():
            pes.pruneHintSegs()


class stemLocCandidate:
    strongMultiplier: float = 1.2
    bandMultiplier: float = 2.0
    """
    Information about a candidate location for a stem in a region
    glyph, derived from segments generated for the glyph or,
    occasionally, directly from point locations.
    """

    def __init__(self, loc) -> None:
        self.loc = loc
        self.strong: float = 0
        self.weak: float = 0

    def addScore(self, score: float, strong: bool) -> None:
        if strong:
            self.strong += score
        else:
            self.weak += score

    def weight(self, inBand: bool) -> float:
        return (self.strong * self.strongMultiplier + self.weak) * (
            self.bandMultiplier if inBand else 1
        )

    def isStrong(self) -> bool:
        return self.strong > 0

    def isMixed(self) -> bool:
        return self.strong > 0 and self.weak > 0

    def __eq__(self, other) -> bool:
        return feq(self.strong, other.strong) and feq(self.weak, other.weak)

    def __lt__(self, other) -> bool:
        return self.strong < other.strong or (
            feq(self.strong, other.strong) and self.weak < other.weak
        )


class instanceStemState:
    """
    State for the process of deciding on the lower and upper locations
    for a particular region stem.
    """

    def __init__(self, loc: float, dhinter: AHinter) -> None:
        self.defaultLoc = loc
        self.dhinter = dhinter
        self.candDict: Dict[float, stemLocCandidate] = {}
        self.usedSegs: Set[int] = set()
        self.bb = None

    def addToLoc(self, loc: float, score: float, strong=False,
                 bb=False, seg: Any = None) -> None:
        if self.bb is not None:
            if self.bb != bb:
                log.info("Mixed bounding-box and "
                         "non-bounding box location data")
        else:
            self.bb = bb
        if seg is not None:
            sid = id(seg)
            if sid in self.usedSegs:
                return
            self.usedSegs.add(sid)
        if loc in self.candDict:
            sLC = self.candDict[loc]
        else:
            sLC = self.candDict[loc] = stemLocCandidate(loc)
        sLC.addScore(score, strong)

    def bestLocation(self, isBottom: bool) -> Any:
        weights = [(x.weight(self.dhinter.inBand(x.loc, isBottom)), x.loc)
                   for x in self.candDict.values()]
        try:
            m = max(weights)
#            try:
#                next_m = m
#                while abs(next_m[1] - m[1]) < 3.01:
#                    weights.remove(next_m)
#                    next_m = max(weights)
#                if m[0] < next_m[0] * 2:
#                    log.warning("Stem location competition: "
#                                "weight %s vs %s" % (m, next_m))
#            except:
#                pass
            return m[1]
        except ValueError:
            return None


class links:
    """
    Tracks which subpaths need which stem hints and calculates a subpath
    to reduce hint substitution

    cnt: The number of subpaths
    links: A cnt x cnt array of integers modified by mark
           (Values only 0 or 1 but kept as ints for later arithmetic)
    """

    def __init__(self, glyph) -> None:
        l = len(glyph.subpaths)
        if l < 4 or l > 100:
            self.cnt = 0
            return
        self.cnt = l
        self.links = [[0] * l for i in range(l)]

    def logLinks(self) -> None:
        """Prints a log message representing links"""
        if self.cnt == 0:
            return
        log.debug("Links")
        log.debug(" ".join((str(i).rjust(2) for i in range(self.cnt))))
        for j in range(self.cnt):
            log.debug(
                " ".join(
                    (
                        ("Y" if self.links[j][i] else " ").rjust(2)
                        for i in range(self.cnt)
                    )
                )
            )

    def logShort(self, shrt, lab) -> None:
        """Prints a log message representing (1-d) shrt"""
        log.debug(lab)
        log.debug(" ".join((str(i).rjust(2) for i in range(self.cnt))))
        log.debug(
            " ".join(
                ((str(shrt[i]) if shrt[i] else " ").rjust(2) for i in range(self.cnt))
            )
        )

    def mark(self, stemValues: List[stemValue], isV) -> None:
        """
        For each stemValue in hntr, set links[m][n] and links[n][m] to 1
        if one side of a stem is in m and the other is in n
        """
        if self.cnt == 0:
            return
        for sv in stemValues:
            if not sv.lseg or not sv.useg or not sv.lseg.pe or not sv.useg.pe:
                continue
            lseg_pe = sv.lseg.pe()
            useg_pe = sv.useg.pe()
            assert lseg_pe and useg_pe
            lsubp, usubp = lseg_pe.position[0], useg_pe.position[0]
            if lsubp == usubp:
                continue
            sv.show(isV, "mark")
            log.debug(" : %d <=> %d" % (lsubp, usubp))
            self.links[lsubp][usubp] = 1
            self.links[usubp][lsubp] = 1

    def moveIdx(
        self, suborder: List[int], subidxs: List[int], outlinks, idx: int
    ) -> None:
        """
        Move value idx from subidxs to the end of suborder and update
        outlinks to record all links shared with idx
        """
        subidxs.remove(idx)
        suborder.append(idx)
        for i in range(len(outlinks)):
            outlinks[i] += self.links[idx][i]
        self.logShort(outlinks, "Outlinks")

    def shuffle(self) -> Optional[List[int]]:
        """
        Returns suborder list with all subpath indexes in decreasing
        order of links shared with previous subpath. (The first subpath
        being the one with most links overall.)
        """
        if self.cnt == 0:
            return None
        sumlinks = [sum(l) for l in zip(*self.links)]
        outlinks = [0] * self.cnt
        self.logLinks()
        self.logShort(sumlinks, "Sumlinks")
        subidxs = list(range(self.cnt))
        suborder: List[int] = []
        while subidxs:
            # negate s to preserve all-links-equal subpath ordering
            _, bst = max(((sumlinks[s], -s) for s in subidxs))
            self.moveIdx(suborder, subidxs, outlinks, -bst)
            while True:
                try:
                    _, _, bst = max(((outlinks[s], sumlinks[s], -s)
                                     for s in subidxs if outlinks[s] > 0))
                except ValueError:
                    break
                self.moveIdx(suborder, subidxs, outlinks, -bst)
        return suborder
