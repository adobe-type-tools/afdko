/* @(#)CM_VerSion lerrno.h atm07 1.2 16164.eco sum= 09123 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Veneer layer for errno.h */

#ifndef LERRNO_H
#define LERRNO_H

#if WITHIN_PS

/* PostScript environment */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PROTOS
#include PUBLICTYPES

#else /* WITHIN_PS */

/* ANSI C environment */
#include <errno.h>

#endif /* WITHIN_PS */

#endif /* LERRNO_H */
