/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_GSUB_H_
#define ADDFEATURES_HOTCONV_GSUB_H_

#include <map>
#include <vector>
#include <memory>

#include "common.h"
#include "feat.h"
#include "otl.h"

#define GSUB_ TAG('G', 'S', 'U', 'B')

/* Standard functions */

void GSUBNew(hotCtx g);
int GSUBFill(hotCtx g);
void GSUBWrite(hotCtx g);
void GSUBReuse(hotCtx g);
void GSUBFree(hotCtx g);

/* Lookup types */
enum {
    GSUBSingle = 1,
    GSUBMultiple,
    GSUBAlternate,
    GSUBLigature,
    GSUBContext,
    GSUBChain,
    GSUBExtension, /* Handled specially: it points to any of the above */
    GSUBReverse,
    GSUBFeatureNameParam,
    GSUBCVParam,
};

class GSUB {
 public:
    struct SubstLookupRecord {
        uint16_t SequenceIndex {0};
        uint16_t LookupListIndex {0};
    };

    struct SubstRule {
        SubstRule() = delete;
        SubstRule(GNode *targ, GNode *repl) : targ(targ), repl(repl) {}
        SubstRule(GNode *targ, GNode *repl, uint16_t data) : targ(targ), repl(repl),
                                                             data(data) {}
        bool operator<(const SubstRule &b) { return targ->gid < b.targ->gid; }
        GNode *targ {nullptr};
        GNode *repl {nullptr};
        uint16_t data {0};
    };

    struct SubtableInfo {  /* New subtable data */
        void reset(uint32_t lt, uint32_t lf, Label l, bool ue, uint16_t umsi) {
            useExtension = ue;
            lkpType = lt;
            label = l;
            lkpFlag = lf;
            markSetIndex = umsi;
            paramNameID = 0;
            parentFeatTag = 0;
            cvParams.reset();
            singles.clear();
            rules.clear();
        }
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        Tag parentFeatTag {0}; /* The parent feature for anonymous lookups made by a chaining contextual feature */
        bool useExtension {false}; /* Use extension lookupType? */
        uint16_t lkpType {0};
        uint16_t lkpFlag {0};
        uint16_t markSetIndex {0};
        Label label {0};
        uint16_t paramNameID {0};
        CVParameterFormat cvParams;
        std::map<GID,GID> singles;
        std::vector<SubstRule> rules;
    };

    struct ExtensionSubstFormat1 {
        static uint16_t size() { return sizeof(uint16_t) * 2 + sizeof(uint32_t); }
        uint16_t SubstFormat {0};
        uint16_t ExtensionLookupType {0};
        LOffset ExtensionOffset {0};
    };

    struct Subtable {
        Subtable() = delete;
        Subtable(GSUB &h, SubtableInfo &si);
        virtual void write(GSUB *h) { }  // For reference tables
        virtual uint16_t subformat() { return 0; }
        virtual std::vector<SubstLookupRecord> *getSubstLookups() { return nullptr; }
        otlTbl &getOtl(GSUB &h) {
            return extension.use ? *extension.otl : h.otl;
        }
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        char id_text[ID_TEXT_SIZE] {0};
        uint16_t lkpType {0};
        uint16_t lkpFlag {0};
        uint16_t markSetIndex {0};
        Label label {0};
        LOffset offset {0};     /* From beginning of first subtable */
        struct {            /* Extension-related data */
            bool use {false};      /* Use extension lookupType? If set, then following used: */
            std::unique_ptr<otlTbl> otl;  /* For coverages and classes of this extension subtable */
            LOffset offset {0}; /* Offset to this extension subtable, from beginning of extension section. Debug only. */
            ExtensionSubstFormat1 tbl;

            /* Subtable data */
        } extension;
    };
    
    struct SingleSubst : public Subtable {
        SingleSubst() = delete;
        SingleSubst(GSUB &h, SubtableInfo &si);
        static void fill(GSUB &h, SubtableInfo &si);
        Offset fillSingleCoverage(SubtableInfo &si, otlTbl &otl);
    };

    struct MultipleSubst : public Subtable {
        struct MultSequence {
            Offset size() { return sizeof(uint16_t) * (1 + gids.size()); }
            LOffset offset {0};
            std::vector<GID> gids;
        };
        static int headerSize(int seqCount) {
            return sizeof(uint16_t) * (3 + seqCount);
        }
        static int size(int seqCount, int subCount) {
            return headerSize(seqCount) + sizeof(uint16_t) * (seqCount + subCount);
        }
        MultipleSubst() = delete;
        MultipleSubst(GSUB &h, SubtableInfo &si, long beg,
                      long end, long sz, uint32_t nSubs);
        uint16_t subformat() override { return 1; }
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        LOffset Coverage;
        std::vector<MultSequence> sequences;
    };

    struct AlternateSubst : public Subtable {
        struct AlternateSet {
            Offset size() { return sizeof(uint16_t) * (1 + gids.size()); }
            LOffset offset {0};
            std::vector<GID> gids;
        };
        static int headerSize(int setCount) {
            return sizeof(uint16_t) * (3 + setCount);
        }
        static int size(int setCount, int altCount) {
            return headerSize(setCount) + sizeof(uint16_t) * (setCount + altCount);
        }
        AlternateSubst() = delete;
        AlternateSubst(GSUB &h, SubtableInfo &si,
                       long beg, long end, long size, uint16_t numAlts);
        uint16_t subformat() override { return 1; }
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        LOffset Coverage;
        std::vector<AlternateSet> altSets;
    };

    struct LigatureSubst : public Subtable {
        struct LigatureGlyph {
            LOffset size() { return sizeof(uint16_t) * (2 + components.size()); }
            LOffset offset {0};
            GID ligGlyph {0};
            std::vector<GID> components;
        };
        struct LigatureSet {
            static LOffset size(int count) { return sizeof(uint16_t) * (1 + count); }
            LOffset offset {0};
            std::vector<LigatureGlyph> ligatures;
        };
        static int headerSize(int setCount) {
            return sizeof(uint16_t) * (3 + setCount);
        }
        static void checkAndSort(GSUB &h, SubtableInfo &si);
        LigatureSubst() = delete;
        LigatureSubst(GSUB &h, SubtableInfo &si);
        uint16_t subformat() override { return 1; }
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        LOffset Coverage {0};
        std::vector<LigatureSet> ligatureSets;
    };

    struct ChainSubst : public Subtable {
        ChainSubst() = delete;
        ChainSubst(GSUB &h, SubtableInfo &si, SubstRule &rule);
        static LOffset substLookupRecSize() { return sizeof(uint16_t) * 2; }
        static LOffset chain3size(uint32_t nback, uint32_t ninput,
                                  uint32_t nlook, uint32_t nsub) {
            return sizeof(uint16_t) * (5 + nback + ninput + nlook) +
                   substLookupRecSize() * nsub;
        }
        uint16_t subformat() override { return 3; }  // only support 3 at present
        std::vector<SubstLookupRecord> *getSubstLookups() override {
            return &substLookupRecords;
        }
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        std::vector<LOffset> backtracks;
        std::vector<LOffset> inputGlyphs;
        std::vector<LOffset> lookaheads;
        std::vector<SubstLookupRecord> substLookupRecords;
    };

    struct ReverseSubst : public Subtable {
        ReverseSubst() = delete;
        ReverseSubst(GSUB &h, SubtableInfo &si, SubstRule &rule);
        static LOffset rchain1size(uint32_t nback, uint32_t nlook,
                                   uint32_t nsub) {
            return sizeof(uint16_t) * (5 + nback + nlook + nsub);
        }
        uint16_t subformat() override { return 1; }
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        LOffset InputCoverage;
        std::vector<LOffset> backtracks;
        std::vector<LOffset> lookaheads;
        std::vector<GID> substitutes;
    };

    struct FeatureNameParam : public Subtable {
        FeatureNameParam() = delete;
        FeatureNameParam(GSUB &h, SubtableInfo &si, uint16_t nameID);
        static void fill(GSUB &h, SubtableInfo &si);
        static int size() { return 2 * sizeof(uint16_t); }
        void write(GSUB *h) override;
        uint16_t subformat() override { return 0; }
        uint16_t nameID;
    };

    struct CVParam : public Subtable {
        CVParam() = delete;
        CVParam(GSUB &h, SubtableInfo &si, CVParameterFormat &&params);
        static void fill(GSUB &h, SubtableInfo &si);
        void write(GSUB *h) override;
        uint16_t subformat() override { return 0; }
        CVParameterFormat params;
    };

#define EXTENSION1_SIZE (sizeof(uint16_t) * 2 + sizeof(uint32_t))

    GSUB() = delete;
    explicit GSUB(hotCtx g) : otl(g), g(g) {}

    int Fill();
    void Write();

    void FeatureBegin(Tag script, Tag language, Tag feature);
    void FeatureEnd();
    void RuleAdd(GNode *targ, GNode *repl);
    void LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                     bool useExtension, uint16_t useMarkSetIndex);
    void LookupEnd(SubtableInfo *si = nullptr);

    void AddSubtable(std::unique_ptr<Subtable> s);
    bool SubtableBreak();
    void featParamsWrite();
    void addSubstRule(SubtableInfo &si, GNode *targ, GNode *repl);
    void SetFeatureNameID(Tag feat, uint16_t nameID);
    void AddFeatureNameParam(uint16_t nameID);
    void AddCVParam(CVParameterFormat &&params);

    void recycleProd(GNode **prod, int count);
    bool addSingleToAnonSubtbl(SubtableInfo &si, GNode *targ, GNode *repl);
    bool addLigatureToAnonSubtbl(SubtableInfo &si, GNode *targ, GNode *repl);
    Label addAnonRule(SubtableInfo &cur_si, GNode *pMarked, uint32_t nMarked, GNode *repl);
    void createAnonLookups();
    void setAnonLookupIndices();
    void setCoverages(std::vector<LOffset> &covs, otlTbl &otl, GNode *p, uint32_t num);

    void fillExtensionSubst(uint32_t ExtensionLookupType, ExtensionSubstFormat1 &tbl);
    void writeExtension(Subtable &sub);

    SubtableInfo nw;
    struct {
        LOffset featParam {0};    /* (Cumulative.) Next subtable offset |->  */
        LOffset subtable {0};     /* (Cumulative.) Next subtable offset |->  */
                                  /* start of subtable section. LOffset to   */
                                  /* check for overflow                      */
        LOffset extension {0};        /* (Cumulative.) Next extension subtable   */
                                  /* offset |-> start of extension section.  */
        LOffset extensionSection {0}; /* Start of extension section |-> start of */
                                  /* main subtable section                   */
    } offset;
    std::vector<std::unique_ptr<Subtable>> subtables;  /* Subtable list */
    std::vector<SubtableInfo> anonSubtable;  /* Anon subtable accumulator */

    std::map<Tag, uint16_t> featNameID;

    /* Info for chaining contextual lookups */

    uint16_t maxContext {0};

    otlTbl otl;  /* OTL table */
    hotCtx g;
};

struct SingleSubstFormat1 : public GSUB::SingleSubst {
    SingleSubstFormat1() = delete;
    SingleSubstFormat1(GSUB &h, GSUB::SubtableInfo &si, int delta);
    void write(GSUB *h) override;
    uint16_t subformat() override { return 1; }
    static LOffset size() { return sizeof(uint16_t) * 3; }
    LOffset Coverage {0};        // 32 bit for overflow check
    GID DeltaGlyphID {0};
};

struct SingleSubstFormat2 : public GSUB::SingleSubst {
    SingleSubstFormat2() = delete;
    SingleSubstFormat2(GSUB &h, GSUB::SubtableInfo &si);
    void write(GSUB *h) override;
    uint16_t subformat() override { return 2; }
    static LOffset size(int count) { return sizeof(uint16_t) * (3 + count); }
    LOffset Coverage {0};        // 32 bit for overflow check
    std::vector<GID> gids;
};

/*
   Each feature definition is bracketed by a FeatureBegin() and
   FeatureEnd(). Each lookup definition is bracketed by LookupBegin()
   and LookupEnd(). Each substitution rule is specified by RuleAdd().
   Substitution occurs when "target" glyphs are substituted by "replace"
   glyphs.
                    target                replacement
                    ------                -----------
   Single:          Single|Class          Single|Class
   Ligature:        Sequence              Single
   Alternate:       Single                Class
   Contextual:      Sequence              Single

   For example, fi and fl ligature formation may be specified thus:

   FeatureBegin(g, latn_, dflt_, liga_);

   LookupBegin(g, GSUBLigature, 0, label, 0, 0);

   RuleAdd(g, targ, repl); // targ->("f")->("i"), repl->("fi")
   RuleAdd(g, targ, repl); // targ->("f")->("l"), repl->("fl")

   LookupEnd(g);

   FeatureEnd(g); */

#endif  // ADDFEATURES_HOTCONV_GSUB_H_
