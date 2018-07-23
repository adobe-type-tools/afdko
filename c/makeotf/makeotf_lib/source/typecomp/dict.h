/* @(#)CM_VerSion dict.h atm08 1.2 16245.eco sum= 50233 atm08.002 */
/* @(#)CM_VerSion dict.h atm07 1.2 16164.eco sum= 03771 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Tokenized dictionary support.
 */

#ifndef DICT_H
#define DICT_H

#include "common.h"
#include "dictops.h"

/* Tokenized dictionary data */
typedef dnaDCL (char, DICT);

/* Compute DICT op size */
#define DICTOPSIZE(op) (((op) & 0xff00) ? 2 : 1)

/* Save dict operator without args */
#define DICTSAVEOP(da, op) do { if ((op) & 0xff00) { *dnaNEXT(da) = cff_escape; } \
		                        *dnaNEXT(da) = (unsigned char)(op); } \
	while (0)

void dictSaveInt(DICT *dict, long i);
void dictSaveNumber(DICT *dict, double d);
void dictSaveT2Number(DICT *dict, double d);

#if TC_DEBUG
void dictDump(tcCtx g, DICT *dict);

#endif /* TC_DEBUG */

#endif /* DICT_H */