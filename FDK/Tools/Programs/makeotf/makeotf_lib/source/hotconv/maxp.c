/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Maximum profile table (only version 0.5 is required for OpenType).
 */

#include "maxp.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	Fixed version;
	unsigned short numGlyphs;
} maxpTbl;

struct maxpCtx_ {
	maxpTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void maxpNew(hotCtx g) {
	maxpCtx h = MEM_NEW(g, sizeof(struct maxpCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.maxp = h;
}

int maxpFill(hotCtx g) {
	maxpCtx h = g->ctx.maxp;

	h->tbl.version      = VERSION(0, 5);
	h->tbl.numGlyphs    = (unsigned short)g->font.glyphs.cnt;

	return 1;
}

void maxpWrite(hotCtx g) {
	maxpCtx h = g->ctx.maxp;

	OUT4(h->tbl.version);
	OUT2(h->tbl.numGlyphs);
}

void maxpReuse(hotCtx g) {
}

void maxpFree(hotCtx g) {
	MEM_FREE(g, g->ctx.maxp);
}

/* ------------------------ Supplementary Functions ------------------------ */