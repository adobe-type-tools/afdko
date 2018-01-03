/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* check.c */

#include "ac.h"
#include "bftoac.h"
#include "machinedep.h"

private boolean xflat, yflat, xdone, ydone, bbquit;
private integer xstate, ystate, xstart, ystart;
static Fixed x0, cy0, x1, cy1, xloc, yloc;
private Fixed x, y, xnxt, ynxt;
private Fixed yflatstartx, yflatstarty, yflatendx, yflatendy;
private Fixed xflatstarty, xflatstartx, xflatendx, xflatendy;
private boolean vert, started, reCheckSmooth;
private Fixed loc, frst, lst, fltnvalue;
private PPathElt e;
private boolean forMultiMaster = FALSE, inflPtFound = FALSE;

#define STARTING (0L)
#define goingUP (1L)
#define goingDOWN (2L)

/* DEBUG 8 BIT. The SDELTA value must tbe increased by 2 due to change in coordinate system from 7 to 8 bit FIXED fraction. */

#define SDELTA (FixInt(8))
#define SDELTA3 (FixInt(10))

private procedure chkBad() {
    reCheckSmooth = ResolveConflictBySplit(e,FALSE,NULL,NULL);;
}

#define GrTan(n,d) (ac_abs(n)*100L > ac_abs(d)*sCurveTan)
#define LsTan(n,d) (ac_abs(n)*100L < ac_abs(d)*sCurveTan)

private procedure chkYDIR() {
    if (y > yloc) { /* going up */
        if (ystate == goingUP) return;
        if (ystate == STARTING) ystart = ystate = goingUP;
        else /*if (ystate == goingDOWN)*/ {
            if (ystart == goingUP) {
                yflatendx = xloc; yflatendy = yloc; }
            else if (!yflat) {
                yflatstartx = xloc; yflatstarty = yloc; yflat = TRUE; }
            ystate = goingUP; }
    }
    else if (y < yloc) { /* going down */
        if (ystate == goingDOWN) return;
        if (ystate == STARTING) ystart = ystate = goingDOWN;
        else /*if (ystate == goingUP)*/ {
            if (ystart == goingDOWN) {
                yflatendx = xloc; yflatendy = yloc; }
            else if (!yflat) {
                yflatstartx = xloc; yflatstarty = yloc; yflat = TRUE; }
            ystate = goingDOWN; }
    }
}

private procedure chkYFLAT() {
    Fixed abstmp;
    if (!yflat) {
        if (LsTan(y-yloc, x-xloc)) {
            yflat = TRUE; yflatstartx = xloc; yflatstarty = yloc; }
        return; }
    if (ystate != ystart) return;
    if (GrTan(y-yloc, x-xloc)) {
        yflatendx = xloc; yflatendy = yloc; ydone = TRUE; }
}

private procedure chkXFLAT() {
    Fixed abstmp;
    if (!xflat) {
        if (LsTan(x-xloc, y-yloc)) {
            xflat = TRUE; xflatstartx = xloc; xflatstarty = yloc; }
        return; }
    if (xstate != xstart) return;
    if (GrTan(x-xloc, y-yloc)) {
        xflatendx = xloc; xflatendy = yloc; xdone = TRUE; }
}

private procedure chkXDIR() {
    if (x > xloc) { /* going up */
        if (xstate == goingUP) return;
        if (xstate == STARTING) xstart = xstate = goingUP;
        else /*if (xstate == goingDOWN)*/ {
            if (xstart == goingUP) {
                xflatendx = xloc; xflatendy = yloc; }
            else if (!xflat) {
                xflatstartx = xloc; xflatstarty = yloc; xflat = TRUE; }
            xstate = goingUP; }
    }
    else if (x < xloc) {
        if (xstate == goingDOWN) return;
        if (xstate == STARTING) xstart = xstate = goingDOWN;
        else /*if (xstate == goingUP)*/ {
            if (xstart == goingDOWN) {
                xflatendx = xloc; xflatendy = yloc; }
            else if (!xflat) {
                xflatstartx = xloc; xflatstarty = yloc; xflat = TRUE; }
            xstate = goingDOWN; }
    }
}

private procedure chkDT(c) Cd c; {
    Fixed abstmp;
    Fixed loc;

    x = c.x, y = c.y;
    ynxt = y; xnxt = x;
    if (!ydone) {
        chkYDIR(); chkYFLAT();
        if (ydone && yflat &&
            ac_abs(yflatstarty-cy0) > SDELTA &&
            ac_abs(cy1-yflatendy) > SDELTA) {
            if ((ystart==goingUP && yflatstarty-yflatendy > SDELTA) ||
                (ystart==goingDOWN && yflatendy-yflatstarty > SDELTA)) {
                if (editChar && !forMultiMaster)
                    chkBad();
                return; }
            if (ac_abs(yflatstartx-yflatendx) > SDELTA3)
            {
                DEBUG_ROUND(yflatstartx);
                DEBUG_ROUND(yflatendx);
                DEBUG_ROUND(yflatstarty);
                DEBUG_ROUND(yflatendy);
                
                loc = (yflatstarty+yflatendy)/2;
                DEBUG_ROUND(loc);
                
            if (!forMultiMaster)
                {
                    AddHSegment(yflatstartx,yflatendx,loc,
                                e,(PPathElt)NULL,sCURVE,13L);
                }
                else
                {
                    inflPtFound = TRUE;
                    fltnvalue = itfmy(loc);
                }
            }
        }
    }
    if (!xdone) {
        chkXDIR(); chkXFLAT();
        if (xdone && xflat &&
            ac_abs(xflatstartx-x0) > SDELTA &&
            ac_abs(x1-xflatendx) > SDELTA) {
            if ((xstart==goingUP && xflatstartx-xflatendx > SDELTA) ||
                (xstart==goingDOWN && xflatendx-xflatstartx > SDELTA)) {
                if (editChar && !forMultiMaster)
                    chkBad();
                return; }
            if (ac_abs(xflatstarty-xflatendy) > SDELTA3)
            {
                DEBUG_ROUND(xflatstarty);
                DEBUG_ROUND(xflatendy);
                DEBUG_ROUND(xflatstartx);
                DEBUG_ROUND(xflatendx);
                
                loc = (xflatstartx+xflatendx)/2;
                DEBUG_ROUND(loc);

                if (!forMultiMaster)
                
                {
                    AddVSegment(xflatstarty,xflatendy,loc,
                               e,(PPathElt)NULL,sCURVE,13L);
                }
                else
                {
                    inflPtFound = TRUE;
                    fltnvalue = itfmx(loc);
                }
                
            }
        }
    }
    xloc = xnxt; yloc = ynxt;
}

#define FQ(x) ((long int)((x) >> 6))
private integer CPDirection(x1,cy1,x2,y2,x3,y3) Fixed x1,cy1,x2,y2,x3,y3; {
    long int q, q1, q2, q3;
    q1 = FQ(x2)*FQ(y3-cy1);
    q2 = FQ(x1)*FQ(y2-y3);
    q3 = FQ(x3)*FQ(cy1-y2);
    q = q1 + q2 + q3;
    if (q > 0) return 1L;
    if (q < 0) return -1L;
    return 0L;
}

private PPathElt PointLine(e, whichcp) PPathElt e; integer whichcp; {
    PPathElt newline;
    if (whichcp == cpCurve1) whichcp = cpStart;
    else if (whichcp == cpCurve2) whichcp = cpEnd;
    if (whichcp == cpEnd && e->type == CLOSEPATH) {
        e = GetDest(e); e = e->next; whichcp = cpStart; }
    newline = (PPathElt)Alloc(sizeof(PathElt));
    newline->type = LINETO;
    if (whichcp == cpStart) { /* insert line in front of e */
        newline->next = e;
        newline->prev = e->prev;
        if (e == pathStart)
        {
            FlushLogFiles();
            sprintf(globmsg, "Malformed path list (Start) in %s.\n", fileName);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        e->prev->next = newline;
        e->prev = newline;
    }
    else { /* add line after e */
        newline->next = e->next;
        e->next->prev = newline;
        if (e == pathEnd)
        {
            FlushLogFiles();
            sprintf(globmsg, "Malformed path list (End) in %s.\n", fileName);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        e->next = newline;
        newline->prev = e;
    }
    GetEndPoint(newline->prev, &newline->x, &newline->y);
    return newline;
}

private procedure MovePoint(x, y, e, whichcp)
Fixed x, y; PPathElt e; int whichcp; {
    if (whichcp == cpStart) { e = e->prev; whichcp = cpEnd; }
    if (whichcp == cpEnd) {
        if (e->type == CLOSEPATH) e = GetDest(e);
        if (e->type == CURVETO) { e->x3 = x; e->y3 = y; }
        else { e->x = x; e->y = y; }
        return;
    }
    if (whichcp == cpCurve1) { e->x1 = x; e->y1 = y; return; }
    if (whichcp == cpCurve2) { e->x2 = x; e->y2 = y; return; }
    {
        FlushLogFiles();
        sprintf(globmsg, "Malformed path list in %s.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
}

public procedure RMovePoint(dx, dy, whichcp, e)
Fixed dx, dy; PPathElt e; integer whichcp; {
    if (whichcp == cpStart) { e = e->prev; whichcp = cpEnd; }
    if (whichcp == cpEnd) {
        if (e->type == CLOSEPATH) e = GetDest(e);
        if (e->type == CURVETO) { e->x3 += dx; e->y3 += dy; }
        else { e->x += dx; e->y += dy; }
        return;
    }
    if (whichcp == cpCurve1) { e->x1 += dx; e->y1 += dy; return; }
    if (whichcp == cpCurve2) { e->x2 += dx; e->y2 += dy; return; }
    {
        FlushLogFiles();
        sprintf(globmsg, "Malformed path list in %s.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    
}

private boolean ZeroLength(e) PPathElt e; {
    Fixed x0, cy0, x1, cy1;
    GetEndPoints(e,&x0,&cy0,&x1,&cy1);
    return (x0 == x1 && cy0 == cy1); }

private boolean ConsiderClipSharpPoint(rx0, ry0, rx1, ry1, rx2, ry2, e)
Fixed rx0, ry0, rx1, ry1, rx2, ry2; PPathElt e; {
    Fixed x0=rx0, cy0=ry0, x1=rx1, cy1=ry1, x2=rx2, y2=ry2;
    Fixed dx0, dy0, dx1, dy1, nlx, nly;
    real rdx0, rdx1, rdy0, rdy1;
    PPathElt newline;
    x0 = itfmx(x0);   x1 = itfmx(x1);   x2 = itfmx(x2);
    cy0 = itfmy(cy0);   cy1 = itfmy(cy1);   y2 = itfmy(y2);
    dx0 = x1 - x0; dy0 = cy1 - cy0;
    dx1 = x2 - x1; dy1 = y2 - cy1;
    acfixtopflt(dx0, &rdx0); acfixtopflt(dx1, &rdx1);
    acfixtopflt(dy0, &rdy0); acfixtopflt(dy1, &rdy1);
    if (rdx1*rdy0 < rdx0*rdy1) return FALSE;
    /* clip right turns (point toward inside of character) */
    /* do not clip left turns (point toward outside of character) */
    if (e->type != CLOSEPATH && e->next->type == CLOSEPATH &&
        ZeroLength(e->next))
        newline = e->next;
    else
        newline = PointLine(e,cpEnd); /* insert a zero length line */
    GetEndPoint(newline, &nlx, &nly);
    (void)NxtForBend(newline, &x1, &cy1, &x2, &y2);
    PrvForBend(newline, &x0, &cy0);
    /* x0 cy0 is the point before newline */
    /* x1 cy1 is the point after newline */
    /* nlx nly is the location of newline */
    if (cy0 == nly)
        MovePoint(nlx, (cy1 > cy0) ? nly+FixOne : nly-FixOne, newline, cpEnd);
    else if (cy1 == nly)
        MovePoint(nlx, (cy0 > cy1) ? nly+FixOne : nly-FixOne, newline, cpStart);
    else if (x0 == nlx)
        MovePoint((x1 > x0) ? nlx+FixOne : nlx-FixOne, nly, newline, cpEnd);
    else if (x1 == nlx)
        MovePoint((x0 > x1) ? nlx+FixOne : nlx-FixOne, nly, newline, cpStart);
    else if (ProdGe0(nlx-x0,x1-nlx)) { /* nlx between x0 and x1 */
        MovePoint((x1 > x0) ? nlx+FixHalf : nlx-FixHalf, nly, newline, cpEnd);
        MovePoint((x0 > x1) ? nlx+FixHalf : nlx-FixHalf, nly, newline, cpStart);
    }
    else if (ProdGe0(nly-cy0,cy1-nly)) { /* nly between cy0 and cy1 */
        MovePoint(nlx, (cy1 > cy0)? nly+FixHalf : nly-FixHalf, newline, cpEnd);
        MovePoint(nlx, (cy0 > cy1)? nly+FixHalf : nly-FixHalf, newline, cpStart);
    }
    else {
        real dydx0, dydx1;
        integer wh1, wh2;
        dx0 = x0 - nlx; dy0 = cy0 - nly;
        acfixtopflt(dx0, &rdx0); acfixtopflt(dy0, &rdy0);
        dydx0 = rdy0 / rdx0; if (dydx0 < 0) dydx0 = -dydx0;
        dx1 = x1 - nlx; dy1 = cy1 - nly;
        acfixtopflt(dx1, &rdx1); acfixtopflt(dy1, &rdy1);
        dydx1 = rdy1 / rdx1; if (dydx1 < 0) dydx1 = -dydx1;
        if (dydx1 < dydx0) { wh1 = cpStart; wh2 = cpEnd; }
        else { wh2 = cpStart; wh1 = cpEnd; }
        MovePoint(nlx, (dy0 > 0)? nly+FixOne : nly-FixOne, newline, wh1);
        MovePoint((dx0 > 0)? nlx+FixOne : nlx-FixOne, nly, newline, wh2);
    }
    return TRUE;
}

public procedure Delete(e) PPathElt e; {
    PPathElt nxt, prv;
    nxt = e->next; prv = e->prev;
    if (nxt != NULL) nxt->prev = prv;
    else pathEnd = prv;
    if (prv != NULL) prv->next = nxt;
    else pathStart = nxt;
}

/* This procedure is called from BuildFont when adding hints
 to base designs of a multi-master font. */
public boolean GetInflectionPoint(x, y, x1, cy1, x2, y2, x3, y3, inflPt)
Fixed x, y, x1, cy1, x2, y2, x3, y3;
Fixed *inflPt;
{
    FltnRec fltnrec;
    Cd c0, c1, c2, c3;
    
    fltnrec.report = chkDT;
    c0.x = tfmx(x);  c0.y = tfmy(y);
    c1.x = tfmx(x1); c1.y = tfmy(cy1);
    c2.x = tfmx(x2); c2.y = tfmy(y2);
    c3.x = tfmx(x3); c3.y = tfmy(y3);
    xstate = ystate = STARTING;
    xdone = ydone = xflat = yflat = inflPtFound = FALSE;
    x0 = c0.x; cy0 = c0.y; x1 = c3.x; cy1 = c3.y;
    xloc = x0; yloc = cy0;
    forMultiMaster = TRUE;
    FltnCurve(c0, c1, c2, c3, &fltnrec);
    if (inflPtFound)
        *inflPt = fltnvalue;
    return inflPtFound;
}

private procedure CheckSCurve(ee) PPathElt ee; {
    FltnRec fr;
    Cd c0, c1, c2, c3;
    if (ee->type != CURVETO)
    {
        FlushLogFiles();
        sprintf(globmsg, "Malformed path list in %s.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    
    GetEndPoint(ee->prev, &c0.x, &c0.y);
    fr.report = chkDT;
    c1.x = ee->x1; c1.y = ee->y1;
    c2.x = ee->x2; c2.y = ee->y2;
    c3.x = ee->x3; c3.y = ee->y3;
    xstate = ystate = STARTING;
    xdone = ydone = xflat = yflat = FALSE;
    x0 = c0.x; cy0 = c0.y; x1 = c3.x; cy1 = c3.y;
    xloc = x0; yloc = cy0;
    e = ee;
    forMultiMaster = FALSE;
    FltnCurve(c0, c1, c2, c3, &fr);
}

private procedure CheckZeroLength() {
    PPathElt e, NxtE;
    Fixed x0, cy0, x1, cy1, x2, y2, x3, y3;
    e = pathStart;
    while (e != NULL) { /* delete zero length elements */
        NxtE = e->next;
        GetEndPoints(e, &x0, &cy0, &x1, &cy1);
        if (e->type == LINETO && x0==x1 && cy0==cy1) {
            Delete(e); goto Nxt1; }
        if (e->type == CURVETO) {
            x2 = e->x1; y2 = e->y1; x3 = e->x2; y3 = e->y2;
            if (x0==x1 && cy0==cy1 && x2==x1 && x3==x1 && y2==cy1 && y3==cy1) {
                Delete(e); goto Nxt1; }
        }
        Nxt1: e = NxtE; }
}

public procedure CheckSmooth() {
    PPathElt e, nxt, NxtE;
    boolean recheck;
    Fixed x0, cy0, x1, cy1, x2, y2, x3, y3, smdiff, xx, yy;
    CheckZeroLength();
restart:
    reCheckSmooth = FALSE; recheck = FALSE;
    e = pathStart;
    while (e != NULL) {
        NxtE = e->next;
        if (e->type == MOVETO || IsTiny(e) || e->isFlex) goto Nxt;
        GetEndPoint(e, &x1, &cy1);
        if (e->type == CURVETO) {
            integer cpd0, cpd1;
            x2 = e->x1; y2 = e->y1; x3 = e->x2; y3 = e->y2;
            GetEndPoint(e->prev, &x0, &cy0);
            cpd0 = CPDirection(x0,cy0,x2,y2,x3,y3);
            cpd1 = CPDirection(x2,y2,x3,y3,x1,cy1);
            if (ProdLt0(cpd0, cpd1))
                CheckSCurve(e);
        }
        nxt = NxtForBend(e, &x2, &y2, &xx, &yy);
        if (nxt->isFlex) goto Nxt;
        PrvForBend(nxt, &x0, &cy0);
        if (!CheckSmoothness(x0, cy0, x1, cy1, x2, y2, &smdiff))
            ReportSmoothError(x1, cy1);
        if (smdiff > FixInt(140)) { /* trim off sharp angle */
            /* As of version 2.21 angle blunting will not occur. */
            /*
             if (editChar && ConsiderClipSharpPoint(x0, cy0, x1, cy1, x2, y2, e)) {
             ReportClipSharpAngle(x1, cy1); recheck = TRUE; }
             else */
            if (smdiff > FixInt(160))
                ReportSharpAngle(x1, cy1);
        }
        Nxt: e = NxtE; }
    if (reCheckSmooth) goto restart;
    if (!recheck) return;
    CheckZeroLength();
    /* in certain cases clip sharp point can produce a zero length line */
}

#define BBdist (FixInt(20)) /* DEBUG 8 BIT. DOuble value from 10 to 20 for change in coordinate system. */

private procedure chkBBDT(c) Cd c; {
    Fixed x = c.x, y = c.y, abstmp;
    if (bbquit) return;
    if (vert) {
        lst = y;
        if (!started && ac_abs(x-loc) <= BBdist) {
            started = TRUE; frst = y; }
        else if (started && ac_abs(x-loc) > BBdist) bbquit = TRUE;
    }
    else {
        lst = x;
        if (!started && ac_abs(y-loc) <= BBdist) {
            started = TRUE; frst = x; }
        else if (started && ac_abs(y-loc) > BBdist) bbquit = TRUE;
    }
}

public procedure CheckForMultiMoveTo() {
    PPathElt e = pathStart;
    boolean moveto;
    moveto = FALSE;
    while (e != NULL) {
        if (e->type != MOVETO) moveto = FALSE;
        else if (!moveto) moveto = TRUE;
        else Delete(e->prev); /* delete previous moveto */
        e = e->next;
    }
}

public procedure CheckBBoxEdge(e, vrt, lc, pf, pl)
PPathElt e; boolean vrt; Fixed lc, *pf, *pl; {
    FltnRec fr;
    Cd c0, c1, c2, c3;
    if (e->type != CURVETO)
    {
        FlushLogFiles();
        sprintf(globmsg, "Malformed path list in %s.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    
    GetEndPoint(e->prev, &c0.x, &c0.y);
    fr.report = chkBBDT;
    bbquit = FALSE;
    c1.x = e->x1; c1.y = e->y1;
    c2.x = e->x2; c2.y = e->y2;
    c3.x = e->x3; c3.y = e->y3;
    loc = lc; vert = vrt; started = FALSE;
    chkBBDT(c0);
    FltnCurve(c0, c1, c2, c3, &fr);
    *pf = frst; *pl = lst;
}

private procedure MakeColinear(tx, ty, x0, cy0, x1, cy1, xptr, yptr)
Fixed tx, ty, x0, cy0, x1, cy1, *xptr, *yptr; {
    Fixed dx, dy;
    real rdx, rdy, dxdy, dxsq, dysq, dsq, xi, yi, rx, ry, rx0, ry0;
    dx = x1-x0; dy = cy1-cy0;
    if (dx==0 && dy==0) { *xptr = tx; *yptr = ty; return; }
    if (dx==0) { *xptr = x0; *yptr = ty; return; }
    if (dy==0) { *xptr = tx; *yptr = cy0; return; }
    acfixtopflt(dx, &rdx); acfixtopflt(dy, &rdy);
    acfixtopflt(x0, &rx0); acfixtopflt(cy0, &ry0);
    acfixtopflt(tx, &rx); acfixtopflt(ty, &ry);
    dxdy = rdx*rdy; dxsq = rdx*rdx; dysq = rdy*rdy; dsq = dxsq+dysq;
    xi = (rx*dxsq+rx0*dysq+(ry-ry0)*dxdy)/dsq;
    yi = ry0+((xi-rx0)*rdy)/rdx;
    *xptr = acpflttofix(&xi);
    *yptr = acpflttofix(&yi);
}

#define DEG(x) ((x)*57.29577951308232088)
extern double atan2();
private Fixed ATan(a, b) Fixed a, b; {
    real aa, bb, cc;
    acfixtopflt(a, &aa); acfixtopflt(b, &bb);
    cc = (real)DEG(atan2((double)aa, (double)bb));
    while (cc < 0) cc += 360.0;
    return acpflttofix(&cc);
}

public boolean CheckSmoothness(x0, cy0, x1, cy1, x2, y2, pd)
Fixed x0, cy0, x1, cy1, x2, y2, *pd; {
    Fixed dx, dy, smdiff, smx, smy, at0, at1, abstmp;
    dx = x0 - x1; dy = cy0 - cy1;
    *pd = 0L;
    if (dx == 0 && dy == 0) return TRUE;
    at0 = ATan(dx, dy);
    dx = x1 - x2; dy = cy1 - y2;
    if (dx == 0 && dy == 0) return TRUE;
    at1 = ATan(dx, dy);
    smdiff = at0 - at1; if (smdiff < 0) smdiff = -smdiff;
    if (smdiff >= FixInt(180)) smdiff = FixInt(360) - smdiff;
    *pd = smdiff;
    if (smdiff == 0 || smdiff > FixInt(30)) return TRUE;
    MakeColinear(x1, cy1, x0, cy0, x2, y2, &smx, &smy);
    smx = FHalfRnd(smx);
    smy = FHalfRnd(smy);
    /* DEBUG 8 BIT. Double hard coded distance values, for change from 7 to 8 bits for fractions. */
    return ac_abs(smx - x1) < FixInt(4) && ac_abs(smy - cy1) < FixInt(4);
}

public procedure CheckForDups() {
    register PPathElt ob, nxt;
    register Fixed x, y;
    ob = pathStart;
    while (ob != NULL) {
        nxt = ob->next;
        if (ob->type == MOVETO) {
            x = ob->x; y = ob->y; ob = nxt;
            while (ob != NULL) {
                if (ob->type == MOVETO && x == ob->x && y == ob->y)
                    goto foundMatch;
                ob = ob->next;
            }
        }
        ob = nxt;
    }
    return;
foundMatch:
    x = itfmx(x); y = itfmy(y);
    ReportDuplicates(x, y);
}

public procedure MoveSubpathToEnd(e) PPathElt e; {
    PPathElt subEnd, subStart, subNext, subPrev;
    subEnd = (e->type == CLOSEPATH)? e : GetClosedBy(e);
    subStart = GetDest(subEnd);
    if (subEnd == pathEnd) return; /* already at end */
    subNext = subEnd->next;
    if (subStart == pathStart) {
        pathStart = subNext; subNext->prev = NULL; }
    else {
        subPrev = subStart->prev;
        subPrev->next = subNext;
        subNext->prev = subPrev;
    }
    pathEnd->next = subStart;
    subStart->prev = pathEnd;
    subEnd->next = NULL;
    pathEnd = subEnd;
}
