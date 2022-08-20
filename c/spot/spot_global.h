/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SPOT_GLOBAL_H
#define SPOT_GLOBAL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "numtypes.h"
#include "da.h"
#include "file.h"
#include "smem.h"
#include "spot.h"
#include "spotmsgs.h"
#include "safetime.h"

#include "sfnt_common.h"
#define DCL_ARRAY(type, name) type *name

/* Global data */
typedef struct _SpotGlobal {
    jmp_buf env; /* Termination environment */
    char *progname;
    volatile int doingScripting;
    unsigned long gTag;
    unsigned long flags;
} SpotGlobal;
extern SpotGlobal spotGlobal;

extern char *dateFormat;

/* Global flags */
#define SUPPRESS_GID_IN_NAME (1 << 0) /* Suppress the terminal gid on TTF glyph names. */

/* ### Constants */
#define DEFAULT_YORIG_KANJI 880

/* ### Macros */
#define TABLE_LEN(t) (sizeof(t) / sizeof((t)[0]))
#define STR2TAG(s) ((uint32_t)(s)[0] << 24 | (uint32_t)(s)[1] << 16 | (s)[2] << 8 | (s)[3])
#define FIX2FLT(v) ((float)(v) / 65536L)
#define DBL2F2Dot14(v) ((F2Dot14)((double)(v) * (1 << 14) + ((v) < 0 ? -0.5 : 0.5)))
#define F2Dot142DBL(v) ((double)(v) / (1 << 14))

/* Conditional dumping control
 *
 * DL general purpose
 * DLT  tag
 * DLV  Apple-style version number
 * DLF  fixed point (16.16)
 * DLP  Pascal string
 * DLs  int16_t
 * DLu  uint16_t
 * DLS  int32_t
 * DLU  uint32_t
 * DLx  16 bit hexadecimal
 * DLX  32 bit hexadecimal
 */
#define DL5(p)                     \
    do {                           \
        if (level == 5) fprintf p; \
    } while (0)
#define DL(l, p)                                  \
    do {                                          \
        if (level >= (l) && level < 5) fprintf p; \
    } while (0)
#define DLT(l, s, v) DL(l, (OUTPUTBUFF, s "%c%c%c%c\n", TAG_ARG(v)))
#define DLV(l, s, v) DL(l, (OUTPUTBUFF, s "%d.%d (%08x)\n", VERSION_ARG(v)))
#define DLF(l, s, v) DL(l, (OUTPUTBUFF, s "%.3f (%08x)\n", FIXED_ARG(v)))
#define DLP(l, s, v) DL(l, (OUTPUTBUFF, s "{%lu,<%s>}\n", STR_ARG(v)))
#define DLs(l, s, v) DL(l, (OUTPUTBUFF, s "%hd\n", (v)))
#define DLu(l, s, v) DL(l, (OUTPUTBUFF, s "%hu\n", (v)))
#define DLS(l, s, v) DL(l, (OUTPUTBUFF, s "%d\n", (v)))
#define DLU(l, s, v) DL(l, (OUTPUTBUFF, s "%u\n", (v)))
#define DLx(l, s, v) DL(l, (OUTPUTBUFF, s "%04hx\n", (v)))
#define DLX(l, s, v) DL(l, (OUTPUTBUFF, s "%08x\n", (v)))

#define DDL(l, p)                                       \
    do {                                                \
        if (level >= (l)) {                             \
            int ret = fprintf p;                        \
            if (ret < 0) spotWarning(SPOT_MSG_GPOS_DUPKRN); \
        }                                               \
    } while (0)
#define DDLT(l, s, v) DDL(l, (OUTPUTBUFF, s "%c%c%c%c", TAG_ARG(v)))
#define DDLV(l, s, v) DDL(l, (OUTPUTBUFF, s "%d.%d (%08x)", VERSION_ARG(v)))
#define DDLF(l, s, v) DDL(l, (OUTPUTBUFF, s "%1.3f (%08x)", FIXED_ARG(v)))
#define DDLP(l, s, v) DDL(l, (OUTPUTBUFF, s "{%lu,<%s>}", STR_ARG(v)))
#define DDLs(l, s, v) DDL(l, (OUTPUTBUFF, s "%hd", (v)))
#define DDLu(l, s, v) DDL(l, (OUTPUTBUFF, s "%hu", (v)))
#define DDLS(l, s, v) DDL(l, (OUTPUTBUFF, s "%d", (v)))
#define DDLU(l, s, v) DDL(l, (OUTPUTBUFF, s "%u", (v)))
#define DDLx(l, s, v) DDL(l, (OUTPUTBUFF, s "%04hx", (v)))
#define DDLX(l, s, v) DDL(l, (OUTPUTBUFF, s "%08x", (v)))

/* Convenience macros for dumping arguments */
#define TAG_ARG(t) (char)((t) >> 24 & 0xff), (char)((t) >> 16 & 0xff), \
                   (char)((t) >> 8 & 0xff), (char)((t)&0xff)
#define VERSION_ARG(v) (v) >> 16 & 0xffff, (v) >> 12 & 0xf, (v)
#define FIXED_ARG(f) FIX2FLT(f), (f)
#define STR_ARG(s) strlen((char *)s), (s)

/* ### Error reporting */
extern void spotFatal(int msgfmtID, ...);
extern void spotWarning(int msgfmtID, ...);
extern void featureWarning(int level, int msgfmtID, ...);
extern void spotMessage(int msgfmtID, ...);
extern int tableMissing(uint32_t table, uint32_t client);
extern void spotInform(int msgfmtID, ...);

/* ### Miscellaneous */
typedef da_DCL(uint16_t, IdList);
extern int parseIdList(int8_t *list, IdList *ids);
extern int getNGlyphs(uint16_t *nGlyphs, uint32_t client);
extern void quit(int status);
extern void initGlyphNames(void);
#define NAME_LEN 128              /*  base glyph name length returned by getGlyphName */
#define MAX_NAME_LEN NAME_LEN + 7 /* max glyph name length returned by getGlyphName */
extern int8_t *getGlyphName(GlyphId glyphId, int forProofing);
extern void dumpAllGlyphNames(int docrlfs);
extern void getMetrics(GlyphId glyphId,
                       int *origShift,
                       int *lsb, int *rsb, int *hwidth,
                       int *tsb, int *bsb, int *vwidth, int *yorig);
extern int getFontBBox(int16_t *xMin, int16_t *yMin, int16_t *xMax, int16_t *yMax);
extern int isCID(void);
extern int8_t *getFontName(void);
extern char *spotOurtime(void);

/* ### Missing prototypes */
/*
extern int fclose(FILE *stream);
extern int fflush(FILE *stream);
extern long strtol(const char *str, char **ptr, int base);
#ifndef __MWERKS__
extern double strtod(const char *str, char **ptr);
#endif
extern int fprintf(FILE *stream, const char *format, ...);
*/

#endif /* SPOT_GLOBAL_H */
