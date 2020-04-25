/* Copyright 2020 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Style Attributes Table
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "STAT.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
    uint16_t majorVersion;
    uint16_t minorVersion;
    uint16_t designAxisSize;
    uint16_t designAxisCount;
    LOffset  designAxesOffset;
    uint16_t axisValueCount;
    LOffset  offsetToAxisValueOffsets;
    uint16_t elidedFallbackNameID;
    dnaDCL(Offset, axisValueOffsets);
} STATTbl;
#define TBL_HDR_SIZE (int32 + uint16 * 4 + uint32 * 2)

/* --------------------------- Context Definition ---------------------------*/

typedef struct {
    Tag axisTag;
    uint16_t axisNameID;
    uint16_t axisOrdering;
} AxisRecord;
#define AXIS_RECORD_SIZE (uint32 + uint16 * 2)

typedef struct {
    Tag axisTag;
    uint16_t axisIndex;
    uint16_t flags;
    uint16_t valueNameID;
    Fixed value;
} AxisValue;

struct STATCtx_ {
    dnaDCL(AxisRecord, designAxes);
    dnaDCL(AxisValue, axisValues);

    STATTbl tbl; /* Table data */
    hotCtx g;    /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void STATNew(hotCtx g) {
    STATCtx h = MEM_NEW(g, sizeof(struct STATCtx_));

    dnaINIT(g->dnaCtx, h->tbl.axisValueOffsets, 5, 5);
    dnaINIT(g->dnaCtx, h->designAxes, 5, 5);
    dnaINIT(g->dnaCtx, h->axisValues, 5, 5);

    /* Link contexts */
    h->g = g;
    g->ctx.STAT = h;
}

int STATFill(hotCtx g) {
    STATCtx h = g->ctx.STAT;
    Offset currOff = 0;
    long i, j;

    if (h->designAxes.cnt == 0 && h->axisValues.cnt == 0 &&
        h->tbl.elidedFallbackNameID == 0) {
        return 0;
    }

    h->tbl.majorVersion = 1;
    h->tbl.minorVersion = 2;
    h->tbl.designAxisSize = AXIS_RECORD_SIZE;
    h->tbl.designAxisCount = h->designAxes.cnt;
    h->tbl.designAxesOffset = NULL_OFFSET;
    h->tbl.axisValueCount = h->axisValues.cnt;
    h->tbl.offsetToAxisValueOffsets = NULL_OFFSET;

    currOff = TBL_HDR_SIZE;

    if (h->tbl.designAxisCount) {
        h->tbl.designAxesOffset = currOff;
        currOff += h->tbl.designAxisCount * AXIS_RECORD_SIZE;
    }

    if (h->tbl.axisValueCount) {
        h->tbl.offsetToAxisValueOffsets = currOff;
        dnaSET_CNT(h->tbl.axisValueOffsets, h->tbl.axisValueCount);

        for (i = 0; i < h->axisValues.cnt; i++) {
            bool found = false;
            AxisValue *av = &h->axisValues.array[i];
            for (j = 0; j < h->designAxes.cnt; j++) {
                AxisRecord *ar = &h->designAxes.array[j];
                if (ar->axisTag == av->axisTag) {
                    av->axisIndex = j;
                    found = true;
                    break;
                }
            }
            if (!found) {
                /* XXX error */
            }
        }
    }

    return 1;
}

void STATWrite(hotCtx g) {
    STATCtx h = g->ctx.STAT;
    Offset start, end;
    long i;

    OUT2(h->tbl.majorVersion);
    OUT2(h->tbl.minorVersion);
    OUT2(h->tbl.designAxisSize);
    OUT2(h->tbl.designAxisCount);
    OUT4(h->tbl.designAxesOffset);
    OUT2(h->tbl.axisValueCount);
    OUT4(h->tbl.offsetToAxisValueOffsets);
    OUT2(h->tbl.elidedFallbackNameID);

    for (i = 0; i < h->designAxes.cnt; i++) {
        AxisRecord *ar = &h->designAxes.array[i];
        OUT4(ar->axisTag);
        OUT2(ar->axisNameID);
        OUT2(ar->axisOrdering);
    }

    start = TELL();
    for (i = 0; i < h->tbl.axisValueOffsets.cnt; i++) {
        OUT2(0);
    }

    for (i = 0; i < h->axisValues.cnt; i++) {
        AxisValue *av = &h->axisValues.array[i];

        h->tbl.axisValueOffsets.array[i] = TELL() - start;

        OUT2(1);
        OUT2(av->axisIndex);
        OUT2(av->flags);
        OUT2(av->valueNameID);
        OUT4(av->value);
    }

    end = TELL();
    SEEK(start);
    for (i = 0; i < h->tbl.axisValueOffsets.cnt; i++) {
        OUT2(h->tbl.axisValueOffsets.array[i]);
    }
    SEEK(end);
}

void STATReuse(hotCtx g) {
}

void STATFree(hotCtx g) {
    STATCtx h = g->ctx.STAT;

    dnaFREE(h->designAxes);
    dnaFREE(h->axisValues);

    MEM_FREE(g, g->ctx.STAT);
}

/* ------------------------ Supplementary Functions ------------------------ */

void STATAddDesignAxis(hotCtx g, Tag tag, uint16_t nameID, uint16_t ordering) {
    STATCtx h = g->ctx.STAT;

    AxisRecord *ar = dnaNEXT(h->designAxes);
    ar->axisTag = tag;
    ar->axisNameID = nameID;
    ar->axisOrdering = ordering;
}

void STATAddAxisValue(hotCtx g, Tag axisTag, uint16_t flags, uint16_t nameID,
                      Fixed value) {
    STATCtx h = g->ctx.STAT;

    AxisValue *av = dnaNEXT(h->axisValues);
    av->axisTag = axisTag;
    av->flags = flags;
    av->valueNameID = nameID;
    av->value = value;
}

void STATSetElidedFallbackNameID(hotCtx g, uint16_t nameID) {
    STATCtx h = g->ctx.STAT;

    if (h->tbl.elidedFallbackNameID) {
        /* XXX: error? */
        return;
    }
    h->tbl.elidedFallbackNameID = nameID;
}
