/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Vertical metrics table.
 */

#include "vmtx.h"

#include <cassert>

#include "varmetrics.h"

void vmtxNew(hotCtx g) {
}

int vmtxFill(hotCtx g) {
    if (g->ctx.vmtx == nullptr)
        return 0;

    auto &vmtx = *g->ctx.vmtx;

    if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) && (!IS_CID(g)))
        return vmtx.Fill();

    uint32_t glyphCount = g->glyphs.size();
    VarValueRecord dfltVAdv = g->ctx.vm->subVVR(g->font.TypoAscender, g->font.TypoDescender);
    VarValueRecord dfltVOrigY = g->font.TypoAscender;

    vmtx.advanceVWidth.reserve(glyphCount);
    vmtx.tsb.reserve(glyphCount);

    vmtx.vmtxClear();

    int32_t gid = 0;
    for (auto &gl : g->glyphs) {
        if (gl.vAdv.isInitialized())
            vmtx.nextVAdv(gl.vAdv, *g->ctx.locMap, g->logger);
        else
            vmtx.nextVAdv(dfltVAdv, *g->ctx.locMap, g->logger);
        VarValueRecord cv;
        auto &VO = gl.vOrigY.isInitialized() ? gl.vOrigY : dfltVOrigY;
        if (g->ctx.axes != nullptr)
            cv = g->ctx.vm->subTop(VO, gid, g->glyphs[gid].vsindex);
        else
            cv.addValue(VO.getDefault() - gl.bbox.top);
        vmtx.nextTsb(cv, *g->ctx.locMap, g->logger);
        gid++;
    }

    /* Optimize metrics */
    FWord vWidth = vmtx.advanceVWidth.back();
    for (gid = vmtx.advanceVWidth.size() - 2; gid >= 0; gid--) {
        if (vmtx.advanceVWidth[gid] != vWidth)
            break;
    }
    if (gid + 2 != vmtx.advanceVWidth.size())
        vmtx.advanceVWidth.resize(gid+2);

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
    header.vertTypoAscender = g->font.VertTypoAscender.getDefault();
    g->ctx.MVAR->addValue(MVAR_vasc_tag, *g->ctx.locMap, g->font.VertTypoAscender, g->logger);
    header.vertTypoDescender = g->font.VertTypoDescender.getDefault();
    g->ctx.MVAR->addValue(MVAR_vdsc_tag, *g->ctx.locMap, g->font.VertTypoDescender, g->logger);
    header.vertTypoLineGap = g->font.VertTypoLineGap.getDefault();
    g->ctx.MVAR->addValue(MVAR_vlgp_tag, *g->ctx.locMap, g->font.VertTypoLineGap, g->logger);

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

    vmtx.VORGClear();

    int16_t dflt;
    vmtx.defaultVertOrigin = dflt = g->font.TypoAscender.getDefault();

    int32_t gid = 0;
    for (auto &gl : g->glyphs) {
        vmtx.nextVOrig(gid, gl.vOrigY, *g->ctx.locMap, g->logger);
        gid++;
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

