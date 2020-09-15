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
        hotMsg(g, hotFATAL, "[STAT] ElidedFallbackNameID points to a nameID that "
                            "does not exist in \"name\" table.");

    h->tbl.majorVersion = 1;
    h->tbl.minorVersion = 1;
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
                               "[STAT] No DesignAxis defined for \"%c%c%c%c\".",
                               TAG_ARG(av->format1.axisTag));
                    }
                    break;

                case 4:
                    h->tbl.minorVersion = 2;
                    for (j = 0; j < av->format4.axisCount; j++) {
                        if (!axisIndexOfTag(h,
                                av->format4.axisValues[j].axisTag,
                                &av->format4.axisValues[j].axisIndex)) {
                            hotMsg(g, hotFATAL,
                                   "[STAT] No DesignAxis defined for \"%c%c%c%c\".",
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
    long index;
    int i;
    long j;

    // Currently registered tags are 'wght', 'wdth', 'opsz', 'ital', 'slnt'
    char tagString[4] = {TAG_ARG(tag)};
    const uint32_t *regTags[5] = {
                                  TAG('i', 't', 'a', 'l'),
                                  TAG('o', 'p', 's', 'z'),
                                  TAG('s', 'l', 'n', 't'),
                                  TAG('w', 'd', 't', 'h'),
                                  TAG('w', 'g', 'h', 't'),
                                  };

    // Unregistered tags should be all uppercase
    int regTag = false;
    int size = sizeof(regTags) / sizeof(regTags[0]);
    for (i = 0; i < size; i++) {
        if (tag == regTags[i]) {
            regTag = true;
            break;
            }
    }
    if (!regTag) {
        int hasLC = false;
        for (j = 0; j < 4; j++) {
            if (tagString[j] >= 'a' && tagString[j] <= 'z') {
                hasLC = true;
                break;
            }
        }
        if (hasLC) {
            hotMsg(g, hotWARNING, "[STAT] Unregistered axis tag \"%c%c%c%c\" "
            "should be uppercase.\n", TAG_ARG(tag));
        }
    }

    for (index = 0; index < h->designAxes.cnt; index++) {
        AxisRecord *ar = &h->designAxes.array[index];
        if (ar->axisTag == tag)
            hotMsg(g, hotFATAL, "[STAT] DesignAxis tag \"%c%c%c%c\" "
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
    long j;
    long k;

    AxisValueTable *av = dnaNEXT(h->axisValues);

    if (count > 1 && format == 1)
        format = 4;

    av->format = format;

    switch (format) {
        case 1:
            for (i = 0; i < h->axisValues.cnt; i++) {
                AxisValueTable *refav = &h->axisValues.array[i];
                if (refav->format1.axisTag == axisTags[0]
                    && refav->format1.value == values[0])
                    hotMsg(g, hotFATAL, "[STAT] AxisValueTable already defined "
                    "for axis \"%c%c%c%c\" with value %.2f\n",
                    TAG_ARG(axisTags[0]), FIX2DBL(values[0]));
            }
            av->size = AXIS_VALUE_TABLE1_SIZE;
            av->format1.axisTag = axisTags[0];
            av->format1.flags = flags;
            av->format1.valueNameID = nameID;
            av->format1.value = values[0];
            break;

        case 2:
            if (minValue > maxValue) {
                    hotMsg(g, hotFATAL, "[STAT] \"%c%c%c%c\" AxisValue "
                        "min %.2f cannot be greater than max %.2f",
                        TAG_ARG(axisTags[0]), FIX2DBL(minValue),
                        FIX2DBL(maxValue));
            }
            if ((values[0] < minValue) || (values[0] > maxValue)) {
                    hotMsg(g, hotFATAL, "[STAT] \"%c%c%c%c\" AxisValue "
                        "default value %.2f is not in range %.2f-%.2f",
                        TAG_ARG(axisTags[0]), FIX2DBL(values[0]),
                        FIX2DBL(minValue), FIX2DBL(maxValue));
            }

            for (i = 0; i < h->axisValues.cnt; i++) {
                AxisValueTable *refav = &h->axisValues.array[i];
                if (refav->format2.axisTag == axisTags[0]
                    && refav->format2.nominalValue == values[0]
                    && refav->format2.rangeMinValue == minValue
                    && refav->format2.rangeMaxValue == maxValue)
                    hotMsg(g, hotFATAL, "[STAT] AxisValueTable already defined "
                    "for axis \"%c%c%c%c\" with values %.2f %.2f %.2f\n",
                    TAG_ARG(axisTags[0]), FIX2DBL(values[0]),
                        FIX2DBL(minValue), FIX2DBL(maxValue));
            }

            av->size = AXIS_VALUE_TABLE2_SIZE;
            av->format2.axisTag = axisTags[0];
            av->format2.flags = flags;
            av->format2.valueNameID = nameID;
            av->format2.nominalValue = values[0];
            av->format2.rangeMinValue = minValue;
            av->format2.rangeMaxValue = maxValue;
            break;

        case 3:
            for (i = 0; i < h->axisValues.cnt; i++) {
                AxisValueTable *refav = &h->axisValues.array[i];
                if (refav->format3.axisTag == axisTags[0]
                    && refav->format3.value == values[0]
                    && refav->format3.linkedValue == minValue)
                    hotMsg(g, hotFATAL, "[STAT] AxisValueTable already defined "
                    "for axis \"%c%c%c%c\" with values %.2f %.2f\n",
                    TAG_ARG(axisTags[0]), FIX2DBL(values[0]),
                    FIX2DBL(minValue));
            }
            av->size = AXIS_VALUE_TABLE3_SIZE;
            av->format3.axisTag = axisTags[0];
            av->format3.flags = flags;
            av->format3.valueNameID = nameID;
            av->format3.value = values[0];
            av->format3.linkedValue = minValue;
            break;

        case 4:
            for (i = 0; i < h->axisValues.cnt; i++) {
                AxisValueTable *refav = &h->axisValues.array[i];
                bool *dupeAVT;
                dupeAVT = MEM_NEW(g, sizeof(bool) * count);
                for (j = 0; j < count; j++) {
                    dupeAVT[j] = false;
                }
                bool isDupe = true;
                if (refav->format4.axisCount == count) {
                    for (j = 0; j < count; j++) {
                        for (k = 0; k < count; k++) {
                            if (refav->format4.axisValues[j].axisTag == axisTags[k]
                                && refav->format4.axisValues[j].value == values[k]) {
                                    dupeAVT[j] = true;
                            }
                        }
                    }
                    for (j = 0; j < count; j++) {
                        if (!dupeAVT[j]) {
                            isDupe = false;
                            break;
                        }
                    }
                    if (isDupe) {
                        char *dupeMsg;
                        if (count <= 8) {
                            dupeMsg = MEM_NEW(g, sizeof(char) * count * 14 + (sizeof(char) * 54));
                            dupeMsg[0] = '\0';
                            sprintf(dupeMsg, "[STAT] AxisValueTable already defined with locations: ");
                            for (j = 0; j < count; j++) {
                                char axisMsg[20];
                                sprintf(axisMsg, "%c%c%c%c %.2f ", TAG_ARG(axisTags[j]), FIX2DBL(values[j]));
                                strcat(dupeMsg, axisMsg);
                            }
                        } else {
                            char baseMsg[] = "[STAT] AxisValueTable already defined with these %d locations.";
                            dupeMsg = MEM_NEW(g, sizeof(char) * (strlen(baseMsg) + count));
                            sprintf(dupeMsg, baseMsg, count);
                        }
                        hotMsg(g, hotFATAL, dupeMsg);
                        /* Shouldn't reach this MEM_FREE due to above hotFATAL, but just in case it gets changed to warning*/
                        MEM_FREE(g, dupeMsg);
                    }
                }
                MEM_FREE(g, dupeAVT);
            }
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
