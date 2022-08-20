/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <string.h>

#include "GLOB.h"
#include "sfnt_GLOB.h"
#include "sfnt.h"

static GLOBTbl *GLOB = NULL;
static int loaded = 0;

/* Allocate and read hybrid value */
static FWord *hybridRead(void) {
    int i;
    FWord *hybrid = sMemNew(sizeof(hybrid[0]) * GLOB->nMasters);

    for (i = 0; i < GLOB->nMasters; i++)
        IN1(hybrid[i]);
    return hybrid;
}

/* Read Pascal string */
static uint8_t *readString(void) {
    uint8_t length;
    uint8_t *new;

    IN1(length);
    new = sMemNew(length + 1);
    IN_BYTES(length, new);
    new[length] = '\0';

    return new;
}

void GLOBRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    GLOB = (GLOBTbl *)sMemNew(sizeof(GLOBTbl));

    SEEK_ABS(start);

    IN1(GLOB->version);
    IN1(GLOB->flags);
    IN1(GLOB->nMasters);

    for (i = 0; i < 6; i++)
        IN1(GLOB->matrix[i]);

    GLOB->italicAngle = sMemNew(GLOB->nMasters * sizeof(GLOB->italicAngle[0]));
    for (i = 0; i < GLOB->nMasters; i++)
        IN1(GLOB->italicAngle[i]);

    GLOB->bboxLeft = hybridRead();
    GLOB->bboxBottom = hybridRead();
    GLOB->bboxRight = hybridRead();
    GLOB->bboxTop = hybridRead();
    GLOB->capHeight = hybridRead();
    GLOB->xHeight = hybridRead();
    GLOB->underlinePosition = hybridRead();
    GLOB->underlineThickness = hybridRead();
    GLOB->dominantV = hybridRead();

    GLOB->avgWidth = hybridRead();
    GLOB->maxWidth = hybridRead();

    IN1(GLOB->defaultChar);
    IN1(GLOB->breakChar);
    IN1(GLOB->unitsPerEm);
    IN1(GLOB->macsfntId);
    IN1(GLOB->winMenuNameOffset);
    IN_BYTES(5, GLOB->winFileNamePrefix);
    GLOB->names = readString();

    loaded = 1;
}

/* Dump hybrid */
static void hybridDump(int8_t *name, FWord *value, int level) {
    if (GLOB->nMasters == 1)
        DL(2, (OUTPUTBUFF, "%s%hd\n", name, value[0]));
    else {
        int i;
        DL(2, (OUTPUTBUFF, "%s{", name));
        for (i = 0; i < GLOB->nMasters; i++)
            DL(2, (OUTPUTBUFF, "%hd%s", value[i], (i + 1 == GLOB->nMasters) ? "}\n" : ","));
    }
}

/* Dump array of Fixed */
static void FixedDump(int8_t *name, Fixed *value, int level) {
    if (GLOB->nMasters == 1)
        DL(2, (OUTPUTBUFF, "%s%1.3f (%08x)\n", name, FIXED_ARG(*value)));
    else {
        int i;
        DL(2, (OUTPUTBUFF, "%s{", name));
        for (i = 0; i < GLOB->nMasters; i++)
            DL(2, (OUTPUTBUFF, "%1.3f (%08x)%s", FIXED_ARG(value[i]),
                   (i + 1 == GLOB->nMasters) ? "}\n" : ","));
    }
}

void GLOBDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [GLOB] (%08lx)\n", start));

    DLV(2, "version           =", GLOB->version);
    DLx(2, "flags             =", GLOB->flags);
    DLu(2, "nMasters          =", GLOB->nMasters);

    DL(2, (OUTPUTBUFF, "--- matrix[index]=value\n"));
    for (i = 0; i < 6; i++)
        DL(2, (OUTPUTBUFF, "[%d]=%1.3f (%08x)\n", i, FIXED_ARG(GLOB->matrix[i])));

    FixedDump("italicAngle       =", GLOB->italicAngle, level);

    hybridDump("bboxLeft          =", GLOB->bboxLeft, level);
    hybridDump("bboxBottom        =", GLOB->bboxBottom, level);
    hybridDump("bboxRight         =", GLOB->bboxRight, level);
    hybridDump("bboxTop           =", GLOB->bboxTop, level);
    hybridDump("capHeight         =", GLOB->capHeight, level);
    hybridDump("xHeight           =", GLOB->xHeight, level);
    hybridDump("underlinePosition =", GLOB->underlinePosition, level);
    hybridDump("underlineThickness=", GLOB->underlineThickness, level);
    hybridDump("dominantV         =", GLOB->dominantV, level);

    hybridDump("avgWidth          =", GLOB->avgWidth, level);
    hybridDump("maxWidth          =", GLOB->maxWidth, level);

    DLu(2, "defaultChar       =", (uint16_t)(GLOB->defaultChar & 0xff));
    DLu(2, "breakChar         =", (uint16_t)(GLOB->breakChar & 0xff));
    DLu(2, "uintsPerEm        =", GLOB->unitsPerEm);
    DLu(2, "macsfntId         =", GLOB->macsfntId);
    DLx(2, "winMenuNameOffset =", GLOB->winMenuNameOffset);
    DL(2, (OUTPUTBUFF, "winFileNamePrefix =<%.5s>\n", GLOB->winFileNamePrefix));
    DL(2, (OUTPUTBUFF, "names[%04x]       ={%lu,<%s>}\n",
           GLOB->winMenuNameOffset, STR_ARG(GLOB->names)));
}

void GLOBUniBBox(FWord *bbLeft, FWord *bbBottom, FWord *bbRight, FWord *bbTop) {
    if (!loaded) {
        if (sfntReadTable(GLOB_)) {
            /* no such table */
            *bbLeft = 0;
            *bbBottom = 0;
            *bbRight = 0;
            *bbTop = 0;
            return;
        }
    }
    *bbLeft = GLOB->bboxLeft[0];
    *bbBottom = GLOB->bboxBottom[0];
    *bbRight = GLOB->bboxRight[0];
    *bbTop = GLOB->bboxTop[0];
}

void GLOBFree(void) {
    if (!loaded)
        return;

    sMemFree(GLOB->italicAngle);
    sMemFree(GLOB->bboxLeft);
    sMemFree(GLOB->bboxBottom);
    sMemFree(GLOB->bboxRight);
    sMemFree(GLOB->bboxTop);
    sMemFree(GLOB->capHeight);
    sMemFree(GLOB->xHeight);
    sMemFree(GLOB->underlinePosition);
    sMemFree(GLOB->underlineThickness);
    sMemFree(GLOB->dominantV);
    sMemFree(GLOB->avgWidth);
    sMemFree(GLOB->maxWidth);
    sMemFree(GLOB->names);
    sMemFree(GLOB);
    GLOB = NULL;
    loaded = 0;
}
