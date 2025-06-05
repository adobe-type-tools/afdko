/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#ifndef ADDFEATURES_HOTCONV_FEATCTX_H_
#define ADDFEATURES_HOTCONV_FEATCTX_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "FeatParser.h"
#include "hotmap.h"
#include "varsupport.h"

class VarLocation;

// Debugging message support
#if HOT_DEBUG
inline int DF_LEVEL(hotCtx g) {
    if (g->font.debug & HOT_DB_FEAT_2)
        return 2;
    else if (g->font.debug & HOT_DB_FEAT_1)
        return 1;
    else
        return 0;
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

struct MetricsInfo {
    static const std::array<int, 4> kernorder;

    enum ValueRecordFlag {
        ValueXPlacement = (1 << 0),
        ValueYPlacement = (1 << 1),
        ValueXAdvance = (1 << 2),
        ValueYAdvance = (1 << 3),
        ValueMask = 15,
        DeviceXPlacement = (1 << 4),
        DeviceYPlacement = (1 << 5),
        DeviceXAdvance = (1 << 6),
        DeviceYAdvance = (1 << 7)
    };

    MetricsInfo() {}
    explicit MetricsInfo(const MetricsInfo &o) : vertical(o.vertical),
                                                 force(o.force),
                                                 normalized(o.normalized),
                                                 metrics(o.metrics) {}
    explicit MetricsInfo(MetricsInfo &&o) : vertical(o.vertical),
                                            force(o.force),
                                            normalized(o.normalized),
                                            metrics(std::move(o.metrics)) {}
    MetricsInfo & operator=(MetricsInfo &&o) {
        vertical = o.vertical;
        force = o.force;
        normalized = o.normalized;
        metrics = std::move(o.metrics);
        return *this;
    }
    void swap(MetricsInfo &o) {
        std::swap(normalized, o.normalized);
        metrics.swap(o.metrics);
    }
    MetricsInfo & operator=(const MetricsInfo &o) {
        normalized = o.normalized;
        metrics = o.metrics;
        return *this;
    }
#if HOT_DEBUG
    void toerr() {
        bool first = true;
        if (metrics.size() > 1)
            std::cerr << '<';
        for (auto &m : metrics) {
            if (first)
                first = false;
            else
                std::cerr << ' ';
            m.toerr();
        }
        if (metrics.size() > 1)
            std::cerr << '>';
    }
#endif  // HOt_DEBUG
    void normalize(bool v = false, bool f = false) {
        if (normalized)
            return;
        vertical = v;
        force = f;
        assert(!normalized);
        for (auto &m : metrics)
            m.normalize();
        if (metrics.size() != 4) {
            ValueVector nmv {4};
            if (metrics.size() > 0) {
                assert(metrics.size() == 1);
                if (vertical)
                    nmv[3].swap(metrics[0]);
                else
                    nmv[2].swap(metrics[0]);
            }
            metrics.swap(nmv);
        }
        normalized = true;
    }
    bool operator==(const MetricsInfo &rhs) const {
        assert(normalized && rhs.normalized);
        assert(metrics.size() == 4 && rhs.metrics.size() == 4);
        for (int i = 3; i >= 0; i--)
            if (!(metrics[i] == rhs.metrics[i]))
                return false;
        return true;
    }
    bool operator<(const MetricsInfo &rhs) const {
        assert(normalized && rhs.normalized);
        assert(metrics.size() == 4 && rhs.metrics.size() == 4);
        for (int i = 3; i >= 0; i--)
            if (!(metrics[i] == rhs.metrics[i]))
                return metrics[i] < rhs.metrics[i];
        return false;
    }
    bool kernlt(const MetricsInfo &o) const {
        for (const auto i : kernorder) {
            if (!metrics[i].nonZero() && o.metrics[i].nonZero())
                return true;
            if (metrics[i].nonZero() && !o.metrics[i].nonZero())
                return false;
            auto adv = ABS(metrics[i].getDefault());
            auto aodv = ABS(o.metrics[i].getDefault());
            if (adv != aodv)
                return adv < aodv;
            if (!(metrics[i] == o.metrics[i]))
                return metrics[i] < o.metrics[i];
        }
        return false;
    }

    uint32_t size() { return metrics.size(); }

    bool hasMetrics() const {
        for (auto &m : metrics)
            if (m.nonZero())
                return true;
        return false;
    }

    uint32_t valueFormat() {
        uint32_t r {0};

        assert(normalized);

        if (metrics[0].nonZero()) {
            r |= ValueXPlacement;
            if (metrics[0].isVariable())
                r |= DeviceXPlacement;
        }
        if (metrics[1].nonZero()) {
            r |= ValueYPlacement;
            if (metrics[1].isVariable())
                r |= DeviceYPlacement;
        }
        if (metrics[2].nonZero()) {
            r |= ValueXAdvance;
            if (metrics[2].isVariable())
                r |= DeviceXAdvance;
        }
        if (metrics[3].nonZero()) {
            r |= ValueYAdvance;
            if (metrics[3].isVariable())
                r |= DeviceYAdvance;
        }
        if (r == 0 && force)
            r = vertical ? ValueYAdvance : ValueXAdvance;
        return r;
    }

    void reset() { metrics.clear(); normalized = false; }
    const VarValueRecord &getRecordByFlag(uint16_t f) {
        assert(f == ValueXPlacement || f == ValueYPlacement ||
               f == ValueXAdvance || f == ValueYAdvance);
        assert(normalized);
        switch (f) {
            case ValueXPlacement:
                return metrics[0];
                break;
            case ValueYPlacement:
                return metrics[1];
                break;
            case ValueXAdvance:
                return metrics[2];
                break;
            case ValueYAdvance:
                return metrics[3];
                break;
        }
        return metrics[0];
    }
    static ValueIndex numValues(uint32_t valFmt) {
        ValueIndex i = 0;
        valFmt &= ValueMask;
        while (valFmt) {
            i++;
            valFmt &= valFmt - 1; /* Remove least significant set bit */
        }
        return i;
    }
    static ValueIndex numVariables(uint32_t valFmt) {
        return numValues(valFmt >> 4);
    }
    bool vertical {false};
    bool force {false};
    bool normalized {false};
    ValueVector metrics;
};

struct AnchorMarkInfo {
    enum { CONTOUR_POINT_UNDEF = 0xFFFF };
    AnchorMarkInfo() {}
    explicit AnchorMarkInfo(int32_t ci) : componentIndex(ci) {}
    AnchorMarkInfo(const AnchorMarkInfo &o) : x(o.x), y(o.y),
                                              contourpoint(o.contourpoint),
                                              componentIndex(o.componentIndex),
                                              markClassIndex(o.markClassIndex),
                                              markClassName(o.markClassName) {}
    AnchorMarkInfo(AnchorMarkInfo &&o) : x(std::move(o.x)), y(std::move(o.y)),
                                         contourpoint(o.contourpoint),
                                         componentIndex(o.componentIndex),
                                         markClassIndex(o.markClassIndex),
                                         markClassName(std::move(o.markClassName)) {}
    VarValueRecord &getXValueRecord() { return x; }
    VarValueRecord &getYValueRecord() { return y; }
    uint16_t &getContourpoint() { return contourpoint; }
    void setContourpoint(uint16_t cp) { contourpoint = cp; }
    void setComponentIndex(int16_t ci) {
        assert(componentIndex == -1);
        componentIndex = ci;
    }
    bool operator== (const AnchorMarkInfo &rhs) const {
        return x == rhs.x && y == rhs.y && contourpoint == rhs.contourpoint;
    }
    bool isInitialized() const {
        return x.isInitialized() || y.isInitialized();
    }
    VarValueRecord x;
    VarValueRecord y;
    uint16_t contourpoint {CONTOUR_POINT_UNDEF};
    int32_t componentIndex {-1};
    int32_t markClassIndex {0};
    std::string markClassName;
};

class FeatCtx;

struct GPat {
    typedef std::unique_ptr<GPat> SP;
    struct GlyphRec {
        explicit GlyphRec(GID g) : gid(g) {}
        GlyphRec(const GlyphRec &gr) = default;
        operator GID() const { return gid; }
        bool operator<(const GlyphRec &gr) const { return gid < gr.gid; }
        bool operator==(const GlyphRec &gr) const { return gid == gr.gid; }
        bool operator==(GID g) const { return g == gid; }
        GID gid {GID_UNDEF};
        std::shared_ptr<AnchorMarkInfo> markClassAnchorInfo;
    };
    struct ClassRec {
        ClassRec() : marked(false), gclass(false), backtrack(false), input(false),
                     lookahead(false), basenode(false), marknode(false),
                     used_mark_class(false) {}
        explicit ClassRec(GID gid) : ClassRec() { glyphs.emplace_back(gid); }
        ClassRec(const ClassRec &o) : glyphs(o.glyphs), lookupLabels(o.lookupLabels),
                                      metricsInfo(o.metricsInfo),
                                      markClassName(o.markClassName),
                                      marked(o.marked), gclass(o.gclass),
                                      backtrack(o.backtrack), input(o.input),
                                      lookahead(o.lookahead), basenode(o.basenode),
                                      marknode(o.marknode),
                                      used_mark_class(o.used_mark_class) {}
        ClassRec(ClassRec &&o) : glyphs(std::move(o.glyphs)),
                                 lookupLabels(std::move(o.lookupLabels)),
                                 metricsInfo(std::move(o.metricsInfo)),
                                 markClassName(std::move(o.markClassName)),
                                 marked(o.marked), gclass(o.gclass), backtrack(o.backtrack),
                                 input(o.input), lookahead(o.lookahead), basenode(o.basenode),
                                 marknode(o.marknode), used_mark_class(o.used_mark_class) {}
        ClassRec & operator=(const ClassRec &other) {
            glyphs = other.glyphs;
            lookupLabels = other.lookupLabels;
            metricsInfo = other.metricsInfo;
            markClassName = other.markClassName;
            marked = other.marked;
            gclass = other.gclass;
            backtrack = other.backtrack;
            input = other.input;
            lookahead = other.lookahead;
            basenode = other.basenode;
            marknode = other.marknode;
            used_mark_class = other.used_mark_class;
            return *this;
        }
        bool operator==(const ClassRec &rhs) const { return glyphs == rhs.glyphs; }
        void reset() {
            glyphs.clear();
            lookupLabels.clear();
            metricsInfo.reset();
            markClassName.clear();
            marked = gclass = backtrack = input = false;
            lookahead = basenode = marknode = used_mark_class = false;
        }
        bool glyphInClass(GID gid) const {
            return std::find(glyphs.begin(), glyphs.end(), gid) != glyphs.end();
        }
        bool is_glyph() const { return glyphs.size() == 1 && !gclass; }
        bool is_multi_class() const { return glyphs.size() > 1; }
        bool is_class() const { return is_multi_class() || gclass; }
        bool has_lookups() const { return lookupLabels.size() > 0; }
        int classSize() const { return glyphs.size(); }
        void addGlyph(GID gid) { glyphs.emplace_back(gid); }
        void sort() { std::sort(glyphs.begin(), glyphs.end()); }
        void makeUnique(hotCtx g, bool report = false);
        void concat(const ClassRec &other) {
            glyphs.insert(glyphs.end(), other.glyphs.begin(), other.glyphs.end());
        }
        std::vector<GlyphRec> glyphs;
        std::vector<int> lookupLabels;
        MetricsInfo metricsInfo;
        // XXX would like to get rid of this
        std::string markClassName;
        bool marked : 1;      // Sequence element is marked
        bool gclass : 1;      // Sequence element is glyph class
        bool backtrack : 1;   // Part of a backtrack sub-sequence
        bool input : 1;       // Part of an input sub-sequence
        bool lookahead : 1;   // Part of a lookhead sub-sequence
        bool basenode : 1;    // Sequence element is base glyph in mark attachment lookup
        bool marknode : 1;    // Sequence element is mark glyph in mark attachment lookup
        bool used_mark_class : 1;  // This marked class is used in a pos statement (can't add new glyphs)
    };
    class CrossProductIterator {
     public:
        CrossProductIterator() = delete;
        explicit CrossProductIterator(const std::vector<GPat::ClassRec *> &c) :
                     classes(c), indices(classes.size(), 0) {}
        explicit CrossProductIterator(std::vector<GPat::ClassRec *> &&c) :
                     classes(std::move(c)), indices(classes.size(), 0) {}
        bool next(std::vector<GID> &gids) {
            size_t i;
            assert(classes.size() == indices.size());
            if (!first) {
                for (i = 0; i < classes.size(); i++) {
                    if (++indices[i] < classes[i]->glyphs.size())
                        break;
                    indices[i] = 0;
                }
                if (i == classes.size())
                    return false;
            }
            first = false;
            gids.clear();
            for (i = 0; i < classes.size(); i++)
                gids.push_back(classes[i]->glyphs[indices[i]].gid);
            return true;
        }

     private:
        std::vector<GPat::ClassRec *> classes;
        std::vector<uint16_t> indices;
        bool first {true};
    };

    GPat() : has_marked(false), ignore_clause(false), lookup_node(false),
             enumerate(false) {}
    explicit GPat(GID gid) : GPat() { classes.emplace_back(gid); }
    explicit GPat(const GPat &o) : classes(o.classes),
                                   has_marked(o.has_marked),
                                   ignore_clause(o.ignore_clause),
                                   lookup_node(o.lookup_node),
                                   enumerate(o.enumerate) {}
/*    GPat & operator=(GPat other) {
        classes = other.classes;
        has_marked = other.has_marked;
        ignore_clause = other.ignore_clause;
        lookup_node = other.lookup_node;
        enumerate = other.enumerate;
        return *this;
    } */
    bool is_glyph() const { return classes.size() == 1 && classes[0].is_glyph(); }
    bool is_class() const { return classes.size() == 1 && classes[0].is_class(); }
    bool is_mult_class() const { return classes.size() == 1 && classes[0].is_multi_class(); }
    bool is_unmarked_glyph() const { return is_glyph() && !has_marked; }
    bool is_unmarked_class() const { return is_class() && !has_marked; }
    uint16_t patternLen() const { return classes.size(); }
    void addClass(ClassRec &cr) { classes.push_back(cr); }
    void addClass(ClassRec &&cr) { classes.emplace_back(std::move(cr)); }
    bool glyphInClass(GID gid, uint16_t idx = 0) const {
        assert(idx < classes.size());
        if (idx >= classes.size())
            return false;
        return classes[idx].glyphInClass(gid);
    }
    void sortClass(uint16_t idx = 0) {
        assert(idx < classes.size());
        if (idx >= classes.size())
            return;
        classes[idx].sort();
    }
    void makeClassUnique(hotCtx g, uint16_t idx = 0, bool report = false) {
        assert(idx < classes.size());
        if (idx >= classes.size())
            return;
        classes[idx].makeUnique(g, report);
    }
    int classSize(uint16_t idx = 0) const {
        assert(idx < classes.size());
        if (idx >= classes.size())
            return 0;
        return classes[idx].classSize();
    }
    std::vector<ClassRec> classes;
    bool has_marked : 1;       // Sequence has at least one marked node
    bool ignore_clause : 1;    // Sequence is an ignore clause
    bool lookup_node : 1;      // Pattern uses direct lookup reference
    bool enumerate : 1;        // Class should be enumerated
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

class FeatVisitor;

class FeatCtx {
    friend class FeatVisitor;

 public:
    FeatCtx() = delete;
    explicit FeatCtx(hotCtx g) : g(g) {}

    void fill();

    void dumpGlyph(GID gid, int ch, bool print);
    void dumpGlyphClass(const GPat::ClassRec &cr, int ch, bool print);
    void dumpPattern(const GPat &pat, int ch, bool print);

    std::string msgPrefix();
    bool validateGPOSChain(GPat::SP &targ, int lookupType);
    Label getNextAnonLabel();
    const GPat::ClassRec &lookupGlyphClass(const std::string &gcname);

    uint16_t getAxisCount();

    // Utility
    Tag str2tag(const std::string &tagName);
    std::string unescString(const std::string &s);

#if HOT_DEBUG
    void tagDump(Tag);
    void dumpLocationDefs();
#endif
    static const int kMaxCodePageValue;
    static const int kCodePageUnSet;

 private:
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
    void sortGlyphClass(GPat::SP &gp, bool unique, bool reportDups, uint16_t idx = 0);
    GPat::ClassRec curGC;
    std::string curGCName;
    std::unordered_map<std::string, GPat::ClassRec> namedGlyphClasses;

    void resetCurrentGC();
    void defineCurrentGC(const std::string &gcname);
    bool openAsCurrentGC(const std::string &gcname);
    void finishCurrentGC();
    void finishCurrentGC(GPat::ClassRec &cr);
    void addGlyphToCurrentGC(GID gid);
    void addGlyphClassToCurrentGC(const GPat::ClassRec &cr, bool init = false);
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
    bool compareGlyphClassCount(int targc, int replc, bool isSubrule);

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
    void callLkp(State &st, bool fromDFLT=false);
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
    void (FeatCtx::*addNameFn)(int32_t platformId, int32_t platspecId,
                               int32_t languageId, const std::string &str);

    void startTable(Tag tag);
    void setGDEFGlyphClassDef(GPat::ClassRec &simple, GPat::ClassRec &ligature,
                              GPat::ClassRec &mark, GPat::ClassRec &component);
    void createDefaultGDEFClasses();
    void setFontRev(const std::string &rev);
    void addNameString(int32_t platformId, int32_t platspecId,
                       int32_t languageId, int32_t nameId,
                       const std::string &str);
    void addSizeNameString(int32_t platformId, int32_t platspecId,
                           int32_t languageId, const std::string &str);
    void addFeatureNameString(int32_t platformId, int32_t platspecId,
                              int32_t languageId, const std::string &str);
    void addFeatureNameParam();
    void addUserNameString(int32_t platformId, int32_t platspecId,
                           int32_t languageId, const std::string &str);
    void addVendorString(std::string str);

    // Anchors
    std::map<std::string, AnchorMarkInfo> anchorDefs;
    std::vector<std::shared_ptr<AnchorMarkInfo>> anchorMarkInfo;
    void addAnchorDef(const std::string &name, AnchorMarkInfo &&a);
    void addAnchorByName(const std::string &name, int componentIndex);
    void addAnchorByValue(std::shared_ptr<AnchorMarkInfo> a, int componentIndex);
    void addMark(const std::string &name, GPat::ClassRec &cr);

    // Metrics
    std::map<std::string, MetricsInfo> valueDefs;
    void addValueDef(const std::string &name, const MetricsInfo &&mi);
    void getValueDef(const std::string &name, MetricsInfo &mi);

    // Substitutions
    void prepRule(Tag newTbl, int newlkpType, const GPat::SP &targ,
                  const GPat::SP &repl, bool fromDFLT=false);
    void addGSUB(int lkpType, GPat::SP targ, GPat::SP repl);
    bool validateGSUBSingle(const GPat::ClassRec &targcr,
                            const GPat::SP &repl, bool isSubrule);
    bool validateGSUBSingle(const GPat::SP &targ, const GPat::SP &repl);
    bool validateGSUBMultiple(const GPat::ClassRec &targcr,
                              const GPat::SP &repl, bool isSubrule);
    bool validateGSUBMultiple(const GPat::SP &targ, const GPat::SP &repl);
    bool validateGSUBAlternate(const GPat::SP &targ, const GPat::SP &repl, bool isSubrule);
    bool validateGSUBLigature(const GPat::SP &targ, const GPat::SP &repl, bool isSubrule);
    bool validateGSUBReverseChain(GPat::SP &targ, GPat::SP &repl);
    bool validateGSUBChain(GPat::SP &targ, GPat::SP &repl);
    void addSub(GPat::SP targ, GPat::SP repl, int lkpType);
    void wrapUpRule();

    // Positions
    void addMarkClass(const std::string &markClassName);
    void addGPOS(int lkpType, GPat::SP targ);
    void addBaseClass(GPat::SP &targ, const std::string &defaultClassName);
    void addPos(GPat::SP targ, int type, bool enumerate);

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
            struct GlyphInfo {
                GlyphInfo(GID g, int16_t a) : rgid(g), aaltIndex(a) {}
                bool operator==(const GlyphInfo &b) const {
                    return aaltIndex == b.aaltIndex;
                }
                bool operator<(const GlyphInfo &b) const {
                    return aaltIndex < b.aaltIndex;
                }
                GID rgid {0};
                int16_t aaltIndex {0};
            };
            explicit RuleInfo(GID gid) : targid(gid) {}
            GID targid;
            std::vector<GlyphInfo> glyphs;
        };
        std::map<GID, RuleInfo> rules;
    } aalt;

    void aaltAddFeatureTag(Tag tag);
    void reportUnusedaaltTags();
    void aaltAddAlternates(const GPat::ClassRec &targcr, const GPat::ClassRec &replcr);
    void aaltCreate();
    bool aaltCheckRule(int type, GPat::SP &targ, GPat::SP &repl);
    void storeRuleInfo(const GPat::SP &targ, const GPat::SP &repl);

    // Variable
    std::unordered_map<std::string, uint32_t> locationDefs;

    var_F2dot14 validAxisLocation(var_F2dot14 v, int8_t adjustment = 0);
    int16_t axisTagToIndex(Tag tag);
    uint32_t locationToIndex(std::shared_ptr<VarLocation> vl);
    bool addLocationDef(const std::string &name, uint32_t loc_idx);
    uint32_t getLocationDef(const std::string &name);

    hotCtx g;
    FeatVisitor *root_visitor {nullptr}, *current_visitor {nullptr};
};

#endif  // ADDFEATURES_HOTCONV_FEATCTX_H_
