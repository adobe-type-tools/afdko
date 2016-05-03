/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/************************************************************************/
/* control.c */

#include "ac.h"
#include "machinedep.h"

#if ALLOWCSOUTPUT && THISISACMAIN
extern boolean charstringoutput;
#endif

extern void CSWrite(void);

private procedure DoHStems();
private procedure DoVStems();

private boolean CounterFailed;

public procedure InitAll(integer reason) {
	InitData(reason); /* must be first */
	InitAuto(reason);
	InitFix(reason);
	InitGen(reason);
	InitPick(reason);
}

private integer PtLstLen(PClrPoint lst) {
	integer cnt = 0;
	while (lst != NULL) {
		cnt++;
		lst = lst->next;
	}
	return cnt;
}

public integer PointListCheck(PClrPoint new, PClrPoint lst) {
	/* -1 means not a member, 1 means already a member, 0 means conflicts */
	Fixed l1, l2, n1, n2, tmp, halfMargin;
	char ch = new->c;
	halfMargin = FixHalfMul(bandMargin);
	halfMargin = FixHalfMul(halfMargin);
    /* DEBUG 8 BIT. In the previous version, with 7 bit fraction coordinates instead of the current
    8 bit, bandMargin is declared as 30, but scaled by half to match the 7 bit fraction coordinate -> a value of 15.
    In the current version this scaling doesn't happen. However, in this part of the code, the hint values are scaled up to 8 bits of fraction even in the original version, but topBand is applied without correcting for the scaling difference. In this version  I need to divide by half again in order to get to the same value. I think the original is a bug, but it has been working for 30 years, so I am not going to change the test now.
     */
	switch (ch) {
		case 'y':
		case 'm': {
			n1 = new->x0;
			n2 = new->x1;
			break;
		}
		case 'b':
		case 'v': {
			n1 = new->y0;
			n2 = new->y1;
			break;
		}
		default: {
			FlushLogFiles();
			sprintf(globmsg, "Illegal character in point list in %s.\n", fileName);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}
	}
	if (n1 > n2) {
		tmp = n1;
		n1 = n2;
		n2 = tmp;
	}
	while (TRUE) {
		if (lst == NULL) {
			return -1;
		}
		if (lst->c == ch) {/* same kind of color */
			switch (ch) {
				case 'y':
				case 'm': {
					l1 = lst->x0;
					l2 = lst->x1;
					break;
				}
				case 'b':
				case 'v': {
					l1 = lst->y0;
					l2 = lst->y1;
					break;
				}
			}
			if (l1 > l2) {
				tmp = l1;
				l1 = l2;
				l2 = tmp;
			}
			if (l1 == n1 && l2 == n2) {
				return 1L;
			}
			/* Add this extra margin to the band to fix a problem in
			 TimesEuropa/Italic/v,w,y where a main hstem hint was
			 being merged with newcolors. This main hstem caused
			 problems in rasterization so it shouldn't be included. */
			l1 -= halfMargin;
			l2 += halfMargin;
			if (l1 <= n2 && n1 <= l2) {
				return 0L;
			}
		}
		lst = lst->next;
	}
}

private boolean SameColorLists(PClrPoint lst1, PClrPoint lst2) {
	if (PtLstLen(lst1) != PtLstLen(lst2)) {
		return FALSE;
	}
	while (lst1 != NULL) {/* go through lst1 */
		if (PointListCheck(lst1, lst2) != 1) {
			return FALSE;
		}
		lst1 = lst1->next;
	}
	return TRUE;
}

public boolean SameColors(integer cn1, integer cn2) {
	if (cn1 == cn2) {
		return TRUE;
	}
	return SameColorLists(ptLstArray[cn1], ptLstArray[cn2]);
}

public procedure MergeFromMainColors(char ch) {
	register PClrPoint lst;
	for (lst = ptLstArray[0]; lst != NULL; lst = lst->next) {
		if (lst->c != ch) {
			continue;
		}
		if (PointListCheck(lst, pointList) == -1) {
			if (ch == 'b') {
				AddColorPoint(0L, lst->y0, 0L, lst->y1, ch, lst->p0, lst->p1);
			}
			else {
				AddColorPoint(lst->x0, 0L, lst->x1, 0L, ch, lst->p0, lst->p1);
			}
		}
	}
}

public procedure AddColorPoint(Fixed x0, Fixed y0, Fixed x1, Fixed y1, char ch, PPathElt p0, PPathElt p1) {
	register PClrPoint pt;
	integer chk;
	pt = (PClrPoint)Alloc(sizeof(ClrPoint));
	pt->x0 = x0;
	pt->y0 = y0;
	pt->x1 = x1;
	pt->y1 = y1;
	pt->c = ch;
	pt->done = FALSE;
	pt->next = NULL;
	pt->p0 = p0;
	pt->p1 = p1;
	chk = PointListCheck(pt, pointList);
	if (chk == 0 && showClrInfo) {
		ReportColorConflict(x0, y0, x1, y1, ch);
	}
	if (chk == -1) {
		pt->next = pointList;
		pointList = pt;
		if (logging) {
			LogColorInfo(pointList);
		}
	}
}

private procedure CopyClrFromLst(char clr, register PClrPoint lst) {
	boolean bvflg = (clr == 'b' || clr == 'v');
	while (lst != NULL) {
		if (lst->c == clr) {
			if (bvflg) {
				AddColorPoint(0L, lst->y0, 0L, lst->y1, clr, lst->p0, lst->p1);
			}
			else {
				AddColorPoint(lst->x0, 0L, lst->x1, 0L, clr, lst->p0, lst->p1);
			}
		}
		lst = lst->next;
	}
}

public procedure CopyMainV() {
	CopyClrFromLst('m', ptLstArray[0]);
}

public procedure CopyMainH() {
	CopyClrFromLst('v', ptLstArray[0]);
}

public procedure AddHPair(PClrVal v, char ch) {
	Fixed bot, top, tmp;
	PPathElt p0, p1, p;
	bot = itfmy(v->vLoc1);
	top = itfmy(v->vLoc2);
	p0 = v->vBst->vSeg1->sElt;
	p1 = v->vBst->vSeg2->sElt;
	if (top < bot) {
		tmp = top;
		top = bot;
		bot = tmp;
		p = p0;
		p0 = p1;
		p1 = p;
	}
	if (v->vGhst) {
		if (v->vSeg1->sType == sGHOST) {
			bot = top;
			p0 = p1;
			p1 = NULL;
			top = bot - FixInt(20); /* width == -20 iff bottom seg is ghost */
		}
		else {
			top = bot;
			p1 = p0;
			p0 = NULL;
			bot = top + FixInt(21); /* width == -21 iff top seg is ghost */
		}
	}
	AddColorPoint(0L, bot, 0L, top, ch, p0, p1);
}

public procedure AddVPair(PClrVal v, char ch) {
	Fixed lft, rght, tmp;
	PPathElt p0, p1, p;
	lft = itfmx(v->vLoc1);
	rght = itfmx(v->vLoc2);
	p0 = v->vBst->vSeg1->sElt;
	p1 = v->vBst->vSeg2->sElt;
	if (lft > rght) {
		tmp = lft;
		lft = rght;
		rght = tmp;
		p = p0;
		p0 = p1;
		p1 = p;
	}
	AddColorPoint(lft, 0L, rght, 0L, ch, p0, p1);
}

private boolean UseCounter(PClrVal sLst, boolean mclr) {
	integer cnt = 0;
	Fixed minLoc, midLoc, maxLoc, abstmp, prevBstVal, bestVal;
	Fixed minDelta, midDelta, maxDelta, loc, delta, th;
	PClrVal lst, newLst;
	minLoc = midLoc = maxLoc = FixInt(20000L);
	minDelta = midDelta = maxDelta = 0;
	lst = sLst;
	while (lst != NULL) {
		cnt++;
		lst = lst->vNxt;
	}
	if (cnt < 3) {
		return FALSE;
	}
	cnt -= 3;
	prevBstVal = 0;
	while (cnt > 0) {
		cnt--;
		if (cnt == 0) {
			prevBstVal = sLst->vVal;
		}
		sLst = sLst->vNxt;
	}
	bestVal = sLst->vVal;
	if (prevBstVal > FixInt(1000L) || bestVal < prevBstVal * 10L) {
		return FALSE;
	}
	newLst = sLst;
	while (sLst != NULL) {
		loc = sLst->vLoc1;
		delta = sLst->vLoc2 - loc;
		loc += FixHalfMul(delta);
		if (loc < minLoc) {
			maxLoc = midLoc;
			maxDelta = midDelta;
			midLoc = minLoc;
			midDelta = minDelta;
			minLoc = loc;
			minDelta = delta;
		}
		else if (loc < midLoc) {
			maxLoc = midLoc;
			maxDelta = midDelta;
			midLoc = loc;
			midDelta = delta;
		}
		else {
			maxLoc = loc;
			maxDelta = delta;
		}
		sLst = sLst->vNxt;
	}
	th = FixInt(5) / 100L;
	if (ac_abs(minDelta - maxDelta) < th &&
		ac_abs((maxLoc - midLoc) - (midLoc - minLoc)) < th) {
		if (mclr) {
			Vcoloring = newLst;
		}
		else {
			Hcoloring = newLst;
		}
		return TRUE;
	}
	if (ac_abs(minDelta - maxDelta) < FixInt(3) &&
		ac_abs((maxLoc - midLoc) - (midLoc - minLoc)) < FixInt(3)) {
		ReportError(mclr ? "Near miss for using V counter hinting." : "Near miss for using H counter hinting.");
	}
	return FALSE;
}

private procedure GetNewPtLst() {
	if (numPtLsts >= maxPtLsts) { /* increase size */
		PClrPoint *newArray;
		integer i;
		maxPtLsts += 5;
		newArray = (PClrPoint *)Alloc(maxPtLsts * sizeof(PClrPoint));
		for (i = 0; i < maxPtLsts - 5; i++) {
			newArray[i] = ptLstArray[i];
		}
		ptLstArray = newArray;
	}
	ptLstIndex = numPtLsts;
	numPtLsts++;
	pointList = NULL;
	ptLstArray[ptLstIndex] = NULL;
}

public procedure XtraClrs(PPathElt e) {
	/* this can be simplified for standalone coloring */
	ptLstArray[ptLstIndex] = pointList;
	if (e->newcolors == 0) {
		GetNewPtLst();
		e->newcolors = (short)ptLstIndex;
	}
	ptLstIndex = e->newcolors;
	pointList = ptLstArray[ptLstIndex];
}

private procedure Blues() {
	Fixed pv, pd, pc, pb, pa;
	PClrVal sLst;

	/* 
	 Top alignment zones are in the global 'topBands', bottom in 'botBands'.
	 
	 This function looks through the path, as defined by the linked list of 'elts', starting at the global 'pathStart', and adds to several lists. Coordinates are stored in the elt.(x,y) as (original value)/2.0, aka right shifted by 1 bit from the original 24.8 Fixed. I suspect that is to allow a larger integer portion - when this program was written, an int was 16 bits.
     
     
	 HStems, Vstems are global lists of Fixed 24.8 numbers..
	 
	 segLists is an array of 4  ClrSeg linked lists. list 0 and 1 are respectively up and down vertical segments. Lists 2 and 3 are
	 respectively left pointing and right pointing horizontal segments. On a counter-clockwise path, this is the same as selecting
	 top and bottom stem locations.
	 
	 NoBlueChar() consults a hard-coded list of glyph names, If the glyph is in this list, set the alignment zones (top and botBands) to empty.
	 
	 1) gen.c:GenHPts() . Buid the raw list of stem segments in global 'topList' and 'botList'.
	 
	 gen.c:GenHPts() steps through the liked list of path segments, starting at 'pathStart' It decides if a path is mostly H,
	 and if so, adds it to a linked list of hstem candidates in segLists, by calling gen.c:AddHSegment(). This calls ReportAddHSeg() (useful in debugging),
	 and then gen.c:AddSegment.
	 
	 If the path segment is in fact entirely vertical and is followed by a sharp bend,
	 gen.c:GenHPts adds two new path segments just 1 unit long,  after the segment end point,
	 called H/VBends (segment type sBend=1). I have no idea what these are for.
	 
	 AddSegment is pretty simple. It creates a new hint segment 'ClrSeg' for the parent path elt , fills it in,
	 adds it to appropriate  list of the 4 segLists, and then sorts by hstem location.
	 seg->sElt is the parent elt
	 seg->sType is the type
	 seg->sLoc is the location in Fixed 18.14: right shift 7 to get integer value.
	 
	 If the current path elt is a Closepath, It also calls LinkSegment() to add the current stem segment to the list of stem segments referenced by this elt.
	 e->Hs/Vs.
	 
	 Note that a hint segment is created for each nearly vertical or horioztonal path elt. Ths means that in an H, there will be two hint segments created for
	 the bottom and top of the H, as there are two horizontal paths with the same Y at the top and bottom of the H.
	 
	 Assign the top and bottom Hstem location lists.
	 topList = segLists[2]
	 botList = segLists[3];
	 
	 2) eval.c::EvalH().  Evaluate every combination of botList and topList, and assign a priority value and a 'Q' value.
	 
	 For each bottom stem
	 for each top stem
	 1) assign priority (spc) and weight (val) values with EvalHPair()
	 2) report stem near misses  in the 'HStems' list with HStemMiss()
	 3) decide whether to add pair to 'HStems' list with AddHValue()
	 
	 Add ghost hints.
	 For each bottom stem segment and then for each top stem segment:
	 if it is in an alignment zone, make a ghost hint segment and add it with AddHValue().
	 
	 EvalHPair() sets priority (spc) and weight (val) values.
		Omit pair by setting value to 0 if:
			bottom is in bottom alignment zone, and top is in top alignment zone. (otherwise, these will override the ghost hints).

		Boost priority by +2 if either the bot or top segment is in an alignment zone.
		
		dy = stem widt ( top - bot)
		
		Calculcate dist. Dist is set to a fudge factor *dy.
		if bottom segment xo->x1 overlaps top x0->x1, the fudge factor is 1.0. The
		less the overlap, the larger the fduge factor.
		if bottom segment xo->x1 overlaps top x0->x1:.
			if  top and bottom overlap exactly, dist = dy
			if they barely overlap, dist = 1.4*dy
			in between, interpolate.
		else, look at closest ends betwen bottom and top segments.
			dx = min X separation between top and bottom segments.
			dist = 1.4 *dy
			dist += dx*dx
			if dx > dy:
				dist *= dx / dy;
		Look through the HStems global list. For each match to dy, boost priority by + 1.
		Calculate weight with gen.c:AdjustVal()
			if dy is more than twice the 1.1.5* the largest hint in HStems, set weight to 0.
			Calculate weight as related to length of the segments squared divied by the distance squared.
			Basically, the greater the ratio segment overlap  to stem width, the higher the value.
			if dy is greater than the largest stem hint in HStems, decrease the value
				scale weight by  of *(largest stem hint in HStems)/ dy)**3.
	 
	 AddHValue() decides whether add a (bottom, top)  pair of color segments.
	 Do not add the pair if:
	 if weight (val) is 0,
	 if both are sBEND segments
	 if neither are a ghost hint, and weight <= pruneD and priority (spc) is <= 0:
	 if either is an sBEND: skip
	 if the BBox for one segment is the same or inside the BBox for the other: skip
	 
	 else add it with eval.c:InsertHValue()
	 add new ClrVal to global valList.
	 item->vVal = val; # weight
	 item->initVal = val; # originl weight from EvalHPair()
	 item->vSpc = spc; # priority
	 item->vLoc1 = bot; # bottom Y value in Fixed 18.14
	 item->vLoc2 = top; # top Y value in Fixed 18.14
	 item->vSeg1 = bSeg; # bottom color segment
	 item->vSeg2 = tSeg; # top color segment
	 item->vGhst = ghst; # if it is a ghost segment.
	 The new item is inserted after the first element where vlist->vLoc2 >= top and vlist->vLoc1 >= bottom
	 
	 3) merge.c:PruneHVals();
	 
	 item2 in the list knocks out item1 if:
	1) (item2 val is more than 3* greater than item1 val) and
		val 1 is less than FixedInt(100) and
		item2 top and bottom is within item 1 top and bottom
		and (  if val1 is more than 50* less than val2 and 
					either top seg1 is close to top seg 2, or bottom seg1 is close to bottom seg 2
			)
		and (val 1 < FixInt(16)) or ( ( item1 top not in blue zone, or top1 = top2) and 
									( item1 bottom not in blue zone, or top1 = bottom2))
	"Close to" for the bottom segment means you can get to the bottom elt for item 2 from bottom elt for 1 within the same path, by
	 stepping either forward or back from item 1's elt, and without going outside the bounds between
	 location 1 and location 2. Same for top segments.
	
		
	 
	 
	 4) pick.c:FindBestHVals();
	 When a hint segment    */

	if (showClrInfo) {
		PrintMessage("generate blues");
	}
	if (NoBlueChar()) {
		lenTopBands = lenBotBands = 0;
	}
	GenHPts();
	if (showClrInfo) {
		PrintMessage("evaluate");
	}
	if (!CounterFailed && HColorChar()) {
		pv = pruneValue;
		pruneValue = (Fixed)minVal;
		pa = pruneA;
		pruneA = (Fixed)minVal;
		pd = pruneD;
		pruneD = (Fixed)minVal;
		pc = pruneC;
		pruneC = (Fixed)maxVal;
		pb = pruneB;
		pruneB = (Fixed)minVal;
	}
	EvalH();
	PruneHVals();
	FindBestHVals();
    MergeVals(FALSE);

	if (showClrInfo) {
		ShowHVals(valList);
		PrintMessage("pick best");
	}
	MarkLinks(valList, TRUE);
	CheckVals(valList, FALSE);
	DoHStems(valList);  /* Report stems and alignment zones, if this has been requested. */
	PickHVals(valList); /* Moves best ClrVal items from valList to Hcoloring list. (? Choose from set of ClrVals for the samte stem values.) */
	if (!CounterFailed && HColorChar()) {
		pruneValue = pv;
		pruneD = pd;
		pruneC = pc;
		pruneB = pb;
		pruneA = pa;
		useH = UseCounter(Hcoloring, FALSE);
		if (!useH) { /* try to fix */
			AddBBoxHV(TRUE, TRUE);
			useH = UseCounter(Hcoloring, FALSE);
			if (!useH) { /* still bad news */
				ReportError("Note: Glyph is in list for using H counter hints, but didn't find any candidates.");
				CounterFailed = TRUE;
			}
		}
	}
	else {
		useH = FALSE;
	}
	if (Hcoloring == NULL) {
		AddBBoxHV(TRUE, FALSE);
	}
	if (showClrInfo) {
		PrintMessage("results");
		PrintMessage(useH ? "rv" : "rb");
		ShowHVals(Hcoloring);
	}
	if (useH) {
		PrintMessage("Using H counter hints.");
	}
	sLst = Hcoloring;
	while (sLst != NULL) {
		AddHPair(sLst, useH ? 'v' : 'b'); /* actually adds hint */
		sLst = sLst->vNxt;
	}
}

private procedure DoHStems(PClrVal sLst1) {
	Fixed bot, top;
	Fixed charTop = MINinteger, charBot = MAXinteger;
	boolean curved;
	if (!doAligns && !doStems) {
		return;
	}
	while (sLst1 != NULL) {
		bot = itfmy(sLst1->vLoc1);
		top = itfmy(sLst1->vLoc2);
		if (top < bot) {
			Fixed tmp = top;
			top = bot;
			bot = tmp;
		}
		if (top > charTop) {
			charTop = top;
		}
		if (bot < charBot) {
			charBot = bot;
		}
		/* skip if ghost or not a line on top or bottom */
		if (sLst1->vGhst) {
			sLst1 = sLst1->vNxt;
			continue;
		}
		curved = !FindLineSeg(sLst1->vLoc1, botList) &&
				 !FindLineSeg(sLst1->vLoc2, topList);
		AddHStem(top, bot, curved);
		sLst1 = sLst1->vNxt;
		if (top != MINinteger || bot != MAXinteger) {
			AddStemExtremes(UnScaleAbs(bot), UnScaleAbs(top));
		}
	}
	if (charTop != MINinteger || charBot != MAXinteger) {
		AddCharExtremes(UnScaleAbs(charBot), UnScaleAbs(charTop));
	}
}

private procedure Yellows() {
	Fixed pv, pd, pc, pb, pa;
	PClrVal sLst;
	if (showClrInfo) {
		PrintMessage("generate yellows");
	}
	GenVPts(SpecialCharType());
	if (showClrInfo) {
		PrintMessage("evaluate");
	}
	if (!CounterFailed && VColorChar()) {
		pv = pruneValue;
		pruneValue = (Fixed)minVal;
		pa = pruneA;
		pruneA = (Fixed)minVal;
		pd = pruneD;
		pruneD = (Fixed)minVal;
		pc = pruneC;
		pruneC = (Fixed)maxVal;
		pb = pruneB;
		pruneB = (Fixed)minVal;
	}
	EvalV();
	PruneVVals();
	FindBestVVals();
	MergeVals(TRUE);
	if (showClrInfo) {
		ShowVVals(valList);
		PrintMessage("pick best");
	}
	MarkLinks(valList, FALSE);
	CheckVals(valList, TRUE);
	DoVStems(valList);
	PickVVals(valList);
	if (!CounterFailed && VColorChar()) {
		pruneValue = pv;
		pruneD = pd;
		pruneC = pc;
		pruneB = pb;
		pruneA = pa;
		useV = UseCounter(Vcoloring, TRUE);
		if (!useV) { /* try to fix */
			AddBBoxHV(FALSE, TRUE);
			useV = UseCounter(Vcoloring, TRUE);
			if (!useV) { /* still bad news */
				ReportError("Note: Glyph is in list for using V counter hints, but didn't find any candidates.");
				CounterFailed = TRUE;
			}
		}
	}
	else {
		useV = FALSE;
	}
	if (Vcoloring == NULL) {
		AddBBoxHV(FALSE, FALSE);
	}
	if (showClrInfo) {
		PrintMessage("results");
		PrintMessage(useV ? "rm" : "ry");
		ShowVVals(Vcoloring);
	}
	if (useV) {
		PrintMessage("Using V counter hints.");
	}
	sLst = Vcoloring;
	while (sLst != NULL) {
		AddVPair(sLst, useV ? 'm' : 'y');
		sLst = sLst->vNxt;
	}
}

private procedure DoVStems(PClrVal sLst) {
	Fixed lft, rght;
	boolean curved;
	if (!doAligns && !doStems) {
		return;
	}
	while (sLst != NULL) {
		curved = !FindLineSeg(sLst->vLoc1, leftList) &&
				 !FindLineSeg(sLst->vLoc2, rightList);
		lft = itfmx(sLst->vLoc1);
		rght = itfmx(sLst->vLoc2);
		if (lft > rght) {
			Fixed tmp = lft;
			lft = rght;
			rght = tmp;
		}
		AddVStem(rght, lft, curved);
		sLst = sLst->vNxt;
	}
}

private procedure RemoveRedundantFirstColors() {
	register PPathElt e;
	if (numPtLsts < 2 || !SameColors(0L, 1L)) {
		return;
	}
	e = pathStart;
	while (e != NULL) {
		if (e->newcolors == 1) {
			e->newcolors = 0;
			return;
		}
		e = e->next;
	}
}

private procedure PreCheckForSolEol() {
	integer code;
	Fixed yStart, yEnd, x1, y1;
	if (!SpecialSolEol() || useV || useH || pathStart == NULL) {
		return;
	}
	code = SolEolCharCode();
	if (code == 0) {
		return;
	}
	GetEndPoint(pathStart, &x1, &y1);
	yStart = itfmy(y1);
	GetEndPoint(GetDest(pathEnd), &x1, &y1);
	yEnd = itfmy(y1);
	if (editChar && ((code == 1 && yStart > yEnd) || (code == -1 && yStart < yEnd))) {
		MoveSubpathToEnd(pathStart);
	}
}

private procedure AddColorsSetup() {
	int i;
	Fixed abstmp;
	vBigDist = 0;
	for (i = 0; i < NumVStems; i++) {
		if (VStems[i] > vBigDist) {
			vBigDist = VStems[i];
		}
	}
	vBigDist = dtfmx(vBigDist);
	if (vBigDist < initBigDist) {
		vBigDist = initBigDist;
	}
	vBigDist = (vBigDist * 23L) / 20L;
	acfixtopflt(vBigDist, &vBigDistR);
	hBigDist = 0;
	for (i = 0; i < NumHStems; i++) {
		if (HStems[i] > hBigDist) {
			hBigDist = HStems[i];
		}
	}
	hBigDist = ac_abs(dtfmy(hBigDist));
	if (hBigDist < initBigDist) {
		hBigDist = initBigDist;
	}
	hBigDist = (hBigDist * 23L) / 20L;
	acfixtopflt(hBigDist, &hBigDistR);
	if ((!scalinghints) && (roundToInt)) {
		RoundPathCoords();
	}
	CheckForMultiMoveTo();
	/* PreCheckForSolEol(); */
}

/* If extracolor is TRUE then it is ok to have multi-level
 coloring. */
private procedure AddColorsInnerLoop(boolean extracolor) {
	integer solEolCode = 2, retryColoring = 0;
	boolean isSolEol = FALSE;
	while (TRUE) {
		PreGenPts();
		CheckSmooth();
		InitShuffleSubpaths();
		Blues();
		if (!doAligns) {
			Yellows();
		}
		if (editChar) {
			DoShuffleSubpaths();
		}
		Hprimary = CopyClrs(Hcoloring);
		Vprimary = CopyClrs(Vcoloring);
		/*
		 isSolEol = SpecialSolEol() && !useV && !useH;
		 solEolCode = isSolEol? SolEolCharCode() : 2;
		 */
		PruneElementColorSegs();
		if (listClrInfo) {
			ListClrInfo();
		}
		if (extracolor) {
			AutoExtraColors(MoveToNewClrs(), isSolEol, solEolCode);
		}
		ptLstArray[ptLstIndex] = pointList;
		if (isSolEol) {
			break;
		}
		retryColoring++;
		/* we want to retry coloring if
		 `1) CounterFailed or
		 2) doFixes changed something, but in both cases, only on the first pass.
		 */
		if (CounterFailed && retryColoring == 1) {
			goto retry;
		}
		if (!DoFixes()) {
			break;
		}
		if (retryColoring > 1) {
			break;
		}
	retry:
		/* if we are doing the stem and zones reporting, we need to discard the reported. */
		if (reportRetryCB != NULL) {
			reportRetryCB();
		}
		if (pathStart == NULL || pathStart == pathEnd) {
			FlushLogFiles();
			sprintf(globmsg, "No character path in %s.\n", fileName);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}

		/* SaveFile(); SaveFile is always called in AddColorsCleanup, so this is a duplciate */
		InitAll(RESTART);
		if (writecoloredbez && !ReadCharFile(FALSE, FALSE, FALSE, TRUE)) {
			break;
		}
		AddColorsSetup();
		if (!PreCheckForColoring()) {
			break;
		}
		if (flexOK) {
			hasFlex = FALSE;
			AutoAddFlex();
		}
		reportErrors = FALSE;
	}
}

private procedure AddColorsCleanup() {
	RemoveRedundantFirstColors();
	reportErrors = TRUE;
	if (writecoloredbez) {

		if (pathStart == NULL || pathStart == pathEnd) {
			sprintf(globmsg, "The %s character path vanished while adding hints.\n  File not written.", fileName);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}
		else {
#if ALLOWCSOUTPUT && THISISACMAIN
			if (charstringoutput) {
				CSWrite();
			}
			else {
#endif
				SaveFile();
#if ALLOWCSOUTPUT && THISISACMAIN
			}
#endif
		}
	}
	InitAll(RESTART);
}

private procedure AddColors(boolean extracolor) {
	if (pathStart == NULL || pathStart == pathEnd) {
		PrintMessage("No character path, so no hints.");
#if ALLOWCSOUTPUT && THISISACMAIN
		if (charstringoutput) {
			CSWrite();
		}
		else {
#endif
			SaveFile(); /* make sure it gets saved with no coloring */
#if ALLOWCSOUTPUT && THISISACMAIN
		}
#endif
		return;
	}
	reportErrors = TRUE;
	CounterFailed = bandError = FALSE;
	CheckPathBBox();
	CheckForDups();
	AddColorsSetup();
	if (!PreCheckForColoring()) {
		return;
	}
	if (flexOK) {
		hasFlex = FALSE;
		AutoAddFlex();
	}
	AddColorsInnerLoop(extracolor);
	AddColorsCleanup();
	Test();
}

public boolean DoFile(char *fname, boolean extracolor) {
	integer lentop = lenTopBands, lenbot = lenBotBands;
	fileName = fname;
	if (!ReadCharFile(TRUE, FALSE, FALSE, TRUE)) {
		sprintf(globmsg, "Cannot open %s file.\n", fileName);
		LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
	}
	PrintMessage(""); /* Just print the file name. */
	LogYMinMax();
	AddColors(extracolor);
	lenTopBands = lentop;
	lenBotBands = lenbot;
	return TRUE;
}
