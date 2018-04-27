/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)Eglobal.h	1.1
 * Changed:    2/12/99 13:36:16
 ***********************************************************************/

#ifndef	GLOBAL_H
#define	GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "supportpublictypes.h"
#undef global

#include "Efile.h"
#include "Emsgs.h"
#include "Eda.h"

/* Global data */
typedef struct
	{
	jmp_buf env;		/* Termination environment */
	char *progname;
	} Global;
extern Global global;

/* ### Constants */
#define MAX_PATH 1024

#define STR2TAG(s) ((Card32)(s)[0]<<24|(Card32)(s)[1]<<16|(s)[2]<<8|(s)[3])

/* ### Error reporting */
extern void fatal(int msgfmtID, ...);
extern void warning(int msgfmtID, ...);
extern void message(int msgfmtID, ...);
extern void inform(int msgfmtID, ...);

/* ### Memory management */
extern void memError(void);
extern void *memNew(size_t size);
extern void *memResize(void *old, size_t size);
extern void memFree(void *ptr);

extern void quit(int status);

/* ### Missing prototypes */
#ifdef SUNOS
/* extern int _flsbuf(char c, FILE *p);
extern int sscanf(char *s, const char *format, ...);
extern int vfprintf(FILE *stream, const char *format, va_list arg); 
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
extern int fprintf(FILE *stream, const char *format, ...);
*/

#endif /* GLOBAL_H */


