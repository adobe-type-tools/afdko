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
#include <unordered_set>
#include <utility>

#include "common.h"
#include "FeatCtx.h"
#include "otl.h"

#define GPOS_ TAG('G', 'P', 'O', 'S')

/* Standard functions */

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

class GPOS : public OTL {
 public:
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

 private:
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
        SingleRec(GID gid, uint32_t valFmt) : gid(gid), valFmt(valFmt) {}
        static bool cmp(const GPOS::SingleRec &a, const GPOS::SingleRec &b);
        static bool cmpGID(const GPOS::SingleRec &a, const GPOS::SingleRec &b);
        bool cmpWithRule(GPat::ClassRec &cr, std::unordered_set<GID> &found);
#if HOT_DEBUG
        void dump(GPOS &h);
#endif
        GID gid {0};
        MetricsRec metricsRec;
        uint32_t valFmt {0};
        struct {
            int16_t valFmt {0};
            int16_t valRec {0};
        } span;
    };

    struct MarkClassRec {
        MarkClassRec(std::string &name, GPat::ClassRec &cr) : name(name), cr(cr) {}
        std::string name;
        GPat::ClassRec cr;
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
        static bool cmpLig(const GPOS::BaseGlyphRec &a, const GPOS::BaseGlyphRec &b);
        GID gid {0};
        std::vector<AnchorMarkInfo> anchorMarkInfo;
        int32_t componentIndex {0};
        std::string locDesc;
    };

    struct KernRec {
        bool operator < (const KernRec &rhs) const {
            if (first != rhs.first)
                return first < rhs.first;
            else if (second != rhs.second)
                return second < rhs.second;
            else if (!(metricsRec1 == rhs.metricsRec1))
                return metricsRec1.kernlt(rhs.metricsRec1);
            return metricsRec2.kernlt(rhs.metricsRec2);
        }
#if HOT_DEBUG
        void dumpStats(hotCtx g);
        void dump(hotCtx g);
        void dump(hotCtx g, GPat::ClassRec &gclass1, GPat::ClassRec &gclass2);
#endif
        GID first {GID_UNDEF};
        GID second {GID_UNDEF};
        MetricsRec metricsRec1;
        MetricsRec metricsRec2;
    };

    struct PosRule {
        PosRule() = delete;
        explicit PosRule(GPat *targ) : targ(targ) {}
        GPat *targ;
    };

    struct ClassInfo {
        ClassInfo() = delete;
        ClassInfo(uint32_t cls, GPat::ClassRec &cr) : cls(cls), cr(cr) {}
        ClassInfo(uint32_t cls, GPat::ClassRec &&cr) : cls(cls), cr(std::move(cr)) {}
        explicit ClassInfo(const ClassInfo &o) : cls(o.cls), cr(o.cr) {}
        explicit ClassInfo(const ClassInfo &&o) : cls(o.cls), cr(std::move(o.cr)) {}
        uint32_t cls;
        GPat::ClassRec cr;
    };

    struct ClassDef {
        void reset(hotCtx g) {
            classInfo.clear();
            cov.clear();
        }
        std::map<GID, ClassInfo> classInfo;
        std::set<GID> cov;
    };

    struct SubtableInfo : public OTL::SubtableInfo {  /* New subtable data */
        SubtableInfo() : OTL::SubtableInfo() {}
        SubtableInfo(SubtableInfo &&si) : OTL::SubtableInfo(std::move(si)),
                parentLkpType(si.parentLkpType), pairFmt(si.pairFmt),
                pairValFmt1(si.pairValFmt1),  pairValFmt2(si.pairValFmt2) {
            params = si.params;
            rules.swap(si.rules);
            markClassList.swap(si.markClassList);
            baseList.swap(si.baseList);
            singles.swap(si.singles);
            pairs.swap(si.pairs);
            glyphsSeen.swap(si.glyphsSeen);
        }
        void reset(uint32_t lt, uint32_t lf, Label l, bool ue, uint16_t umsi) override {
            OTL::SubtableInfo::reset(lt, lf, l, ue, umsi);
            params.fill(0);
            rules.clear();
            markClassList.clear();
            baseList.clear();
            singles.clear();
            pairs.clear();
            glyphsSeen.clear();
            parentLkpType = 0;
            pairFmt = 0;
            pairValFmt1 = 0;
            pairValFmt2 = 0;
        }
        ~SubtableInfo() override {}
        bool checkAddRule(GPat::ClassRec &cr, std::unordered_set<GID> &found);
        int32_t findMarkClassIndex(std::string &name);
        std::array<uint16_t, 5> params;

        std::vector<PosRule> rules;
        std::vector<MarkClassRec> markClassList;
        std::vector<BaseGlyphRec> baseList;

        std::vector<SingleRec> singles;
        std::vector<KernRec> pairs;
        std::map<std::pair<GID, GID>, uint32_t> glyphsSeen;
        uint16_t parentLkpType {0};
        int16_t pairFmt {0};       /* Fmt (1 or 2) of GPOS pair */
        uint16_t pairValFmt1 {0};  /* Fmt (1 or 2) of first value record  of GPOS pair */
        uint16_t pairValFmt2 {0};  /* Fmt (1 or 2) of second value record of GPOS pair */
    };

    struct Subtable : public OTL::Subtable {
        Subtable() = delete;
        Subtable(GPOS &h, SubtableInfo &si);
        ~Subtable() override {}
        void write(OTL *) override {}
    };

    struct FeatureParameters : public Subtable {
        FeatureParameters() = delete;
        FeatureParameters(GPOS &h, SubtableInfo &si, std::array<uint16_t, 5> &p);
        static void fill(GPOS &h, SubtableInfo &si);
        void write(OTL *h) override;
        uint16_t subformat() override { return 0; }
        std::array<uint16_t, 5> params;
    };

    struct SinglePos : public Subtable {
        struct Format1;
        struct Format2;
        SinglePos() = delete;
        SinglePos(GPOS &h, SubtableInfo &si);
        virtual ~SinglePos() {}
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

    struct PairValueRecord {
        GID SecondGlyph {GID_UNDEF};
        ValueRecord Value1 {VAL_REC_UNDEF};
        ValueRecord Value2 {VAL_REC_UNDEF};
    };

    struct PairSet {
        LOffset offset {0};
        std::vector<PairValueRecord> values;
    };

    struct Class2Record {
        ValueRecord Value1 {VAL_REC_UNDEF};
        ValueRecord Value2 {VAL_REC_UNDEF};
    };

    struct PairPos : public Subtable {
        struct Format1;
        struct Format2;
        PairPos() = delete;
        PairPos(GPOS &h, SubtableInfo &si);
        virtual ~PairPos() {}
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
        ChainContextPos(GPOS &h, SubtableInfo &si, GPOS::PosRule &rule);
        static LOffset posLookupRecSize() { return sizeof(uint16_t) * 2; }
        static LOffset chain3Size(uint32_t nback, uint32_t ninput,
                                  uint32_t nlook, uint32_t npos) {
            return sizeof(uint16_t) * (5 + nback + ninput + nlook) +
                   posLookupRecSize() * npos;
        }
        std::vector<LookupRecord> *getLookups() override { return &lookupRecords; }
        void write(OTL *h) override;
        uint16_t subformat() override { return 3; }
        static void fill(GPOS &h, SubtableInfo &si);

        std::vector<LOffset> backtracks;
        std::vector<LOffset> inputGlyphs;
        std::vector<LOffset> lookaheads;
        std::vector<LookupRecord> lookupRecords;
    };

    struct EntryExitRecord {
        LOffset EntryAnchor;
        LOffset ExitAnchor;
    };

    struct AnchorRec {
        AnchorRec(LOffset o, AnchorMarkInfo a) : offset(o), anchor(a) {}
        LOffset offset;
        AnchorMarkInfo anchor;
    };

    struct AnchorPosBase : public Subtable {
        AnchorPosBase() = delete;
        AnchorPosBase(GPOS &h, SubtableInfo &si);
        virtual ~AnchorPosBase() {}
        virtual LOffset getAnchorOffset(const AnchorMarkInfo &anchor);

        std::vector<AnchorRec> anchorList;
        LOffset endArrays;                 /* not part of font data */
    };


    struct CursivePos : public GPOS::AnchorPosBase {
        CursivePos() = delete;
        CursivePos(GPOS &h, GPOS::SubtableInfo &si);
        static LOffset cursive1Size() { return sizeof(uint16_t) * 3; }
        static void fill(GPOS &h, GPOS::SubtableInfo &si);
        void write(OTL *h) override;
        uint16_t subformat() override { return 1; }

        LOffset Coverage {0};                  /* 32-bit for overflow check */
        std::vector<EntryExitRecord> entryExitRecords;
    };

    struct MarkRecord {
        MarkRecord(GID g, LOffset a, uint16_t c) : gid(g), anchor(a), cls(c) {}
        bool operator < (const MarkRecord &rhs) const {
            return gid < rhs.gid;
        }
        GID gid {GID_UNDEF};  // not part of the font data - used to sort recs when building table.
        LOffset anchor {0};   // not part of the font data - used while building table.
        uint16_t cls {0};
    };

    struct MarkBasePos : public AnchorPosBase {
        MarkBasePos() = delete;
        MarkBasePos(GPOS &h, GPOS::SubtableInfo &si);
        static LOffset markBase1Size() { return sizeof(uint16_t) * 6; }
        static void fill(GPOS &h, GPOS::SubtableInfo &si);
        void write(OTL *h) override;
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

    struct MarkLigaturePos : public AnchorPosBase {
        MarkLigaturePos() = delete;
        MarkLigaturePos(GPOS &h, GPOS::SubtableInfo &si);
        ~MarkLigaturePos() override {}
        static LOffset markLig1Size() { return sizeof(uint16_t) * 6; }
        static void fill(GPOS &h, GPOS::SubtableInfo &si);
        void write(OTL *h) override;
        uint16_t subformat() override { return 1; }

        LOffset MarkCoverage {0};
        LOffset LigatureCoverage {0};
        uint16_t ClassCount {0};
        LOffset LigatureOffset {0};
        std::vector<LigatureAttach> LigatureAttaches;
        LOffset MarkOffset {0};
        std::vector<MarkRecord> MarkRecords;
    };

#define EXTENSION1_SIZE (sizeof(uint16_t) * 2 + sizeof(uint32_t))

 public:
    GPOS() = delete;
    explicit GPOS(hotCtx g) : OTL(g, GPOSExtension) {}
    virtual ~GPOS() {}

    virtual const char *objName() { return "GPOS"; }

    int Fill();
    void Write();

    void FeatureBegin(Tag script, Tag language, Tag feature);
    void FeatureEnd();
    void RuleAdd(int lkpType, GPat *targ, std::string &locDesc,
                 std::vector<AnchorMarkInfo> &anchorMarkInfo);
    void LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                     bool useExtension, uint16_t useMarkSetIndex);
    void LookupEnd(SubtableInfo *si = nullptr);
    void AddParameters(const std::vector<uint16_t> &params);
    void SetSizeMenuNameID(uint16_t nameID);
    bool SubtableBreak();

 private:
    void reuseClassDefs();
    uint32_t makeValueFormat(SubtableInfo &si, int32_t xPla, int32_t yPla,
                             int32_t xAdv, int32_t yAdv);
    uint32_t recordValues(uint32_t valFmt, MetricsRec &r);
    void writeValueRecord(uint32_t valFmt, ValueRecord i) override;
    void prepSinglePos(SubtableInfo &si);
    void AddSubtable(std::unique_ptr<OTL::Subtable> s) override;
    bool validInClassDef(int classDefInx, GPat::ClassRec &cr, uint32_t &cls, bool &insert);
    void insertInClassDef(ClassDef &cdef, GPat::ClassRec &cr, uint32_t cls);
    void addSpecPair(SubtableInfo &si, GID first, GID second,
                     MetricsRec &metricsRec1, MetricsRec &metricsRec2);
    void AddPair(SubtableInfo &si, GPat::ClassRec &cr1, GPat::ClassRec &cr2,
                 bool enumerate, std::string &locDesc);
    void addPosRule(SubtableInfo &si, GPat *targ, std::string &locDesc,
                    std::vector<AnchorMarkInfo> &anchorMarkInfo);
    GPat::ClassRec &getCR(uint32_t cls, int classDefInx);
    void printKernPair(GID gid1, GID gid2, int val1, int val2, bool fmt1);
    Offset classDefMake(std::shared_ptr<CoverageAndClass> &cac, int classDefInx,
                        LOffset *coverage, uint16_t &count);

    void checkBaseAnchorConflict(std::vector<BaseGlyphRec> &baselist);
    void checkBaseLigatureConflict(std::vector<BaseGlyphRec> &baselist);
    int32_t addMarkClass(SubtableInfo &si, std::string &name, GPat::ClassRec &ncr);
    void addCursive(SubtableInfo &si, GPat::ClassRec &cr,
                    std::vector<AnchorMarkInfo> &anchorMarkInfo, std::string &locDesc);
    void addMark(SubtableInfo &si, GPat::ClassRec &cr,
                 std::vector<AnchorMarkInfo> &anchorMarkInfo, std::string &locDesc);
    void createAnonLookups() override;
    SubtableInfo &newAnonSubtable(SubtableInfo &cur_si, uint16_t lkpType);
    SubtableInfo &addAnonPosRuleSing(SubtableInfo &cur_si, GPat::ClassRec &cr,
                                     std::unordered_set<GID> &found);
    SubtableInfo &addAnonPosRule(SubtableInfo &cur_si, uint16_t lkpType);
    void AddSingle(SubtableInfo &si, GPat::ClassRec &cr,
                   std::unordered_set<GID> &found, int32_t xPlacement,
                   int32_t yPlacement, int32_t xAdvance, int32_t yAdvance);
#if HOT_DEBUG
    void dumpSingles(std::vector<SingleRec> &singles);
    void rulesDump(SubtableInfo &si);
#endif
    SubtableInfo nw;
    std::vector<int16_t> values;    /* Concatenated value record fields */
    uint16_t featNameID {0};          /* user name ID for sub-family name for 'size' feature.            */
                                  /* needed in order to set the FeatureParam subtable on writing it. */
    bool startNewPairPosSubtbl {false};
    ClassDef classDef[2];

    /* Info for chaining contextual lookups */
    std::vector<SubtableInfo> anonSubtable;  /* Anon subtable accumulator */
};

struct GPOS::SinglePos::Format1 : public GPOS::SinglePos {
    Format1() = delete;
    Format1(GPOS &h, SubtableInfo &si, int iStart, int iEnd);
    void write(OTL *h) override;
    uint16_t subformat() override { return 1; }
    static void fill(GPOS &h, SubtableInfo &si);
    static void fillOne(GPOS &h, SubtableInfo &si);

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    ValueRecord Value {VAL_REC_UNDEF};
};

struct GPOS::SinglePos::Format2 : public GPOS::SinglePos {
    Format2() = delete;
    Format2(GPOS &h, GPOS::SubtableInfo &si, int iStart, int iEnd);
    void write(OTL *h) override;
    uint16_t subformat() override { return 2; }
    static void fill(GPOS &h, SubtableInfo &si);
    static void fillOne(GPOS &h, SubtableInfo &si, int iStart, int iEnd);

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat {0};
    std::vector<ValueRecord> Values;
};

struct GPOS::PairPos::Format1 : public GPOS::PairPos {
    Format1() = delete;
    Format1(GPOS &h, SubtableInfo &si);
    void write(OTL *h) override;
    uint16_t subformat() override { return 1; }

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat1 {0}, ValueFormat2 {0};
    std::vector<PairSet> PairSets;
};

struct GPOS::PairPos::Format2 : public GPOS::PairPos {
    Format2() = delete;
    Format2(GPOS &h, SubtableInfo &si);
    void write(OTL *h) override;
    uint16_t subformat() override { return 2; }

    LOffset Coverage {0};         /* 32-bit for overflow check */
    uint16_t ValueFormat1 {0};
    uint16_t ValueFormat2 {0};
    LOffset ClassDef1 {0};
    LOffset ClassDef2 {0};
    std::vector<std::vector<Class2Record>> ClassRecords;
};

#endif  // ADDFEATURES_HOTCONV_GPOS_H_
