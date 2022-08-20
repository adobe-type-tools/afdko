/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "MMFX.h"
#include "sfnt_MMFX.h"
#include "CFF_.h"
#include "dump.h"
#include "sfnt.h"

static MMFXTbl *MMFX = NULL;
static int loaded = 0;
static uint32_t minoffset = MAX_UINT32;
static uint32_t maxoffset = 0;

void MMFXRead(int32_t start, uint32_t length) {
    int i;
    uint32_t lenstr;

    if (loaded)
        return;

    MMFX = (MMFXTbl *)sMemNew(sizeof(MMFXTbl));
    SEEK_ABS(start);

    IN1(MMFX->version);
    IN1(MMFX->nMetrics);
    IN1(MMFX->offSize);

    MMFX->offset = sMemNew(sizeof(int32_t) * (MMFX->nMetrics + 1));

    for (i = 0; i < MMFX->nMetrics; i++) {
        if (MMFX->offSize == 2) {
            int16_t tmp;
            IN1(tmp);
            MMFX->offset[i] = tmp;
        } else {
            IN1(MMFX->offset[i]);
        }
        if (MMFX->offset[i] < (int32_t)minoffset) minoffset = MMFX->offset[i];
        if (MMFX->offset[i] > (int32_t)maxoffset) maxoffset = MMFX->offset[i];
    }

    lenstr = (start + length) - TELL();
    MMFX->offset[MMFX->nMetrics] = lenstr + 1;
    MMFX->cstrs = sMemNew(sizeof(uint8_t) * (lenstr + 1));
    SEEK_ABS(start + minoffset);
    IN_BYTES(lenstr, MMFX->cstrs);
    loaded = 1;
}

void MMFXDump(int level, int32_t start) {
    int i, pos;
    int16_t tmp;
    uint8_t *ptr;
    uint16_t nMasters;

    DL(1, (OUTPUTBUFF, "### [MMFX] (%08lx)\n", start));

    DLV(2, "Version  =", MMFX->version);
    DLu(2, "nMetrics =", MMFX->nMetrics);
    DLu(2, "offSize  =", MMFX->offSize);

    DL(2, (OUTPUTBUFF, "--- offset[index]=offset\n"));
    if (MMFX->offSize == 2) {
        for (i = 0; i < MMFX->nMetrics; i++) {
            tmp = (int16_t)MMFX->offset[i];
            DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, tmp));
        }
        DL(2, (OUTPUTBUFF, "\n"));
    } else {
        for (i = 0; i < MMFX->nMetrics; i++)
            DL(2, (OUTPUTBUFF, "[%d]=%08x ", i, MMFX->offset[i]));
        DL(2, (OUTPUTBUFF, "\n"));
    }
    DL(2, (OUTPUTBUFF, "\n"));

    CFF_GetNMasters(&nMasters, MMFX_);

    DL(2, (OUTPUTBUFF, "--- cstring[index]=<charstring ops>\n"));
    for (i = 0; i < MMFX->nMetrics; i++) {
        pos = MMFX->offset[i] - minoffset;
        ptr = &(MMFX->cstrs[pos]);
        if (i < 8) {
            switch (i) {
                case 0:
                    DL(2, (OUTPUTBUFF, "[0=Zero]           = <"));
                    break;
                case 1:
                    DL(2, (OUTPUTBUFF, "[1=Ascender]       = <"));
                    break;
                case 2:
                    DL(2, (OUTPUTBUFF, "[2=Descender]      = <"));
                    break;
                case 3:
                    DL(2, (OUTPUTBUFF, "[3=LineGap]        = <"));
                    break;
                case 4:
                    DL(2, (OUTPUTBUFF, "[4=AdvanceWidthMax]= <"));
                    break;
                case 5:
                    DL(2, (OUTPUTBUFF, "[5=AvgCharWidth]   = <"));
                    break;
                case 6:
                    DL(2, (OUTPUTBUFF, "[6=xHeight]        = <"));
                    break;
                case 7:
                    DL(2, (OUTPUTBUFF, "[7=CapHeight]      = <"));
                    break;
            }
        } else {
            DL(2, (OUTPUTBUFF, "[%d]= <", i));
        }

        dump_csDump(MAX_INT32, ptr, nMasters);

        DL(2, (OUTPUTBUFF, ">\n"));
    }
    DL(2, (OUTPUTBUFF, "\n"));
}

void MMFXDumpMetric(uint32_t MetricID) {
    int pos;
    uint8_t *ptr;
    uint16_t nMasters;

    if (!loaded) {
        if (sfntReadTable(MMFX_))
            return;
    }
    if (MetricID >= MMFX->nMetrics)
        return;
    CFF_GetNMasters(&nMasters, MMFX_);
    pos = MMFX->offset[MetricID] - minoffset;
    ptr = &(MMFX->cstrs[pos]);

    dump_csDumpDerel(ptr, nMasters);
}

void MMFXFree(void) {
    if (!loaded)
        return;

    sMemFree(MMFX->offset);
    sMemFree(MMFX->cstrs);
    sMemFree(MMFX);
    MMFX = NULL;
    loaded = 0;
}
