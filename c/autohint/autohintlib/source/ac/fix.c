/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* fix.c */


#include "ac.h"
#include "machinedep.h"

#define maxFixes (100)
private Fixed HFixYs[maxFixes], HFixDYs[maxFixes];
private Fixed VFixXs[maxFixes], VFixDXs[maxFixes];
private integer HFixCount, VFixCount;
private Fixed bPrev, tPrev;

public procedure InitFix(reason) integer reason; {
  switch (reason) {
    case STARTUP: case RESTART:
      HFixCount = VFixCount = 0;
      bPrev = tPrev = FixedPosInf;
    }
  }

private procedure RecordHFix(y,dy) Fixed y, dy; {
  HFixYs[HFixCount] = y;
  HFixDYs[HFixCount] = dy;
  HFixCount++;
  }

private procedure RecordVFix(x,dx) Fixed x, dx; {
  VFixXs[VFixCount] = x;
  VFixDXs[VFixCount] = dx;
  VFixCount++;
  }

private procedure RecordForFix(vert, w, minW, b, t)
  boolean vert; Fixed w, minW, b, t; {
  Fixed mn, mx, delta;
  if (b < t) { mn = b; mx = t; }
  else { mn = t; mx = b; }
  if (!vert && HFixCount+4 < maxFixes && autoHFix) {
    Fixed fixdy = w - minW;
    if (abs(fixdy) <= FixOne) {
      RecordHFix(mn, fixdy);
      RecordHFix(mx - fixdy, fixdy);
      }
    else {
      delta = FixHalfMul(fixdy);
      RecordHFix(mn, delta);
      RecordHFix(mn + fixdy, -delta);
      RecordHFix(mx, -delta);
      RecordHFix(mx - fixdy, delta);
      }
    }
  else if (vert && VFixCount+4 < maxFixes && autoVFix) {
    Fixed fixdx = w - minW;
    if (abs(fixdx) <= FixOne) {
      RecordVFix(mn, fixdx);
      RecordVFix(mx - fixdx, fixdx);
      }
    else {
      delta = FixHalfMul(fixdx);
      RecordVFix(mn, delta);
      RecordVFix(mn + fixdx, -delta);
      RecordVFix(mx, -delta);
      RecordVFix(mx - fixdx, delta);
      }
    }
  }

private boolean CheckForInsideBands(loc, blues, numblues)
  Fixed loc, *blues; integer numblues; {
  integer i;
  for (i = 0; i < numblues; i += 2) {
    if (loc >= blues[i] && loc <= blues[i+1]) return TRUE;
    }
  return FALSE;
  }

#define bFuzz (FixInt(6))
private procedure CheckForNearBands(loc, blues, numblues) 
  Fixed loc, *blues; integer numblues; { 
  integer i; 
  boolean bottom = TRUE;
  for (i = 0; i < numblues; i++) { 
    if ((bottom && loc >= blues[i]-bFuzz && loc < blues[i]) ||
       (!bottom && loc <= blues[i]+bFuzz && loc > blues[i])) {
#if 0
     ReportBandError(bottom? "below" : "above", loc, blues[i]);
#else
     ReportBandNearMiss(bottom? "below" : "above", loc, blues[i]);
#endif
#if 0
      bandError = TRUE;
#endif
      }
    bottom = !bottom;
    }
  }

public boolean FindLineSeg(loc, sL) Fixed loc; PClrSeg sL; {
  while (sL != NULL) {
    if (sL->sLoc ==loc && sL->sType == sLINE) return TRUE;
    sL = sL->sNxt; }
  return FALSE; }

#if 1
/* Traverses hSegList to check for near misses to
   the horizontal alignment zones. The list contains
   segments that may or may not have hints added. */ 
public procedure CheckTfmVal (hSegList, bandList, length)
PClrSeg hSegList; Fixed *bandList; long int length;  {
  Fixed tfmval;
  PClrSeg sList = hSegList;

  while (sList != NULL) {
    tfmval = itfmy(sList->sLoc);
    if ((length >= 2 ) && !bandError &&
      !CheckForInsideBands(tfmval, bandList, length))
        CheckForNearBands(tfmval, bandList, length);
    sList = sList->sNxt;
    }
}
#else
public procedure CheckTfmVal (b, t, vert) Fixed b, t; boolean vert; {
  if (t < b) { Fixed tmp; tmp = t; t = b; b = tmp; }
  if (!vert && (lenTopBands >= 2 || lenBotBands >= 2) && !bandError &&
      !CheckForInsideBands(t, topBands, lenTopBands) &&
      !CheckForInsideBands(b, botBands, lenBotBands)) {
    CheckForNearBands(t, topBands, lenTopBands);
    CheckForNearBands(b, botBands, lenBotBands);
    }
}
#endif

public procedure CheckVal(val, vert) PClrVal val; boolean vert; {
  Fixed *stems;
  integer numstems, i;
  Fixed wd, diff, minDiff, minW, b, t, w;
  boolean curve = FALSE;
  if (vert) {
    stems = VStems; numstems = NumVStems;
    b = itfmx(val->vLoc1); t = itfmx(val->vLoc2); }
  else {
    stems = HStems; numstems = NumHStems;
    b = itfmy(val->vLoc1); t = itfmy(val->vLoc2); }
  w = abs(t-b);
  minDiff = FixInt(1000); minW = 0;
  for (i = 0; i < numstems; i++) {
    wd = stems[i]; diff = abs(wd-w);
    if (diff < minDiff) {
      minDiff = diff; minW = wd;
      if (minDiff == 0) break;
      }
    }
  if (minDiff == 0 || minDiff > FixInt(2))
    return;
  if (b != bPrev || t != tPrev)
  {
    if ((vert && (!FindLineSeg(val->vLoc1, leftList) ||
                !FindLineSeg(val->vLoc2, rightList))) ||
      (!vert && (!FindLineSeg(val->vLoc1, botList) ||
                 !FindLineSeg(val->vLoc2, topList))))
       curve = TRUE;
    if (!val->vGhst)
      ReportStemNearMiss(vert, w, minW, b, t, curve);
  }
  bPrev = b; tPrev = t;
  if ((vert && autoVFix) || (!vert && autoHFix))
    RecordForFix(vert, w, minW, b, t);
  }

public procedure CheckVals(vlst, vert) PClrVal vlst; boolean vert; {
  while (vlst != NULL) {
    CheckVal(vlst, vert);
    vlst = vlst->vNxt;
    }
  }

private procedure FixH(e, fixy, fixdy) PPathElt e; Fixed fixy, fixdy; {
  PPathElt prev, nxt;
  RMovePoint(0L, fixdy, cpStart, e);
  RMovePoint(0L, fixdy, cpEnd, e);
  prev = e->prev;
  if (prev != NULL && prev->type == CURVETO && prev->y2 == fixy)
    RMovePoint(0L, fixdy, cpCurve2, prev);
  if (e->type == CLOSEPATH) e = GetDest(e);
  nxt = e->next;
  if (nxt != NULL && nxt->type == CURVETO && nxt->y1 == fixy)
    RMovePoint(0L, fixdy, cpCurve1, nxt);
  }

private procedure FixHs(fixy, fixdy)
  Fixed fixy, fixdy; { /* y dy in user space */
  PPathElt e;
  Fixed xlst, ylst, xinit, yinit;
  fixy = tfmy(fixy); fixdy = dtfmy(fixdy);
  e = pathStart;
  while (e != NULL) {
    switch (e->type) {
      case MOVETO:
        xlst = xinit = e->x; ylst = yinit = e->y; break;
      case LINETO:
        if (e->y == fixy && ylst == fixy)
          FixH(e, fixy, fixdy);
	xlst = e->x; ylst = e->y; break;
      case CURVETO:
        xlst = e->x3; ylst = e->y3; break;
      case CLOSEPATH:
        if (yinit == fixy && ylst == fixy && xinit != xlst)
          FixH(e, fixy, fixdy);
	break;
      default:
		{
		  FlushLogFiles();
		  sprintf(globmsg, "Illegal operator in path list in %s.\n", fileName);
		  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}

      }
    e = e->next;
    }
  }

private procedure FixV(e, fixx, fixdx) PPathElt e; Fixed fixx, fixdx; {
  PPathElt prev, nxt;
  RMovePoint(fixdx, 0L, cpStart, e);
  RMovePoint(fixdx, 0L, cpEnd, e);
  prev = e->prev;
  if (prev != NULL && prev->type == CURVETO && prev->x2 == fixx)
    RMovePoint(fixdx, 0L, cpCurve2, prev);
  if (e->type == CLOSEPATH) e = GetDest(e);
  nxt = e->next;
  if (nxt != NULL && nxt->type == CURVETO && nxt->x1 == fixx)
    RMovePoint(fixdx, 0L, cpCurve1, nxt);
  }

private procedure FixVs(fixx, fixdx)
  Fixed fixx, fixdx; { /* x dx in user space */
  PPathElt e;
  Fixed xlst, ylst, xinit, yinit;
  fixx = tfmx(fixx); fixdx = dtfmx(fixdx);
  e = pathStart;
  while (e != NULL) {
    switch (e->type) {
      case MOVETO:
        xlst = xinit = e->x; ylst = yinit = e->y; break;
      case LINETO:
        if (e->x == fixx && xlst == fixx)
          FixV(e, fixx, fixdx);
	xlst = e->x; ylst = e->y; break;
      case CURVETO:
        xlst = e->x3; ylst = e->y3; break;
      case CLOSEPATH:
        if (xinit == fixx && xlst == fixx && yinit != ylst)
          FixV(e, fixx, fixdx);
	break;
      default:
		{
		  FlushLogFiles();
		  sprintf(globmsg, "Illegal operator in point list in %s.\n", fileName);
		  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}
      }
    e = e->next;
    }
  }

public boolean DoFixes() {
  boolean didfixes = FALSE;
  integer i;
  if (HFixCount > 0 && autoHFix) {
    PrintMessage("Fixing horizontal near misses.");
    didfixes = TRUE;
    for (i = 0; i < HFixCount; i++)
      FixHs(HFixYs[i], HFixDYs[i]);
    }
  if (VFixCount > 0 && autoVFix) {
    PrintMessage("Fixing vertical near misses.");
    didfixes = TRUE;
    for (i = 0; i < VFixCount; i++)
      FixVs(VFixXs[i], VFixDXs[i]);
    }
  return didfixes;
  }

