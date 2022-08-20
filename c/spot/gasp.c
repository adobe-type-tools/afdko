/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "gasp.h"
#include "sfnt_gasp.h"

static gaspTbl *gasp = NULL;
static int loaded = 0;

void gaspRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    gasp = (gaspTbl *)sMemNew(sizeof(gaspTbl));
    SEEK_ABS(start);

    IN1(gasp->version);
    IN1(gasp->numRanges);
    gasp->gaspRange = sMemNew(sizeof(GaspRange) * gasp->numRanges);
    for (i = 0; i < gasp->numRanges; i++) {
        GaspRange *range = &gasp->gaspRange[i];

        IN1(range->rangeMaxPPEM);
        IN1(range->rangeGaspBehavior);
    }

    loaded = 1;
}

void gaspDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [gasp] (%08lx)\n", start));

    DLu(2, "version=", gasp->version);
    DLu(2, "numRanges=", gasp->numRanges);

    DL(2, (OUTPUTBUFF, "--- gaspRange[index]={rangeMaxPPEM,rangeGaspBehavior}\n"));
    for (i = 0; i < gasp->numRanges; i++) {
        int8_t *p;
        GaspRange *range = &gasp->gaspRange[i];
        uint16_t behavior = range->rangeGaspBehavior;

        DL(2, (OUTPUTBUFF, "[%d]={%hu,%04hx", i, range->rangeMaxPPEM, behavior));
        switch (behavior) {
            case 0x0001:
                p = "[GRIDFIT]";
                break;
            case 0x0002:
                p = "[DOGRAY]";
                break;
            case 0x0003:
                p = "[DOGRAY|GRIDFIT]";
                break;
            default:
                p = "";
                break;
        }
        DL(2, (OUTPUTBUFF, " %s} ", p));
    }
    DL(2, (OUTPUTBUFF, "\n"));
}

void gaspFree(void) {
    if (!loaded)
        return;

    sMemFree(gasp->gaspRange);
    sMemFree(gasp);
    gasp = NULL;
    loaded = 0;
}
