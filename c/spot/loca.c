/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "loca.h"
#include "sfnt_loca.h"
#include "maxp.h"
#include "head.h"
#include "sfnt.h"

static locaTbl *loca = NULL;
static int loaded = 0;
static uint16_t nGlyphs;
static uint16_t locFormat;

static Format0 *readFormat0(void) {
    int i;
    Format0 *format = sMemNew(sizeof(Format0));

    format->offsets = sMemNew(sizeof(format->offsets[0]) * (nGlyphs + 1));
    for (i = 0; i < nGlyphs + 1; i++)
        IN1(format->offsets[i]);

    return format;
}

static Format1 *readFormat1(void) {
    int i;
    Format1 *format = sMemNew(sizeof(Format1));

    format->offsets = sMemNew(sizeof(format->offsets[0]) * (nGlyphs + 1));
    for (i = 0; i < nGlyphs + 1; i++)
        IN1(format->offsets[i]);

    return format;
}

void locaRead(int32_t start, uint32_t length) {
    if (loaded)
        return;

    loca = (locaTbl *)sMemNew(sizeof(locaTbl));

    if (maxpGetNGlyphs(&nGlyphs, loca_) ||
        headGetLocFormat(&locFormat, loca_))
        return;

    SEEK_ABS(start);

    switch (locFormat) {
        case 0:
            loca->format = readFormat0();
            break;
        case 1:
            loca->format = readFormat1();
            break;
        default:
            spotWarning(SPOT_MSG_locaBADFMT, locFormat);
            return;
    }

    loaded = 1;
}

static void dumpFormat0(Format0 *format, int level) {
    int i;

    DL(2, (OUTPUTBUFF, "--- offsets[index]=short (byte offset)\n"));
    for (i = 0; i < nGlyphs + 1; i++)
        DL(2, (OUTPUTBUFF, "[%d]=%04hx (%08x) ", i,
               format->offsets[i], format->offsets[i] * 2));
    DL(2, (OUTPUTBUFF, "\n"));
}

static void dumpFormat1(Format1 *format, int level) {
    int i;

    DL(2, (OUTPUTBUFF, "--- offsets[index]=long\n"));
    for (i = 0; i < nGlyphs + 1; i++)
        DL(2, (OUTPUTBUFF, "[%d]=%08x ", i, format->offsets[i]));
    DL(2, (OUTPUTBUFF, "\n"));
}

void locaDump(int level, int32_t start) {
    DL(1, (OUTPUTBUFF, "### [loca] (%08lx)\n", start));

    switch (locFormat) {
        case 0:
            dumpFormat0(loca->format, level);
            break;
        case 1:
            dumpFormat1(loca->format, level);
            break;
    }
}

void locaFree(void) {
    if (!loaded)
        return;

    switch (locFormat) {
        case 0:
            sMemFree(((Format0 *)loca->format)->offsets);
            sMemFree(loca->format);
            break;
        case 1:
            sMemFree(((Format1 *)loca->format)->offsets);
            sMemFree(loca->format);
            break;
    }

    sMemFree(loca);
    loca = NULL;
    loaded = 0;
}

int locaGetOffset(GlyphId glyphId, int32_t *offset, uint32_t *length,
                   uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(loca_))
            return tableMissing(loca_, client);
    }

    switch (locFormat) {
        case 0: {
            uint16_t *offsets = ((Format0 *)loca->format)->offsets;
            *offset = offsets[glyphId] * 2;
            *length = offsets[glyphId + 1] * 2 - *offset;
            break;
        }
        case 1: {
            uint32_t *offsets = ((Format1 *)loca->format)->offsets;
            *offset = offsets[glyphId];
            *length = offsets[glyphId + 1] - *offset;
            break;
        }
    }
    return 0;
}
