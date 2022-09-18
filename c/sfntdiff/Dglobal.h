/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SFNTDIFF_DGLOBAL_H_
#define SFNTDIFF_DGLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>

#include "supportdefines.h"
#include "supportfp.h"
#include "da.h"
#include "Dfile.h"
#include "smem.h"

#include "sfnt_common.h"

#define DCL_ARRAY(type, name) type *name

#define ISPRINT(c) (isprint((c)) && isascii((c)) && (c) != 9)

extern int level;
extern int DiffExists;

/* ### Constants */
#define MAX_PATH 1024

/* ### Macros */
#define TABLE_LEN(t) (sizeof(t) / sizeof((t)[0]))
#define STR2TAG(s) ((uint32_t)(s)[0] << 24 | (uint32_t)(s)[1] << 16 | (s)[2] << 8 | (s)[3])
#define FIX2FLT(v) ((float)(v) / 65536L)
#define DBL2F2Dot14(v) ((F2Dot14)((double)(v) * (1 << 14) + ((v) < 0 ? -0.5 : 0.5)))
#define F2Dot142DBL(v) ((double)(v) / (1 << 14))

/* Conditional dumping control
 *
 * DL   general purpose
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
#define DL(l, p)                                 \
    do {                                         \
        if (level >= (l) && level < 5) printf p; \
    } while (0)
#define DLT(l, s, v) DL(l, (s "%c%c%c%c\n", TAG_ARG(v)))
#define DLV(l, s, v) DL(l, (s "%ld.%ld (%08lx)\n", VERSION_ARG(v)))
#define DLF(l, s, v) DL(l, (s "%1.3f (%08lx)\n", FIXED_ARG(v)))
#define DLP(l, s, v) DL(l, (s "{%lu,<%s>}\n", STR_ARG(v)))
#define DLs(l, s, v) DL(l, (s "%hd\n", (v)))
#define DLu(l, s, v) DL(l, (s "%hu\n", (v)))
#define DLS(l, s, v) DL(l, (s "%ld\n", (v)))
#define DLU(l, s, v) DL(l, (s "%lu\n", (v)))
#define DLx(l, s, v) DL(l, (s "%04hx\n", (v)))
#define DLX(l, s, v) DL(l, (s "%08lx\n", (v)))

#define DDL(l, p)                   \
    do {                            \
        if (level >= (l)) printf p; \
    } while (0)
#define DDLT(l, s, v) DDL(l, (s "%c%c%c%c\n", TAG_ARG(v)))
#define DDLV(l, s, v) DDL(l, (s "%ld.%ld (%08lx)\n", VERSION_ARG(v)))
#define DDLF(l, s, v) DDL(l, (s "%1.3f (%08lx)\n", FIXED_ARG(v)))
#define DDLP(l, s, v) DDL(l, (s "{%lu,<%s>}\n", STR_ARG(v)))
#define DDLs(l, s, v) DDL(l, (s "%hd\n", (v)))
#define DDLu(l, s, v) DDL(l, (s "%hu\n", (v)))
#define DDLS(l, s, v) DDL(l, (s "%ld\n", (v)))
#define DDLU(l, s, v) DDL(l, (s "%lu\n", (v)))
#define DDLx(l, s, v) DDL(l, (s "%04hx\n", (v)))
#define DDLX(l, s, v) DDL(l, (s "%08lx\n", (v)))

/* Convienence macros for dumping arguments */
#define TAG_ARG(t) (char)((t) >> 24 & 0xff), (char)((t) >> 16 & 0xff), \
                   (char)((t) >> 8 & 0xff), (char)((t)&0xff)
#define VERSION_ARG(v) (v) >> 16 & 0xffff, (v) >> 12 & 0xf, (v)
#define FIXED_ARG(f) FIX2FLT(f), (f)
#define STR_ARG(s) strlen((char *)s), (s)

/* ### Error reporting */
extern void sdFatal(char *fmt, ...);
extern void sdWarning(char *fmt, ...);
extern void sdMessage(char *fmt, ...);
extern void sdNote(char *fmt, ...);
extern int sdTableMissing(uint32_t table, uint32_t client);

/* ### Miscellaneous */
typedef da_DCL(uint16_t, IdList);

extern char *fileModTimeString(uint8_t which, const char *fname);
extern char *ourtime(void);

#endif  // SFNTDIFF_DGLOBAL_H_
