/* Copyright 2020 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Style Attributes Table
 */

#include <stdio.h>
#include <stdlib.h>
#include "name.h"
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
    Tag axisTag; /* used internally, not part of the format */
    uint16_t axisIndex;
    Fixed value;
} AxisValue;
#define AXIS_VALUE_SIZE (uint16 + int32)

typedef struct {
    uint16_t format;
    uint16_t size;
    union {
        struct {
            Tag axisTag; /* used internally, not part of the format */
            uint16_t axisIndex;
            uint16_t flags;
            uint16_t valueNameID;
            Fixed value;
        } format1;
        struct {
            Tag axisTag; /* used internally, not part of the format */
            uint16_t axisIndex;
            uint16_t flags;
            uint16_t valueNameID;
            Fixed nominalValue;
            Fixed rangeMinValue;
            Fixed rangeMaxValue;
        } format2;
        struct {
            Tag axisTag; /* used internally, not part of the format */
            uint16_t axisIndex;
            uint16_t flags;
            uint16_t valueNameID;
            Fixed value;
            Fixed linkedValue;
        } format3;
        struct {
            uint16_t axisCount;
            uint16_t flags;
            uint16_t valueNameID;
            AxisValue* axisValues;
        } format4;
    };
} AxisValueTable;
#define AXIS_VALUE_TABLE1_SIZE (uint16 * 4 + int32)
#define AXIS_VALUE_TABLE2_SIZE (uint16 * 4 + int32 * 3)
#define AXIS_VALUE_TABLE3_SIZE (uint16 * 4 + int32 * 2)
#define AXIS_VALUE_TABLE4_SIZE(n) (uint16 * 4 + AXIS_VALUE_SIZE * n)

struct STATCtx_ {
    dnaDCL(AxisRecord, designAxes);
    dnaDCL(AxisValueTable, axisValues);
    uint16_t elidedFallbackNameID;

    STATTbl tbl; /* Table data */
    hotCtx g;    /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void STATNew(hotCtx g) {
    STATCtx h = MEM_NEW(g, sizeof(struct STATCtx_));

    dnaINIT(g->dnaCtx, h->designAxes, 5, 5);
    dnaINIT(g->dnaCtx, h->axisValues, 5, 5);
    h->elidedFallbackNameID = 0;

    /* Link contexts */
    h->g = g;
    g->ctx.STAT = h;
}

bool axisIndexOfTag(STATCtx h, Tag tag, uint16_t *index) {
    long i;

    for (i = 0; i < h->designAxes.cnt; i++) {
        AxisRecord *ar = &h->designAxes.array[i];
        if (ar->axisTag == tag) {
            *index = i;
            return true;
        }
    }

    return false;
}

int STATFill(hotCtx g) {
    STATCtx h = g->ctx.STAT;
    Offset currOff = 0;
    long i, j;

    if (h->designAxes.cnt == 0 && h->axisValues.cnt == 0 &&
        h->elidedFallbackNameID == 0) {
        return 0;
    }

    if (!nameVerifyIDExists(g, h->elidedFallbackNameID))
        hotMsg(g, hotFATAL, "ElidedFallbackNameID points to a nameID that "
                            "does not exist in \"name\" table.");

    h->tbl.majorVersion = 1;
    h->tbl.minorVersion = 2;
    h->tbl.designAxisSize = AXIS_RECORD_SIZE;
    h->tbl.designAxisCount = h->designAxes.cnt;
    h->tbl.designAxesOffset = NULL_OFFSET;
    h->tbl.axisValueCount = h->axisValues.cnt;
    h->tbl.offsetToAxisValueOffsets = NULL_OFFSET;
    h->tbl.elidedFallbackNameID = h->elidedFallbackNameID;

    currOff = TBL_HDR_SIZE;

    if (h->tbl.designAxisCount) {
        h->tbl.designAxesOffset = currOff;
        currOff += h->tbl.designAxisCount * AXIS_RECORD_SIZE;
    }

    if (h->tbl.axisValueCount) {
        h->tbl.offsetToAxisValueOffsets = currOff;

        for (i = 0; i < h->axisValues.cnt; i++) {
            AxisValueTable *av = &h->axisValues.array[i];
            switch (av->format) {
                case 1:
                case 2:
                case 3:
                    if (!axisIndexOfTag(h, av->format1.axisTag,
                        &av->format1.axisIndex)) {
                        hotMsg(g, hotFATAL,
                               "No STAT DesignAxis defined for \"%c%c%c%c\".",
                               TAG_ARG(av->format1.axisTag));
                    }
                    break;

                case 4:
                    for (j = 0; j < av->format4.axisCount; j++) {
                        if (!axisIndexOfTag(h,
                                av->format4.axisValues[j].axisTag,
                                &av->format4.axisValues[j].axisIndex)) {
                            hotMsg(g, hotFATAL,
                                   "No STAT DesignAxis defined for \"%c%c%c%c\".",
                                   TAG_ARG(av->format4.axisValues[j].axisTag));
                        }
                    }
                    break;

                default:
                    hotMsg(g, hotFATAL,
                           "[internal] unknown STAT Axis Value Table format <%d>.",
                           av->format);
                    break;
            }
        }
    }

    return 1;
}

void STATWrite(hotCtx g) {
    STATCtx h = g->ctx.STAT;
    Offset offset;
    long i, j;

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

    offset = h->axisValues.cnt * uint16;
    for (i = 0; i < h->axisValues.cnt; i++) {
        AxisValueTable *av = &h->axisValues.array[i];
        OUT2(offset);
        offset += av->size;
    }

    for (i = 0; i < h->axisValues.cnt; i++) {
        AxisValueTable *av = &h->axisValues.array[i];
        OUT2(av->format);
        switch (av->format) {
            case 1:
                OUT2(av->format1.axisIndex);
                OUT2(av->format1.flags);
                OUT2(av->format1.valueNameID);
                OUT4(av->format1.value);
                break;

            case 2:
                OUT2(av->format2.axisIndex);
                OUT2(av->format2.flags);
                OUT2(av->format2.valueNameID);
                OUT4(av->format2.nominalValue);
                OUT4(av->format2.rangeMinValue);
                OUT4(av->format2.rangeMaxValue);
                break;

            case 3:
                OUT2(av->format3.axisIndex);
                OUT2(av->format3.flags);
                OUT2(av->format3.valueNameID);
                OUT4(av->format3.value);
                OUT4(av->format3.linkedValue);
                break;

            case 4:
                OUT2(av->format4.axisCount);
                OUT2(av->format4.flags);
                OUT2(av->format4.valueNameID);
                for (j = 0; j < av->format4.axisCount; j++) {
                    OUT2(av->format4.axisValues[j].axisIndex);
                    OUT4(av->format4.axisValues[j].value);
                }
                break;

            default:
                hotMsg(g, hotFATAL,
                       "[internal] unknown STAT Axis Value Table format <%d>.",
                       av->format);
                break;
        }
    }
}

void STATReuse(hotCtx g) {
    STATCtx h = g->ctx.STAT;
    long i;

    for (i = 0; i < h->axisValues.cnt; i++) {
        AxisValueTable *av = &h->axisValues.array[i];
        if (av->format == 4) {
            MEM_FREE(g, av->format4.axisValues);
        }
    }

    h->designAxes.cnt = 0;
    h->axisValues.cnt = 0;
    h->elidedFallbackNameID = 0;
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
    AxisRecord *ar;
    long i;

    for (i = 0; i < h->designAxes.cnt; i++) {
        AxisRecord *ar = &h->designAxes.array[i];
        if (ar->axisTag == tag)
            hotMsg(g, hotFATAL, "STAT DesignAxis with \"%c%c%c%c\" tag "
                   "is already defined.", TAG_ARG(tag));
    }

    ar = dnaNEXT(h->designAxes);
    ar->axisTag = tag;
    ar->axisNameID = nameID;
    ar->axisOrdering = ordering;
}

void STATAddAxisValueTable(hotCtx g, uint16_t format, Tag *axisTags,
                           Fixed *values, long count, uint16_t flags,
                           uint16_t nameID, Fixed minValue, Fixed maxValue) {
    STATCtx h = g->ctx.STAT;
    long i;

    AxisValueTable *av = dnaNEXT(h->axisValues);

    if (count > 1 && format == 1)
        format = 4;

    av->format = format;

    switch (format) {
        case 1:
            av->size = AXIS_VALUE_TABLE1_SIZE;
            av->format1.axisTag = axisTags[0];
            av->format1.flags = flags;
            av->format1.valueNameID = nameID;
            av->format1.value = values[0];
            break;

        case 2:
            av->size = AXIS_VALUE_TABLE2_SIZE;
            av->format2.axisTag = axisTags[0];
            av->format2.flags = flags;
            av->format2.valueNameID = nameID;
            av->format2.nominalValue = values[0];
            av->format2.rangeMinValue = minValue;
            av->format2.rangeMaxValue = maxValue;
            break;

        case 3:
            av->size = AXIS_VALUE_TABLE3_SIZE;
            av->format3.axisTag = axisTags[0];
            av->format3.flags = flags;
            av->format3.valueNameID = nameID;
            av->format3.value = values[0];
            av->format3.linkedValue = minValue;
            break;

        case 4:
            av->size = AXIS_VALUE_TABLE4_SIZE(count);
            av->format4.axisCount = count;
            av->format4.flags = flags;
            av->format4.valueNameID = nameID;
            av->format4.axisValues = MEM_NEW(g, sizeof(AxisValue) * count);
            for (i = 0; i < count; i++) {
                av->format4.axisValues[i].axisTag = axisTags[i];
                av->format4.axisValues[i].value = values[i];
            }
            break;

        default:
            hotMsg(g, hotFATAL,
                   "[internal] unknown STAT Axis Value Table format <%d>.",
                   av->format);
            break;
    }
}

bool STATSetElidedFallbackNameID(hotCtx g, uint16_t nameID) {
    STATCtx h = g->ctx.STAT;

    if (h->elidedFallbackNameID)
        return false;
    h->elidedFallbackNameID = nameID;
    return true;
}
