
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 *	Glyph substitution table.
 */
#include <limits.h>
#include "GSUB.h"
#include "OS_2.h"
#include "otl.h"
#include "map.h"
#include "vmtx.h"
#include "feat.h"
#include "common.h"
#include "name.h"
#include <stdlib.h>
#include <stdio.h>

#define CHECK4OVERFLOW(n, s) do { if ((n) > 0xFFFF) { \
									  hotMsg(g, hotFATAL, "Coverage offset overflow (0x%lx) in %s substitution", \
									         (n), (s)); } \
} \
	while (0)

/* --------------------------- Context Definition -------------------------- */


typedef struct {
	unsigned short SubstFormat;             /* =1 */
	unsigned short ExtensionLookupType;
	LOffset ExtensionOffset;
} ExtensionSubstFormat1;
#define EXTENSION1_SIZE (uint16 * 2 + uint32)

typedef struct {            /* Subtable record */
	Tag script;
	Tag language;
	Tag feature;
	unsigned short lkpType;
	unsigned short lkpFlag;
	unsigned short markSetIndex;
	Label label;
	LOffset offset;         /* From beginning of first subtable */
	struct {                /* Extension-related data */
		short use;          /* Use extension lookupType? If set, then following
		                       used: */
		otlTbl otl;         /* For coverages and classes of this extension
		                       subtable */
		LOffset offset;     /* Offset to this extension subtable, from
		                       beginning of extension section. Debug only. */
		ExtensionSubstFormat1 *tbl;
		/* Subtable data */
	} extension;
	void *tbl;              /* Format-specific subtable data */
} Subtable;

typedef struct {
	GNode *targ;
	GNode *repl;
	unsigned short data;            /* For ligature: length of targ */
} SubstRule;

typedef struct {                    /* New subtable data */
	Tag script;
	Tag language;
	Tag feature;
	short useExtension;             /* Use extension lookupType? */
	unsigned short lkpType;
	unsigned short lkpFlag;
	unsigned short markSetIndex;
	Label label;

	dnaDCL(SubstRule, rules);
	Subtable *sub;                   /* Current subtable */
	char *fileName;                 /* the current feature file name */
} SubtableInfo;

typedef struct {
	unsigned short SequenceIndex;
	unsigned short LookupListIndex;
} SubstLookupRecord;
#define SUBST_LOOKUP_RECORD_SIZE    (uint16 * 2)


typedef struct {            /* Subtable record */
	unsigned short version;
	unsigned short nameID;
} FeatureNameParameterFormat;     /* Special case format for subtable data. */

typedef struct {            /* Subtable record */
	unsigned short Format;
	unsigned short FeatUILabelNameID;
	unsigned short FeatUITooltipTextNameID;
	unsigned short SampleTextNameID;
    unsigned short NumNamedParameters;
	unsigned short FirstParamUILabelNameID;
	dnaDCL(unsigned long, charValues);
} CVParameterFormat;     /* Special case format for subtable data. */
#define CV_PARAM_SIZE(p)    ((uint16 * 7) + 3*(p->charValues.cnt))


typedef struct {
	Tag featTag;
	short unsigned nameID;
} FeatureNameRec;


struct GSUBCtx_ {
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
	dnaDCL(Subtable, subtables);    /* Subtable list */

	/* Info for chaining contextual lookups */
	dnaDCL(SubtableInfo, anonSubtable); /* Anon subtbl accumulator */
	dnaDCL(SubstLookupRecord *, subLookup); /* Ptrs to all records that need
	                                           to be adjusted */
	dnaDCL(GNode *, prod);          /* Tmp for cross product */

	/* info for feature names */
	dnaDCL(FeatureNameRec, featNameID);

	unsigned short maxContext;

	otlTbl otl;                     /* OTL table */
	hotCtx g;                       /* Package context */
};


static void fillGSUBFeatureNameParam(hotCtx g, GSUBCtx h, Subtable *sub);
static void writeGSUBFeatNameParam(GSUBCtx h, Subtable *sub);
static void freeGSUBFeatParam(hotCtx g, Subtable *sub);

static void fillGSUBCVParam(hotCtx g, GSUBCtx h, Subtable *sub);
static void writeGSUBCVParam(GSUBCtx h, Subtable *sub);
static void freeGSUBCVParam(hotCtx g, Subtable *sub);


static void fillSingle(hotCtx g, GSUBCtx h);
static void writeSingle(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeSingle(hotCtx g, GSUBCtx h, Subtable *sub);

#if HOT_FEAT_SUPPORT
static void fillMultiple(hotCtx g, GSUBCtx h);
static void writeMultiple(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeMultiple(hotCtx g, GSUBCtx h, Subtable *sub);

static void fillAlternate(hotCtx g, GSUBCtx h);
static void writeAlternate(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeAlternate(hotCtx g, GSUBCtx h, Subtable *sub);

static void fillLigature(hotCtx g, GSUBCtx h);
static void writeLigature(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeLigature(hotCtx g, GSUBCtx h, Subtable *sub);

static void fillChain(hotCtx g, GSUBCtx h);
static void writeChain(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeChain(hotCtx g, GSUBCtx h, Subtable *sub);

static void fillReverseChain(hotCtx g, GSUBCtx h);
static void writeReverseChain(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeReverseChain(hotCtx g, GSUBCtx h, Subtable *sub);

static void createAnonLookups(hotCtx g, GSUBCtx h);
static void setAnonLookupIndices(hotCtx g, GSUBCtx h);

#endif /* HOT_FEAT_SUPPORT */

static ExtensionSubstFormat1 *fillExtension(hotCtx g, GSUBCtx h,
                                            unsigned ExtensionLookupType);
static void writeExtension(hotCtx g, GSUBCtx h, Subtable *sub);
static void freeExtension(hotCtx g, GSUBCtx h, Subtable *sub);

/* --------------------------- Standard Functions -------------------------- */

/* Element initializer */

static void subtableInit(void *ctx, long count, SubtableInfo *si) {
	hotCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, si->rules, 10, 50);
		si++;
	}
	return;
}

void GSUBNew(hotCtx g) {
	GSUBCtx h = MEM_NEW(g, sizeof(struct GSUBCtx_));

	h->new.script = h->new.language = h->new.feature = TAG_UNDEF;

	dnaINIT(g->dnaCtx, h->new.rules, 50, 200);
	h->offset.subtable = h->offset.extension = h->offset.extensionSection = 0;
	dnaINIT(g->dnaCtx, h->subtables, 10, 10);
	dnaINIT(g->dnaCtx, h->anonSubtable, 3, 10);
	h->anonSubtable.func = subtableInit;
	dnaINIT(g->dnaCtx, h->subLookup, 25, 100);
	dnaINIT(g->dnaCtx, h->prod, 20, 100);
	dnaINIT(g->dnaCtx, h->featNameID, 8, 8);

	h->maxContext = 0;
	h->otl = NULL;

	/* Link contexts */
	h->g = g;
	g->ctx.GSUB = h;
}

int GSUBFill(hotCtx g) {
	int i;
	GSUBCtx h = g->ctx.GSUB;

	if (h->subtables.cnt == 0) {
		if (g->convertFlags & HOT_ALLOW_STUB_GSUB) {
			return 1;
		}
		else {
			return 0;
		}
	}

#if HOT_FEAT_SUPPORT
	createAnonLookups(g, h);
#endif /* HOT_FEAT_SUPPORT */

	/* Add OTL features */
	for (i = 0; i < h->subtables.cnt; i++) {
		Subtable *sub = &h->subtables.array[i];
		int isExt = sub->extension.use;
		int hasFeatureParam;
		hasFeatureParam =  ((sub->lkpType == GSUBFeatureNameParam) ||
                            (sub->lkpType == GSUBCVParam ) );

		otlSubtableAdd(g, h->otl, sub->script, sub->language, sub->feature,
		               isExt ? GSUBExtension : sub->lkpType,
		               sub->lkpFlag,  sub->markSetIndex,
		               isExt ? sub->lkpType : 0,
		               IS_REF_LAB(sub->label) ? 0 : sub->offset,
		               sub->label,
		               (unsigned short)(IS_REF_LAB(sub->label) ? 0 :
		                                isExt ? sub->extension.tbl->SubstFormat
										: *(unsigned short *)sub->tbl),
		               hasFeatureParam);
	}
	DF(1, (stderr, "### GSUB:\n"));

	otlTableFill(g, h->otl);

	h->offset.extensionSection = h->offset.subtable
	    + otlGetCoverageSize(h->otl)
	    + otlGetClassSize(h->otl);
#if HOT_DEBUG
	otlDumpSizes(g, h->otl, h->offset.subtable, h->offset.extension);
#endif /* HOT_DEBUG */

#if HOT_FEAT_SUPPORT
	/* setAnonLookupIndices marks as used not only the anonymous lookups, but also all lookups that were referenced from chain sub rules,
	   including the stand-alone lookups. This is why checkStandAloneTablRefs has to follow setAnonLookupIndices. */
	setAnonLookupIndices(g, h);

	checkStandAloneTablRefs(g, h->otl);
#endif /* HOT_FEAT_SUPPORT */


	OS_2SetMaxContext(g, h->maxContext);

	return 1;
}

void GSUBWrite(hotCtx g) {
	int i;
	GSUBCtx h = g->ctx.GSUB;

	/* Write OTL features */
	if (h->subtables.cnt == 0) {
		/* When (h->subtables.cnt ==0), we get here only if teh -fs option has been specified. */
		h->otl = otlTableNew(g);
		otlTableFillStub(g, h->otl);
		otlTableWrite(g, h->otl);
		return;
	}

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
			case GSUBSingle:    writeSingle(g, h, sub);
				break;

#if HOT_FEAT_SUPPORT
			case GSUBMultiple:  writeMultiple(g, h, sub);
				break;

			case GSUBLigature:  writeLigature(g, h, sub);
				break;

			case GSUBAlternate: writeAlternate(g, h, sub);
				break;

			case GSUBChain:     writeChain(g, h, sub);
				break;

			case GSUBReverse:   writeReverseChain(g, h, sub);
				break;

			case GSUBFeatureNameParam: writeGSUBFeatNameParam(h, sub);
				break;
                
			case GSUBCVParam: writeGSUBCVParam(h, sub);
				break;
                
			case GSUBContext:
				break;
#endif /* HOT_FEAT_SUPPORT */
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
			case GSUBSingle:    writeSingle(g, h, sub);
				break;

#if HOT_FEAT_SUPPORT
			case GSUBMultiple:  writeMultiple(g, h, sub);
				break;

			case GSUBLigature:  writeLigature(g, h, sub);
				break;

			case GSUBAlternate: writeAlternate(g, h, sub);
				break;

			case GSUBChain:     writeChain(g, h, sub);
				break;

			case GSUBReverse:   writeReverseChain(g, h, sub);
				break;

			case GSUBContext:
				break;
#endif /* HOT_FEAT_SUPPORT */
		}
	}
}

void GSUBReuse(hotCtx g) {
	GSUBCtx h = g->ctx.GSUB;
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
			case GSUBSingle:    freeSingle(g, h, sub);
				break;

#if HOT_FEAT_SUPPORT
			case GSUBMultiple:  freeMultiple(g, h, sub);
				break;

			case GSUBLigature:  freeLigature(g, h, sub);
				break;

			case GSUBAlternate: freeAlternate(g, h, sub);
				break;

			case GSUBChain:     freeChain(g, h, sub);
				break;

			case GSUBReverse:   freeReverseChain(g, h, sub);
				break;

			case GSUBFeatureNameParam:  freeGSUBFeatParam(g, sub);
				break;
                
			case GSUBCVParam:  freeGSUBCVParam(g, sub);
				break;
                
			case GSUBContext:
				break;
#endif /* HOT_FEAT_SUPPORT */
		}
	}

	h->new.rules.cnt = 0;
	h->offset.subtable = h->offset.extension = h->offset.extensionSection = 0;
	h->subtables.cnt = 0;

	h->anonSubtable.cnt = 0;
	h->subLookup.cnt = 0;
	h->featNameID.cnt = 0;

	h->maxContext = 0;
	otlTableReuse(g, h->otl);
}

void GSUBFree(hotCtx g) {
	GSUBCtx h = g->ctx.GSUB;
	long i;

	dnaFREE(h->new.rules);
	dnaFREE(h->subtables);

	for (i = 0; i < h->anonSubtable.size; i++) {
		dnaFREE(h->anonSubtable.array[i].rules);
	}
	dnaFREE(h->anonSubtable);
	dnaFREE(h->subLookup);
	dnaFREE(h->prod);
	dnaFREE(h->featNameID);

	otlTableFree(g, h->otl);
	MEM_FREE(g, g->ctx.GSUB);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Begin new feature (can be called multiple times for same feature) */

void GSUBFeatureBegin(hotCtx g, Tag script, Tag language, Tag feature) {
	GSUBCtx h = g->ctx.GSUB;

	DF(2, (stderr, "\n"));
#if 1
	DF(1, (stderr, "{ GSUB '%c%c%c%c', '%c%c%c%c', '%c%c%c%c'\n",
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

static void startNewSubtable(hotCtx g) {
	GSUBCtx h = g->ctx.GSUB;
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
		sub->extension.tbl = fillExtension(g, h, h->new.lkpType);
	}
	else {
		sub->extension.otl = NULL;
		sub->extension.offset = 0;
		sub->extension.tbl = NULL;
	}
}

/* Begin new lookup */

void GSUBLookupBegin(hotCtx g, unsigned lkpType, unsigned lkpFlag, Label label,
                     short useExtension, unsigned short useMarkSetIndex) {
	GSUBCtx h = g->ctx.GSUB;

	DF(2, (stderr,
	       " { GSUB lkpType=%s%d lkpFlag=%d label=%x\n",
	       useExtension ? "EXTENSION:" : "", lkpType, lkpFlag, label));

	h->new.useExtension = useExtension;
	h->new.lkpType = lkpType;
	h->new.lkpFlag = lkpFlag;
	h->new.markSetIndex = useMarkSetIndex;
	h->new.label = label;

	h->new.rules.cnt = 0;
}

#if HOT_DEBUG

static void rulesDump(GSUBCtx h) {
	long i;

	fprintf(stderr, "# Dump lookupType %d rules:\n", h->new.lkpType);
	for (i = 0; i < h->new.rules.cnt; i++) {
		SubstRule *rule = &h->new.rules.array[i];

		fprintf(stderr, "  [%ld] ", i);
		featPatternDump(h->g, rule->targ, ' ', 1);
		featPatternDump(h->g, rule->repl, '\n', 1);
	}
}

#endif

/* Add rule (enumerating if necessary) to subtable si */

static void addRule(hotCtx g, GSUBCtx h, SubtableInfo *si, GNode *targ,
                    GNode *repl) {
	GNode *t;
	GNode *r;
	SubstRule *rule;

#if HOT_DEBUG
	if (DF_LEVEL >= 2) {
		DF(2, (stderr, "  * GSUB RuleAdd "));
		featPatternDump(g, targ, ' ', 1);
		if (repl != NULL) {
			featPatternDump(g, repl, '\n', 1);
		}
	}
#endif

	/* Add rule(s), enumerating if not supported by the OT format */
	if (si->lkpType == GSUBSingle) {
		/* Duplicate repl if a single glyph: */
		if (targ->nextCl != NULL && repl->nextCl == NULL) {
			featExtendNodeToClass(g, repl, featGetGlyphClassCount(g, targ) - 1);
		}

		for (; targ != NULL; targ = t, repl = r) {
			rule = dnaNEXT(si->rules);
			t = targ->nextCl;
			r = repl->nextCl;
			targ->nextCl = NULL;
			repl->nextCl = NULL;
			rule->targ = targ;
			rule->repl = repl;
		}
		return;
	}
#if HOT_FEAT_SUPPORT
	else if (si->lkpType == GSUBLigature) {
		GNode *t;
		unsigned length = featGetPatternLen(g, targ);

		for (t = targ; t != NULL; t = t->nextSeq) {
			if (t->nextCl != NULL) {
				break;
			}
		}

		if (t != NULL) {
			unsigned i;
			unsigned nSeq;
			GNode ***prod = featMakeCrossProduct(g, targ, &nSeq);

			featRecycleNodes(g, targ);
			for (i = 0; i < nSeq; i++) {
				rule = dnaNEXT(si->rules);
				rule->targ = (*prod)[i];
				rule->repl = i == 0 ? repl : featSetNewNode(g, repl->gid);
				rule->data = length;
#if HOT_DEBUG
				if (DF_LEVEL >= 2) {
					fprintf(stderr, "               > ");
					featPatternDump(g, rule->targ, '\n', 1);
				}
#endif
			}
			return;
		}
		else {
			rule = dnaNEXT(si->rules);
			rule->targ = targ;
			rule->repl = repl;
			rule->data = length;
		}
	}
	else {
		/* Add whole rule intact (no enumeration needed) */
		rule = dnaNEXT(si->rules);
		rule->targ = targ;
		rule->repl = repl;
	}
#endif /* HOT_FEAT_SUPPORT */
}

/* Stores input GNodes; they are recycled at GSUBLookupEnd. */

void GSUBRuleAdd(hotCtx g, GNode *targ, GNode *repl) {
	GSUBCtx h = g->ctx.GSUB;

	if (g->hadError) {
		return;
	}

	addRule(g, h, &h->new, targ, repl);
}

/* Break the subtable at this point. Return 0 if successful, else 1. */

int GSUBSubtableBreak(hotCtx g) {
	return 1;
}

/* End lookup */

void GSUBLookupEnd(hotCtx g, Tag feature) {
	GSUBCtx h = g->ctx.GSUB;
	long i;

	DF(2, (stderr, " } GSUB\n"));

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

	if (h->new.rules.cnt == 0) {
		int hasParam = ((h->new.lkpType == GSUBFeatureNameParam) ||
                        (h->new.lkpType == GSUBCVParam));
		if (!hasParam) {
			hotMsg(g, hotFATAL, "Empty GSUB lookup in feature '%c%c%c%c'", TAG_ARG(h->new.feature));
		}
	}

	if (h->otl == NULL) {
		/* Allocate table if not done so already */
		h->otl = otlTableNew(g);
	}

	switch (h->new.lkpType) {
		case GSUBSingle:    fillSingle(g, h);
			break;

#if HOT_FEAT_SUPPORT
		case GSUBMultiple:  fillMultiple(g, h);
			break;

		case GSUBAlternate: fillAlternate(g, h);
			break;

		case GSUBLigature:  fillLigature(g, h);
			break;

		case GSUBChain:     fillChain(g, h);
			break;

		case GSUBReverse:   fillReverseChain(g, h);
			break;

		case GSUBFeatureNameParam: fillGSUBFeatureNameParam(g, h,  h->new.sub);
			break;
            
		case GSUBCVParam: fillGSUBCVParam(g, h,  h->new.sub);
			break;
            
            
            
            
		case GSUBContext:
			hotMsg(g, hotFATAL, "unsupported GSUB lkpType <%d>", h->new.lkpType);

#endif /* HOT_FEAT_SUPPORT */
		default:
			hotMsg(g, hotFATAL, "unknown GSUB lkpType <%d>", h->new.lkpType);
	}

	if (h->offset.subtable > 0xFFFF) {
		hotMsg(g, hotFATAL, "GSUB feature '%c%c%c%c' causes overflow of offset"
		       " to a subtable (0x%lx)", TAG_ARG(h->new.feature),
		       h->offset.subtable);
	}

	/* Recycle Glyph nodes, since formatted tbl has been constructed */
	for (i = 0; i < h->new.rules.cnt; i++) {
		SubstRule *rule = &h->new.rules.array[i];
		featRecycleNodes(g, rule->targ);
		featRecycleNodes(g, rule->repl);
	}
	h->new.rules.cnt = 0;
	/* Ths prevents the rules from being re-used un-intentionally in the
	   case where an emtpy GPOS feature is called for, and becuase it is empty, the table type doesn't get
	   correctly assigned, and the code comes through here.*/
}

/* Performs no action but brackets feature calls */
void GSUBFeatureEnd(hotCtx g) {
	DF(2, (stderr, "} GSUB\n"));
}

void GSUBSetFeatureNameID(hotCtx g, Tag feat, unsigned short nameID) {
	FeatureNameRec *featNameRec;

	GSUBCtx h = g->ctx.GSUB;
	featNameRec = dnaNEXT(h->featNameID);
	featNameRec->featTag = feat;
	featNameRec->nameID = nameID;
}

/* Write feature parameter table */


static void fillGSUBFeatureNameParam(hotCtx g, GSUBCtx h, Subtable *sub) {
	FeatureNameParameterFormat *feat_param = (FeatureNameParameterFormat *)sub->tbl;
	unsigned short nameid = feat_param->nameID;
    unsigned short cvNumber = ((sub->feature >>8 & 0xFF) - (int)'0') << 8;
    cvNumber += (sub->feature & 0xFF) - (int)'0';
    if (((sub->feature >>24 & 0xFF) == (int)'s') &&
        ((sub->feature >>16 & 0xFF) == (int)'s') &&
        (cvNumber >= 0) &&(cvNumber <= 99)
        )
    {
       if (nameid != 0) {
            unsigned short nameIDPresent = nameVerifyDefaultNames(g, nameid);
            if (nameIDPresent && nameIDPresent & MISSING_WIN_DEFAULT_NAME) {
                hotMsg(g, hotFATAL, "Missing Windows default name for for feature name  nameid %i",  nameid);
            }
        }
    }
    else
    {
        hotMsg(g, hotFATAL, "A 'featureNames' block is allowed only Stylistic Set (ssXX) features. It is being used in feature '%c%c%c%c'.",  TAG_ARG(sub->feature));
    }
}

static void writeGSUBFeatNameParam(GSUBCtx h, Subtable *sub) {
	FeatureNameParameterFormat *feat_param;
	feat_param = (FeatureNameParameterFormat *)sub->tbl;

	OUT2(feat_param->version);
	OUT2(feat_param->nameID);
}

static void freeGSUBFeatParam(hotCtx g, Subtable *sub) {
	MEM_FREE(g, sub->tbl);
}

void GSUBAddFeatureMenuParam(hotCtx g, void *param) {
	GSUBCtx h = g->ctx.GSUB;
	Subtable *sub;
	FeatureNameParameterFormat *feat_param = NULL;
	Offset param_size = sizeof(FeatureNameParameterFormat);

	startNewSubtable(g);
	sub = h->new.sub;

	feat_param = MEM_NEW(g, param_size);

	feat_param->version = 0;
	feat_param->nameID = *((unsigned short *)param);
	sub->tbl = feat_param;
	h->offset.subtable += param_size;
}

static void fillGSUBCVParam(hotCtx g, GSUBCtx h, Subtable *sub) {
	CVParameterFormat *feat_param = (CVParameterFormat *)sub->tbl;
    int i = 0;
    unsigned short nameIDs[4];
    unsigned short cvNumber = ((sub->feature >>8 & 0xFF) - (int)'0') << 8;
    cvNumber += (sub->feature & 0xFF) - (int)'0';
    if (((sub->feature >>24 & 0xFF) == (int)'c') &&
        ((sub->feature >>16 & 0xFF) == (int)'v') &&
        (cvNumber >= 0) &&(cvNumber <= 99)
        )
    {
        nameIDs[0]= feat_param->FeatUILabelNameID;
        nameIDs[1]= feat_param->FeatUITooltipTextNameID;
        nameIDs[2]= feat_param->SampleTextNameID;
        nameIDs[3]= feat_param->FirstParamUILabelNameID;
        
        while (i < 4)
        {
            unsigned short nameid = nameIDs[i++];
            if (nameid != 0) {
                unsigned short nameIDPresent = nameVerifyDefaultNames(g, nameid);
                if (nameIDPresent && nameIDPresent & MISSING_WIN_DEFAULT_NAME) {
                    hotMsg(g, hotFATAL, "Missing Windows default name for for feature name  nameid %i",  nameid);
                }
            }
            
        }
    }
    else
    {
        hotMsg(g, hotFATAL, "A 'cvParameters' block is allowed only Character Variant (cvXX) features. It is being used in feature '%c%c%c%c'.",  TAG_ARG(sub->feature));
    }
}

static void writeGSUBCVParam(GSUBCtx h, Subtable *sub) {
    int i = 0;
    CVParameterFormat  *feat_param = (CVParameterFormat *)sub->tbl;
    
	OUT2(feat_param->Format);
	OUT2(feat_param->FeatUILabelNameID);
	OUT2(feat_param->FeatUITooltipTextNameID);
	OUT2(feat_param->SampleTextNameID);
	OUT2(feat_param->NumNamedParameters);
	OUT2(feat_param->FirstParamUILabelNameID);
	OUT2((unsigned short)feat_param->charValues.cnt);
    while  (i < feat_param->charValues.cnt)
    {
        unsigned long uv = feat_param->charValues.array[i++];
        char val1 = (char)(uv >>16);
        unsigned short val2  = (unsigned short)(uv & 0x0000FFFF);
        OUT1(val1);
        OUT2(val2);
    }
}

static void freeGSUBCVParam(hotCtx g, Subtable *sub) {
    CVParameterFormat  *feat_param = (CVParameterFormat *)sub->tbl;
    dnaFREE(feat_param->charValues);
	MEM_FREE(g, sub->tbl);
}

void GSUBAddCVParam(hotCtx g, void *param) {
	GSUBCtx h = g->ctx.GSUB;
	Subtable *sub;
    int i;
	CVParameterFormat *feat_param = (CVParameterFormat*)param;
	CVParameterFormat *new_param;
	Offset param_size = sizeof(CVParameterFormat);
	Offset param_offset = (Offset)CV_PARAM_SIZE(feat_param);
    
	startNewSubtable(g);
	sub = h->new.sub;
    
	new_param = MEM_NEW(g, param_size);
    
	new_param->Format = feat_param->Format;
	new_param->FeatUILabelNameID = feat_param->FeatUILabelNameID;
	new_param->FeatUITooltipTextNameID = feat_param->FeatUITooltipTextNameID;
	new_param->SampleTextNameID = feat_param->SampleTextNameID;
	new_param->NumNamedParameters = feat_param->NumNamedParameters;
	new_param->FirstParamUILabelNameID = feat_param->FirstParamUILabelNameID;
    
    dnaINIT(g->dnaCtx, new_param->charValues, 20,20);
    i = 0;
    while (i < feat_param->charValues.cnt)
    {
        *dnaNEXT(new_param->charValues) = feat_param->charValues.array[i++];
    }
    
	sub->tbl = new_param;
	h->offset.subtable += param_offset;
}



/* -------------------------- Single Substitution -------------------------- */


typedef struct {
	unsigned short SubstFormat;     /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	short DeltaGlyphID;
} SingleSubstFormat1;
#define SINGLE1_SIZE    (uint16 * 3)

typedef struct {
	unsigned short SubstFormat;     /* =2 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short GlyphCount;
	GID *Substitute;                /* [GlyphCount] */
} SingleSubstFormat2;
#define SINGLE2_SIZE(nGlyphs)   (uint16 * 3 + uint16 * (nGlyphs))

/* Compare single substitution rules by target GID. Recycled duplicates sink
   to the bottom. */

static int CDECL cmpSingle(const void *first, const void *second) {
	GNode *a = ((SubstRule *)first)->targ;
	GNode *b = ((SubstRule *)second)->targ;

	if (a == NULL && b == NULL) {
		return 0;
	}
	else if (a == NULL) {
		return 1;
	}
	else if (b == NULL) {
		return -1;
	}
	else if (a->gid < b->gid) {
		return -1;
	}
	else if (a->gid > b->gid) {
		return 1;
	}
	return 0;
}

/* Check for duplicate pairs and target glyphs */

static void checkAndSortSingle(hotCtx g, GSUBCtx h) {
	long i;
	int nDuplicates = 0;

	/* Sort into increasing target glyph id order */
	qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule), cmpSingle);

	for (i = 1; i < h->new.rules.cnt; i++) {
		SubstRule *curr = &h->new.rules.array[i];
		SubstRule *prev = curr - 1;

		if (curr->targ->gid == prev->targ->gid) {
			if (curr->repl->gid == prev->repl->gid) {
				featGlyphDump(g, curr->targ->gid, ',', 0);
				*dnaNEXT(g->note) = ' ';
				featGlyphDump(g, curr->repl->gid, '\0', 0);
				hotMsg(g, hotNOTE, "Removing duplicate single substitution "
				       "in '%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
				       g->note.array);

				/* Set prev duplicates to NULL */
				featRecycleNodes(g, prev->targ);
				featRecycleNodes(g, prev->repl);
				prev->targ = NULL;
				prev->repl = NULL;
				nDuplicates++;
			}
			else {
				featGlyphDump(g, curr->targ->gid, '\0', 0);
				hotMsg(g, hotFATAL, "Duplicate target glyph for single "
				       "substitution in '%c%c%c%c' feature: %s",
				       TAG_ARG(h->new.feature), g->note.array);
			}
		}
	}

	if (nDuplicates > 0) {
		/* Duplicates sink to the bottom */
		qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule),
		      cmpSingle);
		h->new.rules.cnt -= nDuplicates;
	}
}

/* Fill single substitution format coverage table */

static Offset fillSingleCoverage(hotCtx g, GSUBCtx h, otlTbl otl) {
	int i;
	otlCoverageBegin(g, otl);
	for (i = 0; i < h->new.rules.cnt; i++) {
		otlCoverageAddGlyph(g, otl, h->new.rules.array[i].targ->gid);
	}
	return otlCoverageEnd(g, otl);
}

/* Fill format 1 single substitution subtable */

static void fillSingle1(hotCtx g, GSUBCtx h, int delta) {
	LOffset size = SINGLE1_SIZE;
	Subtable *sub = h->new.sub; /* startNewSubtable() called already. */
	otlTbl otl = sub->extension.use ? sub->extension.otl : h->otl;
	SingleSubstFormat1 *fmt = MEM_NEW(g, sizeof(SingleSubstFormat1));

	fmt->SubstFormat = 1;
	fmt->Coverage = fillSingleCoverage(g, h, otl); /* Adjusted later */
	fmt->DeltaGlyphID = delta;

	if (sub->extension.use) {
		fmt->Coverage += size;          /* Final value */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
}

/* Fill format 2 single substitution subtable */

static void fillSingle2(hotCtx g, GSUBCtx h) {
	int i;
	LOffset size;
	Subtable *sub = h->new.sub; /* startNewSubtable() called already. */
	otlTbl otl = sub->extension.use ? sub->extension.otl : h->otl;
	SingleSubstFormat2 *fmt = MEM_NEW(g, sizeof(SingleSubstFormat2));

	fmt->SubstFormat = 2;
	fmt->Coverage = fillSingleCoverage(g, h, otl); /* Adjusted later */
	fmt->GlyphCount = (unsigned short)h->new.rules.cnt;
	fmt->Substitute = MEM_NEW(g, sizeof(GID) * fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) {
		fmt->Substitute[i] = h->new.rules.array[i].repl->gid;
	}

	size = SINGLE2_SIZE(fmt->GlyphCount);
	if (sub->extension.use) {
		fmt->Coverage += size;          /* Final value */
		h->offset.extension += size + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += size;
	}

	sub->tbl = fmt;
}

/* Fill single substitution subtable */

static void fillSingle(hotCtx g, GSUBCtx h) {
	int i;
	long delta;
	SubstRule *rule;

	h->maxContext = MAX(h->maxContext, 1);

	startNewSubtable(g);

	checkAndSortSingle(g, h);


	if (h->new.feature == vrt2_) {
		hotGlyphInfo *glyphs;
		if (!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) {
			g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;
		}

		/* Set repl's vAdv from targ's hAdv */
		glyphs = g->font.glyphs.array;
		for (i = 0; i < h->new.rules.cnt; i++) {
			hotGlyphInfo *hotgi;
			rule = &h->new.rules.array[i];
			hotgi = &glyphs[rule->repl->gid];
			if (hotgi->vAdv == SHRT_MAX) {
				/* don't set it if it has already been set, as with vmtx overrides */
				hotgi->vAdv = -glyphs[rule->targ->gid].hAdv;
			}
		}
	}

	/* Determine format and fill subtable */
	rule = &h->new.rules.array[0];
	delta = rule->repl->gid - rule->targ->gid;
	for (i = 1; i < h->new.rules.cnt; i++) {
		rule = &h->new.rules.array[i];

		if (rule->repl->gid - rule->targ->gid != delta) {
			fillSingle2(g, h);
			return;
		}
	}

	fillSingle1(g, h, delta);
}

/* Write single substitution format 1 table */

static void writeSingle1(hotCtx g, GSUBCtx h, Subtable *sub) {
	SingleSubstFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "single");

	OUT2(fmt->SubstFormat);
	OUT2((Offset)(fmt->Coverage));
	OUT2(fmt->DeltaGlyphID);

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write single substitution format 2 table */

static void writeSingle2(hotCtx g, GSUBCtx h, Subtable *sub) {
	int i;
	SingleSubstFormat2 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "single");

	OUT2(fmt->SubstFormat);
	OUT2((Offset)(fmt->Coverage));
	OUT2(fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) {
		OUT2(fmt->Substitute[i]);
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write single substitution format table */

static void writeSingle(hotCtx g, GSUBCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			writeSingle1(g, h, sub);
			break;

		case 2:
			writeSingle2(g, h, sub);
			break;
	}
}

/* Free single substitution format 1 table */

static void freeSingle1(hotCtx g, GSUBCtx h, Subtable *sub) {
	SingleSubstFormat1 *fmt = sub->tbl;

	MEM_FREE(g, fmt);
}

/* Free single substitution format 2 table */

static void freeSingle2(hotCtx g, GSUBCtx h, Subtable *sub) {
	SingleSubstFormat2 *fmt = sub->tbl;

	MEM_FREE(g, fmt->Substitute);
	MEM_FREE(g, fmt);
}

/* Free single substitution format table */

static void freeSingle(hotCtx g, GSUBCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			freeSingle1(g, h, sub);
			break;

		case 2:
			freeSingle2(g, h, sub);
			break;
	}
}

#if HOT_FEAT_SUPPORT

/* ------------------------- Multiple Substitution ------------------------- */


typedef struct {
	unsigned short GlyphCount;
	GID *Substitute;                        /* [GlyphCount] */
} Sequence;
#define SEQUENCE_SIZE(nGlyphs)  (uint16 + uint16 * (nGlyphs))

typedef struct {
	unsigned short SubstFormat;             /* =1 */
	LOffset Coverage;                       /* 32-bit for overflow check */
	unsigned short SequenceCount;
	DCL_OFFSET_ARRAY(Sequence, Sequence);   /* [SequenceCount] */
} MultipleSubstFormat1;

#define MULTIPLE1_HDR_SIZE(nSequences)  (uint16 * 3 + uint16 * (nSequences))

#define MULTIPLE_TOTAL_SIZE(nSequences, nSubs) \
	(MULTIPLE1_HDR_SIZE(nSequences) + uint16 * (nSequences) + uint16 * (nSubs))



static int CDECL cmpMultiple(const void *first, const void *second) {
	GID a = ((SubstRule *)first)->targ->gid;
	GID b = ((SubstRule *)second)->targ->gid;

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

/* Create a subtable with rules from index [beg] to [end]. size: total size
   (excluding actual coverage). numAlts: total number of replacement glyphs. */

static void fillMultiple1(hotCtx g, GSUBCtx h, long beg, long end, long size,
                          unsigned int nSubs) {
	otlTbl otl;
	Subtable *sub;
	int isRTL;
	unsigned nSequences;
	GID *pSubs;
	long i;
	Offset offset;
	MultipleSubstFormat1 *fmt = MEM_NEW(g, sizeof(MultipleSubstFormat1));
#define AALT_STATS 1    /* Print aalt statistics with debug output */

#if HOT_DEBUG
	if (beg != 0 || end != h->new.rules.cnt - 1) {
		DF(1, (stderr, "fillMultiple1() from %ld->%ld; totNumRules=%ld\n", beg, end,
		       h->new.rules.cnt));
	}
#endif /* HOT_DEBUG */

	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;
	isRTL = sub->lkpFlag & otlRightToLeft;

	fmt->SubstFormat = 1;
	fmt->SequenceCount = nSequences = end - beg + 1;

	fmt->Sequence = MEM_NEW(g, sizeof(LOffset) * nSequences);
	fmt->Sequence_ = MEM_NEW(g, sizeof(Sequence) * nSequences);
	pSubs = MEM_NEW(g, sizeof(GID) * nSubs);

	offset = MULTIPLE1_HDR_SIZE(nSequences);
	otlCoverageBegin(g, otl);
	for (i = 0; i < (long)nSequences; i++) {
		GNode *node;
		SubstRule *rule = &h->new.rules.array[i + beg];
		Sequence *seqSet = &fmt->Sequence_[i];

		otlCoverageAddGlyph(g, otl, rule->targ->gid);

		/* --- Fill an Sequence --- */
		seqSet->Substitute = pSubs;
		for (node = rule->repl; node != NULL; node = node->nextSeq) {
			*pSubs++ = node->gid;
		}
		seqSet->GlyphCount = pSubs - seqSet->Substitute;

		fmt->Sequence[i] = (Offset)offset;
		offset += SEQUENCE_SIZE(seqSet->GlyphCount);
	}

#if HOT_DEBUG
	if (offset != size) {
		hotMsg(g, hotFATAL, "[internal] fillSubstitute() size miscalculation");
	}
#if AALT_STATS
	if (h->new.feature == aalt_) {
		DF(1, (stderr, "# aalt lkptype 3 subtbl: average %.2f repl gids "
		       "per rule for %ld rules. subtbl size: %hx\n",
		       (double)nSubs / i, i, offset));
	}
#endif
#endif /* HOT_DEBUG */

	fmt->Coverage = otlCoverageEnd(g, otl); /* Adjusted later */
	if (sub->extension.use) {
		fmt->Coverage += offset;            /* Final value */
		h->offset.extension += offset + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += offset;
	}

	h->new.sub->tbl = fmt;
	h->maxContext = MAX(h->maxContext, 1);
}

#if HOT_DEBUG
/* Dump accumulated aalt rules */
static void aaltDump(hotCtx g, GSUBCtx h) {
	if (h->new.feature == aalt_) {
		long i;
		fprintf(stderr, "--- aalt GSUBAlternate --- %ld rules\n",
		        h->new.rules.cnt);
		for (i = 0; i < h->new.rules.cnt; i++) {
			SubstRule *rule = &h->new.rules.array[i];

			fprintf(stderr, "sub ");
			featGlyphDump(g, rule->targ->gid, -1, 1);
			fprintf(stderr, " from ");
			featPatternDump(g, rule->repl, '\n', 1);
		}
	}
}

#endif

/* Fill the currently accumulated alternate substitution subtable, auto-
   breaking into several subtables if needed. */

static void fillMultiple(hotCtx g, GSUBCtx h) {
	long i = 0;
	long j;
	long size = 0;
	unsigned nSubs = 0; /* Accumulated number of alternates */

	/* Sort by target glyph */
	qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule),
	      cmpMultiple);
#if 0
	aaltDump(g, h);
#endif
	for (j = 0; j < h->new.rules.cnt; j++) {
		GNode *node;
		unsigned nSubsNew;
		long sizeNew;
		SubstRule *rule = &h->new.rules.array[j];

		/* Check for duplicates */
		if (j != 0 && rule->targ->gid == (rule - 1)->targ->gid) {
			featGlyphDump(g, rule->targ->gid, '\0', 0);
			hotMsg(g, hotFATAL,
			       "Duplicate target glyph for alternate substitution in "
			       "'%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
			       g->note.array);
		}

		/* Calculate new size if this rule were included: */
		nSubsNew = nSubs;
		for (node = h->new.rules.array[j].repl; node != NULL;
		     node = node->nextSeq) {
			nSubsNew++;
		}
		sizeNew = MULTIPLE_TOTAL_SIZE(j - i + 1, nSubsNew);

		if (sizeNew > 0xFFFF) {
			/* Just overflowed size; back up one rule */
			fillMultiple1(g, h, i, j - 1, size, nSubs);

			/* Initialize for next subtable */
			size = nSubs = 0;
			i = j--;
		}
		else if (j == h->new.rules.cnt - 1) {
			/* At end of array */
			fillMultiple1(g, h, i, j, sizeNew, nSubsNew);
		}
		else {
			/* Not ready for a subtable break yet */
			size = sizeNew;
			nSubs = nSubsNew;
		}
	}
}

static void writeMultiple(hotCtx g, GSUBCtx h, Subtable *sub) {
	long i;
	MultipleSubstFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "multiple");

	OUT2(fmt->SubstFormat);
	OUT2((Offset)(fmt->Coverage));
	OUT2(fmt->SequenceCount);

	for (i = 0; i < fmt->SequenceCount; i++) {
		OUT2(fmt->Sequence[i]);
	}
	for (i = 0; i < fmt->SequenceCount; i++) {
		long j;
		Sequence *seqSet = &fmt->Sequence_[i];

		OUT2(seqSet->GlyphCount);
		for (j = 0; j < seqSet->GlyphCount; j++) {
			OUT2(seqSet->Substitute[j]);
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeMultiple(hotCtx g, GSUBCtx h, Subtable *sub) {
	MultipleSubstFormat1 *fmt = sub->tbl;

	MEM_FREE(g, fmt->Sequence_[0].Substitute);
	MEM_FREE(g, fmt->Sequence_);
	MEM_FREE(g, fmt->Sequence);
	MEM_FREE(g, fmt);
}

/* ------------------------- Alternate Substitution ------------------------ */


typedef struct {
	unsigned short GlyphCount;
	GID *Alternate;                                 /* [GlyphCount] */
} AlternateSet;
#define ALTERNATE_SET_SIZE(nGlyphs) (uint16 + uint16 * (nGlyphs))

typedef struct {
	unsigned short SubstFormat;                     /* =1 */
	LOffset Coverage;                           /* 32-bit for overflow check */
	unsigned short AlternateSetCount;
	DCL_OFFSET_ARRAY(AlternateSet, AlternateSet);   /* [AlternateSetCount] */
} AlternateSubstFormat1;
#define ALTERNATE1_HDR_SIZE(nAltSets)   (uint16 * 3 + uint16 * (nAltSets))

#define ALTERNATE_TOTAL_SIZE(nAltSets, nAlts) \
	(ALTERNATE1_HDR_SIZE(nAltSets) + uint16 * (nAltSets) + uint16 * (nAlts))


static int CDECL cmpAlternate(const void *first, const void *second) {
	GID a = ((SubstRule *)first)->targ->gid;
	GID b = ((SubstRule *)second)->targ->gid;

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

/* Create a subtable with rules from index [beg] to [end]. size: total size
   (excluding actual coverage). numAlts: total number of replacement glyphs. */

static void fillAlternate1(hotCtx g, GSUBCtx h, long beg, long end, long size,
                           unsigned numAlts) {
	otlTbl otl;
	Subtable *sub;
	unsigned nAltSets;
	GID *pAlts;
	long i;
	Offset offset;
	AlternateSubstFormat1 *fmt = MEM_NEW(g, sizeof(AlternateSubstFormat1));
#define AALT_STATS 1    /* Print aalt statistics with debug output */

#if HOT_DEBUG
	if (beg != 0 || end != h->new.rules.cnt - 1) {
		DF(1, (stderr, "fillAlt() from %ld->%ld; totNumRules=%ld\n", beg, end,
		       h->new.rules.cnt));
	}
#endif /* HOT_DEBUG */

	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	fmt->SubstFormat = 1;
	fmt->AlternateSetCount = nAltSets = end - beg + 1;

	fmt->AlternateSet = MEM_NEW(g, sizeof(Offset) * nAltSets);
	fmt->AlternateSet_ = MEM_NEW(g, sizeof(AlternateSet) * nAltSets);
	pAlts = MEM_NEW(g, sizeof(GID) * numAlts);

	offset = ALTERNATE1_HDR_SIZE(nAltSets);
	otlCoverageBegin(g, otl);
	for (i = 0; i < (long)nAltSets; i++) {
		GNode *node;
		SubstRule *rule = &h->new.rules.array[i + beg];
		AlternateSet *altSet = &fmt->AlternateSet_[i];

		otlCoverageAddGlyph(g, otl, rule->targ->gid);

		/* --- Fill an AlternateSet --- */
		altSet->Alternate = pAlts;
		for (node = rule->repl; node != NULL; node = node->nextCl) {
			*pAlts++ = node->gid;
		}
		altSet->GlyphCount = pAlts - altSet->Alternate;

		fmt->AlternateSet[i] = (Offset)offset;
		offset += ALTERNATE_SET_SIZE(altSet->GlyphCount);
	}

#if HOT_DEBUG
	if (offset != size) {
		hotMsg(g, hotFATAL, "[internal] fillAlternate() size miscalculation");
	}
#if AALT_STATS
	if (h->new.feature == aalt_) {
		DF(1, (stderr, "# aalt lkptype 3 subtbl: average %.2f repl gids "
		       "per rule for %ld rules. subtbl size: %hx\n",
		       (double)numAlts / i, i, offset));
	}
#endif
#endif /* HOT_DEBUG */

	fmt->Coverage = otlCoverageEnd(g, otl); /* Adjusted later */
	if (sub->extension.use) {
		fmt->Coverage += offset;            /* Final value */
		h->offset.extension += offset + otlGetCoverageSize(otl);
		/* h->offset.subtable already incr in fillExtension() */
	}
	else {
		h->offset.subtable += offset;
	}

	h->new.sub->tbl = fmt;
	h->maxContext = MAX(h->maxContext, 1);
}

/* Fill the currently accumulated alternate substitution subtable, auto-
   breaking into several subtables if needed. */

static void fillAlternate(hotCtx g, GSUBCtx h) {
	long i = 0;
	long j;
	long size = 0;
	unsigned numAlts = 0;   /* Accumulated number of alternates */

	/* Sort by target glyph */
	qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule),
	      cmpAlternate);
#if 0
	aaltDump(g, h);
#endif
	for (j = 0; j < h->new.rules.cnt; j++) {
		GNode *node;
		unsigned numAltsNew;
		long sizeNew;
		SubstRule *rule = &h->new.rules.array[j];

		/* Check for duplicates */
		if (j != 0 && rule->targ->gid == (rule - 1)->targ->gid) {
			featGlyphDump(g, rule->targ->gid, '\0', 0);
			hotMsg(g, hotFATAL,
			       "Duplicate target glyph for alternate substitution in "
			       "'%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
			       g->note.array);
		}

		/* Calculate new size if this rule were included: */
		numAltsNew = numAlts;
		for (node = h->new.rules.array[j].repl; node != NULL;
		     node = node->nextCl) {
			numAltsNew++;
		}
		sizeNew = ALTERNATE_TOTAL_SIZE(j - i + 1, numAltsNew);

		if (sizeNew > 0xFFFF) {
			/* Just overflowed size; back up one rule */
			fillAlternate1(g, h, i, j - 1, size, numAlts);

			/* Initialize for next subtable */
			size = numAlts = 0;
			i = j--;
		}
		else if (j == h->new.rules.cnt - 1) {
			/* At end of array */
			fillAlternate1(g, h, i, j, sizeNew, numAltsNew);
		}
		else {
			/* Not ready for a subtable break yet */
			size = sizeNew;
			numAlts = numAltsNew;
		}
	}
}

static void writeAlternate(hotCtx g, GSUBCtx h, Subtable *sub) {
	long i;
	AlternateSubstFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "alternate");

	OUT2(fmt->SubstFormat);
	OUT2((Offset)(fmt->Coverage));
	OUT2(fmt->AlternateSetCount);

	for (i = 0; i < fmt->AlternateSetCount; i++) {
		OUT2(fmt->AlternateSet[i]);
	}
	for (i = 0; i < fmt->AlternateSetCount; i++) {
		long j;
		AlternateSet *altSet = &fmt->AlternateSet_[i];

		OUT2(altSet->GlyphCount);
		for (j = 0; j < altSet->GlyphCount; j++) {
			OUT2(altSet->Alternate[j]);
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeAlternate(hotCtx g, GSUBCtx h, Subtable *sub) {
	AlternateSubstFormat1 *fmt = sub->tbl;

	MEM_FREE(g, fmt->AlternateSet_[0].Alternate);
	MEM_FREE(g, fmt->AlternateSet_);
	MEM_FREE(g, fmt->AlternateSet);
	MEM_FREE(g, fmt);
}

/* ------------------------- Ligature Substitution ------------------------- */


typedef struct {
	GID LigGlyph;
	unsigned short CompCount;
	GID *Component;                                 /* [CompCount - 1] */
} Ligature;
#define LIGATURE_SIZE(nComponents)  (uint16 * 2 + uint16 * ((nComponents) - 1))

typedef struct {
	unsigned short LigatureCount;
	DCL_OFFSET_ARRAY(Ligature, Ligature);           /* [LigatureCount] */
} LigatureSet;
#define LIGATURE_SET_SIZE(nLigatures)   (uint16 + uint16 * (nLigatures))

typedef struct {
	unsigned short SubstFormat;                     /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short LigSetCount;
	DCL_OFFSET_ARRAY(LigatureSet, LigatureSet);     /* [LigSetCount] */
} LigatureSubstFormat1;
#define LIGATURE1_HDR_SIZE(nLigSets)    (uint16 * 3 + uint16 * (nLigSets))

/* Caution: assumes lengths are the same */

static int cmpSeqGIDs(GNode *a, GNode *b) {
	for (; a != NULL; a = a->nextSeq, b = b->nextSeq) {
		if (a->gid < b->gid) {
			return -1;
		}
		else if (a->gid > b->gid) {
			return 1;
		}
	}
	return 0;
}

/* Sort by targ's first gid, targ's length (decr), then all of targ's GIDs.
   Deleted duplicates (targ == NULL) sink to the bottom */

static int CDECL cmpLigature(const void *first, const void *second) {
	SubstRule *a = (SubstRule *)first;
	SubstRule *b = (SubstRule *)second;

	if (a->targ != NULL && b->targ == NULL) {
		return -1;
	}
	else if (a->targ == NULL && b->targ != NULL) {
		return 1;
	}
	else if (a->targ == NULL && b->targ == NULL) {
		return 0;
	}
	else if (a->targ->gid < b->targ->gid) {
		return -1;
	}
	else if (a->targ->gid > b->targ->gid) {
		return 1;
	}
	else if (a->data > b->data) {
		return -1;
	}
	else if (a->data < b->data) {
		return 1;
	}
	else {
		return cmpSeqGIDs(a->targ, b->targ);
	}
}

/* Check for duplicate ligatures; sort */

static void checkAndSortLigature(hotCtx g, GSUBCtx h) {
	long i;
	int nDuplicates = 0;

	qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule),
	      cmpLigature);

	for (i = 1; i < h->new.rules.cnt; i++) {
		SubstRule *curr = &h->new.rules.array[i];
		SubstRule *prev = curr - 1;

		if (cmpLigature(curr, prev) == 0) {
			if (curr->repl->gid == prev->repl->gid) {
				featPatternDump(g, curr->targ, ',', 0);
				*dnaNEXT(g->note) = ' ';
				featGlyphDump(g, curr->repl->gid, '\0', 0);
				hotMsg(g, hotNOTE, "Removing duplicate ligature substitution"
				       " in '%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
				       g->note.array);

				/* Set prev duplicates to NULL */
				featRecycleNodes(g, prev->targ);
				featRecycleNodes(g, prev->repl);
				prev->targ = NULL;
				prev->repl = NULL;
				nDuplicates++;
			}
			else {
				featPatternDump(g, curr->targ, '\0', 0);
				hotMsg(g, hotFATAL, "Duplicate target sequence but different "
				       "replacement glyphs in ligature substitutions in "
				       "'%c%c%c%c' feature: %s", TAG_ARG(h->new.feature),
				       g->note.array);
			}
		}
	}

	if (nDuplicates > 0) {
		/* Duplicates sink to the bottom */
		qsort(h->new.rules.array, h->new.rules.cnt, sizeof(SubstRule),
		      cmpLigature);
		h->new.rules.cnt -= nDuplicates;
	}
}

static unsigned totNumComp(GSUBCtx h) {
	long i;
	GNode *node;
	unsigned cnt = 0;

	for (i = 0; i < h->new.rules.cnt; i++) {
		for (node = h->new.rules.array[i].targ; node != NULL;
		     node = node->nextSeq) {
			cnt++;
		}
	}
	return cnt;
}

/* Fill ligature substitution subtable */

static void fillLigature(hotCtx g, GSUBCtx h) {
	otlTbl otl;
	Subtable *sub;
	long i;
	int j;
	LOffset offset;
	int iLigSet;
	GID *pComp = NULL;  /* Suppress optimizer warning */
	unsigned nLigSets = 0;
	LigatureSubstFormat1 *fmt = MEM_NEW(g, sizeof(LigatureSubstFormat1));

	startNewSubtable(g);
	sub = h->new.sub;
	otl = sub->extension.use ? sub->extension.otl : h->otl;

	checkAndSortLigature(g, h);

#if 0
	fprintf(stderr, "--- Final sorted lig list (%ld els): \n",
	        h->new.rules.cnt);
	rulesDump(h);
#endif

	otlCoverageBegin(g, otl);
	for (i = 0; i < h->new.rules.cnt; i++) {
		SubstRule *rule = &h->new.rules.array[i];

		if (i == 0 || rule->targ->gid != (rule - 1)->targ->gid) {
			nLigSets++;
			otlCoverageAddGlyph(g, otl, rule->targ->gid);
		}
	}
	fmt->SubstFormat = 1;
	fmt->LigSetCount = nLigSets;
	offset = LIGATURE1_HDR_SIZE(nLigSets);

	fmt->LigatureSet = MEM_NEW(g, sizeof(Offset) * nLigSets);
	fmt->LigatureSet_ = MEM_NEW(g, sizeof(LigatureSet) * nLigSets);

	iLigSet = 0;
	j = 0;
	for (i = 1; i <= h->new.rules.cnt; i++) {
		if (i == h->new.rules.cnt || h->new.rules.array[i].targ->gid !=
		    (h->new.rules.array[i - 1]).targ->gid) {
			/* --- Fill a LigatureSet --- */
			int k;
			long iLast;
			LOffset offLig;
			LigatureSet *ligSet = &fmt->LigatureSet_[j];

			ligSet->LigatureCount = (unsigned short)i - iLigSet;

			/* Initialize arrays */
			if (j == 0) {
				ligSet->Ligature =
				    MEM_NEW(g, sizeof(Offset) * h->new.rules.cnt);
				ligSet->Ligature_ =
				    MEM_NEW(g, sizeof(Ligature) * h->new.rules.cnt);
			}
			else {
				LigatureSet *firstSet = &fmt->LigatureSet_[0];
				ligSet->Ligature = &firstSet->Ligature[iLigSet];
				ligSet->Ligature_ = &firstSet->Ligature_[iLigSet];
			}

			offLig = LIGATURE_SET_SIZE(ligSet->LigatureCount);
			iLast = iLigSet + ligSet->LigatureCount - 1;

			for (k = iLigSet; k <= iLast; k++) {
				/* --- Fill a Ligature --- */
				GNode *node;
				SubstRule *ligRule = &h->new.rules.array[k];
				Ligature *lig = &ligSet->Ligature_[k - iLigSet];
				int nComp = 1;

				lig->LigGlyph = ligRule->repl->gid;

				if (j == 0 && k == 0) {
					lig->Component = pComp =
					        MEM_NEW(g, (totNumComp(h) - h->new.rules.cnt) *
					                sizeof(GID));
				}
				else {
					lig->Component = pComp;
				}

				for (node = ligRule->targ->nextSeq; node != NULL;
				     node = node->nextSeq) {
					nComp++;
					*pComp++ = node->gid;
				}

				lig->CompCount = nComp;

				ligSet->Ligature[k - iLigSet] = (Offset)offLig;
				offLig += LIGATURE_SIZE(nComp);

				h->maxContext = MAX(h->maxContext, nComp);
			}

			fmt->LigatureSet[j++] = (Offset)offset;
			offset += offLig;

			iLigSet = i;
		}
	}

	if (offset > 0xFFFF) {
		hotMsg(g, hotFATAL, "Ligature lookup subtable in GSUB feature "
		       "'%c%c%c%c' causes offset overflow.",
		       TAG_ARG(h->new.feature), i);
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
}

static void writeLigature(hotCtx g, GSUBCtx h, Subtable *sub) {
	long i;
	LigatureSubstFormat1 *fmt = sub->tbl;

	if (!sub->extension.use) {
		fmt->Coverage += h->offset.subtable - sub->offset; /* Adjust offset */
	}
	CHECK4OVERFLOW(fmt->Coverage, "ligature");

	OUT2(fmt->SubstFormat);
	OUT2((Offset)(fmt->Coverage));
	OUT2(fmt->LigSetCount);

	for (i = 0; i < fmt->LigSetCount; i++) {
		OUT2(fmt->LigatureSet[i]);
	}
	for (i = 0; i < fmt->LigSetCount; i++) {
		int j;
		LigatureSet *ligSet = &fmt->LigatureSet_[i];

		OUT2(ligSet->LigatureCount);

		for (j = 0; j < ligSet->LigatureCount; j++) {
			OUT2(ligSet->Ligature[j]);
		}
		for (j = 0; j < ligSet->LigatureCount; j++) {
			int k;
			Ligature *lig = &ligSet->Ligature_[j];

			OUT2(lig->LigGlyph);
			OUT2(lig->CompCount);
			for (k = 0; k < lig->CompCount - 1; k++) {
				OUT2(lig->Component[k]);
			}
		}
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

static void freeLigature(hotCtx g, GSUBCtx h, Subtable *sub) {
	LigatureSubstFormat1 *fmt = sub->tbl;
	LigatureSet *firstLigSet = &fmt->LigatureSet_[0];

	MEM_FREE(g, firstLigSet->Ligature_[0].Component);
	MEM_FREE(g, firstLigSet->Ligature_);
	MEM_FREE(g, firstLigSet->Ligature);
	MEM_FREE(g, fmt->LigatureSet_);
	MEM_FREE(g, fmt->LigatureSet);
	MEM_FREE(g, fmt);
}

#if 0
/* -------------------------- Context Substitution ------------------------- */


typedef struct {
	unsigned short GlyphCount;
	unsigned short SubstCount;
	GID *Input;                                     /* [GlyphCount - 1] */
	SubstLookupRecord *SubstLookupRecord;   /* [SubstCount] */
} SubRule;
#define SUB_RULE_SIZE(nGlyphs, nSubsts)  (uint16 * 2 + \
	                                      uint16 * (nGlyphs) + \
	                                      SUBST_LOOKUP_RECORD_SIZE * (nSubsts))
typedef struct {
	unsigned short SubRuleCount;
	DCL_OFFSET_ARRAY(SubRule, SubRule);             /* [SubRuleCount] */
} SubRuleSet;
#define SUB_RULE_SET_SIZE(nRules)   (uint16 + uint16 * (nRules))

typedef struct {
	unsigned short SubstFormat;                     /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short SubRuleSetCount;
	DCL_OFFSET_ARRAY(SubRuleSet, SubRuleSet);       /* [SubRuleSetCount] */
} ContextSubstFormat1;
#define CONTEXT1_SIZE(nRuleSets)    (uint16 * 3 + uint16 * (nRuleSets))

typedef struct {
	unsigned short GlyphCount;
	unsigned short SubstCount;
	unsigned short *Class;                          /* [GlyphCount - 1] */
	SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
} SubClassRule;
#define SUB_CLASS_RULE_SIZE(nGlyphs, nSubsts) \
	(uint16 * 2 + \
	 uint16 * (nGlyphs - 1) + \
	 SUBST_LOOKUP_RECORD_SIZE * (nSubsts))
typedef struct {
	unsigned short SubClassRuleCnt;
	DCL_OFFSET_ARRAY(SubClassRule, SubClassRule);   /* [SubClassRuleCnt] */
} SubClassSet;
#define SUB_CLASS_SET_SIZE(nSubClassRules)  (uint16 + uint16 * (nSubClassRules))

typedef struct {
	unsigned short SubstFormat;                     /* =2 */
	LOffset Coverage;               /* 32-bit for overflow check */
	LOffset ClassDef;               /* 32-bit for overflow check */
	unsigned short SubClassSetCnt;
	DCL_OFFSET_ARRAY(SubClassSet, SubClassSet);     /* [SubClassSetCnt] */
} ContextSubstFormat2;
#define CONTEXT2_HDR_SIZE(nSubClassSets)    (uint16 * 4 + uint16 * (nSubClassSets))

typedef struct {
	unsigned short SubstFormat;                     /* =3 */
	unsigned short GlyphCount;
	unsigned short SubstCount;
	Offset *Coverage;                               /* [GlyphCount] */
	SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
} ContextSubstFormat3;
#define CONTEXT3_SIZE(nGlyphs, nSubsts)  (uint16 * 3 + \
	                                      uint16 * (nGlyphs) + \
	                                      SUBST_LOOKUP_RECORD_SIZE * (nSubsts))
#endif

/* ------------------ Chaining Contextual Substitution --------------------- */


typedef struct {
	unsigned short BacktrackGlyphCount;
	GID *Backtrack;                                 /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	GID *Input;                                     /* [InputGlyphCount - 1] */
	unsigned short LookaheadGlyphCount;
	GID *Lookahead;                                 /* [LookaheadGlyphCount] */
	unsigned short SubstCount;
	SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
} ChainSubRule;
#define CHAIN_SUB_RULE_SIZE(nBack, nInput, nLook, nSubst)  (uint16 * 4 + \
	                                                        uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                                        SUBST_LOOKUP_RECORD_SIZE * (nSubst))
typedef struct {
	unsigned short ChainSubRuleCount;
	DCL_OFFSET_ARRAY(ChainSubRule, ChainSubRule);   /* [ChainSubRuleCount] */
} ChainSubRuleSet;
#define CHAIN_SUB_RULE_SET_SIZE(nRules) (uint16 + uint16 * (nRules))

typedef struct {
	unsigned short SubstFormat;                     /* =1 */
	LOffset Coverage;               /* 32-bit for overflow check */
	unsigned short ChainSubRuleSetCount;
	DCL_OFFSET_ARRAY(ChainSubRuleSet, ChainSubRuleSet);
	/* [ChainSubRuleSetCount]*/
} ChainContextSubstFormat1;
#define CHAIN1_HDR_SIZE(nRuleSets)  (uint16 * 3 + uint16 * (nRuleSets))


typedef struct {
	unsigned short BacktrackGlyphCount;
	unsigned short *Backtrack;                      /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	unsigned short *Input;                          /* [InputGlyphCount - 1] */
	unsigned short LookaheadGlyphCount;
	unsigned short *Lookahead;                      /* [LookaheadGlyphCount] */
	unsigned short SubstCount;
	SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
} ChainSubClassRule;
#define CHAIN_SUB_CLASS_RULE_SIZE(nBack, nInput, nLook, nSubst)  (uint16 * 4 + \
	                                                              uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                                              SUBST_LOOKUP_RECORD_SIZE * (nSubst))
typedef struct {
	unsigned short ChainSubClassRuleCnt;
	DCL_OFFSET_ARRAY(ChainSubClassRule, ChainSubClassRule);
	/* [ChainSubClassRuleCnt]*/
} ChainSubClassSet;
#define CHAIN_SUB_CLASS_SET_SIZE(nRules)    (uint16 + uint16 * (nRules))

typedef struct {
	unsigned short SubstFormat;                     /* =2 */
	LOffset Coverage;               /* 32-bit for overflow check */
	LOffset ClassDef;               /* 32-bit for overflow check */
	unsigned short ChainSubClassSetCnt;
	DCL_OFFSET_ARRAY(ChainSubClassSet, ChainSubClassSet);
	/* [ChainSubClassSetCnt] */
} ChainContextSubstFormat2;
#define CHAIN2_HDR_SIZE(nSets)  (uint16 * 4 + uint16 * (nSets))


typedef struct {
	unsigned short SubstFormat;                     /* =3 */
	unsigned short BacktrackGlyphCount;
	LOffset *Backtrack;                             /* [BacktrackGlyphCount] */
	unsigned short InputGlyphCount;
	LOffset *Input;                                 /* [InputGlyphCount] */
	unsigned short LookaheadGlyphCount;
	LOffset *Lookahead;                             /* [LookaheadGlyphCount] */
	unsigned short SubstCount;
	SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
} ChainContextSubstFormat3;
#define CHAIN3_SIZE(nBack, nInput, nLook, nSubst)  (uint16 * 5 + \
	                                                uint16 * (nBack) + uint16 * (nInput) + uint16 * (nLook) + \
	                                                SUBST_LOOKUP_RECORD_SIZE * (nSubst))

typedef struct {
	unsigned short SubstFormat;                     /* =1 */
	LOffset InputCoverage;                                  /* a single coverage table, for a singel glyph sub */
	unsigned short BacktrackGlyphCount;
	LOffset *Backtrack;                             /* [BacktrackGlyphCount] offsets to coverage tables */
	unsigned short LookaheadGlyphCount;
	LOffset *Lookahead;                             /* [LookaheadGlyphCount] offsets to coverage tables */
	unsigned short GlyphCount;
	GID *Substitute;            /* [GlyphCount] */
} ReverseChainContextSubstFormat1;

#define REVERSECHAIN1_SIZE(nBack, nLook, nSubst)  (uint16 * 5 + \
	                                               uint16 * (nBack) + uint16 * (nLook) + \
	                                               uint16 * (nSubst))


static void recycleProd(GSUBCtx h) {
	long i;
	for (i = 0; i < h->prod.cnt; i++) {
		featRecycleNodes(h->g, h->prod.array[i]);
	}
}

/* Tries to add rule to current anon subtbl. If successful, returns 1, else 0.
   If rule already exists in subtbl, recycles targ and repl */

static int addToAnonSubtbl(hotCtx g, GSUBCtx h, SubtableInfo *si, GNode *targ,
                           GNode *repl) {
	if (si->lkpType == GSUBSingle) {
		long i;
		GNode *t = targ;
		GNode *r = repl;
		int nTarg = 0;
		int nFound = 0;

		/* Determine which rules already exist in the subtable */
		for (; t != NULL && r != NULL; t = t->nextCl, r = r->nextCl) {
			nTarg++;
			t->flags &= ~FEAT_MISC;         /* Clear "found" flag */
			for (i = 0; i < si->rules.cnt; i++) {
				SubstRule *rule = &si->rules.array[i];
				if (t->gid == rule->targ->gid) {
					if (r->gid != rule->repl->gid) {
						return 0;   /* Same targ, diff repl not allowed */
					}
					else {
						nFound++;
						t->flags |= FEAT_MISC;
						break;
					}
				}
			}
		}

		if (nFound == 0) {
			/* No rules exist in subtbl */
			addRule(h->g, h, si, targ, repl);
		}
		else if (nTarg == nFound) {
			/* All rules already exist in subtbl! */
		}
		else {
			/* Some but not all rules need to be added */
			int nToAdd = nTarg - nFound;
			for (t = targ, r = repl; t != NULL && r != NULL;
			     t = t->nextCl, r = r->nextCl) {
				if (!(t->flags & FEAT_MISC)) {
					addRule(h->g, h, si, featSetNewNode(g, t->gid),
					        featSetNewNode(g, r->gid));
					if (--nToAdd == 0) {
						break;
					}
				}
			}
		}
		if (nFound != 0) {
			featRecycleNodes(g, targ);
			featRecycleNodes(g, repl);
		}
		return 1;
	}

	else if (si->lkpType == GSUBLigature) {
		unsigned i;
		unsigned nSeq;
		GNode ***prod = featMakeCrossProduct(g, targ, &nSeq);

		dnaSET_CNT(h->prod, (long)nSeq);
		COPY(&h->prod.array[0], *prod, nSeq);

		for (i = 0; i < nSeq; i++) {
			long j;
			GNode *t = h->prod.array[i];

			t->flags &= ~FEAT_MISC;         /* Clear "found" flag */
			for (j = 0; j < si->rules.cnt; j++) {
				GNode *pI;
				GNode *pJ;
				SubstRule *rule = &si->rules.array[j];

				if (t->gid != rule->targ->gid) {
					continue;
				}

				pI = t;
				pJ = rule->targ;

				for (; pI->nextSeq != NULL && pJ->nextSeq != NULL &&
				     pI->nextSeq->gid == pJ->nextSeq->gid;
				     pI = pI->nextSeq, pJ = pJ->nextSeq) {
				}
				/* pI and pJ now point at the last identical node */

				if (pI->nextSeq == NULL && pJ->nextSeq == NULL) {
					/* Identical targets */
					if (repl->gid == rule->repl->gid) {
						/* Identical targ and repl */
						t->flags |= FEAT_MISC;
						goto nextP;
					}
					else {
						recycleProd(h);
						return 0;
					}
				}
				else if (pI->nextSeq == NULL || pJ->nextSeq == NULL) {
					/* One is a subset of the other */
					recycleProd(h);
					return 0;
				}
			}
nextP:
			;
		}

		/* Add any rules that were not found */
		featRecycleNodes(g, targ);
		for (i = 0; i < nSeq; i++) {
			GNode *t = h->prod.array[i];
			if (!(t->flags & FEAT_MISC)) {
				addRule(h->g, h, si, t, featSetNewNode(g, repl->gid));
			}
			else {
				featRecycleNodes(g, t);
			}
		}
		featRecycleNodes(g, repl);

		return 1;
	}

	return 0;   /* Suppress compiler warning */
}

/* Add the "anonymous" rule that occurs in a substitution within a chaining
   contextual rule. Return the label of the anonymous lookup */

static Label addAnonRule(hotCtx g, GSUBCtx h, GNode *pMarked, unsigned nMarked,
                         GNode *repl, unsigned lkpFlag, unsigned short markSetIndex) {
	GNode *targCp;
	GNode *replCp;
	SubtableInfo *si;
	int lkpType = (nMarked == 1) ? GSUBSingle : GSUBLigature;

	/* Make copies in targCp, replCp */
	featPatternCopy(g, &targCp, pMarked, nMarked);
	featPatternCopy(g, &replCp, repl, -1);

	if (h->anonSubtable.cnt > 0) {
		int i = h->anonSubtable.cnt;

		si = dnaINDEX(h->anonSubtable, i - 1);
		/* Don't need to match lkpFlag */
		if (si->lkpType == lkpType && addToAnonSubtbl(g, h, si, targCp, replCp)) {
			return si->label;
		}
	}

	/* Must create new anon subtable */
	si = dnaNEXT(h->anonSubtable);
	si->script = si->language = si->feature = TAG_UNDEF;
	si->lkpType = lkpType;
	si->lkpFlag = lkpFlag;
	si->markSetIndex = markSetIndex;
	si->label = featGetNextAnonLabel(g);

	si->rules.cnt = 0;
	addRule(g, h, si, targCp, replCp);

	return si->label;
}

/* Create anonymous lookups (referred to only from within chain ctx lookups) */

static void createAnonLookups(hotCtx g, GSUBCtx h) {
	long i;

	for (i = 0; i < h->anonSubtable.cnt; i++) {
		long j;
		SubtableInfo *si = &h->anonSubtable.array[i];

		GSUBFeatureBegin(g, si->script, si->language, si->feature);
		GSUBLookupBegin(g, si->lkpType, si->lkpFlag, si->label, 0, si->markSetIndex);

		for (j = 0; j < si->rules.cnt; j++) {
			SubstRule *rule = &si->rules.array[j];
			GSUBRuleAdd(g, rule->targ, rule->repl);
		}

		GSUBLookupEnd(g,  si->feature);
		GSUBFeatureEnd(g);
	}
}

/* Change anon SubstLookupRecord labels to lookup inxs, now that they've been
   calculated by otlFill() */

static void setAnonLookupIndices(hotCtx g, GSUBCtx h) {
	int i;

	for (i = 0; i < h->subLookup.cnt; i++) {
		SubstLookupRecord *slr = h->subLookup.array[i];
		DF(2, (stderr, "slr: Label 0x%x", slr->LookupListIndex));
		slr->LookupListIndex =
		    otlLabel2LookupIndex(g, h->otl, slr->LookupListIndex);
		DF(2, (stderr, " -> LookupListIndex %u\n", slr->LookupListIndex));
	}
}

#if HOT_DEBUG

static LOffset fillChain1(hotCtx g, GSUBCtx h, long inx) {
	ChainContextSubstFormat1 *fmt =
	    MEM_NEW(g, sizeof(ChainContextSubstFormat1));

	fmt->SubstFormat = 1;
	/* xxx */

	h->new.sub->tbl = fmt;
	return 0;
}

static LOffset fillChain2(hotCtx g, GSUBCtx h, long inx) {
	ChainContextSubstFormat2 *fmt =
	    MEM_NEW(g, sizeof(ChainContextSubstFormat2));

	fmt->SubstFormat = 2;
	/* xxx */

	h->new.sub->tbl = fmt;
	return 0;
}

#endif

/* p points to an input sequence; return new array of num coverages */

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

static void fillChain3(hotCtx g, GSUBCtx h, otlTbl otl, Subtable *sub,
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

	GNode *p;
	int iSeq = 0;   /* Suppress optimizer warning */

	SubstRule *rule = &h->new.rules.array[inx];
	unsigned nSubst = rule->repl != NULL;   /* Limited support */

	ChainContextSubstFormat3 *fmt =
	    MEM_NEW(g, sizeof(ChainContextSubstFormat3));

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
					iSeq = seqCnt;
				}
				nMarked++;
				if (p->lookupLabel >= 0) {
					nSubst++;
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
	fmt->SubstFormat = 3;

	fmt->BacktrackGlyphCount = nBack;
	fmt->Backtrack = setCoverages(g, otl, pBack, nBack);
	fmt->InputGlyphCount = nInput;
	fmt->Input = setCoverages(g, otl, pInput, nInput);
	fmt->LookaheadGlyphCount = nLook;
	fmt->Lookahead = setCoverages(g, otl, pLook, nLook);

	fmt->SubstCount = nSubst;
	if (nSubst == 0) {
		fmt->SubstLookupRecord = NULL;  /* no action */
	}
	else {
		if (rule->repl != NULL) {
			/* There is only a single replacement rule, not using direct lookup references */
			SubstLookupRecord **slr;

			fmt->SubstLookupRecord = MEM_NEW(g, sizeof(SubstLookupRecord) *
			                                 nSubst);
			fmt->SubstLookupRecord->SequenceIndex = iSeq;

			/* Store anon subtable's label for now; replace with LL index later */
			fmt->SubstLookupRecord->LookupListIndex =
			    addAnonRule(g, h, pMarked, nMarked, rule->repl, h->new.lkpFlag, h->new.markSetIndex);

			/* Store ptr for easy access later on when lkpInx's are available */
			slr = dnaNEXT(h->subLookup);
			*slr = fmt->SubstLookupRecord;
		}
		else {
			GNode *nextNode = pMarked;
			SubstLookupRecord **slr;
			int nRec = 0;
			unsigned i;
			fmt->SubstLookupRecord = MEM_NEW(g, sizeof(SubstLookupRecord) *
			                                 nSubst);
			for (i = 0; i < nMarked; i++) {
				if (nextNode->lookupLabel < 0) {
					nextNode = nextNode->nextSeq;
					continue;
				}
				slr = dnaNEXT(h->subLookup);
				*slr = &(fmt->SubstLookupRecord[nRec]);
				(*slr)->SequenceIndex = i;
				(*slr)->LookupListIndex = nextNode->lookupLabel;
				nRec++;
				nextNode = nextNode->nextSeq;
			}
		}
	}

	h->maxContext = MAX(h->maxContext, nInput + nLook);

	size = CHAIN3_SIZE(nBack, nInput, nLook, nSubst);
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
}

static void fillChain(hotCtx g, GSUBCtx h) {
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
			hotMsg(g, hotFATAL, "Chain contextual lookup subtable in GSUB "
			       "feature '%c%c%c%c' causes offset overflow.",
			       TAG_ARG(h->new.feature), i);
		}
	}
}

static void writeSubstLookupRecords(GSUBCtx h, SubstLookupRecord *slr, int cnt) {
	int i;
	for (i = 0; i < cnt; i++) {
		OUT2(slr[i].SequenceIndex);
		OUT2(slr[i].LookupListIndex);
	}
}

static void writeChain1(hotCtx g, GSUBCtx h, Subtable *sub) {
#if 0
	long i;
	ChainContextSubstFormat1 *fmt = sub->tbl;

	/* Adjust coverage offsets just before writing, below */
#endif
}

static void writeChain2(hotCtx g, GSUBCtx h, Subtable *sub) {
#if 0
	long i;
	ChainSubstFormat2 *fmt = sub->tbl;

	/* Adjust coverage offsets just before writing, below */
#endif
}

static void writeChain3(hotCtx g, GSUBCtx h, Subtable *sub) {
	long i;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	ChainContextSubstFormat3 *fmt = sub->tbl;
	int isExt = sub->extension.use;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	OUT2(fmt->SubstFormat);

	OUT2(fmt->BacktrackGlyphCount);

	if (g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
		/* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
		for (i = 0; i < fmt->BacktrackGlyphCount; i++) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}
	else {
		/* do it per OpenType spec 1.5 */
		for (i = fmt->BacktrackGlyphCount - 1; i >= 0; i--) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}

	OUT2(fmt->InputGlyphCount);
	for (i = 0; i < fmt->InputGlyphCount; i++) {
		if (!sub->extension.use) {
			fmt->Input[i] += adjustment;
		}
		CHECK4OVERFLOW(fmt->Input[i], "chain contextual");
		OUT2((unsigned short)fmt->Input[i]);
	}

	OUT2(fmt->LookaheadGlyphCount);
	for (i = 0; i < fmt->LookaheadGlyphCount; i++) {
		if (!isExt) {
			fmt->Lookahead[i] += adjustment;
		}
		CHECK4OVERFLOW(fmt->Lookahead[i], "chain contextual");
		OUT2((unsigned short)fmt->Lookahead[i]);
	}

	OUT2(fmt->SubstCount);
	writeSubstLookupRecords(h, fmt->SubstLookupRecord, fmt->SubstCount);

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Write Chain substitution format table */

static void writeChain(hotCtx g, GSUBCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			writeChain1(g, h, sub);
			break;

		case 2:
			writeChain2(g, h, sub);
			break;

		case 3:
			writeChain3(g, h, sub);
			break;
	}
}

/* Free Chain substitution format 1 table */

static void freeChain1(hotCtx g, GSUBCtx h, Subtable *sub) {
	ChainContextSubstFormat1 *fmt = sub->tbl;

	/* xxx */

	MEM_FREE(g, fmt);
}

/* Free Chain substitution format 2 table */

static void freeChain2(hotCtx g, GSUBCtx h, Subtable *sub) {
	ChainContextSubstFormat2 *fmt = sub->tbl;

	/* xxx */

	MEM_FREE(g, fmt);
}

/* Free Chain substitution format 3 table */

static void freeChain3(hotCtx g, GSUBCtx h, Subtable *sub) {
	ChainContextSubstFormat3 *fmt = sub->tbl;

	if (fmt->Backtrack != NULL) {
		MEM_FREE(g, fmt->Backtrack);
	}
	if (fmt->Input != NULL) {
		MEM_FREE(g, fmt->Input);
	}
	if (fmt->Lookahead != NULL) {
		MEM_FREE(g, fmt->Lookahead);
	}
	if (fmt->SubstLookupRecord != NULL) {
		MEM_FREE(g, fmt->SubstLookupRecord);
	}
	MEM_FREE(g, fmt);
}

/* Free Chain substitution format table */

static void freeChain(hotCtx g, GSUBCtx h, Subtable *sub) {
	switch (*(unsigned short *)sub->tbl) {
		case 1:
			freeChain1(g, h, sub);
			break;

		case 2:
			freeChain2(g, h, sub);
			break;

		case 3:
			freeChain3(g, h, sub);
			break;
	}
}

#endif /* HOT_FEAT_SUPPORT */


static int CDECL cmpNode(const void *first, const void *second) {
	GID a = (*(GNode **)first)->gid;
	GID b = (*(GNode **)second)->gid;
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

void sortInputList(GSUBCtx h, GNode **list) {
	long i;
	GNode *p = *list;
	h->prod.cnt = 0;
	/* Copy over pointers */
	for (; p != NULL; p = p->nextCl) {
		*dnaNEXT(h->prod) = p;
	}

	qsort(h->prod.array, h->prod.cnt, sizeof(GNode *), cmpNode);

	/* Move pointers around */
	for (i = 0; i < h->prod.cnt - 1; i++) {
		h->prod.array[i]->nextCl = h->prod.array[i + 1];
	}
	h->prod.array[i]->nextCl = NULL;

	*list = h->prod.array[0];
}

/* Fill chaining contextual subtable format 3 */

/* Compare glyph ids */
static int CDECL cmpGlyphIds(const void *first, const void *second) {
	GID a = *(GID *)first;
	GID b = *(GID *)second;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	return 0;
}

static void fillReverseChain1(hotCtx g, GSUBCtx h, otlTbl otl, Subtable *sub,
                              long inx) {
	LOffset size;
	unsigned int i;
	unsigned nBack = 0;
	unsigned nInput = 0;
	unsigned nLook = 0;
	GNode *pBack = NULL;
	GNode *pInput = NULL;
	GNode *pLook = NULL;
	GNode *p, *r;
	unsigned subCount = 0;

	SubstRule *rule = &h->new.rules.array[inx];

	ReverseChainContextSubstFormat1 *fmt =
	    MEM_NEW(g, sizeof(ReverseChainContextSubstFormat1));

	/* Set counts of and pointers to Back, Input, Look, Marked areas */
	pBack = rule->targ;
	for (p = rule->targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_BACKTRACK) {
			nBack++;
		}
		else if (p->flags & FEAT_INPUT) {
			/* Note: we validate that there is only one Input glyph in feat.c */
			if (pInput == NULL) {
				pInput = p;
			}
			nInput++;
		}
		else if (p->flags & FEAT_LOOKAHEAD) {
			if (pLook == NULL) {
				pLook = p;
			}
			nLook++;
		}
	}

	/* Fill table */
	fmt->SubstFormat = 1;

	/* When we call otlCoverageEnd, the input coverage will be sorted in GID order.
	   The replacement glyph list must also be sorted in that order. So, I copy the replacement
	   glyph gids into the target Gnodes as the nextSeq value. We can then sort the target list,
	   and get the Substitutions values back out in that order. Since the target list is then
	   sorted in GID order, otlCoverageEnd won't change the order again.
	 */
	if (rule->repl != NULL) {
		/* Mo need for this whole block, if subCount === 0. */
		for (p = pInput, r = rule->repl; p != NULL; p = p->nextCl, r = r->nextCl) {
			/* I require in feat.c that pInput and repl lists have the same length */
			p->nextSeq = r;
			subCount++;
		}
		sortInputList(h, &pInput);
	}

	otlCoverageBegin(g, otl);
	for (p = pInput; p != NULL; p = p->nextCl) {
		otlCoverageAddGlyph(g, otl, p->gid);
	}
	fmt->InputCoverage = otlCoverageEnd(g, otl);    /* Adjusted later */

	fmt->BacktrackGlyphCount = nBack;
	fmt->Backtrack = setCoverages(g, otl, pBack, nBack);
	fmt->LookaheadGlyphCount = nLook;
	fmt->Lookahead = setCoverages(g, otl, pLook, nLook);

	fmt->GlyphCount = subCount;
	/* When parsing the feat file, I enforced that the targ and repl glyph or glyph classes
	   be the same length, except in the case of the 'ignore' statement. In the latter case, repl is NULL */
	if ((subCount == 0) || (rule->repl == NULL)) {
		fmt->Substitute = NULL; /* no action */
		fmt->GlyphCount = subCount = 0;
	}
	else {
		fmt->Substitute = MEM_NEW(g, sizeof(GID) * subCount);
		i = 0;
		for (p = pInput; p != NULL; p = p->nextCl) {
			fmt->Substitute[i++] = p->nextSeq->gid;
			p->nextSeq = NULL; /* Remove this reference to the repl node from the target node, else
			                      featRecycleNodes will add it to the free list twice, once when freeing
			                      the targ nodes, and once when freeing the repl nodes. */
		}
	}

	h->maxContext = MAX(h->maxContext, nInput + nLook);

	size = REVERSECHAIN1_SIZE(nBack, nLook, subCount);
	if (sub->extension.use) {
		long i;

		/* Set final values for coverages */
		fmt->InputCoverage += size;
		for (i = 0; i < fmt->BacktrackGlyphCount; i++) {
			fmt->Backtrack[i] += size;
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
}

static void fillReverseChain(hotCtx g, GSUBCtx h) {
	long i;

	for (i = 0; i < h->new.rules.cnt; i++) {
		Subtable *sub;
		otlTbl otl;

		startNewSubtable(g);
		sub = h->new.sub;
		otl = sub->extension.use ? sub->extension.otl : h->otl;

		fillReverseChain1(g, h, otl, sub, i);

		if (h->offset.subtable > 0xFFFF) {
			hotMsg(g, hotFATAL, "Reverse Chain contextual lookup subtable in GSUB "
			       "feature '%c%c%c%c' causes offset overflow.",
			       TAG_ARG(h->new.feature), i);
		}
	}
}

static void writeReverseChain(hotCtx g, GSUBCtx h, Subtable *sub) {
	long i;
	LOffset adjustment = 0; /* (Linux compiler complains) */
	ReverseChainContextSubstFormat1 *fmt = sub->tbl;
	int isExt = sub->extension.use;

	if (!isExt) {
		adjustment = (h->offset.subtable - sub->offset);
	}

	OUT2(fmt->SubstFormat);

	if (!isExt) {
		fmt->InputCoverage += adjustment;
	}
	OUT2((short)fmt->InputCoverage);

	OUT2(fmt->BacktrackGlyphCount);

	if (g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
		/* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
		for (i = 0; i < fmt->BacktrackGlyphCount; i++) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}
	else {
		/* do it per OpenType spec 1.5 */
		for (i = fmt->BacktrackGlyphCount - 1; i >= 0; i--) {
			if (!isExt) {
				fmt->Backtrack[i] += adjustment;
			}
			CHECK4OVERFLOW(fmt->Backtrack[i], "chain contextual");
			OUT2((unsigned short)fmt->Backtrack[i]);
		}
	}

	OUT2(fmt->LookaheadGlyphCount);
	for (i = 0; i < fmt->LookaheadGlyphCount; i++) {
		if (!isExt) {
			fmt->Lookahead[i] += adjustment;
		}
		CHECK4OVERFLOW(fmt->Lookahead[i], "chain contextual");
		OUT2((unsigned short)fmt->Lookahead[i]);
	}

	OUT2(fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) {
		OUT2((unsigned short)fmt->Substitute[i]);
	}

	if (sub->extension.use) {
		otlCoverageWrite(g, sub->extension.otl);
	}
}

/* Free Reverse Chain substitution format 1 table */

static void freeReverseChain(hotCtx g, GSUBCtx h, Subtable *sub) {
	ReverseChainContextSubstFormat1 *fmt = sub->tbl;

	if (fmt->Backtrack != NULL) {
		MEM_FREE(g, fmt->Backtrack);
	}
	if (fmt->Lookahead != NULL) {
		MEM_FREE(g, fmt->Lookahead);
	}
	if (fmt->Substitute != NULL) {
		MEM_FREE(g, fmt->Substitute);
	}
	MEM_FREE(g, fmt);
}

/* ------------------------ Extension Substitution ------------------------- */

/* Fill extension substitution subtable */

static ExtensionSubstFormat1 *fillExtension(hotCtx g, GSUBCtx h,
                                            unsigned ExtensionLookupType) {
	ExtensionSubstFormat1 *fmt = MEM_NEW(g, sizeof(ExtensionSubstFormat1));

	fmt->SubstFormat = 1;
	fmt->ExtensionLookupType = ExtensionLookupType;
	fmt->ExtensionOffset = h->offset.extension; /* Adjusted later */

	h->offset.subtable += EXTENSION1_SIZE;
	return fmt;
}

static void writeExtension(hotCtx g, GSUBCtx h, Subtable *sub) {
	ExtensionSubstFormat1 *fmt = sub->extension.tbl;

	/* Adjust offset */
	fmt->ExtensionOffset += h->offset.extensionSection - sub->offset;

	DF(1, (stderr, "  GSUB Extension: fmt=%1d, lkpType=%2d, offset=%08lx\n",
	       fmt->SubstFormat, fmt->ExtensionLookupType, fmt->ExtensionOffset));

	OUT2(fmt->SubstFormat);
	OUT2(fmt->ExtensionLookupType);
	OUT4(fmt->ExtensionOffset);
}

static void freeExtension(hotCtx g, GSUBCtx h, Subtable *sub) {
	ExtensionSubstFormat1 *fmt = sub->extension.tbl;

	otlTableReuse(g, sub->extension.otl);
	otlTableFree(g, sub->extension.otl);
	MEM_FREE(g, fmt);
}

#if HOT_DEBUG
/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, rulesDump
#if HOT_FEAT_SUPPORT
	      , fillChain1, fillChain2, aaltDump
#endif /* HOT_FEAT_SUPPORT */
	      );
}

#endif
