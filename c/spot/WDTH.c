/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "WDTH.h"
#include "sfnt_WDTH.h"

static WDTHTbl *WDTH = NULL;
static int loaded = 0;

void WDTHRead(int32_t start, uint32_t length) {
    int i;
    int size;
    int nElements;
    int nOffsets;

    if (loaded)
        return;
    WDTH = (WDTHTbl *)sMemNew(sizeof(WDTHTbl));

    SEEK_ABS(start);

    IN1(WDTH->version);
    IN1(WDTH->flags);
    IN1(WDTH->nMasters);
    IN1(WDTH->nRanges);

    size = (WDTH->flags & LONG_OFFSETS) ? sizeof(uint32_t) : sizeof(uint16_t);
    nElements = WDTH->nRanges + 1;

    /* Read first glyphs */
    WDTH->firstGlyph = sMemNew(sizeof(GlyphId) * nElements);
    for (i = 0; i < nElements; i++)
        IN1(WDTH->firstGlyph[i]);

    /* Read offsets */
    if (WDTH->flags & LONG_OFFSETS) {
        uint32_t *offset = WDTH->offset = sMemNew(sizeof(uint32_t) * nElements);
        for (i = 0; i < nElements; i++)
            IN1(offset[i]);
        nOffsets = offset[WDTH->nRanges] - offset[0];
    } else {
        uint16_t *offset = WDTH->offset = sMemNew(sizeof(uint16_t) * nElements);
        for (i = 0; i < nElements; i++)
            IN1(offset[i]);
        nOffsets = offset[WDTH->nRanges] - offset[0];
    }

    /* Read widths */
    WDTH->width = sMemNew(sizeof(uFWord) * WDTH->nMasters * nOffsets);
    for (i = 0; i < nOffsets; i++)
        IN1(WDTH->width[i]);

    loaded = 1;
}

void WDTHDump(int level, int32_t start) {
    int i;
    int j;
    int k;
    int iWidth;
    int nElements = WDTH->nRanges + 1;

    DL(1, (OUTPUTBUFF, "### [WDTH] (%08lx)\n", start));

    DLV(2, "version =", WDTH->version);
    DLu(2, "flags   =", WDTH->flags);
    DLu(2, "nMasters=", WDTH->nMasters);
    DLu(2, "nRanges =", WDTH->nRanges);

    DL(3, (OUTPUTBUFF, "--- firstGlyph[index]=glyphId\n"));
    for (i = 0; i < nElements; i++)
        DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, WDTH->firstGlyph[i]));
    DL(3, (OUTPUTBUFF, "\n"));

    DL(3, (OUTPUTBUFF, "--- offset[index]=offset\n"));
    if (WDTH->flags & LONG_OFFSETS)
        for (i = 0; i < nElements; i++)
            DL(3, (OUTPUTBUFF, "[%d]=%08x ", i, ((uint32_t *)WDTH->offset)[i]));
    else
        for (i = 0; i < nElements; i++)
            DL(3, (OUTPUTBUFF, "[%d]=%04hx ", i, ((uint16_t *)WDTH->offset)[i]));
    DL(3, (OUTPUTBUFF, "\n"));

    iWidth = 0;
    if (WDTH->nMasters == 1) {
        DL(3, (OUTPUTBUFF, "--- width[offset]=value\n"));
        if (WDTH->flags & LONG_OFFSETS) {
            uint32_t *offset = WDTH->offset;
            for (i = 0; i < WDTH->nRanges; i++) {
                int span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
                for (j = 0; j < span; j++)
                    DL(3, (OUTPUTBUFF, "[%08lx]=%hu ", offset[i] + j * sizeof(uFWord),
                           WDTH->width[iWidth++]));
            }
        } else {
            uint16_t *offset = WDTH->offset;
            for (i = 0; i < WDTH->nRanges; i++) {
                int span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
                for (j = 0; j < span; j++)
                    DL(3, (OUTPUTBUFF, "[%04lx]=%hu ", offset[i] + j * sizeof(uFWord),
                           WDTH->width[iWidth++]));
            }
        }
    } else {
        DL(3, (OUTPUTBUFF, "--- width[offset]={value+}\n"));
        if (WDTH->flags & LONG_OFFSETS) {
            uint32_t *offset = WDTH->offset;
            for (i = 0; i < WDTH->nRanges; i++) {
                int span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
                for (j = 0; j < span; j++) {
                    DL(3, (OUTPUTBUFF, "[%08lx]={", offset[i] + j * sizeof(uFWord)));
                    for (k = 0; k < WDTH->nMasters; k++)
                        DL(3, (OUTPUTBUFF, "%hu%s", WDTH->width[iWidth++],
                               k == WDTH->nMasters - 1 ? "} " : ","));
                }
            }
        } else {
            uint16_t *offset = WDTH->offset;
            for (i = 0; i < WDTH->nRanges; i++) {
                int span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
                for (j = 0; j < span; j++) {
                    DL(3, (OUTPUTBUFF, "[%04lx]={", offset[i] + j * sizeof(uFWord)));
                    for (k = 0; k < WDTH->nMasters; k++)
                        DL(3, (OUTPUTBUFF, "%hu%s", WDTH->width[iWidth++],
                               k == WDTH->nMasters - 1 ? "} " : ","));
                }
            }
        }
    }
    DL(3, (OUTPUTBUFF, "\n"));
}

void WDTHFree(void) {
    if (!loaded)
        return;

    sMemFree(WDTH->firstGlyph);
    sMemFree(WDTH->offset);
    sMemFree(WDTH->width);
    sMemFree(WDTH);
    WDTH = NULL;
    loaded = 0;
}
