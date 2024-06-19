/* Copyright 2014-2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Baseline table (default values can be adjusted in the feature file)
 */

#include "BASE.h"

#include <stdlib.h>
#include <stdio.h>

void BASENew(hotCtx g) {
    g->ctx.BASEp = new BASE(g);
}

void BASE::Axis::prep(hotCtx g) {
    if (baseTagList.size() == 0)
        return;

    if (baseScriptList.size() == 0)
        g->logger->log(sFATAL, "scripts not specified for %s baseline axis", desc);

    std::sort(baseScriptList.begin(), baseScriptList.end());
}

Offset BASE::Axis::fill(Offset curr) {
    Offset sz = size();
    auto nBaseTags = baseTagList.size();

    if (nBaseTags == 0) {
        o = NULL_OFFSET;
        return 0;
    }

    o = curr;

    baseTagOffset = sz;
    sz += tag_list_size();
    baseScriptOffset = sz;
    for (auto &bsi : baseScriptList)  // Size is adjusted later
        bsi.baseScriptOffset = BaseValues::script_size(0) * bsi.baseScriptInx - (curr + sz);

    sz += script_list_size();

    return sz;
}

Offset BASE::fillSharedData() {
    auto nBScr = baseScript.size();
    Offset bsSize = 0; /* Accumulator for BaseScript section */
    Offset bvSize = 0; /* Accumulator for BaseValues section */
    Offset bsTotal = (Offset)(nBScr * BaseValues::script_size(0));

    baseValues.reserve(nBScr);

    for (auto &bsi : baseScript) {
        uint32_t nCoord = bsi.coordInx.size();
        BaseValues bv;

        bv.DefaultIndex = bsi.dfltBaselineInx;
        bv.o = bsTotal - bsSize + bvSize;

        bv.BaseCoordOffset.reserve(nCoord);
        for (auto c : bsi.coordInx)
            bv.BaseCoordOffset.push_back(baseCoords[c].o - bvSize);

        bsSize += bv.script_size(0);
        bvSize += bv.size();

        baseValues.emplace_back(std::move(bv));

    }

    /* Adjust BaseValue coord offsets */
    for (auto &bv : baseValues) {
        for (auto &bc : bv.BaseCoordOffset)
            bc += bvSize;
    }

    auto &lastbc = baseCoords.back();

    return bsSize + bvSize + lastbc.o + lastbc.size();
}

int BASEFill(hotCtx g) {
    BASE *h = g->ctx.BASEp;
    return h->Fill();
}

int BASE::Fill() {
    if (HorizAxis.baseTagList.size() == 0 && VertAxis.baseTagList.size() == 0)
        return 0;

    HorizAxis.prep(g);
    VertAxis.prep(g);

    bool seenVariable = false;
    for (auto &bc : baseCoords) {
        if (bc.vvr.isVariable()) {
            seenVariable = true;
            bc.pair = ivs.addValue(*(g->ctx.locMap), bc.vvr, g->logger);
        }
    }

    if (seenVariable)
        version = VERSION(1, 1);
    else
        version = VERSION(1, 0);

    offset.curr = hdr_size(seenVariable);

    offset.curr += HorizAxis.fill(offset.curr);
    offset.curr += VertAxis.fill(offset.curr);

    offset.shared = offset.curr;  // Indicates start of shared area
    offset.curr += fillSharedData();

    if (seenVariable)
        ivsOffset = offset.curr;

    return 1;
}

void BASE::Axis::write(Offset shared, BASE *h) {
    /* --- Write axis header --- */
    OUT2(baseTagOffset);
    OUT2(baseScriptOffset);

    /* --- Write baseTagList --- */
    OUT2((uint16_t)baseTagList.size());
    for (auto t : baseTagList)
        OUT4(t);

    /* --- Write BaseScriptList --- */
    OUT2((uint16_t)baseScriptList.size());
    for (auto &bsr : baseScriptList) {
        OUT4(bsr.baseScriptTag);
        OUT2((Offset)bsr.baseScriptOffset + shared);
    }
}

void BASE::writeSharedData() {
    auto h = this;
    for (auto &bv : baseValues) {
        OUT2(bv.o);
        OUT2(NULL_OFFSET);  // OUT2(bs->DefaultMinMax);
        OUT2(0);            // OUT2(bs->BaseLangSysCount);
                            // DefaultMinMax_ and BaseLangSysRecord not needed
    }

    for (auto &bv : baseValues) {
        OUT2(bv.DefaultIndex);
        OUT2((uint16_t)bv.BaseCoordOffset.size());
        for (auto &c : bv.BaseCoordOffset)
            OUT2((uint16_t)c);
    }

    for (auto &b : baseCoords)
        b.write(this);
}

void BASEWrite(hotCtx g) {
    BASE *h = g->ctx.BASEp;
    h->Write();
}

void BASE::Write() {
    auto h = this;
    OUT4(version);
    OUT2(HorizAxis.o);
    OUT2(VertAxis.o);
    if (ivsOffset != 0)
        OUT4(ivsOffset);
    if (HorizAxis.baseTagList.size() > 0)
        HorizAxis.write(offset.shared, this);
    if (VertAxis.baseTagList.size() > 0)
        VertAxis.write(offset.shared, this);
    writeSharedData();
    if (ivsOffset != 0)
        ivs.write(g->vw);
}

void BASEReuse(hotCtx g) {
    delete g->ctx.BASEp;
    g->ctx.BASEp = new BASE(g);
}

void BASEFree(hotCtx g) {
    delete g->ctx.BASEp;
    g->ctx.BASEp = nullptr;
}

void BASE::setBaselineTags(bool doVert, std::vector<Tag> &baselineTag) {
#if HOT_DEBUG && 0
    fprintf(stderr, "#BASESetBaselineTags(%d):", doVert);
    for (auto t : baselineTag)
        fprintf(stderr, " %c%c%c%c", TAG_ARG(t));
    fprintf(stderr, "\n");
#endif

    if (baselineTag.size() == 0) {
        g->logger->log(sERROR, "empty baseline tag list");
        return;
    }

    bool lastTag = baselineTag[0];
    for (auto t : baselineTag) {
        if (t < lastTag)
            g->logger->log(sFATAL, "baseline tag list not sorted for %s axis",
                           doVert ? "vertical" : "horizontal");
        lastTag = t;
    }

    if (doVert)
        VertAxis.baseTagList.swap(baselineTag);
    else
        HorizAxis.baseTagList.swap(baselineTag);
}

int32_t BASE::addCoord(VarValueRecord &vvr) {
    /* See if coord can be shared */
    Offset o = 0;
    for (int32_t i = 0; i < (int32_t) baseCoords.size(); i++) {
        if (baseCoords[i] == vvr)
            return i;
    }
    if (!baseCoords.empty()) {
        auto &back = baseCoords.back();
        o = back.o + back.size();
    }

    baseCoords.emplace_back(std::move(vvr), o);
    return baseCoords.size() - 1;
}

int BASE::addBaseScript(int dfltInx, size_t nBaseTags, std::vector<VarValueRecord> &coords) {
    /* See if a baseScript can be shared */
    for (size_t i = 0; i < baseScript.size(); i++) {
        auto &bsi = baseScript[i];
        if (bsi.dfltBaselineInx == dfltInx && nBaseTags == bsi.coordInx.size()) {
            bool match = true;
            for (size_t j = 0; j < nBaseTags; j++) {
                if (!(baseCoords[bsi.coordInx[j]] == coords[j])) {
                    match = false;
                    break;
                }
            }
            if (match)
                return i;
        }
    }

    /* Add a baseScript */

    BaseScriptInfo bsi {(int16_t)dfltInx};
    bsi.coordInx.reserve(nBaseTags);

    for (auto &vvr : coords)
        bsi.coordInx.push_back(addCoord(vvr));

    baseScript.emplace_back(std::move(bsi));

    return baseScript.size() - 1;
}

void BASE::addScript(bool doVert, Tag script, Tag dfltBaseline,
                     std::vector<VarValueRecord> &coords) {
    if (doVert)
        VertAxis.addScript(*this, script, dfltBaseline, coords);
    else
        HorizAxis.addScript(*this, script, dfltBaseline, coords);
}

void BASE::Axis::addScript(BASE &h, Tag script, Tag dfltBaseline, std::vector<VarValueRecord> &coords) {
    size_t nBaseTags = baseTagList.size();

    if (nBaseTags == 0)
        h.g->logger->log(sFATAL, "baseline tags not specified for %s axis", desc);

    /* Calculate dfltInx */
    int dfltInx = -1;
    for (size_t i = 0; i < nBaseTags; i++) {
        if (dfltBaseline == baseTagList[i])
            dfltInx = (int) i;
    }

    if (dfltInx == -1)
        h.g->logger->log(sFATAL, "baseline %c%c%c%c not specified for %s axis",
                         TAG_ARG(dfltBaseline), desc);

    baseScriptList.emplace_back(script, h.addBaseScript(dfltInx, nBaseTags, coords));
}
