/* @(#)CM_VerSion cs.h atm08 1.2 1.2 1.2 1.2 16245.eco sum= 21318 atm08.002 */
/* @(#)CM_VerSion cs.h atm07 1.2 1.2 1.2 1.2 16164.eco sum= 38848 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * CharString processor.
 */

#ifndef CS_H
#define CS_H

#include "common.h"
#include "pstoken.h"
#include <stdint.h>

#define CS_MAX_SIZE 65535   /* Max charstring size (bytes) */

/* Single char/subr reference */
typedef struct {
	unsigned short length;
	char *cstr;
} Charstring;

/* Grouped charstring/subr data */
typedef struct {
	unsigned short nStrings;
	Offset *offset;     /* Offset (index) array */
	char *data;         /* Charstring data buffer */
	char *refcopy;      /* Copy of "data" pointer for use by subroutinizer */
} CSData;

/* Vector conversion subr record */
typedef struct {
	SID sid;
	unsigned short iSubr;   /* Subr index */
	Charstring data;        /* Subr data */
} ConvSubr;

#include "font.h"

/* Charstring conversion procs */
typedef void (*csDecrypt)(unsigned length, unsigned char *cstr);
typedef struct {
	void (*newFont)(tcCtx g, Font *font);
	long (*endFont)(tcCtx g);
	void (*addChar)(tcCtx g, unsigned length, char *cstr, unsigned id,
	                unsigned nSubrs, Charstring *subrs, int fd);
	char *(*getChar)(tcCtx g, unsigned iChar, int fd, unsigned *length);
	void (*writeChar)(tcCtx g, Font *font, unsigned iChar);
	void (*addAuth)(tcCtx g, char *auth);
} csConvProcs;

void csNew(tcCtx g);
void csFree(tcCtx g);

void csNewFont(tcCtx g, Font *font);
void csNewPrivate(tcCtx g, int fd, int lenIV, csDecrypt decrypt);
void csAddSubr(tcCtx g, unsigned length, char *cstr, int fd);
void csAddChar(tcCtx g, unsigned length, char *cstr,
               unsigned id, int fd, int encrypted);
void csEndFont(tcCtx g, unsigned nChars, unsigned short *recode);
void csSetConvProcs(tcCtx g, csConvProcs *procs);

int csEncInteger(int32_t i, char *t);
int csEncFixed(Fixed f, char *t);

long csSizeChars(tcCtx g, Font *font);
void csWriteChars(tcCtx g, Font *font);

void csWriteData(tcCtx g, CSData *data, long dataSize, int offSize);
void csFreeData(tcCtx g, CSData *data);
void csFreeFont(tcCtx g, Font *font);

#if TC_DEBUG
unsigned csDump(long length, unsigned char *cstr, int nMasters, int t1);
void csDumpSubrs(tcCtx g, Font *font);

#endif /* TC_DEBUG */

#endif /* CS_H */