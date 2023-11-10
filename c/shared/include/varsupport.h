/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_VARSUPPORT_H_
#define SHARED_INCLUDE_VARSUPPORT_H_

#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "sfntread.h"
#include "supportfp.h"
#include "absfont.h"

/* Variable Font Tables Reader Library
   =======================================
   This library parses tables common tables used by variable OpenType fonts.
*/

#define VARSUPPORT_VERSION CTL_MAKE_VERSION(2, 0, 8)
#define F2DOT14_TO_FIXED(v) (((Fixed)(v)) << 2)
#define FIXED_TO_F2DOT14(v) ((var_F2dot14)(((Fixed)(v) + 0x00000002) >> 2))

#define F2DOT14_ZERO 0
#define F2DOT14_ONE (1 << 14)
#define F2DOT14_MINUS_ONE (-F2DOT14_ONE)

typedef int16_t var_F2dot14; /* 2.14 fixed point number */

/* glyph width and side-bearing */
struct var_glyphMetrics {
    float width {0};
    float sideBearing {0};
};

struct var_indexPair {
    uint16_t outerIndex {0};
    uint16_t innerIndex {0};
};

struct var_indexMap {
    uint32_t offset {0};
    std::vector<var_indexPair> map;
};

/* convert 2.14 fixed value to float */
inline float var_F2dot14ToFloat(var_F2dot14 v) { return (float)v / (1 << 14); }

/* variable font axis tables */
class var_axes {
 public:
    var_axes(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    // Retrieves the number of axes from the axes table data.
    uint16_t getAxisCount() { return axes.size(); }


    /* Retrieves info on an axis from the axes table data.

    index        - specifies an axis index.
    tag          - where the four-letter axis tag is returned.
                   May be NULL if tag is not needed.
    minValue     - where the minimum coordinate value for the axis is returned.
                   May be NULL if the minimum value is not needed.
    defaultValue - where the default coordinate value for the axis is returned.
                   May be NULL if the minimum value is not needed.
    maxValue     - where the maximum coordinate value for the axis is returned.
                   May be NULL if the maximum value is not needed.
    flags        - where the axis flags value is returned.
                   May be NULL if the flags value is not needed.
    */
    bool getAxis(uint16_t index, ctlTag *tag, Fixed *minValue,
                 Fixed *defaultValue, Fixed *maxValue, uint16_t *flags);

    int16_t getAxisIndex(ctlTag tag);
    /* normalizeCoords() normalizes a variable font value coordinates.

    sscb       - a pointer to shared stream callback functions.
    userCoords - user coordinates to be normalized.
    normCoords - where the resulting normalized coordinates are returned as 2.14 fixed point values.
    */
    bool normalizeCoords(ctlSharedStmCallbacks *sscb, Fixed *userCoords, Fixed *normCoords);
    bool normalizeCoord(uint16_t index, Fixed userCoord, Fixed &normCoord);

    // Returns the number of named instances from the axes table data.
    uint16_t getInstanceCount() { return instances.size(); }

    /* Searches for a named instance matching the given coordinates.
       Returns the instance index if >= 0, -1 if not found.

    axisCount    - the number of axes.
    subfamilyID  - where the name ID of the subfamily name of the instance
                   is returned.
    postscriptID - where the name ID of the PostScript name of the instance
                   is returned, or 0 if no PostScript name ID is provided.
    */
    int findInstance(float *userCoords, uint16_t axisCount, uint16_t &subfamilyID,
                     uint16_t &postscriptID);

 private:
    bool load_avar(sfrCtx sfr, ctlSharedStmCallbacks *sscb);
    bool load_fvar(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    // avar-related
    struct axisValueMap {
        axisValueMap(Fixed fromCoord, Fixed toCoord) : fromCoord(fromCoord),
                                                       toCoord(toCoord) {}
        Fixed fromCoord;
        Fixed toCoord;
    };

    struct segmentMap {
        std::vector<axisValueMap> valueMaps;
    };

    uint16_t avarAxisCount {0};
    std::vector<segmentMap> segmentMaps;

    // fvar-related
    struct variationAxis {
        ctlTag tag {0};
        Fixed minValue {0};
        Fixed defaultValue {0};
        Fixed maxValue {0};
        uint16_t flags {0};
        uint16_t nameID {0};
    };

    struct variationInstance {
        uint16_t subfamilyNameID {0};
        uint16_t flags {0};
        std::vector<float> coordinates;
        uint16_t postScriptNameID {0};
    };

    static Fixed defaultNormalizeAxis(const variationAxis &axis,
                                      Fixed userValue);
    static Fixed applySegmentMap(const segmentMap &seg, Fixed value);

    std::vector<variationAxis> axes;
    std::vector<variationInstance> instances;
};

class var_location {
 public:
    struct cmpSP {
        bool operator()(const std::shared_ptr<var_location> &a,
                        const std::shared_ptr<var_location> &b) const {
            if (a == nullptr && b == nullptr)
                return false;
            if (a == nullptr)
                return true;
            if (b == nullptr)
                return false;
            return *a < *b;
        }
    };
    var_location() = delete;
    var_location(std::vector<var_F2dot14> &l) : alocs(std::move(l)) {}
    var_location(const std::vector<var_F2dot14> &l) : alocs(l) {}
    bool operator==(const var_location &o) const { return alocs == o.alocs; }
    bool operator<(const var_location &o) const { return alocs < o.alocs; }
    auto at(int i) const { return alocs.at(i); }
    auto size() const { return alocs.size(); }
    void toerr() const {
        int i {0};
        for (auto f2d: alocs) {
            if (i++ > 0)
                std::cerr << ", ";
            std::cerr << var_F2dot14ToFloat(f2d);
        }
    }
    std::vector<var_F2dot14> alocs;
};


class var_location_map {
 public:
    uint32_t getIndex(std::shared_ptr<var_location> &l) {
        const auto [it, success] = locmap.emplace(l, static_cast<uint32_t>(locvec.size()));
        if (success)
            locvec.push_back(l);
        return it->second;
    }
    std::shared_ptr<var_location> getLocation(uint32_t i) {
        if (i >= locvec.size())
            return nullptr;
        return locvec[i];
    }
    void toerr() {
        int i {0};
        for (auto &loc: locvec) {
            std::cerr << i++ << " ";
            loc->toerr();
            std::cerr << std::endl;
        }
    }
 private:
    std::map<std::shared_ptr<var_location>, uint32_t, var_location::cmpSP> locmap;
    std::vector<std::shared_ptr<var_location>> locvec;
};

class itemVariationStore {
 public:
    /* Parses the Item Variation Store (IVS) sub-table.
       returns a pointer to IVS data, or NULL if not found.

    sscb        - a pointer to shared stream callback functions.
    tableOffset - offset to the beginning of the container table from the
                  beginning of the stream.
    tableLength - length of the container table.
    ivsOffset   - offset to the IVS data from the beginning of the
                  container table.
    */
    itemVariationStore(ctlSharedStmCallbacks *sscb, uint32_t tableOffset,
                       uint32_t tableLength, uint32_t ivsOffset);

    // Returns the number of regions in the region list in the IVS data.
    uint16_t getRegionCount() { return regions.size() / axisCount; }

    uint32_t getRegionListSize() {
        return 4 + regions.size() * 6;
    }

    uint32_t getDataItemSize(uint16_t i) {
        if (i >= subtables.size())  // XXX would be an error, assert instead?
            return 6;
        auto &ivd = subtables[i];
        return 6 + ivd.regionIndices.size() * 2 + ivd.itemCount * 2;
    }

    /* Returns the number of sub-regions applicable to an IVS sub-table.

    vsIndex - IVS ItemVariationData sub-table index.
    */
    uint16_t getDRegionCountForIndex(uint16_t vsIndex) {
        if (vsIndex >= subtables.size())
            return 0;

        return subtables[vsIndex].regionIndices.size();
    }

    /* Returns indices of sub-regions applicable to an an IVS sub-table.

    vsIndex       - IVS sub-table index.
    regionIndices - where region indices are returned.
    regionCount   - the length of regionIndices array.
    */

    int32_t getRegionIndices(uint16_t vsIndex, uint16_t *regionIndices,
                             int32_t regionCount);

    /* Calculates scalars for all regions given a normalized design vector for
       an instance.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number axes. this is taken from the fvar table. If a
                 naked CFF2 is being dumped it is updated from the CFF2
                 VarationStore.
    instCoords - a pointer to normalized design vector of a font instance.
    scalars    - where scalars are returned as float values.
    */
    void calcRegionScalars(ctlSharedStmCallbacks *sscb, uint16_t &axisCount,
                           Fixed *instCoords, float *scalars);
// private:  (Can't make private until cffwrite_varstore restructuring
    struct variationRegion {
        Fixed startCoord;
        Fixed peakCoord;
        Fixed endCoord;
    };

    struct itemVariationDataSubtable {
        uint16_t itemCount;
        std::vector<int16_t> regionIndices;
        std::vector<int16_t> deltaValues;
    };
    float applyDeltasForIndexPair(ctlSharedStmCallbacks *sscb,
                                  const var_indexPair &pair, float *scalars,
                                  int32_t regionListCount);
    float applyDeltasForGid(ctlSharedStmCallbacks *sscb, var_indexMap &map,
                            uint16_t gid, float *scalars,
                            int32_t regionListCount);
    void reset() { axisCount = 0; regions.clear(); subtables.clear(); }

    // typedef std::vector<uint16_t> indexArray;

    uint16_t axisCount;
    std::vector<variationRegion> regions;
    std::vector<itemVariationDataSubtable> subtables;
};

/* horizontal metrics tables */

class var_hmtx {
 public:
    var_hmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    /* lookup horizontal metrics for a glyph optionally blended using font
       instance coordinates.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number of axes.
    instCoords - a pointer to normalized font instance coordinates. May be NULL
                 if no blending required.
    gid        - the glyph ID to be looked up.
    metrics    - where the horizontal glyph metrics are returned.
    */
    bool lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount,
                Fixed *instCoords, uint16_t gid, var_glyphMetrics &metrics);

 private:
    struct var_hhea {
        Fixed version {0};
        int16_t ascender {0};
        int16_t descender {0};
        int16_t lineGap {0};
        uint16_t advanceWidthMax {0};
        int16_t minLeftSideBearing {0};
        int16_t minRightSideBearing {0};
        int16_t xMaxExtent {0};
        int16_t caretSlopeRise {0};
        int16_t caretSlopeRun {0};
        int16_t caretOffset {0};
        int16_t reserved[4] {0, 0, 0, 0};
        int16_t metricDataFormat {0};
        uint16_t numberOfHMetrics {0};
    } header;
    std::vector<var_glyphMetrics> defaultMetrics;

    /* HHVAR */
    std::unique_ptr<itemVariationStore> ivs;
    var_indexMap widthMap;
    var_indexMap lsbMap;
    var_indexMap rsbMap;
};

/* vertical metrics tables */

class var_vmtx {
 public:
    var_vmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    /* lookup vertical metrics for a glyph optionally blended usingx
       font instance coordinates.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number of axes.
    instCoords - a pointer to normalized font instance coordinates. May be NULL
                 if no blending required.
    gid        - the glyph ID to be looked up.
    metrics    - where the vertical glyph metrics are returned.
    */
    bool lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount,
                Fixed *instCoords, uint16_t gid, var_glyphMetrics &metrics);

 private:
    struct var_vhea_ {
        Fixed version {0};
        int16_t vertTypoAscender {0};
        int16_t vertTypoDescender {0};
        int16_t vertTypoLineGap {0};
        uint16_t advanceHeightMax {0};
        int16_t minTop {0};
        int16_t minBottom {0};
        int16_t yMaxExtent {0};
        int16_t caretSlopeRise {0};
        int16_t caretSlopeRun {0};
        int16_t caretOffset {0};
        int16_t reserved[4] {0, 0, 0, 0};
        int16_t metricDataFormat {0};
        uint16_t numOfLongVertMetrics {0};
    } header;
    std::vector<var_glyphMetrics> defaultMetrics;
    std::vector<int16_t> vertOriginY;

    std::unique_ptr<itemVariationStore> ivs;
    var_indexMap widthMap;
    var_indexMap tsbMap;
    var_indexMap bsbMap;
    var_indexMap vorgMap;
};

/* Predefined MVAR tags */
#define MVAR_hasc_tag CTL_TAG('h', 'a', 's', 'c') /* ascender                    OS/2.sTypoAscender */
#define MVAR_hdsc_tag CTL_TAG('h', 'd', 's', 'c') /* horizontal descender        OS/2.sTypoDescender */
#define MVAR_hlgp_tag CTL_TAG('h', 'l', 'g', 'p') /* horizontal line gap         OS/2.sTypoLineGap */
#define MVAR_hcla_tag CTL_TAG('h', 'c', 'l', 'a') /* horizontal clipping ascent  OS/2.usWinAscent */
#define MVAR_hcld_tag CTL_TAG('h', 'c', 'l', 'd') /* horizontal clipping descent OS/2.usWinDescent */
#define MVAR_vasc_tag CTL_TAG('v', 'a', 's', 'c') /* vertical ascender           vhea.ascent */
#define MVAR_vdsc_tag CTL_TAG('v', 'd', 's', 'c') /* vertical descender          vhea.descent */
#define MVAR_vlgp_tag CTL_TAG('v', 'l', 'g', 'p') /* vertical line gap           vhea.lineGap */
#define MVAR_hcrs_tag CTL_TAG('h', 'c', 'r', 's') /* horizontal caret rise       hhea.caretSlopeRise */
#define MVAR_hcrn_tag CTL_TAG('h', 'c', 'r', 'n') /* horizontal caret run        hhea.caretSlopeRun */
#define MVAR_hcof_tag CTL_TAG('h', 'c', 'o', 'f') /* horizontal caret offset     hhea.caretOffset */
#define MVAR_vcrs_tag CTL_TAG('v', 'c', 'r', 's') /* vertical caret rise         vhea.caretSlopeRise */
#define MVAR_vcrn_tag CTL_TAG('v', 'c', 'r', 'n') /* vertical caret run          vhea.caretSlopeRun */
#define MVAR_vcof_tag CTL_TAG('v', 'c', 'o', 'f') /* vertical caret offset       vhea.caretOffset */
#define MVAR_xhgt_tag CTL_TAG('x', 'h', 'g', 't') /* x height                    OS/2.sxHeight */
#define MVAR_cpht_tag CTL_TAG('c', 'p', 'h', 't') /* cap height                  OS/2.sCapHeight */
#define MVAR_sbxs_tag CTL_TAG('s', 'b', 'x', 's') /* subscript em x size         OS/2.ySubscriptXSize */
#define MVAR_sbys_tag CTL_TAG('s', 'b', 'y', 's') /* subscript em y size         OS/2.ySubscriptYSize */
#define MVAR_sbxo_tag CTL_TAG('s', 'b', 'x', 'o') /* subscript em x offset       OS/2.ySubscriptXOffset */
#define MVAR_sbyo_tag CTL_TAG('s', 'b', 'y', 'o') /* subscript em y offset       OS/2.ySubscriptYOffset */
#define MVAR_spxs_tag CTL_TAG('s', 'p', 'x', 's') /* superscript em x size       OS/2.ySuperscriptXSize */
#define MVAR_spys_tag CTL_TAG('s', 'p', 'y', 's') /* superscript em y size       OS/2.ySuperscriptYSize */
#define MVAR_spxo_tag CTL_TAG('s', 'p', 'x', 'o') /* superscript em x offset     OS/2.ySuperscriptXOffset */
#define MVAR_spyo_tag CTL_TAG('s', 'p', 'y', 'o') /* superscript em y offset     OS/2.ySuperscriptYOffset */
#define MVAR_strs_tag CTL_TAG('s', 't', 'r', 's') /* strikeout size              OS/2.yStrikeoutSize */
#define MVAR_stro_tag CTL_TAG('s', 't', 'r', 'o') /* strikeout offset            OS/2.yStrikeoutPosition */
#define MVAR_unds_tag CTL_TAG('u', 'n', 'd', 's') /* underline size              post.underlineThickness */
#define MVAR_undo_tag CTL_TAG('u', 'n', 'd', 'o') /* underline offset            post.underlinePosition */

#define MVAR_gsp0_tag CTL_TAG('g', 's', 'p', '0') /* gaspRange[0] gasp.gaspRange[0].rangeMaxPPEM */
#define MVAR_gsp1_tag CTL_TAG('g', 's', 'p', '1') /* gaspRange[1] gasp.gaspRange[1].rangeMaxPPEM */
#define MVAR_gsp2_tag CTL_TAG('g', 's', 'p', '2') /* gaspRange[2] gasp.gaspRange[2].rangeMaxPPEM */
#define MVAR_gsp3_tag CTL_TAG('g', 's', 'p', '3') /* gaspRange[3] gasp.gaspRange[3].rangeMaxPPEM */
#define MVAR_gsp4_tag CTL_TAG('g', 's', 'p', '4') /* gaspRange[4] gasp.gaspRange[4].rangeMaxPPEM */
#define MVAR_gsp5_tag CTL_TAG('g', 's', 'p', '5') /* gaspRange[5] gasp.gaspRange[5].rangeMaxPPEM */
#define MVAR_gsp6_tag CTL_TAG('g', 's', 'p', '6') /* gaspRange[6] gasp.gaspRange[6].rangeMaxPPEM */
#define MVAR_gsp7_tag CTL_TAG('g', 's', 'p', '7') /* gaspRange[7] gasp.gaspRange[7].rangeMaxPPEM */
#define MVAR_gsp8_tag CTL_TAG('g', 's', 'p', '8') /* gaspRange[8] gasp.gaspRange[8].rangeMaxPPEM */
#define MVAR_gsp9_tag CTL_TAG('g', 's', 'p', '9') /* gaspRange[9] gasp.gaspRange[9].rangeMaxPPEM */

/* MVAR table */

class var_MVAR {
 public:
    var_MVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    /* lookup font-wide metric values for a tag blended using font instance
       coordinates.  Returns 0 if successful, otherwise non-zero.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number of axes.
    instCoords - a pointer to normalized font instance coordinates. May be NULL if no blending required.
    tag        - the tag of the metric value to be looked up.
    value      - where the blended metric value is returned.
     */
    bool lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount, Fixed *instCoords, ctlTag tag, float &value);
 private:
    struct MVARValueRecord {
        ctlTag valueTag {0};
        var_indexPair pair;
    };

    std::unique_ptr<itemVariationStore> ivs;
    uint16_t axisCount {0};
    std::vector<MVARValueRecord> values;
};

/* returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h).
 */

void varsupportGetVersion(ctlVersionCallbacks *cb);

#endif  // SHARED_INCLUDE_VARSUPPORT_H_
