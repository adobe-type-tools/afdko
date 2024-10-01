/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#if OSX
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <string.h>
#include "head.h"
#include "sfnt_head.h"
#include "sfnt.h"
#include "SING.h"

static headTbl *head = NULL;
static int loaded = 0;

void headRead(int32_t start, uint32_t length) {
    if (loaded)
        return;

    head = (headTbl *)sMemNew(sizeof(headTbl));

    SEEK_ABS(start);

    IN1(head->version);
    IN1(head->fontRevision);
    IN1(head->checkSumAdjustment);
    IN1(head->magicNumber);
    IN1(head->flags);
    IN1(head->unitsPerEm);
    IN_BYTES(sizeof(head->created), head->created);
    IN_BYTES(sizeof(head->modified), head->modified);
    IN1(head->xMin);
    IN1(head->yMin);
    IN1(head->xMax);
    IN1(head->yMax);
    IN1(head->macStyle);
    IN1(head->lowestRecPPEM);
    IN1(head->fontDirectionHint);
    IN1(head->indexToLocFormat);
    IN1(head->glyphDataFormat);

    loaded = 1;
}

#if FOR_REFERENCE_ONLY
/* Convert ANSI standard date/time format to Apple LongDateTime format.
   Algorithm adapted from standard Julian Day calculation. */
void CTUANSITime2LongDateTime(struct tm *ansi, CTULongDateTime ldt) {
    unsigned long elapsed;
    int month = ansi->tm_mon + 1;
    int year = ansi->tm_year;

    if (month < 3) {
        /* Jan and Feb become months 13 and 14 of the previous year */
        month += 12;
        year--;
    }

    /* Calculate elapsed seconds since 1 Jan 1904 00:00:00 */
    elapsed = (((year - 4L) * 365 +             /* Add years (as days) */
                year / 4 +                      /* Add leap year days */
                (306 * (month + 1)) / 10 - 64 + /* Add month days */
                ansi->tm_mday) *                /* Add days */
                   24 * 60 * 60 +               /* Convert days to secs */
               ansi->tm_hour * 60 * 60 +        /* Add hours (as secs) */
               ansi->tm_min * 60 +              /* Add minutes (as secs) */
               ansi->tm_sec);                   /* Add seconds */

    /* Set value */
    ldt[0] = ldt[1] = ldt[2] = ldt[3] = 0;
    ldt[4] = (char)(elapsed >> 24);
    ldt[5] = (char)(elapsed >> 16);
    ldt[6] = (char)(elapsed >> 8);
    ldt[7] = (char)elapsed;
}

#endif

/* Convert Apple LongDateTime format to ANSI standard date/time format.
   Algorithm adapted from standard Julian Day calculation. */
static void LongDateTime2ANSITime(struct tm *ansitime, longDateTime ldt) {
    unsigned long elapsed = ((unsigned long)ldt[4] << 24 |
                             (unsigned long)ldt[5] << 16 |
                             ldt[6] << 8 |
                             ldt[7]);
    long A = elapsed / (24 * 60 * 60L);
    long B = A + 1524;
    long C = (long)((B - 122.1) / 365.25);
    long D = (long)(C * 365.25);
    long E = (long)((B - D) / 30.6001);
    long F = (long)(E * 30.6001);
    long secs = elapsed - A * (24 * 60 * 60L);

    if (E > 13) {
        ansitime->tm_year = C + 1;
        ansitime->tm_mon = E - 14;
        ansitime->tm_yday = B - D - 429;
    } else {
        ansitime->tm_year = C;
        ansitime->tm_mon = E - 2;
        ansitime->tm_yday = B - D - 64;
    }
    ansitime->tm_mday = B - D - F;
    ansitime->tm_hour = secs / (60 * 60);
    secs -= ansitime->tm_hour * (60 * 60);
    ansitime->tm_min = secs / 60;
    ansitime->tm_sec = secs - ansitime->tm_min * 60;
    ansitime->tm_wday = (A + 5) % 7;
    ansitime->tm_isdst = 0;
#if SUNOS
    ansitime->tm_zone = "GMT";
#endif
}

static char tday[32];

char *headGetCreatedDate(uint32_t client) {
    struct tm tmp;

    tday[0] = '\0';
    if (!loaded) {
        if (sfntReadTable(head_)) {
            tableMissing(head_, client);
            return tday;
        }
    }
    LongDateTime2ANSITime(&tmp, head->created);
    if (strftime(tday, sizeof(tday), dateFormat, &tmp) == 0) {
        spotFatal(SPOT_MSG_STRFTIME0);
    }
    tday[24] = '\0';
    return tday;
}

char *headGetModifiedDate(uint32_t client) {
    struct tm tmp;

    tday[0] = '\0';
    if (!loaded) {
        if (sfntReadTable(head_)) {
            tableMissing(head_, client);
            return tday;
        }
    }
    LongDateTime2ANSITime(&tmp, head->modified);
    if (strftime(tday, sizeof(tday), dateFormat, &tmp) == 0) {
        spotFatal(SPOT_MSG_STRFTIME0);
    }
    tday[24] = '\0';
    return tday;
}

void headDump(int level, int32_t start) {
    DL(1, (OUTPUTBUFF, "### [head] (%08lx)\n", start));

    DLV(2, "version           =", head->version);
    DLF(2, "fontRevision      =", head->fontRevision);
    DLX(2, "checkSumAdjustment=", head->checkSumAdjustment);
    DLX(2, "magicNumber       =", head->magicNumber);
    DLx(2, "flags             =", head->flags);
    DLu(2, "unitsPerEm        =", head->unitsPerEm);
    DL(2, (OUTPUTBUFF, "created           =%x%x%x%x%x%x%x%x (%s)\n",
           head->created[0], head->created[1],
           head->created[2], head->created[3],
           head->created[4], head->created[5],
           head->created[6], head->created[7], headGetCreatedDate(head_)));
    DL(2, (OUTPUTBUFF, "modified          =%x%x%x%x%x%x%x%x (%s)\n",
           head->modified[0], head->modified[1],
           head->modified[2], head->modified[3],
           head->modified[4], head->modified[5],
           head->modified[6], head->modified[7], headGetModifiedDate(head_)));
    DLs(2, "xMin              =", head->xMin);
    DLs(2, "yMin              =", head->yMin);
    DLs(2, "xMax              =", head->xMax);
    DLs(2, "yMax              =", head->yMax);
    DLx(2, "macStyle          =", head->macStyle);
    DLu(2, "lowestRecPPEM     =", head->lowestRecPPEM);
    DLs(2, "fontDirectionHint =", head->fontDirectionHint);
    DLs(2, "indexToLocFormat  =", head->indexToLocFormat);
    DLs(2, "glyphDataFormat   =", head->glyphDataFormat);
}

void headFree_spot(void) {
    if (!loaded) return;
    sMemFree(head);
    head = NULL;
    loaded = 0;
}

/* Return head->indexToLocFormat */
int headGetLocFormat(uint16_t *locFormat, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_))
            return tableMissing(head_, client);
    }
    *locFormat = head->indexToLocFormat;
    return 0;
}

/* Return head->uintsPerEm */
int headGetUnitsPerEm(uint16_t *unitsPerEm, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_)) {
            if (SINGGetUnitsPerEm(unitsPerEm, client)) {
                *unitsPerEm = 1000; /* default */
            }
            return tableMissing(head_, client);
        }
    }
    *unitsPerEm = head->unitsPerEm;
    return 0;
}

/* Return head->flags & head_SET_LSB */
int headGetSetLsb(uint16_t *setLsb, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_))
            return tableMissing(head_, client);
    }
    *setLsb = (head->flags & head_SET_LSB) != 0;
    return 0;
}

/* Return head->xMin, head->yMin, head->xMax, head->yMax */
int headGetBBox(int16_t *xMin, int16_t *yMin, int16_t *xMax, int16_t *yMax,
                 uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_)) {
            *xMin = 0;
            *yMin = 0;
            *xMax = 0;
            *yMax = 0;
            return 1;
        }
    }
    *xMin = head->xMin;
    *yMin = head->yMin;
    *xMax = head->xMax;
    *yMax = head->yMax;
    return 0;
}

int headGetFontRevision(float *rev, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_)) {
            *rev = 0;
            return 1;
        }
    }
    *rev = FIX2FLT(head->fontRevision);
    return 0;
}

int headGetVersion(float *ver, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(head_)) {
            *ver = 0;
            return 1;
        }
    }
    *ver = FIX2FLT(head->version);
    return 0;
}
