/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* pick.c */


#include "ac.h"
#include "machinedep.h"

private PClrVal Vrejects, Hrejects;

public procedure InitPick(reason) integer reason; {
	switch (reason) {
		case STARTUP:
		case RESTART:
			Vrejects = Hrejects = NULL;
    }
}

#define LtPruneB(val) ((val) < FixOne && ((val)<<10) < pruneB)

private boolean ConsiderPicking(bestSpc, bestVal, colorList, prevBestVal)
Fixed bestSpc, bestVal, prevBestVal; PClrVal colorList; {
	if (bestSpc > 0L) return TRUE;
	if (colorList == NULL) return bestVal >= pruneD;
	if (bestVal > pruneA) return TRUE;
	if (LtPruneB(bestVal)) return FALSE;
	return (bestVal < FixedPosInf / pruneC) ?
	(prevBestVal <= bestVal * pruneC) : (prevBestVal / pruneC <= bestVal);
}

public procedure PickVVals(valList) PClrVal valList; {
	PClrVal colorList, rejectList, vlist, prev, best, bestPrev, nxt;
	Fixed bestVal, prevBestVal; Fixed lft, rght, vlft, vrght;
	colorList = rejectList = NULL; prevBestVal = 0;
	while (TRUE) {
		vlist = valList; prev = bestPrev = best = NULL;
		while (vlist != NULL) {
			if ((best == NULL || CompareValues(vlist,best,spcBonus,0L)) &&
				ConsiderPicking(vlist->vSpc, vlist->vVal, colorList, prevBestVal)) {
				best = vlist; bestPrev = prev; bestVal = vlist->vVal; }
			prev = vlist; vlist = vlist->vNxt; }
		if (best == NULL) break; /* no more */
		if (bestPrev == NULL) valList = best->vNxt;
		else bestPrev->vNxt = best->vNxt;
		/* have now removed best from valList */
		best->vNxt = colorList; /* add best to front of list */
		colorList = best; prevBestVal = bestVal;
		lft = best->vLoc1 - bandMargin;
		rght = best->vLoc2 + bandMargin;
		/* remove segments from valList that overlap lft..rght */
		vlist = valList; prev = NULL;
		while (vlist != NULL) {
			vlft = vlist->vLoc1; vrght = vlist->vLoc2;
			if ((vlft <= rght) && (vrght >= lft)) {
				nxt = vlist->vNxt;
				vlist->vNxt = rejectList; rejectList = vlist;
				vlist = nxt;
				if (prev == NULL) valList = vlist;
				else prev->vNxt = vlist; }
			else { prev = vlist; vlist = vlist->vNxt; }
		}
    }
	vlist = valList; /* move rest of valList to rejectList */
	while (vlist != NULL) {
		nxt = vlist->vNxt;
		vlist->vNxt = rejectList;
		rejectList = vlist;
		vlist = nxt; }
	if (colorList == NULL) ClrVBnds();
	Vcoloring = colorList;
	Vrejects = rejectList;
}

private boolean InSerifBand(y0,y1,n,p) register Fixed y0, y1, *p; integer n; {
	register integer i;
	if (n <= 0) return FALSE;
	y0 = itfmy(y0); y1 = itfmy(y1);
	if (y0 > y1) { Fixed tmp = y1; y1 = y0; y0 = tmp; }
	for (i=0; i < n; i += 2)
		if (p[i] <= y0 && p[i+1] >= y1) return TRUE;
	return FALSE; }

private boolean ConsiderValForSeg(val, seg, loc, nb, b, ns, s, primary)
PClrVal val; PClrSeg seg;
Fixed loc, *b, *s; integer nb, ns; boolean primary; {
	if (primary && val->vSpc > 0.0) return TRUE;
	if (InBlueBand(loc,nb,b)) return TRUE;
	if (val->vSpc <= 0.0 &&
		InSerifBand(seg->sMax, seg->sMin, ns, s)) return FALSE;
	if (LtPruneB(val->vVal)) return FALSE;
	return TRUE; }

private PClrVal FndBstVal(
						  seg, seg1Flg, cList, rList, nb, b, ns, s, locFlg, hFlg)
PClrSeg seg; PClrVal cList, rList;
boolean seg1Flg; integer nb, ns; Fixed *b, *s;
boolean locFlg, hFlg; {
	Fixed loc, vloc;
	PClrVal best, vList, initLst;
	PClrSeg vseg;
	best = NULL;
	loc = seg->sLoc;
	vList = cList;
	while (TRUE) {
		initLst = vList;
		while (vList != NULL) {
			if (seg1Flg) {vseg = vList->vSeg1; vloc = vList->vLoc1;}
			else {vseg = vList->vSeg2; vloc = vList->vLoc2;}
			if (abs(loc-vloc) <= maxMerge &&
				(locFlg ? !vList->vGhst :
				 (vseg == seg || CloseSegs(seg,vseg,!hFlg))) &&
				(best == NULL ||
				 (vList->vVal == best->vVal && vList->vSpc == best->vSpc &&
				  vList->initVal > best->initVal) ||
				 CompareValues(vList,best,spcBonus,3L)) &&
				/* last arg is "ghostshift" that penalizes ghost values */
				/* ghost values are set to 20 */
				/* so ghostshift of 3 means prefer nonghost if its
				 value is > (20 >> 3) */
				ConsiderValForSeg(vList,seg,loc,nb,b,ns,s,TRUE))
				best = vList;
			vList = vList->vNxt;
		}
		if (initLst == rList) break;
		vList = rList;
    }
	if (showClrInfo)
		ReportFndBstVal(seg,best,hFlg);
	return best;
}

#define FixSixteenth (0x10L)
private PClrVal FindBestValForSeg(
								  seg, seg1Flg, cList, rList, nb, b, ns, s, hFlg)
PClrSeg seg; PClrVal cList, rList;
boolean seg1Flg, hFlg; integer nb, ns; Fixed *b, *s; {
	PClrVal best, nonghst, ghst = NULL;
	best = FndBstVal(seg, seg1Flg, cList, rList, nb, b, ns, s, FALSE, hFlg);
	if (best != NULL && best->vGhst) {
		nonghst = FndBstVal(seg, seg1Flg, cList, rList, nb, b, ns, s, TRUE, hFlg);
		/* If nonghst hints are "better" use it instead of ghost band. */
		if (nonghst != NULL && nonghst->vVal >= FixInt(2)) {
			/* threshold must be greater than 1.004 for ITC Garamond Ultra "q" */
			ghst = best; best = nonghst; }
    }
	if (best != NULL) {
		if (best->vVal < FixSixteenth &&
			(ghst == NULL || ghst->vVal < FixSixteenth)) best = NULL;
		/* threshold must be > .035 for Monotype/Plantin/Bold Thorn
		 and < .08 for Bookman2/Italic asterisk */
		else best->pruned = FALSE;
    }
	return best;
}

private boolean MembValList(val, vList) register PClrVal val, vList; {
	while (vList != NULL) {
		if (val == vList) return TRUE;
		vList = vList->vNxt;
    }
	return FALSE;
}

private PClrVal PrevVal(val, vList) register PClrVal val, vList; {
	PClrVal prev;
	if (val == vList) return NULL;
	prev = vList; 
	while (TRUE) {
		vList = vList->vNxt;
		if (vList == NULL)
			{
			FlushLogFiles();
			sprintf(globmsg, "Malformed value list in %s.\n", fileName);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
			}
		
		if (vList == val) return prev;
		prev = vList; }
}

private procedure FindRealVal(vlist, top, bot, pseg1, pseg2)
PClrVal vlist; PClrSeg *pseg1, *pseg2; Fixed top, bot; {
	while (vlist != NULL) {
		if (vlist->vLoc2 == top && vlist->vLoc1 == bot && !vlist->vGhst) {
			*pseg1 = vlist->vSeg1;
			*pseg2 = vlist->vSeg2;
			return;
		}
		vlist = vlist->vNxt;
    }
}

public procedure PickHVals(valList) PClrVal valList; {
	PClrVal vlist, colorList, rejectList, bestPrev, prev, best, nxt;
	Fixed bestVal, prevBestVal;
	Fixed bot, top, vtop, vbot;
	PClrVal newBst;
	PClrSeg seg1, seg2;
	colorList = rejectList = NULL; prevBestVal = 0;
	while (TRUE) {
		vlist = valList;
		prev = bestPrev = best = NULL;
		while (vlist != NULL) {
			if ((best==NULL || CompareValues(vlist,best,spcBonus,0L)) &&
				ConsiderPicking(vlist->vSpc, vlist->vVal, colorList, prevBestVal)) {
				best = vlist; bestPrev = prev; bestVal = vlist->vVal; }
			prev = vlist; vlist = vlist->vNxt; }
		if (best != NULL) {
			seg1 = best->vSeg1; seg2 = best->vSeg2;
			if (best->vGhst) { /* find real segments at same loc as best */
				FindRealVal(valList, best->vLoc2, best->vLoc1, &seg1, &seg2);
			}
			if (seg1->sType == sGHOST) {
				/*newBst = FindBestValForSeg(seg2, FALSE, valList,
				 (PClrVal)NULL, 0L, (Fixed *)NIL, 0L, (Fixed *)NIL, TRUE);*/
				newBst = seg2->sLnk;
				if (newBst != NULL && newBst != best
					&& MembValList(newBst,valList)) {
					best = newBst; bestPrev = PrevVal(best, valList); }}
			else if (seg2->sType == sGHOST) {
				/*newBst = FindBestValForSeg(seg1, TRUE, valList,
				 (PClrVal)NULL, 0L, (Fixed *)NIL, 0L, (Fixed *)NIL, TRUE); */
				newBst = seg2->sLnk;
				if (newBst != NULL && newBst != best
					&& MembValList(newBst,valList)) {
					best = newBst; bestPrev = PrevVal(best, valList); }}
		}
		if (best == NULL) goto noMore;
		prevBestVal = bestVal;
		if (bestPrev == NULL) valList = best->vNxt;
		else bestPrev->vNxt = best->vNxt;
		/* have now removed best from valList */
		best->vNxt = colorList; colorList = best; /* add best to front of list */
		bot = best->vLoc1;
		top = best->vLoc2;
		/* The next if statement was added so that ghost bands are given
		 0 width for doing the conflict tests for bands too close together.
		 This was a problem in Minion/DisplayItalic onequarter and onehalf. */
		if (best->vGhst) { /* collapse width */
			if (best->vSeg1->sType == sGHOST) bot = top;
			else top = bot;
		}
		if (YgoesUp) { bot -= bandMargin; top += bandMargin; }
		else { bot += bandMargin; top -= bandMargin; }
		/* remove segments from valList that overlap bot..top */
		vlist = valList; prev = NULL;
		while (vlist != NULL) {
			vbot = vlist->vLoc1; vtop = vlist->vLoc2;
			/* The next if statement was added so that ghost bands are given
			 0 width for doing the conflict tests for bands too close together. */
			if (vlist->vGhst) { /* collapse width */
				if (vlist->vSeg1->sType == sGHOST) vbot = vtop;
				else vtop = vbot;
			}
			if ((YgoesUp && (vbot <= top) && (vtop >= bot)) ||
				((!YgoesUp && (vbot >= top) && (vtop <= bot)))) {
				nxt = vlist->vNxt;
				vlist->vNxt = rejectList; rejectList = vlist; vlist = nxt;
				if (prev == NULL) valList = vlist;
				else prev->vNxt = vlist; }
			else { prev = vlist; vlist = vlist->vNxt; }
		}
    }
noMore:
	vlist = valList; /* move rest of valList to rejectList */
	while (vlist != NULL) {
		nxt = vlist->vNxt;
		vlist->vNxt = rejectList;
		rejectList = vlist;
		vlist = nxt; }
	if (colorList == NULL) ClrHBnds();
	Hcoloring = colorList;
	Hrejects = rejectList;
}

private procedure FindBestValForSegs(sList,seg1Flg,cList,rList,nb,b,ns,s,hFlg)
PClrSeg sList; PClrVal cList, rList;
boolean seg1Flg, hFlg; integer nb, ns; Fixed *b, *s; {
	PClrVal best;
	while (sList != NULL) {
		best = FindBestValForSeg(sList, seg1Flg, cList, rList, nb, b, ns, s, hFlg);
		sList->sLnk = best;
		sList = sList->sNxt; }
}

private procedure SetPruned() {
	register PClrVal vL = valList;
	while (vL != NULL) {
		vL->pruned = TRUE;
		vL = vL->vNxt;
    }
}

public procedure FindBestHVals() {
	SetPruned();
	FindBestValForSegs(topList, FALSE, valList, NULL,
					   lenTopBands, topBands, 0L, (Fixed *)NULL, TRUE);
	FindBestValForSegs(botList, TRUE, valList, NULL,
					   lenBotBands, botBands, 0L, (Fixed *)NULL, TRUE);
	DoPrune();
}

public procedure FindBestVVals() {
	SetPruned();
	FindBestValForSegs(leftList, TRUE, valList, NULL,
					   0L, (Fixed *)NULL, numSerifs, serifs, FALSE);
	FindBestValForSegs(rightList, FALSE, valList, NULL,
					   0L, (Fixed *)NULL, numSerifs, serifs, FALSE);
	DoPrune();
}

