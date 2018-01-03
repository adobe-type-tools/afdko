/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Dummy Type 13 module for implementations that don't support Moriswa
 * fonts and Type 13.
 */

#include "t13.h"

struct t13Ctx_ {
	tcCtx g;                    /* Package context */
};

/* Initialize module */
void t13New(tcCtx g) {
	t13Ctx h = MEM_NEW(g, sizeof(struct t13Ctx_));

	/* Link contexts */
	h->g = g;
	g->ctx.t13 = h;
}

/* Free resources */
void t13Free(tcCtx g) {
	t13Ctx h = g->ctx.t13;
	MEM_FREE(g, h);
}

/* Return 0 for regular CIDFont and 1 otherwise */
int t13CheckConv(tcCtx g, psCtx ps, csDecrypt *decrypt) {
	psToken *token = psGetToken(ps);
	return token->type != PS_LITERAL || !psMatchValue(ps, token, "/CCRun");
}

/* Return 0 */
int t13CheckAuth(tcCtx g, Font *font) {
	return 0;
}