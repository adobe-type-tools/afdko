# Copyright 2021 Adobe. All rights reserved.

"""
Internal representation of a T2 CharString glyph with hints
"""

import numbers
import threading
import operator
from copy import deepcopy
from math import sqrt
from collections import defaultdict
from builtins import tuple as _tuple
from typing import Optional, Tuple, Union

# pytype: disable=import-error
from fontTools.misc.bezierTools import (
    solveQuadratic,
    solveCubic,
    calcCubicParameters,
    splitCubicAtT,
    segmentPointAtT,
    approximateCubicArcLength,
)
from fontTools.pens.basePen import BasePen

import logging

# from .hintstate import glyphHintState
from . import Number

log = logging.getLogger(__name__)

def norm_float(value: float) -> Number:
    """Converts a float (whose decimal part is zero) to integer"""
    if isinstance(value, float):
        value = round(value, 4)
        if value.is_integer():
            return int(value)
        return value
    return value


def feq(a: float, b: float, factor=1.52e-5) -> bool:
    """Returns True if a and b are close enough to be considered equal"""
    return abs(a - b) < factor


def fne(a: float, b: float, factor=1.52e-5) -> bool:
    """Returns True if a and b are not close enough to be considered equal"""
    return abs(a - b) >= factor


class pt:
    """A 2-tuple representing a point in 2D space"""

    tl = threading.local()
    tl.align = None

    x: Number
    y: Number

    @classmethod
    def setAlign(cls, vertical=False):
        """
        Class-level method to control the value of properties a and o.
        "a" is meant so suggest "aligned" and "o" is meant to suggest
        "opposite".

        When called with vertical==False (the default):
            a will be equivalent to x
            o will be equivalent to y
        When called with vertical==True
            a will be equivalent to y
            o will be equivalent to x

        Note that the internal align variable is thread-specific
        """
        if vertical:
            cls.tl.align = 2
        else:
            cls.tl.align = 1

    @classmethod
    def clearAlign(cls):
        """
        Class-level method to unset the internal align variable
        so that accessing properties a or o will result in an error
        """
        cls.tl.align = None

    def __init__(self, x: Number = 0, y: Number = 0, roundCoords=False):
        """
        Creates a new pt object initialied with x and y.

        If roundCoords is True the values are rounded before storing
        """
        if isinstance(x, tuple):
            y = float(x[1])
            x = float(x[0])
        if roundCoords:
            x = round(x)
            y = round(y)
        self.x = x
        self.y = y

    def __getitem__(self, ix) -> Union[int, float]:
        if ix == 0:
            return self.x
        if ix == 1:
            return self.y
        raise IndexError("pt index out of range")

    @property
    def a(self):
        """See note in setAlign"""
        if self.tl.align == 1:
            return self[0]
        elif self.tl.align == 2:
            return self[1]
        else:
            raise RuntimeError("glyphData.pt property a used without " +
                               "setting align")

    @property
    def o(self):
        """See note in setAlign"""
        if self.tl.align == 1:
            return self[1]
        elif self.tl.align == 2:
            return self[0]
        else:
            raise RuntimeError("glyphData.pt property o used without " +
                               "setting align")

    def ao(self):
        """See note in setAlign"""
        if self.tl.align == 1:
            return (self.x, self.y)
        elif self.tl.align == 2:
            return (self.y, self.x)
        else:
            raise RuntimeError("glyphData.pt method ao used without " +
                               "setting align")

    def norm_float(self):
        return pt(norm_float(self[0]), norm_float(self[1]))

    # for two pts
    def __add__(self, other):
        """
        Returns a new pt object representing the sum of respective
        values of the two arguments
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.__add__ must be pts')
        return pt(self[0] + other[0], self[1] + other[1])

    def __sub__(self, other):
        """
        Returns a new pt object representing the difference of respective
        values of the two arguments
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.__sub__ must be pts')
        return pt(self[0] - other[0], self[1] - other[1])

    def avg(self, other):
        """
        Returns a new pt object representing the average of this pt object
        with the argument
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.avg must be pts')
        return pt((self[0] + other[0]) * 0.5, (self[1] + other[1]) * 0.5)

    def dot(self, other):
        """
        Returns a numeric value representing the dot product of this
        pt object with the argument
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.dot must be pts')
        return self[0] * other[0] + self[1] * other[1]

    def scross(self, other):
        """
        Returns a numeric value representing the cross product of this
        pt object with the argument
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.dot must be pts')
        return other[1] * self[0] - other[0] * self[1]

    def distsq(self, other):
        """
        Returns a numerical value representing the squared distance
        between this pt object and the argument
        """
        if not isinstance(other, pt):
            raise TypeError('Both arguments to pt.distsq must be pts')
        dx = self[0] - other[0]
        dy = self[1] - other[1]
        return dx * dx + dy * dy

    def a_dist(self, other):
        """
        Returns a numerical value representing the distance between this
        pt object and the argument in the "a" dimension
        """
        return abs((self - other).a)

    def o_dist(self, other):
        """
        Returns a numerical value representing the distance between this
        pt object and the argument in the "o" dimension
        """
        return abs((self - other).o)

    def normsq(self):
        """Returns the squared magnitude of the pt (treated as a vector)"""
        return self[0] * self[0] + self[1] * self[1]

    def abs(self):
        """
        Returns a new pt object with the absolute values of the coordinates
        """
        return pt(abs(self[0]), abs(self[1]))

    def round(self, dec=0):
        """Returns a new pt object with rounded coordinate values"""
        return pt(round(self[0], dec), round(self[1], dec))

    # for pt and number
    def __mul__(self, other):
        """
        Returns a new pt object with this object's coordinates multiplied by
        a scalar value
        """
        if not isinstance(other, (int, float)):
            raise TypeError('One argument to pt.__mul__ must be a scalar ' +
                            'number')
        return pt(self[0] * other, self[1] * other)

    def __rmul__(self, other):
        """Same as __mul__ for right-multiplication"""
        if not isinstance(other, (int, float)):
            raise TypeError('One argument to pt.__rmul__ must be a scalar ' +
                            'number')
        return pt(self[0] * other, self[1] * other)

    def __eq__(self, other, factor=1.52e-5):
        """Returns True if each coordinate is feq to that of the argument"""
        return (feq(self[0], other[0], factor) and
                feq(self[1], other[1], factor))

    def eq_exact(self, other):
        """Returns True if each coordinate is equal to that of the argument"""
        return self[0] == other[0] and self[1] == other[1]


class stem(tuple):
    """
    A 2-tuple representing a stem hint.

    self[0] is the bottom/left coordinate
    self[1] is the top/right coordinate

    If self[1] is less than self[0] the stem represents a ghost hint
    and the difference should be 20 or 21 points
    """
    BandMargin = 30
    __slots__ = ()

    def __new__(cls, lb=0, rt=0):
        if isinstance(lb, tuple):
            return _tuple.__new__(cls, lb)
        return _tuple.__new__(cls, (lb, rt))

    @property
    def lb(self):
        """The left or bottom value, depending on stem alignment"""
        return self[0]

    @property
    def rt(self):
        """The right or top value, depending on stem alignment"""
        return self[1]

    def __str__(self):
        """Returns a string representation of the stem"""
        isgh = self.isGhost()
        isbad = self.isBad()
        if not isgh and not isbad:
            return "({0} -> {1})".format(self.lb, self.rt)
        elif isgh == "high":
            return "({0} (high ghost))".format(self.rt)
        elif isgh == "low":
            return "({0} (low ghost))".format(self.rt)
        else:
            return "(bad stem values {0}, {1})".format(self.lb, self.rt)

    def isGhost(self, doBool=False):
        """Returns True if the stem is a ghost hint"""
        width = self.rt - self.lb
        if width == -20:   # from topGhst in ac.h
            return "high" if not doBool else True
        if width == -21:   # from botGhst in ac.h
            return "low" if not doBool else True
        return False

    def ghostCompat(self, other):
        if self.isGhost(True) == other.isGhost(True):
            return False
        if self.isGhost() == 'high' or other.isGhost() == 'high':
            return self.lb == other.rt
        else:
            return self.lb == other.lb

    def isBad(self):
        """Returns True if the stem is malformed"""
        return not self.isGhost() and (self.rt - self.lb) < 0

    def relVals(self, last=None):
        """
        Returns a tuple of "relative" stem values (start relative to
        the passed last stem, then width) appropriate for
        vstem/hstem/vstemhm/hstemhm output
        """
        if last:
            l = last.rt
        else:
            l = 0
        return (norm_float(self.lb - l), norm_float(self.rt - self.lb))

    def UFOVals(self):
        """Returns a tuple of stem values appropriate for UFO output"""
        return (self.lb, self.rt - self.lb)

    def overlaps(self, other):
        """
        Returns True if this stem is within BandMargin of overlapping the
        passed stem
        """
        lloc, uloc = self.lb, self.rt
        sig = self.isGhost()
        # c.f. page 22 of Adobe TN #5177 "The Type 2 Charstring Format"
        if sig == 'high':
            uloc = lloc
        elif sig == 'low':
            lloc = uloc
        olloc, ouloc = other.lb, other.rt
        oig = other.isGhost()
        if oig == 'high':
            ouloc = olloc
        elif oig == 'low':
            olloc = ouloc
        uloc += stem.BandMargin
        lloc -= stem.BandMargin
        return olloc <= uloc and ouloc >= lloc

    def distance(self, loc):
        """
        Returns the distance between this stem and the passed location,
        which is zero if the location falls within the stem
        """
        lloc, uloc = self.lb, self.rt
        sig = self.isGhost()
        # c.f. page 22 of Adobe TN #5177 "The Type 2 Charstring Format"
        if sig == 'high':
            uloc = lloc
        elif sig == 'low':
            lloc = uloc
        if loc >= lloc and loc <= uloc:
            return 0
        return min(abs(lloc - loc), abs(uloc - loc))


class boundsState:
    """
    Calculates and stores the bounds of a pathElement (spline) and the point
    locations that define the boundaries.
    """
    def __init__(self, c):
        """
        Initialize the object with the passed pathElement and calculate the
        bounds
        """
        # Start with the bounds of the line betwen the endpoints
        self.lb = self.linearBounds(c)
        if c.is_line:
            self.bounds = self.lb
        else:
            # For curves, add the control points into the calcuation.
            # If they exceed the linear bounds calculate the proper
            # curve bounds
            cpb = deepcopy(self.lb)
            self.mergePt(cpb, c.cs, .25, doExt=False)  # t value isn't used
            self.mergePt(cpb, c.ce, .75, doExt=False)  # t value isn't used
            if self.lb != cpb:
                self.calcCurveBounds(c)
            else:
                self.bounds = self.lb

    def mergePt(self, b, p, t, doExt=True):
        """
        Add the passed point into the bounds as a potential extreme.

        If it is an extreme
            store the point at the appropriate extpts subscripts
            store the t value at the same tmap subscripts
        """
        for i, cmp_o in enumerate([operator.lt, operator.gt]):
            for j in range(2):
                if cmp_o(p[j], b[i][j]):
                    b[i][j] = p[j]
                    if doExt:
                        self.extpts[i][j] = p
                        self.tmap[i][j] = t

    def linearBounds(self, c):
        """
        Calculate the bounds of the line betwen the start and end points of
        the passed pathElement.
        """
        self.tmap = [[0, 0], [0, 0]]
        self.extpts = [[c.s, c.s], [c.s, c.s]]
        lb = [[*c.s], [*c.s]]
        self.mergePt(lb, c.e, 1)
        return lb

    def calcCurveBounds(self, pe):
        """
        Calculate the bounds of the passed path element relative to the
        already-calculated linear bounds
        """
        curveb = deepcopy(self.lb)
        a, b, c, d = pe.cubicParameters()
        roots = []
        for dim in range(2):
            roots += [t for t in solveQuadratic(a[dim] * 3, b[dim] * 2, c[dim])
                      if 0 <= t < 1]

        for t in roots:
            p = pt(a[0] * t * t * t + b[0] * t * t + c[0] * t + d[0],
                   a[1] * t * t * t + b[1] * t * t + c[1] * t + d[1])
            self.mergePt(curveb, p, t)

        self.bounds = curveb

    def farthestExtreme(self, doY=False):
        """
        Returns the location, defining point, and t value for the
        bound farthest from the linear bounds in the dimension selected
        with doY. If the linear bounds are the curve bounds returns
        0, None, None

        The fourth return value is False if the defining point's location
        is less than the linear bound and True if it is greater
        """
        idx = 1 if doY else 0
        d = [abs(self.lb[i][idx] - self.bounds[i][idx]) for i in range(2)]
        if d[0] > d[1]:
            return d[0], self.extpts[0][idx], self.tmap[0][idx], False
        elif fne(d[1], 0):
            return d[1], self.extpts[1][idx], self.tmap[1][idx], True
        else:
            return 0, None, None, None

    def intersects(self, other, margin=0):
        """
        Returns True if the bounds of this object are within those of
        the argument
        """
        return (self.bounds[0][0] <= other.bounds[1][0] + margin and
                self.bounds[1][0] + margin >= other.bounds[0][0] and
                self.bounds[0][1] <= other.bounds[1][1] + margin and
                self.bounds[1][1] + margin >= other.bounds[0][1])


class pathBoundsState:
    """
    Calculates and stores the bounds of a glyphData object (path) and
    the pathElements (splines) that define the boundaries.
    """
    def __init__(self, pe):
        """Initialize the bounds with those of a single pathElement"""
        self.bounds = pe.getBounds().bounds
        self.extpes = [[pe, pe], [pe, pe]]

    def merge(self, other):
        """Merge this pathBoundsState object with the bounds of another"""
        for i, cmp_o in enumerate([operator.lt, operator.gt]):
            for j in range(2):
                if cmp_o(other.bounds[i][j], self.bounds[i][j]):
                    self.bounds[i][j] = other.bounds[i][j]
                    self.extpes[i][j] = other.extpes[i][j]

    def within(self, other):
        """
        Returns True if the bounds of this object are within those of
        the argument
        """
        return (self.bounds[0][0] >= other.bounds[0][0] and
                self.bounds[0][1] >= other.bounds[0][1] and
                self.bounds[1][0] <= other.bounds[1][0] and
                self.bounds[1][1] <= other.bounds[1][1])


class pathElement:
    """
    Stores the coordinates of a spline (line or curve) and
        hintmask values to add directly before the spline
        Whether the spline is the first or second part of a flex hint
        a boundsState object for the spline
        The position (subpath, offset) of the spline in the glyphData path

        self.s is the first point, self.e is the last.
        If the spline is a curve self.cs is the first control point and
        self.ce is the second.

        When segment_sub is not None it must either be a list or an
        integer. If it is a list then that list must contain pathElements,
        representing the same path (in order). These will be used in place
        of the pathElement during segment generation. Each of these
        substitution pathElements must have segment_sub equal to the
        offset in the parent's segment_sub list.
    """
    assocMatchFactor = 93
    tSlop = .005

    def __init__(self, *args, is_close=False, masks=None, flex=False,
                 position=None):
        self.is_line = False
        self.is_close = is_close
        for p in args:
            if not isinstance(p, pt):
                raise TypeError('Positional arguments to ' +
                                'pathElement.__init__ must be pt objects')
        if len(args) == 2:
            self.is_line = True
            self.s = args[0]
            self.e = args[1]
        elif len(args) == 4:
            if is_close:
                raise TypeError('is_close must be False for path curves')
            self.s = args[0]
            self.cs = args[1]
            self.ce = args[2]
            self.e = args[3]
        else:
            raise TypeError('Wrong number of positional arguments to ' +
                            'pathElement.__init__')
        self.masks = masks
        self.flex = flex
        self.bounds = None
        self.position: Tuple[int, int] = position or (-1, -1)
        self.segment_sub = None

    def getBounds(self):
        """Returns the bounds object for the object, generating it if needed"""
        if self.bounds:
            return self.bounds
        self.bounds = boundsState(self)
        return self.bounds

    def clearTempState(self):
        self.bounds = None
        self.segment_sub = None

    def isLine(self):
        """Returns True if the spline is a line"""
        return self.is_line

    def isClose(self):
        """Returns True if this pathElement implicitly closes a subpath"""
        return self.is_close

    def isStart(self):
        """Returns True if this pathElement starts a subpath"""
        assert self.position is not None
        return self.position[1] == 0

    def isTiny(self):
        """
        Returns True if the start and end points of the spline are within
        two em-units in both dimensions
        """
        d = (self.e - self.s).abs()
        return d.x < 2 and d.y < 2

    def isShort(self):
        """
        Returns True if the start and end points of the spline are within
        about six em-units
        """
        d = (self.e - self.s).abs()
        mx, mn = sorted(tuple(d))
        return mx + mn * .336 < 6  # head.c IsShort

    def convertToLine(self):
        """
        If the pathElement is not already a line, make it one with the same
        start and end points
        """
        if self.is_line:
            return
        if self.flex is not None:
            log.error("Cannot convert flex-hinted curve to line: skipping.")
            return
        self.is_line = True
        del self.cs
        del self.ce
        self.bounds = None

    def convertToCurve(self, sRatio=.333333, eRatio=None, roundCoords=False):
        """
        If the pathElement is not already a curve, make it one. The control
        points are made colinear to preseve the shape. self.cs will be
        positioned at ratio sRatio from self.s and self.ce will be positioned
        at eRatio away from self.e
        """
        if not self.is_line:
            return
        if eRatio is None:
            eRatio = sRatio
        self.is_line = False
        self.cs = self.s * (1 - sRatio) + self.e * sRatio
        self.ce = self.s * eRatio + self.e * (1 - eRatio)

    def clearHints(self, doVert=False):
        """Clear the vertical or horizontal masks, if any"""
        if doVert and self.masks is not None:
            self.masks = [self.masks[0], None] if self.masks[0] else None
        elif not doVert and self.masks is not None:
            self.masks = [None, self.masks[1]] if self.masks[1] else None

    def cubicParameters(self):
        """Returns the fontTools cubic parameters for this pathElement"""
        return calcCubicParameters(self.s, self.cs, self.ce, self.e)

    def getAssocFactor(self):
        if self.is_line:
            l = sqrt(self.s.distsq(self.e))
        else:
            l = approximateCubicArcLength(self.s, self.cs, self.ce, self.e)
        return l / self.assocMatchFactor

    def containsPoint(self, p, factor, returnT=False):
        if self.is_line:
            # We sometimes want t anyway, so why not?
            ds = sqrt(self.s.distsq(self.e))
            d1 = sqrt(self.s.distsq(p))
            d2 = sqrt(self.e.distsq(p))
            iseq = feq(ds, d1 + d2, factor)
            if not returnT:
                return iseq
            else:
                return iseq, d1 / ds
        a, b, c, d = self.cubicParameters()
        if abs(self.s.x - self.e.x) > abs(self.s.y - self.e.y):
            i, j = 0, 1
        else:
            i, j = 1, 0
        tl = solveCubic(a[i], b[i], c[i], d[i] - p[i])
        sols = [(a[j] * t * t * t + b[j] * t * t + c[j] * t + d[j], t)
                for t in tl if -.1 <= t <= 1.1]
        tsols = [t for v, t in sols if feq(v, p[j], factor)]
        if not returnT:
            return len(tsols) > 0
        if len(tsols) > 0:
            return True, tsols[0]
        else:
            return False, 0

    def slopePoint(self, t):
        """
        Returns the point definiing the slope of the pathElement
        (relative to the on-curve point) at t==0 or t==1
        """
        if t == 1:
            if self.is_line:
                return self.s
            if self.e == self.ce:
                return self.cs
            return self.ce
        else:
            if self.is_line:
                return self.e
            if self.s == self.cs:
                return self.ce
            return self.cs

    def __deepcopy__(self, memo):
        """Don't deepcopy pathElement objects"""
        return self

    @staticmethod
    def stemBytes(masks):
        """Calculate bytes corresponding to a (boolean array) hintmask"""
        t = masks[0] + masks[1]
        lenb = len(t)
        lenB = (lenb + 7) // 8
        t += [False for i in range(lenB * 8 - lenb)]
        return int(''.join(('1' if i else '0' for i in t)),
                   2).to_bytes(lenB, byteorder='big')

    def relVals(self):
        """
        Return relative coordinates appropriate for an rLineTo or
        rCurveTo T2 operator
        """
        if self.is_line:
            r = self.e - self.s
            return [*r]
        else:
            r1 = self.cs - self.s
            r2 = self.ce - self.cs
            r3 = self.e - self.ce
            return [*r1, *r2, *r3]

    def T2(self, is_start=None):
        """Returns an array of T2 operators corresponding to the pathElement"""
        prog = []

        if is_start:
            if is_start[1].masks:
                prog.extend(['hintmask',
                             pathElement.stemBytes(is_start[1].masks)])
            rmt = self.s - is_start[0]
            prog.extend([*rmt, 'rmoveto'])

        if self.masks and self.flex == 2:
            log.warning("Hintmask added in middle of flex hint: ignoring")
        elif self.masks and not self.is_close:
            prog.extend(['hintmask', pathElement.stemBytes(self.masks)])

        rv = self.relVals()

        after = self.e

        if self.is_close:
            after = self.s
        elif self.flex == 1:
            prog.extend(rv)
        elif self.flex == 2:
            prog.extend(rv)
            prog.extend([50, 'flex'])  # 50 from gDMin in C implementation
        elif not self.is_line:
            prog.extend(rv)
            prog.append('rrcurveto')
        else:
            prog.extend(rv)
            prog.append('rlineto')

        return prog, after

    def splitAtInflectionsForSegs(self):
        a, b, c, d, = self.cubicParameters()
        t2c = 3 * (b[0] * a[1] - a[0] * b[1])
        t1c = 3 * (c[0] * a[1] - a[0] * c[1])
        t0c = (c[0] * b[1] - b[0] * c[1])
        sols = [t for t in solveQuadratic(t2c, t1c, t0c) if 0 < t < 1]
        if len(sols) > 0:
            self.segment_sub = []
            for s in splitCubicAtT(self.s, self.cs, self.ce, self.e, *sols):
                pts = [pt(tup) for tup in s]
                spe = pathElement(*pts)
                spe.position = self.position
                spe.segment_sub = len(self.segment_sub)
                self.segment_sub.append(spe)
            return True
        return False

    def splitAt(self, t):
        if self.is_line:
            pb = self.s + (self.e - self.s) * t
            ret = pathElement(pb, self.e, position=self.position,
                              is_close=self.is_close)
            self.e = pb
            self.is_close = False
            self.bounds = None
            return ret
        else:
            s, n = splitCubicAtT(self.s, self.cs, self.ce, self.e, t)
            ptsn = [pt(p) for p in n]
            ret = pathElement(*ptsn, position=self.position,
                              is_close=self.is_close)
            assert s[0] == self.s and n[3] == ret.e
            self.cs = pt(s[1])
            self.ce = pt(s[2])
            self.e = pt(s[3])
            self.is_close = False
            self.bounds = None
            return ret

    def atT(self, t):
        return pt(segmentPointAtT(self.fonttoolsSegment(), t))

    def fonttoolsSegment(self):
        if self.is_line:
            return [tuple(self.s), tuple(self.e)]
        else:
            return [tuple(self.s), tuple(self.cs), tuple(self.ce), tuple(self.e)]


class glyphData(BasePen):
    """Stores state corresponding to a T2 CharString"""

    def __init__(self, roundCoords, name=''):
        super().__init__()
        self.roundCoords = roundCoords

        self.subpaths = []
        self.hstems = []
        self.vstems = []
        self.startmasks = None
        self.cntr = []
        self.name = name
        self.wdth = None

        self.is_hm = None
        self.flex_count = 0
        self.lastcp = None

        self.nextmasks = None
        self.nextflex = None
        self.changed = False
        self.pathEdited = False
        self.boundsMap = {}

        self.hhs: Optional[object] = None
        self.vhs: Optional[object] = None

    # pen methods:

    def _moveTo(self, ptup):
        """moveTo pen method"""
        self.lastcp = None
        self.subpaths.append([])

    def _lineTo(self, ptup):
        """lineTo pen method"""
        self.lastcp = None
        curpt = pt(self._getCurrentPoint(), roundCoords=self.roundCoords)
        newpt = pt(ptup, roundCoords=self.roundCoords)
        self.subpaths[-1].append(pathElement(curpt, newpt,
                                             masks=self.getStemMasks(),
                                             flex=self.checkFlex(False),
                                             position=self.getPosition()))

    def _curveToOne(self, ptup1, ptup2, ptup3):
        """lineTo pen method"""
        self.lastcp = None
        curpt = pt(self._getCurrentPoint(), roundCoords=self.roundCoords)
        newpt1 = pt(ptup1, roundCoords=self.roundCoords)
        newpt2 = pt(ptup2, roundCoords=self.roundCoords)
        newpt3 = pt(ptup3, roundCoords=self.roundCoords)
        self.subpaths[-1].append(pathElement(curpt, newpt1, newpt2, newpt3,
                                             masks=self.getStemMasks(),
                                             flex=self.checkFlex(True),
                                             position=self.getPosition()))

    # closePath is a courtesy of the caller, not an instruction, so
    # we rely on its semantics here
    def _closePath(self):
        """closePath (courtesy) pen method"""
        curpt = pt(self._getCurrentPoint(), roundCoords=self.roundCoords)
        if len(self.subpaths[-1]) == 0:  # No content after moveTo
            self.subpaths.pop()
            return
        t = self.subpaths[-1][0].s
        if curpt.eq_exact(t):
            return
        self.subpaths[-1].append(pathElement(curpt, t,
                                             masks=self.getStemMasks(),
                                             flex=self.checkFlex(False),
                                             is_close=True,
                                             position=self.getPosition()))
        self.lastcp = self.subpaths[-1][-1]

    def getPosition(self):
        """Returns position (subpath idx, offset) of next spline to be drawn"""
        return (len(self.subpaths) - 1, len(self.subpaths[-1]))

    # "hintpen" methods:
    def nextIsFlex(self):
        """quasi-pen method noting that next spline starts a flex hint"""
        self.flex_count += 1
        self.nextflex = 1

    def hStem(self, data, is_hm):
        """
        quasi-pen method to pass horizontal stem data (in relative format)
        """
        if self.is_hm is not None and self.is_hm != is_hm:
            # XXX Should refactor so this prints as warning when
            # using existing hint data
            log.info("Horizontal stem hints mask setting does not match" +
                     " previous setting")
        self.is_hm = is_hm
        self.hstems.extend(self.toStems(data))

    def vStem(self, data, is_hm):
        """quasi-pen method passing vertical stem data (in relative format)"""
        if is_hm is not None:
            if self.is_hm is not None and self.is_hm != is_hm:
                # XXX Should refactor so this prints as warning when
                # using existing hint data
                log.info("Vertical stem hints mask setting does not match" +
                         " previous setting")
            self.is_hm = is_hm
        self.vstems.extend(self.toStems(data))

    def hintmask(self, hhints, vhints):
        """quasi-pen method passing hintmask data"""
        if not self.is_hm:
            # XXX Should refactor so this prints as warning when
            # using existing hint data
            log.info("Hintmask found when stems weren't marked as having" +
                     " hintmasks")
            self.is_hm = True
        if len(self.subpaths) == 0:
            self.startmasks = [hhints, vhints]
        else:
            if not self.startmasks and not self.cntr:
                # XXX Should refactor so this prints as warning when
                # using existing hint data
                log.info("Initial hintmask missing in current glyph %s" %
                         self.name)
            # In the glyphdata format the end of a path is implicit in the
            # charstring but explicit in the subpath, while a moveto is
            # explicit in the charstring and implicit in the subpath. So
            # we record mid-stream hintmasks prior to movetos in the
            # prior "is_close" line. When writing out these will wind up
            # in the same place in the stream.
            if self.lastcp:
                self.lastcp.masks = [hhints, vhints]
            else:
                self.nextmasks = [hhints, vhints]

    def cntrmask(self, hhints, vhints):
        """quasi-pen method passing cntrmask data"""
        self.cntr.append([hhints, vhints])

    # width
    def setWidth(self, width):
        self.wdth = width

    def getWidth(self):
        return self.wdth

    # status
    def isEmpty(self):
        """Returns true if there are no subpaths"""
        return not len(self.subpaths) > 0

    def hasFlex(self):
        """Returns True if at least one curve pair is flex-hinted"""
        return self.flex_count > 0

    def hasHints(self, doVert=False, both=False, either=False):
        """
        Returns True if there are hints of the parameter-specified type(s)
        """
        if both:
            return len(self.vstems) > 0 and len(self.hstems) > 0
        elif either:
            return len(self.vstems) > 0 or len(self.hstems) > 0
        elif doVert:
            return len(self.vstems) > 0
        else:
            return len(self.hstems) > 0

    def syncPositions(self):
        """
        Reset the pathElement.position tuples if the path has been edited
        """
        if self.pathEdited:
            for sp in range(len(self.subpaths)):
                for ofst in range(len(self.subpaths[sp])):
                    pe = self.subpaths[sp][ofst]
                    pe.position = (sp, ofst)
                    if isinstance(pe.segment_sub, list):
                        for i, spe in enumerate(pe.segment_sub):
                            spe.position = (sp, ofst)
                            spe.segment_sub = i

    def setPathEdited(self):
        self.pathEdited = True
        self.boundsMap = {}

    def getBounds(self, subpath=None):
        """
        Returns the bounds of the specified subpath, or of the whole
        path if subpath is None
        """
        b = self.boundsMap.get(subpath, None)
        if b is not None:
            return b
        if len(self.subpaths) == 0:
            return None
        if subpath is None:
            b = deepcopy(self.getBounds(0))
            for i in range(1, len(self.subpaths)):
                b.merge(self.getBounds(i))
            self.boundsMap[subpath] = b
            return b
        else:
            b = pathBoundsState(self.subpaths[subpath][0])
            for c in self.subpaths[subpath][1:]:
                b.merge(pathBoundsState(c))
            self.boundsMap[subpath] = b
            return b

    def T2(self, version=1):
        """Returns an array of T2 operators corresponding to the object"""
        prog = []

        if self.hstems:
            prog.extend(self.fromStems(self.hstems))
            prog.append('hstemhm' if self.is_hm else 'hstem')
        if self.vstems:
            prog.extend(self.fromStems(self.vstems))
            if not self.is_hm:
                prog.append('vstem')
            elif len(self.cntr) == 0 and self.startmasks is None:
                prog.append('vstemhm')

        for c in self.cntr:
            prog.extend(['cntrmask', pathElement.stemBytes(c)])
        if self.startmasks:
            prog.extend(['hintmask', pathElement.stemBytes(self.startmasks)])

        curpt = pt(0, 0)

        for c in self:
            is_start = None
            if c.isStart():
                is_start = (curpt, self.subpaths[c.position[0]][-1])
            c_t2, curpt = c.T2(is_start=is_start)
            prog.extend(c_t2)

        if version == 1:
            prog.append('endchar')
        return prog

    def drawPoints(self, pen, ufoH=None):
        """
        Calls pointPen commands on pen to draw the glyph, optionally naming
        some points and building a library of hint annotations
        """
        doHints = ufoH is not None and (self.hasFlex() or
                                        self.hasHints(either=True))
        pln, pn = 0, None
        for s in self.subpaths:
            wrapi = len(s) - 1
            if wrapi < 0:
                continue
            w = s[wrapi]
            assert w.e == s[0].s
            if w.e == w.s:
                wrapi -= 1
                w = s[wrapi]
            pen.beginPath()
            wt = 'line' if w.isLine() else 'curve'
            if doHints:
                pln, pn = ufoH(w, pln, True)
            pen.addPoint((w.e.x, w.e.y), segmentType=wt, name=pn)
            for i in range(0, wrapi):
                c = s[i]
                if doHints:
                    pln, pn = ufoH(c, pln)
                if c.isLine():
                    pen.addPoint((c.e.x, c.e.y), segmentType="line", name=pn)
                else:
                    pen.addPoint((c.cs.x, c.cs.y), name=pn)
                    pen.addPoint((c.ce.x, c.ce.y))
                    pen.addPoint((c.e.x, c.e.y), segmentType="curve")
            if not w.isLine():
                if doHints:
                    pln, pn = ufoH(w, pln)
                pen.addPoint((w.cs.x, w.cs.y), name=pn)
                pen.addPoint((w.ce.x, w.ce.y))
            pen.endPath()

    # XXX deal with or avoid reordering when preserving any hints
    def reorder(self, neworder):
        """Change the order of subpaths according to neworder"""
        log.debug("Reordering subpaths: %r" % neworder)
        spl = self.subpaths
        assert len(neworder) == len(spl)
        self.subpaths = [spl[i] for i in neworder]
        self.setPathEdited()

    def first(self):
        """Returns the first pathElement of the path"""
        if self.subpaths and self.subpaths[0]:
            return self.subpaths[0][0]
        return None

    def last(self):
        """Returns the last (implicit close) pathElement of the path"""
        if self.subpaths and self.subpaths[-1]:
            return self.subpaths[-1][-1]
        return None

    def next(self, c, segSub=False):
        """
        If c == self, returns the first elemeht of the path

        If c is a pathElement, returns the following element of the path
        or None if there is no such element
        """
        if c is None:
            return None
        if c is self:
            if self.subpaths and self.subpaths[0]:
                t = self.subpaths[0][0]
                if not segSub or not isinstance(t.segment_sub, list):
                    return t
                else:
                    return t.segment_sub[0]
            return None
        assert ((segSub or not isinstance(c.segment_sub, int)) and
                (not segSub or not isinstance(c.segment_sub, list)))
        self.syncPositions()
        subpath, offset = c.position
        if isinstance(c.segment_sub, int):
            suboff = c.segment_sub + 1
            t = self.subpaths[subpath][offset]
            if len(t.segment_sub) > suboff:
                return t.segment_sub[suboff]
        offset += 1
        if offset < len(self.subpaths[subpath]):
            t = self.subpaths[subpath][offset]
            if not segSub or not isinstance(t.segment_sub, list):
                return t
            else:
                return t.segment_sub[0]
        subpath += 1
        offset = 0
        if subpath < len(self.subpaths):
            t = self.subpaths[subpath][offset]
            if not segSub or not isinstance(t.segment_sub, list):
                return t
            else:
                return t.segment_sub[0]
        return None

    # Iterate through path elements but start each subpath
    # with implicit closing line (if any). Skip pro forma
    # (zero-length) close elements
    def nextForHints(self, c):
        """
        Like next() but returns the next element in "hint order", with
        implicit close elements coming first in the subpath instead of
        last
        """
        if c is None:
            return None
        if c is self:
            if not self.subpaths or not self.subpaths[0]:
                return None
            c = self.subpaths[0][-1]
            if not c.isClose():
                c = self.subpaths[0][0]
            return c
        self.syncPositions()
        subpath, offset = c.position
        if c.isClose():
            n = self.subpaths[subpath][0]
            if n == c:
                return None
            return n
        offset += 1
        if offset < len(self.subpaths[subpath]) - 1:
            c = self.subpaths[subpath][offset]
            if not c.isClose():
                return c
        subpath += 1
        if subpath < len(self.subpaths):
            c = self.subpaths[subpath][-1]
            if not c.isClose():
                c = self.subpaths[subpath][0]
            return c
        return None

    def inSubpath(self, c, i, skipTiny, closeWrapOK, segSub):
        """Utility function for nextInSubpath and prevInSubpath"""
        if c is None or (c is self and i == -1):
            return None
        if c is self:
            return self.next(c, segSub)
        assert ((segSub or not isinstance(c.segment_sub, int)) and
                (not segSub or not isinstance(c.segment_sub, list)))
        self.syncPositions()
        subpath, offset = c.position
        sp = self.subpaths[subpath]
        l = len(sp)
        o = offset
        while True:
            done = False
            if isinstance(c.segment_sub, int):
                suboff = c.segment_sub + i
                t = self.subpaths[subpath][o]
                if 0 <= suboff < len(t.segment_sub):
                    c = t.segment_sub[suboff]
                    done = True
            if not done:
                o = (o + i) % l
                if o == offset:
                    return None
                if segSub and isinstance(sp[o].segment_sub, list):
                    if i < 0:
                        suboff = len(sp[o].segment_sub) + i
                    else:
                        suboff = 0
                    c = sp[o].segment_sub[suboff]
                else:
                    c = sp[o]
            if not skipTiny or not c.isTiny():
                break
            if (not closeWrapOK and
                    ((i > 0 and o < offset) or (i < 0 and o > offset))):
                return None
        return c

    def nextInSubpath(self, c, skipTiny=False, closeWrapOK=True, segSub=False):
        """
        Returns the next element in the subpath after c.

        If c is the last element and closeWrapOK is True returns the
        first element of the subpath.

        If c is the last element and closeWrapOK is False returns None
        """
        return self.inSubpath(c, 1, skipTiny, closeWrapOK, segSub)

    def prevInSubpath(self, c, skipTiny=False, closeWrapOK=True, segSub=False):
        """
        Returns the previous element in the subpath before c.

        If c is the first element and closeWrapOK is True returns the
        last element of the subpath.

        If c is the first element and closeWrapOK is False returns None
        """
        return self.inSubpath(c, -1, skipTiny, closeWrapOK, segSub)

    def nextSlopePoint(self, c):
        """Returns the slope point of the element of the subpath after c"""
        n = self.nextInSubpath(c, skipTiny=True, segSub=True)
        return None if n is None else n.slopePoint(0)

    def prevSlopePoint(self, c):
        """Returns the slope point of the element of the subpath before c"""
        p = self.prevInSubpath(c, skipTiny=True, segSub=True)
        return None if p is None else p.slopePoint(1)

    class glyphiter:
        """An iterator for a glyphData path"""

        __slots__ = ('gd', 'pos')

        def __init__(self, gd):
            self.gd = self.pos = gd

        def __next__(self):
            self.pos = self.gd.next(self.pos)
            if self.pos is None:
                raise StopIteration
            return self.pos

        def __iter__(self):
            return self

    def __iter__(self):
        return self.glyphiter(self)

    # utility

    def checkAssocPoint(self, segs, spe, ope, sp, op, mapEnd, factor=None):
        if factor is None:
            factor = ope.getAssocFactor()
        if sp == op or ope.containsPoint(sp, factor):
            if not ope.containsPoint(spe.atT(.5), factor):
                return False
            spe.association = ope
            return True
        else:
            does, t = spe.containsPoint(op, factor, True)
            if does and spe.tSlop < t < 1 - spe.tSlop:
                midMapped = (1 - (1 - t) / 2) if mapEnd else t / 2
                if not ope.containsPoint(spe.atT(midMapped), factor):
                    return False
                following = spe.splitAt(t)
                if mapEnd:
                    following.association = ope
                    sl = self.subpaths[spe.position[0]]
                    sl.insert(sl.index(spe) + 1, following)
                    segs.add(spe)
                else:
                    spe.association = ope
                    sl = self.subpaths[spe.position[0]]
                    sl.insert(sl.index(spe), following)
                    segs.add(following)
                self.setPathEdited()
                return True
        return False

    def associatePath(self, orig):
        peMap = defaultdict(list)
        for oc in orig:
            peMap[tuple(oc.e.round(1))].append(oc)
        segs = set()
        for s in self.subpaths:
            segs.update(s)
        # The position[0] subpath pointers for self pathElements should stay
        # accurate during the loop but the position[1] indexes won't.
        while segs:
            # Check a segment with the same start point
            c = segs.pop()
            done = False
            oepel = peMap.get(tuple(c.e.round(1)), [])
            if oepel:
                for oepe in oepel:
                    if self.checkAssocPoint(segs, c, oepe, c.s, oepe.s, True):
                        done = True
                        break
                    else:
                        oepen = orig.nextInSubpath(oepe)
                        if self.checkAssocPoint(segs, c, oepen, c.s, oepen.e,
                                                True):
                            done = True
                            break
                if done:
                    continue
            ospel = peMap.get(tuple(c.s.round(1)), [])
            if ospel:
                for ospe in ospel:
                    ospen = orig.nextInSubpath(ospe)
                    if self.checkAssocPoint(segs, c, ospen, c.e, ospen.e,
                                            False):
                        done = True
                        break
                    elif self.checkAssocPoint(segs, c, ospe, c.e, ospe.s,
                                              False):
                        done = True
                        break
                if done:
                    continue
            cBounds = c.getBounds()
            for oc in orig:
                factor = oc.getAssocFactor()
                if cBounds.intersects(oc.getBounds(), factor):
                    if (oc.containsPoint(c.s, factor) and
                            (self.checkAssocPoint(segs, c, oc, c.e, oc.e,
                                                  False, factor) or
                             self.checkAssocPoint(segs, c, oc, c.e, oc.s,
                                                  False, factor))):
                        done = True
                        break
            if done:
                continue
            # Didn't find the start point anywhere, probably because it
            # was very close to but not quite identical to another start point.
            # Try again from the other end.
            for oc in orig:
                factor = oc.getAssocFactor()
                if cBounds.intersects(oc.getBounds(), factor):
                    if (oc.containsPoint(c.e, factor) and
                            (self.checkAssocPoint(segs, c, oc, c.s, oc.s,
                                                  True, factor) or
                             self.checkAssocPoint(segs, c, oc, c.s, oc.e,
                                                  True, factor))):
                        done = True
                        break
            if done:
                continue
            assert not hasattr(c, 'association')
            log.warning("Unable to map derived path element from " +
                        "%s to %s" % (c.s, c.e))
            c.association = None
        self.syncPositions()

    def addNullClose(self, si):
        sp = self.subpaths[si]
        assert not sp[-1].isClose()
        pe = pathElement(sp[0].s, sp[0].s, is_close=True,
                         position=(si, len(sp)))
        sp.append(pe)

    def getStemMasks(self):
        """Utility function for pen methods"""
        s = self.nextmasks
        self.nextmasks = None
        return s

    def checkFlex(self, is_curve):
        """Utility function for pen methods"""
        if self.nextflex is None:
            return None
        elif not is_curve:
            log.warning("Line marked with flex hint: ignoring")
            self.nextflex = None
            return None
        elif self.nextflex == 1:
            self.nextflex = 2
            return 1
        elif self.nextflex == 2:
            self.nextflex = None
            return 2
        else:
            log.error("Internal error: Bad flex value")
            self.nextflex = None
            return None

    def toStems(self, data):
        """Converts relative T2 charstring stem data to stem object array"""
        high = 0
        sl = []
        for i in range(len(data) // 2):
            low = high + data[i * 2]
            high = low + data[i * 2 + 1]
            sl.append(stem(low, high))  # pytype: disable=wrong-arg-count
        return sl

    def fromStems(self, stems):
        """Converts stem array to relative T2 charstring stem data"""
        l = None
        data = []
        for s in stems:
            data.extend(s.relVals(last=l))
            l = s
        return data

    def clearFlex(self):
        """Clears any flex hints"""
        if self.flex_count == 0:
            return
        for c in self:
            c.flex = None
        self.flex_count = 0

    def clearHints(self, doVert=False):
        """Clears stem hints in specified dimension"""
        if doVert:
            self.vstems = []
            if self.startmasks and self.startmasks[0]:
                self.startmasks = [self.startmasks[0], None]
            else:
                self.startmasks = None
        else:
            self.hstems = []
            if self.startmasks and self.startmasks[1]:
                self.startmasks = [None, self.startmasks[1]]
            else:
                self.startmasks = None
        cntr = []
        for cn in self.cntr:
            if doVert and cn[0]:
                cntr.append((cn[0], None))
            elif not doVert and cn[1]:
                cntr.append((None, cn[1]))
        self.cntr = cntr
        for c in self:
            c.clearHints(doVert=doVert)

    def clearTempState(self):
        self.hhs = self.vhs = None
        for c in self:
            c.clearTempState()
