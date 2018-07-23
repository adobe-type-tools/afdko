/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* auto.c */

#include "ac.h"
#include "machinedep.h"

private boolean clrBBox, clrHBounds, clrVBounds, haveHBnds, haveVBnds,
  mergeMain;

public procedure InitAuto(reason) integer reason; {
  switch (reason) {
    case STARTUP:
    case RESTART:
      clrBBox = clrHBounds = clrVBounds = haveHBnds = haveVBnds = FALSE;
    }
  }

private PPathElt GetSubPathNxt(e) PPathElt e; {
  if (e->type == CLOSEPATH) return GetDest(e);
  return e->next; }

private PPathElt GetSubPathPrv(e) PPathElt e; {
  if (e->type == MOVETO) e = GetClosedBy(e);
  return e->prev; }

private PClrVal FindClosestVal(sLst, loc) PClrVal sLst; Fixed loc; {
  Fixed dist = FixInt(10000), bot, top, d;
  PClrVal best = NULL;
  while (sLst != NULL) {
    bot = sLst->vLoc1; top = sLst->vLoc2;
    if (bot > top) { Fixed tmp = bot; bot = top; top = tmp; }
    if (loc >= bot && loc <= top) { best = sLst; break; }
    if (loc < bot) d = bot - loc;
    else d = loc - top;
    if (d < dist) { dist = d; best = sLst; }
    sLst = sLst->vNxt;
    }
  return best;
  }

private procedure CpyHClr(e) PPathElt e; {
  Fixed x1, y1;
  PClrVal best;
  GetEndPoint(e, &x1, &y1);
  best = FindClosestVal(Hprimary, y1);
  if (best != NULL)
    AddHPair(best, 'b');
  }

private procedure CpyVClr(e) PPathElt e; {
  Fixed x1, y1;
  PClrVal best;
  GetEndPoint(e, &x1, &y1);
  best = FindClosestVal(Vprimary, x1);
  if (best != NULL)
    AddVPair(best, 'y');
  }

private procedure PruneColorSegs(e, hFlg) PPathElt e; boolean hFlg; {
  PSegLnkLst lst, nxt, prv;
  PSegLnk lnk;
  PClrSeg seg;
  PClrVal val;
  lst = hFlg ? e->Hs : e->Vs;
  prv = NULL;
  while (lst != NULL) {
    val = NULL;
    lnk = lst->lnk;
    if (lnk != NULL) {
      seg = lnk->seg;
      if (seg != NULL) val = seg->sLnk;
      }
    nxt = lst->next;
    if (val == NULL) { /* prune this one */
      if (prv == NULL) {
        if (hFlg) e->Hs = nxt;
	else e->Vs = nxt;
	}
      else prv->next = nxt;
      lst = nxt;
      }
    else { prv = lst; lst = nxt; }
    }
  }

public procedure PruneElementColorSegs() {
  register PPathElt e;
  e = pathStart;
  while (e != NULL) {
    PruneColorSegs(e,TRUE);
    PruneColorSegs(e,FALSE);
    e = e->next; }
  }

#define ElmntClrSegLst(e, hFlg) (hFlg) ? (e)->Hs : (e)->Vs

private procedure RemLnk(e,hFlg,rm)
  PPathElt e; boolean hFlg; PSegLnkLst rm; {
  PSegLnkLst lst, prv, nxt;
  lst = hFlg ? e->Hs : e->Vs;
  prv = NULL;
  while (lst != NULL) {
    nxt = lst->next;
    if (lst == rm) {
      if (prv == NULL) {
        if (hFlg) e->Hs = nxt; else e->Vs = nxt; }
      else prv->next = nxt;
      return; }
    prv = lst; lst = nxt;
    }
  FlushLogFiles();
  sprintf(globmsg, "Badly formatted segment list in file: %s.\n", fileName);
  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }

private boolean AlreadyOnList(v, lst)  register PClrVal v, lst; {
  while (lst != NULL) {
    if (v == lst) return TRUE;
    lst = lst->vNxt;
    }
  return FALSE;
  }

private procedure AutoVSeg(sLst) PClrVal sLst; {
  AddVPair(sLst, 'y');
  }

private procedure AutoHSeg(sLst) PClrVal sLst; {
  AddHPair(sLst, 'b');
  }

private procedure AddHColoring(h) PClrVal h; {
  if (useH || AlreadyOnList(h,Hcoloring)) return;
  h->vNxt = Hcoloring; Hcoloring = h;
  AutoHSeg(h); }

private procedure AddVColoring(v) PClrVal v; {
  if (useV || AlreadyOnList(v,Vcoloring)) return;
  v->vNxt = Vcoloring; Vcoloring = v;
  AutoVSeg(v); }

private integer TestColor(s, colorList, flg, doLst)
  PClrSeg s; PClrVal colorList; boolean flg, doLst; {
  /* -1 means already in colorList; 0 means conflicts; 1 means ok to add */
  PClrVal v, clst;
  Fixed top, bot, cTop, cBot, vT, vB, loc, abstmp;
  boolean loc1;
  if (s == NULL) return -1L;
  v = s->sLnk; loc = s->sLoc;
  if (v == NULL) return -1L;
  vT = top = v->vLoc2;
  vB = bot = v->vLoc1;
  if (v->vGhst) { /* collapse width for conflict test */
    if (v->vSeg1->sType == sGHOST) bot = top;
    else top = bot;
    }
  if (DEBUG) {
    integer cnt = 0;
    clst = colorList;
    while (clst != NULL) {
      if (++cnt > 100) {
        LogMsg("Loop in hintlist for TestHint\n\007", WARNING, OK, TRUE);
	return 0L; }
      clst = clst->vNxt; }
    }
  if (v->vGhst) {
    /* if best value for segment uses a ghost, and
       segment loc is already in the colorList, then return -1 */
    clst = colorList;
    if (ac_abs(loc-vT) < ac_abs(loc-vB)) { loc1 = FALSE; loc = vT; }
    else { loc1 = TRUE; loc = vB; }
    while (clst != NULL) {
      if ((loc1 ? clst->vLoc1 : clst->vLoc2) == loc)
        return -1;
      clst = clst->vNxt;
      if (!doLst) break; }
    }
  if (flg) {top += bandMargin; bot -= bandMargin;}
  else {top -= bandMargin; bot += bandMargin;}
  while (colorList != NULL) { /* check for conflict */
    cTop = colorList->vLoc2; cBot = colorList->vLoc1;
    if (vB == cBot && vT == cTop) {
      return -1L; }
    if (colorList->vGhst) { /* collapse width for conflict test */
      if (colorList->vSeg1->sType == sGHOST) cBot = cTop;
      else cTop = cBot;
      }
    if ((flg && (cBot <= top) && (cTop >= bot)) ||
       (!flg && (cBot >= top) && (cTop <= bot))) {
         return 0L; }
    colorList = colorList->vNxt;
    if (!doLst) break; }
  return 1L;}

#define TestHColorLst(h) TestColorLst(h, Hcoloring, YgoesUp, TRUE)
#define TestVColorLst(v) TestColorLst(v, Vcoloring, TRUE, TRUE)

public int TestColorLst(lst, colorList, flg, doLst)
  PSegLnkLst lst; PClrVal colorList; boolean flg, doLst; {
  /* -1 means already in colorList; 0 means conflicts; 1 means ok to add */
  int result, i, cnt;
  result = -1; cnt = 0;
  while (lst != NULL) {
    i = TestColor(lst->lnk->seg, colorList, flg, doLst);
    if (i == 0) { result = 0; break; }
    if (i == 1) result = 1L;
    lst = lst->next;
    if (++cnt > 100) {
      LogMsg("Looping in TestHintLst\007\n", WARNING, OK, TRUE);
      return 0L; }
    }
  return result; }

#define FixedMidPoint(m,a,b) \
  (m).x=((a).x+(b).x)>>1; \
  (m).y=((a).y+(b).y)>>1

#define FixedBezDiv(a0, a1, a2, a3, b0, b1, b2, b3) \
  FixedMidPoint(b2, a2, a3); \
  FixedMidPoint(a3, a1, a2); \
  FixedMidPoint(a1, a0, a1); \
  FixedMidPoint(a2, a1, a3); \
  FixedMidPoint(b1, a3, b2); \
  FixedMidPoint(a3, a2, b1);

public boolean ResolveConflictBySplit(e,Hflg,lnk1,lnk2)
  register PPathElt e; boolean Hflg; PSegLnkLst lnk1, lnk2; {
  /* insert new pathelt immediately following e */
  /* e gets first half of split; new gets second */
  /* e gets lnk1 in Hs or Vs; new gets lnk2 */
  register PPathElt new;
  Cd d0, d1, d2, d3, d4, d5, d6, d7;
  if (e->type != CURVETO || e->isFlex) return FALSE;
  ReportSplit(e);
  new = (PPathElt)Alloc(sizeof(PathElt));
  new->next = e->next; e->next = new; new->prev = e;
  if (new->next == NULL) pathEnd = new;
  else new->next->prev = new;
  if (Hflg) { e->Hs = lnk1; new->Hs = lnk2; }
  else { e->Vs = lnk1; new->Vs = lnk2; }
  if (lnk1 != NULL) lnk1->next = NULL;
  if (lnk2 != NULL) lnk2->next = NULL;
  new->type = CURVETO;
  GetEndPoint(e->prev, &d0.x, &d0.y);
  d1.x = e->x1; d1.y = e->y1;
  d2.x = e->x2; d2.y = e->y2;
  d3.x = e->x3; d3.y = e->y3;
  d4 = d0; d5 = d1; d6 = d2; d7 = d3;
  new->x3 = d3.x; new->y3 = d3.y;
  FixedBezDiv(d4, d5, d6, d7, d0, d1, d2, d3);
  e->x1 = d5.x; e->y1 = d5.y;
  e->x2 = d6.x; e->y2 = d6.y;
  e->x3 = d7.x; e->y3 = d7.y;
  new->x1 = d1.x; new->y1 = d1.y;
  new->x2 = d2.x; new->y2 = d2.y;
  return TRUE;
  }

private procedure RemDupLnks(e,Hflg) PPathElt e; boolean Hflg; {
  PSegLnkLst l1, l2, l2nxt;
  l1 = Hflg ? e->Hs : e->Vs;
  while (l1 != NULL) {
    l2 = l1->next;
    while (l2 != NULL) {
      l2nxt = l2->next;
      if (l1->lnk->seg == l2->lnk->seg)
        RemLnk(e,Hflg,l2);
      l2 = l2nxt;
      }
    l1 = l1->next;
    }
  }

#define OkToRemLnk(loc,Hflg,spc) \
  (!(Hflg) || (spc) == 0 || \
    (!InBlueBand((loc),lenTopBands,topBands) && \
     !InBlueBand((loc),lenBotBands,botBands)))

/* The changes made here were to fix a problem in MinisterLight/E.
   The top left point was not getting colored. */
private boolean TryResolveConflict(e,Hflg)
  PPathElt e; boolean Hflg; {
  integer typ;
  PSegLnkLst lst, lnk1, lnk2;
  PClrSeg seg, seg1, seg2;
  PClrVal val1, val2;
  Fixed lc1, lc2, loc0, loc1, loc2, loc3, x0, y0, x1, y1, abstmp;
  RemDupLnks(e,Hflg);
  typ = e->type;
  if (typ == MOVETO) GetEndPoints(GetClosedBy(e), &x0, &y0, &x1, &y1);
  else if (typ == CURVETO) {
    x0 = e->x1; y0 = e->y1; x1 = e->x3; y1 = e->y3;
    }
  else GetEndPoints(e, &x0, &y0, &x1, &y1);
  loc1 = Hflg? y0 : x0; loc2 = Hflg? y1 : x1;
  lst = Hflg ? e->Hs : e->Vs;
  seg1 = lst->lnk->seg; lc1 = seg1->sLoc; lnk1 = lst;
  lst = lst->next;
  seg2 = lst->lnk->seg; lc2 = seg2->sLoc; lnk2 = lst;
  if (lc1==loc1 || lc2==loc2) {}
  else if (ac_abs(lc1-loc1) > ac_abs(lc1-loc2) ||
           ac_abs(lc2-loc2) > ac_abs(lc2-loc1)) {
    seg = seg1; seg1 = seg2; seg2 = seg;
    lst = lnk1; lnk1 = lnk2; lnk2 = lst;
    }
  val1 = seg1->sLnk; val2 = seg2->sLnk;
  if (val1->vVal < FixInt(50) && OkToRemLnk(loc1,Hflg,val1->vSpc)) {
    RemLnk(e,Hflg,lnk1);
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  if (val2->vVal < FixInt(50) && val1->vVal > val2->vVal * 20L &&
      OkToRemLnk(loc2,Hflg,val2->vSpc)) {
    RemLnk(e,Hflg,lnk2);  
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  if (typ != CURVETO ||
      ((((Hflg && IsHorizontal(x0,y0,x1,y1)) ||
        (!Hflg && IsVertical(x0,y0,x1,y1)))) &&
        OkToRemLnk(loc1,Hflg,val1->vSpc))) {
    RemLnk(e,Hflg,lnk1);
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  GetEndPoints(GetSubPathPrv(e), &x0, &y0, &x1, &y1);
  loc0 = Hflg? y0 : x0;
  if (ProdLt0(loc2-loc1,loc0-loc1)) {
    RemLnk(e,Hflg,lnk1);
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  GetEndPoint(GetSubPathNxt(e), &x1, &y1);
  loc3 = Hflg? y1 : x1;
  if (ProdLt0(loc3-loc2,loc1-loc2)) {
    RemLnk(e,Hflg,lnk2);  
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  if ((loc2 == val2->vLoc1 || loc2 == val2->vLoc2) &&
      loc1 != val1->vLoc1 && loc1 != val1->vLoc2) {
    RemLnk(e,Hflg,lnk1);
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  if ((loc1 == val1->vLoc1 || loc1 == val1->vLoc2) &&
      loc2 != val2->vLoc1 && loc2 != val2->vLoc2) {
    RemLnk(e,Hflg,lnk2);
    if (showClrInfo) ReportRemConflict(e);
    return TRUE; }
  if (editChar && ResolveConflictBySplit(e,Hflg,lnk1,lnk2))
    return TRUE;
  else return FALSE;
  }

private boolean CheckColorSegs(PPathElt e, boolean flg, boolean Hflg) 
{
  PSegLnkLst lst;
  PSegLnkLst lst2;
  PClrSeg seg;
  PClrVal val;
  lst = Hflg ? e->Hs : e->Vs;
  while (lst != NULL) {
    lst2 = lst->next;
    if (lst2 != NULL) {
      seg = lst->lnk->seg; val = seg->sLnk;
      if (val != NULL && TestColorLst(lst2,val,flg,FALSE)==0) {
        if (TryResolveConflict(e,Hflg))
          return CheckColorSegs(e,flg,Hflg);
        AskForSplit(e);
	if (Hflg) e->Hs = NULL; else e->Vs = NULL;
        return TRUE;
        }
      }
    lst = lst2;
    }
  return FALSE;
  }

private procedure CheckElmntClrSegs() {
  PPathElt e;
  e = pathStart;
  while (e != NULL) {
    if (!CheckColorSegs(e,YgoesUp,TRUE))
      (void)CheckColorSegs(e,TRUE,FALSE);
    e = e->next; }
  }
private boolean ClrLstsClash(lst1,lst2,flg)
  PSegLnkLst lst1, lst2; boolean flg; {
  PClrSeg seg;
  PClrVal val;
  PSegLnkLst lst;
  while (lst1 != NULL) {
    seg = lst1->lnk->seg; val = seg->sLnk;
    if (val != NULL) {
      lst = lst2;
      while (lst != NULL) {
        if (TestColorLst(lst,val,flg,FALSE)==0) {
          return TRUE;
          }
        lst = lst->next;
        }
      }
    lst1 = lst1->next;
    }
  return FALSE;
  }

private PSegLnkLst BestFromLsts(lst1,lst2) PSegLnkLst lst1, lst2; {
  PSegLnkLst lst, bst;
  PClrSeg seg;
  PClrVal val;
  Fixed bstval;
  integer i;
  bst = NULL; bstval = 0;
  for (i = 0; i < 2; i++) {
    lst = i? lst1 : lst2;
    while (lst != NULL) {
      seg = lst->lnk->seg; val = seg->sLnk;
      if (val != NULL && val->vVal > bstval) {
        bst = lst; bstval = val->vVal; }
      lst = lst->next;
      }
    }
  return bst;
  }

private boolean ClrsClash(e, p, hLst, vLst, phLst, pvLst)
  PPathElt e, p; PSegLnkLst *hLst, *vLst, *phLst, *pvLst; {
  boolean clash = FALSE;
  PSegLnkLst bst, new;
  if (ClrLstsClash(*hLst,*phLst,YgoesUp)) {
    clash = TRUE;
    bst = BestFromLsts(*hLst,*phLst);
    if (bst) {
      new = (PSegLnkLst)Alloc(sizeof(SegLnkLst));
      new->next = NULL; new->lnk = bst->lnk;
      }
    else new = NULL;
    e->Hs = p->Hs = *hLst = *phLst = new;
    }
  if (ClrLstsClash(*vLst,*pvLst,TRUE)) {
    clash = TRUE;
    bst = BestFromLsts(*vLst,*pvLst);
    if (bst) {
      new = (PSegLnkLst)Alloc(sizeof(SegLnkLst));
      new->next = NULL; new->lnk = bst->lnk;
      }
    else new = NULL;
    e->Vs = p->Vs = *vLst = *pvLst = new;
    }
  return clash;
  }

private procedure GetColorLsts(e, phLst, pvLst, ph, pv) 
  PPathElt e; PSegLnkLst *phLst, *pvLst; integer *ph, *pv; {
  PSegLnkLst hLst, vLst;
  integer h, v;
  if (useH) {
    hLst = NULL; h = -1; }
  else {
    hLst = e->Hs;
    if (hLst == NULL) h = -1;
    else h = TestHColorLst(hLst);
    }
  if (useV) {
    vLst = NULL; v = -1; }
  else {
    vLst = e->Vs;
    if (vLst == NULL) v = -1;
    else v = TestVColorLst(vLst);
    }
  *pvLst = vLst; *phLst = hLst;
  *ph = h; *pv = v;
  }

private procedure ReClrBounds(e) PPathElt e; {
  if (!useH) {
    if (clrHBounds && Hcoloring==NULL && !haveHBnds)
      ReClrHBnds();
    else if (!clrBBox) {
      if (Hcoloring==NULL) CpyHClr(e);
      if (mergeMain) MergeFromMainColors('b');
      }
    }
  if (!useV) {
    if (clrVBounds && Vcoloring==NULL && !haveVBnds)
      ReClrVBnds();
    else if (!clrBBox) {
      if (Vcoloring==NULL) CpyVClr(e);
      if (mergeMain) MergeFromMainColors('y');
      }
    }
  }

private procedure AddColorLst(lst,vert) PSegLnkLst lst; boolean vert; {
  PClrVal val;
  PClrSeg seg;
  while (lst != NULL) {
    seg = lst->lnk->seg;
    val = seg->sLnk;
    if (vert) AddVColoring(val);
    else AddHColoring(val);
    lst = lst->next;
    }
  }

private procedure StartNewColoring(e, hLst, vLst)
  PPathElt e; PSegLnkLst hLst, vLst; {
  ReClrBounds(e);
  if (e->newcolors != 0)
	{
	  FlushLogFiles();
	  sprintf(globmsg, "Uninitialized extra hints list in file: %s.\n", fileName);
	  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
	}
  XtraClrs(e);
  clrBBox = FALSE;
  if (useV) CopyMainV();
  if (useH) CopyMainH();
  Hcoloring = Vcoloring = NULL;
  if (!useH) AddColorLst(hLst,FALSE);
  if (!useV) AddColorLst(vLst,TRUE);
  }

private boolean IsIn(h,v) integer h, v; {
  return (h == -1 && v == -1);
  }

private boolean IsOk(h,v) integer h, v; {
  return (h != 0 && v != 0);
  }

#define AddIfNeedV(v,vLst) if (!useV && v == 1) AddColorLst(vLst,TRUE)
#define AddIfNeedH(h,hLst) if (!useH && h == 1) AddColorLst(hLst,FALSE)

private procedure SetHColors(lst) PClrVal lst; {
  if (useH) return;
  Hcoloring = lst;
  while (lst != NULL) {
    AutoHSeg(lst);
    lst = lst->vNxt;
    }
  }

private procedure SetVColors(lst) PClrVal lst; {
  if (useV) return;
  Vcoloring = lst;
  while (lst != NULL) {
    AutoVSeg(lst);
    lst = lst->vNxt;
    }
  }

public PClrVal CopyClrs(lst) PClrVal lst; {
  PClrVal v, vlst;
  int cnt;
  vlst = NULL; cnt = 0;
  while (lst != NULL) {
    v = (PClrVal)Alloc(sizeof(ClrVal));
    *v = *lst;
    v->vNxt = vlst;
    vlst = v; 
    if (++cnt > 100) {
      LogMsg("Loop in CopyClrs\007\n", WARNING, OK, TRUE);
      return vlst; }
    lst = lst->vNxt; }
  return vlst; }

private PPathElt ColorBBox(e) PPathElt e; {
  e = FindSubpathBBox(e);
  ClrBBox();
  clrBBox = TRUE;
  return e;
  }

private boolean IsFlare(loc,e,n,Hflg) Fixed loc; PPathElt e, n; boolean Hflg; {
  Fixed x, y, abstmp;
  while (e != n) {
    GetEndPoint(e,&x,&y);
    if ((Hflg && ac_abs(y-loc) > maxFlare) || (!Hflg && ac_abs(x-loc) > maxFlare))
      return FALSE;
    e = GetSubPathNxt(e);
    }
  return TRUE;
  }

private boolean IsTopSegOfVal(loc, top, bot) Fixed loc, top, bot; {
  Fixed d1, d2, abstmp;
  d1 = top-loc; d2 = bot-loc;
  return (ac_abs(d1) <= ac_abs(d2))? TRUE : FALSE;
  }

private procedure RemFlareLnk(e, hFlg, rm, e2, i)
  PPathElt e, e2; boolean hFlg; PSegLnkLst rm; integer i; {
  RemLnk(e,hFlg,rm); if (showClrInfo) ReportRemFlare(e,e2,hFlg,i); }

public boolean CompareValues(val1,val2,factor,ghstshift)
  register PClrVal val1, val2; integer factor, ghstshift; {
  register Fixed v1 = val1->vVal, v2 = val2->vVal, mx;
  mx = v1 > v2 ? v1 : v2; mx <<= 1;
  while (mx > 0) { mx <<= 1; v1 <<= 1; v2 <<= 1; }
  if (ghstshift > 0 && val1->vGhst != val2->vGhst) {
    if (val1->vGhst) v1 >>= ghstshift;
    if (val2->vGhst) v2 >>= ghstshift;
    }
  if ((val1->vSpc > 0 && val2->vSpc > 0) ||
      (val1->vSpc == 0 && val2->vSpc == 0))
    return v1 > v2;
  if (val1->vSpc > 0L)
    return (v1 < FixedPosInf / factor) ?
             (v1 * factor > v2) : (v1 > v2 / factor);
  return (v2 < FixedPosInf/factor) ?
             (v1 > v2 * factor) : (v1 / factor > v2);
  }

private procedure RemFlares(Hflg) boolean Hflg; {
  PSegLnkLst lst1, lst2, nxt1, nxt2;
  PPathElt e, n;
  PClrSeg seg1, seg2;
  PClrVal val1, val2;
  Fixed diff, abstmp;
  boolean nxtE;
  boolean Nm1, Nm2;
  if (Hflg) {Nm1 = TRUE; Nm2 = FALSE;} else {Nm1 = FALSE; Nm2 = TRUE;}
  e = pathStart;
  while (e != NULL) {
    if (Nm1 ? e->Hs == NULL : e->Vs == NULL) { e = e->next; continue; }
    /* e now is an element with Nm1 prop */
    n = GetSubPathNxt(e); nxtE = FALSE;
    while (n != e && !nxtE) {
      if (Nm1 ? n->Hs != NULL : n->Vs != NULL) {
        lst1 = ElmntClrSegLst(e,Nm1);
	while (lst1 != NULL) {
	  seg1 = lst1->lnk->seg;
	  nxt1 = lst1->next;
	  lst2 = ElmntClrSegLst(n,Nm1);
	  while (lst2 != NULL) {
            seg2 = lst2->lnk->seg;
	    nxt2 = lst2->next;
            if (seg1 != NULL && seg2 != NULL) {
              diff = seg1->sLoc - seg2->sLoc;
              if (ac_abs(diff) > maxFlare) {nxtE = TRUE; goto Nxt2;}
              if (!IsFlare(seg1->sLoc,e,n,Hflg)) {nxtE = TRUE; goto Nxt2;}
              val1 = seg1->sLnk; val2 = seg2->sLnk;
              if (diff != 0 &&
                  IsTopSegOfVal(seg1->sLoc, val1->vLoc2, val1->vLoc1)==
		  IsTopSegOfVal(seg2->sLoc, val2->vLoc2, val2->vLoc1)) {
                if (CompareValues(val1,val2,spcBonus,0L)) {
                  /* This change was made to fix flares in Bodoni2. */
                  if (val2->vSpc == 0 && val2->vVal < FixInt(1000))
                     RemFlareLnk(n,Nm1,lst2,e,1); }
                  else if (val1->vSpc == 0 && val1->vVal < FixInt(1000)) {
                   RemFlareLnk(e,Nm1,lst1,n,2); goto Nxt1; }
                }
              }
            Nxt2: lst2 = nxt2;
            }
          Nxt1: lst1 = nxt1;
	  }
        }
      if (Nm2 ? n->Hs != NULL : n->Vs != NULL) break;
      n = GetSubPathNxt(n);
      }
    e = e->next; }
  }

private procedure CarryIfNeed(loc,vert,clrs)
  PClrVal clrs; Fixed loc; boolean vert; {
  PClrSeg seg;
  PClrVal seglnk;
  Fixed l0, l1, tmp, halfMargin;
  if ((vert && useV) || (!vert && useH)) return;
  halfMargin = FixHalfMul(bandMargin);
  /* DEBUG 8 BIT. Needed to double test from 10 to 20 for change in coordinate system */
  if (halfMargin > FixInt(20)) halfMargin = FixInt(20);
  while (clrs != NULL) {
    seg = clrs->vSeg1;
    if (clrs->vGhst && seg->sType == sGHOST) seg = clrs->vSeg2;
    if (seg == NULL) goto Nxt;
    l0 = clrs->vLoc2; l1 = clrs->vLoc1;
    if (l0 > l1) { tmp = l1; l1 = l0; l0 = tmp; }
    l0 -= halfMargin; l1 += halfMargin;
    if (loc > l0 && loc < l1) {
      seglnk = seg->sLnk; seg->sLnk = clrs;
      if (vert) {
        if (TestColor(seg,Vcoloring,TRUE,TRUE) == 1) {
          if (showClrInfo)
              ReportCarry(l0, l1, loc, clrs, vert);
          AddVColoring(clrs);
	  seg->sLnk = seglnk;
	  break; }
        }
      else if (TestColor(seg,Hcoloring,YgoesUp,TRUE) == 1) {
        if (showClrInfo)
            ReportCarry(l0, l1, loc, clrs, vert);
        AddHColoring(clrs);
	seg->sLnk = seglnk;
	break; }
      seg->sLnk = seglnk;
      }
    Nxt: clrs = clrs->vNxt;
    }
  }

#define PRODIST (FixInt(100)) /* DEBUG 8 BIT. Needed to double test from 50 to 100 for change in coordinate system */
private procedure ProClrs(e,hFlg,loc)
  PPathElt e; Fixed loc; boolean hFlg; {
  PSegLnkLst lst, plst;
  PPathElt prv;
  Fixed cx, cy, dst, abstmp;
  lst = ElmntClrSegLst(e, hFlg);
  if (lst == NULL) return;
  if (hFlg ? e->Hcopy : e->Vcopy) return;
  prv = e;
  while (TRUE) {
    prv = GetSubPathPrv(prv);
    plst = ElmntClrSegLst(prv, hFlg);
    if (plst != NULL) return;
    GetEndPoint(prv, &cx, &cy);
    dst = (hFlg ? cy : cx) - loc;
    if (ac_abs(dst) > PRODIST) return;
    if (hFlg) { prv->Hs = lst; prv->Hcopy = TRUE; }
    else { prv->Vs = lst; prv->Vcopy = TRUE; }
    }
  }

private procedure PromoteColors() {
  PPathElt e;
  Fixed cx, cy;
  e = pathStart;
  while (e != NULL) {
    GetEndPoint(e, &cx, &cy);
    ProClrs(e, TRUE, cy);
    ProClrs(e, FALSE, cx);
    e = e->next;
    }
  }

private procedure RemPromotedClrs() {
  PPathElt e;
  e = pathStart;
  while (e != NULL) {
    if (e->Hcopy) { e->Hs = NULL; e->Hcopy = FALSE; }
    if (e->Vcopy) { e->Vs = NULL; e->Vcopy = FALSE; }
    e = e->next;
    }
  }

private procedure RemShortColors() {
  /* Must not change colors at a short element. */
  PPathElt e;
  Fixed cx, cy, ex, ey, abstmp;
  e = pathStart;
  cx = 0; cy = 0;
  while (e != NULL) {
    GetEndPoint(e, &ex, &ey);
    if (ac_abs(cx-ex) < minColorElementLength &&
        ac_abs(cy-ey) < minColorElementLength) {
      if (showClrInfo) ReportRemShortColors(ex, ey);
      e->Hs = NULL; e->Vs = NULL;
      }
    e = e->next; cx = ex; cy = ey;
    }
  }

public procedure AutoExtraColors(movetoNewClrs, soleol, solWhere)
boolean movetoNewClrs, soleol; integer solWhere; {
    integer h, v, ph, pv;
    PPathElt e, cp, p;
    integer etype;
    PSegLnkLst hLst, vLst, phLst, pvLst;
    PClrVal mtVclrs, mtHclrs, prvHclrs, prvVclrs;
    
    boolean (*Tst)(), newClrs = TRUE;
    boolean isSpc;
    Fixed x, y;
    
    isSpc = clrBBox = clrVBounds = clrHBounds = FALSE;
    mergeMain = (CountSubPaths() <= 5);
    e = pathStart;
    if (AutoExtraDEBUG) PrintMessage("RemFlares");
    RemFlares(TRUE); RemFlares(FALSE);
    if (AutoExtraDEBUG) PrintMessage("CheckElmntClrSegs");
    CheckElmntClrSegs();
    if (AutoExtraDEBUG) PrintMessage("PromoteColors");
    PromoteColors();
    if (AutoExtraDEBUG) PrintMessage("RemShortColors");
    RemShortColors();
    haveVBnds = clrVBounds;
    haveHBnds = clrHBounds;
    p = NULL;
    Tst = IsOk; /* it is ok to add to primary coloring */
    if (AutoExtraDEBUG) PrintMessage("color loop");
    mtVclrs = mtHclrs = NULL;
    while (e != NULL) {
        etype = e->type;
        if (movetoNewClrs && etype == MOVETO) {
            StartNewColoring(e, (PSegLnkLst)NULL, (PSegLnkLst)NULL); Tst = IsOk; }
        if (soleol && etype == MOVETO) { /* start new coloring on soleol mt */
            if ((solWhere == 1 && IsUpper(e)) ||
                (solWhere == -1 && IsLower(e)) ||
                (solWhere == 0)) { /* color bbox of next subpath */
                StartNewColoring(e, (PSegLnkLst)NULL, (PSegLnkLst)NULL);
                Tst = IsOk; haveHBnds = haveVBnds = isSpc = TRUE;
                e = ColorBBox(e); continue; }
            else if (isSpc) { /* new coloring after the special */
                StartNewColoring(e, (PSegLnkLst)NULL, (PSegLnkLst)NULL);
                Tst = IsOk; haveHBnds = haveVBnds = isSpc = FALSE; }
        }
        if (newClrs && e == p) {
            StartNewColoring(e, (PSegLnkLst)NULL, (PSegLnkLst)NULL);
            SetHColors(mtHclrs); SetVColors(mtVclrs);
            Tst = IsIn;
        }
        GetColorLsts(e, &hLst, &vLst, &h, &v);
        if (etype == MOVETO && IsShort(cp = GetClosedBy(e))) {
            GetColorLsts(p = cp->prev, &phLst, &pvLst, &ph, &pv);
            if (ClrsClash(e, p, &hLst, &vLst, &phLst, &pvLst)) {
                GetColorLsts(e, &hLst, &vLst, &h, &v);
                GetColorLsts(p, &phLst, &pvLst, &ph, &pv);
            }
            if (!(*Tst)(ph,pv) || !(*Tst)(h,v)) {
                StartNewColoring(e, hLst, vLst); Tst = IsOk;
                ph = pv = 1; /* force add of colors for p also */
            }
            else { AddIfNeedH(h,hLst); AddIfNeedV(v,vLst); }
            AddIfNeedH(ph,phLst); AddIfNeedV(pv,pvLst);
            newClrs = FALSE; /* so can tell if added new colors in subpath */
        }
        else if (!(*Tst)(h,v)) { /* e needs new coloring */
            if (etype == CLOSEPATH) { /* do not attach extra colors to closepath */
                e = e->prev; GetColorLsts(e, &hLst, &vLst, &h, &v); }
            prvHclrs = CopyClrs(Hcoloring);
            prvVclrs = CopyClrs(Vcoloring);
            if (!newClrs) { /* this is the first extra since mt */
                newClrs = TRUE;
                mtVclrs = CopyClrs(prvVclrs);
                mtHclrs = CopyClrs(prvHclrs); }
            StartNewColoring(e, hLst, vLst);
            Tst = IsOk;
            if (etype == CURVETO) { x = e->x1; y = e->y1; }
            else GetEndPoint(e, &x, &y);
            CarryIfNeed(y,FALSE,prvHclrs);
            CarryIfNeed(x,TRUE,prvVclrs);
        }
        else { /* do not need to start new coloring */
            AddIfNeedH(h,hLst);
            AddIfNeedV(v,vLst);
        }
        e = e->next; }
    ReClrBounds(pathEnd);
    if (AutoExtraDEBUG) PrintMessage("RemPromotedClrs");
    RemPromotedClrs();
    if (AutoExtraDEBUG) PrintMessage("done autoextracolors");
}

