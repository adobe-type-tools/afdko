/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_GPOS_H_
#define ADDFEATURES_HOTCONV_GPOS_H_

#include <array>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <string>
// XXX  #include <unordered_map>
#include <utility>

#include "common.h"
#include "feat.h"
#include "otl.h"

#define GPOS_ TAG('G', 'P', 'O', 'S')

/* Standard functions */

typedef int32_t ValueRecord; /* Stores index into h->values, which is read at write time. If -1, then write 0; */
#define VAL_REC_UNDEF (-1)


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
    GPOSContext,
    GPOSChain,
    GPOSExtension,    /* Handled specially: it points to any of the above */
    GPOSFeatureParam, /* Treated like a lookup in the code */
};

class GPOS {
 public:
    struct PosLookupRecord {
        uint16_t SequenceIndex {0};
        uint16_t LookupListIndex {0};
    };

    enum ValueRecordFlag {
        ValueXPlacement = (1 << 0),
        ValueYPlacement = (1 << 1),
        ValueXAdvance = (1 << 2),
        ValueYAdvance = (1 << 3)
    };

    enum ParamFlag {
        ParamOpticalSize,
        ParamSubFamilyID,
        ParamMenuNameID,
        ParamLowEndRange,
        ParamHighEndRange
    };

    struct MetricsRec {
        void set(int16_t xPla, int16_t yPla, int16_t xAdv, int16_t yAdv) {
            value[0] = xPla;
            value[1] = yPla;
            value[2] = xAdv;
            value[3] = yAdv;
        }
        bool operator < (const MetricsRec &rhs) const {
            for (int i = 3; i >= 0; i--)
                if (value[i] != rhs.value[i])
                    return value[i] < rhs.value[i];
            return false;
        }
        bool kernlt(const MetricsRec &o) const {
            for (auto i : kernorder) {
                if (value[i] == 0 && o.value[i] != 0)
                    return true;
                if (value[i] != 0 && o.value[i] == 0)
                    return false;
                if (ABS(value[i]) != ABS(o.value[i]))
                    return ABS(value[i]) < ABS(o.value[i]);
                else if (value[i] != o.value[i])
                    return value[i] < o.value[i];
            }
            return false;
        }
        bool operator == (const MetricsRec &rhs) const {
            for (int i = 3; i >= 0; i--)
                if (value[i] != rhs.value[i])
                    return false;
            return true;
        }
        int16_t value[4] {0};
        static const std::array<int, 4> kernorder;
    };

    struct SingleRec {
        GID gid {0};
        MetricsRec metricsRec;
        uint16_t valFmt {0};
        struct {
            int16_t valFmt {0};
            int16_t valRec {0};
        } span;
    #if HOT_DEBUG
        void dump(GPOS &h);
    #endif
    };

    struct MarkClassRec {
        explicit MarkClassRec(GNode *gnode) : gnode(gnode) {}
        GNode *gnode {nullptr};
    };

    struct BaseGlyphRec {
        bool operator < (const BaseGlyphRec &rhs) const {
            if (gid != rhs.gid)
                return gid < rhs.gid;
            if (anchorMarkInfo.size() != rhs.anchorMarkInfo.size())
                return anchorMarkInfo.size() < rhs.anchorMarkInfo.size();
            for (uint64_t i = 0; i < anchorMarkInfo.size(); i++) {
                if (!(anchorMarkInfo[i] == rhs.anchorMarkInfo[i]))
                    return anchorMarkInfo[i] < rhs.anchorMarkInfo[i];
            }
            return false;
        }
        GID gid {0};
        std::vector<AnchorMarkInfo> anchorMarkInfo;
        int32_t componentIndex {0};
        std::string locDesc;
    };

    struct KernRec {
        GID first {GID_UNDEF};
        GID second {GID_UNDEF};
        MetricsRec metricsRec1;
        MetricsRec metricsRec2;
        bool operator < (const KernRec &rhs) const {
            if (first != rhs.first)
                return first < rhs.first;
            else if (second != rhs.second)
                return second < rhs.second;
            else if (!(metricsRec1 == rhs.metricsRec1))
                return metricsRec1.kernlt(rhs.metricsRec1);
            return metricsRec2.kernlt(rhs.metricsRec2);
        }
    };

    struct PosRule {
        PosRule() = delete;
        explicit PosRule(GNode *targ) : targ(targ) {}
        GNode *targ;
    };

    struct ClassInfo {
        ClassInfo() = delete;
        ClassInfo(uint32_t cls, GNode *gc) : cls(cls), gc(gc) {}
        uint32_t cls;
        GNode *gc;
    };

    struct ClassDef {
        void reset(hotCtx g) {
            for (auto ci : classInfo)
                featRecycleNodes(g, ci.second.gc);
            classInfo.clear();
            cov.clear();
        }
        std::map<GID, ClassInfo> classInfo;
        std::set<GID> cov;
    };

    struct SubtableInfo {  /* New subtable data */
        void reset(uint32_t lt, uint32_t lf, Label l, bool ue, uint16_t umsi) {
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
            singles.clear();
            pairs.clear();
            params.fill(0);
            glyphsSeen.clear();
        }
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        Tag parentFeatTag {0}; /* The parent feature for anonymous lookups made by a chaining contextual feature */
        bool useExtension {false}; /* Use extension lookupType? */
        uint16_t lkpType {0};
        uint16_t parentLkpType {0};
        uint16_t lkpFlag {0};
        uint16_t markSetIndex {0};
        Label label {0};
        std::array<uint16_t, 5> params;

        std::vector<PosRule> rules;
        std::vector<MarkClassRec> markClassList;
        std::vector<BaseGlyphRec> baseList;

        std::vector<SingleRec> singles;
        std::vector<KernRec> pairs;
        // XXX unordered_map?
        std::map<std::pair<GID, GID>, uint32_t> glyphsSeen;
        int16_t pairFmt {0};       /* Fmt (1 or 2) of GPOS pair */
        uint16_t pairValFmt1 {0};  /* Fmt (1 or 2) of first value record  of GPOS pair */
        uint16_t pairValFmt2 {0};  /* Fmt (1 or 2) of second value record of GPOS pair */
    };

    struct ExtensionPosFormat1 {
        static uint16_t size() { return sizeof(uint16_t) * 2 + sizeof(uint32_t); }
        uint16_t PosFormat {0};
        uint16_t ExtensionLookupType {0};
        LOffset ExtensionOffset {0};
    };

    struct Subtable {
        Subtable() = delete;
        Subtable(GPOS &h, SubtableInfo &si);
        virtual void write(GPOS *h) { }  // For reference tables
        virtual uint16_t subformat() { return 0; }
        virtual std::vector<PosLookupRecord> *getPosLookups() { return nullptr; }
        bool isRef() { return IS_REF_LAB(label); }
        otlTbl &getOtl(GPOS &h) {
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
            ExtensionPosFormat1 tbl;

            /* Subtable data */
        } extension;
    };

    struct FeatureParameters : public Subtable {
        FeatureParameters() = delete;
        FeatureParameters(GPOS &h, SubtableInfo &si, std::array<uint16_t, 5> &p);
        static void fill(GPOS &h, SubtableInfo &si);
        void write(GPOS *h) override;
        uint16_t subformat() override { return 0; }
        std::array<uint16_t, 5> params;
    };

    struct SinglePos : public Subtable {
        SinglePos() = delete;
        SinglePos(GPOS &h, SubtableInfo &si);
        static LOffset single1Size(uint32_t nval) {
            return sizeof(uint16_t) * 3 + sizeof(int16_t) * nval;
        }
        static LOffset single2Size(uint32_t valCnt, uint32_t nval) {
            return sizeof(uint16_t) * 4 + sizeof(int16_t) * nval * valCnt;
        }
        static LOffset pos1Size(SubtableInfo &si, int iStart, int iEnd);
        static LOffset pos2Size(SubtableInfo &si, int iStart, int iEnd);
        static LOffset allPos1Size(SubtableInfo &si, int &nsub);
        static LOffset allPos2Size(SubtableInfo &si, int &nsub);
        static void fill(GPOS &h, SubtableInfo &si);
    };

    struct PairPos : public Subtable {
        PairPos() = delete;
        PairPos(GPOS &h, SubtableInfo &si);
        static LOffset pairSetSize(uint32_t nRecs, uint32_t nvals1,
                                   uint32_t nvals2) {
            return sizeof(uint16_t) * (1 + (1 + nvals1 + nvals2) * nRecs);
        }
        static LOffset pair1Size(uint32_t nPairSets) {
            return sizeof(uint16_t) * (5 + nPairSets);
        }
        static LOffset pair2Size(uint32_t class1cnt, uint32_t class2cnt,
                                 uint32_t nval) {
            return sizeof(uint16_t) * (8 + class1cnt * class2cnt * nval);
        }
        static void fill(GPOS &h, SubtableInfo &si);
    };

    struct ChainContextPos : public Subtable {
        ChainContextPos() = delete;
        ChainContextPos(GPOS &h, SubtableInfo &si);
        static LOffset posLookupRecSize() { return sizeof(uint16_t) * 2; }
        static LOffset chain3Size(uint32_t nback, uint32_t ninput,
                                  uint32_t nlook, uint32_t npos) {
            return sizeof(uint16_t) * (5 + nback + ninput + nlook) +
                   posLookupRecSize() * npos;
        }
        static void fill(GPOS &h, SubtableInfo &si);
    };

    struct AnchorRec {
        AnchorRec(LOffset o, AnchorMarkInfo a) : offset(o), anchor(a) {}
        LOffset offset;
        AnchorMarkInfo anchor;
    };

    struct AnchorPosBase : public Subtable {
        AnchorPosBase() = delete;
        AnchorPosBase(GPOS &h, SubtableInfo &si);
        virtual LOffset getAnchorOffset(const AnchorMarkInfo &anchor);

        std::vector<AnchorRec> anchorList;
        LOffset endArrays;                 /* not part of font data */
    };

#define EXTENSION1_SIZE (sizeof(uint16_t) * 2 + sizeof(uint32_t))

    GPOS() = delete;
    explicit GPOS(hotCtx g) : otl(g), g(g) {}

    int Fill();
    void Write();

    void FeatureBegin(Tag script, Tag language, Tag feature);
    void FeatureEnd();
    void RuleAdd(int lkpType, GNode *targ, std::string &locDesc,
                 std::vector<AnchorMarkInfo> &anchorMarkInfo);
    void LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                     bool useExtension, uint16_t useMarkSetIndex);
    void LookupEnd(SubtableInfo *si = nullptr);
    void AddParameters(const std::vector<uint16_t> &params);
    void SetSizeMenuNameID(uint16_t nameID);
    bool SubtableBreak();
    void featParamsWrite();
    void reuseClassDefs();
    uint32_t makeValueFormat(SubtableInfo &si, int32_t xPla, int32_t yPla,
                             int32_t xAdv, int32_t yAdv);
    uint32_t recordValues(uint32_t valFmt, MetricsRec &r);
    void writeValueRecord(unsigned valFmt, ValueRecord i);
    void prepSinglePos(SubtableInfo &si);
    void AddSubtable(std::unique_ptr<Subtable> s);
    void EnsureOtl();
    bool validInClassDef(int classDefInx, GNode *gc, uint32_t &cls, bool &insert);
    void insertInClassDef(ClassDef &cdef, GNode *gc, uint32_t cls);
    void addSpecPair(SubtableInfo &si, GID first, GID second,
                     MetricsRec &metricsRec1, MetricsRec &metricsRec2);
    void AddPair(SubtableInfo &si, GNode *first, GNode *second, std::string &locDesc);
    void addPosRule(SubtableInfo &si, GNode *targ, std::string &locDesc,
                    std::vector<AnchorMarkInfo> &anchorMarkInfo);
    GNode *getGNodes(uint32_t cls, int classDefInx);
    void printKernPair(GID gid1, GID gid2, int val1, int val2, bool fmt1);
    Offset classDefMake(otlTbl &otl, int classDefInx, LOffset *coverage,
                        uint16_t &count);


    void fillExtensionPos(uint32_t ExtensionLookupType, ExtensionPosFormat1 &tbl);
    void writeExtension(Subtable &sub);

    void checkBaseAnchorConflict(std::vector<BaseGlyphRec> &baselist);
    void checkBaseLigatureConflict(std::vector<BaseGlyphRec> &baselist);
    int32_t addMarkClass(SubtableInfo &si, GNode *markNode);
    void addCursive(SubtableInfo &si, GNode *targ,
                    std::vector<AnchorMarkInfo> &anchorMarkInfo, std::string &locDesc);
    void addMark(SubtableInfo &si, GNode *targ,
                 std::vector<AnchorMarkInfo> &anchorMarkInfo, std::string &locDesc);
    void createAnonLookups();
    void setAnonLookupIndices();
    void setCoverages(std::vector<LOffset> &covs, otlTbl &otl, GNode *p, uint32_t num);

    SubtableInfo &addAnonPosRule(SubtableInfo &cur_si, uint16_t lkpType,
                                 GNode *targ);
    void AddSingle(SubtableInfo &si, GNode *targ, int32_t xPlacement,
                   int32_t yPlacement, int32_t xAdvance, int32_t yAdvance);
#if HOT_DEBUG
    void dumpSingles(std::vector<SingleRec> &singles);
    void rulesDump(SubtableInfo &si);
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
    std::vector<int16_t> values;    /* Concatenated value record fields */
    std::vector<std::unique_ptr<Subtable>> subtables;  /* Subtable list */
    uint16_t featNameID {0};          /* user name ID for sub-family name for 'size' feature.            */
                                  /* needed in order to set the FeatureParam subtable on writing it. */

    bool startNewPairPosSubtbl {false};
    ClassDef classDef[2];

    bool hadError {false}; /* Flags if error occurred */

    /* Info for chaining contextual lookups */
    std::vector<SubtableInfo> anonSubtable;  /* Anon subtable accumulator */

    uint16_t maxContext {0};

    otlTbl otl;  /* OTL table */
    hotCtx g;
};

struct SinglePosFormat1 : public GPOS::SinglePos {
    SinglePosFormat1() = delete;
    SinglePosFormat1(GPOS &h, GPOS::SubtableInfo &si, int iStart, int iEnd);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }
    static void fill(GPOS &h, GPOS::SubtableInfo &si);
    static void fillOne(GPOS &h, GPOS::SubtableInfo &si);

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    ValueRecord Value {VAL_REC_UNDEF};
};

struct SinglePosFormat2 : public GPOS::SinglePos {
    SinglePosFormat2() = delete;
    SinglePosFormat2(GPOS &h, GPOS::SubtableInfo &si, int iStart, int iEnd);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 2; }
    static void fill(GPOS &h, GPOS::SubtableInfo &si);
    static void fillOne(GPOS &h, GPOS::SubtableInfo &si, int iStart, int iEnd);

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    std::vector<ValueRecord> Values;
};

struct PairValueRecord {
    GID SecondGlyph {GID_UNDEF};
    ValueRecord Value1 {VAL_REC_UNDEF};
    ValueRecord Value2 {VAL_REC_UNDEF};
};

struct PairSet {
    LOffset offset {0};
    std::vector<PairValueRecord> values;
};

struct PairPosFormat1 : public GPOS::PairPos {
    PairPosFormat1() = delete;
    PairPosFormat1(GPOS &h, GPOS::SubtableInfo &si);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat1 {0}, ValueFormat2 {0};
    std::vector<PairSet> PairSets;
};

struct Class2Record {
    ValueRecord Value1 {VAL_REC_UNDEF};
    ValueRecord Value2 {VAL_REC_UNDEF};
};

struct PairPosFormat2 : public GPOS::PairPos {
    PairPosFormat2() = delete;
    PairPosFormat2(GPOS &h, GPOS::SubtableInfo &si);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 2; }

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat1 {0};
    uint16_t ValueFormat2 {0};
    LOffset ClassDef1 {0};
    LOffset ClassDef2 {0};
    std::vector<std::vector<Class2Record>> ClassRecords;
};

struct ChainContextPosFormat3 : public GPOS::ChainContextPos {
    ChainContextPosFormat3() = delete;
    ChainContextPosFormat3(GPOS &h, GPOS::SubtableInfo &si, GPOS::PosRule &rule);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 3; }
    std::vector<GPOS::PosLookupRecord> *getPosLookups() override {
        return &posLookupRecords;
    }

    std::vector<LOffset> backtracks;
    std::vector<LOffset> inputGlyphs;
    std::vector<LOffset> lookaheads;
    std::vector<GPOS::PosLookupRecord> posLookupRecords;
};

struct EntryExitRecord {
    LOffset EntryAnchor;
    LOffset ExitAnchor;
};


struct CursivePosFormat1 : public GPOS::AnchorPosBase {
    CursivePosFormat1() = delete;
    CursivePosFormat1(GPOS &h, GPOS::SubtableInfo &si);
    static LOffset cursive1Size() { return sizeof(uint16_t) * 3; }
    static void fill(GPOS &h, GPOS::SubtableInfo &si);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }

    LOffset Coverage {0};                  /* 32-bit for overflow check */
    std::vector<EntryExitRecord> entryExitRecords;
};

struct MarkRecord {
    MarkRecord(GID g, LOffset a, uint16_t c) : gid(g), anchor(a), cls(c) {}
    bool operator < (const MarkRecord &rhs) const {
        return gid < rhs.gid;
    }
    GID gid {GID_UNDEF};        /* not part of the font data - used to sort recs when building table. */
    LOffset anchor {0}; /* not part of the font data - used while building table. */
    uint16_t cls {0};
};

struct MarkBasePosFormat1 : public GPOS::AnchorPosBase {
    MarkBasePosFormat1() = delete;
    MarkBasePosFormat1(GPOS &h, GPOS::SubtableInfo &si);
    static LOffset markBase1Size() { return sizeof(uint16_t) * 6; }
    static void fill(GPOS &h, GPOS::SubtableInfo &si);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }

    LOffset MarkCoverage {0};
    LOffset BaseCoverage {0};
    uint16_t ClassCount {0};
    LOffset BaseOffset {0};
    std::vector<std::vector<LOffset>> BaseRecords;
    LOffset MarkOffset {0};
    std::vector<MarkRecord> MarkRecords;
};

struct LigatureAttach {
    LOffset offset {0};
    std::vector<std::vector<LOffset>> components;
};

struct MarkLigaturePosFormat1 : public GPOS::AnchorPosBase {
    MarkLigaturePosFormat1() = delete;
    MarkLigaturePosFormat1(GPOS &h, GPOS::SubtableInfo &si);
    static LOffset markLig1Size() { return sizeof(uint16_t) * 6; }
    static void fill(GPOS &h, GPOS::SubtableInfo &si);
    void write(GPOS *h) override;
    uint16_t subformat() override { return 1; }

    LOffset MarkCoverage {0};
    LOffset LigatureCoverage {0};
    uint16_t ClassCount {0};
    LOffset LigatureOffset {0};
    std::vector<LigatureAttach> LigatureAttaches;
    LOffset MarkOffset {0};
    std::vector<MarkRecord> MarkRecords;
};

#endif  // ADDFEATURES_HOTCONV_GPOS_H_
