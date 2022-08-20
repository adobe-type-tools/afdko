/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <string.h>

#include "SING.h"
#include "sfnt_SING.h"
#include "sfnt.h"

static SINGTbl *SING = NULL;
static int loaded = 0;

/* Read Pascal string */
static uint8_t *readString(void) {
    uint8_t length;
    uint8_t *newstr;

    IN1(length);
    newstr = sMemNew(length + 1);
    IN_BYTES(length, newstr);
    newstr[length] = '\0';

    return newstr;
}

void SINGRead(int32_t start, uint32_t length) {
    if (loaded)
        return;

    SING = (SINGTbl *)sMemNew(sizeof(SINGTbl));

    SEEK_ABS(start);

    IN1(SING->tableVersionMajor);
    IN1(SING->tableVersionMinor);
    IN1(SING->glyphletVersion);
    IN1(SING->permissions);
    IN1(SING->mainGID);
    IN1(SING->unitsPerEm);
    IN1(SING->vertAdvance);
    IN1(SING->vertOrigin);

    IN_BYTES(SING_UNIQUENAMELEN, SING->uniqueName);
    IN_BYTES(SING_MD5LEN, SING->METAMD5);
    IN1(SING->nameLength);
    SING->baseGlyphName = (uint8_t *)sMemNew(SING->nameLength + 1);
    IN_BYTES(SING->nameLength, SING->baseGlyphName);
    SING->baseGlyphName[SING->nameLength] = '\0';
    loaded = 1;
}

int SINGGetUnitsPerEm(uint16_t *unitsPerEm, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(SING_)) {
            *unitsPerEm = 1000;
            return tableMissing(SING_, client);
        }
    }
    *unitsPerEm = SING->unitsPerEm;
    return 0;
}

#define VERSION88_ARG(v) (v) >> 8 & 0xff, (v)&0xff, (v)

void SINGDump(int level, int32_t start) {
    int i;
    char nam[MAX_NAME_LEN];
    DL(1, (OUTPUTBUFF, "### [SING] (%08lx)\n", start));

    DLu(2, "tableVersionMajor =", SING->tableVersionMajor);
    DLu(2, "tableVersionMinor =", SING->tableVersionMinor);
    DLu(2, "glyphletVersion   =", SING->glyphletVersion);
    DLu(2, "permissions       =", SING->permissions);
    DLu(2, "mainGID           =", SING->mainGID);
    DLu(2, "unitsPerEm        =", SING->unitsPerEm);
    DLs(2, "vertAdvance       =", SING->vertAdvance);
    DLs(2, "vertOrigin        =", SING->vertOrigin);
    strncpy(nam, (const char *)(SING->uniqueName), SING_UNIQUENAMELEN);
    nam[SING_UNIQUENAMELEN] = '\0';
    DL(2, (OUTPUTBUFF,
           "uniqueName        =<%s>\n", nam));
    DL(2, (OUTPUTBUFF,
           "MD5 signature of META table = {"));
    for (i = 0; i < SING_MD5LEN; i++)
        DL(2, (OUTPUTBUFF, "%02X%s", (SING->METAMD5[i]),
               (i + 1 == SING_MD5LEN) ? "}\n" : ","));
    DLu(2, "nameLength        =", (uint16_t)SING->nameLength);
    strncpy(nam, (const char *)(SING->baseGlyphName), SING->nameLength);
    nam[SING->nameLength] = '\0';
    DL(2, (OUTPUTBUFF,
           "baseGlyphName     =<%s>\n", nam));
}

void SINGFree(void) {
    if (!loaded)
        return;

    sMemFree(SING->baseGlyphName);
    sMemFree(SING);
    SING = NULL;
    loaded = 0;
}
