/* @(#)CM_VerSion font.h atm08 1.2 16245.eco sum= 24755 atm08.002 */
/* @(#)CM_VerSion font.h atm07 1.2 16164.eco sum= 07383 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Font data structure declarations.
 */

#ifndef FONT_H
#define FONT_H

#include "dict.h"
#include "fdselect.h"

typedef struct Font_ Font;

#include "cs.h"

/* SID/code mapping */
typedef struct {
	unsigned char code;
	SID sid;
} CodeMap;
#define CODE_MAP_SIZE   (sizeCard8 + sizeCard16)

/* CID FD info */
typedef struct {
	/* For taking apart the CIDFont: */
	long SubrMapOffset;
	long SubrCount;
	short SDBytes;
	short seenChar;     /* Have there been any characters from this FD? */
	FDIndex newFDIndex; /* After possible removal of some unused fd's (due to
	                       subsetting), this is the new index of this fd. */
	DICT FD;
	DICT Private;
	CSData subrs;
	CSData chars;       /* Temporary subroutinized charstrings for DICT */
	unsigned short iChar;   /* Index of next char to be copied */
	struct {
		Offset Private;
		Offset Subrs;
	} offset;
	struct {
		long FD;
		long Private;
		long Subrs;
	} size;
} FDInfo;

/* Font data */
struct Font_ {
	short flags;
#define FONT_SYNTHETIC  (1 << 0)  /* Synthetic font */
#define FONT_CID        (1 << 1)  /* CID font */
#define FONT_CHAMELEON  (1 << 2)  /* Chameleon font */
	char *filename;         /* Source data filename */
	char *FontName;         /* PostScript font name */
	DICT dict;              /* Font dictionary data */
	DICT Private;           /* Private dictionary data (non-CID) */
	FDIndex *fdIndex;       /* CID: Map each glyph into fdInfo */
	short fdCount;          /* CID: Number of font & private dicts */
	FDInfo *fdInfo;         /* CID: [fdCount] */
	short iFDSelect;        /* CID: FDSelect array index */
	short iEncoding;        /* Encoding array index */
	short iCharset;         /* Charset array index */
	CSData subrs;           /* Subr data (non-CID) */
	CSData chars;           /* Char data */
	struct {                /* --- Synthetic font data */
		char *baseName;     /* Base font name */
		double oblique_term; /* aka FontMatrix 'c' */
		DICT dict;          /* Small synthetic font dict */
		short iEncoding;    /* Encoding array index */
	}
	synthetic;
	struct {                /* --- Chameleon font data */
		char *data;
		unsigned short nGlyphs;
	}
	chameleon;
	struct {                 /* --- Offsets */
		Offset encoding;
		Offset charset;
		Offset fdselect;
		Offset CharStrings;
		Offset FDArray;
		Offset Private;
		Offset Subrs;
	}
	offset;
	struct {                 /* --- Sizes */
		long FontName;
		long dict;
		long CharStrings;
		long FDArray;
		long Private;
		long Subrs;
	}
	size;
#if TC_STATISTICS
	long flatSize;          /* Size of flattened charstrings */
#endif /* TC_STATISTICS */
};

#endif /* FONT_H */