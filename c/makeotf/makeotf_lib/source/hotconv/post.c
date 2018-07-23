/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	PostScript table (only version 3.0 [no names] is required for OpenType).
 */

#include "post.h"

/* ---------------------------- Table Definition --------------------------- */
typedef struct {
	Fixed version;
	Fixed italicAngle;
	FWord underlinePosition;
	FWord underlineThickness;
	unsigned long isFixedPitch;
	unsigned long minMemType42;
	unsigned long maxMemType42;
	unsigned long minMemType1;
	unsigned long maxMemType1;
} postTbl;

/* --------------------------- Context Definition -------------------------- */
struct postCtx_ {
	postTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void postNew(hotCtx g) {
	postCtx h = MEM_NEW(g, sizeof(struct postCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.post = h;
}

int postFill(hotCtx g) {
	postCtx h = g->ctx.post;

	h->tbl.version              = VERSION(3, 0);
	h->tbl.italicAngle          = g->font.ItalicAngle;
	h->tbl.underlinePosition    = g->font.UnderlinePosition +
	    g->font.UnderlineThickness / 2;
	h->tbl.underlineThickness   = g->font.UnderlineThickness;
	h->tbl.isFixedPitch         = (g->font.flags & FI_FIXED_PITCH) != 0;
	h->tbl.minMemType42         = 0;
	h->tbl.maxMemType42         = 0;
	h->tbl.minMemType1          = 0;
	h->tbl.maxMemType1          = 0;

	return 1;
}

void postWrite(hotCtx g) {
	postCtx h = g->ctx.post;

	OUT4(h->tbl.version);
	OUT4(h->tbl.italicAngle);
	OUT2(h->tbl.underlinePosition);
	OUT2(h->tbl.underlineThickness);
	OUT4(h->tbl.isFixedPitch);
	OUT4(h->tbl.minMemType42);
	OUT4(h->tbl.maxMemType42);
	OUT4(h->tbl.minMemType1);
	OUT4(h->tbl.maxMemType1);
}

void postReuse(hotCtx g) {
}

void postFree(hotCtx g) {
	MEM_FREE(g, g->ctx.post);
}