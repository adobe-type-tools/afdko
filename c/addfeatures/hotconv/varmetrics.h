/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.  This software is licensed as OpenSource, under the Apache License, Version 2.0.  This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef ADDFEATURES_HOTCONV_VARMETRICS_H_
#define ADDFEATURES_HOTCONV_VARMETRICS_H_

#include <cassert>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>
#include <utility>

#include "varsupport.h"
#include "cffread_abs.h"
#include "supportfp.h"
#include "absfont.h"

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

#endif  // ADDFEATURES_HOTCONV_VARMETRICS_H_
