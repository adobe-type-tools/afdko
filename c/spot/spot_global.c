/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Miscellaneous functions and system calls.
 */

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "slogger.h"
#include "spot_global.h"
#include "opt.h"
#include "sfnt.h"
#include "maxp.h"
#include "TYP1.h"
#include "CID_.h"
#include "post.h"
#include "cmap.h"
#include "CFF_.h"
#include "glyf.h"
#include "head.h"
#include "name.h"
#include "map.h"
#if OSX
#include <sys/time.h>
#else
#include <time.h>
#endif

SpotGlobal spotGlobal; /* Global data */
static int nameLookupType = 0;
extern jmp_buf spot_mark;
char *dateFormat = "%a %b %d %H:%M:%S %Y";

/* ### Error reporting */

/* Print fatal message */
void spotFatal(int msgfmtID, ...) {
    va_list ap;

    fflush(OUTPUTBUFF);
    va_start(ap, msgfmtID);
    svLog(sFATAL, spotMsg(msgfmtID), ap);
    // Not reached
    va_end(ap);
    /* longjmp(spot_mark, -1); */
    exit(1);
}

/* Print informational message */
void spotMessage(int msgfmtID, ...) {
    va_list ap;

    fflush(OUTPUTBUFF);
    va_start(ap, msgfmtID);
    svLog(sINFO, spotMsg(msgfmtID), ap);
    va_end(ap);
}

/* Print warning message */
void spotWarning(int msgfmtID, ...) {
    va_list ap;

    fflush(OUTPUTBUFF);
    va_start(ap, msgfmtID);
    svLog(sWARNING, spotMsg(msgfmtID), ap);
    va_end(ap);
}

void featureWarning(int level, int msgfmtID, ...) {
    va_list ap;

    fflush(OUTPUTBUFF);
    va_start(ap, msgfmtID);
    svLog(sWARNING, spotMsg(msgfmtID), ap);
    va_end(ap);
}

/* Print Simple informational message */
void spotInform(int msgfmtID, ...) {
    va_list ap;

#ifndef EXECUTABLE
    if (opt_Present("-O"))
        return;
#endif
    va_start(ap, msgfmtID);
    svLog(sINFO, spotMsg(msgfmtID), ap);
    va_end(ap);
}

/* Warn of missing table */
int tableMissing(uint32_t table, uint32_t client) {
    spotWarning(SPOT_MSG_TABLEMISSING, TAG_ARG(client), TAG_ARG(table));
    return 1;
}

/* ### Miscellaneous */

/* Parse id list of the following forms: "N" the id N, and "N-M" the ids N-M */
int parseIdList(int8_t *list, IdList *ids) {
    int8_t *p;

    for (p = strtok(list, ","); p != NULL; p = strtok(NULL, ",")) {
        int one;
        int two;

        if (sscanf(p, "%d-%d", &one, &two) == 2)
            ;
        else if (sscanf(p, "%d", &one) == 1)
            two = one;
        else
            return 1;

        if (one < 0 || two < 0)
            return 1; /* Negative numbers aren't allowed */

        /* Add id range */
        if (one > two)
            while (one >= two)
                *da_NEXT(*ids) = one--;
        else
            while (one <= two)
                *da_NEXT(*ids) = one++;
    }
    return 0;
}

/* Get number of glyphs in font */
int getNGlyphs(uint16_t *nGlyphs, uint32_t client) {
    *nGlyphs = 0;
    if (nameLookupType == 0)
        initGlyphNames();

    if ((nameLookupType == 1) || (nameLookupType == 2))
        maxpGetNGlyphs(nGlyphs, client);
    else if (nameLookupType == 2)
        maxpGetNGlyphs(nGlyphs, client);
    else if (nameLookupType == 3)
        CFF_GetNGlyphs(nGlyphs, client);
    else if (nameLookupType == 4)
        TYP1GetNGlyphs(nGlyphs, client);
    else if (nameLookupType == 5)
        CID_GetNGlyphs(nGlyphs, client);
    else
        maxpGetNGlyphs(nGlyphs, client);
    if (!*nGlyphs)
        return 1;
    return 0;
}

void getMetrics(GlyphId glyphId,
                int *origShift,
                int *lsb, int *rsb, int *hwidth,
                int *tsb, int *bsb, int *vwidth, int *yorig) {
    /* check to see that glyphId is within range: */
    {
        uint16_t nglyphs;
        if (getNGlyphs(&nglyphs, TAG('g', 'l', 'o', 'b'))) /*maxpGetNGlyphs(&nglyphs, TAG('g','l','o','b')))*/
        {
            glyphId = 0;
        }
        if (glyphId >= nglyphs) {
            spotWarning(SPOT_MSG_GIDTOOLARGE, glyphId);
            glyphId = 0;
        }
    }

    /* try CFF first */
    CFF_getMetrics(glyphId,
                   origShift,
                   lsb, rsb, hwidth,
                   tsb, bsb, vwidth, yorig);

    if ((*origShift == 0) &&
        (*lsb == 0) && (*rsb == 0) &&
        (*hwidth == 0) && (*vwidth == 0) &&
        (*tsb == 0) && (*bsb == 0)) {
        glyfgetMetrics(glyphId,
                       origShift,
                       lsb, rsb, hwidth,
                       tsb, bsb, vwidth, yorig);
    }
}

int getFontBBox(int16_t *xMin, int16_t *yMin, int16_t *xMax, int16_t *yMax) {
    if (headGetBBox(xMin, yMin, xMax, yMax, TAG('g', 'l', 'o', 'b'))) {
        return CFF_GetBBox(xMin, yMin, xMax, yMax, TAG('g', 'l', 'o', 'b'));
    }
    return 0;
}

int isCID(void) {
    if (CID_isCID()) return 1;
    if (CFF_isCID()) return 1;
    return 0;
}

int8_t *getFontName(void) {
    if (isCID()) {
        return namePostScriptName();
    } else {
        return nameFontName();
    }
}

/* Quit processing */
void quit(int status) {
    exit(0);
    /* longjmp(spotGlobal.env, status+1); */
}

/* Initialize glyph name fetching */
void initGlyphNames(void) {
    if (CFF_InitName())
        nameLookupType = 3;
    else if (postInitName())
        nameLookupType = 1;
    else if (cmapInitName())
        nameLookupType = 2;
    else if (0 == sfntReadTable(TYP1_))
        nameLookupType = 4;
    else if (0 == sfntReadTable(CID__))
        nameLookupType = 5;
    else
        nameLookupType = 6;
}

/* Get glyph name. If unable to get a name return "@<gid>" string. Returns
   pointer to SINGLE static buffer so subsequent calls will overwrite. */
int8_t *getGlyphName(GlyphId glyphId, int forProofing) {
    static int8_t name[NAME_LEN + 1];
    static int8_t nicename[NAME_LEN + 7]; /* allow an extra 6 chars for GID. */
    int8_t *p;
    int length = 0;

    if (glyphId == 0xffff) {
        sprintf(name, "undefined");
        return name;
    }

    if (nameLookupType == 0) initGlyphNames();

    /* check to see that glyphId is within range: */
    {
        uint16_t nglyphs;
        if (getNGlyphs(&nglyphs, TAG('g', 'l', 'o', 'b'))) /*maxpGetNGlyphs(&nglyphs, TAG('g','l','o','b')))*/
        {
            sprintf(name, "@%hu", glyphId);
            return name;
        }
        if (glyphId >= nglyphs) {
            spotWarning(SPOT_MSG_GIDTOOLARGE, glyphId);
            glyphId = 0;
        }
    }

    if (nameLookupType == 1)
        p = postGetName(glyphId, &length);
    else if (nameLookupType == 2)
        p = cmapGetName(glyphId, &length);
    else if (nameLookupType == 3)
        p = CFF_GetName(glyphId, &length, forProofing);

    /* For names derived from the post table or from the cmap table, we add the
       GID as a string. This is because more than one glyph can end up with the
       same name under some circumstances. Also needed for nameLookupType == 4,
       where we can't get the name. */
    if (length == 0) {
        if (spotGlobal.flags & SUPPRESS_GID_IN_NAME)
            sprintf(name, "%hu", glyphId);
        else
            sprintf(name, "%hu@%hu", glyphId, glyphId);
    } else {
        if (!opt_Present("-m")) {
            if ((((nameLookupType == 1) || (nameLookupType == 2))) && (!(spotGlobal.flags & SUPPRESS_GID_IN_NAME)))
                sprintf(name, "%.*s@%hu", (length > (NAME_LEN - 6)) ? (NAME_LEN - 6) : length, p, glyphId);
            else
                sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
        } else { /* perform nice name mapping */
            const int8_t *p2 = NULL;
            int newlen = 0;

            sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
            if ((p2 = map_nicename(name, &newlen)) != NULL) {
                if (forProofing) {
#define INDICATOR_MARKER 0x27 /* tick mark */
                    sprintf(nicename, "%c%.*s", INDICATOR_MARKER,
                            (newlen > NAME_LEN) ? NAME_LEN : newlen, p2);
                } else {
                    sprintf(nicename, "%.*s",
                            (newlen > NAME_LEN) ? NAME_LEN : newlen, p2);
                }
                if ((((nameLookupType == 1) || (nameLookupType == 2))) && (!(spotGlobal.flags & SUPPRESS_GID_IN_NAME))) {
                    /* Don't need to shorten name is if is < NAME_LEN
                       when the GID is added; nicename has the space. */
                    char *p = &nicename[newlen];
                    sprintf(p, "@%hu", glyphId);
                }

                return nicename;
            } else {
                if ((((nameLookupType == 1) || (nameLookupType == 2))) && (!(spotGlobal.flags & SUPPRESS_GID_IN_NAME)))
                    sprintf(name, "%.*s@%hu", (length > (NAME_LEN - 6)) ? (NAME_LEN - 6) : length, p, glyphId);
                else
                    sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
            }
        }
    }

    return name;
#undef NAME_LEN
}

/* Dump all glyph names in font */
void dumpAllGlyphNames(int docrlfs) {
    int i;
    uint16_t nGlyphs;

    initGlyphNames();
    if (getNGlyphs(&nGlyphs, TAG('d', 'u', 'm', 'p'))) {
        spotWarning(SPOT_MSG_UNKNGLYPHS);
        return;
    }

    fprintf(OUTPUTBUFF, "--- names[glyphId]=<name>\n");
    for (i = 0; i < nGlyphs; i++) {
        fprintf(OUTPUTBUFF, "[%d]=<%s> ", i, getGlyphName(i, 0));
        if (docrlfs)
            fprintf(OUTPUTBUFF, "\n");
    }
    fprintf(OUTPUTBUFF, "\n");
}

char *spotOurtime(void) {
    static char ourtday[32];
    static int done = 0;

    if (!done) {
        time_t seconds_since_epoch;
        struct tm local_time;
        ourtday[0] = '\0';
        time(&seconds_since_epoch);
        SAFE_LOCALTIME(&seconds_since_epoch, &local_time);
        if (strftime(ourtday, sizeof(ourtday), dateFormat, &local_time) == 0) {
            spotFatal(SPOT_MSG_STRFTIME0);
        }
        ourtday[24] = '\0';
        done = 1;
    }
    return ourtday;
}
