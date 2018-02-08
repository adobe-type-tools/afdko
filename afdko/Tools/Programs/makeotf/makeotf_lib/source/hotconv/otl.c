/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	Shared OpenType Layout support.
 */

#include "otl.h"
#include "feat.h"
#include <stdio.h>

#include <stdlib.h>

/* --------------------------- Context Definition -------------------------- */

struct otlCtx_ {
	hotCtx g;       /* Package context */
};

/* -------------------------- Coverage Definition -------------------------- */

/* --- Format --- */

typedef struct {
	unsigned short CoverageFormat;          /* =1 */
	unsigned short GlyphCount;
	GID *GlyphArray;                        /* [GlyphCount] */
} CoverageFormat1;
#define COVERAGE1_SIZE(nGlyphs) (uint16 * 2 + uint16 * (nGlyphs))

typedef struct {
	GID Start;
	GID End;
	unsigned short StartCoverageIndex;
} RangeRecord;
#define RANGE_RECORD_SIZE       (uint16 * 3)

typedef struct {
	unsigned short CoverageFormat;          /* =2 */
	unsigned short RangeCount;
	RangeRecord *RangeRecord;               /* [RangeCount] */
} CoverageFormat2;
#define COVERAGE2_SIZE(nRanges) (uint16 * 2 + RANGE_RECORD_SIZE * (nRanges))

/* --- Build --- */

typedef struct {
	dnaDCL(GID, glyph);                     /* Covered glyph list */
	Offset offset;       /* Offset to coverage |=> start of coverage section */
	void *tbl;                              /* Formatted table */
} CoverageRecord;

typedef struct {
	CoverageRecord *new;                    /* Table under construction */
	LOffset offset;                         /* Cumulative size of coverages */
	dnaDCL(CoverageRecord, tables);         /* Coverage tables */
} Coverage;

/* ---------------------------- Class Definition --------------------------- */

/* --- Format --- */

typedef struct {
	unsigned short ClassFormat;             /* =1 */
	GID StartGlyph;
	unsigned short GlyphCount;
	unsigned short *ClassValueArray;        /* [GlyphCount] */
} ClassDefFormat1;
#define CLASS1_SIZE(nGlyphs)    (uint16 * 3 + uint16 * (nGlyphs))

typedef struct {
	GID Start;
	GID End;
	unsigned short Class;
} ClassRangeRecord;
#define CLASS_RANGE_RECORD_SIZE (uint16 * 3)

typedef struct {
	unsigned short ClassFormat;             /* =2 */
	unsigned short ClassRangeCount;
	ClassRangeRecord *ClassRangeRecord;     /* [ClassRangeCount] */
} ClassDefFormat2;
#define CLASS2_SIZE(nRanges)    (uint16 * 2 + CLASS_RANGE_RECORD_SIZE * (nRanges))

/* --- Build --- */

typedef struct {
	GID glyph;
	unsigned short class;
} ClassMap;

typedef struct {
	dnaDCL(ClassMap, map);                  /* Class mappings */
	Offset offset;             /* Offset to class |=> start of class section */
	void *tbl;                              /* Formatted table */
} ClassRecord;

typedef struct {
	ClassRecord *new;                       /* Table under construction */
	LOffset offset;                         /* Cumulative size of classes */
	dnaDCL(ClassRecord, tables);            /* Class tables */
} Class;

/* --------------------------- Device Definition --------------------------- */

typedef struct {
	unsigned short StartSize;
	unsigned short EndSize;
	unsigned short DeltaFormat;
#define DeltaBits2  1
#define DeltaBits4  2
#define DeltaBits8  3
	unsigned short *DeltaValue;             /* [((EndSize-StartSize+1)*
	                                           (2^DeltaFormat)+15)/16] */
} DeviceTable;

void otlDeviceBeg(hotCtx g, otlTbl t) {
	/* Todo */
}

void otlDeviceAddEntry(hotCtx g, otlTbl t, unsigned short ppem, short delta) {
	/* Todo */
}

Offset otlDeviceEnd(hotCtx g, otlTbl t) {
	/* Todo */
	return 0;
}

void otlDeviceWrite(hotCtx g, otlTbl t) {
	/* Todo */
}

/* --------------------------------- Table --------------------------------- */

/* --- Format --- */

/* --- ScriptList --- */
typedef struct {
	Offset LookupOrder;                     /* Reserved=NULL */
	unsigned short ReqFeatureIndex;
	unsigned short FeatureCount;
	unsigned short *FeatureIndex;           /* [FeatureCount] */
} LangSys;
#define LANG_SYS_SIZE(nFeatures)    (uint16 * 3 + uint16 * (nFeatures))

typedef struct {
	Tag LangSysTag;
	DCL_OFFSET(LangSys, LangSys);           /* |-> Script */
} LangSysRecord;
#define LANG_SYS_RECORD_SIZE        (uint32 + uint16)

typedef struct {
	DCL_OFFSET(LangSys, DefaultLangSys);    /* |-> Script */
	unsigned short LangSysCount;
	LangSysRecord *LangSysRecord;           /* [LangSysCount] */
} Script;
#define SCRIPT_SIZE(nLanguages) (uint16 * 2 + LANG_SYS_RECORD_SIZE * (nLanguages))

typedef struct {
	Tag ScriptTag;
	DCL_OFFSET(Script, Script);             /* |-> ScriptList */
} ScriptRecord;
#define SCRIPT_RECORD_SIZE          (uint32 + uint16)

typedef struct {
	unsigned short ScriptCount;
	ScriptRecord *ScriptRecord;             /* [ScriptCount] */
} ScriptList;
#define SCRIPT_LIST_SIZE(nScripts)  (uint16 + SCRIPT_RECORD_SIZE * (nScripts))

/* --- FeatureList --- */
typedef struct {
	Offset FeatureParams;                   /* Reserved=NULL */
	unsigned short LookupCount;
	unsigned short *LookupListIndex;        /* [LookupCount] */
} Feature;
#define FEATURE_SIZE(nLookups)      (uint16 * 2 + uint16 * (nLookups))
#define IS_FEAT_PARAM(L)   (((L) >> 16) == GPOSFeatureParam)   /* test otl SubTable->lookup value. */

typedef struct {
	Tag FeatureTag;
	DCL_OFFSET(Feature, Feature);           /* |-> FeatureList */
} FeatureRecord;
#define FEATURE_RECORD_SIZE         (uint32 + uint16)

typedef struct {
	unsigned short FeatureCount;
	FeatureRecord *FeatureRecord;           /* [FeatureCount] */
} FeatureList;
#define FEATURE_LIST_SIZE(nFeatures) (uint16 + FEATURE_RECORD_SIZE * (nFeatures))
#define FEAT_PARAM_ZERO 1 /* a dummy valued, used to differenatiate NULL_OFFSET from a real zero offset for the FeatureParams field.*/
/* --- LookupList --- */
typedef struct {
	unsigned short LookupType;
	unsigned short LookupFlag;
	unsigned short SubTableCount;
	unsigned short UseMarkSetIndex; /* This is not counted in LOOKUP_HDR_SIZE, as it is written only if (LookupFlag & otlUseMarkFilteringSet).*/
	long *SubTable;             /* [SubTableCount] |-> Lookup
	                               long instead of Offset for temp -ve value */
} Lookup;
#define LOOKUP_HDR_SIZE             (uint16 * 3)
#define LOOKUP_SIZE(nSubtables)     (LOOKUP_HDR_SIZE + uint16 * (nSubtables))

typedef struct {
	unsigned short LookupCount;
	DCL_OFFSET_ARRAY(Lookup, Lookup);       /* [LookupCount] |-> LookupList */
} LookupList;
#define LOOKUP_LIST_SIZE(nLookups)  (uint16 + uint16 * (nLookups))

/* --- Header --- */
typedef struct {
	Fixed Version;
	DCL_OFFSET(ScriptList, ScriptList);     /* |-> Header */
	DCL_OFFSET(FeatureList, FeatureList);   /* |-> Header */
	DCL_OFFSET(LookupList, LookupList);     /* |-> Header */
} Header;
#define HEADER_SIZE                 (uint32 + uint16 * 3)

/* --- Build --- */

typedef struct {
	Tag script;
	Tag language;
	Tag feature;
	unsigned long lookup;   /* Lookup type (31-16), lookup flag (15-0) */
	unsigned short extensionLookupType;
	unsigned short offset;
	unsigned short markSetIndex; /* Used and written only of lookupFlag UseMarkEst is set. */
	Label label;
	unsigned short fmt;     /* Subtable format (Debug only) */
	int isFeatParam;
	int seenInFeature;
	struct {                 /* Reference indexes */
		short feature;
		short lookup;
	} index;
	struct {                 /* Indexes of next different elements in table */
		short script;
		short language;
		short feature;
		short lookup;
	} span;
} Subtable;

typedef struct {
	Label label;
	int lookupInx;
	int used; /* set to 1 if this label is actually referenced */
} LabelInfo;

typedef struct {
	Label baseLabel;  /* label of referenced subtable */
	Label refLabel; /* label of referencing subtable */
} RefLabelInfo;

struct otlTbl_ {
	dnaDCL(Subtable, subtable);             /* Subtable list */
	dnaDCL(LabelInfo, label);               /* maps: label -> lookup index */
	dnaDCL(RefLabelInfo, refLabel);         /* used to make sure that all stand-alone lookups actually get referenced. */
	Header tbl;                             /* Formatted table */
	Offset lookupSize;                      /* Size of Lookup List + Lookups */
	Coverage coverage;
	Class class;
	short nAnonSubtables;
	short nStandAloneSubtables;
	short nRefLookups; /* number of otl Subtable 's that are only references to look ups*/
	short nFeatParams; /* number of otl Subtable 's that hold feature table parameters.*/
#if HOT_DEBUG
	int nCoverageReused;
	int nClassReused;
#endif
};

#if HOT_DEBUG
/* Initialize subtable. Not needed except for debug dumps */
static void initSubtable(void *ctx, long count, Subtable *sub) {
	long i;
	for (i = 0; i < count; i++) {
		sub->seenInFeature = sub->isFeatParam = 0;
		sub->span.script = sub->span.language = sub->span.feature =
		            sub->span.lookup = 0;
		sub->index.feature = sub->index.lookup = -1;
		sub++;
	}
	return;
}

static void valDump(short val, short excep, int isRef) {
	if (val == excep) {
		fprintf(stderr, "   * ");
	}
	else if (isRef) {
		fprintf(stderr, "   ->%hd ", val);
	}
	else {
		fprintf(stderr, "%4hd ", val);
	}
}

static void tagDump(Tag tag, char ch) {
	if (tag == TAG_UNDEF) {
		fprintf(stderr, "****");
	}
	else {
		fprintf(stderr, "%c%c%c%c", TAG_ARG(tag));
	}
	if (ch != 0) {
		fprintf(stderr, "%c", ch);
	}
}

void otlDumpSubtables(hotCtx g, otlTbl t) {
	long i;

	fprintf(stderr,
	        "# nCov: %ld, nClass: %ld; nCovReused: %d, nClassReused: %d\n"
	        "# [offs relative to start of subtable section]\n",
	        t->coverage.tables.cnt, t->class.tables.cnt,
	        t->nCoverageReused, t->nClassReused);
	fprintf(stderr,
	        "      -----tag------ --look-----            -------span--------  --index--\n"
	        "      scri lang feat typ/fmt|flg offs  lab  scri lang feat look  feat look"
	        "\n");
	for (i = 0; i < t->subtable.cnt; i++) {
		Subtable *sub = &t->subtable.array[i];
		fprintf(stderr, "[%3ld] ", i);
		tagDump(sub->script, ' ');
		tagDump(sub->language, ' ');
		tagDump(sub->feature, ' ');
		fprintf(stderr, "   %2lu/%hu|%-3lx %4hx ",
		        sub->lookup >> 16,
		        sub->fmt,
		        sub->lookup & 0xffff,
		        sub->offset);
		fprintf(stderr, "%4x ", sub->label);

		valDump(sub->span.script, 0, 0);
		valDump(sub->span.language, 0, 0);
		valDump(sub->span.feature, 0, 0);
		valDump(sub->span.lookup, 0, 0);
		fprintf(stderr, " ");
		valDump(sub->index.feature, -1, 0);
		valDump(sub->index.lookup, -1, IS_REF_LAB(sub->label));
		fprintf(stderr, "\n");
	}
}

/* Must call only after otlTableFill(). Returns total length of the Script-,
   Feature-, and Lookup Lists, with all their associated structures (Script,
   LangSys, Feature, Lookup). */
static LOffset otlGetHeaderSize(otlTbl t) {
	return t->tbl.LookupList + t->lookupSize;
}

void otlDumpSizes(hotCtx g, otlTbl t, LOffset subtableSize,
                  LOffset extensionSectionSize) {
	LOffset s, tot = 0;

	s = otlGetHeaderSize(t);
	tot += s;
	DF(1, (stderr, "  Scr,Fea,Lkp lists:     size %5u, tot %5u\n", s, tot));

	s = subtableSize;
	tot += s;
	DF(1, (stderr, "  Main subtable section: size %5u, tot %5u\n", s, tot));

	s = otlGetCoverageSize(t);
	tot += s;
	DF(1, (stderr, "  Main coverage section: size %5u, tot ", s));
	if (s != 0) {
		DF(1, (stderr, "%5u\n", tot));
	}
	else {
		DF(1, (stderr, "    \"\n"));
	}

	s = otlGetClassSize(t);
	tot += s;
	DF(1, (stderr, "  Main class section:    size %5u, tot ", s));
	if (s != 0) {
		DF(1, (stderr, "%5u\n", tot));
	}
	else {
		DF(1, (stderr, "    \"\n"));
	}

	s = extensionSectionSize;
	tot += s;
	DF(1, (stderr, "  Extension section:     size %5u, tot ", s));
	if (s != 0) {
		DF(1, (stderr, "%5u\n", tot));
	}
	else {
		DF(1, (stderr, "    \"\n"));
	}
}

#endif /* HOT_DEBUG */

/* --------------------------- Standard Functions -------------------------- */

void otlNew(hotCtx g) {
	otlCtx h = MEM_NEW(g, sizeof(struct otlCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.otl = h;
}

void otlFree(hotCtx g) {
	MEM_FREE(g, g->ctx.otl);
}

/* --------------------------- Coverage Functions -------------------------- */

/* Element initializer */
static void coverageRecordInit(void *ctx, long count, CoverageRecord *rec) {
	hotCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, rec->glyph, 50, 50);
		rec++;
	}
	return;
}

/* Initialize coverage tables */
static void coverageNew(hotCtx g, otlTbl t) {
	t->coverage.offset = 0;
	dnaINIT(g->dnaCtx, t->coverage.tables, 10, 5);
	t->coverage.tables.func = coverageRecordInit;
}

/* Fill format 1 table */
static CoverageFormat1 *fillCoverage1(hotCtx g, unsigned nGlyphs, GID *glyph) {
	unsigned int i;
	unsigned int dstIndex;
	CoverageFormat1 *fmt = MEM_NEW(g, sizeof(CoverageFormat1)); /* This may be more than we need, becuase we skip suplicate glyoh ID;s, but there's no harm in that. */

	fmt->CoverageFormat = 1;
	fmt->GlyphArray = MEM_NEW(g, sizeof(GID) * nGlyphs);
	//COPY(fmt->GlyphArray, glyph, nGlyphs);

	fmt->GlyphArray[0] = glyph[0];
	dstIndex = 1;
	for (i = 1; i < nGlyphs; i++) {
		if (glyph[i - 1] == glyph[i]) {
			/* Skip duplicate glyphs */

#if HOT_DEBUG
			printf("duplicated glyph ['%d']in coverage fmt 1.\n", glyph[i]);
#endif
			continue;
		}
		fmt->GlyphArray[dstIndex] = glyph[i];
		dstIndex++;
	}
	fmt->GlyphCount = dstIndex;
	return fmt;
}

/* Fill format 2 table */
static CoverageFormat2 *fillCoverage2(hotCtx g, int nRanges, long nGlyphs,
                                      GID *glyph) {
	int i;
	int iRange;
	int iStart;
	unsigned int curNGlyph;
	CoverageFormat2 *fmt = MEM_NEW(g, sizeof(CoverageFormat2));

	fmt->CoverageFormat = 2;
	fmt->RangeCount = nRanges;
	fmt->RangeRecord = MEM_NEW(g, sizeof(RangeRecord) * nRanges);

	iRange = 0;
	iStart = 0;
	curNGlyph = 0;

	for (i = 1; i <= nGlyphs; i++) {
		if (i == nGlyphs || glyph[i - 1] != glyph[i] - 1) {
			RangeRecord *rec;

			if ((i != nGlyphs) && (glyph[i - 1] == glyph[i])) {
				/* Skip duplicate glyphs */

	#if HOT_DEBUG
				printf("duplicated glyph ['%d']in coverage fmt 2.\n", glyph[i]);
	#endif
				continue;
			}
			rec = &fmt->RangeRecord[iRange++];

			rec->Start = glyph[iStart];
			rec->End = glyph[i - 1];
			iStart = i;
			rec->StartCoverageIndex = curNGlyph - (rec->End - rec->Start);
		}
		curNGlyph++;
	}
	return fmt;
}

/* Fill coverage table */
static Offset fillCoverage(hotCtx g, otlTbl t) {
	int i;
	int nRanges;
	unsigned size1;
	unsigned size2;
	CoverageRecord *new = t->coverage.new;
	long nGlyphs = new->glyph.cnt;
	long nFinalGlyphs;
	GID *glyph = new->glyph.array;

	new->offset = (unsigned short)t->coverage.offset;

	/* Count ranges (assumes glyphs sorted by increasing GID) */
	nRanges = 1;
	nFinalGlyphs = 1;
	for (i = 1; i < nGlyphs; i++) {
		if (glyph[i - 1] == glyph[i]) {
			/* Skip duplicate glyphs */
			continue;
		}
		nFinalGlyphs++;
		if (glyph[i - 1] != glyph[i] - 1) {
			nRanges++;
		}
	}

	size1 = COVERAGE1_SIZE(nFinalGlyphs);
	size2 = COVERAGE2_SIZE(nRanges);

	/* Choose smallest representation */

	if (size1 < size2) {
		new->tbl = fillCoverage1(g, nGlyphs, glyph);
		t->coverage.offset += size1;
	}
	else {
		new->tbl = fillCoverage2(g, nRanges, nGlyphs, glyph);
		t->coverage.offset += size2;
	}

	if (t->coverage.offset > 0xFFFF) {
		hotMsg(g, hotFATAL, "coverage section too large (%0lx)",
		       t->coverage.offset);
	}

	return new->offset; /* Return new table's offset */
}

/* Write format 1 table */
static void writeCoverage1(otlCtx h, CoverageFormat1 *fmt) {
	int i;

	OUT2(fmt->CoverageFormat);
	OUT2(fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) {
		OUT2(fmt->GlyphArray[i]);
	}
}

/* Write format 2 table */
static void writeCoverage2(otlCtx h, CoverageFormat2 *fmt) {
	int i;

	OUT2(fmt->CoverageFormat);
	OUT2(fmt->RangeCount);
	for (i = 0; i < fmt->RangeCount; i++) {
		RangeRecord *rec = &fmt->RangeRecord[i];
		OUT2(rec->Start);
		OUT2(rec->End);
		OUT2(rec->StartCoverageIndex);
	}
}

/* Write all coverage tables */
void otlCoverageWrite(hotCtx g, otlTbl t) {
	int i;
	otlCtx h = g->ctx.otl;

	for (i = 0; i < t->coverage.tables.cnt; i++) {
		void *tbl = t->coverage.tables.array[i].tbl;

		switch (*(unsigned short *)tbl) {
			case 1:
				writeCoverage1(h, tbl);
				break;

			case 2:
				writeCoverage2(h, tbl);
				break;
		}
	}
}

/* Free format 1 table */
static void freeCoverage1(hotCtx g, CoverageFormat1 *fmt) {
	MEM_FREE(g, fmt->GlyphArray);
}

/* Free format 2 table */
static void freeCoverage2(hotCtx g, CoverageFormat2 *fmt) {
	MEM_FREE(g, fmt->RangeRecord);
}

/* Free formated table */
static void freeCoverage(hotCtx g, CoverageRecord *rec) {
	switch (*(unsigned short *)rec->tbl) {
		case 1:
			freeCoverage1(g, rec->tbl);
			break;

		case 2:
			freeCoverage2(g, rec->tbl);
			break;
	}
	MEM_FREE(g, rec->tbl);
}

static void coverageReuse(hotCtx g, otlTbl t) {
	int i;
	for (i = 0; i < t->coverage.tables.cnt; i++) {
		freeCoverage(g, &t->coverage.tables.array[i]);
	}
	t->coverage.tables.cnt = 0;
	t->coverage.offset = 0;
}

/* Free all coverage tables */
static void coverageFree(hotCtx g, otlTbl t) {
	int i;
	for (i = 0; i < t->coverage.tables.size; i++) {
		dnaFREE(t->coverage.tables.array[i].glyph);
	}
	dnaFREE(t->coverage.tables);
}

/* Begin new coverage table */
void otlCoverageBegin(hotCtx g, otlTbl t) {
	t->coverage.new = dnaNEXT(t->coverage.tables);
	t->coverage.new->glyph.cnt = 0;
}

/* Add coverage glyph */
void otlCoverageAddGlyph(hotCtx g, otlTbl t, GID glyph) {
	*dnaNEXT(t->coverage.new->glyph) = glyph;
}

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

/* End coverage table; uniqueness of GIDs up to client. Sorting done here. */
Offset otlCoverageEnd(hotCtx g, otlTbl t) {
	long i;
	CoverageRecord *new = t->coverage.new;

	/* Sort glyph ids into increasing order */
	qsort(new->glyph.array, new->glyph.cnt, sizeof(GID), cmpGlyphIds);

	/* Check for matching table */
	for (i = 0; i < t->coverage.tables.cnt - 1; i++) {
		CoverageRecord *old = &t->coverage.tables.array[i];

		if (new->glyph.cnt == old->glyph.cnt) {
			long j;
			for (j = 0; j < new->glyph.cnt; j++) {
				if (new->glyph.array[j] != old->glyph.array[j]) {
					goto next;
				}
			}

			/* Found match */
#if HOT_DEBUG
			t->nCoverageReused++;
#if 0
			if (DF_LEVEL >= 2) {
				fprintf(stderr, "# Using coverage already present:");
				for (j = 0; j < old->glyph.cnt; j++) {
					gidDump(g, old->glyph.array[j],
					        j != old->glyph.cnt - 1 ? ' ' : 0);
				}
				fprintf(stderr, "\n");
			}
#endif
#endif
			t->coverage.tables.cnt--;   /* Remove new table */
			return old->offset;         /* Return matching table's offset */
		}
next:;
	}

	/* No match; fill table and return its offset  */
	return fillCoverage(g, t);
}

/* Returns total length of the coverage section, for all coverages currently
   defined. */
LOffset otlGetCoverageSize(otlTbl t) {
	return t->coverage.offset;
}

/* Returns total length of the class section, for all classes currently
   defined. */
LOffset otlGetClassSize(otlTbl t) {
	return t->class.offset;
}

/* ---------------------------- Class Functions ---------------------------- */

/* Class record element initializer */
static void classRecordInit(void *ctx, long count, ClassRecord *rec) {
	hotCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->dnaCtx, rec->map, 50, 50);
		rec++;
	}
	return;
}

static void classNew(hotCtx g, otlTbl t) {
	t->class.offset = 0;
	dnaINIT(g->dnaCtx, t->class.tables, 10, 5);
	t->class.tables.func = classRecordInit;
}

/* Fill format 1 table */
static ClassDefFormat1 *fillClass1(hotCtx g,
                                   int nMaps, ClassMap *maps, int nGlyphs) {
	int i;
	ClassDefFormat1 *fmt = MEM_NEW(g, sizeof(ClassDefFormat1));

	fmt->ClassFormat = 1;
	fmt->StartGlyph = nMaps ? maps[0].glyph : GID_NOTDEF;
	fmt->GlyphCount = nGlyphs;
	fmt->ClassValueArray = nMaps ? MEM_NEW(g, sizeof(unsigned short) * nGlyphs)
		: NULL;

	/* Initialize to class 0 */
	for (i = 0; i < nGlyphs; i++) {
		fmt->ClassValueArray[i] = 0;
	}

	/* Add explicit glyph classes */
	for (i = 0; i < nMaps; i++) {
		fmt->ClassValueArray[maps[i].glyph - fmt->StartGlyph] = maps[i].class;
	}
	return fmt;
}

/* Fill format 2 table */
static ClassDefFormat2 *fillClass2(hotCtx g,
                                   int nMaps, ClassMap *maps, int nRanges) {
	int i;
	int iRange;
	int iStart;
	ClassDefFormat2 *fmt = MEM_NEW(g, sizeof(ClassDefFormat2));

	fmt->ClassFormat = 2;
	fmt->ClassRangeCount = nRanges;
	fmt->ClassRangeRecord = nRanges ?
	    MEM_NEW(g, sizeof(ClassRangeRecord) * nRanges) : NULL;

	iRange = 0;
	iStart = 0;
	for (i = 1; i <= nMaps; i++) {
		if (i == nMaps ||
		    maps[i - 1].glyph != maps[i].glyph - 1 ||
		    maps[i - 1].class != maps[i].class) {
			ClassRangeRecord *rec = &fmt->ClassRangeRecord[iRange++];

			rec->Start = maps[iStart].glyph;
			rec->End = maps[i - 1].glyph;
			rec->Class = maps[i - 1].class;

			iStart = i;
		}
	}
	return fmt;
}

/* Fill class table */
static Offset fillClass(hotCtx g, otlTbl t) {
	int i;
	int nRanges;
	unsigned size1;
	unsigned size2;
	ClassRecord *new = t->class.new;
	int nMaps = new->map.cnt;
	ClassMap *maps = new->map.array;
	int nGlyphs = nMaps ? maps[nMaps - 1].glyph - maps[0].glyph + 1
		: 0;

	new->offset = (unsigned short)t->class.offset;

	/* Count ranges (assumes glyphs sorted by increasing GID) */
	nRanges = nMaps != 0;
	for (i = 1; i < nMaps; i++) {
		if (maps[i - 1].glyph != maps[i].glyph - 1 ||
		    maps[i - 1].class != maps[i].class) {
			/* Start new range when break in GID sequence or new class */
			nRanges++;
		}
	}

	size1 = CLASS1_SIZE(nGlyphs);
	size2 = CLASS2_SIZE(nRanges);

	/* Choose smallest representation */
	if (size1 < size2) {
		new->tbl = fillClass1(g, nMaps, maps, nGlyphs);
		t->class.offset += size1;
	}
	else {
		new->tbl = fillClass2(g, nMaps, maps, nRanges);
		t->class.offset += size2;
	}
	if (t->class.offset > 0xFFFF) {
		hotMsg(g, hotFATAL, "class section too large (%0lx)", t->class.offset);
	}

	return new->offset; /* Return new table's offset */
}

/* Write format 1 table */
static void writeClass1(otlCtx h, ClassDefFormat1 *fmt) {
	int i;

	OUT2(fmt->ClassFormat);
	OUT2(fmt->StartGlyph);
	OUT2(fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) {
		OUT2(fmt->ClassValueArray[i]);
	}
}

/* Write format 2 table */
static void writeClass2(otlCtx h, ClassDefFormat2 *fmt) {
	int i;

	OUT2(fmt->ClassFormat);
	OUT2(fmt->ClassRangeCount);
	for (i = 0; i < fmt->ClassRangeCount; i++) {
		ClassRangeRecord *rec = &fmt->ClassRangeRecord[i];
		OUT2(rec->Start);
		OUT2(rec->End);
		OUT2(rec->Class);
	}
}

/* Write all class tables */
void otlClassWrite(hotCtx g, otlTbl t) {
	int i;
	otlCtx h = g->ctx.otl;

	for (i = 0; i < t->class.tables.cnt; i++) {
		void *tbl = t->class.tables.array[i].tbl;

		switch (*(unsigned short *)tbl) {
			case 1:
				writeClass1(h, tbl);
				break;

			case 2:
				writeClass2(h, tbl);
				break;
		}
	}
}

/* Free format 1 table */
static void freeClass1(hotCtx g, ClassDefFormat1 *fmt) {
	MEM_FREE(g, fmt->ClassValueArray);
}

/* Free format 2 table */
static void freeClass2(hotCtx g, ClassDefFormat2 *fmt) {
	MEM_FREE(g, fmt->ClassRangeRecord);
}

/* Free formated table */
static void freeClass(hotCtx g, ClassRecord *rec) {
	switch (*(unsigned short *)rec->tbl) {
		case 1:
			freeClass1(g, rec->tbl);
			break;

		case 2:
			freeClass2(g, rec->tbl);
			break;
	}
	MEM_FREE(g, rec->tbl);
}

static void classReuse(hotCtx g, otlTbl t) {
	int i;
	for (i = 0; i < t->class.tables.cnt; i++) {
		freeClass(g, &t->class.tables.array[i]);
	}
	t->class.tables.cnt = 0;
	t->class.offset = 0;
}

/* Free all class tables */
static void classFree(hotCtx g, otlTbl t) {
	int i;
	for (i = 0; i < t->class.tables.size; i++) {
		dnaFREE(t->class.tables.array[i].map);
	}
	dnaFREE(t->class.tables);
}

/* Begin new class table */
void otlClassBegin(hotCtx g, otlTbl t) {
	t->class.new = dnaNEXT(t->class.tables);
	t->class.new->map.cnt = 0;
}

/* Add class mapping; ignore class 0's */
void otlClassAddMapping(hotCtx g, otlTbl t, GID glyph, unsigned int class) {
	if (class != 0) {
		ClassMap *map = dnaNEXT(t->class.new->map);
		map->glyph = glyph;
		map->class = class;
	}
}

/* Compare glyph ids */
static int CDECL cmpClassMaps(const void *first, const void *second) {
	unsigned short aClass;
	unsigned short bClass;
	GID a = ((ClassMap *)first)->glyph;
	GID b = ((ClassMap *)second)->glyph;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	/* Note that I am sorting the classes in increasing class index. The result is that a higher index
	   will overwrite a lower index, when this is written to a class definition.. */
	aClass = ((ClassMap *)first)->class;
	bClass = ((ClassMap *)second)->class;
	if (aClass < bClass) {
		return -1;
	}
	else if (aClass > bClass) {
		return 1;
	}
	return 0;
}

/* End class table */
Offset otlClassEnd(hotCtx g, otlTbl t) {
	int i;
	ClassRecord *new = t->class.new;

	/* Sort glyph ids into increasing order */
	qsort(new->map.array, new->map.cnt, sizeof(ClassMap), cmpClassMaps);

	/* Check for matching table */
	for (i = 0; i < t->class.tables.cnt - 1; i++) {
		ClassRecord *old = &t->class.tables.array[i];

		if (new->map.cnt == old->map.cnt) {
			int j;
			for (j = 0; j < new->map.cnt; j++) {
				if (new->map.array[j].glyph != old->map.array[j].glyph ||
				    new->map.array[j].class != old->map.array[j].class) {
					goto next;
				}
			}

			/* Found match */
#if HOT_DEBUG
			t->nClassReused++;
#if 0
			if (DF_LEVEL >= 2) {
				fprintf(stderr, "# Using class already present:");
				for (j = 0; j < old->map.cnt; j++) {
					fprintf(stderr, " ");
					gidDump(g, old->map.array[j].glyph, 0);
					fprintf(stderr, ":%u", old->map.array[j].class);
				}
				fprintf(stderr, "\n");
			}
#endif
#endif
			t->class.tables.cnt--;      /* Remove new table */
			return old->offset;         /* Return matching table's offset */
		}
next:;
	}

	/* No match; fill table and return its offset  */
	return fillClass(g, t);
}

/* ---------------------------- Device Functions --------------------------- */

/* I don't know if we'll ever need device tables. If we do it would be
   implemented here */

/* ---------------------------- Table Functions ---------------------------- */

/* Create new table context */
otlTbl otlTableNew(hotCtx g) {
	otlTbl t = MEM_NEW(g, sizeof(struct otlTbl_));

	dnaINIT(g->dnaCtx, t->subtable, 10, 5);
	dnaINIT(g->dnaCtx, t->label, 50, 100);
	dnaINIT(g->dnaCtx, t->refLabel, 50, 100);

#if HOT_DEBUG
	t->subtable.func = initSubtable;
	t->nCoverageReused = 0;
	t->nClassReused = 0;
#endif
	coverageNew(g, t);
	classNew(g, t);
	t->nAnonSubtables = t->nStandAloneSubtables = t->nRefLookups = t->nFeatParams = 0;

	return t;
}

/* Compare labels */
static int CDECL cmpLabels(const void *first, const void *second, void *ctx) {
	const LabelInfo *a = first;
	const LabelInfo *b = second;

	if (a->label < b->label) {
		return -1;
	}
	else if (a->label > b->label) {
		return 1;
	}
	else if (a != b) {
		hotCtx *g = ctx;
		hotMsg(*g, hotFATAL, "[internal] duplicate subtable label encountered");
	}
	return 0;   /* Suppress compiler warning */
}

static int CDECL cmpLabelsByLookup(const void *first, const void *second, void *ctx) {
	const LabelInfo *a = first;
	const LabelInfo *b = second;

	if (a->lookupInx < b->lookupInx) {
		return -1;
	}
	else if (a->lookupInx > b->lookupInx) {
		return 1;
	}
	else if (a->label < b->label) {
		return -1;
	}
	else if (a->label > b->label) {
		return 1;
	}
	else if (a != b) {
		hotCtx *g = ctx;
		hotMsg(*g, hotFATAL, "[internal] duplicate subtable label encountered");
	}
	return 0;   /* Suppress compiler warning */
}

static int CDECL matchLabel(const void *key, const void *value) {
	int a = *(int *)key;
	int b = ((LabelInfo *)value)->label;
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

static int CDECL matchLabelByLookup(const void *key, const void *value) {
	int a = *(int *)key;
	int b = ((LabelInfo *)value)->lookupInx;
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

/* Returns the lookup index associated with baselabel (which should not have
   the reference bit set). May be called from outside this module only after
   otlTableFill() */
int otlLabel2LookupIndex(hotCtx g, otlTbl t, int baselabel) {
	LabelInfo *li =
	    (LabelInfo *)bsearch(&baselabel, t->label.array, t->label.cnt,
	                         sizeof(LabelInfo), matchLabel);
	if (li == NULL) {
		hotMsg(g, hotFATAL, "(internal) label 0x%x not found", baselabel);
	}
	else {
		li->used = 1;
	}
	return li->lookupInx;
}

/* Looks through the lable list for the named lookup index Returns 1 if it has been referenced, else
   returns -1. May be called from outside this module only after otlTableFill() */
int findSeenRef(hotCtx g, otlTbl t, int baseLookup) {
	LabelInfo *li =
	    (LabelInfo *)bsearch(&baseLookup, t->label.array, t->label.cnt,
	                         sizeof(LabelInfo), matchLabelByLookup);
	if (li == NULL) {
		hotMsg(g, hotFATAL, "Base lookup %d not found", baseLookup);
	}

	return li->used;
}

/* Sort in LookupList order: non-reference subtbls by offset (with aalt sorting
   first); then Feat Params; last references. */
static int CDECL cmpLookupList(const void *first, const void *second,
                               void *ctx) {
	const Subtable *a = first;
	const Subtable *b = second;
	Tag aalt_tag = TAG('a', 'a', 'l', 't');

	if (!IS_REF_LAB(a->label) && IS_REF_LAB(b->label)) {
		return -1;
	}
	else if (IS_REF_LAB(a->label) && !IS_REF_LAB(b->label)) {
		return 1;
	}
	else if (IS_REF_LAB(a->label) && IS_REF_LAB(b->label)) {
		return 0;                       /* Don't bother sorting references */
	}
	else if ((!a->isFeatParam) && b->isFeatParam) {
		return -1;
	}
	else if (a->isFeatParam && (!b->isFeatParam)) {
		return 1;
	}

	else if ((a->feature != b->feature) && (a->feature == aalt_tag)) {
		return -1;
	}
	else if ((a->feature != b->feature) && (b->feature == aalt_tag)) {
		return 1;
	}

	else if (a->offset < b->offset) {
		return -1;
	}
	else if (a->offset > b->offset) {
		return 1;
	}

	else if (a != b) {
		hotCtx *g = ctx;
		hotMsg(*g, hotFATAL, "[internal] subtables have same offset");
	}
	return 0;   /* Suppress compiler warning */
}

void sortLabelList(hotCtx g, otlTbl t) {
	ctuQSort(t->label.array, t->label.cnt, sizeof(LabelInfo), cmpLabels, &g);
}

/* Calculate LookupList indexes. Granularity of lookups has already been
   determined by the client of this module by labels. Prepare the label ->
   lkpInx mapping */
static void calcLookupListIndexes(hotCtx g, otlTbl t) {
	long i;
	int indexCnt = 0;

	ctuQSort(t->subtable.array, t->subtable.cnt, sizeof(Subtable),
	         cmpLookupList, &g);

	/* Process all regular subtables and subtables with feature parameters.
	   We want the lookups indices to be ordered in the order that they were created
	   by feature file definitions.
	 */
	for (i = 0; i < (t->subtable.cnt - t->nRefLookups); i++) {
		Subtable *sub = &t->subtable.array[i];

		if (i == 0 || sub->label != (sub - 1)->label) {
			/* Label i.e. lookup change. Store lab -> lookupInx info in this first subtable of the span of tables with the same label.  */
			LabelInfo *lab = dnaNEXT(t->label);
			lab->used = 0; /* new label; hasn't been referenced by anything yet. */

			/* sub-tables with feature Params currently don't have any look-ups. */
			if (sub->isFeatParam) {
				sub->index.lookup = -1;
			}
			else {
				sub->index.lookup = indexCnt++;
			}
			lab->label = sub->label;
			lab->lookupInx = sub->index.lookup;
		}
		else {
			sub->index.lookup = indexCnt - 1;
		}
	}

	ctuQSort(t->label.array, t->label.cnt, sizeof(LabelInfo), cmpLabels, &g);


	/* Fill the index of any reference subtables (sorted at end) */
	for (; i < t->subtable.cnt; i++) {
		Subtable *sub = &t->subtable.array[i];
		sub->index.lookup = otlLabel2LookupIndex(g, t, sub->label & ~REF_LAB);
	}
}

/* Determine granularity of a Feature */
static int CDECL cmpFeatTmp(const void *first, const void *second) {
	const Subtable *a = first;
	const Subtable *b = second;

	if ((a->feature == TAG_STAND_ALONE) && (b->feature != TAG_STAND_ALONE)) {
		return 1;
	}
	else if ((b->feature == TAG_STAND_ALONE) && (a->feature != TAG_STAND_ALONE)) {
		return -1;
	}
	else if (a->script < b->script) {
		/* Make sure that anonymous lookups sort last (script == TAG_UDEF, aka 0cFFFF) */
		return -1;
	}
	else if (a->script > b->script) {
		return 1;
	}
	else if (a->language < b->language) {
		/* Test language */
		return -1;
	}
	else if (a->language > b->language) {
		return 1;
	}
	else if (a->feature < b->feature) {
		/* Test feature */
		return -1;
	}
	else if (a->feature > b->feature) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Sort in FeatureList order */
static int CDECL cmpFeatureList(const void *first, const void *second) {
	const Subtable *a = first;
	const Subtable *b = second;

	if ((a->feature == TAG_STAND_ALONE) && (b->feature != TAG_STAND_ALONE)) {
		return 1;
	}
	else if ((b->feature == TAG_STAND_ALONE) && (a->feature != TAG_STAND_ALONE)) {
		return -1;
	}
	else if (a->feature < b->feature) {
		/* Test feature */
		return -1;
	}
	else if (a->feature > b->feature) {
		return 1;
	}
	else if (a->feature == TAG_UNDEF) {
		/* Don't bother further checking anon lookups - these are sorted to the end by now. */
		return 0;
	}
	else if (a->index.feature < b->index.feature) {
		/* Test feature index */
		return -1;
	}
	else if (a->index.feature > b->index.feature) {
		return 1;
	}
	else if (a->index.lookup < b->index.lookup) {
		/* Test lookup index */
		return -1;
	}
	else if (a->index.lookup > b->index.lookup) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Calculate feature indexes */
static void calcFeatureListIndexes(hotCtx g, otlTbl t) {
	long i;
	int indexCnt;
	int prevIndex;

	/* Determine granularity of features */
	qsort(t->subtable.array, t->subtable.cnt, sizeof(Subtable), cmpFeatTmp);

	/* Assign temporary feature indexes according to feature granularity (i.e.
	   same script, language & feature), disregarding whether reference or has
	   parameters or not */
	indexCnt = 0;
	for (i = 0; i < t->subtable.cnt - t->nAnonSubtables; i++) {
		Subtable *sub = &t->subtable.array[i];
		Subtable *prev = sub - 1;   /* xxx */

		sub->index.feature = (i == 0 || !(sub->script == prev->script &&
		                                  sub->language == prev->language &&
		                                  sub->feature == prev->feature))
		    ? indexCnt++    /* Feature change */
			: indexCnt - 1;
	}

	/* Sort in final feature order */
	qsort(t->subtable.array, t->subtable.cnt, sizeof(Subtable), cmpFeatureList);

	/* Assign final feature indexes */
	prevIndex = -1;
	indexCnt = 0;
	for (i = 0; i < t->subtable.cnt - t->nAnonSubtables; i++) {
		Subtable *sub = &t->subtable.array[i];

		if (sub->index.feature != prevIndex) {
			/* -- Feature change --  */
			prevIndex = sub->index.feature;
			sub->index.feature = indexCnt++;
		}
		else {
			sub->index.feature = indexCnt - 1;
		}
	}
}

/* Sort in ScriptList order */
static int CDECL cmpScriptList(const void *first, const void *second) {
	const Subtable *a = first;
	const Subtable *b = second;

	if ((a->feature == TAG_STAND_ALONE) && (b->feature != TAG_STAND_ALONE)) {
		return 1;
	}
	else if ((b->feature == TAG_STAND_ALONE) && (a->feature != TAG_STAND_ALONE)) {
		return -1;
	}
	else if (a->script < b->script) {
		/* Test script Tag TAG_UNDEF is sorted to the end, as it is 0xFFFF. */
		return -1;
	}
	else if (a->script > b->script) {
		return 1;
	}
	else if (a->script == TAG_UNDEF) {
		return 0;                       /* Don't bother checking anon lkps */
	}
	else if (a->language < b->language) {
		/* Test language */
		return -1;
	}
	else if (a->language > b->language) {
		return 1;
	}
	else if (a->feature < b->feature) {
		/* Test feature index */
		return -1;
	}
	else if (a->feature > b->feature) {
		return 1;
	}
	else {
		return 0;   /* Further testing not needed for ScriptList */
	}
}

/* Calculate script, language, and index.feature spans */
static void prepScriptList(hotCtx g, otlTbl t) {
	int scr;
	int Scr;
	int spanLimit = t->subtable.cnt - t->nAnonSubtables;

	qsort(t->subtable.array, t->subtable.cnt, sizeof(Subtable), cmpScriptList);

	Scr = 0;
	for (scr = 1; scr <= spanLimit; scr++) {
		if (scr == spanLimit || t->subtable.array[scr].script !=
		    t->subtable.array[Scr].script) {
			/* -- Script change -- */
			int lan;
			int Lan = Scr;

			t->subtable.array[Scr].span.script = scr;

			for (lan = Lan + 1; lan <= scr; lan++) {
				if (lan == scr || t->subtable.array[lan].language !=
				    t->subtable.array[Lan].language) {
					/* -- Language change -- */
					int fea;
					int Fea = Lan;

					t->subtable.array[Lan].span.language = lan;

					for (fea = Fea + 1; fea <= lan; fea++) {
						if (fea == lan ||
						    t->subtable.array[fea].index.feature !=
						    t->subtable.array[Fea].index.feature) {
							/* -- Feature index change -- */
							t->subtable.array[Fea].span.feature = fea;
							Fea = fea;
						}
					}
					Lan = lan;
				}
			}
			Scr = scr;
		}
	}
}

/* Fill language system record */
static Offset fillLangSysRecord(hotCtx g, otlTbl t, LangSys *sys,
                                int iLanguage) {
	int i;
	int j;
	int nFeatures;
	Subtable *sub;
	int iNext = t->subtable.array[iLanguage].span.language;

	/* Count features in this language system span: */
	nFeatures = 0;
	for (i = iLanguage; i < iNext; i = t->subtable.array[i].span.feature) {
		nFeatures++;
	}

	/* Fill record */
	sys->LookupOrder = NULL_OFFSET;
	sys->ReqFeatureIndex = 0xffff;  /* xxx unsuported at present */
	sys->FeatureCount = nFeatures;
	sys->FeatureIndex = MEM_NEW(g, sizeof(unsigned short) * nFeatures);

	j = 0;
	for (i = iLanguage; i < iNext; i = sub->span.feature) {
		sub = &t->subtable.array[i];
		sys->FeatureIndex[j++] = sub->index.feature;
	}

	return LANG_SYS_SIZE(nFeatures);
}

/* Fill ScriptList */
static Offset fillScriptList(hotCtx g, otlTbl t) {
	int i;
	int nScripts;
	int iScript;
	Offset oScriptList;
	/* This works becuase prepScript has sorted the subtables so that anon subtabes are last, preceded by Stand-Alone subtables */
	int spanLimit = t->subtable.cnt - (t->nAnonSubtables + t->nStandAloneSubtables);

	/* Count scripts */
	nScripts = 0;
	for (i = 0; i < spanLimit; i = t->subtable.array[i].span.script) {
		nScripts++;
	}

	/* Allocate scripts */
	t->tbl.ScriptList_.ScriptCount = nScripts;
	t->tbl.ScriptList_.ScriptRecord =
	    MEM_NEW(g, sizeof(ScriptRecord) * nScripts);

	oScriptList = SCRIPT_LIST_SIZE(nScripts);

	/* Build table */
	iScript = 0;
	for (i = 0; i < nScripts; i++) {
		int j;
		int iLanguage;
		int nLanguages;
		Offset oScript;
		Subtable *sub = &t->subtable.array[iScript];
		ScriptRecord *rec = &t->tbl.ScriptList_.ScriptRecord[i];
		Script *script = &rec->Script_;

		/* Fill ScriptRecord */
		rec->ScriptTag = sub->script;
		rec->Script = oScriptList;

		if (sub->language == dflt_) {
			/* Fill default language system record */
			script->DefaultLangSys =
			    fillLangSysRecord(g, t, &script->DefaultLangSys_, iScript);
			iLanguage = sub->span.language;
		}
		else {
			script->DefaultLangSys = NULL_OFFSET;
			iLanguage = iScript;
		}

		/* Count languages for this script */
		nLanguages = 0;
		for (j = iLanguage; j < sub->span.script;
		     j = t->subtable.array[j].span.language) {
			nLanguages++;
		}

		/* Allocate Languages */
		script->LangSysCount = nLanguages;
		script->LangSysRecord = (nLanguages == 0) ?
		    NULL : MEM_NEW(g, sizeof(LangSysRecord) * nLanguages);

		oScript = SCRIPT_SIZE(nLanguages);

		if (script->DefaultLangSys != NULL_OFFSET) {
			Offset tmp = script->DefaultLangSys;
			script->DefaultLangSys = oScript;
			oScript += tmp;
		}

		/* Fill languages system records */
		for (j = 0; j < nLanguages; j++) {
			Subtable *sub = &t->subtable.array[iLanguage];
			LangSysRecord *rec = &script->LangSysRecord[j];

			/* Fill langSysRecord */
			rec->LangSysTag = sub->language;
			rec->LangSys = oScript;

			oScript += fillLangSysRecord(g, t,
			                             &script->LangSysRecord[j].LangSys_,
			                             iLanguage);

			/* Advance to next language */
			iLanguage = sub->span.language;
		}

		/* Advance to next script */
		oScriptList += oScript;
		iScript = sub->span.script;
	}

	return oScriptList;
}

/* Sort; span by feature index and lookup index. Caution: overwrites
   span.feature from prepScriptList().
 */
static void prepFeatureList(hotCtx g, otlTbl t) {
	int fea;
	int Fea;
	int spanLimit = t->subtable.cnt - t->nAnonSubtables;

	qsort(t->subtable.array, t->subtable.cnt, sizeof(Subtable), cmpFeatureList);

	Fea = 0;
	for (fea = 1; fea <= spanLimit; fea++) {
		/* This logic steps through the  t->subtable.array.
		   Whenever it encorunters a new feature subtable.array[fea].index.feature index,
		   it stores the current subtable index in the first subtale of the sequence of subtables that
		   had the previous subtable index. The array is this divided into sequences of subtables with the
		   same index.feature, and the span.featur of the first subtable in the index gives the index of the first
		   subtable in the next run.
		   The same is then done for sequences of subtables with the same index.lookup
		   within the previous sequence of subtables with the same same index.feature. */
		if (fea == spanLimit || t->subtable.array[fea].index.feature !=
		    t->subtable.array[Fea].index.feature) {
			/* Feature index change */
			int loo;
			int Loo = Fea;

			t->subtable.array[Fea].span.feature = fea;

			for (loo = Loo + 1; loo <= fea; loo++) {
				if (loo == fea || t->subtable.array[loo].index.lookup !=
				    t->subtable.array[Loo].index.lookup) {
					/* Lookup index change */
					t->subtable.array[Loo].span.lookup = loo;
					Loo = loo;
				}
			}
			Fea = fea;
		}
	}
}

/* Sort; span by lookup index */
static void prepLookupList(hotCtx g, otlTbl t) {
	int loo;
	int Loo;
	int spanLimit = t->subtable.cnt - (t->nRefLookups + t->nFeatParams);

	ctuQSort(t->subtable.array, t->subtable.cnt, sizeof(Subtable),
	         cmpLookupList, &g);
	Loo = 0;
	for (loo = 1; loo <= spanLimit; loo++) {
		if (loo == spanLimit || t->subtable.array[loo].index.lookup !=
		    t->subtable.array[Loo].index.lookup) {
			/* Lookup index change */
			t->subtable.array[Loo].span.lookup = loo;
			Loo = loo;
		}
	}
}

/* Fill FeatureList */
static Offset findFeatParamOffset(Tag featTag, Label featLabel, Subtable *subArray, int numSubtables) {
	int i;
	Subtable *sub;
	Offset value = 0;
	Label matchlabel = (Label)(featLabel & ~REF_LAB);

	for (i = 0; i < numSubtables; i++) {
		sub = &subArray[i];
		if ((sub->feature == featTag) && (sub->label == matchlabel)) {
			value = sub->offset;
			break;
		}
	}

	return value;
}

static Offset fillFeatureList(hotCtx g, otlTbl t) {
	int i;
	int nFeatures;
	int iFeature;
	Offset oFeatureList;
	/* This works becuase prepFeature has sorted the subtables so that anon subtabes are last, preceded by Stand-Alone subtables */
	int spanLimit = t->subtable.cnt - (t->nAnonSubtables + t->nStandAloneSubtables);

	nFeatures = t->subtable.array[spanLimit - 1].index.feature + 1;
	/* Allocate features */
	t->tbl.FeatureList_.FeatureCount = nFeatures;
	t->tbl.FeatureList_.FeatureRecord =
	    MEM_NEW(g, sizeof(FeatureRecord) * nFeatures);

	oFeatureList = FEATURE_LIST_SIZE(nFeatures);

	/* Build table */
	iFeature = 0;
	for (i = 0; i < nFeatures; i++) {
		int j;
		int k;
		int nLookups;
		int nParamSubtables;
		Offset nParamOffset;
		Subtable *sub = &t->subtable.array[iFeature];
		FeatureRecord *rec = &t->tbl.FeatureList_.FeatureRecord[i];
		Feature *feature = &rec->Feature_;
		/*if ( sub->feature == (Tag)TAG_STAND_ALONE)
		    continue;
		 */
		/* Fill FeatureRecord */
		rec->FeatureTag = sub->feature;
		rec->Feature = oFeatureList;

		/* Count lookups for this feature. These will include references. */
		nLookups = 0;
		nParamSubtables = 0;
		nParamOffset = 0;
		/* sub is is the first subtable in a run of subtables with the same feature table index.
		   sub->span.feature is the array index for the first subtable with a different feature table index.
		   This field is NOT set in any of the other subtables in the current run.
		   Within the current run, the first subtable of a sequence with the same lookup table index
		   span.lookup set to the first subtable of the nexte sequence with a different lookup index or deature index.
		 */
		for (j = iFeature; j < sub->span.feature; j = t->subtable.array[j].span.lookup) {
			Subtable *lsub = &t->subtable.array[j];

			if (lsub->isFeatParam) {
				nParamSubtables++;     /* There ought to be only one of these! */
				nParamOffset = lsub->offset;      /* Note! this is only the offset from the start of the subtable block that follows the lookupList */
				if (IS_REF_LAB(lsub->label)) {
					nParamOffset = findFeatParamOffset(lsub->feature, lsub->label, t->subtable.array, spanLimit);
				}
			}
			else {
				nLookups++;
			}
		}

		/* Allocate Lookups */
		feature->LookupCount = nLookups;

		if (nLookups > 0) {
			feature->LookupListIndex =
			    MEM_NEW(g, sizeof(unsigned short) * nLookups);

			/* Fill lookup list */
			k = 0;
			for (j = iFeature; j < sub->span.feature;
			     j = t->subtable.array[j].span.lookup) {
				if (!t->subtable.array[j].isFeatParam) {
					feature->LookupListIndex[k++] = t->subtable.array[j].index.lookup;
				}
			}
		}
		else {
			feature->LookupListIndex = (void *)0xFFFFFFFF;
		}
		oFeatureList += FEATURE_SIZE(nLookups);

		/* Add feature parameter */
		if (nParamSubtables > 0) {
			if (nParamSubtables > 1) {
				hotMsg(g, hotFATAL, "GPOS feature '%c%c%c%c' has more than one FeatureParameter subtable! ",
				       TAG_ARG(rec->FeatureTag));
			}

			if (nParamOffset == 0) {
				/* This happens when the 'size' feature is the first GPOS feature in the feature file.*/
				nParamOffset = FEAT_PARAM_ZERO; /* So we can tell the difference between 'undefined' and a real zero value.*/
			}
			feature->FeatureParams = nParamOffset; /* Note! this is only the offset from the start of the subtable block that follows the lookupList */
		}
		else {
			feature->FeatureParams = NULL_OFFSET;
		}

		/* Advance to next feature */
		iFeature = sub->span.feature;
	}

	return oFeatureList;
}

static void fixFeatureParmOffsets(hotCtx g, otlTbl t, short shortfeatureParamBaseOffset) {
	int nFeatures = t->tbl.FeatureList_.FeatureCount;
	int i;

	for (i = 0; i < nFeatures; i++) {
		FeatureRecord *rec = &t->tbl.FeatureList_.FeatureRecord[i];
		Feature *feature = &rec->Feature_;
		if (feature->FeatureParams !=  0) {
			/* Undo fix so we can tell the diff between feature->FeatureParam==NULL_OFFSET, and a real 0 offset from the
			   start of the subtable array.*/
			if (feature->FeatureParams == FEAT_PARAM_ZERO) {
				feature->FeatureParams = 0;
			}
			/* feature->FeatureParams is now: (offset from start of subtable block that follows the LookupList)

			   shortfeatureParamBaseOffset is (size of featureList) + (size of LookupList).

			 */
			feature->FeatureParams = feature->FeatureParams + shortfeatureParamBaseOffset - rec->Feature;
		}
	}
}

/* Fill LookupList */
static Offset fillLookupList(hotCtx g, otlTbl t) {
	int i;
	Subtable *sub;
	int nLookups = 0;
	Offset oLookupList;
	int lkpInx;
	int spanLimit = t->subtable.cnt - (t->nRefLookups + t->nFeatParams);

	/* Count lookups */
	if (spanLimit > 0) {
		/*can  have 0 lookups when there is only a GPOS 'size' feature, and no other features.*/
		nLookups = t->subtable.array[spanLimit - 1].index.lookup + 1;
	}

	DF(2, (stderr, ">OTL: %d lookup%s allocated (%d ref%s skipped)\n",
	       nLookups, nLookups == 1 ? "" : "s",
	       t->nRefLookups, t->nRefLookups == 1 ? "" : "s"));

	/* Allocate lookups */
	t->tbl.LookupList_.LookupCount = nLookups;
	t->tbl.LookupList_.Lookup = MEM_NEW(g, sizeof(Offset) * nLookups);
	t->tbl.LookupList_.Lookup_ = MEM_NEW(g, sizeof(Lookup) * nLookups);

	oLookupList = LOOKUP_LIST_SIZE(nLookups);

	/* Build table */
	lkpInx = 0;
	for (i = 0; i < spanLimit; i = sub->span.lookup) {
		Lookup *lookup;
		int nSubtables;
		int j;

		sub = &t->subtable.array[i];
		lookup = &t->tbl.LookupList_.Lookup_[lkpInx];
		nSubtables = sub->span.lookup - i;

		t->tbl.LookupList_.Lookup[lkpInx] = oLookupList;

		lookup->LookupType = (unsigned short)(sub->lookup >> 16);
		lookup->LookupFlag = (unsigned short)(sub->lookup & 0xffff);
		/* Allocate subtables */
		lookup->SubTableCount = nSubtables;
		lookup->SubTable = MEM_NEW(g, sizeof(long) * nSubtables);

		/* Fill subtable array. Offsets may be -ve; will be adjusted later. */
		for (j = 0; j < nSubtables; j++) {
			lookup->SubTable[j] = (sub + j)->offset - oLookupList;
		}

		oLookupList += LOOKUP_SIZE(nSubtables);
		if (lookup->LookupFlag & otlUseMarkFilteringSet) {
			lookup->UseMarkSetIndex = sub->markSetIndex;
			oLookupList += sizeof(lookup->UseMarkSetIndex);
		}
		lkpInx++;
	}

	return oLookupList;
}

void checkStandAloneTablRefs(hotCtx g, otlTbl t) {
	int i;
	/* Now go back through all the real subtables, and check that the stand-alone ones have been referenced. */
	ctuQSort(t->label.array, t->label.cnt, sizeof(LabelInfo), cmpLabelsByLookup, &g);
	for (i = 0; i < (t->subtable.cnt - (t->nAnonSubtables + t->nRefLookups)); i++) {
		Subtable *sub = &t->subtable.array[i];
		if (sub->seenInFeature != 0) {
			continue;
		}
		/* This is a stand-alone lookup */
		sub->seenInFeature = findSeenRef(g, t, sub->index.lookup);
		if (sub->seenInFeature == 0) {
			hotMsg(g, hotWARNING, "Stand-alone lookup with Lookup Index %d was not referenced from within any feature, and will never be used.", sub->index.lookup);
		}
	}
}

void otlTableFill(hotCtx g, otlTbl t) {
	Offset offset = HEADER_SIZE;

	calcLookupListIndexes(g, t);
	calcFeatureListIndexes(g, t);

	t->tbl.Version = VERSION(1, 0);

	prepScriptList(g, t);
	t->tbl.ScriptList = offset;
	offset += fillScriptList(g, t);

#if HOT_DEBUG
	if (DF_LEVEL >= 1) {
		otlDumpSubtables(g, t);
	}
#endif

	prepFeatureList(g, t);
	t->tbl.FeatureList = offset;
	offset += fillFeatureList(g, t);

	prepLookupList(g, t);
	t->tbl.LookupList = offset;
	t->lookupSize = fillLookupList(g, t);

	if (t->nFeatParams > 0) {
		/* The feature table FeatureParam offsets are currently from the start of the subtable block.*/
		/* featureParamBaseOffset is the (size of the feature list + feature record array) + size of the lookup list. */
		short featureParamBaseOffset = (t->tbl.LookupList - t->tbl.FeatureList) + t->lookupSize;
		fixFeatureParmOffsets(g, t, featureParamBaseOffset);
	}
}

void otlTableFillStub(hotCtx g, otlTbl t) {
	Header *hdr = &t->tbl;

	t->tbl.Version = VERSION(1, 0);

	t->tbl.ScriptList = 0;
	t->tbl.FeatureList = 0;
	t->tbl.LookupList = 0;

	hdr->ScriptList_.ScriptCount = 0;
	hdr->FeatureList_.FeatureCount = 0;
	hdr->LookupList_.LookupCount = 0;
}

/* Write language system record */
static void writeLangSys(otlCtx h, LangSys *rec) {
	int i;

	OUT2(rec->LookupOrder);
	OUT2(rec->ReqFeatureIndex);
	OUT2(rec->FeatureCount);

	for (i = 0; i < rec->FeatureCount; i++) {
		OUT2(rec->FeatureIndex[i]);
	}
}

void otlTableWrite(hotCtx g, otlTbl t) {
	int i;
	int j;
	otlCtx h = g->ctx.otl;
	Header *hdr = &t->tbl;

	/* Write header */
	OUT4(hdr->Version);
	OUT2(hdr->ScriptList);
	OUT2(hdr->FeatureList);
	OUT2(hdr->LookupList);

	if (hdr->ScriptList == 0) {
		return;
	}

	/* Write script list */
	OUT2(hdr->ScriptList_.ScriptCount);
	for (i = 0; i < hdr->ScriptList_.ScriptCount; i++) {
		ScriptRecord *rec = &hdr->ScriptList_.ScriptRecord[i];

		OUT4(rec->ScriptTag);
		OUT2(rec->Script);
	}
	for (i = 0; i < hdr->ScriptList_.ScriptCount; i++) {
		Script *script = &hdr->ScriptList_.ScriptRecord[i].Script_;

		OUT2(script->DefaultLangSys);
		OUT2(script->LangSysCount);

		for (j = 0; j < script->LangSysCount; j++) {
			LangSysRecord *rec = &script->LangSysRecord[j];

			OUT4(rec->LangSysTag);
			OUT2(rec->LangSys);
		}

		if (script->DefaultLangSys != 0) {
			writeLangSys(h, &script->DefaultLangSys_);
		}

		for (j = 0; j < script->LangSysCount; j++) {
			writeLangSys(h, &script->LangSysRecord[j].LangSys_);
		}
	}

	/* Write feature list */
	OUT2(hdr->FeatureList_.FeatureCount);
	for (i = 0; i < hdr->FeatureList_.FeatureCount; i++) {
		FeatureRecord *rec = &hdr->FeatureList_.FeatureRecord[i];

		OUT4(rec->FeatureTag);
		OUT2(rec->Feature);
	}
	for (i = 0; i < hdr->FeatureList_.FeatureCount; i++) {
		Feature *feature = &hdr->FeatureList_.FeatureRecord[i].Feature_;

		OUT2(feature->FeatureParams);
		OUT2(feature->LookupCount);

		for (j = 0; j < feature->LookupCount; j++) {
			OUT2(feature->LookupListIndex[j]);
		}
	}


	/* Write lookup list */
	OUT2(hdr->LookupList_.LookupCount);
	for (i = 0; i < hdr->LookupList_.LookupCount; i++) {
		OUT2(hdr->LookupList_.Lookup[i]);
	}
	for (i = 0; i < hdr->LookupList_.LookupCount; i++) {
		Lookup *lookup = &hdr->LookupList_.Lookup_[i];

		OUT2(lookup->LookupType);
		OUT2(lookup->LookupFlag);
		OUT2(lookup->SubTableCount);
		for (j = 0; j < lookup->SubTableCount; j++) {
			OUT2((short)(lookup->SubTable[j] + t->lookupSize));
		}
		if (lookup->LookupFlag & otlUseMarkFilteringSet) {
			OUT2(lookup->UseMarkSetIndex);
		}
	}
}

/* Free language system record */
static void freeLangSys(hotCtx g, LangSys *rec) {
	MEM_FREE(g, rec->FeatureIndex);
}

static void freeTable(hotCtx g, otlTbl t) {
	int i;
	int j;
	Header *hdr = &t->tbl;

	/* Free script list */
	for (i = 0; i < hdr->ScriptList_.ScriptCount; i++) {
		Script *script = &hdr->ScriptList_.ScriptRecord[i].Script_;

		if (script->DefaultLangSys != 0) {
			freeLangSys(g, &script->DefaultLangSys_);
		}

		for (j = 0; j < script->LangSysCount; j++) {
			freeLangSys(g, &script->LangSysRecord[j].LangSys_);
		}

		script->LangSysCount = 0;
		MEM_FREE(g, script->LangSysRecord);
	}
	MEM_FREE(g, hdr->ScriptList_.ScriptRecord);

	/* Free feature list */
	for (i = 0; i < hdr->FeatureList_.FeatureCount; i++) {
		Feature *feature = &hdr->FeatureList_.FeatureRecord[i].Feature_;
		if (feature->LookupCount > 0) {
			MEM_FREE(g, feature->LookupListIndex);
		}
	}
	MEM_FREE(g, hdr->FeatureList_.FeatureRecord);

	/* Free lookup list */
	for (i = 0; i < hdr->LookupList_.LookupCount; i++) {
		Lookup *lookup = &hdr->LookupList_.Lookup_[i];

		MEM_FREE(g, lookup->SubTable);
	}
	MEM_FREE(g, hdr->LookupList_.Lookup);
	MEM_FREE(g, hdr->LookupList_.Lookup_);
}

void otlTableReuse(hotCtx g, otlTbl t) {
	if (t->subtable.cnt != 0) {
		freeTable(g, t);
	}
	t->subtable.cnt = 0;
	t->label.cnt = 0;
	t->refLabel.cnt = 0;

#if HOT_DEBUG
	t->subtable.func = initSubtable;
	t->nCoverageReused = 0;
	t->nClassReused = 0;
#endif
	coverageReuse(g, t);
	classReuse(g, t);
	t->nStandAloneSubtables = t->nAnonSubtables = t->nRefLookups = t->nFeatParams = 0;
}

void otlTableFree(hotCtx g, otlTbl t) {
	if (t == NULL) {
		return;
	}
	dnaFREE(t->subtable);
	dnaFREE(t->label);
	dnaFREE(t->refLabel);
	coverageFree(g, t);
	classFree(g, t);
	MEM_FREE(g, t);
}

/* Add subtable to list. Assumes label has been set for intended granularity of
   lookups (i.e. all subtables of the same lookup should have the same
   label). */
void otlSubtableAdd(hotCtx g, otlTbl t, Tag script, Tag language, Tag feature,
                    int lkpType, int lkpFlag, unsigned short markSetIndex, unsigned extensionLookupType,
                    unsigned offset, Label label, unsigned short fmt,
                    int isFeatParam) {
	Subtable *sub = dnaNEXT(t->subtable);

	sub->script = script;
	sub->language = language;
	sub->feature = feature;
	sub->lookup = (unsigned long)lkpType << 16 | lkpFlag;
	sub->markSetIndex = markSetIndex;
	sub->extensionLookupType = extensionLookupType;
	sub->offset = offset;
	sub->label = label;
	sub->fmt = fmt;
	sub->isFeatParam = isFeatParam;
	if (feature == (Tag)TAG_STAND_ALONE) {
		sub->seenInFeature = 0;
	}
	else {
		sub->seenInFeature = 1;
	}

	if (script == TAG_UNDEF) {
		t->nAnonSubtables++;
	}
	if (feature == (Tag)TAG_STAND_ALONE) {
		t->nStandAloneSubtables++;
	}

	/* FeatParam subtables may be labelled, but should NOT be added
	   to the list of real look ups. */
	if (IS_REF_LAB(label)) {
		t->nRefLookups++;
	}
	else if (isFeatParam) {
		t->nFeatParams++;
	}
}