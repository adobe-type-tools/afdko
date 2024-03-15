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
    enum ParamFlag {
        ParamOpticalSize,
        ParamSubFamilyID,
        ParamMenuNameID,
        ParamLowEndRange,
        ParamHighEndRange
    };

 private:
    struct SingleRec {
        SingleRec(GID gid, uint32_t valFmt) : gid(gid), valFmt(valFmt) {}
        SingleRec(GID gid, uint32_t valFmt, const MetricsInfo &mi) : gid(gid),
             valFmt(valFmt), metricsInfo(mi) {}
        SingleRec(const SingleRec &o) = delete;
        SingleRec(SingleRec &&o) = default;
        SingleRec &operator=(SingleRec &&o) = default;
        SingleRec &operator=(const SingleRec &o) = delete;
        static bool cmp(const GPOS::SingleRec &a, const GPOS::SingleRec &b);
        static bool cmpGID(const GPOS::SingleRec &a, const GPOS::SingleRec &b);
        bool cmpWithRule(GPat::ClassRec &cr, std::unordered_set<GID> &found);
#if HOT_DEBUG
        void dump(GPOS &h);
#endif
        GID gid {0};
        uint32_t valFmt {0};
        MetricsInfo metricsInfo;
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
        BaseGlyphRec() = default;
        BaseGlyphRec(const BaseGlyphRec &o) = delete;
        BaseGlyphRec(BaseGlyphRec &&o) = default;
        BaseGlyphRec &operator=(BaseGlyphRec &&o) = default;
        BaseGlyphRec &operator=(const BaseGlyphRec &o) = delete;
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
        KernRec() = delete;
        KernRec(GID first, GID second, MetricsInfo &mi1, MetricsInfo &mi2) : first(first),
                second(second), metricsInfo1(mi1), metricsInfo2(mi2) {}
        KernRec(const KernRec &kr) = delete;
        KernRec(KernRec &&kr) = default;
        KernRec & operator=(KernRec &&kr) = default;
        KernRec & operator=(const KernRec &kr) = delete;
        bool operator < (const KernRec &rhs) const {
            if (first != rhs.first)
                return first < rhs.first;
            else if (second != rhs.second)
                return second < rhs.second;
            else if (!(metricsInfo1 == rhs.metricsInfo1))
                return metricsInfo1.kernlt(rhs.metricsInfo1);
            return metricsInfo2.kernlt(rhs.metricsInfo2);
        }
#if HOT_DEBUG
        void dumpStats(hotCtx g);
        void dump(hotCtx g);
        void dump(hotCtx g, GPat::ClassRec &gclass1, GPat::ClassRec &gclass2);
#endif
        GID first {GID_UNDEF};
        GID second {GID_UNDEF};
        MetricsInfo metricsInfo1;
        MetricsInfo metricsInfo2;
    };

    struct PosRule {
        PosRule() = delete;
        explicit PosRule(GPat::SP targ) : targ(std::move(targ)) {}
        GPat::SP targ;
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
        static LOffset single1DevOffset(uint32_t valFmt) {
            auto nval = MetricsInfo::numValues(valFmt);
            auto nvar = MetricsInfo::numVariables(valFmt);
            return sizeof(uint16_t) * 3 + sizeof(int16_t) * nval +
                   sizeof(int16_t) * nvar;
        }
        static LOffset single1Size(uint32_t valFmt) {
            auto nvar = MetricsInfo::numVariables(valFmt);
            return single1DevOffset(valFmt) + sizeof(uint16_t) * 3 * nvar;
        }
        static LOffset single2DevOffset(uint32_t valCnt, uint32_t valFmt) {
            auto nval = MetricsInfo::numValues(valFmt);
            auto nvar = MetricsInfo::numVariables(valFmt);
            return sizeof(uint16_t) * 4 +
                   (sizeof(int16_t) * nval + sizeof(uint16_t) * nvar) * valCnt;
        }
        static LOffset single2Size(uint32_t valCnt, uint32_t valFmt) {
            auto nvar = MetricsInfo::numVariables(valFmt);
            return single2DevOffset(valCnt, valFmt) + sizeof(uint16_t) * 3 * nvar;
        }
        static LOffset pos1Size(SubtableInfo &si, int iStart);
        static LOffset pos2Size(SubtableInfo &si, int iStart, int iEnd);
        static LOffset allPos1Size(SubtableInfo &si, int &nsub);
        static LOffset allPos2Size(SubtableInfo &si, int &nsub);
        static void fill(GPOS &h, SubtableInfo &si);

        LOffset Coverage {0};         /* 32-bit for overflow check */
        uint16_t ValueFormat {0};
        ValueIndex valueIndex {VAL_REC_UNDEF};
    };

    struct PairSet {
        explicit PairSet(LOffset o) : offset(o) {}
        LOffset offset {0};
        std::vector<GID> secondGlyphs;
    };

    struct PairPos : public Subtable {
        struct Format1;
        struct Format2;
        PairPos() = delete;
        PairPos(GPOS &h, SubtableInfo &si);
        virtual ~PairPos() {}
        static LOffset pairSetSize(uint32_t nRecs, uint32_t nvals,
                                   uint32_t nvars) {
            return sizeof(uint16_t) * (1 + (1 + nvals + nvars) * nRecs);
        }
        static LOffset pair1Size(uint32_t nPairSets) {
            return sizeof(uint16_t) * (5 + nPairSets);
        }
        static LOffset pair2Size(uint32_t class1cnt, uint32_t class2cnt,
                                 uint32_t nval, uint32_t nvar) {
            return sizeof(uint16_t) * (8 + class1cnt * class2cnt * (nval + nvar));
        }
        static void fill(GPOS &h, SubtableInfo &si);
        uint16_t ValueFormat1 {0}, ValueFormat2 {0};
        LOffset Coverage {0};         /* 32-bit for overflow check */
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
    void RuleAdd(int lkpType, GPat::SP targ, std::string &locDesc,
                 std::vector<AnchorMarkInfo> &anchorMarkInfo);
    void LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                     bool useExtension, uint16_t useMarkSetIndex);
    void LookupEnd(SubtableInfo *si = nullptr);
    void AddParameters(const std::vector<uint16_t> &params);
    void SetSizeMenuNameID(uint16_t nameID);
    bool SubtableBreak();

 private:
    ValueIndex addValue(const VarValueRecord &vvr);
    void setDevOffset(ValueIndex vi, LOffset o);
    ValueIndex nextValueIndex();
    void reuseClassDefs();
    LOffset recordValues(uint32_t valFmt, MetricsInfo &mi, LOffset o);
    void writeValueRecord(uint32_t valFmt, ValueIndex i) override;
    void writeVarSubtables(uint32_t valFmt, ValueIndex i) override;
    void prepSinglePos(SubtableInfo &si);
    void AddSubtable(std::unique_ptr<OTL::Subtable> s) override;
    bool validInClassDef(int classDefInx, GPat::ClassRec &cr, uint32_t &cls, bool &insert);
    void insertInClassDef(ClassDef &cdef, GPat::ClassRec &cr, uint32_t cls);
    void addSpecPair(SubtableInfo &si, GID first, GID second,
                     MetricsInfo &mi1, MetricsInfo &mi2);
    void AddPair(SubtableInfo &si, GPat::ClassRec &cr1, GPat::ClassRec &cr2,
                 bool enumerate, std::string &locDesc);
    void addPosRule(SubtableInfo &si, GPat::SP targ, std::string &locDesc,
                    std::vector<AnchorMarkInfo> &anchorMarkInfo);
    GPat::ClassRec &getCR(uint32_t cls, int classDefInx);
    void printKernPair(GID gid1, GID gid2, MetricsInfo &mi1, MetricsInfo &mi2,
                       bool fmt1);
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
                   std::unordered_set<GID> &found, MetricsInfo &mi);
#if HOT_DEBUG
    void dumpSingles(std::vector<SingleRec> &singles);
    void rulesDump(SubtableInfo &si);
#endif
    SubtableInfo nw;
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
};

struct GPOS::SinglePos::Format2 : public GPOS::SinglePos {
    Format2() = delete;
    Format2(GPOS &h, GPOS::SubtableInfo &si, int iStart, int iEnd);
    void write(OTL *h) override;
    uint16_t subformat() override { return 2; }
    static void fill(GPOS &h, SubtableInfo &si);
    static void fillOne(GPOS &h, SubtableInfo &si, int iStart, int iEnd);

    uint16_t valueCount {0};
};

struct GPOS::PairPos::Format1 : public GPOS::PairPos {
    Format1() = delete;
    Format1(GPOS &h, SubtableInfo &si);
    void write(OTL *h) override;
    uint16_t subformat() override { return 1; }

    ValueIndex valueIndex {VAL_REC_UNDEF};
    std::vector<PairSet> PairSets;
};

struct GPOS::PairPos::Format2 : public GPOS::PairPos {
    Format2() = delete;
    Format2(GPOS &h, SubtableInfo &si);
    void write(OTL *h) override;
    uint16_t subformat() override { return 2; }

    LOffset ClassDef1 {0};
    LOffset ClassDef2 {0};
    std::vector<std::vector<ValueIndex>> classRecords;
};

#endif  // ADDFEATURES_HOTCONV_GPOS_H_
