/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_GPOS_H_
#define ADDFEATURES_HOTCONV_GPOS_H_

#include <array>
#include <memory>
#include <vector>

#include "common.h"
#include "feat.h"
#include "otl.h"

#define GPOS_ TAG('G', 'P', 'O', 'S')

/* Standard functions */

class GPOS;

void GPOSNew(hotCtx g);
int GPOSFill(hotCtx g);
void GPOSWrite(hotCtx g);
void GPOSReuse(hotCtx g);
void GPOSFree(hotCtx g);

/* Lookup types */
enum {
    GPOSSingle = 1,
    GPOSPair,
    GPOSCursive,
    GPOSMarkToBase,
    GPOSMarkToLigature,
    GPOSMarkToMark,
    GPOSChain,
    GPOSExtension,    /* Handled specially: it points to any of the above */
    GPOSFeatureParam, /* Treated like a lookup in the code */
};

struct ExtensionPosFormat1 {
    uint16_t PosFormat {0};
    uint16_t ExtensionLookupType {0};
    LOffset ExtensionOffset {0};
};
#define EXTENSION1_SIZE (sizeof(uint16_t) * 2 + sizeof(uint32_t))

struct Subtable {
    Subtable() = delete;
    Subtable(GPOS &h);
    virtual void write(GPOS &h) = 0;
    virtual uint16_t subformat() = 0;
    Tag script;
    Tag language;
    Tag feature;
    char id_text[ID_TEXT_SIZE] {0};
    uint16_t lkpType;
    uint16_t lkpFlag;
    uint16_t markSetIndex;
    Label label;
    LOffset offset;     /* From beginning of first subtable */
    struct {            /* Extension-related data */
        uint16_t use {0};      /* Use extension lookupType? If set, then following used: */
        otlTbl otl {nullptr};     /* For coverages and classes of this extension subtable */
        LOffset offset {0}; /* Offset to this extension subtable, from beginning of extension section. Debug only. */
        ExtensionPosFormat1 tbl;

        /* Subtable data */
    } extension;
};

struct FeatureParameters : public Subtable {
    FeatureParameters() = delete;
    FeatureParameters(GPOS &h, std::array<uint16_t, 5> &p) : Subtable(h), params(p) {}
    static std::unique_ptr<Subtable> fill(GPOS &h);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 0; }

    std::array<uint16_t, 5> params;
};

struct SubtableInfo;

struct SinglePos : public Subtable {
    SinglePos() = delete;
    SinglePos(GPOS &h) : Subtable(h) {}
    static LOffset pos1Size(SubtableInfo &si, int iStart, int iEnd);
    static LOffset pos2Size(SubtableInfo &si, int iStart, int iEnd);
    static LOffset allPos1Size(SubtableInfo &si, int &nsub);
    static LOffset allPos2Size(SubtableInfo &si, int &nsub);
    static std::unique_ptr<Subtable> fill(GPOS &h);
};

struct SinglePosFormat1 : public SinglePos {
    SinglePosFormat1() = delete;
    SinglePosFormat1();
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }
    static std::unique_ptr<Subtable> fill(GPOS &h);

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    ValueRecord Value;
};

struct SinglePosFormat2 : public SinglePos {
    SinglePosFormat2() = delete;
    SinglePosFormat2();
    void write(GPOS *h) override;
    uint16_t subformat() override { return 2; }
    static std::unique_ptr<Subtable> fill(GPOS &h);
    
    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    std::vector<ValueRecord> Values;
};


enum {
    kSizeOpticalSize,
    kSizeSubFamilyID,
    kSizeMenuNameID,
    kSizeLowEndRange,
    kSizeHighEndRange
};


struct PosRule {
    GNode *targ {nullptr};
};

struct PosLookupRecord {
    uint16_t SequehceIndex {0};
    uint16_t LookupListIndex {0};
};

struct SingleRec {
    GID gid {0};
    int16_t xPla {0};
    int16_t yPla {0};
    int16_t xAdv {0};
    int16_t yAdv {0};
    uint16_t valFmt {0};
    struct {
        int16_t valFmt {0};
        int16_t valRec {0};
    } span;
#if HOT_DEBUG
    void dump();
#endif;
};


struct MarkClassRec {
    GNode *gnode {nullptr};
}

struct BaseGlyphRec {
    GID gid {0};
    std::vector<AnchorMarkInfo> anchorMarkInfo;
    int32_t componentIndex;
    char *locDesc;
};

union KernGlyph {
    GID gid;
    GNode *gcl;
};

struct MetricsRec {
    int16_t value[4] {0};
};

struct KernRec {
    KernGlyph first;
    KernGlyph second;
    uint32_t lineIndex {0};  // used to sort records in order of occurrence
    int16_t metricsCnt1;
    MetricsRec metricsRec1;
    int16_t metricsCnt2;
    MetricsRec metricsRec2;
};

struct ClassInfo {
    ClassInfo() {};
    ~ClassInfo() { free(gc); }
    uint32_t cls {0};
    GNode *gc {nullptr};
}

struct ClassDef {
    void reset(hotCtx g) {
        for (auto &ci: classInfo)
            featRecycleNodes(g, ci.gc);
        classInfo.clear(g);
        cov.clear();
    }
    std::vector<ClassInfo> classInfo;
    std::vector<GID> cov;
};

struct SubtableInfo {  /* New subtable data */
    void reset(uint32_t lt, uint32_t lf, Label l, int16_t ue, uint16_t umsi) {
        script = langauge = feature = TAG_UNDEF;
        useExtension = ue;
        lkpType = lt;
        label = l;
        lkpFlag = lf;
        markSetIndex = umsi;
        parentLkpType = 0;
        pairFmt = 0;
        pairValFmt1 = 0;
        pairValFmt2 = 0;
        rules.clear();
        markClassList.clear();
        baseList.clear();
        single.clear();
        pairs.clear();
        params.fill(0);
    }
    Tag script {TAG_UNDEF};
    Tag language {TAG_UNDEF};
    Tag feature {TAG_UNDEF};
    Tag parentFeatTag {0}; /* The parent feature for anonymous lookups made by a chaining contextual feature */
    int16_t useExtension {0}; /* Use extension lookupType? */
    uint16_t lkpType {0};
    uint16_t parentLkpType {0};
    uint16_t lkpFlag {0};
    uint16_t markSetIndex {0};
    Label label {0};
    std::array<uint16_t, 5> params;

    std::vector<PosRule> rules;
    std::vector<MarkClassRec> markClassList;
    std::vector<BaseGlyphRec> baseList;

    std::vector<SingleRec> single;
    std::vector<KernRec> pairs;
    int16_t pairFmt {0};       /* Fmt (1 or 2) of GPOS pair */
    uint16_t pairValFmt1 {0};  /* Fmt (1 or 2) of first value record  of GPOS pair */
    uint16_t pairValFmt2 {0};  /* Fmt (1 or 2) of second value record of GPOS pair */
};

class GPOS {
 public:
    GPOS() = delete;
    GPOS(hotCtx g) : g(g) {};

    int Fill();
    
/* Supplementary functions (See otl.h for script, language, feature,
   and lookup flag definitions) */
    void FeatureBegin(Tag script, Tag language, Tag feature);
    void FeatureEnd();
    void RuleAdd(int lkpType, GNode *targ, const char *locDesc,
                 int anchorCount, const AnchorMarkInfo *anchorMarkInfo);
    void LookupBegin(unsigned lkpType, unsigned lkpFlag, Label label,
                     short useExtension, unsigned short useMarkSetIndex);
    void LookupEnd(Tag feature);
    void AddSize(short *params, unsigned short numParams);
    void SetSizeMenuNameID(unsigned short nameID);
    int SubtableBreak();
    void PrintAFMKernData();
 private:
    void featParamsWrite();
    void reuseClassDefs();
    void startNewSubtable();
    void recyclePairs();
    uint32_t makeValueFormat(int32_t xPla, int32_t yPla, int32_t xAdv,
                             int32_t yAdv);
    void recordValues(uint32_t valFmt, int32_t xPla, int32_t yPla,
                      int32_t xAdv, int32_t yAdv);
    void writeValueRecord(unsigned valFmt, ValueRecord i);
    void SetSizeMenuNameID(uint16_t nameID);
    void checkAndSortSinglePos();
    void prepSinglePos();
    void AddSubtable(std::unique_ptr<Subtable> s);


    void fillSinglePos();
    void writeSinglePos(Subtable &sub);
    void freeSinglePos(Subtable &sub);

    void fillPairPos();
    void writePairPos(Subtable &sub);
    void freePairPos(Subtable &sub);

    void fillExtensionPos(uint32_t ExtensionLookupType, ExtensionPosFormat1 &tbl);
    void writeExtension(Subtable &sub);
    void freeExtension(Subtable &sub);

    void fillSizeFeature(Subtable &sub);
    void writeFeatParam(Subtable &sub);
    void fillChain();
    void writeChainPos(Subtable &sub);
    void freeChain3(Subtable &sub);
    void freeChain(Subtable &sub);
    void checkBaseAnchorConflict(BaseGlyphRec *baseGlyphArray, int32_t recCnt,
                                 int32_t isMarkToLigature);
    int32_t findMarkClassIndex(SubtableInfo *si, GNode *markNode);
    int32_t addMarkClass(SubtableInfo *si, GNode *markNode);
    void AdCursive(SubtableInfo *si, GNode *targ, int32_t anchorCount,
                   const AnchorMarkInfo *anchorMarkInfo, const char *locDesc);
    void addMark(SubtableInfo *si, GNode *targ, int32_t anchorCount,
                 const AnchorMarkInfo *anchorMarkInfo, const char *locDesc);
    void fillMarkToBase();
    void writeMarkToBase(Subtable &sub);
    void freeMarkToBase(Subtable &sub);
    void fillMarkToLigature();
    void writeMarkToLigature(Subtable &sub);
    void freeMarkToLigature(Subtable &sub);
    void fillCursive();
    void writeCursive(Subtable &sub);
    void freeCursive(Subtable &sub);
    void createAnonLookups();
    void setAnonLookupIndices();
    SubtableInfo *addAnonPosRule(SubtableInfo *cur_si, uint16_t lkpType,
                                 GNode *targ);
    void AddSingle(SubtableInfo *si, GNode *targ, int32_t xPlacement,
                   int32_t yPlacement, int32_t xAdvance, int32_t yAdvance);
#if HOT_DEBUG
    void dumpSingles();
    void rulesDump();
#endif
    SubtableInfo nw;
    struct {
        LOffset featParam {0};        /* (Cumulative.) Next subtable offset |->  */

        LOffset subtable {0};         /* (Cumulative.) Next subtable offset |->  */
                                  /* start of subtable section. LOffset to   */
                                  /* check for overflow                      */
        LOffset extension {0};        /* (Cumulative.) Next extension subtable   */
                                  /* offset |-> start of extension section.  */
        LOffset extensionSection {0}; /* Start of extension section |-> start of */
                                  /* main subtable section                   */
    } offset;
    std::vector<short> values;    /* Concatenated value record fields */
    std::vector<std::unique<Subtable>> subtables;  /* Subtable list */
    uint16_t featNameID {0};          /* user name ID for sub-family name for 'size' feature.            */
                                  /* needed in order to set the FeatureParam subtable on writing it. */

    int16_t startNewPairPosSubtbl {0};
    ClassDef classDef[2];

    int16_t hadError {0}; /* Flags if error occurred */

    /* Info for chaining contextual lookups */
    std::vector<SubtableInfo> anonSubtable;  /* Anon subtable accumulator */
    std::vector<PosLookupRecord *> posLookup;  /* Pointers to all records that need to be adjusted */

    uint16_t maxContext {0};

    otlTbl otl {nullptr};  /* OTL table */
    hotCtx g;
};

#endif  // ADDFEATURES_HOTCONV_GPOS_H_
