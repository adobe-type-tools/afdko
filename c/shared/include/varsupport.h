/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_VARSUPPORT_H_
#define SHARED_INCLUDE_VARSUPPORT_H_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <utility>
#include <variant>

#include "sfntread.h"
#include "cffread_abs.h"
#include "supportfp.h"
#include "absfont.h"

/* Variable Font Tables Reader Library
   =======================================
   This library parses tables common tables used by variable OpenType fonts.
*/

/* In some otf tables an Offset is a byte offset of data relative to some
   format component, normally the beginning of the record it belongs to. The
   OFFSET macros allow a simple declaration of both the byte offset field and a
   structure containing the data for that offset. (In header definitions the
   symbol |-> is used as a shorthand for "relative to".) */
typedef uint32_t LOffset;

#define VARSUPPORT_VERSION CTL_MAKE_VERSION(2, 0, 8)
#define F2DOT14_TO_FIXED(v) (((Fixed)(v)) << 2)
#define FIXED_TO_F2DOT14(v) ((var_F2dot14)(((Fixed)(v) + 0x00000002) >> 2))

#define F2DOT14_ZERO 0
#define F2DOT14_ONE (1 << 14)
#define F2DOT14_MINUS_ONE (-F2DOT14_ONE)

// Stores index into h->values, which is read at write time. If -1, then write 0;
typedef int32_t ValueIndex;
#define VAL_INDEX_UNDEF (-1)

typedef int16_t var_F2dot14; /* 2.14 fixed point number */

/* glyph width and side-bearing */

class VarWriter {
 public:
    virtual void w1(char o) = 0;
    virtual void w2(int16_t o) = 0;
    virtual void w3(int32_t o) = 0;
    virtual void w4(int32_t o) = 0;
    virtual void w(size_t count, char *data) = 0;
};

struct var_indexPair {
    var_indexPair() {}
    var_indexPair(uint16_t outer, uint16_t inner) : outerIndex(outer), innerIndex(inner) {}
    uint16_t outerIndex {0};
    uint16_t innerIndex {0};
};

class VarValueRecord;
class VarLocationMap;

struct var_indexMap {
    LOffset size(uint8_t entryBytes) {
        if (map.size() == 0)
            return 0;
        return sizeof(uint8_t) * (2 + entryBytes * map.size()) + sizeof(uint32_t);
    }
    uint8_t entryFormat(uint8_t entryBytes, uint8_t deltaBits);
    void write(VarWriter &vw, uint8_t entryBytes, uint8_t deltaBits);
    uint32_t offset {0};
    void addValue(VarValueRecord &v, VarLocationMap &vlm, uint16_t gid,
                  std::unique_ptr<itemVariationStore> &ivs,
                  std::shared_ptr<slogger> logger);
    void clear() {
        offset = 0;
        map.clear();
    }
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
    bool normalizeCoord(uint16_t index, Fixed userCoord, Fixed &normCoord);
    bool normalizeCoords(ctlSharedStmCallbacks *sscb, Fixed *userCoords, std::vector<Fixed> &normCoords);

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

template <typename T>
struct cmpSP {
    bool operator()(const std::shared_ptr<T> &a,
                    const std::shared_ptr<T> &b) const {
        if (a == nullptr && b == nullptr)
            return false;
        if (a == nullptr)
            return true;
        if (b == nullptr)
            return false;
        return *a < *b;
    }
};

class VarLocation {
 public:
    VarLocation() = delete;
    explicit VarLocation(std::vector<var_F2dot14> &l) : alocs(std::move(l)) {}
    explicit VarLocation(const std::vector<var_F2dot14> &l) : alocs(l) {}
    bool operator==(const VarLocation &o) const { return alocs == o.alocs; }
    bool operator<(const VarLocation &o) const { return alocs < o.alocs; }
    auto at(int i) const { return alocs.at(i); }
    auto size() const { return alocs.size(); }
    void toerr() const {
        int i {0};
        for (auto f2d : alocs) {
            if (i++ > 0)
                std::cerr << ", ";
            std::cerr << var_F2dot14ToFloat(f2d);
        }
    }
    std::vector<var_F2dot14> alocs;
};

class VarLocationMap {
 public:
    VarLocationMap() = delete;
    explicit VarLocationMap(uint16_t axis_count) : axis_count(axis_count) {
        // Store default location first, always has index 0
        std::vector<var_F2dot14> defaxis(axis_count, 0);
        auto defloc = std::make_shared<VarLocation>(std::move(defaxis));
        getIndex(defloc);
    }
    uint32_t getIndex(std::shared_ptr<VarLocation> &l) {
        const auto [it, success] = locmap.emplace(l, static_cast<uint32_t>(locvec.size()));
        if (success)
            locvec.push_back(l);
        return it->second;
    }
    uint16_t getAxisCount() const { return axis_count; }
    const std::shared_ptr<VarLocation> getLocation(uint32_t i) {
        if (i >= locvec.size())
            return nullptr;
        return locvec[i];
    }
    void toerr() {
        if (axis_count == 0)
            return;
        int i {0};
        for (auto &loc : locvec) {
            std::cerr << i++ << ":  ";
            loc->toerr();
            std::cerr << std::endl;
        }
    }

 private:
    uint16_t axis_count;
    std::map<std::shared_ptr<VarLocation>, uint32_t, cmpSP<VarLocation>> locmap;
    std::vector<std::shared_ptr<VarLocation>> locvec;
};

class VarValueRecord {
 public:
    VarValueRecord() {}
    explicit VarValueRecord(int32_t value) : defaultValue(value), seenDefault(true) {}
    VarValueRecord(const VarValueRecord &vvr) : defaultValue(vvr.defaultValue),
                                                seenDefault(vvr.seenDefault),
                                                locationValues(vvr.locationValues) {}
    VarValueRecord(VarValueRecord &&vvr) : defaultValue(vvr.defaultValue),
                                           seenDefault(vvr.seenDefault),
                                           locationValues(std::move(vvr.locationValues)) {}
    VarValueRecord &operator=(const VarValueRecord &o) {
        defaultValue = o.defaultValue;
        seenDefault = o.seenDefault;
        locationValues = o.locationValues;
        return *this;
    }
    VarValueRecord &operator=(VarValueRecord &&o) {
        defaultValue = o.defaultValue;
        seenDefault = o.seenDefault;
        locationValues.swap(o.locationValues);
        return *this;
    }
    void addValue(int32_t value, bool overwriteOK = false) {
        if (!overwriteOK)
            assert(!seenDefault);
        seenDefault = true;
        defaultValue = value;
    }
#if HOT_DEBUG
    void toerr() {
        if (isVariable()) {
            std::cerr << '(' << defaultValue;
            for (auto i : locationValues)
                std::cerr << ' ' << i.first << ':' << i.second;
            std::cerr << ')';
        } else {
            std::cerr << defaultValue;
        }
    }
#endif
    void swap(VarValueRecord &o) {
        std::swap(defaultValue, o.defaultValue);
        std::swap(seenDefault, o.seenDefault);
        locationValues.swap(o.locationValues);
    }
    bool addLocationValue(uint32_t locIndex, int32_t value, std::shared_ptr<slogger> logger) {
        if (locIndex == 0) {
            if (seenDefault) {
                logger->msg(sERROR, "Duplicate values for default location");
                return false;
            } else {
                defaultValue = value;
                seenDefault = true;
                return true;
            }
        } else {
            const auto [it, success] = locationValues.emplace(locIndex, value);
            if (!success)
                logger->msg(sERROR, "Duplicate values for location");
            return success;
        }
    }
    void verifyDefault(std::shared_ptr<slogger> logger) const {
        if (!seenDefault)
            logger->msg(sERROR, "No default entry for variable value");
    }
    int32_t getDefault() const { return defaultValue; }
    bool isVariable() const { return locationValues.size() > 0; }
    bool nonZero() const { return isVariable() || defaultValue != 0; }
    bool isInitialized() const { return isVariable() || seenDefault; }
    std::set<uint32_t> getLocations() const {
        std::set<uint32_t> r;
        for (auto i : locationValues)
            r.insert(i.first);
        return r;
    }
    bool hasLocation(uint32_t l) const {
        if (!isInitialized())
            return false;
        if (l == 0)
            return seenDefault;
        return locationValues.find(l) != locationValues.end();
    }
    VarValueRecord addSame(const VarValueRecord &o) {
        assert(isInitialized() && o.isInitialized());
        assert(locationValues.size() == o.locationValues.size());
        auto i = locationValues.begin();
        auto oi = o.locationValues.begin();
        VarValueRecord r;
        r.addValue(defaultValue + o.defaultValue);
        for ( ; i != locationValues.end() && oi != o.locationValues.end(); i++, oi++) {
            assert(i->first == oi->first);
            r.locationValues.emplace(i->first, i->second + oi->second);
        }
        return r;
    }
    VarValueRecord subSame(const VarValueRecord &o) {
        assert(isInitialized() && o.isInitialized());
        assert(locationValues.size() == o.locationValues.size());
        auto i = locationValues.begin();
        auto oi = o.locationValues.begin();
        VarValueRecord r;
        r.addValue(defaultValue - o.defaultValue);
        for ( ; i != locationValues.end() && oi != o.locationValues.end(); i++, oi++) {
            assert(i->first == oi->first);
            r.locationValues.emplace(i->first, i->second - oi->second);
        }
        return r;
    }
    bool operator==(const VarValueRecord &rhs) const {
        return defaultValue == rhs.defaultValue &&
               locationValues == rhs.locationValues;
    }
    bool operator<(const VarValueRecord &rhs) const {
        if (defaultValue != rhs.defaultValue)
            return defaultValue < rhs.defaultValue;
        return locationValues < rhs.locationValues;
    }
    int16_t getLocationValue(uint32_t locationIndex) const {
        auto f = locationValues.find(locationIndex);
        if (f == locationValues.end())
            return defaultValue;
        return f->second;
    }
    void normalize() {
        bool locationDiffers {false};
        for (auto i : locationValues)
            if (i.second != defaultValue)
                locationDiffers = true;
        if (!locationDiffers)
            locationValues.clear();
    }
/*    void negate() {
        assert(isInitialized());
        defaultValue = -defaultValue();
        for (auto &[k, v] : locationValues)
            v = -v;
    }*/

 private:
     int16_t defaultValue {0};
     bool seenDefault {false};
     std::map<uint32_t, int32_t> locationValues;
};

typedef std::vector<VarValueRecord> ValueVector;

class VarModel;

class itemVariationStore {
 public:
    friend class VarModel;
    typedef std::tuple<var_F2dot14, var_F2dot14, var_F2dot14> AxisRegion;
    typedef std::vector<AxisRegion> VariationRegion;

    class ValueTracker {
     public:
        ValueTracker() = delete;
        explicit ValueTracker(int16_t v, uint16_t outer, uint16_t inner) :
            defaultValue(v), pair(outer, inner) {}
        explicit ValueTracker(int32_t v, uint16_t outer, uint16_t inner) :
            defaultValue(v), pair(outer, inner) {}
        int32_t getDefault() const { return defaultValue; }
        LOffset getDevOffset() const { return devOffset; }
        void setDevOffset(LOffset o) { devOffset = o; }
        bool isVariable() const { return pair.outerIndex != 0xFFFF; }
        uint16_t getOuterIndex() const { return pair.outerIndex; }
        uint16_t getInnerIndex() const { return pair.innerIndex; }
        void writeVariationIndex(VarWriter &vw) const {
            vw.w2(pair.outerIndex);
            vw.w2(pair.innerIndex);
            vw.w2(0x8000);
        }
     private:
        int32_t defaultValue {0};
        LOffset devOffset {0xFFFFFFFF};
        var_indexPair pair {0xFFFF, 0xFFFF};
    };

    /* Parses the Item Variation Store (IVS) sub-table.
       returns a pointer to IVS data, or NULL if not found.

    sscb        - a pointer to shared stream callback functions.
    tableOffset - offset to the beginning of the container table from the
                  beginning of the stream.
    tableLength - length of the container table.
    ivsOffset   - offset to the IVS data from the beginning of the
                  container table.
    */
    itemVariationStore() {}
    itemVariationStore(ctlSharedStmCallbacks *sscb, uint32_t tableOffset,
                       uint32_t tableLength, uint32_t ivsOffset);

    void setAxisCount(uint16_t ac) { axisCount = ac; }
    uint16_t getAxisCount() { return axisCount; }
    // Returns the number of regions in the region list in the IVS data.
    uint16_t getRegionCount() { return regions.size(); }
    bool isEmpty() { return models.size() == 0; }
    uint16_t numSubtables() { return subtables.size(); }
    uint16_t maxDeltaSetCount() {
        uint16_t mc {0};
        for (auto &s : subtables)
            if (mc < s.deltaValues.size())
                mc = s.deltaValues.size();
        return mc;
    }
    // ideally called before write() and after any new values
    void preWriteOptimize(bool reorder = true) {
        for (auto &s : subtables)
            s.preWriteOptimize(reorder);
    }

#if HOT_DEBUG
    void toerr() {
        int i = 0;
        for (auto &s : subtables) {
            std::cerr << std::endl << "Subtable " << i++ << ":" << std::endl;
            s.toerr(regions);
        }
        i = -1;
        for (auto &v : values) {
            i++;
            if (v.getOuterIndex() == 0xFFFF)
                continue;
            std::cerr << std::endl << "Value " << i++ << " (subtable " << v.getOuterIndex() << "):  " << v.getDefault();
            auto &dv = subtables[v.getOuterIndex()].deltaValues[v.getInnerIndex()];
            for (auto d : dv)
                std::cerr << " " << d;
            std::cerr << std::endl;
        }
    }
#endif  // HOT_DEBUG

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

    bool getRegionIndices(uint16_t vsIndex,
                          std::vector<uint16_t> &regionIndices);

    bool getLocationIndices(uint16_t vsIndex, VarLocationMap &vlm,
                            std::set<uint32_t> &locations);
    /* Calculates scalars for all regions given a normalized design vector for
       an instance.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number axes. this is taken from the fvar table. If a
                 naked CFF2 is being dumped it is updated from the CFF2
                 VarationStore.
    instCoords - a pointer to normalized design vector of a font instance.
    scalars    - where scalars are returned as float values.
    */
    Fixed calcRegionScalar(uint16_t refRegionIndex, const std::vector<Fixed> &alocs);
    Fixed calcRegionScalar(uint16_t refRegionIndex, uint16_t locRegionIndex);

    void calcRegionScalars(ctlSharedStmCallbacks *sscb,
                           const std::vector<Fixed> &instCoords,
                           std::vector<Fixed> &scalars);

    void calcRegionScalars(std::shared_ptr<slogger> &logger,
                           const std::vector<var_F2dot14> &alocs,
                           std::vector<Fixed> &scalars);

    Fixed applyDeltasForIndexPair(ctlSharedStmCallbacks *sscb,
                                  const var_indexPair &pair,
                                  const std::vector<Fixed> &scalars);

    // XXX recombine later
    Fixed applyDeltasForIndexPair(const var_indexPair &pair,
                                  const std::vector<Fixed> &scalars,
                                  std::shared_ptr<slogger> logger);
    Fixed applyDeltasForGid(ctlSharedStmCallbacks *sscb, var_indexMap &map,
                            uint16_t gid, const std::vector<Fixed> &scalars);

    var_indexPair addValue(VarLocationMap &vlm, const VarValueRecord &vvr,
                           std::shared_ptr<slogger> logger);
    uint32_t addTrackerValue(VarLocationMap &vlm, const VarValueRecord &vvr,
                             std::shared_ptr<slogger> logger);

    void setDevOffset(ValueIndex vi, LOffset o) {
        assert(vi < (ValueIndex)values.size());
        values[vi].setDevOffset(o);
    }

    VariationRegion sampleVariationRegion();

    // The pair values 0xFFFF,0xFFFF indicate that a value doesn't
    // vary from its default. Unfortunately those values are the
    // worst case for packing into a var_indexMap. So instead when
    // needed we allocate a model slot with all zeros when needed.
    var_indexPair getStaticPair();

    const std::vector<ValueTracker> &getValues() { return values; }

    uint16_t newSubtable(std::vector<VariationRegion> reg);

    uint32_t getSize() {
        uint32_t r = 8 + subtables.size() * 4;
        r += getRegionListSize();
        for (auto &sub : subtables)
            r += sub.getSize();
        return r;
    }

    void write(VarWriter &vw);

    void reset(bool clearAxisCount = false) {
        if (clearAxisCount)
            axisCount = 0;
        regions.clear();
        regionMap.clear();
        subtables.clear();
        values.clear();
        models.clear();
        locationSetMap.clear();
    }

 private:
    uint32_t getRegionListSize() {
        return 4 + regions.size() * axisCount * 6;
    }

    struct itemVariationDataSubtable {
        itemVariationDataSubtable() {}
        itemVariationDataSubtable(itemVariationDataSubtable &&s) : regionIndices(std::move(s.regionIndices)), deltaValues(std::move(s.deltaValues)) {}
        void toerr(std::vector<VariationRegion> &regions) {
            std::cerr << "Regions:" << std::endl;
            for (auto ri : regionIndices) {
                std::cerr << ri << ":  ";
                auto &r = regions[ri];
                bool first = true;
                for (auto &a : r) {
                    if (first)
                        first = false;
                    else
                        std::cerr << ',';
                    std::cerr << '{' << var_F2dot14ToFloat(std::get<0>(a))
                              << ',' << var_F2dot14ToFloat(std::get<1>(a))
                              << ',' << var_F2dot14ToFloat(std::get<2>(a)) << '}';
                }
                std::cerr << std::endl;
            }
        }
        void preWriteOptimize(bool reorder);
        uint32_t getSize() {
            uint16_t s = regionIndices.size();
            return 6 + s * 2 + deltaValues.size() * ((s - numBytes) * 2 + numBytes);
        }
        void write(VarWriter &vw);
        uint32_t deltaTrackKey(std::vector<int16_t> &deltas) {
            auto t = deltas.size();
            uint16_t zeroth = t > 0 ? (uint16_t)deltas[0] : 0;
            uint16_t first = t > 1 ? (uint16_t)deltas[1] : 0;
            return (zeroth << 16) + first;
        }
        uint16_t numBytes {0};
        std::vector<uint16_t> regionIndices;
        std::vector<std::vector<int16_t>> deltaValues;
        std::unordered_multimap<uint32_t, uint16_t> deltaTracker;
    };

    void writeRegionList(VarWriter &vw);

    uint16_t axisCount;
    std::vector<VariationRegion> regions;
    std::map<VariationRegion, uint32_t> regionMap;
    std::vector<itemVariationDataSubtable> subtables;
    std::vector<ValueTracker> values;
    std::vector<std::unique_ptr<VarModel>> models;
    std::map<std::set<uint32_t>, uint32_t> locationSetMap;
    var_indexPair staticPair {0xFFFF, 0xFFFF};
};

typedef std::vector<itemVariationStore::ValueTracker> VarTrackVec;

class VarModel {
 public:
    VarModel() = delete;
    VarModel(itemVariationStore &ivs, VarLocationMap &vlm, std::set<uint32_t> locationSet);
    uint16_t addValue(const VarValueRecord &vvr, std::shared_ptr<slogger> logger);
    explicit VarModel(VarModel &&vm) : subtableIndex(vm.subtableIndex),
                                       sortedLocations(std::move(vm.sortedLocations)),
                                       deltaWeights(std::move(vm.deltaWeights)),
                                       ivs(vm.ivs) {}
    static std::vector<std::set<var_F2dot14>> getAxisPoints(VarLocationMap &vlm, std::vector<uint32_t> locationList);
    struct cmpLocation {
        cmpLocation() = delete;
        explicit cmpLocation(VarLocationMap &vlm, std::vector<uint32_t> locationList) : vlm(vlm) {
            axisPoints = VarModel::getAxisPoints(vlm, locationList);
        }
        bool operator()(uint32_t &a, uint32_t &b);
        std::vector<std::set<var_F2dot14>> axisPoints;
        VarLocationMap &vlm;
    };
    void sortLocations(VarLocationMap &vlm, std::set<uint32_t> locationSet) {
        sortedLocations.reserve(locationSet.size());
        for (auto l : locationSet)
            sortedLocations.push_back(l);
        cmpLocation cl(vlm, sortedLocations);
        std::sort(sortedLocations.begin(), sortedLocations.end(), cl);
    }
    std::vector<itemVariationStore::VariationRegion> locationsToInitialRegions(VarLocationMap &vlm,
                                                                               std::vector<uint32_t> locationList);
    void narrowRegions(std::vector<itemVariationStore::VariationRegion> &reg);
    void calcDeltaWeights(VarLocationMap &vlm);
    uint16_t subtableIndex {0};
    std::vector<uint32_t> sortedLocations;
    std::vector<std::vector<std::pair<uint16_t, Fixed>>> deltaWeights;
    itemVariationStore &ivs;
};

/* horizontal metrics tables */

class var_hmtx {
 public:
    var_hmtx() {}
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
    bool lookup(ctlSharedStmCallbacks *sscb, const std::vector<Fixed> &instCoords,
                uint16_t gid, Fixed &width, Fixed &lsb);

    bool Fill();
    void write_hhea(VarWriter &vw);
    void write(VarWriter &vw);
    void write_HVAR(VarWriter &vw);

    struct var_hhea {
        Fixed version {0x00010000};
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
        int16_t metricDataFormat {0};
        uint16_t numberOfHMetrics {0};
    } header;
    std::vector<uint16_t> advanceWidth;
    std::vector<int16_t> lsb;

    /* HHVAR */
    std::unique_ptr<itemVariationStore> ivs;
    var_indexMap widthMap;
    var_indexMap lsbMap;
    var_indexMap rsbMap;
};

/* vertical metrics tables */

class var_vmtx {
 public:
    var_vmtx() {}
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
    bool lookup(ctlSharedStmCallbacks *sscb, const std::vector<Fixed> &instCoords,
                uint16_t gid, Fixed &width, Fixed &tsb);

    void vmtxClear(bool clearIvs = true) {
        if (clearIvs)
            ivs = nullptr;
        advanceVWidth.clear();
        tsb.clear();
        widthMap.clear();
        tsbMap.clear();
        bsbMap.clear();
    }
    void VORGClear() {
        defaultVertOrigin = 0;
        vertOriginY.clear();
        vorgMap.clear();
    }

    void nextVAdv(VarValueRecord &v, VarLocationMap &vlm, std::shared_ptr<slogger> logger);
    void nextTsb(VarValueRecord &v, VarLocationMap &vlm, std::shared_ptr<slogger> logger);
    void nextVOrig(uint16_t gid, VarValueRecord &v, VarLocationMap &vlm,
                   std::shared_ptr<slogger> logger);

    bool Fill();
    void write_vhea(VarWriter &vw);
    void write(VarWriter &vw);
    void write_VORG(VarWriter &vw);
    void write_VVAR(VarWriter &vw);

    struct var_vhea_ {
        Fixed version {0x00010001};
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
        int16_t metricDataFormat {0};
        uint16_t numOfLongVertMetrics {0};
    } header;
    std::vector<uint16_t> advanceVWidth;
    std::vector<int16_t> tsb;
    int16_t defaultVertOrigin {0};
    std::map<uint16_t, int16_t> vertOriginY;

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
    var_MVAR() {}
    var_MVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    /* lookup font-wide metric values for a tag blended using font instance
       coordinates.  Returns 0 if successful, otherwise non-zero.

    sscb       - a pointer to shared stream callback functions.
    axisCount  - the number of axes.
    instCoords - a pointer to normalized font instance coordinates. May be NULL if no blending required.
    tag        - the tag of the metric value to be looked up.
    value      - where the blended metric value is returned.
     */
    bool valueAdjust(ctlSharedStmCallbacks *sscb,
                     const std::vector<Fixed> &instCoords,
                     ctlTag tag, Fixed &value);

    void setAxisCount(uint16_t ac) {
        assert(axisCount == 0);
        axisCount = ac;
        if (ivs == nullptr) {
            ivs = std::make_unique<itemVariationStore>();
            ivs->setAxisCount(ac);
        }
    }
    void write(VarWriter &vw);
    void addValue(ctlTag tag, VarLocationMap &vlm, const VarValueRecord &vvr,
                  std::shared_ptr<slogger> logger);
    bool hasValues() { return values.size() > 0; }

 private:
    uint16_t axisCount {0};
    std::map<ctlTag, var_indexPair> values;
    std::unique_ptr<itemVariationStore> ivs;
};

class VarMetrics {
 public:
    VarMetrics() = delete;
    explicit VarMetrics(cfrCtx cfr, VarLocationMap &vlm, std::shared_ptr<slogger> logger) :
        cfr(cfr), vlm(vlm), logger(logger) {
        assert(cfr != nullptr);
        cfrIvs = cfrGetItemVariationStore(cfr);
        if (cfrIvs == nullptr)
            return;
        scratchIvs.setAxisCount(cfrIvs->getAxisCount());
    }
    struct GlyphInstMetrics {
        int32_t left;
        int32_t bottom;
        int32_t right;
        int32_t top;
        bool peak_glyph_region;
    };
    void prepGlyphData(uint16_t gid, uint16_t vsindex, const std::set<uint32_t> &locations);
    void prepGlyphData(uint16_t gid, uint16_t vsindex, uint32_t location) {
        std::set<uint32_t> locations;
        locations.insert(location);
        prepGlyphData(gid, vsindex, locations);
    }
    void prepGlyphData(uint16_t gid, uint16_t vsindex, const VarValueRecord &vvr) {
        auto locations = vvr.getLocations();
        prepGlyphData(gid, vsindex, locations);
    }
    VarValueRecord addVVR(const VarValueRecord &a, const VarValueRecord &b) {
        return ensureLocations(a, b.getLocations()).addSame(ensureLocations(b, a.getLocations()));
    }

    VarValueRecord subVVR(const VarValueRecord &a, const VarValueRecord &b) {
        return ensureLocations(a, b.getLocations()).subSame(ensureLocations(b, a.getLocations()));
    }
    VarValueRecord subTop(const VarValueRecord &v, uint16_t gid, uint16_t vsindex);
    GlyphInstMetrics &getGlyphData(uint16_t gid, uint16_t vsindex,
                                   uint32_t location, bool onRetry = false);
    float blendCurrent(uint16_t curIndex, abfBlendArg *v);
    std::vector<abfMetricsCtx_> currentInstState;
    std::vector<uint32_t> currentLocations;
    uint16_t currentVsindex {0};

 private:
    VarValueRecord ensureLocations(const VarValueRecord &v, const std::set<uint32_t> &locations);
    std::vector<Fixed> &getLocationScalars(uint32_t location);
    std::vector<uint16_t> &getRegionIndices();

    std::unordered_map<uint16_t, std::unordered_map<uint32_t, GlyphInstMetrics>> glyphData;
    std::unordered_map<uint32_t, std::vector<Fixed>> locationScalars;
    std::unordered_map<uint16_t, std::vector<uint16_t>> regionsForVsindex;

    cfrCtx cfr;
    VarLocationMap &vlm;
    std::shared_ptr<slogger> logger;

    itemVariationStore *cfrIvs {nullptr};
    itemVariationStore scratchIvs;
};

/* returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h).
 */

void varsupportGetVersion(ctlVersionCallbacks *cb);

#endif  // SHARED_INCLUDE_VARSUPPORT_H_
