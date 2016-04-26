/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* merge.c */


#include "ac.h"

private boolean CloseElements(e1,e2,loc1,loc2,vert)
/* TRUE iff you can go from e1 to e2 without going out of band loc1..loc2 */
/* if vert is TRUE, then band is vert (test x values) */
/* else band is horizontal (test y values) */
/* band is expanded by CLSMRG in each direction */
#define CLSMRG (PSDist(20))
register PPathElt e1, e2;
register Fixed loc1, loc2;
register boolean vert; {
	register Fixed tmp;
	Fixed x, y;
	register PPathElt e;
	if (e1 == e2) return TRUE;
	if (loc1 < loc2)
		{ 
		if ((loc2 -loc1) > 5*CLSMRG)
			return FALSE;
		loc1 -= CLSMRG;
		 loc2 += CLSMRG; 
		 }
	else 
		{
		if ((loc1 -loc2) > 5*CLSMRG)
			return FALSE;
		 tmp = loc1;
		  loc1 = loc2-CLSMRG;
		   loc2 = tmp+CLSMRG; 
		   }
		
	e = e1;
	while (TRUE) {
		if (e == e2) return TRUE;
		GetEndPoint(e,&x,&y);
		tmp = vert? x : y;
		if (tmp > loc2 || tmp < loc1) return FALSE;
		if (e->type == CLOSEPATH) e = GetDest(e);
		else e = e->next;
		if (e == e1) return FALSE; }
}

public boolean CloseSegs(s1,s2,vert) PClrSeg s1, s2; boolean vert; {
	/* TRUE if the elements for these segs are "close" in the path */
	PPathElt e1, e2;
	Fixed loc1, loc2;
	if (s1 == s2) return TRUE;
	e1 = s1->sElt; e2 = s2->sElt;
	if (e1 == NULL || e2 == NULL) return TRUE;
	loc1 = s1->sLoc; loc2 = s2->sLoc;
	return (CloseElements(e1,e2,loc1,loc2,vert) ||
			CloseElements(e2,e1,loc2,loc1,vert))? TRUE : FALSE;
}

public procedure DoPrune() {
	/* Step through valList to the first item which is not pruned; set
	that to be the head of the list. Then remove from the list
	any subsequent element for which 'pruned' is true.
	*/
	PClrVal vL = valList, vPrv;
	while (vL != NULL && vL->pruned)
		vL = vL->vNxt;
	valList = vL;
	if (vL == NULL) return;
	vPrv = vL; vL = vL->vNxt;
	while (vL != NULL) {
		if (vL->pruned)
			vPrv->vNxt = vL = vL->vNxt;
		else {
			vPrv = vL; vL = vL->vNxt; }
    }
}

private PClrVal PruneOne(sLst, hFlg, sL, i)
PClrVal sLst, sL; boolean hFlg; integer i; {
/* Simply set the 'pruned' field to True for sLst. */
	if (hFlg) ReportPruneHVal(sLst,sL,i);
	else ReportPruneVVal(sLst,sL,i);
	sLst->pruned = TRUE;
	return sLst->vNxt; }

#define PRNDIST (PSDist(10))
#define PRNFCTR (3L)

#define PruneLt(val, v) \
(((v) < (FixedPosInf/10L) && (val) < (FixedPosInf/PRNFCTR)) ? \
((val) * PRNFCTR < (v) * 10L) : ((val)/10L < (v)/PRNFCTR))
#define PruneLe(val, v) \
(((val) < (FixedPosInf/PRNFCTR)) ? \
((v) <= (val) * PRNFCTR) : ((v)/PRNFCTR <= (val)))
#define PruneGt(val, v) \
(((val) < (FixedPosInf/PRNFCTR)) ? \
((v) > (val) * PRNFCTR) : ((v)/PRNFCTR > (val)))
#define MUCHFCTR (50L)
#define PruneMuchGt(val, v) \
(((val) < (FixedPosInf/MUCHFCTR)) ? \
((v) > (val) * MUCHFCTR) : ((v)/MUCHFCTR > (val)))
#define VERYMUCHFCTR (100L)
#define PruneVeryMuchGt(val, v) \
(((val) < (FixedPosInf/VERYMUCHFCTR)) ? \
((v) > (val) * VERYMUCHFCTR) : ((v)/VERYMUCHFCTR > (val)))

/* The changes made here and in PruneHVals are to fix a bug in
 MinisterLight/E where the top left point was not getting colored. */
public procedure PruneVVals() {
	PClrVal sLst, sL, sPrv;
	PClrSeg seg1, seg2, sg1, sg2;
	Fixed lft, rht, l, r, prndist;
	Fixed val, v, abstmp;
	boolean flg, otherLft, otherRht;
	sLst = valList;
	sPrv = NULL;
	prndist = PRNDIST;
	while (sLst != NULL) {
		flg = TRUE; otherLft = otherRht = FALSE;
		val = sLst->vVal;
		lft = sLst->vLoc1;
		rht = sLst->vLoc2;
		seg1 = sLst->vSeg1;
		seg2 = sLst->vSeg2;
		sL = valList;
		while (sL != NULL) {
			v = sL->vVal; sg1 = sL->vSeg1; sg2 = sL->vSeg2;
			l = sL->vLoc1; r = sL->vLoc2;
			if ((l==lft && r==rht) || PruneLe(val, v)) goto NxtSL;
			if (rht+prndist >= r && lft-prndist <= l &&
				(val < FixInt(100L) && PruneMuchGt(val, v)?
				 (CloseSegs(seg1,sg1,TRUE) || CloseSegs(seg2,sg2,TRUE)) :
				 (CloseSegs(seg1,sg1,TRUE) && CloseSegs(seg2,sg2,TRUE)))) {
					sLst = PruneOne(sLst,FALSE,sL,1L);
					flg = FALSE; break; }
			if (seg1 != NULL && seg2 != NULL) {
				if (ac_abs(l-lft) < FixOne) {
					if (!otherLft && PruneLt(val,v) && ac_abs(l-r) < ac_abs(lft-rht) &&
						CloseSegs(seg1,sg1,TRUE))
						otherLft = TRUE;
					if (seg2->sType == sBEND && CloseSegs(seg1,sg1,TRUE)) {
						sLst = PruneOne(sLst,FALSE,sL,2L);
						flg = FALSE; break; }}
				if (ac_abs(r-rht) < FixOne) {
					if (!otherRht && PruneLt(val,v) && ac_abs(l-r) < ac_abs(lft-rht) &&
						CloseSegs(seg2,sg2,TRUE))
						otherRht = TRUE;
					if (seg1->sType == sBEND && CloseSegs(seg2,sg2,TRUE)) {
						sLst = PruneOne(sLst,FALSE,sL,3L);
						flg = FALSE; break; }}
				if (otherLft && otherRht) {
					sLst = PruneOne(sLst,FALSE,sL,4L);
					flg = FALSE; break; }
			}
			NxtSL: sL = sL->vNxt; }
		if (flg) { sPrv = sLst; sLst = sLst->vNxt; }
    }
	DoPrune();
}

#define Fix16 (FixOne << 4)
public procedure PruneHVals() {
	PClrVal sLst, sL, sPrv;
	PClrSeg seg1, seg2, sg1, sg2;
	Fixed bot, top, t, b;
	Fixed val, v, abstmp, prndist;
	boolean flg, otherTop, otherBot, topInBlue, botInBlue, ghst;
	sLst = valList;
	sPrv = NULL;
	prndist = PRNDIST;
	while (sLst != NULL) {
		flg = TRUE; otherTop = otherBot = FALSE;
		seg1 = sLst->vSeg1; seg2 = sLst->vSeg2; /* seg1 is bottom, seg2 is top */
		ghst = sLst->vGhst;
		val = sLst->vVal;
		bot = sLst->vLoc1;
		top = sLst->vLoc2;
		topInBlue = InBlueBand(top, lenTopBands, topBands);
		botInBlue = InBlueBand(bot, lenBotBands, botBands);
		sL = valList;
		while (sL != NULL) {
			if ((sL->pruned) && (doAligns || !doStems))
				goto NxtSL;
				
			sg1 = sL->vSeg1; sg2 = sL->vSeg2; /* sg1 is b, sg2 is t */
			v = sL->vVal;
			if (!ghst && sL->vGhst && !PruneVeryMuchGt(val, v))
				goto NxtSL; /* Do not bother checking if we should prune, if slSt is not ghost hint, sL is ghost hint,
							 and not (sL->vVal is  more than 50* bigger than sLst->vVal. 
							 Basically, we prefer non-ghost hints over ghost unless vVal is really low. */
			b = sL->vLoc1;
			t = sL->vLoc2;
			if (t==top && b==bot)
				goto NxtSL; /* Don't compare two valList elements that have the same top and bot. */
			
			if	( /* Prune sLst if the following are all true */
				 PruneGt(val, v) && /*  v is more than 3* val */
				 
				 (    (YgoesUp  && top+prndist >= t && bot-prndist <= b) ||
				  (!YgoesUp && top-prndist <= t && bot+prndist >= b)
				  ) &&				/* The sL hint is within the sLst hint */
				 
				 (   val < FixInt(100L) &&
				  PruneMuchGt(val, v)?
				  (CloseSegs(seg1,sg1,FALSE) || CloseSegs(seg2,sg2,FALSE)) :
				  (CloseSegs(seg1,sg1,FALSE) && CloseSegs(seg2,sg2,FALSE))
				  ) && /* val is less than 100, and the segments are close to each other.*/
				 
				 (   val < Fix16 ||
				  /* needs to be greater than FixOne << 3 for
				   HelveticaNeue 95 Black G has val == 2.125
				   Poetica/ItalicOne H has val == .66  */ 
				  (
				   (!topInBlue || top == t) &&
				   (!botInBlue || bot == b)
				   )
				  ) /* either val is small ( < Fixed 16) or, for both bot and top, the value is the same as SL, and not in a blue zone. */
				 
				 ) {
				sLst = PruneOne(sLst,TRUE,sL,5L);
				flg = FALSE;
				break; 
			}
			
			if (seg1 == NULL || seg2 == NULL)
				goto NxtSL; /* If the sLst is aghost hint, skip  */
			
			if (ac_abs(b-bot) < FixOne) {
				/* If the bottoms of the stems are within 1 unit */
				
				if (PruneGt(val, v) && /* If v is more than 3* val) */
					!topInBlue && 
					seg2->sType == sBEND &&
					CloseSegs(seg1,sg1,FALSE) /* and the tops are close */
					)
					{
					sLst = PruneOne(sLst,TRUE,sL,6L);
					flg = FALSE;
					break; 
					}
				
				if (!otherBot && PruneLt(val,v) && ac_abs(t-b) < ac_abs(top-bot)) {
					if (CloseSegs(seg1,sg1,FALSE))
						otherBot = TRUE;
				}
			}
			
			if (ac_abs(t-top) < FixOne) {
				/* If the tops of the stems are within 1 unit */
				if (PruneGt(val, v) && /* If v is more than 3* val) */
					!botInBlue &&
					seg2->sType == sBEND && 
					CloseSegs(seg1,sg1,FALSE)) /* and the tops are close */
					{
					sLst = PruneOne(sLst,TRUE,sL,7L);
					flg = FALSE;
					break;
					}
				
				if (!otherTop && PruneLt(val,v) && ac_abs(t-b) < ac_abs(top-bot)) {
					if (CloseSegs(seg2,sg2,FALSE))
						otherTop = TRUE;
				}
			}
			
			if (otherBot && otherTop) {
				/* if v less than  val by a factor of 3, and the sl stem width is less than the sLst stem width,
				 and the tops and bottoms are close */
				sLst = PruneOne(sLst,TRUE,sL,8L);
				flg = FALSE;
				break; }
		NxtSL:
			sL = sL->vNxt;
		}
		if (flg) {
			sPrv = sLst;
			sLst = sLst->vNxt;
		}
    }
	DoPrune();
}

private procedure FindBestVals(vL) register PClrVal vL; {
	register Fixed bV, bS;
	register Fixed t, b;
	register PClrVal vL2, vPrv, bstV;
	for (; vL != NULL; vL = vL->vNxt) {
		if (vL->vBst != NULL) continue; /* already assigned */
		bV = vL->vVal; bS = vL->vSpc; bstV = vL;
		b = vL->vLoc1; t = vL->vLoc2;
		vL2 = vL->vNxt; vPrv = vL;
		for (; vL2 != NULL; vL2 = vL2->vNxt) {
			if (vL2->vBst != NULL || vL2->vLoc1 != b || vL2->vLoc2 != t) continue;
			if ((vL2->vSpc == bS && vL2->vVal > bV) || (vL2->vSpc > bS)) {
				bS = vL2->vSpc; bV = vL2->vVal; bstV = vL2; }
			vL2->vBst = vPrv; vPrv = vL2;
		}
		while (vPrv != NULL) {
			vL2 = vPrv->vBst; vPrv->vBst = bstV; vPrv = vL2;
		}
    }
}

/* The following changes were made to fix a problem in Ryumin-Light and
 possibly other fonts as well.  The old version causes bogus coloring
 and extra newcolors. */
private procedure ReplaceVals(oldB,oldT,newB,newT,newBst,vert)
register Fixed oldB, oldT, newB, newT; boolean vert;
register PClrVal newBst; {
	register PClrVal vL;
	for (vL = valList; vL != NULL; vL = vL->vNxt) {
		if (vL->vLoc1 != oldB || vL->vLoc2 != oldT || vL->merge) continue;
		if (showClrInfo) {
			if (vert) ReportMergeVVal(oldB, oldT, newB, newT,
									  vL->vVal, vL->vSpc, newBst->vVal, newBst->vSpc);
			else ReportMergeHVal(oldB, oldT, newB, newT,
								 vL->vVal, vL->vSpc, newBst->vVal, newBst->vSpc); 
		}
		vL->vLoc1 = newB; vL->vLoc2 = newT;
		vL->vVal = newBst->vVal; vL->vSpc = newBst->vSpc;
		vL->vBst = newBst; vL->merge = TRUE;
    }
}

public procedure MergeVals(vert) boolean vert; {
	register PClrVal vLst, vL;
	PClrVal bstV, bV;
	PClrSeg seg1, seg2, sg1, sg2;
	Fixed bot, top, b, t;
	Fixed val, v, spc, s, abstmp;
	boolean ghst;
	FindBestVals(valList);
	for (vL = valList; vL != NULL; vL = vL->vNxt) vL->merge = FALSE;
	while (TRUE) {
		/* pick best from valList with merge field still set to false */
		vLst = valList; vL = NULL;
		while (vLst != NULL) {
			if (vLst->merge) {}
			else if (vL == NULL || CompareValues(vLst->vBst,vL->vBst,SFACTOR,0L))
				vL = vLst;
			vLst = vLst->vNxt;
		}
		if (vL == NULL) break;
		vL->merge = TRUE;
		ghst = vL->vGhst;
		b = vL->vLoc1;
		t = vL->vLoc2;
		sg1 = vL->vSeg1; /* left or bottom */
		sg2 = vL->vSeg2; /* right or top */
		vLst = valList;
		bV = vL->vBst;
		v = bV->vVal;
		s = bV->vSpc;
        while (vLst != NULL) { /* consider replacing vLst by vL */
            if (vLst->merge || ghst != vLst->vGhst) goto NxtVL;
            bot = vLst->vLoc1;
            top = vLst->vLoc2;
            if (bot == b && top == t)
                goto NxtVL;
            bstV = vLst->vBst;
            val = bstV->vVal;
            spc = bstV->vSpc;
            if ((top == t && CloseSegs(sg2,vLst->vSeg2,vert) &&
                 (vert || (
                           !InBlueBand(t,lenTopBands,topBands) &&
                           !InBlueBand(bot,lenBotBands,botBands) &&
                           !InBlueBand(b,lenBotBands,botBands)))) ||
                (bot == b && CloseSegs(sg1,vLst->vSeg1,vert) &&
                 (vert || (
                           !InBlueBand(b,lenBotBands,botBands) &&
                           !InBlueBand(t,lenTopBands,topBands) &&
                           !InBlueBand(top,lenTopBands,topBands)))) ||
                (ac_abs(top-t) <= maxMerge && ac_abs(bot-b) <= maxMerge &&
                 (vert || (t == top || !InBlueBand(top,lenTopBands,topBands))) &&
                 (vert || (b == bot || !InBlueBand(bot,lenBotBands,botBands))))) {
                    if (s==spc && val==v && !vert) {
                        if (InBlueBand(t,lenTopBands,topBands)) {
                            if ((YgoesUp && t > top) || (!YgoesUp && t < top))
                                goto replace; }
                        else if (InBlueBand(b,lenBotBands,botBands)) {
                            if ((YgoesUp && b < bot) || (!YgoesUp && b > bot))
                                goto replace; }
                    }
                    else
                        goto replace;
                }
            else if (s==spc && sg1 != NULL && sg2 != NULL) {
                seg1 = vLst->vSeg1; seg2 = vLst->vSeg2;
                if (seg1 != NULL && seg2 != NULL) {
                    if (ac_abs(bot-b) <= FixOne && ac_abs(top-t) <= maxBendMerge) {
                        if (seg2->sType == sBEND &&
                            (vert || !InBlueBand(top,lenTopBands,topBands)))
                            goto replace;
                    }
                    else if (ac_abs(top-t) <= FixOne && ac_abs(bot-b) <= maxBendMerge) {
                        if (v > val && seg1->sType == sBEND &&
                            (vert || !InBlueBand(bot,lenBotBands,botBands)))
                            goto replace;
                    }
                }
            }
            goto NxtVL;
        replace: ReplaceVals(bot,top,b,t,bV,vert);
        NxtVL: vLst = vLst->vNxt;
        }
		vL = vL->vNxt;
    }
}
