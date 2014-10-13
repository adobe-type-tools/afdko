/* @(#)CM_VerSion charset.c atm08 1.2 16245.eco sum= 58114 atm08.002 */
/* @(#)CM_VerSion charset.c atm07 1.2 16164.eco sum= 10445 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "charset.h"
#include "dynarr.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	SID *glyph;         /* [nGlyphs] */
} Format0;
#define FORMAT0_SIZE(n) (sizeCard8 + sizeCard16 * (n))

typedef struct {
	SID first;          /* First glyph in range */
	unsigned char nLeft; /* Number of glyphs left in range (excluding first) */
} Range1;
#define RANGE1_SIZE     (sizeCard16 + sizeCard8)

typedef struct {
	Range1 *range;      /* [nRanges] */
} Format1;
#define FORMAT1_SIZE(n) (sizeCard8 + RANGE1_SIZE * (n))

typedef struct {
	SID first;          /* First glyph in range */
	unsigned short nLeft; /* Number of glyphs left in range (excluding first) */
} Range2;
#define RANGE2_SIZE     (sizeCard16 * 2)

typedef struct {
	Range2 *range;  /* [nRanges] */
} Format2;
#define FORMAT2_SIZE(n) (sizeCard8 + RANGE2_SIZE * (n))

typedef struct {
	unsigned short nGlyphs;
	SID *glyph;     /* [nGlyphs] */
	unsigned char format;
	Offset offset;
} Charset;

/* --- Predefined charsets --- */
static SID isoadobe[] = {
#include "isocs0.h"
};
static SID expert[] = {
#include "excs0.h"
};
static SID expertsubset[] = {
#include "exsubcs0.h"
};
static Charset predef[] = {
	{ TABLE_LEN(isoadobe),       isoadobe },      /* 0 isoadobe */
	{ TABLE_LEN(expert),         expert },        /* 1 Expert */
	{ TABLE_LEN(expertsubset),   expertsubset },  /* 2 Expert subset */
};
#define PREDEF_CNT TABLE_LEN(predef)

/* Module context */
struct charsetCtx_ {
	dnaDCL(Charset, charsets);
	tcCtx g;                /* Package context */
};

/* Initialize module */
void charsetNew(tcCtx g) {
	charsetCtx h = MEM_NEW(g, sizeof(struct charsetCtx_));
	unsigned int i;

	dnaINIT(g->ctx.dnaCtx, h->charsets, 4, 10);

	/* Add standard charsets to accumulator */
	for (i = 0; i < PREDEF_CNT; i++) {
		*dnaNEXT(h->charsets) = predef[i];
	}

	/* Link contexts */
	h->g = g;
	g->ctx.charset = h;
}

/* Free resources */
void charsetFree(tcCtx g) {
	charsetCtx h = g->ctx.charset;
	dnaFREE(h->charsets);
	MEM_FREE(g, g->ctx.charset);
}

/* Add charset to accumulator if unique and return index */
int charsetAdd(tcCtx g, unsigned nGlyphs, SID *glyph, int is_cid) {
	charsetCtx h = g->ctx.charset;
	int i;
	Charset *charset;

	for (i = is_cid ? PREDEF_CNT : 0; i < h->charsets.cnt; i++) {
		charset = &h->charsets.array[i];
		if (nGlyphs <= charset->nGlyphs) {
			unsigned j;
			for (j = 0; j < nGlyphs; j++) {
				if (glyph[j] != charset->glyph[j]) {
					goto mismatch;
				}
			}
			return i;   /* Found match */
mismatch:;
		}
	}
	/* No match found; add charset */
	charset = dnaNEXT(h->charsets);
	charset->nGlyphs = nGlyphs;
	charset->glyph = MEM_NEW(g, nGlyphs * sizeof(SID));
	COPY(charset->glyph, glyph, nGlyphs);

	return h->charsets.cnt - 1;
}

/* Fill charsets */
long charsetFill(tcCtx g) {
	charsetCtx h = g->ctx.charset;
	int i;
	Offset offset;

	/* Compute charset offsets */
	offset = 0;
	for (i = PREDEF_CNT; i < h->charsets.cnt; i++) {
		unsigned j;
		unsigned nRanges1;
		unsigned nRanges2;
		unsigned nLeft1;
		unsigned nLeft2;
		Charset *charset = &h->charsets.array[i];
		long size0 = FORMAT0_SIZE(charset->nGlyphs);
		long size1;
		long size2;

		/* Determine range1 size */
		nRanges1 = nRanges2 = 1;
		nLeft1 = nLeft2 = 0;
		for (j = 1; j < charset->nGlyphs; j++) {
			int breakSeq = charset->glyph[j - 1] + 1 != charset->glyph[j];
			if (breakSeq || nLeft1 == 255) {
				nRanges1++;
				nLeft1 = 0;
			}
			else {
				nLeft1++;
			}
			if (breakSeq) {
				nRanges2++;
				nLeft2 = 0;
			}
			else {
				nLeft2++;
			}
		}
		size1 = FORMAT1_SIZE(nRanges1);
		size2 = FORMAT2_SIZE(nRanges2);

		/* Save format and offset */
		charset->offset = offset;
		if (size0 < size1) {
			if (size0 < size2) {
				charset->format = 0;
				offset += size0;
			}
			else {
				charset->format = 2;
				offset += size2;
			}
		}
		else {
			if (size1 < size2) {
				charset->format = 1;
				offset += size1;
			}
			else {
				charset->format = 2;
				offset += size2;
			}
		}
	}
	return offset;
}

/* Get module into reusable state */
static void reuseInit(tcCtx g, charsetCtx h) {
	int i;
	for (i = PREDEF_CNT; i < h->charsets.cnt; i++) {
		MEM_FREE(g, h->charsets.array[i].glyph);
	}
	h->charsets.cnt = PREDEF_CNT;
}

/* Write charsets and encodings */
void charsetWrite(tcCtx g) {
	charsetCtx h = g->ctx.charset;
	int i;

	/* Write non-standard charsets */
	for (i = PREDEF_CNT; i < h->charsets.cnt; i++) {
		unsigned j;
		Charset *charset = &h->charsets.array[i];

		OUT1(charset->format);
		switch (charset->format) {
			case 0:
				/* Write format 0 */
				for (j = 0; j < charset->nGlyphs; j++) {
					OUT2(charset->glyph[j]);
				}
				break;

			case 1: {
				/* Write format 1 */
				unsigned nLeft = 0;

				OUT2(charset->glyph[0]);
				for (j = 1; j < charset->nGlyphs; j++) {
					if (charset->glyph[j - 1] + 1 != charset->glyph[j] ||
					    nLeft == 255) {
						OUT1(nLeft);
						OUT2(charset->glyph[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				OUT1(nLeft);
			}
			break;

			case 2: {
				/* Write format 2 */
				unsigned nLeft = 0;

				OUT2(charset->glyph[0]);
				for (j = 1; j < charset->nGlyphs; j++) {
					if (charset->glyph[j - 1] + 1 != charset->glyph[j]) {
						OUT2(nLeft);
						OUT2(charset->glyph[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				OUT2(nLeft);
			}
			break;
		}
	}
	reuseInit(g, h);
}

/* Get absolute charset offset */
Offset charsetGetOffset(tcCtx g, int iCharset, Offset base) {
	charsetCtx h = g->ctx.charset;
	if (iCharset < (long)PREDEF_CNT) {
		return iCharset;
	}
	else {
		return base + h->charsets.array[iCharset].offset;
	}
	/* Xcode compiler persists in complaining about signed/unsigned comparisons.
	   return (iCharset < PREDEF_CNT)?
	    iCharset: base + h->charsets.array[iCharset].offset;
	 */
}