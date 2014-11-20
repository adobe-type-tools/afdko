/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *  Horizontal metrics table.
 */

#include "hmtx.h"

/* ---------------------------- Table Definition --------------------------- */
typedef struct {
	uFWord advanceWidth;
	FWord lsb;
} LongHorMetrics;

typedef struct {
	dnaDCL(LongHorMetrics, hMetrics);
	dnaDCL(FWord, leftSideBearing);
} hmtxTbl;

/* --------------------------- Context Definition -------------------------- */
struct hmtxCtx_ {
	unsigned short nLongHorMetrics;
	unsigned short nLeftSideBearings;
	hmtxTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void hmtxNew(hotCtx g) {
	hmtxCtx h = MEM_NEW(g, sizeof(struct hmtxCtx_));

	dnaINIT(g->dnaCtx, h->tbl.hMetrics, 315, 14000);
	dnaINIT(g->dnaCtx, h->tbl.leftSideBearing, 315, 14000);

	/* Link contexts */
	h->g = g;
	g->ctx.hmtx = h;
}

int hmtxFill(hotCtx g) {
	hmtxCtx h = g->ctx.hmtx;
	int i;
	int j;
	FWord width;

	/* Fill table */
	dnaSET_CNT(h->tbl.hMetrics, g->font.glyphs.cnt);
	for (i = 0; i < h->tbl.hMetrics.cnt; i++) {
		LongHorMetrics *metric = &h->tbl.hMetrics.array[i];
		hotGlyphInfo *glyph = &g->font.glyphs.array[i];
		metric->advanceWidth = glyph->hAdv;
		metric->lsb = glyph->bbox.left;
	}

	/* Optimize metrics */
	width = h->tbl.hMetrics.array[h->tbl.hMetrics.cnt - 1].advanceWidth;
	for (i = h->tbl.hMetrics.cnt - 2; i >= 0; i--) {
		if (h->tbl.hMetrics.array[i].advanceWidth != width) {
			break;
		}
	}

	/* Construct left sidebearing array */
	dnaSET_CNT(h->tbl.leftSideBearing, h->tbl.hMetrics.cnt - i - 2);
	j = 0;
	for (i += 2; i < h->tbl.hMetrics.cnt; i++) {
		h->tbl.leftSideBearing.array[j++] = h->tbl.hMetrics.array[i].lsb;
	}
	h->tbl.hMetrics.cnt -= h->tbl.leftSideBearing.cnt;

	return 1;
}

void hmtxWrite(hotCtx g) {
	long i;
	hmtxCtx h = g->ctx.hmtx;

	/* Write horizontal metrics */
	for (i = 0; i < h->tbl.hMetrics.cnt; i++) {
		LongHorMetrics *metric = &h->tbl.hMetrics.array[i];
		OUT2(metric->advanceWidth);
		OUT2(metric->lsb);
	}

	/* Write left sidebreaings */
	for (i = 0; i < h->tbl.leftSideBearing.cnt; i++) {
		OUT2(h->tbl.leftSideBearing.array[i]);
	}
}

void hmtxReuse(hotCtx g) {
}

void hmtxFree(hotCtx g) {
	hmtxCtx h = g->ctx.hmtx;
	dnaFREE(h->tbl.hMetrics);
	dnaFREE(h->tbl.leftSideBearing);
	MEM_FREE(g, g->ctx.hmtx);
}

/* ------------------------ Supplementary Functions ------------------------ */

int hmtxGetNLongHorMetrics(hotCtx g) {
	hmtxCtx h = g->ctx.hmtx;
	return h->tbl.hMetrics.cnt;
}