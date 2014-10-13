/* @(#)CM_VerSion fdselect.c atm08 1.2 16245.eco sum= 59571 atm08.002 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "fdselect.h"
#include "dynarr.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* Declarations used for determining sizes and for format documentation */
typedef struct {
	unsigned char *fds;         /* [nGlyphs] */
} Format0;
#define FORMAT0_SIZE(glyphs)    (sizeCard8 + sizeCard8 * (glyphs))

typedef struct {
	unsigned short first;
	unsigned char fd;
} Range3;
#define RANGE3_SIZE             (sizeCard16 + sizeCard8)

typedef struct {
	unsigned short nRanges;
	Range3 *range;
	unsigned short sentinel;
} Format3;
#define FORMAT3_SIZE(ranges) \
	(sizeCard8 + sizeCard16 + RANGE3_SIZE * (ranges) + sizeCard16)

typedef struct {
	unsigned short nGlyphs;
	FDIndex *fd;                /* [nGlyphs] */
	char format;
	long offset;
	unsigned short nRanges;
} FDSelect;

/* Module context */
struct fdselectCtx_ {
	dnaDCL(FDSelect, selectors);
	tcCtx g;                    /* Package context */
};

/* Initialize module */
void fdselectNew(tcCtx g) {
	fdselectCtx h = MEM_NEW(g, sizeof(struct fdselectCtx_));

	dnaINIT(g->ctx.dnaCtx, h->selectors, 4, 10);

	/* Link contexts */
	h->g = g;
	g->ctx.fdselect = h;
}

/* Free resources */
void fdselectFree(tcCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	dnaFREE(h->selectors);
	MEM_FREE(g, g->ctx.fdselect);
}

/* Add fdindex to accumulator if unique and return index */
int fdselectAdd(tcCtx g, unsigned nGlyphs, FDIndex *fd) {
	fdselectCtx h = g->ctx.fdselect;
	int i;
	FDSelect *selector;

	for (i = 0; i < h->selectors.cnt; i++) {
		selector = &h->selectors.array[i];
		if (nGlyphs <= selector->nGlyphs) {
			unsigned j;
			for (j = 0; j < nGlyphs; j++) {
				if (fd[j] != selector->fd[j]) {
					goto mismatch;
				}
			}
			return i;   /* Found match */
mismatch:;
		}
	}
	/* No match found; add fdindex */
	selector = dnaNEXT(h->selectors);
	selector->nGlyphs = nGlyphs;
	selector->fd = MEM_NEW(g, nGlyphs * sizeof(FDIndex));
	COPY(selector->fd, fd, nGlyphs);

	return h->selectors.cnt - 1;
}

/* Fill FDSelect */
long fdselectFill(tcCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	int i;
	Offset offset;

	/* Compute fdindex offsets */
	offset = 0;
	for (i = 0; i < h->selectors.cnt; i++) {
		unsigned j;
		long size0;
		long size3;
		FDSelect *selector = &h->selectors.array[i];

		/* Count ranges */
		selector->nRanges = 1;
		for (j = 1; j < selector->nGlyphs; j++) {
			if (selector->fd[j - 1] != selector->fd[j]) {
				selector->nRanges++;
			}
		}

		/* Save format and offset */
		size0 = FORMAT0_SIZE(selector->nGlyphs);
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

/* Get module into reusable state */
static void reuseInit(tcCtx g, fdselectCtx h) {
	int i;
	for (i = 0; i < h->selectors.cnt; i++) {
		MEM_FREE(g, h->selectors.array[i].fd);
	}
	h->selectors.cnt = 0;
}

/* Write FDSelect */
void fdselectWrite(tcCtx g) {
	fdselectCtx h = g->ctx.fdselect;
	int i;

	for (i = 0; i < h->selectors.cnt; i++) {
		unsigned j;
		FDSelect *selector = &h->selectors.array[i];

		OUT1(selector->format);
		switch (selector->format) {
			case 0:
				/* Write format 0 */
				for (j = 0; j < selector->nGlyphs; j++) {
					OUT1(selector->fd[j]);
				}
				break;

			case 3: {
				/* Write format 3 */
				FDIndex lastfd;

				OUT2(selector->nRanges);

				/* Begin first range */
				OUT2(0);
				lastfd = selector->fd[0];
				for (j = 1; j < selector->nGlyphs; j++) {
					if (lastfd != selector->fd[j]) {
						/* Complete last range and start new one */
						OUT1(lastfd);
						OUT2(j);
						lastfd = selector->fd[j];
					}
				}
				/* Complete last range and write sentinel */
				OUT1(lastfd);
				OUT2(selector->nGlyphs);
			}
			break;
		}
	}
	reuseInit(g, h);
}

/* Get absolute FDSelect offset */
Offset fdselectGetOffset(tcCtx g, int iFDSelect, Offset base) {
	fdselectCtx h = g->ctx.fdselect;
	return base +
	       ((h->selectors.cnt != 0) ? h->selectors.array[iFDSelect].offset : 0);
}