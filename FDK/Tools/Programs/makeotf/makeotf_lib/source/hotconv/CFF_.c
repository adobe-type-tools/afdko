/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Compact Font Format table.
 */

#include "CFF_.h"

/* ---------------------------- Table Definition --------------------------- */

struct CFF_Ctx_ {
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void CFF_New(hotCtx g) {
	CFF_Ctx h = MEM_NEW(g, sizeof(struct CFF_Ctx_));

	/* Link contexts */
	h->g = g;
	g->ctx.CFF_ = h;
}

int CFF_Fill(hotCtx g) {
	return 1;
}

void CFF_Write(hotCtx g) {
	CFF_Ctx h = g->ctx.CFF_;
	long count;
	char *ptr;

	/* Block copy CFF data */
	for (ptr = g->cb.cffSeek(g->cb.ctx, 0, &count);
	     count > 0;
	     ptr = g->cb.cffRefill(g->cb.ctx, &count)) {
		OUTN(count, ptr);
	}
}

void CFF_Reuse(hotCtx g) {
}

void CFF_Free(hotCtx g) {
	MEM_FREE(g, g->ctx.CFF_);
}