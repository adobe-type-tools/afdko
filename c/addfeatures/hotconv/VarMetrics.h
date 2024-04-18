/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#ifndef ADDFEATURES_HOTCONV_VARMETRICS_H_
#define ADDFEATURES_HOTCONV_VARMETRICS_H_

#include <cassert>
#include <unordered_map>

#include "common.h"
#include "cffread_abs.h"

class VarMetrics {
 public:
    VarMetrics() = delete;
    explicit VarMetrics(cfrCtx cfr, std::shared_ptr<slogger> logger) : 
        cfr(cfr), logger(logger) {
        ivs = cfrGetItemVariationStore(cfr);
    }
    struct GlyphInstMetrics {
        BBox bbox;
    };
    void prepGlyphData(GID gid, const std::vector<uint32_t> &locations, VarLocationMap &vlm);
    void prepGlyphData(GID gid, uint32_t location, VarLocationMap &vlm) {
        std::vector<uint32_t> locations = {location};
        prepGlyphData(gid, locations, vlm);
    }
    void prepGlyphData(GID gid, VarValueRecord &vvr, VarLocationMap &vlm) {
        auto locations = vvr.getLocations();
        prepGlyphData(gid, locations, vlm);
    }
    GlyphInstMetrics &getGlyphData(GID gid, uint32_t location, VarLocationMap &vlm, bool onRetry = false);
    float blendCurrent(uint16_t vsindex, uint16_t curIndex, abfBlendArg *v);
    std::vector<abfMetricsCtx_> currentInstState;
    std::vector<uint32_t> currentLocations;
 private:
    std::vector<Fixed> &getLocationScalars(uint32_t location, VarLocationMap &vlm);
    std::vector<uint16_t> &getRegionIndices(uint16_t vsindex);

    std::unordered_map<GID,std::unordered_map<uint32_t, GlyphInstMetrics>> glyphData;
    std::unordered_map<uint32_t, std::vector<Fixed>> locationScalars;
    std::unordered_map<uint16_t, std::vector<uint16_t>> regionsForVsindex;
    cfrCtx cfr;

    std::shared_ptr<slogger> logger;
    itemVariationStore *ivs {nullptr};
};

#endif  // ADDFEATURES_HOTCONV_VARMETRICS_H_
