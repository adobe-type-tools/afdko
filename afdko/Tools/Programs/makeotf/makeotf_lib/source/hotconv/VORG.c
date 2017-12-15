/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Vertical Origin table
 */

#include "VORG.h"
#include "dynarr.h"
/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	GID gid;
	short vertOriginY;
} VertOriginYMetric;

typedef struct {
	unsigned short majorVersion;
	unsigned short minorVersion;
	short defaultVertOriginY;
	unsigned short numVertOriginYMetrics;
	dnaDCL(VertOriginYMetric, vertOriginYMetrics);
} VORGTbl;

struct VORGCtx_ {
	VORGTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void VORGNew(hotCtx g) {
	VORGCtx h = MEM_NEW(g, sizeof(struct VORGCtx_));

	dnaINIT(g->dnaCtx, h->tbl.vertOriginYMetrics, 300, 600);

	/* Link contexts */
	h->g = g;
	g->ctx.VORG = h;
}

int VORGFill(hotCtx g) {
	VORGCtx h = g->ctx.VORG;
	long i;
	short dflt;

	if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE))  && (!IS_CID(g))) {
		return 0;
	}

	h->tbl.majorVersion = 1;
	h->tbl.minorVersion = 0;
	h->tbl.defaultVertOriginY = dflt = g->font.TypoAscender;

	for (i = 0; i < g->font.glyphs.cnt; i++) {
		if (g->font.glyphs.array[i].vOrigY != dflt) {
			VertOriginYMetric *vOrigMtx = dnaNEXT(h->tbl.vertOriginYMetrics);
			vOrigMtx->gid = (unsigned short)i;
			vOrigMtx->vertOriginY = g->font.glyphs.array[i].vOrigY;
		}
	}

	h->tbl.numVertOriginYMetrics =
	    (unsigned short)h->tbl.vertOriginYMetrics.cnt;

	return 1;
}

void VORGWrite(hotCtx g) {
	VORGCtx h = g->ctx.VORG;
	long i;

	OUT2(h->tbl.majorVersion);
	OUT2(h->tbl.minorVersion);
	OUT2(h->tbl.defaultVertOriginY);
	OUT2(h->tbl.numVertOriginYMetrics);

	for (i = 0; i < h->tbl.vertOriginYMetrics.cnt; i++) {
		VertOriginYMetric *vOrigMtx = &h->tbl.vertOriginYMetrics.array[i];
		OUT2(vOrigMtx->gid);
		OUT2(vOrigMtx->vertOriginY);
	}
}

void VORGReuse(hotCtx g) {
	VORGCtx h = g->ctx.VORG;
	h->tbl.vertOriginYMetrics.cnt = 0;
}

void VORGFree(hotCtx g) {
	VORGCtx h = g->ctx.VORG;

	dnaFREE(h->tbl.vertOriginYMetrics);
	MEM_FREE(g, g->ctx.VORG);
}

/* ------------------------ Supplementary Functions ------------------------ */