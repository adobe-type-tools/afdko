/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Baseline table (default values can be adjusted in the feature file)
 */

#include "BASE.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------- Table Definition --------------------------- */


struct BaseCoordFormat1 {
    BaseCoordFormat1() = delete;
    explicit BaseCoordFormat1(int16_t c) : Coordinate(c) {}
    uint16_t BaseCoordFormat {1};
    int16_t Coordinate;
};
#define BASE_COORD1_SIZE (uint16 * 2)

/*
typedef struct {
    unsigned short BaseCoordFormat;
    short Coordinate;
    GID ReferenceGlyph;
    unsigned short BaseCoordPoint;
} BaseCoordFormat2;

typedef struct {
    unsigned short BaseCoordFormat;
    short Coordinate;
    DCL_OFFSET(void *, DeviceTable);
} BaseCoordFormat3;
*/

struct BaseValues {
    BaseValues() {}
    BaseValues(BaseValues &&o) : DefaultIndex(o.DefaultIndex),
                                 BaseCoord(std::move(o.BaseCoord)) {}
    uint16_t DefaultIndex {0};
    std::vector<int32_t> BaseCoord;  // int32_t instead of Offset for temp -ve value
};
#define BASE_VALUES_SIZE(nCoord) (uint16 * 2 + uint16 * (nCoord))

/*
typedef struct {
    Tag FeatureTableTag;
    DCL_OFFSET(void *, MinCoord);
    DCL_OFFSET(void *, MaxCoord);
} FeatMinMaxRecord;

typedef struct {
    DCL_OFFSET(void *, MinCoord);
    DCL_OFFSET(void *, MaxCoord);
    unsigned short FeatMinMaxCount;
    FeatMinMaxRecord *featMinMaxRecord; // [FeatMinMaxCount]
} MinMax;

typedef struct {
    Tag BaseLangSysTag;
    DCL_OFFSET(MinMax, minMax);
} BaseLangSysRecord;
*/
#define BASE_LANGSYSREC_SIZE (uint32 + uint16)

struct BaseScript {
    BaseScript() = delete;
    explicit BaseScript(Offset bv) : BaseValues(bv) {}
    Offset BaseValues;
/*    DCL_OFFSET(MinMax, DefaultMinMax);
    unsigned short BaseLangSysCount;
    BaseLangSysRecord *baseLangSysRecord; // [BaseLangSysCount]
*/
};
#define BASE_SCRIPT_SIZE(nLangSys) (uint16 * 3 + BASE_LANGSYSREC_SIZE * (nLangSys))

struct BaseScriptRecord {
    BaseScriptRecord(Tag t, int32_t bs) : BaseScriptTag(t), BaseScript(bs) {}
    Tag BaseScriptTag {0};
    int32_t BaseScript {0} ; /* |-> BaseScriptList                        */
                             /* long instead of Offset for temp -ve value */
};

struct Axis {
    void write(Offset shared, BASE *h);
    std::vector<Tag> baseTagList;
    Offset baseTagOffset;
    std::vector<BaseScriptRecord> baseScriptList;
    Offset baseScriptOffset;
    Offset o;
};
#define BASE_SCRIPT_RECORD_SIZE (uint32 + uint16)
#define BASE_SCRIPT_LIST_SIZE(nRec) (uint16 + BASE_SCRIPT_RECORD_SIZE * (nRec))
#define BASE_TAG_LIST_SIZE(nTag) (uint16 + uint32 * (nTag))
#define AXIS_SIZE (uint16 * 2)

typedef struct {
    Fixed version;
    Axis HorizAxis;
    Axis VertAxis;

    /* Shared data */
    std::vector<BaseScript> baseScripts;
    std::vector<BaseValues> baseValues;
    std::vector<BaseCoordFormat1> coord;
} BASETbl;
#define TBL_HDR_SIZE (int32 + uint16 * 2)

/* --------------------------- Context Definition ---------------------------*/

struct BaseScriptInfo {
    explicit BaseScriptInfo(int16_t dbli) : dfltBaselineInx(dbli) {}
    BaseScriptInfo(BaseScriptInfo &&o) : dfltBaselineInx(o.dfltBaselineInx),
                                         coordInx(std::move(o.coordInx)) {}
    int16_t dfltBaselineInx {0};
    std::vector<int16_t> coordInx;
};

struct ScriptInfo {
    bool operator < (const ScriptInfo &rhs) const { return script < rhs.script; }
    Tag script;
    int16_t baseScriptInx;
};

struct AxisInfo {
    AxisInfo() = delete;
    explicit AxisInfo(const char *desc) : desc(desc) {}
    void prep(hotCtx g);
    std::vector<Tag> baseline;
    std::vector<ScriptInfo> script;
    const char *desc;
};

class BASE {
 public:
    BASE() = delete;
    explicit BASE(hotCtx g) : g(g) {}
    Offset fillBaseScriptList(Offset size, bool doVert);
    Offset fillAxis(bool doVert);
    Offset fillSharedData();
    int Fill();
    void writeSharedData();
    void Write();
    void setBaselineTags(bool doVert, std::vector<Tag> &baselineTag);
    int32_t addCoord(int16_t c);
    int _addScript(int dfltInx, size_t nBaseTags, std::vector<int16_t> &coords);
    void addScript(bool doVert, Tag script, Tag dfltBaseline,
                   std::vector<int16_t> &coords);
    AxisInfo horiz {"horizontal"};
    AxisInfo vert {"vertical"};

    /* Shared data */
    std::vector<BaseScriptInfo> baseScript;
    std::vector<int16_t> coord;

    struct {
        Offset curr {0};
        Offset shared {0}; /* Offset to start of shared area */
    } offset;

    BASETbl tbl; /* Table data */
    hotCtx g;    /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void BASENew(hotCtx g) {
    g->ctx.BASEp = new BASE(g);
}

void AxisInfo::prep(hotCtx g) {
    if (baseline.size() == 0)
        return;

    if (script.size() == 0)
        g->logger->log(sFATAL, "scripts not specified for %s baseline axis", desc);

    std::sort(script.begin(), script.end());
}

Offset BASE::fillBaseScriptList(Offset size, bool doVert) {
    AxisInfo &ai = doVert ? vert : horiz;
    auto &bsl = doVert ? tbl.VertAxis.baseScriptList : tbl.HorizAxis.baseScriptList;

    bsl.reserve(ai.script.size());
    for (auto &si : ai.script)
        // Size is adjusted later
        bsl.emplace_back(si.script, BASE_SCRIPT_SIZE(0) * si.baseScriptInx - (offset.curr + size));

    return (Offset)BASE_SCRIPT_LIST_SIZE(bsl.size());
}

Offset BASE::fillAxis(bool doVert) {
    Offset size = AXIS_SIZE;
    AxisInfo &ai = doVert ? vert : horiz;
    Axis &axis = doVert ? tbl.VertAxis : tbl.HorizAxis;
    auto nBaseTags = ai.baseline.size();

    if (nBaseTags == 0)
        return 0;

    /* Fill baseTagList */
    axis.baseTagOffset = size;
    std::copy(ai.baseline.begin(), ai.baseline.end(), std::back_inserter(axis.baseTagList));
    size += BASE_TAG_LIST_SIZE(nBaseTags);

    /* Fill baseScriptList */
    axis.baseScriptOffset = size;
    size += fillBaseScriptList(size, doVert);

    return size;
}

Offset BASE::fillSharedData() {
    auto nBScr = baseScript.size();
    Offset bsSize = 0; /* Accumulator for BaseScript section */
    Offset bvSize = 0; /* Accumulator for BaseValues section */
    Offset bsTotal = (Offset)(nBScr * BASE_SCRIPT_SIZE(0));

    /* --- Fill BaseScript and BaseValues in parallel --- */
    tbl.baseScripts.reserve(nBScr);
    tbl.baseValues.reserve(nBScr);

    for (auto &bsi : baseScript) {
        uint32_t nCoord = bsi.coordInx.size();
        BaseValues bv;

        bv.DefaultIndex = bsi.dfltBaselineInx;
        bv.BaseCoord.reserve(nCoord);
        for (auto c : bsi.coordInx)
            bv.BaseCoord.push_back(BASE_COORD1_SIZE * c - bvSize);

        tbl.baseScripts.emplace_back(bsTotal - bsSize + bvSize);
        tbl.baseValues.emplace_back(std::move(bv));

        bsSize += BASE_SCRIPT_SIZE(0);
        bvSize += BASE_VALUES_SIZE(nCoord);
    }

    /* Adjust BaseValue coord offsets */
    for (auto &bv : tbl.baseValues) {
        for (auto &bc : bv.BaseCoord)
            bc += bvSize;
    }

    tbl.coord.reserve(coord.size());
    for (auto &c : coord)
        tbl.coord.emplace_back(c);

    return bsSize + bvSize + BASE_COORD1_SIZE * tbl.coord.size();
}

int BASEFill(hotCtx g) {
    BASE *h = g->ctx.BASEp;
    return h->Fill();
}

int BASE::Fill() {
    Offset axisSize;

    if (horiz.baseline.size() == 0 && vert.baseline.size() == 0)
        return 0;

    horiz.prep(g);
    vert.prep(g);

    tbl.version = VERSION(1, 0);

    offset.curr = TBL_HDR_SIZE;

    axisSize = fillAxis(false);
    tbl.HorizAxis.o = (axisSize == 0) ? NULL_OFFSET : offset.curr;
    offset.curr += axisSize;

    axisSize = fillAxis(true);
    tbl.VertAxis.o = (axisSize == 0) ? NULL_OFFSET : offset.curr;
    offset.curr += axisSize;

    offset.shared = offset.curr;  // Indicates start of shared area

    offset.curr += fillSharedData();

    return 1;
}

void Axis::write(Offset shared, BASE *h) {
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
        OUT4(bsr.BaseScriptTag);
        OUT2((Offset)bsr.BaseScript + shared);
    }
}

void BASE::writeSharedData() {
    auto h = this;
    for (auto &bs : tbl.baseScripts) {
        OUT2(bs.BaseValues);
        OUT2(NULL_OFFSET);  // OUT2(bs->DefaultMinMax);
        OUT2(0);            // OUT2(bs->BaseLangSysCount);
        /* DefaultMinMax_ and BaseLangSysRecord not needed */
    }

    for (auto &bv : tbl.baseValues) {
        OUT2(bv.DefaultIndex);
        OUT2((uint16_t)bv.BaseCoord.size());
        for (auto &c : bv.BaseCoord)
            OUT2((uint16_t)c);
    }

    for (auto &bcf1 : tbl.coord) {
        OUT2(bcf1.BaseCoordFormat);
        OUT2(bcf1.Coordinate);
    }
}

void BASEWrite(hotCtx g) {
    BASE *h = g->ctx.BASEp;
    h->Write();
}

void BASE::Write() {
    auto h = this;
    OUT4(tbl.version);
    OUT2(tbl.HorizAxis.o);
    OUT2(tbl.VertAxis.o);
    if (horiz.baseline.size() > 0)
        tbl.HorizAxis.write(offset.shared, this);
    if (vert.baseline.size() > 0)
        tbl.VertAxis.write(offset.shared, this);
    writeSharedData();
}

void BASEReuse(hotCtx g) {
    delete g->ctx.BASEp;
    g->ctx.BASEp = new BASE(g);
}

void BASEFree(hotCtx g) {
    delete g->ctx.BASEp;
    g->ctx.BASEp = nullptr;
}

/* ------------------------ Supplementary Functions ------------------------ */

void BASESetBaselineTags(hotCtx g, bool doVert, std::vector<Tag> &baselineTag) {
    g->ctx.BASEp->setBaselineTags(doVert, baselineTag);
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
        vert.baseline.swap(baselineTag);
    else
        horiz.baseline.swap(baselineTag);
}

int32_t BASE::addCoord(int16_t c) {
    /* See if coord can be shared */
    for (int32_t i = 0; i < (int32_t) coord.size(); i++) {
        if (coord[i] == c)
            return i;
    }

    coord.push_back(c);
    return coord.size() - 1;
}

int BASE::_addScript(int dfltInx, size_t nBaseTags, std::vector<int16_t> &coords) {
    /* See if a baseScript can be shared */
    for (size_t i = 0; i < baseScript.size(); i++) {
        auto &bsi = baseScript[i];
        if (bsi.dfltBaselineInx == dfltInx && nBaseTags == bsi.coordInx.size()) {
            bool match = true;
            for (size_t j = 0; j < nBaseTags; j++) {
                if (coord[bsi.coordInx[j]] != coords[j]) {
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

    for (auto &c : coords)
        bsi.coordInx.push_back(addCoord(c));

    baseScript.emplace_back(std::move(bsi));

    return baseScript.size() - 1;
}

void BASEAddScript(hotCtx g, bool doVert, Tag script, Tag dfltBaseline,
                   std::vector<int16_t> &coords) {
    g->ctx.BASEp->addScript(doVert, script, dfltBaseline, coords);
}
        

void BASE::addScript(bool doVert, Tag script, Tag dfltBaseline,
                     std::vector<int16_t> &coord) {
    AxisInfo &ai = doVert ? vert : horiz;
    ScriptInfo si;
    size_t nBaseTags = ai.baseline.size();

    if (nBaseTags == 0) {
        g->logger->log(sFATAL, "baseline tags not specified for %s axis",
                       doVert ? "vertical" : "horizontal");
    }

    /* Calculate dfltInx */
    int dfltInx = -1;
    for (size_t i = 0; i < nBaseTags; i++) {
        if (dfltBaseline == ai.baseline[i]) {
            dfltInx = (int) i;
        }
    }
    if (dfltInx == -1) {
        g->logger->log(sFATAL, "baseline %c%c%c%c not specified for %s axis",
                       TAG_ARG(dfltBaseline), doVert ? "vertical" : "horizontal");
    }

    si.script = script;
    si.baseScriptInx = _addScript(dfltInx, nBaseTags, coord);
    ai.script.push_back(si);
}
