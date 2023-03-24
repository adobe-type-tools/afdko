# Copyright 2022 Adobe. All rights reserved.

# Portions of the code in removeOverlap are adapted from the MIT-licensed
# booleanOperations python package, located at
# https://github.com/typemytype/booleanOperations
# parts of which are also imported

from booleanOperations.flatten import InputContour, OutputContour
from booleanOperations.booleanOperationManager import clipExecute
from booleanOperations.booleanGlyph import BooleanGlyph
from fontTools.pens.pointPen import PointToSegmentPen
from .glyphData import glyphData
import logging

log = logging.getLogger(__name__)


def removeOverlap(glyph):
    """
    If the glyphData 'glyph' object has overlap, create a new glyphData
    object with the overlap removed and return it.  If it has no overlap
    return None
    """
    bg = BooleanGlyph()
    pointPen = bg.getPointPen()
    pointPen.copyContourData = True
    glyph.drawPoints(pointPen)

    inputContours = [InputContour(c) for c in bg.contours if c and len(c) > 1]
    inputFlats = [c.originalFlat for c in inputContours]
    it = 0
    while True:
        resultFlats = clipExecute(inputFlats, [], "union",
                                  subjectFillType="nonZero",
                                  clipFillType="nonZero")
        iPoints = set()
        for c in inputFlats:
            iPoints.update(c)
        rPoints = set()
        for c in resultFlats:
            rPoints.update(c)
        intersects = iPoints - rPoints
        if not intersects or it >= 8:
            break
        inputFlats = resultFlats
        it += 1
    if not intersects and it == 0:
        return None
    elif intersects:
        log.warning("Could not eliminate all overlap within 8 iterations")

    outputContours = [OutputContour(c) for c in resultFlats]
    for ic in inputContours:
        for oc in outputContours:
            if oc.final:
                continue
            if oc.reCurveFromEntireInputContour(ic):
                break
    for oc in outputContours:
        oc.reCurveSubSegments(inputContours)

    dest = glyphData(roundCoords=False, name=glyph.name)
    pointPen = PointToSegmentPen(dest)
    for oc in outputContours:
        oc.drawPoints(pointPen)
    return dest
