/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	MODULE DESCRIPTION.
 */

#include "xxxx.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
} xxxxTbl;

struct xxxxCtx_ {
	xxxxTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void xxxxNew(hotCtx g) {
	xxxxCtx h = MEM_NEW(g, sizeof(struct xxxxCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.xxxx = h;
}

int xxxxFill(hotCtx g) {
	xxxxCtx h = g->ctx.xxxx;
	return 1;
}

void xxxxWrite(hotCtx g) {
	xxxxCtx h = g->ctx.xxxx;
}

void xxxxReuse(hotCtx g) {
	xxxxCtx h = g->ctx.xxxx;
}

void xxxxFree(hotCtx g) {
	MEM_FREE(g, g->ctx.xxxx);
}

/* ------------------------ Supplementary Functions ------------------------ */