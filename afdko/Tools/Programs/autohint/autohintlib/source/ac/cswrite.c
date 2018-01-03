/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* cswrite.c */

#include "ac.h"
#if THISISACMAIN && ALLOWCSOUTPUT /* entire file */

#include "machinedep.h"

#include "opcodes.h"
/*#include "type_crypt_lib.h"*/

#if __CENTERLINE__
#define STDOUT 1
#define DEBUG 0
#else
#define STDOUT 0
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

extern int get_char_width(char *cname);
extern Fixed GetPathLSB(void);

FILE *cstmpfile;
extern FILE *outputfile;
private Fixed currentx, currenty;
private boolean firstFlex, wrtColorInfo;
private char S0[128];
private PClrPoint bst;
private char bch;
private Fixed bx, by;
private boolean bstB;
private Fixed lsb;
private boolean needtoSubLSB;
private short trilockcount = 0;
#if DEBUG
#else
FILE *outIOFILE;
/*crypt_Token tok;*/
long val;
#endif

#if DEBUG
private ws(char *str) {
	fputc(' ', cstmpfile);
	fputs(str, cstmpfile);
}

#define WRTNUM(i) {          \
	sprintf(S0, "%ld", (i)); \
	ws(S0);                  \
}

#else
void test(void) {
	fprintf(stderr, "hi");
}

private void ws(char *str) {
	if (str[0] != ' ') {
		fputc(' ', cstmpfile);
	}
	fputs(str, cstmpfile);
	if ((str[0] <= '0' || str[0] >= '9') && str[0] != '-' && str[strlen(str) - 1] != '\n') {
		fputc('\n', cstmpfile);
	}
}

#define WRTNUM(i) {          \
	sprintf(S0, "%ld", (i)); \
	ws(S0);                  \
}
#endif

/*To avoid pointless hint subs*/
#define HINTMAXSTR 2048
static char hintmaskstr[HINTMAXSTR];
static char prevhintmaskstr[HINTMAXSTR];

private void safestrcat(char *s1, char *s2) {
	if (strlen(s1) + strlen(s2) + 1 > HINTMAXSTR) {
		sprintf(S0, "ERROR: Hint information overflowing buffer: %s\n", fileName);
		LogMsg(S0, LOGERROR, FATALERROR, TRUE);
	}
	else {
		strcat(s1, s2);
	}
}

private void sws(char *str) {
	safestrcat(hintmaskstr, " ");
	safestrcat(hintmaskstr, str);
	if ((str[0] <= '0' || str[0] >= '9') && str[0] != '-') {
		safestrcat(hintmaskstr, "\n");
	}
}

#define SWRTNUM(i) {         \
	sprintf(S0, "%ld", (i)); \
	sws(S0);                 \
}

private void wrtfx(Fixed f) {
	Fixed i = FRnd(f);
	WRTNUM(FTrunc(i));
}

private procedure wrtx(Fixed x, boolean subLSB) {
	Fixed i = FRnd(x);
	if (subLSB) {
		WRTNUM(FTrunc(i - currentx - lsb));
	}
	else {
		WRTNUM(FTrunc(i - currentx));
	}
	currentx = i;
}

private procedure wrty(Fixed y) {
	Fixed i = FRnd(y);
	WRTNUM(FTrunc(i - currenty));
	currenty = i;
}

#define wrtcd(c)  \
	wrtx(c.x, 0); \
	wrty(c.y)

private procedure NewBest(PClrPoint lst) {
	Fixed x0, x1, y0, y1;
	bst = lst;
	bch = lst->c;
	if (bch == 'y' || bch == 'm') {
		bstB = TRUE;
		x0 = lst->x0;
		x1 = lst->x1;
		bx = MIN(x0, x1);
	}
	else {
		bstB = FALSE;
		y0 = lst->y0;
		y1 = lst->y1;
		by = MIN(y0, y1);
	}
}

private procedure WriteOne(Fixed s) { /* write s to output file */
	Fixed r = UnScaleAbs(s);
	if (scalinghints) {
		r = FRnd(r);
	}
	if (FracPart(r) == 0) {
		SWRTNUM(FTrunc(r));
	}
	else {
		if (FracPart(r * 10L) == 0) {
			SWRTNUM(FTrunc(FRnd(r * 10L)));
			sws("10 div");
		}
		else {
			SWRTNUM(FTrunc(FRnd(r * 100L)));
			sws("100 div");
		}
	}
}

private procedure WritePointItem(PClrPoint lst) {
	switch (lst->c) {
		case 'b':
			WriteOne(lst->y0);
			WriteOne(lst->y1 - lst->y0);
			sws("hstem");
			trilockcount = 0;
			break;

		case 'v':
			WriteOne(lst->y0);
			WriteOne(lst->y1 - lst->y0);
			trilockcount++;
			if (trilockcount == 3) {
				sws("hstem3");
				trilockcount = 0;
			}
			break;

		case 'y':
			WriteOne(lst->x0 - lsb);
			WriteOne(lst->x1 - lst->x0);
			sws("vstem");
			trilockcount = 0;
			break;

		case 'm':
			WriteOne(lst->x0 - lsb);
			WriteOne(lst->x1 - lst->x0);
			trilockcount++;
			if (trilockcount == 3) {
				sws("vstem3");
				trilockcount = 0;
			}
			break;
		default:
			break;
	}
}

private procedure WrtPntLst(PClrPoint lst) {
	PClrPoint ptLst;
	char ch;
	Fixed x0, x1, y0, y1;

	ptLst = lst;
	while (lst != NULL) { /* mark all as not yet done */
		lst->done = FALSE;
		lst = lst->next;
	}
	while (TRUE) { /* write in sort order */
		lst = ptLst;
		bst = NULL;
		while (lst != NULL) { /* find first not yet done as init best */
			if (!lst->done) {
				NewBest(lst);
				break;
			}
			lst = lst->next;
		}
		if (bst == NULL) {
			break; /* finished with entire list */
		}
		lst = bst->next;
		while (lst != NULL) { /* search for best */
			if (!lst->done) {
				ch = lst->c;
				if (ch > bch) {
					NewBest(lst);
				}
				else if (ch == bch) {
					if (bstB) {
						x0 = lst->x0;
						x1 = lst->x1;
						if (MIN(x0, x1) < bx) {
							NewBest(lst);
						}
					}
					else {
						y0 = lst->y0;
						y1 = lst->y1;
						if (MIN(y0, y1) < by) {
							NewBest(lst);
						}
					}
				}
			}
			lst = lst->next;
		}
		bst->done = TRUE; /* mark as having been done */
		WritePointItem(bst);
	}
}

private procedure wrtnewclrs(PPathElt e) {
	if (!wrtColorInfo) {
		return;
	}
	hintmaskstr[0] = '\0';
	WrtPntLst(ptLstArray[e->newcolors]);
	if (strcmp(prevhintmaskstr, hintmaskstr)) {
		ws("4 callsubr");
		ws(hintmaskstr);
		strcpy(prevhintmaskstr, hintmaskstr);
	}
}

private boolean IsFlex(PPathElt e) {
	PPathElt e0, e1;
	if (firstFlex) {
		e0 = e;
		e1 = e->next;
	}
	else {
		e0 = e->prev;
		e1 = e;
	}
	return (e0 != NULL && e0->isFlex && e1 != NULL && e1->isFlex);
}

private procedure mt(Cd c, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (needtoSubLSB) {
		if (FRnd(c.y) == currenty) {
			wrtx(c.x, 1);
			ws("hmoveto");
			needtoSubLSB = FALSE;
		}
		else if ((FRnd(c.x) - currentx - FRnd(lsb)) == 0) {
			wrty(c.y);
			ws("vmoveto");
		}
		else {
			wrtx(c.x, 1);
			wrty(c.y);
			ws("rmoveto");
			needtoSubLSB = FALSE;
		}
	}
	else {
		if (FRnd(c.y) == currenty) {
			wrtx(c.x, 0);
			ws("hmoveto");
		}
		else if (FRnd(c.x) == currentx) {
			wrty(c.y);
			ws("vmoveto");
		}
		else {
			wrtcd(c);
			ws("rmoveto");
		}
	}
}

private Fixed currentFlexx, currentFlexy;
private boolean firstFlexMT;

private procedure wrtFlexx(Fixed x, boolean subLSB) {
	Fixed i = FRnd(x);
	if (subLSB) {
		WRTNUM(FTrunc(i - currentFlexx - lsb));
	}
	else {
		WRTNUM(FTrunc(i - currentFlexx));
	}

	currentFlexx = i;
}

private procedure wrtFlexy(Fixed y) {
	Fixed i = FRnd(y);
	WRTNUM(FTrunc(i - currentFlexy));
	currentFlexy = i;
}

private procedure wrtFlexmt(Cd c) {
	if (firstFlexMT) {
		if (FRnd(c.y) == currentFlexy) {
			wrtFlexx(c.x, 1);
			ws("hmoveto");
		}
		else if ((FRnd(c.x) - currentFlexx - FRnd(lsb)) == 0) {
			wrtFlexy(c.y);
			ws("vmoveto");
		}
		else {
			wrtFlexx(c.x, 1);
			wrtFlexy(c.y);
			ws("rmoveto");
		}
		firstFlexMT = FALSE;
	}
	else {
		if (FRnd(c.y) == currentFlexy) {
			wrtFlexx(c.x, 0);
			ws("hmoveto");
		}
		else if (FRnd(c.x) == currentFlexx) {
			wrtFlexy(c.y);
			ws("vmoveto");
		}
		else {
			wrtFlexx(c.x, 0);
			wrtFlexy(c.y);
			ws("rmoveto");
		}
	}
}

private procedure dt(Cd c, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (needtoSubLSB) {
		if (FRnd(c.y) == currenty) {
			wrtx(c.x, 1);
			ws("hlineto");
			needtoSubLSB = FALSE;
		}
		else if ((FRnd(c.x) - currentx - FRnd(lsb)) == 0) {
			wrty(c.y);
			ws("vlineto");
		}
		else if (FRnd(c.x) == currentx) {
			wrty(c.y);
			ws("vlineto");
			needtoSubLSB = FALSE;
		}
		else {
			wrtx(c.x, 1);
			wrty(c.y);
			ws("rlineto");
			needtoSubLSB = FALSE;
		}
	}
	else {
		if (FRnd(c.y) == currenty) {
			wrtx(c.x, 0);
			ws("hlineto");
		}
		else if (FRnd(c.x) == currentx) {
			wrty(c.y);
			ws("vlineto");
		}
		else {
			wrtx(c.x, 0);
			wrty(c.y);
			ws("rlineto");
		}
	}
}

private Fixed flX, flY;
private Cd fc1, fc2, fc3;

private procedure wrtflex(Cd c1, Cd c2, Cd c3, PPathElt e) {
	integer dmin, delta;
	boolean yflag;
	Cd c13;
	real shrink, r1, r2;
	if (firstFlex) {
		flX = currentx;
		flY = currenty;
		fc1 = c1;
		fc2 = c2;
		fc3 = c3;
		firstFlex = FALSE;
		return;
	}
	yflag = e->yFlex;
	dmin = DMIN;
	delta = DELTA;
	ws("1 callsubr");
	if (yflag) {
		if (fc3.y == c3.y) {
			c13.y = c3.y;
		}
		else {
			acfixtopflt(fc3.y - c3.y, &shrink);
			shrink = (real)delta / shrink;
			if (shrink < 0.0) {
				shrink = -shrink;
			}
			acfixtopflt(fc3.y - c3.y, &r1);
			r1 *= shrink;
			acfixtopflt(c3.y, &r2);
			r1 += r2;
			c13.y = acpflttofix(&r1);
		}
		c13.x = fc3.x;
	}
	else {
		if (fc3.x == c3.x) {
			c13.x = c3.x;
		}
		else {
			acfixtopflt(fc3.x - c3.x, &shrink);
			shrink = (real)delta / shrink;
			if (shrink < 0.0) {
				shrink = -shrink;
			}
			acfixtopflt(fc3.x - c3.x, &r1);
			r1 *= shrink;
			acfixtopflt(c3.x, &r2);
			r1 += r2;
			c13.x = acpflttofix(&r1);
		}
		c13.y = fc3.y;
	}

#define wrtpreflx2(c) \
	wrtFlexmt(c);     \
	ws("2 callsubr")

	currentFlexx = currentx;
	currentFlexy = currenty;

	wrtpreflx2(c13);
	wrtpreflx2(fc1);
	wrtpreflx2(fc2);
	wrtpreflx2(fc3);
	wrtpreflx2(c1);
	wrtpreflx2(c2);
	wrtpreflx2(c3);
	currentx = c3.x;
	currenty = c3.y;

	WRTNUM(dmin);
	WRTNUM(FTrunc(FRnd(currentx)));
	WRTNUM(FTrunc(FRnd(currenty)));
	ws("0 callsubr");
	firstFlex = TRUE;
}

private procedure ct(Cd c1, Cd c2, Cd c3, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (e->isFlex && IsFlex(e)) {
		wrtflex(c1, c2, c3, e);
	}
	else if (needtoSubLSB && ((FRnd(c1.x) - currentx - FRnd(lsb)) == 0) && (c2.y == c3.y)) {
		wrty(c1.y);
		wrtx(c2.x, 1);
		wrty(c2.y);
		wrtx(c3.x, 0);
		ws("vhcurveto");
		needtoSubLSB = FALSE;
	}
	else if ((FRnd(c1.x) == currentx) && (c2.y == c3.y)) {
		wrty(c1.y);
		wrtx(c2.x, 0);
		wrty(c2.y);
		wrtx(c3.x, 0);
		ws("vhcurveto");
		needtoSubLSB = FALSE;
	}
	else if ((FRnd(c1.y) == currenty) && (c2.x == c3.x)) {
		wrtx(c1.x, needtoSubLSB);
		wrtcd(c2);
		wrty(c3.y);
		ws("hvcurveto");
		needtoSubLSB = FALSE;
	}
	else {
		wrtx(c1.x, needtoSubLSB);
		wrty(c1.y);
		wrtcd(c2);
		wrtcd(c3);
		ws("rrcurveto");
		needtoSubLSB = FALSE;
	}
	if (firstFlex) {
		firstFlexMT = FALSE;
	}
}

private procedure cp(PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	ws("closepath");
}

void CSWrite(void) {
	register PPathElt e = pathStart;
	Cd c1, c2, c3;
	char outfile[MAXPATHLEN], tempfilename[MAXPATHLEN];
	int width;

	width = get_char_width(fileName);
	lsb = GetPathLSB();

	sprintf(tempfilename, "%s", TEMPFILE);
#if STDOUT
	cstmpfile = stdout;
#else
	cstmpfile = ACOpenFile(tempfilename, "w", OPENWARN);
#endif
	if (cstmpfile == NULL) {
		return;
	}

	wrtColorInfo = (pathStart != NULL && pathStart != pathEnd);
	fprintf(cstmpfile, "## -| {");

	wrtfx(lsb);
	WRTNUM((long int)width);
	ws("hsbw");

	prevhintmaskstr[0] = '\0';
	if (wrtColorInfo && !e->newcolors) {
		hintmaskstr[0] = '\0';
		WrtPntLst(ptLstArray[0]);
		ws(hintmaskstr);
		strcpy(prevhintmaskstr, hintmaskstr);
	}

	firstFlex = TRUE;
	needtoSubLSB = TRUE;
	firstFlexMT = TRUE;

	currentx = currenty = 0L;
	while (e != NULL) {
		switch (e->type) {
			case CURVETO:
				c1.x = UnScaleAbs(itfmx(e->x1));
				c1.y = UnScaleAbs(itfmy(e->y1));
				c2.x = UnScaleAbs(itfmx(e->x2));
				c2.y = UnScaleAbs(itfmy(e->y2));
				c3.x = UnScaleAbs(itfmx(e->x3));
				c3.y = UnScaleAbs(itfmy(e->y3));
				ct(c1, c2, c3, e);
				break;
			case LINETO:
				c1.x = UnScaleAbs(itfmx(e->x));
				c1.y = UnScaleAbs(itfmy(e->y));
				dt(c1, e);
				break;
			case MOVETO:
				c1.x = UnScaleAbs(itfmx(e->x));
				c1.y = UnScaleAbs(itfmy(e->y));
				mt(c1, e);
				break;
			case CLOSEPATH:
				cp(e);
				break;
			default: {
				sprintf(S0, "Illegal path list for file: %s.\n", fileName);
				LogMsg(S0, LOGERROR, NONFATALERROR, TRUE);
			}
		}

		e = e->next;
	}

	ws("endchar");
	fprintf(cstmpfile, " }  |- \n");
	fclose(cstmpfile);
	sprintf(outfile, "%s%s", outPrefix, fileName);
	RenameFile(tempfilename, outfile);
}

#endif /*THISISACMAIN && ALLOWCSOUTPUT*/
