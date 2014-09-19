/* @(#)CM_VerSion lstdlib.h atm07 1.2 16164.eco sum= 22075 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Veneer layer for stdlib.h */

#ifndef LSTDLIB_H
#define LSTDLIB_H

#if WITHIN_PS

/* PostScript environment */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PROTOS
#include PUBLICTYPES
#include PSLIB
#include PSSUPPORT

#define size_t IntX	/* xxx this can't be correct */

typedef int (*CompareProc)(const void *keyval, const void *datum);
void qsort(void *base, size_t count, size_t size, CompareProc cmp);
void *bsearch(const void *key, const void *base, size_t count, size_t size, 
			  CompareProc cmp);
#define exit os_exit

#else /* WITHIN_PS */

/* ANSI C environment */
#include <stdlib.h>

#endif /* WITHIN_PS */

#endif /* LSTDLIB_H */
