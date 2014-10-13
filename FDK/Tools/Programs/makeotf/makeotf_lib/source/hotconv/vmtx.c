/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Vertical metrics table.
 */

#include <stdlib.h>
#include <stdio.h>
#include "vmtx.h"
#include "dynarr.h"
/* ---------------------------- Table Definition --------------------------- */
typedef struct {
	uFWord advanceHeight;
	FWord tsb;
} LongVerMetrics;

typedef struct {
	dnaDCL(LongVerMetrics, vMetrics);
	dnaDCL(FWord, topSideBearing);
} vmtxTbl;

/* --------------------------- Context Definition -------------------------- */

typedef struct {
	GID gid;
	uFWord advanceHeight;
} PropInfo;

struct vmtxCtx_ {
	vmtxTbl tbl;                    /* Table data */
	hotCtx g;                       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void vmtxNew(hotCtx g) {
	vmtxCtx h = MEM_NEW(g, sizeof(struct vmtxCtx_));

	dnaINIT(g->dnaCtx, h->tbl.vMetrics, 14000, 14000);
	dnaINIT(g->dnaCtx, h->tbl.topSideBearing, 14000, 14000);

	/* Link contexts */
	h->g = g;
	g->ctx.vmtx = h;
}

int vmtxFill(hotCtx g) {
	vmtxCtx h = g->ctx.vmtx;
	long i;
	int j;
	FWord height;

	if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE))  && (!IS_CID(g))) {
		return 0;
	}

	/* Fill table */
	dnaSET_CNT(h->tbl.vMetrics, g->font.glyphs.cnt);
	for (i = 0; i < h->tbl.vMetrics.cnt; i++) {
		LongVerMetrics *metric = &h->tbl.vMetrics.array[i];
		hotGlyphInfo *glyph = &g->font.glyphs.array[i];

		metric->advanceHeight = -glyph->vAdv;
		metric->tsb = glyph->vOrigY - glyph->bbox.top;
	}

	/* Optimize metrics */
	height = h->tbl.vMetrics.array[h->tbl.vMetrics.cnt - 1].advanceHeight;
	for (i = h->tbl.vMetrics.cnt - 2; i >= 0; i--) {
		if (h->tbl.vMetrics.array[i].advanceHeight != height) {
			break;
		}
	}

	/* Construct top sidebearing array */
	dnaSET_CNT(h->tbl.topSideBearing, h->tbl.vMetrics.cnt - i - 2);
	j = 0;
	for (i += 2; i < h->tbl.vMetrics.cnt; i++) {
		h->tbl.topSideBearing.array[j++] = h->tbl.vMetrics.array[i].tsb;
	}
	h->tbl.vMetrics.cnt -= h->tbl.topSideBearing.cnt;

	return 1;
}

void vmtxWrite(hotCtx g) {
	long i;
	vmtxCtx h = g->ctx.vmtx;

	/* Write vertical metrics */
	for (i = 0; i < h->tbl.vMetrics.cnt; i++) {
		LongVerMetrics *metric = &h->tbl.vMetrics.array[i];
		OUT2(metric->advanceHeight);
		OUT2(metric->tsb);
	}

	/* Write top sidebearings */
	for (i = 0; i < h->tbl.topSideBearing.cnt; i++) {
		OUT2(h->tbl.topSideBearing.array[i]);
	}
}

void vmtxReuse(hotCtx g) {
}

void vmtxFree(hotCtx g) {
	vmtxCtx h = g->ctx.vmtx;
	dnaFREE(h->tbl.vMetrics);
	dnaFREE(h->tbl.topSideBearing);
	MEM_FREE(g, g->ctx.vmtx);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Must be called after vmtxFill() */
int vmtxGetNLongVerMetrics(hotCtx g) {
	vmtxCtx h = g->ctx.vmtx;
	return h->tbl.vMetrics.cnt;
}