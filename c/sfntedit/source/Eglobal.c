/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)Eglobal.c	1.1
 * Changed:    2/12/99 13:36:15
 ***********************************************************************/

/*
 * Miscellaneous functions and system calls.
 */

#include <stdarg.h> 
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include "Eglobal.h"
#include "Efile.h"

/*extern jmp_buf mark; Not needed in exe */

Global global;			/* Global data */

extern char *tmpname;

/* ### Error reporting */

/* Print fatal message */
void fatal(int msgfmtID, ...)
	{
	const char *fmt;
	va_list ap;

	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [FATAL]: ", global.progname);
	fmt = sfntedMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	/*longjmp(mark, -1);*/

	if (fileExists(tmpname))
		{
		remove(tmpname);
		}
	exit(1);
	}

/* Print informational message */
void message(int msgfmtID, ...)
	{
	const char *fmt;
	va_list ap;

	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [MESSAGE]: ", global.progname);
	fmt = sfntedMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* Print warning message */
void warning(int msgfmtID, ...)
	{
	const char *fmt;
	va_list ap;

	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [WARNING]: ", global.progname);
	fmt = sfntedMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* Print Simple informational message */
void inform(int msgfmtID, ...)
	{
	const char *fmt;
	va_list ap;

	va_start(ap, msgfmtID);
	fmt = sfntedMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* ### Memory management */

/* Quit on memory error */
void memError(void)
	{
	fatal(SFED_MSG_NOMOREMEM);
	}

/* Allocate memory */
void *memNew(size_t size)
	{
	void *ptr;
	if (size == 0) size = 4;
	ptr = malloc(size);
	if (ptr == NULL)
		memError();
	return ptr;
	}
	
/* Resize allocated memory */
void *memResize(void *old, size_t size)
	{
	void *new = realloc(old, size);
	if (new == NULL)
		memError();
	return new;
	}

/* Free memory */
void memFree(void *ptr)
	{
	free(ptr);
	}


/* Quit processing */
void quit(int status)
	{
	exit(1);
	/* longjmp(global.env, status + 1); not used in exe */
	}

