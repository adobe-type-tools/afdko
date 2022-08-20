/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "MMSD.h"
#include "sfnt_MMSD.h"

static MMSDTbl *(MMSD) = NULL;
static int loaded = 0;

void MMSDRead(int32_t start, uint32_t length) {
    if (loaded)
        return;

    MMSD = (MMSDTbl *)sMemNew(sizeof(MMSDTbl));
    SEEK_ABS(start);

    loaded = 1;
}

void MMSDDump(int level, int32_t start) {
    DL(1, (stderr, "### [MMSD] (%08lx)\n", start));
}

void MMSDFree(void) {
    if (!loaded)
        return;
    sMemFree(MMSD);
    MMSD = NULL;
    loaded = 0;
}
