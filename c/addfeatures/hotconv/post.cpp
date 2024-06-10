/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * PostScript table (only version 3.0 [no names] is required for OpenType).
 */

#include "post.h"

#include <string.h>

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
    int namelength;
    char *nameblock;
} postTbl;

/* --------------------------- Context Definition -------------------------- */
struct postCtx_ {
    postTbl tbl; /* Table data */
    hotCtx g;    /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void postNew(hotCtx g) {
    postCtx h = (postCtx)MEM_NEW(g, sizeof(struct postCtx_));

    /* Link contexts */
    h->g = g;
    g->ctx.post = h;
    h->tbl.namelength = 0;
    h->tbl.nameblock = NULL;
    h->tbl.version = VERSION(3, 0);
}

void postRead(hotCtx g, int offset, int length) {
    postCtx h = g->ctx.post;

    g->cb.stm.seek(&g->cb.stm, g->in_stream, offset);
    g->bufleft = 0;
    h->tbl.version = hotIn4(g);
    /*
    h->tbl.italicAngle = hotIn4(g);
    h->tbl.underlinePosition = hotIn2(g);
    h->tbl.underlineThickness = hotIn2(g);
    h->tbl.isFixedPitch = hotIn4(g);
    h->tbl.minMemType42 = hotIn4(g);
    h->tbl.maxMemType42 = hotIn4(g);
    h->tbl.minMemType1 = hotIn4(g);
    h->tbl.maxMemType1 = hotIn4(g);
    */
    if (h->tbl.version != VERSION(2, 0) || length <= 32) {
        g->logger->log(sWARNING, "here1");
        return;
    }
    g->cb.stm.seek(&g->cb.stm, g->in_stream, offset + 32);
    char *buf, *p;
    length -= 32;
    h->tbl.namelength = length;
    p = h->tbl.nameblock = (char *)MEM_NEW(g, length);
    while (length > 0) {
        int l = g->cb.stm.read(&g->cb.stm, g->in_stream, &buf);
        if (l <= 0) {
            g->logger->log(sFATAL, "Input name table unexpectedly short");
        } else if (l < length) {
            memcpy(p, buf, l);
            p += l;
            length -= l;
        } else {
            memcpy(p, buf, length);
            length = 0;
        }
    }
}

int postFill(hotCtx g) {
    postCtx h = g->ctx.post;

    h->tbl.italicAngle = g->font.ItalicAngle;
    h->tbl.underlinePosition = g->font.UnderlinePosition.getDefault() +
                               g->font.UnderlineThickness.getDefault() / 2;
    h->tbl.underlineThickness = g->font.UnderlineThickness.getDefault();
    h->tbl.isFixedPitch = (g->font.flags & FI_FIXED_PITCH) != 0;
    h->tbl.minMemType42 = 0;
    h->tbl.maxMemType42 = 0;
    h->tbl.minMemType1 = 0;
    h->tbl.maxMemType1 = 0;

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
    if (h->tbl.namelength > 0)
        OUTN(h->tbl.namelength, h->tbl.nameblock);
}

void postReuse(hotCtx g) {
    postCtx h = g->ctx.post;
    MEM_FREE(g, h->tbl.nameblock);
    h->tbl.namelength = 0;
    h->tbl.nameblock = NULL;
}

void postFree(hotCtx g) {
    MEM_FREE(g, g->ctx.post->tbl.nameblock);
    MEM_FREE(g, g->ctx.post);
}
