/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 *	Glyph positioning table.
 */

#include "GPOS.h"
#include "MMFX.h"
#include "OS_2.h"

#include "otl.h"
#include "map.h"
#include "feat.h"
#include "name.h"

#include <stdlib.h>
#include <stdio.h>

#define CHECK4OVERFLOW(n,t,s) do{if ((n) > 0xFFFF) \
	hotMsg(g, hotFATAL, "%s offset overflow (0x%0lx) in %s positioning",\
		   (t), (n), (s));}while(0)

/* --------------------------- Context Definition -------------------------- */



typedef struct {
	unsigned short PosFormat;               /* =1 */
	unsigned short ExtensionLookupType;
	LOffset ExtensionOffset;
} ExtensionPosFormat1;
#define EXTENSION1_SIZE (uint16 * 2 + uint32)

typedef struct {             /* Subtable record */
	Tag script;
	Tag language;
	Tag feature;
	unsigned short lkpType;
	unsigned short lkpFlag;
	unsigned short markSetIndex;
	Label label;
	LOffset offset;         /* From beginning of first subtable */
	struct {                 /* Extension-related data */
		short use;          /* Use extension lookupType? If set, then following
		                       used: */
		otlTbl otl;         /* For coverages and classes of this extension
		                       subtable */
		LOffset offset;     /* Offset to this extension subtable, from
		                       beginning of extension section. Debug only. */
		ExtensionPosFormat1 *tbl;

		/* Subtable data */
	} extension;
	void *tbl;              /* Format-specific subtable data */
} Subtable;

typedef union {              /* Subtable record */
	short numParams;        /* First two bytes is the length of the paramters */
	char params[1];         /* A place holder */
} FeatureParameterFormat;     /* Special case format for subtable data. */

enum {
	kSizeOpticalSize,
	kSizeSubFamilyID,
	kSizeMenuNameID,
	kSizeLowEndRange,
	kSizeHighEndRange
};

typedef struct {
	GNode *targ;
} PosRule;

typedef struct {
	unsigned short SequenceIndex;
	unsigned short LookupListIndex;
} PosLookupRecord;
#define POS_LOOKUP_RECORD_SIZE  (uint16 * 2)

typedef struct {
	GID gid;
	short xPla;
	short yPla;
	short xAdv;
	short yAdv;
	unsigned short valFmt;
	struct {
		short valFmt;
		short valRec;
	} span;
} SingleRec;

typedef struct {
	GNode *gnode;
} MarKClassRec;     /* used in markClassList, to provide a quick lookup fur the currently useds mark classes */

typedef struct {
	GID gid;
	dnaDCL(AnchorMarkInfo, anchorMarkInfo);
	long componentIndex; /* order in mark to lig statement that this base glyph reference was encountered.*/
	long lineNum;
} BaseGlyphRec;


typedef union {
	GID gid;                        /* glyph ID */
	GNode *gcl;                     /* glyph class */
} KernGlyph;

typedef struct MetricsRec {
	short value[4];
} MetricsRec;


typedef struct {                     /* Kerning pair */
	KernGlyph first;
	KernGlyph second;
	short metricsCnt1; /* Allowable values are 1 ( x advance adjsutment only) or 4. */
	short metricsRec1[4];
	short metricsCnt2; /* Allowable values are 1 ( x advance adjsutment only) or 4. */
	short metricsRec2[4];
} KernRec;

typedef struct {
	unsigned class;
	GNode *gc;
} ClassInfo;

typedef struct {
	dnaDCL(ClassInfo, classInfo);
	dnaDCL(GID, cov);               /* Coverage */
} ClassDef;

typedef struct {                     /* New subtable data */
	Tag script;
	Tag language;
	Tag feature;
	short useExtension;             /* Use extension lookupType? */
	unsigned short lkpType;
	unsigned short parentLkpType;
	unsigned short lkpFlag;
	unsigned short markSetIndex;
	Label label;

	dnaDCL(PosRule, rules);
	dnaDCL(MarKClassRec, markClassList);
	dnaDCL(BaseGlyphRec, baseList);

	dnaDCL(SingleRec, single);	/* Single pos accumulator */
	dnaDCL(KernRec, pairs);		/* Kern pair accumulator */
	Subtable *sub;				/* Current subtable */
	short pairFmt;				/* Fmt (1 or 2) of GPOS pair */
        unsigned short pairValFmt1;	/* Fmt (1 or 2) of first value record  of GPOS pair */
        unsigned short pairValFmt2;	/* Fmt (1 or 2) of second value record of GPOS pair */
	char* fileName;				/* the current feature file name */
} SubtableInfo;


struct GPOSCtx_ {
	SubtableInfo new;
	struct {
		LOffset subtable;         /* (Cumulative.) Next subtable offset |->
		                             start of subtable section. LOffset to
		                             check for overflow */
		LOffset extension;        /* (Cumulative.) Next extension subtable
		                             offset |-> start of extension section. */
		LOffset extensionSection; /* Start of extension section |-> start of
		                             main subtable section */
	}
	offset;
	dnaDCL(short, values);          /* Concatenated value record fields */
	dnaDCL(Subtable, subtables);    /* Subtable list */
	unsigned short featNameID;  /* user name ID for sub-family name for 'size' feature.
	                                    needed in order to set the FeatureParam subtable on writing it. */

	short startNewPairPosSubtbl;
	ClassDef classDef[2];


	short hadError;                 /* Flags if error occurred */

	/* Info for chaining contextual lookups */
	dnaDCL(SubtableInfo, anonSubtable); /* Anon subtbl accumulator */
	dnaDCL(PosLookupRecord *, posLookup); /* Ptrs to all records that need
	                                           to be adjusted */
	dnaDCL(GNode *, prod);          /* Tmp for cross product */

	unsigned short maxContext;

	otlTbl otl;                     /* OTL table */
	hotCtx g;                       /* Package context */
};


static void fillSinglePos(hotCtx g, GPOSCtx h);
static void writeSinglePos(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeSinglePos(hotCtx g, Subtable *sub);

static void fillPairPos(hotCtx g, GPOSCtx h);
static void writePairPos(hotCtx g, GPOSCtx h, Subtable *sub);
static void freePairPos(hotCtx g, Subtable *sub);

static ExtensionPosFormat1 *fillExtensionPos(hotCtx g, GPOSCtx h,
											 unsigned ExtensionLookupType);
static void writeExtension(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeExtension(hotCtx g, GPOSCtx h, Subtable *sub);

static void fillSizeFeature(hotCtx g, GPOSCtx h, Subtable *sub);
static void writeFeatParam(GPOSCtx h, Subtable *sub);
static void freeFeatParam(hotCtx g, Subtable *sub);
static void fillChain(hotCtx g, GPOSCtx h);
static void writeChainPos(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeChain3(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeChain(hotCtx g, GPOSCtx h, Subtable *sub);
static void checkBaseAnchorConflict(hotCtx g, BaseGlyphRec *baseGlyphArray, long recCnt, char *fileName, int isMarkToLigature);
static int findMarkClassIndex(SubtableInfo *si, GNode *markNode);
static int addMarkClass(hotCtx g, SubtableInfo *si, GNode *markNode, char *filename, int lineNum);
static void GPOSAdCursive(hotCtx g, SubtableInfo *si, GNode *targ, int anchorCount, AnchorMarkInfo *anchorMarkInfo, char *fileName, long lineNum);
static void GPOSAddMark(hotCtx g, SubtableInfo *si, GNode *targ, int anchorCount, AnchorMarkInfo *anchorMarkInfo, char *fileName, long lineNum);
static void fillMarkToBase(hotCtx g, GPOSCtx h);
static void writeMarkToBase(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeMarkToBase(hotCtx g, Subtable *sub);
static void fillMarkToLigature(hotCtx g, GPOSCtx h);
static void writeMarkToLigature(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeMarkToLigature(hotCtx g, Subtable *sub);
static void fillCursive(hotCtx g, GPOSCtx h);
static void writeCursive(hotCtx g, GPOSCtx h, Subtable *sub);
static void freeCursive(hotCtx g, Subtable *sub);
static void createAnonLookups(hotCtx g, GPOSCtx h);
static void setAnonLookupIndices(hotCtx g, GPOSCtx h);
static SubtableInfo *addAnonPosRule(hotCtx g, GPOSCtx h, SubtableInfo *cur_si, unsigned short lkpType, GNode *targ);

static void GPOSAddSingle(hotCtx g, SubtableInfo *si, GNode *targ,
                          int xPlacement, int yPlacement, int xAdvance, int yAdvance);

#if HOT_DEBUG
/* Initialize sub. Not needed; just for debug */

static void initSingle(void *ctx, long count, SingleRec *s) {
	long i;
	for (i = 0; i < count; i++) {
		s->span.valFmt = s->span.valRec = 0;
		s++;
	}
	return;
}

static void dumpSingles(hotCtx g) {
	GPOSCtx h = g->ctx.GPOS;
	int i;

	fprintf(stderr,
	        ">GPOS: dumpSingles [%ld]            valFmt span.valFmt  span.valRec\n",
	        h->new.single.cnt);
	for (i = 0; i < h->new.single.cnt; i++) {
		SingleRec *s = &h->new.single.array[i];

		fprintf(stderr, "[%2d] ", i);
		featGlyphDump(g, s->gid, -1, 1);
		fprintf(stderr, "\t%4d %4d %4d %4d",
		        s->xPla, s->yPla, s->xAdv, s->yAdv);
		fprintf(stderr, " %5x ", s->valFmt);
		if (s->span.valFmt == 0) {
			fprintf(stderr, "       *");
		}
		else {
			fprintf(stderr, "   %5d", s->span.valFmt);
		}
		if (s->span.valRec == 0) {
			fprintf(stderr, "      *");
		}
		else {
			fprintf(stderr, "  %5d", s->span.valRec);
		}
		fprintf(stderr, "\n");
	}
}

static void kernRecDump(hotCtx g, GID glyph1, GID glyph2, short metricsCnt, short *values,
                        GNode *gclass1, GNode *gclass2,
                        unsigned cl1, unsigned cl2) {
	if (gclass1 == NULL) {
		featGlyphDump(g, glyph1, ' ', 1);
		featGlyphDump(g, glyph2, ' ', 1);
	}
	else {
		featGlyphClassDump(g, gclass1, -1, 1);
		fprintf(stderr, ":%u  ", cl1);
		featGlyphClassDump(g, gclass2, -1, 1);
		fprintf(stderr, ":%u  ", cl2);
	}
	if (metricsCnt == 1) {
		fprintf(stderr, "%hd\n", values[0]);
	}
	else {
		int i;
		fprintf(stderr, "<");
		for (i = 0; i < metricsCnt; i++) {
			fprintf(stderr, "%hd ", values[0]);
		}
		fprintf(stderr, ">\n");
	}
}

#endif

/* --------------------------- Standard Functions -------------------------- */

/* Element initializer */

static void anonSubtableInit(void *ctx, long count, SubtableInfo *si) {
	hotCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, si->rules, 50, 50);
		dnaINIT(g->dnaCtx, si->single, 500, 1000);
		dnaINIT(g->dnaCtx, si->markClassList, 8, 8);
		dnaINIT(g->dnaCtx, si->baseList, 300, 300);
		dnaINIT(g->dnaCtx, si->pairs, 1000, 500);
	#if HOT_DEBUG
		si->single.func = initSingle;
	#endif
		si++;
	}
	return;
}

void GPOSNew(hotCtx g) {
	GPOSCtx h = MEM_NEW(g, sizeof(struct GPOSCtx_));

	h->new.script = h->new.language = h->new.feature = TAG_UNDEF;

	dnaINIT(g->dnaCtx, h->new.rules, 50, 200);
	dnaINIT(g->dnaCtx, h->new.single, 500, 1000);
	dnaINIT(g->dnaCtx, h->new.markClassList, 8, 8);
	dnaINIT(g->dnaCtx, h->new.baseList, 300, 300);
	dnaINIT(g->dnaCtx, h->new.pairs, 1000, 500);
#if HOT_DEBUG
	h->new.single.func = initSingle;
#endif
	h->offset.subtable = h->offset.extension = h->offset.extensionSection = 0;
	dnaINIT(g->dnaCtx, h->values, 1000, 500);
	dnaINIT(g->dnaCtx, h->subtables, 10, 10);
	dnaINIT(g->dnaCtx, h->anonSubtable, 3, 10);
	h->anonSubtable.func = anonSubtableInit;
	dnaINIT(g->dnaCtx, h->posLookup, 25, 100);
	dnaINIT(g->dnaCtx, h->prod, 20, 100);

	h->startNewPairPosSubtbl = 0;
	dnaINIT(g->dnaCtx, h->classDef[0].classInfo, 200, 500);
	dnaINIT(g->dnaCtx, h->classDef[0].cov, 50, 100);
	dnaINIT(g->dnaCtx, h->classDef[1].classInfo, 200, 500);
	dnaINIT(g->dnaCtx, h->classDef[1].cov, 50, 100);

	h->featNameID = 0;
	h->maxContext = 0;
	h->hadError = 0;
	h->otl = NULL;

	/* Link contexts */
	h->g = g;
	g->ctx.GPOS = h;
}

int GPOSFill(hotCtx g) {
	int i;
	GPOSCtx h = g->ctx.GPOS;

	if (h->subtables.cnt == 0) {
		return 0;
	}

	if (h->hadError) {
		hotMsg(g, hotFATAL, "aborting because of errors");
	}

#if HOT_FEAT_SUPPORT
	createAnonLookups(g, h);
#endif /* HOT_FEAT_SUPPORT */

	/* Add OTL features */
	for (i = 0; i < h->subtables.cnt; i++) {
		Subtable *sub = &h->subtables.array[i];
		int isExt = sub->extension.use;

		otlSubtableAdd(g, h->otl, sub->script, sub->language, sub->feature,
		               isExt ? GPOSExtension : sub->lkpType,
		               sub->lkpFlag, sub->markSetIndex,
		               isExt ? sub->lkpType : 0,
		               IS_REF_LAB(sub->label) ? 0 : sub->offset,
		               sub->label,
		               (unsigned short)(IS_REF_LAB(sub->label) ? 0 :
		                                isExt ? sub->extension.tbl->PosFormat
										: *(unsigned short *)sub->tbl),
		               (sub->lkpType == GPOSFeatureParam));
	}
	DF(1, (stderr, "### GPOS:\n"));

	otlTableFill(g, h->otl);

	h->offset.extensionSection = h->offset.subtable
		   						 + otlGetCoverageSize(h->otl)
		   						 + otlGetClassSize(h->otl);

#if HOT_DEBUG
	otlDumpSizes(g, h->otl, h->offset.subtable, h->offset.extension);
#endif /* HOT_DEBUG */

#if HOT_FEAT_SUPPORT
	/* setAnonLookupIndices marks as used not only the anonymous lookups, but also all lookups that were referenced from chain pos rules,
	   including the stand-alone lookups. This is why checkStandAloneTablRefs has to follow setAnonLookupIndices. */
	setAnonLookupIndices(g, h);
#endif /* HOT_FEAT_SUPPORT */

	checkStandAloneTablRefs(g, h->otl);

	OS_2SetMaxContext(g, h->maxContext);

	return 1;
}

void GPOSWrite(hotCtx g) {
	int i;
	GPOSCtx h = g->ctx.GPOS;

	/* Write OTL features */
	otlTableWrite(g, h->otl);

	/* Write main subtable section */
	for (i = 0; i < h->subtables.cnt; i++) {
		Subtable *sub = &h->subtables.array[i];

		if (IS_REF_LAB(sub->label)) {
			continue;
		}

		if (sub->extension.use) {
			writeExtension(g, h, sub);
			continue;
		}

		switch (sub->lkpType) {
			case GPOSSingle:
				writeSinglePos(g, h, sub);
				break;

			case GPOSPair:
				writePairPos(g, h, sub);
				break;

			case GPOSFeatureParam:
				writeFeatParam(h, sub);
				break;

			case GPOSChain:
				writeChainPos(g, h, sub);
				break;

			case GPOSCursive:
				writeCursive(g, h, sub);
				break;

			case GPOSMarkToBase:
				writeMarkToBase(g, h, sub);
				break;

			case GPOSMarkToLigature:
				writeMarkToLigature(g, h, sub);
				break;

			case GPOSMarkToMark:
				writeMarkToBase(g, h, sub);
				break;

			case GPOSContext:
				break;
		}
	}

	/* Write main coverage and class tables */
	otlCoverageWrite(g, h->otl);
	otlClassWrite(g, h->otl);

	/* Write extension subtables section. Each subtable is immediatedly
	   followed by its coverages and classes */
	for (i = 0; i < h->subtables.cnt; i++) {
		Subtable *sub = &h->subtables.array[i];

		if (IS_REF_LAB(sub->label) || sub->extension.use == 0) {
			continue;
		}

		switch (sub->lkpType) {
			case GPOSSingle:
				writeSinglePos(g, h, sub);
				break;

			case GPOSPair:
				writePairPos(g, h, sub);
				break;

			case GPOSChain:
				writeChainPos(g, h, sub);
				break;

			case GPOSCursive:
				writeCursive(g, h, sub);
				break;

			case GPOSMarkToBase:
				writeMarkToBase(g, h, sub);
				break;

			case GPOSMarkToLigature:
				writeMarkToLigature(g, h, sub);
				break;

			case GPOSMarkToMark:
				writeMarkToBase(g, h, sub);
				break;

			case GPOSContext:
				break;
		}
	}
}

void GPOSReuse(hotCtx g) {
	GPOSCtx h = g->ctx.GPOS;
	int i;

	h->new.script = h->new.language = h->new.feature = TAG_UNDEF;

	/* Free subtables */
	for (i = 0; i < h->subtables.cnt; i++) {
		Subtable *sub = &h->subtables.array[i];

		if (IS_REF_LAB(sub->label)) {
			continue;
		}

		if (sub->extension.use) {
			freeExtension(g, h, sub); /* In addition to the following frees: */
		}
		switch (sub->lkpType) {
			case GPOSSingle:
				freeSinglePos(g, sub);
				break;

			case GPOSPair:
				freePairPos(g, sub);
				break;

			case GPOSFeatureParam:
				freeFeatParam(g, sub);
				break;

			case GPOSCursive:
				freeCursive(g, sub);
				break;

			case GPOSMarkToBase:
				freeMarkToBase(g, sub);
				break;

			case GPOSMarkToLigature:
				freeMarkToLigature(g, sub);
				break;

			case GPOSMarkToMark:
				freeMarkToBase(g, sub);
				break;

			case GPOSContext:
				break;

			case GPOSChain:
				freeChain(g, h, sub);
				break;
		}
	}

	h->new.rules.cnt = 0;
	h->new.markClassList.cnt = 0;
	h->new.baseList.cnt = 0;
	h->new.single.cnt = 0;
	h->new.pairs.cnt = 0;
	h->offset.subtable = h->offset.extension = h->offset.extensionSection = 0;
	h->values.cnt = 0;
	h->subtables.cnt = 0;

	h->new.parentLkpType = 0;
	h->featNameID = 0;
	h->anonSubtable.cnt = 0;
	h->posLookup.cnt = 0;

	h->maxContext = 0;
	h->hadError = 0;
	otlTableReuse(g, h->otl);
}

void GPOSFree(hotCtx g) {
	long i;
	GPOSCtx h = g->ctx.GPOS;

	dnaFREE(h->new.rules);
	dnaFREE(h->new.markClassList);
	for (i = 0; i < h->new.baseList.cnt; i++) {
		dnaFREE(h->new.baseList.array[i].anchorMarkInfo);
	}
	dnaFREE(h->new.baseList);
	dnaFREE(h->new.single);
	dnaFREE(h->new.pairs);
	dnaFREE(h->values);
	dnaFREE(h->subtables);
	/* anonSubtable has an init function, so uou need to deallcoate size number of rules */
	for (i = 0; i < h->anonSubtable.size; i++) {
		dnaFREE(h->anonSubtable.array[i].rules);
	}
	dnaFREE(h->anonSubtable);
	dnaFREE(h->posLookup);
	dnaFREE(h->prod);

	dnaFREE(h->classDef[0].classInfo);
	dnaFREE(h->classDef[0].cov);
	dnaFREE(h->classDef[1].classInfo);
	dnaFREE(h->classDef[1].cov);

	otlTableFree(g, h->otl);
	h->otl = NULL;
	MEM_FREE(g, g->ctx.GPOS);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Begin new feature (can be called multiple times for same feature) */

void GPOSFeatureBegin(hotCtx g, Tag script, Tag language, Tag feature) {
	GPOSCtx h = g->ctx.GPOS;

	DF(2, (stderr, "\n"));
#if 1
	DF(1, (stderr, "{ GPOS '%c%c%c%c', '%c%c%c%c', '%c%c%c%c'\n",
		TAG_ARG(script), TAG_ARG(language), TAG_ARG(feature)));
#endif

	if (h->new.script != script) {
		h->new.script = script;
	}
	if (h->new.language != language) {
		h->new.language = language;
	}
	if (h->new.feature != feature) {
		h->new.feature = feature;
	}
}

static void reuseClassDefs(GPOSCtx h) {
	int i;
	for (i = 0; i < 2; i++) {
		int j;
		ClassDef *cdef = &h->classDef[i];
		for (j = 0; j < cdef->classInfo.cnt; j++) {
			featRecycleNodes(h->g, cdef->classInfo.array[j].gc);
		}
		cdef->classInfo.cnt = 0;
		cdef->cov.cnt = 0;
	}
}

/* Start new subtable explicitly. */

static void startNewSubtable(hotCtx g) {
	GPOSCtx h = g->ctx.GPOS;
	Subtable *sub = h->new.sub = dnaNEXT(h->subtables);

	sub->script = h->new.script;
	sub->language = h->new.language;
	sub->feature = h->new.feature;
	sub->lkpType = h->new.lkpType;
	sub->lkpFlag = h->new.lkpFlag;
	sub->markSetIndex = h->new.markSetIndex;
	sub->label = h->new.label;
	sub->offset = h->offset.subtable;

	sub->extension.use = h->new.useExtension;
	if (h->new.useExtension && !IS_REF_LAB(h->new.label)) {
		sub->extension.otl = otlTableNew(g);
		sub->extension.offset = h->offset.extension; /* Not needed */
		sub->extension.tbl = fillExtensionPos(g, h, h->new.lkpType);
	}
	else {
		sub->extension.otl = NULL;
		sub->extension.offset = 0;
		sub->extension.tbl = NULL;
	}

	reuseClassDefs(h);
}

#if HOT_DEBUG

static void rulesDump(GPOSCtx h) {
	long i;

	fprintf(stderr, "# Dump lookupType %d rules:\n", h->new.lkpType);
	for (i = 0; i < h->new.rules.cnt; i++) {
		PosRule *rule = &h->new.rules.array[i];

		fprintf(stderr, "  [%ld] ", i);
		featPatternDump(h->g, rule->targ, ' ', 1);
	}
}

#endif

/* Begin new lookup */

void GPOSLookupBegin(hotCtx g, unsigned lkpType, unsigned lkpFlag, Label label,
					short useExtension, unsigned short useMarkSetIndex) {
	GPOSCtx h = g->ctx.GPOS;
	SubtableInfo *newsi = &h->new;

	DF(2, (stderr,
			" { GPOS lkpType=%s%d lkpFlag=%d label=%x\n",
			useExtension ? "EXTENSION:" : "", lkpType, lkpFlag, label));

	newsi->useExtension = useExtension;
	newsi->lkpType = lkpType;
	newsi->lkpFlag = lkpFlag;
	newsi->markSetIndex = useMarkSetIndex;
	newsi->label = label;
	newsi->parentLkpType = 0;

	/* These are accumulators for all rules in a single lookup; they may in
	   fact be broken up into several subtables within this module */
	newsi->single.cnt = 0;
	newsi->pairs.cnt = 0;
	newsi->pairFmt = 0;
	newsi->pairValFmt1 = 0;
	newsi->pairValFmt2 = 0;
	newsi->markClassList.cnt = 0;
	newsi->baseList.cnt = 0;
	newsi->rules.cnt = 0;
}

/* Recycle class kern pairs */

static void recyclePairs(GPOSCtx h) {
	long i;
	for (i = 0; i < h->new.pairs.cnt; i++) {
		KernRec *pair = &h->new.pairs.array[i];
		featRecycleNodes(h->g, pair->first.gcl);
		featRecycleNodes(h->g, pair->second.gcl);
	}
}

/* End lookup */

void GPOSLookupEnd(hotCtx g, Tag feature) {
	GPOSCtx h = g->ctx.GPOS;
	DF(2, (stderr, " } GPOS\n"));

	/* Return if simply a reference */
	if (IS_REF_LAB(h->new.label)) {
		startNewSubtable(g);
		return;
	}

	if (g->hadError) {
		return;
	}

	if (feature != h->new.feature) {
		/* This happens when a feature definition is empty. */
		hotMsg(g, hotFATAL, "Empty lookup in feature '%c%c%c%c'", TAG_ARG(feature));
	}

	if (h->otl == NULL) {
		/* Allocate table if not done so already */
		h->otl = otlTableNew(g);
	}

	switch (h->new.lkpType) {
		case GPOSSingle:
			if (h->new.single.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS Single lookup");
			}
			fillSinglePos(g, h);
			break;

		case GPOSPair:
			if (h->new.pairs.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS Pair lookup");
			}
			fillPairPos(g, h);
			if (h->new.pairFmt == 2) {
				recyclePairs(h);
				reuseClassDefs(h);
			}
			break;

		case GPOSFeatureParam:
			switch (h->new.feature) {
				case size_:
					fillSizeFeature(g, h,  h->new.sub);
					break;

				default:
					hotMsg(g, hotFATAL,
							"feature '%c%c%c%c'with FeatureParameter is not supported.",
							TAG_ARG(h->new.feature));
			}
			break;

		case GPOSChain:
			if (h->new.rules.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS Contextual Positioning lookup");
			}
			fillChain(g, h);
			break;

		case GPOSCursive:
			if (h->new.baseList.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS Cursive lookup");
			}
			fillCursive(g, h);
			break;

		case GPOSMarkToBase:
			if (h->new.baseList.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS MarkToBase lookup");
			}
			fillMarkToBase(g, h);
			break;

		case GPOSMarkToLigature:
			if (h->new.baseList.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS MarkToLigature lookup");
			}
			fillMarkToLigature(g, h);
			break;

		case GPOSMarkToMark:
			if (h->new.baseList.cnt == 0) {
				hotMsg(g, hotFATAL, "Empty GPOS MarkToMark lookup");
			}
			fillMarkToBase(g, h);
			break;

		case GPOSContext:
			hotMsg(g, hotFATAL, "unsupported GPOS lkpType <%d>\n", h->new.lkpType);
			break;

		default:
			hotMsg(g, hotFATAL, "unknown GPOS lkpType <%d>\n", h->new.lkpType);
	}

	if (h->offset.subtable > 0xFFFF) {
		hotMsg(g, hotFATAL, "GPOS feature '%c%c%c%c' causes overflow of offset"
				" to a subtable (0x%lx)", TAG_ARG(h->new.feature),
				h->offset.subtable);
	}

	if (h->startNewPairPosSubtbl != 0) {
		h->startNewPairPosSubtbl = 0;
	}
}

/* Performs no action but brackets feature calls */
void GPOSFeatureEnd(hotCtx g) {
	DF(2, (stderr, "} GPOS\n"));
}

/* -------------------------- Shared Declarations -------------------------- */

/* --- ValueRecord --- */
#define ValueXPlacement     (1 << 0)
#define ValueYPlacement     (1 << 1)
#define ValueXAdvance       (1 << 2)
#define ValueYAdvance       (1 << 3)
#define ValueXPlaDevice     (1 << 4)
#define ValueYPlaDevice     (1 << 5)
#define ValueXAdvDevice     (1 << 6)
#define ValueYAdvDevice     (1 << 7)

/* MM extensions */
#define ValueXIdPlacement   (1 << 8)
#define ValueYIdPlacement   (1 << 9)
#define ValueXIdAdvance     (1 << 10)
#define ValueYIdAdvance     (1 << 11)

#define VAL_REC_UNDEF       (-1)
typedef long ValueRecord;   /* Stores index into h->values, which is read at write time. If -1, then write 0; */



/* --- Anchor Table --- */
typedef struct {
	unsigned short AnchorFormat;      /* =1 */
	short XCoordinate;
	short YCoordinate;
} AnchorFormat1;

typedef struct {
	unsigned short AnchorFormat;      /* =2 */
	short XCoordinate;
	short YCoordinate;
	unsigned short AnchorPoint;
} AnchorFormat2;

typedef struct {
	unsigned short AnchorFormat;      /* =3 */
	short XCoordinate;
	short YCoordinate;
	Offset XDeviceTable;
	Offset YDeviceTable;
} AnchorFormat3;


/* --- Mark Array --- */
typedef struct {
	GID gid; /* not part of the font data - used to sort recs when building table. */
	LOffset anchor; /* not part of the font data - used while building table. */
	unsigned short Class;
} MarkRecord;

typedef struct {
	unsigned short MarkCount;
	MarkRecord *MarkRecord;           /* [MarkCount] */
} MarkArray;



/* --- ValueFormat and ValueRecord utility functions --- */

static unsigned int isVertFeature(Tag featTag)
{
    int isVert = 0;
    if ( (featTag == vkrn_) || (featTag == vpal_)  || (featTag == vhal_)  || (featTag == valt_) )
        isVert = 1;
    return isVert;
}

/* Calculate value format */

static unsigned makeValueFormat(hotCtx g, int xPla, int yPla, int xAdv, int yAdv) {
    unsigned val = 0;
    GPOSCtx h = g->ctx.GPOS;

     if (xPla) {
		val |= IS_MM(g) ? ValueXIdPlacement : ValueXPlacement;
	}
	if (yPla) {
		val |= IS_MM(g) ? ValueYIdPlacement : ValueYPlacement;
	}
	if (yAdv) {
		val |= IS_MM(g) ? ValueYIdAdvance : ValueYAdvance;
	}
    
    if (xAdv) {
        if  ((val == 0) && (isVertFeature(h->new.feature)))
            val =  (IS_MM(g) ? ValueYIdAdvance: ValueYAdvance);
        else
            val |= IS_MM(g) ? ValueXIdAdvance : ValueXAdvance;
    }

        return val;
}

static void recordValues(GPOSCtx h, unsigned valFmt, int xPla, int yPla, int xAdv, int yAdv) {
	if (valFmt == 0) {
		return;
	}
	if (valFmt & ValueXPlacement || valFmt & ValueXIdPlacement) {
		*dnaNEXT(h->values) = xPla;
	}
	if (valFmt & ValueYPlacement || valFmt & ValueYIdPlacement) {
		*dnaNEXT(h->values) = yPla;
	}
	if (valFmt & ValueXAdvance   || valFmt & ValueXIdAdvance) {
		*dnaNEXT(h->values) = xAdv;
	}
	if (valFmt & ValueYAdvance   || valFmt & ValueYIdAdvance) {
		*dnaNEXT(h->values) = yAdv;
	}
}

/* Counts bits set in valFmt */

static int numValues(unsigned valFmt) {
	int i = 0;
	while (valFmt) {
		i++;
		valFmt &= valFmt - 1;       /* Remove least significant set bit */
	}
	return i;
}

/* Write value record */

static void writeValueRecord(GPOSCtx h, unsigned valFmt, ValueRecord i) {
	while (valFmt) {
		/* Write 1 field per valFmt bit, if index is valid */
		OUT2((short)((i == VAL_REC_UNDEF) ? 0 : h->values.array[i++]));
		valFmt &= valFmt - 1;       /* Remove least significant set bit */
	}
}

/* --------------------------- Single Adjustment --------------------------- */

typedef struct {
	unsigned short PosFormat;      /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short ValueFormat;
	ValueRecord Value;
} SinglePosFormat1;
#define SINGLE1_SIZE(nVal)  (uint16 * 3 + int16 * (nVal))

typedef struct {
	unsigned short PosFormat;      /* =2 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short ValueFormat;
	unsigned short ValueCount;
	ValueRecord *Value;            /* [ValueCount] */
} SinglePosFormat2;
#define SINGLE2_SIZE(valCnt, nVal)  (uint16 * 4 + int16 * (valCnt) * (nVal))


/* The input array has either 2 or 4 elements ( 2 if the second is 0, else 4),
 * and the output array has 5 elements. The first 2 are in the same order
 * as the feature file parameters. The last two are in the same order, but one
 * index value less, as the input record does not contain the kSizeMenuNameID.
 * The kSizeMenuNameID element of the output record
 * is filled in only when fillSizeFeature() is called.
 */
void GPOSAddSize(hotCtx g, short *params, unsigned short numParams) {
	GPOSCtx h = g->ctx.GPOS;
	Subtable *sub;
	FeatureParameterFormat *feat_param = NULL;
	unsigned short *outParamPtr;

	int param_offset = sizeof(feat_param->numParams);
	int outParamSize = (uint16 * 5) + param_offset;  /* output record size in bytes */

	if ((params[kSizeSubFamilyID] != 0) && (numParams != 4)) {
		hotMsg(g, hotFATAL, "size feature must have 4 parameters if sub family ID code is non-zero!");
	}
	else if ((params[kSizeSubFamilyID] == 0) && (numParams != 4) && (numParams != 2)) {
		hotMsg(g, hotFATAL, "size feature must have 4 or 2 parameters if sub family code is zero!");
	}

	startNewSubtable(g);
	sub = h->new.sub;

	feat_param = MEM_NEW(g, outParamSize);
	feat_param->numParams = 5; /* The output record has an additional field. */
	outParamPtr = (unsigned short *)&(feat_param->params[param_offset]);

	outParamPtr[kSizeOpticalSize] = params[kSizeOpticalSize]; /* decipoint size, in decipoints */
	outParamPtr[kSizeSubFamilyID] = params[kSizeSubFamilyID]; /* subfamily code. If zero, rest must be zero. */
	if (params[kSizeSubFamilyID] == 0) {
		outParamPtr[kSizeMenuNameID] = 0;
		outParamPtr[kSizeLowEndRange] = 0;
		outParamPtr[kSizeHighEndRange] = 0;
	}
	else {
		outParamPtr[kSizeMenuNameID] = 0xFFFF;  /* put a bad value in for the name ID value so we'll know
		                                           if it doesn't get overwritten. */
		outParamPtr[kSizeLowEndRange] =  params[kSizeLowEndRange - 1]; /* exclusive low end of size range, in decipoints */
		outParamPtr[kSizeHighEndRange] = params[kSizeHighEndRange - 1]; /* inclusive high end of size range, in decipoints */
	}

	sub->tbl = feat_param;
	h->offset.subtable += uint16 * 5; /* When we write it out, we leave off the initial numParams shart value.*/
}

void GPOSSetSizeMenuNameID(hotCtx g, unsigned short nameID) {
	GPOSCtx h = g->ctx.GPOS;
	h->featNameID = nameID;
}

/* Targ may be a class, as a convenience. Input GNodes are recycled. */

static void GPOSAddSingle(hotCtx g, SubtableInfo *si, GNode *targ, int xPla, int yPla, int xAdv, int yAdv) {
	GNode *p;
    unsigned valFmt;
    GPOSCtx h = g->ctx.GPOS;
   
    valFmt = makeValueFormat(g, xPla, yPla, xAdv, yAdv);

	if (g->hadError) {
		return;
	}

	for (p = targ; p != NULL; p = p->nextCl) {
		SingleRec *single;
		if (p->flags & FEAT_MISC) {
			continue; /* skip rules that are duplicates of others that are in this lookup. This is set in addAnonPosRule */
		}
		single = dnaNEXT(si->single);

		single->gid = p->gid;
		single->xPla = xPla;
		single->yPla = yPla;
        if (isVertFeature(h->new.feature) && (valFmt == ValueYAdvance) && (yAdv == 0))
             {
                 single->xAdv = 0;
                 single->yAdv = xAdv;
             }
             else
             {
                 single->xAdv = xAdv;
                 single->yAdv = yAdv;
             }
		single->valFmt = valFmt;
#if HOT_DEBUG
		if (DF_LEVEL >= 2) {
			fprintf(stderr, "  * GPOSSingle ");
			featGlyphDump(g, p->gid, ' ', 1);
			fprintf(stderr, " %d %d %d %d\n", xPla, yPla, xAdv, yAdv);
		}
#endif
	}
}

/* Compare valfmt, then gid */

static int CDECL cmpSingle(const void *first, const void *second) {
	SingleRec *a = (SingleRec *)first;
	SingleRec *b = (SingleRec *)second;

	if (a->yAdv < b->yAdv) {
		return -1;
	}
	else if (a->yAdv > b->yAdv) {
		return 1;
	}
	else if (a->xAdv < b->xAdv) {
		return -1;
	}
	else if (a->xAdv > b->xAdv) {
		return 1;
	}
	else if (a->yPla < b->yPla) {
		return -1;
	}
	else if (a->yPla > b->yPla) {
		return 1;
	}
	else if (a->xPla < b->xPla) {
		return -1;
	}
	else if (a->xPla > b->xPla) {
		return 1;
	}
	else if (a->gid < b->gid) {
		return -1;
	}
	else if (a->gid > b->gid) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Compare gid, then valfmt */

static int CDECL cmpSingleGID(const void *first, const void *second) {
	SingleRec *a = (SingleRec *)first;
	SingleRec *b = (SingleRec *)second;

	if (a->gid < b->gid) {
		return -1;
	}
	else if (a->gid > b->gid) {
		return 1;
	}
	else if (a->yAdv < b->yAdv) {
		return -1;
	}
	else if (a->yAdv > b->yAdv) {
		return 1;
	}
	else if (a->xAdv < b->xAdv) {
		return -1;
	}
	else if (a->xAdv > b->xAdv) {
		return 1;
	}
	else if (a->yPla < b->yPla) {
		return -1;
	}
	else if (a->yPla > b->yPla) {
		return 1;
	}
	else if (a->xPla < b->xPla) {
		return -1;
	}
	else if (a->xPla > b->xPla) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Check for duplicate target glyph */

static void checkAndSortSinglePos(hotCtx g, GPOSCtx h) {
	long i;
	int nDuplicates = 0;

	/* Sort into increasing target glyph id order */
	qsort(h->new.single.array, h->new.single.cnt, sizeof(SingleRec),
		  cmpSingleGID);

	for (i = 1; i < h->new.single.cnt; i++) {
		SingleRec *curr = &h->new.single.array[i];
		SingleRec *prev = curr - 1;

		if (curr->gid == prev->gid) {
			if (cmpSingle(curr, prev) == 0) {
				featGlyphDump(g, curr->gid, '\0', 0);
				hotMsg(g, hotNOTE, "Removing duplicate single positioning "
					   "in '%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
					   g->note.array);

				/* Set prev duplicate to NULL */
				prev->gid = GID_UNDEF;
				nDuplicates++;
			}
			else {
				featGlyphDump(g, curr->gid, '\0', 0);
				hotMsg(g, hotFATAL, "Duplicate single positioning glyph with "
					   "different values in '%c%c%c%c' feature: %s",
					   TAG_ARG(h->new.feature), g->note.array);
			}
		}
	}

	if (nDuplicates > 0) {
		/* Duplicates sink to the bottom */
		qsort(h->new.single.array, h->new.single.cnt, sizeof(SingleRec),
			  cmpSingleGID);
		h->new.single.cnt -= nDuplicates;
	}

	/* Now sort by valfmt (then gid) */
	qsort(h->new.single.array, h->new.single.cnt, sizeof(SingleRec), cmpSingle);
}

static void prepSinglePos(hotCtx g) {
	GPOSCtx h = g->ctx.GPOS;
	int Fmt;
	int fmt;

	checkAndSortSinglePos(g, h);

	/* Calculate value format and value record spans */
	dnaNEXT(h->new.single)->valFmt = -1;
	Fmt = 0;
	for (fmt = 1; fmt < h->new.single.cnt; fmt++) {
		if (h->new.single.array[fmt].valFmt != h->new.single.array[Fmt].valFmt) {
			/* Value format change. Now calc valRec spans. */
			int rec;
			int Rec = Fmt;

			for (rec = Rec + 1; rec <= fmt; rec++) {
				SingleRec *r = &h->new.single.array[rec];
				SingleRec *R = &h->new.single.array[Rec];

				if (rec == fmt || r->xPla != R->xPla || r->xAdv != R->xAdv ||
						r->yPla != R->yPla || r->yAdv != R->yAdv) {
					/* val rec change */
					h->new.single.array[Rec].span.valRec = rec;
					Rec = rec;
				}
			}
			h->new.single.array[Fmt].span.valFmt = fmt;
			Fmt = fmt;
		}
	}
	h->new.single.array[Fmt].span.valFmt = fmt;
	h->new.single.cnt--;
}

static void fillSizeFeature(hotCtx g, GPOSCtx h, Subtable *sub) {
	FeatureParameterFormat *feat_param;
	unsigned short *params;
	int paramOffset;

	feat_param = (FeatureParameterFormat *)sub->tbl;
	paramOffset = sizeof(feat_param->numParams);
	params = (unsigned short *)&feat_param->params[paramOffset];

	/* if the kSizeSubFamilyID field is non-zero, then we need to fill in the
	 * kSizeMenuNameID field in the parameters array. This value may be zero;
	 * if so, then there is no special sub family menu name.
	 */
	if ((sub->feature == size_) && (params[kSizeSubFamilyID] != 0)) {
		unsigned short nameid =  h->featNameID;

		params[kSizeMenuNameID] = nameid;

		/* If there is a sub family menu name id, check if the default names are present,
		 * and complain if they are not.
		 */
		if (nameid != 0) {
		    unsigned short nameIDPresent = nameVerifyDefaultNames(g, nameid);
		    if (nameIDPresent && nameIDPresent & MISSING_WIN_DEFAULT_NAME) {
			hotMsg(g, hotFATAL, "Missing Windows default name for for feature name  nameid %i",  nameid);
		    }
		}
	}
	else {
		params[kSizeMenuNameID] = 0;
	}
}

static LOffset fillSinglePos1(hotCtx g, int iStart, int iEnd, int simulate) {
	GPOSCtx h = g->ctx.GPOS;
	otlTbl otl;
	Subtable *sub;
	int i;
	SinglePosFormat1 *fmt;
	SingleRec *s = &h->new.single.array[iStart];
	unsigned short valfmt = s->valFmt;
	LOffset size = SINGLE1_SIZE(numValues(valfmt));

	if (simulate) {
		return size;
	}

	fmt = MEM_NEW(g, sizeof(SinglePosFormat1));
	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	otlCoverageBegin(g, otl);
	for (i = iStart; i < iEnd; i++) {
		otlCoverageAddGlyph(g, otl, h->new.single.array[i].gid);
	}

	fmt->PosFormat = 1;
	fmt->ValueFormat = valfmt;
	fmt->Value = h->values.cnt;
	recordValues(h, s->valFmt, s->xPla, s->yPla, s->xAdv, s->yAdv);

	fmt->Coverage = otlCoverageEnd(g, otl); /* Adjusted later */
	if (sub->extension.use) {
		fmt->Coverage += size;              /* Final value */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
	return size;
}

static LOffset fillSinglePos2(hotCtx g, int iStart, int iEnd, int simulate) {
	GPOSCtx h = g->ctx.GPOS;
	otlTbl otl;
	Subtable *sub;
	int i;
	SinglePosFormat2 *fmt;
	SingleRec *s = &h->new.single.array[iStart];
	unsigned short valfmt = s->valFmt;
	unsigned short valcnt = iEnd - iStart;
	LOffset size = SINGLE2_SIZE(valcnt, numValues(valfmt));

	if (simulate) {
		return size;
	}

	/* Sort subrange by GID */
	qsort(&h->new.single.array[iStart], iEnd - iStart, sizeof(SingleRec),
		  cmpSingleGID);

	fmt = MEM_NEW(g, sizeof(SinglePosFormat2));
	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	fmt->PosFormat = 2;
	fmt->ValueFormat = valfmt;
	fmt->ValueCount = valcnt;
	fmt->Value = MEM_NEW(g, fmt->ValueCount * sizeof(ValueRecord));

	otlCoverageBegin(g, otl);
	for (i = iStart; i < iEnd; i++) {
		s = &h->new.single.array[i];
		otlCoverageAddGlyph(g, otl, s->gid);

		fmt->Value[i - iStart] = h->values.cnt;
		recordValues(h, s->valFmt, s->xPla, s->yPla, s->xAdv, s->yAdv);
	}

	fmt->Coverage = otlCoverageEnd(g, otl); /* Adjusted later */
	if (sub->extension.use) {
		fmt->Coverage += size;              /* Final value */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
	return size;
}

/* Fill whole lookup with fmt 1 subtables */

static LOffset fillAllSinglePos1(hotCtx g, GPOSCtx h, int simulate, int *nSub) {
	int iFmt;
	SingleRec *sFmt;
	LOffset totsize = 0;

	if (nSub != NULL) {
		*nSub = 0;
	}
	for (iFmt = 0; iFmt < h->new.single.cnt; iFmt = sFmt->span.valFmt) {
		int iRec;
		SingleRec *sRec;

		sFmt = &h->new.single.array[iFmt];
		for (iRec = iFmt; iRec < sFmt->span.valFmt; iRec = sRec->span.valRec) {
			LOffset size;

			sRec = &h->new.single.array[iRec];
			if (nSub != NULL) {
				(*nSub)++;
			}
			size = fillSinglePos1(g, iRec, sRec->span.valRec, simulate);
			totsize += size;
		}
	}
	return totsize;
}

/* Fill whole lookup with fmt 2 subtables */

static LOffset fillAllSinglePos2(hotCtx g, GPOSCtx h, int simulate, int *nSub) {
	int iFmt;
	LOffset totsize = 0;
	unsigned nextSpanValFmt;

	if (nSub != NULL) {
		*nSub = 0;
	}
	for (iFmt = 0; iFmt < h->new.single.cnt; iFmt = nextSpanValFmt) {
		LOffset size;

		nextSpanValFmt = h->new.single.array[iFmt].span.valFmt;
		if (nSub != NULL) {
			(*nSub)++;
		}
		size = fillSinglePos2(g, iFmt, nextSpanValFmt, simulate);
		totsize += size;
	}
	return totsize;
}

/* Fill single postioning subtable(s). */

static void fillSinglePos(hotCtx g, GPOSCtx h) {
	LOffset size1;
	LOffset size2;
	int nSub1;
	int nSub2;

	if (h->new.single.cnt == 0) {
		return;
	}

	prepSinglePos(g);

#if HOT_DEBUG
	if (DF_LEVEL >= 2) {
		dumpSingles(g);
	}
#endif

	size1 = fillAllSinglePos1(g, h, 1 /* simulate */, &nSub1);
	size2 = fillAllSinglePos2(g, h, 1 /* simulate */, &nSub2);

#if 1
	DF(2, (stderr, "### singlePos1 size=%lu (%d subtables)\n", size1, nSub1));
	DF(2, (stderr, "### singlePos2 size=%lu (%d subtables)\n", size2, nSub2));
#endif

	/* Select subtable format */
	if (size1 < size2) {
		fillAllSinglePos1(g, h, 0 /* for real */, NULL);
	}
	else {
		fillAllSinglePos2(g, h, 0 /* for real */, NULL);
	}

	h->maxContext = MAX(h->maxContext, 1);
}

/* Write format 1 single positioning table */

static void writeSinglePos1(hotCtx g, GPOSCtx h, Subtable *sub) {
	SinglePosFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "Coverage", "single");

	OUT2(fmt->PosFormat);
	OUT2((Offset)fmt->Coverage);
	OUT2(fmt->ValueFormat);
	writeValueRecord(h, fmt->ValueFormat, fmt->Value);

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write format 2 single positioning table */

static void writeSinglePos2(hotCtx g, GPOSCtx h, Subtable *sub) {
	int i;
	SinglePosFormat2 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "Coverage", "single");

	OUT2(fmt->PosFormat);
	OUT2((Offset)fmt->Coverage);
	OUT2(fmt->ValueFormat);
	OUT2(fmt->ValueCount);
	for (i = 0; i < fmt->ValueCount; i++) {
		writeValueRecord(h, fmt->ValueFormat, fmt->Value[i]);
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void writeSinglePos(hotCtx g, GPOSCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			writeSinglePos1(g, h, sub);
			break;

		case 2:
			writeSinglePos2(g, h, sub);
			break;
	}
}

static void freeSinglePos1(hotCtx g, Subtable *sub) {
	SinglePosFormat1 *fmt = sub->tbl;
	MEM_FREE(g, fmt);
}

static void freeSinglePos2(hotCtx g, Subtable *sub) {
	SinglePosFormat2 *fmt = sub->tbl;
	MEM_FREE(g, fmt->Value);
	MEM_FREE(g, fmt);
}

static void freeSinglePos(hotCtx g, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			freeSinglePos1(g, sub);
			break;

		case 2:
			freeSinglePos2(g, sub);
			break;
	}
}

/* ---------------------------- Pair Adjustment ---------------------------- */

typedef struct {
	GID SecondGlyph;
	ValueRecord Value1;
	ValueRecord Value2;
} PairValueRecord;

typedef struct {
	unsigned short PairValueCount;
	PairValueRecord *PairValueRecord;  /* [PairValueCount] */
} PairSet;

#define PAIR_SET_SIZE(nRecs, nValues1, nValues2) \
	(uint16+(uint16+uint16*(nValues1)+uint16*(nValues2))*(nRecs))

typedef struct {
	unsigned short PosFormat;          /* =1 */
	LOffset Coverage;                   /* 32-bit for overflow check */
	unsigned short ValueFormat1;
	unsigned short ValueFormat2;
	unsigned short PairSetCount;
	DCL_OFFSET_ARRAY(PairSet, PairSet); /* [PairSetCount] */
} PairPosFormat1;
#define PAIR_POS1_SIZE(nPairSets)       (uint16 * 5 + uint16 * (nPairSets))

typedef struct {
	ValueRecord Value1;
	ValueRecord Value2;
} Class2Record;

typedef struct {
	Class2Record *Class2Record;         /* [Class2Count] */
} Class1Record;

typedef struct {
	unsigned short PosFormat;           /* =2 */
	LOffset Coverage;                    /* 32-bit for overflow check */
	unsigned short ValueFormat1;
	unsigned short ValueFormat2;
	LOffset ClassDef1;                   /* 32-bit for overflow check */
	LOffset ClassDef2;                   /* 32-bit for overflow check */
	unsigned short Class1Count;
	unsigned short Class2Count;
	Class1Record *Class1Record;         /* [Class1Count] */
} PairPosFormat2;
#define PAIR_POS2_SIZE(cl1Cnt, cl2Cnt, nVal) \
	(uint16 * 8 + (cl1Cnt) * (cl2Cnt) * (nVal) * uint16)

/* Break the subtable at this point. Return 0 if successful, else 1. */

int GPOSSubtableBreak(hotCtx g) {
	GPOSCtx h = g->ctx.GPOS;

	if (h->new.lkpType != GPOSPair) {
		/* xxx for now */
		return 1;
	}

	h->startNewPairPosSubtbl = 1;

	return 0;
}

static int CDECL matchFirstInClass(const void *first, const void *second, void *ctx) {
	const GID a = *(GID *)first;
	const GID b = ((ClassInfo *)second)->gc->gid;
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

static int CDECL matchGID(const void *first, const void *second, void *ctx) {
	const GID a = *(GID *)first;
	const GID b = *(GID *)second;
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

/* Checks to see if gc can be a valid member of classDef.
   Returns inx, which is -1 if gc cannot be a valid member of the classDef.
   If  > -1, then class is set to gc's class value. insert is set to 1 if
   gc needs to be inserted by a subsequent call to insertInClassDef(). */

static int validInClassDef(GPOSCtx h, int classDefInx, GNode *gc,
							unsigned short *class, int *insert) {
	size_t inx;
	GNode *p;
	ClassDef *cdef = &h->classDef[classDefInx];

	if (ctuLookup(&gc->gid, cdef->classInfo.array, cdef->classInfo.cnt,
				  sizeof(ClassInfo), matchFirstInClass, &inx, h)) {
		/* First glyph matches. Check to see if rest match. */
		GNode *q = cdef->classInfo.array[inx].gc->nextCl;

		for (p = gc->nextCl; p != NULL && q != NULL; p = p->nextCl, q = q->nextCl) {
			if (p->gid != q->gid) {
				return -1;
			}
		}
		if (p != NULL || q != NULL) {
			return -1;
		}

		*class = cdef->classInfo.array[inx].class;      /* gc already exists */
		*insert = 0;
		return (int)inx;
	}
	else {
		/* First glyph does not match. Check to see if any members of gc are
		   already in coverage */
		size_t tmp;

		for (p = gc; p != NULL; p = p->nextCl) {
			if (ctuLookup(&p->gid, cdef->cov.array, cdef->cov.cnt,
						  sizeof(GID), matchGID, &tmp, h)) {
				return -1;  /* One of the glyphs already exists in a class */
			}
		}
		/* Classes start numbering from 0 for ClassDef1, 1 for ClassDef2 */
		*class = (unsigned short)((classDefInx == 0) ? cdef->classInfo.cnt
								  : cdef->classInfo.cnt + 1);
		*insert = 1;
		return (int)inx;
	}
}

/* Inserts gc into the relevant class def. Input GNodes stored. */

static void insertInClassDef(GPOSCtx h, int classDefInx, GNode *gc, int inx,
							 unsigned class) {
	ClassDef *cdef = &h->classDef[classDefInx];
	ClassInfo *ci = INSERT(cdef->classInfo, inx);

	/* Classes start numbering from 0 for ClassDef1, 1 for ClassDef2 */
	ci->class = (classDefInx == 0) ? cdef->classInfo.cnt - 1
		: cdef->classInfo.cnt;
	ci->class = class;
	ci->gc = gc;

	/* Add gids to glyph accumulator */
	for (; gc != NULL; gc = gc->nextCl) {
		size_t gInx;
		if (!ctuLookup(&gc->gid, cdef->cov.array, cdef->cov.cnt,
					   sizeof(GID), matchGID, &gInx, h)) {
			*INSERT(cdef->cov, gInx) = gc->gid;
		}
		else {
			hotMsg(h->g, hotWARNING, "Duplicate glyph found in glyph class");
		}
	}
}

/* Add a specific kern pair */

static void addSpecPair(hotCtx g, GID first, GID second,  short metricsCnt1, short *values1, short metricsCnt2, short *values2) {
	GPOSCtx h = g->ctx.GPOS;
	KernRec *pair = dnaNEXT(h->new.pairs);
	int i;

	pair->first.gid = first;
	pair->second.gid = second;
	/* metricsCnt is either 1 or 4 */
	pair->metricsCnt1 = metricsCnt1;
	for (i=0; i < pair->metricsCnt1; i++) {
		pair->metricsRec1[i] = values1[i];
	}
	pair->metricsCnt2 = metricsCnt2;
	for (i=0; i < pair->metricsCnt2; i++) {
		pair->metricsRec2[i] = values2[i];
	}
#if HOT_DEBUG
	if (DF_LEVEL >= 2) {
		fprintf(stderr, "  * GPOSPair ");
		kernRecDump(g, first, second, metricsCnt2, values2, NULL, NULL, 0, 0);
	}
#endif
}

/* Add pair. first and second must already be sorted with duplicate gids
   removed. Input GNodes are stored; they are recycled in this function
   (if fmt1), or at GPOSLookupEnd(?) (if fmt2). */


void GPOSAddPair(hotCtx g, void *subtableInfo, GNode *first, GNode *second,  char *filename, int lineNum) {
	GPOSCtx h = g->ctx.GPOS;
	SubtableInfo * si = (SubtableInfo*)subtableInfo;
	short metricsCnt1 = 0;
	short metricsCnt2 = 0;
	short* values1 = NULL;
	short* values2 = NULL;
    int enumerate;
    int pairFmt;
    unsigned valFmt1 = 0;
    unsigned valFmt2 = 0;
    
	if (first->metricsInfo == NULL) {
		/* If the only metrics record is appplied to the second glyph,
		then this is shorthand for applying a single kern value to the first glyph.
		The parser enforces that if first->metricsInfo == null, then the 
		second value record must exist.*/
		first->metricsInfo = second->metricsInfo;
		second->metricsInfo = NULL;
		metricsCnt1 = first->metricsInfo->cnt;
		values1 = first->metricsInfo->metrics;
	}
	else {
		/* first->metricsInfo exists, but second->metricsInfo  may or may not exist */
		metricsCnt1 = first->metricsInfo->cnt;
		values1 = first->metricsInfo->metrics;
		if (second->metricsInfo != NULL) {
			metricsCnt2 = second->metricsInfo->cnt;
			values2 = second->metricsInfo->metrics;
		}
	}

	
	enumerate = first->flags & FEAT_ENUMERATE;

	/* The FEAT_GCLASS is essential for identifying a singleton gclass */
	pairFmt = ((first->flags & FEAT_GCLASS || second->flags & FEAT_GCLASS) && !enumerate) ? 2 : 1;


	if (first->metricsInfo->cnt == 1) {
		if (isVertFeature(h->new.feature)) {
			valFmt1 =  (unsigned short)(IS_MM(g) ? ValueYIdAdvance: ValueYAdvance);
		}
		else {
			valFmt1 =  (unsigned short) (IS_MM(g) ? ValueXIdAdvance: ValueXAdvance);
		}
	}
	else {
		valFmt1 = makeValueFormat(g, values1[0], values1[1], values1[2], values1[3]);
		if (valFmt1 == 0) {
			/* if someone has specified a value of <0 0 0 0>, then valFmt is 0.
			This will cause a subtable break, since the valFmt will differ from
			any non-zero record. Set the val fmt to the default value. */
			if (isVertFeature(h->new.feature)) {
				valFmt1 =  (unsigned short)(IS_MM(g) ? ValueYIdAdvance: ValueYAdvance);
			}
			else {
				valFmt1 =  (unsigned short) (IS_MM(g) ? ValueXIdAdvance: ValueXAdvance);
			}
		}
	}
	
	if (second->metricsInfo != NULL) {
		if (second->metricsInfo->cnt == 1) {
			if (isVertFeature(h->new.feature)) {
				valFmt2 =  (unsigned short)(IS_MM(g) ? ValueYIdAdvance: ValueYAdvance);
			}
			else {
				valFmt2 =  (unsigned short) (IS_MM(g) ? ValueXIdAdvance: ValueXAdvance);
			}
		}
		else {
			valFmt2 = makeValueFormat(g, values2[0], values2[1], values2[2], values2[3]);
			if (valFmt2 == 0) {
				/* if someone has specified a value of <0 0 0 0>, then valFmt is 0.
				This will cause a subtable break, since the valFmt wil differ from
				any non-zero record. Set the val fmt to the default value. */
				if (isVertFeature(h->new.feature)) {
					valFmt2 = (unsigned short)(IS_MM(g) ? ValueYIdAdvance: ValueYAdvance);
				}
				else {
					valFmt2 = (unsigned short) (IS_MM(g) ? ValueXIdAdvance: ValueXAdvance);
				}
			}
		}
	}
	if (si==NULL) {
		si = &h->new;
	}
	else {
		/* Changing valFmt causes a sub-table break. If the current valFmt can be represented by
		the previous valFmt, then use the previous valFmt. */
		if ((valFmt1 != si->pairValFmt1) && ((valFmt1 & si->pairValFmt1) == valFmt1)) {
			valFmt1 = si->pairValFmt1;
		}
		if ((valFmt2 != si->pairValFmt2) && ((valFmt2 & si->pairValFmt2) == valFmt2)) {
			valFmt2 = si->pairValFmt2;
		}
	}

	first->nextSeq = NULL; /* else featRecycleNodes  recycles the second mode as well. */
	second->nextSeq = NULL; /* else featRecycleNodes  recycles the second mode as well. */

	if (g->hadError) {
		return;
	}

	if (pairFmt != si->pairFmt /* First subtable in this lookup */
		|| valFmt1 != si->pairValFmt1
		|| valFmt2 != si->pairValFmt2
		|| h->startNewPairPosSubtbl /* Automatic or explicit break */) {
		GPOSCtx h = g->ctx.GPOS;

		if (h->startNewPairPosSubtbl != 0) {
			h->startNewPairPosSubtbl = 0;
		}

		if (si->pairFmt != 0) {
			/* Pause to create accumulated subtable */
			fillPairPos(g, h);
			if (si->pairFmt == 2) {
				recyclePairs(h);
				if (pairFmt == 1) {
					char msg[1024];

					featGlyphDump(g, first->gid, ' ', 0);
					featGlyphDump(g, second->gid, '\0', 0);
					if (filename != NULL) {
						sprintf(msg, " [%s %d]", filename, lineNum);
					}
					else {
						msg[0] = '\0';
					}
					hotMsg(g, hotWARNING, "Specific kern pair occurring after "
					       "class kern pair: %s%s", g->note.array, msg);
				}
			}
		}
		startNewSubtable(g);
		si->pairs.cnt = 0;
		si->pairFmt = pairFmt;
		si->pairValFmt1 = valFmt1;
		si->pairValFmt2 = valFmt2;
	}

	/* Check if value record format needs updating */
	if (pairFmt == 1) {
		/* --- Add specific pair(s) --- */
		first->nextSeq = second;
		if (first->nextCl == NULL && second->nextCl == NULL) {
			addSpecPair(g, first->gid, second->gid, metricsCnt1, values1, metricsCnt2, values2);
		}
#if HOT_FEAT_SUPPORT
		else {
			/* Enumerate */
			unsigned i;
			unsigned length;
			GNode ***prod;

			prod = featMakeCrossProduct(g, first, &length);
			for (i = 0; i < length; i++) {
				GNode *specPair = (*prod)[i];
				addSpecPair(g, specPair->gid, specPair->nextSeq->gid, metricsCnt1, values1, metricsCnt2, values2);
				featRecycleNodes(g, specPair);
			}
		}
#endif /* HOT_FEAT_SUPPORT */
		featRecycleNodes(g, first);
	}
	else {
		/* --- Add class pair --- */
		int inx1;
		int inx2;
		int i;
		unsigned short cl1;
		unsigned short cl2;
		int insert1;
		int insert2;
		KernRec *pair;

		if ((inx1 = validInClassDef(h, 0, first, &cl1, &insert1)) != -1 &&
				(inx2 = validInClassDef(h, 1, second, &cl2, &insert2)) != -1) {
			/* Add pair to current kern pair accumulator */
			pair = dnaNEXT(si->pairs);
			pair->first.gcl = featSetNewNode(g, cl1);   /* Store classes */
			pair->second.gcl = featSetNewNode(g, cl2);

			pair->metricsCnt1 = metricsCnt1;
			/* metricsCnt is either 1 or 4 */
			for (i=0; i < metricsCnt1; i++) {
				pair->metricsRec1[i] = values1[i];
			}
			pair->metricsCnt2 = metricsCnt2;
			/* metricsCnt is either 1 or 4 */
			for (i=0; i < metricsCnt2; i++) {
				pair->metricsRec2[i] = values2[i];
			}
#if HOT_DEBUG
			if (DF_LEVEL >= 2) {
				fprintf(stderr, "  * GPOSPair ");
				kernRecDump(g, 0, 0, metricsCnt2, values2, first, second, pair->first.gcl->gid, pair->second.gcl->gid);
			}
#endif
			/* Insert into the classDefs, if needed */
			if (insert1) {
				insertInClassDef(h, 0, first, inx1, cl1);
			}
			else {
				featRecycleNodes(g, first);
			}
			if (insert2) {
				insertInClassDef(h, 1, second, inx2, cl2);
			}
			else {
				featRecycleNodes(g, second);
			}
		}
		else {
			char msg[1024];
			featGlyphClassDump(g, first, ' ', 0);
			featGlyphClassDump(g, second, '\0', 0);
			if (filename != NULL) {
				sprintf(msg, " [%s %d]", filename, lineNum);
			}
			else {
				msg[0] = '\0';
			}
			
			hotMsg(g, hotWARNING, "Start of new pair positioning subtable; "
				   "some pairs may never be accessed: %s%s", g->note.array,
				   msg);
			
			h->startNewPairPosSubtbl = 1;
			GPOSAddPair(g, si, first, second, filename, lineNum);
		}
	}
}

static void addPosRule(hotCtx g, GPOSCtx h, SubtableInfo *si, GNode *targ, char *fileName, long lineNum, int anchorCount, AnchorMarkInfo *anchorMarkInfo) {
	PosRule *rule;
	unsigned short lkpType;

	if (si->parentLkpType == 0) {
		lkpType = si->lkpType;
	}
	else {
		lkpType = si->parentLkpType;
	}

#if HOT_DEBUG
	if (DF_LEVEL >= 2) {
		DF(2, (stderr, "  * GPOS RuleAdd "));
		featPatternDump(g, targ, ' ', 1);
	}
#endif

	if (lkpType == GPOSSingle) {
		GNode *nextNode = targ;
		if (targ->flags & FEAT_HAS_MARKED) {
			SubtableInfo *anon_si;
			if (featValidateGPOSChain(g, targ, lkpType) == 0) {
				return;
			}

			/*
			   for each glyph node in the input sequence, add the pos rule to the anonymous table, if there is one,
			 */
			while (nextNode != NULL) {
				if (!(nextNode->flags & FEAT_MARKED)) {
					nextNode = nextNode->nextSeq;
					continue;
				}
				if ((nextNode->metricsInfo == NULL) || (nextNode->metricsInfo->cnt == 0)) {
					nextNode = nextNode->nextSeq;
					continue;
				}

				anon_si = addAnonPosRule(g, h,  si, lkpType, nextNode);
				if (nextNode->metricsInfo->cnt == 1) {
					/* assume it is an xAdvance adjustment */
					GPOSAddSingle(g, anon_si, nextNode, 0, 0, nextNode->metricsInfo->metrics[0], 0);
				}
				else {
					short *metrics = &nextNode->metricsInfo->metrics[0];

					GPOSAddSingle(g, anon_si, nextNode, metrics[0], metrics[1],
					              metrics[2], metrics[3]);
				}
				nextNode->lookupLabel = anon_si->label;
				nextNode = nextNode->nextSeq;
			}
			/* now add the nodes to the contextual rule list. */
			si->parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */

			rule = dnaNEXT(si->rules);
			rule->targ = targ;
		}
		else {
			if (targ->metricsInfo->cnt == 1) {
				/* assume it is an xAdvance adjustment */
				GPOSAddSingle(g, si, targ, 0, 0, targ->metricsInfo->metrics[0], 0);
			}
			else {
				short *metrics = &targ->metricsInfo->metrics[0];

				GPOSAddSingle(g, si, targ, metrics[0], metrics[1],
				              metrics[2], metrics[3]);
			}
		}
		return;
	}
#if HOT_FEAT_SUPPORT
	else if (lkpType == GPOSPair) {
		/* metrics are now associated with the node they follow */
		GPOSAddPair(g, si, targ, targ->nextSeq,
		            fileName, lineNum);
		return;
	}
	else if (lkpType == GPOSCursive) {
		/* Add BaseGlyphRec records, making sure that there is no overlap in anchor and markClass */
		GNode *nextNode = targ;

		/* Check if this is a contextual rule.*/
		if (targ->flags & FEAT_HAS_MARKED) {
			SubtableInfo *anon_si;
			if (featValidateGPOSChain(g, targ, lkpType) == 0) {
				return;
			}

			/* add the pos rule to the anonymous tables. First, find the base glyph */
			while ((nextNode != NULL) && (!(nextNode->flags & FEAT_IS_BASE_NODE))) {
				nextNode = nextNode->nextSeq;
			}

			anon_si = addAnonPosRule(g, h,  si, lkpType, nextNode);
			GPOSAdCursive(g, anon_si, nextNode, anchorCount, anchorMarkInfo, fileName, lineNum);

			/* now add the nodes to the contextual rule list. */
			si->parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */

			rule = dnaNEXT(si->rules);
			rule->targ = targ;
			/*  add the lookupLabel. */
			nextNode->lookupLabel =  anon_si->label;
		}    /* is contextual */
		else {
			/* isn't contextual */

			GPOSAdCursive(g, si, targ, anchorCount, anchorMarkInfo, fileName, lineNum);
			featRecycleNodes(g, targ); /* I do this here rather than in feat.c;
			                              addPos, as I have to NOT recyle htem if they are contextual */
		}
	}
	else if ((lkpType == GPOSMarkToBase) || (lkpType == GPOSMarkToMark) || (lkpType == GPOSMarkToLigature)) {
		/* Add BaseGlyphRec records, making sure that there is no overlap in anchor and markClass */
		GNode *nextNode = targ;

		/* Check if this is a contextual rule.*/
		if (targ->flags & FEAT_HAS_MARKED) {
			SubtableInfo *anon_si;
			if (featValidateGPOSChain(g, targ, lkpType) == 0) {
				return;
			}

			/* add the pos rule to the anonymous tables. First, find the base glyph */
			while ((nextNode != NULL) && (!(nextNode->flags & FEAT_IS_BASE_NODE))) {
				nextNode = nextNode->nextSeq;
			}

			anon_si = addAnonPosRule(g, h,  si, lkpType, nextNode);

			GPOSAddMark(g, anon_si, nextNode, anchorCount, anchorMarkInfo, fileName, lineNum);

			/* now add the nodes to the contextual rule list. */
			si->parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */

			rule = dnaNEXT(si->rules);
			rule->targ = targ;
			/* find the mark class and add the lookupLabel. */
			while ((nextNode != NULL) && (!(nextNode->flags & FEAT_IS_MARK_NODE))) {
				nextNode = nextNode->nextSeq;
			}
			nextNode->lookupLabel = anon_si->label;
		}    /* is contextual */
		else {
			/* isn't contextual */

			GPOSAddMark(g, si, targ, anchorCount, anchorMarkInfo, fileName, lineNum);
			featRecycleNodes(g, targ); /* I do this here rather than in feat.c;
			                              addPos, as I have to NOT recyle htem if they are contextual */
		}
	}
	else {
		/* Add whole rule intact (no enumeration needed) */
		rule = dnaNEXT(si->rules);
		rule->targ = targ;
	}
#endif /* HOT_FEAT_SUPPORT */
}

/* Add rule (enumerating if necessary) to subtable si */

void GPOSRuleAdd(hotCtx g, int lkpType, GNode *targ, char *fileName, long lineNum, int anchorCount, AnchorMarkInfo *anchorMarkInfo) {
	GPOSCtx h = g->ctx.GPOS;

	if (g->hadError) {
		return;
	}

	h->new.fileName = fileName;
	if (targ->flags & FEAT_HAS_MARKED) {
		h->new.parentLkpType = lkpType;
		h->new.lkpType = GPOSChain;
	}

	addPosRule(g, h, &h->new, targ, fileName, lineNum, anchorCount, anchorMarkInfo);
}

/* Compare kern pairs, fmt1. Pairs marked for deletion sink to bottom */

static int CDECL cmpPairPos1(const void *first, const void *second) {
	const KernRec *a = first;
	const KernRec *b = second;
	int i;
	int metricsCnt;
	
	if (a->first.gid == GID_UNDEF && b->first.gid == GID_UNDEF) {
		return 0;
	}
	else if (a->first.gid == GID_UNDEF) {
		return 1;
	}
	else if (b->first.gid == GID_UNDEF) {
		return -1;
	}
	else if (a->first.gid < b->first.gid) {
		/* Compare first glyphs */
		return -1;
	}
	else if (a->first.gid > b->first.gid) {
		return 1;
	}
	else if (a->second.gid < b->second.gid) {
		/* Compare second glyphs */
		return -1;
	}
	else if (a->second.gid > b->second.gid) {
		return 1;
	}
	if ((a->metricsRec1[0] == 0) && (b->metricsRec1[0] != 0)) {	/* Abs values in decr order */
		return -1;
	}
	else if ((a->metricsRec1[0] != 0) && (b->metricsRec1[0] == 0)) {
		return 1;
	}
	metricsCnt = (a->metricsCnt1 > b->metricsCnt1) ?  b->metricsCnt1 : a->metricsCnt1;
	for (i=0; i < metricsCnt; i++) {
		if (ABS(a->metricsRec1[i]) > ABS(b->metricsRec1[i])) {	/* Abs values in decr order */
			return -1;
		}
		else if (ABS(a->metricsRec1[i]) < ABS(b->metricsRec1[i])) {
			return 1;
		}
		else if (a->metricsRec1[i] > b->metricsRec1[i]) {			/* Values in decr order */
			return -1;
		}
		else if (a->metricsRec1[i] < b->metricsRec1[i]) {
			return 1;
		}
	}
	
	if ((a->metricsRec2[0] == 0) && (b->metricsRec2[0] != 0)) {	/* Abs values in decr order */
		return -1;
	}
	else if ((a->metricsRec2[0] != 0) && (b->metricsRec2[0] == 0)) {
		return 1;
	}
	metricsCnt  = (a->metricsCnt2 > b->metricsCnt2) ?  b->metricsCnt2 : a->metricsCnt2;
	for (i=0; i < metricsCnt; i++) {
		if (ABS(a->metricsRec2[i]) > ABS(b->metricsRec2[i])) {	/* Abs values in decr order */
			return -1;
		}
		else if (ABS(a->metricsRec2[i]) < ABS(b->metricsRec2[i])) {
			return 1;
		}
		else if (a->metricsRec2[i] > b->metricsRec2[i]) {			/* Values in decr order */
			return -1;
		}
		else if (a->metricsRec2[i] < b->metricsRec2[i]) {
			return 1;
		}
	}
	return 0;
}

/* Compare kern pairs, fmt2. Pairs marked for deletion sink to bottom */

static int CDECL cmpPairPos2(const void *first, const void *second) {
	const KernRec *a = first;
	const KernRec *b = second;
	int i;
	
	int metricsCnt;

	if (a->first.gcl == NULL && b->first.gcl == NULL) {
		return 0;
	}
	else if (a->first.gcl == NULL) {
		return 1;
	}
	else if (b->first.gcl == NULL) {
		return -1;
	}
	else if (a->first.gcl->gid < b->first.gcl->gid) {
		/* Compare 1st glyphs */
		return -1;
	}
	else if (a->first.gcl->gid > b->first.gcl->gid) {
		return 1;
	}
	else if (a->second.gcl->gid < b->second.gcl->gid) {
		/* Compare 2nd glyphs*/
		return -1;
	}
	else if (a->second.gcl->gid > b->second.gcl->gid) {
		return 1;
	}
	if ((a->metricsRec1[0] == 0) && (b->metricsRec1[0] != 0)) {	/* Abs values in decr order */
		return -1;
	}
	else if ((a->metricsRec1[0] != 0) && (b->metricsRec1[0] == 0)) {
		return 1;
	}
	
	metricsCnt = (a->metricsCnt1 > b->metricsCnt1) ?  b->metricsCnt1 : a->metricsCnt1;
	for (i=0; i < metricsCnt; i++) {
		if (ABS(a->metricsRec1[i]) > ABS(b->metricsRec1[i])) {	/* Abs values in decr order */
			return -1;
		}
		else if (ABS(a->metricsRec1[i]) < ABS(b->metricsRec1[i])) {
			return 1;
		}
		else if (a->metricsRec1[i] > b->metricsRec1[i]) {			/* Values in decr order */
			return -1;
		}
		else if (a->metricsRec1[i] < b->metricsRec1[i]) {
			return 1;
		}
	}
	if ((a->metricsRec2[0] == 0) && (b->metricsRec2[0] != 0)) {	/* Abs values in decr order */
		return -1;
	}
	else if ((a->metricsRec2[0] != 0) && (b->metricsRec2[0] == 0)) {
		return 1;
	}
	metricsCnt = (a->metricsCnt2 > b->metricsCnt2) ?  b->metricsCnt2 : a->metricsCnt2;
	for (i=0; i < metricsCnt; i++) {
		if (ABS(a->metricsRec2[i]) > ABS(b->metricsRec2[i])) {	/* Abs values in decr order */
			return -1;
		}
		else if (ABS(a->metricsRec2[i]) < ABS(b->metricsRec2[i])) {
			return 1;
		}
		else if (a->metricsRec2[i] > b->metricsRec2[i]) {			/* Values in decr order */
			return -1;
		}
		else if (a->metricsRec2[i] < b->metricsRec2[i]) {
			return 1;
		}
	}
	return 0;
}

/* For a particular classDef and a given class, return the GNode linked list */

static GNode *getGNodes(hotCtx g, unsigned class, int classDefInx) {
	int i;
	GPOSCtx h = g->ctx.GPOS;
	ClassDef *cdef = &h->classDef[classDefInx];

	/* Seq search; will be rarely used */
	for (i = 0; i < cdef->classInfo.cnt; i++) {
		ClassInfo *ci = &cdef->classInfo.array[i];
		if (ci->class == class) {
			return ci->gc;
		}
	}
	hotMsg(g, hotFATAL, "class <%u> not valid in classDef", class);
	return NULL;    /* Suppress compiler warning */
}

/* For duplicate error reporting. For PairPos2, first and second are classes */

static void printKernPair(hotCtx g, GID gid1, GID gid2, int val1, int val2,
						  int fmt1) {
	if (fmt1) {
		featGlyphDump(g, gid1, ' ', 0);
		featGlyphDump(g, gid2, '\0', 0);
	}
	else {
		featGlyphClassDump(g, getGNodes(g, gid1, 0), ' ', 0);
		featGlyphClassDump(g, getGNodes(g, gid2, 1), '\0', 0);
	}
}

/* Check for duplicate pairs; sort */

static void checkAndSortPairPos(hotCtx g, GPOSCtx h, SubtableInfo *si) {
#define REPORT_DUPE_KERN 1  /* Turn off for bad fonts, which may flood you
							  with warnings */
	long i;
	int nDuplicates = 0;
	int fmt1 = si->pairFmt == 1;
	KernRec *prev = NULL; /* Suppress optimizer warning */

	/* Sort by first, second, value */
	qsort(si->pairs.array,  si->pairs.cnt, sizeof(KernRec),
		  fmt1 ? cmpPairPos1 : cmpPairPos2);

	for (i = 1; i <  si->pairs.cnt; i++) {
		GID curr1;
		GID curr2;
		GID prev1;
		GID prev2;
		KernRec *curr = &si->pairs.array[i];

		prev = curr - 1;
		if (fmt1) {
			curr1 = curr->first.gid;
			curr2 = curr->second.gid;
			prev1 = prev->first.gid;
			prev2 = prev->second.gid;
		}
		else {
			curr1 = curr->first.gcl->gid;
			curr2 = curr->second.gcl->gid;
			prev1 = prev->first.gcl->gid;
			prev2 = prev->second.gcl->gid;
		}

		if (curr1 == prev1 && curr2 == prev2) {
			KernRec *delete;
			int isDuplicate = 0;

			/* Check if values conflict. Report different message if they are duplicates.  */
			if ((curr->metricsCnt1 == prev->metricsCnt1) && ((curr->metricsCnt1 == 0) || (curr->metricsRec1[0] == prev->metricsRec1[0]))) {
				isDuplicate = 1;
			}
			else if ((curr->metricsCnt2 == prev->metricsCnt2) && ((curr->metricsCnt2 == 0) || (curr->metricsRec2[0] == prev->metricsRec2[0]))) {
				isDuplicate = 1;
			}
			if (isDuplicate) {
#if REPORT_DUPE_KERN
				printKernPair(g, curr1, curr2, curr->metricsRec1[0], prev->metricsRec1[0], fmt1);
				hotMsg(g, hotNOTE, "Removing duplicate pair positioning in "
				       "'%c%c%c%c' feature: %s", TAG_ARG(si->feature),
				       g->note.array);
#endif /* REPORT_DUPE_KERN */
			}
			else {
				int currVal = IS_MM(g)? FIX2INT(MMFXExecMetric(g, curr->metricsRec1[0])) : curr->metricsRec1[0];
				int prevVal = IS_MM(g)? FIX2INT(MMFXExecMetric(g, prev->metricsRec1[0])) : prev->metricsRec1[0];
#if REPORT_DUPE_KERN
				printKernPair(g, curr1, curr2, currVal, prevVal, fmt1);
				hotMsg(g, hotWARNING,
					   "Pair positioning has conflicting statements in "
					   "'%c%c%c%c' feature; choosing the first "
					   "value: %s", TAG_ARG( si->feature), g->note.array);
				/* ... for MMs, at default instance */
#endif /* REPORT_DUPE_KERN */
			}
			/* Mark previous (unless samePrev) record for deletion */
			delete = curr;
			if (fmt1) {
				delete->first.gid = GID_UNDEF;
			}
			else {
				featRecycleNodes(g, delete->first.gcl);
				featRecycleNodes(g, delete->second.gcl);
				delete->first.gcl = NULL;   /* Marker for deletion */
				delete->second.gcl = NULL;
			}
			nDuplicates++;
		}
	}

	if (nDuplicates > 0) {
		/* Duplicates sink to the bottom */
		qsort(si->pairs.array,  si->pairs.cnt, sizeof(KernRec),
		      fmt1 ? cmpPairPos1 : cmpPairPos2);
		si->pairs.cnt -= nDuplicates;
	}
}

/* Fill format 1 pair positioning subtable. */

static LOffset fillPairPos1(hotCtx g, GPOSCtx h) {
	int i;
	int j;
	int iFirst;
	GID first;
	LOffset offset;
	int nPairSets;
	Subtable *sub = h->new.sub; /* startNewSubtable() called already. */
	otlTbl otl = sub->extension.use ? sub->extension.otl : h->otl;
	PairPosFormat1 *fmt = MEM_NEW(g, sizeof(PairPosFormat1));

	checkAndSortPairPos(g, h, &h->new);

	/* Count pair sets and build coverage table */
	otlCoverageBegin(g, otl);
	nPairSets = 1;
	first = h->new.pairs.array[0].first.gid;
	for (i = 1; i < h->new.pairs.cnt; i++) {
		if (h->new.pairs.array[i].first.gid != first) {
			otlCoverageAddGlyph(g, otl, first);
			nPairSets++;
			first = h->new.pairs.array[i].first.gid;
		}
	}
	otlCoverageAddGlyph(g, otl, first);

	fmt->PosFormat = 1;

  	fmt->ValueFormat1 = h->new.pairValFmt1;
	fmt->ValueFormat2 = h->new.pairValFmt2;

	fmt->PairSetCount = nPairSets;

	/* Allocate pair set arrays */
	fmt->PairSet = MEM_NEW(g, sizeof(Offset) * nPairSets);
	fmt->PairSet_ = MEM_NEW(g, sizeof(PairSet) * nPairSets);

	/* Fill pair sets */
	j = 0;
	iFirst = 0;
	offset = PAIR_POS1_SIZE(nPairSets);
	first = h->new.pairs.array[0].first.gid;
	for (i = 1; i <= h->new.pairs.cnt; i++) {
		if (i == h->new.pairs.cnt || h->new.pairs.array[i].first.gid !=
		    h->new.pairs.array[iFirst].first.gid) {
			int k;
			PairSet *tbl = &fmt->PairSet_[j];

			fmt->PairSet[j++] = (Offset)offset;

			tbl->PairValueCount = i - iFirst;
			tbl->PairValueRecord =
			    MEM_NEW(g, sizeof(PairValueRecord) * tbl->PairValueCount);

			for (k = 0; k < tbl->PairValueCount; k++) {
				KernRec *src = &h->new.pairs.array[iFirst + k];
				PairValueRecord *dst = &tbl->PairValueRecord[k];

				dst->SecondGlyph = src->second.gid;

				/* Fill in first glyph values.
				First value record is always defined, but could be a dummy with
				a count of 1 and a value of 0. */
				if ((src->metricsCnt1 == 1) && (src->metricsRec1[0] == 0)) {
					dst->Value1 = VAL_REC_UNDEF;
				}
				else {
					dst->Value1 = h->values.cnt;
					if (src->metricsCnt1 == 1) {
						*dnaNEXT(h->values) = src->metricsRec1[0];
					}
					else {
						recordValues(h, fmt->ValueFormat1,  src->metricsRec1[0],  src->metricsRec1[1],
						src->metricsRec1[2],  src->metricsRec1[3]);
					}
				}
				/* Fill in second glyph values */

				if ((src->metricsCnt2 == 0) || ((src->metricsCnt2 == 1) && (src->metricsRec2[0] == 0))) {
					dst->Value2 = VAL_REC_UNDEF;
				}
				else {
					dst->Value2 = h->values.cnt;
					if (src->metricsCnt2 == 1) {
						*dnaNEXT(h->values) = src->metricsRec2[0];
					}
					else {
						recordValues(h, fmt->ValueFormat2,  src->metricsRec2[0],  src->metricsRec2[1],
						src->metricsRec2[2],  src->metricsRec2[3]);
					}
				}
			}
			offset += PAIR_SET_SIZE(tbl->PairValueCount, numValues(fmt->ValueFormat1), numValues(fmt->ValueFormat2));
			iFirst = i;
		}
	}

	fmt->Coverage = otlCoverageEnd(g, otl); /* Adjusted later */
	if (sub->extension.use) {
		fmt->Coverage += offset;            /* Final value */
		h->offset.extension += offset + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += offset;
	}

	sub->tbl = fmt;
	return offset;
}

static Offset classDefMake(hotCtx g, GPOSCtx h, otlTbl t, int cdefInx,
						  LOffset *coverage, unsigned short *count) {
	int i;
	GNode *p;
	ClassDef *cdef = &h->classDef[cdefInx];

	if (cdef->classInfo.cnt == 0 || cdef->cov.cnt == 0) {
		hotMsg(g, hotFATAL, "empty classdef");
	}

	/* --- Create coverage, if needed --- */
	if (coverage != NULL) {
		otlCoverageBegin(g, t);
		for (i = 0; i < cdef->cov.cnt; i++) {
			otlCoverageAddGlyph(g, t, cdef->cov.array[i]);
		}
		*coverage = otlCoverageEnd(g, t);   /* Adjusted later */
	}

	/* --- Create classdef --- */
	/* Classes start numbering from 0 for ClassDef1, 1 for ClassDef2 */
    if (g->convertFlags & HOT_DO_NOT_OPTIMIZE_KERN)
        *count = (unsigned short)cdef->classInfo.cnt + 1;
    else
        *count = (unsigned short)((cdefInx == 0) ? cdef->classInfo.cnt : cdef->classInfo.cnt + 1);
	otlClassBegin(g, t);
	for (i = 0; i < cdef->classInfo.cnt; i++) {
		ClassInfo *ci = &cdef->classInfo.array[i];
		for (p = ci->gc; p != NULL; p = p->nextCl) {
			if (ci->class != 0) {
				otlClassAddMapping(g, t, p->gid, ci->class);
			}
		}
	}
	return otlClassEnd(g, t);
}

/* Fill format 2 pair positioning subtable */

static void fillPairPos2(hotCtx g, GPOSCtx h) {
	int i;
	int secondClVal;
	LOffset size;
	Subtable *sub = h->new.sub; /* startNewSubtable() called already. */
	otlTbl otl = sub->extension.use ? sub->extension.otl : h->otl;
	PairPosFormat2 *fmt = MEM_NEW(g, sizeof(PairPosFormat2));
#if HOT_DEBUG
	int nFilled = 0;
#endif

	checkAndSortPairPos(g, h, &h->new);

	fmt->PosFormat = 2;

	fmt->ValueFormat1 = h->new.pairValFmt1;
	fmt->ValueFormat2 = h->new.pairValFmt2;

	/* (ClassDef offsets adjusted later) */
	fmt->ClassDef1 = classDefMake(g, h, otl, 0, &fmt->Coverage,
	                              &fmt->Class1Count);
	fmt->ClassDef2 = classDefMake(g, h, otl, 1, NULL, &fmt->Class2Count);

	/* --- Allocate and initialize 2-dimensional array Class1Record */
	fmt->Class1Record = MEM_NEW(g, fmt->Class1Count * sizeof(Class1Record));

	fmt->Class1Record[0].Class2Record =
	    MEM_NEW(g, fmt->Class1Count * fmt->Class2Count * sizeof(Class2Record));
	for (i = 0; i < fmt->Class1Count; i++) {
		int j;
		if (i != 0) {
			fmt->Class1Record[i].Class2Record =
			    &fmt->Class1Record[0].Class2Record[fmt->Class2Count * i];
		}
		for (j = 0; j < fmt->Class2Count; j++) {
			fmt->Class1Record[i].Class2Record[j].Value1 =
			    fmt->Class1Record[i].Class2Record[j].Value2 = VAL_REC_UNDEF;
		}
	}

	/* --- Fill in Class1Record */
	secondClVal = 0;
	for (i = 0; i < h->new.pairs.cnt; i++) {
		KernRec *pair = &h->new.pairs.array[i];
		unsigned cl1 = pair->first.gcl->gid;
		unsigned cl2 = pair->second.gcl->gid;
		Class2Record *dst = &fmt->Class1Record[cl1].Class2Record[cl2];

		if ((pair->metricsCnt1 == 0) || ((pair->metricsCnt1 == 1) && (pair->metricsRec1[0] == 0))) {
			dst->Value1 = VAL_REC_UNDEF;
		}
		else {
			dst->Value1 = h->values.cnt;
			if (pair->metricsCnt1 == 1) {
				*dnaNEXT(h->values) = pair->metricsRec1[0];
			}
			else {
				recordValues(h, fmt->ValueFormat1,  pair->metricsRec1[0],  pair->metricsRec1[1],
				pair->metricsRec1[2],  pair->metricsRec1[3]);
			}
		}
		
		if ((pair->metricsCnt2 == 0) || ((pair->metricsCnt2 == 1) && (pair->metricsRec2[0] == 0))) {
			dst->Value2 = VAL_REC_UNDEF;
		}
		else {
			dst->Value2 = h->values.cnt;
			if (pair->metricsCnt2 == 1) {
				*dnaNEXT(h->values) = pair->metricsRec2[0];
			}
			else {
				recordValues(h, fmt->ValueFormat2,  pair->metricsRec2[0],  pair->metricsRec2[1],
				pair->metricsRec2[2],  pair->metricsRec2[3]);
			}
		}

#if HOT_DEBUG
		nFilled++;
#endif
	}

	size = PAIR_POS2_SIZE(fmt->Class1Count, fmt->Class2Count,
	                      numValues(fmt->ValueFormat1) +
	                      numValues(fmt->ValueFormat2));
#if HOT_DEBUG
	DF(1, (stderr, "#Cl kern: %d of %u(%hux%u) array is filled "
		   "(%4.2f%%), excl ClassDef2's class 0. Subtbl size: %lu\n",
		   nFilled,
		   fmt->Class1Count * (fmt->Class2Count - 1),
		   fmt->Class1Count,
		   fmt->Class2Count - 1,
		   nFilled * 100.00 /(fmt->Class1Count * (fmt->Class2Count-1)),
		   size));
#endif

	if (sub->extension.use) {
		fmt->Coverage  += size;                           /* Final value */
		fmt->ClassDef1 += size + otlGetCoverageSize(otl); /* Final value */
		fmt->ClassDef2 += size + otlGetCoverageSize(otl); /* Final value */
		h->offset.extension += size + otlGetCoverageSize(otl)
		    + otlGetClassSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
}

/* Fill pair postioning subtable (last one if there were several) */

static void fillPairPos(hotCtx g, GPOSCtx h) {
	if (h->otl == NULL) {
		/* Allocate table if not done so already */
		h->otl = otlTableNew(g);
	}

	if (h->new.pairs.cnt != 0) {
		switch (h->new.pairFmt) {
			case 1:
				fillPairPos1(g, h);
				break;

			case 2:
				fillPairPos2(g, h);
				break;
		}
	}

	h->maxContext = MAX(h->maxContext, 2);
}

/* ------------------ Chaining Contextual Substitution --------------------- */


typedef struct {
	unsigned short BacktrackGlyphCount;
	GID *Backtrack;                                 /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	GID *Input;                                     /* [InputGlyphCount - 1] */
	unsigned short LookaheadGlyphCount;
	GID *Lookahead;                                 /* [LookaheadGlyphCount] */
	unsigned short PosCount;
	PosLookupRecord *PosLookupRecord;           /* [PosCount] */
} ChainSubRule;
#define CHAIN_SUB_RULE_SIZE(nBack, nInput, nLook, nPos)  (uint16 * 4 + \
	                                                      uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                                      POS_LOOKUP_RECORD_SIZE * (nPos))
typedef struct {
	unsigned short ChainSubRuleCount;
	DCL_OFFSET_ARRAY(ChainSubRule, ChainSubRule);   /* [ChainSubRuleCount] */
} ChainSubRuleSet;
#define CHAIN_SUB_RULE_SET_SIZE(nRules) (uint16 + uint16 * (nRules))

typedef struct {
	unsigned short PosFormat;                       /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short ChainSubRuleSetCount;
	DCL_OFFSET_ARRAY(ChainSubRuleSet, ChainSubRuleSet);
	/* [ChainSubRuleSetCount]*/
} ChainContextPosFormat1;
#define CHAIN1_HDR_SIZE(nRuleSets)  (uint16 * 3 + uint16 * (nRuleSets))


typedef struct {
	unsigned short BacktrackGlyphCount;
	unsigned short *Backtrack;                      /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	unsigned short *Input;                          /* [InputGlyphCount - 1] */
	unsigned short LookaheadGlyphCount;
	unsigned short *Lookahead;                      /* [LookaheadGlyphCount] */
	unsigned short PosCount;
	PosLookupRecord *PosLookupRecord;           /* [PosCount] */
} ChainSubClassRule;
#define CHAIN_SUB_CLASS_RULE_SIZE(nBack, nInput, nLook, nPos)  (uint16 * 4 + \
	                                                            uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                                            POS_LOOKUP_RECORD_SIZE * (nPos))
typedef struct {
	unsigned short ChainSubClassRuleCnt;
	DCL_OFFSET_ARRAY(ChainSubClassRule, ChainSubClassRule);
	/* [ChainSubClassRuleCnt]*/
} ChainSubClassSet;
#define CHAIN_SUB_CLASS_SET_SIZE(nRules)    (uint16 + uint16 * (nRules))

typedef struct {
	unsigned short PosFormat;                       /* =2 */
	LOffset Coverage;               /* 32-bit for overflow check */
	LOffset ClassDef;               /* 32-bit for overflow check */
	unsigned short ChainSubClassSetCnt;
	DCL_OFFSET_ARRAY(ChainSubClassSet, ChainSubClassSet);
	/* [ChainSubClassSetCnt] */
} ChainContextPosFormat2;
#define CHAIN2_HDR_SIZE(nSets)  (uint16 * 4 + uint16 * (nSets))


typedef struct {
	unsigned short PosFormat;                       /* =3 */
	unsigned short BacktrackGlyphCount;
	LOffset *Backtrack;                             /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	LOffset *Input;                                 /* [InputGlyphCount] */
	unsigned short LookaheadGlyphCount;
	LOffset *Lookahead;                             /* [LookaheadGlyphCount] */
	unsigned short PosCount;
	PosLookupRecord *PosLookupRecord;           /* [PosCount] */
} ChainContextPosFormat3;
#define CHAIN3_SIZE(nBack, nInput, nLook, nPos)  (uint16 * 5 + \
	                                              uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                              POS_LOOKUP_RECORD_SIZE * (nPos))

static void recycleProd(GPOSCtx h) {
	long i;
	for (i = 0; i < h->prod.cnt; i++) {
		featRecycleNodes(h->g, h->prod.array[i]);
	}
}

/* Tries to add rule to current anon subtbl. If successful, returns 1, else 0.
   If rule already exists in subtbl, recycles targ */

static int cmpWithPosRule(GNode *targ,   SingleRec *cmpRec, int nFound) {
	GNode *t1 = targ;
	GID gid2 = cmpRec->gid;
	int checkedMetrics = 0;
	int i;
	short metricsCnt1 = targ->metricsInfo->cnt;
	short metricsCnt2;
	short *metrics1 = targ->metricsInfo->metrics;
	short metrics2[4];

	if (cmpRec->valFmt == ValueXAdvance) {
		metricsCnt2 = 1;
		metrics2[0] = cmpRec->xAdv;
	}
	else {
		metricsCnt2 = 4;
		metrics2[0] = cmpRec->xPla;
		metrics2[1] = cmpRec->yPla;
		metrics2[2] = cmpRec->xAdv;
		metrics2[3] = cmpRec->yAdv;
	}


	for (; t1 != NULL; t1 = t1->nextCl) {
		if ((t1->gid == gid2) && (!(t1->flags & FEAT_MISC))) {
			if (!checkedMetrics) {
				/* a test so that we only check the metrics for teh head node of the class */
				if (metricsCnt1 != metricsCnt2) {
					return -1;
				}
				for (i = 0; i < metricsCnt1; i++) {
					if (metrics1[i] != metrics2[i]) {
						return -1;
					}
				}
				checkedMetrics = 1;
			}
			nFound++;
			t1->flags |= FEAT_MISC;
		}
	}
	return nFound;
}

/* Add the "anonymous" rule that occurs in a substitution within a chaining
   contextual rule. Return the label of the anonymous lookup */

static int checkAddRule(SubtableInfo *si, GNode *targ) {
	GNode *t = targ;
	int nTarg = 0;
	int nFound = 0;
	int i;
	/* If all the class members for this single pos rule are in the curent subtable,
	   we can just return, after recycling all the nodes.
	   If any one is present but with a different metrics record, we need to return 0, not found.
	   If some nodes are new and others are not, we need to remove the "already known" nodes from
	   the rule, adn recycle them */

	for (; t != NULL; t = t->nextCl) {
		nTarg++;
	}

	for (i = 0; i < si->single.cnt; i++) {
		SingleRec *cmpRec = &si->single.array[i];

		nFound = cmpWithPosRule(targ, cmpRec, nFound);
		if (nFound < 0) {
			break; /* found a match in coverage, but with a different metrics record. Must put it in a different lookup.*/
		}
		if (nFound == nTarg) {
			break;
		}
	}

	if (nFound < 0) {
		return 0; /* Conflicting records - must use new subtable */
	}
	else if (nTarg == nFound) {
		return -1; /* All already present; no need to add more */
	}
	else {
		return 1; /* Can add to current subtable */
	}
}

static SubtableInfo *addAnonPosRule(hotCtx g, GPOSCtx h, SubtableInfo *cur_si, unsigned short lkpType, GNode *targ) {
	SubtableInfo *si;


	/* For GPOSSingle lookups only, we check if the rule can be added to the most current subtable
	   in the anonymous list, and if not, look to see if it can be added to earlier subtables.
	   For all others, we check only the must current subtable, and assume that it is OK if the type matcches.
	 */
	if (h->anonSubtable.cnt > 0) {
		int i = h->anonSubtable.cnt;

		if (lkpType != GPOSSingle) {
			si = dnaINDEX(h->anonSubtable, i - 1);
			if ((si->script == cur_si->script) &&
			    (si->language == cur_si->language) &&
			    (si->feature == cur_si->feature) &&
			    (si->useExtension == cur_si->useExtension) &&
			    (si->lkpFlag == cur_si->lkpFlag) &&
			    (si->markSetIndex == cur_si->markSetIndex) &&
			    (lkpType == si->lkpType)) {
				return si;
			}
		}
		else {
			while (i-- > 0) {
				si = dnaINDEX(h->anonSubtable, i);
				if ((si->script == cur_si->script) &&
				    (si->language == cur_si->language) &&
				    (si->feature == cur_si->feature) &&
				    (si->useExtension == cur_si->useExtension) &&
				    (si->lkpFlag == cur_si->lkpFlag) &&
				    (si->markSetIndex == cur_si->markSetIndex) &&
				    (lkpType == si->lkpType)) {
					if (checkAddRule(si, targ) != 0) {
						return si;
					}
				}
				else {
					continue;
				}
			}
		}
	}

	/* If we get here, we could not use the most recent anonomous table, and
	   must add a new one */

	si = dnaNEXT(h->anonSubtable);
	/* *si = *cur_si; Can't do this, as it copies all the dna array pointers */
	/* Copy all values except the dna array and format specific stuff */
	si->script = cur_si->script;
	si->language = cur_si->language;
	si->feature = cur_si->feature;
	si->useExtension = cur_si->useExtension;                /* Use extension lookupType? */
	si->lkpFlag = cur_si->lkpFlag;
	si->markSetIndex = cur_si->markSetIndex;
	si->fileName = cur_si->fileName;				/* the current feature file name */

	/* Now for the new values that are specific to this table */
	si->lkpType = lkpType;
	si->lkpFlag = cur_si->lkpFlag;
	si->markSetIndex = cur_si->markSetIndex;
	si->parentLkpType = 0;
	si->label = featGetNextAnonLabel(g);

	return si;
}

/* Create anonymous lookups (referred to only from within chain ctx lookups) */

static void createAnonLookups(hotCtx g, GPOSCtx h) {
	long i;

	for (i = 0; i < h->anonSubtable.cnt; i++) {
		SubtableInfo *si = &h->anonSubtable.array[i];
		SubtableInfo *newsi = &h->new;
		si->script = si->language =  si->feature = TAG_UNDEF; /* so that these will sort to the end of the subtable array
		                                                         and will not be considered for adding to the FeatureList table */
		*newsi = *si;

		GPOSLookupEnd(g, si->feature); /* This is where the fill unctions get called */
		GPOSFeatureEnd(g);
	}
}

/* Change anon PosLookupRecord labels to lookup inxs, now that they've been
   calculated by otlFill() */

static void setAnonLookupIndices(hotCtx g, GPOSCtx h) {
	int i;

	sortLabelList(g, h->otl);

	for (i = 0; i < h->posLookup.cnt; i++) {
		PosLookupRecord *slr = h->posLookup.array[i];
		DF(2, (stderr, "slr: Label 0x%x", slr->LookupListIndex));
		slr->LookupListIndex =
		    otlLabel2LookupIndex(g, h->otl, slr->LookupListIndex);
		DF(2, (stderr, " -> LookupListIndex %u\n", slr->LookupListIndex));
	}
}

/* points to an input sequence; return new array of num coverages */

static LOffset *setCoverages(hotCtx g, otlTbl otl, GNode *p, unsigned num) {
	unsigned i;
	LOffset *cov;

	if (num == 0) {
		return NULL;
	}

	cov = MEM_NEW(g, sizeof(LOffset) * num);
	for (i = 0; i != num; i++, p = p->nextSeq) {
		GNode *q;
		otlCoverageBegin(g, otl);
		for (q = p; q != NULL; q = q->nextCl) {
			otlCoverageAddGlyph(g, otl, q->gid);
		}
		cov[i] = otlCoverageEnd(g, otl);    /* Adjusted later */
	}
	return cov;
}

/* Fill chaining contextual subtable format 3 */

static void fillChain3(hotCtx g, GPOSCtx h, otlTbl otl, Subtable *sub,
                       long inx) {
	LOffset size;
	unsigned nBack = 0;
	unsigned nInput = 0;
	unsigned nLook = 0;
	unsigned nMarked = 0;
	unsigned seqCnt = 0;
	GNode *pBack = NULL;
	GNode *pInput = NULL;
	GNode *pLook = NULL;
	GNode *pMarked = NULL;
	int nPos = 0;
	unsigned i;

	GNode *p;
	PosRule *rule = &h->new.rules.array[inx];

	ChainContextPosFormat3 *fmt =
	    MEM_NEW(g, sizeof(ChainContextPosFormat3));

	/* Set counts of and pointers to Back, Input, Look, Marked areas */
	pBack = rule->targ;
	for (p = rule->targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_BACKTRACK) {
			nBack++;
		}
		else if (p->flags & FEAT_INPUT) {
			if (pInput == NULL) {
				pInput = p;
			}
			nInput++;
			if (p->flags & FEAT_MARKED) {
				/* Marked must be within Input */
				if (pMarked == NULL) {
					pMarked = p;
				}
				nMarked++;
				if (p->lookupLabel >= 0) {
					nPos++;
				}
			}
			seqCnt++;
		}
		else if (p->flags & FEAT_LOOKAHEAD) {
			if (pLook == NULL) {
				pLook = p;
			}
			nLook++;
		}
	}

	/* Fill table */
	fmt->PosFormat = 3;

	fmt->BacktrackGlyphCount = nBack;
	fmt->Backtrack = setCoverages(g, otl, pBack, nBack);
	fmt->InputGlyphCount = nInput;
	fmt->Input = setCoverages(g, otl, pInput, nInput);
	fmt->LookaheadGlyphCount = nLook;
	fmt->Lookahead = setCoverages(g, otl, pLook, nLook);

	fmt->PosCount = nPos;
	if (nPos == 0) {
		fmt->PosLookupRecord = NULL;    /* no action */
	}
	else {
		int posIndex = 0;
		GNode *nextNode = pInput;

		fmt->PosLookupRecord = MEM_NEW(g, sizeof(PosLookupRecord) *
		                               nPos);
		for (i = 0; i < seqCnt; i++) {
			PosLookupRecord **slr;
			if (nextNode->lookupLabel < 0) {
				nextNode = nextNode->nextSeq;
				continue;
			}
			/* Store ptr for easy access later on when lkpInx's are available */
			slr = dnaNEXT(h->posLookup);
			*slr = &fmt->PosLookupRecord[posIndex];

			(*slr)->SequenceIndex = i;
			(*slr)->LookupListIndex = nextNode->lookupLabel;

			nextNode = nextNode->nextSeq;
			posIndex++;
			if (nextNode == NULL) {
				break;
			}
		}
	}

	h->maxContext = MAX(h->maxContext, nInput + nLook);

	size = CHAIN3_SIZE(nBack, nInput, nLook, nPos);
	if (sub->extension.use) {
		long i;

		/* Set final values for coverages */
		for (i = 0; i < fmt->BacktrackGlyphCount; i++) {
			fmt->Backtrack[i] += size;
		}
		for (i = 0; i < fmt->InputGlyphCount; i++) {
			fmt->Input[i] += size;
		}
		for (i = 0; i < fmt->LookaheadGlyphCount; i++) {
			fmt->Lookahead[i] += size;
		}

		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
	featRecycleNodes(g, rule->targ);
}

static void fillChain(hotCtx g, GPOSCtx h) {
	long i;

	/* xxx Each rule in a fmt3 for now */
	for (i = 0; i < h->new.rules.cnt; i++) {
		Subtable *sub;
		otlTbl otl;

		startNewSubtable(g);
		sub = h->new.sub;
		otl = sub->extension.use ? sub->extension.otl : h->otl;

		fillChain3(g, h, otl, sub, i);

		if (h->offset.subtable > 0xFFFF) {
			hotMsg(g, hotFATAL, "Chain contextual lookup subtable in GPOS "
			       "feature '%c%c%c%c' causes offset overflow.",
			       TAG_ARG(h->new.feature), i);
		}
	}
}

static void writePosLookupRecords(GPOSCtx h, PosLookupRecord *slr, int cnt) {
	int i;
	for (i = 0; i < cnt; i++) {
		OUT2(slr[i].SequenceIndex);
		OUT2(slr[i].LookupListIndex);
	}
}

static void writeChain3(hotCtx g, GPOSCtx h, Subtable *sub) {
	long i;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	ChainContextPosFormat3 *fmt = sub->tbl;
	int isExt = sub->extension.use;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	OUT2(fmt->PosFormat);

	OUT2(fmt->BacktrackGlyphCount);

	if (g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
		/* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
		for (i = 0; i < fmt->BacktrackGlyphCount; i++) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "backtrack", "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}
	else {
		/* do it per OpenType spec 1.5 */
		for (i = fmt->BacktrackGlyphCount - 1; i >= 0; i--) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "backtrack", "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}

	OUT2(fmt->InputGlyphCount);
	for (i = 0; i < fmt->InputGlyphCount; i++) {
		if (!sub->extension.use) {
			fmt->Input[i] += adjustment;
		}
		CHECK4OVERFLOW(fmt->Input[i], "input", "chain contextual");
		OUT2((unsigned short)fmt->Input[i]);
	}

	OUT2(fmt->LookaheadGlyphCount);
	for (i = 0; i < fmt->LookaheadGlyphCount; i++) {
		if (!isExt) {
			fmt->Lookahead[i] += adjustment;
		}
		CHECK4OVERFLOW(fmt->Lookahead[i], "look-ahead", "chain contextual");
		OUT2((unsigned short)fmt->Lookahead[i]);
	}

	OUT2(fmt->PosCount);
	writePosLookupRecords(h, fmt->PosLookupRecord, fmt->PosCount);

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write Chain substitution format table */

static void writeChainPos(hotCtx g, GPOSCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			break;

		case 2:
			break;

		case 3:
			writeChain3(g, h, sub);
			break;
	}
}

/* Write format 1 pair positioning table */

static void writePairPos1(hotCtx g, GPOSCtx h, Subtable *sub) {
	int i;
	PairPosFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "Coverage", "pair");

	/* Write header */
	OUT2(fmt->PosFormat);
	OUT2((Offset)fmt->Coverage);
	OUT2(fmt->ValueFormat1);
	OUT2(fmt->ValueFormat2);
	OUT2(fmt->PairSetCount);

	/* Write pair sets */
	for (i = 0; i < fmt->PairSetCount; i++) {
		OUT2(fmt->PairSet[i]);
	}
	for (i = 0; i < fmt->PairSetCount; i++) {
		int j;
		PairSet *tbl = &fmt->PairSet_[i];

		/* Write pair value records */
		OUT2(tbl->PairValueCount);
		for (j = 0; j < tbl->PairValueCount; j++) {
			PairValueRecord *rec = &tbl->PairValueRecord[j];

			OUT2(rec->SecondGlyph);
			writeValueRecord(h, fmt->ValueFormat1, rec->Value1);
			writeValueRecord(h, fmt->ValueFormat2, rec->Value2);
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write format 2 pair positioning table */

static void writePairPos2(hotCtx g, GPOSCtx h, Subtable *sub) {
	unsigned i;
	PairPosFormat2 *fmt = sub->tbl;

	if (!sub->extension.use) {
		/* Adjust coverage and class offsets */
		LOffset classAdjust = h->offset.subtable - sub->offset +
		    otlGetCoverageSize(h->otl);
		fmt->Coverage += h->offset.subtable - sub->offset;
		fmt->ClassDef1 += classAdjust;
		fmt->ClassDef2 += classAdjust;
	}
	CHECK4OVERFLOW(fmt->Coverage, "Coverage", "pair");
	CHECK4OVERFLOW(fmt->ClassDef1, "ClassDef", "pair");
	CHECK4OVERFLOW(fmt->ClassDef2, "ClassDef", "pair");

	/* Write header */
	OUT2(fmt->PosFormat);
	OUT2((Offset)fmt->Coverage);
	OUT2(fmt->ValueFormat1);
	OUT2(fmt->ValueFormat2);
	OUT2((unsigned short)fmt->ClassDef1);
	OUT2((unsigned short)fmt->ClassDef2);
	OUT2(fmt->Class1Count);
	OUT2(fmt->Class2Count);

	for (i = 0; i < fmt->Class1Count; i++) {
		unsigned j;
		for (j = 0; j < fmt->Class2Count; j++) {
			Class2Record *rec = &fmt->Class1Record[i].Class2Record[j];

			writeValueRecord(h, fmt->ValueFormat1, rec->Value1);
			writeValueRecord(h, fmt->ValueFormat2, rec->Value2);
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
		otlClassWrite(g, sub->extension.otl);
	}
}

/* Write pair positioning table */

static void writePairPos(hotCtx g, GPOSCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			writePairPos1(g, h, sub);
			break;

		case 2:
			writePairPos2(g, h, sub);
			break;
	}
}

/* Write feature parameter table */

static void writeFeatParam(GPOSCtx h, Subtable *sub) {
	FeatureParameterFormat *feat_param;
	unsigned short *params;
	int paramOffset;
	int i;

	feat_param = (FeatureParameterFormat *)sub->tbl;
	paramOffset = sizeof(feat_param->numParams);
	params = (unsigned short *)&feat_param->params[paramOffset];
	i = feat_param->numParams;
	for (i = 0; i < feat_param->numParams; i++) {
		OUT2(params[i]);
	}
}

/* Free format 1 pair positioning subtable */

static void freePairPos1(hotCtx g, Subtable *sub) {
	int i;
	PairPosFormat1 *fmt = sub->tbl;

	/* Free pair sets */
	for (i = 0; i < fmt->PairSetCount; i++) {
		MEM_FREE(g, fmt->PairSet_[i].PairValueRecord);
	}

	/* Free pair set arrays */
	MEM_FREE(g, fmt->PairSet);
	MEM_FREE(g, fmt->PairSet_);

	MEM_FREE(g, fmt);
}

/* Free format 2 pair positioning subtable */

static void freePairPos2(hotCtx g, Subtable *sub) {
	PairPosFormat2 *fmt = sub->tbl;

	MEM_FREE(g, fmt->Class1Record[0].Class2Record);
	MEM_FREE(g, fmt->Class1Record);

	MEM_FREE(g, fmt);
}

/* Free pair postioning subtable */

static void freePairPos(hotCtx g, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			freePairPos1(g, sub);
			break;

		case 2:
			freePairPos2(g, sub);
			break;
	}
}

/* Free feature format subtable */

static void freeFeatParam(hotCtx g, Subtable *sub) {
	MEM_FREE(g, sub->tbl);
}

/* Free Chain substitution format 3 table */

static void freeChain3(hotCtx g, GPOSCtx h, Subtable *sub) {
	ChainContextPosFormat3 *fmt = sub->tbl;

	if (fmt->Backtrack != NULL) {
		MEM_FREE(g, fmt->Backtrack);
	}
	if (fmt->Input != NULL) {
		MEM_FREE(g, fmt->Input);
	}
	if (fmt->Lookahead != NULL) {
		MEM_FREE(g, fmt->Lookahead);
	}
	if (fmt->PosLookupRecord != NULL) {
		MEM_FREE(g, fmt->PosLookupRecord);
	}
	MEM_FREE(g, fmt);
}

/* Free Chain substitution format table */

static void freeChain(hotCtx g, GPOSCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			break;

		case 2:
			break;

		case 3:
			freeChain3(g, h, sub);
			break;
	}
}

/* --------------------------- Cursive Attachment -------------------------- */


typedef struct {
	LOffset offset;
	AnchorMarkInfo anchor;
} AnchorListRec;


typedef struct {
	LOffset EntryAnchor;
	LOffset ExitAnchor;
} EntryExitRecord;

typedef struct {
	dnaDCL(AnchorListRec, anchorList); /* not part of the offical format - just used to hold the anchors. for the lookup */
	LOffset endArrays; /* not part of font data */
	unsigned short PosFormat;                       /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short EntryExitCount;
	EntryExitRecord *EntryExitRecord;               /* [EntryExitCount] */
} CursivePosFormat1;

static void initAnchor(void *ctx, long count, AnchorMarkInfo *anchor) {
	long i;
	for (i = 0; i < count; i++) {
		anchor->x = 0;
		anchor->y = 0;
		anchor->contourpoint = 0;
		anchor->format = 0;
		anchor->markClass = 0;
		anchor->markClassIndex = 0;
		anchor->componentIndex = 0;
		anchor++;
	}
	return;
}

static void initAnchorListRec(void *ctx, long count, AnchorListRec *anchorListRec) {
	long i;
	AnchorMarkInfo *anchor = &anchorListRec->anchor;
	for (i = 0; i < count; i++) {
		anchor->x = 0;
		anchor->y = 0;
		anchor->contourpoint = 0;
		anchor->format = 0;
		anchor->markClass = 0;
		anchor->markClassIndex = 0;
		anchor->componentIndex = 0;
		anchor++;
	}
	return;
}

#define CURSIVE_1_SIZE (uint16 * 3)
/* ------------------------ Mark To Base Attachment ------------------------ */
/* ------------------------ Mark To Mark Attachment ------------------------ */



typedef struct {
	LOffset *BaseAnchorArray;
} BaseRecord;

typedef struct {
	unsigned short BaseCount;
	BaseRecord *BaseRecord;                         /* [BaseCount] */
} BaseArray;

typedef struct {
	unsigned short ComponentCount;
	LOffset *ComponentRecordList; /* one block f LOffsets per ligature components - basically, a list of offsets to anchor tables. */
} LigatureAttach;

typedef struct {
	unsigned short LigatureCount;
	DCL_OFFSET_ARRAY(LigatureAttach, LigatureAttach);                           /* [LigatureCount] */
} LigatureArray;

typedef struct {
	dnaDCL(AnchorListRec, anchorList); /* not part of the offical format - just used to hold the anchors. for the lookup */
	LOffset endArrays; /* not part of font data */
	unsigned short PosFormat;                       /* =1 */
	LOffset MarkCoverage;
	LOffset BaseCoverage;
	unsigned short ClassCount;
	DCL_OFFSET(MarkArray, MarkArray);
	DCL_OFFSET(BaseArray, BaseArray);
} MarkBasePosFormat1;
#define MARK_TO_BASE_1_SIZE (uint16 * 6)

typedef struct {
	dnaDCL(AnchorListRec, anchorList); /* not part of the offical format - just used to hold the anchors. for the lookup */
	LOffset endArrays; /* not part of font data */
	unsigned short PosFormat;                       /* =1 */
	LOffset MarkCoverage;
	LOffset LigatureCoverage;
	unsigned short ClassCount;
	DCL_OFFSET(MarkArray, MarkArray);
	DCL_OFFSET(LigatureArray, LigatureArray);
} MarkLigaturePosFormat1;
#define MARK_TO_LIGATURE_1_SIZE (uint16 * 6)


static int cmpAnchors(AnchorMarkInfo *first, AnchorMarkInfo *second) {
	if (first->componentIndex > second->componentIndex) {
		return 1;
	}
	else if (first->componentIndex < second->componentIndex) {
		return -1;
	}

	if (first->markClassIndex > second->markClassIndex) {
		return 1;
	}
	else if (first->markClassIndex < second->markClassIndex) {
		return -1;
	}

	if (first->format > second->format) {
		return 1;
	}
	else if (first->format < second->format) {
		return -1;
	}

	if (first->x > second->x) {
		return 1;
	}
	else if (first->x < second->x) {
		return -1;
	}


	if (first->y > second->y) {
		return 1;
	}
	else if (first->y < second->y) {
		return -1;
	}


	if (first->format == 2) {
		if (first->contourpoint > second->contourpoint) {
			return 1;
		}
		else if (first->contourpoint < second->contourpoint) {
			return -1;
		}
	}

	return 0;
}

static int CDECL cmpBaseRec(const void *first, const void *second) {
	BaseGlyphRec *bgr1 = (BaseGlyphRec *)first;
	BaseGlyphRec *bgr2 = (BaseGlyphRec *)second;
	GID gid1, gid2;
	int i;
	int retVal = 0;

	gid1 = bgr1->gid;
	gid2 = bgr2->gid;
	if (gid1 > gid2) {
		return 1;
	}
	else if (gid1 < gid2) {
		return -1;
	}

	if (bgr1->anchorMarkInfo.cnt  > bgr2->anchorMarkInfo.cnt) {
		return 1;
	}
	else if (bgr1->anchorMarkInfo.cnt  < bgr2->anchorMarkInfo.cnt) {
		return -1;
	}
	for (i = 0; i < bgr1->anchorMarkInfo.cnt; i++) {
		retVal =  cmpAnchors(&bgr1->anchorMarkInfo.array[i], &bgr2->anchorMarkInfo.array[i]);
		if (retVal != 0) {
			return retVal;
		}
	}
	return retVal;
}

static int CDECL cmpLigBaseRec(const void *first, const void *second) {
	BaseGlyphRec *bgr1 = (BaseGlyphRec *)first;
	BaseGlyphRec *bgr2 = (BaseGlyphRec *)second;
	GID gid1, gid2;
	unsigned short cpi1, cpi2;
	int retVal = 0;

	gid1 = bgr1->gid;
	gid2 = bgr2->gid;
	if (gid1 > gid2) {
		return 1;
	}
	else if (gid1 < gid2) {
		return -1;
	}

	cpi1 = bgr1->anchorMarkInfo.array[0].componentIndex;
	cpi2 = bgr2->anchorMarkInfo.array[0].componentIndex;
	if (cpi1 > cpi2) {
		return 1;
	}
	else if (cpi1 < cpi2) {
		return -1;
	}

	return retVal;
}

static void checkBaseAnchorConflict(hotCtx g, BaseGlyphRec *baseGlyphArray, long recCnt, char *fileName, int isMarkToLigature) {
	BaseGlyphRec *prev, *cur, *last;
	if (recCnt < 2) {
		return;
	}

	/* Sort by GID, anchor count, and then by anchor value */
	qsort(baseGlyphArray, recCnt, sizeof(BaseGlyphRec), cmpBaseRec);
	prev = &(baseGlyphArray[0]);
	cur = &(baseGlyphArray[1]);
	last =  &(baseGlyphArray[recCnt - 1]);
	/* Now step through, and report if for any entry, the previous entry with the same GID has  the same  mark class */
	do {
		if (cur->gid == prev->gid) {
			/* For mark to base and mark to mark, only a single entry is allowed in  the baseGlyphArray for a given GID. */
			char msg[1024];
			featGlyphDump(g, cur->gid, ':', 0);
			featGlyphDump(g, prev->gid, '\0', 0);
			if (fileName != NULL) {
				sprintf(msg, " [%s current %ld previous %ld]", fileName, cur->lineNum, prev->lineNum);
			}
			else {
				msg[0] = '\0';
			}
			hotMsg(g, hotERROR, "MarkToBase or MarkToMark error 0: A previous statement has already defined the anchors and marks on this the same glyph '%s'. Skipping rule. %s",
			       g->note.array, msg);
		}
		else {
			/* For mark to ligature, each successive base glyph entry defines the next conponent. We just need to make sure that
			   no anchors overlap. */
			prev = cur;
		}
	}
	while (cur++ != last);


	return;
}

static void checkBaseLigatureConflict(hotCtx g, BaseGlyphRec *baseGlyphArray, long recCnt, char *fileName, int isMarkToLigature) {
	BaseGlyphRec *prev, *cur, *last;
	if (recCnt < 2) {
		return;
	}

	/* Sort by GID, then component index */
	qsort(baseGlyphArray, recCnt, sizeof(BaseGlyphRec), cmpLigBaseRec);
	prev = &(baseGlyphArray[0]);
	cur = &(baseGlyphArray[1]);
	last =  &(baseGlyphArray[recCnt - 1]);
	/* Now step through, and report if for any entry, the previous entry with the same GID has  the same component index. */
	do {
		if (cur->gid == prev->gid) {
			unsigned short cpi1, cpi2;
			cpi1 = prev->anchorMarkInfo.array[0].componentIndex;
			cpi2 = cur->anchorMarkInfo.array[0].componentIndex;
			if (cpi1 == cpi2) {
				char msg[1024];
				featGlyphDump(g, cur->gid, ':', 0);
				if (fileName != NULL) {
					sprintf(msg, " [%s current %ld previous %ld]", fileName, cur->lineNum, prev->lineNum);
				}
				else {
					msg[0] = '\0';
				}
				hotMsg(g, hotERROR, "MarkToLigature error 0: A previous statement has already defined mark to component attachments for this same ligature glyph '%s'. Skipping rule. %s",
				       g->note.array, msg);
			}
		}
		else {
			prev = cur;
		}
	}
	while (cur++ != last);


	return;
}

static int findMarkClassIndex(SubtableInfo *si, GNode *markNode) {
	short i;

	for (i = 0; i < si->markClassList.cnt; i++) {
		if (markNode == si->markClassList.array[i].gnode) {
			return i;
		}
	}
	return -1;
}

static int addMarkClass(hotCtx g, SubtableInfo *si, GNode *markNode, char *fileName, int lineNum) {
	GNode *nextClass = markNode;
	int ci = si->markClassList.cnt;
	int i;

	/* validate that there is no overlap between this class and previous classes; if not, add the MarkRec entry */
	while (nextClass != NULL) {
		GID newGID = nextClass->gid;

		for (i = 0; i < si->markClassList.cnt; i++) {
			GNode *curNode = si->markClassList.array[i].gnode;
			while (curNode != NULL) {
				if (curNode->gid == newGID) {
					char msg[1024];
					GNode *prevMarkNode =  si->markClassList.array[i].gnode;
					featGlyphDump(g, newGID, '\0', 0);
					if (fileName != NULL) {
						sprintf(msg, " [%s %d]", fileName, lineNum);
					}
					else {
						msg[0] = '\0';
					}
					hotMsg(g, hotERROR, "Glyph '%s' occurs in two different mark classes. Previous mark class: %s. Current mark class %s. %s%s",
					       g->note.array, prevMarkNode->markClassName, markNode->markClassName,  msg);
					ci = -1;
				}
				curNode = curNode->nextCl;
			}     /* end for all gnodes in markClassList.array[markRec->class].gnode */
		}     /* end for all previously seen classes for this lookup */

		nextClass = nextClass->nextCl;
	}     /* end all nodes in the new mark class */

	{
		/* Now validate that all glyph ID's are unique within the new class */
		nextClass = markNode;
		while (nextClass != NULL) {
			GNode *testNode = nextClass->nextCl;
			while (testNode != NULL) {
				if (testNode->gid == nextClass->gid) {
					char msg[1024];
					featGlyphDump(g, testNode->gid, '\0', 0);
					if (fileName != NULL) {
						sprintf(msg, " [%s %d]", fileName, lineNum);
					}
					else {
						msg[0] = '\0';
					}
					hotMsg(g, hotERROR, "The same glyph '%s' is repeated in the current class definition.  mark class %s. %s",
					       g->note.array, markNode->markClassName,  msg);
					ci = -1;
				}
				testNode = testNode->nextCl;
			}
			nextClass = nextClass->nextCl;
		}
	}
	/* If we got here, there was no GID conflict.*/
	/* now add the mark class head node to the  markClassList */
	{
		MarKClassRec *markRec = dnaNEXT(si->markClassList);
		markRec->gnode = markNode;
	}
	return ci;
}

static int CDECL cmpMarkRec(const void *first, const void *second) {
	GID gid1, gid2;

	gid1 = ((MarkRecord *)first)->gid;
	gid2 = ((MarkRecord *)second)->gid;
	if (gid1 == gid2) {
		return 0;
	}
	else if (gid1 > gid2) {
		return 1;
	}
	else {
		return -1;
	}
}

static LOffset getAnchoOffset(hotCtx g, AnchorMarkInfo *anchor, void *fmt) {
	long i = 0;
	MarkBasePosFormat1 *localFmt = (MarkBasePosFormat1 *)fmt;
	AnchorListRec *anchorRec = NULL;

	if (localFmt->anchorList.cnt == 0) {
		anchorRec = dnaNEXT(localFmt->anchorList);
		anchorRec->anchor = *anchor;
		anchorRec->offset = 0;
		return anchorRec->offset;
	}

	while (i < localFmt->anchorList.cnt) {
		if (cmpAnchors(&localFmt->anchorList.array[i].anchor, anchor) == 0) {
			break;
		}
		i++;
	}

	if (i == localFmt->anchorList.cnt) {
		/* did not find the anchor in the list. Add it */
		AnchorListRec *prevAnchorRec = &localFmt->anchorList.array[i - 1];
		anchorRec = dnaNEXT(localFmt->anchorList);
		anchorRec->anchor = *anchor;
		anchorRec->offset = prevAnchorRec->offset;
		if (prevAnchorRec->anchor.format == 2) {
			anchorRec->offset += uint16 * 4;
		}
		else {
			anchorRec->offset += uint16 * 3;
		}
	}
	else {
		anchorRec = &localFmt->anchorList.array[i];
	}
	return anchorRec->offset;
}

static void GPOSAddMark(hotCtx g, SubtableInfo *si, GNode *targ, int anchorCount, AnchorMarkInfo *anchorMarkInfo, char *fileName,  long lineNum) {
	GNode *nextNode = targ;
	BaseGlyphRec *baseRec = NULL;
	/* Ligatures have multiple components.
	   We can tell when we hit a new component, as the anchorMarkInfo componentIndex increases.
	   For mark-to-base and mark-to-mark, there is only one component, and only one baseRec.
	 */
	while (nextNode != NULL) {
		int j;
		int prevComponentIndex = -1;

		for (j = 0; j < anchorCount; j++) {
			int markClassIndex;

			if (prevComponentIndex != anchorMarkInfo[j].componentIndex) {
				baseRec = dnaNEXT(si->baseList);
				dnaINIT(g->dnaCtx, baseRec->anchorMarkInfo, 4, 4);
				baseRec->anchorMarkInfo.func = initAnchor;
				baseRec->gid =  nextNode->gid;
				baseRec->lineNum = lineNum;
				prevComponentIndex = anchorMarkInfo[j].componentIndex;
			}
			{
				AnchorMarkInfo *baseAR = dnaNEXT(baseRec->anchorMarkInfo);
				*baseAR = anchorMarkInfo[j];
				if (baseAR->markClass == NULL) {
					/* happens with null anchors in mark to ligature. */
					continue;
				}
				markClassIndex =  findMarkClassIndex(si, baseAR->markClass);
				if (markClassIndex < 0) {
					markClassIndex = addMarkClass(g, si, baseAR->markClass, fileName, lineNum);
				}
				baseAR->markClassIndex = markClassIndex;
			}
		}
		nextNode = nextNode->nextCl;
	}
}

static void fillMarkToBase(hotCtx g, GPOSCtx h) {
	long i;
	Subtable *sub;
	otlTbl otl;
	LOffset size = MARK_TO_BASE_1_SIZE;
	LOffset markAnchorSize;
	unsigned short numMarkGlyphs = 0;
	MarkBasePosFormat1 *fmt = MEM_NEW(g, sizeof(MarkBasePosFormat1));
	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	fmt->PosFormat = 1;
	fmt->ClassCount =  (unsigned short)h->new.markClassList.cnt;
	dnaINIT(g->dnaCtx, fmt->anchorList, 100, 100);
	fmt->anchorList.func = initAnchorListRec;
	/* Build mark coverage list from list of mark classes.
	   Each mark class is a linked list of nodes. We have already validated that there is no overlap in GID's.
	   We step through the classes. For each class, we step through each node,  add its GID to the  Coverage table,
	   and its info to the mark record list.
	 */
	{
		int i;
		otlCoverageBegin(g, otl);
		markAnchorSize = 0;
		for (i = 0; i < h->new.markClassList.cnt; i++) {
			GNode *nextNode = (h->new.markClassList.array)[i].gnode;
			while (nextNode != NULL) {
				otlCoverageAddGlyph(g, otl, nextNode->gid);
				numMarkGlyphs++;
				nextNode = nextNode->nextCl;
			}
		}
		fmt->MarkCoverage =  otlCoverageEnd(g, otl); /* otlCoverageEnd does the sort by GID */
	}

	/* Now we know how many mark nodes there are, we can buld the MarkArray table, and get its size.
	   We will keep things simple, and write the MarkArray right after the MarkToBase subtable,
	   followed by the BaseArray table, followed by a list of the anchor tables.
	 */
	{
		MarkRecord *nextRec;
		long markArraySize = uint16 + (numMarkGlyphs * (2 * uint16));
		fmt->MarkArray  = (Offset)size; /* offset from the start of the MarkToBase subtable */
		fmt->MarkArray_.MarkCount  = numMarkGlyphs;
		fmt->MarkArray_.MarkRecord = nextRec = MEM_NEW(g, sizeof(MarkRecord) * numMarkGlyphs);
		for (i = 0; i < h->new.markClassList.cnt; i++) {
			GNode *nextNode = (h->new.markClassList.array)[i].gnode;
			while (nextNode != NULL) {
				nextRec->gid = nextNode->gid;
				nextRec->anchor = getAnchoOffset(g, &nextNode->markClassAnchorInfo, fmt); /* offset from start of anchor list */
				nextRec->Class = (unsigned short)i;
				nextNode = nextNode->nextCl;
				nextRec++;
			}
		}
		size += markArraySize;
	}
	qsort(fmt->MarkArray_.MarkRecord, numMarkGlyphs, sizeof(MarkRecord), cmpMarkRec);

	/* Build base glyph coverage list from rules.
	   First, we need to sort the base record list by GID, and make sure there are no errors
	   Then we can simply step through it to make the base coverage table.
	 */
	checkBaseAnchorConflict(g, h->new.baseList.array, h->new.baseList.cnt, h->new.fileName, 0);
	{
		long numBaseGlyphs = 0;
		BaseRecord *nextRec;
		long baseArraySize;
		GID prevGID;
		AnchorMarkInfo kDefaultAnchor = {
			0, 0, 0, 1, NULL, 0
		};

		fmt->BaseArray = (Offset)size; /* offset from the start of the MarkToBase subtable = size of subtable + size of MarkArray table. */
		fmt->BaseArray_.BaseRecord = nextRec = MEM_NEW(g, sizeof(BaseRecord) * h->new.baseList.cnt);
		prevGID =  h->new.baseList.array[0].gid;

		otlCoverageBegin(g, otl);
		baseArraySize =  uint16; /* size of BaseArray.BaseCount */
		/* Note: I have to add the size of the AnchorArray for each baseRec individually, as they can be different sizes. */
		for (i = 0; i < h->new.baseList.cnt; i++) {
			int j;
			BaseGlyphRec *baseRec = &(h->new.baseList.array[i]);
			if ((i > 0) && (prevGID == baseRec->gid)) {
				char msg[1024];
				featGlyphDump(g, prevGID, '\0', 0);
				if (h->new.fileName != NULL) {
					sprintf(msg, " [%s %ld]", h->new.fileName, baseRec->lineNum);
				}
				else {
					msg[0] = '\0';
				}
				hotMsg(g, hotERROR, "MarkToBase or MarkToMark error 1: A previous statement has already defined the anchors and marks for the current base glyph '%s'. Skipping rule. %s",
				       g->note.array, msg);
			}
			else {
				/* Add new glyph entry*/
				long blockSize;

				/* we are seeing a new glyph ID; need to allocate the anchor tables for it, and add it to the coverage table. */
				otlCoverageAddGlyph(g, otl, baseRec->gid);
				nextRec = &(fmt->BaseArray_.BaseRecord[numBaseGlyphs]);
				baseArraySize += fmt->ClassCount * uint16; /* this adds fmt->ClassCount offset values to the output.*/
				numBaseGlyphs++;
				blockSize = sizeof(LOffset) * fmt->ClassCount;
				nextRec->BaseAnchorArray =  MEM_NEW(g, blockSize);
				memset(nextRec->BaseAnchorArray, 0xFF,  blockSize); /* Flag unused records with xFF, as we may add values to only some of the anchor tables in the block. */
				prevGID =  baseRec->gid;
			}

			/* Check uniqueness of anchor vs mark class*/
			for (j = 0; j < baseRec->anchorMarkInfo.cnt; j++) {
				if (nextRec->BaseAnchorArray[baseRec->anchorMarkInfo.array[j].markClassIndex] != 0xFFFFFFFFL) {
					/*it has already been filled in !*/
					char msg[1024];
					featGlyphDump(g, prevGID, '\0', 0);
					if (h->new.fileName != NULL) {
						sprintf(msg, " [%s %ld]", h->new.fileName, baseRec->lineNum);
					}
					else {
						msg[0] = '\0';
					}
					hotMsg(g, hotERROR, "MarkToBase or MarkToMark error 2: A previous statement has already assigned the current mark class to another anchor point on the same glyph '%s'. Skipping rule. %s",
					       g->note.array, msg);
				}
				else {
					nextRec->BaseAnchorArray[baseRec->anchorMarkInfo.array[j].markClassIndex] = getAnchoOffset(g, &baseRec->anchorMarkInfo.array[j], fmt);     /* offset from start of anchor list */
				}
			}
		}     /* end for each base glyph entry */

		/* For any glyph, the user may not have specified an anchor for any particular mark class.
		   We need to fill these in with default anchors at 0.0 */

		for (i = 0; i  < numBaseGlyphs; i++) {
			int j;
			LOffset *baseAnchorArray = fmt->BaseArray_.BaseRecord[i].BaseAnchorArray;

			for (j = 0; j < fmt->ClassCount; j++) {
				if (baseAnchorArray[j] == 0xFFFFFFFFL) {
					baseAnchorArray[j] = getAnchoOffset(g, &kDefaultAnchor, fmt); /* this returns the offset from the start of the anchor list. To be adjusted later*/
				}
			}
		}
		fmt->BaseArray_.BaseCount = (unsigned short)numBaseGlyphs;
		size += baseArraySize;
		fmt->BaseCoverage =  otlCoverageEnd(g, otl); /* otlCoverageEnd does the sort by GID */
	}

	fmt->endArrays = size;
	/* Now add the size of the anchor list*/
	{
		AnchorListRec *anchorRec = &fmt->anchorList.array[fmt->anchorList.cnt - 1];
		size += anchorRec->offset;
		if (anchorRec->anchor.format == 2) {
			size += uint16 * 4;
		}
		else {
			size += uint16 * 3;
		}
	}

	if (sub->extension.use) {
		fmt->MarkCoverage += size; /* Adjust offset */
		fmt->BaseCoverage += size; /* Adjust offset */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	if (h->offset.subtable > 0xFFFF) {
		hotMsg(g, hotFATAL, "MarkToBase lookup subtable in GPOS "
		       "feature '%c%c%c%c' causes offset overflow.",
		       TAG_ARG(h->new.feature));
	}

	sub->tbl = fmt;
}

static void writeMarkToBase(hotCtx g, GPOSCtx h, Subtable *sub) {
	long i, j;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	MarkBasePosFormat1 *fmt = sub->tbl;
	int isExt = sub->extension.use;
	long markClassCnt = fmt->ClassCount;
	MarkRecord *markRec;
	LOffset anchorListOffset;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	fmt->MarkCoverage += adjustment; /* Adjust offset */
	fmt->BaseCoverage += adjustment; /* Adjust offset */

	OUT2(fmt->PosFormat);
	CHECK4OVERFLOW(fmt->MarkCoverage, "mark coverage", "MarkToBase");
	OUT2((Offset)fmt->MarkCoverage);
	CHECK4OVERFLOW(fmt->BaseCoverage, "base coverage", "MarkToBase");
	OUT2((Offset)fmt->BaseCoverage);
	OUT2(fmt->ClassCount);
	OUT2(fmt->MarkArray);
	OUT2(fmt->BaseArray);
	/* Now write out MarkArray */
	OUT2(fmt->MarkArray_.MarkCount);
	/* Now write out MarkRecs */
	anchorListOffset = fmt->endArrays - fmt->MarkArray;
	markRec = &fmt->MarkArray_.MarkRecord[0];
	for (i = 0; i < fmt->MarkArray_.MarkCount; i++) {
		OUT2(markRec->Class);
		OUT2((Offset)(markRec->anchor + anchorListOffset));
		markRec++;
	}

	/* Now write out BaseArray */
	OUT2(fmt->BaseArray_.BaseCount);
	anchorListOffset = fmt->endArrays - fmt->BaseArray;
	/* Now write out BaseRecs */
	for (i = 0; i < fmt->BaseArray_.BaseCount; i++) {
		BaseRecord *baseRec = &fmt->BaseArray_.BaseRecord[i];
		for (j = 0; j < markClassCnt; j++) {
			OUT2((Offset)(baseRec->BaseAnchorArray[j] + anchorListOffset));
		}
	}

	/* Now write out the  anchor list */
	for (i = 0; i < fmt->anchorList.cnt; i++) {
		AnchorMarkInfo *anchorRec = &(fmt->anchorList.array[i].anchor);
		if (anchorRec->format == 0) {
			anchorRec->format = 1; /* This is an anchor record that never got filled in, or was specified as a NULL anchor.
		                              In either case, it is written as a default anchor with x=y =0 */
		}
		OUT2(anchorRec->format);  /*offset to anchor */
		OUT2(anchorRec->x);  /*offset to anchor */
		OUT2(anchorRec->y);  /*offset to anchor */
		if (anchorRec->format == 2) {
			OUT2(anchorRec->contourpoint);  /*offset to anchor */
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeMarkToBase(hotCtx g, Subtable *sub) {
	long i;
	MarkBasePosFormat1 *fmt = sub->tbl;

	for (i = 0; i < fmt->BaseArray_.BaseCount; i++) {
		BaseRecord *baseRec = &fmt->BaseArray_.BaseRecord[i];
		MEM_FREE(g, baseRec->BaseAnchorArray);
	}
	MEM_FREE(g, fmt->BaseArray_.BaseRecord);
	MEM_FREE(g, fmt->MarkArray_.MarkRecord);
	dnaFREE(fmt->anchorList);
	MEM_FREE(g, fmt);
}

/* ---------------------- Mark To Ligature Attachment ---------------------- */

static void fillMarkToLigature(hotCtx g, GPOSCtx h) {
	int i, j;
	Subtable *sub;
	otlTbl otl;
	LOffset size = MARK_TO_BASE_1_SIZE;
	LOffset markhAnchorSize;
	long numMarkGlyphs = 0;
	MarkLigaturePosFormat1 *fmt = MEM_NEW(g, sizeof(MarkLigaturePosFormat1));
	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	fmt->PosFormat = 1;
	fmt->ClassCount =  (unsigned short)h->new.markClassList.cnt;
	dnaINIT(g->dnaCtx, fmt->anchorList, 100, 100);
	fmt->anchorList.func = initAnchorListRec;
	/* Build mark coverage list from list of mark classes.
	   Each mark class is a linked list of nodes. We have already validated that there is no overlap in GID's.
	   We step through the classes. For each class, we step through each node,  add its GID to the  Coverage table,
	   and its info to the mark record list.
	 */
	{
		otlCoverageBegin(g, otl);
		markhAnchorSize = 0;
		for (i = 0; i < h->new.markClassList.cnt; i++) {
			GNode *nextNode = (h->new.markClassList.array)[i].gnode;
			while (nextNode != NULL) {
				otlCoverageAddGlyph(g, otl, nextNode->gid);
				numMarkGlyphs++;
				nextNode = nextNode->nextCl;
			}
		}
		fmt->MarkCoverage =  otlCoverageEnd(g, otl); /* otlCoverageEnd does the sort by GID */
	}

	/* Now we know how many mark nodes there are, we can buld the MarkArray table, and get its size.
	   We will keep things simple, and write the MarkArray right after the MarkLigature subtable,
	   followed by the LigatureArray table, followed by a list of the anchor tables.
	 */
	{
		MarkRecord *nextRec;
		long markArraySize = uint16 + (numMarkGlyphs * (2 * uint16));
		fmt->MarkArray  = (Offset)size; /* offset from the start of the MarkToBase subtable */
		fmt->MarkArray_.MarkCount  = (unsigned short)numMarkGlyphs;
		fmt->MarkArray_.MarkRecord = nextRec = MEM_NEW(g, sizeof(MarkRecord) * numMarkGlyphs);
		for (i = 0; i < h->new.markClassList.cnt; i++) {
			GNode *nextNode = (h->new.markClassList.array)[i].gnode;
			while (nextNode != NULL) {
				nextRec->gid = nextNode->gid;
				nextRec->anchor = getAnchoOffset(g, &nextNode->markClassAnchorInfo, fmt); /* offset from start of anchor list */
				nextRec->Class = i;
				nextNode = nextNode->nextCl;
				nextRec++;
			}
		}
		size += markArraySize;
	}
	qsort(fmt->MarkArray_.MarkRecord, numMarkGlyphs, sizeof(MarkRecord), cmpMarkRec);

	/* Build ligature glyph coverage list from rules.
	   First, we need to sort the base record list by GID, and make sure there are no errors
	   Then we can simply step thorugh it to make the base coverage table.
	 */
	/* sort the recs by base gid value, then by component index. */
	checkBaseLigatureConflict(g, h->new.baseList.array, h->new.baseList.cnt, h->new.fileName, 0);
	{
		long numLigatureGlyphs = 0;
		unsigned short numComponents;
		LigatureAttach *nextRec;
		long ligArraySize;
		int componentIndex;
		GID prevGID;
		fmt->LigatureArray = (Offset)size; /* offset from the start of the MarkLigature subtable = size of subtable + size of MarkArray table. */
		fmt->LigatureArray_.LigatureAttach =  MEM_NEW(g, uint16 * h->new.baseList.cnt); /* this is guaranteed to be larger than we need, as there are several baseList entries for each BaseRecord, aka ligature glyph */
		fmt->LigatureArray_.LigatureAttach_ = nextRec = MEM_NEW(g, sizeof(LigatureAttach) * h->new.baseList.cnt); /* this is guaranteed to be larger than we need, for same reason as above. */
		prevGID =  h->new.baseList.array[0].gid;

		otlCoverageBegin(g, otl);
		ligArraySize =  uint16; /* size of LigatureArray.LigatureCount */
		/* Note: I have to add the size of the AnchorArray for each baseRec individually, as they can be different sizes. */
		i = -1;
		prevGID = 0;
		componentIndex = 0;
		while (++i <  h->new.baseList.cnt) {
			BaseGlyphRec *baseRec = &(h->new.baseList.array[i]);
			GID curGID = baseRec->gid;

			if ((i == 0) || (prevGID != baseRec->gid)) {
				long blockSize;
				prevGID = curGID;
				componentIndex = 0;
				/* First, count the number of baseRecs for this base glyph. There is one base rec for each component */
				numComponents =  1;
				while ((i + numComponents) < h->new.baseList.cnt) {
					if (curGID != h->new.baseList.array[i + numComponents].gid) {
						break;
					}
					numComponents++;
				}

				/* we are seeing a new glyph ID; need to allocate the anchor tables for it, and add it to the coverage table. */
				otlCoverageAddGlyph(g, otl, baseRec->gid);
				nextRec = &(fmt->LigatureArray_.LigatureAttach_[numLigatureGlyphs]);
				nextRec->ComponentCount = numComponents;
				blockSize = sizeof(LOffset) * fmt->ClassCount * nextRec->ComponentCount;
				nextRec->ComponentRecordList = MEM_NEW(g, blockSize); /*this adds fmt->ClassCount* ComponentCount offset values */
				memset(nextRec->ComponentRecordList, 0xFF,  blockSize); /* Flag unused records with xFF, as we may add values to only some of the anchor tables in the block. */
				fmt->LigatureArray_.LigatureAttach[numLigatureGlyphs] = (Offset)ligArraySize; /* currently just offset from start of LigatureAttach offset list */
				ligArraySize += uint16 + (numComponents * fmt->ClassCount * uint16); /* (LigatureAttach.ComponentCount) + (ComponentCount *fmt->ClassCount offset values).*/
				numLigatureGlyphs++;
			}     /* end if is new gid */
			else {
				componentIndex++;
			}
			/* Check uniqueness of anchors in the baseRec, and fill in the ligature attach table */
			for (j = 0; j < baseRec->anchorMarkInfo.cnt; j++) {
				LOffset *ligatureAnchor = &nextRec->ComponentRecordList[componentIndex * fmt->ClassCount];
				if (ligatureAnchor[baseRec->anchorMarkInfo.array[j].markClassIndex] != 0xFFFFFFFFL) {
					/*it has already been filled in !*/
					char msg[1024];
					featGlyphDump(g, curGID, '\0', 0);
					if (h->new.fileName != NULL) {
						sprintf(msg, " [%s %ld]", h->new.fileName, baseRec->lineNum);
					}
					else {
						msg[0] = '\0';
					}
					hotMsg(g, hotERROR, "MarkToLigature statement for glyph '%s' contains a duplicate or conflicting mark class assignment. Skipping rule. %s",
					       g->note.array, msg);
				}
				else {
					if (baseRec->anchorMarkInfo.array[j].format != 0) {
						/* Skip anchor if the format is 0 aka NULL anchor */
						ligatureAnchor[baseRec->anchorMarkInfo.array[j].markClassIndex] = getAnchoOffset(g, &baseRec->anchorMarkInfo.array[j], fmt); /* offset from start of anchor list */
					}
				}
			}     /* end for loop */
		} /* end for allBaseGlyphRecs */

		fmt->LigatureArray_.LigatureCount = (unsigned short)numLigatureGlyphs;
		ligArraySize += uint16 * numLigatureGlyphs; /* the number of LigatureAttach offsets */
		size += ligArraySize;
		fmt->LigatureCoverage =  otlCoverageEnd(g, otl); /* otlCoverageEnd does the sort by GID */
	}

	fmt->endArrays = size;
	/* Now add the size of the anchor list*/
	{
		AnchorListRec *anchorRec = &fmt->anchorList.array[fmt->anchorList.cnt - 1];
		size += anchorRec->offset;
		if (anchorRec->anchor.format == 2) {
			size += uint16 * 4;
		}
		else {
			size += uint16 * 3;
		}
	}

	if (sub->extension.use) {
		fmt->MarkCoverage += size; /* Adjust offset */
		fmt->LigatureCoverage += size; /* Adjust offset */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	if (h->offset.subtable > 0xFFFF) {
		hotMsg(g, hotFATAL, "MarkToBase lookup subtable in GPOS "
		       "feature '%c%c%c%c' causes offset overflow.",
		       TAG_ARG(h->new.feature));
	}

	sub->tbl = fmt;
}

static void writeMarkToLigature(hotCtx g, GPOSCtx h, Subtable *sub) {
	long i, j, k;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	MarkLigaturePosFormat1 *fmt = sub->tbl;
	int isExt = sub->extension.use;
	long markClassCnt = fmt->ClassCount;
	MarkRecord *markRec;
	LOffset anchorListOffset;
	LOffset ligAttachOffsetSize;
	long numLigatures;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	fmt->MarkCoverage += adjustment; /* Adjust offset */
	fmt->LigatureCoverage += adjustment; /* Adjust offset */

	OUT2(fmt->PosFormat);
	CHECK4OVERFLOW(fmt->MarkCoverage, "mark coverage", "MarkToLigature");
	OUT2((Offset)fmt->MarkCoverage);
	CHECK4OVERFLOW(fmt->LigatureCoverage, "ligature coverage", "MarkToLigature");
	OUT2((Offset)fmt->LigatureCoverage);
	OUT2(fmt->ClassCount);
	OUT2(fmt->MarkArray);
	OUT2(fmt->LigatureArray);
	/* Now write out MarkArray */
	OUT2(fmt->MarkArray_.MarkCount);
	/* Now write out MarkRecs */
	anchorListOffset = fmt->endArrays - fmt->MarkArray;
	markRec = &fmt->MarkArray_.MarkRecord[0];
	for (i = 0; i < fmt->MarkArray_.MarkCount; i++) {
		OUT2(markRec->Class);
		OUT2((Offset)(markRec->anchor + anchorListOffset));
		markRec++;
	}

	/* Now write out LigatureArray */
	numLigatures = fmt->LigatureArray_.LigatureCount;
	OUT2((short)numLigatures);
	ligAttachOffsetSize = uint16 * numLigatures;
	/* fmt->endArrays is the position of the start of the anchor List.
	   fmt->endArrays - fmt->LigatureArray is the offfset from the fmt->LigatureArray to the start of the anchor table.
	 */
	anchorListOffset = fmt->endArrays - fmt->LigatureArray;

	/*Now write out the offsets to the LigatureAttach recs*/
	for (i = 0; i < numLigatures; i++) {
		fmt->LigatureArray_.LigatureAttach[i] += (Offset)ligAttachOffsetSize;
		OUT2((Offset)fmt->LigatureArray_.LigatureAttach[i]);
	}

	/* Now write the Ligature Attach recs */
	for (i = 0; i < numLigatures; i++) {
		LigatureAttach *ligAttachRec = &fmt->LigatureArray_.LigatureAttach_[i];
		long compCnt = ligAttachRec->ComponentCount;
		OUT2((short)compCnt);
		for (j = 0; j < compCnt; j++) {
			for (k = 0; k < markClassCnt; k++) {
				LOffset *anchorOffset = &ligAttachRec->ComponentRecordList[j * markClassCnt];
				if (anchorOffset[k] == 0xFFFFFFFFL) {
					OUT2(0);
				}
				else {
					/* anchorOffset[k] + anchorListOffset is the offset from teh start of the LigatureArray to the offset.
					   We then need to subtract the offset from the start of the LigatureArray table to teh LigatureAttach record.
					 */
					OUT2((Offset)(anchorOffset[k] + (anchorListOffset - fmt->LigatureArray_.LigatureAttach[i])));
				}
			}
		}
	}

	/* Now write out the  anchor list */
	for (i = 0; i < fmt->anchorList.cnt; i++) {
		AnchorMarkInfo *anchorRec = &(fmt->anchorList.array[i].anchor);
		OUT2(anchorRec->format);  /*offset to anchor */
		OUT2(anchorRec->x);  /*offset to anchor */
		OUT2(anchorRec->y);  /*offset to anchor */
		if (anchorRec->format == 2) {
			OUT2(anchorRec->contourpoint);  /*offset to anchor */
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeMarkToLigature(hotCtx g, Subtable *sub) {
	long i;
	MarkLigaturePosFormat1 *fmt = sub->tbl;

	for (i = 0; i < fmt->LigatureArray_.LigatureCount; i++) {
		LigatureAttach *nextRec = &(fmt->LigatureArray_.LigatureAttach_[i]);
		MEM_FREE(g, nextRec->ComponentRecordList);
	}
	MEM_FREE(g, fmt->LigatureArray_.LigatureAttach);
	MEM_FREE(g, fmt->LigatureArray_.LigatureAttach_);
	MEM_FREE(g, fmt->MarkArray_.MarkRecord);
	dnaFREE(fmt->anchorList);
	MEM_FREE(g, fmt);
}

/* ------------------------ Cursive Attachment ------------------------ */

static void GPOSAdCursive(hotCtx g, SubtableInfo *si, GNode *targ, int anchorCount, AnchorMarkInfo *anchorMarkInfo, char *fileName, long lineNum) {
	GNode *nextNode = targ;

	while (nextNode != NULL) {
		int j;
		BaseGlyphRec *baseRec = dnaNEXT(si->baseList);
		dnaINIT(g->dnaCtx, baseRec->anchorMarkInfo, 4, 4);
		baseRec->anchorMarkInfo.func = initAnchor;
		baseRec->gid =  nextNode->gid;

		for (j = 0; j < anchorCount; j++) {
			AnchorMarkInfo *baseAR = dnaNEXT(baseRec->anchorMarkInfo);
			*baseAR = anchorMarkInfo[j];
		}
		baseRec->lineNum = lineNum;
		nextNode = nextNode->nextCl;
	}
}

static void fillCursive(hotCtx g, GPOSCtx h) {
	long i;
	Subtable *sub;
	otlTbl otl;
	LOffset size = CURSIVE_1_SIZE;
	CursivePosFormat1 *fmt;

	fmt = MEM_NEW(g, sizeof(CursivePosFormat1));
	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	fmt->PosFormat = 1;
	dnaINIT(g->dnaCtx, fmt->anchorList, 100, 100);
	fmt->anchorList.func = initAnchorListRec;
	qsort(h->new.baseList.array,  h->new.baseList.cnt, sizeof(BaseGlyphRec), cmpBaseRec);  /* Get them in GID order, so the recs will match the Coverage order */
	{
		long numRecs = 0;
		BaseGlyphRec *prevRec = NULL;
		fmt->EntryExitRecord = MEM_NEW(g, sizeof(EntryExitRecord) * h->new.baseList.cnt); /* This will certainly be long enough; it may be too long, if there are duplicates */
		otlCoverageBegin(g, otl);
		for (i = 0; i < h->new.baseList.cnt; i++) {
			BaseGlyphRec *baseRec = &(h->new.baseList.array[i]);
			if (prevRec && (prevRec->gid == baseRec->gid)) {
				char msg[1024];
				featGlyphDump(g, prevRec->gid, '\0', 0);
				if (h->new.fileName != NULL) {
					sprintf(msg, " [%s current %ld previous %ld]",  h->new.fileName, baseRec->lineNum, prevRec->lineNum);
				}
				else {
					msg[0] = '\0';
				}
				hotMsg(g, hotERROR, "Cursive statement error: A previous statment has already referenced the same glyph '%s'. Skipping rule. %s",
				       g->note.array, msg);
			}
			else {
				EntryExitRecord *fmtRec = &fmt->EntryExitRecord[numRecs];
				AnchorMarkInfo *anchorInfo;
				otlCoverageAddGlyph(g, otl, baseRec->gid);
				anchorInfo = dnaINDEX(baseRec->anchorMarkInfo, 0);
				if (anchorInfo->format == 0) {
					fmtRec->EntryAnchor = 0xFFFF; /* Can't use 0 as a flag, as 0 is the offset to the first anchor record */
				}
				else {
					fmtRec->EntryAnchor = getAnchoOffset(g, anchorInfo, fmt); /* this returns the offset from the start of the anchor list. To be adjusted later*/
				}
				anchorInfo = dnaINDEX(baseRec->anchorMarkInfo, 1);
				if (anchorInfo->format == 0) {
					fmtRec->ExitAnchor = 0xFFFF; /* Can't use 0 as a flag, as 0 is he offset to the first anchor record */
				}
				else {
					fmtRec->ExitAnchor = getAnchoOffset(g, anchorInfo, fmt);
				}
				numRecs += 1;
			}
			prevRec = baseRec;
		}
		fmt->Coverage =  otlCoverageEnd(g, otl); /* otlCoverageEnd does the sort by GID */
		fmt->EntryExitCount = (unsigned short)numRecs;
		size += numRecs * (2 * uint16);
	}

	fmt->endArrays = size;

	/* Now add the size of the anchor list*/
	{
		AnchorListRec *anchorRec = &fmt->anchorList.array[fmt->anchorList.cnt - 1];
		size += anchorRec->offset;
		if (anchorRec->anchor.format == 2) {
			size += uint16 * 4;
		}
		else {
			size += uint16 * 3;
		}
	}

	if (sub->extension.use) {
		fmt->Coverage += size; /* Adjust offset */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	if (h->offset.subtable > 0xFFFF) {
		hotMsg(g, hotFATAL, "Cursive Attach lookup subtable in GPOS "
		       "feature '%c%c%c%c' causes offset overflow.",
		       TAG_ARG(h->new.feature));
	}

	sub->tbl = fmt;
}

static void writeCursive(hotCtx g, GPOSCtx h, Subtable *sub) {
	long i;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	CursivePosFormat1 *fmt = sub->tbl;
	int isExt = sub->extension.use;
	long recCnt = fmt->EntryExitCount;
	EntryExitRecord *fmtRec;
	LOffset anchorListOffset;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	fmt->Coverage += adjustment; /* Adjust offset */

	anchorListOffset = fmt->endArrays;

	OUT2(fmt->PosFormat);
	OUT2((Offset)fmt->Coverage);
	OUT2((Offset)fmt->EntryExitCount);
	CHECK4OVERFLOW(fmt->Coverage, "cursive coverage", "CursivePos");
	for (i = 0; i < recCnt; i++) {
		fmtRec = &fmt->EntryExitRecord[i];

		if (fmtRec->EntryAnchor ==  0xFFFF) {
			OUT2((Offset)0);
		}
		else {
			OUT2((Offset)(fmtRec->EntryAnchor + anchorListOffset));
		}

		if (fmtRec->ExitAnchor ==  0xFFFF) {
			OUT2((Offset)0);
		}
		else {
			OUT2((Offset)(fmtRec->ExitAnchor + anchorListOffset));
		}
	}

	/* Now write out the  anchor list */
	for (i = 0; i < fmt->anchorList.cnt; i++) {
		AnchorMarkInfo *anchorRec = &(fmt->anchorList.array[i].anchor);
		if (anchorRec->format == 0) {
			continue; /* This is an anchor record that never got filled in, or was specified as a NULL anchor. */
		}
		OUT2(anchorRec->format);  /*offset to anchor */
		OUT2(anchorRec->x);  /*offset to anchor */
		OUT2(anchorRec->y);  /*offset to anchor */
		if (anchorRec->format == 2) {
			OUT2(anchorRec->contourpoint);  /*offset to anchor */
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeCursive(hotCtx g, Subtable *sub) {
	CursivePosFormat1 *fmt = sub->tbl;
	MEM_FREE(g, fmt->EntryExitRecord);
	dnaFREE(fmt->anchorList);
	MEM_FREE(g, fmt);
}

/* ------------------------ Contextual Positioning ------------------------- */

/* ------------------- Chaining Contextual Positioning --------------------- */

/* ------------------- Chaining Contextual Positioning --------------------- */

/* ------------------------ Extension Positioning -------------------------- */

/* Fill extension positioning subtable */

static ExtensionPosFormat1 *fillExtensionPos(hotCtx g, GPOSCtx h,
                                             unsigned ExtensionLookupType) {
	ExtensionPosFormat1 *fmt = MEM_NEW(g, sizeof(ExtensionPosFormat1));

	fmt->PosFormat = 1;
	fmt->ExtensionLookupType = ExtensionLookupType;
	fmt->ExtensionOffset = h->offset.extension; /* Adjusted later */

	h->offset.subtable += EXTENSION1_SIZE;
	return fmt;
}

static void writeExtension(hotCtx g, GPOSCtx h, Subtable *sub) {
	ExtensionPosFormat1 *fmt = sub->extension.tbl;

	/* Adjust offset */
	fmt->ExtensionOffset += h->offset.extensionSection - sub->offset;

	DF(1, (stderr, "  GPOS Extension: fmt=%1d, lkpType=%2d, offset=%08lx\n",
	       fmt->PosFormat, fmt->ExtensionLookupType, fmt->ExtensionOffset));

	OUT2(fmt->PosFormat);
	OUT2(fmt->ExtensionLookupType);
	OUT4(fmt->ExtensionOffset);
}

static void freeExtension(hotCtx g, GPOSCtx h, Subtable *sub) {
	ExtensionPosFormat1 *fmt = sub->extension.tbl;

	otlTableReuse(g, sub->extension.otl);
	otlTableFree(g, sub->extension.otl);
	MEM_FREE(g, fmt);
}
