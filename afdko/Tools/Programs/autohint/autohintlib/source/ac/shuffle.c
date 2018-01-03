/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* shuffle.c */

#include "ac.h"
#define MAXCNT (100)

private unsigned char *links;
private integer rowcnt;

public procedure InitShuffleSubpaths() {
	register integer cnt = -1;
	register PPathElt e = pathStart;
	while (e != NULL) { /* every element is marked with its subpath count */
		if (e->type == MOVETO) cnt++;
		if (DEBUG)
			{
			if (e->type == MOVETO) { /* DEBUG */
				sprintf(globmsg, "subpath %ld starts at %g %g\n", cnt,
						FixToDbl(itfmx(e->x)), FixToDbl(itfmy(e->y)));
				PrintMessage(globmsg);
			}
			}
		e->count = (short)cnt;
		e = e->next;
    }
	cnt++;
	rowcnt = cnt;
	links = (cnt < 4 || cnt >= MAXCNT) ? NULL :
    (unsigned char *)Alloc(cnt * cnt);
}

private procedure PrintLinks() {
	integer i, j;
	PrintMessage("Links ");
	for (i = 0; i < rowcnt; i++) {
		sprintf(globmsg, "%ld  ", i);
		PrintMessage(globmsg);
		if (i < 10) PrintMessage(" ");
    }
	PrintMessage("\n");
	for (i = 0; i < rowcnt; i++) {
		sprintf(globmsg, " %ld   ", i);
		PrintMessage(globmsg);
		if (i < 10) PrintMessage(" ");
		for (j = 0; j < rowcnt; j++)
			{
			sprintf(globmsg, "%d   ", links[rowcnt * i + j]);
			PrintMessage(globmsg);
			}
		PrintMessage("\n");
    }
}

private procedure PrintSumLinks(sumlinks) char *sumlinks; {
	integer i;
	PrintMessage("Sumlinks ");
	for (i = 0; i < rowcnt; i++) {
		sprintf(globmsg, "%ld  ", i);
		PrintMessage(globmsg);
		if (i < 10) PrintMessage(" ");
    }
	PrintMessage("\n");
	PrintMessage("         ");
	for (i = 0; i < rowcnt; i++)
		{
		sprintf(globmsg, "%d   ", sumlinks[i]);
		PrintMessage(globmsg);
		}
	PrintMessage("\n");
}

private procedure PrintOutLinks(unsigned char *outlinks) {
	integer i;
	PrintMessage("Outlinks ");
	for (i = 0; i < rowcnt; i++) {
		sprintf(globmsg, "%ld  ", i);
		PrintMessage(globmsg);
		if (i < 10) PrintMessage(" ");
    }
	PrintMessage("\n");
	PrintMessage("         ");
	for (i = 0; i < rowcnt; i++)
		{
		sprintf(globmsg, "%d   ", outlinks[i]);
		PrintMessage(globmsg);
		}
	PrintMessage("\n");
}

public procedure MarkLinks(vL,hFlg) PClrVal vL; boolean hFlg; {
	register integer i, j;
	register PClrSeg seg;
	register PPathElt e;
	if (links == NULL) return;
	for (; vL != NULL; vL = vL->vNxt) {
		if (vL == NULL) continue;
		seg = vL->vSeg1;
		if (seg == NULL) continue;
		e = seg->sElt;
		if (e == NULL) continue;
		i = e->count;
		seg = vL->vSeg2;
		if (seg == NULL) continue;
		e = seg->sElt;
		if (e == NULL) continue;
		j = e->count;
		if (i == j) continue;
		if (DEBUG)
			{
			if (hFlg) ShowHVal(vL); else ShowVVal(vL);
			sprintf(globmsg, " : %ld <-> %ld\n", i, j);
			PrintMessage(globmsg);
			}
		links[rowcnt * i + j] = 1;
		links[rowcnt * j + i] = 1;
    }
}

private procedure Outpath(links, outlinks, output, bst)
unsigned char *links, *outlinks, *output; integer bst; {
	register unsigned char *lnks, *outlnks;
	register integer i = bst;
	register PPathElt e = pathStart;
	while (e != NULL) {
		if (e->count == i) break;
		e = e->next;
    }
	MoveSubpathToEnd(e);
	if (DEBUG)
		{
		sprintf(globmsg, "move subpath %ld to end\n", bst); /* DEBUG */
		PrintMessage(globmsg);
		}  output[bst] = 1;
	lnks = &links[bst * rowcnt];
	outlnks = outlinks;
	for (i = 0; i < rowcnt; i++)
		*outlnks++ += *lnks++;
	if (DEBUG)
		PrintOutLinks(outlinks);
}

/* The intent of this code is to order the subpaths so that
 the hints will not need to change constantly because it
 is jumping from one subpath to another.  Kanji characters
 had the most problems with this which caused huge files
 to be created. */
public procedure DoShuffleSubpaths() {
	unsigned char sumlinks[MAXCNT], output[MAXCNT], outlinks[MAXCNT];
	register unsigned char *lnks;
	register integer i, j, bst, bstsum, bstlnks;
	if (links == NULL) return;
	if (DEBUG)
		PrintLinks();
	for (i = 0; i < rowcnt; i++)
		output[i] = sumlinks[i] = outlinks[i] = 0;
	lnks = links;
	for (i = 0; i < rowcnt; i++) {
		for (j = 0; j < rowcnt; j++) {
			if (*lnks++ != 0) sumlinks[i]++;
		}
    }
	if (DEBUG)
		PrintSumLinks((char*)sumlinks);
	while (TRUE) {
		bst = -1; bstsum = 0;
		for (i = 0; i < rowcnt; i++) {
			if (output[i] == 0 && (bst == -1 || sumlinks[i] > bstsum)) {
				bstsum = sumlinks[i]; bst = i; }
		}
		if (bst == -1) break;
		Outpath(links, outlinks, output, bst);
		while (TRUE) {
			bst = -1; bstsum = 0; bstlnks = 0;
			for (i = 0; i < rowcnt; i++) {
				if (output[i] == 0 && outlinks[i] >= bstlnks) {
					if (outlinks[i] > 0 &&
						(bst == -1 || outlinks[i] > bstlnks ||
						 (outlinks[i] == bstlnks && sumlinks[i] > bstsum))) {
							bstlnks = outlinks[i]; bst = i; bstsum = sumlinks[i]; }
				}
			}
			if (bst == -1) break;
			Outpath(links, outlinks, output, bst);
		}
    }
}
