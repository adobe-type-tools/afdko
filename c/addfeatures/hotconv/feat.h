/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_FEAT_H_
#define ADDFEATURES_HOTCONV_FEAT_H_

#include <string>

#include "common.h"

#if HOT_DEBUG
#define DF_LEVEL ((g->font.debug & HOT_DB_FEAT_2) ? 2 : ((g->font.debug & HOT_DB_FEAT_1) ? 1 : 0))
#define DF(L, p)             \
    do {                     \
        if (DF_LEVEL >= L) { \
            fprintf p;       \
        }                    \
    } while (0)
#else
#define DF(L, p)
#endif

/* Preferred to 0 for proper otl sorting. This can't conflict with a valid
   tag since tag characters must be in ASCII 32-126. */
#define TAG_UNDEF 0xFFFFFFFF
#define TAG_STAND_ALONE 0x01010101  // Feature, script. language tags used for stand-alone lookups

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
   "references" are stored as the original lookup's label with bit 15 set.
 */

#define FEAT_NAMED_LKP_BEG 0
#define FEAT_NAMED_LKP_END 0x1FFF
#define FEAT_ANON_LKP_BEG (FEAT_NAMED_LKP_END + 1)
#define FEAT_ANON_LKP_END 0x7FFE

#define LAB_UNDEF 0xFFFF

#define REF_LAB (1 << 15)
#define IS_REF_LAB(L) ((L) != LAB_UNDEF && (L)&REF_LAB)
#define IS_NAMED_LAB(L) (((L) & ~REF_LAB) >= FEAT_NAMED_LKP_BEG && \
                         ((L) & ~REF_LAB) <= FEAT_NAMED_LKP_END)
#define IS_ANON_LAB(L) (((L) & ~REF_LAB) >= FEAT_ANON_LKP_BEG && \
                        ((L) & ~REF_LAB) <= FEAT_ANON_LKP_END)

typedef uint16_t Label;

struct CVParameterFormat {
    CVParameterFormat() {}
    CVParameterFormat(const CVParameterFormat &other) = delete;
    explicit CVParameterFormat(CVParameterFormat &&other) {
        FeatUILabelNameID = other.FeatUILabelNameID;
        FeatUITooltipTextNameID = other.FeatUITooltipTextNameID;
        SampleTextNameID = other.SampleTextNameID;
        NumNamedParameters = other.NumNamedParameters;
        FirstParamUILabelNameID = other.FirstParamUILabelNameID;
        other.FeatUILabelNameID = 0;
        other.FeatUITooltipTextNameID = 0;
        other.SampleTextNameID = 0;
        other.NumNamedParameters = 0;
        other.FirstParamUILabelNameID = 0;
        charValues.swap(other.charValues);
    };
    void swap(CVParameterFormat &&other) {
        std::swap(FeatUILabelNameID, other.FeatUILabelNameID);
        std::swap(FeatUITooltipTextNameID, other.FeatUITooltipTextNameID);
        std::swap(SampleTextNameID, other.SampleTextNameID);
        std::swap(NumNamedParameters, other.NumNamedParameters);
        std::swap(FirstParamUILabelNameID, other.FirstParamUILabelNameID);
        charValues.swap(other.charValues);
    }
    void reset() {
        FeatUILabelNameID = 0;
        FeatUITooltipTextNameID = 0;
        SampleTextNameID = 0;
        NumNamedParameters = 0;
        FirstParamUILabelNameID = 0;
        charValues.clear();
    }
    int size() { return 7 * sizeof(uint16_t) + 3 * charValues.size(); }
    uint16_t FeatUILabelNameID {0};
    uint16_t FeatUITooltipTextNameID {0};
    uint16_t SampleTextNameID {0};
    uint16_t NumNamedParameters {0};
    uint16_t FirstParamUILabelNameID {0};
    std::vector<uint32_t> charValues;
};

typedef struct GNode_ GNode;

typedef struct {
    int8_t cnt;
    short metrics[4];
} MetricsInfo;

#define METRICSINFOEMPTYPP { -1, { 0, 0, 0, 0 } }

struct AnchorMarkInfo {
    bool operator < (const AnchorMarkInfo &rhs) const {
        if (componentIndex != rhs.componentIndex)
            return componentIndex < rhs.componentIndex;
        if (markClassIndex != rhs.markClassIndex)
            return markClassIndex < rhs.markClassIndex;
        if (format != rhs.format)
            return format < rhs.format;
        if (x != rhs.x)
            return x < rhs.x;
        if (y != rhs.y)
            return y < rhs.y;
        if (format == 2 && contourpoint != rhs.contourpoint)
            return contourpoint < rhs.contourpoint;
        return false;
    }
    bool operator == (const AnchorMarkInfo &rhs) const {
        return componentIndex == rhs.componentIndex &&
               markClassIndex == rhs.markClassIndex &&
               format == rhs.format &&
               x == rhs.x && y == rhs.y &&
               (format != 2 || contourpoint == rhs.contourpoint);
    }
    int16_t x {0};
    int16_t y {0};
    uint16_t contourpoint {0};
    uint32_t format {0};
    GNode *markClass {nullptr};
    int32_t markClassIndex {0};
    int32_t componentIndex {0};
};

struct GNode_ {
    /* On first node in pattern: */
#define FEAT_HAS_MARKED (1 << 0)      /* Pat has at least one marked node */
#define FEAT_USED_MARK_CLASS (1 << 8) /* [This marked class as been used in a pos statement - not allowed to add new glyphs.] */
#define FEAT_IGNORE_CLAUSE (1 << 9)   /* Pat is an ignore clause */
#define FEAT_LOOKUP_NODE (1 << 12)    /* Pattern uses  direct lookup reference */

    /* This particular node is ... */
#define FEAT_MARKED (1 << 1)    /* Marked */
#define FEAT_GCLASS (1 << 2)    /* Is glyph class (useful for singleton classes) */
#define FEAT_BACKTRACK (1 << 3) /* Part of backtrack sequence */
#define FEAT_INPUT (1 << 4)     /* Part of input sequence */
#define FEAT_LOOKAHEAD (1 << 5) /* Part of lookahead sequence */
#define FEAT_MISC (1 << 6)      /* [For use in local algorithms] */
#define FEAT_ENUMERATE (1 << 7)
#define FEAT_IS_BASE_NODE (1 << 10) /* Node is the base glyph in a mark attachment lookup. */
#define FEAT_IS_MARK_NODE (1 << 11) /* Node is the mark glyph in a mark attachment lookup. */
    unsigned short flags;
    GID gid;
    GNode *nextSeq;   /* next element in sequence, or mark class node id in a base glyph or mark glyph class. */
    GNode *nextCl;    /* next element of class */
    signed aaltIndex; /* index of contributing feature, in order of the aalt definitions. */
                      /* Used only within aalCreate.                                      */
    MetricsInfo metricsInfo;
    int lookupLabelCount;
    int lookupLabels[255];
    char *markClassName;
    AnchorMarkInfo markClassAnchorInfo; /* Used only be mark class definitions */
};

/* --- Standard functions --- */

void featNew(hotCtx g);
void featFill(hotCtx g);
void featReuse(hotCtx g);
void featFree(hotCtx g);

/* --- Supplementary functions --- */
std::string featMsgPrefix(hotCtx g);
GNode *featSetNewNode(hotCtx g, GID gid);
void featRecycleNodes(hotCtx g, GNode *node);
GNode **featGlyphClassCopy(hotCtx g, GNode **dst, GNode *src);

void featGlyphDump(hotCtx g, GID gid, int ch, int print);
void featGlyphClassDump(hotCtx g, GNode *gc, int ch, int print);
void featPatternDump(hotCtx g, GNode *pat, int ch, int print);

GNode **featPatternCopy(hotCtx g, GNode **dst, GNode *src, int num);

void featExtendNodeToClass(hotCtx g, GNode *node, int num);
int featGetGlyphClassCount(hotCtx g, GNode *gc);

unsigned int featGetPatternLen(hotCtx g, GNode *pat);
void featGlyphClassSort(hotCtx g, GNode **list, int unique, int reportDups);
GNode **featMakeCrossProduct(hotCtx g, GNode *pat, uint32_t *n);

Label featGetNextAnonLabel(hotCtx g);

int featValidateGPOSChain(hotCtx g, GNode *targ, int lookupType);

#endif  // ADDFEATURES_HOTCONV_FEAT_H_
