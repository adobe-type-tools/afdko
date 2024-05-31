/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "glyphmetrics.h"

#include <limits>
#include <algorithm>

#include "common.h"

std::vector<Fixed> &GlyphMetrics::getLocationScalars(uint32_t location) {
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

std::vector<uint16_t> &GlyphMetrics::getRegionIndices() {
    auto i = regionsForVsindex.find(currentVsindex);
    if (i != regionsForVsindex.end())
        return i->second;

    std::vector<uint16_t> regionIndices;
    cfrIvs->getRegionIndices(currentVsindex, regionIndices);
    auto s = regionsForVsindex.insert_or_assign(currentVsindex, std::move(regionIndices));
    return s.first->second;
}

float GlyphMetrics::blendCurrent(uint16_t curIndex, abfBlendArg *v) {
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

void GlyphMetrics::beginGlyph(abfGlyphInfo *info) {
    if (currentGID == 0xFFFF)
        currentGID = info->tag;
    assert(currentGID == info->tag);

    auto gi = glyphData.find(currentGID);

    if (initialRun) {
        assert(gi == glyphData.end());
        currentLocations.push_back(0);
        if (cfrIvs != nullptr)
            getLocationScalars(0);
        if (hctxptr != nullptr) {
            auto &glyph = hctxptr->glyphs[currentGID];

            if (info->gname.ptr != NULL)
                glyph.gname = info->gname.ptr;
            if (IS_ROS(hctxptr))
                glyph.id = info->cid;
            else
                glyph.id = info->gname.impl;  // The SID (when CFF 1)

            glyph.code = info->encoding.code;
            abfEncoding *e = info->encoding.next;
            while (e != NULL) {
                glyph.sup.push_back(e->code);
                e = e->next;
            }
        }
    }
    for (auto l : neededLocations) {
        assert(cfrIvs != nullptr);
        if (l != 0 && (gi == glyphData.end() ||
                       gi->second.find(l) == gi->second.end())) {
            currentLocations.push_back(l);
            getLocationScalars(l);
        }
    }
    currentInstState.resize(currentLocations.size());
}

static int gmetricsBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
    cb->info = info;
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    vm->beginGlyph(info);

    if (vm->currentInstState.size() == 0)  // locations already found
        return ABF_SKIP_RET;

    for (auto &cs : vm->currentInstState) {
        cb->direct_ctx = (void *) &cs;
        abfGlyphMetricsCallbacks.beg(cb, info);
    }
    cb->direct_ctx = (void *) vm;
    return ABF_CONT_RET;
}

static void gmetricsWidth(abfGlyphCallbacks *cb, float w) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    if (vm->hctxptr != nullptr) {
        vm->hctxptr->glyphs[vm->currentGID].hAdv = w;
    }
}

static void gmetricsMove(abfGlyphCallbacks *cb, float x0, float y0) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.move(cb, x0, y0);
    }
    cb->direct_ctx = (void *) vm;
}

static void gmetricsLine(abfGlyphCallbacks *cb, float x1, float y1) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.line(cb, x1, y1);
    }
    cb->direct_ctx = (void *) vm;
}

static void gmetricsCurve(abfGlyphCallbacks *cb, float x1, float y1,
                          float x2, float y2, float x3, float y3) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.curve(cb, x1, y1, x2, y2, x3, y3);
    }
    cb->direct_ctx = (void *) vm;
}

void GlyphMetrics::firstBlend(uint16_t vsindex) {
    currentVsindex = vsindex;
    assert(cfrIvs != nullptr);

    if (initialRun) {
        assert(glyphData.find(currentGID) == glyphData.end());
        std::set<uint32_t> glyphLocations;
        assert(currentLocations[0] == 0);
        cfrIvs->getLocationIndices(vsindex, vlm, glyphLocations);
        for (auto l : glyphLocations) {
            auto iter = std::find(currentLocations.begin(), currentLocations.end(), l);
            if (iter != currentLocations.end()) {
                currentLocations.push_back(l);
                // Copy the inst state for location zero to use for new location
                currentInstState.push_back(currentInstState[0]);
                getLocationScalars(l);
            }
        }
    }
}

static void gmetricsMoveVF(abfGlyphCallbacks *cb, abfBlendArg *x0, abfBlendArg *y0) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    if (vm->currentVsindex == -1)
        vm->firstBlend(cb->info->blendInfo.vsindex);

    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_0 = vm->blendCurrent(i, x0);
        float y_0 = vm->blendCurrent(i, y0);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.move(cb, x_0, y_0);
    }
    cb->direct_ctx = (void *) vm;
}

static void gmetricsLineVF(abfGlyphCallbacks *cb, abfBlendArg *x1, abfBlendArg *y1) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    if (vm->currentVsindex == -1)
        vm->firstBlend(cb->info->blendInfo.vsindex);

    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        float x_1 = vm->blendCurrent(i, x1);
        float y_1 = vm->blendCurrent(i, y1);
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.line(cb, x_1, y_1);
    }
    cb->direct_ctx = (void *) vm;
}

static void gmetricsCurveVF(abfGlyphCallbacks *cb,
                            abfBlendArg *x1, abfBlendArg *y1,
                            abfBlendArg *x2, abfBlendArg *y2,
                            abfBlendArg *x3, abfBlendArg *y3) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    if (vm->currentVsindex == -1)
        vm->firstBlend(cb->info->blendInfo.vsindex);

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

static void gmetricsEnd(abfGlyphCallbacks *cb) {
    GlyphMetrics *vm = (GlyphMetrics *)cb->direct_ctx;
    for (size_t i = 0; i < vm->currentInstState.size(); i++) {
        cb->direct_ctx = (void *) &vm->currentInstState[i];
        abfGlyphMetricsCallbacks.end(cb);
        vm->finishInstanceState(vm->currentGID, i);
    }
    cb->direct_ctx = (void *) vm;
    vm->currentGID = 0xFFFF;
    vm->currentVsindex = -1;
    vm->currentInstState.clear();
    vm->currentLocations.clear();
}

void GlyphMetrics::finishInstanceState(uint16_t gid, size_t i) {
    auto &is = currentInstState[i];
    InstMetrics im;
    im.left = is.int_mtx.left;
    im.bottom = is.int_mtx.bottom;
    im.right = is.int_mtx.right;
    im.top = is.int_mtx.top;
    im.peak_glyph_region = initialRun;
    // insert won't overwrite an existing entry, so this
    // works whether there is or is not a GID entry already
    auto gii = glyphData.insert({gid, {}});
    gii.first->second.insert({currentLocations[i], im});

    if (im.left == 0 && im.right == 0 && im.bottom == 0 && im.top == 0)
        return;

    if (initialRun && hctxptr != nullptr && currentLocations[i] == 0) {
        if (hctxptr->font.maxAdv.h < hctxptr->glyphs[gid].hAdv)
            hctxptr->font.maxAdv.h = hctxptr->glyphs[gid].hAdv;

        if (im.left < hctxptr->font.minBearing.left)
            hctxptr->font.minBearing.left = im.left;

        auto hAdv = hctxptr->glyphs[gid].hAdv;
        if (hAdv - im.right < hctxptr->font.minBearing.right)
            hctxptr->font.minBearing.right = hAdv - im.right;

        if (im.right > hctxptr->font.maxExtent.h)
            hctxptr->font.maxExtent.h = im.right;
    }
}

void GlyphMetrics::initialProcessingRun(hotCtx g) {
    neededLocations.clear();
    initialRun = true;
    hctxptr = g;
    std::vector<uint16_t> gids;  // empty to scan all glyphs
    processGlyphs(gids);

    using NL = std::numeric_limits<int32_t>;
    InstMetrics fdinit = { NL::max(), NL::max(), NL::min(), NL::min(), false };
    for (auto &[gid, lmap] : glyphData) {
        for (auto &[l, ldata] : lmap) {
            if (ldata.top == 0 && ldata.bottom == 0 &&
                ldata.left == 0 && ldata.right == 0)
                continue;
            auto fdi = fontData.insert({l, fdinit});
            auto &d = fdi.first->second;
            d.left = std::min(ldata.left, d.left);
            d.bottom = std::min(ldata.bottom, d.bottom);
            d.top = std::max(ldata.top, d.top);
            d.right = std::max(ldata.right, d.right);
        }
    }
    hctxptr = nullptr;
}

void GlyphMetrics::processGlyphs(const std::vector<uint16_t> &gids,
                                 const std::set<uint32_t> &locations) {
    neededLocations = locations;
    initialRun = false;
    processGlyphs(gids);
}

void GlyphMetrics::processGlyphs(const std::vector<uint16_t> &gids) {
    static abfGlyphCallbacks glyphcb = {
        (void *)this,
        NULL,
        NULL,
        gmetricsBeg,
        gmetricsWidth,
        gmetricsMove,
        gmetricsLine,
        gmetricsCurve,
        NULL,
        NULL,
        NULL,
        NULL,
        gmetricsEnd,
        gmetricsMoveVF,
        gmetricsLineVF,
        gmetricsCurveVF,
        NULL
    };
    assert(currentVsindex == -1);
    assert(currentInstState.size() == 0);
    assert(currentLocations.size() == 0);
    if (gids.size() > 0) {
        for (auto gid : gids) {
            currentGID = gid;
            cfrGetGlyphByTag(cfr, gid, &glyphcb);
        }
    } else {
        currentGID = 0xFFFF;
        cfrIterateGlyphs(cfr, &glyphcb);
    }
}

VarValueRecord GlyphMetrics::ensureLocations(const VarValueRecord &v,
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
VarValueRecord GlyphMetrics::subTop(const VarValueRecord &v, uint16_t gid) {
    if (v.isVariable())
        processGlyph(gid, v);
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

/*
GlyphMetrics::InstMetrics &GlyphMetrics::getGlyphData(uint16_t gid,
                                                       uint16_t vsindex,
                                                       uint32_t location,
                                                       bool onRetry) {
    auto gi = glyphData.find(gid);
    if (gi == glyphData.end()) {
        if (onRetry) {
            logger->log(sFATAL, "Internal error: Could not load metrics data "
                                 "for glyph %d", (int)gid);
            InstMetrics fake;
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
            InstMetrics fake;
            return fake;
        } else {
            prepGlyphData(gid, vsindex, location);
            return getGlyphData(gid, vsindex, location, true);
        }
    }
    return li->second;
}
*/
