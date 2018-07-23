/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "cffwrite_charset.h"

#include <string.h>

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	SID *glyph;         /* [nGlyphs] */
} Format0;
#define FORMAT0_SIZE(n) (1 + (n) * 2)

typedef struct {
	SID first;          /* First glyph in range */
	unsigned char nLeft; /* Number of glyphs left in range (excluding first) */
} Range1;
#define RANGE1_SIZE     (2 + 1)

typedef struct {
	unsigned char format;
	Range1 *range;      /* [nRanges] */
} Format1;
#define FORMAT1_SIZE(n) (1 + (n) * RANGE1_SIZE)

typedef struct {
	SID first;          /* First glyph in range */
	unsigned short nLeft; /* Number of glyphs left in range (excluding first) */
} Range2;
#define RANGE2_SIZE     (2 * 2)

typedef struct {
	unsigned char format;
	Range2 *range;  /* [nRanges] */
} Format2;
#define FORMAT2_SIZE(n) (1 + (n) * RANGE2_SIZE)

typedef struct {
	dnaDCL(unsigned short, nameids); /* Name ids (SID/CID) */
	unsigned char format;   /* Selected format */
	Offset offset;          /* Offset relative to first charset */
	int is_cid;             /* Flags CID charset */
} Charset;

/* Predefined charsets */
static const SID isoadobe[] = {
#include "isocs0.h"
};
static const SID expert[] = {
#include "excs0.h"
};
static const SID expertsubset[] = {
#include "exsubcs0.h"
};
static const struct {
	long cnt;
	const SID *gnames;
}
predef[] = {
	{ ARRAY_LEN(isoadobe),       isoadobe },      /* 0 isoadobe */
	{ ARRAY_LEN(expert),         expert },        /* 1 Expert */
	{ ARRAY_LEN(expertsubset),   expertsubset },  /* 2 Expert subset */
};
#define PREDEF_CNT ARRAY_LEN(predef)

/* ----------------------------- Module context ---------------------------- */

struct charsetCtx_ {
	dnaDCL(Charset, charsets);
	Charset *_new;           /* Charset being added */
	cfwCtx g;               /* Package context */
};

/* Initialize charset. */
static void initCharset(void *ctx, long cnt, Charset *charset) {
	cfwCtx g = (cfwCtx)ctx;
	while (cnt--) {
		dnaINIT(g->ctx.dnaSafe, charset->nameids, 256, 750);
		charset++;
	}
}

/* Initialize module. */
void cfwCharsetNew(cfwCtx g) {
	charsetCtx h = (charsetCtx)cfwMemNew(g, sizeof(struct charsetCtx_));
	unsigned int i;

	/* Link contexts */
	h->g = g;
	g->ctx.charset = h;

	dnaINIT(g->ctx.dnaSafe, h->charsets, 4, 10);
	h->charsets.func = initCharset;

	/* Add standard charsets to accumulator */
	for (i = 0; i < PREDEF_CNT; i++) {
		Charset *charset = dnaNEXT(h->charsets);
		charset->nameids.cnt = predef[i].cnt;
		charset->nameids.array = (SID *)predef[i].gnames;
	}
}

/* Prepare module for reuse. */
void cfwCharsetReuse(cfwCtx g) {
	charsetCtx h = g->ctx.charset;
	h->charsets.cnt = PREDEF_CNT;
}

/* Free resources. */
void cfwCharsetFree(cfwCtx g) {
	charsetCtx h = g->ctx.charset;
	int i;

	if (h == NULL) {
		return;
	}

	for (i = PREDEF_CNT; i < h->charsets.size; i++) {
		dnaFREE(h->charsets.array[i].nameids);
	}
	dnaFREE(h->charsets);

	cfwMemFree(g, h);
	g->ctx.charset = NULL;
}

/* Begin new charset. */
void cfwCharsetBeg(cfwCtx g, int is_cid) {
	charsetCtx h = g->ctx.charset;
	h->_new = dnaNEXT(h->charsets);
	h->_new->nameids.cnt = 0;
	h->_new->is_cid = is_cid;
}

/* Add SID/CID to charset. */
void cfwCharsetAddGlyph(cfwCtx g, unsigned short nameid) {
	charsetCtx h = g->ctx.charset;
	*dnaNEXT(h->_new->nameids) = nameid;
}

/* End glyph name addition and check for duplicate charset. */
int cfwCharsetEnd(cfwCtx g) {
	charsetCtx h = g->ctx.charset;
	int i;

	for (i = h->_new->is_cid ? PREDEF_CNT : 0; i < h->charsets.cnt - 1; i++) {
		Charset *charset = &h->charsets.array[i];
		if (h->_new->nameids.cnt <= charset->nameids.cnt &&
		    memcmp(h->_new->nameids.array,
		           charset->nameids.array,
		           h->_new->nameids.cnt *
		           sizeof(h->_new->nameids.array[0])) == 0) {
			/* Found match; remove new charset from list */
			h->charsets.cnt--;
			return i;
		}
	}

	/* No match found; leave charset in list */
	return h->charsets.cnt - 1;
}

/* Fill charsets. */
long cfwCharsetFill(cfwCtx g) {
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
		long size0 = FORMAT0_SIZE(charset->nameids.cnt);
		long size1;
		long size2;

		/* Determine range1 size */
		nRanges1 = nRanges2 = 1;
		nLeft1 = nLeft2 = 0;
		for (j = 1; j < (unsigned)charset->nameids.cnt; j++) {
			int breakSeq =
			    charset->nameids.array[j - 1] + 1 != charset->nameids.array[j];
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

/* Write charsets and encodings. */
void cfwCharsetWrite(cfwCtx g) {
	charsetCtx h = g->ctx.charset;
	int i;

	/* Write non-standard charsets */
	for (i = PREDEF_CNT; i < h->charsets.cnt; i++) {
		unsigned j;
		Charset *charset = &h->charsets.array[i];

		cfwWrite1(g, charset->format);
		switch (charset->format) {
			case 0:
				/* Write format 0 */
				for (j = 0; j < (unsigned)charset->nameids.cnt; j++) {
					cfwWrite2(g, charset->nameids.array[j]);
				}
				break;

			case 1:
			{
				/* Write format 1 */
				unsigned char nLeft = 0;

				cfwWrite2(g, charset->nameids.array[0]);
				for (j = 1; j < (unsigned)charset->nameids.cnt; j++) {
					if (charset->nameids.array[j - 1] + 1 !=
					    charset->nameids.array[j] ||
					    nLeft == 255) {
						cfwWrite1(g, nLeft);
						cfwWrite2(g, charset->nameids.array[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				cfwWrite1(g, nLeft);
			}
			break;

			case 2:
			{
				/* Write format 2 */
				unsigned short nLeft = 0;

				cfwWrite2(g, charset->nameids.array[0]);
				for (j = 1; j < (unsigned)charset->nameids.cnt; j++) {
					if (charset->nameids.array[j - 1] + 1 !=
					    charset->nameids.array[j]) {
						cfwWrite2(g, nLeft);
						cfwWrite2(g, charset->nameids.array[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				cfwWrite2(g, nLeft);
			}
			break;
		}
	}
}

/* Get absolute charset offset. */
Offset cfwCharsetGetOffset(cfwCtx g, int iCharset, Offset base) {
	charsetCtx h = g->ctx.charset;
	return (iCharset < (int)PREDEF_CNT) ?
	       iCharset : base + h->charsets.array[iCharset].offset;
}