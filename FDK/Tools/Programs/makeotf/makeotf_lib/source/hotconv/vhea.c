/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Vertical header table.
 */

#include "vhea.h"
#include "vmtx.h"
#include "map.h"

#include <math.h>

/* ---------------------------- Table Definition --------------------------- */
typedef struct {
	Fixed version;
	FWord vertTypoAscender;
	FWord vertTypoDescender;
	FWord vertTypoLineGap;
	FWord advanceHeightMax;
	FWord minTopSideBearing;
	FWord minBottomSideBearing;
	FWord yMaxExtent;
	short caretSlopeRise;
	short caretSlopeRun;
	short caretOffset;
	short reserved[4];
	short metricDataFormat;
	unsigned short numberOfLongVerMetrics;
} vheaTbl;

/* --------------------------- Context Definition -------------------------- */
struct vheaCtx_ {
	vheaTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void vheaNew(hotCtx g) {
	vheaCtx h = MEM_NEW(g, sizeof(struct vheaCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.vhea = h;
}

int vheaFill(hotCtx g) {
	vheaCtx h = g->ctx.vhea;
	FontInfo_ *font = &g->font;

	if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE))  && (!IS_CID(g))) {
		return 0;
	}

	h->tbl.version                  = VERSION(1, 1);
	h->tbl.vertTypoAscender         = font->VertTypoAscender;
	h->tbl.vertTypoDescender        = font->VertTypoDescender;
	h->tbl.vertTypoLineGap          = font->VertTypoLineGap;

	/* Always set a horizontal caret for vertical writing */
	h->tbl.caretSlopeRise           = 0;
	h->tbl.caretSlopeRun            = 1;
	h->tbl.caretOffset              = 0;

	h->tbl.advanceHeightMax         = font->maxAdv.v;
	h->tbl.minTopSideBearing        = font->minBearing.top;
	h->tbl.minBottomSideBearing     = font->minBearing.bottom;
	h->tbl.yMaxExtent               = font->maxExtent.v;
	h->tbl.reserved[0]              = 0;
	h->tbl.reserved[1]              = 0;
	h->tbl.reserved[2]              = 0;
	h->tbl.reserved[3]              = 0;
	h->tbl.metricDataFormat         = 0;
	h->tbl.numberOfLongVerMetrics   = vmtxGetNLongVerMetrics(g);

	return 1;
}

void vheaWrite(hotCtx g) {
	vheaCtx h = g->ctx.vhea;

	OUT4(h->tbl.version);
	OUT2(h->tbl.vertTypoAscender);
	OUT2(h->tbl.vertTypoDescender);
	OUT2(h->tbl.vertTypoLineGap);
	OUT2(h->tbl.advanceHeightMax);
	OUT2(h->tbl.minTopSideBearing);
	OUT2(h->tbl.minBottomSideBearing);
	OUT2(h->tbl.yMaxExtent);
	OUT2(h->tbl.caretSlopeRise);
	OUT2(h->tbl.caretSlopeRun);
	OUT2(h->tbl.caretOffset);
	OUT2(h->tbl.reserved[0]);
	OUT2(h->tbl.reserved[1]);
	OUT2(h->tbl.reserved[2]);
	OUT2(h->tbl.reserved[3]);
	OUT2(h->tbl.metricDataFormat);
	OUT2(h->tbl.numberOfLongVerMetrics);
}

void vheaReuse(hotCtx g) {
}

void vheaFree(hotCtx g) {
	MEM_FREE(g, g->ctx.vhea);
}