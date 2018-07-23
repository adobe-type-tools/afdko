/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* head.c */


#include "ac.h"
#include "machinedep.h"

public PPathElt GetDest(cldest) register PPathElt cldest; {
  if (cldest == NULL) return NULL;
  while (TRUE) {
    cldest = cldest->prev;
    if (cldest == NULL) return pathStart;
    if (cldest->type == MOVETO) return cldest;
    }
  }

public PPathElt GetClosedBy(clsdby) register PPathElt clsdby; {
  if (clsdby == NULL) return NULL;
  if (clsdby->type == CLOSEPATH) return clsdby;
  while (TRUE) {
    clsdby = clsdby->next;
    if (clsdby == NULL) return NULL;
    if (clsdby->type == MOVETO) return NULL;
    if (clsdby->type == CLOSEPATH) return clsdby;
    }
  }

public procedure GetEndPoint(e, x1p, y1p)
  register PPathElt e; Fixed *x1p, *y1p; {
  if (e == NULL) { *x1p = 0; *y1p = 0; return; }
  retry:
  switch (e->type) {
    case MOVETO: case LINETO:
      *x1p = e->x; *y1p = e->y; break;
    case CURVETO:
      *x1p = e->x3; *y1p = e->y3; break;
    case CLOSEPATH:
      e = GetDest(e);
      if (e == NULL || e->type == CLOSEPATH)
      {
        sprintf(globmsg, "Bad character description file: %s.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      goto retry;
    default:
    {
      sprintf(globmsg, "Illegal operator in character file: %s.\n", fileName);
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    }
  }

public procedure GetEndPoints(p,px0,py0,px1,py1)
  PPathElt p; Fixed *px0, *py0, *px1, *py1; {
  GetEndPoint(p,px1,py1);
  GetEndPoint(p->prev,px0,py0); }

#define Interpolate(q,v0,q0,v1,q1) (v0+(q-q0)*((v1-v0)/(q1-q0)))
private Fixed HVness(pq) real *pq; {
  register real q;
  real result;
  /* approximately == 2 q neg exp */
  /* as q -> 0, result goes to 1.0 */
  /* as q -> inf, result goes to 0.0 */
  q = *pq;
  if (q < .25) result = (real)Interpolate(q,1.0,0.0,.841,.25);
  else if (q < .5) result = (real)Interpolate(q,.841,.25,.707,.5);
  else if (q < 1) result = (real)Interpolate(q,.707,.5,.5,1.0);
  else if (q < 2) result = (real)Interpolate(q,.5,1.0,.25,2.0);
  else if (q < 4) result = (real)Interpolate(q,.25,2.0,0.0,4.0);
  else result = 0.0;
  return acpflttofix(&result); }

public Fixed VertQuo(xk,yk,xl,yl) Fixed xk,yk,xl,yl; {
    /* FixOne means exactly vertical. 0 means not vertical */
    /* intermediate values mean almost vertical */
    register Fixed xabs, yabs;
    real rx, ry, q;
    xabs = xk-xl;
    if (xabs < 0L)
        xabs = -xabs;
    if (xabs==0)
        return FixOne;
    yabs = yk-yl;
    if (yabs < 0L)
        yabs = -yabs;
    if (yabs==0)
        return 0L;
    acfixtopflt(xabs, &rx);
    acfixtopflt(yabs, &ry);
    q = (real)(rx*rx)/(theta*ry); /* DEBUG 8 BIT. Used to by 2*(rx*rx)/(theta*ry). Don't need thsi with the 8 bits of Fixed fraction. */
    return HVness(&q);
}

public Fixed HorzQuo(xk,yk,xl,yl) Fixed xk,yk,xl,yl; {
    register Fixed xabs, yabs;
    real rx, ry, q;
    yabs = yk-yl;
    if (yabs < 0)
        yabs = -yabs;
    if (yabs==0L)
        return FixOne;
    xabs = xk-xl;
    if (xabs < 0)
        xabs = -xabs;
    if (xabs==0L)
        return 0L;
    acfixtopflt(xabs, &rx);
    acfixtopflt(yabs, &ry);
    q = (real)(ry*ry)/(theta*rx); /* DEBUG 8 BIT. Used to by 2*(ry*ry)/(theta*ry). Don't need thsi with the 8 bits of Fixed fraction. */
    return HVness(&q);
}

public boolean IsTiny(e) PPathElt e; {
  Fixed x0, y0, x1, y1, abstmp;
  GetEndPoints(e,&x0,&y0,&x1,&y1);
  return ((ac_abs(x0-x1) < FixTwo) && (ac_abs(y0-y1) < FixTwo))? TRUE : FALSE; }

public boolean IsShort(e) PPathElt e; {
    Fixed x0, y0, x1, y1, dx, dy, mn, mx, abstmp;
    GetEndPoints(e,&x0,&y0,&x1,&y1);
    dx = ac_abs(x0-x1);
    dy = ac_abs(y0-y1);
    if (dx > dy)
    { mn = dy;
        mx = dx;
    }
    else {
        mn = dx;
        mx = dy;
    }
    return ((mx + (mn*42L)/125L) < FixInt(6))? TRUE : FALSE; /* DEBUG 8 BIT. Increased threshold from 3 to 6, for change in coordinare system. */
}

public PPathElt NxtForBend(p,px2,py2,px3,py3)
  PPathElt p; Fixed *px2, *py2, *px3, *py3; {
  PPathElt nxt, nxtMT = NULL;
  Fixed x, y, x2, y2;
  nxt = p;
  GetEndPoint(p, &x, &y);
  while (TRUE) {
    if (nxt->type == CLOSEPATH) {
      nxt = GetDest(nxt);
      /* The following test was added to prevent an infinite loop. */
      if (nxtMT != NULL && nxtMT == nxt)
      {
        ReportPossibleLoop(p);
        nxt = NULL;
      }
      else { nxtMT = nxt; nxt = nxt->next; }
    }
    else nxt = nxt->next;
    if (nxt == NULL) { /* forget it */
      *px2 = *py2 = *px3 = *py3 = FixInt(-9999); return nxt; }
    if (!IsTiny(nxt)) break; }
  if (nxt->type == CURVETO) {
    x2 = nxt->x1; y2 = nxt->y1;
    if (x2 == x && y2 == y) { x2 = nxt->x2; y2 = nxt->y2; }
    *px2 = x2; *py2 = y2; }
  else GetEndPoint(nxt,px2,py2);
  GetEndPoint(nxt,px3,py3);
  return nxt;
  }

public PPathElt PrvForBend(p,px2,py2) PPathElt p; Fixed *px2, *py2; {
  PPathElt prv, prvCP = NULL;
  Fixed x2, y2;
  prv = p;
  while (TRUE) {
    prv = prv->prev;
    if (prv == NULL) goto Bogus;
    if (prv->type == MOVETO) {
      prv = GetClosedBy(prv);
      /* The following test was added to prevent an infinite loop. */
      if (prv == NULL || (prvCP != NULL && prvCP == prv))
        goto Bogus;
      prvCP = prv;
    }
    if (!IsTiny(prv)) break; }
  if (prv->type == CURVETO) {
    x2 = prv->x2; y2 = prv->y2;
    if (x2 == prv->x3 && y2 == prv->y3) { x2 = prv->x1; y2 = prv->y1; }
    *px2 = x2; *py2 = y2; }
  else {
    p = prv->prev;
    if (p == NULL) goto Bogus;
    GetEndPoint(p,px2,py2); }
  return prv;
 Bogus: *px2 = *py2 = FixInt(-9999);
   return prv;
  }

private boolean CheckHeight(upperFlag,p) 
  boolean upperFlag; PPathElt p; {
  PPathElt ee;
  Fixed y, yy;
  ee = pathStart;
  y = itfmy(p->y);
  while (ee != NULL) {
    if (ee->type == MOVETO && ee != p) {
      yy = itfmy(ee->y);
      if ((upperFlag && yy > y) || (!upperFlag && yy < y)) return FALSE;
      }
    ee = ee->next; }
  return TRUE; }

public boolean IsLower(p) PPathElt p; {
  return CheckHeight(FALSE,p); }

public boolean IsUpper(p) PPathElt p; {
  return CheckHeight(TRUE,p); }
