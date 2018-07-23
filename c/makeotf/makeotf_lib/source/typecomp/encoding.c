/* @(#)CM_VerSion encoding.c atm08 1.2 16245.eco sum= 44268 atm08.002 */
/* @(#)CM_VerSion encoding.c atm07 1.2 16164.eco sum= 04987 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "encoding.h"
#include "sindex.h"
#include "dynarr.h"

#include <string.h>

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	unsigned char nCodes;
	unsigned char *code;    /* [nCodes] */
} Format0;
#define FORMAT0_SIZE(n) (sizeCard8 * 2 + sizeCard8 * (n))

typedef struct {
	unsigned char first;    /* First code in range */
	unsigned char nLeft;    /* Number codes left in range (excluding first) */
} Range;
#define RANGE_SIZE      (sizeCard8 * 2)

typedef struct {
	unsigned char nRanges;
	Range *range;           /* [nRanges] */
} Format1;
#define FORMAT1_SIZE(n) (sizeCard8 * 2 + RANGE_SIZE * (n))

/* Supplemental encodings */
typedef struct {
	unsigned char nSups;
	CodeMap *supplement;
} Supplemental;
#define SUPPLEMENTAL_SIZE(n) (sizeCard8 + CODE_MAP_SIZE * (n))

typedef struct {
	short nCodes;
	unsigned char *code;        /* Comparison array [nCodes] */
	unsigned char nRanges;
	unsigned char nSups;
	CodeMap *supplement;
	unsigned char format;
	Offset offset;
} Encoding;

/* --- Predefined encodings --- */
static SID predef[][256] = {
	{
#include "stdenc1.h"    /* 0 Standard */
	},
	{
#include "exenc1.h"     /* 1 Expert (also used with expert subset fonts) */
	},
};
#define PREDEF_CNT TABLE_LEN(predef)

/* Standard encoded glyph names */
static char *std[256] = {
#include "stdenc2.h"
};

/* Module context */
struct encodingCtx_ {
	dnaDCL(Encoding, encodings);
	tcCtx g;                /* Package context */
};

/* Initialize encoding */
static void initEncoding(void *ctx, long count, Encoding *encoding) {
	long i;
	for (i = 0; i < count; i++) {
		encoding->supplement = NULL;
		encoding++;
	}
	return;
}

/* Initialize module */
void encodingNew(tcCtx g) {
	encodingCtx h = MEM_NEW(g, sizeof(struct encodingCtx_));

	dnaINIT(g->ctx.dnaCtx, h->encodings, 1, 10);
	h->encodings.func = initEncoding;

	/* Link contexts */
	h->g = g;
	g->ctx.encoding = h;
}

/* Free resources */
void encodingFree(tcCtx g) {
	encodingCtx h = g->ctx.encoding;
	dnaFREE(h->encodings);
	MEM_FREE(g, g->ctx.encoding);
}

/* Return pointer to standard encoding (SID elements) */
SID *encodingGetStd(void) {
	return predef[0];
}

/* Return pointer to standard encoding (name elements) */
char **encodingGetStdNames(void) {
	return std;
}

/* Return glyph name from standard encoding */
char *encodingGetStdName(int code) {
	return std[code];
}

/* Check to see if encoding matches a predefined encoding. The match succeeds
   if every ENCODED glyph matches with predefined encoding. Therefore, it is
   possible for subset fonts to use predefined encodings, thus saving space */
int encodingCheckPredef(SID *enc) {
	unsigned int i;
	for (i = 0; i < PREDEF_CNT; i++) {
		int j;
		for (j = 0; j < 256; j++) {
			if (enc[j] != SID_NOTDEF && enc[j] != predef[i][j]) {
				goto mismatch;
			}
		}
		return i; /* Match found */
mismatch:;
	}
	return -1;  /* No match */
}

/* Add encoding to accumulator if unique and return index */
int encodingAdd(tcCtx g, int nCodes, unsigned char *code,
                int nSups, CodeMap *supplement) {
	encodingCtx h = g->ctx.encoding;
	int i;
	Encoding *encoding;

	for (i = 0; i < h->encodings.cnt; i++) {
		encoding = &h->encodings.array[i];
		if (nCodes == encoding->nCodes && nSups == encoding->nSups) {
			int j;
			for (j = 0; j < nCodes; j++) {
				if (code[j] != encoding->code[j]) {
					goto mismatch;
				}
			}
			for (j = 0; j < nSups; j++) {
				if (supplement[j].code != encoding->supplement[j].code ||
				    supplement[j].sid != encoding->supplement[j].sid) {
					goto mismatch;
				}
			}
			return PREDEF_CNT + i;  /* Found match */
mismatch:
			;
		}
	}
	/* No match found; add encoding */
	encoding = dnaNEXT(h->encodings);
	encoding->nCodes = nCodes;
	encoding->code = MEM_NEW(g, nCodes);
	COPY(encoding->code, code, nCodes);
	encoding->nSups = nSups;
	if (nSups > 0) {
		encoding->supplement = MEM_NEW(g, nSups * sizeof(CodeMap));
		COPY(encoding->supplement, supplement, nSups);
	}

	return PREDEF_CNT + h->encodings.cnt - 1;
}

/* Fill encodings */
long encodingFill(tcCtx g) {
	encodingCtx h = g->ctx.encoding;
	int i;
	Offset offset;

	offset = 0;
	for (i = 0; i < h->encodings.cnt; i++) {
		int j;
		Encoding *encoding = &h->encodings.array[i];
		long size0;
		long size1;

		/* Count ranges */
		encoding->nRanges = 1;
		for (j = 1; j < encoding->nCodes; j++) {
			if (encoding->code[j - 1] + 1 != encoding->code[j]) {
				encoding->nRanges++;
			}
		}

		/* Save format and offset */
		size0 = FORMAT0_SIZE(encoding->nCodes);
		size1 = FORMAT1_SIZE(encoding->nRanges);
		encoding->format = size0 > size1;
		encoding->offset = offset;
		offset += (size0 > size1) ? size1 : size0;
		if (encoding->nSups > 0) {
			encoding->format |= 0x80;
			offset += SUPPLEMENTAL_SIZE(encoding->nSups);
		}
	}
	return offset;
}

/* Get module into reusable state */
static void reuseInit(tcCtx g, encodingCtx h) {
	int i;
	for (i = 0; i < h->encodings.cnt; i++) {
		Encoding *encoding = &h->encodings.array[i];
		MEM_FREE(g, encoding->code);
		if (encoding->supplement != NULL) {
			MEM_FREE(g, encoding->supplement);
			encoding->supplement = NULL;
		}
	}
	h->encodings.cnt = 0;
}

/* Write encodings */
void encodingWrite(tcCtx g) {
	encodingCtx h = g->ctx.encoding;
	int i;

	/* Write non-standard encodings */
	for (i = 0; i < h->encodings.cnt; i++) {
		int j;
		Encoding *encoding = &h->encodings.array[i];

		OUT1(encoding->format);
		switch (encoding->format & 0x7f) {
			case 0:
				/* Write format 0 */
				OUT1(encoding->nCodes);
				for (j = 0; j < encoding->nCodes; j++) {
					OUT1(encoding->code[j]);
				}
				break;

			case 1: {
				/* Write format 1 */
				unsigned nLeft = 0;

				OUT1(encoding->nRanges);
				OUT1(encoding->code[0]);
				for (j = 1; j < encoding->nCodes; j++) {
					if (encoding->code[j - 1] + 1 != encoding->code[j] ||
					    nLeft == 255) {
						OUT1(nLeft);
						OUT1(encoding->code[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				OUT1(nLeft);
			}
			break;
		}
		if (encoding->format & 0x80) {
			OUT1(encoding->nSups);
			for (j = 0; j < encoding->nSups; j++) {
				CodeMap *supplement = &encoding->supplement[j];
				OUT1(supplement->code);
				OUT2(supplement->sid);
			}
		}
	}

	reuseInit(g, h);
}

/* Get absolute encoding offset */
Offset encodingGetOffset(tcCtx g, int iEncoding, Offset base) {
	encodingCtx h = g->ctx.encoding;
	if (iEncoding < (long)PREDEF_CNT) {
		return iEncoding;
	}
	else {
		return base + h->encodings.array[iEncoding - PREDEF_CNT].offset;
	}
}
