# Copyright 2021 Adobe. All rights reserved.

"""
Associates segments with points on a pathElement, then builds
these into candidate stem pairs that are evaluated and pruned
to an optimal set.
"""

import logging
import bisect
import math
from copy import copy, deepcopy
from abc import abstractmethod, ABC
from collections import namedtuple

from fontTools.misc.bezierTools import solveCubic

from .glyphData import pt, feq, fne, stem
from .hintstate import (hintSegment, stemValue, glyphHintState, links,
                        instanceStemState)
from .overlap import removeOverlap
from .report import GlyphReport
from .logging import logging_reconfig, set_log_parameters

log = logging.getLogger(__name__)

GlyphPE = namedtuple("GlyphPE", "glyph pe")
LocDict = namedtuple("LocDict", "l u used")

# A few variable conventions not documented elsewhere:

# When used in a path-related context "c" is a variable containing a
# path element (in the way that "i" is often a variable containing an
# integer. In loops it will in a sense, be the "current" path element.
# In lower-level functions it may just be the parameter containing the
# element to operate on.

# When "n" or "p" are used in a similar contexts they will typically
# respectively contain the next or previous path elements relative to c.

# "cl" generally refers to a list of (glyph, pathElement) tuples, where
# each pathElement corresponds to the location of a corresponding line or
# curve in master or VF location indicated by the glyph.

# In a context with multiple path elements but without a clear relation
# between them, pe1, pe2, and so forth will be used. p1, p2, etc. will
# similarly contain points, and cp1, cp2 specifically control points.

# Analogously "gl" is a variable containing a glyphData object.

# "t" is a generic temporary variable, typically used when deciding
# whether to replace one value with another. It should be short-lived.


class dimensionHinter(ABC):
    """
    Common hinting implementation inherited by vertical and horizontal
    variants
    """
    @staticmethod
    def diffSign(a, b):
        return fne(a, 0) and fne(b, 0) and ((a > 0) != (b > 0))

    @staticmethod
    def sameSign(a, b):
        return fne(a, 0) and fne(b, 0) and ((a > 0) == (b > 0))

    def __init__(self, options):
        self.StemLimit = 22  # ((kStackLimit) - 2) / 2), kStackLimit == 46
        """Initialize constant values and miscelaneous state"""
        self.MaxStemDist = 150  # initial maximum stem width allowed for hints
        self.InitBigDist = self.MaxStemDist
        self.BigDistFactor = 23.0 / 20.0
        # MinDist must be <= 168 for ITC Garamond Book It p, q, thorn
        self.MinDist = 7
        self.GhostWidth = 20
        self.GhostLength = 4
        self.GhostVal = 20
        self.GhostSpecial = 2
        self.BendLength = 2
        self.BendTangent = 0.55735  # 30 sin 30 cos div abs == .57735
        self.Theta = 0.38  # Must be <= .38 for Ryumin-Light 32
        self.MinValue = 0.0039
        self.MaxValue = 8000000.0
        self.PruneA = 50
        # PruneB and PruneValue must be <= 0.01 for Yakout/Light/heM
        self.PruneB = 0.0117187
        self.PruneC = 100
        self.PruneD = 1
        self.PruneValue = 0.0117187
        self.PruneFactor = 3.0
        self.PruneFactorLt = 10.0 / 3.0
        self.PruneDistance = 10
        self.MuchPF = 50
        self.VeryMuchPF = 100
        self.CPfrac = 0.4
        self.ConflictValMin = 50.0
        self.ConflictValMult = 20.0
        # BandMargin must be < 46 for Americana-Bold d bowl vs stem hinting
        self.BandMargin = 30
        self.MaxFlare = 10
        self.MaxBendMerge = 6
        self.MinHintElementLength = 12
        self.AutoLinearCurveFix = True
        self.MaxFlex = 20
        self.FlexLengthRatioCutoff = 0.11  # .33^2 (ratio of 1:3 or better)
        self.FlexCand = 4
        self.SCurveTangent = 0.025
        self.CPfrac = 0.4
        self.OppoFlatMax = 4
        self.FlatMin = 50
        self.ExtremaDist = 20
        self.NearFuzz = 6
        self.NoOverlapPenalty = 7.0 / 5.0
        # XXX GapDistDenom might have been miscorrected from 40 in ebb49206
        self.GapDistDenom = 40
        self.CloseMerge = 20
        # MaxMerge must be < 3 for Cushing-BookItalic z
        self.MaxMerge = 2
        self.MinHighSegVal = 1.0 / 16.0
        self.SFactor = 20
        self.SpcBonus = 1000
        self.SpecialCharBonus = 200
        self.GhostFactor = 1.0 / 8.0
        self.DeltaDiffMin = .05
        self.DeltaDiffReport = 3
        self.NoSegScore = 0.1

        self.FlexStrict = True

        self.HasFlex = False
        self.options = options

        self.fddicts = None
        self.gllist = None
        self.glyph = None
        self.report = None
        self.name = None
        self.isMulti = False

    def setGlyph(self, fddicts, report, gllist, name, clearPrev=True):
        """Initialize the state for processing a specific glyph"""
        self.fddicts = fddicts
        self.report = report
        self.glyph = gllist[0]
        self.fddict = fddicts[0]
        self.gllist = gllist
        self.isMulti = (len(gllist) > 1)
        self.name = name
        self.HasFlex = False
        self.Bonus = None
        self.Pruning = True
        if self.isV():
            self.hs = self.glyph.vhs = glyphHintState()
        else:
            self.hs = self.glyph.hhs = glyphHintState()

    def resetForHinting(self):
        """Reset state for rehinting same glyph"""
        self.Bonus = None
        self.Pruning = True
        if self.isV():
            self.hs = self.glyph.vhs = glyphHintState()
        else:
            self.hs = self.glyph.hhs = glyphHintState()

    class glIter:
        """A pathElement set iterator for the glyphData object list"""
        __slots__ = ('gll', 'il')

        def __init__(self, gllist, glidx=None):
            if glidx is not None:
                assert isinstance(glidx, int) and glidx > 0
                self.gll = [gllist[0], gllist[glidx]]
            else:
                self.gll = gllist
            self.il = [gl.__iter__() for gl in self.gll]

        def __next__(self):
            return [GlyphPE(self.gll[i], ii.__next__())
                    for i, ii in enumerate(self.il)]

        def __iter__(self):
            return self

    def __iter__(self, glidx=None):
        return self.glIter(self.gllist, glidx)

    # Methods implemented by subclasses
    @abstractmethod
    def startFlex(self):
        pass

    @abstractmethod
    def stopFlex(self):
        pass

    @abstractmethod
    def startHint(self):
        pass

    @abstractmethod
    def stopHint(self):
        pass

    @abstractmethod
    def startStemConvert(self):
        pass

    @abstractmethod
    def stopStemConvert(self):
        pass

    @abstractmethod
    def startMaskConvert(self):
        pass

    @abstractmethod
    def stopMaskConvert(self):
        pass

    @abstractmethod
    def isV(self):
        pass

    @abstractmethod
    def segmentLists(self):
        pass

    @abstractmethod
    def dominantStems(self):
        pass

    @abstractmethod
    def isCounterGlyph(self):
        pass

    @abstractmethod
    def inBand(self, loc, isBottom=False):
        pass

    @abstractmethod
    def hasBands(self):
        pass

    @abstractmethod
    def aDesc(self):
        pass

    @abstractmethod
    def isSpecial(self, lower=False):
        pass

    @abstractmethod
    def checkTfm(self):
        pass

    # Flex
    def linearFlexOK(self):
        return False

    def addFlex(self, force=True, inited=False):
        """Path-level interface to add flex hints to current glyph"""
        self.startFlex()
        hasflex = (gl.flex_count != 0 for gl in self.gllist)
        if not inited and any(hasflex):
            if force or not all(hasflex):
                log.info("Clearing existing flex hints")
                for gl in self.gllist:
                    gl.clearFlex()
            else:
                log.info("Already has flex hints, skipping addFlex")
                return
        for cl in self:
            if all((self.tryFlex(x.glyph, x.pe) for x in cl)):
                self.markFlex(cl)
        self.stopFlex()

    def tryFlex(self, gl, c):
        """pathElement-level interface to add flex hints to current glyph"""
        # Not a curve, already flexed, or flex depth would be too large
        if not c or c.isLine() or c.flex or c.s.a_dist(c.e) > self.MaxFlex:
            return False
        n = gl.nextInSubpath(c, skipTiny=True)
        if not n or n.isLine() or n.flex:
            return False

        da, do = (c.s - n.e).abs().ao()
        # Difference in "bases" too high to report a near miss or length
        # too short (Reuse of MaxFlex in other dimension is coincidental)
        if da > self.FlexCand or do < self.MaxFlex:
            return False

        # The end-to-end distance should be at least 3 times the flex depth
        if do < 3 * c.s.avg(n.e).a_dist(c.e):
            return False

        # Start and end points should be on the same side of mid-point
        if self.diffSign(c.e.a - c.s.a, c.e.a - n.e.a):
            return False

        d1sq = c.s.distsq(c.e)
        d2sq = c.e.distsq(n.e)
        quot = d2sq / d1sq if d1sq > d2sq else d1sq / d2sq
        if quot < self.FlexLengthRatioCutoff:
            return False

        if self.FlexStrict:
            nn = gl.nextInSubpath(n, skipTiny=True)
            # Endpoint of spline after n bends back towards flex midpoint
            if not nn or self.diffSign(nn.e.a - n.e.a, c.e.a - n.e.a):
                return False
            p = gl.prevInSubpath(c, skipTiny=True)
            # Endpoint of spline before e bends back towards flex midpoint
            if not p or self.diffSign(p.s.a - c.s.a, c.e.a - c.s.a):
                return False

            # flex is not convex
            if self.isV() and (c.s.x > c.e.x) != (c.e.y < c.s.y):
                return False
            elif not self.isV() and (c.s.y > n.e.y) != (c.s.x < c.e.x):
                return False

        real_n = gl.nextInSubpath(c, skipTiny=False)
        if real_n is not n:
            log.info("Remove short spline at %g %g to add flex." % c.e)
            return False
        elif real_n.isClose() or real_n.isStart():
            log.info("Move closepath from %g %.gto add flex." % c.e)
            return False

        if fne(c.s.a, n.e.a):
            log.info("Curves from %g %g to %g %g " % (*c.s, *n.e) +
                     "near miss for adding flex")
            return False

        if (feq(n.s.a, n.cs.a) and feq(n.cs.a, n.ce.a) and
                feq(n.ce.a, n.e.a) and not self.linearFlexOK()):
            log.info("Make curves from %g %g to %g %g" % (*c.s, *n.e) +
                     "non-linear to add flex")  # XXX what if only one line?
            return False

        return True

    def markFlex(self, cl):
        for gl, c in cl:
            n = gl.nextInSubpath(c, skipTiny=False)
            c.flex = 1
            n.flex = 2
            gl.flex_count += 1
            gl.changed = True
        if not self.HasFlex:
            log.info("Added flex operators to this glyph.")
            self.HasFlex = True

    def calcHintValues(self, lnks, force=True, tryCounter=True):
        """
        Top-level method for calculating stem hints for a glyph in one
        dimension
        """
        assert self.glyph is not None
        self.startHint()
        if self.glyph.hasHints(doVert=self.isV()):
            if force:
                log.info("Clearing existing %s hints" % self.aDesc())
                self.glyph.clearHints(self.isV())
                self.glyph.changed = True
            else:
                self.keepHints = True
                log.info("Already has %s hints" % self.aDesc())
                self.stopHint()
                return
        self.keepHints = False
        self.BigDist = max(self.dominantStems() + [self.InitBigDist])
        self.BigDist *= self.BigDistFactor
        log.debug("generate %s segments" % self.aDesc())
        self.genSegs()
        self.limitSegs()
        log.debug("generate %s stem values" % self.aDesc())
        self.Pruning = not (tryCounter and self.isCounterGlyph())
        self.genStemVals()
        self.pruneStemVals()
        self.highestStemVals()
        self.mergeVals()
        self.limitVals()
        for sv in self.hs.stemValues:
            sv.show(self.isV(), "postmerge")
        log.debug("pick main %s" % self.aDesc())
        assert self.glyph is not None
        self.glyph.syncPositions()
        lnks.mark(self.hs.stemValues, self.isV())
        self.checkVals()
        self.reportStems()
        self.mainVals()
        if tryCounter and self.isCounterGlyph():
            self.Pruning = True
            self.hs.counterHinted = self.tryCounterHinting()
            if not self.hs.counterHinted:
                self.addBBox(True)
                self.hs.counterHinted = self.tryCounterHinting()
                if not self.hs.counterHinted:
                    log.info(("Glyph is in list for using %s counter hints " +
                              "but didn't find any candidates.") %
                             self.aDesc())
                    self.resetForHinting()
                    self.calcHintValues(lnks, force=True, tryCounter=False)
                    self.stopHint()
                    return
                else:
                    self.highestStemVals()  # for bbox segments
        if len(self.hs.mainValues) == 0:
            self.addBBox(False)
        log.debug("%s results" % self.aDesc())
        if self.hs.counterHinted:
            log.debug("Using %s counter hints." % self.aDesc())
        for hv in self.hs.mainValues:
            hv.show(self.isV(), "result")
        self.markStraySegs()
        self.hs.pruneHintSegs()
        self.stopHint()

    # Segments
    def handleOverlap(self):
        if self.options.overlapForcing is True:
            return True
        elif self.options.overlapForcing is False:
            return False
        elif self.name in self.options.overlapList:
            return True
        else:
            return self.isMulti

    def addSegment(self, fr, to, loc, pe1, pe2, typ, desc, mid=False):
        if pe1 is not None and isinstance(pe1.segment_sub, int):
            subpath, offset = pe1.position
            t = self.glyph.subpaths[subpath][offset]
            if pe1 != t.segment_sub[-1]:
                mid = True
            pe1 = t
        if pe2 is not None and isinstance(pe2.segment_sub, int):
            subpath, offset = pe2.position
            t = self.glyph.subpaths[subpath][offset]
            if pe2 != t.segment_sub[0]:
                mid = True
            pe2 = t
        if pe1 == pe2:
            pe2 = None
        mid1 = mid2 = mid
        if pe1 is not None and hasattr(pe1, 'association'):
            ope1 = pe1.association
            if (not mid1 and ope1 is not None and
                    not pe1.e.__eq__(ope1.e, ope1.getAssocFactor())):
                mid1 = True
            pe1 = ope1
        if pe2 is not None and hasattr(pe2, 'association'):
            ope2 = pe2.association
            if (not mid2 and ope2 is not None and
                    not pe2.e.__eq__(ope2.e, ope2.getAssocFactor())):
                mid2 = True
            pe2 = ope2

        if not pe1 and not pe2:
            return
        self.hs.addSegment(fr, to, loc, pe1, pe2, typ, self.Bonus,
                           self.isV(), mid1, mid2, desc)

    def CPFrom(self, cp2, cp3):
        """Return point cp3 adjusted relative to cp2 by CPFrac"""
        return (cp3 - cp2) * (1.0 - self.CPfrac) + cp2

    def CPTo(self, cp0, cp1):
        """Return point cp1 adjusted relative to cp0 by CPFrac"""
        return (cp1 - cp0) * self.CPfrac + cp0

    def adjustDist(self, v, q):
        return v * q

    def testTan(self, p):
        """Test angle of p (treated as vector) relative to BendTangent"""
        return abs(p.a) > (abs(p.o) * self.BendTangent)

    @staticmethod
    def interpolate(q, v0, q0, v1, q1):
        return v0 + (q - q0) * (v1 - v0) / (q1 - q0)

    def flatQuo(self, p1, p2, doOppo=False):
        """
        Returns a measure of the flatness of the line between p1 and p2

        1 means exactly flat wrt dimension a (or o if doOppo)
        0 means not interestingly flat in dimension a. (or o if doOppo)
        Intermediate values represent degrees of interesting flatness
        """
        d = (p1 - p2).abs()
        if doOppo:
            d = pt(d.y, d.x)
        if feq(d.o, 0):
            return 1
        if feq(d.a, 0):
            return 0
        q = (d.o * d.o) / (self.Theta * d.a)
        if q < 0.25:
            result = self.interpolate(q, 1, 0, 0.841, 0.25)
        elif q < .5:
            result = self.interpolate(q, 0.841, 0.25, 0.707, 0.5)
        elif q < 1:
            result = self.interpolate(q, 0.707, 0.5, 0.5, 1)
        elif q < 2:
            result = self.interpolate(q, .5, 1, 0.25, 2)
        elif q < 4:
            result = self.interpolate(q, 0.25, 2, 0, 4)
        else:
            result = 0
        return result

    def testBend(self, p0, p1, p2):
        """Test of the angle between p0-p1 and p1-p2"""
        d1 = p1 - p0
        d2 = p2 - p1
        dp = d1.dot(d2)
        q = d1.normsq() * d2.normsq()
        # Shouldn't hit this in practice
        if feq(q, 0):
            return False
        return dp * dp / q <= 0.5

    def isCCW(self, p0, p1, p2):
        """
        Returns true if p0 -> p1 -> p2 is counter-clockwise in glyph space.
        """
        d0 = p1 - p0
        d1 = p2 - p1
        return d0.x * d1.y >= d1.x * d0.y

    # Generate segments

    def relPosition(self, c, lower=False):
        """
        Return value indicates whether c is in the upper (or lower)
        subpath of the glyph (assuming a strict ordering of subpaths
        in this dimension)
        """
        for subp in self.glyph.subpaths:
            if ((lower and subp[0].s.a < c.s.a) or
                    (not lower and subp[0].s.a > c.s.a)):
                return True
        return False

    # I initially combined the two doBends but the result was more confusing
    # and difficult to debug than having them separate
    def doBendsNext(self, c):
        """
        Adds a BEND segment (short segments marking somewhat flat
        areas) at the end of a spline. In some cases the segment is
        added in both "directions"
        """
        p0, p1, p2 = c.slopePoint(1), c.e, self.glyph.nextSlopePoint(c)
        if feq(p0.o, p1.o) or p2 is None:
            return
        osame = self.diffSign(p2.o - p1.o, p1.o - p0.o)
        if osame or (self.testTan(p1 - p2) and
                     (self.diffSign(p2.a - p1.a, p1.a - p0.a) or
                      (self.flatQuo(p0, p1, doOppo=True) > 0 and
                       self.testBend(p0, p1, p2)))):
            delta = self.BendLength / 2
            doboth = False
            if p0.a <= p1.a < p2.a or p0.a < p1.a <= p2.a:
                pass
            elif p2.a < p1.a <= p0.a or p2.a <= p1.a < p0.a:
                delta = -delta
            elif osame:
                tst = p0.o > p1.o
                if self.isV():
                    delta = -delta
                if tst == self.isCCW(p0, p1, p2):
                    delta = -delta
            else:
                if self.isV():
                    delta = -delta
                doboth = True
            strt = p1.a - delta
            end = p1.a + delta
            self.addSegment(strt, end, p1.o, c, None,
                            hintSegment.sType.BEND, 'next bend forward')
            if doboth:
                self.addSegment(end, strt, p1.o, c, None,
                                hintSegment.sType.BEND, 'next bend reverse')

    def doBendsPrev(self, c):
        """
        Adds a BEND segment (short segments marking somewhat flat
        areas) at the start of a spline. In some cases the segment is
        added in both "directions"
        """
        p0, p1, p2 = c.s, c.slopePoint(0), self.glyph.prevSlopePoint(c)
        if feq(p0.o, p1.o) or p2 is None:
            return
        cs = self.glyph.prevInSubpath(c, segSub=True)
        osame = self.diffSign(p2.o - p0.o, p0.o - p1.o)
        if osame or (self.testTan(p0 - p2) and
                     (self.diffSign(p2.a - p0.a, p0.a - p1.a) or
                      (self.flatQuo(p1, p0, doOppo=True) > 0 and
                       self.testBend(p2, p0, p1)))):
            delta = self.BendLength / 2
            if p2.a <= p0.a < p1.a or p2.a < p0.a <= p1.a:
                pass
            elif p1.a < p0.a <= p2.a or p1.a <= p0.a < p2.a:
                delta = -delta
            elif osame:
                tst = p2.o < p0.o
                if self.isV():
                    delta = -delta
                if tst == self.isCCW(p2, p0, p1):
                    delta = -delta
            else:
                if self.isV():
                    delta = -delta
            strt = p0.a - delta
            end = p0.a + delta
            self.addSegment(strt, end, p0.o, cs, None, hintSegment.sType.BEND,
                            'prev bend forward')

    def nodeIsFlat(self, c, doPrev=False):
        """
        Returns true if the junction of this spline and the next
        (or previous) is sufficiently flat, measured by OppoFlatMax
        and FlatMin
        """
        if not c:
            return
        if doPrev:
            sp = self.glyph.prevSlopePoint(c)
            if sp is None:
                return False
            d = (sp - c.cs).abs()
        else:
            sp = self.glyph.nextSlopePoint(c)
            if sp is None:
                return False
            d = (sp - c.ce).abs()
        return d.o <= self.OppoFlatMax and d.a >= self.FlatMin

    def sameDir(self, c, doPrev=False):
        """
        Returns True if the next (or previous) spline continues in roughly
        the same direction as c
        """
        if not c:
            return False
        if doPrev:
            p = self.glyph.prevInSubpath(c, skipTiny=True, segSub=True)
            if p is None:
                return
            p0, p1, p2 = c.e, c.s, p.s
        else:
            n = self.glyph.nextInSubpath(c, skipTiny=True, segSub=True)
            if n is None:
                return
            p0, p1, p2 = c.s, c.e, n.e
        if (self.diffSign(p0.y - p1.y, p1.y - p2.y) or
                self.diffSign(p0.x - p1.x, p1.x - p2.x)):
            return False
        return not self.testBend(p0, p1, p2)

    def extremaSegment(self, pe, extp, extt, isMn):
        """
        Given a curved pathElement pe and a point on that spline extp at
        t == extt, calculates a segment intersecting extp where all portions
        of the segment are within ExtremaDist of pe
        """
        a, b, c, d = pe.cubicParameters()
        loc = round(extp.o) + (-self.ExtremaDist
                               if isMn else self.ExtremaDist)

        horiz = not self.isV()  # When finding vertical stems solve for x
        sl = solveCubic(a[horiz], b[horiz], c[horiz], d[horiz] - loc)
        pl = [(pt(a[0] * t * t * t + b[0] * t * t + c[0] * t + d[0],
                  a[1] * t * t * t + b[1] * t * t + c[1] * t + d[1]), t)
              for t in sl if 0 <= t <= 1]

        # Find closest solution on each side of extp
        mn_p = mx_p = None
        mn_td = mx_td = 2
        for p, t in pl:
            td = abs(t - extt)
            if t < extt and td < mn_td:
                mn_p, mn_td = p, td
            elif t > extt and td < mx_td:
                mx_p, mx_td = p, td

        # If a side isn't found the spline end on that side should be
        # within the ExtremaDist of extp.o
        if not mn_p:
            mn_p = pe.s
            assert abs(mn_p.o - extp.o) < self.ExtremaDist + 0.01
        if not mx_p:
            mx_p = pe.e
            assert abs(mx_p.o - extp.o) < self.ExtremaDist + 0.01

        return mn_p.a, mx_p.a

    def pickSpot(self, p0, p1, dist, pp0, pp1, prv, nxt):
        """
        Picks a segment location based on candidates p0 and p1 and
        other locations and metrics picked from the spline and
        the adjacent splines. Locations within BlueValue bands are
        priviledged.
        """
        inBand0 = self.inBand(p0.o, dist >= 0)
        inBand1 = self.inBand(p1.o, dist >= 0)
        if inBand0 and not inBand1:
            return p0.o
        if inBand1 and not inBand0:
            return p1.o
        if feq(p0.o, pp0.o) and fne(p1.o, pp1.o):
            return p0.o
        if fne(p0.o, pp0.o) and feq(p1.o, pp1.o):
            return p1.o
        if (prv and feq(p0.o, prv.o)) and (not nxt or fne(p1.o, nxt.o)):
            return p0.o
        if (not prv or fne(p0.o, prv.o)) and (prv and feq(p1.o, nxt.o)):
            return p1.o
        if inBand0 and inBand1:
            upper, lower = (p1.o, p0.o) if p0.o < p1.o else (p0.o, p1.o)
            return upper if dist < 0 else lower
        if abs(pp0.a - p0.a) > abs(pp1.a - p1.a):
            return p0.o
        if abs(pp1.a - p1.a) > abs(pp0.a - p0.a):
            return p1.o
        if prv and feq(p0.o, prv.o) and nxt and feq(p1.o, nxt.o):
            if abs(p0.a - prv.a) > abs(p1.a - nxt.a):
                return p0.o
            return p1.o
        return (p0.o + p1.o) / 2

    def cpDirection(self, p0, p1, p2):
        """
        Utility function for detecting singly-inflected curves.
        See original C code or "Fast Detection o the Geometric Form of
        Two-Dimensional Cubic Bezier Curves" by Stephen Vincent
        """
        det = (p0.x * (p1.y - p2.y) +
               p1.x * (p2.y - p0.y) +
               p2.x * (p0.y - p1.y))
        if det > 0:
            return 1
        elif det < 0:
            return -1
        return 0

    def prepForSegs(self):
        for c in self.glyph:
            if (not c.isLine() and
                    (self.cpDirection(c.s, c.cs, c.ce) !=
                     self.cpDirection(c.cs, c.ce, c.e))):
                if c.splitAtInflectionsForSegs():
                    log.debug("splitting at inflection point in %d %d" %
                              (c.position[0], c.position[1] + 1))

    def genSegs(self):
        """
        Calls genSegsForPathElement for each pe and cleans up the
        generated segment lists
        """
        origGlyph = None
        if self.handleOverlap():
            # To handle overlap, make a copy of the glyph without
            # overlap and associate its pathElements with those of
            # the unmodified glyph. Then set self.glyph to the modified
            # version. The segment generation code only looks at
            # self.hs in addSegment(), which has the appropriate code
            # to follow the associations and add the segments to the
            # unmodified glyph
            origGlyph = self.glyph
            self.glyph = removeOverlap(origGlyph)
            if self.glyph is None:
                self.glyph = origGlyph
                origGlyph = None
                self.hs.overlapRemoved = False
            else:
                self.glyph.associatePath(origGlyph)

        self.prepForSegs()
        self.Bonus = 0
        c = self.glyph.next(self.glyph, segSub=True)
        while c:
            self.genSegsForPathElement(c)
            c = self.glyph.next(c, segSub=True)

        if origGlyph is not None:
            self.glyph = origGlyph

        self.hs.compactLists()
        self.hs.remExtraBends()
        self.hs.cleanup()
        self.checkTfm()

    def genSegsForPathElement(self, c):
        """
        Calculates and adds segments for pathElement c. These segments
        indicate "flat" areas of the glyph in the relevant dimension
        weighted by segment length.
        """
        prv = self.glyph.prevInSubpath(c, segSub=True)
        log.debug("Element %d %d" % (c.position[0], c.position[1] + 1))
        if c.isStart():
            # Certain glyphs with one contour above (or below) all others
            # are marked as "Special", so that segments on that contour
            # are given a bonus weight increasing the likelihood of their
            # segment locations ending up in the final set of stem hints.
            self.Bonus = 0
            if (self.isSpecial(lower=False) and
                    self.relPosition(c, lower=False)):
                self.Bonus = self.SpecialCharBonus
            elif (self.isSpecial(lower=True) and
                  self.relPosition(c, lower=True)):
                self.Bonus = self.SpecialCharBonus
        if c.isLine() and not c.isTiny():
            # If the line is completely flat, add the line itself as a
            # segment. Otherwise if it is somewhat flat add a segment
            # reduced in length by adjustDist between the start and end
            # points at the location picked by pickSpot. Warn if the
            # line is close to but not quite horizontal.
            q = self.flatQuo(c.s, c.e)
            if q > 0:
                if feq(c.s.o, c.e.o):
                    self.addSegment(c.s.a, c.e.a, c.s.o, prv, c,
                                    hintSegment.sType.LINE, "flat line")
                else:
                    if q < .25:
                        q = .25
                    adist = self.adjustDist(c.e.a - c.s.a, q) / 2
                    aavg = (c.s.a + c.e.a) / 2
                    sp = self.pickSpot(c.s, c.e, adist, c.s, c.e,
                                       self.glyph.prevSlopePoint(c),
                                       self.glyph.nextSlopePoint(c))
                    self.addSegment(aavg - adist, aavg + adist, sp, prv, c,
                                    hintSegment.sType.LINE, "line")
                    d = (c.s - c.e).abs()
                    if d.o <= 2 and (d.a > 10 or d.normsq() > 100):
                        log.info("The line from %g %g to %g %g" %
                                 (*c.s, *c.e) +
                                 " is not exactly " + self.aDesc())
            else:
                # If the line is not somewhat flat just add BEND segments
                self.doBendsNext(c)
                self.doBendsPrev(c)
        elif not c.isLine():
            if c.flex == 2:
                # Flex-hinted curves have a segment corresponding to the
                # line between the curve-pair endpoints. For a pair flat
                # in the relevant dimension this is the line used for the
                # pair at lower resolutions.
                fl1prv = self.glyph.prevInSubpath(prv, segSub=True)
                if self.flatQuo(prv.s, c.e) > 0:
                    self.addSegment(prv.s.a, c.e.a, c.e.o, fl1prv, c,
                                    hintSegment.sType.LINE, "flex")
            if c.flex != 2:
                # flex != 2 => the start point is not in the middle of a
                # flex hint, so this code is for hinting the start points
                # of curves.
                #
                # If the start of the segment is not interestingly flat
                # just add BEND segments. Otherwise add a CURVE segment
                # for the start of the curve if the start slope is flat
                # or the node is (relative to the previous spline) either
                # flat or it bends away sharply.
                #
                # q2 measures the slope of the start point to the second
                # control point and just functions to modify the
                # segment length calculation.
                q = self.flatQuo(c.cs, c.s)
                if q == 0:
                    self.doBendsPrev(c)
                else:
                    if (feq(c.cs.o, c.s.o) or
                        (fne(c.ce.o, c.e.o) and
                         (self.nodeIsFlat(c, doPrev=True) or
                          not self.sameDir(c, doPrev=True)))):
                        q2 = self.flatQuo(c.ce, c.s)
                        if (q2 > 0 and self.sameSign(c.cs.a - c.s.a,
                                                     c.ce.a - c.s.a) and
                                abs(c.ce.a - c.s.a) > abs(c.cs.a - c.s.a)):
                            adist = self.adjustDist(self.CPTo(c.cs.a,
                                                              c.ce.a) -
                                                    c.s.a, q2)
                            end = self.adjustDist(self.CPTo(c.s.a,
                                                            c.cs.a) -
                                                  c.s.a, q)
                            if abs(end) > abs(adist):
                                adist = end
                            self.addSegment(c.s.a, c.s.a + adist, c.s.o,
                                            prv, c,
                                            hintSegment.sType.CURVE,
                                            "curve start 1")
                        else:
                            adist = self.adjustDist(self.CPTo(c.s.a,
                                                              c.cs.a) -
                                                    c.s.a, q)
                            self.addSegment(c.s.a, c.s.a + adist, c.s.o,
                                            prv, c,
                                            hintSegment.sType.CURVE,
                                            "curve start 2")
            if c.flex != 1:
                # flex != 1 => the end point is not in the middle of a
                # flex hint, so this code is for hinting the end points
                # of curves.
                #
                # If the end of the segment is not interestingly flat
                # just add BEND segments. Otherwise add a CURVE segment
                # for the start of the curve if the end slope is flat
                # or the node is (relative to the next spline) either
                # flat or it bends away sharply.
                #
                # The second and third segment types correspond to the
                # first and second start point types. The first handles
                # the special case of a close-to-flat curve, treating
                # it somewhat like a close-to-flat line.
                q = self.flatQuo(c.ce, c.e)
                if q == 0:
                    self.doBendsNext(c)
                elif (feq(c.ce.o, c.e.o) or
                      (fne(c.cs.o, c.s.o) and
                       (self.nodeIsFlat(c, doPrev=False) or
                        not self.sameDir(c, doPrev=False)))):
                    adist = self.adjustDist(c.e.a -
                                            self.CPFrom(c.ce.a, c.e.a), q)
                    q2 = self.flatQuo(c.s, c.e)
                    if q2 > 0:
                        ad2 = self.adjustDist(c.e.a - c.s.a, q2)
                    else:
                        ad2 = 0
                    if q2 > 0 and abs(ad2) > abs(adist):
                        ccs, cce = c.cs, c.ce
                        if (feq(c.s.o, c.cs.o) and feq(c.cs.o, c.ce.o) and
                                feq(c.ce.o, c.e.o)):
                            if (self.options.allowChanges and
                                    self.AutoLinearCurveFix and
                                    not self.isMulti):
                                c.convertToLine()
                                ccs, cce = c.s, c.e
                                log.info("Curve from %s to %s " % (c.s,
                                                                   c.e) +
                                         "changed to a line.")
                            elif not self.isMulti:
                                log.info("Curve from %s to %s " % (c.s,
                                                                   c.e) +
                                         "can be changed to a line.")
                        adist = ad2 / 2
                        aavg = (c.s.a + c.e.a) / 2
                        sp = self.pickSpot(c.s, c.e, adist, ccs, cce,
                                           self.glyph.prevSlopePoint(c),
                                           self.glyph.nextSlopePoint(c))
                        self.addSegment(aavg - adist, aavg + adist, sp, c,
                                        None, hintSegment.sType.CURVE,
                                        "curve end 1")
                    else:
                        q2 = self.flatQuo(c.cs, c.e)
                        if (q2 > 0 and self.sameSign(c.cs.a - c.e.a,
                                                     c.ce.a - c.e.a) and
                                abs(c.ce.a - c.e.a) < abs(c.cs.a - c.e.a)):
                            aend = self.adjustDist(c.e.a -
                                                   self.CPFrom(c.cs.a,
                                                               c.ce.a),
                                                   q2)
                            if abs(aend) > abs(adist):
                                adist = aend
                            self.addSegment(c.e.a - adist, c.e.a, c.e.o, c,
                                            None, hintSegment.sType.CURVE,
                                            "curve end 2")
                        else:
                            self.addSegment(c.e.a - adist, c.e.a, c.e.o, c,
                                            None, hintSegment.sType.CURVE,
                                            "curve end 3")
            if c.flex is None:
                # Curves that are not part of a flex hint can have an
                # extrema that is flat in the relative dimension. This
                # calls farthestExtreme() and if there is such an extrema
                # more than 2 points away from the start and end points
                # it calls extremaSegment() and adds the segment.
                tmp = c.getBounds().farthestExtreme(not self.isV())
                d, extp, extt, isMin = tmp
                if d > 2:
                    frst, lst = self.extremaSegment(c, extp, extt, isMin)
                    aavg = (frst + lst) / 2
                    if feq(frst, lst):
                        adist = (c.e.a - c.s.a) / 10
                    else:
                        adist = (lst - frst) / 2
                    if abs(adist) < self.BendLength:
                        adist = math.copysign(adist, self.BendLength)
                    self.addSegment(aavg - adist, aavg + adist,
                                    round(extp.o + 0.5), c, None,
                                    hintSegment.sType.CURVE,
                                    "curve extrema", True)

    def limitSegs(self):
        maxsegs = max(len(self.hs.increasingSegs), len(self.hs.decreasingSegs))
        if (not self.options.explicitGlyphs and
                maxsegs > self.options.maxSegments):
            log.warning("Calculated %d segments, skipping %s stem testing" %
                        (maxsegs, self.aDesc()))
            self.hs.deleteSegments()

    def showSegs(self):
        """
        Adds a debug log message for each generated segment.
        This information is redundant with the genSegs info except that
        it shows the result of processing with compactLists(),
        remExtraBends(), etc.
        """
        log.debug("Generated segments")
        for pe in self.glyph:
            log.debug("for path element x %g y %g" % (pe.e.x, pe.e.y))
            pestate = self.hs.getPEState(pe)
            seglist = pestate.segments() if pestate else []
            if seglist:
                for seg in seglist:
                    seg.show("generated")
            else:
                log.debug("None")

    # Generate candidate stems with values

    def genStemVals(self):
        """
        Pairs segments of opposite direction and adds them as potential
        stems weighted by evalPair(). Also adds ghost stems for segments
        within BlueValue bands
        """
        ll, ul = self.segmentLists()

        for ls in ll:
            for us in ul:
                if ls.loc > us.loc:
                    continue
                val, spc = self.evalPair(ls, us)
                self.stemMiss(ls, us)
                self.addStemValue(ls.loc, us.loc, val, spc, ls, us)

        if self.hasBands():
            ghostSeg = hintSegment(0, 0, 0, None, hintSegment.sType.GHOST,
                                   0, self.isV(), False, "ghost")
            for s in ll:
                if self.inBand(s.loc, isBottom=True):
                    ghostSeg.loc = s.loc + self.GhostWidth
                    cntr = (s.max + s.min) / 2
                    ghostSeg.max = round(cntr + self.GhostLength / 2)
                    ghostSeg.min = round(cntr - self.GhostLength / 2)
                    ghostSeg.isInc = not s.isInc
                    self.addStemValue(s.loc, ghostSeg.loc, self.GhostVal,
                                      self.GhostSpecial, s, ghostSeg)
            for s in ul:
                if self.inBand(s.loc, isBottom=False):
                    ghostSeg.loc = s.loc - self.GhostWidth
                    cntr = (s.max + s.min) / 2
                    ghostSeg.max = round(cntr + self.GhostLength / 2)
                    ghostSeg.min = round(cntr - self.GhostLength / 2)
                    ghostSeg.isInc = not s.isInc
                    self.addStemValue(ghostSeg.loc, s.loc, self.GhostVal,
                                      self.GhostSpecial, ghostSeg, s)
        self.combineStemValues()

    def evalPair(self, ls, us):
        """
        Calculates the initial "value" and "special" weights of a potential
        stem.

        Stems in one BlueValue band are given a spc boost but stems in
        both are ignored (presuambly because the Blues and OtherBlues are
        sufficient for hinting).

        Otherwise the value is based on:
           o The distance between the segment locations
           o The segment lengths
           o the extent of segment overlap (in the opposite direction)
           o Segment "bonus" values
        """
        spc = 0
        loc_d = abs(us.loc - ls.loc)
        if loc_d < self.MinDist:
            return 0, spc
        inBBand = self.inBand(ls.loc, isBottom=True)
        inTBand = self.inBand(us.loc, isBottom=False)
        if inBBand and inTBand:
            return 0, spc
        if inBBand or inTBand:
            spc += 2
        if us.min <= ls.max and us.max >= ls.min:  # overlap
            overlen = min(us.max, ls.max) - max(us.min, ls.min)
            minlen = min(us.max - us.min, ls.max - ls.min)
            if feq(overlen, minlen):
                dist = loc_d
            else:
                dist = loc_d * (1 + .4 * (1 - overlen / minlen))
        else:
            o_d = min(abs(us.min - ls.max), abs(us.max - ls.min))
            dist = round(loc_d * self.NoOverlapPenalty +
                         o_d * o_d / self.GapDistDenom)
            if o_d > loc_d:  # XXX this didn't work before
                dist *= o_d / loc_d
        dist = max(dist, 2 * self.MinDist)
        if min(ls.bonus, us.bonus) > 0:
            spc += 2
        for dsw in self.dominantStems():
            if dsw == abs(loc_d):
                spc += 1
                break
        dist = max(dist, 2)
        bl = max(ls.max - ls.min, 2)
        ul = max(us.max - us.min, 2)
        rl = bl * bl
        ru = ul * ul
        q = dist * dist
        v = 1000 * rl * ru / (q * q)
        if loc_d > self.BigDist:
            fac = self.BigDist / loc_d
            if fac > .5:
                v *= fac**8
            else:
                return 0, spc
        v = max(v, self.MinValue)
        v = min(v, self.MaxValue)
        return v, spc

    def stemMiss(self, ls, us):
        """
        Adds an info message for each stem within two em-units of a dominant
        stem width
        """
        loc_d = abs(us.loc - ls.loc)

        if loc_d < self.MinDist:
            return 0

        if us.min > ls.max or us.max < ls.min:  # no overlap
            return

        overlen = min(us.max, ls.max) - max(us.min, ls.min)
        minlen = min(us.max - us.min, ls.max - ls.min)
        if feq(overlen, minlen):
            dist = loc_d
        else:
            dist = loc_d * (1 + .4 * (1 - overlen / minlen))
        if dist < self.MinDist / 2:
            return

        d, nearStem = min(((abs(s - loc_d), s) for s in self.dominantStems()))
        if d == 0 or d > 2:
            return
        log.info("%s %s stem near miss: %g instead of %g at %g to %g." %
                 (self.aDesc(),
                  "curve" if (ls.isCurve() or us.isCurve()) else "linear",
                  loc_d, nearStem, ls.loc, us.loc))

    def addStemValue(self, lloc, uloc, val, spc, lseg, useg):
        """Adapts the stem parameters into a stemValue object and adds it"""
        if val == 0:
            return
        if (self.Pruning and val < self.PruneValue) and spc <= 0:
            return
        if lseg.isBend() and useg.isBend():
            return
        ghst = (lseg.isGhost() or useg.isGhost())
        if not ghst and (self.Pruning and val <= self.PruneD) and spc <= 0:
            if lseg.isBend() or useg.isBend():
                return
            lpesub = lseg.pe().position[0]
            upesub = useg.pe().position[0]
            if lpesub != upesub:
                lsb = self.glyph.getBounds(lpesub)
                usb = self.glyph.getBounds(upesub)
                if not lsb.within(usb) and not usb.within(lsb):
                    return
        if not useg:
            return

        sv = stemValue(lloc, uloc, val, spc, lseg, useg, ghst)
        self.insertStemValue(sv)

    def insertStemValue(self, sv, note="add"):
        """
        Adds a stemValue object into the stemValues list in sort order,
        skipping redundant GHOST stems
        """
        svl = self.hs.stemValues
        i = bisect.bisect_left(svl, sv)
        if sv.isGhost:
            j = i
            while j < len(svl) and svl[j] == sv:
                if (not svl[j].isGhost and
                        (svl[j].lseg == sv.lseg or svl[j].useg == sv.useg) and
                        svl[j].val > sv.val):
                    # Don't add
                    return
                j += 1
        svl.insert(i, sv)
        sv.show(self.isV(), note)

    def combineStemValues(self):
        """
        Adjusts the values of stems with the same locations to give them
        each the same combined value.
        """
        svl = self.hs.stemValues
        l = len(svl)
        i = 0
        while i < l:
            val = svl[i].val
            j = i + 1
            while j < l and svl[i] == svl[j]:
                if svl[j].isGhost:
                    val = svl[j].val
                else:
                    val += svl[j].val + 2 * math.sqrt(val * svl[j].val)
                j += 1
            for k in range(i, j):
                svl[k].val = val
            i = j

    # Prune unneeded candidate stems

    def pruneStemVals(self):
        """
        Prune (remove) candidate stems based on comparisons to other stems.
        """
        ignorePruned = not self.isV() and (self.options.report_zones or
                                           not self.options.report_stems)
        svl = self.hs.stemValues
        for c in svl:
            otherLow = otherHigh = False
            uInBand = self.inBand(c.uloc, isBottom=False)
            lInBand = self.inBand(c.lloc, isBottom=True)
            for d in svl:
                if d.pruned and not ignorePruned:
                    continue
                if (not c.isGhost and d.isGhost and
                        not d.val > c.val * self.VeryMuchPF):
                    continue
                if feq(c.lloc, d.lloc) and feq(c.uloc, d.uloc):
                    continue
                if self.isV() and d.val <= c.val * self.PruneFactor:
                    continue
                csl = self.closeSegs(c.lseg, d.lseg)
                csu = self.closeSegs(c.useg, d.useg)
                if c.val < 100 and d.val > c.val * self.MuchPF:
                    cs_tst = csl or csu
                else:
                    cs_tst = csl and csu
                if (d.val > c.val * self.PruneFactor and
                    c.lloc - self.PruneDistance <= d.lloc and
                    c.uloc + self.PruneDistance >= d.uloc and cs_tst and
                    (self.isV() or c.val < 16 or
                     ((not uInBand or feq(c.uloc, d.uloc)) and
                      (not lInBand or feq(c.lloc, d.lloc))))):
                    self.prune(c, d, "close and higher value")
                    break
                if c.lseg is not None and c.useg is not None:
                    if abs(c.lloc - d.lloc) < 1:
                        if (not otherLow and
                                c.val < d.val * self.PruneFactorLt and
                                abs(d.uloc - d.lloc) < abs(c.uloc - c.lloc) and
                                csl):
                            otherLow = True
                        if ((self.isV() or
                                (d.val > c.val * self.PruneFactor and
                                 not uInBand)) and c.useg.isBend() and csl):
                            self.prune(c, d, "lower bend")
                            break
                    if abs(c.uloc - d.uloc) < 1:
                        if (not otherHigh and
                                c.val < d.val * self.PruneFactorLt and
                                abs(d.uloc - d.lloc) < abs(c.uloc - c.lloc) and
                                csu):
                            otherHigh = True
                        if ((self.isV() or
                                (d.val > c.val * self.PruneFactor and
                                 not lInBand)) and c.lseg.isBend() and csu):
                            self.prune(c, d, "upper bend")
                            break
                    if otherLow and otherHigh:
                        self.prune(c, d, "low and high")
                        break
        self.hs.stemValues = [sv for sv in self.hs.stemValues if not sv.pruned]

    def closeSegs(self, s1, s2):
        """
        Returns true if the segments (and the path between them)
        are within CloseMerge of one another
        """
        if not s1 or not s2:
            return False
        if s1 is s2 or not s1.pe or not s2.pe or s1.pe is s2.pe:
            return True
        if s1.loc < s2.loc:
            loca, locb = s1.loc, s2.loc
        else:
            locb, loca = s1.loc, s2.loc
        if (locb - loca) > 5 * self.CloseMerge:
            return False
        loca -= self.CloseMerge
        locb += self.CloseMerge
        n = s1.pe()
        p = self.glyph.prevInSubpath(n)
        pe2 = s2.pe()
        ngood = pgood = True
        # To be close in this sense the segments should be in the same
        # subpath and are connected by a route that stays within CloseMerge
        # So we trace around the s1.pe's subpath looking for pe2 in each
        # direction and give up once we've iterated the length of the subpath.
        for _cnt in range(len(self.glyph.subpaths[n.position[0]])):
            if not ngood and not pgood:
                return False
            assert n and p
            if (ngood and n is pe2) or (pgood and p is pe2):
                return True
            if n.e.o > locb or n.e.o < loca:
                ngood = False
            if p.e.o > locb or p.e.o < loca:
                pgood = False
            n = self.glyph.nextInSubpath(n)
            p = self.glyph.prevInSubpath(p)
        return False

    def prune(self, sv, other_sv, desc):
        """
        Sets the pruned property on sv and logs it and the "better" stemValue
        """
        log.debug("Prune %s val: %s" % (self.aDesc(), desc))
        sv.show(self.isV(), "pruned")
        other_sv.show(self.isV(), "pruner")
        sv.pruned = True

    # Associate segments with the highest valued close stem

    def highestStemVals(self):
        """
        Associates each segment in both lists with the highest related stemVal,
        pruning stemValues with no association
        """
        for sv in self.hs.stemValues:
            sv.pruned = True
        ll, ul = self.segmentLists()
        self.findHighestValForSegs(ul, True)
        self.findHighestValForSegs(ll, False)
        self.hs.stemValues = [sv for sv in self.hs.stemValues
                              if not sv.pruned]

    def findHighestValForSegs(self, segl, isU):
        """Associates each segment in segl with the highest related stemVal"""
        for seg in segl:
            ghst = None
            highest = self.findHighestVal(seg, isU, False)
            if highest is not None and highest.isGhost:
                nonGhst = self.findHighestVal(seg, isU, True)
                if nonGhst is not None and nonGhst.val >= 2:
                    ghst = highest
                    highest = nonGhst
            if highest is not None:
                if not (highest.val < self.MinHighSegVal and
                        (ghst is None or ghst.val < self.MinHighSegVal)):
                    highest.pruned = False
                    seg.hintval = highest

    def findHighestVal(self, seg, isU, locFlag):
        """Finds the highest stemVal related to seg"""
        highest = None
        svl = self.hs.stemValues

        def OKcond(sv):
            vs, vl = (sv.useg, sv.uloc) if isU else (sv.lseg, sv.lloc)
            if locFlag:
                loctest = not sv.isGhost
            else:
                loctest = vs is seg or self.closeSegs(vs, seg)
            return (abs(vl - seg.loc) <= self.MaxMerge and loctest and
                    self.considerValForSeg(sv, seg, isU))

        try:
            _, highest = max(((sv.compVal(self.SpcBonus, self.GhostFactor), sv)
                              for sv in svl if OKcond(sv)))
        except ValueError:
            pass
        log.debug("findHighestVal: loc %g min %g max %g" %
                  (seg.loc, seg.min, seg.max))
        if highest:
            highest.show(self.isV(), "highest")
        else:
            log.debug("NULL")
        return highest

    def considerValForSeg(self, sv, seg, isU):
        """Utility test for findHighestVal"""
        if sv.spc > 0 or self.inBand(seg.loc, not isU):
            return True
        return not (self.Pruning and sv.val < self.PruneB)

    # Merge related candidate stems

    def findBestValues(self):
        """
        Looks among stemValues with the same locations and finds the one
        with the highest spc/val. Assigns that stemValue to the .best
        property of that set
        """
        svl = self.hs.stemValues
        svll = len(svl)
        for i in range(svll):
            if svl[i].best is not None:
                continue
            blst = [svl[i]]
            b = i
            for j in range(i + 1, svll):
                if (svl[j].best is not None or fne(svl[j].lloc, svl[b].lloc) or
                        fne(svl[j].uloc, svl[b].lloc)):
                    continue
                blst.append(svl[j])
                if ((svl[j].spc == svl[b].spc and svl[j].val > svl[b].val) or
                        svl[j].spc > svl[b].spc):
                    b = j
            for sv in blst:
                sv.best = svl[b]

    def replaceVals(self, oldl, oldu, newl, newu, newbest):
        """
        Finds each stemValue at oldl, oldu and gives it a new "best"
        stemValue reference and its val and spc.
        """
        for sv in self.hs.stemValues:
            if fne(sv.lloc, oldl) or fne(sv.uloc, oldu) or sv.merge:
                continue
            log.debug("Replace %s hints pair at %g %g by %g %g" %
                      (self.aDesc(), oldl, oldu, newl, newu))
            log.debug("\told value %g %g new value %g %g" %
                      (sv.val, sv.spc, newbest.val, newbest.spc))
            sv.lloc = newl
            sv.uloc = newu
            sv.val = newbest.val
            sv.spc = newbest.spc
            sv.best = newbest
            sv.merge = True

    def mergeVals(self):
        """
        Finds stem pairs with sides close to one another (in different
        senses) and uses replaceVals() to substitute one for another
        """
        self.findBestValues()

        if not self.options.report_zones:
            return
        svl = self.hs.stemValues
        for sv in svl:
            sv.merge = False
        while True:
            try:
                _, bst = max(((sv.best.compVal(self.SFactor), sv) for sv in svl
                              if not sv.merge))
            except ValueError:
                break
            bst.merge = True
            for sv in svl:
                replace = False
                if sv.merge or bst.isGhost != sv.isGhost:
                    continue
                if feq(bst.lloc, sv.lloc) and feq(bst.uloc, sv.uloc):
                    continue
                svuIB = self.inBand(sv.uloc, isBottom=False)
                svlIB = self.inBand(sv.lloc, isBottom=True)
                btuIB = self.inBand(bst.uloc, isBottom=False)
                btlIB = self.inBand(bst.lloc, isBottom=True)
                if ((feq(bst.lloc, sv.lloc) and
                     self.closeSegs(bst.lseg, sv.lseg) and
                     not btlIB and not svuIB and not btuIB) or
                    (feq(bst.uloc, sv.uloc) and
                     self.closeSegs(bst.useg, sv.useg) and
                     not btuIB and not svlIB and not btlIB) or
                    (abs(bst.lloc - sv.lloc) <= self.MaxMerge and
                     abs(bst.uloc - sv.uloc) <= self.MaxMerge and
                     (self.isV() or feq(bst.lloc, sv.lloc) or not svlIB) and
                     (self.isV() or feq(bst.uloc, sv.uloc) or not svuIB))):
                    if (feq(bst.best.spc, sv.spc) and
                            feq(bst.best.val, sv.val) and
                            not self.isV()):
                        if svlIB:
                            if bst.lloc > sv.lloc:
                                replace = True
                        elif svuIB:
                            if bst.uloc < sv.uloc:
                                replace = True
                    else:
                        replace = True
                elif (feq(bst.best.spc, sv.spc) and bst.lseg is not None and
                      bst.useg is not None):
                    if sv.lseg is not None and sv.useg is not None:
                        if (abs(bst.lloc - sv.lloc) <= 1 and
                                abs(bst.uloc - sv.uloc) <= self.MaxBendMerge):
                            if sv.useg.isBend() and not svuIB:
                                replace = True
                        elif (abs(bst.uloc - sv.uloc) <= 1 and
                              abs(bst.lloc - sv.lloc) <= self.MaxBendMerge):
                            if sv.lseg.isBend() and not svlIB:
                                replace = True
                if replace:
                    self.replaceVals(sv.lloc, sv.uloc, bst.lloc, bst.uloc,
                                     bst.best)

    # Limit number of stems

    def limitVals(self):
        """
        Limit the number of stem values in a dimension
        """
        svl = self.hs.stemValues
        if len(svl) <= self.StemLimit:
            return

        log.info("Trimming stem list to %g from %g" %
                 (self.StemLimit, len(svl)))
        # This will leave some segments with .highest entries that aren't
        # part of the stemValues list, but those won't get .idx values so
        # things will mostly work out. We could do better trying to find
        # alternative hints for segments. (Thee previous code just chopped
        # the list at one end.)
        ol = [(sv.compVal(self.SpcBonus), sv) for sv in svl]
        ol.sort(reverse=True)

        svl[:] = sorted((sv for weight, sv in ol[:self.StemLimit]))

    # Reporting

    def checkVals(self):
        """Reports stems with widths close to a dominant stem width"""
        lPrev = uPrev = -1e20
        for sv in self.hs.stemValues:
            l, u = sv.lloc, sv.uloc
            w = abs(u - l)
            minDiff = 0
            try:
                minDiff, mdw = min(((abs(w - dw), dw)
                                    for dw in self.dominantStems()))
            except ValueError:
                pass
            if feq(minDiff, 0) or minDiff > 2:
                continue
            if fne(l, lPrev) or fne(u, uPrev):
                line = self.findLineSeg(l, True) and self.findLineSeg(u, False)
                if not sv.isGhost:
                    log.info("%s %s stem near miss: " %
                             (self.aDesc(), "linear" if line else "curve") +
                             "%g instead of %g at %g to %g" % (w, mdw, l, u))
            lPrev, uPrev = l, u

    def findLineSeg(self, loc, isBottom=False):
        """Returns LINE segments with the passed location"""
        for s in self.segmentLists()[0 if isBottom else 1]:
            if feq(s.loc, loc) and s.isLine():
                return True
        return False

    def reportStems(self):
        """Reports stem zones and char ("alignment") zones"""
        glyphTop = -1e40
        glyphBot = 1e40
        isV = self.isV()
        for sv in self.hs.stemValues:
            l, u = sv.lloc, sv.uloc
            glyphBot = min(l, glyphBot)
            glyphTop = max(u, glyphTop)
            if not sv.isGhost:
                line = self.findLineSeg(l, True) and self.findLineSeg(u, False)
                self.report.stem(l, u, line, isV)
                if not isV:
                    self.report.stemZone(l, u)  # XXX ?

        if not isV and glyphTop > glyphBot:
            self.report.charZone(glyphBot, glyphTop)

    # "Main" stems

    def mainVals(self):
        """Picks set of highest-valued non-overlapping stems"""
        mainValues = []
        rejectValues = []
        svl = copy(self.hs.stemValues)
        prevBV = 0
        while True:
            try:
                _, best = max(((sv.compVal(self.SpcBonus), sv) for sv in svl
                               if self.mainOK(sv.spc, sv.val, mainValues,
                                              prevBV)))
            except ValueError:
                break
            lseg, useg = best.lseg, best.useg
            if best.isGhost:
                for sv in svl:
                    if (feq(best.lloc, sv.lloc) and feq(best.uloc, sv.uloc) and
                            not sv.isGhost):
                        lseg, useg = sv.lseg, sv.useg
                        break
                if lseg.isGhost():
                    newbest = useg.hintval
                    if (newbest is not None and newbest is not best and
                            newbest in svl):
                        best = newbest
                elif useg.isGhost():
                    newbest = lseg.hintval
                    if (newbest is not None and newbest is not best and
                            newbest in svl):
                        best = newbest
            svl.remove(best)
            prevBV = best.val
            mainValues.append(best)
            llocb, ulocb = best.lloc, best.uloc
            if best.isGhost:
                if best.lseg.isGhost():
                    llocb = ulocb
                else:
                    ulocb = llocb
            llocb -= self.BandMargin
            ulocb += self.BandMargin
            i = 0
            while i < len(svl):
                llocsv, ulocsv = svl[i].lloc, svl[i].uloc
                if svl[i].isGhost:
                    if svl[i].lseg.isGhost():
                        llocsv = ulocsv
                    else:
                        ulocsv = llocsv
                if llocsv <= ulocb and ulocsv >= llocb:
                    rejectValues.append(svl[i])
                    del svl[i]
                else:
                    i += 1
        rejectValues.extend(svl)
        self.hs.mainValues = mainValues
        self.hs.rejectValues = rejectValues

    def mainOK(self, spc, val, hasHints, prevBV):
        """Utility test for mainVals"""
        if spc > 0:
            return True
        if not hasHints:
            return not self.Pruning or val >= self.PruneD
        if not self.Pruning or val > self.PruneA:
            return True
        if self.Pruning and val < self.PruneB:
            return False
        return not self.Pruning or prevBV <= val * self.PruneC

    def tryCounterHinting(self):
        """
        Attempts to counter-hint the dimension with the first three
        (e.g. highest value) mainValue stems
        """
        minloc = midloc = maxloc = 1e40
        mindelta = middelta = maxdelta = 0
        hvl = self.hs.mainValues
        hvll = len(hvl)
        if hvll < 3:
            return False
        bestv = hvl[0].val
        pbestv = hvl[3].val if hvll > 3 else 0
        if pbestv > 1000 or bestv < pbestv * 10:
            return False
        for hv in hvl[0:3]:
            loc = hv.lloc
            delta = hv.uloc - loc
            loc += delta / 2
            if loc < minloc:
                maxloc, midloc, minloc = midloc, minloc, loc
                maxdelta, middelta, mindelta = middelta, mindelta, delta
            elif loc < midloc:
                maxloc, midloc = midloc, loc
                maxdelta, middelta = middelta, delta
            else:
                maxloc = loc
                maxdelta = delta
        if (abs(mindelta - maxdelta) < self.DeltaDiffMin and
                abs((maxloc - midloc) - (midloc - minloc)) <
                self.DeltaDiffMin):
            self.hs.rejectValues.extend(hvl[3:])
            self.hs.mainValues = hvl[0:3]
            return True
        if (abs(mindelta - maxdelta) < self.DeltaDiffReport and
                abs((maxloc - midloc) - (midloc - minloc)) <
                self.DeltaDiffReport):
            log.info("Near miss for %s counter hints." % self.aDesc())
        return False

    def addBBox(self, doSubpaths=False):
        """
        Adds the top and bottom (or left and right) sides of the glyph
        as a stemValue -- serves as a backup hint stem when few are found

        When called with doSubpaths == True adds stem hints for the
        top/bottom or right/left of each subpath
        """
        gl = self.glyph
        if doSubpaths:
            paraml = range(len(gl.subpaths))
            ltype = hintSegment.sType.LSBBOX
            utype = hintSegment.sType.USBBOX
        else:
            paraml = [None]
            ltype = hintSegment.sType.LGBBOX
            utype = hintSegment.sType.UGBBOX
        for param in paraml:
            pbs = gl.getBounds(param)
            if pbs is None:
                continue
            mn_pt, mx_pt = pt(tuple(pbs.bounds[0])), pt(tuple(pbs.bounds[1]))
            peidx = 0 if self.isV() else 1
            if any((hv.lloc <= mx_pt.o and mn_pt.o <= hv.uloc
                    for hv in self.hs.mainValues)):
                continue
            lseg = hintSegment(mn_pt.o, mn_pt.a, mx_pt.a, pbs.extpes[0][peidx],
                               ltype, 0, self.isV(), not self.isV(), "l bbox")
            pestate = self.hs.getPEState(pbs.extpes[0][peidx], True)
            assert pestate is not None
            pestate.m_segs.append(lseg)
            useg = hintSegment(mx_pt.o, mn_pt.a, mx_pt.a, pbs.extpes[1][peidx],
                               utype, 0, self.isV(), self.isV(), "u bbox")
            pestate = self.hs.getPEState(pbs.extpes[1][peidx], True)
            assert pestate is not None
            pestate.m_segs.append(useg)
            hv = stemValue(mn_pt.o, mx_pt.o, 100, 0, lseg, useg, False)
            self.insertStemValue(hv, "bboxadd")
            self.hs.mainValues.append(hv)
            self.hs.mainValues.sort(key=lambda sv: sv.compVal(self.SpcBonus),
                                    reverse=True)

    def markStraySegs(self):
        """
        highestStemVals() may not assign a hintval to a given segment.
        Once the list of stems has been arrived at we go through each
        looking for stems where the segment on one side is unassigned
        and assign it to that stem.
        """
        for sv in self.hs.stemValues:
            for seg in (sv.lseg, sv.useg):
                assert seg is not None
                if not seg.isGhost() and seg.hintval is None:
                    seg.hintval = sv
                    # assert False

    # masks

    def convertToStemLists(self):
        """
        This method builds up the information needed to mostly get away from
        looking at stem values when distributing hintmasks.

        hs.stems: Tuple of arrays of the eventual hstems or vstems objects
                  that will be copied into the glyphData object, one array
                  per gllist.
        """
        self.startStemConvert()
        if self.keepHints:
            if self.isV():
                self.hs.stems = tuple((g.vstems if g.vstems else []
                                       for g in self.gllist))
            else:
                self.hs.stems = tuple((g.hstems if g.hstems else []
                                       for g in self.gllist))
                l = self.hs.stems[0]
            self.stopStemConvert()
            return all((len(sl) == l for sl in self.hs.stems))

        # Build up the default stem list
        valuepairmap = {}
        self.hs.stems = tuple(([] for g in self.gllist))
        if self.options.removeConflicts:
            wl = self.hs.weights = []
        dsl = self.hs.stems[0]
        if self.hs.counterHinted:
            self.hs.stemValues = sorted(self.hs.mainValues)
        for sv in self.hs.stemValues:
            p = sv.ghosted()
            sidx = valuepairmap.get(p, None)
            if sidx is not None:
                sv.idx = sidx
                if self.options.removeConflicts:
                    wl[sidx] += sv.compVal(self.SpcBonus)[0]**.25
            else:
                valuepairmap[p] = sv.idx = len(dsl)
                dsl.append(stem(*p))
                if self.options.removeConflicts:
                    # The stem value is proportional to length(lseg)**2 *
                    # length(useg)**2. We want a "weight" that is vaguely
                    # additive with respect to segment length while still
                    # giving a healthy boost to stems with "special"
                    # segments. This isn't great but may be good enough.
                    wl.append(sv.compVal(self.SpcBonus)[0]**.25)

        # Determine the stem locations for other regions

        for glidx in range(1, len(self.gllist)):
            self.calcInstanceStems(glidx)

        log.debug("Initial %s stem list (including regions, if any):" %
                  self.aDesc())
        for sidx in range(len(self.hs.stems[0])):
            log.debug("Stem %d: %s" % (sidx, [sl[sidx]
                                              for sl in self.hs.stems]))
        self.stopStemConvert()
        return True

    def calcInstanceStems(self, glidx):
        self.glyph = self.gllist[glidx]
        self.fddict = self.fddicts[glidx]
        hs0 = self.hs
        numStems = len(hs0.stems[0])
        if numStems == 0:
            return
        set_log_parameters(instance=self.fddict.FontName)
        if self.isV():
            self.hs = self.glyph.vhs = glyphHintState()
        else:
            self.hs = self.glyph.hhs = glyphHintState()

        if self.glyph.hasHints(doVert=self.isV()):
            pass  # XXX to figure out

        self.keepHints = False
        self.startHint()
        self.BigDist = max(self.dominantStems() + [self.InitBigDist])
        self.BigDist *= self.BigDistFactor
        self.genSegs()

        iSSl = [(instanceStemState(s.lb, self), instanceStemState(s.rt, self))
                for s in hs0.stems[0]]

        for c in self.__iter__(glidx):
            c0 = c[0]
            ci = c[1]
            peS0 = hs0.getPEState(c0.pe)
            if peS0 is None:
                continue
            peSi = self.hs.getPEState(ci.pe)
            done = [[False, False] for x in range(numStems)]
            for pos0, seg0l in peS0.segLists():
                for seg0 in seg0l:
                    assert seg0.hintval is not None
                    sidx = seg0.hintval.idx
                    # assert sidx is not None
                    if sidx is None:
                        continue
                    ul = 1 if (seg0.isInc == self.isV()) else 0
                    if done[sidx][ul]:
                        continue

                    iSS = iSSl[sidx][ul]
                    found = False
                    assert self.glyph
                    if seg0.isBBox():
                        if seg0.isGBBox():
                            pbs = self.glyph.getBounds(None)
                        else:
                            pbs = self.glyph.getBounds(peSi.position[0])
                        if pbs is not None:
                            mn_pt = pt(tuple(pbs.bounds[0]))
                            mx_pt = pt(tuple(pbs.bounds[1]))
                            score = mx_pt.a - mn_pt.a
                            loc = mx_pt.o if seg0.isUBBox() else mn_pt.o
                            iSS.addToLoc(loc, score, strong=True, bb=True)
                            found = True
                    elif peSi is not None:
                        for posi, segil in peSi.segLists(pos0):
                            for segi in segil:
                                if segi.isInc != seg0.isInc:
                                    continue
                                score = segi.max - segi.min
                                iSS.addToLoc(segi.loc, score,
                                             strong=(posi == pos0),
                                             seg=segi)
                                found = True
                            if found and posi == pos0:
                                break
                    # Fall back to point location
                    if not found:
                        if pos0 == 's':
                            loc = ci.pe.s.o
                        elif pos0 == 'e':
                            loc = ci.pe.e.o
                        else:
                            loc = (ci.pe.s.o + ci.pe.e.o) / 2
                        log.info("Falling back to point location for "
                                 "segment %s" % (ci.pe.position,))
                        iSS.addToLoc(loc, self.NoSegScore)
                    done[sidx][ul] = True

        sl = hs0.stems[glidx]
        for sidx, s0 in enumerate(hs0.stems[0]):
            isG = s0.isGhost()
            if isG != 'high':
                lo = self.bestLocation(sidx, 0, iSSl, hs0)
            if isG != 'low':
                hi = self.bestLocation(sidx, 1, iSSl, hs0)
            if isG == 'low':
                lo = lo + 21
                hi = lo - 21
            elif isG == 'high':
                lo = hi
                hi = lo - 20
            else:
                if hi < lo:
                    # Differentiate bad stem from ghost stem
                    lo = hi + 50
                    log.warning("Stem end is less than start for non-"
                                "ghost stem, will be removed from set")
            sl.append(stem((lo, hi)))

        self.fddict = self.fddicts[0]
        self.glyph = self.gllist[0]
        set_log_parameters(instance=self.fddict.FontName)
        self.hs = hs0

    def bestLocation(self, sidx, ul, iSSl, hs0):
        loc = iSSl[sidx][ul].bestLocation(ul == 0)
        if loc is not None:
            return loc
        for sv in hs0.stemValues:
            if sv.idx != sidx:
                continue
            seg = sv.useg if ul == 1 else sv.lseg
            if seg.hintval.idx != sidx:
                loc = iSSl[seg.hintval.idx][ul].bestLocation(ul == 0)
                if loc is not None:
                    return loc
        log.warning("No data for %s location of stem %d" %
                    ('lower' if ul == 0 else 'upper', sidx))
        return hs0.stems[0][sidx][ul]

    def unconflict(self, sc, curSet=None, pinSet=None):
        l = len(sc)

        if curSet is None:
            curSet = [True] * l
        else:
            curSet = curSet.copy()
        if pinSet is None:
            pinSet = [False] * l
        else:
            pinSet = pinSet.copy()

        doIdx = None
        doConflictSet = None
        for sidx in range(l):
            if pinSet[sidx] or not curSet[sidx]:
                continue
            conflictSet = []
            for sjdx in range(sidx, l):
                if pinSet[sjdx] or not curSet[sidx]:
                    continue
                if sc[sidx][sjdx]:
                    conflictSet.append(sjdx)
            if conflictSet:
                if doIdx is None:
                    doIdx = sidx
                    doConflictSet = conflictSet
            else:
                pinSet[sidx] = True

        if doIdx is None:
            return (sum((self.hs.weights[x]
                         for x in range(l) if curSet[sidx])), curSet)
        else:
            pinSet[doIdx] = True
            for sidx in doConflictSet:
                curSet[sidx] = False
            r1 = self.unconflict(sc, curSet, pinSet)
            pinSet[doIdx] = curSet[doIdx] = False
            for sidx in doConflictSet:
                curSet[sidx] = True
            r2 = self.unconflict(sc, curSet, pinSet)
            return r1 if r1[0] > r2[0] else r2

    def convertToMasks(self):
        """
        This method builds up the information needed to mostly get away from
        looking at stem values when distributing hintmasks.

        hs.stemOverlaps: A map of which stems overlap with which other stems.
        hs.ghostCompat: [i][j] is true if stem i is a ghost and stem j can
                        substitute for it.
        """
        self.startMaskConvert()
        if self.keepHints:
            # XXX to figure out
            self.stopMaskConvert()
            return

        dsl = self.hs.stems[0]
        l = len(dsl)
        hasConflicts = False
        self.hs.goodMask = [True] * l
        if self.options.removeConflicts:
            sc = [[False] * l for _ in range(l)]
            for sl in self.hs.stems:
                for sidx in range(l):
                    if sl[sidx].isBad():
                        self.hs.goodMask[sidx] = False
                        continue
                    for sjdx in range(sidx + 1, l):
                        if sl[sidx] > sl[sjdx]:
                            hasConflicts = True
                            sc[sidx][sjdx] = sc[sjdx][sidx] = True
        if hasConflicts:
            _, self.hs.goodMask = self.unconflict(sc, self.hs.goodMask,
                                                  [not a
                                                   for a in self.hs.goodMask])
            log.info("Removed %s stems %s to resolve conflicts" %
                     (self.aDesc(), [i for i, g in enumerate(self.hs.goodMask)
                                     if not g]))

        self.hs.hasOverlaps = False
        self.hs.stemOverlaps = so = [[False] * l for _ in range(l)]
        for sidx in range(l):
            if not self.hs.goodMask[sidx]:
                continue
            for sjdx in range(sidx + 1, l):
                if not self.hs.goodMask[sjdx]:
                    continue
                for sl in self.hs.stems:
                    if sl[sidx].overlaps(sl[sjdx]) or sl[sidx] > sl[sjdx]:
                        self.hs.hasOverlaps = True
                        so[sidx][sjdx] = so[sjdx][sidx] = True

        if self.hs.counterHinted and self.hs.hasOverlaps and not self.isMulti:
            log.warning("XXX TEMPORARY WARNING: overlapping counter hints")

        self.hs.ghostCompat = gc = [None] * l

        # A ghost stem in the default should be a ghost everywhere
        for sidx, s in enumerate(dsl):
            if not self.hs.goodMask[sidx] or not s.isGhost(True):
                continue
            gc[sidx] = [False] * l
            for sjdx in range(l):
                if sidx == sjdx or not self.hs.goodMask[sjdx]:
                    continue
                if all(sl[sidx].ghostCompat(sl[sjdx]) for sl in self.hs.stems):
                    assert so[sidx][sjdx]
                    gc[sidx][sjdx] = True

        # The stems corresponding to mainValues may conflict now. This isn't
        # a fatal problem because mainMask stems are only added if they don't
        # conflict with those already in the current mask. However, since
        # they're not checked in the optimal order it's better to go through
        # them again to remove conflicts
        self.hs.mainMask = mm = [False] * l
        okl = []
        mv = self.hs.mainValues.copy()
        while mv:
            ok = True
            c = mv.pop(0)
            for s in okl:
                if so[c.idx][s.idx]:
                    ok = False
                    break
            if ok and self.hs.goodMask[c.idx]:
                mm[c.idx] = True
                okl.append(c)

        for pe in self.glyph:
            pestate = self.hs.getPEState(pe)
            if not pestate:
                continue
            self.makePEMask(pestate, pe)
        self.stopMaskConvert()

    def makePEMask(self, pestate, c):
        """Convert the hints desired by pathElement to a conflict-free mask"""
        l = len(self.hs.stems[0])
        mask = [False] * l
        segments = pestate.segments()
        for seg in segments:
            if (not seg.hintval or seg.hintval.idx is None or
                    not self.hs.goodMask[seg.hintval.idx] or
                    seg.suppressed):
                continue
            mask[seg.hintval.idx] = True
        p0, p1 = (c.s, c.e) if c.isLine() else (c.cs, c.e)
        if self.hs.hasOverlaps and True in mask:
            for sidx in range(l):
                for sjdx in range(sidx + 1, l):
                    if not (mask[sidx] and mask[sjdx] and
                            self.hs.stemOverlaps[sidx][sjdx]):
                        continue
                    remidx = None
                    _, segi = max(((sg.hintval.compVal(self.SpcBonus), sg)
                                  for sg in segments
                                  if (not sg.suppressed and
                                      sg.hintval.idx == sidx)))
                    _, segj = max(((sg.hintval.compVal(self.SpcBonus), sg)
                                  for sg in segments
                                  if (not sg.suppressed and
                                      sg.hintval.idx == sjdx)))
                    vali, valj = segi.hintval, segj.hintval
                    assert vali.lloc <= valj.lloc or valj.isGhost
                    n = self.glyph.nextInSubpath(c)
                    p = self.glyph.prevInSubpath(c)
                    if (vali.val < self.ConflictValMin and
                            self.OKToRem(segi.loc, vali.spc)):
                        remidx = sidx
                    elif (valj.val < self.ConflictValMin and
                          vali.val > valj.val * self.ConflictValMult and
                          self.OKToRem(segj.loc, valj.spc)):
                        remidx = sjdx
                    elif (c.isLine() or self.flatQuo(p0, p1) > 0 and
                          self.OKToRem(segi.loc, vali.spc)):
                        remidx = sidx
                    elif self.diffSign(p1.o - p0.o, p.s.o - p0.o):
                        remidx = sidx
                    elif self.diffSign(n.e.o - p1.o, p0.o - p1.o):
                        remidx = sjdx
                    elif ((feq(p1.o, valj.lloc) or
                           feq(p1.o, valj.uloc)) and
                          fne(p0.o, vali.lloc) and fne(p0.o, vali.uloc)):
                        remidx = sidx
                    elif ((feq(p0.o, vali.lloc) or
                           feq(p0.o, vali.uloc)) and
                          fne(p1.o, valj.lloc) and fne(p1.o, valj.uloc)):
                        remidx = sjdx
                    # XXX ResolveConflictBySplit was called here.
                    if remidx is not None:
                        mask[remidx] = False
                        log.debug("Resolved conflicting hints at %g %g" %
                                  (c.e.x, c.e.y))
                    else:
                        # Removing the hints from the spline doesn't mean
                        # neither will be hinted, it means that whatever
                        # winds up getting hinted will depend on the
                        # previous spline hint states. That could be
                        # neither but it is likely one will be in the set
                        mask[sidx] = mask[sjdx] = False
                        log.debug("Could not resolve conflicting hints" +
                                  " at %g %g" % (c.e.x, c.e.y) +
                                  ", removing both")
        if True in mask:
            maskstr = ''.join(('1' if i else '0' for i in mask))
            log.debug("%s mask %s at %g %g" %
                      (self.aDesc(), maskstr, c.e.x, c.e.y))
            pestate.mask = mask
        else:
            pestate.mask = None

    def OKToRem(self, loc, spc):
        return (spc == 0 or
                (not self.inBand(loc, False) and not self.inBand(loc, True)))


class hhinter(dimensionHinter):
    def startFlex(self):
        """Make pt.a map to x and pt.b map to y"""
        set_log_parameters(dimension='-')
        pt.setAlign(False)

    def stopFlex(self):
        set_log_parameters(dimension='')
        pt.clearAlign()

    def startHint(self):
        """
        Make pt.a map to x and pt.b map to y and store BlueValue bands
        for easier processing
        """
        self.startFlex()
        blues = self.fddict.BlueValuesPairs + self.fddict.OtherBlueValuesPairs
        self.topPairs = [pair for pair in blues if not pair[4]]
        self.bottomPairs = [pair for pair in blues if pair[4]]

    startStemConvert = startMaskConvert = startFlex

    stopHint = stopStemConvert = stopMaskConvert = stopFlex

    def dominantStems(self):
        return self.fddict.DominantH

    def isV(self):
        """Mark the hinter as horizontal rather than vertical"""
        return False

    def inBand(self, loc, isBottom=False):
        """Return true if loc is within the selected set of bands"""
        if self.name in self.options.noBlues:
            return False
        if isBottom:
            pl = self.bottomPairs
        else:
            pl = self.topPairs
        for p in pl:
            if (p[0] + self.fddict.BlueFuzz >= loc and
                    p[1] - self.fddict.BlueFuzz <= loc):
                return True
        return False

    def hasBands(self):
        return len(self.topPairs) + len(self.bottomPairs) > 0

    def isSpecial(self, lower=False):
        return False

    def aDesc(self):
        return 'horizontal'

    def checkTfm(self):
        self.checkTfmVal(self.hs.decreasingSegs, self.topPairs)
        self.checkTfmVal(self.hs.increasingSegs, self.bottomPairs)

    def checkTfmVal(self, sl, pl):
        for s in sl:
            if not self.checkInsideBands(s.loc, pl):
                self.checkNearBands(s.loc, pl)

    def checkInsideBands(self, loc, pl):
        for p in pl:
            if loc <= p[0] and loc >= p[1]:
                return True
        return False

    def checkNearBands(self, loc, pl):
        for p in pl:
            if loc >= p[1] - self.NearFuzz and loc < p[1]:
                log.info("Near miss above horizontal zone at " +
                         "%f instead of %f." % (loc, p[1]))
            if loc <= p[0] + self.NearFuzz and loc > p[0]:
                log.info("Near miss below horizontal zone at " +
                         "%f instead of %f." % (loc, p[0]))

    def segmentLists(self):
        return self.hs.increasingSegs, self.hs.decreasingSegs

    def isCounterGlyph(self):
        return self.name in self.options.hCounterGlyphs


class vhinter(dimensionHinter):
    def startFlex(self):
        set_log_parameters(dimension='|')
        pt.setAlign(True)

    def stopFlex(self):
        set_log_parameters(dimension='')
        pt.clearAlign()

    startHint = startStemConvert = startMaskConvert = startFlex
    stopHint = stopStemConvert = stopMaskConvert = stopFlex

    def isV(self):
        return True

    def dominantStems(self):
        return self.fddict.DominantV

    def inBand(self, loc, isBottom=False):
        return False

    def hasBands(self):
        return False

    def isSpecial(self, lower=False):
        """Check the Specials list for the current glyph"""
        if lower:
            return self.name in self.options.lowerSpecials
        else:
            return self.name in self.options.upperSpecials

    def aDesc(self):
        return 'vertical'

    def checkTfm(self):
        pass

    def segmentLists(self):
        return self.hs.decreasingSegs, self.hs.increasingSegs

    def isCounterGlyph(self):
        return self.name in self.options.vCounterGlyphs


class glyphHinter:
    """
    Adapter between high-level autohint.py code and the 1D hinter.
    Also contains code that uses hints from both dimensions, primarily
    for hintmask distribution
    """
    impl = None

    @classmethod
    def initialize(cls, options, dictRecord, logQueue=None):
        cls.impl = cls(options, dictRecord)
        if logQueue is not None:
            logging_reconfig(logQueue, options.verbose)

    @classmethod
    def hint(cls, name, glyphTuple=None, fdKey=None):
        if cls.impl is None:
            raise RuntimeError("glyphHinter implementation not initialized")
        if isinstance(name, tuple):
            return cls.impl._hint(*name)
        else:
            return cls.impl._hint(name, glyphTuple, fdKey)

    def __init__(self, options, dictRecord):
        self.options = options
        self.dictRecord = dictRecord
        self.hHinter = hhinter(options)
        self.vHinter = vhinter(options)
        self.cnt = 0
        if options.justReporting():
            self.taskDesc = 'analysis'
        else:
            self.taskDesc = 'hinting'

        self.FlareValueLimit = 1000
        self.MaxHalfMargin = 20  # XXX 10 might better match original C code
        self.PromotionDistance = 50

    def getSegments(self, glyph, pe, oppo=False):
        """Returns the list of segments for pe in the requested dimension"""
        gstate = glyph.vhs if (self.doV == (not oppo)) else glyph.hhs
        pestate = gstate.getPEState(pe)
        return pestate.segments() if pestate else []

    def getMasks(self, glyph, pe):
        """
        Returns the masks of hints needed by/desired for pe in each dimension
        """
        masks = []
        for i, hs in enumerate((glyph.hhs, glyph.vhs)):
            mask = None
            if hs.keepHints:
                if pe.mask:
                    mask = copy(pe.mask[i])
            elif hs.counterHinted:
                mask = copy(hs.mainMask)
            else:
                pes = hs.getPEState(pe)
                if pes and pes.mask:
                    mask = copy(pes.mask)
            if mask is None:
                mask = [False] * len(hs.stems[0])
            masks.append(mask)
        return masks

    def _hint(self, name, glyphTuple, fdKey):
        """Top-level flex and stem hinting method for a glyph"""
        if isinstance(fdKey, tuple):
            assert len(fdKey) == 2
            fdDictList = self.dictRecord[fdKey[0]][fdKey[1]]
        else:
            fdDictList = fdKey
        gllist = [g for g in glyphTuple if g is not None]
        fddicts = [d for g, d in zip(glyphTuple, fdDictList) if g is not None]

        defglyph = gllist[0]
        fddict = fddicts[0]

        an = self.options.nameAliases.get(name, name)
        if an != name:
            log_name = '%s (%s)' % (name, an)
        else:
            log_name = name

        if len(glyphTuple) > 1:
            set_log_parameters(glyph=log_name, instance=fddict.FontName)
        else:
            set_log_parameters(glyph=log_name, instance='')

        log.info("Begin %s (using fdDict '%s').", self.taskDesc,
                 fddict.DictName)

        gr = GlyphReport(name, self.options.report_all_stems)
        self.hHinter.setGlyph(fddicts, gr, gllist, name)
        self.vHinter.setGlyph(fddicts, gr, gllist, name)

        if not self.compatiblePaths(gllist, fddicts):
            set_log_parameters(glyph='', instance='')
            return name, None

        defglyph.changed = False

        if not self.options.noFlex:
            self.hHinter.addFlex()
            self.vHinter.addFlex(inited=True)

        lnks = links(defglyph)

        self.hHinter.calcHintValues(lnks)
        self.vHinter.calcHintValues(lnks)

        if self.options.justReporting():
            set_log_parameters(glyph='', instance='')
            return name, gr

        if self.hHinter.keepHints and self.vHinter.keepHints:
            set_log_parameters(glyph='', instance='')
            return name, None

        if self.options.allowChanges:
            neworder = lnks.shuffle()
            if neworder:
                for g in gllist:
                    g.reorder(neworder)

        self.doV = False
        self.listHintInfo(defglyph)

        if not self.hHinter.keepHints:
            self.remFlares(defglyph)

        self.doV = True
        if not self.vHinter.keepHints:
            self.remFlares(defglyph)

        self.hHinter.convertToStemLists()
        self.vHinter.convertToStemLists()

        self.otherInstanceStems(gllist)

        self.hHinter.convertToMasks()
        self.vHinter.convertToMasks()

        usedmasks = self.distributeMasks(defglyph)
        self.cleanupUnused(gllist, usedmasks)

        self.otherInstanceMasks(gllist)

        for g in gllist:
            g.clearTempState()

        set_log_parameters(glyph='', instance='')
        return name, glyphTuple

    def compatiblePaths(self, gllist, fddicts):
        if len(gllist) < 2:
            return True

        glyph = gllist[0]
        fddict = fddicts[0]

        for i, (g, fd) in enumerate(zip(gllist[1:], fddicts[1:])):
            set_log_parameters(instance=fd.FontName)
            if len(glyph.subpaths) != len(g.subpaths):
                log.warning("Path has a different number of subpaths compared "
                            "with '%s': Cannot hint." % fd.FontName)
                return False
            for si, dp in enumerate(glyph.subpaths):
                gp = g.subpaths[si]
                dpl, gpl = len(dp), len(gp)
                if gpl != dpl:
                    # XXX decide on warning message for these
                    if (gpl == dpl + 1 and gp[-1].isClose() and
                            not dp[-1].isClose()):
                        for _gi in range(i + 1):
                            gllist[i].addNullClose(si)
                        continue
                    if (dpl == gpl + 1 and dp[-1].isClose() and
                            not gp[-1].isClose()):
                        g.addNullClose(si)
                        continue
                    log.warning("subpath %d has %d elements " % (si, len(gp)) +
                                "while '%s' had %d elements: Cannot hint" %
                                (fd.FontName, len(dp)))
                    return False

        set_log_parameters(instance=fddict.FontName)
        # Use hHinter for path element list iterator
        for c in self.hHinter:
            isline = [x.pe.isLine() for x in c]
            if any(isline) and not all(isline):
                if self.options.allowChanges:
                    for p in c:
                        if p.pe.isLine():
                            p.pe.convertToCurve()
                else:
                    log.warning("Mixed lines and curves without " +
                                "allow-changes: Cannot hint")
                    return False
        return True

    def distributeMasks(self, glyph):
        """
        When necessary, chose the locations and contents of hintmasks for
        the glyph
        """
        stems = [None, None]
        masks = [None, None]
        lnstm = [0, 0]
        # Initial horizontal data
        # If keepHints was true hhs.stems was already set to glyph.hstems in
        # converttoMasks()
        stems[0] = glyph.hstems = glyph.hhs.stems[0]
        lnstm[0] = len(stems[0])
        if self.hHinter.keepHints:
            if glyph.startmasks and glyph.startmasks[0]:
                masks[0] = glyph.startmasks[0]
            elif not glyph.hhs.hasOverlaps:
                masks[0] = [True] * lnstm[0]
            else:
                pass  # XXX existing hints have conflicts but no start mask
        else:
            masks[0] = [False] * lnstm[0]

        # Initial vertical data
        stems[1] = glyph.vstems = glyph.vhs.stems[0]
        lnstm[1] = len(stems[1])
        if self.vHinter.keepHints:
            if glyph.startmasks and glyph.startmasks[1]:
                masks[1] = glyph.startmasks[1]
            elif not glyph.hhs.hasOverlaps:
                masks[1] = [True] * lnstm[1]
            else:
                pass  # XXX existing hints have conflicts but no start mask
        else:
            masks[1] = [False] * lnstm[1]

        self.buildCounterMasks(glyph)

        if not glyph.hhs.hasOverlaps and not glyph.vhs.hasOverlaps:
            if False in glyph.hhs.goodMask or False in glyph.vhs.goodMask:
                return [glyph.hhs.goodMask, glyph.vhs.goodMask]
            else:
                glyph.startmasks = None
                glyph.is_hm = False
                return None

        usedmasks = deepcopy(masks)
        if glyph.hhs.counterHinted:
            usedmasks[0] = [mv or uv for mv, uv in
                            zip(glyph.hhs.mainMask, usedmasks[0])]
        if glyph.vhs.counterHinted:
            usedmasks[1] = [mv or uv for mv, uv in
                            zip(glyph.vhs.mainMask, usedmasks[1])]

        glyph.is_hm = True
        glyph.startmasks = masks
        NOTSHORT, SHORT, CONFLICT = 0, 1, 2
        mode = NOTSHORT
        ns = None
        c = glyph.nextForHints(glyph)
        while c:
            if c.isShort() or c.flex == 2:
                if mode == NOTSHORT:
                    if ns:
                        mode = SHORT
                        oldmasks = masks
                        masks = deepcopy(masks)
                        incompatmasks = self.getMasks(glyph, ns)
                    else:
                        mode = CONFLICT
            else:
                ns = c
                if mode == SHORT:
                    oldmasks[:] = masks
                    masks = oldmasks
                    incompatmasks = None
                mode = NOTSHORT
            cmasks = self.getMasks(glyph, c)
            candmasks, conflict = self.joinMasks(masks, cmasks,
                                                 mode == CONFLICT)
            maskstr = ''.join(('1' if i else '0'
                               for i in (candmasks[0] + candmasks[1])))
            log.debug("mask %s at %g %g, mode %d, conflict: %r" %
                      (maskstr, c.e.x, c.e.y, mode, conflict))
            if conflict:
                if mode == NOTSHORT:
                    self.bridgeMasks(glyph, masks, cmasks, usedmasks, c)
                    masks = c.masks = cmasks
                elif mode == SHORT:
                    assert ns
                    newinc, _ = self.joinMasks(incompatmasks, cmasks, True)
                    self.bridgeMasks(glyph, oldmasks, newinc, usedmasks, ns)
                    masks = ns.masks = newinc
                    mode = CONFLICT
                else:
                    assert mode == CONFLICT
                    masks[:] = candmasks
            else:
                masks[:] = candmasks
                if mode == SHORT:
                    incompatmasks, _ = self.joinMasks(incompatmasks, cmasks,
                                                      False)
            c = glyph.nextForHints(c)
        if mode == SHORT:
            oldmasks[:] = masks
            masks = oldmasks
        self.bridgeMasks(glyph, masks, None, usedmasks, glyph.last())

        return usedmasks

    def buildCounterMasks(self, glyph):
        """
        For glyph dimensions that are counter-hinted, make a cntrmask
        with all Trues in that dimension (because only h/vstem3 style counter
        hints are supported)
        """
        assert not glyph.hhs.keepHints or not glyph.vhs.keepHints
        if not glyph.hhs.keepHints:
            hcmsk = [glyph.hhs.counterHinted] * len(glyph.hhs.stems[0])
        if not glyph.vhs.keepHints:
            vcmsk = [glyph.vhs.counterHinted] * len(glyph.vhs.stems[0])
        if (glyph.hhs.keepHints or glyph.vhs.keepHints) and glyph.cntr:
            cntr = []
            for cm in glyph.cntr:
                hm = cm[0] if glyph.hhs.keepHints else hcmsk
                vm = cm[1] if glyph.vhs.keepHints else vcmsk
                cntr.append([hm, vm])
        elif glyph.hhs.counterHinted or glyph.vhs.counterHinted:
            cntr = [[hcmsk, vcmsk]]
        else:
            cntr = []
        glyph.cntr = cntr

    def joinMasks(self, m, cm, log):
        """
        Try to add the stems in cm to m, or start a new mask if there are
        conflicts.
        """
        conflict = False
        nm = [None, None]
        for hv in range(2):
            hs = self.vHinter.hs if hv == 1 else self.hHinter.hs
            l = len(m[hv])
#            if hs.counterHinted:
#                nm[hv] = [True] * l
#                continue
            c = cm[hv]
            n = nm[hv] = copy(m[hv])
            if hs.keepHints:
                conflict = True in c
                continue
            assert len(c) == l
            for i in range(l):
                iconflict = ireplaced = False
                if not c[i] or n[i]:
                    continue
                # look for conflicts
                for j in range(l):
                    if not hs.hasOverlaps:
                        break
                    if j == i:
                        continue
                    if n[j] and hs.stemOverlaps[i][j]:
                        # See if we can do a ghost stem swap
                        if hs.ghostCompat[i]:
                            for k in range(l):
                                if not n[k] or not hs.ghostCompat[i][k]:
                                    continue
                                else:
                                    ireplaced = True
                                    break
                        if not ireplaced:
                            iconflict = True
                    if ireplaced:
                        break
                if not iconflict and not ireplaced:
                    n[i] = True
                elif iconflict:
                    conflict = True
                    # XXX log conflict here if log is true
        return nm, conflict

    def bridgeMasks(self, glyph, o, n, used, pe):
        """
        For switching hintmasks: Clean up o by adding compatible stems from
        mainMask and add stems from o to n when they are close to pe

        used contains a running map of which stems have ever been included
        in a hintmask
        """
        stems = [glyph.hstems, glyph.vstems]
        po = pe.e if pe.isLine() else pe.cs
        carryMask = [[False] * len(o[0]), [False] * len(o[1])]
        goodmask = [glyph.hhs.goodMask, glyph.vhs.goodMask]
        for hv in range(2):
            # Carry a previous hint forward if it is compatible and close
            # to the current pathElement
            nloc = pe.e.x if hv == 1 else pe.e.y
            for i in range(len(o[hv])):
                if not o[hv][i]:
                    continue
                dlimit = max(self.hHinter.BandMargin / 2, self.MaxHalfMargin)
                if stems[hv][i].distance(nloc) < dlimit:
                    carryMask[hv][i] = True
            # If there are no hints in o in this dimension add the closest to
            # the current path element
            if True not in o[hv]:
                oloc = po.x if hv == 1 else po.y
                try:
                    _, ms = min(((stems[hv][i].distance(oloc), i)
                                 for i in range(len(o[hv]))
                                 if goodmask[hv][i]))
                    o[hv][ms] = True
                except ValueError:
                    pass
        if self.mergeMain(glyph):
            no, _ = self.joinMasks(o, [glyph.hhs.mainMask, glyph.vhs.mainMask],
                                   False)
            o[:] = no
        for hv in range(2):
            used[hv] = [ov or uv for ov, uv in zip(o[hv], used[hv])]
        if n is not None:
            nm, _ = self.joinMasks(n, carryMask, False)
            n[:] = nm

    def mergeMain(self, glyph):
        return len(glyph.subpaths) <= 5

    def cleanupUnused(self, gllist, usedmasks):
        if (usedmasks is None or
                (False not in usedmasks[0] and False not in usedmasks[1])):
            return

        for g in gllist:
            self.delUnused([g.hstems, g.vstems], usedmasks)
        # The rest of the state at this point is in masks in gllist[0]
        glyph = gllist[0]
        if glyph.startmasks is not None:
            self.delUnused(glyph.startmasks, usedmasks)
        for c in glyph.cntr:
            self.delUnused(c, usedmasks)
        foundPEMask = False
        for c in glyph:
            if c.masks:
                foundPEMask = True
                self.delUnused(c.masks, usedmasks)
        if not foundPEMask:
            glyph.startmasks = None
            glyph.is_hm = False

    def delUnused(self, l, ml):
        """If ml[d][i] is False delete that entry from ml[d]"""
        for hv in range(2):
            l[hv][:] = [l[hv][i] for i in range(len(l[hv])) if ml[hv][i]]

    def listHintInfo(self, glyph):
        """
        Output debug messages about which stems are associated with which
        segments
        """
        for pe in glyph:
            hList = self.getSegments(glyph, pe, False)
            vList = self.getSegments(glyph, pe, True)
            if hList or vList:
                log.debug("hintlist x %g y %g" % (pe.e.x, pe.e.y))
                for seg in hList:
                    seg.hintval.show(False, "listhint")
                for seg in vList:
                    seg.hintval.show(True, "listhint")

    def remFlares(self, glyph):
        """
        When two paths are witin MaxFlare and connected by a path that
        also stays within MaxFlare, and both desire different stems,
        (sometimes) remove the lower-valued stem of the pair
        """
        for c in glyph:
            csl = self.getSegments(glyph, c)
            if not csl:
                continue
            n = glyph.nextInSubpath(c)
            cDone = False
            while c != n and not cDone:
                nsl = self.getSegments(glyph, n)
                if not nsl:
                    if not self.getSegments(glyph, n, True):
                        break
                    else:
                        n = glyph.nextInSubpath(n)
                        continue
                for cseg in csl:
                    if cseg.deleted:
                        continue
                    for nseg in nsl:
                        if nseg.deleted:
                            continue
                        diff = abs(cseg.loc - nseg.loc)
                        if diff > self.hHinter.MaxFlare:
                            cDone = True
                            continue
                        if not self.isFlare(cseg.loc, glyph, c, n):
                            cDone = True
                            continue
                        chv, nhv = cseg.hintval, nseg.hintval
                        if (diff != 0 and
                            self.isUSeg(cseg.loc, chv.uloc, chv.lloc) ==
                                self.isUSeg(nseg.loc, nhv.uloc, nhv.lloc)):
                            if (chv.compVal(self.hHinter.SpcBonus) >
                                    nhv.compVal(self.hHinter.SpcBonus)):
                                if (nhv.spc == 0 and
                                        nhv.val < self.FlareValueLimit):
                                    self.reportRemFlare(n, c, "n")
                                    nseg.suppressed = True
                            else:
                                if (chv.spc == 0 and
                                        chv.val < self.FlareValueLimit):
                                    self.reportRemFlare(c, n, "c")
                                    cseg.suppressed = True
                                    break
                n = glyph.nextInSubpath(n)

    def isFlare(self, loc, glyph, c, n):
        """Utility function for remFlares"""
        while c is not n:
            v = c.e.x if self.doV else c.e.y
            if abs(v - loc) > self.hHinter.MaxFlare:
                return False
            c = glyph.nextInSubpath(c)
        return True

    def isUSeg(self, loc, uloc, lloc):
        return abs(uloc - loc) <= abs(lloc - loc)

    def reportRemFlare(self, pe, pe2, desc):
        log.debug("Removed %s flare at %g %g by %g %g : %s" %
                  ("vertical" if self.doV else "horizontal",
                   pe.e.x, pe.e.y, pe2.e.x, pe2.e.y, desc))

    def otherInstanceStems(self, gllist):
        if len(gllist) < 2:
            return True

        glyph = gllist[0]

        # We stored the other instance stems in the default glyph's
        # hintState object, so copy them into the glyphs here.
        for i, g in enumerate(gllist[1:], 1):
            g.hstems = glyph.hhs.stems[i]
            g.vstems = glyph.vhs.stems[i]

    def otherInstanceMasks(self, gllist):
        if len(gllist) < 2:
            return True

        glyph = gllist[0]

        for g in gllist[1:]:
            g.cntr = glyph.cntr
            g.startmasks = glyph.startmasks
            g.is_hm = glyph.is_hm

        for c in self.hHinter:
            for gpe in c[1:]:
                gpe.pe.masks = c[0].pe.masks
