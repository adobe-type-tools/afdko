/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Font Variations table.
 */

#include "fvar.h"
#include "name.h"
#include "dynarr.h"

/* ---------------------------- Table Definition --------------------------- */
#define fvar_VERSION VERSION(1, 0)

typedef struct {
	unsigned long axisTag;
	Fixed minValue;
	Fixed defaultValue;
	Fixed maxValue;
	unsigned short flags;
	unsigned short nameId;
} Axis;
#define AXIS_SIZE   (uint32 + 3 * int32 + 2 * uint16)

typedef struct {
	unsigned short nameId;
	unsigned short flags;
	dnaDCL(Fixed, coord);
} Instance;
#define INSTANCE_SIZE(axes) (2 * uint16 + (axes) * int32)

typedef struct {
	Fixed version;
	unsigned short offsetToData;
	unsigned short countSizePairs;
	unsigned short axisCount;
	unsigned short axisSize;
	unsigned short instanceCount;
	unsigned short instanceSize;
	dnaDCL(Axis, axis);
	dnaDCL(Instance, instance);
} fvarTbl;
#define HEADER_SIZE (int32 + 6 * uint16)

/* --------------------------- Context Definition -------------------------- */
struct fvarCtx_ {
	fvarTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

/* Initialize instance */
static void initInstance(void *ctx, long count, Instance *instance) {
	hotCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, instance->coord, 3, 12);
		instance++;
	}
	return;
}

void fvarNew(hotCtx g) {
	fvarCtx h = MEM_NEW(g, sizeof(struct fvarCtx_));

	dnaINIT(g->dnaCtx, h->tbl.axis, 4, 11);
	dnaINIT(g->dnaCtx, h->tbl.instance, 12, 12);
	h->tbl.instance.func = initInstance;

	/* Link contexts */
	h->g = g;
	g->ctx.fvar = h;
}

int fvarFill(hotCtx g) {
	int i;
	fvarCtx h = g->ctx.fvar;

	if (!IS_MM(g)) {
		return 0;
	}

	h->tbl.version = VERSION(1, 0);
	h->tbl.offsetToData = HEADER_SIZE;
	h->tbl.countSizePairs = 2;
	h->tbl.axisCount = (unsigned short)g->font.mm.axis.cnt;
	h->tbl.axisSize = AXIS_SIZE;
	h->tbl.instanceCount = (unsigned short)g->font.mm.instance.cnt;
	h->tbl.instanceSize = INSTANCE_SIZE(h->tbl.axisCount);
	dnaSET_CNT(h->tbl.axis, h->tbl.axisCount);
	dnaSET_CNT(h->tbl.instance, h->tbl.instanceCount);

	/* Fill axes */
	for (i = 0; i < h->tbl.axisCount; i++) {
		MMAxis *src = &g->font.mm.axis.array[i];
		Axis *dst = &h->tbl.axis.array[i];

		dst->axisTag = src->tag;
		dst->minValue = src->minRange;
		dst->maxValue = src->maxRange;
		if (g->font.mm.UDV[i] != g->font.mm.instance.array[0].UDV[i]) {
			hotMsg(g, hotWARNING,
			       "default instance doesn't match user design vector in font");
		}
		dst->defaultValue = g->font.mm.UDV[i];
		dst->flags = 0;
		dst->nameId = nameAddUser(g, src->longLabel);
	}

	/* Fill instances */
	for (i = 0; i < h->tbl.instanceCount; i++) {
		MMInstance *src = &g->font.mm.instance.array[i];
		Instance *dst = &h->tbl.instance.array[i];

		dst->nameId = nameAddUser(g, src->name.array);
		dst->flags = 0;
		dnaSET_CNT(dst->coord, h->tbl.axisCount);
		COPY(dst->coord.array, src->UDV, h->tbl.axisCount);
	}
	return 1;
}

void fvarWrite(hotCtx g) {
	int i;
	fvarCtx h = g->ctx.fvar;

	/* Write header */
	OUT4(h->tbl.version);
	OUT2(h->tbl.offsetToData);
	OUT2(h->tbl.countSizePairs);
	OUT2(h->tbl.axisCount);
	OUT2(h->tbl.axisSize);
	OUT2(h->tbl.instanceCount);
	OUT2(h->tbl.instanceSize);

	/* Write axes */
	for (i = 0; i < h->tbl.axisCount; i++) {
		Axis *axis = &h->tbl.axis.array[i];

		OUT4(axis->axisTag);
		OUT4(axis->minValue);
		OUT4(axis->defaultValue);
		OUT4(axis->maxValue);
		OUT2(axis->flags);
		OUT2(axis->nameId);
	}

	/* Write instances */
	for (i = 0; i < h->tbl.instanceCount; i++) {
		int j;
		Instance *instance = &h->tbl.instance.array[i];

		OUT2(instance->nameId);
		OUT2(instance->flags);

		for (j = 0; j < h->tbl.axisCount; j++) {
			OUT4(instance->coord.array[j]);
		}
	}
}

void fvarReuse(hotCtx g) {
}

void fvarFree(hotCtx g) {
	fvarCtx h = g->ctx.fvar;
	int i;

	for (i = 0; i < h->tbl.instance.size; i++) {
		dnaFREE(h->tbl.instance.array[i].coord);
	}

	dnaFREE(h->tbl.axis);
	dnaFREE(h->tbl.instance);
	MEM_FREE(g, g->ctx.fvar);
}