/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Anonymous table (from client) support.
 */

#include "anon.h"
#include "dynarr.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	Tag tag;
	hotAnonRefill refill;
} AnonTbl;

struct anonCtx_ {
	dnaDCL(AnonTbl, tbls);
	int iTbl;
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void anonNew(hotCtx g) {
	anonCtx h = MEM_NEW(g, sizeof(struct anonCtx_));

	dnaINIT(g->dnaCtx, h->tbls, 5, 5);
	h->iTbl = 0;

	/* Link contexts */
	h->g = g;
	g->ctx.anon = h;
}

int anonFill(hotCtx g) {
	anonCtx h = g->ctx.anon;
	return h->tbls.cnt != 0;
}

void anonWrite(hotCtx g) {
	anonCtx h = g->ctx.anon;
	AnonTbl *tbl = &h->tbls.array[h->iTbl++];

	/* Copy client data to output */
	for (;; ) {
		long count;
		char *data = tbl->refill(g->cb.ctx, &count, tbl->tag);
		if (count == 0) {
			break;
		}
		OUTN(count, data);
	}
}

void anonReuse(hotCtx g) {
	anonCtx h = g->ctx.anon;
	h->tbls.cnt = 0;
	h->iTbl = 0;
}

void anonFree(hotCtx g) {
	anonCtx h = g->ctx.anon;
	dnaFREE(h->tbls);
	MEM_FREE(g, g->ctx.anon);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Add anonymous table to list */
void anonAddTable(hotCtx g, unsigned long tag, hotAnonRefill refill) {
	anonCtx h = g->ctx.anon;
	AnonTbl *tbl = dnaNEXT(h->tbls);
	tbl->tag = tag;
	tbl->refill = refill;
}