/* @(#)CM_VerSion subr.h atm08 1.2 16245.eco sum= 63581 atm08.002 */
/* @(#)CM_VerSion subr.h atm07 1.2 16164.eco sum= 62866 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Subroutinization support.
 *
 * xxx get this comment up to date.
 * After initialization each font's charstring data is added with
 * subrFont(). When all charstring data in the font set has been added
 * the fonts are rescanned and overlap conflicts are resolved by a call to
 * subrRescanFont(). Partial subroutine selection is then performed by a call
 * to subrSelect(). Finally, each font is subroutinized by a call to
 * subrAddLocal().
 */

#ifndef CFFWRITE_SUBR_H
#define CFFWRITE_SUBR_H

#include "cffwrite.h"
#include "cffwrite_share.h"
#include "txops.h"

/* In order to avoid matching repeats across charstring boundaries it is
   necessary to add a unique separator between charstrings. This is achieved by
   inserting the t2_separator operator after each endchar. The 1-byte operator
   value is followed by a 24-bit number that is guaranteed to be different for
   each charstring thereby removing any possibility of a match spanning an
   endchar operator. These operators are inserted by the recode module and
   removed by the subroutinizer */
#define t2_separator    t2_reserved9

/* Types */
typedef unsigned char subr_FDIndex; /* FDArray index */

typedef struct {
	unsigned short nStrings;
	Offset *offset;     /* Offset (index) array */
	char *data;         /* Charstring data buffer */
	char *refcopy;      /* Copy of "data" pointer for use by subroutinizer */
} subr_CSData;

typedef struct                  /* INDEX table header */
{
	unsigned short count;
	OffSize offSize;
} INDEXHdr;

/* CID FD info */
typedef struct {
	subr_CSData subrs;
	subr_CSData chars;      /* Temporary subroutinized charstrings for DICT */
	unsigned short iChar;   /* Index of next char to be copied */
} subr_FDInfo;

typedef struct {
	short flags;
#define SUBR_FONT_CID       (1 << 1)  /* CID font */
	subr_FDIndex *fdIndex;      /* CID: Map each glyph into fdInfo */
	short fdCount;          /* CID: Number of font & private dicts */
	subr_FDInfo *fdInfo;            /* CID: [fdCount] */
	subr_CSData subrs;          /* Subr data (non-CID) */
	subr_CSData chars;          /* Char data */
} subr_Font;

void cfwSubrNew(cfwCtx g);
void cfwSubrFree(cfwCtx g);
void cfwSubrReuse(cfwCtx g);

void cfwSubrSubrize(cfwCtx g, int nFonts, subr_Font *fonts);

long cfwSubrSizeLocal(cfwCtx g, subr_CSData *subrs);
void cfwSubrWriteLocal(cfwCtx g, subr_CSData *subrs);

void cfwAddCubeGSUBR(cfwCtx g, char *cstr, long length);
void cfwMergeCubeGSUBR(cfwCtx g);
void cfwSeenLEIndex(cfwCtx g, long leIndex);
long cfwSubrSizeGlobal(cfwCtx g);
void cfwSubrWriteGlobal(cfwCtx g);

#endif /* CFFWRITE_SUBR_H */
