/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#ifndef ADDFEATURES_HOTCONV_FEATCTX_H_
#define ADDFEATURES_HOTCONV_FEATCTX_H_

#include <cassert>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

#include "FeatParser.h"
#include "hotmap.h"

// Debugging message support
#if HOT_DEBUG
inline int DF_LEVEL(hotCtx g) {
    if (g->font.debug & HOT_DB_FEAT_2)
        return 2;
    else if (g->font.debug & HOT_DB_FEAT_1)
        return 1;
    else return 0;
}
#define DF(L, p)             \
    do {                     \
        if (DF_LEVEL(g) >= L) { \
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

// number of possible entries in list of Unicode blocks
#define kLenUnicodeList 128
// number of possible entries in list of code page numbers
#define kLenCodePageList 64

typedef uint16_t Label;

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

// This should technically be in GSUB but it's easier this way
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
    void swap(CVParameterFormat &other) {
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

class FeatVisitor;

class FeatCtx {
    friend class FeatVisitor;

 public:
    FeatCtx() = delete;
    explicit FeatCtx(hotCtx g);
    ~FeatCtx();

    void fill();

    // Implementations of "external" calls
    GNode *setNewNode(GID gid);
    void recycleNodes(GNode *node);

    void dumpGlyph(GID gid, int ch, bool print);
    void dumpGlyphClass(GNode *gc, int ch, bool print);
    void dumpPattern(GNode *pat, int ch, bool print);

    GNode **copyGlyphClass(GNode **dst, GNode *src);
    GNode **copyPattern(GNode **dst, GNode *src, int num);
    void extendNodeToClass(GNode *node, int num);
    static int getGlyphClassCount(GNode *gc);
    static unsigned int getPatternLen(GNode *pat);
    void sortGlyphClass(GNode **list, bool unique, bool reportDups);

    std::string msgPrefix();
    GNode **makeCrossProduct(GNode *pat, unsigned *n);
    bool validateGPOSChain(GNode *targ, int lookupType);
    Label getNextAnonLabel();

    // Utility
    Tag str2tag(const std::string &tagName);

    static inline bool is_glyph(GNode *p) {
        return p != nullptr && p->nextSeq == nullptr &&
               p->nextCl == nullptr &&
               !(p->flags & FEAT_GCLASS);
    }
    static inline bool is_class(GNode *p) {
        return p != nullptr && p->nextSeq == nullptr &&
               (p->nextCl != nullptr || (p->flags & FEAT_GCLASS));
    }
    static inline bool is_mult_class(GNode *p) {
        return p != nullptr && p->nextSeq == nullptr && p->nextCl != nullptr;
    }
    static inline bool is_unmarked_glyph(GNode *p) {
        return is_glyph(p) && !(p->flags & FEAT_HAS_MARKED);
    }
    static inline bool is_unmarked_class(GNode *p) {
        return is_class(p) && !(p->flags & FEAT_HAS_MARKED);
    }

#if HOT_DEBUG
    void nodeStats();
    void tagDump(Tag);
#endif
    static const int kMaxCodePageValue;
    static const int kCodePageUnSet;

 private:
    // GNode memory management and utilities
    struct BlockNode {
        GNode *data;
        BlockNode *next;
    };

    struct {
        BlockNode *first {nullptr};
        // Current BlockNode being filled
        BlockNode *curr {nullptr};
        // Index of next free GNode, relative to curr->data
        long cnt {0};
        long intl {3000};
        long incr {6000};
    } blockList;
    GNode *freelist {nullptr};
#if HOT_DEBUG
    long int nAdded2FreeList {0};
    long int nNewFromBlockList {0};
    long int nNewFromFreeList {0};

    void checkFreeNode(GNode *node);
#endif
    void addBlock();
    void freeBlocks();
    GNode *newNodeFromBlock();
    GNode *newNode();
    void recycleNode(GNode *pat);

    // Console message hooks
    struct {
        unsigned short numExcept {0};
    } syntax;
    std::string tokenStringBuffer;

    void CTL_CDECL featMsg(int msgType, const char *fmt, ...);
    void CTL_CDECL featMsg(int msgType, FeatVisitor *v,
                           antlr4::Token *t, const char *fmt, ...);
    const char *tokstr();
    void setIDText();
    void reportOldSyntax();

    // State flags
    enum gFlagValues { gNone = 0, seenFeature = 1<<0, seenLangSys = 1<<1,
                       seenGDEFGC = 1<<2, seenIgnoreClassFlag = 1<<3,
                       seenMarkClassFlag = 1<<4,
                       seenNonDFLTScriptLang = 1<<5 };
    unsigned int gFlags {gNone};

    enum fFlagValues { fNone = 0, seenScriptLang = 1<<0,
                       langSysMode = 1<<1 };
    unsigned int fFlags {fNone};

    // Glyphs
    GID mapGName2GID(const char *gname, bool allowNotdef);
    inline GID mapGName2GID(const std::string &gname, bool allowNotdef) {
        return mapGName2GID(gname.c_str(), allowNotdef);
    }
    GID cid2gid(const std::string &cidstr);

    // Glyph Classes
    GNode *curGCHead {nullptr}, **curGCTailAddr {nullptr};
    std::string curGCName;
    std::unordered_map<std::string, GNode *> namedGlyphClasses;

    void resetCurrentGC();
    void defineCurrentGC(const std::string &gcname);
    bool openAsCurrentGC(const std::string &gcname);
    GNode *finishCurrentGC();
    void addGlyphToCurrentGC(GID gid);
    void addGlyphClassToCurrentGC(GNode *src);
    void addGlyphClassToCurrentGC(const std::string &gcname);
    void addAlphaRangeToCurrentGC(GID first, GID last,
                                  const char *firstname, const char *p,
                                  char q);
    void addNumRangeToCurrentGC(GID first, GID last, const char *firstname,
                                const char *p1, const char *p2,
                                const char *q1, int numLen);
    void addRangeToCurrentGC(GID first, GID last,
                             const std::string &firstName,
                             const std::string &lastName);
    GNode *lookupGlyphClass(const std::string &gcname);
    bool compareGlyphClassCount(GNode *targ, GNode *repl, bool isSubrule);

    // Tag management
    enum TagType { featureTag, scriptTag, languageTag, tableTag };
    typedef std::unordered_set<Tag> TagArray;
    TagArray script, language, feature, table;

    inline bool addTag(TagArray &a, Tag t) {
        if ( a.find(t) == a.end() )
            return false;
        a.emplace(t);
        return true;
    }
    bool tagAssign(Tag tag, enum TagType type, bool checkIfDef);

    // Scripts and languages
    struct LangSys {
        LangSys() = delete;
        LangSys(Tag script, Tag lang) : script(script), lang(lang) {}
        Tag script;
        Tag lang;
        bool operator<(const LangSys &b) const;
        bool operator==(const LangSys &b) const;
    };

    std::map<LangSys, bool> langSysMap;
    bool include_dflt = true, seenOldDFLT = false;

    int startScriptOrLang(TagType type, Tag tag);
    void includeDFLT(bool includeDFLT, int langChange, bool seenOD);
    void addLangSys(Tag script, Tag language, bool checkBeforeFeature,
                    FeatParser::TagContext *langctx = nullptr);
    void registerFeatureLangSys();

    // Features
    struct State {
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        Tag tbl {TAG_UNDEF};  // GSUB_ or GPOS_
        int lkpType {0};     // GSUBsingle, GPOSSingle, etc.
        unsigned int lkpFlag {0};
        unsigned short markSetIndex {0};
        Label label {LAB_UNDEF};
        bool operator==(const State &b) const;
    };
    State curr, prev;
    std::vector<State> DFLTLkps;

#ifdef HOT_DEBUG
    void stateDump(State &st);
#endif
    void startFeature(Tag tag);
    void endFeature();
    void flagExtension(bool isLookup);
    void closeFeatScriptLang(State &st);
    void addFeatureParam(const std::vector<uint16_t> &params);
    void subtableBreak();

    // Lookups
    struct LookupInfo {
        LookupInfo(Tag t, int lt, unsigned int lf, unsigned short msi,
                   Label l, bool ue) : tbl(t), lkpType(lt), lkpFlag(lf),
                                       markSetIndex(msi), label(l),
                                       useExtension(ue) {}
        Tag tbl;          // GSUB_ or GPOS_
        int lkpType;      // GSUBsingle, GPOSSingle, etc.
        unsigned int lkpFlag;
        unsigned short markSetIndex;
        Label label;
        bool useExtension;
    };
    std::vector<LookupInfo> lookup;

    void startLookup(const std::string &name, bool isTopLevel);
    void endLookup();
    uint16_t setLkpFlagAttribute(uint16_t val, unsigned int attr,
                                 uint16_t markAttachClassIndex);
    void setLkpFlag(uint16_t flagVal);
    void callLkp(State &st);
    void useLkp(const std::string &name);

    struct NamedLkp {
        NamedLkp() = delete;
        NamedLkp(const std::string &name, bool tl) : name(name),
                                                     isTopLevel(tl) {}
        std::string name;
        State state;
        bool useExtension {false}, isTopLevel;
    };
    static_assert(FEAT_NAMED_LKP_BEG == 0,
                  "Named label values must start at zero");
    std::vector<NamedLkp> namedLkp;
    Label currNamedLkp {LAB_UNDEF};
    bool endOfNamedLkpOrRef {false};
    Label anonLabelCnt = FEAT_ANON_LKP_BEG;

    NamedLkp *name2NamedLkp(const std::string &lkpName);
    NamedLkp *lab2NamedLkp(Label lab);
    Label getNextNamedLkpLabel(const std::string &name, bool isa);
    Label getLabelIndex(const std::string &name);

    // Tables
    uint16_t featNameID;
    bool sawSTAT {false};
    bool sawFeatNames {false};
    struct STAT {
        uint16_t flags, format, prev;
        std::vector<Tag> axisTags;
        std::vector<Fixed> values;
        Fixed min, max;
    } stat;
    bool axistag_vert {false}, sawBASEvert {false}, sawBASEhoriz {false};
    size_t axistag_count {0};
    antlr4::Token *axistag_token {nullptr};
    FeatVisitor *axistag_visitor {nullptr};
    void (FeatCtx::*addNameFn)(long platformId, long platspecId,
                               long languageId, const std::string &str);

    void startTable(Tag tag);
    void setGDEFGlyphClassDef(GNode *simple, GNode *ligature, GNode *mark,
                              GNode *component);
    void createDefaultGDEFClasses();
    void setFontRev(const std::string &rev);
    void addNameString(long platformId, long platspecId,
                       long languageId, long nameId,
                       const std::string &str);
    void addSizeNameString(long platformId, long platspecId,
                           long languageId, const std::string &str);
    void addFeatureNameString(long platformId, long platspecId,
                              long languageId, const std::string &str);
    void addFeatureNameParam();
    void addUserNameString(long platformId, long platspecId,
                           long languageId, const std::string &str);
    void addVendorString(std::string str);

    // Anchors
    struct AnchorDef {
        short x {0};
        short y {0};
        unsigned int contourpoint {0};
        bool hasContour {false};
    };
    std::map<std::string, AnchorDef> anchorDefs;
    std::vector<AnchorMarkInfo> anchorMarkInfo;
    std::vector<GNode *> markClasses;
    void addAnchorDef(const std::string &name, const AnchorDef &a);
    void addAnchorByName(const std::string &name, int componentIndex);
    void addAnchorByValue(const AnchorDef &a, bool isNull,
                          int componentIndex);
    void addMark(const std::string &name, GNode *targ);

    // Metrics
    std::map<std::string, MetricsInfo> valueDefs;
    void addValueDef(const std::string &name, const MetricsInfo &mi);
    void getValueDef(const std::string &name, MetricsInfo &mi);

    // Substitutions
    void prepRule(Tag newTbl, int newlkpType, GNode *targ, GNode *repl);
    void addGSUB(int lkpType, GNode *targ, GNode *repl);
    bool validateGSUBSingle(GNode *targ, GNode *repl, bool isSubrule);
    bool validateGSUBMultiple(GNode *targ, GNode *repl, bool isSubrule);
    bool validateGSUBAlternate(GNode *targ, GNode *repl, bool isSubrule);
    bool validateGSUBLigature(GNode *targ, GNode *repl, bool isSubrule);
    bool validateGSUBReverseChain(GNode *targ, GNode *repl);
    bool validateGSUBChain(GNode *targ, GNode *repl);
    void addSub(GNode *targ, GNode *repl, int lkpType);
    void wrapUpRule();

    // Positions
    void addMarkClass(const std::string &markClassName);
    void addGPOS(int lkpType, GNode *targ);
    void addBaseClass(GNode *targ, const std::string &defaultClassName);
    void addPos(GNode *targ, int type, bool enumerate);

    // CVParameters
    enum { cvUILabelEnum = 1, cvToolTipEnum, cvSampleTextEnum,
           kCVParameterLabelEnum };
    CVParameterFormat cvParameters;
    bool sawCVParams {false};
    void clearCVParameters();
    void addCVNameID(int labelID);
    void addCVParametersCharValue(unsigned long uv);
    void addCVParam();

    // Ranges
    void setUnicodeRange(short unicodeList[kLenUnicodeList]);
    void setCodePageRange(short codePageList[kLenCodePageList]);

    // AALT
    struct AALT {
        State state;
        short useExtension {false};
        struct FeatureRecord {
            Tag feature;
            bool used;
            bool operator==(const FeatureRecord &b) const;
            bool operator==(const Tag &b) const;
        };
        std::vector<FeatureRecord> features;
        struct RuleInfo {
            GNode *targ;
            GNode *repl;
        };
        std::unordered_map<GID, RuleInfo> rules;
    } aalt;

    void aaltAddFeatureTag(Tag tag);
    void reportUnusedaaltTags();
    void aaltRuleSort(GNode **list);
    void aaltAddAlternates(GNode *targ, GNode *repl);
    void aaltCreate();
    bool aaltCheckRule(int type, GNode *targ, GNode *repl);
    void storeRuleInfo(GNode *targ, GNode *repl);

    // Temporary for cross product
    std::vector<GNode *> prod;

    hotCtx g;
    FeatVisitor *root_visitor {nullptr}, *current_visitor {nullptr};
};

#endif  // ADDFEATURES_HOTCONV_FEATCTX_H_
