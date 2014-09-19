/* @(#)CM_VerSion lstring.h atm07 1.2 16164.eco sum= 64282 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Veneer layer for string.h */
#ifndef LSTRING_H
#define LSTRING_H

#if WITHIN_PS

/* PostScript environment */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PROTOS
#include EXCEPT	
#include PUBLICTYPES
#include PSLIB
#include UTIL

#define strncpy os_strncpy
#define strcpy os_strcpy
#define strncmp os_strncmp
#define strcmp os_strcmp
#define strlen os_strlen
#define strchr os_index
double strtod(const char *str, char **ptr);
long strtol(const char *str, char **ptr, int base);
#define memcpy(d, s, n) os_bcopy(s, d, n)
#define memmove(d, s, n) os_bcopy(s, d, n)
#define memset(d, v, n) os_bvalue(d, n, v)

#else /* WITHIN_PS */

/* ANSI C environment */
#include <string.h>

#if SUNOS
/* SunOS libc doesn't define memmove() but bcopy() can be substituted */
char *bcopy(const void *src, void *dst, int len);
#define memmove(d,s,l)  bcopy(s,d,l)
#endif /* SUNOS */

#endif /* WITHIN_PS */

#endif /* LSTRING_H */
