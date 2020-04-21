/* Copyright 2020 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Style Attributes Table
 */

#include <stdlib.h>
#include <stdio.h>
#include "STAT.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
    Tag axisTag;
    uint16_t axisNameID;
    uint16_t axisOrdering;
} AxisRecord;
#define AXIS_RECORD_SIZE (uint32 + uint16 * 2)

typedef struct {
    Fixed version;
    uint16_t designAxisSize;
    uint16_t designAxisCount;
    LOffset  designAxesOffset;
    uint16_t axisValueCount;
    LOffset  offsetToAxisValueOffsets;
    uint16_t elidedFallbackNameID;
    AxisRecord *designAxes;
    Offset *axisValueOffsets;
} STATTbl;
#define TBL_HDR_SIZE (int32 + uint16 * 4 + uint32 * 2)

/* --------------------------- Context Definition ---------------------------*/

struct STATCtx_ {
    struct {
        Offset curr;
        Offset shared;
    } offset;

    STATTbl tbl; /* Table data */
    hotCtx g;    /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void STATNew(hotCtx g) {
    STATCtx h = MEM_NEW(g, sizeof(struct STATCtx_));

    /* Link contexts */
    h->g = g;
    g->ctx.STAT = h;
}

int STATFill(hotCtx g) {
    STATCtx h = g->ctx.STAT;

    h->tbl.version = VERSION(1, 2);
    h->tbl.designAxisSize = AXIS_RECORD_SIZE;

    h->offset.curr = TBL_HDR_SIZE;

    h->offset.shared = h->offset.curr;

    return 1;
}

void STATWrite(hotCtx g) {
    STATCtx h = g->ctx.STAT;

    OUT4(h->tbl.version);
    OUT2(h->tbl.designAxisSize);
    OUT2(h->tbl.designAxisCount);
    OUT4(h->tbl.designAxesOffset);
    OUT2(h->tbl.axisValueCount);
    OUT4(h->tbl.offsetToAxisValueOffsets);
    OUT2(h->tbl.elidedFallbackNameID);
}

void STATReuse(hotCtx g) {
}

void STATFree(hotCtx g) {
    MEM_FREE(g, g->ctx.STAT);
}
