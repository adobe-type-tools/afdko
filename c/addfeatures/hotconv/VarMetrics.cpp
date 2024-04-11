/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "VarMetrics.h"


std::vector<Fixed> &VarMetrics::getLocationScalars(uint32_t location,
                                                   VarLocationMap &vlm) {
    auto i = locationScalars.find(location);
    if (i != locationScalars.end())
        return i->second;

    std::vector<Fixed> scalars;
    auto l = vlm.getLocation(location);
    assert(l != nullptr);
    ivs->calcRegionScalars(logger, l->alocs, scalars);
    auto s = locationScalars.insert_or_assign(location, std::move(scalars));
    return s.first->second;
}

std::vector<uint16_t> &VarMetrics::getRegionIndices(uint16_t vsindex) {
    auto i = regionsForVsindex.find(vsindex);
    if (i != regionsForVsindex.end())
        return i->second;

    std::vector<uint16_t> regionIndices;
    ivs->getRegionIndices(vsindex, regionIndices);
    auto s = regionsForVsindex.insert_or_assign(vsindex, std::move(regionIndices));
    return s.first->second;
}

float VarMetrics::blendCurrent(uint16_t vsindex, uint16_t curIndex, abfBlendArg *v) {
    assert(curIndex < currentLocations.size());
    uint32_t loc = currentLocations[curIndex];

    auto lsi = locationScalars.find(loc);
    assert(lsi != locationScalars.end());
    auto &locScalars = lsi->second;
    auto &regions = getRegionIndices(vsindex);

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

static void vmetricsMoveVF(abfGlyphCallbacks *cb, abfBlendArg *x0, abfBlendArg *y0) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    auto vsindex = cb->info->blendInfo.vsindex;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_0 = vm->blendCurrent(vsindex, i, x0);
        float y_0 = vm->blendCurrent(vsindex, i, y0);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.move(cb, x_0, y_0);
    }
    cb->direct_ctx = (void *) vm;
}

static void vmetricsLineVF(abfGlyphCallbacks *cb, abfBlendArg *x1, abfBlendArg *y1) {
    VarMetrics *vm = (VarMetrics *)cb->direct_ctx;
    auto vsindex = cb->info->blendInfo.vsindex;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_1 = vm->blendCurrent(vsindex, i, x1);
        float y_1 = vm->blendCurrent(vsindex, i, y1);
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
    auto vsindex = cb->info->blendInfo.vsindex;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_1 = vm->blendCurrent(vsindex, i, x1);
        float y_1 = vm->blendCurrent(vsindex, i, y1);
        float x_2 = vm->blendCurrent(vsindex, i, x2);
        float y_2 = vm->blendCurrent(vsindex, i, y2);
        float x_3 = vm->blendCurrent(vsindex, i, x3);
        float y_3 = vm->blendCurrent(vsindex, i, y3);
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

void VarMetrics::prepGlyphData(GID gid, const std::vector<uint32_t> locations,
                               VarLocationMap &vlm) {
    auto gi = glyphData.find(gid);
    if (gi == glyphData.end())
        currentLocations = locations;
    else {
        currentLocations.clear();
        for (auto l : locations)
            if (gi->second.find(l) == gi->second.end())
                currentLocations.push_back(l);
    }
    for (uint32_t loc : currentLocations)
        getLocationScalars(loc, vlm);  // Pre-load the scalars
    currentInstState.resize(currentLocations.size());
    static abfGlyphCallbacks glyphcb = {
        (void *)this,
        NULL,
        NULL,
        vmetricsBeg,
        NULL,
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
        gim.bbox.left = (int16_t) is.int_mtx.left;
        gim.bbox.bottom = (int16_t) is.int_mtx.bottom;
        gim.bbox.right = (int16_t) is.int_mtx.right;
        gim.bbox.top = (int16_t) is.int_mtx.top ;
        // insert won't overwrite an existing entry, so this
        // works whether there is or is not a GID entry already
        auto gii = glyphData.insert({gid, {}});
        gii.first->second.insert({i, gim});
    }
}

VarMetrics::GlyphInstMetrics &VarMetrics::getGlyphData(GID gid,
                                                       uint32_t location,
                                                       VarLocationMap &vlm,
                                                       bool onRetry) {
    auto gi = glyphData.find(gid);
    if (gi == glyphData.end()) {
        if (onRetry) {
            logger->log(sFATAL, "Internal error: Could not load metrics data "
                                 "for glyph %d", (int)gid);
            GlyphInstMetrics fake;
            return fake;
        } else {
            prepGlyphData(gid, location, vlm);
            return getGlyphData(gid, location, vlm, true);
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
            prepGlyphData(gid, location, vlm);
            return getGlyphData(gid, location, vlm, true);
        }
    }
    return li->second;
}
