/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Multiple master font metrics table.
 */

#include "MMFX.h"

#include <stdlib.h>

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	Fixed version;
	unsigned short nMetrics;
	unsigned short offSize;
	long *offset;           /* [nMetrics] */
} MMFXTbl;
#define HEADER_SIZE (int32 + uint16 * 2)

typedef struct {
	unsigned short id;      /* Metric id */
	short length;           /* Charstring length */
	long index;             /* Charstring index */
} Metric;

typedef struct {            /* Used as lookup key */
	MMFXCtx ctx;
	int length;
	char *cstr;
} MetricLookup;

struct MMFXCtx_ {
	MMFXTbl tbl;                    /* Table data */
	unsigned short nextUnnamedId;   /* Next available unnamed metric id */
	dnaDCL(Metric, metrics);        /* Charstring indexes */
	dnaDCL(char, cstrs);            /* Charstrings */
	hotCtx g;                       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void MMFXNew(hotCtx g) {
	MMFXCtx h = MEM_NEW(g, sizeof(struct MMFXCtx_));

	h->nextUnnamedId = MMFXNamedMetricCnt;
	h->tbl.offset = NULL;
	dnaINIT(g->dnaCtx, h->metrics, 1000, 2000);
	dnaINIT(g->dnaCtx, h->cstrs, 5000, 10000);

	/* Link contexts */
	h->g = g;
	g->ctx.MMFX = h;
}

/* Compare metric ids */
static int CDECL cmpIds(const void *first, const void *second) {
	unsigned a = ((Metric *)first)->id;
	unsigned b = ((Metric *)second)->id;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	else {
		return 0;
	}
}

int MMFXFill(hotCtx g) {
	MMFXCtx h = g->ctx.MMFX;
	int i;
	Offset offset;

	if (!IS_MM(g)) {
		return 0;
	}

	/* Fill header */
	h->tbl.version  = VERSION(1, 0);
	h->tbl.nMetrics = (unsigned short)h->metrics.cnt;
	h->tbl.offSize  =
	    (HEADER_SIZE + uint16 * h->metrics.cnt +
	     h->metrics.array[h->metrics.cnt - 1].index > 65535) ? 4 : 2;

	/* Sort metrics by id */
	qsort(h->metrics.array, h->metrics.cnt, sizeof(Metric), cmpIds);

	/* Check if all named metrics present */
	for (i = 0; i < h->metrics.cnt; i++) {
		if (h->metrics.array[i].id >= MMFXNamedMetricCnt) {
			break;
		}
	}
	if (i < MMFXNamedMetricCnt) {
		hotMsg(g, hotFATAL, "Missing named metrics");
	}
	else if (i > MMFXNamedMetricCnt) {
		hotMsg(g, hotFATAL, "Duplicate named metrics");
	}

	/* Allocate and fill offset array */
	offset = (unsigned short)(HEADER_SIZE + h->tbl.offSize * h->metrics.cnt);
	h->tbl.offset = MEM_NEW(g, sizeof(long) * h->metrics.cnt);
	for (i = 0; i < h->metrics.cnt; i++) {
		h->tbl.offset[i] = offset + h->metrics.array[i].index;
	}

	return 1;
}

void MMFXWrite(hotCtx g) {
	MMFXCtx h = g->ctx.MMFX;
	int i;

	/* Write header */
	OUT4(h->tbl.version);
	OUT2(h->tbl.nMetrics);
	OUT2(h->tbl.offSize);

	/* Write offset array */
	if (h->tbl.offSize == 2) {
		for (i = 0; i < h->tbl.nMetrics; i++) {
			OUT2((short)h->tbl.offset[i]);
		}
	}
	else {
		for (i = 0; i < h->tbl.nMetrics; i++) {
			OUT4(h->tbl.offset[i]);
		}
	}

	/* Write charstring data */
	OUTN(h->cstrs.cnt, h->cstrs.array);
}

void MMFXReuse(hotCtx g) {
	MMFXCtx h = g->ctx.MMFX;

	h->nextUnnamedId = MMFXNamedMetricCnt;
	MEM_FREE(g, h->tbl.offset);
	h->metrics.cnt = 0;
	h->cstrs.cnt = 0;
}

void MMFXFree(hotCtx g) {
	MMFXCtx h = g->ctx.MMFX;
	dnaFREE(h->metrics);
	dnaFREE(h->cstrs);
	MEM_FREE(g, g->ctx.MMFX);
}

/* ------------------------ Supplementary Functions ------------------------ */

static int CDECL matchMetric(const void *first, const void *second, void *ctx) {
	MetricLookup *a = (MetricLookup *)first;
	Metric *b = (Metric *)second;

	if (a->length < b->length) {
		return -1;
	}
	else if (a->length > b->length) {
		return 1;
	}
	else {
		MMFXCtx h = a->ctx;
		return memcmp(a->cstr, &h->cstrs.array[b->index], a->length);
	}
}

/* Add metric data */
static unsigned addMetric(MMFXCtx h, long id, FWord *metric) {
	Metric *dst;
	size_t index;
	MetricLookup met;
	char cstr[64];
	int found;

	met.ctx = h;
	met.cstr = cstr;
	met.length = hotMakeMetric(h->g, metric, met.cstr);
	found = ctuLookup(&met, h->metrics.array, h->metrics.cnt, sizeof(Metric),
	                  matchMetric, &index, h);
	if (id == -1) {
		/* Unnamed metric */
		if (found) {
			/* Already in list; return its id */
			return h->metrics.array[index].id;
		}
		else {
			/* Not in list; allocate id */
			id = h->nextUnnamedId++;
		}
	}

	/* Insert id in list */
	dst = INSERT(h->metrics, index);
	dst->id = (unsigned short)id;
	dst->length = met.length;
	if (found) {
		/* Record index of charstring already in pool */
		dst->index = h->metrics.array[index].index;
	}
	else {
		/* Add new charstring to pool */
		dst->index = h->cstrs.cnt;
		memcpy(dnaEXTEND(h->cstrs, met.length), met.cstr, met.length);
	}

	return (unsigned)id;
}

/* Add metric data to accumulator and return its id */
unsigned MMFXAddMetric(hotCtx g, FWord *metric) {
	MMFXCtx h = g->ctx.MMFX;
	return addMetric(h, -1, metric);
}

/* Add named metric to accumulator. Call only once for each named metric id
   (overwriting not possible). */
void MMFXAddNamedMetric(hotCtx g, unsigned id, FWord *metric) {
	MMFXCtx h = g->ctx.MMFX;
	(void)addMetric(h, id, metric);
}

/* Get cstr index for id. Return 0 if id found; 1 otherwise. */
static int getMetric(MMFXCtx h, unsigned id, unsigned *cstrInx,
                     unsigned *length) {
	long i;
	/* xxx Reverse linear search for now. This will be efficient for current
	   use. */
	for (i = h->metrics.cnt - 1; i >= 0; i--) {
		Metric *met = &h->metrics.array[i];
		if (met->id == id) {
			*cstrInx = met->index;
			*length = (unsigned)met->length;
			return 0;
		}
	}
	return 1;
}

/* Execute metric at default instance. Assumes only one returned result */
cffFixed MMFXExecMetric(hotCtx g, unsigned id) {
	MMFXCtx h = g->ctx.MMFX;
	unsigned length;
	unsigned cstrInx;
	cffFixed result[T2_MAX_OP_STACK];

	if (getMetric(h, id, &cstrInx, &length)) {
		hotMsg(g, hotFATAL, "Metric id <%u> not found", id);
	}

	cffExecLocalMetric(g->ctx.cff, &h->cstrs.array[cstrInx],
	                   h->cstrs.cnt - cstrInx, result);
	return result[0];
}