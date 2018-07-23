/* @(#)CM_VerSion string.h atm08 1.2 16245.eco sum= 56533 atm08.002 */
/* @(#)CM_VerSion string.h atm07 1.2 16164.eco sum= 63208 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * String storage and identification.
 */

#ifndef SINDEX_H
#define SINDEX_H

#include "common.h"

#define SID_NOTDEF          0       /* SID for .notdef */
#define SID_UNDEF           0xffff  /* SID of undefined string */

#if TC_EURO_SUPPORT
/* Standard string ids */
#define SID_DOLLAR          5
#define SID_ZERO            17
#define SID_STERLING        98
#define SID_TRADEMARK       153
#define SID_DOLLAROLDSTYLE  231
#define SID_ZEROOLDSTYLE    239
#define SID_CAP_O   48
#define SID_LOWERCASE_X 89
#endif /* TC_EURO_SUPPORT */

/* Non-NUL-terminated sindex */
typedef struct {
	unsigned length;
	char *data;
} String;

void sindexNew(tcCtx g);
void sindexFree(tcCtx g);

SID sindexGetId(tcCtx g, unsigned length, char *strng);
SID sindexGetGlyphNameId(tcCtx g, unsigned length, char *strng);
int sindexSeenGlyphNameId(tcCtx g, SID sid);
SID sindexCheckId(tcCtx g, unsigned length, char *strng);
char *sindexGetString(tcCtx g, SID id);
void sindexFontInit(tcCtx g);

long sindexSize(tcCtx g);
void sindexWrite(tcCtx g);

#endif /* SINDEX_H */