/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Horizontal header table.
 */

#include "hhea.h"
#include "hmtx.h"
#include "map.h"

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <limits.h>

/* ---------------------------- Table Definition --------------------------- */
typedef struct {
	Fixed version;
	FWord ascender;
	FWord descender;
	FWord lineGap;
	uFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	short caretSlopeRise;
	short caretSlopeRun;
	short caretOffset;
	short reserved[4];
	short metricDataFormat;
	unsigned short numberOfLongHorMetrics;
} hheaTbl;

/* --------------------------- Context Definition -------------------------- */
struct hheaCtx_ {
	hheaTbl tbl;            /* Table data */

	struct {                /* Used in caret offset calculation */
		double bboxLeft;    /* Bounds of italic glyph sheared upright */
		double bboxRight;
		double tangent;     /* Tangent of italic angle */
	}
	caretoff;

	hotCtx g;               /* Package context */
};


static short calcCaretOffset(hotCtx g);

#if HOT_DEBUG
static void printCaretOffset(hotCtx g, short caretoff, int iMaster);

#endif


/* --------------------------- Standard Functions -------------------------- */

void hheaNew(hotCtx g) {
	hheaCtx h = MEM_NEW(g, sizeof(struct hheaCtx_));

	h->tbl.caretOffset = SHRT_MAX;

	/* Link contexts */
	h->g = g;
	g->ctx.hhea = h;
}

int hheaFill(hotCtx g) {
	hheaCtx h = g->ctx.hhea;

	h->tbl.version                  = VERSION(1, 0);

	{
		if (OVERRIDE(g->font.hheaAscender)) {
			h->tbl.ascender = g->font.hheaAscender;
		}
		else {
			h->tbl.ascender = g->font.TypoAscender;
		}

		if (OVERRIDE(g->font.hheaDescender)) {
			h->tbl.descender = g->font.hheaDescender;
		}
		else {
			h->tbl.descender = g->font.TypoDescender;
		}

		if (OVERRIDE(g->font.hheaLineGap)) {
			h->tbl.lineGap = g->font.hheaLineGap;
		}
		else {
			h->tbl.lineGap = g->font.TypoLineGap;
		}
	}

	h->tbl.advanceWidthMax          = g->font.maxAdv.h;
	h->tbl.minLeftSideBearing       = g->font.minBearing.left;
	h->tbl.minRightSideBearing      = g->font.minBearing.right;
	h->tbl.xMaxExtent               = g->font.maxExtent.h;

	if (g->font.ItalicAngle == 0) {
		h->tbl.caretSlopeRise       = 1;
		h->tbl.caretSlopeRun        = 0;
	}
	else {
		h->tbl.caretSlopeRise       = 1000;
		h->tbl.caretSlopeRun        =
		    (short)RND(tan(FIX2DBL(-g->font.ItalicAngle) / RAD_DEG) * 1000);
	}
	if (h->tbl.caretOffset == SHRT_MAX) {
		h->tbl.caretOffset = (g->font.ItalicAngle == 0) ? 0 : calcCaretOffset(g);
	}

	h->tbl.reserved[0]              = 0;
	h->tbl.reserved[1]              = 0;
	h->tbl.reserved[2]              = 0;
	h->tbl.reserved[3]              = 0;
	h->tbl.metricDataFormat         = 0;
	h->tbl.numberOfLongHorMetrics   = hmtxGetNLongHorMetrics(g);

	return 1;
}

void hheaWrite(hotCtx g) {
	hheaCtx h = g->ctx.hhea;

	OUT4(h->tbl.version);
	OUT2(h->tbl.ascender);
	OUT2(h->tbl.descender);
	OUT2(h->tbl.lineGap);
	OUT2(h->tbl.advanceWidthMax);
	OUT2(h->tbl.minLeftSideBearing);
	OUT2(h->tbl.minRightSideBearing);
	OUT2(h->tbl.xMaxExtent);
	OUT2(h->tbl.caretSlopeRise);
	OUT2(h->tbl.caretSlopeRun);
	OUT2(h->tbl.caretOffset);
	OUT2(h->tbl.reserved[0]);
	OUT2(h->tbl.reserved[1]);
	OUT2(h->tbl.reserved[2]);
	OUT2(h->tbl.reserved[3]);
	OUT2(h->tbl.metricDataFormat);
	OUT2(h->tbl.numberOfLongHorMetrics);
}

void hheaReuse(hotCtx g) {
	hheaCtx h = g->ctx.hhea;
	h->tbl.caretOffset = SHRT_MAX;
}

void hheaFree(hotCtx g) {
	MEM_FREE(g, g->ctx.hhea);
}

/* -------------------------- Other functions ------------------------------ */

void hheaSetCaretOffset(hotCtx g, short caretOffset) {
	hheaCtx h = g->ctx.hhea;
	h->tbl.caretOffset = caretOffset;
}

/* Check point against current bounds */
static void checkPoint(hotCtx g, cffFixed x, cffFixed y) {
	hheaCtx h = g->ctx.hhea;
	double xSheared = FIX2DBL(x) - FIX2DBL(y) * h->caretoff.tangent;

	if (xSheared < h->caretoff.bboxLeft) {
		h->caretoff.bboxLeft = xSheared;
	}
	if (xSheared > h->caretoff.bboxRight) {
		h->caretoff.bboxRight = xSheared;
	}
}

/* [cffread path callback] Add moveto to path */
static void caretoffMoveto(void *ctx, cffFixed x1, cffFixed y1) {
	hotCtx g = ctx;
	checkPoint(g, x1, y1);
}

/* [cffread path callback] Add lineto to path */
static void caretoffLineto(void *ctx, cffFixed x1, cffFixed y1) {
	hotCtx g = ctx;
	checkPoint(g, x1, y1);
}

/* [cffread path callback] Add curveto to path */
static void caretoffCurveto(void *ctx, int flex, cffFixed x1, cffFixed y1,
                            cffFixed x2, cffFixed y2, cffFixed x3, cffFixed y3) {
	hotCtx g = ctx;
	checkPoint(g, x3, y3);
}

/* Calculate caret offset for an italic font. Shear heuristic glyph upright,
   calculate its left (a) and right (b) sidebearings. Return a-(a+b)/2 */
static short calcCaretOffset(hotCtx g) {
	hheaCtx h = g->ctx.hhea;
	unsigned int i;
	double a;
	double b;
	FWord hAdv;
	short caretoff;
	GID gid = GID_UNDEF;
	static cffPathCallbacks pathcb = {
		NULL,
		caretoffMoveto,
		caretoffLineto,
		caretoffCurveto,
		NULL,
		NULL,
		NULL,
		NULL,
	};
	UV uv[] = {
		UV_CARET_OFFSET_1,
		UV_CARET_OFFSET_2,
		UV_CARET_OFFSET_3,
	};

	/* Determine heuristic glyph */
	for (i = 0; i < ARRAY_LEN(uv); i++) {
		if ((gid = mapUV2GID(g, uv[i])) != GID_UNDEF) {
			break;
		}
	}
	if (gid == GID_UNDEF) {
		return 0;
	}

	h->caretoff.tangent = tan(FIX2DBL(-g->font.ItalicAngle) / RAD_DEG);
	h->caretoff.bboxLeft = DBL_MAX;
	h->caretoff.bboxRight = 0;

	if (IS_MM(g)) {
		cffSetUDV(g->ctx.cff, g->font.mm.axis.cnt, g->font.mm.UDV);
	}
	hAdv = cffGetGlyphInfo(g->ctx.cff, gid, &pathcb)->hAdv;

	if (h->caretoff.bboxLeft == DBL_MAX) {
		return 0;
	}

	a = h->caretoff.bboxLeft;
	b = hAdv - h->caretoff.bboxRight;
	caretoff = (short)RND(a - (a + b) / 2);

#if HOT_DEBUG
#if 0
	printCaretOffset(g, caretoff, -1);
	if (IS_MM(g)) {
		Fixed WV[TX_MAX_MASTERS];
		int i;
		for (i = 0; i < g->font.mm.nMasters; i++) {
			WV[i] = 0;
		}

		for (i = 0; i < g->font.mm.nMasters; i++) {
			WV[i] = INT2FIX(1);
			cffSetWV(g->ctx.cff, g->font.mm.nMasters, WV);
			printCaretOffset(g, caretoff, i);
			WV[i] = 0;
		}
	}
#endif
#endif

	return caretoff;
}

/* ------------------------- Caret offset dump ----------------------------- */

#if HOT_DEBUG
/* [cffread path callback] Add moveto to path */
static void dumpMoveto(void *ctx, cffFixed x1, cffFixed y1) {
	printf("%d %d _MT\n", FIX2INT(x1), FIX2INT(y1));
}

/* [cffread path callback] Add lineto to path */
static void dumpLineto(void *ctx, cffFixed x1, cffFixed y1) {
	printf("%d %d _LT\n", FIX2INT(x1), FIX2INT(y1));
}

/* [cffread path callback] Add curveto to path */
static void dumpCurveto(void *ctx, int flex, cffFixed x1, cffFixed y1,
                        cffFixed x2, cffFixed y2, cffFixed x3, cffFixed y3) {
	printf("%d %d %d %d %d %d _CT\n", FIX2INT(x1), FIX2INT(y1), FIX2INT(x2),
	       FIX2INT(y2), FIX2INT(x3), FIX2INT(y3));
}

/* [cffread path callback] Close path */
static void dumpClosepath(void *ctx) {
	printf("_CP\n");
}

/* [cffread path callback] End char */
static void dumpEndchar(void *ctx) {
	printf("0 setlinewidth fill\n");
}

static void printGlyphRun(hotCtx g, int *xLine, int lineMax, int caretoff,
                          int doOffset) {
	unsigned int i;
	static cffPathCallbacks pathcb = {
		NULL,
		dumpMoveto,
		dumpLineto,
		dumpCurveto,
		dumpClosepath,
		dumpEndchar,
		NULL,
		NULL,
	};
	char *sample[] = {
		"O", "H", "e", "k",
		"y", "o", "h", "f", "onesuperior",
		"l", "semicolon", "ampersand", "five", "c",
		"exclam", "P",
		"A", "bullet", "g"
	};

	if (g->font.Encoding == CFF_EXP_ENC) {
		sample[1] = "Osmall";
		sample[2] = "Hsmall";
		sample[3] = "Asmall";
		sample[4] = "Fsmall";
		sample[5] = "threeoldstyle";
		sample[6] = "twooldstyle";
		sample[7] = "onesuperior";
		sample[8] = "ampersandsmall";
		sample[9] = "comma";
		sample[10] = "figuredash";
		sample[11] = "onequarter";
		sample[11] = "ffi";
		sample[11] = "sixinferior";
		sample[11] = "sevenoldstyle";
		sample[11] = "nineoldstyle";
	}

	for (i = 0; i < ARRAY_LEN(sample); i++) {
		GID gid = mapName2GID(g, sample[i], NULL);
		if (gid != GID_UNDEF) {
			FWord hAdv;

			cffGetGlyphWidth(g->ctx.cff, gid, &hAdv, NULL);
			if (*xLine + hAdv < lineMax) {
				*xLine += hAdv;
				printf("_NP\n");
				cffGetGlyphInfo(g->ctx.cff, gid, &pathcb);
				if (doOffset) {
					printf("%d 0 translate\n"
					       "caret\n"
					       "%d 0 translate\n\n", hAdv + caretoff, -caretoff);
				}
				else {
					printf("%d 0 translate\n"
					       "caret\n", hAdv);
				}
			}
			else {
				return;
			}
		}
	}
}

/* Print proofs at the current UDV */
static void printCaretOffset(hotCtx g, short caretoff, int iMaster) {
#define IN2PIX(a) ((a) * 72)
#define MARGIN    IN2PIX(.25)
#define PTSIZE    (60)
#define YTOP      IN2PIX(11.4)
#define LINEGAP   IN2PIX(1.3)
#define SCALE     ((double)PTSIZE / 1000)
#define GAP (.15)

	hheaCtx h = g->ctx.hhea;
	int xLine;
	char msg[1024];
	static int caretPrint = 0;
	static double xCurr = MARGIN;
	static double yCurr = YTOP;

	if (caretPrint == 0) {
		printf("%%!\n"
		       "/_MT{moveto}bind def\n"
		       "/_NP{newpath}bind def\n"
		       "/_LT{lineto}bind def\n"
		       "/_CT{curveto}bind def\n"
		       "/_CP{closepath}bind def\n");
		printf("/Times-Roman findfont 9 scalefont setfont\n\n");
		caretPrint = 1;
	}

	printf("/caret {_NP "
	       "%hd %f mul %hd _MT "
	       "%hd %f mul %hd _LT\n"
	       " .1 setlinewidth stroke 0 0 _MT}bind def\n\n",
	       g->font.TypoAscender, h->caretoff.tangent, g->font.TypoAscender,
	       g->font.TypoDescender, h->caretoff.tangent, g->font.TypoDescender);

	/* Set up for a line */
	printf("gsave\n");
	yCurr -= LINEGAP;
	if (yCurr < MARGIN + IN2PIX(.3)) {
		printf("showpage\n");
		yCurr = YTOP - LINEGAP;
	}

	printf("%f %f translate\n", xCurr, yCurr);

	/* Draw baselines */
	printf("0 0 _MT %d 0 _LT ", IN2PIX(5));
	printf("%f 0 _MT %f 0 _LT\n", IN2PIX(5 + GAP), (double)IN2PIX(8));
	printf(".1 setlinewidth stroke\n");

	/* Prepare message */
	sprintf(msg, "%d %s", caretoff, g->font.FontName.array);
	if (IS_MM(g)) {
		if (iMaster == -1) {
			sprintf(&msg[strlen(msg)], "[dflt]");
		}
		else {
			sprintf(&msg[strlen(msg)], "[%d/%d]",
			        iMaster, g->font.mm.nMasters);
		}
	}

	printf("0 -20 _MT (%s) show\n", msg);
	fprintf(stderr, "%s\n", msg);

	printf("%.3f %.3f scale\n", SCALE, SCALE);

	/* Print glyphs with offset */
	xLine = 0;
	printGlyphRun(g, &xLine, (int)(IN2PIX(5) / SCALE), caretoff, 1);

	/* Print some glyphs without the offset, for contrast */
	printf("%f 0 translate\n", IN2PIX(5 + GAP) / SCALE - xLine);
	xLine = (int)(IN2PIX(5 + GAP) / SCALE);
	printGlyphRun(g, &xLine, (int)(IN2PIX(8) / SCALE), caretoff, 0);

	printf("grestore\n");
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, printCaretOffset);
}

#endif
