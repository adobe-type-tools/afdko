/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Baseline table (default values can be adjusted in the feature file)
 */

#include <stdlib.h>
#include <stdio.h>
#include "BASE.h"

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	unsigned short BaseTagCount;
	Tag *BaselineTag;                       /* [BaseTagCount] */
} BaseTagList;
#define BASE_TAG_LIST_SIZE(nTag) (uint16 + uint32 * (nTag))

typedef struct {
	unsigned short BaseCoordFormat;
	short Coordinate;
} BaseCoordFormat1;
#define BASE_COORD1_SIZE (uint16 * 2)

typedef struct {
	unsigned short BaseCoordFormat;
	short Coordinate;
	GID ReferenceGlyph;
	unsigned short BaseCoordPoint;
} BaseCoordFormat2;

typedef struct {
	unsigned short BaseCoordFormat;
	short Coordinate;
	DCL_OFFSET(void *, DeviceTable);
} BaseCoordFormat3;

typedef struct {
	unsigned short DefaultIndex;
	unsigned short BaseCoordCount;
	long *BaseCoord;        /* |-> BaseValues
	                           long instead of Offset for temp -ve value */
} BaseValues;
#define BASE_VALUES_SIZE(nCoord) (uint16 * 2 + uint16 * (nCoord))

typedef struct {
	Tag FeatureTableTag;
	DCL_OFFSET(void *, MinCoord);
	DCL_OFFSET(void *, MaxCoord);
} FeatMinMaxRecord;

typedef struct {
	DCL_OFFSET(void *, MinCoord);
	DCL_OFFSET(void *, MaxCoord);
	unsigned short FeatMinMaxCount;
	FeatMinMaxRecord *FeatMinMaxRecord;     /* [FeatMinMaxCount] */
} MinMax;

typedef struct {
	Tag BaseLangSysTag;
	DCL_OFFSET(MinMax, MinMax);
} BaseLangSysRecord;
#define BASE_LANGSYSREC_SIZE (uint32 + uint16)

typedef struct {
	Offset BaseValues;
	DCL_OFFSET(MinMax, DefaultMinMax);
	unsigned short BaseLangSysCount;
	BaseLangSysRecord *BaseLangSysRecord;   /* [BaseLangSysCount] */
} BaseScript;
#define BASE_SCRIPT_SIZE(nLangSys) (uint16 * 3 + BASE_LANGSYSREC_SIZE * (nLangSys))

typedef struct {
	Tag BaseScriptTag;
	long BaseScript;        /* |-> BaseScriptList
	                           long instead of Offset for temp -ve value */
} BaseScriptRecord;
#define BASE_SCRIPT_RECORD_SIZE (uint32 + uint16)

typedef struct {
	unsigned short BaseScriptCount;
	BaseScriptRecord *BaseScriptRecord;     /* [BaseScriptCount] */
} BaseScriptList;
#define BASE_SCRIPT_LIST_SIZE(nRec) (uint16 + BASE_SCRIPT_RECORD_SIZE * (nRec))

typedef struct {
	DCL_OFFSET(BaseTagList, BaseTagList);
	DCL_OFFSET(BaseScriptList, BaseScriptList);
} Axis;
#define AXIS_SIZE (uint16 * 2)

typedef struct {
	Fixed version;
	DCL_OFFSET(Axis, HorizAxis);
	DCL_OFFSET(Axis, VertAxis);

	/* Shared data */
	BaseScript *baseScript;     /* [h->baseScript.cnt] */
	BaseValues *baseValues;     /* [h->baseScript.cnt] */
	BaseCoordFormat1 *coord;    /* [h->coord.cnt] */
} BASETbl;
#define TBL_HDR_SIZE (int32 + uint16 * 2)

/* --------------------------- Context Definition ---------------------------*/

typedef struct {
	short dfltBaselineInx;
	dnaDCL(short, coordInx);
} BaseScriptInfo;

typedef struct {
	Tag script;
	short baseScriptInx;
} ScriptInfo;

typedef struct {
	dnaDCL(Tag, baseline);
	dnaDCL(ScriptInfo, script);
} AxisInfo;

struct BASECtx_ {
	AxisInfo horiz;
	AxisInfo vert;

	/* Shared data */
	dnaDCL(BaseScriptInfo, baseScript);
	dnaDCL(short, coord);   /* Only format 1 supported */

	struct {
		Offset curr;
		Offset shared;      /* Offset to start of shared area */
	}
	offset;

	BASETbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

/* Element initializer */
static void baseScriptInit(void *ctx, long count, BaseScriptInfo *bsi) {
	hotCtx g = ctx;
	int i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, bsi->coordInx, 5, 5);
		bsi++;
	}
}

void BASENew(hotCtx g) {
	BASECtx h = MEM_NEW(g, sizeof(struct BASECtx_));

	dnaINIT(g->dnaCtx, h->horiz.baseline, 5, 5);
	dnaINIT(g->dnaCtx, h->horiz.script, 10, 10);
	dnaINIT(g->dnaCtx, h->vert.baseline, 5, 5);
	dnaINIT(g->dnaCtx, h->vert.script, 10, 10);

	dnaINIT(g->dnaCtx, h->baseScript, 10, 10);
	h->baseScript.func = baseScriptInit;
	dnaINIT
	    (g->dnaCtx, h->coord, 10, 20);

	/* Link contexts */
	h->g = g;
	g->ctx.BASE = h;
}

static int CDECL cmpScriptInfo(const void *first, const void *second) {
	ScriptInfo *a = (ScriptInfo *)first;
	ScriptInfo *b = (ScriptInfo *)second;

	if (a->script < b->script) {
		return -1;
	}
	else if (a->script > b->script) {
		return 1;
	}
	else {
		return 0;
	}
}

static void prepAxis(BASECtx h, int vert) {
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	if (ai->baseline.cnt != 0) {
		if (ai->script.cnt == 0) {
			hotMsg(h->g, hotFATAL,
			       "scripts not specified for %s baseline axis",
			       vert ? "vertical" : "horizontal");
		}
		qsort(ai->script.array, ai->script.cnt, sizeof(ScriptInfo),
		      cmpScriptInfo);
	}
}

static Offset fillBaseScriptList(BASECtx h, Offset size, int vert) {
	long i;
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	Axis *axis = vert ? &h->tbl.VertAxis_ : &h->tbl.HorizAxis_;
	BaseScriptList *bsl = &axis->BaseScriptList_;
	long nScr = ai->script.cnt;

	bsl->BaseScriptCount = (unsigned short)nScr;
	bsl->BaseScriptRecord = MEM_NEW(h->g, sizeof(BaseScriptRecord) * nScr);
	for (i = 0; i < nScr; i++) {
		BaseScriptRecord *bsr = &bsl->BaseScriptRecord[i];
		ScriptInfo *si = &ai->script.array[i];

		bsr->BaseScriptTag = si->script;
		bsr->BaseScript = BASE_SCRIPT_SIZE(0) * si->baseScriptInx
		    - (h->offset.curr + size);      /* adjusted at write time */
	}

	return (Offset)BASE_SCRIPT_LIST_SIZE(nScr);
}

static Offset fillAxis(BASECtx h, int vert) {
	Offset size = AXIS_SIZE;
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	Axis *axis = vert ? &h->tbl.VertAxis_ : &h->tbl.HorizAxis_;
	unsigned nBaseTags = (unsigned)ai->baseline.cnt;

	if (nBaseTags == 0) {
		return 0;
	}

	/* Fill BaseTagList */
	axis->BaseTagList = size;
	axis->BaseTagList_.BaseTagCount = nBaseTags;
	axis->BaseTagList_.BaselineTag = MEM_NEW(h->g, sizeof(Tag) * nBaseTags);
	COPY(axis->BaseTagList_.BaselineTag, ai->baseline.array, nBaseTags);
	size += BASE_TAG_LIST_SIZE(nBaseTags);

	/* Fill BaseScriptList */
	axis->BaseScriptList = size;
	size += fillBaseScriptList(h, size, vert);

	return size;
}

static Offset fillSharedData(BASECtx h) {
	long i;
	unsigned j;
	BASETbl *fmt = &h->tbl;
	long nBScr = h->baseScript.cnt;
	Offset bsSize = 0;          /* Accumulator for BaseScript section */
	Offset bvSize = 0;          /* Accumulator for BaseValues section */
	Offset bsTotal = (Offset)(nBScr * BASE_SCRIPT_SIZE(0));

	/* --- Fill BaseScript and BaseValues in parallel --- */
	fmt->baseScript = MEM_NEW(h->g, sizeof(BaseScript) * nBScr);
	fmt->baseValues = MEM_NEW(h->g, sizeof(BaseValues) * nBScr);

	for (i = 0; i < nBScr; i++) {
		BaseScriptInfo *bsi = &h->baseScript.array[i];
		unsigned nCoord = (unsigned)bsi->coordInx.cnt;
		BaseScript *bs = &fmt->baseScript[i];
		BaseValues *bv = &fmt->baseValues[i];

		bs->BaseValues = bsTotal - bsSize + bvSize;
		bs->DefaultMinMax = NULL_OFFSET;
		bs->BaseLangSysCount = 0;

		bv->DefaultIndex = bsi->dfltBaselineInx;
		bv->BaseCoordCount = nCoord;
		bv->BaseCoord = MEM_NEW(h->g, sizeof(long) * nCoord);
		for (j = 0; j < nCoord; j++) {
			/* Adjusted below */
			bv->BaseCoord[j] = BASE_COORD1_SIZE * bsi->coordInx.array[j]
			    - bvSize;
		}

		bsSize += BASE_SCRIPT_SIZE(0);
		bvSize += BASE_VALUES_SIZE(nCoord);
	}

	/* Adjust BaseValue coord offsets */
	for (i = 0; i < nBScr; i++) {
		BaseValues *bv = &fmt->baseValues[i];
		for (j = 0; j < bv->BaseCoordCount; j++) {
			bv->BaseCoord[j] += bvSize;
		}
	}

	/* --- Fill coord data --- */
	fmt->coord = MEM_NEW(h->g, sizeof(BaseCoordFormat1) * h->coord.cnt);

	for (i = 0; i < h->coord.cnt; i++) {
		BaseCoordFormat1 *coord = &fmt->coord[i];
		coord->BaseCoordFormat = 1;
		coord->Coordinate = h->coord.array[i];
	}

	return bsSize + bvSize + BASE_COORD1_SIZE * (unsigned)h->coord.cnt;
}

int BASEFill(hotCtx g) {
	BASECtx h = g->ctx.BASE;
	Offset axisSize;

	if (h->horiz.baseline.cnt == 0 && h->vert.baseline.cnt == 0) {
		return 0;
	}
	prepAxis(h, 0);
	prepAxis(h, 1);

	h->tbl.version = VERSION(1, 0);

	h->offset.curr = TBL_HDR_SIZE;

	axisSize = fillAxis(h, 0);
	h->tbl.HorizAxis = (axisSize == 0) ? NULL_OFFSET : h->offset.curr;
	h->offset.curr += axisSize;

	axisSize = fillAxis(h, 1);
	h->tbl.VertAxis = (axisSize == 0) ? NULL_OFFSET : h->offset.curr;
	h->offset.curr += axisSize;

	h->offset.shared = h->offset.curr;  /* Indicates start of shared area */

	h->offset.curr += fillSharedData(h);

	return 1;
}

static void writeAxis(BASECtx h, int vert) {
	unsigned i;
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	Axis *axis = vert ? &h->tbl.VertAxis_ : &h->tbl.HorizAxis_;
	BaseTagList *btl = &axis->BaseTagList_;
	BaseScriptList *bsl = &axis->BaseScriptList_;

	if (ai->baseline.cnt == 0) {
		return;
	}

	/* --- Write axis header --- */
	OUT2(axis->BaseTagList);
	OUT2(axis->BaseScriptList);

	/* --- Write BaseTagList --- */
	OUT2(btl->BaseTagCount);
	for (i = 0; i < btl->BaseTagCount; i++) {
		OUT4(btl->BaselineTag[i]);
	}

	/* --- Write BaseScriptList --- */
	OUT2(bsl->BaseScriptCount);
	for (i = 0; i < bsl->BaseScriptCount; i++) {
		BaseScriptRecord *bsr = &bsl->BaseScriptRecord[i];
		OUT4(bsr->BaseScriptTag);
		OUT2((Offset)(bsr->BaseScript + h->offset.shared));
	}
}

static void writeSharedData(BASECtx h) {
	long i;
	BASETbl *fmt = &h->tbl;
	long nBScr = h->baseScript.cnt;

	/* --- Write BaseScript section --- */
	for (i = 0; i < nBScr; i++) {
		BaseScript *bs = &fmt->baseScript[i];

		OUT2(bs->BaseValues);
		OUT2(bs->DefaultMinMax);
		OUT2(bs->BaseLangSysCount);
		/* DefaultMinMax_ and BaseLangSysRecord not needed */
	}

	/* --- Write BaseValues section --- */
	for (i = 0; i < nBScr; i++) {
		unsigned j;
		BaseValues *bv = &fmt->baseValues[i];

		OUT2(bv->DefaultIndex);
		OUT2(bv->BaseCoordCount);
		for (j = 0; j < bv->BaseCoordCount; j++) {
			OUT2((unsigned short)(bv->BaseCoord[j]));
		}
	}

	/* --- Write coord section --- */
	for (i = 0; i < h->coord.cnt; i++) {
		BaseCoordFormat1 *coord = &fmt->coord[i];

		OUT2(coord->BaseCoordFormat);
		OUT2(coord->Coordinate);
	}
}

void BASEWrite(hotCtx g) {
	BASECtx h = g->ctx.BASE;

	OUT4(h->tbl.version);
	OUT2(h->tbl.HorizAxis);
	OUT2(h->tbl.VertAxis);
	writeAxis(h, 0);
	writeAxis(h, 1);
	writeSharedData(h);
}

static void freeAxis(BASECtx h, int vert) {
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	if (ai->baseline.cnt != 0) {
		Axis *axis = vert ? &h->tbl.VertAxis_ : &h->tbl.HorizAxis_;
		MEM_FREE(h->g, axis->BaseTagList_.BaselineTag);
		MEM_FREE(h->g, axis->BaseScriptList_.BaseScriptRecord);
	}
}

static void reuseAxisInfo(BASECtx h, int vert) {
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	ai->baseline.cnt = 0;
	ai->script.cnt = 0;
}

void BASEReuse(hotCtx g) {
	BASECtx h = g->ctx.BASE;
	long i;
	BASETbl *fmt = &h->tbl;

	freeAxis(h, 0);
	freeAxis(h, 1);

	MEM_FREE(g, fmt->baseScript);
	for (i = 0; i < h->baseScript.cnt; i++) {
		MEM_FREE(g, fmt->baseValues[i].BaseCoord);
	}
	MEM_FREE(g, fmt->baseValues);
	MEM_FREE(g, fmt->coord);

	reuseAxisInfo(h, 0);
	reuseAxisInfo(h, 1);

	h->baseScript.cnt = 0;

	h->coord.cnt = 0;
}

static void freeAxisInfo(AxisInfo *ai) {
	dnaFREE(ai->baseline);
	dnaFREE(ai->script);
}

void BASEFree(hotCtx g) {
	BASECtx h = g->ctx.BASE;
	long i;

	freeAxisInfo(&h->horiz);
	freeAxisInfo(&h->vert);

	for (i = 0; i < h->baseScript.size; i++) {
		dnaFREE(h->baseScript.array[i].coordInx);
	}
	dnaFREE(h->baseScript);

	dnaFREE(h->coord);

	MEM_FREE(g, g->ctx.BASE);
}

/* ------------------------ Supplementary Functions ------------------------ */

void BASESetBaselineTags(hotCtx g, int vert, int nTag, Tag *baselineTag) {
	BASECtx h = g->ctx.BASE;
	int i;
	AxisInfo *ai = vert ? &h->vert : &h->horiz;

#if HOT_DEBUG && 0
	fprintf(stderr, "#BASESetBaselineTags(%d):", vert);
	for (i = 0; i < nTag; i++) {
		fprintf(stderr, " %c%c%c%c", TAG_ARG(baselineTag[i]));
	}
	fprintf(stderr, "\n");
#endif

	for (i = 1; i < nTag; i++) {
		if (baselineTag[i] < baselineTag[i - 1]) {
			hotMsg(g, hotFATAL, "baseline tag list not sorted for %s axis",
			       vert ? "vertical" : "horizontal");
		}
	}

	dnaSET_CNT(ai->baseline, nTag);
	COPY(ai->baseline.array, baselineTag, nTag);
}

static int addCoord(BASECtx h, int coord) {
	long i;

	/* See if coord can be shared */
	for (i = 0; i < h->coord.cnt; i++) {
		if (h->coord.array[i] == coord) {
			return i;
		}
	}

	/* Add a coord */
	*dnaNEXT(h->coord) = coord;
	return h->coord.cnt - 1;
}

static int addScript(BASECtx h, int dfltInx, int nBaseTags, short *coord) {
	long i;
	BaseScriptInfo *bsi;

	/* See if a baseScript can be shared */
	for (i = 0; i < h->baseScript.cnt; i++) {
		bsi = &h->baseScript.array[i];
		if (bsi->dfltBaselineInx == dfltInx && nBaseTags == bsi->coordInx.cnt) {
			int j;
			for (j = 0; j < nBaseTags; j++) {
				if (h->coord.array[bsi->coordInx.array[j]] != coord[j]) {
					goto nextScript;
				}
			}
			return i;
		}
nextScript:
		;
	}

	/* Add a baseScript */
	bsi = dnaNEXT(h->baseScript);

	bsi->dfltBaselineInx = dfltInx;

	dnaSET_CNT(bsi->coordInx, nBaseTags);
	for (i = 0; i < nBaseTags; i++) {
		bsi->coordInx.array[i] = addCoord(h, coord[i]);
	}

	return h->baseScript.cnt - 1;
}

void BASEAddScript(hotCtx g, int vert, Tag script, Tag dfltBaseline,
                   short *coord) {
	BASECtx h = g->ctx.BASE;
	int i;
	AxisInfo *ai = vert ? &h->vert : &h->horiz;
	ScriptInfo *si = dnaNEXT(ai->script);
	int dfltInx = -1;
	long nBaseTags = ai->baseline.cnt;

	if (ai->baseline.cnt == 0) {
		hotMsg(g, hotFATAL, "baseline tags not specified for %s axis",
		       vert ? "vertical" : "horizontal");
	}

	/* Calculate dfltInx */
	for (i = 0; i < nBaseTags; i++) {
		if (dfltBaseline == ai->baseline.array[i]) {
			dfltInx = i;
		}
	}
	if (dfltInx == -1) {
		hotMsg(g, hotFATAL, "baseline %c%c%c%c not specified for %s axis",
		       TAG_ARG(dfltBaseline), vert ? "vertical" : "horizontal");
	}

#if 0
	fprintf(stderr, "#BASEAddScript(%d): (dflt:%c%c%c%c [%d])", vert,
	        TAG_ARG(dfltBaseline), dfltInx);
	for (i = 0; i < nBaseTags; i++) {
		fprintf(stderr, " %d", coord[i]);
	}
	fprintf(stderr, "\n");
#endif

	si->script = script;
	si->baseScriptInx = addScript(h, dfltInx, nBaseTags, coord);
}