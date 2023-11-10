/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Vertical metrics table.
 */

#include "vmtx.h"

#include <cassert>

void vmtxNew(hotCtx g) {
    g->ctx.vmtxp = new vmtx(g);
}

int vmtxFill(hotCtx g) {
    return g->ctx.vmtxp->Fill();
}

int vmtx::Fill() {
    uint32_t glyphCount = g->glyphs.size();

    if ((!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) && (!IS_CID(g)))
        return 0;

    advanceHeight.reserve(glyphCount);
    tsb.reserve(glyphCount);

    for (auto &g : g->glyphs) {
        advanceHeight.push_back(-g.vAdv);
        tsb.push_back(g.vOrigY - g.bbox.top);
    }

    /* Optimize metrics */
    FWord height = advanceHeight.back();
    int64_t i;
    for (i = (int64_t)advanceHeight.size() - 2; i >= 0; i--) {
        if (advanceHeight[i] != height)
            break;
    }
    if (i + 2 != (int64_t)advanceHeight.size())
        advanceHeight.resize(i+2);

    filled = true;
    return 1;
}

void vmtxWrite(hotCtx g) {
    g->ctx.vmtxp->Write();
}

void vmtx::Write() {
    auto h = this;
    auto tsbl = tsb.size();
    auto ahl = advanceHeight.size();

    assert(ahl <= tsbl);
    for (size_t i = 0; i < tsbl; i++) {
        if (i < ahl)
            OUT2(advanceHeight[i]);
        OUT2(tsb[i]);
    }
}

void vmtxReuse(hotCtx g) {
    delete g->ctx.vmtxp;
    g->ctx.vmtxp = new vmtx(g);
}

void vmtxFree(hotCtx g) {
    delete g->ctx.vmtxp;
    g->ctx.vmtxp = nullptr;
}
