/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Vertical metrics table.
 */

#include "vmtx.h"

#include <cassert>

void vmtxNew(hotCtx g) {
}

int vmtxFill(hotCtx g) {
    if (g->ctx.vmtx == nullptr)
        return 0;

    auto &vmtx = *g->ctx.vmtx;

    if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) && (!IS_CID(g)))
        return vmtx.Fill();

    uint32_t glyphCount = g->glyphs.size();
    int16_t dfltVAdv = g->font.TypoAscender - g->font.TypoDescender;
    int16_t dfltVOrigY = g->font.TypoAscender;

    vmtx.advanceVWidth.reserve(glyphCount);
    vmtx.tsb.reserve(glyphCount);

    for (auto &gl : g->glyphs) {
        int16_t vAdv {dfltVAdv};
        if (gl.vAdv.isInitialized())
            vAdv = gl.vAdv.getDefault();
        int16_t vOrigY {dfltVOrigY};
        if (gl.vOrigY.isInitialized())
            vOrigY = gl.vOrigY.getDefault();
        vmtx.advanceVWidth.push_back(vAdv);
        vmtx.tsb.push_back(vOrigY - gl.bbox.top);
    }

    /* Optimize metrics */
    FWord vWidth = vmtx.advanceVWidth.back();
    int64_t i;
    for (i = vmtx.advanceVWidth.size() - 2; i >= 0; i--) {
        if (vmtx.advanceVWidth[i] != vWidth)
            break;
    }
    if (i + 2 != vmtx.advanceVWidth.size())
        vmtx.advanceVWidth.resize(i+2);

    return vmtx.Fill();
}

void vmtxWrite(hotCtx g) {
    g->ctx.vmtx->write(g->vw);
}

void vmtxReuse(hotCtx g) {
    delete g->ctx.vmtx;
    g->ctx.vmtx = nullptr;
}

void vmtxFree(hotCtx g) {
    delete g->ctx.vmtx;
    g->ctx.vmtx = nullptr;
}

void vheaNew(hotCtx g) { }

int vheaFill(hotCtx g) {
    if (g->ctx.vmtx == nullptr) {
        if ((g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE) || IS_CID(g))
            g->ctx.vmtx = new var_vmtx();
        else
            return 0;
    }

    auto &header = g->ctx.vmtx->header;

    header.version = VERSION(1, 1);
    header.vertTypoAscender = g->font.VertTypoAscender;
    header.vertTypoDescender = g->font.VertTypoDescender;
    header.vertTypoLineGap = g->font.VertTypoLineGap;

    header.advanceHeightMax = g->font.maxAdv.v;
    header.minTop = g->font.minBearing.top;
    header.minBottom = g->font.minBearing.bottom;
    header.yMaxExtent = g->font.maxExtent.v;

    /* Always set a horizontal caret for vertical writing */
    header.caretSlopeRise = 0;
    header.caretSlopeRun = 1;
    header.caretOffset = 0;

    return 1;
}

void vheaWrite(hotCtx g) {
    assert(g->ctx.vmtx != nullptr);
    g->ctx.vmtx->write_vhea(g->vw);
}

void vheaReuse(hotCtx g) { }

void vheaFree(hotCtx g) { }

void VORGNew(hotCtx g) { }

int VORGFill(hotCtx g) {
    if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) && (!IS_CID(g))) {
        if (g->ctx.vmtx == nullptr)
            return 0;
        else
            return !g->ctx.vmtx->vertOriginY.empty();
    }

    assert(g->ctx.vmtx != nullptr);

    auto &vmtx = *g->ctx.vmtx;

    int16_t dflt;
    vmtx.defaultVertOrigin = dflt = g->font.TypoAscender;
    vmtx.vertOriginY.clear();

    for (size_t i = 0; i < g->glyphs.size(); i++) {
        auto &gl = g->glyphs[i];
        
        if (gl.vOrigY.isInitialized() && gl.vOrigY.getDefault() != dflt)
            vmtx.vertOriginY.emplace((uint16_t) i, gl.vOrigY.getDefault());
    }

    return 1;
}

void VORGWrite(hotCtx g) {
    assert(g->ctx.vmtx != nullptr);
    g->ctx.vmtx->write_VORG(g->vw);
}

void VORGReuse(hotCtx g) { }

void VORGFree(hotCtx g) { }

// hhea data is part of the var_hmtx object
void VVARNew(hotCtx g) { }

int VVARFill(hotCtx g) {
    if (g->ctx.vmtx == nullptr)
        return 0;
    else
        return g->ctx.vmtx->ivs != nullptr && g->ctx.vmtx->ivs->numSubtables() > 0;
}

void VVARWrite(hotCtx g) {
    assert(g->ctx.vmtx != nullptr);
    g->ctx.vmtx->write_VVAR(g->vw);
}

void VVARReuse(hotCtx g) { }

void VVARFree(hotCtx g) { }

