/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 *	handles feat file
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include "map.h"
#include "otl.h"
#include "GPOS.h"
#include "GSUB.h"
#include "GDEF.h"
#include "BASE.h"
#include "name.h"

#define MAX_NUM_LEN 3   /* For glyph ranges */
#define  MAX_PATH  1024

/* Pattern is one node, not a class (i.e. excludes singleton class). Note:
   singleton class rejected only if FEAT_GCLASS has been properly set. */
#define IS_GLYPH(p) ((p) != NULL && (p)->nextSeq == NULL && \
	                 (p)->nextCl == NULL && !((p)->flags & FEAT_GCLASS))

/* Pattern is one class (may be singleton). Note: singleton class accepted only
   if FEAT_GCLASS has been properly set. */
#define IS_CLASS(p) ((p) != NULL && (p)->nextSeq == NULL && \
	                 ((p)->nextCl != NULL || (p)->flags & FEAT_GCLASS))

/* Pattern is one class that contains more than one node (FEAT_GCLASS is not
   checked, so it need not be properly set */
#define IS_MULT_CLASS(p) ((p) != NULL && (p)->nextSeq == NULL && \
	                      (p)->nextCl != NULL)

/* Pattern IS_GLYPH and unmarked */
#define IS_UNMARKED_GLYPH(p) (IS_GLYPH(p) && !((p)->flags & FEAT_HAS_MARKED))

/* Pattern IS_CLASS and unmarked */
#define IS_UNMARKED_CLASS(p) (IS_CLASS(p) && !((p)->flags & FEAT_HAS_MARKED))

#define USE_LANGSYS_MSG \
	"use \"languagesystem\" statement(s) at beginning of file instead to" \
	" specify the language system(s) this feature should be registered under"

#define kLenUnicodeList 128 /* number of possible entries in list of Unicode blocks. */
#define kLenCodePageList 64 /* number of possible entries in list of code  page numbers. */
#define kMaxCodePageValue 32
#define kCodePageUnSet -1

static void addGPOS(int lkpType, GNode *targ, char *fileName, long lineNum, int anchorCount, AnchorMarkInfo *anchorMarkInfo);
static GNode **gcOpen(char *gcname);
static GNode *gcLookup(char *gcName);
GNode **featGlyphClassCopy(hotCtx g, GNode **dst, GNode *src);


/* --------------------------- Context Definition -------------------------- */

#if HOT_FEAT_SUPPORT

enum {
	featureTag = 1,     /* Tag type */
	scriptTag,
	languageTag,
	tableTag
};

#define MAX_INCL 5
#define INCL_CNT        (h->stack.cnt)
#define INCL            (h->stack.array[INCL_CNT - 1])

typedef struct {
	char *file;
	int lineno;
	long offset;
} InclFile;

/* --- Rules accumulator --- */

typedef struct {
	GNode *targ;
	GNode *repl;
	unsigned lkpFlag;   /* xxx not used for aalt */
	Label label;        /* xxx not used for aalt */
} RuleInfo;

/* --- Hash table support --- */


typedef struct _element {
	char *name;
	struct _element *next;
	union {
#if 0
		int type;
#endif
		GNode *head;
	} value;
} HashElement;

#define HASH_SIZE 2048

typedef struct {
	Tag script;
	Tag lang;
	unsigned int excludeDflt;
} LangSys;

typedef dnaDCL (Tag, TagArray);
typedef dnaDCL (LangSys, LangSysArray);

typedef struct {
	Tag script;
	Tag language;
	Tag feature;
	Tag tbl;                /* GSUB_ or GPOS_ */
	int lkpType;            /* GSUBSingle, ... GPOSSingle, ... */
	unsigned lkpFlag;
	unsigned short markSetIndex;
	Label label;
} State;

typedef struct {
	Tag tbl;                /* GSUB_ or GPOS_ */
	int lkpType;            /* GSUBSingle, ... GPOSSingle, ... */
	unsigned lkpFlag;
	unsigned short markSetIndex;
	Label label;
	short useExtension;    /* Create this lookup with extension lookupTypes? */
} LookupInfo;

typedef struct {
	char *name;             /* User's descriptive name */
	State state;            /* All rules in the block must have same State */
	short useExtension;    /* Create this lookup with extension lookupTypes? */
} NamedLkp;
#endif /* HOT_FEAT_SUPPORT */

/* --- Glyph Node support --- */



#define AALT_INDEX -1 /* used as feature index for sorting alt glyphs in rule for aalt feature. */
typedef struct BlockNode_ BlockNode;
struct BlockNode_ {
	GNode *data;
	BlockNode *next;
};

typedef struct {
	BlockNode *first;
	BlockNode *curr;    /* Current BlockNode being filled */
	long cnt;           /* Index of next free GNode, relative to curr->data */
	long intl;
	long incr;
} BlockList;

/* ---Named Value support --- */
typedef struct {
	char anchorName[64];
	short int x;
	short int y;
	unsigned int contourpoint;
	int hasContour;
} AnchorDef;

typedef struct {
	char valueName[64];
	short metrics[4];
} ValueDef;


struct featCtx_ {
	/* --- Glyph node management --- */
	GNode *freelist;
	BlockList blockList;        /* Main storage for all Nodes */
	dnaDCL(GNode *, sortTmp);   /* Tmp for sorting a list */
#if HOT_DEBUG
	long int nAdded2FreeList;
	long int nNewFromBlockList;
	long int nNewFromFreeList;
#endif

#if HOT_FEAT_SUPPORT
	int featFileExists;

	/* --- File refill management --- */
	long offset;            /* Tracks offset in current feature file */
	long nextoffset;        /* if == offset, then refill buffer */
	char *data;             /* Points to data in client's buffer */
	dnaDCL(InclFile, stack);

	/* --- State information --- */
	State curr;
	State prev;

	short gFlags;           /* Global flags: apply to entire feature file: */
#define GF_SEEN_FEATURE (1 << 0) /* A feature block has been seen */
#define GF_SEEN_LANGSYS (1 << 1) /* A languagesystem keyword has been seen */
#define GF_SEEN_GDEF_GLYPHCLASS (1 << 2) /* An explicit GDEF glyph class has been seen. */
#define GF_SEEN_IGNORE_CLASS_FLAG (1 << 3) /* any lookup flag has been seen for ignoring any GDEF class. */
#define GF_SEEN_MARK_CLASS_FLAG (1 << 4) /* any lookup flag has been seen for ignoring any GDEF class. */

	short fFlags;           /* Feature flags: set to 0 at every feat start: */
#define FF_SEEN_SCRLANG (1 << 0) /* A script or language keyword has been seen */
#define FF_LANGSYS_MODE (1 << 1) /* When this is present, register lookups under all languagesystems */

	LangSysArray langSys;   /* global language systems; used in langsys mode */
	dnaDCL(LookupInfo, lookup); /* Stores default feature-level lookups for langsys mode */

	TagArray script;        /* All script tags for current feature */
	TagArray language;      /* All language tags for current script */
	TagArray feature;       /* All feature tags in file */
	TagArray table;         /* All table tags in file */
	dnaDCL(NamedLkp, namedLkp); /* Stores state info for named lkps */
	dnaDCL(State, DFLTLkps);    /* Store DFLT lkp states for the current script for inclusion in
	                               language-specific situations */

	int include_dflt;       /* For current non-DFLT language */
	int seenOldDFLT;        /*To allowing warning only once of use of 'DFLT' lang tag instead of 'dflt' */
	int endOfNamedLkpOrRef; /* Prev rule was end of a named lkp block or ref */
	Label currNamedLkp;     /* Indicates within a named lkp block or ref. */
	Label namedLkpLabelCnt; /* Counts named lookup labels */
	Label anonLabelCnt;     /* Counts anon labels */

	struct {
		State state;
		short useExtension;      /* Create entire aalt with extension
		                            lookupTypes? */
		TagArray features;       /* to be included in 'aalt' creation */
		dnaDCL(int, usedFeatures); /*Incremented in tandem with aalt.features and set to 0,
		                              it is set to 1 when the feature tag is used, so we can detect tags that don't exist */
		dnaDCL(RuleInfo, rules); /* Rules accumulator */
	}
	aalt;

    struct {
            unsigned short Format;
            unsigned short FeatUILabelNameID;
            unsigned short FeatUITooltipTextNameID;
            unsigned short SampleTextNameID;
            unsigned short NumNamedParameters;
            unsigned short FirstParamUILabelNameID;
            dnaDCL(unsigned long, charValues);
    } cvParameters;
    
	/* --- Hash stuff --- */
	HashElement *ht[HASH_SIZE]; /* Hash table */
	HashElement *he;            /* Current hash element */

	GNode **gcInsert;           /* Insertion pt in current named/anon gl.cl. */

	/* --- Anonymous data in feature file --- */
	struct {
		Tag tag;
		int iLineStart;
		dnaDCL(char, data);
	}
	anonData;

	/* --- Stores stats on deprecated feature file syntax */
	struct {
		unsigned short numExcept;
	}
	syntax;

	dnaDCL(GNode *, markClasses);   /* Container for head nodes or mark classes */
	dnaDCL(AnchorDef, anchorDefs);  /* List of named anchors, sorted by name */
	dnaDCL(ValueDef, valueDefs);        /* List of named value records, sorted by name */
	GNode *ligGlyphs;       /* Container for the ligature glyph  class */

	/* --- Temp and misc stuff --- */
	dnaDCL(char, nameString);       /* Tmp for a nameid string */
	dnaDCL(MetricsInfo, metricsInfo);           /* Tmp for a pos rule's metrics */
	dnaDCL(AnchorMarkInfo, anchorMarkInfo);         /* Tmp for a pos rule's anchor and mark info */
	short mmMetric[TX_MAX_MASTERS]; /* Tmp for a mm metric */
	struct {
		/* PCCTS mode to return to */
		int include;
		int tag;
	}
	returnMode;
	dnaDCL(GNode *, prod);          /* Tmp for cross product */
	int linenum;                    /* Tmp feature file line number */
	unsigned short featNameID;
#endif /* HOT_FEAT_SUPPORT */

	hotCtx g;                   /* Package context */
};

#if HOT_FEAT_SUPPORT

static void prepRule(Tag newTbl, int newlkpType, GNode *targ, GNode *repl);

enum {
    cvUILabelEnum =1,
    cvToolTipEnum,
    cvSampletextEnum,
    kCVParameterLabelEnum
};


#endif /* HOT_FEAT_SUPPORT */

int setVendId_str(hotCtx g, char *vend);

void featSetUnicodeRange(hotCtx g, short unicodeList[kLenUnicodeList]);

void featSetCodePageRange(hotCtx g, short [kLenCodePageList]);


/* -------------------------- dump functions ------------------------------- */

/* If print == 0, add to g->notes */

void featGlyphDump(hotCtx g, GID gid, int ch, int print) {
	char msg[64];
	int len;
	if (IS_CID(g)) {
		sprintf(msg, "\\%hd", mapGID2CID(gid));
	}
	else {
		sprintf(msg, "%s", mapGID2Name(gid));
	}
	len = strlen(msg);
	if (ch >= 0) {
		msg[len++] = ch;
	}

	if (print) {
		msg[len++] = '\0';
		fprintf(stderr, "%s", msg);
	}
	else {
		strncpy(dnaEXTEND(g->note, len), msg, len);
	}
}

#define DUMP_CH(ch, print) do { if (print) fprintf(stderr, "%c", ch); \
								else *dnaNEXT(g->note) = (ch); } while(0)

/* If !print, add to g->notes */

void featGlyphClassDump(hotCtx g, GNode *gc, int ch, int print) {
	GNode *p = gc;

	DUMP_CH('[', print);
	for (; p != NULL; p = p->nextCl) {
		featGlyphDump(g, p->gid, (char)(p->nextCl != NULL ? ' ' : -1), print);
	}
	DUMP_CH(']', print);
	if (ch >= 0) {
		DUMP_CH(ch, print);
	}
}

/* If !print, add to g->notes */

void featPatternDump(hotCtx g, GNode *pat, int ch, int print) {
	DUMP_CH('{', print);
	for (; pat != NULL; pat = pat->nextSeq) {
		if (pat->nextCl == NULL) {
			featGlyphDump(g, pat->gid, -1, print);
		}
		else {
			featGlyphClassDump(g, pat, -1, print);
		}
		if (pat->flags & FEAT_MARKED) {
			DUMP_CH('\'', print);
		}
		if (pat->nextSeq != NULL) {
			DUMP_CH(' ', print);
		}
	}
	DUMP_CH('}', print);
	if (ch >= 0) {
		DUMP_CH(ch, print);
	}
}

#if HOT_DEBUG

static void nodeStats(featCtx h) {
	BlockNode *p;
	BlockList *bl = &h->blockList;

	fprintf(stderr, "### GNode Stats\n"
	        "nAdded2FreeList: %ld, "
	        "nNewFromBlockList: %ld, "
	        "nNewFromFreeList: %ld.\n",
	        h->nAdded2FreeList, h->nNewFromBlockList, h->nNewFromFreeList);
	fprintf(stderr, "%ld not freed\n",
	        h->nNewFromBlockList + h->nNewFromFreeList - h->nAdded2FreeList);

	fprintf(stderr, "### BlockList:");

	for (p = bl->first; p != NULL; p = p->next) {
		long blSize = p == bl->first ? bl->intl : bl->incr;
		if (p->next != NULL) {
			fprintf(stderr, " %ld ->", blSize);
		}
		else {
			fprintf(stderr, " %ld/%ld\n", bl->cnt, blSize);
		}
	}
}

#if HOT_FEAT_SUPPORT

static void hashStats(featCtx h) {
	unsigned i;
	fprintf(stderr, "### hashStats\n");
	for (i = 0; i < HASH_SIZE; i++) {
		HashElement *j;
		for (j = h->ht[i]; j != NULL; j = j->next) {
			fprintf(stderr, "[%4u] @%s = ", i, j->name);
			featGlyphClassDump(g, j->value.head, '\n', 1);
		}
	}
}

static void tagDump(Tag tag) {
	if (tag == TAG_UNDEF) {
		fprintf(stderr, "****");
	}
	else {
		fprintf(stderr, "%c%c%c%c", TAG_ARG(tag));
	}
}

static void stateDump(State *st) {
	fprintf(stderr, "scr='");
	tagDump(st->script);
	fprintf(stderr, "' lan='");
	tagDump(st->language);
	fprintf(stderr, "' fea='");
	tagDump(st->feature);
	fprintf(stderr, "' tbl='");
	tagDump(st->tbl);
	fprintf(stderr, "' lkpTyp=%d lkpFlg=%d label=%X\n",
	        st->lkpType, st->lkpFlag, st->label);
}

#endif /* HOT_FEAT_SUPPORT */
#endif /* HOT_DEBUG */

#if HOT_FEAT_SUPPORT
/* Varargs wrapper for zzerr */

static void CDECL zzerrVA(char *fmt, ...) {
	va_list ap;
	char msg[1024];

	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	va_end(ap);
	zzerr(msg);
}

/* Prints message. Message should not have newline */

static void CDECL featMsg(int msgType, char *fmt, ...) {
	va_list ap;
	char msgVar[1024];
	char msg[1024];

	va_start(ap, fmt);
	vsprintf(msgVar, fmt, ap);
	va_end(ap);
	sprintf(msg, "%s [%s %d]", msgVar, INCL.file, zzline);

	hotMsg(g, msgType, msg);
}

/* Make a copy of a string */

static void copyStr(hotCtx g, char **dst, char *src) {
	if (src == NULL) {
		*dst = NULL;
	}
	else {
		*dst = MEM_NEW(g, strlen(src) + 1);
		strcpy(*dst, src);
	}
}

/* ------------------------ Supplementary Functions ------------------------ */


void featSetIncludeReturnMode(int mode) {
	h->returnMode.include = mode;
}

int featGetIncludeReturnMode(void) {
	return h->returnMode.include;
}

void featSetTagReturnMode(int mode) {
	h->returnMode.tag = mode;
}

int featGetTagReturnMode(void) {
	return h->returnMode.tag;
}

/* Add ch to the accumulating name table override string */

void featAddNameStringChar(char ch) {
	*dnaNEXT(h->nameString) = ch;
}

/* --- Device handling --- */

static void deviceBeg(void) {
	/* xxx Todo */
}

static void deviceAddEntry(unsigned short ppem, short delta) {
	/* xxx Todo */
}

static Offset deviceEnd(void) {
	/* xxx Todo */
	return 0;
}

/* --- GDEF interface --- */

static void setGDEFGlyphClassDef(GNode *simple, GNode *ligature, GNode *mark,
                                 GNode *component) {
	h->gFlags |= GF_SEEN_GDEF_GLYPHCLASS;
	setGlyphClassGDef(g, simple, ligature, mark, component);
}

static void addGDEFAttach(GNode *pat, unsigned short contour) {
	int seenContourIndex = 0;
	GNode *next = pat;
	if (pat->nextSeq != NULL) {
		featMsg(hotERROR, "Only one glyph|glyphClass may be present per"
		        " AttachTable statement");
	}

	while (next != NULL) {
		seenContourIndex = 0;
		seenContourIndex = addAttachEntryGDEF(g, next, contour);
		if (seenContourIndex) {
			featMsg(hotWARNING, "Skipping duplicate contour index %d", contour);
		}
		next = next->nextCl;
	}
}

static void setGDEFLigatureCaret(GNode *pat, unsigned short caretValue, unsigned short format) {
	int seenCaretValue;
	GNode *next = pat;

	if (pat->nextSeq != NULL) {
		featMsg(hotERROR, "Only one glyph|glyphClass may be present per"
		        " LigatureCaret statement");
	}

	while (next != NULL) {
		seenCaretValue = 0;
		seenCaretValue = addLigCaretEntryGDEF(g, next, caretValue,  format);
		if (seenCaretValue) {
			featMsg(hotWARNING, "Skipping duplicate caret value %d", caretValue);
		}
		next = next->nextCl;
	}
}

static void creatDefaultGDEFClasses() {
	if (!(h->gFlags & GF_SEEN_GDEF_GLYPHCLASS)) {
		/* Add the GlyphClass def */
		GNode **gcInsert;

		GNode *base = NULL;
		GNode *ligature = NULL;
		GNode *marks  = NULL;
		GNode *component  = NULL;
		GNode *tempSrc;

		/* I need to call gcOpen first, as these classes may have never been instantiated,
		   in which case gcLookup would post a fatal error on the lookup. */

		/* The source glyph classes are all named classes. Named class
		   glyphs get recycled when hashFree() is called, so we need to
		   make a copy of these classes here, and recycle the copy after
		   in GDEF.c::createGlyphClassDef(). This is because
		   createGlyphClassDef deletes glyphs from within the class lists
		   to eliminate duplicates. If we operate on a named class list,
		   then any deleted duplicated glyphs gets deleted again when
		   hashFree() is called.Also, the hash element head points to the original first glyph,
		   which may be sorted further down the list. */

		gcInsert = gcOpen(kDEFAULT_BASECLASS_NAME);
		base = gcLookup(kDEFAULT_BASECLASS_NAME);
		if (base != NULL) {
			featGlyphClassCopy(h->g, &tempSrc, base);
			base = tempSrc;
			featGlyphClassSort(h->g, &base, 1, 0);
		}

		gcInsert = gcOpen(kDEFAULT_LIGATURECLASS_NAME);
		ligature = gcLookup(kDEFAULT_LIGATURECLASS_NAME);
		if (ligature != NULL) {
			featGlyphClassCopy(h->g, &tempSrc, ligature);
			ligature = tempSrc;
			featGlyphClassSort(h->g, &ligature, 1, 0);
		}

		gcInsert = gcOpen(kDEFAULT_MARKCLASS_NAME);
		marks = gcLookup(kDEFAULT_MARKCLASS_NAME);
		if (marks != NULL) {
			featGlyphClassCopy(h->g, &tempSrc, marks);
			marks = tempSrc;
			featGlyphClassSort(h->g, &marks, 1, 0);
		}

		gcInsert = gcOpen(kDEFAULT_COMPONENTCLASS_NAME);
		component = gcLookup(kDEFAULT_COMPONENTCLASS_NAME);
		if (component != NULL) {
			featGlyphClassCopy(h->g, &tempSrc, component);
			component = tempSrc;
			featGlyphClassSort(h->g, &component, 1, 0);
		}

		setGlyphClassGDef(g, base, ligature, marks, component);
	}
}

/* -------------------------- misc table functions ------------------------------- */



/* Add name string to name table. */

static void addNameString(long platformId, long platspecId,
                          long languageId, long nameId) {

    int nameError = 0;
    if ((nameId == 2) ||(nameId == 6) || ((nameId >= 25) && (nameId <= 255)))
        nameError = 1;
    else if ((nameId > 0) && ((nameId < 7) && (!(g->convertFlags & HOT_OVERRIDE_MENUNAMES))))
    {
        nameError = 1;
    }
    if (nameError)
    {
        hotMsg(g, hotWARNING,
               "name id %d cannot be set from the feature file. "
               "ignoring record [%s %d]", nameId, 
               INCL.file, h->linenum);
        return;
    }

    /* Add defaults */
	if (platformId == -1) {
		platformId = HOT_NAME_MS_PLATFORM;
	}
	if (platformId == HOT_NAME_MS_PLATFORM) {
		if (platspecId == -1) {
			platspecId = HOT_NAME_MS_UGL;
		}
		if (languageId == -1) {
			languageId = HOT_NAME_MS_ENGLISH;
		}
	}
	else if (platformId == HOT_NAME_MAC_PLATFORM) {
		if (platspecId == -1) {
			platspecId = HOT_NAME_MAC_ROMAN;
		}
		if (languageId == -1) {
			languageId = HOT_NAME_MAC_ENGLISH;
		}
	}

	/* Terminate name and add to name table */
	*dnaNEXT(h->nameString) = '\0';
	if (hotAddName(g,
	               (unsigned short)platformId, (unsigned short)platspecId,
	               (unsigned short)languageId, (unsigned short)nameId,
	               h->nameString.array)) {
		featMsg(hotERROR, "Bad string");
	}
}

/* Add 'size' feature submenuname name string to name table. */

static void addSizeNameString(long platformId, long platspecId,
                              long languageId) {
	unsigned short nameID;

	/* We only need to reserve a name ID *once*; after the first time,
	 * all subsequent sizemenunames will share the same nameID.
	 */
	if (h->featNameID == 0) {
		nameID = nameReserveUserID(g);
		GPOSSetSizeMenuNameID(g, nameID);
		h->featNameID = nameID;
	}
	else {
		nameID = h->featNameID;
	}

	addNameString(platformId, platspecId, languageId, nameID);
}

/* Add 'name for feature string to name table. */

static void addFeatureNameString(long platformId, long platspecId,
                                 long languageId) {
	unsigned short nameID;

	/* We only need to reserve a name ID *once*; after the first time,
	 * all subsequent sizemenunames will share the same nameID.
	 */
	if (h->featNameID == 0) {
		nameID = nameReserveUserID(g);
		GSUBSetFeatureNameID(g, h->curr.feature, nameID);
		h->featNameID = nameID;
	}
	else {
		nameID = h->featNameID;
	}

	addNameString(platformId, platspecId, languageId, nameID);
}


/* Add Unicode and CodePage ranges to  OS/2 table. */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
#define SET_BIT_ARR(a, b) (a[(b) / 32] |= 1UL << (b) % 32)

const short kValidCodePageNumbers[kLenCodePageList] = {
	1252,               /*  bit 0  Latin 1 */
	1250,               /*  bit 1  Latin 2: Eastern Europe */
	1251,               /*  bit 2  Cyrillic */
	1253,               /*  bit 3  Greek */
	1254,               /*  bit 4  Turkish */
	1255,               /*  bit 5  Hebrew */
	1256,               /*  bit 6  Arabic */
	1257,               /*  bit 7  Windows Baltic */
	1258,               /*  bit 8  Vietnamese */
	kCodePageUnSet,             /*  bit 9  Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 10  Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 11  Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 12  Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 13  Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 14 Reserved for Alternate ANSI */
	kCodePageUnSet,             /*  bit 15  Reserved for Alternate ANSI */
	874,                /*  bit 16  Thai */
	932,                /*  bit 17  JIS/Japan */
	936,                /*  bit 18  Chinese: Simplified chars--PRC and Singapore */
	949,                /*  bit 19  Korean Wansung */
	950,                /*  bit 20  Chinese: Traditional chars--Taiwan and Hong Kong */
	1361,               /*  bit 21  Korean Johab */
	kCodePageUnSet,             /*  bit 22-28  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 23  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 24  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 25  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 26  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 27  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 28  Reserved for Alternate ANSI & OEM */
	kCodePageUnSet,             /*  bit 29  Macintosh Character Set (US Roman) */
	kCodePageUnSet,             /*  bit 30  OEM Character Set */
	kCodePageUnSet,             /*  bit 31  Symbol Character Set */
	kCodePageUnSet,             /*  bit 32  Reserved for OEM */
	kCodePageUnSet,             /*  bit 33  Reserved for OEM */
	kCodePageUnSet,             /*  bit 34  Reserved for OEM */
	kCodePageUnSet,             /*  bit 35  Reserved for OEM */
	kCodePageUnSet,             /*  bit 36  Reserved for OEM */
	kCodePageUnSet,             /*  bit 37  Reserved for OEM */
	kCodePageUnSet,             /*  bit 38  Reserved for OEM */
	kCodePageUnSet,             /*  bit 39  Reserved for OEM */
	kCodePageUnSet,             /*  bit 40  Reserved for OEM */
	kCodePageUnSet,             /*  bit 41  Reserved for OEM */
	kCodePageUnSet,             /*  bit 42  Reserved for OEM */
	kCodePageUnSet,             /*  bit 43  Reserved for OEM */
	kCodePageUnSet,             /*  bit 44  Reserved for OEM */
	kCodePageUnSet,             /*  bit 45  Reserved for OEM */
	kCodePageUnSet,             /*  bit 46  Reserved for OEM */
	kCodePageUnSet,             /*  bit 47  Reserved for OEM */
	869,                /*  bit 48  IBM Greek */
	866,                /*  bit 49  MS-DOS Russian */
	865,                /*  bit 50  MS-DOS Nordic */
	864,                /*  bit 51  Arabic */
	863,                /*  bit 52  MS-DOS Canadian French */
	862,                /*  bit 53  Hebrew */
	861,                /*  bit 54  MS-DOS Icelandic */
	860,                /*  bit 55  MS-DOS Portuguese */
	857,                /*  bit 56  IBM Turkish */
	855,                /*  bit 57  IBM Cyrillic; primarily Russian */
	852,                /*  bit 58  Latin 2 */
	775,                /*  bit 59  MS-DOS Baltic */
	737,                /*  bit 60  Greek; former 437 G */
	708,                /*  bit 61  Arabic; ASMO 708 */
	850,                /*  bit 62  WE/Latin 1 */
	437,                /*  bit 63  US */
};

static int validateCodePage(short pageNum) {
	int i;
	int validPageBitIndex = kCodePageUnSet;

	for (i = 0; i < kLenCodePageList; i++) {
		if (pageNum  == kValidCodePageNumbers[i]) {
			validPageBitIndex = i;
			break;
		}
	}

	return validPageBitIndex;
}

void featSetUnicodeRange(hotCtx g, short unicodeList[kLenUnicodeList]) {
	unsigned long unicodeRange[4];
	short i = 0;

	unicodeRange[0] = unicodeRange[1] = unicodeRange[2] = unicodeRange[3] = 0;

	while (i < kLenUnicodeList) {
		short bitnum = unicodeList[i];

		if (bitnum != kCodePageUnSet) {
			if ((bitnum >= 0) && (bitnum < kLenUnicodeList)) {
				SET_BIT_ARR(unicodeRange, bitnum);
			}
			else {
				featMsg(hotERROR, "OS/2 Bad Unicode block value <%d>. All values must be in [0 ...127] inclusive.", bitnum);
			}
		}
		else {
			break;
		}
		i++;
	}

	OS_2SetUnicodeRanges(g, unicodeRange[0], unicodeRange[1],
	                     unicodeRange[2], unicodeRange[3]);
}

void featSetCodePageRange(hotCtx g, short codePageList[kLenCodePageList]) {
	unsigned long codePageRange[2];
	int i = 0;
	short validPageBitIndex;
	codePageRange[0] = codePageRange[1] = 0;

	while (i < kLenCodePageList) {
		short pageNumber = codePageList[i];

		if (pageNumber != kCodePageUnSet) {
			validPageBitIndex = validateCodePage(pageNumber);
			if (validPageBitIndex != kCodePageUnSet) {
				SET_BIT_ARR(codePageRange, validPageBitIndex);
			}
			else {
				featMsg(hotERROR, "OS/2 Code page value <%d> is not permitted according to the OpenType spec v1.4.", pageNumber);
			}
		}
		else {
			break;
		}

		i++;
	}

	OS_2SetCodePageRanges(g, codePageRange[0], codePageRange[1]);
}

/* Add vendor name to OS/2 table. */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
static void addVendorString(hotCtx g) {
	int tooshort = 0;

	while (h->nameString.cnt < 4) {
		*dnaNEXT(h->nameString) = ' ';
		tooshort = 1;
	}
	if (tooshort) {
		featMsg(hotWARNING, "Vendor name too short. Padded automatically to 4 characters.");
	}

	if (h->nameString.cnt > 4) {
		featMsg(hotERROR, "Vendor name too long. Max is 4 characters.");
	}

	*dnaNEXT(h->nameString) = '\0';
	if (setVendId_str(g, h->nameString.array)) {
		featMsg(hotERROR, "Bad string");
	}
}

/* Return 1 if last line was seen. isEOL indicates whether ch is the last char
   on the line (i.e. if isEOL is 1, then ch must be \r or \n). */

int featAddAnonDataChar(char ch, int isEOL) {
	char *p;
	char *q;
	int sol;
	int i;

	*dnaNEXT(h->anonData.data) = ch;

	if (!isEOL) {
		return 0;
	}

	/* If this is the end of line, then analyze the line to see whether it's
	   within the body of the anon block or the closing line, e.g. "} sbit;",
	   of the anon block: */

	sol = h->anonData.iLineStart;
	p = &h->anonData.data.array[sol];
	q = &h->anonData.data.array[h->anonData.data.cnt - 1];
	h->anonData.iLineStart = h->anonData.data.cnt;

#define SKIP_SPACE(p, q) while ((p) < (q) && isspace(*(p) & 0xff)) (p)++
#define MATCH_CHAR(p, c) do { if (*(p)++ != (c)) { return 0; } \
} \
	while (0)

	SKIP_SPACE(p, q);
	MATCH_CHAR(p, '}');
	SKIP_SPACE(p, q);
	for (i = 24; i >= 0; i -= 8) {
		char t = (char)(h->anonData.tag >> i & 0xff);
		if (i != 24 && t == ' ') {
			break;
		}
		MATCH_CHAR(p, t);
	}
	SKIP_SPACE(p, q);
	MATCH_CHAR(p, ';');
	SKIP_SPACE(p, q);

	if (*p == '#' || p == q) {
		h->anonData.data.cnt = sol;
		return 1;
	}

	return 0;
}

static void featAddAnonData(void) {
	if (g->cb.featAddAnonData != NULL) {
		g->cb.featAddAnonData(g->cb.ctx, h->anonData.data.array,
		                      h->anonData.data.cnt, h->anonData.tag);
	}
	h->anonData.data.cnt = 0;
	h->anonData.iLineStart = 0;
}

static void subtableBreak(void) {
	int retval = 0; /* Suppress optimizer warning */

	if (h->curr.feature == aalt_ || h->curr.feature == size_) {
		featMsg(hotERROR,
		        "\"subtable\" use not allowed in 'aalt' or 'size' feature");
		return;
	}

	if (h->curr.tbl == GSUB_) {
		retval = GSUBSubtableBreak(g);
	}
	else if (h->curr.tbl == GPOS_) {
		retval = GPOSSubtableBreak(g);
	}
	else {
		featMsg(hotWARNING, "Statement not expected here");
		return;
	}

	if (retval) {
		featMsg(hotWARNING, "subtable break is supported only in class kerning lookups");
	}
}

void featUnexpectedEOF(void) {
	featMsg(hotFATAL, "Unexpected end of file");
}

/* What zzerr was set to point to. Modified copy of zzerrstd() from
   dlgauto.h */

static void zzerrCustom(const char *s) {
	featMsg(hotFATAL, "%s (text was \"%s\")",
	        ((s == NULL) ? "Lexical error" : s), zzlextext);
}

/* Print zztoken[inx]; may be modified to look more accurate to user. */

void zztokenPrint(FILE *fp, int inx) {
	ANTLRChar *t = zztokens[inx];
	char quoteChar[MAX_TOKEN];

	if (strcmp(t, "K_OS_2") == 0) {
		t = "OS/2";
	}
	else if (strlen(t) >= 3 && (t[0] == 'K' || t[0] == 'T') && t[1] == '_') {
		t += 2;     /* Skip K_ or T_ prefix */
	}
	else if (strlen(t) == 2 && t[0] == '\\' && strpbrk(t + 1, "[]{}-") != NULL) {
		/* Skip backslash in quoted metacharacters */
		sprintf(quoteChar, "\"%c\"", *(t + 1));
		t = quoteChar;
	}
	else if (strlen(t) == 1 && strpbrk(t, ";,'@=<>") != NULL) {
		sprintf(quoteChar, "\"%c\"", *t); /* Single quote other punctuation */
		t = quoteChar;
	}
	/* Not treated specially:  #  \  (  )  */

	fprintf(fp, "%s", t);
}

/* Moved zzsyn from err.h to here; so that USER_ZZSYN won't have to be set. */

void zzsyn(char *text, int tok, char *egroup, SetWordType *eset, int etok,
           int k, char *bad_text) {
	fprintf(stderr, "syntax error at \"%s\"",
	        (tok == zzEOF_TOKEN) ? "EOF" : bad_text);
	if (!etok && !eset) {
	}
	else {
		if (k == 1) {
			fprintf(stderr, " missing");
		}
		else {
			fprintf(stderr, "; \"%s\" not", bad_text);
			if (zzset_deg(eset) > 1) {
				fprintf(stderr, " in");
			}
		}
		if (zzset_deg(eset) > 0) {
			zzedecode(eset);    /* prints something to stderr! */
		}
		else {
			fprintf(stderr, " ");
			zztokenPrint(stderr, etok);
		}

		if (strlen(egroup) > 0) {
			fprintf(stderr, " in %s", egroup);
		}
	}
	fprintf(stderr, " [%s %d]\n", INCL.file, zzline);
	hotMsg(g, hotFATAL, "aborting because of errors");
}

static Tag str2tag(char *tagName) {
	if (strcmp(tagName, "dflt") == 0) {
		return dflt_;
	}
	else {
		int i;
		char buf[4];

		strncpy(buf, tagName, 4);
		for (i = 3; buf[i] == '\0'; i--) {
			buf[i] = ' ';
		}
		return TAG(buf[0], buf[1], buf[2], buf[3]);
	}
}

void zzcr_attr(Attrib *attr, int type, char *text) {
	if (type == T_NUM) {
		attr->lval = strtol(text, NULL, 10);
	}
	else if (type == T_NUMEXT) {
		attr->lval = strtol(text, NULL, 0);
	}
	else if (type == T_CID) {
		attr->lval = strtol(text + 1, NULL, 10);    /* Skip initial '\' */
		if (attr->lval < 0 || attr->lval > 65535) {
			zzerr("not in range 0 .. 65535");
		}
	}
	else if (type == T_TAG ||
	         type == K_BASE ||
	         type == K_GDEF ||
	         type == K_head ||
	         type == K_hhea ||
	         type == K_name ||
	         type == K_OS_2 ||
	         type == K_vhea ||
	         type == K_vmtx) {
		attr->ulval = str2tag(text);
	}
	else {
		char *startCopy = (type == T_GNAME && *text == '\\') ? text + 1
			: text;
		char firstChar = (type == T_GCLASS) ? *text :
		    ((type == T_GNAME &&
		      strcmp(startCopy, ".notdef") != 0)
		     || type == T_LABEL) ?
		    *startCopy : '\0';

		if (strlen(text) >= MAX_TOKEN) {
			zzerrVA("token too long; max length is %d", MAX_TOKEN - 1);
		}

		/* Enforce valid names */
		if (firstChar != '\0' && (isdigit(firstChar) || firstChar == '.')) {
			zzerr("invalid first character in name");
		}

		strncpy(attr->text, startCopy, MAX_TOKEN - 1);
		attr->text[MAX_TOKEN - 1] = '\0';
	}
}

/* Makes copy; trim parens and spaces from each end of string */

char *featTrimParensSpaces(char *text) {
	char *s;
	char *e;
	char str[MAX_PATH];
	char *result;
	int len = strlen(text);

	strcpy(str, text);
	s = &str[1];
	e = &str[len - 2];

	if (str[0] != '(' || str[len - 1] != ')') {
		hotMsg(g, hotFATAL, "bad include file: <%s>", text);
	}

	/* Add sentinels at each end of string */
	str[0] = '\0';
	str[len - 1] = '\0';

	while (isspace(*s)) {
		/* Skip beginning spaces */
		s++;
	}
	while (isspace(*e)) {
		/* Skip ending spaces */
		e--;
	}
	*(e + 1) = '\0';

	copyStr(g, &result, s);
	return result;
}

#endif /* HOT_FEAT_SUPPORT */

/* --- Block list support --- */


static void blockListInit(featCtx h, long intl, long incr) {
	BlockList *bl = &h->blockList;

	bl->first = NULL;
	bl->intl = intl;
	bl->incr = incr;
}

static void blockListAddBlock(hotCtx g, featCtx h) {
	BlockList *bl = &h->blockList;

	if (bl->first == NULL) {
		/* Initial allocation */
		bl->first = bl->curr = MEM_NEW(g, sizeof(BlockNode));
		bl->curr->data = MEM_NEW(g, sizeof(GNode) * bl->intl);
	}
	else {
		/* Incremental allocation */
		bl->curr->next = MEM_NEW(g, sizeof(BlockNode));
		bl->curr = bl->curr->next;
		bl->curr->data = MEM_NEW(g, sizeof(GNode) * bl->incr);
	}
	bl->curr->next = NULL;
	bl->cnt = 0;
}

static GNode *blockListGetNewNode(hotCtx g, featCtx h) {
	BlockList *bl = &h->blockList;
	if (bl->first == NULL ||
	    bl->cnt == (bl->curr == bl->first ? bl->intl : bl->incr)) {
		blockListAddBlock(g, h);
	}
	return &bl->curr->data[bl->cnt++];
}

static void blockListFree(hotCtx g) {
	featCtx h = g->ctx.feat;
	BlockNode *p;
	BlockNode *pNext;
	BlockList *bl = &h->blockList;

	for (p = bl->first; p != NULL; p = pNext) {
		pNext = p->next;
		MEM_FREE(g, p->data);
		MEM_FREE(g, p);
	}
}

/* --- Linked list support --- */

/* Returns a glyph node, uninitialized except for flags */

static GNode *newNode(featCtx h) {
	GNode *ret;
	if (h->freelist != NULL) {
		GNode *newGNode;

#if HOT_DEBUG
		h->nNewFromFreeList++;
#endif
		/* Return first item from freelist */
		newGNode = h->freelist;
		h->freelist = h->freelist->nextSeq;
		ret = newGNode;
	}
	else {
		/* Return new item from da */
#if HOT_DEBUG
		h->nNewFromBlockList++;
#endif
		ret = blockListGetNewNode(h->g, h);
	}

	ret->flags = 0;
	ret->nextSeq = NULL;
	ret->nextCl = NULL;
	ret->lookupLabel = -1;
	ret->metricsInfo = NULL;
	ret->aaltIndex = 0;
	ret->markClassName = NULL;
	ret->markClassAnchorInfo.format = 0;
	return ret;
}

GNode *featSetNewNode(hotCtx g, GID gid) {
	featCtx h = g->ctx.feat;
	GNode *new = newNode(h);

	new->gid = gid;
	return new;
}

#if HOT_FEAT_SUPPORT
/* Gets length of pattern sequence (ignores any classes) */

unsigned featGetPatternLen(hotCtx g, GNode *pat) {
	unsigned len = 0;
	for (; pat != NULL; pat = pat->nextSeq) {
		len++;
	}
	return len;
}

/* Duplicates node (copies GID) num times to create a class */

void featExtendNodeToClass(hotCtx g, GNode *node, int num) {
	int i;

	if (node->nextCl != NULL) {
		hotMsg(g, hotFATAL, "[internal] can't extend glyph class");
	}

	if (!(node->flags & FEAT_GCLASS)) {
		node->flags |= FEAT_GCLASS;
	}

	for (i = 0; i < num; i++) {
		node->nextCl = featSetNewNode(g, node->gid);
		node = node->nextCl;
	}
}

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

/* Sort a glyph class; head node retains flags of original head node */
/* nextSeq, flags, markClassName, and markCnt of head now are zeroed. */

void featGlyphClassSort(hotCtx g, GNode **list, int unique, int reportDups) {
	featCtx h = g->ctx.feat;
	long i;
	GNode *p = *list;
	/* Preserve values that are kept with the head node only, and then zero them in the head node. */
	GNode *nextTarg = p->nextSeq;
	short flags = (*list)->flags;
	MetricsInfo *metricsInfo =  p->metricsInfo;
	char *markClassName = p->markClassName;
	p->markClassName = NULL;
	p->metricsInfo = NULL;
	p->nextSeq = NULL;
	p->flags = 0;
	h->sortTmp.cnt = 0;

	/* Copy over pointers */
	for (; p != NULL; p = p->nextCl) {
		*dnaNEXT(h->sortTmp) = p;
	}

	qsort(h->sortTmp.array, h->sortTmp.cnt, sizeof(GNode *), cmpNode);

	/* Move pointers around */
	for (i = 0; i < h->sortTmp.cnt - 1; i++) {
		h->sortTmp.array[i]->nextCl = h->sortTmp.array[i + 1];
	}
	h->sortTmp.array[i]->nextCl = NULL;

	*list = h->sortTmp.array[0];

	/* Check for duplicates */
	if (unique && !g->hadError) {
		for (p = *list; p->nextCl != NULL; ) {
			if (p->gid == p->nextCl->gid) {
				GNode *tmp = p->nextCl;

				p->nextCl = tmp->nextCl;
				tmp->nextCl = NULL;
				tmp->nextSeq = NULL;

				/* if INCL_CNT is zero, we are being called after ther feature files have been closed.
				   In this case, we are sorting GDEF classes, and duplicates don't need a warning.*/
				if ((INCL_CNT > 0) && reportDups) {
					featGlyphDump(g, tmp->gid, '\0', 0);
					featMsg(hotNOTE, "Removing duplicate glyph <%s>",
					        g->note.array);
				}
				featRecycleNodes(g, tmp);
			}
			else {
				p = p->nextCl;
			}
		}
	}


	/*restore head node values to the new head node.*/
	p = *list;
	p->flags = flags;
	p->nextSeq = nextTarg;
	p->metricsInfo = metricsInfo;
	p->markClassName = markClassName;
}

/* Return address of last nextCl. Preserves everything but sets nextSeq of each
   copied GNode to NULL	*/

GNode **featGlyphClassCopy(hotCtx g, GNode **dst, GNode *src) {
	featCtx h = g->ctx.feat;
	GNode **newDst = dst;
	for (; src != NULL; src = src->nextCl) {
		*newDst = newNode(h);
		**newDst = *src;
		(*newDst)->nextSeq = NULL;
		newDst = &(*newDst)->nextCl;
	}
	return newDst;
}

/* Make a copy of src pattern. If num != -1, copy upto num nodes only (assumes
   they exist); set the last nextSeq to NULL. Preserves all flags.
   Return address of last nextSeq (so that client may add on to the end). */

GNode **featPatternCopy(hotCtx g, GNode **dst, GNode *src, int num) {
	int i = 0;

	for (; (num == -1) ? src != NULL : i < num; i++, src = src->nextSeq) {
		featGlyphClassCopy(g, dst, src);
		dst = &(*dst)->nextSeq;
	}
	return dst;
}

/* Creates the cross product of pattern pat, and returns a pointer to the array
   of *n resulting sequences. pat is left intact; the client is responsible for
   recycling the result. */

GNode ***featMakeCrossProduct(hotCtx g, GNode *pat, unsigned *n) {
	GNode *cl;
	featCtx h = g->ctx.feat;

	h->prod.cnt = 0;

	/* Add each glyph class in pat to the cross product */
	for (cl = pat; cl != NULL; cl = cl->nextSeq) {
		long i;
		long nProd;

		if (cl == pat) {
			*dnaNEXT(h->prod) = NULL;
		}
		nProd = h->prod.cnt;

		for (i = 0; i < nProd; i++) {
			GNode *p;
			int cntCl = 0;
			int lenSrc = 0;
			/* h->prod.array[i] is the source. Don't store its address as a
			   pointer in a local variable since dnaINDEX below may obsolete
			   it! */

			for (p = cl; p != NULL; p = p->nextCl) {
				GNode **r;

				if (p == cl) {
					for (r = &h->prod.array[i]; *r != NULL; r = &(*r)->nextSeq) {
						lenSrc++;
					}
				}
				else {
					int inxTarg = nProd * cntCl + i;
					GNode **targ = dnaINDEX(h->prod, inxTarg);

					if (inxTarg >= h->prod.cnt) {
						/* can't use da_INDEXS */
						h->prod.cnt = inxTarg + 1;
					}
					r = featPatternCopy(g, targ, h->prod.array[i], lenSrc);
				}
				*r = featSetNewNode(g, p->gid);
				cntCl++;
			}
		}
	}

	*n = (unsigned)h->prod.cnt;
	return &h->prod.array;
}

static int CDECL matchValueDef(const void *key, const void *value) {
	return strcmp((char *)key, ((ValueDef *)value)->valueName);
}

static int CDECL cmpValueDefs(const void *first, const void *second, void *ctx) {
	const ValueDef *a = first;
	const ValueDef *b = second;
	return strcmp(a->valueName, b->valueName);
}

void featAddValRecDef(short *metrics, char *valueName) {
	ValueDef *vd;
	vd = (ValueDef *)bsearch(valueName, h->valueDefs.array, h->valueDefs.cnt,
	                         sizeof(ValueDef), matchValueDef);

	if (vd != NULL) {
		featMsg(hotFATAL, "Named value record definition '%s' is a a duplicate of an earlier named value record definition.", valueName);
		return;
	}

	vd =  (ValueDef *)dnaNEXT(h->valueDefs);
	vd->metrics[0] =  metrics[0];
	vd->metrics[1] =  metrics[1];
	vd->metrics[2] =  metrics[2];
	vd->metrics[3] =  metrics[3];

	strcpy(vd->valueName, valueName);
	ctuQSort(h->valueDefs.array, h->valueDefs.cnt, sizeof(ValueDef), cmpValueDefs, &h);
}

static void fillMetricsFromValueDef(char *valueName, short *metrics) {
	ValueDef *vd;
	vd = (ValueDef *)bsearch(valueName, h->valueDefs.array, h->valueDefs.cnt,
	                         sizeof(ValueDef), matchValueDef);

	if (vd == NULL) {
		featMsg(hotERROR, "Named value reference '%s' is not in list of named value records.", valueName);
		metrics[0] = 0;
		metrics[1] = 0;
		metrics[2] = 0;
		metrics[3] = 0;
		return;
	}
	metrics[0] = vd->metrics[0];
	metrics[1] = vd->metrics[1];
	metrics[2] = vd->metrics[2];
	metrics[3] = vd->metrics[3];
}

static int CDECL matchAnchorDef(const void *key, const void *value) {
	return strcmp((char *)key, ((AnchorDef *)value)->anchorName);
}

static int CDECL cmpAnchorDefs(const void *first, const void *second, void *ctx) {
	const AnchorDef *a = first;
	const AnchorDef *b = second;
	return strcmp(a->anchorName, b->anchorName);
}

void featAddAnchorDef(short x, short y, unsigned short contourIndex, int hasContour, char *anchorName) {
	AnchorDef *ad;
	ad = (AnchorDef *)bsearch(anchorName, h->anchorDefs.array, h->anchorDefs.cnt,
	                          sizeof(AnchorDef), matchAnchorDef);

	if (ad != NULL) {
		featMsg(hotFATAL, "Named anchor definition '%s' is a a duplicate of an earlier named anchor definition.", anchorName);
		return;
	}

	ad =  (AnchorDef *)dnaNEXT(h->anchorDefs);
	ad->x = x;
	ad->y = y;
	if (hasContour) {
		ad->contourpoint = contourIndex;
		ad->hasContour = 1;
	}
	else {
		ad->hasContour = 0;
	}

	strcpy(ad->anchorName, anchorName);
	ctuQSort(h->anchorDefs.array, h->anchorDefs.cnt, sizeof(AnchorDef), cmpAnchorDefs, &h);
}

static void featAddAnchor(short xVal, short yVal, unsigned short contourIndex, int isNULL, int hasContour, char *anchorName, int componentIndex) {
	AnchorMarkInfo *anchorMarkInfo = dnaNEXT(h->anchorMarkInfo);
	anchorMarkInfo->markClass = NULL;

	if (anchorName != NULL) {
		AnchorDef *ad;
		ad = (AnchorDef *)bsearch(anchorName, h->anchorDefs.array, h->anchorDefs.cnt,
		                          sizeof(AnchorDef), matchAnchorDef);

		if (ad == NULL) {
			featMsg(hotFATAL, "Named anchor reference '%s' is not in list of named anchors.", anchorName);
			return;
		}
		anchorMarkInfo->x = ad->x;
		anchorMarkInfo->y = ad->y;

		if (ad->hasContour) {
			anchorMarkInfo->format = 2;
			anchorMarkInfo->contourpoint = contourIndex;
		}
		else {
			anchorMarkInfo->format = 1;
		}
	}
	else {
		anchorMarkInfo->x = xVal;
		anchorMarkInfo->y = yVal;
		if (isNULL) {
			anchorMarkInfo->format = 0;
		}
		else if (hasContour) {
			anchorMarkInfo->format = 2;
			anchorMarkInfo->contourpoint = contourIndex;
		}
		else {
			anchorMarkInfo->format = 1;
		}
	}
	anchorMarkInfo->componentIndex = componentIndex;
}

/* Add new mark class definition */
static void featAddMark(GNode *targ, char *markClassName) {
	GNode *markNode;
	GNode *curNode;
	GNode **gcInsert;
	AnchorMarkInfo *anchorMarkInfo = dnaINDEX(h->anchorMarkInfo, h->anchorMarkInfo.cnt - 1);

	/* save this anchor info in all glyphs of this mark class */
	curNode = targ;
	while (curNode != NULL) {
		curNode->markClassAnchorInfo = *anchorMarkInfo;
		curNode = curNode->nextCl;
	}


	gcInsert = gcOpen(markClassName); /* Get or creates a new class node with this node in the hash of class names, and sets to be the target of class additions, */
	/* If it is a new class, sets the head class gnode into h->he, and sets h->gcInsert to &h->he->value.head.
	   else gcInsert holds the  the address of the last gnode->nextCl */
	featGlyphClassCopy(g, gcInsert, targ);
	markNode =  gcLookup(markClassName); /* gcInsert points to the address of the nextCl field in the last node in the class chain; we need the address of the head node */
	if (markNode->flags & FEAT_USED_MARK_CLASS) {
		featMsg(hotERROR, "You cannot add glyphs to a mark class after the mark class has been used in a position statement. %s.", markClassName);
	}

	if (*gcInsert == markNode) {
		/* this is the first time this mark class name has been refererenced; save the class name in the head node. */
		copyStr(g, &markNode->markClassName, markClassName);
		*dnaNEXT(h->markClasses) = markNode; /* This is an array that captures all the named mark classes referenced in the feature file.
		                                        This is not the same as the list in the GPOS.c file, which isa list only of mark classes used the lookup. */
	}

	/* add mark glyphs to default base class */
	gcInsert = gcOpen(kDEFAULT_MARKCLASS_NAME);
	featGlyphClassCopy(g, gcInsert, targ);

	featRecycleNodes(g, targ);

	h->gFlags |= GF_SEEN_MARK_CLASS_FLAG;
}

/* Add mark class reference to current anchorMarkInfo for the rule. */
static void addMarkClass(char *markClassName) {
	GNode *headNode;
	AnchorMarkInfo *anchorMarkInfo = dnaINDEX(h->anchorMarkInfo, h->anchorMarkInfo.cnt - 1);
	headNode = gcLookup(markClassName);
	if (headNode == NULL) {
		featMsg(hotERROR, "MarkToBase or MarkToMark rule references a mark class (%s) that has not yet been defined", markClassName);
		return;
	}
	headNode->flags |= FEAT_USED_MARK_CLASS;
	anchorMarkInfo->markClass = headNode;
}

static void getMarkSetIndex(char *markClassName, unsigned short *markSetIndex) {
	GNode *markClass = gcLookup(markClassName);
	*markSetIndex = addMarkSetClassGDEF(g, markClass);
}

static void getGDEFMarkClassIndex(char *markClassName, unsigned short *markAttachClassIndex) {
	GNode *markClass = gcLookup(markClassName);
	*markAttachClassIndex = addGlyphMarkClassGDEF(g, markClass);
	if (*markAttachClassIndex > kMaxMarkAttachClasses) {
		featMsg(hotERROR, "No more than 15 different class names can be used with the \"lookupflag MarkAttachmentType <class name.\" statement \"%s\" would be a 16th.", markClassName);
	}
}

#if HOT_DEBUG


static void prodDump(void) {
	int i;
	for (i = 0; i < h->prod.cnt; i++) {
		GNode **seq = &h->prod.array[i];
		fprintf(stderr, "[%d] ", i);
		featPatternDump(g, *seq, '\n', 1);
	}
}

static void checkFreeNode(featCtx h, GNode *node) {
	GNode *testNode = h->freelist;
	long cnt = 0;
	while (testNode->nextSeq != NULL) {
		if ((testNode->nextSeq == node) || (testNode == node)) {
			printf("Node duplication in free list %lu. gid: %d!\n", (unsigned long)node, node->gid);
		}
		testNode = testNode->nextSeq;
		cnt++;
		if (cnt > h->nAdded2FreeList) {
			printf("Endless loop in checkFreeList.\n");
			break;
		}
	}
}

#endif

#endif /* HOT_FEAT_SUPPORT */

/* Add node to freelist (Insert at beginning) */

static void recycleNode(featCtx h, GNode *node) {
#if HOT_DEBUG
	h->nAdded2FreeList++;
#endif
	if (h->freelist == NULL) {
		h->freelist = node;
		h->freelist->nextSeq = NULL;
	}
	else {
		GNode *list = h->freelist;
#if HOT_DEBUG
		checkFreeNode(h, node);
#endif
		h->freelist = node;
		h->freelist->nextSeq = list;
	}
}

#define ITERATIONLIMIT 100000

/* Add nodes to freelist */

void featRecycleNodes(hotCtx g, GNode *node) {
	featCtx h = g->ctx.feat;
	GNode *nextSeq;
	long iterations = 0;


	for (; node != NULL; node = nextSeq) {
		nextSeq = node->nextSeq;

		if (node->nextCl == NULL) {
			recycleNode(h, node);
		}
		else {
			GNode *nextCl;
			GNode *cl = node;

			/* Recycle class */
			for (; cl != NULL; cl = nextCl) {
				nextCl = cl->nextCl;
				recycleNode(h, cl);
				if (iterations++ > ITERATIONLIMIT) {
					fprintf(stderr, "Makeotf [WARNING]: Timeout in featRecycleNode. Possible error.\n");
					return;
				}
			}
		}
	}
}

#if HOT_FEAT_SUPPORT
/* --- Hash table support --- */

/* Initialize hash table */

static void hashInit(featCtx h) {
	int i;
	for (i = 0; i < HASH_SIZE; i++) {
		h->ht[i] = NULL;
	}
}

/* Form hash value for key */

static unsigned hashGetValue(char *p) {
	unsigned val = 0;
	for (; *p != '\0'; p++) {
		val = val * 33 + *p;
	}
	return val & (HASH_SIZE - 1);
}

/* Lookup name in hash table */

static HashElement *hashLookup(char *name) {
	HashElement *p;

	for (p = h->ht[hashGetValue(name)]; p != NULL; p = p->next) {
		if (strcmp(name, p->name) == 0) {
			return p;
		}
	}
	return NULL;
}

static HashElement *hashInstallElement(char *name, int existsOK) {
	HashElement *el = hashLookup(name);

	if (el == NULL) {
		/* Not found; add to hash table at the beginning of the list of any
		   current elements already at that value */
		unsigned hashval = hashGetValue(name);
		el = MEM_NEW(g, sizeof(HashElement));
		el->next = h->ht[hashval];
		el->value.head = NULL;
		h->ht[hashval] = el;
		copyStr(g, &el->name, name);
	}
	else {
		/* Already present. For mark classes, we add to classes with each new mark statement; otherwise, we are redefining a class */
		if (!existsOK) {
			featMsg(hotWARNING, "Glyph class @%s redefined",
			        g->font.FontName.array, name);
			MEM_FREE(g, el->name);
			featRecycleNodes(g, el->value.head);
			el->value.head = NULL;
			copyStr(g, &el->name, name);
		}
	}
	return el;
}

static void hashFree(featCtx h) {
	unsigned i;
	int iterations = 0;

	for (i = 0; i < HASH_SIZE; i++) {
		HashElement *curr;
		HashElement *next;
		for (curr = h->ht[i]; curr != NULL; curr = next) {
			next = curr->next;
			featRecycleNodes(g, curr->value.head);
			MEM_FREE(g, curr->name);
			MEM_FREE(g, curr);
			if (iterations++ > ITERATIONLIMIT) {
				fprintf(stderr, "Makeotf [WARNING]: Timeout in hashFree. Possible error.\n");
				return;
			}
		}
	}
}

/* --- Glyph --- */


/* Map feature file glyph name to gid; emit error message and return notdef if
   not found (in order to continue until hotQuitOnError() called) */
static GID featMapGName2GID(hotCtx g, char *gname, int allowNotdef) {
	GID gid;
	char *realname;

//	if (IS_CID(g)) {
//		zzerr("glyph name specified for a CID font");
//	}

	gid = mapName2GID(g, gname, &realname);

	if (gid != GID_UNDEF) {
		return gid;
	}
	if (allowNotdef == 0) {
		if (realname != NULL && strcmp(gname, realname) != 0) {
			featMsg(hotERROR, "Glyph \"%s\" (alias \"%s\") not in font",
					realname, gname);
		}
		else {
			featMsg(hotERROR, "Glyph \"%s\" not in font (featMapGName2GID)", gname);
		}
	}
	return GID_NOTDEF;
}

static GID cid2gid(hotCtx g, CID cid) {
	GID gid = 0;    /* Suppress optimizer warning */
	if (!IS_CID(g)) {
		zzerr("CID specified for a non-CID font");
	}
	else if ((gid = mapCID2GID(g, cid)) == GID_UNDEF) {
		zzerr("CID not found in font");
	}
	return gid;     /* Suppress compiler warning */
}

/* --- Glyph class --- */

/* Start defining a glyph class */

static void gcDefine(char *gcname) {
	h->he = hashInstallElement(gcname + 1, 0);
	h->gcInsert = &h->he->value.head;
}

/* Start defining a mark glyph class; finds or create a new glyph class, then
   sets  h->gcInsert to teh nextClass of the last current defintion.
   Used when adding to a class definiton. */

static GNode **gcOpen(char *gcname) {
	h->he = hashInstallElement(gcname + 1, 1);
	if (h->he->value.head == NULL) {
		h->gcInsert = &h->he->value.head;
	}
	else {
		/* find the last class definition */
		GNode *lastNode = h->he->value.head;
		GNode *nextClass = lastNode->nextCl;
		while (nextClass != NULL) {
			lastNode = nextClass;
			nextClass = nextClass->nextCl;
		}
		h->gcInsert = &(lastNode->nextCl);
	}
	return h->gcInsert;
}

/* Add glyph to end of current glyph class */

static void gcAddGlyph(GID gid) {
	*h->gcInsert = featSetNewNode(g, gid);
	h->gcInsert = &(*h->gcInsert)->nextCl;
#if 0
	fprintf(stderr, "adding glyph ");
	featGlyphDump(g, gid, '\n', 1);
#endif
}

static long getNum(char *str, int length) {
	char buf[MAX_NUM_LEN + 1];
	strncpy(buf, str, length);
	buf[length] = '\0';
	return strtol(buf, NULL, 10);
}

/* Glyph names in range differ by a single letter */

static void addAlphaRange(GID first, GID last, char *firstName, char *p,
                          char q) {
	char gname[MAX_TOKEN];
	char *ptr;

	strcpy(gname, firstName);
	ptr = &gname[p - firstName];

	for (; *ptr <= q; (*ptr)++) {
		GID gid = (*ptr == *p) ? first : (*ptr == q) ? last :
		    featMapGName2GID(g, gname, FALSE);
		gcAddGlyph(gid);
	}
}

/* Glyph names in range differ by a decimal number */


static void addNumRange(GID first, GID last, char *firstName, char *p1,
                        char *p2, char *q1, int numLen) {
	/* --- Range is valid number range --- */
	char gname[MAX_TOKEN];
	long i;
	long firstNum = getNum(p1, numLen);
	long lastNum = getNum(q1, numLen);

	for (i = firstNum; i <= lastNum; i++) {
		GID gid;
		char fmt[128];
		char preNum[MAX_TOKEN];

		if (i == firstNum) {
			gid = first;
		}
		else if (i == lastNum) {
			gid = last;
		}
		else {
			if (i == firstNum + 1) {
				sprintf(fmt, "%%s%%0%dd%%s", numLen);
				/* Part of glyph name before number; p2 marks
				   post-number */
				strncpy(preNum, firstName, p1 - firstName);
				preNum[p1 - firstName] = '\0';
			}
			sprintf(gname, fmt, preNum, i, p2);
			gid = featMapGName2GID(g, gname, FALSE);
		}
		gcAddGlyph(gid);
	}
}

/* Add glyph range to end of current glyph class */

static void gcAddRange(GID first, GID last, char *firstName, char *lastName) {
#define INVALID_RANGE featMsg(hotFATAL, "Invalid glyph range [%s-%s]", \
	                          firstName, lastName)

	if (IS_CID(g)) {
		if (first <= last) {
			GID i;
			for (i = first; i <= last; i++) {
				gcAddGlyph(i);
			}
		}
		else {
			featMsg(hotFATAL, "Bad GID range: %u thru %u", first, last);
		}
	}
	else {
		/* Enforce glyph range rules */
		char *p1, *p2;
		char *q1, *q2;

		if (strlen(firstName) != strlen(lastName)) {
			INVALID_RANGE;
		}

		for (p1 = firstName, q1 = lastName;
		     *p1 != '\0' && *p1 == *q1;
		     p1++, q1++) {
		}
		if (*p1 == '\0') {
			INVALID_RANGE;      /* names are same */
		}
		for (p2 = p1, q2 = q1;
		     *p2 != '\0' && *p2 != *q2;
		     p2++, q2++) {
		}
		/* Both ends of the first difference are now marked. The remainder
		   should be the same: */
		if (strcmp(p2, q2) != 0) {
			INVALID_RANGE;
		}

		/* A difference of up to 3 digits is allowed; 1 for letters */
		switch (p2 - p1) {
			int i;

			case 1:
				if (isalpha(*p1) && isalpha(*q1) && *q1 > *p1 && *q1 - *p1 < 26) {
					addAlphaRange(first, last, firstName, p1, *q1);
					return;
				}

			case 2:
			case 3:
				/* All differences should be digits */
				for (i = 0; i < p2 - p1; i++) {
					if (!isdigit(p1[i]) || !isdigit(q1[i])) {
						INVALID_RANGE;
					}
				}
				/* Search for largest enclosing number */
				for (; p1 > firstName && isdigit(*(p1 - 1)); p1--, q1--) {
				}
				for (; isdigit(*p2); p2++, q2++) {
				}
				if (p2 - p1 > MAX_NUM_LEN) {
					INVALID_RANGE;
				}
				else {
					addNumRange(first, last, firstName, p1, p2, q1, p2 - p1);
					return;
				}
				break;

			default:
				INVALID_RANGE;
		}
	}
}

static GNode *gcLookup(char *gcName) {
	HashElement *gc = hashLookup(gcName + 1);
	if (gc == NULL) {
		zzerr("glyph class not defined");
	}
	return gc->value.head;
}

/* Insert copy of gc in currently-defined anon/named gc */

static GNode *gcAddGlyphClass(char *subGCName, int named) {
	HashElement *subGC = hashLookup(subGCName + 1);

	if (subGC == NULL) {
		zzerr("glyph class not defined");
	}

	if (named) {
		if (strcmp(subGC->name, h->he->name) == 0) {
			zzerr("can't define a glyph class in terms of itself");
		}
	}
	h->gcInsert = featGlyphClassCopy(g, h->gcInsert, subGC->value.head);

	return h->he->value.head;
}

/* If named, return ptr to beginning of gc */

static GNode *gcEnd(int named) {
	*h->gcInsert = NULL;
	return (named) ? h->he->value.head : NULL;
}

/* ----------------------- Opening and closing files ----------------------- */

/* Replenish input buffer using refill function and handle end of input */

static int fillBuf(featCtx h) {
	long count;
    int retVal;
	h->data = g->cb.featRefill(g->cb.ctx, &count);
	h->nextoffset += count;
	retVal =  (count == 0) ? EOF : (unsigned char)*h->data++;
    return retVal;
}

/* Get next character of current file being read */

static int getFeatCh(void) {
	return (h->offset++ == h->nextoffset) ? fillBuf(h) :
	       (unsigned char)*h->data++;
}

/* filename can be a partial path; NULL indicates main feature file. */

int featOpenIncludeFile(hotCtx g, char *filename) {
	featCtx h = g->ctx.feat;
	char *fullpath;
#if HOT_DEBUG
	int i;
#endif

	/* Save current file data and close file */
	if (INCL_CNT > 0) {
		if (INCL_CNT == MAX_INCL) {
			featMsg(hotFATAL,
			        "Can't include [%s]; maximum include levels <%d> reached",
			        filename, MAX_INCL);
		}

		INCL.lineno = zzline;
		INCL.offset = h->offset;
		g->cb.featClose(g->cb.ctx);
	}

	/* Open new file */
	dnaNEXT(h->stack);
	if (g->cb.featOpen == NULL) {
		return 0;       /* Indicates no feature file for this font */
	}
	fullpath = g->cb.featOpen(g->cb.ctx, filename, 0);
	if (fullpath == NULL) {
		if (filename == NULL) {
			return 0;   /* Indicates no feature file for this font */
		}
		else {
			InclFile *prev = &h->stack.array[h->stack.cnt - 2];
			hotMsg(g, hotFATAL, "include file <%s> not found [%s %d]",
			       filename, prev->file, prev->lineno);
		}
	}

	copyStr(g, &INCL.file, fullpath);
	if (INCL.file == NULL) {
		return 0;
	}
	h->offset = h->nextoffset = 0;
	if (INCL_CNT > 1) {
		MEM_FREE(g, filename);  /* Was allocated by featTrimParensSpaces() */
	}
#if HOT_DEBUG
	for (i = 0; i < INCL_CNT - 1; i++) {
		DF(3, (stderr, " "));
	}
	DF(3, (stderr, "--- opening [%s]\n", INCL.file));
#endif

	if (INCL_CNT == 1) {
		/* Caution: macro not enclosed in block */
		ANTLRf(featureFile(), &getFeatCh);
	}
	else {
		zzrdfunc(&getFeatCh);       /* Sets zzline = 1 */
	}
	return 1;
}

/* Returns 0 if main file is current. Main file should be closed only at end of
   parse for proper error reporting after eof is reached. */

int featCloseIncludeFile(hotCtx g, int closeMain) {
	featCtx h = g->ctx.feat;

	if (INCL_CNT == 1 && !closeMain) {
		return 0;
	}
	else {
#if HOT_DEBUG
		int i;
		for (i = 0; i < INCL_CNT - 1; i++) {
			DF(2, (stderr, " "));
		}
		DF(2, (stderr, "--- closing [%s]\n", INCL.file));
#endif

		/* Close top of stack */
		g->cb.featClose(g->cb.ctx);
		MEM_FREE(g, INCL.file);
		INCL_CNT--;

		if (INCL_CNT > 0) {
			/* Reopen file at proper offset */
			char *filename = (INCL_CNT == 1) ? NULL : INCL.file;

			if (g->cb.featOpen(g->cb.ctx, filename, INCL.offset) == NULL) {
				hotMsg(g, hotFATAL, "include file <%s> not found", INCL.file);
			}
			h->offset = h->nextoffset = INCL.offset;
			zzline = INCL.lineno;
			DF(3, (stderr, "[reopening %s at off %ld]\n", INCL.file,
			       INCL.offset));
		}

		return 1;   /* not used for closeMain == 1 */
	}
}

/* --- Checking if fea/scr/lan/lkp already defined --- */


static int tagDefined(Tag tag, TagArray *ta) {
	long i;
	for (i = 0; i < ta->cnt; i++) {
		/* Sequential search */
		if (ta->array[i] == tag) {
			return 1;
		}
	}
	return 0;
}

static int tagIndex(Tag tag, TagArray *ta) {
	long i;
	long result = -1;
	for (i = 0; i < ta->cnt; i++) {
		/* Sequential search */
		if (ta->array[i] == tag) {
			result = i;
		}
	}
	return result;
}

static NamedLkp *name2NamedLkp(char *lkpName) {
	int i;
	for (i = 0; i < h->namedLkp.cnt; i++) {
		NamedLkp *curr = &h->namedLkp.array[i];
		if (strcmp(curr->name, lkpName) == 0) {
			return curr;
		}
	}
	return NULL;
}

static NamedLkp *lab2NamedLkp(Label lab) {
	Label baselab = (Label)(lab & ~REF_LAB);

	/* Base label is simply the index of h->namedLkp */
	if (!IS_NAMED_LAB(baselab) || baselab >= (Label)h->namedLkp.cnt) {
		return NULL;
	}
	else {
		return &h->namedLkp.array[baselab];
	}
}

/* Call when the GSUB/GPOS lookup/feature calls need to be closed */

static void closeFeaScrLan(State *st) {
	if (st->tbl != TAG_UNDEF) {
		/* Close current lookup */
		if (st->lkpType != 0) {
			if (st->tbl == GSUB_) {
				GSUBLookupEnd(g, st->feature);
			}
			else if (st->tbl == GPOS_) {
				GPOSLookupEnd(g, st->feature);
			}
			else {
				hotMsg(g, hotFATAL, "[internal] unrecognized tbl");
			}
		}

		/* Close feature */
		if (st->tbl == GSUB_) {
			GSUBFeatureEnd(g);
		}
		else if (st->tbl == GPOS_) {
			GPOSFeatureEnd(g);
		}
		else {
			hotMsg(g, hotFATAL, "[internal] unrecognized tbl");
		}
	}
}

static void wrapUpRule(void) {
	h->prev = h->curr;
	if (h->endOfNamedLkpOrRef == 1) {
		h->endOfNamedLkpOrRef = 0;
	}
}

/* Add a language system that all features (that do not contain language or
   script statements) would be registered under */

static void addLangSys(Tag script, Tag language, int checkBeforeFeature) {
	long i;
	LangSys *new;

	if (checkBeforeFeature && h->gFlags & GF_SEEN_FEATURE) {
		featMsg(hotERROR, "languagesystem must be specified before all"
		        " feature blocks");
		return;
	}
	if (!(h->gFlags & GF_SEEN_LANGSYS)) {
		h->gFlags |= GF_SEEN_LANGSYS;
	}
	else if (script == DFLT_) {
		featMsg(hotERROR, "The languagesystem DFLT dlft; statement must preceed all other language system statements.");
	}

	if (script == dflt_) {
		featMsg(hotWARNING, "'dflt' is not a valid script tag for a languagesystem  statement; using 'DFLT'.");
		script = DFLT_;
	}

	if (language == DFLT_) {
		featMsg(hotWARNING, "'DFLT' is not a valid language tag for a languagesystem  statement; using 'dflt'.");
		language = dflt_;
	}

	if ((script == DFLT_) && (language != dflt_)) {
		featMsg(hotERROR, "The languagesystem script tag DFLT must be used only with the language tag dflt.");
	}



	/* First check if already exists */
	for (i = 0; i < h->langSys.cnt; i++) {
		if (h->langSys.array[i].script == script &&
		    h->langSys.array[i].lang == language) {
			featMsg(hotWARNING, "Duplicate specification of language system");
			return;
		}
	}

	new = dnaNEXT(h->langSys);
	(*new).script = script;
	(*new).lang = language;
	(*new).excludeDflt = 0;

	if (!checkBeforeFeature) {
		DF(2, (stderr, "languagesystem '%c%c%c%c' '%c%c%c%c' ;\n",
		       TAG_ARG(script), TAG_ARG(language)));
	}
}

/* Add tag to the appropriate array, checking first if it already exists (if
   requested to by checkIfDef) */

static int tagAssign(Tag tag, int type, int checkIfDef) {
	TagArray *ta = NULL;    /* Suppress optimizer warning */
	Tag *curr = NULL;

	if (type == featureTag) {
		ta = &h->feature;
		curr = &h->curr.feature;
	}
	else if (type == scriptTag) {
		ta = &h->script;
		if (tag == dflt_) {
			tag = dflt_;
			featMsg(hotWARNING, "'dflt' is not a valid tag for a script statement; using 'DFLT'.");
		}
		curr = &h->curr.script;
	}
	else if (type == languageTag) {
		ta = &h->language;
		if (tag == DFLT_) {
			tag = dflt_;
			featMsg(hotWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
		}
		curr = &h->curr.language;
	}
	else if (type == tableTag) {
		ta = &h->table;
	}
	else {
		featMsg(hotFATAL, "[internal] unrecognized tag type");
	}

	if (checkIfDef && tagDefined(tag, ta)) {
		if ((type == featureTag) && (curr != NULL)) {
			*curr = tag;
		}
		return 0;
	}

	*dnaNEXT(*ta) = tag;
	if (curr != NULL) {
		*curr = tag;
	}

	return 1;
}

/* Indicate current feature or labeled lookup block to be created with
   extension lookupTypes. */

static void flagExtension(int isLookup) {
	if (isLookup) {
		/* lookup block scope */
		NamedLkp *curr = lab2NamedLkp(h->currNamedLkp);
		if (curr == NULL) {
			hotMsg(g, hotFATAL, "[internal] label not found\n");
		}
		curr->useExtension = 1;
	}
	else {
		/* feature block scope */
		if (h->curr.feature == aalt_) {
			h->aalt.useExtension = 1;
		}
		else {
			featMsg(hotERROR, "\"useExtension\" allowed in feature-scope only"
			        " for 'aalt'");
		}
	}
}

/* Register current feature under remaining language systems */

static void registerFeatureLangSys(void) {
	long i, j;

	for (i = 1; i < h->langSys.cnt; i++) {
		LangSys *langSys = &h->langSys.array[i];
		int seenGSUB = 0;
		int seenGPOS = 0;

		/* If we have seen an explicit language statement for this script,
		   then the default lookups have already been included ( or excluded) by
		   the includeDDflt function.
		 */
		if (langSys->excludeDflt) {
			langSys->excludeDflt = 0; /* Clear this so it won't affect the next feature */
			continue;
		}

		for (j = 0; j < h->lookup.cnt; j++) {
			LookupInfo *lkp = &h->lookup.array[j];
			if (lkp->tbl == GSUB_) {
				if (!seenGSUB) {
					GSUBFeatureBegin(g, (*langSys).script, (*langSys).lang,
					                 h->curr.feature);
					seenGSUB = 1;
				}
				GSUBLookupBegin(g, lkp->lkpType, lkp->lkpFlag,
				                (Label)(lkp->label | REF_LAB),
				                lkp->useExtension, lkp->markSetIndex);
				GSUBLookupEnd(g, h->curr.feature);
			}
			else {
				/* lkp->tbl == GPOS_ */
				if (!seenGPOS) {
					GPOSFeatureBegin(g, (*langSys).script, (*langSys).lang,
					                 h->curr.feature);
					seenGPOS = 1;
				}
				GPOSLookupBegin(g, lkp->lkpType, lkp->lkpFlag,
				                (Label)(lkp->label | REF_LAB),
				                lkp->useExtension, lkp->markSetIndex);
				GPOSLookupEnd(g, h->curr.feature);
			}
		}
		if (seenGSUB) {
			GSUBFeatureEnd(g);
		}
		if (seenGPOS) {
			GPOSFeatureEnd(g);
		}
	}
}

/* Called when a feature, script, language, or table tag is seen. atStart
   indicates if at start of a block (set to 1 for script and language tags).
   Returned 1 if statement had an effect, 0 if not, -1 if error */

static int checkTag(Tag tag, int type, int atStart) {
	if (atStart) {
		if (type == featureTag) {
			/* Allow interleaving features */
			if (tagAssign(tag, featureTag, 1) == 0) {
				if (tag != (Tag)TAG_STAND_ALONE) {
					/* This is normal for stand-alone lookup blocks- we use the same feature tag for each. */
					featMsg(hotWARNING, "feature already defined: %s", zzlextext);
				}
			}

			h->fFlags = 0;
			if (!(h->gFlags & GF_SEEN_FEATURE)) {
				h->gFlags |= GF_SEEN_FEATURE;
			}

			h->lookup.cnt = 0;
			h->script.cnt = 0;
			if (h->langSys.cnt == 0) {
				addLangSys(DFLT_, dflt_, 0);
				featMsg(hotWARNING,
				        "[internal] Feature block seen before any language system statement.  You should place languagesystem statements before any feature definition", zzlextext);
			}
			tagAssign(h->langSys.array[0].script, scriptTag, 0);

			h->language.cnt = 0;
			tagAssign(h->langSys.array[0].lang, languageTag, 0);

			h->include_dflt = 1;
			h->DFLTLkps.cnt = 0;

			h->curr.lkpFlag = 0;
			h->curr.markSetIndex = 0;
		}
		else if (type == scriptTag || type == languageTag) {
			if (h->curr.feature == aalt_ || h->curr.feature == size_) {
				featMsg(hotERROR, "\"script\" and \"language\" statements"
				        " not allowed anymore in 'aalt' or 'size' feature; "
				        USE_LANGSYS_MSG);
				return -1;
			}
			else if ((tag != (Tag)TAG_STAND_ALONE) && (h->curr.feature == (Tag)TAG_STAND_ALONE)) {
				featMsg(hotERROR, "\"script\" and \"language\" statements"
				        "not allowed in within named lookup blocks; ");
			}
			/*
			   if (h->fFlags & FF_LANGSYS_MODE)
			    {
			    featMsg(hotWARNING,
			    "If you don't want feature '%c%c%c%c' registered under all"
			    " the language systems you specified earlier on in the feature"
			    " file by the \"languagesystem\" keyword, you must start this"
			    " feature with an explicit \"script\" statement",
			    TAG_ARG(h->curr.feature));
			    return -1;
			    }
			 */
			if (!(h->fFlags & FF_SEEN_SCRLANG)) {
				h->fFlags |= FF_SEEN_SCRLANG;
			}


			if (type == scriptTag) {
				if (tag == h->curr.script && h->curr.language == dflt_) {
					return 0;
				}    /* Statement has no effect */

				/* Once we have seen a script or a language statement other than 'dflt'
				   any further rules in the featue should nto be added to the default list.
				 */
				if ((h->fFlags & FF_LANGSYS_MODE)) {
					h->fFlags &= ~FF_LANGSYS_MODE;
				}

				if (tag != h->curr.script) {
					if (tagAssign(tag, scriptTag, 1) == 0) {
						zzerr("script behavior already specified");
					}

					h->language.cnt = 0;
					h->DFLTLkps.cnt = 0; /* reset the script-specific default list to 0 */
				}
				if (h->curr.language != dflt_) {
					tagAssign(dflt_, languageTag, 0);
				}
				if (h->include_dflt != 1) {
					h->include_dflt = 1;
				}
				h->curr.lkpFlag = 0;
				h->curr.markSetIndex = 0;
			}
			else {
				/* type == languageTag */
				if (tag == DFLT_) {
					tag = dflt_;
					featMsg(hotWARNING, "'DFLT' is not a valid tag for a language statement; using 'dflt'.");
				}

				/* Once we have seen a script or a language statement other than 'dflt'
				   any further rules in the feature should nto be added to the default list.
				 */
				if ((h->fFlags & FF_LANGSYS_MODE) && (tag != dflt_)) {
					h->fFlags &= ~FF_LANGSYS_MODE;
				}

				if (tag == h->curr.language) {
					return 0;           /* Statement has no effect */
				}
				if (tag == dflt_) {
					zzerr("dflt must precede language-specific behavior");
				}
				else if (h->curr.script == DFLT_) {
					zzerr("DFLT script tag may be used only with the dlft language tag.");
				}

				if (tagAssign(tag, languageTag, 1) == 0) {
					zzerr("language-specific behavior already specified");
				}
			}
		}
		else if (type == tableTag) {
			if (tagAssign(tag, tableTag, 1) == 0) {
				zzerr("table already specified");
			}
		}
		else {
			hotMsg(g, hotFATAL, "[internal] unknown tag type <%d>", type);
		}
	}
	else {
		/* Closing tag */
		Tag startTag = 0;   /* Suppress optimizer warning */

		/* Match tag to beginning of block */
		if (type == featureTag) {
			startTag = h->curr.feature;
		}
		else if (type == tableTag) {
			startTag = h->table.array[h->table.cnt - 1];
		}
		else {
			hotMsg(g, hotFATAL, "[internal] unsupported closing tag type <%d>",
			       type);
		}
		if (tag != startTag) {
			zzerr("doesn't match starting tag of block");
		}

		if (type == featureTag) {
			/* End of feature block */
			if (h->curr.feature != aalt_) {
				closeFeaScrLan(&h->curr);
				registerFeatureLangSys();
			}
			h->prev.tbl = TAG_UNDEF;
		}
	}
	return 1;
}

/* ------------------------------ aalt creation ---------------------------- */


static void aaltAddFeatureTag(Tag tag) {
	if (h->curr.feature != aalt_) {
		featMsg(hotERROR,
		        "\"feature\" statement allowed only in 'aalt' feature");
	}
	else if (tag != aalt_ && !tagDefined(tag, &h->aalt.features)) {
		*dnaNEXT(h->aalt.features) = tag;
		*dnaNEXT(h->aalt.usedFeatures) = 0;
	}
}

static void reportUnusedaaltTags(featCtx h) {
	int i;
	for (i = 0; i < h->aalt.features.cnt; i++) {
		if (h->aalt.usedFeatures.array[i] == 0) {
			Tag tag = h->aalt.features.array[i];
			hotMsg(g, hotWARNING,
			       "feature '%c%c%c%c', referenced in aalt feature, either is not defined or had no rules which could be included in the aalt feature.",
			       TAG_ARG(tag));
		}
	}
}

/* If gid not in cl, return address of end insertion point, else NULL */

static int glyphInClass(GID gid, GNode **cl, GNode ***lastClass) {
	GNode **p = cl;

	for (p = cl; *p != NULL; p = &((*p)->nextCl)) {
		if ((*p)->gid == gid) {
			*lastClass = p;
			return 1;
		}
	}

	*lastClass = p;
	return 0;
}

/* Sort a list of alternates for a glyph, by index of contributing feature. */
/* ------------------------------------------------------------------- */

/* A simple insertion sort. Good enough when count < 50. */
static void aaltInsertSort(GNode **array,  size_t count) {
	signed a;
	GNode *p1;
	GNode *p2;

	unsigned int i, j;

	for (i = 1; i < count; i++) {
		j = i;
		p1 = array[i];
		a = p1->aaltIndex;
		while ((j > 0) && ((p2 = array[j - 1])->aaltIndex > a)) {
			array[j] = p2;
			j--;
		}
		array[j] = p1;
	}
}

/* For the alternate glyphs in a  GSUBAlternate rule, sort them */
/* in the order of the GNode aaltIndex field, aka the order that */
/* the features were named in the aalt definition. Alts that are */
/* explicitly defined in the aalt feature have an index of -1. */
/* See aaltAddAlternates. */
static void aaltRuleSort(hotCtx g, GNode **list) {
	featCtx h = g->ctx.feat;
	long i;
	GNode *p = *list;
	short flags = (*list)->flags;

	h->sortTmp.cnt = 0;

	/* Copy over pointers  */
	for (; p != NULL; p = p->nextCl) {
		*dnaNEXT(h->sortTmp) = p;
	}

	aaltInsertSort(h->sortTmp.array, h->sortTmp.cnt);

	/* Move pointers around */
	for (i = 0; i < h->sortTmp.cnt - 1; i++) {
		h->sortTmp.array[i]->nextCl = h->sortTmp.array[i + 1];
	}
	h->sortTmp.array[i]->nextCl = NULL;

	*list = h->sortTmp.array[0];

	(*list)->flags = flags;
}

static int CDECL matchTarget(const void *key, const void *value, void *ctx) {
	GID a = *((GID *)key);
	GID b = ((RuleInfo *)value)->targ->gid;

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

/* Input GNodes will be copied. Ranges of 1-1, single 1-1, or 1 from n. (Only
   first glyph position in targ will be looked at)  */
/* The aaltIndex is set to non-zero here in order to facilitate later sorting */
/* of the target glyph alternates, by the order that the current feature file is */
/* named in the aalt feature. If the alt glyph is defined explicitly in the */
/* aalt feature, via a 'sub' directive, it gets an index of AALT_INDEX, aka, -1, */
/* so that it will sort before all the alts from included features. */

static void aaltAddAlternates(GNode *targ, GNode *repl) {
	int range = targ->nextCl != NULL;

	/* Duplicate repl if a single glyph: */
	if (targ->nextCl != NULL && repl->nextCl == NULL) {
		featExtendNodeToClass(g, repl, featGetGlyphClassCount(g, targ) - 1);
	}

	for (; targ != NULL; targ = targ->nextCl, repl = repl->nextCl) {
		size_t inx;
		GNode *replace;

		/* Start new accumulator for targ->gid, if it doesn't already exist */
		if (!ctuLookup(&targ->gid, h->aalt.rules.array,
		               h->aalt.rules.cnt, sizeof(RuleInfo), matchTarget,
		               &inx, g)) {
			RuleInfo *rule = INSERT(h->aalt.rules, inx);

			rule->targ = featSetNewNode(g, targ->gid);
			rule->repl = NULL;
		}

		/* Add alternates to alt set, checking for uniqueness & preserving
		   order */
		replace = repl;
		for (; replace != NULL; replace = range ? NULL : replace->nextCl) {
			GNode **lastClass;
			GNode *p;

			if (glyphInClass(replace->gid,
			                 &h->aalt.rules.array[inx].repl, &lastClass)) {
				short aaltTagIndex;

				aaltTagIndex = tagIndex(h->curr.feature, &h->aalt.features);
				p = *lastClass;

				if (aaltTagIndex < p->aaltIndex) {
					p->aaltIndex = aaltTagIndex;
				}
			}
			else {
				*lastClass = featSetNewNode(g, replace->gid);
				p = *lastClass;

				if (h->curr.feature == aalt_) {
					p->aaltIndex = AALT_INDEX;
				}
				else {
					p->aaltIndex = tagIndex(h->curr.feature, &h->aalt.features);
				}
			}
		}
	}
}

/* Return 1 if this rule is to be treated specially */

static int aaltCheckRule(int type, GNode *targ, GNode *repl) {
	if (h->curr.feature == aalt_) {
		if (type == GSUBSingle || type == GSUBAlternate) {
			aaltAddAlternates(targ, repl);
			featRecycleNodes(g, targ);
			featRecycleNodes(g, repl);
		}
		else {
			featMsg(hotWARNING, "Only single and alternate "
			        "substitutions are allowed within an 'aalt' feature");
		}
		return 1;
	}

	return 0;
}

static int CDECL cmpLkpType(const void *first, const void *second) {
	GNode *a = ((RuleInfo *)first)->repl;
	GNode *b = ((RuleInfo *)second)->repl;

	/* Sort single sub rules before alt sub rules */
	if (IS_GLYPH(a) && IS_MULT_CLASS(b)) {
		return -1;
	}
	else if (IS_MULT_CLASS(a) && IS_GLYPH(b)) {
		return 1;
	}
	else if (IS_MULT_CLASS(a) && IS_MULT_CLASS(b)) {
		/* Sort alt sub rules by targ GID */
		GID aTargGID = ((RuleInfo *)first)->targ->gid;
		GID bTargGID = ((RuleInfo *)second)->targ->gid;

		if (aTargGID < bTargGID) {
			return -1;
		}
		else if (aTargGID > bTargGID) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}

/* Create aalt at end of feature file processing */

static void aaltCreate(void) {
	long i;
	long j;
	long nSingleRules;
	long nAltRules;
	long nInterSingleRules = 0; /* Rules that interact with the alt rules; i.e.
	                               its repl GID == an alt rule's targ GID */
	Label labelSingle    = LAB_UNDEF;   /* Init to suppress compiler warning */
	Label labelAlternate = LAB_UNDEF;   /* Init to Suppress compiler warning */

	if (h->aalt.rules.cnt == 0) {
		return;
	}

	/* Sort single subs before alt subs, sort alt subs by targ GID */
	qsort(h->aalt.rules.array, h->aalt.rules.cnt, sizeof(RuleInfo),
	      cmpLkpType);
	for (i = 0; i < h->aalt.rules.cnt && IS_GLYPH(h->aalt.rules.array[i].repl);
	     i++) {
	}
	nSingleRules = i;
	nAltRules = h->aalt.rules.cnt - nSingleRules;

	if (nSingleRules && nAltRules) {
		/* If the repl GID of a SingleSub rule is the same as an AltSub rule's
		   targ GID, then the SingleSub rule sinks to the bottom of the
		   SingleSub rules, and becomes part of the AltSub rules section: */
		for (i = nSingleRules - 1; i >= 0; i--) {
			size_t inx;
			RuleInfo *rule = &h->aalt.rules.array[i];
			if (ctuLookup(&rule->repl->gid, &h->aalt.rules.array[nSingleRules],
			              nAltRules, sizeof(RuleInfo), matchTarget,
			              &inx, g)) {
/*			if (bsearch(&rule->repl->gid, &h->aalt.rules.array[nSingleRules],
                        nAltRules, sizeof(RuleInfo), matchTarget))
 */
				long iDest = nSingleRules - 1 - nInterSingleRules;
				if (i != iDest) {
					RuleInfo *dest = &h->aalt.rules.array[iDest];
					RuleInfo tmp = *dest;
					*dest = *rule;
					*rule = tmp;
				}
				nInterSingleRules++;
			}
		}
		if (nInterSingleRules) {
			nSingleRules -= nInterSingleRules;
			nAltRules    += nInterSingleRules;
			DF(1, (stderr, "# aalt: %ld SingleSub rule%s moved to AltSub "
			       "lookup to prevent lookup interaction\n", nInterSingleRules,
			       nInterSingleRules == 1 ? "" : "s"));
		}
	}

	/* Add default lang sys if none specified */
	if (h->langSys.cnt == 0) {
		addLangSys(DFLT_, dflt_, 0);
		if (h->fFlags & FF_LANGSYS_MODE) {
			hotMsg(g, hotWARNING,
			       "[internal] aalt language system unspecified");
		}
	}

	GSUBFeatureBegin(g, h->langSys.array[0].script,
	                 h->langSys.array[0].lang, aalt_);

	/* --- Feed in single subs --- */
	if (nSingleRules) {
		labelSingle = featGetNextAnonLabel(g);
		GSUBLookupBegin(g, GSUBSingle, 0, labelSingle, h->aalt.useExtension, 0);
		for (i = 0; i < nSingleRules; i++) {
			RuleInfo *rule = &h->aalt.rules.array[i];
			GSUBRuleAdd(g, rule->targ, rule->repl);
		}
		GSUBLookupEnd(g, aalt_);
	}

	/* --- Feed in alt subs --- */
	if (nAltRules) {
		labelAlternate = featGetNextAnonLabel(g);
		GSUBLookupBegin(g, GSUBAlternate, 0, labelAlternate,
		                h->aalt.useExtension, 0);
		for (i = 0; i < nAltRules; i++) {
			RuleInfo *rule = &h->aalt.rules.array[nSingleRules + i];
			aaltRuleSort(g, &rule->repl); /* sort alts in order of feature def
			                                 in aalt feature */
			GSUBRuleAdd(g, rule->targ, rule->repl);
		}
		GSUBLookupEnd(g, aalt_);
	}

	GSUBFeatureEnd(g);

	/* Also register these lookups under any other lang systems, if needed: */
	for (j = 1; j < h->langSys.cnt; j++) {
		GSUBFeatureBegin(g, h->langSys.array[j].script,
		                 h->langSys.array[j].lang, aalt_);

		if (nSingleRules) {
			GSUBLookupBegin(g, GSUBSingle, 0, (Label)(labelSingle | REF_LAB),
			                h->aalt.useExtension, 0);
			GSUBLookupEnd(g, aalt_);
		}
		if (nAltRules) {
			GSUBLookupBegin(g, GSUBAlternate, 0,
			                (Label)(labelAlternate | REF_LAB),
			                h->aalt.useExtension, 0);
			GSUBLookupEnd(g, aalt_);
		}

		GSUBFeatureEnd(g);
	}
}

static void storeRuleInfo(GNode *targ, GNode *repl) {
	if (h->curr.tbl == GPOS_ || repl == NULL) {
		return;     /* No GPOS or except clauses */
	}
	if (h->curr.feature == aalt_ ||
	    tagDefined(h->curr.feature, &h->aalt.features)) {
		/* Now check if lkpType is appropriate */

		switch (h->curr.lkpType) {
			case GSUBSingle:
			case GSUBAlternate:
				break;

			case GSUBChain:
				/* Go to first marked glyph (guaranteed to be present) */
				for (; !(targ->flags & FEAT_MARKED); targ = targ->nextSeq) {
				}
				if (targ->nextSeq != NULL && targ->nextSeq->flags & FEAT_MARKED) {
					return; /* Ligature sub-substitution */
				}
				break;

			default:
				return;
		}

		if (!(h->curr.feature == aalt_)) {
			int i = tagIndex(h->curr.feature, &h->aalt.features);
			if (i >= 0) {
				h->aalt.usedFeatures.array[i] = 1;
			}
		}
		aaltAddAlternates(targ, repl);
	}
}

/* Called at end-of-file of main feature file */

void featWrapUpFeatFile(void) {
}

/* --- Rule additions --- */

/* Current fea, scr, lan, lkpFlag already set. Need to set label. */

static void prepRule(Tag newTbl, int newlkpType, GNode *targ, GNode *repl) {
	int accumDFLTLkps = 1;

	/* Update current tbl and lkpType if needed */
	if (h->curr.tbl != newTbl) {
		h->curr.tbl = newTbl;
	}
	if (h->curr.lkpType != newlkpType) {
		h->curr.lkpType = newlkpType;
	}

	/* Proceed in language sytem mode for this feature if (1) languagesystem
	   specified at global scope and (2) this feature did not start with an
	   explicit script or language statement */
	if (!(h->fFlags & FF_LANGSYS_MODE) &&
	    h->gFlags & GF_SEEN_LANGSYS &&
	    !(h->fFlags & FF_SEEN_SCRLANG)) {
		h->fFlags |= FF_LANGSYS_MODE;

		/* We're now poised at the start of the first rule of this feature.
		   Start registering rules under the first language system (register
		   under the rest of the language systems at end of feature). */
		tagAssign(h->langSys.array[0].script, scriptTag, 0);
		tagAssign(h->langSys.array[0].lang, languageTag, 0);
	}

	/* Set the label */
	if (h->currNamedLkp != LAB_UNDEF) {
		/* This is a named lkp or a reference (to an anon/named) lkp */
		if (h->curr.label != h->currNamedLkp) {
			h->curr.label = h->currNamedLkp;
		}
		else if (h->curr.script == h->prev.script &&
		         h->curr.feature == h->prev.feature) {
			/* Store lkp state only once per lookup. If the script/feature has
			   changed but the named label hasn't, accumDFLTLkps shd be 1. */
			accumDFLTLkps = 0;
		}
	}
	else {
		/* This is an anonymous rule */
		if (h->prev.script == h->curr.script &&
		    h->prev.language == h->curr.language &&
		    h->prev.feature == h->curr.feature &&
		    h->prev.tbl == h->curr.tbl &&
		    h->prev.lkpType == h->curr.lkpType
		    ) {
			/* I don't test againster lookupFlag and markSetIndex, as I do not want to silently
			   start a new lookup becuase these change */
			if (h->endOfNamedLkpOrRef) {
				h->curr.label = featGetNextAnonLabel(g);
			}
			else {
				h->curr.label = h->prev.label;
				accumDFLTLkps = 0;  /* Store lkp state only once per lookup */
			}
		}
		else {
			h->curr.label = featGetNextAnonLabel(g);
		}
	}

	/* --- Everything is now in current state for the new rule --- */

	/* Check that rules in a named lkp block are of the same tbl, lkpType,
	   lkpFlag */
	if (h->currNamedLkp != LAB_UNDEF && IS_NAMED_LAB(h->curr.label) &&
	    !IS_REF_LAB(h->curr.label) && h->prev.label == h->curr.label) {
		if (h->curr.tbl != h->prev.tbl || h->curr.lkpType !=
		    h->prev.lkpType) {
			featMsg(hotFATAL, "Lookup type different from previous "
			        "rules in this lookup block");
		}
		else if (h->curr.lkpFlag != h->prev.lkpFlag) {
			featMsg(hotFATAL, "Lookup flags different from previous "
			        "rules in this block");
		}
		else if (h->curr.markSetIndex != h->prev.markSetIndex) {
			featMsg(hotFATAL, "Lookup flag UseMarkSetIndex different from previous "
			        "rules in this block");
		}
	}

	/* Reset DFLTLkp accumulator at feature change or start of DFLT run */
	if ((h->curr.feature != h->prev.feature) ||
	    (h->curr.script != h->prev.script)) {
		h->DFLTLkps.cnt = 0;
	}

	/* stop accumulating script specific defaults once a language other than 'dflt' is seen */
	if (accumDFLTLkps && h->curr.language != dflt_) {
		accumDFLTLkps = 0;
	}

	if (accumDFLTLkps) {
		/* Save for possible inclusion later in lang-specific stuff */
		*dnaNEXT(h->DFLTLkps) = h->curr;
	}

	/* Store, if applicable, for GPOSContext and aalt creation */
	if ((!IS_REF_LAB(h->curr.label)) && (repl != NULL)) {
		storeRuleInfo(targ, repl);
	}

	/* If h->prev != h->curr */
	if (h->prev.script != h->curr.script ||
	    h->prev.language != h->curr.language ||
	    h->prev.feature != h->curr.feature ||
	    h->prev.tbl != h->curr.tbl ||
	    h->prev.lkpType != h->curr.lkpType ||
	    h->prev.label != h->curr.label) {
		short useExtension;

		closeFeaScrLan(&h->prev);

		/* Determine whether extension lookups are to be used */
		if (h->currNamedLkp != LAB_UNDEF && IS_NAMED_LAB(h->curr.label)) {
			NamedLkp *lkp = lab2NamedLkp(h->currNamedLkp);
			if (lkp == NULL) {
				hotMsg(g, hotFATAL, "[internal] label not found\n");
			}
			useExtension = lkp->useExtension;
		}
		else {
			useExtension = 0;
		}

		/* Initiate calls to GSUB/GPOS */
		if (h->curr.tbl == GSUB_) {
			GSUBFeatureBegin(g, h->curr.script, h->curr.language,
			                 h->curr.feature);
			GSUBLookupBegin(g, h->curr.lkpType, h->curr.lkpFlag,
			                h->curr.label, useExtension, h->curr.markSetIndex);
		}
		else if (h->curr.tbl == GPOS_) {
			GPOSFeatureBegin(g, h->curr.script, h->curr.language,
			                 h->curr.feature);
			GPOSLookupBegin(g, h->curr.lkpType, h->curr.lkpFlag,
			                h->curr.label, useExtension, h->curr.markSetIndex);
		}

		/* If LANGSYS mode, snapshot lookup info for registration under other
		   language systems at end of feature */
		if (h->fFlags & FF_LANGSYS_MODE) {
			LookupInfo *lkp = dnaNEXT(h->lookup);

			lkp->tbl          = h->curr.tbl;
			lkp->lkpType      = h->curr.lkpType;
			lkp->lkpFlag      = h->curr.lkpFlag;
			lkp->markSetIndex     = h->curr.markSetIndex;
			lkp->label        = h->curr.label;
			lkp->useExtension = useExtension;
		}

		/* --- COPY CURRENT TO PREVIOUS STATE: */
		h->prev = h->curr;
	}
	else {
		/* current state sate is the same for everything except maybe lkpFlag and markSetIndex */
		if (h->curr.lkpFlag != h->prev.lkpFlag) {
			featMsg(hotFATAL, "Lookup flags different from previous "
			        "rules in this block");
		}
		else if (h->curr.markSetIndex != h->prev.markSetIndex) {
			featMsg(hotFATAL, "Lookup flag UseMarkSetIndex different from previous "
			        "rules in this block");
		}
	}
}

/* Get count of number of nodes in glyph class */

int featGetGlyphClassCount(hotCtx g, GNode *gc) {
	int cnt = 0;
	for (; gc != NULL; gc = gc->nextCl) {
		cnt++;
	}
	return cnt;
}

/* Return 1 if targ and repl have same number of glyphs in class, else
   emit error message and return 0 */

static int compareGlyphClassCount(GNode *targ, GNode *repl, int isSubrule) {
	int nTarg = featGetGlyphClassCount(g, targ);
	int nRepl = featGetGlyphClassCount(g, repl);

	if (nTarg == nRepl) {
		return 1;
	}
	featMsg(hotERROR, "Target glyph class in %srule doesn't have the same"
	        " number of elements as the replacement class; the target has %d,"
	        " the replacement, %d",
	        isSubrule ? "sub-" : "", nTarg, nRepl);
	return 0;
}

static void addGSUB(int lkpType, GNode *targ, GNode *repl) {
	if (aaltCheckRule(lkpType, targ, repl)) {
		return;
	}

	prepRule(GSUB_, lkpType, targ, repl);

	GSUBRuleAdd(g, targ, repl);

	wrapUpRule();
}

#if 0 /* xxx */
/* Returns 1 if all sequence nodes in pattern are marked */
static int isAllMarkedPattern(GNode *node) {
	for (; node != NULL; node = node->nextSeq) {
		if (!(node->flags & FEAT_MARKED)) {
			return 0;   /* This node is unmarked */
		}
	}
	return 1;
}

#endif

/* Return 1 if node is an unmarked pattern of 2 or more glyphs; glyph classes
   may be present */
static int isUnmarkedSeq(GNode *node) {
	if (node == NULL || node->flags & FEAT_HAS_MARKED || node->nextSeq == NULL) {
		return 0;
	}
	return 1;
}

/* Return 1 if node is an unmarked pattern of 2 or more glyphs, and no glyph
   classes are present */
static int isUnmarkedGlyphSeq(GNode *node) {
	if (!isUnmarkedSeq(node)) {
		return 0;
	}

	for (; node != NULL; node = node->nextSeq) {
		if (node->nextCl != NULL) {
			return 0;   /* A glyph class is present */
		}
	}
	return 1;
}

static void addFeatureNameParam(hotCtx g, unsigned short nameID) {
	prepRule(GSUB_, GSUBFeatureNameParam, NULL, NULL);
    
	GSUBAddFeatureMenuParam(g, &nameID);
    
	wrapUpRule();
}

static void addCVNameID(unsigned int nameID, int labelID)
{
    switch(labelID)
    {
            
        case cvUILabelEnum:
        {
            if (h->cvParameters.FeatUILabelNameID != 0)
            {
                featMsg(hotERROR, "A Character Variant parameter table can have only one FeatUILabelNameID entry.");
            }
            h->cvParameters.FeatUILabelNameID = h->featNameID;
            break;
        }
            
        case cvToolTipEnum:
        {
            if (h->cvParameters.FeatUITooltipTextNameID != 0)
            {
                featMsg(hotERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            h->cvParameters.FeatUITooltipTextNameID = h->featNameID;
            break;
        }
            
        case cvSampletextEnum:
        {
            if (h->cvParameters.SampleTextNameID != 0)
            {
                featMsg(hotERROR, "A Character Variant parameter table can have only one SampleTextNameID entry.");
            }
            h->cvParameters.SampleTextNameID = h->featNameID;
            break;
        }
            
        case kCVParameterLabelEnum:
        {
            h->cvParameters.NumNamedParameters++;
            if (h->cvParameters.FirstParamUILabelNameID == 0)
            {
                h->cvParameters.FirstParamUILabelNameID = h->featNameID;
            }
            break;
        }
    }

    h->featNameID = 0;
}

static void addCVParametersCharValue(unsigned long uv)
{
    unsigned long *uvp = dnaNEXT(h->cvParameters.charValues);
    *uvp = uv;
   
}

static void addCVParam(hotCtx g) {
	prepRule(GSUB_, GSUBCVParam, NULL, NULL);
    
	GSUBAddCVParam(g, &h->cvParameters);
    
	wrapUpRule();
}

/* Validate targ and repl for GSUBSingle. targ and repl come in with nextSeq
   NULL and repl not marked. If valid return 1, else emit error message(s) and
   return 0 */

static int validateGSUBSingle(hotCtx g, GNode *targ, GNode *repl, int isSubrule) {
	int valid = 1;
	if (!isSubrule && targ->flags & FEAT_MARKED) {
		featMsg(hotERROR, "Target must not be marked in this rule");
		valid = 0;
	}

	if (IS_GLYPH(targ)) {
		if (!IS_GLYPH(repl)) {
			featMsg(hotERROR, "Replacement in %srule must be a single glyph",
			        isSubrule ? "sub-" : "");
			valid = 0;
		}
	}
	else if (repl->nextCl != NULL &&
	         !compareGlyphClassCount(targ, repl, isSubrule)) {
		valid = 0;
	}

	return valid;
}

/* Validate targ and repl for GSUBMultiple. targ comes in with nextSeq NULL and
   repl NULL or with nextSeq non-NULL (and unmarked). If valid return 1, else
   emit error message(s) and return 0 */

static int validateGSUBMultiple(hotCtx g, GNode *targ, GNode *repl,
                                int isSubrule) {
	int valid = 1;

	if (!isSubrule && targ->flags & FEAT_MARKED) {
		featMsg(hotERROR, "Target must not be marked in this rule");
		valid = 0;
	}

	if (!(IS_GLYPH(targ) && isUnmarkedGlyphSeq(repl))) {
		featMsg(hotERROR, "Invalid multiple substitution rule");
		valid = 0;
	}
	return valid;
}

/* Validate targ and repl for GSUBAlternate. repl is not marked. If valid
   return 1, else emit error message(s) and return 0 */

static int validateGSUBAlternate(hotCtx g, GNode *targ, GNode *repl,
                                 int isSubrule) {
	int valid = 1;

	if (!isSubrule && targ->flags & FEAT_MARKED) {
		featMsg(hotERROR, "Target must not be marked in this rule");
		valid = 0;
	}

	if (!IS_UNMARKED_GLYPH(targ)) {
		featMsg(hotERROR, "Target of alternate substitution %srule must be a"
		        " single unmarked glyph", isSubrule ? "sub-" : "");
		valid = 0;
	}
	if (!IS_CLASS(repl)) {
		featMsg(hotERROR, "Replacement of alternate substitution %srule must "
		        "be a glyph class", isSubrule ? "sub-" : "");
		valid = 0;
	}
	return valid;
}

/* Validate targ and repl for GSUBLigature. targ comes in with nextSeq
   non-NULL and repl is unmarked. If valid return 1, else emit error message(s)
   and return 0 */

static int validateGSUBLigature(hotCtx g, GNode *targ, GNode *repl,
                                int isSubrule) {
	int valid = 1;

	if (!isSubrule && targ->flags & FEAT_HAS_MARKED) {
		featMsg(hotERROR, "Target must not be marked in this rule");
		valid = 0;
	}

	if (!(IS_GLYPH(repl))) {
		featMsg(hotERROR, "Invalid ligature %srule replacement",
		        isSubrule ? "sub-" : "");
		valid = 0;
	}
	return valid;
}

/* Analyse GSUBChain targ and repl. Return 1 if valid, else 0 */

static int validateGSUBReverseChain(hotCtx g, GNode *targ, GNode *repl) {
	int state;
	GNode *p;
	GNode *input = NULL; /* first node  of input sequence */
	int nMarked = 0;
	GNode *m = NULL;    /* Suppress optimizer warning */

	if (repl == NULL) {
		/* Except clause */
		if (targ->flags & FEAT_HAS_MARKED) {
			/* Mark backtrack */
			for (p = targ; p != NULL && !(p->flags & FEAT_MARKED);
			     p = p->nextSeq) {
				p->flags |= FEAT_BACKTRACK;
			}
			for (; p != NULL && p->flags & FEAT_MARKED; p = p->nextSeq) {
				p->flags |= FEAT_INPUT;
			}
		}
		else {
			/* If clause is unmarked: first char is INPUT, rest LOOKAHEAD */
			targ->flags |= FEAT_INPUT;
			p = targ->nextSeq;
		}
		/* Mark rest of glyphs as lookahead */
		for (; p != NULL; p = p->nextSeq) {
			if (p->flags & FEAT_MARKED) {
				featMsg(hotERROR, "ignore clause may have at most one run "
				        "of marked glyphs");
				return 0;
			}
			else {
				p->flags |= FEAT_LOOKAHEAD;
			}
		}
		return 1;
	}

	/* At most one run of marked positions supported, for now */
	for (p = targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_MARKED) {
			if (++nMarked == 1) {
				m = p;
			}
		}
		else if (p->nextSeq != NULL && p->nextSeq->flags & FEAT_MARKED
		         && nMarked > 0) {
			featMsg(hotERROR, "Reverse contextual GSUB rule may must have one and only one glyph or class marked for replacement");
			return 0;
		}
	}

	/* If nothing is marked, mark everything [xxx?] */
	if (nMarked == 0) {
		m = targ;
		for (p = targ; p != NULL; p = p->nextSeq) {
			p->flags |= FEAT_MARKED;
			nMarked++;
		}
	}
	/* m now points to beginning of marked run */

#if 0
	targ->flags |= (nMarked == 1) ? FEAT_HAS_SINGLE_MARK
		: FEAT_HAS_LIGATURE_MARK;
#endif

	if (repl->nextSeq != NULL) {
		featMsg(hotERROR, "Reverse contextual GSUB replacement sequence may have only one glyph or class");
		return 0;
	}

	if (nMarked != 1) {
		featMsg(hotERROR, "Reverse contextual GSUB rule may must have one and only one glyph or class marked for replacement");
		return 0;
	}

	/* Divide into backtrack, input, and lookahead (xxx ff interface?). For
	   now, input is marked glyphs */
	state = FEAT_BACKTRACK;
	for (p = targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_MARKED) {
			if (input == NULL) {
				input = p;
			}
			state = FEAT_INPUT;
		}
		else if (state != FEAT_BACKTRACK) {
			state = FEAT_LOOKAHEAD;
		}
		p->flags |= state;
	}

	if (!compareGlyphClassCount(input, repl, 0)) {
		return 0;
	}


	return 1;
}

/* Analyse GSUBChain targ and repl. Return 1 if valid, else 0 */

static int validateGSUBChain(hotCtx g, GNode *targ, GNode *repl) {
	int state;
	GNode *p;
	int nMarked = 0;
	GNode *m = NULL;    /* Suppress optimizer warning */
	int hasDirectLookups = (targ->flags & FEAT_LOOKUP_NODE);

	if (targ->flags & FEAT_IGNORE_CLAUSE) {
		/* Except clause */
		if (targ->flags & FEAT_HAS_MARKED) {
			/* Mark backtrack */
			for (p = targ; p != NULL && !(p->flags & FEAT_MARKED);
			     p = p->nextSeq) {
				p->flags |= FEAT_BACKTRACK;
			}
			for (; p != NULL && p->flags & FEAT_MARKED; p = p->nextSeq) {
				p->flags |= FEAT_INPUT;
			}
		}
		else {
			/* If clause is unmarked: first char is INPUT, rest LOOKAHEAD */
			targ->flags |= FEAT_INPUT;
			p = targ->nextSeq;
		}
		/* Mark rest of glyphs as lookahead */
		for (; p != NULL; p = p->nextSeq) {
			if (p->flags & FEAT_MARKED) {
				featMsg(hotERROR, "ignore clause may have at most one run "
				        "of marked glyphs");
				return 0;
			}
			else {
				p->flags |= FEAT_LOOKAHEAD;
			}
		}
		return 1;
	}
	else if ((repl == NULL) && (!hasDirectLookups)) {
		featMsg(hotERROR, "contextual substitution clause must have a replacement rule or direct lookup reference.");
		return 0;
	}

	if (hasDirectLookups) {
		if (repl != NULL) {
			featMsg(hotERROR, "contextual substitution clause cannot both have a replacement rule and a direct lookup reference.");
			return 0;
		}
		if (!(targ->flags & FEAT_HAS_MARKED)) {
			featMsg(hotERROR, "The  direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
			return 0;
		}
	}

	/* At most one run of marked positions supported, for now */
	for (p = targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_MARKED) {
			if (++nMarked == 1) {
				m = p;
			}
		}
		else if (p->lookupLabel >= 0) {
			featMsg(hotERROR, "The  direct lookup reference in a contextual substitution clause must be marked as part of a contextual input sequence.");
			return 0;
		}
		else if (p->nextSeq != NULL && p->nextSeq->flags & FEAT_MARKED
		         && nMarked > 0) {
			featMsg(hotERROR, "Unsupported contextual GSUB target sequence");
			return 0;
		}
	}

	/* If nothing is marked, mark everything [xxx?] */
	if (nMarked == 0) {
		m = targ;
		for (p = targ; p != NULL; p = p->nextSeq) {
			p->flags |= FEAT_MARKED;
			nMarked++;
		}
	}
	/* m now points to beginning of marked run */

	if (repl) {
		if (repl->nextSeq != NULL) {
			featMsg(hotERROR, "Unsupported contextual GSUB replacement sequence");
			return 0;
		}

		if (nMarked == 1) {
			if (IS_GLYPH(m) && IS_CLASS(repl)) {
				featMsg(hotERROR, "Contextual alternate rule not yet supported");
				return 0;
			}

			if (!validateGSUBSingle(g, m, repl, 1)) {
				return 0;
			}
		}
		else if (nMarked > 1) {
			/* Ligature run may contain classes, but only with a single repl */
			if (!validateGSUBLigature(g, m, repl, 1)) {
				return 0;
			}
		}
	}

	/* Divide into backtrack, input, and lookahead (xxx ff interface?). For
	   now, input is marked glyphs */
	state = FEAT_BACKTRACK;
	for (p = targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_MARKED) {
			state = FEAT_INPUT;
		}
		else if (state != FEAT_BACKTRACK) {
			state = FEAT_LOOKAHEAD;
		}
		p->flags |= state;
	}

	return 1;
}

static void addGPOS(int lkpType, GNode *targ, char *fileName, long lineNum, int anchorCount, AnchorMarkInfo *anchorMarkInfo) {
	prepRule(GPOS_, (targ->flags & FEAT_HAS_MARKED) ? GPOSChain : lkpType, targ, NULL);

	GPOSRuleAdd(g, lkpType, targ, fileName, lineNum,  anchorCount, anchorMarkInfo);

	wrapUpRule();
}

static void addSub(GNode *targ, GNode *repl, int lkpType, int targLine) {
	GNode *nextNode = targ;

	for (nextNode = targ; nextNode != NULL; nextNode = nextNode->nextSeq) {
		if (nextNode->flags & FEAT_MARKED) {
			targ->flags |= FEAT_HAS_MARKED;
			if ((lkpType != GSUBReverse) && (lkpType != GSUBChain)) {
				lkpType = GSUBChain;
			}
			break;
		}
	}


	if ((repl == NULL) || lkpType == GSUBChain || (targ->flags & FEAT_IGNORE_CLAUSE)) {
		/* Chain sub exceptions (further analysed below).
		   "sub f i by fi;" will be here if there was an "except" clause */

		if (!g->hadError) {
			if (validateGSUBChain(g, targ, repl)) {
				lkpType = GSUBChain;
				addGSUB(GSUBChain, targ, repl);
			}
		}
	}

	else if (lkpType == GSUBReverse) {
		if (validateGSUBReverseChain(g, targ, repl)) {
			addGSUB(GSUBReverse, targ, repl);
		}
	}
	else if (lkpType == GSUBAlternate) {
		if (validateGSUBAlternate(g, targ, repl, 0)) {
			addGSUB(GSUBAlternate, targ, repl);
		}
	}

	else if (targ->nextSeq == NULL && (repl == NULL || repl->nextSeq != NULL)) {
		if (validateGSUBMultiple(g, targ, repl, 0)) {
			addGSUB(GSUBMultiple, targ, repl);
		}
	}

	else if (targ->nextSeq == NULL && repl->nextSeq == NULL) {
		if (validateGSUBSingle(g, targ, repl, 0)) {
			addGSUB(GSUBSingle, targ, repl);
		}
	}

	else {
		GNode *next;
		GNode **gcInsert;
		GNode *head;

		if (validateGSUBLigature(g, targ, repl, 0)) {
			/* add glyphs to lig and component classes, in case we need to make
			   a default GDEF table. Note that we may make a lot of duplicated. These get weeded out later.
			   The components are linked by the next->nextSeq fields. For each component*/
			gcInsert = gcOpen(kDEFAULT_COMPONENTCLASS_NAME); /* looks up class, making if needed. Sets h->gcInsert to adress of nextCl of last node, and returns it.*/
			next = targ;
			while (next != NULL) {
				if (next->nextCl != NULL) {
					/* the current target node is a glyph class. Need to add all members of the class to the kDEFAULT_COMPONENTCLASS_NAME. */
					head = gcLookup(kDEFAULT_COMPONENTCLASS_NAME); /* Finds the named class, returns the ptr to head node. Does not set  h->gcInsert */
					h->gcInsert = featGlyphClassCopy(g, h->gcInsert, next); /* copies contents of next to gcInsert, creating new nodes.  returns adress of last ndoe's nextCl.*/
				}
				else {
					gcAddGlyph(next->gid); /* adds new node at h->gcInsert, sets h->gcInsert to address of new node's nextCl */
				}
				next = next->nextSeq;
			}
			gcInsert = gcOpen(kDEFAULT_LIGATURECLASS_NAME);
			next = repl;
			while (next != NULL) {
				if (next->nextCl != NULL) {
					gcInsert = featGlyphClassCopy(g, h->gcInsert, next);
				}
				else {
					gcAddGlyph(next->gid);
				}
				next = next->nextSeq;
			}
			addGSUB(GSUBLigature, targ, repl);
		}
	}
}

/* Analyse featValidateGPOSChain targ metrics. Return 1 if valid, else 0 */
/* Also sets flags in backtrack and look-ahead sequences */

int featValidateGPOSChain(hotCtx g, GNode *targ, int lkpType) {
	int state;
	GNode *p;
	int nMarked = 0;
	int nNodesWithMetrics = 0;
	int seenTerminalMetrics = 0; /* special case for kern -like syntax in a contextual sub statement. */
	int nBaseGlyphs = 0;
	int nLookupRefs = 0;
	GNode *m = NULL;    /* Suppress optimizer warning */
	GNode *lastNode = NULL; /* Suppress optimizer warning */

	/* At most one run of marked positions supported, for now */
	for (p = targ; p != NULL; p = p->nextSeq) {
		lastNode = p;

		if (p->flags & FEAT_MARKED) {
			if (++nMarked == 1) {
				m = p;
			}
			if (p->metricsInfo != NULL) {
				nNodesWithMetrics++;
			}
			if (p->lookupLabel >= 0) {
				nLookupRefs++;
			}
		}
		else {
			if (p->lookupLabel >= 0) {
				featMsg(hotERROR, "Lookup references are allowed only in the input sequencee: this is the sequence of marked glyphs.");
			}

			if (p->flags & FEAT_IS_MARK_NODE) {
				featMsg(hotERROR, "The final mark class must be marked as part of the input sequence: this is the sequence of marked glyphs.");
			}

			if ((p->nextSeq != NULL) && (p->nextSeq->flags & FEAT_MARKED) && (nMarked > 0)) {
				featMsg(hotERROR, "Unsupported contextual GPOS target sequence: only one run of marked glyphs  is supported.");
				return 0;
			}

			/* We actiually do allow  a value records after the last glyoh node, if there is only one marked glyph */
			if (p->metricsInfo != NULL) {
				if (nMarked == 0) {
					featMsg(hotERROR, "Positioning cannot be applied in the bactrack glyph sequence, before the marked glyph sequence.");
					return 0;
				}
				if ((p->nextSeq != NULL) || (nMarked > 1)) {
					featMsg(hotERROR, "Positioning values are allowed only in the marked glyph sequence, or after the final glyph node when only one glyph node is marked.");
					return 0;
				}

				if ((p->nextSeq == NULL) && (nMarked == 1)) {
					seenTerminalMetrics = 1;
				}

				nNodesWithMetrics++;
			}
		}

		if (p->flags & FEAT_IS_BASE_NODE) {
			nBaseGlyphs++;
			if ((lkpType == GPOSCursive) && (!(p->flags & FEAT_MARKED))) {
				featMsg(hotERROR, "The base glyph or glyph class must be marked as part of the input sequence in a contextual pos cursive statement.");
			}
		}
	}

	/* m now points to beginning of marked run */

	/* Check for special case of a single marked node, with one or more lookahead nodes, and a single value record attached to the last node */
	if (seenTerminalMetrics) {
		m->metricsInfo = lastNode->metricsInfo;
		lastNode->metricsInfo = NULL;
	}

	if (targ->flags & FEAT_IGNORE_CLAUSE) {
		/* An ignore clause is always contextual. If not marked, then mark the first glyph in the sequence */
		if (nMarked == 0) {
			targ->flags |= FEAT_MARKED;
			nMarked = 1;
		}
	}
	else if ((nNodesWithMetrics == 0) && (nBaseGlyphs == 0) && (nLookupRefs == 0)) {
		featMsg(hotERROR, "Contextual positioning rule must specify a positioning value or a mark attachent rule ro a direct lookup reference.");
		return 0;
	}

	/* Divide into backtrack, input, and lookahead (xxx ff interface?). For
	   now, input is marked glyphs */
	state = FEAT_BACKTRACK;
	for (p = targ; p != NULL; p = p->nextSeq) {
		if (p->flags & FEAT_MARKED) {
			state = FEAT_INPUT;
		}
		else if (state != FEAT_BACKTRACK) {
			state = FEAT_LOOKAHEAD;
		}
		p->flags |= state;
	}


	return 1;
}

/* Add feature parameters */

static void addFeatureParam(hotCtx g, short *params, unsigned short numParams) {
	switch (h->curr.feature) {
		case size_:
			prepRule(GPOS_, GPOSFeatureParam, NULL, NULL);

			GPOSAddSize(g, params, numParams);

			wrapUpRule();

			break;

		default:
			featMsg(hotERROR, "Feature parameter not supported for feature"
			        " '%c%c%c%c'", TAG_ARG(h->curr.feature));
	}
}

/* ----------- ----------- -------------------------
   Targ        Metrics     Lookup type
   ----------- ----------- -------------------------
   g           m m m m     Single                   g: single glyph
   c           m m m m     Single (range)           c: glyph class (2 or more)
                                                    x: g or c
   g g         m           Pair (specific)
   g c         m           Pair (class)
   c g         m           Pair (class)
   c c         m           Pair (class)

   x x' x x'   m m         Chain ctx (not supported)
   ----------- ----------- -------------------------

   Add a pos rule from the feature file.
    Can assume targ is at least one node. If h->metrics.cnt == 0
   then targ is an "ignore position" pattern. */

static MetricsInfo *getNextMetricsInfoRec(void) {
	return dnaNEXT(h->metricsInfo);
}

static void addBaseClass(GNode *targ, char *defaultClassName) {
	/* Find base node in a possibly contextual sequence,
	   and add it to the default base glyph class */
	GNode *nextNode = targ;

	/* If it is contextual, first find the base glyph */
	if (targ->flags & FEAT_HAS_MARKED) {
		while (nextNode != NULL) {
			if (nextNode->flags & FEAT_IS_BASE_NODE) {
				break;
			}
			nextNode = nextNode->nextSeq;
		}
	}

	if (nextNode->flags & FEAT_IS_BASE_NODE) {
		GNode **gcInsert = gcOpen(defaultClassName);
		if (nextNode->nextCl != NULL) {
			gcInsert = featGlyphClassCopy(g, gcInsert, nextNode);
		}
		else {
			gcAddGlyph(nextNode->gid);
			gcInsert = h->gcInsert;
		}
	}
}

static void addPos(GNode *targ, int type, int enumerate) {
	int glyphCount = 0;
	int markedCount = 0;
	int lookupLabelCnt = 0;
	GNode *next_targ = NULL;
	GNode *copyHeadNode;

	if (enumerate) {
		targ->flags |= FEAT_ENUMERATE;
	}

	/* count glyphs, figure out if is single, pair, or context. */
	next_targ = targ;
	while (next_targ != NULL) {
		glyphCount++;
		if (next_targ->flags & FEAT_MARKED) {
			markedCount++;
		}
		if (next_targ->lookupLabel >= 0) {
			lookupLabelCnt++;
			if (!next_targ->flags & FEAT_MARKED) {
				featMsg(hotERROR, "the glyph which precedes the 'lookup' keyword must be marked as part of the contextual input sequence");
			}
		}
		next_targ = next_targ->nextSeq;
	}

	/* The ignore statement can only be used with contextual lookups.
	   If no glyph is marked, then mark the first. */
	if (targ->flags & FEAT_IGNORE_CLAUSE) {
		type = GPOSChain;
		if (markedCount == 0) {
			markedCount = 1;
			targ->flags |= FEAT_MARKED;
		}
	}

	if (markedCount > 0) {
		targ->flags |= FEAT_HAS_MARKED; /* used later to decide if stuff is contextual */
	}
	if ((glyphCount == 2) && (markedCount == 0) && (type == GPOSSingle)) {
		type = GPOSPair;
	}
	else if (enumerate) {
		featMsg(hotERROR, "\"enumerate\" only allowed with pair positionings,");
	}


	if (type == GPOSSingle) {
		addGPOS(GPOSSingle, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		/* These nodes are recycled in GPOS.c */
	}
	else if (type == GPOSPair) {
		next_targ = targ->nextSeq;
		if (targ->nextCl != NULL) {
			/* In order to sort and remove duplicates, I need to copy the original class definition. This is because
			   class definitions may be used for sequences as well as real classes, and sorting and removing duplicates from
			   the original class is a bad idea.
			 */
			featGlyphClassCopy(h->g, &copyHeadNode, targ);
			targ = copyHeadNode;
			featGlyphClassSort(g, &targ, 1, 1);
			targ->nextSeq = next_targ; /* featGlyphClassCopy zeroes the  nextSeq field in all nodes.*/
		}
		if (next_targ->nextCl != NULL) {
			/* In order to sort and remove duplicates, I need to copy the original class definition. This is because
			   class definitions may be used for sequences as well as real classes, and sorting and removing duplicates from
			   the original class is a bad idea.
			 */
			featGlyphClassCopy(h->g, &copyHeadNode, next_targ);
			next_targ = copyHeadNode;
			featGlyphClassSort(g, &next_targ, 1, 1);
			targ->nextSeq = next_targ; /* featGlyphClassSort may change which node in the next_targ class is the head node.  */
		}
		/*addGPOSPair(targ, second, enumerate);*/
		addGPOS(GPOSPair, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		/* These nodes are recycled in GPOS.c due to some complicated copying of nodes. */
	}
	else if (type == GPOSCursive) {
		if (h->anchorMarkInfo.cnt != 2) {
			featMsg(hotERROR, "The 'cursive' statement requires two anchors. This has %d. Skipping rule.", h->anchorMarkInfo.cnt);
		}
		else if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) || (targ->nextSeq != NULL))) {
			featMsg(hotERROR, "This statement has contextual glyphs around the cursive statement, but no glyphs are marked as part of the input sequence. Skipping rule.", h->anchorMarkInfo.cnt);
		}
		else {
			addGPOS(GPOSCursive, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		}
		/* These nodes are recycled in GPOS.c */
	}
	else if (type == GPOSMarkToBase) {
		addBaseClass(targ, kDEFAULT_BASECLASS_NAME);
		if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) ||  ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
			featMsg(hotERROR, "This statement has contextual glyphs around the base-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.", h->anchorMarkInfo.cnt);
		}
		addGPOS(GPOSMarkToBase, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		/* These nodes are recycled in GPOS.c */
	}
	else if (type == GPOSMarkToLigature) {
		addBaseClass(targ, kDEFAULT_LIGATURECLASS_NAME);
		if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) ||  ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
			featMsg(hotERROR, "This statement has contextual glyphs around the ligature statement, but no glyphs are marked as part of the input sequence. Skipping rule.", h->anchorMarkInfo.cnt);
		}

		/* With mark to ligatures, we may see the same mark class on different components, which leads to duplicate GID's in the contextual mark node
		   As a result, we need to sort and get rid of duplicates */
		if (targ->flags & FEAT_HAS_MARKED) {
			/* find the mark node */
			GNode *markClassNode = targ;
			GNode *prevNode = NULL;

			while (markClassNode != NULL) {
				if (markClassNode->flags & FEAT_IS_MARK_NODE) {
					break;
				}
				prevNode = markClassNode;
				markClassNode = markClassNode->nextSeq;
			}
			if (markClassNode->flags & FEAT_IS_MARK_NODE) {
				featGlyphClassCopy(h->g, &copyHeadNode, markClassNode);
				markClassNode = copyHeadNode;
				featGlyphClassSort(g, &markClassNode, 1, 0);     /* changes value of markClassNode. I specify to NOT warn of duplicates, beacuse they can happen with correct syntax. */
				prevNode->nextSeq = markClassNode;
			}
		}

		addGPOS(GPOSMarkToLigature, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		/* These nodes are recycled in GPOS.c */
	}
	else if (type == GPOSMarkToMark) {
		addBaseClass(targ, kDEFAULT_MARKCLASS_NAME);
		if ((!(targ->flags & FEAT_HAS_MARKED)) && ((!(targ->flags & FEAT_IS_BASE_NODE)) ||  ((targ->nextSeq != NULL) && (targ->nextSeq->nextSeq != NULL)))) {
			featMsg(hotERROR, "This statement has contextual glyphs around the mark-to-mark statement, but no glyphs are marked as part of the input sequence. Skipping rule.", h->anchorMarkInfo.cnt);
		}
		addGPOS(GPOSMarkToMark, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		/* These nodes are recycled in GPOS.c */
	}
	else if (type == GPOSChain) {
		/* is contextual */
		if (markedCount == 0) {
			featMsg(hotERROR, "The 'lookup' keyword can be used only in a contextual statement. At least one glyph in the sequence must be marked. Skipping rule.");
		}
		else {
			featValidateGPOSChain(g, targ, type);
			addGPOS(GPOSChain, targ, INCL.file, zzline, h->anchorMarkInfo.cnt, &h->anchorMarkInfo.array[0]);
		}
		/* These nodes are recycled in GPOS.c, as they are used in the fill phase, some tim eafter this function returns. */
	}
	else {
		featMsg(hotERROR, "This rule type is not recognized..");
	}
}

static void setFontRev(char *rev) {
	char *fraction = 0;
	double minor = 0;

	long major = strtol(rev, &fraction, 10);

	if (major < 1 || major > 255) {
		featMsg(hotWARNING, "Major version number not in range 1 .. 255");
	}

	if ((fraction != 0) && (strlen(fraction) > 0)) {
		short strLen = strlen(fraction);
		minor = strtod(fraction, NULL);

		if (strLen != 4) {
			double version = major + minor;
			featMsg(hotWARNING, "head FontRevision entry <%s> should have 3 fractional decimal places. Stored as <%.3f>", rev, version);
		}
	}
	else {
		featMsg(hotWARNING, "head FontRevision entry <%d> should have 3 fractional decimal places; it now has none.", major);
	}



	/* Convert to Fixed */
	g->font.version.otf = (Fixed)((major + minor) * 65536.0);
}

static int featFileExists(hotCtx g) {
	featCtx h = g->ctx.feat;

	if (h->featFileExists == -1) {
		hotMsg(g, hotFATAL, "[internal] featFileExists not set");
	}
	return h->featFileExists;
}

static Label getNextNamedLkpLabel(void) {
	if (h->namedLkpLabelCnt > FEAT_NAMED_LKP_END) {
		featMsg(hotFATAL, "[internal] maximum number of named lookups reached:"
		        " %d", FEAT_NAMED_LKP_END - FEAT_NAMED_LKP_BEG + 1);
	}
	return h->namedLkpLabelCnt++;
}

Label featGetNextAnonLabel(hotCtx g) {
	if (h->anonLabelCnt > FEAT_ANON_LKP_END) {
		hotMsg(g, hotFATAL, "[internal] maximum number of lookups reached:"
		       " %d", FEAT_ANON_LKP_END - FEAT_ANON_LKP_BEG + 1);
	}
	return h->anonLabelCnt++;
}

/* Validate and set a particular flag/subcomponent of the lookupflag. markType
   only used if attr is otlMarkAttachmentType */


static void setLkpFlagAttribute(unsigned short *val, unsigned attr,
                                unsigned short markAttachClassIndex) {
	if (attr > 1) {
		/* 1 is the RTL flag - does not need to set this */
		h->gFlags |= GF_SEEN_IGNORE_CLASS_FLAG;
	}

	if (attr == otlMarkAttachmentType) {
		if (markAttachClassIndex == 0) {
			featMsg(hotERROR, "must specify non-zero MarkAttachmentType value");
		}
		else if (*val & attr) {
			featMsg(hotERROR,
			        "MarkAttachmentType already specified in this statement");
		}
		else {
			*val |= (markAttachClassIndex & 0xFF) << 8;
		}
	}
	else if (attr == otlUseMarkFilteringSet) {
		if (*val & attr) {
			featMsg(hotERROR,
			        "UseMarkSetType already specified in this statement");
		}
		h->curr.markSetIndex = markAttachClassIndex;
		*val |= attr;
	}
	else {
		if (*val & attr) {
			featMsg(hotWARNING,
			        "\"%s\" repeated in this statement; ignoring", zzlextext);
		}
		else {
			*val |= attr;
		}
	}
}

static void setLkpFlag(unsigned short flagVal) {
	unsigned short flag = flagVal;
	if (h->curr.feature == aalt_ || h->curr.feature == size_) {
		featMsg(hotERROR,
		        "\"lookupflag\" use not allowed in 'aalt' or 'size' feature");
	}
	else if (flag == h->curr.lkpFlag) {
		/* Statement has no effect */
	}
	else {
		h->curr.lkpFlag = flag;
	}
	/* if UseMarkSet, then the markSetIndex is set in setLkpFlagAttribute() */
}

/* Named or anon */

static void callLkp(State *st) {
	Label lab = st->label;

#if HOT_DEBUG
	if (g->font.debug & HOT_DB_FEAT_2) {
		if (h->curr.tbl == GSUB_) {
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "# call lkp ");
		if (IS_REF_LAB(lab)) {
			fprintf(stderr, "REF->");
		}
		if (IS_NAMED_LAB(lab)) {
			fprintf(stderr, "<%s>", lab2NamedLkp(lab)->name);
		}
		else if (IS_ANON_LAB(lab)) {
			fprintf(stderr, "<ANON>");
		}
		else {
			hotMsg(g, hotFATAL, "undefined label");
		}
		fprintf(stderr, "[label=%x]", lab);
/*		fprintf(stderr, " : %s:%d:%d",
            st->tbl == GPOS_ ? "GPOS" : "GSUB", st->lkpType, st->lkpFlag); */
		fprintf(stderr, "(but with s'%c%c%c%c' l'%c%c%c%c' f'%c%c%c%c') :\n",
		        TAG_ARG(h->curr.script), TAG_ARG(h->curr.language),
		        TAG_ARG(h->curr.feature));
	}
#endif

	/* Use the scr, lan, fea that's *currently* set.
	   Use the *original* lkpFlag and, of course, tbl and lkpType.
	   Copy the lookup label, set its reference bit.
	   Simulate the first rule in the original block simply by calling
	   prepRule() ! */

	h->currNamedLkp = (Label)(lab | REF_LAB);   /* As in:  lookup <name> {  */
	h->curr.lkpFlag = st->lkpFlag;
	h->curr.markSetIndex = st->markSetIndex;
	prepRule(st->tbl, st->lkpType, NULL, NULL);
	/* No actual rules will be fed into GPOS/GSUB */
	wrapUpRule();
	h->currNamedLkp = LAB_UNDEF;    /* As in:  } <name>;        */
	h->endOfNamedLkpOrRef = 1;
}

static void useLkp(char *name) {
	NamedLkp *lkp = name2NamedLkp(name);

	if (h->curr.feature == aalt_) {
		featMsg(hotERROR, "\"lookup\" use not allowed in 'aalt' feature");
		return;
	}
	else {
		int i = tagIndex(h->curr.feature, &h->aalt.features);
		if (i >= 0) {
			h->aalt.usedFeatures.array[i] = 1;
		}
	}


	if (h->curr.feature == size_) {
		featMsg(hotERROR, "\"lookup\" use not allowed anymore in 'size'"
		        " feature; " USE_LANGSYS_MSG);
		return;
	}

	if (lkp == NULL) {
		featMsg(hotERROR, "lookup name \"%s\" not defined", name);
	}
	else {
		callLkp(&lkp->state);
	}
}

static Label featGetLabelIndex(char *name) {
	NamedLkp *curr = NULL;

	curr = name2NamedLkp(name);
	if (curr == NULL) {
		featMsg(hotFATAL, "lookup name \"%s\" already defined", name);
		return -1;
	}
	return curr->state.label;
}

static void checkLkpName(char *name, int linenum, int atStart, int isStandAlone) {
	if (isStandAlone) {
		/* This is a stand-alone lookup, used outside of a feature block. Add it to the dummy feature
		   'A\0\0A' */
		if (atStart) {
			checkTag((Tag)TAG_STAND_ALONE, featureTag, atStart);
			checkTag((Tag)TAG_STAND_ALONE, scriptTag, atStart);
		}
		else {
			checkTag((Tag)TAG_STAND_ALONE, featureTag, atStart);
		}
	}
	else {
		if (h->curr.feature == aalt_) {
			if (atStart) {
				featMsg(hotERROR, "\"lookup\" use not allowed in 'aalt' feature");
			}
			return;
		}
		if (h->curr.feature == size_) {
			if (atStart) {
				featMsg(hotERROR, "\"lookup\" use not allowed anymore in 'size'"
				        " feature; " USE_LANGSYS_MSG);
			}
			return;
		}
	}
	if (atStart) {
		NamedLkp *new;

		DF(2, (stderr, "# at start of named lookup %s\n", name));
		if (name2NamedLkp(name) != NULL) {
			featMsg(hotFATAL, "lookup name \"%s\" already defined", name);
		}

		h->currNamedLkp = getNextNamedLkpLabel();

		/* Store in named lookup array */
		new = dnaNEXT(h->namedLkp);
		new->useExtension = 0;
		copyStr(g, &new->name, name);
		/* State will be recorded at end of block section, below */
	}
	else {
		NamedLkp *curr = lab2NamedLkp(h->currNamedLkp);

		DF(2, (stderr, "# at end of named lookup %s\n", name));
		if (curr == NULL) {
			hotMsg(g, hotFATAL, "[internal] label not found\n");
		}
		if (strcmp(curr->name, name) != 0) {
			zzerr("doesn't match starting label of block");
		}

		/* Store state for future lookup references to it */
		curr->state = h->curr;

		h->currNamedLkp = LAB_UNDEF;
		h->endOfNamedLkpOrRef = 1;
	}
}

/* Include dflt rules in lang-specific behavior if includeDFLT != 0 */

static void includeDFLT(int includeDFLT, int langChange, int seenOldDFLT) {
	if (seenOldDFLT && (!h->seenOldDFLT)) {
		h->seenOldDFLT = 1;
		featMsg(hotWARNING, "Use of includeDFLT and excludeDFLT tags has been deprecated. It will work, but please use 'include_dflt' and 'exclude_dflt' tags instead.");
	}
	/* Set value */
	if (langChange) {
		h->include_dflt = includeDFLT;
	}
	else if (includeDFLT != h->include_dflt) {
		zzerr("can't change whether a language should include dflt rules once "
		      "this has already been indicated");
	}

	if (includeDFLT) {
		/* Include dflt rules for current script and explicit language statement.
		   Languages which don't exclude_dflt get the feature-level defaults at the end of the feature.
		 */
		long i;

		for (i = 0; i < h->DFLTLkps.cnt; i++) {
			State *st = &h->DFLTLkps.array[i];
			callLkp(st);
		}
	}
	else {
		/* Defaults have been explicitly excluded from the this language. Find the matching  script-lang lang-sys,
		   and set the flag so it won;t add the feature level defautls at the end of the feature.
		 */
		int i;

		for (i = 0; i < h->langSys.cnt; i++) {
			LangSys *curLangSys = &h->langSys.array[i];
			if (curLangSys->script ==  h->curr.script && curLangSys->lang ==  h->curr.language) {
				curLangSys->excludeDflt = 1;
				break;
			}
		}     /* end for all lang sys */
	}
}

#endif /* HOT_FEAT_SUPPORT */

/* --------------------------- Standard Functions -------------------------- */

/* Caution: g and h setup different from other modules if HOT_FEAT_SUPPORT */

void featNew(hotCtx hot) {
#if !HOT_FEAT_SUPPORT
	featCtx h;
#endif /* !HOT_FEAT_SUPPORT */
	h = MEM_NEW(hot, sizeof(struct featCtx_));
	memset(h, 0, sizeof(struct featCtx_));
	blockListInit(h, 3000, 6000);

#if HOT_FEAT_SUPPORT
	zzerr = zzerrCustom;

	h->featFileExists = -1;

	dnaINIT(hot->dnaCtx, h->langSys, 5, 10);
	dnaINIT(hot->dnaCtx, h->lookup, 7, 20);

	dnaINIT(hot->dnaCtx, h->stack, 5, 5);

	dnaINIT(hot->dnaCtx, h->script, 4, 4);
	dnaINIT(hot->dnaCtx, h->language, 4, 4);
	dnaINIT(hot->dnaCtx, h->feature, 15, 15);
	dnaINIT(hot->dnaCtx, h->table, 5, 5);
	dnaINIT(hot->dnaCtx, h->namedLkp, 5, 20);
	dnaINIT(hot->dnaCtx, h->DFLTLkps, 5, 20);

	dnaINIT(hot->dnaCtx, h->aalt.features, 10, 20);
	dnaINIT(hot->dnaCtx, h->aalt.usedFeatures, 10, 20);
	dnaINIT(hot->dnaCtx, h->aalt.rules, 50, 200);
	dnaINIT(hot->dnaCtx, h->cvParameters.charValues, 10, 10);

 	dnaINIT(hot->dnaCtx, h->sortTmp, 100, 200);

	hashInit(h);

	dnaINIT(hot->dnaCtx, h->anonData.data, 2000, 4000);

	h->returnMode.include = h->returnMode.tag = START;
	dnaINIT(hot->dnaCtx, h->nameString, 256, 256);
	dnaINIT(hot->dnaCtx, h->metricsInfo, 4, 10);
	dnaINIT(hot->dnaCtx, h->anchorMarkInfo, 4, 10);
	dnaINIT(hot->dnaCtx, h->prod, 100, 2000);
	dnaINIT(hot->dnaCtx, h->markClasses, 8, 8);
	dnaINIT(hot->dnaCtx, h->anchorDefs, 8, 8);
	dnaINIT(hot->dnaCtx, h->valueDefs, 8, 8);


#endif /* HOT_FEAT_SUPPORT */

	/* Link contexts */
	h->g = hot;
	hot->ctx.feat = h;
#if HOT_FEAT_SUPPORT
	g = hot;
#endif /* HOT_FEAT_SUPPORT */
}

/* Emit report on any deprecated feature file syntax encountered */

static void reportOldSyntax(void) {
	if (h->syntax.numExcept > 0) {
		int one = h->syntax.numExcept == 1;

		hotMsg(g, hotNOTE,
		       "There %s %hd instance%s of the deprecated \"except\" syntax in the"
		       " feature file. Though such statements are processed correctly by"
		       " this parser for backward compatibility, please update them to the"
		       " newer \"ignore substitute\" syntax. For example, change \"except a @LET"
		       " sub a by a.end;\" to \"ignore sub a @LET; sub a' by a.end;\". (Note that"
		       " the second rule is now required to be marked to identify it as a Chain"
		       " Contextual and not a Single Substitution rule.)",
		       one ? "is" : "are",
		       h->syntax.numExcept,
		       one ? "" : "s");
	}
}

int featFill(hotCtx g) {
#if HOT_FEAT_SUPPORT
	featCtx h = g->ctx.feat;
	State initState = {
		TAG_UNDEF,
		TAG_UNDEF,
		TAG_UNDEF,
		TAG_UNDEF,
		0,
		0,
		0,
		LAB_UNDEF
	};

	/* --- Initializations */

	h->offset = h->nextoffset = 0;
	h->data = '\0';

	h->curr = h->prev = h->aalt.state = initState;

	h->include_dflt = 1;
	h->seenOldDFLT = 0;
	h->endOfNamedLkpOrRef = 0;
	h->currNamedLkp = LAB_UNDEF;
	h->namedLkpLabelCnt = FEAT_NAMED_LKP_BEG;
	h->anonLabelCnt     = FEAT_ANON_LKP_BEG;

	/* --- Read feature file */
	h->featFileExists = featOpenIncludeFile(g, NULL);
	if (h->featFileExists != 0) {
		featCloseIncludeFile(g, 1);
	}

	reportOldSyntax();

	aaltCreate();

	/* if an IgnoreMark lookup flags have been used,  of if any mark classes have been used, we may need to make a default GDEF and fix lookup gflag indices */
	if (h->gFlags & (GF_SEEN_IGNORE_CLASS_FLAG | GF_SEEN_MARK_CLASS_FLAG)) {
		creatDefaultGDEFClasses();
	}

	reportUnusedaaltTags(h);


	hotQuitOnError(g);

#endif /* HOT_FEAT_SUPPORT */

	return 1;
}

void featReuse(hotCtx g) {
#if HOT_FEAT_SUPPORT
	featCtx h = g->ctx.feat;

	long i;

	h->featFileExists = -1;

	h->gFlags = h->fFlags = 0;
	h->langSys.cnt = 0;
	h->lookup.cnt = 0;

	/* freeList remains same. blockList remains same. */

	h->script.cnt = 0;
	h->language.cnt = 0;
	h->feature.cnt = 0;
	h->table.cnt = 0;

	h->aalt.useExtension = 0;
	h->aalt.features.cnt = 0;
	h->aalt.usedFeatures.cnt = 0;
	for (i = 0; i < h->aalt.usedFeatures.cnt; i++) {
		h->aalt.usedFeatures.array[i] = 0;
	}

	for (i = 0; i < h->namedLkp.cnt; i++) {
		MEM_FREE(g, h->namedLkp.array[i].name);
	}
	h->namedLkp.cnt = 0;

	h->DFLTLkps.cnt = 0;
	h->aalt.rules.cnt = 0;
    h->cvParameters.charValues.cnt = 0;
    
	hashFree(h);
	hashInit(h);

	h->anonData.data.cnt = 0;
	h->anonData.iLineStart = 0;

	h->returnMode.include = h->returnMode.tag = START;

	h->stack.cnt = 0;
	h->featNameID = 0;

	h->nameString.cnt = 0;
	h->metricsInfo.cnt = 0;
	h->anchorMarkInfo.cnt = 0;
	h->prod.cnt = 0;
	h->markClasses.cnt = 0;
	h->anchorDefs.cnt = 0;
	h->valueDefs.cnt = 0;

#endif /* HOT_FEAT_SUPPORT */
}

void featFree(hotCtx g) {
#if HOT_FEAT_SUPPORT || HOT_DEBUG
	featCtx h = g->ctx.feat;
#endif
#if HOT_FEAT_SUPPORT
	long i;
#endif

#if HOT_DEBUG
	if (DF_LEVEL >= 2) {
		nodeStats(h);
	}
#endif
	blockListFree(g);       /* Once for entire program */

#if HOT_FEAT_SUPPORT
	dnaFREE(h->stack);

	dnaFREE(h->langSys);
	dnaFREE(h->lookup);

	dnaFREE(h->script);
	dnaFREE(h->language);
	dnaFREE(h->feature);
	dnaFREE(h->table);
	dnaFREE(h->aalt.features);
	dnaFREE(h->aalt.usedFeatures);

	for (i = 0; i < h->namedLkp.cnt; i++) {
		MEM_FREE(g, h->namedLkp.array[i].name);
	}
	dnaFREE(h->namedLkp);

	dnaFREE(h->DFLTLkps);

	dnaFREE(h->aalt.rules);
	dnaFREE(h->cvParameters.charValues);

	dnaFREE(h->sortTmp);
	hashFree(h);

	dnaFREE(h->anonData.data);

	dnaFREE(h->nameString);
	dnaFREE(h->metricsInfo);
	dnaFREE(h->anchorMarkInfo);
	dnaFREE(h->prod);
	dnaFREE(h->markClasses);
	dnaFREE(h->anchorDefs);
	dnaFREE(h->valueDefs);
#endif /* HOT_FEAT_SUPPORT */

	MEM_FREE(g, g->ctx.feat);
}

int featDefined(hotCtx g, Tag feat) {
#if HOT_FEAT_SUPPORT
	featCtx h = g->ctx.feat;
	return featFileExists(g) && tagDefined(feat, &h->feature);
#else
	return 0;
#endif
}

#if HOT_FEAT_SUPPORT
#if HOT_DEBUG
/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, stateDump, prodDump, hashStats);
}

#endif
#endif /* HOT_FEAT_SUPPORT */
