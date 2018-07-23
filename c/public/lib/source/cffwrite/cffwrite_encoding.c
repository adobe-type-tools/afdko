/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "cffwrite_encoding.h"

#include <string.h>
#include <stdlib.h>

typedef struct              /* Supplementary encoding */
{
	unsigned char code;
	SID sid;
} SupCode;
#define SUP_CODE_SIZE   (1 + 2)

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	unsigned char format;
	unsigned char nCodes;
	unsigned char *code;    /* [nCodes] */
} Format0;
#define FORMAT0_SIZE(n) (2 * 1 + (n) * 1)

typedef struct {
	unsigned char first;    /* First code in range */
	unsigned char nLeft;    /* Number codes left in range (excluding first) */
} Range;
#define RANGE_SIZE      (2 * 1)

typedef struct {
	unsigned char format;
	unsigned char nRanges;
	Range *range;           /* [nRanges] */
} Format1;
#define FORMAT1_SIZE(n) (2 * 1 + (n) * RANGE_SIZE)

/* Supplemental encodings */
typedef struct {
	unsigned char nSups;
	SupCode *supplement;
} Supplemental;
#define SUPPLEMENTAL_SIZE(n) (1 + (n) * SUP_CODE_SIZE)

typedef struct {
	dnaDCL(unsigned char, codes);   /* Character codes */
	dnaDCL(SupCode, supcodes);      /* Supplementary codes */
	unsigned char nRanges;          /* # code ranges */
	unsigned char format;           /* Selected format */
	Offset offset;                  /* Offset relative to first encoding */
} Encoding;

/* Predefined encodings */
static const unsigned char stdenc[] = {
#include "stdenc0.h"    /* 0 Standard */
};
static const unsigned char exenc[] = {
#include "exenc0.h"     /* 1 Expert (also used with expert subset fonts) */
};

static const struct {
	short cnt;
	const unsigned char *array;
}
predefs[] = {
	{ ARRAY_LEN(stdenc), stdenc },
	{ ARRAY_LEN(exenc), exenc },
};
#define PREDEF_CNT ARRAY_LEN(predefs)

/* ----------------------------- Module Context ---------------------------- */

struct encodingCtx_ {
	dnaDCL(Encoding, encodings);
	Encoding *_new;          /* Encoding being added */
	cfwCtx g;               /* Package context */
};

/* Initialize encoding. */
static void initEncoding(void *ctx, long cnt, Encoding *encoding) {
	cfwCtx g = (cfwCtx)ctx;
	while (cnt--) {
		dnaINIT(g->ctx.dnaSafe, encoding->codes, 256, 256);
		dnaINIT(g->ctx.dnaSafe, encoding->supcodes, 5, 10);
		encoding++;
	}
}

/* Initialize module. */
void cfwEncodingNew(cfwCtx g) {
	encodingCtx h = (encodingCtx)cfwMemNew(g, sizeof(struct encodingCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.encoding = h;

	dnaINIT(g->ctx.dnaSafe, h->encodings, 1, 10);
	h->encodings.func = initEncoding;
}

/* Prepare module for reuse. */
void cfwEncodingReuse(cfwCtx g) {
	encodingCtx h = g->ctx.encoding;
	h->encodings.cnt = 0;
}

/* Free resources */
void cfwEncodingFree(cfwCtx g) {
	encodingCtx h = g->ctx.encoding;
	int i;

	if (h == NULL) {
		return;
	}

	for (i = 0; i < h->encodings.size; i++) {
		Encoding *encoding = &h->encodings.array[i];
		dnaFREE(encoding->codes);
		dnaFREE(encoding->supcodes);
	}
	dnaFREE(h->encodings);

	cfwMemFree(g, h);
	g->ctx.encoding = NULL;
}

/* Get pre-defined encoding identified by "id" (beginning at 0). An array of
   "cnt" elements is returned that maps an SID to a character code or 0 if
   unencoded. A return of NULL indicates no encoding is pre-defined for the
   specified id. */
const unsigned char *cfwEncodingGetPredef(int id, int *cnt) {
	if (id >= (int)PREDEF_CNT) {
		return NULL;
	}

	*cnt = predefs[id].cnt;
	return predefs[id].array;
}

/* Begin new encoding. */
void cfwEncodingBeg(cfwCtx g) {
	encodingCtx h = g->ctx.encoding;
	h->_new = dnaNEXT(h->encodings);
	h->_new->codes.cnt = 0;
	h->_new->supcodes.cnt = 0;
}

/* Add code point to encoding. */
void cfwEncodingAddCode(cfwCtx g, unsigned char code) {
	encodingCtx h = g->ctx.encoding;
	*dnaNEXT(h->_new->codes) = code;
}

/* Add supplementary code point to encoding. */
void cfwEncodingAddSupCode(cfwCtx g, unsigned char code, SID sid) {
	encodingCtx h = g->ctx.encoding;
	SupCode *sup = dnaNEXT(h->_new->supcodes);
	sup->code = code;
	sup->sid = sid;
}

/* Compare supplementary encodings. */
static int CTL_CDECL cmpSupCodes(const void *first, const void *second) {
	SupCode *a = (SupCode *)first;
	SupCode *b = (SupCode *)second;
	if (a->code < b->code) {
		return -1;
	}
	else if (a->code > b->code) {
		return 1;
	}
	else if (a->sid < b->sid) {
		return -1;
	}
	else if (a->sid > b->sid) {
		return 1;
	}
	else {
		return 0;
	}
}

/* End code point addition and check for duplicate encoding. */
int cfwEncodingEnd(cfwCtx g) {
	encodingCtx h = g->ctx.encoding;
	int i;

	/* Sort supplementary encoding by code */
	if (h->_new->supcodes.cnt > 0) {
		qsort(h->_new->supcodes.array, h->_new->supcodes.cnt, sizeof(SupCode),
		      cmpSupCodes);
	}

	for (i = 0; i < h->encodings.cnt - 1; i++) {
		Encoding *encoding = &h->encodings.array[i];

		if (h->_new->codes.cnt == encoding->codes.cnt &&
		    h->_new->supcodes.cnt == encoding->supcodes.cnt &&
		    memcmp(h->_new->codes.array,
		           encoding->codes.array,
		           h->_new->codes.cnt) == 0 &&
		    memcmp(h->_new->supcodes.array,
		           encoding->supcodes.array,
		           h->_new->supcodes.cnt) == 0) {
			/* Found match; remove new encoding from list */
			h->encodings.cnt--;
			return PREDEF_CNT + i;
		}
	}

	/* No match found; leave encoding in list */
	return PREDEF_CNT + h->encodings.cnt - 1;
}

/* Fill encodings and return total size of all encodings. */
long cfwEncodingFill(cfwCtx g) {
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
		for (j = 1; j < encoding->codes.cnt; j++) {
			if (encoding->codes.array[j - 1] + 1 != encoding->codes.array[j]) {
				encoding->nRanges++;
			}
		}

		encoding->offset = offset;

		/* Save format and offset */
		size0 = FORMAT0_SIZE(encoding->codes.cnt);
		size1 = FORMAT1_SIZE(encoding->nRanges);
		if (size0 < size1) {
			/* Select format 0 */
			offset += size0;
			encoding->format = 0;
		}
		else {
			/* Select format 1 */
			offset += size1;
			encoding->format = 1;
		}
		if (encoding->supcodes.cnt > 0) {
			encoding->format |= 0x80;
			offset += SUPPLEMENTAL_SIZE(encoding->supcodes.cnt);
		}
	}
	return offset;
}

/* Write encodings. */
void cfwEncodingWrite(cfwCtx g) {
	encodingCtx h = g->ctx.encoding;
	int i;

	/* Write non-standard encodings */
	for (i = 0; i < h->encodings.cnt; i++) {
		int j;
		Encoding *encoding = &h->encodings.array[i];

		cfwWrite1(g, encoding->format);
		switch (encoding->format & 0x7f) {
			case 0:
				/* Write format 0 */
				cfwWrite1(g, (unsigned char)encoding->codes.cnt);
				cfwWrite(g, encoding->codes.cnt, (char *)encoding->codes.array);
				break;

			case 1:
			{
				/* Write format 1 */
				unsigned char nLeft = 0;

				cfwWrite1(g, encoding->nRanges);
				cfwWrite1(g, encoding->codes.array[0]);
				for (j = 1; j < encoding->codes.cnt; j++) {
					if (encoding->codes.array[j - 1] + 1 !=
					    encoding->codes.array[j] ||
					    nLeft == 255) {
						cfwWrite1(g, nLeft);
						cfwWrite1(g, encoding->codes.array[j]);
						nLeft = 0;
					}
					else {
						nLeft++;
					}
				}
				cfwWrite1(g, nLeft);
			}
			break;
		}
		if (encoding->format & 0x80) {
			cfwWrite1(g, (unsigned char)encoding->supcodes.cnt);
			for (j = 0; j < encoding->supcodes.cnt; j++) {
				SupCode *supplement = &encoding->supcodes.array[j];
				cfwWrite1(g, supplement->code);
				cfwWrite2(g, supplement->sid);
			}
		}
	}
}

/* Get absolute encoding offset. */
Offset cfwEncodingGetOffset(cfwCtx g, int iEncoding, Offset base) {
	encodingCtx h = g->ctx.encoding;
	return (iEncoding < (int)PREDEF_CNT) ?
	       iEncoding : base + h->encodings.array[iEncoding - PREDEF_CNT].offset;
}