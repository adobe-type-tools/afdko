/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_OTL_H_
#define ADDFEATURES_HOTCONV_OTL_H_

#include <map>
#include <set>
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

#define aalt_ TAG('a', 'a', 'l', 't') /* All alternates */

class otlTbl {
    // Coverage tables
 public:
    struct RangeRecord {
        GID Start {GID_UNDEF};
        GID End {GID_UNDEF};
        uint16_t startCoverageIndex {0};
    };

    struct CoverageRecord {
        CoverageRecord() = delete;
        CoverageRecord(Offset o, std::set<GID> &gl);
        Offset cov1size() {
            return sizeof(uint16_t) * (2 + glyphs.size());
        }
        Offset cov2size() {
            return sizeof(uint16_t) * (2 + 3 * ranges.size());
        }
        Offset size() { return ranges.size() != 0 ? cov2size() : cov1size(); }
        void write(otlTbl &t);
        Offset offset;
        std::set<GID> glyphs;             // format 1
        std::vector<RangeRecord> ranges;  // format 2
    };

    struct Coverage {
        Offset fill(otlTbl &t);
        void write(otlTbl &t);
        LOffset size {0};
#if HOT_DEBUG
        int16_t reused {0};
#endif
        std::vector<CoverageRecord> records;
        std::set<GID> current;
    };

    // Classes

    struct ClassRangeRecord {
        GID Start {GID_UNDEF};
        GID End {GID_UNDEF};
        uint16_t classId {0xFFFF};
    };

    struct ClassRecord {
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
        void write(otlTbl &t);

        std::map<GID, uint16_t> map;
        Offset offset;
        GID startGlyph {GID_UNDEF};
        std::vector<uint16_t> values;
        std::vector<ClassRangeRecord> ranges;
    };

    struct Class {
        Offset fill(otlTbl &t);
        void write(otlTbl &t);
        LOffset size {0};
#if HOT_DEBUG
        int16_t reused {0};
#endif
        std::vector<ClassRecord> records;
        std::map<GID, uint16_t> current;
    };

    struct Subtable;

    struct LangSys {
        static LOffset size(uint32_t nfeatures) {
            return sizeof(uint16_t) * (3 + nfeatures);
        }
        static LOffset recordSize() {
            return sizeof(uint32_t) + sizeof(uint16_t);
        }
        Offset fill(Offset o, std::vector<Subtable>::iterator sl);
        void write(otlTbl &t);

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
        Lookup(Offset o, uint32_t ft) : offset(o), Type(ft >> 16),
                                        Flag(ft & 0xFFFF) {}
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
        void write(otlTbl &t);
        void lookupListWrite(otlTbl &g, Offset lookupSize);
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

    struct Subtable {
        Subtable() = delete;
        Subtable(Tag s, Tag l, Tag f, int32_t lt, int32_t lf, uint16_t msi,
                 uint16_t elt, LOffset o, Label lb, uint16_t fmt, bool ifp)
                : script(s), language(l), feature(f), lookup(lt << 16 | lf),
                  markSetIndex(msi), extensionLookupType(elt), offset(o),
                  label(lb), fmt(fmt), isFeatParam(ifp),
                  seenInFeature(feature != TAG_STAND_ALONE) {}
        static bool ltLookupList(const Subtable &a, const Subtable &b) {
            if (a.isRef() != b.isRef())
                return b.isRef();
            if (a.isParam() != b.isParam())
                return b.isParam();
            if (a.isAALT() != b.isAALT())  // aalt's come first
                return a.isAALT();
            return a.offset < b.offset;
        }
        static bool ltScriptList(const Subtable &a, const Subtable &b) {
            if (a.isStandAlone() != b.isStandAlone())
                return b.isStandAlone();
            if (a.isAnon() != b.isAnon())
                return b.isAnon();
            if (a.script != b.script)
                return a.script < b.script;
            if (a.language != b.language)
                return a.language < b.language;
            return a.feature < b.feature;
        }
        bool operator < (const Subtable &rhs) const {
            if (isAnon() != rhs.isAnon())
                return rhs.isAnon();
            if (isStandAlone() != rhs.isStandAlone())
                return rhs.isStandAlone();
            if (feature != rhs.feature)
                return feature < rhs.feature;
            if (index.feature != rhs.index.feature)
                return index.feature < rhs.index.feature;
            return index.lookup < rhs.index.lookup;
        }
        bool isStandAlone() const { return feature == TAG_STAND_ALONE; }
        bool isAnon() const { return script == TAG_UNDEF; }
        bool isRef() const { return IS_REF_LAB(label); }
        bool isParam() const { return isFeatParam; }
        bool isAALT() const { return feature == aalt_; }
#if HOT_DEBUG
        void dump(std::vector<Subtable>::iterator sb);
#endif
        Tag script {TAG_UNDEF};
        Tag language {TAG_UNDEF};
        Tag feature {TAG_UNDEF};
        uint32_t lookup {0};
        uint16_t markSetIndex {0};
        uint16_t extensionLookupType {0};
        Offset offset {0};
        Label label {0};
        uint16_t fmt {0};
        bool isFeatParam {false};
        bool seenInFeature {false};
        struct {
            int16_t feature {-1};
            int16_t lookup {-1};
        } index;
        struct {
            std::vector<Subtable>::iterator script;
            std::vector<Subtable>::iterator language;
            std::vector<Subtable>::iterator feature;
            std::vector<Subtable>::iterator lookup;
        } span;
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
    otlTbl() = delete;
    explicit otlTbl(hotCtx g) : g(g) {}
    void fill(LOffset params_size);
    void fillStub();
    void write() { header.write(*this); }
    void lookupListWrite() { header.lookupListWrite(*this, lookupSize); }
    void sortLabelList();
    int32_t label2LookupIndex(Label baselabel);
    void checkStandAloneRefs();
#if HOT_DEBUG
    void dumpSubtables();
    LOffset getHeaderSize();
    void dumpSizes(LOffset subtableSize, LOffset extensionSectionSize);
#endif
    void coverageBegin();
    void coverageAddGlyph(GID glyph, bool warn = false);
    Offset coverageEnd();
    void coverageWrite();

    LOffset getCoverageSize();

    void calcLookupListIndices();
    void calcFeatureListIndices();

    void prepScriptList();
    Offset fillScriptList();
    void fixFeatureParamOffsets(Offset size);

    void prepLookupList();
    Offset findFeatParamOffset(Tag featTag, Label featLabel);
    Offset fillLookupList();

    void prepFeatureList();
    Offset fillFeatureList();

    bool seenRef(Label baseLookup);

    void subtableAdd(Tag script, Tag language, Tag feature,
                     int32_t lkpType, int32_t lkpFlag,
                     uint16_t markSetIndex, uint16_t extensionLookupType,
                     LOffset offset, Label label, uint16_t fmt,
                     bool isFeatParam);

    void classBegin();
    void classAddMapping(GID glyph, uint32_t classId);
    Offset classEnd();
    void classWrite();

    LOffset getClassSize();

    // Temporary
    void o2(int16_t v) { hotOut2(g, v); }
    void o4(int32_t v) { hotOut4(g, v); }

    std::vector<Subtable> subtables;
    std::map<Label, LabelInfo> labelMap;
    std::vector<RefLabelInfo> refLabels;

    Header header;
    Offset lookupSize {0};
    Coverage coverage;
    Class classObj;
    int16_t nAnonSubtables {0};
    int16_t nStandAloneSubtables {0};
    int16_t nRefLookups {0};
    int16_t nFeatParams {0};
    LOffset params_size {0};
    hotCtx g;
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
