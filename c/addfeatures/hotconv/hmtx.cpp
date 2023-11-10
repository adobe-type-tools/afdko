/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 *  Horizontal metrics table.
 */

#include "hmtx.h"

#include <cassert>

/* --------------------------- Standard Functions -------------------------- */

void hmtxNew(hotCtx g) {
    g->ctx.hmtxp = new hmtx(g);
}

int hmtxFill(hotCtx g) {
    return g->ctx.hmtxp->Fill();
}

int hmtx::Fill() {
    /* Fill table */
    uint32_t glyphCount = g->glyphs.size();

    advanceWidth.reserve(glyphCount);
    lsb.reserve(glyphCount);

    for (auto &g : g->glyphs) {
        advanceWidth.push_back(g.hAdv);
        lsb.push_back(g.bbox.left);
    }

    /* Optimize metrics */
    FWord width = advanceWidth.back();
    size_t i;
    for (i = advanceWidth.size() - 2; i >= 0; i--) {
        if (advanceWidth[i] != width)
            break;
    }
    if (i + 2 != advanceWidth.size())
        advanceWidth.resize(i+2);

    filled = true;
    return 1;
}

void hmtxWrite(hotCtx g) {
    g->ctx.hmtxp->Write();
}

void hmtx::Write() {
    /* Write horizontal metrics */
    auto h = this;
    auto lsbl = lsb.size();
    auto awl = advanceWidth.size();

    assert(awl <= lsbl);
    for (size_t i = 0; i < lsbl; i++) {
        if (i < awl)
            OUT2(advanceWidth[i]);
        OUT2(lsb[i]);
    }
}

void hmtxReuse(hotCtx g) {
    delete g->ctx.hmtxp;
    g->ctx.hmtxp = new hmtx(g);
}

void hmtxFree(hotCtx g) {
    delete g->ctx.hmtxp;
    g->ctx.hmtxp = nullptr;
}
