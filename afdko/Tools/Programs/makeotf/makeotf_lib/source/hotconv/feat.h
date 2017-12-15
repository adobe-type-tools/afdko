/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/


#ifndef FEAT_H
#define FEAT_H

#include "common.h"

#if HOT_DEBUG
#define DF_LEVEL (g->font.debug & HOT_DB_FEAT_2 ? 2 : \
	              (g->font.debug & HOT_DB_FEAT_1 ? 1 : 0))
#define DF(L, p)  do { if (DF_LEVEL >= L) { fprintf p; } \
} \
	while (0)
#else
#define DF(L, p)
#endif

/* Preferred to 0 for proper otl sorting. This can't conflict with a valid
   tag since tag characters must be in ASCII 32-126. */
#define TAG_UNDEF   0xFFFFFFFF
#define TAG_STAND_ALONE 0x01010101 /* Feature, script. language tags used for stand-alone lookups */

#define MAX_FEAT_PARAM_NUM 256

/* Labels: Each lookup is identified by a label. There are 2 kinds of hotlib
   lookups:

   1. Named: these are named by the font editor in the feature file. e.g.
      "lookup ZERO {...} ZERO;"
   2. Anonymous: all other lookups. They are automatically generated.

   You can tell which kind of lookup a label refers to by the value of the
   label: use the macros IS_NAMED_LAB() and IS_ANON_LAB().

   Both kinds of lookups can be referred to later on, when sharing them; e.g.
   specified by the font editor explicitly by "lookup ZERO;" or implicitly by
   "language DEU;" where the hotlib includes the default lookups. These lookup
   "references" are stored as the original loookup's label with bit 15 set.
 */

#define FEAT_NAMED_LKP_BEG  0
#define FEAT_NAMED_LKP_END  0x1FFF
#define FEAT_ANON_LKP_BEG   (FEAT_NAMED_LKP_END + 1)
#define FEAT_ANON_LKP_END   0x7FFE

#define LAB_UNDEF           0xFFFF

#define REF_LAB         (1 << 15)
#define IS_REF_LAB(L)   ((L) != LAB_UNDEF && (L)&REF_LAB)
#define IS_NAMED_LAB(L) (((L)&~REF_LAB) >= FEAT_NAMED_LKP_BEG && \
	                     ((L)&~REF_LAB) <= FEAT_NAMED_LKP_END)
#define IS_ANON_LAB(L)  (((L)&~REF_LAB) >= FEAT_ANON_LKP_BEG && \
	                     ((L)&~REF_LAB) <= FEAT_ANON_LKP_END)

typedef unsigned short Label;



/* --- Linked list support --- */

typedef struct AnchorRec_ {
	unsigned int format;
	short x;
	short y;
	unsigned short pointIndex;
}  AnchorRec;

#define kMaxAnchors 16
typedef struct GNode_ GNode;

typedef struct {
	char cnt;
	short metrics[4];
} MetricsInfo;

typedef struct {
	short int x;
	short int y;
	unsigned short contourpoint;
	unsigned int format;
	GNode *markClass;
	int markClassIndex;
	int componentIndex;
} AnchorMarkInfo;


struct GNode_ {
	/* On first node in pattern: */
#define FEAT_HAS_MARKED        (1 << 0) /* Pat has at least one marked node */
#define FEAT_USED_MARK_CLASS   (1 << 8) /* [This marked class as been used in a pos statement - not allowed to add new glyphs.] */
#define FEAT_IGNORE_CLAUSE     (1 << 9) /* Pat is an ignore clause */
#define FEAT_LOOKUP_NODE        (1 << 12) /* Pattern uses  direct lookup reference */

	/* This particular node is ... */
#define FEAT_MARKED            (1 << 1) /* Marked */
#define FEAT_GCLASS            (1 << 2) /* Is glyph class (useful for singleton
	                                       classes) */
#define FEAT_BACKTRACK         (1 << 3) /* Part of backtrack sequence */
#define FEAT_INPUT             (1 << 4) /* Part of input sequence */
#define FEAT_LOOKAHEAD         (1 << 5) /* Part of lookahead sequence */
#define FEAT_MISC              (1 << 6) /* [For use in local algorithms] */
#define FEAT_ENUMERATE         (1 << 7)
#define FEAT_IS_BASE_NODE      (1 << 10) /* Node is the base glyph in a mark attachment lookup. */
#define FEAT_IS_MARK_NODE      (1 << 11) /* Node is the mark glyph in a mark attachment lookup. */
	unsigned short flags;
	GID gid;
	GNode *nextSeq;     /* next element in sequence, or mark class node id in a base glyph or mark glyph class. */
	GNode *nextCl;      /* next element of class */
	signed aaltIndex;  /* index of contributing feature, in order of the aalt definitions.
	                    * Used only within aalCreate.
	                    */
	MetricsInfo *metricsInfo;
	int lookupLabel;
	char *markClassName;
	AnchorMarkInfo markClassAnchorInfo; /* Used obnly be mark class definitions */
};


/* --- Standard functions --- */

void featNew(hotCtx g);
int featFill(hotCtx g);
void featReuse(hotCtx g);
void featFree(hotCtx g);

/* --- Supplementary functions --- */
int featDefined(hotCtx g, Tag feat);

GNode *featSetNewNode(hotCtx g, GID gid);
void featRecycleNodes(hotCtx g, GNode *node);
GNode **featGlyphClassCopy(hotCtx g, GNode **dst, GNode *src);

void featGlyphDump(hotCtx g, GID gid, int ch, int print);
void featGlyphClassDump(hotCtx g, GNode *gc, int ch, int print);
void featPatternDump(hotCtx g, GNode *pat, int ch, int print);

GNode **featPatternCopy(hotCtx g, GNode **dst, GNode *src, int num);

void featExtendNodeToClass(hotCtx g, GNode *node, int num);
int featGetGlyphClassCount(hotCtx g, GNode *gc);

#if HOT_FEAT_SUPPORT

unsigned featGetPatternLen(hotCtx g, GNode *pat);
void featGlyphClassSort(hotCtx g, GNode **list, int unique, int reportDups);
GNode ***featMakeCrossProduct(hotCtx g, GNode *pat, unsigned *n);

Label featGetNextAnonLabel(hotCtx g);
Label featGetLookupLabel(hotCtx g, GNode *sub, unsigned lkpFlags);

/* --- PCCTS scanner functions --- */
int featOpenIncludeFile(hotCtx g, char *filename);
int featCloseIncludeFile(hotCtx g, int closeBase);
char *featTrimParensSpaces(char *text);
void featWrapUpFeatFile(void);
void featSetIncludeReturnMode(int mode);
int featGetIncludeReturnMode(void);
void featSetTagReturnMode(int mode);
int featGetTagReturnMode(void);
int featAddAnonDataChar(char ch, int isEOL);
void featAddNameStringChar(char ch);
void featUnexpectedEOF(void);
int featValidateGPOSChain(hotCtx g, GNode *targ, int lookupType);

#define kDEFAULT_BASECLASS_NAME "FDK_BASE_CLASS"
#define kDEFAULT_LIGATURECLASS_NAME "FDK_LIGATURE_CLASS"
#define kDEFAULT_MARKCLASS_NAME "FDK_DEFAULT_MARK_CLASS"
#define kDEFAULT_COMPONENTCLASS_NAME "FDK_DEFAULT_COMPONENT_CLASS"

#endif /* HOT_FEAT_SUPPORT */

#endif /* FEAT_H */
