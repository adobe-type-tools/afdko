/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "LTSH.h"
#include "sfnt_LTSH.h"

static LTSHTbl *LTSH = NULL;
static int loaded = 0;

void LTSHRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    LTSH = (LTSHTbl *)sMemNew(sizeof(LTSHTbl));

    SEEK_ABS(start);

    IN1(LTSH->version);
    IN1(LTSH->numGlyphs);

    LTSH->yPels = sMemNew(sizeof(LTSH->yPels[0]) * LTSH->numGlyphs);
    for (i = 0; i < LTSH->numGlyphs; i++)
        IN1(LTSH->yPels[i]);

    loaded = 1;
}

void LTSHDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [LTSH] (%08lx)\n", start));

    DLu(2, "version  =", LTSH->version);
    DLu(2, "numGlyphs=", LTSH->numGlyphs);

    DL(2, (OUTPUTBUFF, "--- yPels[index]=value\n"));
    for (i = 0; i < LTSH->numGlyphs; i++)
        DL(2, (OUTPUTBUFF, "[%d]=%u ", i, LTSH->yPels[i]));
    DL(2, (OUTPUTBUFF, "\n"));
}

void LTSHFree(void) {
    if (!loaded)
        return;

    sMemFree(LTSH->yPels);
    sMemFree(LTSH);
    LTSH = NULL;
    loaded = 0;
}
