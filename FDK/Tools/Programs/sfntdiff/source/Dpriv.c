/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    
 * Changed:    
 ***********************************************************************/

/*
 * Miscellaneous functions and system calls.
 */

#include <stdarg.h> 
#include <string.h>
#include <ctype.h>

#include "Dglobal.h"
#include "Dopt.h"


/* ### Error reporting */

/* Print fatal message */
void fatal(char *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s [FATAL]: ", global.progname);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	quit(1);
	}

/* Print informational message */
void message(char *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s [MESSAGE]: ", global.progname);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

void note(char *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fflush(stdout);
	va_end(ap);
	}

/* Print warning message */
void warning(char *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s [WARNING]: ", global.progname);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}
