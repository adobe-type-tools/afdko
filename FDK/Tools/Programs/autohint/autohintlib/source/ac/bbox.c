/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* bbox.c */


#include "ac.h"
#include "bftoac.h"
#include "machinedep.h"

private Fixed xmin, ymin, xmax, ymax, vMn, vMx, hMn, hMx;
private PPathElt pxmn, pxmx, pymn, pymx, pe, pvMn, pvMx, phMn, phMx;

private procedure FPBBoxPt(c) Cd c; {
  if (c.x < xmin) { xmin = c.x; pxmn = pe; }
  if (c.x > xmax) { xmax = c.x; pxmx = pe; }
  if (c.y < ymin) { ymin = c.y; pymn = pe; }
  if (c.y > ymax) { ymax = c.y; pymx = pe; }
  }

private procedure FindPathBBox() {
  FltnRec fr;
  register PPathElt e;
  Cd c0, c1, c2, c3;
  if (pathStart == NULL) {
    xmin = ymin = xmax = ymax = 0;
    pxmn = pxmx = pymn = pymx = NULL;
    return;
    }
  fr.report = FPBBoxPt;
  xmin = ymin = FixInt(10000);
  xmax = ymax = -xmin;
  e = pathStart;
  while (e != NULL) {
    switch (e->type) {
      case MOVETO:
      case LINETO:
        c0.x = e->x; c0.y = e->y; pe = e;
        FPBBoxPt(c0);
	break;
      case CURVETO:
        c1.x = e->x1; c1.y = e->y1;
	c2.x = e->x2; c2.y = e->y2;
	c3.x = e->x3; c3.y = e->y3;
	pe = e;
        FltnCurve(c0, c1, c2, c3, &fr);
	c0 = c3;
	break;
      case CLOSEPATH:
        break;
      default:
      {
		FlushLogFiles();
        sprintf (globmsg, "Undefined operator in %s character.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      }
    e = e->next;
    }
  xmin = FHalfRnd(xmin);
  ymin = FHalfRnd(ymin);
  xmax = FHalfRnd(xmax);
  ymax = FHalfRnd(ymax);
  }

public PPathElt FindSubpathBBox(e) PPathElt e; {
  FltnRec fr;
  Cd c0, c1, c2, c3;
  if (e == NULL) {
    xmin = ymin = xmax = ymax = 0;
    pxmn = pxmx = pymn = pymx = NULL;
    return NULL;
    }
  fr.report = FPBBoxPt;
  xmin = ymin = FixInt(10000);
  xmax = ymax = -xmin;
#if 0
  e = GetDest(e); /* back up to moveto */
#else
  /* This and the following change (in the next else clause) were made
     to fix the coloring in characters in the SolEol lists.  These are
     supposed to have subpath bbox colored, but were getting path bbox
     colored instead. */
  if (e->type != MOVETO) e = GetDest(e); /* back up to moveto */
#endif
  while (e != NULL) {
    switch (e->type) {
      case MOVETO:
      case LINETO:
        c0.x = e->x; c0.y = e->y; pe = e;
	FPBBoxPt(c0);
	break;
      case CURVETO:
        c1.x = e->x1; c1.y = e->y1;
	c2.x = e->x2; c2.y = e->y2;
	c3.x = e->x3; c3.y = e->y3;
	pe = e;
        FltnCurve(c0, c1, c2, c3, &fr);
	c0 = c3;
	break;
      case CLOSEPATH:
#if 0
        break;
#else
        e = e->next;
        goto done;
#endif
      default:
      {
		FlushLogFiles();
        sprintf (globmsg, "Undefined operator in %s character.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      }
    e = e->next;
    }
#if 1
  done:
#endif
  xmin = FHalfRnd(xmin);
  ymin = FHalfRnd(ymin);
  xmax = FHalfRnd(xmax);
  ymax = FHalfRnd(ymax);
  return e;
  }

public procedure FindCurveBBox(
  x0,y0,px1,py1,px2,py2,x1,y1,pllx,plly,purx,pury)
  Fixed x0,y0,px1,py1,px2,py2,x1,y1,*pllx,*plly,*purx,*pury; {
  FltnRec fr;
  Cd c0, c1, c2, c3;
  fr.report = FPBBoxPt;
  xmin = ymin = FixInt(10000);
  xmax = ymax = -xmin;
  c0.x = x0; c0.y = y0;
  c1.x = px1; c1.y = py1;
  c2.x = px2; c2.y = py2;
  c3.x = x1; c3.y = y1;
  FPBBoxPt(c0);
  FltnCurve(c0, c1, c2, c3, &fr);
  *pllx = FHalfRnd(xmin);
  *plly = FHalfRnd(ymin);
  *purx = FHalfRnd(xmax);
  *pury = FHalfRnd(ymax);
  }

public procedure ClrVBnds() {
  Fixed tmp;
  PPathElt p;
  if (pathStart == NULL || VColorChar()) return;
  FindPathBBox();
  vMn = itfmx(xmin); vMx = itfmx(xmax);
  pvMn = pxmn; pvMx = pxmx;
  if (vMn > vMx) {
    tmp = vMn; vMn = vMx; vMx = tmp;
    p = pvMn; pvMn = pvMx; pvMx = p;
    }
  AddColorPoint(vMn, 0L, vMx, 0L, 'y', pvMn, pvMx);
  }

public procedure ReClrVBnds() {
  AddColorPoint(vMn, 0L, vMx, 0L, 'y', pvMn, pvMx);
  }

public procedure ClrHBnds() {
  Fixed tmp;
  PPathElt p;
  if (pathStart == NULL || HColorChar()) return;
  FindPathBBox();
  hMn = itfmy(ymin); hMx = itfmy(ymax);
  phMn = pymn; phMx = pymx;
  if (hMn > hMx) {
    tmp = hMn; hMn = hMx; hMx = tmp;
    p = phMn; phMn = phMx; phMx = p;
    }
  AddColorPoint(0L, hMn, 0L, hMx, 'b', phMn, phMx);
  }

public procedure ReClrHBnds() {
  AddColorPoint(0L, hMn, 0L, hMx, 'b', phMn, phMx);
  }

private boolean CheckValOverlaps(lft, rht, lst, xflg)
  Fixed lft, rht; PClrVal lst; boolean xflg; {
  Fixed lft2, rht2, tmp;
  if (xflg) { lft = itfmx(lft); rht = itfmx(rht); }
  else { lft = itfmy(lft); rht = itfmy(rht); }
  if (lft > rht) { tmp = lft; lft = rht; rht = tmp; }
  while (lst != NULL) {
    lft2 = lst->vLoc1; rht2 = lst->vLoc2;
    if (xflg) { lft2 = itfmx(lft2); rht2 = itfmx(rht2); }
    else { lft2 = itfmy(lft2); rht2 = itfmy(rht2); }
    if (lft2 > rht2) { tmp = lft2; lft2 = rht2; rht2 = tmp; }
    if (lft2 <= rht && lft <= rht2) return TRUE;
    lst = lst->vNxt;
    }
  return FALSE;
  }

public procedure AddBBoxHV(Hflg, subs) boolean Hflg, subs; {
  PPathElt e;
  PClrVal val;
  PClrSeg seg1, seg2;
  e = pathStart;
  while (e != NULL) {
    if (subs) e = FindSubpathBBox(e);
    else { FindPathBBox(); e = NULL; }
    if (!Hflg) {
      if (!CheckValOverlaps(xmin, xmax, Vcoloring, TRUE)) {
        val = (PClrVal)Alloc(sizeof(ClrVal));
	seg1 = (PClrSeg)Alloc(sizeof(ClrSeg));
	seg1->sLoc = xmin; seg1->sElt = pxmn;
	seg1->sBonus = 0; seg1->sType = sLINE;
	seg1->sMin = ymin; seg1->sMax = ymax;
	seg1->sNxt = NULL; seg1->sLnk = NULL;
	seg2 = (PClrSeg)Alloc(sizeof(ClrSeg));
	seg2->sLoc = xmax; seg2->sElt = pxmx;
	seg2->sBonus = 0; seg2->sType = sLINE;
	seg2->sMin = ymin; seg2->sMax = ymax;
	seg2->sNxt = NULL; seg2->sLnk = NULL;
	val->vVal = 100;
	val->vSpc = 0;
	val->vLoc1 = xmin;
	val->vLoc2 = xmax;
	val->vSeg1 = seg1;
	val->vSeg2 = seg2;
	val->vGhst = FALSE;
	val->vNxt = Vcoloring;
	val->vBst = val;
	Vcoloring = val;
        }
      }
    else {
      if (!CheckValOverlaps(ymin, ymax, Hcoloring, FALSE)) {
        val = (PClrVal)Alloc(sizeof(ClrVal));
	seg1 = (PClrSeg)Alloc(sizeof(ClrSeg));
	seg1->sLoc = ymax; seg1->sElt = pymx;
	seg1->sBonus = 0; seg1->sType = sLINE;
	seg1->sMin = xmin; seg1->sMax = xmax;
	seg1->sNxt = NULL; seg1->sLnk = NULL;
	seg2 = (PClrSeg)Alloc(sizeof(ClrSeg));
	seg2->sLoc = ymin; seg2->sElt = pymn;
	seg2->sBonus = 0; seg2->sType = sLINE;
	seg2->sMin = xmin; seg2->sMax = xmax;
	seg2->sNxt = NULL; seg2->sLnk = NULL;
	val->vVal = 100;
	val->vSpc = 0;
	val->vLoc1 = ymax; /* bot is > top because y axis is reversed */
	val->vLoc2 = ymin;
	val->vSeg1 = seg1;
	val->vSeg2 = seg2;
	val->vGhst = FALSE;
	val->vNxt = Hcoloring;
	val->vBst = val;
	Hcoloring = val;
        }
      }
    }
  }

public procedure ClrBBox() {
  Fixed llx, lly, urx, ury, tmp;
  PPathElt p, p0, p1;
  if (!useV) {
    llx = itfmx(xmin); urx = itfmx(xmax);
    p0 = pxmn; p1 = pxmx;
    if (llx > urx) {
      tmp = llx; llx = urx; urx = tmp;
      p = p0; p0 = p1; p1 = p0;
      }
    AddColorPoint(llx, 0L, urx, 0L, 'y', p0, p1);
    }
  if (!useH) {
    lly = itfmy(ymax); ury = itfmy(ymin);
    p0 = pymx; p1 = pymn;
    if (lly > ury) {
      tmp = lly; lly = ury; ury = tmp;
      p = p0; p0 = p1; p1 = p0;
      }
    AddColorPoint(0L, lly, 0L, ury, 'b', p0, p1);
    }
  }

public procedure CheckPathBBox() {
  Fixed llx, lly, urx, ury, tmp;
  FindPathBBox();
  llx = itfmx(xmin); urx = itfmx(xmax);
  if (llx > urx) { tmp = llx; llx = urx; urx = tmp; }
  lly = itfmy(ymax); ury = itfmy(ymin);
  if (lly > ury) { tmp = lly; lly = ury; ury = tmp; }
  if (llx < FixInt(-600) || lly < FixInt(-600) ||
      urx > FixInt(1600) || ury > FixInt(1600))
    ReportBBoxBogus(llx, lly, urx, ury);
  }

public Fixed GetPathLSB(void) {
  Fixed llx, urx, tmp;
  FindPathBBox();
  llx = itfmx(xmin); urx = itfmx(xmax);
  if (llx > urx) { tmp = llx; llx = urx; urx = tmp; }
  return (llx);
  }

public boolean CheckBBoxes(e1, e2) PPathElt e1, e2; {
  /* return TRUE if e1 and e2 in same subpath or i
     the bbox for one is inside the bbox of the other */
  Fixed xmn, xmx, ymn, ymx;
  e1 = GetDest(e1); e2 = GetDest(e2);
  if (e1 == e2) return TRUE; /* same subpath */
  FindSubpathBBox(e1);
  xmn = xmin; xmx = xmax; ymn = ymin; ymx = ymax;
  FindSubpathBBox(e2);
  return ((xmn <= xmin && xmax <= xmx && ymn <= ymin && ymax <= ymx) ||
          (xmn >= xmin && xmax >= xmx && ymn >= ymin && ymax >= ymx));
  }

