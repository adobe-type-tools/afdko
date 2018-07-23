/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "cffwrite_fdselect.h"

#include <string.h>

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	unsigned char format;
	unsigned char *fds;         /* [nGlyphs] */
} Format0;
#define FORMAT0_SIZE(nGlyphs)   (1 + (nGlyphs) * 1)

typedef struct {
	unsigned short first;
	unsigned char fd;
} Range3;
#define RANGE3_SIZE             (2 + 1)

typedef struct {
	unsigned char format;
	unsigned short nRanges;
	Range3 *range;
	unsigned short sentinel;
} Format3;
#define FORMAT3_SIZE(nRanges)   (1 + 2 + (nRanges) * RANGE3_SIZE + 2)

typedef struct {
	dnaDCL(unsigned char, fds);
	unsigned short nRanges;
	char format;
	Offset offset;
} Selector;

/* ----------------------------- Module context ---------------------------- */

struct fdselectCtx_ {
	dnaDCL(Selector, selectors);
	Selector *_new;
	cfwCtx g;                   /* Package context */
};

/* Initialize selector. */
static void initSelector(void *ctx, long cnt, Selector *selector) {
	cfwCtx g = (cfwCtx)ctx;
	while (cnt--) {
		dnaINIT(g->ctx.dnaSafe, selector->fds, 10000, 20000);
		selector++;
	}
}

/* Initialize module */
void cfwFdselectNew(cfwCtx g) {
	fdselectCtx h = (fdselectCtx)cfwMemNew(g, sizeof(struct fdselectCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.fdselect = h;

	dnaINIT(g->ctx.dnaSafe, h->selectors, 1, 5);
	h->selectors.func = initSelector;
}

/* Prepare module for reuse. */
void cfwFdselectReuse(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	h->selectors.cnt = 0;
}

/* Free resources */
void cfwFdselectFree(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	long i;

	if (h == NULL) {
		return;
	}

	for (i = 0; i < h->selectors.size; i++) {
		dnaFREE(h->selectors.array[i].fds);
	}
	dnaFREE(h->selectors);

	cfwMemFree(g, h);
	g->ctx.fdselect = NULL;
}

/* Begin new fdselect. */
void cfwFdselectBeg(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	h->_new = dnaNEXT(h->selectors);
	h->_new->fds.cnt = 0;
}

/* Add fd index to selector. */
void cfwFdselectAddIndex(cfwCtx g, unsigned char fd) {
	fdselectCtx h = g->ctx.fdselect;
	*dnaNEXT(h->_new->fds) = fd;
}

/* End fd index addition and check for duplicate selector. */
int cfwFdselectEnd(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	int i;

	for (i = 0; i < h->selectors.cnt - 1; i++) {
		Selector *selector = &h->selectors.array[i];
		if (h->_new->fds.cnt <= selector->fds.cnt &&
		    memcmp(h->_new->fds.array, selector->fds.array,
		           h->_new->fds.cnt) == 0) {
			/* Found match; remove new selector from list */
			h->selectors.cnt--;
			return i;
		}
	}

	/* No match found; leave selector in list */
	return h->selectors.cnt - 1;
}

/* Fill selectors. */
long cfwFdselectFill(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	int i;
	Offset offset;

	offset = 0;
	for (i = 0; i < h->selectors.cnt; i++) {
		unsigned j;
		long size0;
		long size3;
		Selector *selector = &h->selectors.array[i];

		/* Count ranges */
		selector->nRanges = 1;
		for (j = 1; j < (unsigned)selector->fds.cnt; j++) {
			if (selector->fds.array[j - 1] != selector->fds.array[j]) {
				selector->nRanges++;
			}
		}

		/* Save format and offset */
		size0 = FORMAT0_SIZE(selector->fds.cnt);
		size3 = FORMAT3_SIZE(selector->nRanges);
		selector->offset = offset;
		if (size0 <= size3) {
			selector->format = 0;
			offset += size0;
		}
		else {
			selector->format = 3;
			offset += size3;
		}
	}
	return offset;
}

/* Write selectors. */
void cfwFdselectWrite(cfwCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	int i;

	for (i = 0; i < h->selectors.cnt; i++) {
		unsigned j;
		Selector *selector = &h->selectors.array[i];

		cfwWrite1(g, selector->format);
		switch (selector->format) {
			case 0:
				/* Write format 0 */
				cfwWrite(g, selector->fds.cnt, (char *)selector->fds.array);
				break;

			case 3:
			{
				/* Write format 3 */
				unsigned char lastfd;

				cfwWrite2(g, selector->nRanges);

				/* Begin first range */
				cfwWrite2(g, 0);
				lastfd = selector->fds.array[0];
				for (j = 1; j < (unsigned)selector->fds.cnt; j++) {
					unsigned char fd = selector->fds.array[j];
					if (fd != lastfd) {
						/* Complete last range and start new one */
						cfwWrite1(g, lastfd);
						cfwWrite2(g, (unsigned short)j);
						lastfd = fd;
					}
				}
				/* Complete last range and write sentinel */
				cfwWrite1(g, lastfd);
				cfwWrite2(g, (unsigned short)j);
			}
			break;
		}
	}
}

/* Get absolute selector offset */
Offset cfwFdselectGetOffset(cfwCtx g, int iSelector, Offset base) {
	fdselectCtx h = g->ctx.fdselect;
	return base +
	       ((h->selectors.cnt != 0) ? h->selectors.array[iSelector].offset : 0);
}