/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* eval.c */


#include "ac.h"

#define MAXF (1L << 15)
private procedure AdjustVal(pv,l1,l2,dist,d,hFlg)
Fixed *pv, l1, l2, dist, d; boolean hFlg; {
	real v, q, r1, r2, rd;
	Fixed abstmp;
    /* DEBUG 8 BIT. To get the saem result as the old auothint, had to change from FixedOne to FixedTwo. Since the returned weight is proportional to the square of l1 and l2,
     these need to be clamped to twice the old clamped value, else when the clamped values are used, the weight comes out as 1/4 of the original value. */
	if (dist < FixTwo)
		dist = FixTwo;
	if (l1 < FixTwo)
		l1 = FixTwo;
	if (l2 < FixTwo)
		l2 = FixTwo;
	if (ac_abs(l1) < MAXF)
		r1 = (real)(l1 * l1);
	else {
		r1 = (real)l1;
		r1 = r1*r1; 
	}
	if (ac_abs(l2) < MAXF)
		r2 = (real)(l2 * l2);
	else {
		r2 = (real)l2;
		r2 = r2*r2; 
	}
	if (ac_abs(dist) < MAXF)
		q = (real)(dist * dist);
	else {
		q = (real)dist;
		q = q*q; 
	}
	v = (real)((1000.0 * r1 * r2) / (q * q));
	if (d <= (hFlg? hBigDist : vBigDist))
		goto done;
	acfixtopflt(d, &rd);
	q = (hFlg? hBigDistR : vBigDistR) / rd; /* 0 < q < 1.0 */
	if (q <= 0.5) 
		{ 
			v = 0.0;
			goto done; 
		}
	q *= q;
	q *= q;
	q *= q; /* raise q to 8th power */
	v = v * q; /* if d is twice bigDist, value goes down by factor of 256 */
done:
	if (v > maxVal)
		v = maxVal;
	else if (v > 0.0 && v < minVal)
		v = minVal;
	*pv = acpflttofix(&v);
}

private Fixed CalcOverlapDist(d,overlaplen,minlen)
Fixed d, overlaplen, minlen; {
	real r = (real)d, ro = (real)overlaplen, rm = (real)minlen;
	r = r * ((real)(1.0 + 0.4 * (1.0 - ro / rm)));
	d = (Fixed)r;
	return d;
}

#define GapDist(d) (((d) < FixInt(127)) ? \
FTrunc(((d) * (d)) / 40) : ((long)  (((double)(d)) * (d) / (40*256))))
/* if d is >= 127.0 Fixed, then d*d will overflow the signed int 16 bit value. */
/* DEBUG 8 BIT. No idea why d*d was divided by 20, but we need to divide it by 2 more to get a dist that is only 2* the old autohint value. 
 With the 8.8 fixed coordinate system, we still overflow a long int with d*(d/40), so rather than casting this to a long int and then doing >>8, we need to divide by 256, then cast to long int. 
 I also fail to understand why the original used FTrunc, which right shifts by 256. For the current coordinate space, which has a fractional part of 8 bits, you do need to divide by 256 after doing a simple int multiply, but the previous coordinate space
    has a 7 bit Fixed fraction, and should be dividing by 128. I suspect that there was a yet earlier version which used a 8 bit fraction, and this is a bug.
 */

private procedure EvalHPair(botSeg,topSeg,pspc,pv)
PClrSeg botSeg, topSeg; Fixed *pspc, *pv; {
	Fixed brght, blft, bloc, tloc, trght, tlft, ldst, rdst;
	Fixed mndist, dist, dx, dy, minlen, overlaplen, abstmp;
	boolean inBotBand, inTopBand;
	int i;
	*pspc = 0;
	brght = botSeg->sMax;
	blft = botSeg->sMin;
	trght = topSeg->sMax;
	tlft = topSeg->sMin;
	bloc = botSeg->sLoc;
	tloc = topSeg->sLoc;
	dy = ac_abs(bloc-tloc);
	if (dy < minDist) { *pv = 0; return; }
	inBotBand = InBlueBand(bloc, lenBotBands, botBands);
	inTopBand = InBlueBand(tloc, lenTopBands, topBands);
	if (inBotBand && inTopBand) { /* delete these */
		*pv = 0; return; }
	if (inBotBand || inTopBand) /* up the priority of these */
		*pspc = FixInt(2);
	/* left is always < right */
	if ((tlft <= brght) && (trght >= blft)) { /* overlap */
		overlaplen = MIN(trght,brght) - MAX(tlft,blft);
		minlen = MIN(trght-tlft,brght-blft);
		if (minlen == overlaplen) dist = dy;
		else dist = CalcOverlapDist(dy, overlaplen, minlen);
    }
	else { /* no overlap; take closer ends */
		ldst = ac_abs(tlft-brght); rdst = ac_abs(trght-blft);
		dx = MIN(ldst, rdst);
		dist = GapDist(dx);
        dist += (7*dy)/5; /* extra penalty for nonoverlap
										* changed from 7/5 to 12/5 for Perpetua/Regular/
										* n, r ,m and other lowercase serifs;
										* undid change for Berthold/AkzidenzGrotesk 9/16/91;
										* this did not make Perpetua any worse. */
        DEBUG_ROUND(dist) /* DEBUG 8 BIT */
		if (dx > dy)
            dist *= dx / dy;
    }
	mndist = FixTwoMul(minDist);     dist = MAX(dist, mndist);
	if (NumHStems > 0) {
		Fixed w = idtfmy(dy);
		w = ac_abs(w);
		for (i=0; i < NumHStems; i++)
			if (w == HStems[i]) { *pspc += FixOne; break; }
    }
	AdjustVal(pv, brght-blft, trght-tlft, dist, dy, TRUE);
}

private procedure HStemMiss(botSeg,topSeg)
PClrSeg botSeg, topSeg; {
	Fixed brght, blft, bloc, tloc, trght, tlft, abstmp;
	Fixed mndist, dist, dy, minlen, overlaplen;
	Fixed b, t, diff, minDiff, minW, w, sw;
	int i;
	if (NumHStems == 0)
		return;
	brght = botSeg->sMax;
	blft = botSeg->sMin;
	trght = topSeg->sMax;
	tlft = topSeg->sMin;
	bloc = botSeg->sLoc;
	tloc = topSeg->sLoc;
	dy = ac_abs(bloc-tloc);
	if (dy < minDist) return;
	/* left is always < right */
	if ((tlft <= brght) && (trght >= blft)) { /* overlap */
		overlaplen = MIN(trght,brght) - MAX(tlft,blft);
		minlen = MIN(trght-tlft,brght-blft);
		if (minlen == overlaplen) dist = dy;
		else dist = CalcOverlapDist(dy, overlaplen, minlen);
    }
	else return;
	mndist = FixTwoMul(minDist);
	if (dist < mndist)
		return;
	minDiff = FixInt(1000); minW = 0;
	b = itfmy(bloc); 
	t = itfmy(tloc);
	w = t-b;
	/* don't check ghost bands for near misses */
	if ((( w = t-b ) == botGhst) || ( w == topGhst))
		return;
	w = ac_abs(w);
	for (i=0; i < NumHStems; i++)
		{
		sw = HStems[i];
		diff = ac_abs(sw - w);
		if (diff == 0)
			return;
		if (diff < minDiff)
			{ minDiff = diff; minW = sw; }
		}
	if (minDiff > FixInt(2))
		return;
	ReportStemNearMiss(FALSE, w, minW, b, t, 
					   (botSeg->sType == sCURVE) || (topSeg->sType == sCURVE));
}

private procedure EvalVPair(leftSeg,rightSeg,pspc,pv)
PClrSeg leftSeg, rightSeg; Fixed *pspc, *pv; {
	Fixed ltop, lbot, lloc, rloc, rtop, rbot, tdst, bdst;
	Fixed mndist, dx, dy, dist, overlaplen, minlen, abstmp;
	Fixed bonus, lbonus, rbonus;
	int i;
	*pspc = 0;
	ltop = leftSeg->sMax;
	lbot = leftSeg->sMin;
	rtop = rightSeg->sMax;
	rbot = rightSeg->sMin;
	lloc = leftSeg->sLoc;
	rloc = rightSeg->sLoc;
	dx = ac_abs(lloc-rloc);
	if (dx < minDist) { *pv = 0; return; }
	/* top is always > bot, independent of YgoesUp */
	if ((ltop >= rbot) && (lbot <= rtop)) { /* overlap */
		overlaplen = MIN(ltop,rtop) - MAX(lbot,rbot);
		minlen = MIN(ltop-lbot,rtop-rbot);
		if (minlen == overlaplen) dist = dx;
		else dist = CalcOverlapDist(dx, overlaplen, minlen);
    }
	else { /* no overlap; take closer ends */
		tdst = ac_abs(ltop-rbot);
        bdst = ac_abs(lbot-rtop);
		dy = MIN(tdst, bdst);
		dist = (7*dx)/5 + GapDist(dy); /* extra penalty for nonoverlap */
        DEBUG_ROUND(dist) /* DEBUG 8 BIT */
		if (dy > dx)
            dist *= dy / dx;
    }
	mndist = FixTwoMul(minDist);
	dist = MAX(dist, mndist);
	lbonus = leftSeg->sBonus;
	rbonus = rightSeg->sBonus;
	bonus = MIN(lbonus,rbonus);
	*pspc = (bonus > 0)? FixInt(2): 0; /* this is for sol-eol characters */
	if (NumVStems > 0) {
		Fixed w = idtfmx(dx);
		w = ac_abs(w);
		for (i=0; i < NumVStems; i++)
			if (w == VStems[i]) { *pspc = *pspc + FixOne; break; }
    }
	AdjustVal(pv, ltop-lbot, rtop-rbot, dist, dx, FALSE);
}

private procedure VStemMiss(leftSeg,rightSeg)
PClrSeg leftSeg, rightSeg; {
	Fixed ltop, lbot, lloc, rloc, rtop, rbot;
	Fixed mndist, dx, dist, overlaplen, minlen, abstmp;
	Fixed l, r, diff, minDiff, minW, w, sw;
	int i;
	if (NumVStems == 0)
		return;
	ltop = leftSeg->sMax;
	lbot = leftSeg->sMin;
	rtop = rightSeg->sMax;
	rbot = rightSeg->sMin;
	lloc = leftSeg->sLoc;
	rloc = rightSeg->sLoc;
	dx = ac_abs(lloc-rloc);
	if (dx < minDist) return;
	/* top is always > bot, independent of YgoesUp */
	if ((ltop >= rbot) && (lbot <= rtop)) { /* overlap */
		overlaplen = MIN(ltop,rtop) - MAX(lbot,rbot);
		minlen = MIN(ltop-lbot,rtop-rbot);
		if (minlen == overlaplen) dist = dx;
		else dist = CalcOverlapDist(dx, overlaplen, minlen);
    }
	else return;
	mndist = FixTwoMul(minDist);
	dist = MAX(dist, mndist);
	l = itfmx (lloc);
	r = itfmx (rloc);
	w = ac_abs(r-l);
	minDiff = FixInt(1000); minW = 0;
	for (i=0; i < NumVStems; i++)
		{
		sw = VStems[i];
		diff = ac_abs(sw - w);
		if (diff < minDiff)
			{ minDiff = diff; minW = sw; }
		if (minDiff == 0)
			return;
		}
	if (minDiff > FixInt(2))
		return;
	ReportStemNearMiss(TRUE, w, minW, l, r, 
					   (leftSeg->sType == sCURVE) || (rightSeg->sType == sCURVE));
}

private procedure InsertVValue(lft,rght,val,spc,lSeg,rSeg)
Fixed lft, rght, val, spc; PClrSeg lSeg, rSeg; {
	register PClrVal item, vlist, vprev;
	item = (PClrVal)Alloc(sizeof(ClrVal));
	item->vVal = val;
	item->initVal = val;
	item->vLoc1 = lft;
	item->vLoc2 = rght;
	item->vSpc = spc;
	item->vSeg1 = lSeg;
	item->vSeg2 = rSeg;
	item->vGhst = FALSE;
	vlist = valList; vprev = NULL;
	while (vlist != NULL) {
		if (vlist->vLoc1 >= lft) break;
		vprev = vlist; vlist = vlist->vNxt; }
	while (vlist != NULL && vlist->vLoc1 == lft) { 
		if (vlist->vLoc2 >= rght) break; 
		vprev = vlist; vlist = vlist->vNxt; }
	if (vprev == NULL) valList = item;
	else vprev->vNxt = item;
	item->vNxt = vlist;
	if (showClrInfo && showVs)
		ReportAddVVal(item);
}

#define LePruneValue(val) ((val) < FixOne && ((val)<<10) <= pruneValue)

private procedure AddVValue(lft,rght,val,spc,lSeg,rSeg)
Fixed lft, rght, val, spc; PClrSeg lSeg, rSeg; {
	if (val == 0) return;
	if (LePruneValue(val) && spc <= 0) return;
	if (lSeg != NULL && lSeg->sType == sBEND &&
		rSeg != NULL && rSeg->sType == sBEND) return;
	if (val <= pruneD && spc <= 0 && lSeg != NULL && rSeg != NULL) {
		if (lSeg->sType == sBEND || rSeg->sType == sBEND ||
			!CheckBBoxes(lSeg->sElt, rSeg->sElt))
			return;
    }
	InsertVValue(lft,rght,val,spc,lSeg,rSeg);
}

private procedure InsertHValue(bot,top,val,spc,bSeg,tSeg,ghst)
Fixed bot, top, val, spc; PClrSeg bSeg, tSeg; boolean ghst; {
	PClrVal item, vlist, vprev, vl;
	Fixed b;
	b = itfmy(bot);
	vlist = valList; vprev = NULL;
	while (vlist != NULL) {
		if (vlist->vLoc2 >= top) break;
		vprev = vlist; vlist = vlist->vNxt; }
	while (vlist != NULL && vlist->vLoc2 == top) { 
		if (vlist->vLoc1 >= bot) break; 
		vprev = vlist; vlist = vlist->vNxt; }
	/* prune ghost pair that is same as non ghost pair for same segment
     only if val for ghost is less than an existing val with same
     top and bottom segment (vl) */
	vl = vlist;
	while (ghst && vl != NULL && vl->vLoc2 == top && vl->vLoc1 == bot) {
		if (!vl->vGhst && (vl->vSeg1 == bSeg || vl->vSeg2 == tSeg) && vl->vVal > val)
			return;
		vl = vl->vNxt;
    }
	item = (PClrVal)Alloc(sizeof(ClrVal));
	item->vVal = val;
	item->initVal = val;
	item->vSpc = spc;
	item->vLoc1 = bot;
	item->vLoc2 = top;
	item->vSeg1 = bSeg;
	item->vSeg2 = tSeg;
	item->vGhst = ghst;
	if (vprev == NULL) valList = item;
	else vprev->vNxt = item;
	item->vNxt = vlist;
	if (showClrInfo && showHs)
		ReportAddHVal(item);
}

private procedure AddHValue(bot,top,val,spc,bSeg,tSeg)
Fixed bot, top, val, spc; PClrSeg bSeg, tSeg; {
	boolean ghst;
	if (val == 0) return;
	if (LePruneValue(val) && spc <= 0) return;
	if (bSeg->sType == sBEND && tSeg->sType == sBEND) return;
	ghst = bSeg->sType == sGHOST || tSeg->sType == sGHOST;
	if (!ghst && val <= pruneD && spc <= 0) {
		if (bSeg->sType == sBEND || tSeg->sType == sBEND ||
			!CheckBBoxes(bSeg->sElt, tSeg->sElt))
			return;
    }
	InsertHValue(bot,top,val,spc,bSeg,tSeg,ghst);
}

private real mfabs(real in)
{
	if (in>0) return in;
	return -in;
}

private Fixed CombVals(v1,v2) Fixed v1,v2; {
	register integer i;
	real r1, r2;
	register real x, a, xx;
	acfixtopflt(v1,&r1); 
	acfixtopflt(v2,&r2);
	/* home brew sqrt */
	a = r1 * r2; x = a;
	for (i = 0; i < 16; i++) {
		xx = ((real)0.5) * (x + a / x);
		if (i >= 8 && mfabs(xx-x) <= mfabs(xx) * 0.0000001) break;
		x = xx;
    }
	r1 += r2 + ((real)2.0) * xx;
	if (r1 > maxVal) r1 = maxVal;
	else if (r1 > 0 && r1 < minVal) r1 = minVal;
	return acpflttofix(&r1);
}

private procedure CombineValues() { /* works for both H and V */
	PClrVal vlist, v1;
	Fixed loc1, loc2;
	Fixed val;
	boolean match;
	vlist = valList;
	while (vlist != NULL) {
		v1 = vlist->vNxt;
		loc1 = vlist->vLoc1;
		loc2 = vlist->vLoc2;
		val = vlist->vVal;
		match = FALSE;
		while (v1 != NULL && v1->vLoc1 == loc1 && v1->vLoc2 == loc2) {
			if (v1->vGhst) val = v1->vVal;
			else val = CombVals(val, v1->vVal);
			/* increase value to compensate for length squared effect */
			match = TRUE; v1 = v1->vNxt; }
		if (match) {
			while (vlist != v1) {
				vlist->vVal = val; vlist = vlist->vNxt; }
		}
		else vlist = v1;
    }
}

public procedure EvalV() {
	PClrSeg lList, rList;
	Fixed lft, rght;
	Fixed val, spc;
	valList = NULL;
	lList = leftList;
	while (lList != NULL) {
		rList = rightList;
		while (rList != NULL) {
			lft = lList->sLoc;
			rght = rList->sLoc;
			if (lft < rght) {
				EvalVPair(lList,rList,&spc,&val);
				VStemMiss(lList,rList);
				AddVValue(lft,rght,val,spc,lList,rList);}
			rList = rList->sNxt; }
		lList = lList->sNxt; }
	CombineValues();
}

public procedure EvalH() {
	PClrSeg bList, tList, lst, ghostSeg;
	Fixed lstLoc, tempLoc, cntr;
	Fixed val, spc;
	valList = NULL;
	bList = botList;
	while (bList != NULL) {
		tList = topList;
		while (tList != NULL) {
			Fixed bot, top;
			bot = bList->sLoc;
			top = tList->sLoc;
			if ((bot < top && YgoesUp) || (bot > top && !YgoesUp)) {
				EvalHPair(bList,tList,&spc,&val);
				HStemMiss(bList,tList);
				AddHValue(bot,top,val,spc,bList,tList); }
			tList = tList->sNxt; }
		bList = bList->sNxt; }
	ghostSeg = (PClrSeg)Alloc(sizeof(ClrSeg));
	ghostSeg->sType = sGHOST;
	ghostSeg->sElt = NULL;
	if (lenBotBands < 2 && lenTopBands < 2) goto done;
	lst = botList;
	while (lst != NULL) {
		lstLoc = lst->sLoc;
		if (InBlueBand(lstLoc, lenBotBands, botBands)) {
			tempLoc = lstLoc;
			if (YgoesUp) tempLoc += ghostWidth;
			else tempLoc -= ghostWidth;
			ghostSeg->sLoc = tempLoc;
			cntr = (lst->sMax + lst->sMin)/2;
			ghostSeg->sMax = cntr + ghostLength/2;
			ghostSeg->sMin = cntr - ghostLength/2;
			DEBUG_ROUND(ghostSeg->sMax)/* DEBUG 8 BIT */
			DEBUG_ROUND(ghostSeg->sMin)/* DEBUG 8 BIT */
			spc = FixInt(2);
			val = FixInt(20);
			AddHValue(lstLoc,tempLoc,val,spc,lst,ghostSeg); }
		lst = lst->sNxt; }
	lst = topList;
	while (lst != NULL) {
		lstLoc = lst->sLoc;
		if (InBlueBand(lstLoc, lenTopBands, topBands)) {
			tempLoc = lstLoc;
			if (!YgoesUp) tempLoc += ghostWidth;
			else tempLoc -= ghostWidth;
			ghostSeg->sLoc = tempLoc;
			cntr = (lst->sMin+lst->sMax)/2;
			ghostSeg->sMax = cntr + ghostLength/2;
			ghostSeg->sMin = cntr - ghostLength/2;
			spc = FixInt(2);
			val = FixInt(20);
			AddHValue(tempLoc,lstLoc,val,spc,ghostSeg,lst); }
		lst = lst->sNxt; }
done:
    CombineValues();
}

