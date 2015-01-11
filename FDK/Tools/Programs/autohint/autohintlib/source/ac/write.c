/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* write.c */

#include "ac.h"
#include "cryptdefs.h"
#include "fipublic.h"
#include "machinedep.h"

#define WRTABS_COMMENT (0)

FILE *outputfile;
Fixed currentx, currenty;
boolean firstFlex, wrtColorInfo;
char S0[128];
PClrPoint bst;
PClrPoint prv_bst;
char bch;
Fixed bx, by;
boolean bstB;
short subpathcount;
extern char *bezoutput;
extern int bezoutputalloc;
extern int bezoutputactual;

#define ws(str) WriteString(str)

/* returns the number of characters written and possibly encrypted*/
static long WriteString(char *str) {
	if (bezoutput) {
		if ((bezoutputactual + (int)strlen(str)) >= bezoutputalloc) {
			int desiredsize = MAX(bezoutputalloc * 2, (bezoutputalloc + (int)strlen(str)));
#if DOMEMCHECK
			bezoutput = (char *)memck_realloc(bezoutput, desiredsize);
#else
			bezoutput = (char *)ACREALLOCMEM(bezoutput, desiredsize);
#endif
			if (bezoutput) {
				bezoutputalloc = desiredsize;
			}
			else {
				return (-1); /*FATAL ERROR*/
			}
		}
		strcat(bezoutput, str);
		bezoutputactual += (int)strlen(str);
		return (long)strlen(str);
	}
	else {
		return DoContEncrypt((char *)str, outputfile, FALSE, (unsigned long)INLEN);
	}
}

#define WRTNUM(i)                           \
	{                                       \
		sprintf(S0, "%ld ", (long int)(i)); \
		ws(S0);                             \
	}

static void wrtfx(Fixed f) {
	Fixed i = FRnd(f);
	WRTNUM(FTrunc(i))
}

static void wrtx(Fixed x) {
	Fixed i = FRnd(x);
	WRTNUM(FTrunc(i - currentx)) currentx = i;
}

static void wrty(Fixed y) {
	Fixed i = FRnd(y);
	WRTNUM(FTrunc(i - currenty)) currenty = i;
}

#define wrtcd(c) \
	wrtx(c.x);   \
	wrty(c.y)

/*To avoid pointless hint subs*/
#define HINTMAXSTR 2048
static char hintmaskstr[HINTMAXSTR];
static char prevhintmaskstr[HINTMAXSTR];

void safestrcat(char *s1, char *s2) {
	if (strlen(s1) + strlen(s2) + 1 > HINTMAXSTR) {
		sprintf(S0, "ERROR: Hint information overflowing buffer: %s\n", fileName);
		LogMsg(S0, LOGERROR, FATALERROR, TRUE);
	}
	else {
		strcat(s1, s2);
	}
}

#define sws(str) safestrcat(hintmaskstr, (char *)str)

#define SWRTNUM(i) {                    \
	sprintf(S0, "%ld ", (long int)(i)); \
	sws(S0);                            \
}

static void NewBest(PClrPoint lst) {
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

static void WriteOne(Fixed s) { /* write s to output file */
	Fixed r = UnScaleAbs(s);
	if (scalinghints) {
		r = FRnd(r);
	}
	if (FracPart(r) == 0) {
		SWRTNUM(FTrunc(r))
	}
	else {
		SWRTNUM(FTrunc(FRnd(r * 100L)))
		sws("100 div ");
	}
}

static void WritePointItem(PClrPoint lst) {
	switch (lst->c) {
		case 'b':
		case 'v':
			WriteOne(lst->y0);
			WriteOne(lst->y1 - lst->y0);
			sws(((lst->c == 'b') ? "rb" : "rv"));
			break;
		case 'y':
		case 'm':
			WriteOne(lst->x0);
			WriteOne(lst->x1 - lst->x0);
			sws(((lst->c == 'y') ? "ry" : "rm"));
			break;
		default: {
			sprintf(S0, "Illegal point list data for file: %s.\n",
					fileName);
			LogMsg(S0, LOGERROR, NONFATALERROR, TRUE);
		}
	}
	sws(" % ");
	SWRTNUM(lst->p0 != NULL ? lst->p0->count : 0);
	SWRTNUM(lst->p1 != NULL ? lst->p1->count : 0);
	sws("\n");
}

static void WrtPntLst(PClrPoint lst) {
	PClrPoint ptLst;
	char ch;
	Fixed x0, x1, y0, y1;
	ptLst = lst;

	while (lst != NULL) {/* mark all as not yet done */
		lst->done = FALSE;
		lst = lst->next;
	}
	while (TRUE) {/* write in sort order */
		lst = ptLst;
		bst = NULL;
		while (lst != NULL) {/* find first not yet done as init best */
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
		while (lst != NULL) {/* search for best */
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

static void wrtnewclrs(PPathElt e) {
	if (!wrtColorInfo) {
		return;
	}
	hintmaskstr[0] = '\0';
	WrtPntLst(ptLstArray[e->newcolors]);
	if (strcmp(prevhintmaskstr, hintmaskstr)) {
		ws("beginsubr snc\n");
		ws(hintmaskstr);
		ws("endsubr enc\nnewcolors\n");
		strcpy(prevhintmaskstr, hintmaskstr);
	}
}

boolean IsFlex(PPathElt e) {
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

static void mt(Cd c, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (FRnd(c.y) == currenty) {
		wrtx(c.x);
		ws("hmt\n");
	}
	else if (FRnd(c.x) == currentx) {
		wrty(c.y);
		ws("vmt\n");
	}
	else {
		wrtcd(c);
		ws("rmt\n");
	}
	if (e->eol) {
		ws("eol\n");
	}
	if (e->sol) {
		ws("sol\n");
	}
}

static void dt(Cd c, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (FRnd(c.y) == currenty) {
		wrtx(c.x);
		ws("hdt\n");
	}
	else if (FRnd(c.x) == currentx) {
		wrty(c.y);
		ws("vdt\n");
	}
	else {
		wrtcd(c);
		ws("rdt\n");
	}
	if (e->eol) {
		ws("eol\n");
	}
	if (e->sol) {
		ws("sol\n");
	}
}

Fixed flX, flY;
Cd fc1, fc2, fc3;

static void wrtflex(Cd c1, Cd c2, Cd c3, PPathElt e) {
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
	ws("preflx1\n");
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
	wrtcd(c);         \
	ws("rmt\npreflx2\n")
	wrtpreflx2(c13);
	wrtpreflx2(fc1);
	wrtpreflx2(fc2);
	wrtpreflx2(fc3);
	wrtpreflx2(c1);
	wrtpreflx2(c2);
	wrtpreflx2(c3);
	currentx = flX;
	currenty = flY;
	wrtcd(fc1);
	wrtcd(fc2);
	wrtcd(fc3);
	wrtcd(c1);
	wrtcd(c2);
	wrtcd(c3);
	WRTNUM(dmin) WRTNUM(delta) WRTNUM(yflag)
		WRTNUM(FTrunc(FRnd(currentx)));
	WRTNUM(FTrunc(FRnd(currenty)));
	ws("flx\n");
	firstFlex = TRUE;
}

static void ct(Cd c1, Cd c2, Cd c3, PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (e->isFlex && IsFlex(e)) {
		wrtflex(c1, c2, c3, e);
	}
	else if ((FRnd(c1.x) == currentx) && (c2.y == c3.y)) {
		wrty(c1.y);
		wrtcd(c2);
		wrtx(c3.x);
		ws("vhct\n");
	}
	else if ((FRnd(c1.y) == currenty) && (c2.x == c3.x)) {
		wrtx(c1.x);
		wrtcd(c2);
		wrty(c3.y);
		ws("hvct\n");
	}
	else {
		wrtcd(c1);
		wrtcd(c2);
		wrtcd(c3);
		ws("rct\n");
	}
	if (e->eol) {
		ws("eol\n");
	}
	if (e->sol) {
		ws("sol\n");
	}
}

static void cp(PPathElt e) {
	if (e->newcolors != 0) {
		wrtnewclrs(e);
	}
	if (idInFile) {
		WRTNUM(subpathcount++);
		ws("id\n");
	}
	ws("cp\n");
	if (e->eol) {
		ws("eol\n");
	}
	if (e->sol) {
		ws("sol\n");
	}
}

static void NumberPath() {
	register short int cnt;
	register PPathElt e;
	e = pathStart;
	cnt = 1;
	while (e != NULL) {
		e->count = cnt++;
		e = e->next;
	}
}

void SaveFile() {
	register PPathElt e = pathStart;
	Cd c1, c2, c3;
	char outfile[MAXPATHLEN], tempfilename[MAXPATHLEN];
/* AddSolEol(); */
#ifdef IS_LIB
	if (bezoutput) {
		outputfile = NULL;
	}
	else {
#endif
		sprintf(tempfilename, "%s%s", outPrefix, TEMPFILE);
		outputfile = ACOpenFile(tempfilename, "wb", OPENWARN);
		if (outputfile == NULL) {
			return;
		}
		subpathcount = 1;
#ifdef IS_LIB
	}
#endif
#ifdef ENCRYPTOUTPUT
	DoInitEncrypt(outputfile, OTHER, HEX, 64L, FALSE);
#endif
	sprintf(S0, "%% %s\n", fileName);
	ws(S0);
	wrtColorInfo = (pathStart != NULL && pathStart != pathEnd);
	NumberPath();
	prevhintmaskstr[0] = '\0';
	if (wrtColorInfo && (!e->newcolors)) {
		hintmaskstr[0] = '\0';
		WrtPntLst(ptLstArray[0]);
		ws(hintmaskstr);
		strcpy(prevhintmaskstr, hintmaskstr);
	}

	ws("sc\n");
	firstFlex = TRUE;
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
#if WRTABS_COMMENT
		ws(" % ");
		WRTNUM(e->count)
		switch (e->type) {
			case CURVETO:
				wrtfx(c1.x);
				wrtfx(c1.y);
				wrtfx(c2.x);
				wrtfx(c2.y);
				wrtfx(c3.x);
				wrtfx(c3.y);
				ws("ct");
				break;
			case LINETO:
				wrtfx(c1.x);
				wrtfx(c1.y);
				ws("dt");
				break;
			case MOVETO:
				wrtfx(c1.x);
				wrtfx(c1.y);
				ws("mt");
				break;
			case CLOSEPATH:
				ws("cp");
				break;
		}
		ws("\n");
#endif
		e = e->next;
	}
	ws("ed\n");
#ifdef ISLIB
	fclose(outputfile);
#endif

#ifndef IS_LIB
	if (featurefiledata) {
		sprintf(outfile, "%s", fileName);
	}
	else {
#endif
		sprintf(outfile, "%s%s", outPrefix, fileName);
#ifndef IS_LIB
	}
	RenameFile(tempfilename, outfile);
#endif
}
