/* @(#)CM_VerSion lstdio.h atm07 1.2 16164.eco sum= 11728 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Veneer layer for stdio.h */

#ifndef LSTDIO_H
#define LSTDIO_H

#if WITHIN_PS

/* PostScript environment */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PROTOS
#include PUBLICTYPES
#include STREAM

#define FILE StmRec
#define fprintf os_fprintf
#define vfprintf os_fprintf		/* xxx this could be a problem */
#define stdout os_stdout
#define stdin os_stdin
#define stderr os_stderr
#define fopen os_fopen
#define fgets os_fgets
#define fseek os_fseek

#else /* WITHIN_PS */

/* ANSI C environment */
#include <stdio.h>

#if SUNOS

#include <sys/unistd.h>	/* For SEEK_* macros */

#ifndef FILENAME_MAX
/* SunOS doesn't define this ANSI macro anywhere */
#include <sys/param.h>
#define	FILENAME_MAX	MAXPATHLEN
#endif /* FILENAME_MAX */

#endif /* SUNOS */

#endif /* WITHIN_PS */

#endif /* LSTDIO_H */
