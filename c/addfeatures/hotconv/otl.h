/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_OTL_H_
#define ADDFEATURES_HOTCONV_OTL_H_

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "feat.h"
#include "common.h"

/* Subtable lookup flags */
enum {
    otlRightToLeft = (1 << 0),
    otlIgnoreBaseGlyphs = (1 << 1),
    otlIgnoreLigatures = (1 << 2),
    otlIgnoreMarks =(1 << 3),
    otlUseMarkFilteringSet = (1 << 4)
};
#define otlMarkAttachmentType 0xFF00 /* Mask */

#define aalt_ TAG('a', 'a', 'l', 't')

// Stores index into h->values, which is read at write time. If -1, then write 0;
typedef int32_t ValueRecord;
#define VAL_REC_UNDEF (-1)

class CoverageAndClass {
 public:
    CoverageAndClass() = delete;
    explicit CoverageAndClass(hotCtx g) : g(g) {}
    virtual void coverageBegin();
    virtual void coverageAddGlyph(GID gid, bool warn = false);
    virtual void coverageWrite();
    virtual Offset coverageEnd();
    virtual LOffset coverageSize() { return coverage.size; }
    virtual void classBegin();
    virtual void classAddMapping(GID gid, uint32_t classId);
    virtual void classWrite();
    virtual Offset classEnd();
    virtual LOffset classSize() { return cls.size; }
#if HOT_DEBUG
    virtual void dump();
#endif

 private:
    virtual Offset coverageFill();
    virtual Offset classFill();
    class CoverageRecord {
     public:
        CoverageRecord() = delete;
        CoverageRecord(Offset o, std::set<GID> &gl);
        Offset cov1size() {
            return sizeof(uint16_t) * (2 + glyphs.size());
        }
        Offset cov2size() {
            return sizeof(uint16_t) * (2 + 3 * ranges.size());
        }
        Offset size() { return ranges.size() != 0 ? cov2size() : cov1size(); }
        void write(hotCtx g);
        Offset offset;
        std::set<GID> glyphs;             // format 1
        struct RangeRecord {
            GID Start {GID_UNDEF};
            GID End {GID_UNDEF};
            uint16_t startCoverageIndex {0};
        };
        std::vector<RangeRecord> ranges;  // format 2
    };

    class ClassRecord {
     public:
        ClassRecord() = delete;
        ClassRecord(Offset o, std::map<GID, uint16_t> &gl);
        Offset cls1size(uint16_t nvalues) {
            return sizeof(uint16_t) * (3 + nvalues);
        }
        Offset cls2size() {
            return sizeof(uint16_t) * (2 + 3 * ranges.size());
        }
        Offset size() {
            return values.size() != 0 ? cls1size(values.size()) : cls2size();
        }
        void write(hotCtx g);
        std::map<GID, uint16_t> map;
        Offset offset;
        GID startGlyph {GID_UNDEF};
        std::vector<uint16_t> values;
        struct ClassRangeRecord {
            GID Start {GID_UNDEF};
            GID End {GID_UNDEF};
            uint16_t classId {0xFFFF};
        };
        std::vector<ClassRangeRecord> ranges;
    };

 protected:
    struct {
        LOffset size {0};
#if HOT_DEBUG
        int16_t reused {0};
#endif
        std::vector<CoverageRecord> records;
        std::set<GID> current;
    } coverage;
    struct {
        LOffset size {0};
#if HOT_DEBUG
        int16_t reused {0};
#endif
        std::vector<ClassRecord> records;
        std::map<GID, uint16_t> current;
    } cls;
    hotCtx g;
};

class OTL {
 public:
    struct LookupRecord {
        LookupRecord() = delete;
        LookupRecord(uint16_t si, uint16_t lli) : SequenceIndex(si), LookupListIndex(lli) {}
        uint16_t SequenceIndex {0};
        uint16_t LookupListIndex {0};
    };
    struct SubtableInfo {  /* New subtable data */
        virtual void reset(uint32_t lt, uint32_t lf, Label l, bool ue, uint16_t umsi) {
            useExtension = ue;
            lkpType = lt;
            label = l;
            lkpFlag = lf;
            markSetIndex = umsi;
            parentFeatTag = 0;
        }
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        Tag parentFeatTag {0};      // The parent feature for anonymous lookups made by a chaining contextual feature
        bool useExtension {false};  // Use extension lookupType?
        uint16_t lkpType {0};
        uint16_t lkpFlag {0};
        uint16_t markSetIndex {0};
        Label label {0};
    };

    class Subtable {
     public:
        struct ExtensionFormat1 {
            static uint16_t size() { return sizeof(uint16_t) * 2 + sizeof(uint32_t); }
            static uint16_t subformat() { return 1; }
            void write(hotCtx g, uint16_t lkpType, LOffset adjust);
            LOffset offset {0};
        };
        Subtable() = delete;
        Subtable(OTL *otl, SubtableInfo *si, std::string &id_text, bool isFeatParam);
        static bool ltLookupList(const std::unique_ptr<Subtable> &a, const std::unique_ptr<Subtable> &b) {
            if (a->isRef() != b->isRef())
                return b->isRef();
            if (a->isParam() != b->isParam())
                return b->isParam();
            if (a->isAALT() != b->isAALT())  // aalt's come first
                return a->isAALT();
            return a->offset < b->offset;
        }
        static bool ltScriptList(const std::unique_ptr<Subtable> &a, const std::unique_ptr<Subtable> &b) {
            if (a->isStandAlone() != b->isStandAlone())
                return b->isStandAlone();
            if (a->isAnon() != b->isAnon())
                return b->isAnon();
            if (a->script != b->script)
                return a->script < b->script;
            if (a->language != b->language)
                return a->language < b->language;
            return a->feature < b->feature;
        }
        static bool ltFeatureList(const std::unique_ptr<Subtable> &a, const std::unique_ptr<Subtable> &b) {
            if (a->isAnon() != b->isAnon())
                return b->isAnon();
            if (a->isStandAlone() != b->isStandAlone())
                return b->isStandAlone();
            if (a->feature != b->feature)
                return a->feature < b->feature;
            if (a->index.feature != b->index.feature)
                return a->index.feature < b->index.feature;
            return a->index.lookup < b->index.lookup;
        }
        static bool ltOffset(const std::unique_ptr<Subtable> &a, const std::unique_ptr<Subtable> &b) {
            return a->offset < b->offset;
        }
        bool isStandAlone() const { return feature == TAG_STAND_ALONE; }
        bool isAnon() const { return script == TAG_UNDEF; }
        bool isRef() const { return IS_REF_LAB(label); }
        bool isParam() const { return isFeatParam; }
        bool isAALT() const { return feature == aalt_; }
        bool isExt() const { return useExtension; }
        virtual uint16_t subformat() { return 0; }
        virtual uint16_t fmt() { return isRef() ? 0 : isExt() ? extension.subformat() : subformat(); }
        virtual std::vector<LookupRecord> *getLookups() { return nullptr; }
        virtual void writeExt(OTL *h, uint32_t extSec) { extension.write(h->g, lkpType, extSec - offset); }
        virtual void write(OTL *h) = 0;
#if HOT_DEBUG
        virtual void dump(typename std::vector<std::unique_ptr<Subtable>>::iterator sb,
                          uint32_t extLkpType);
#endif
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        bool useExtension {false};
        uint16_t lkpType {0};
        uint16_t lkpFlag {0};
        uint16_t markSetIndex {0};
        Offset offset {0};
        Label label {0};
        bool seenInFeature {false};
        bool isFeatParam {false};
        ExtensionFormat1 extension;
        std::string id_text;
        std::shared_ptr<CoverageAndClass> cac;
        struct {
            int16_t feature {-1};
            int16_t lookup {-1};
        } index;
        struct {
            typename std::vector<std::unique_ptr<Subtable>>::iterator script;
            typename std::vector<std::unique_ptr<Subtable>>::iterator language;
            typename std::vector<std::unique_ptr<Subtable>>::iterator feature;
            typename std::vector<std::unique_ptr<Subtable>>::iterator lookup;
        } span;
    };

 private:
    struct LangSys {
        static LOffset size(uint32_t nfeatures) {
            return sizeof(uint16_t) * (3 + nfeatures);
        }
        static LOffset recordSize() {
            return sizeof(uint32_t) + sizeof(uint16_t);
        }
        Offset fill(Offset o, typename std::vector<std::unique_ptr<Subtable>>::iterator sl);
        void write(hotCtx g);

        Offset offset {0};
        Tag LangSysTag {TAG_UNDEF};
        Offset LookupOrder {0};
        uint16_t ReqFeatureIndex {0};
        std::vector<uint16_t> featureIndices;
    };

    struct Script {
        Script() = delete;
        Script(Offset o, Tag st) : offset(o), ScriptTag(st) {}
        static LOffset size(uint32_t nlangs) {
            return sizeof(uint16_t) * 2 + nlangs * LangSys::recordSize();
        }
        static LOffset listSize(uint32_t nscripts) {
            return sizeof(uint16_t) +
                   (sizeof(uint32_t) + sizeof(uint16_t)) * nscripts;
        }
        Offset offset {0};
        Tag ScriptTag {TAG_UNDEF};
        LangSys defaultLangSys;
        std::vector<LangSys> langSystems;
    };

    struct Feature {
        Feature() = delete;
        Feature(Offset o, Tag ft) : offset(o), FeatureTag(ft) {}
        static LOffset size(uint32_t nlookups) {
            return sizeof(uint16_t) * (2 + nlookups);
        }
        static LOffset listSize(uint32_t nfeatures) {
            return sizeof(uint16_t) +
                   (sizeof(uint16_t) + sizeof(uint32_t)) * nfeatures;
        }
        Offset offset {0};
        Tag FeatureTag {TAG_UNDEF};
        LOffset featureParams {NULL_OFFSET};
        std::vector<uint16_t> lookupIndices;
    };

    struct Lookup {
        Lookup() = delete;
        Lookup(Offset o, uint16_t type, uint16_t flag) : offset(o), Type(type),
                                        Flag(flag) {}
        static Offset size(uint32_t nsubs) {
            return sizeof(uint16_t) * (3 + nsubs);
        }
        static Offset listSize(uint32_t nlookups) {
            return sizeof(uint16_t) * (1 + nlookups);
        }
        Offset offset;
        uint16_t Type {0};
        uint16_t Flag {0};
        uint16_t useMarkSetIndex {0};
        std::vector<int32_t> subtableOffsets;
    };

    struct Header {
        static Offset size() {
            return sizeof(uint32_t) + sizeof(uint16_t) * 3;
        }
        void write(hotCtx g);
        void lookupListWrite(hotCtx g, Offset lookupSize);
        Fixed Version {VERSION(1, 0)};
        LOffset scriptOffset {0};
        std::vector<Script> scripts;
        LOffset featureOffset {0};
        std::vector<Feature> features;
        static LOffset lookupsSize(uint32_t nlookups) {
            return sizeof(uint16_t) * (1 + nlookups);
        }
        LOffset lookupOffset {0};
        std::vector<Lookup> lookups;
    };

    struct LabelInfo {
        LabelInfo() = delete;
        explicit LabelInfo(int32_t li) : lookupInx(li) {}
        int32_t lookupInx {0};
        bool used {false};
    };

    struct RefLabelInfo {
        Label baseLabel {0};
        Label refLabel {0};
    };

 public:
    virtual LOffset extOffset() { return offset.extension; }
    virtual LOffset subOffset() { return offset.subtable; }
    virtual void incExtOffset(LOffset o) { offset.extension += o; }
    virtual void incSubOffset(LOffset o) { offset.subtable += o; }
    virtual void incFeatParamOffset(LOffset o) { offset.featParam += o; }
    virtual void checkOverflow(const char* offsetType, long offset,
                               const char* posType);
    virtual void writeValueRecord(uint32_t valFmt, ValueRecord i) { assert(false); }

 protected:
    OTL() = delete;
    explicit OTL(hotCtx g, uint16_t elt) : g(g), cac(std::make_shared<CoverageAndClass>(g)), extLkpType(elt) {}
    virtual ~OTL() {}
    virtual int fillOTL(bool force = false);
    virtual void writeOTL();
    virtual void AddSubtable(typename std::unique_ptr<Subtable> s);
    virtual void updateMaxContext(uint16_t m) { maxContext = MAX(m, maxContext); }
#if HOT_DEBUG
    void dumpSizes(LOffset subtableSize, LOffset extensionSectionSize);
#endif
    virtual const char *objName() = 0;
    virtual void createAnonLookups() = 0;

 private:
    static void valDump(int16_t val, int16_t excep, bool isRef);
    static void tagDump(Tag tag, char ch);

    virtual void lookupListWrite() { header.lookupListWrite(g, lookupSize); }
    virtual int32_t label2LookupIndex(Label baselabel);
    virtual void checkStandAloneRefs();

    virtual void setAnonLookupIndices();

    virtual void calcLookupListIndices();
    virtual void calcFeatureListIndices();

    virtual void prepScriptList();
    virtual Offset fillScriptList();
    virtual void fixFeatureParamOffsets(Offset size);

    virtual void prepLookupList();
    virtual Offset findFeatParamOffset(Tag featTag, Label featLabel);
    virtual Offset fillLookupList();

    virtual void prepFeatureList();
    virtual Offset fillFeatureList();

#if HOT_DEBUG
    virtual void dumpSubtables();
    virtual LOffset getHeaderSize();
#endif

 public:
    hotCtx g;

 protected:
    std::shared_ptr<CoverageAndClass> cac;

 private:
    uint16_t extLkpType {0};
    struct {
        LOffset featParam {0};
        LOffset subtable {0};
        LOffset extension {0};
        LOffset extensionSection {0};
    } offset;
    std::vector<std::unique_ptr<Subtable>> subtables;
    std::map<Label, LabelInfo> labelMap;
    std::vector<RefLabelInfo> refLabels;
    uint16_t maxContext {0};

    Header header;
    Offset lookupSize {0};
    int16_t nAnonSubtables {0};
    int16_t nStandAloneSubtables {0};
    int16_t nRefLookups {0};
    int16_t nFeatParams {0};
    LOffset params_size {0};
};

/* Script tags used by hotconv */
#define arab_ TAG('a', 'r', 'a', 'b') /* Arabic */
#define cyrl_ TAG('c', 'y', 'r', 'l') /* Cyrillic */
#define deva_ TAG('d', 'e', 'v', 'a') /* Devanagari */
#define grek_ TAG('g', 'r', 'e', 'k') /* Greek */
#define gujr_ TAG('g', 'u', 'j', 'r') /* Gujarati */
#define hebr_ TAG('h', 'e', 'b', 'r') /* Hebrew */
#define kana_ TAG('k', 'a', 'n', 'a') /* Kana (Hiragana & Katakana) */
#define latn_ TAG('l', 'a', 't', 'n') /* Latin */
#define punj_ TAG('p', 'u', 'n', 'j') /* Gurmukhi */
#define thai_ TAG('t', 'h', 'a', 'i') /* Thai */
#define DFLT_ TAG('D', 'F', 'L', 'T') /* Default */

/* Language tags used by hotconv */
#define dflt_ TAG(' ', ' ', ' ', ' ') /* Default (sorts first) */

/* Feature tags used by hotconv */
#define kern_ TAG('k', 'e', 'r', 'n') /* Kerning */
#define vkrn_ TAG('v', 'k', 'r', 'n') /* Vertical kerning */
#define vpal_ TAG('v', 'p', 'a', 'l') /* Vertical kerning */
#define vhal_ TAG('v', 'h', 'a', 'l') /* Vertical kerning */
#define valt_ TAG('v', 'a', 'l', 't') /* Vertical kerning */
#define vert_ TAG('v', 'e', 'r', 't') /* Vertical writing */
#define vrt2_ TAG('v', 'r', 't', '2') /* Vertical rotation */
#define size_ TAG('s', 'i', 'z', 'e') /* Vertical rotation */

#endif  // ADDFEATURES_HOTCONV_OTL_H_
