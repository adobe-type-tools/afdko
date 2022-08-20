/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "sfnt.h"
#include "sfnt_EBLC.h"
#include "EBLC.h"

static EBLCTableHeader *EBLC = NULL;
static int loaded = 0;

static void EBLCReadsbitLineMetrics(sbitLineMetrics *ptr) {
    IN1(ptr->ascender);
    IN1(ptr->descender);
    IN1(ptr->widthMax);
    IN1(ptr->caretSlopeNumerator);
    IN1(ptr->caretSlopeDenominator);
    IN1(ptr->caretOffset);
    IN1(ptr->minOriginSB);
    IN1(ptr->minAdvanceSB);
    IN1(ptr->maxBeforeBL);
    IN1(ptr->minAfterBL);
    IN1(ptr->padding);
}

static void EBLCReadBitmapSizeTables(void) {
    int i;
    EBLCBitmapSizeTable *tblptr;

    EBLC->bitmapSizeTable = (EBLCBitmapSizeTable *)sMemNew(EBLC->numSizes * sizeof(EBLCBitmapSizeTable));

    for (i = 0; i < (int)EBLC->numSizes; i++) {
        tblptr = &(EBLC->bitmapSizeTable[i]);
        IN1(tblptr->indexSubTableArrayOffset);
        IN1(tblptr->indexTableSize);
        IN1(tblptr->numberofIndexSubTables);
        IN1(tblptr->colorRef);

        EBLCReadsbitLineMetrics(&(tblptr->hori));
        EBLCReadsbitLineMetrics(&(tblptr->vert));
        IN1(tblptr->startGlyphIndex);
        IN1(tblptr->endGlyphIndex);
        IN1(tblptr->ppemX);
        IN1(tblptr->ppemY);
        IN1(tblptr->bitDepth);
        IN1(tblptr->flags);
    }
}

void EBLCRead(int32_t start, uint32_t length) {
    if (loaded)
        return;

    EBLC = (EBLCTableHeader *)sMemNew(sizeof(EBLCTableHeader));
    SEEK_ABS(start);

    IN1(EBLC->version);
    IN1(EBLC->numSizes);

    EBLCReadBitmapSizeTables();
    loaded = 1;
}

static void EBLCDumpsbitLineMetrics(sbitLineMetrics *ptr, int level) {
    DLs(2, "\tascender     =", (int16_t)ptr->ascender);
    DLs(2, "\tdescender    =", (uint16_t)ptr->descender);
    DLu(2, "\twidthMax     =", (int16_t)ptr->widthMax);
    DLs(2, "\tcaretNumer   =", (int16_t)ptr->caretSlopeNumerator);
    DLs(2, "\tcaretDenom   =", (int16_t)ptr->caretSlopeDenominator);
    DLs(2, "\tcaretOffset  =", (int16_t)ptr->caretOffset);
    DLs(2, "\tminOriginSB  =", (int16_t)ptr->minOriginSB);
    DLs(2, "\tminAdvanceSB =", (int16_t)(int16_t)ptr->minAdvanceSB);
    DLs(2, "\tmaxBeforeBL  =", (int16_t)ptr->maxBeforeBL);
    DLs(2, "\tminAfterBL   =", (int16_t)ptr->minAfterBL);
}

static void EBLCDumpBitmapSizeTables(int level) {
    int i;
    EBLCBitmapSizeTable *tblptr;

    DLU(2, "numSizes     =", EBLC->numSizes);

    for (i = 0; i < (int)EBLC->numSizes; i++) {
        tblptr = &(EBLC->bitmapSizeTable[i]);
        DL(2, (OUTPUTBUFF, "--- bitmapSizeTable[%d]\n", i));
        DL(2, (OUTPUTBUFF, "indexSubTableArrayOffset (%08x)\n", tblptr->indexSubTableArrayOffset));
        DLU(2, "indexSubTableArraySize =", tblptr->indexTableSize);
        DLU(2, "numberofIndexSubTables =", tblptr->numberofIndexSubTables);
        DLU(2, "colorRef =", tblptr->colorRef);

        DL(2, (OUTPUTBUFF, "  HorizontalLineMetrics:\n"));
        EBLCDumpsbitLineMetrics(&(tblptr->hori), level);
        DL(2, (OUTPUTBUFF, "  VerticalLineMetrics:\n"));
        EBLCDumpsbitLineMetrics(&(tblptr->vert), level);
        DLu(2, "startGlyphIndex =", tblptr->startGlyphIndex);
        DLu(2, "endGlyphIndex   =", tblptr->endGlyphIndex);
        DLu(2, "ppemX           =", (uint16_t)tblptr->ppemX);
        DLu(2, "ppemY           =", (uint16_t)tblptr->ppemY);
        DLu(2, "bitDepth        =", (uint16_t)tblptr->bitDepth);
        if (tblptr->flags & EBLC_FLAG_HORIZONTAL)
            DL(2, (OUTPUTBUFF, "flags           =HORIZONTAL\n"));
        else if (tblptr->flags & EBLC_FLAG_VERTICAL)
            DL(2, (OUTPUTBUFF, "flags           =VERTICAL\n"));
    }
}

void EBLCDump(int level, int32_t start) {
    DL(1, (OUTPUTBUFF, "### [EBLC] (%08lx)\n", start));

    DLV(2, "Version     =", EBLC->version);
    EBLCDumpBitmapSizeTables(level);
}

void EBLCFree(void) {
    if (!loaded) return;
    sMemFree(EBLC);
    EBLC = NULL;
    loaded = 0;
}
