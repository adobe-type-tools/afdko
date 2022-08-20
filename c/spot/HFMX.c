/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "HFMX.h"
#include "sfnt_HFMX.h"
#include "BLND.h"

static HFMXTbl *HFMX = NULL;
static int loaded = 0;
static int nMasters;

/* Allocate and read hybrid value */
static FWord *hybridRead(void) {
    int i;
    FWord *hybrid = sMemNew(sizeof(hybrid[0]) * nMasters);

    for (i = 0; i < nMasters; i++)
        IN1(hybrid[i]);
    return hybrid;
}

void HFMXRead(int32_t start, uint32_t length) {
    if (loaded)
        return;
    HFMX = (HFMXTbl *)sMemNew(sizeof(HFMXTbl));

    nMasters = BLNDGetNMasters();
    SEEK_ABS(start);

    IN1(HFMX->Version);

    HFMX->Ascent = hybridRead();
    HFMX->Descent = hybridRead();
    HFMX->LineGap = hybridRead();
    HFMX->CaretSlopeRise = hybridRead();
    HFMX->CaretSlopeRun = hybridRead();
    HFMX->CaretOffset = hybridRead();

    loaded = 1;
}

/* Dump hybrid */
static void hybridDump(int8_t *name, FWord *value, int level) {
    if (nMasters == 1)
        DL(2, (OUTPUTBUFF, "%s%hd\n", name, value[0]));
    else {
        int i;
        DL(2, (OUTPUTBUFF, "%s{", name));
        for (i = 0; i < nMasters; i++)
            DL(2, (OUTPUTBUFF, "%hd%s", value[i], (i + 1 == nMasters) ? "}\n" : ","));
    }
}

void HFMXDump(int level, int32_t start) {
    DL(1, (OUTPUTBUFF, "### [HFMX] (%08lx)\n", start));

    DLV(2, "Version           =", HFMX->Version);

    hybridDump("Ascent            =", HFMX->Ascent, level);
    hybridDump("Descent           =", HFMX->Descent, level);
    hybridDump("LineGap           =", HFMX->LineGap, level);
    hybridDump("CaretSlopeRise    =", HFMX->CaretSlopeRise, level);
    hybridDump("CaretSlopeRun     =", HFMX->CaretSlopeRun, level);
    hybridDump("CaretOffset       =", HFMX->CaretOffset, level);
}

void HFMXFree(void) {
    if (!loaded)
        return;

    sMemFree(HFMX->Ascent);
    sMemFree(HFMX->Descent);
    sMemFree(HFMX->LineGap);
    sMemFree(HFMX->CaretSlopeRise);
    sMemFree(HFMX->CaretSlopeRun);
    sMemFree(HFMX->CaretOffset);
    sMemFree(HFMX);
    HFMX = NULL;
    loaded = 0;
}
