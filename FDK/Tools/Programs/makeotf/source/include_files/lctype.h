/* @(#)CM_VerSion lctype.h atm07 1.2 16164.eco sum= 44376 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Veneer layer for ctype.h */

#ifndef LCTYPE_H
#define LCTYPE_H

#if WITHIN_PS

/* PostScript environment */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PROTOS
#include PUBLICTYPES
int isspace(int c);

#else /* WITHIN_PS */

/* ANSI C environment */
#include <ctype.h>

#endif /* WITHIN_PS */

#endif /* LCTYPE_H */
