/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "hdmx.h"
#include "sfnt_hdmx.h"
#include "maxp.h"

static hdmxTbl *hdmx = NULL;
static int loaded = 0;
static uint16_t nGlyphs;

void hdmxRead(int32_t start, uint32_t length) {
    int i;
    int32_t recordOffset = start + TBL_HDR_SIZE;

    if (loaded)
        return;

    hdmx = (hdmxTbl *)sMemNew(sizeof(hdmxTbl));

    if (maxpGetNGlyphs(&nGlyphs, hdmx_))
        return;

    SEEK_ABS(start);

    IN1(hdmx->version);
    IN1(hdmx->nRecords);
    IN1(hdmx->recordSize);

    hdmx->record = sMemNew(sizeof(DeviceRecord) * hdmx->nRecords);
    for (i = 0; i < hdmx->nRecords; i++) {
        int j;
        DeviceRecord *rec = &hdmx->record[i];

        IN1(rec->pixelsPerEm);
        IN1(rec->maxWidth);

        rec->widths = sMemNew(sizeof(rec->widths[0]) * nGlyphs);
        for (j = 0; j < nGlyphs; j++)
            IN1(rec->widths[j]);

        recordOffset += hdmx->recordSize;
        SEEK_ABS(recordOffset);
    }

    loaded = 1;
}

void hdmxDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [hdmx->] (%08lx)\n", start));

    DLu(2, "version   =", hdmx->version);
    DLu(2, "nRecords  =", hdmx->nRecords);
    DLU(2, "recordSize=", hdmx->recordSize);

    for (i = 0; i < hdmx->nRecords; i++) {
        int j;
        DeviceRecord *rec = &hdmx->record[i];

        DL(2, (OUTPUTBUFF, "--- device record[%d]\n", i));
        DLu(2, "pixelsPerEm=", (uint16_t)rec->pixelsPerEm);
        DLu(2, "maxWidth   =", (uint16_t)rec->maxWidth);

        DL(3, (OUTPUTBUFF, "--- widths[index]=value\n"));
        for (j = 0; j < nGlyphs; j++)
            DL(3, (OUTPUTBUFF, "[%d]=%u ", j, rec->widths[j]));
        DL(3, (OUTPUTBUFF, "\n"));
    }
}

void hdmxFree(void) {
    int i;

    if (!loaded)
        return;

    for (i = 0; i < hdmx->nRecords; i++)
        sMemFree(hdmx->record[i].widths);
    sMemFree(hdmx->record);
    sMemFree(hdmx);
    hdmx = NULL;
    loaded = 0;
}
