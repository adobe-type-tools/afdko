/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)global.h	1.13
 * Changed:    4/7/98 11:09:43
 ***********************************************************************/

#ifndef	GLOBAL_H
#define	GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "numtypes.h"
#include "Dda.h"
#include "Dfile.h"

#include "sfnt_common.h"

#define DCL_ARRAY(type,name) type *name

/* Global data */
typedef struct
	{
	jmp_buf env;		/* Termination environment */
	char *progname;
	} Global;
extern Global global;

extern IntX level;
extern IntX DiffExists;

/* ### Constants */
#define MAX_PATH 1024

/* ### Macros */
#define TABLE_LEN(t) (sizeof(t)/sizeof((t)[0]))
#define STR2TAG(s) ((Card32)(s)[0]<<24|(Card32)(s)[1]<<16|(s)[2]<<8|(s)[3])
#define FIX2FLT(v) ((float)(v)/65536L)
#define DBL2F2Dot14(v) ((F2Dot14)((double)(v)*(1<<14)+((v)<0?-0.5:0.5)))
#define F2Dot142DBL(v) ((double)(v)/(1<<14))

/* Conditional dumping control
 *
 * DL	general purpose
 * DLT	tag
 * DLV	Apple-style version number
 * DLF	fixed point (16.16)
 * DLP  Pascal string
 * DLs	Int16
 * DLu	Card16
 * DLS	Int32
 * DLU	Card32
 * DLx  16 bit hexadecimal
 * DLX  32 bit hexadecimal
 */
#define DL(l,p) do{if(level>=(l)&&level<5)printf p;}while(0)
#define	DLT(l,s,v) DL(l,(s "%c%c%c%c\n",TAG_ARG(v)))
#define	DLV(l,s,v) DL(l,(s "%ld.%ld (%08lx)\n",VERSION_ARG(v)))
#define	DLF(l,s,v) DL(l,(s "%1.3f (%08lx)\n",FIXED_ARG(v)))
#define	DLP(l,s,v) DL(l,(s "{%lu,<%s>}\n",STR_ARG(v)))
#define	DLs(l,s,v) DL(l,(s "%hd\n",(v)))
#define	DLu(l,s,v) DL(l,(s "%hu\n",(v)))
#define	DLS(l,s,v) DL(l,(s "%ld\n",(v)))
#define	DLU(l,s,v) DL(l,(s "%lu\n",(v)))
#define	DLx(l,s,v) DL(l,(s "%04hx\n",(v)))
#define	DLX(l,s,v) DL(l,(s "%08lx\n",(v)))

#define DDL(l,p) do{if(level>=(l))printf p;}while(0)
#define	DDLT(l,s,v) DDL(l,(s "%c%c%c%c\n",TAG_ARG(v)))
#define	DDLV(l,s,v) DDL(l,(s "%ld.%ld (%08lx)\n",VERSION_ARG(v)))
#define	DDLF(l,s,v) DDL(l,(s "%1.3f (%08lx)\n",FIXED_ARG(v)))
#define	DDLP(l,s,v) DDL(l,(s "{%lu,<%s>}\n",STR_ARG(v)))
#define	DDLs(l,s,v) DDL(l,(s "%hd\n",(v)))
#define	DDLu(l,s,v) DDL(l,(s "%hu\n",(v)))
#define	DDLS(l,s,v) DDL(l,(s "%ld\n",(v)))
#define	DDLU(l,s,v) DDL(l,(s "%lu\n",(v)))
#define	DDLx(l,s,v) DDL(l,(s "%04hx\n",(v)))
#define	DDLX(l,s,v) DDL(l,(s "%08lx\n",(v)))

/* Convienence macros for dumping arguments */
#define TAG_ARG(t) (char)((t)>>24&0xff),(char)((t)>>16&0xff), \
	(char)((t)>>8&0xff),(char)((t)&0xff)
#define VERSION_ARG(v) (v)>>16&0xffff,(v)>>12&0xf,(v)
#define FIXED_ARG(f) FIX2FLT(f),(f)
#define STR_ARG(s) strlen((char *)s),(s)

/* ### Error reporting */
extern void fatal(Byte8 *fmt, ...);
extern void warning(Byte8 *fmt, ...);
extern void message(Byte8 *fmt, ...);
extern void note(Byte8 *fmt, ...);
extern IntX tableMissing(Card32 table, Card32 client);

/* ### Memory management */
extern void memError(void);
extern void *memNew(size_t size);
extern void *memResize(void *old, size_t size);
extern void memFree(void *ptr);

/* ### Miscellaneous */
typedef da_DCL(Card16, IdList);
extern IntX parseIdList(Byte8 *list, IdList *ids);
extern void quit(IntN status);

extern IntX getNGlyphs(Card8 which, Card16 *nGlyphs, Card32 client);
extern void initGlyphNames(Card8 which);
extern Byte8 *getGlyphName(Card8 which, GlyphId glyphId);
extern IntX isCID(Card8 which);
extern Byte8 *getFontName(Card8 which);
extern Byte8 *fileModTimeString(Card8 which, Byte8 *fname);
extern Byte8 *ourtime(void);

/* ### Missing prototypes */
#ifdef SUNOS
/* extern int _flsbuf(char c, FILE *p);
extern int sscanf(char *s, const char *format, ...);
extern int vfprintf( FILE *stream, const char *format, va_list arg); 
*/
#endif
/*
extern int _filbuf(FILE *p);
extern int printf(const char *format, ...);
extern int fclose(FILE *stream);
extern int fflush(FILE *stream);
extern long strtol(const char *str, char **ptr, int base);
#ifndef __MWERKS__
extern double strtod(const char *str, char **ptr);
#endif
extern int fprintf( FILE *stream, const char *format, ...);
*/
#endif /* GLOBAL_H */


