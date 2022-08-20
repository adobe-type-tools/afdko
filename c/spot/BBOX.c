/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "BBOX.h"
#include "sfnt_BBOX.h"

static BBOXTbl *BBOX = NULL;
static int loaded = 0;

void BBOXRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    BBOX = (BBOXTbl *)sMemNew(sizeof(BBOXTbl));

    SEEK_ABS(start);

    IN1(BBOX->version);
    IN1(BBOX->flags);
    IN1(BBOX->nGlyphs);
    IN1(BBOX->nMasters);

    BBOX->bbox = sMemNew(sizeof(BBox) * BBOX->nGlyphs);
    for (i = 0; i < BBOX->nGlyphs; i++) {
        int j;
        BBox *bbox = &BBOX->bbox[i];

        bbox->left = sMemNew(sizeof(FWord) * BBOX->nMasters);
        for (j = 0; j < BBOX->nMasters; j++)
            IN1(bbox->left[j]);

        bbox->bottom = sMemNew(sizeof(FWord) * BBOX->nMasters);
        for (j = 0; j < BBOX->nMasters; j++)
            IN1(bbox->bottom[j]);

        bbox->right = sMemNew(sizeof(FWord) * BBOX->nMasters);
        for (j = 0; j < BBOX->nMasters; j++)
            IN1(bbox->right[j]);

        bbox->top = sMemNew(sizeof(FWord) * BBOX->nMasters);
        for (j = 0; j < BBOX->nMasters; j++)
            IN1(bbox->top[j]);
    }

    loaded = 1;
}

void BBOXDump(int level, int32_t start) {
    int i;
    int j;

    DL(1, (OUTPUTBUFF, "### [BBOX] (%08lx)\n", start));

    DLV(2, "version =", BBOX->version);
    DLu(2, "flags   =", BBOX->flags);
    DLu(2, "nGlyphs =", BBOX->nGlyphs);
    DLu(2, "nMasters=", BBOX->nMasters);

    if (BBOX->nMasters == 1) {
        DL(3, (OUTPUTBUFF, "--- bbox[glyphId]={left,bottom,right,top}\n"));

        for (i = 0; i < BBOX->nGlyphs; i++) {
            BBox *bbox = &BBOX->bbox[i];
            DL(3, (OUTPUTBUFF, "[%d]={%hd,%hd,%hd,%hd} ", i,
                   bbox->left[0], bbox->bottom[0],
                   bbox->right[0], bbox->top[0]));
        }
        DL(3, (OUTPUTBUFF, "\n"));
    } else {
        DL(3, (OUTPUTBUFF, "--- bbox[glyphId]={{left+},{bottom+},{right+},{top+}}\n"));
        for (i = 0; i < BBOX->nGlyphs; i++) {
            BBox *bbox = &BBOX->bbox[i];
            DL(3, (OUTPUTBUFF, "[%d]={{", i));

            for (j = 0; j < BBOX->nMasters; j++)
                DL(3, (OUTPUTBUFF, "%hd%s", bbox->left[j],
                       j == BBOX->nMasters - 1 ? "},{" : ","));

            for (j = 0; j < BBOX->nMasters; j++)
                DL(3, (OUTPUTBUFF, "%hd%s", bbox->bottom[j],
                       j == BBOX->nMasters - 1 ? "},{" : ","));

            for (j = 0; j < BBOX->nMasters; j++)
                DL(3, (OUTPUTBUFF, "%hd%s", bbox->right[j],
                       j == BBOX->nMasters - 1 ? "},{" : ","));

            for (j = 0; j < BBOX->nMasters; j++)
                DL(3, (OUTPUTBUFF, "%hd%s", bbox->top[j],
                       j == BBOX->nMasters - 1 ? "}} " : ","));
        }
        DL(3, (OUTPUTBUFF, "\n"));
    }
}

void BBOXFree(void) {
    int i;

    if (!loaded)
        return;

    for (i = 0; i < BBOX->nMasters; i++) {
        BBox *bbox = &BBOX->bbox[i];

        sMemFree(bbox->left);
        sMemFree(bbox->bottom);
        sMemFree(bbox->right);
        sMemFree(bbox->top);
    }
    sMemFree(BBOX->bbox);
    sMemFree(BBOX);
    BBOX = NULL;
    loaded = 0;
}
