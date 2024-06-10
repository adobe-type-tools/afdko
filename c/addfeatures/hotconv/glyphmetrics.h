/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.  This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef ADDFEATURES_HOTCONV_GLYPHMETRICS_H_
#define ADDFEATURES_HOTCONV_GLYPHMETRICS_H_

#include <cassert>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>
#include <utility>

#include "hotconv.h"
#include "varsupport.h"
#include "cffread_abs.h"
#include "supportfp.h"
#include "absfont.h"

class GlyphMetrics {
 public:
    GlyphMetrics() = delete;
    explicit GlyphMetrics(cfrCtx cfr, VarLocationMap &vlm, std::shared_ptr<slogger> logger) :
        cfr(cfr), vlm(vlm), logger(logger) {
        assert(cfr != nullptr);
        cfrIvs = cfrGetItemVariationStore(cfr);
        if (cfrIvs == nullptr)
            return;
        scratchIvs.setAxisCount(cfrIvs->getAxisCount());
    }
    struct InstMetrics {
        int32_t left;
        int32_t bottom;
        int32_t right;
        int32_t top;
        bool peak_glyph_region;
    };

    void initialProcessingRun(hotCtx g);
    void processGlyphs(const std::vector<uint16_t> &gid,
                       const std::set<uint32_t> &locations);
    void processGlyphs(const std::vector<uint16_t> &gids);
    void processGlyph(uint16_t gid, const VarValueRecord &vvr) {
        std::vector<uint16_t> gids = {gid};
        auto locations = vvr.getLocations();
        processGlyphs(gids, locations);
    }

    InstMetrics defaultMetrics(uint16_t gid) {
        auto gi = glyphData.find(gid);
        assert(gi != glyphData.end());
        auto li = gi->second.find(0);
        assert(li != gi->second.end());
        return li->second;
    }

    VarValueRecord getBBoxTop(uint16_t gid) {
        auto gi = glyphData.find(gid);
        assert(gi != glyphData.end());
        VarValueRecord r;
        for (auto &[l, im] : gi->second)
            r.addLocationValue(l, im.top);
        return r;
    }
    VarValueRecord getBBoxTop() {
        VarValueRecord r;
        for (auto &[l, im] : fontData)
            r.addLocationValue(l, im.top);
        return r;
    }

    VarValueRecord getBBoxBottom(uint16_t gid) {
        auto gi = glyphData.find(gid);
        assert(gi != glyphData.end());
        VarValueRecord r;
        for (auto &[l, im] : gi->second)
            r.addLocationValue(l, im.bottom);
        return r;
    }
    VarValueRecord getBBoxBottom() {
        VarValueRecord r;
        for (auto &[l, im] : fontData)
            r.addLocationValue(l, im.bottom);
        return r;
    }

    VarValueRecord addVVR(const VarValueRecord &a, const VarValueRecord &b) {
        return ensureLocations(a, b.getLocations()).addSame(ensureLocations(b, a.getLocations()));
    }

    VarValueRecord subVVR(const VarValueRecord &a, const VarValueRecord &b) {
        return ensureLocations(a, b.getLocations()).subSame(ensureLocations(b, a.getLocations()));
    }
    VarValueRecord subTop(const VarValueRecord &v, uint16_t gid);
    /* InstMetrics &getGlyphData(uint16_t gid, uint16_t vsindex,
                                   uint32_t location, bool onRetry = false);
    */
    float blendCurrent(uint16_t curIndex, abfBlendArg *v);
    void finishInstanceState(uint16_t gid, size_t i);
    void beginGlyph(abfGlyphInfo *info);
    void firstBlend(uint16_t vsindex);

    // state for current glyph iterations
    std::set<uint32_t> neededLocations;
    hotCtx hctxptr;
    // per-glyph
    uint16_t currentGID {0};
    std::vector<abfMetricsCtx_> currentInstState;
    std::vector<uint32_t> currentLocations;
    bool initialRun {true};
    int32_t currentVsindex {-1};

 private:
    VarValueRecord ensureLocations(const VarValueRecord &v, const std::set<uint32_t> &locations);
    std::vector<Fixed> &getLocationScalars(uint32_t location);
    std::vector<uint16_t> &getRegionIndices();

    std::unordered_map<uint16_t, std::unordered_map<uint32_t, InstMetrics>> glyphData;
    std::unordered_map<uint32_t, InstMetrics> fontData;
    std::unordered_map<uint32_t, std::vector<Fixed>> locationScalars;
    std::unordered_map<uint16_t, std::vector<uint16_t>> regionsForVsindex;

    cfrCtx cfr;
    VarLocationMap &vlm;
    std::shared_ptr<slogger> logger;

    itemVariationStore *cfrIvs {nullptr};
    itemVariationStore scratchIvs;
};

#endif  // ADDFEATURES_HOTCONV_GLYPHMETRICS_H_
