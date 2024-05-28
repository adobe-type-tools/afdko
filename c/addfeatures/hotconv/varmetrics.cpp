/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "varmetrics.h"

std::vector<Fixed> &VarMetrics::getLocationScalars(uint32_t location) {
    auto i = locationScalars.find(location);
    if (i != locationScalars.end())
        return i->second;

    std::vector<Fixed> scalars;
    auto l = vlm.getLocation(location);
    assert(l != nullptr);
    cfrIvs->calcRegionScalars(logger, l->alocs, scalars);
    auto s = locationScalars.insert_or_assign(location, std::move(scalars));
    return s.first->second;
}

std::vector<uint16_t> &VarMetrics::getRegionIndices() {
    auto i = regionsForVsindex.find(currentVsindex);
    if (i != regionsForVsindex.end())
        return i->second;

    std::vector<uint16_t> regionIndices;
    cfrIvs->getRegionIndices(currentVsindex, regionIndices);
    auto s = regionsForVsindex.insert_or_assign(currentVsindex, std::move(regionIndices));
    return s.first->second;
}

float VarMetrics::blendCurrent(uint16_t curIndex, abfBlendArg *v) {
    assert(curIndex < currentLocations.size());
    uint32_t loc = currentLocations[curIndex];

    auto lsi = locationScalars.find(loc);
    assert(lsi != locationScalars.end());
    auto &locScalars = lsi->second;
    auto &regions = getRegionIndices();

    float r = v->value, fsc;
    if (v->hasBlend) {
        for (size_t i = 0; i < regions.size(); i++) {
            assert(regions[i] < locScalars.size());
            fixtopflt(locScalars[regions[i]], &fsc);
            r += v->blendValues[i] * fsc;
        }
    }
    return r;
}

static int vmetricsBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
    cb->info = info;
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    for (auto &cs : vm->currentInstState) {
        cb->direct_ctx = (void *) &cs;
        abfGlyphMetricsCallbacks.beg(cb, info);
    }
    cb->direct_ctx = (void *) vm;
    return ABF_CONT_RET;
}

static void vmetricsWidth(abfGlyphCallbacks *cb, float w) {}

static void vmetricsMoveVF(abfGlyphCallbacks *cb, abfBlendArg *x0, abfBlendArg *y0) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_0 = vm->blendCurrent(i, x0);
        float y_0 = vm->blendCurrent(i, y0);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.move(cb, x_0, y_0);
    }
    cb->direct_ctx = (void *) vm;
}

static void vmetricsLineVF(abfGlyphCallbacks *cb, abfBlendArg *x1, abfBlendArg *y1) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_1 = vm->blendCurrent(i, x1);
        float y_1 = vm->blendCurrent(i, y1);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.line(cb, x_1, y_1);
    }
    cb->direct_ctx = (void *) vm;
}

static void vmetricsCurveVF(abfGlyphCallbacks *cb,
                            abfBlendArg *x1, abfBlendArg *y1,
                            abfBlendArg *x2, abfBlendArg *y2,
                            abfBlendArg *x3, abfBlendArg *y3) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_1 = vm->blendCurrent(i, x1);
        float y_1 = vm->blendCurrent(i, y1);
        float x_2 = vm->blendCurrent(i, x2);
        float y_2 = vm->blendCurrent(i, y2);
        float x_3 = vm->blendCurrent(i, x3);
        float y_3 = vm->blendCurrent(i, y3);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.curve(cb, x_1, y_1, x_2, y_2, x_3, y_3);
    }
    cb->direct_ctx = (void *) vm;
}

static void vmetricsEnd(abfGlyphCallbacks *cb) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    for (auto &cs : vm->currentInstState) {
        cb->direct_ctx = (void *) &cs;
        abfGlyphMetricsCallbacks.end(cb);
    }
    cb->direct_ctx = (void *) vm;
}

void VarMetrics::prepGlyphData(uint16_t gid, uint16_t vsindex,
                               const std::set<uint32_t> &locations) {
    currentVsindex = vsindex;
    currentLocations.clear();
    std::set<uint32_t> glyphLocations;
    auto gi = glyphData.find(gid);
    if (gi == glyphData.end()) {
        cfrIvs->getLocationIndices(vsindex, vlm, glyphLocations);
        for (auto l : glyphLocations) {
            currentLocations.push_back(l);
            getLocationScalars(l);
        }
    }
    for (auto l : locations) {
        if (glyphLocations.find(l) != glyphLocations.end())  // already added location
            continue;
        if (gi == glyphData.end() || gi->second.find(l) == gi->second.end()) {
            currentLocations.push_back(l);
            getLocationScalars(l);
        }
    }
    currentInstState.resize(currentLocations.size());
    static abfGlyphCallbacks glyphcb = {
        (void *)this,
        NULL,
        NULL,
        vmetricsBeg,
        vmetricsWidth,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        vmetricsEnd,
        vmetricsMoveVF,
        vmetricsLineVF,
        vmetricsCurveVF,
        NULL
    };

    cfrGetGlyphByTag(cfr, gid, &glyphcb);

    for (size_t i = 0; i < currentInstState.size(); i++) {
        auto &is = currentInstState[i];
        GlyphInstMetrics gim;
        gim.left = is.int_mtx.left;
        gim.bottom = is.int_mtx.bottom;
        gim.right = is.int_mtx.right;
        gim.top = is.int_mtx.top;
        gim.peak_glyph_region = glyphLocations.find(currentLocations[i]) != glyphLocations.end();
        // insert won't overwrite an existing entry, so this
        // works whether there is or is not a GID entry already
        auto gii = glyphData.insert({gid, {}});
        gii.first->second.insert({currentLocations[i], gim});
    }
}

VarValueRecord VarMetrics::ensureLocations(const VarValueRecord &v,
                                           const std::set<uint32_t> &locations) {
    assert(v.hasLocation(0));
    VarValueRecord r = v;
    var_indexPair pair {0xFFFF, 0xFFFF};
    std::vector<Fixed> scalars;
    for (auto l : locations) {
        if (v.hasLocation(l))
            continue;
        if (pair.outerIndex == 0xFFFF)
            pair = scratchIvs.addValue(vlm, v, logger);
        auto lp = vlm.getLocation(l);
        assert(lp != nullptr);
        // XXX This could be better optimized
        scratchIvs.calcRegionScalars(logger, lp->alocs, scalars);
        Fixed adjustment = scratchIvs.applyDeltasForIndexPair(pair, scalars, logger);
        r.addLocationValue(l, v.getDefault() + FRound(adjustment), logger);
    }
    // XXX scratchIvs.clear();
    return r;
}

/* Subtract the top of the bounding box from the value. As different
 * instances will have different bounding boxes, this calculation
 * needs to find the bounding box of both the relevant glyph instances
 * and of the instances in v, if those are different
 */
VarValueRecord VarMetrics::subTop(const VarValueRecord &v, uint16_t gid, uint16_t vsindex) {
    prepGlyphData(gid, vsindex, v);
    VarValueRecord top;
    auto gi = glyphData.find(gid);
    assert(gi != glyphData.end());
    for (auto &[l, gim] : gi->second) {
        if (gim.peak_glyph_region || v.hasLocation(l)) {
            if (l == 0)
                top.addValue(gim.top);
            else
                top.addLocationValue(l, gim.top, logger);
        }
    }
    return ensureLocations(v, top.getLocations()).subSame(top);
}

// XXX unused?
VarMetrics::GlyphInstMetrics &VarMetrics::getGlyphData(uint16_t gid,
                                                       uint16_t vsindex,
                                                       uint32_t location,
                                                       bool onRetry) {
    auto gi = glyphData.find(gid);
    if (gi == glyphData.end()) {
        if (onRetry) {
            logger->log(sFATAL, "Internal error: Could not load metrics data "
                                 "for glyph %d", (int)gid);
            GlyphInstMetrics fake;
            return fake;
        } else {
            prepGlyphData(gid, vsindex, location);
            return getGlyphData(gid, vsindex, location, true);
        }
    }
    auto li = gi->second.find(location);
    if (li == gi->second.end()) {
        if (onRetry) {
            logger->log(sFATAL, "Internal error: Could not load metrics data "
                                 "for glyph %d", (int)gid);
            GlyphInstMetrics fake;
            return fake;
        } else {
            prepGlyphData(gid, vsindex, location);
            return getGlyphData(gid, vsindex, location, true);
        }
    }
    return li->second;
}
