/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Glyph positioning table.
 */

#include "GPOS.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hotmap.h"
#include "name.h"

#define REPORT_DUPE_KERN 1

const std::array<int, 4> GPOS::MetricsRec::kernorder = {2, 3, 1, 0};

#if HOT_DEBUG

void GPOS::SingleRec::dump(GPOS &h) {
    featGlyphDump(h.g, gid, -1, 1);
    fprintf(stderr, "\t%4d %4d %4d %4d",
            metricsRec.value[0], metricsRec.value[1],
            metricsRec.value[2], metricsRec.value[3]);
    fprintf(stderr, " %5x ", valFmt);
    if (span.valFmt == 0) {
        fprintf(stderr, "       *");
    } else {
        fprintf(stderr, "   %5d", span.valFmt);
    }
    if (span.valRec == 0) {
        fprintf(stderr, "      *");
    } else {
        fprintf(stderr, "  %5d", span.valRec);
    }
    fprintf(stderr, "\n");
}

void GPOS::dumpSingles(std::vector<SingleRec> &singles) {
    fprintf(stderr,
            ">GPOS: dumpSingles [%ld]            valFmt span.valFmt  span.valRec\n",
            singles.size());
    for (uint32_t i = 0; i < singles.size(); i++) {
        fprintf(stderr, "[%2d] ", i);
        singles[i].dump(*this);
    }
}

void GPOS::KernRec::dump(hotCtx g, GNode *gclass1, GNode *gclass2) {
    if (gclass1 == NULL) {
        featGlyphDump(g, first, ' ', 1);
        featGlyphDump(g, second, ' ', 1);
    } else {
        featGlyphClassDump(g, gclass1, -1, 1);
        fprintf(stderr, ":%u  ", first);
        featGlyphClassDump(g, gclass2, -1, 1);
        fprintf(stderr, ":%u  ", second);
    }
    fprintf(stderr, "<");
    for (int i = 0; i < 4; i++) {
        fprintf(stderr, "%hd ", metricsRec1.value[i]);
    }
    fprintf(stderr, "> <\n");
    for (int i = 0; i < 4; i++) {
        fprintf(stderr, "%hd ", metricsRec2.value[i]);
    }
    fprintf(stderr, ">\n");
}

#endif

/* --------------------------- Standard Functions -------------------------- */

void GPOSNew(hotCtx g) {
    g->ctx.GPOSp =  new GPOS(g);
}

int GPOSFill(hotCtx g) {
    return g->ctx.GPOSp->Fill();
}

int GPOS::Fill() {
    return fillOTL();
}

void GPOSWrite(hotCtx g) {
    g->ctx.GPOSp->Write();
}

void GPOS::Write() {
    writeOTL();
}

void GPOSReuse(hotCtx g) {
    delete g->ctx.GPOSp;
    g->ctx.GPOSp = new GPOS(g);
}

void GPOSFree(hotCtx g) {
    delete g->ctx.GPOSp;
    g->ctx.GPOSp = nullptr;
}

/* ------------------------ Supplementary Functions ------------------------ */

void GPOS::FeatureBegin(Tag script, Tag language, Tag feature) {
    DF(2, (stderr, "\n"));
#if 1
    DF(1, (stderr, "{ GPOS '%c%c%c%c', '%c%c%c%c', '%c%c%c%c'\n",
           TAG_ARG(script), TAG_ARG(language), TAG_ARG(feature)));
#endif

    nw.script = script;
    nw.language = language;
    nw.feature = feature;
}

void GPOS::reuseClassDefs() {
    classDef[0].reset(g);
    classDef[1].reset(g);
}

GPOS::Subtable::Subtable(GPOS &h, SubtableInfo &si) :
    OTL::Subtable::Subtable(&h, &si, h.g->error_id_text, si.lkpType == GPOSFeatureParam) {}

void GPOS::AddSubtable(std::unique_ptr<OTL::Subtable> s) {
    reuseClassDefs();
    OTL::AddSubtable(std::move(s));
}

#if HOT_DEBUG

void GPOS::rulesDump(SubtableInfo &si) {
    fprintf(stderr, "# Dump lookupType %d rules:\n", si.lkpType);
    for (uint32_t i = 0; i < si.rules.size(); i++) {
        PosRule &rule = si.rules[i];

        fprintf(stderr, "  [%d] ", i);
        featPatternDump(g, rule.targ, ' ', 1);
    }
}

#endif

void GPOS::LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                       bool useExtension, uint16_t useMarkSetIndex) {
    DF(2, (stderr,
           " { GPOS lkpType=%s%d lkpFlag=%d label=%x\n",
           useExtension ? "EXTENSION:" : "", lkpType, lkpFlag, label));

    nw.reset(lkpType, lkpFlag, label, useExtension, useMarkSetIndex);
}

void GPOS::LookupEnd(SubtableInfo *si) {
    DF(2, (stderr, " } GPOS\n"));

    if (si == nullptr)
        si = &nw;

    if (IS_REF_LAB(si->label)) {
        AddSubtable(std::move(std::make_unique<Subtable>(*this, *si)));
        return;
    }

    if (g->hadError)
        return;

    switch (si->lkpType) {
        case GPOSFeatureParam:
            FeatureParameters::fill(*this, *si);
            break;

        case GPOSSingle:
            SinglePos::fill(*this, *si);
            break;

        case GPOSChain:
            ChainContextPos::fill(*this, *si);
            break;

        case GPOSCursive:
            CursivePos::fill(*this, *si);
            break;

        case GPOSMarkToBase:
            MarkBasePos::fill(*this, *si);
            break;

        case GPOSMarkToLigature:
            MarkLigaturePos::fill(*this, *si);
            break;

        case GPOSMarkToMark:
            MarkBasePos::fill(*this, *si);
            break;

        case GPOSPair:
            PairPos::fill(*this, *si);
            break;

        default:
            // Can't get here, but it is a useful check for future development.
            hotMsg(g, hotFATAL, "unknown GPOS lkpType <%d> in %s.",
                   si->lkpType, g->error_id_text.c_str());
    }

    checkOverflow("lookup subtable", subOffset(), "positioning");
    startNewPairPosSubtbl = false;
}

/* Performs no action but brackets feature calls */
void GPOS::FeatureEnd() {
    DF(2, (stderr, "} GPOS\n"));
}

/* -------------------------- Shared Declarations -------------------------- */

/* --- ValueFormat and ValueRecord utility functions --- */

static bool isVertFeature(Tag featTag) {
    return (featTag == vkrn_) || (featTag == vpal_) ||
           (featTag == vhal_) || (featTag == valt_);
}

uint32_t GPOS::makeValueFormat(SubtableInfo &si, int32_t xPla, int32_t yPla,
                               int32_t xAdv, int32_t yAdv) {
    uint32_t val = 0;

    if (xPla) {
        val |= ValueXPlacement;
    }
    if (yPla) {
        val |= ValueYPlacement;
    }
    if (yAdv) {
        val |= ValueYAdvance;
    }

    if (xAdv) {
        if ((val == 0) && (isVertFeature(si.feature)))
            val = ValueYAdvance;
        else
            val |= ValueXAdvance;
    }

    return val;
}

uint32_t GPOS::recordValues(uint32_t valFmt, MetricsRec &r) {
    uint32_t t = values.size(), ret = VAL_REC_UNDEF;
    if (valFmt & ValueXPlacement) {
        ret = t;
        values.push_back(r.value[0]);
    }
    if (valFmt & ValueYPlacement) {
        ret = t;
        values.push_back(r.value[1]);
    }
    if (valFmt & ValueXAdvance) {
        ret = t;
        values.push_back(r.value[2]);
    }
    if (valFmt & ValueYAdvance) {
        ret = t;
        values.push_back(r.value[3]);
    }
    return ret;
}

/* Counts bits set in valFmt */

static int numValues(uint32_t valFmt) {
    int i = 0;
    while (valFmt) {
        i++;
        valFmt &= valFmt - 1; /* Remove least significant set bit */
    }
    return i;
}

void GPOS::writeValueRecord(uint32_t valFmt, ValueRecord i) {
    GPOS *h = this;  // XXX for OUT2
    while (valFmt) {
        /* Write 1 field per valFmt bit, if index is valid */
        OUT2((int16_t)((i == VAL_REC_UNDEF) ? 0 : values[i++]));
        valFmt &= valFmt - 1; /* Remove least significant set bit */
    }
}

/* The input vector has either 2 or 4 elements ( 2 if the second is 0, else 4),
 * and the output array has 5 elements. The first 2 are in the same order
 * as the feature file parameters. The last two are in the same order, but one
 * index value less, as the input record does not contain the ParamMenuNameID.
 * The ParamMenuNameID element of the output record
 * is filled in only when fillSizeFeature() is called.
 */
void GPOS::AddParameters(const std::vector<uint16_t> &params) {
    int l = params.size();
    if (l < 2) {
        hotMsg(g, hotFATAL, "'size' feature must have at least two parameters! In %s.", g->error_id_text.c_str());
    } else if (params[ParamSubFamilyID] != 0 && l != 4) {
        hotMsg(g, hotFATAL, "'size' feature must have 4 parameters if sub family ID code is non-zero! In %s.", g->error_id_text.c_str());
    } else if (params[ParamSubFamilyID] == 0 && l != 4 && l != 2) {
        hotMsg(g, hotFATAL, "'size' feature must have 4 or 2 parameters if sub family code is zero! In %s.", g->error_id_text.c_str());
    }

    nw.params[ParamOpticalSize] = params[ParamOpticalSize];  /* decipoint size, in decipoints */
    nw.params[ParamSubFamilyID] = params[ParamSubFamilyID];  /* subfamily code. If zero, rest must be zero. */
    if (params[ParamSubFamilyID] != 0) {
        nw.params[ParamMenuNameID] = 0xFFFF;  /* put a bad value in for the name ID value so we'll know */
                                               /* if it doesn't get overwritten.                         */
        nw.params[ParamLowEndRange] = params[ParamLowEndRange - 1];
        nw.params[ParamHighEndRange] = params[ParamHighEndRange - 1];
    }
}

GPOS::FeatureParameters::FeatureParameters(GPOS &h, SubtableInfo &si,
                                           std::array<uint16_t, 5> &p) :
                                           Subtable(h, si), params(p) {}

void GPOS::FeatureParameters::fill(GPOS &h, SubtableInfo &si) {
    /* if the kSizeSubFamilyID field is non-zero, then we need to fill in the */
    /* kSizeMenuNameID field in the parameters array. This value may be zero; */
    /* if so, then there is no special sub family menu name.                  */
    auto &p = si.params;

    if ((si.feature == size_) && (p[ParamSubFamilyID] != 0)) {
        uint16_t nameid = h.featNameID;

        p[ParamMenuNameID] = nameid;

        /* If there is a sub family menu name id,  */
        /* check if the default names are present, */
        /* and complain if they are not.           */
        if (nameid != 0) {
            uint16_t nameIDPresent = nameVerifyDefaultNames(h.g, nameid);
            if (nameIDPresent && nameIDPresent & MISSING_WIN_DEFAULT_NAME) {
                hotMsg(h.g, hotFATAL, "Missing Windows default name for 'sizemenuname' nameid %i in 'size' feature.", nameid);
            }
        }
    } else {
        p[ParamMenuNameID] = 0;
    }

    std::unique_ptr<GPOS::Subtable> s = std::make_unique<FeatureParameters>(h, si, p);
    h.incFeatParamOffset(sizeof(uint16_t) * 5);
    h.AddSubtable(std::move(s));
}

void GPOS::FeatureParameters::write(OTL *h) {
    for (auto p : params)
        OUT2(p);
}

/* --------------------------- Single Adjustment --------------------------- */

GPOS::SinglePos::SinglePos(GPOS &h, SubtableInfo &si) : Subtable(h, si) {}

LOffset GPOS::SinglePos::pos1Size(SubtableInfo &si, int iStart, int iEnd) {
    return single1Size(numValues(si.singles[iStart].valFmt));
}

LOffset GPOS::SinglePos::pos2Size(SubtableInfo &si, int iStart, int iEnd) {
    return single2Size(iEnd - iStart, numValues(si.singles[iStart].valFmt));
}

LOffset GPOS::SinglePos::allPos2Size(SubtableInfo &si, int &nsub) {
    LOffset r = 0;
    uint32_t nextSpanValFmt;

    nsub = 0;
    for (uint32_t iFmt = 0; iFmt < si.singles.size(); iFmt = nextSpanValFmt) {
        nextSpanValFmt = si.singles[iFmt].span.valFmt;
        nsub++;
        r += pos2Size(si, iFmt, nextSpanValFmt);
    }
    return r;
}

LOffset GPOS::SinglePos::allPos1Size(SubtableInfo &si, int &nsub) {
    LOffset r = 0;

    nsub = 0;
    for (uint32_t iFmt = 0; iFmt < si.singles.size(); iFmt = si.singles[iFmt].span.valFmt) {
        for (int iRec = iFmt; iRec < si.singles[iFmt].span.valFmt; iRec = si.singles[iRec].span.valRec) {
            nsub++;
            r += pos1Size(si, iRec, si.singles[iRec].span.valRec);
        }
    }
    return r;
}

void GPOS::SetSizeMenuNameID(uint16_t nameID) {
    featNameID = nameID;
}

/* Compare valfmt, then gid */

bool GPOS::SingleRec::cmp(const GPOS::SingleRec &a, const GPOS::SingleRec &b) {
    if (!(a.metricsRec == b.metricsRec))
        return a.metricsRec < b.metricsRec;
    else
        return a.gid < b.gid;
}

/* Compare gid, then valfmt */

bool GPOS::SingleRec::cmpGID(const GPOS::SingleRec &a, const GPOS::SingleRec &b) {
    if (a.gid != b.gid)
        return a.gid < b.gid;
    else
        return a.metricsRec < b.metricsRec;
}

/* Targ may be a class, as a convenience. Input GNodes are recycled. */

void GPOS::AddSingle(SubtableInfo &si, GNode *targ, int xPla, int yPla, int xAdv, int yAdv) {
    GNode *p;
    uint32_t valFmt;

    valFmt = makeValueFormat(si, xPla, yPla, xAdv, yAdv);

    if (g->hadError)
        return;

    for (p = targ; p != NULL; p = p->nextCl) {
        SingleRec single;
        if (p->flags & FEAT_MISC)
            continue; /* skip rules that are duplicates of others that are in this lookup. This is set in addAnonPosRule */
        single.gid = p->gid;
        single.valFmt = valFmt;
        if (isVertFeature(si.feature) && (valFmt == ValueYAdvance) && (yAdv == 0))
            single.metricsRec.set(xPla, yPla, 0, xAdv);
        else
            single.metricsRec.set(xPla, yPla, xAdv, yAdv);

        auto [i, b] = si.glyphsSeen.insert({std::pair<GID, GID>{single.gid, GID_UNDEF},
                                           (uint32_t) si.singles.size()});
        if (!b) {
            featGlyphDump(g, single.gid, '\0', 0);
            if (!SingleRec::cmp(single, si.singles[i->second]) &&
                !SingleRec::cmp(si.singles[i->second], single)) {
                hotMsg(g, hotNOTE,
                       "Removing duplicate single positioning "
                       "in %s: %s",
                       g->error_id_text.c_str(),
                       g->note.array);
            } else {
                hotMsg(g, hotFATAL,
                       "Duplicate single positioning glyph with "
                       "different values in %s: %s",
                       g->error_id_text.c_str(), g->note.array);
            }
        } else {
#if HOT_DEBUG
            if (DF_LEVEL >= 2) {
                fprintf(stderr, "  * GPOSSingle ");
                featGlyphDump(g, p->gid, ' ', 1);
                fprintf(stderr, " %d %d %d %d\n", xPla, yPla, xAdv, yAdv);
            }
#endif
            si.singles.emplace_back(std::move(single));
        }
    }
}

void GPOS::prepSinglePos(SubtableInfo &si) {
    /* Now sort by valfmt (then gid) */
    std::sort(si.singles.begin(), si.singles.end(), SingleRec::cmp);

    /* Calculate value format and value record spans */
    SingleRec sn;
    sn.valFmt = -1;
    si.singles.emplace_back(std::move(sn));
    uint32_t pfmt = 0, fmt;
    for (fmt = 1; fmt < si.singles.size(); fmt++) {
        if (si.singles[fmt].valFmt != si.singles[pfmt].valFmt) {
            /* Value format change. Now calc valRec spans. */
            int Rec = pfmt;

            for (uint32_t rec = Rec + 1; rec <= fmt; rec++) {
                SingleRec &r = si.singles[rec];
                SingleRec &R = si.singles[Rec];

                if (rec == fmt || !(r.metricsRec == R.metricsRec)) {
                    /* val rec change */
                    R.span.valRec = rec;
                    Rec = rec;
                }
            }
            si.singles[pfmt].span.valFmt = fmt;
            pfmt = fmt;
        }
    }
    si.singles[pfmt].span.valFmt = fmt;
    si.singles.pop_back();
}

GPOS::SinglePos::Format1::Format1(GPOS &h, GPOS::SubtableInfo &si,
                                  int iStart, int iEnd) : SinglePos(h, si) {
    auto &s = si.singles[iStart];
    ValueFormat = s.valFmt;
    LOffset size = single1Size(numValues(ValueFormat));

    cac->coverageBegin();

    for (int i = iStart; i < iEnd; i++)
        cac->coverageAddGlyph(si.singles[i].gid);

    Value = h.recordValues(ValueFormat, s.metricsRec);

    Coverage = cac->coverageEnd(); /* Adjusted later */

    if (isExt()) {
        Coverage += size; /* Final value */
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }
}

GPOS::SinglePos::Format2::Format2(GPOS &h, GPOS::SubtableInfo &si,
                                  int iStart, int iEnd) : SinglePos(h, si) {
    ValueFormat = si.singles[iStart].valFmt;
    LOffset size = pos2Size(si, iStart, iEnd);

    /* Sort subrange by GID */
    std::sort(si.singles.begin() + iStart, si.singles.begin() + iEnd, SingleRec::cmpGID);
    cac->coverageBegin();
    Values.reserve(iEnd - iStart);

    for (int i = iStart; i < iEnd; i++) {
        auto &s = si.singles[i];
        cac->coverageAddGlyph(s.gid);
        Values.push_back(h.recordValues(s.valFmt, s.metricsRec));
    }

    Coverage = cac->coverageEnd(); /* Adjusted later */
    if (useExtension) {
        Coverage += size; /* Final value */
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }
}

void GPOS::SinglePos::Format1::fill(GPOS &h, GPOS::SubtableInfo &si) {
    GPOS::SingleRec *sFmt;
    for (uint32_t iFmt = 0; iFmt < si.singles.size(); iFmt = sFmt->span.valFmt) {
        GPOS::SingleRec *sRec;
        sFmt = &si.singles[iFmt];
        for (int iRec = iFmt; iRec < sFmt->span.valFmt; iRec = sRec->span.valRec) {
            sRec = &si.singles[iRec];
            h.AddSubtable(std::move(std::make_unique<Format1>(h, si, iRec, sRec->span.valRec)));
        }
    }
}

void GPOS::SinglePos::Format2::fill(GPOS &h, GPOS::SubtableInfo &si) {
    uint32_t nextSpanValFmt;

    for (uint32_t iFmt = 0; iFmt < si.singles.size(); iFmt = nextSpanValFmt) {
        nextSpanValFmt = si.singles[iFmt].span.valFmt;
        h.AddSubtable(std::move(std::make_unique<Format2>(h, si, iFmt, nextSpanValFmt)));
    }
}

/* Fill single postioning subtable(s). */

void GPOS::SinglePos::fill(GPOS &h, SubtableInfo &si) {
    LOffset size1;
    LOffset size2;
    int nSub1;
    int nSub2;

    if (si.singles.size() == 0) {
        return;
    }

    h.prepSinglePos(si);

#if HOT_DEBUG
    {
        auto g = h.g;
        if (DF_LEVEL >= 2) {
            h.dumpSingles(si.singles);
        }
    }
#endif

    size1 = allPos1Size(si, nSub1);
    size2 = allPos2Size(si, nSub2);

    {
#if HOT_DEBUG
        auto g = h.g;
#endif
        DF(2, (stderr, "### singlePos1 size=%u (%d subtables)\n", size1, nSub1));
        DF(2, (stderr, "### singlePos2 size=%u (%d subtables)\n", size2, nSub2));
    }

    /* Select subtable format */
    if (size1 < size2) {
        Format1::fill(h, si);
    } else {
        Format2::fill(h, si);
    }

    h.updateMaxContext(1);
}

void GPOS::SinglePos::Format1::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "single positioning");

    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2(ValueFormat);
    h->writeValueRecord(ValueFormat, Value);

    if (isExt())
        cac->coverageWrite();
}

void GPOS::SinglePos::Format2::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "single positioning");

    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2(ValueFormat);
    OUT2((uint16_t)Values.size());
    for (auto v : Values)
        h->writeValueRecord(ValueFormat, v);

    if (isExt())
        cac->coverageWrite();
}

/* ---------------------------- Pair Adjustment ---------------------------- */

/* Break the subtable at this point. Return 0 if successful, else 1. */

bool GPOS::SubtableBreak() {
    if (nw.lkpType != GPOSPair)
        /* xxx for now */
        return false;

    startNewPairPosSubtbl = true;
    return true;
}

/* Checks to see if gc can be a valid member of classDef.
   Returns inx, which is -1 if gc cannot be a valid member of the classDef.
   If  > -1, then class is set to gc's class value. insert is set to 1 if
   gc needs to be inserted by a subsequent call to insertInClassDef(). */

bool GPOS::validInClassDef(int classDefInx, GNode *gc, uint32_t &cls, bool &insert) {
    ClassDef &cdef = classDef[classDefInx];

    GNode *p;
    auto i = cdef.classInfo.find(gc->gid);
    if (i != cdef.classInfo.end()) {
        /* First glyph matches. Check to see if rest match. */
        GNode *q = i->second.gc->nextCl;
        for (p = gc->nextCl; p != NULL && q != NULL; p = p->nextCl, q = q->nextCl) {
            if (p->gid != q->gid)
                return false;
        }
        if (p != NULL || q != NULL) {
            return false;
        }
        insert = false;
        cls = i->second.cls; /* gc already exists */
    } else {
        /* First glyph does not match.                                */
        /* Check to see if any members of gc are already in coverage. */
        for (p = gc; p != NULL; p = p->nextCl) {
            if (cdef.cov.find(p->gid) != cdef.cov.end())
                return false;
        }
        /* Classes start numbering from 0 for ClassDef1, 1 for ClassDef2 */
        insert = true;
        cls = cdef.classInfo.size();
        if (classDefInx != 0)
            cls++;
    }
    return true;
}

/* Inserts gc into the relevant class def. Input GNodes stored. */

void GPOS::insertInClassDef(ClassDef &cdef, GNode *gc, uint32_t cls) {
    ClassInfo ci {cls, gc};
    cdef.classInfo.insert({gc->gid, ci});

    /* Add gids to glyph accumulator */
    for (; gc != NULL; gc = gc->nextCl)
        cdef.cov.insert(gc->gid);
}

void GPOS::addSpecPair(SubtableInfo &si, GID first, GID second,
                       MetricsRec &metricsRec1, MetricsRec &metricsRec2) {
    KernRec pair;

    pair.first = first;
    pair.second = second;
    /* metricsCnt is either 1 or 4 */
    pair.metricsRec1 = metricsRec1;
    pair.metricsRec2 = metricsRec2;
#if HOT_DEBUG
    if (DF_LEVEL >= 2) {
        fprintf(stderr, "  * GPOSPair ");
        pair.dump(g, NULL, NULL);
    }
#endif
    auto [i, b] = si.glyphsSeen.insert({std::pair<GID, GID>(first, second),
                                        (uint32_t)si.pairs.size()});
    if (!b) {
        {}
#if REPORT_DUPE_KERN
        KernRec &prev = si.pairs[i->second];
        printKernPair(first, second, pair.metricsRec1.value[0], pair.metricsRec1.value[0], true);
        if (pair.metricsRec1 == prev.metricsRec1 && pair.metricsRec2 == prev.metricsRec2) {
            hotMsg(g, hotNOTE,
                   "Removing duplicate pair positioning in %s: %s",
                   g->error_id_text.c_str(),
                   g->note.array);
        } else {
            hotMsg(g, hotWARNING,
                   "Pair positioning has conflicting statements in "
                   "%s; choosing the first "
                   "value: %s",
                   g->error_id_text.c_str(), g->note.array);
        }
#endif
    } else {
        si.pairs.emplace_back(std::move(pair));
    }
}

/* Add pair. first and second must already be sorted with duplicate gids
   removed. Input GNodes are stored; they are recycled in this function
   (if fmt1), or at GPOSLookupEnd(?) (if fmt2). */

void GPOS::AddPair(SubtableInfo &si, GNode *first, GNode *second, std::string &locDesc) {
    bool enumerate;
    int8_t pairFmt;
    uint32_t valFmt1 = 0;
    uint32_t valFmt2 = 0;
    int16_t *values1 = NULL;
    int16_t *values2 = NULL;
    MetricsRec metricsRec1;
    MetricsRec metricsRec2;

    if (first->metricsInfo.cnt == -1) {
        /* If the only metrics record is applied to the second glyph, then */
        /* this is shorthand for applying a single kern value to the first  */
        /* glyph. The parser enforces that if first->metricsInfo == null,   */
        /* then the second value record must exist.                         */
        first->metricsInfo = second->metricsInfo;
        second->metricsInfo = METRICSINFOEMPTYPP;
        values1 = first->metricsInfo.metrics;
    } else {
        /* first->metricsInfo exists, but second->metricsInfo may or may not exist */
        values1 = first->metricsInfo.metrics;
        if (second->metricsInfo.cnt != -1) {
            values2 = second->metricsInfo.metrics;
        }
    }

    enumerate = first->flags & FEAT_ENUMERATE;

    /* The FEAT_GCLASS is essential for identifying a singleton gclass */
    pairFmt = ((first->flags & FEAT_GCLASS || second->flags & FEAT_GCLASS) && !enumerate) ? 2 : 1;

    if (first->metricsInfo.cnt == 1) {
        if (isVertFeature(si.feature)) {
            valFmt1 = (uint16_t) ValueYAdvance;
            metricsRec1.value[3] = values1[0];
        } else {
            valFmt1 = (uint16_t) ValueXAdvance;
            metricsRec1.value[2] = values1[0];
        }
    } else {
        assert(first->metricsInfo.cnt == 4);
        metricsRec1.set(values1[0], values1[1], values1[2], values1[3]);
        valFmt1 = makeValueFormat(si, values1[0], values1[1], values1[2], values1[3]);
        if (valFmt1 == 0) {
            /* If someone has specified a value of <0 0 0 0>, then valFmt   */
            /* is 0. This will cause a subtable break, since the valFmt     */
            /* will differ from any non-zero record. Set the val fmt to the */
            /* default value.                                               */
            if (isVertFeature(si.feature)) {
                valFmt1 = (uint16_t) ValueYAdvance;
            } else {
                valFmt1 = (uint16_t) ValueXAdvance;
            }
        }
    }

    if (second->metricsInfo.cnt != -1) {
        if (second->metricsInfo.cnt == 1) {
            if (isVertFeature(si.feature)) {
                valFmt2 = (uint16_t) ValueYAdvance;
                metricsRec2.value[3] = values2[0];
            } else {
                valFmt2 = (uint16_t) ValueXAdvance;
                metricsRec2.value[2] = values2[0];
            }
        } else {
            assert(second->metricsInfo.cnt == 4);
            metricsRec2.set(values2[0], values2[1], values2[2], values2[3]);
            valFmt2 = makeValueFormat(si, values2[0], values2[1], values2[2], values2[3]);
            if (valFmt2 == 0) {
                /* If someone has specified a value of <0 0 0 0>, then      */
                /* valFmt is 0. This will cause a subtable break, since the */
                /* valFmt will differ from any non-zero record. Set the val  */
                /* fmt to the default value.                                */
                if (isVertFeature(si.feature)) {
                    valFmt2 = (uint16_t) ValueYAdvance;
                } else {
                    valFmt2 = (uint16_t) ValueXAdvance;
                }
            }
        }
    }
    /* Changing valFmt causes a sub-table break. If the current valFmt  */
    /* can be represented by the previous valFmt, then use the previous */
    /* valFmt.                                                          */
    if ((valFmt1 != si.pairValFmt1) && ((valFmt1 & si.pairValFmt1) == valFmt1)) {
        valFmt1 = si.pairValFmt1;
    }
    if ((valFmt2 != si.pairValFmt2) && ((valFmt2 & si.pairValFmt2) == valFmt2)) {
        valFmt2 = si.pairValFmt2;
    }

    first->nextSeq = NULL;  /* else featRecycleNodes  recycles the second mode as well. */
    second->nextSeq = NULL; /* else featRecycleNodes  recycles the second mode as well. */

    if (g->hadError)
        return;

    if (pairFmt != si.pairFmt /* First subtable in this lookup */
        || valFmt1 != si.pairValFmt1 || valFmt2 != si.pairValFmt2 || startNewPairPosSubtbl /* Automatic or explicit break */) {
        startNewPairPosSubtbl = false;

        if (si.pairFmt != 0) {
            /* Pause to create accumulated subtable */
            PairPos::fill(*this, si);
            if (si.pairFmt == 2 && pairFmt == 1) {
                featGlyphDump(g, first->gid, ' ', 0);
                featGlyphDump(g, second->gid, '\0', 0);
                hotMsg(g, hotWARNING,
                       "Single kern pair occurring after "
                       "class kern pair in %s: %s",
                       g->error_id_text.c_str(), g->note.array);
            }
        }
        si.glyphsSeen.clear();
        si.pairs.clear();
        si.pairFmt = pairFmt;
        si.pairValFmt1 = valFmt1;
        si.pairValFmt2 = valFmt2;
    }

    /* Check if value record format needs updating */
    if (pairFmt == 1) {
        /* --- Add specific pair(s) --- */
        first->nextSeq = second;
        if (first->nextCl == NULL && second->nextCl == NULL) {
            addSpecPair(si, first->gid, second->gid, metricsRec1, metricsRec2);
        } else {
            /* Enumerate */
            unsigned length;
            GNode **prod;

            prod = featMakeCrossProduct(g, first, &length);
            for (uint32_t i = 0; i < length; i++) {
                GNode *specPair = prod[i];
                addSpecPair(si, specPair->gid, specPair->nextSeq->gid, metricsRec1, metricsRec2);
                featRecycleNodes(g, specPair);
            }
        }
        featRecycleNodes(g, first);
    } else {
        /* --- Add class pair --- */
        uint32_t cl1;
        uint32_t cl2;
        bool insert1;
        bool insert2;
        KernRec pair;

        if ((validInClassDef(0, first, cl1, insert1)) &&
            (validInClassDef(1, second, cl2, insert2))) {
            /* Add pair to current kern pair accumulator */
            pair.first = cl1; /* Store classes */
            pair.second = cl2;
            pair.metricsRec1 = metricsRec1;
            pair.metricsRec2 = metricsRec2;
#if HOT_DEBUG
            if (DF_LEVEL >= 2) {
                fprintf(stderr, "  * GPOSPair ");
                pair.dump(g, first, second);
            }
#endif
            auto [i, b] = si.glyphsSeen.insert({std::pair<GID, GID>{cl1, cl2},
                                               (uint32_t)si.pairs.size()});
            if (!b) {
                {}
#if REPORT_DUPE_KERN
                KernRec &prev = si.pairs[i->second];
                printKernPair(cl1, cl2, pair.metricsRec1.value[0], pair.metricsRec1.value[0], false);
                if (pair.metricsRec1 == prev.metricsRec1 && pair.metricsRec2 == prev.metricsRec2) {
                    hotMsg(g, hotNOTE,
                           "Removing duplicate pair positioning in %s: %s",
                           g->error_id_text.c_str(),
                           g->note.array);
                } else {
                    hotMsg(g, hotWARNING,
                           "Pair positioning has conflicting statements in "
                           "%s; choosing the first "
                           "value: %s",
                           g->error_id_text.c_str(), g->note.array);
                }
#endif
            } else {
                /* Insert into the classDefs, if needed */
                if (insert1) {
                    insertInClassDef(classDef[0], first, cl1);
                } else {
                    featRecycleNodes(g, first);
                }
                if (insert2) {
                    insertInClassDef(classDef[1], second, cl2);
                } else {
                    featRecycleNodes(g, second);
                }
                si.pairs.emplace_back(std::move(pair));
            }
        } else {
            featGlyphClassDump(g, first, ' ', 0);
            featGlyphClassDump(g, second, '\0', 0);
            hotMsg(g, hotWARNING,
                   "Start of new pair positioning subtable forced by overlapping glyph classes in %s; "
                   "some pairs may never be accessed: %s",
                   g->error_id_text.c_str(),
                   g->note.array);

            startNewPairPosSubtbl = true;
            AddPair(si, first, second, locDesc);
        }
    }
}

void GPOS::addPosRule(SubtableInfo &si, GNode *targ, std::string &locDesc,
                      std::vector<AnchorMarkInfo> &anchorMarkInfo) {
    uint16_t lkpType;

    if (si.parentLkpType == 0)
        lkpType = si.lkpType;
    else
        lkpType = si.parentLkpType;

#if HOT_DEBUG
    if (DF_LEVEL >= 2) {
        DF(2, (stderr, "  * GPOS RuleAdd "));
        featPatternDump(g, targ, ' ', 1);
        DF(2, (stderr, "\n"));
    }
#endif

    if (lkpType == GPOSSingle) {
        GNode *nextNode = targ;
        if (targ->flags & FEAT_HAS_MARKED) {
            if (featValidateGPOSChain(g, targ, lkpType) == 0) {
                return;
            }

            /* for each glyph node in the input sequence, */
            /* add the pos rule to the anonymous table,   */
            /* if there is one                            */
            while (nextNode != NULL) {
                if (!(nextNode->flags & FEAT_MARKED)) {
                    nextNode = nextNode->nextSeq;
                    continue;
                }
                if (nextNode->metricsInfo.cnt == -1) {
                    nextNode = nextNode->nextSeq;
                    continue;
                }

                SubtableInfo &anon_si = addAnonPosRule(si, lkpType, nextNode);
                if (nextNode->metricsInfo.cnt == 1) {
                    /* assume it is an xAdvance adjustment */
                    AddSingle(anon_si, nextNode, 0, 0, nextNode->metricsInfo.metrics[0], 0);
                } else {
                    assert(nextNode->metricsInfo.cnt == 4);
                    int16_t *metrics = nextNode->metricsInfo.metrics;

                    AddSingle(anon_si, nextNode, metrics[0], metrics[1],
                              metrics[2], metrics[3]);
                }

                if (nextNode->lookupLabelCount > 255)
                    hotMsg(g, hotFATAL, "Anonymous lookup in chain caused overflow.");

                nextNode->lookupLabels[nextNode->lookupLabelCount] = anon_si.label;
                nextNode->lookupLabelCount++;

                nextNode = nextNode->nextSeq;
            }
            /* now add the nodes to the contextual rule list. */
            si.parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */
            si.rules.emplace_back(targ);
        } else {
            if (targ->metricsInfo.cnt == 1) {
                /* assume it is an xAdvance adjustment */
                AddSingle(si, targ, 0, 0, targ->metricsInfo.metrics[0], 0);
            } else {
                assert(targ->metricsInfo.cnt == 4);
                int16_t *metrics = targ->metricsInfo.metrics;
                AddSingle(si, targ, metrics[0], metrics[1],
                          metrics[2], metrics[3]);
            }
        }
        return;
    } else if (lkpType == GPOSPair) {
        /* metrics are now associated with the node they follow */
        AddPair(si, targ, targ->nextSeq, locDesc);
        return;
    } else if (lkpType == GPOSCursive) {
        /* Add BaseGlyphRec records, making sure that there is no overlap in anchor and markClass */
        GNode *nextNode = targ;

        /* Check if this is a contextual rule.*/
        if (targ->flags & FEAT_HAS_MARKED) {
            if (featValidateGPOSChain(g, targ, lkpType) == 0) {
                return;
            }

            /* add the pos rule to the anonymous tables. First, find the base glyph */
            while ((nextNode != NULL) && (!(nextNode->flags & FEAT_IS_BASE_NODE))) {
                nextNode = nextNode->nextSeq;
            }

            SubtableInfo &anon_si = addAnonPosRule(si, lkpType, nextNode);
            addCursive(anon_si, nextNode, anchorMarkInfo, locDesc);

            /* now add the nodes to the contextual rule list. */
            si.parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */
            si.rules.emplace_back(targ);
            if (nextNode != NULL) {
                /*  add the lookupLabel. */
                if (nextNode->lookupLabelCount > 255)
                    hotMsg(g, hotFATAL, "Anonymous lookup in chain caused overflow.");

                nextNode->lookupLabels[nextNode->lookupLabelCount] = anon_si.label;
                nextNode->lookupLabelCount++;
            } else {
                hotMsg(g, hotFATAL, "aborting due to unexpected NULL nextNode pointer");
            }
            /* is contextual */
        } else {
            /* isn't contextual */
            addCursive(si, targ, anchorMarkInfo, locDesc);
            featRecycleNodes(g, targ); /* I do this here rather than in feat.c;                       */
                                       /* addPos, as I have to NOT recycle them if they are contextual */
        }
    } else if ((lkpType == GPOSMarkToBase) || (lkpType == GPOSMarkToMark) || (lkpType == GPOSMarkToLigature)) {
        /* Add BaseGlyphRec records, making sure that there is no overlap in anchor and markClass */
        GNode *nextNode = targ;

        /* Check if this is a contextual rule.*/
        if (targ->flags & FEAT_HAS_MARKED) {
            if (featValidateGPOSChain(g, targ, lkpType) == 0) {
                return;
            }

            /* add the pos rule to the anonymous tables. First, find the base glyph.
             In a chain contextual rule, this will be the one and only marked glyph
             or glyph class. */
            while ((nextNode != NULL) && (!(nextNode->flags & FEAT_MARKED))) {
                nextNode = nextNode->nextSeq;
            }

            SubtableInfo &anon_si = addAnonPosRule(si, lkpType, nextNode);

            addMark(anon_si, nextNode, anchorMarkInfo, locDesc);

            /* now add the nodes to the contextual rule list. */
            si.parentLkpType = GPOSChain; /* So that this subtable will be processed as a chain at lookup end -> fill. */

            si.rules.emplace_back(targ);
            if (nextNode != NULL) {
                if (nextNode->lookupLabelCount > 255)
                    hotMsg(g, hotFATAL, "Anonymous lookup in chain caused overflow.");

                nextNode->lookupLabels[nextNode->lookupLabelCount] = anon_si.label;
                nextNode->lookupLabelCount++;
            } else {
                hotMsg(g, hotFATAL, "aborting due to unexpected NULL nextNode pointer");
            }
        } else {
            /* isn't contextual */
            addMark(si, targ, anchorMarkInfo, locDesc);
            featRecycleNodes(g, targ); /* I do this here rather than in feat.c;                       */
                                       /* addPos, as I have to NOT recycle them if they are contextual */
        }
    } else {
        /* Add whole rule intact (no enumeration needed) */
        si.rules.emplace_back(targ);
    }
}

/* Add rule (enumerating if necessary) to subtable si */

void GPOS::RuleAdd(int lkpType, GNode *targ, std::string &locDesc, std::vector<AnchorMarkInfo> &anchorMarkInfo) {
    if (g->hadError)
        return;

    if (targ->flags & FEAT_HAS_MARKED) {
        nw.parentLkpType = lkpType;
        nw.lkpType = GPOSChain;
    }

    addPosRule(nw, targ, locDesc, anchorMarkInfo);
}

/* For a particular classDef and a given class, return the GNode linked list */

GNode *GPOS::getGNodes(uint32_t cls, int classDefInx) {
    for (auto ci : classDef[classDefInx].classInfo) {
        if (ci.second.cls == cls)
            return ci.second.gc;
    }
    /* can't get here: the class definitions have already been conditioned in feat.c::addPos().
    hotMsg(g, hotFATAL, "class <%u> not valid in classDef", cls);
    */
    return NULL; /* Suppress compiler warning */
}

void GPOS::printKernPair(GID gid1, GID gid2, int val1, int val2, bool fmt1) {
    if (fmt1) {
        featGlyphDump(g, gid1, ' ', 0);
        featGlyphDump(g, gid2, '\0', 0);
    } else {
        featGlyphClassDump(g, getGNodes(gid1, 0), ' ', 0);
        featGlyphClassDump(g, getGNodes(gid2, 1), '\0', 0);
    }
}

GPOS::PairPos::PairPos(GPOS &h, SubtableInfo &si) : Subtable(h, si) {}

GPOS::PairPos::Format1::Format1(GPOS &h, GPOS::SubtableInfo &si) : PairPos(h, si) {
    std::sort(si.pairs.begin(), si.pairs.end());
    // Count pair sets and build coverage table
    cac->coverageBegin();
    int nPairSets = 0;
    GID prev = -1;
    for (auto &p : si.pairs) {
        if (p.first != prev) {
            cac->coverageAddGlyph(p.first);
            nPairSets++;
            prev = p.first;
        }
    }

    ValueFormat1 = si.pairValFmt1;
    ValueFormat2 = si.pairValFmt2;

    PairSets.reserve(nPairSets);

    /* Fill pair sets */
    int iFirst = 0;
    LOffset offst = pair1Size(nPairSets);
    for (uint32_t i = 1; i <= si.pairs.size(); i++) {
        if (i == si.pairs.size() || si.pairs[i].first != si.pairs[iFirst].first) {
            int valueCount = i - iFirst;
            PairSet curr;
            curr.offset = offst;
            curr.values.reserve(valueCount);
            for (int k = 0; k < valueCount; k++) {
                GPOS::KernRec &src = si.pairs[iFirst + k];
                PairValueRecord pvr;
                pvr.SecondGlyph = src.second;
                pvr.Value1 = h.recordValues(ValueFormat1, src.metricsRec1);
                pvr.Value2 = h.recordValues(ValueFormat2, src.metricsRec2);
                curr.values.emplace_back(std::move(pvr));
            }
            PairSets.emplace_back(std::move(curr));
            offst += pairSetSize(valueCount, numValues(ValueFormat1), numValues(ValueFormat2));
            iFirst = i;
        }
    }

    Coverage = cac->coverageEnd(); /* Adjusted later */
    if (isExt()) {
        Coverage += offst; /* Final value */
        h.incExtOffset(offst + cac->coverageSize());
    } else {
        h.incSubOffset(offst);
    }
}

Offset GPOS::classDefMake(std::shared_ptr<CoverageAndClass> &cac,
                          int cdefInx, LOffset *coverage, uint16_t &count) {
    ClassDef &cdef = classDef[cdefInx];

    /* --- Create coverage, if needed --- */
    if (coverage != NULL) {
        cac->coverageBegin();
        for (auto gid : cdef.cov)
            cac->coverageAddGlyph(gid);

        *coverage = cac->coverageEnd(); /* Adjusted later */
    }

    /* --- Create classdef --- */
    /* Classes start numbering from 0 for ClassDef1, 1 for ClassDef2 */
    if (g->convertFlags & HOT_DO_NOT_OPTIMIZE_KERN)
        count = (uint16_t)cdef.classInfo.size() + 1;
    else
        count = (uint16_t)((cdefInx == 0) ? cdef.classInfo.size() : cdef.classInfo.size() + 1);
    cac->classBegin();
    for (auto &ci : cdef.classInfo) {
        for (GNode *p = ci.second.gc; p != NULL; p = p->nextCl) {
            if (ci.second.cls != 0) {
                cac->classAddMapping(p->gid, ci.second.cls);
            }
        }
    }
    return cac->classEnd();
}

GPOS::PairPos::Format2::Format2(GPOS &h, GPOS::SubtableInfo &si) : PairPos(h, si) {
#if HOT_DEBUG
    int nFilled = 0;
#endif
    uint16_t class1Count, class2Count;
    std::sort(si.pairs.begin(), si.pairs.end());

    ValueFormat1 = si.pairValFmt1;
    ValueFormat2 = si.pairValFmt2;

    /* (ClassDef offsets adjusted later) */
    ClassDef1 = h.classDefMake(cac, 0, &Coverage, class1Count);
    ClassDef2 = h.classDefMake(cac, 1, NULL, class2Count);

    /* --- Allocate and initialize 2-dimensional array Class1Record */
    ClassRecords.resize(class1Count);
    for (auto &cr : ClassRecords)
        cr.resize(class2Count);

    /* --- Fill in Class1Record */
    for (auto p : si.pairs) {
        uint32_t cl1 = p.first;
        uint32_t cl2 = p.second;
        Class2Record &dst = ClassRecords[cl1][cl2];
        dst.Value1 = h.recordValues(ValueFormat1, p.metricsRec1);
        dst.Value2 = h.recordValues(ValueFormat2, p.metricsRec2);
#if HOT_DEBUG
        nFilled++;
#endif
    }

    LOffset size = pair2Size(class1Count, class2Count,
                             numValues(ValueFormat1) +
                             numValues(ValueFormat2));
#if HOT_DEBUG
    auto g = h.g;
    DF(1, (stderr,
           "#Cl kern: %d of %u(%hux%u) array is filled "
           "(%4.2f%%), excl ClassDef2's class 0. Subtbl size: %u\n",
           nFilled,
           class1Count * (class2Count - 1),
           class1Count,
           class2Count - 1,
           nFilled * 100.00 / (class1Count * (class2Count - 1)),
           size));
#endif

    if (isExt()) {
        Coverage += size;                         // Final value
        ClassDef1 += size + cac->coverageSize();  // Final value
        ClassDef2 += size + cac->coverageSize();  // Final value
        h.incExtOffset(size + cac->coverageSize() + cac->classSize());
    } else {
        h.incSubOffset(size);
    }
}

void GPOS::PairPos::fill(GPOS &h, SubtableInfo &si) {
    if (si.pairs.size() != 0) {
        if (si.pairFmt == 1) {
            h.AddSubtable(std::move(std::make_unique<Format1>(h, si)));
        } else {
            assert(si.pairFmt == 2);
            h.AddSubtable(std::move(std::make_unique<Format2>(h, si)));
        }
    }
    h.updateMaxContext(2);
}

void GPOS::PairPos::Format1::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset;  // Adjust offset

    h->checkOverflow("coverage table", Coverage, "pair positioning");

    // Write header
    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2(ValueFormat1);
    OUT2(ValueFormat2);
    OUT2((uint16_t)PairSets.size());

    // Write pair sets
    for (auto &ps : PairSets)
        OUT2((Offset)ps.offset);

    for (auto &ps : PairSets) {
        OUT2((uint16_t)ps.values.size());
        for (auto &pvr : ps.values) {
            OUT2(pvr.SecondGlyph);
            h->writeValueRecord(ValueFormat1, pvr.Value1);
            h->writeValueRecord(ValueFormat2, pvr.Value2);
        }
    }

    if (isExt()) {
        cac->coverageWrite();
    }
}

void GPOS::PairPos::Format2::write(OTL *h) {
    if (!isExt()) {
        /* Adjust coverage and class offsets */
        LOffset classAdjust = h->subOffset() - offset +
                              cac->coverageSize();
        Coverage += h->subOffset() - offset;
        ClassDef1 += classAdjust;
        ClassDef2 += classAdjust;
    }
    h->checkOverflow("coverage table", Coverage, "pair positioning");
    h->checkOverflow("class 1 definition table", ClassDef1, "pair positioning");
    h->checkOverflow("class 2 definition table", ClassDef2, "pair positioning");

    /* Write header */
    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2(ValueFormat1);
    OUT2(ValueFormat2);
    OUT2((uint16_t)ClassDef1);
    OUT2((uint16_t)ClassDef2);
    OUT2((uint16_t)ClassRecords.size());
    OUT2((uint16_t)ClassRecords[0].size());

    for (auto cr1 : ClassRecords) {
        for (auto cr2 : cr1) {
            h->writeValueRecord(ValueFormat1, cr2.Value1);
            h->writeValueRecord(ValueFormat2, cr2.Value2);
        }
    }

    if (isExt()) {
        cac->coverageWrite();
        cac->classWrite();
    }
}

/* ------------------ Chaining Contextual Substitution --------------------- */

/* Tries to add rule to current anon subtbl. If successful, returns 1, else 0.
   If rule already exists in subtbl, recycles targ */

int GPOS::SingleRec::cmpWithRule(GNode *targ, int nFound) {
    GNode *t1 = targ;
    int checkedMetrics = 0;
    int i;
    short metricsCnt1 = targ->metricsInfo.cnt;
    short metricsCnt2;
    short *metrics1 = targ->metricsInfo.metrics;
    short metrics2[4];

    if (valFmt == GPOS::ValueXAdvance) {
        metricsCnt2 = 1;
        metrics2[0] = metricsRec.value[2];
    } else {
        metricsCnt2 = 4;
        metrics2[0] = metricsRec.value[0];
        metrics2[1] = metricsRec.value[1];
        metrics2[2] = metricsRec.value[2];
        metrics2[3] = metricsRec.value[3];
    }

    assert(metricsCnt1 != -1);

    for (; t1 != NULL; t1 = t1->nextCl) {
        if ((t1->gid == gid) && (!(t1->flags & FEAT_MISC))) {
            if (!checkedMetrics) {
                /* a test so that we only check the metrics for the head node of the class */
                if (metricsCnt1 != metricsCnt2) {
                    return -1;
                }
                for (i = 0; i < metricsCnt1; i++) {
                    if (metrics1[i] != metrics2[i]) {
                        return -1;
                    }
                }
                checkedMetrics = 1;
            }
            nFound++;
            t1->flags |= FEAT_MISC;
        }
    }
    return nFound;
}

/* Add the "anonymous" rule that occurs in a substitution within a chaining
   contextual rule. Return the label of the anonymous lookup */

int32_t GPOS::SubtableInfo::checkAddRule(GNode *targ) {
    GNode *t = targ;
    int nTarg = 0;
    int nFound = 0;

    /* If all the class members for this single pos rule are in the current */
    /* subtable, we can just return, after recycling all the nodes. If any  */
    /* one is present but with a different metrics record, we need to       */
    /* return 0, not found. If some nodes are new and others are not, we    */
    /* need to remove the "already known" nodes from the rule, and recycle  */
    /* them.                                                                */
    for (; t != NULL; t = t->nextCl) {
        nTarg++;
    }

    for (auto &cmpRec : singles) {
        nFound = cmpRec.cmpWithRule(targ, nFound);
        if (nFound < 0) {
            break; /* found a match in coverage, but with a different metrics record. Must put it in a different lookup.*/
        }
        if (nFound == nTarg) {
            break;
        }
    }

    if (nFound < 0) {
        return 0; /* Conflicting records - must use new subtable */
    } else if (nTarg == nFound) {
        return -1; /* All already present; no need to add more */
    } else {
        return 1; /* Can add to current subtable */
    }
}

GPOS::SubtableInfo &GPOS::addAnonPosRule(SubtableInfo &cur_si, uint16_t lkpType, GNode *targ) {
    /* For GPOSSingle lookups only, we check if the rule can be added to    */
    /* the most current subtable in the anonymous list, and if not, look to */
    /* see if it can be added to earlier subtables. For all others, we      */
    /* check only the must current subtable, and assume that it is OK if    */
    /* the type matches.                                                    */
    for (auto si = anonSubtable.rbegin(); si != anonSubtable.rend(); si++) {
        if ((si->script == cur_si.script) &&
            (si->language == cur_si.language) &&
            (si->feature == cur_si.feature) &&
            (si->useExtension == cur_si.useExtension) &&
            (si->lkpFlag == cur_si.lkpFlag) &&
            (si->markSetIndex == cur_si.markSetIndex) &&
            (si->parentFeatTag == nw.feature) &&
            (lkpType == si->lkpType)) {
                if (lkpType != GPOSSingle || si->checkAddRule(targ) != 0)
                    return *si;
            }
        if (lkpType != GPOSSingle)
            break;
    }

    /* If we get here, we could not use the most recent anonymous table, */
    /* and must add a new one.                                           */
    SubtableInfo asi;

    /* *si = *cur_si; Can't do this, as it copies all the dna array pointers */
    /* Copy all values except the dna array and format specific stuff */
    asi.script = cur_si.script;
    asi.language = cur_si.language;
    asi.feature = cur_si.feature;
    asi.parentFeatTag = nw.feature;
    asi.useExtension = cur_si.useExtension; /* Use extension lookupType? */
    asi.lkpFlag = cur_si.lkpFlag;
    asi.markSetIndex = cur_si.markSetIndex;

    /* Now for the new values that are specific to this table */
    asi.lkpType = lkpType;
    asi.parentLkpType = 0;
    asi.label = featGetNextAnonLabel(g);

    anonSubtable.emplace_back(std::move(asi));

    return anonSubtable.back();
}

/* Create anonymous lookups (referred to only from within chain ctx lookups) */

void GPOS::createAnonLookups() {
    for (auto &si : anonSubtable) {
        si.script = si.language = si.feature = TAG_UNDEF; /* so that these will sort to the end of the subtable array       */
                                                             /* and will not be considered for adding to the FeatureList table */

        g->error_id_text = std::string("feature '") + TAG_STR(si.parentFeatTag) + std::string("'");
        LookupEnd(&si); /* This is where the fill functions get called */
        FeatureEnd();
    }
}

/* points to an input sequence; return new array of num coverages */

void GPOS::setCoverages(std::vector<LOffset> &covs, std::shared_ptr<CoverageAndClass> &cac,
                        GNode *p, uint32_t num) {
    if (num == 0)
        return;

    covs.reserve(num);
    for (uint32_t i = 0; i != num; i++, p = p->nextSeq) {
        cac->coverageBegin();
        for (GNode *q = p; q != NULL; q = q->nextCl)
            cac->coverageAddGlyph(q->gid);

        covs.push_back(cac->coverageEnd()); /* Adjusted later */
    }
}

GPOS::ChainContextPos::ChainContextPos(GPOS &h, GPOS::SubtableInfo &si, GPOS::PosRule &rule) : Subtable(h, si) {
    LOffset size;
    uint32_t nBack = 0;
    uint32_t nInput = 0;
    uint32_t nLook = 0;
    uint32_t nMarked = 0;
    uint32_t seqCnt = 0;
    GNode *pBack = NULL;
    GNode *pInput = NULL;
    GNode *pLook = NULL;
    GNode *pMarked = NULL;
    int nPos = 0;

    GNode *p;

    /* Set counts of and pointers to Back, Input, Look, Marked areas */
    pBack = rule.targ;
    for (p = rule.targ; p != NULL; p = p->nextSeq) {
        if (p->flags & FEAT_BACKTRACK) {
            nBack++;
        } else if (p->flags & FEAT_INPUT) {
            if (pInput == NULL) {
                pInput = p;
            }
            nInput++;
            if (p->flags & FEAT_MARKED) {
                /* Marked must be within Input */
                if (pMarked == NULL) {
                    pMarked = p;
                }
                nMarked++;
                nPos += p->lookupLabelCount;
            }
            seqCnt++;
        } else if (p->flags & FEAT_LOOKAHEAD) {
            if (pLook == NULL) {
                pLook = p;
            }
            nLook++;
        }
    }

    /* Fill table */
    h.setCoverages(backtracks, cac, pBack, nBack);
    h.setCoverages(inputGlyphs, cac, pInput, nInput);
    h.setCoverages(lookaheads, cac, pLook, nLook);

    if (nPos > 0) {
        lookupRecords.reserve(nPos);
        GNode *nextNode = pInput;

        for (uint32_t i = 0; i < seqCnt; i++) {
            if (nextNode->lookupLabelCount == 0) {
                nextNode = nextNode->nextSeq;
                continue;
            }
            /* Store ptr for easy access later on when lkpInx's are available */
            for (int j = 0; j < nextNode->lookupLabelCount; j++)
                lookupRecords.emplace_back(i, nextNode->lookupLabels[j]);

            nextNode = nextNode->nextSeq;
            if (nextNode == NULL) {
                break;
            }
        }
    }

    h.updateMaxContext(nInput + nLook);

    size = chain3Size(nBack, nInput, nLook, nPos);
    if (isExt()) {
        /* Set final values for coverages */
        for (auto &bt : backtracks)
            bt += size;
        for (auto &ig : inputGlyphs)
            ig += size;
        for (auto &la : lookaheads)
            la += size;
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }
    featRecycleNodes(h.g, rule.targ);
}

void GPOS::ChainContextPos::fill(GPOS &h, SubtableInfo &si) {
    /* xxx Each rule in a fmt3 for now */
    for (auto &rule : si.rules) {
        h.AddSubtable(std::move(std::make_unique<ChainContextPos>(h, si, rule)));
        h.checkOverflow("lookup subtable", h.subOffset(), "chain contextual positioning");
    }
}

void GPOS::ChainContextPos::write(OTL *h) {
    LOffset adjustment = 0;

    if (!isExt())
        adjustment = h->subOffset() - offset;

    OUT2(subformat());
    OUT2((uint16_t) backtracks.size());

    if (h->g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
        /* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
        for (auto &bt : backtracks) {
            if (!isExt())
                bt += adjustment;
            h->checkOverflow("backtrack coverage table", bt, "chain contextual positioning");
            OUT2((uint16_t)bt);
        }
    } else {
        /* do it per OpenType spec 1.5 */
        for (auto ri = backtracks.rbegin(); ri != backtracks.rend(); ri++) {
            if (!isExt())
                *ri += adjustment;
            h->checkOverflow("backtrack coverage table", *ri, "chain contextual positioning");
            OUT2((uint16_t)*ri);
        }
    }

    OUT2((uint16_t)inputGlyphs.size());
    for (auto &ig : inputGlyphs) {
        if (!isExt())
            ig += adjustment;
        h->checkOverflow("input coverage table", ig, "chain contextual positioning");
        OUT2((uint16_t)ig);
    }

    OUT2((uint16_t)lookaheads.size());
    for (auto &la : lookaheads) {
        if (!isExt())
            la += adjustment;
        h->checkOverflow("input coverage table", la, "chain contextual positioning");
        OUT2((uint16_t)la);
    }

    OUT2((uint16_t)lookupRecords.size());
    for (auto &lr : lookupRecords) {
        OUT2(lr.SequenceIndex);
        OUT2(lr.LookupListIndex);
    }

    if (isExt())
        cac->coverageWrite();
}

/* --------------------------- Cursive Attachment -------------------------- */

bool GPOS::BaseGlyphRec::cmpLig(const GPOS::BaseGlyphRec &a, const GPOS::BaseGlyphRec &b) {
    if (a.gid != b.gid)
        return a.gid < b.gid;
    return a.anchorMarkInfo[0].componentIndex < b.anchorMarkInfo[0].componentIndex;
}

void GPOS::checkBaseAnchorConflict(std::vector<BaseGlyphRec> &baseList) {
    if (baseList.size() < 2)
        return;

    std::sort(baseList.begin(), baseList.end());
    /* Sort by GID, anchor count, and then by anchor value */
    auto cur = baseList.begin();
    auto prev = cur;
    /* Now step through, and report if for any entry, the previous entry with the same GID has  the same  mark class */
    while (++cur != baseList.end()) {
        if (cur->gid == prev->gid) {
            /* For mark to base and mark to mark, only a single entry is allowed in  the baseGlyphArray for a given GID. */
            featGlyphDump(g, cur->gid, '\0', 0);
            hotMsg(g, hotERROR, "MarkToBase or MarkToMark error in %s. Another statement has already defined the anchors and marks on glyph '%s'. [current at %s, previous at %s]",
                    g->error_id_text.c_str(), g->note.array, cur->locDesc.c_str(), prev->locDesc.c_str());
        }
        prev = cur;
    }
}

void GPOS::checkBaseLigatureConflict(std::vector<BaseGlyphRec> &baseList) {
    if (baseList.size() < 2)
        return;

    /* Sort by GID, then component index */
    std::sort(baseList.begin(), baseList.end(), BaseGlyphRec::cmpLig);
    auto cur = baseList.begin();
    auto prev = cur;
    /* Now step through, and report if for any entry, the previous entry with the same GID has  the same component index. */
    while (++cur != baseList.end()) {
        if (cur->gid == prev->gid &&
            cur->anchorMarkInfo[0].componentIndex == prev->anchorMarkInfo[0].componentIndex) {
            featGlyphDump(g, cur->gid, '\0', 0);
            hotMsg(g, hotERROR,
                   "MarkToLigature error in %s. Two different statements referencing the ligature glyph '%s' have assigned the same mark class to the same ligature component. [current at %s, previous at %s]",
                g->error_id_text.c_str(), g->note.array, cur->locDesc.c_str(), prev->locDesc.c_str());
        }
        prev = cur;
    }
}

int32_t GPOS::SubtableInfo::findMarkClassIndex(GNode *markNode) {
    int32_t i = 0;
    for (auto &mc : markClassList) {
        if (markNode == mc.gnode) {
            return i;
        }
        i++;
    }
    return -1;
}

int GPOS::addMarkClass(SubtableInfo &si, GNode *markNode) {
    /* validate that there is no overlap between this class and previous classes; if not, add the MarkRec entry */
    GNode *nextClass = markNode;
    while (nextClass != NULL) {
        GID newGID = nextClass->gid;

        for (auto &mc : si.markClassList) {
            GNode *curNode = mc.gnode;
            while (curNode != NULL) {
                if (curNode->gid == newGID) {
                    featGlyphDump(g, newGID, '\0', 0);
                    hotMsg(g, hotERROR, "In %s, glyph '%s' occurs in two different mark classes. Previous mark class: %s. Current mark class: %s.",
                           g->error_id_text.c_str(), g->note.array, mc.gnode->markClassName, markNode->markClassName);
                    return -1;
                }
                curNode = curNode->nextCl;
            } /* end for all gnodes in markClassList.array[markRec->cls].gnode */
        }     /* end for all previously seen classes for this lookup */

        nextClass = nextClass->nextCl;
    } /* end all nodes in the new mark class */

    /* Now validate that all glyph ID's are unique within the new class */
    nextClass = markNode;
    while (nextClass != NULL) {
        GNode *testNode = nextClass->nextCl;
        while (testNode != NULL) {
            if (testNode->gid == nextClass->gid) {
                featGlyphDump(g, testNode->gid, '\0', 0);
                hotMsg(g, hotERROR, "In %s, glyph '%s' is repeated in the current class definition. Mark class: %s.",
                       g->error_id_text.c_str(), g->note.array, markNode->markClassName);
                return -1;
            }
            testNode = testNode->nextCl;
        }
        nextClass = nextClass->nextCl;
    }
    /* If we got here, there was no GID conflict.*/
    /* now add the mark class head node to the markClassList */
    si.markClassList.emplace_back(markNode);
    return si.markClassList.size() - 1;
}

GPOS::AnchorPosBase::AnchorPosBase(GPOS &h, SubtableInfo &si) : Subtable(h, si) {}

LOffset GPOS::AnchorPosBase::getAnchorOffset(const AnchorMarkInfo &anchor) {
    uint32_t lastFormat = 0;
    LOffset offst = 0;

    for (auto &ar : anchorList) {
        if (ar.anchor == anchor)
            return ar.offset;
        lastFormat = ar.anchor.format;
        offst = ar.offset;
    }

    if (lastFormat == 2)
        offst += sizeof(uint16_t) * 4;
    else if (lastFormat != 0)
        offst += sizeof(uint16_t) * 3;

    anchorList.emplace_back(offst, anchor);
    return offst;
}

void GPOS::addMark(SubtableInfo &si, GNode *targ,
                   std::vector<AnchorMarkInfo> &anchorMarkInfo,
                   std::string &locDesc) {
    GNode *nextNode = targ;

    /* Ligatures have multiple components. We can tell when we hit a new    */
    /* component, as the anchorMarkInfo componentIndex increases. For       */
    /* mark-to-base and mark-to-mark, there is only one component, and only */
    /* one baseRec.                                                         */
    while (nextNode != NULL) {
        BaseGlyphRec baseRec;
        baseRec.componentIndex = -1;

        for (auto am : anchorMarkInfo) {
            if (baseRec.componentIndex != am.componentIndex) {
                if (baseRec.componentIndex != -1)
                    si.baseList.push_back(std::move(baseRec));
                baseRec.gid = nextNode->gid;
                baseRec.locDesc = locDesc;
                baseRec.componentIndex = am.componentIndex;
            }
            AnchorMarkInfo nam = am;
            if (nam.markClass != NULL) {
                nam.markClassIndex = si.findMarkClassIndex(nam.markClass);
                if (nam.markClassIndex < 0) {
                    nam.markClassIndex = addMarkClass(si, nam.markClass);
                }
            }
            baseRec.anchorMarkInfo.push_back(nam);
        }
        if (baseRec.componentIndex != -1)
            si.baseList.push_back(std::move(baseRec));
        nextNode = nextNode->nextCl;
    }
}


GPOS::MarkBasePos::MarkBasePos(GPOS &h, GPOS::SubtableInfo &si) : AnchorPosBase(h, si) {
    LOffset size = markBase1Size();

    ClassCount = (uint16_t) si.markClassList.size();

    /* Build mark coverage list from list of mark classes. Each mark class  */
    /* is a linked list of nodes. We have already validated that there is   */
    /* no overlap in GID's. We step through the classes. For each class, we */
    /* step through each node,  add its GID to the  Coverage table, and its */
    /* info to the mark record list. */
    uint32_t numMarkGlyphs = 0;
    cac->coverageBegin();
    for (auto mc : si.markClassList) {
        GNode *nextNode = mc.gnode;
        while (nextNode != NULL) {
            cac->coverageAddGlyph(nextNode->gid);
            numMarkGlyphs++;
            nextNode = nextNode->nextCl;
        }
    }
    MarkCoverage = cac->coverageEnd(); /* otlCoverageEnd does the sort by GID */

    /* Now we know how many mark nodes there are, we can build the MarkArray */
    /* table, and get its size. We will keep things simple, and write the    */
    /* MarkArray right after the MarkToBase subtable, followed by the        */
    /* BaseArray table, followed by a list of the anchor tables.             */
    MarkOffset = (Offset)size; /* offset from the start of the MarkToBase subtable */
    uint16_t i = 0;
    for (auto mc : si.markClassList) {
        for (GNode *nextNode = mc.gnode; nextNode != NULL; nextNode = nextNode->nextCl) {
            MarkRecords.emplace_back(nextNode->gid,
                                     getAnchorOffset(nextNode->markClassAnchorInfo),
                                     i);
        }
        i++;
    }
    size += sizeof(uint16_t) + (numMarkGlyphs * (2 * sizeof(uint16_t)));

    std::sort(MarkRecords.begin(), MarkRecords.end());

    /* Build base glyph coverage list from rules. First, we need to sort   */
    /* the base record list by GID, and make sure there are no errors Then */
    /* we can simply step through it to make the base coverage table.      */
    h.checkBaseAnchorConflict(si.baseList);

    BaseOffset = (Offset)size; /* offset from the start of the MarkToBase subtable = size of subtable + size of MarkArray table. */
    long baseArraySize = sizeof(uint16_t); /* size of BaseArray.BaseCount */
    cac->coverageBegin();
        /* Note: I have to add the size of the AnchorArray for each baseRec individually, as they can be different sizes. */
    int32_t prevGID = -1;
    for (auto &baseRec : si.baseList) {
        if (prevGID == baseRec.gid)
            continue;
            /* No need for logic to report base glyph conflict; this is already reported in checkBaseAnchorConflict() */
        /* we are seeing a new glyph ID; need to allocate the anchor tables for it, and add it to the coverage table. */
        cac->coverageAddGlyph(baseRec.gid);
        baseArraySize += ClassCount * sizeof(uint16_t); /* this adds fmt->ClassCount offset values to the output.*/
        std::vector<LOffset> br(ClassCount, 0xFFFFFFFFL);
        prevGID = baseRec.gid;

        /* Check uniqueness of anchor vs mark class*/
        for (auto am : baseRec.anchorMarkInfo) {
            if (br[am.markClassIndex] != 0xFFFFFFFFL) {
                /*it has already been filled in !*/
                featGlyphDump(h.g, prevGID, '\0', 0);
                hotMsg(h.g, hotERROR, "MarkToBase or MarkToMark error in %s. Another statement has already assigned the current mark class to another anchor point on glyph '%s'. [previous at %s]",
                       h.g->error_id_text.c_str(), h.g->note.array, baseRec.locDesc.c_str());
            } else {
                br[am.markClassIndex] = getAnchorOffset(am); /* offset from start of anchor list */
            }
        }
        BaseRecords.emplace_back(std::move(br));
    }

    /* For any glyph, the user may not have specified an anchor for any */
    /* particular mark class. We will fill these in with NULL anchors,  */
    /* but will also report a warning.                                  */
    for (uint32_t i = 0; i < BaseRecords.size(); i++) {
        auto &br = BaseRecords[i];
        GPOS::BaseGlyphRec &baseRec = si.baseList[i];

        for (int j = 0; j < ClassCount; j++) {
            if (br[j] == 0xFFFFFFFFL) {
                featGlyphDump(h.g, baseRec.gid, '\0', 0);
                hotMsg(h.g, hotWARNING, "MarkToBase or MarkToMark error in %s. Glyph '%s' does not have an anchor point for a mark class that was used in a previous statement in the same lookup table. Setting the anchor point offset to 0. [previous at %s]",
                    h.g->error_id_text.c_str(), h.g->note.array, baseRec.locDesc.c_str());
            }
        }
    }

    size += baseArraySize;
    BaseCoverage = cac->coverageEnd(); /* otlCoverageEnd does the sort by GID */
    endArrays = size;

    /* Now add the size of the anchor list*/
    auto &anchorRec = anchorList.back();
    size += anchorRec.offset;
    if (anchorRec.anchor.format == 2) {
        size += sizeof(uint16_t) * 4;
    } else {
        size += sizeof(uint16_t) * 3;
    }

    if (isExt()) {
        MarkCoverage += size; /* Adjust offset */
        BaseCoverage += size; /* Adjust offset */
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }

    h.checkOverflow("lookup subtable", h.subOffset(), "mark to base positioning");
}

void GPOS::MarkBasePos::fill(GPOS &h, GPOS::SubtableInfo &si) {
    h.AddSubtable(std::move(std::make_unique<MarkBasePos>(h, si)));
}

void GPOS::MarkBasePos::write(OTL *h) {
    LOffset adjustment = 0; /* (Linux compiler complains) */
    if (!isExt())
        adjustment = (h->subOffset() - offset);

    MarkCoverage += adjustment; /* Adjust offset */
    BaseCoverage += adjustment; /* Adjust offset */

    OUT2(subformat());
    h->checkOverflow("mark coverage table", MarkCoverage, "mark to base positioning");
    OUT2((Offset)MarkCoverage);
    h->checkOverflow("base coverage table", BaseCoverage, "mark to base positioning");
    OUT2((Offset)BaseCoverage);
    OUT2(ClassCount);
    OUT2((Offset)MarkOffset);
    OUT2((Offset)BaseOffset);
    /* Now write out MarkArray */
    OUT2((uint16_t)MarkRecords.size());
    /* Now write out MarkRecs */
    uint32_t anchorListOffset = endArrays - MarkOffset;
    for (auto &mr : MarkRecords) {
        OUT2(mr.cls);
        OUT2((Offset)(mr.anchor + anchorListOffset));
    }

    /* Now write out BaseArray */
    OUT2((uint16_t)BaseRecords.size());
    anchorListOffset = endArrays - BaseOffset;
    /* Now write out BaseRecs */
    for (auto &br : BaseRecords) {
        for (auto &o : br) {
            if (o == 0xFFFFFFFFL)
                OUT2(0);
            else
                OUT2((Offset)(o + anchorListOffset));
        }
    }

    /* Now write out the  anchor list */
    for (auto &an : anchorList) {
        auto &anchorRec = an.anchor;
        if (anchorRec.format == 0) {
            anchorRec.format = 1; /* This is an anchor record that never got filled in, or was specified as a NULL anchor. */
                                   /* In either case, it is written as a default anchor with x=y =0                         */
        }
        OUT2(anchorRec.format); /*offset to anchor */
        OUT2(anchorRec.x);      /*offset to anchor */
        OUT2(anchorRec.y);      /*offset to anchor */
        if (anchorRec.format == 2) {
            OUT2(anchorRec.contourpoint); /*offset to anchor */
        }
    }

    if (isExt())
        cac->coverageWrite();
}

/* ---------------------- Mark To Ligature Attachment ---------------------- */

GPOS::MarkLigaturePos::MarkLigaturePos(GPOS &h, GPOS::SubtableInfo &si) : AnchorPosBase(h, si) {
    LOffset size = markLig1Size();

    ClassCount = (uint16_t)si.markClassList.size();
    /* Build mark coverage list from list of mark classes. Each mark class  */
    /* is a linked list of nodes. We have already validated that there is   */
    /* no overlap in GID's. We step through the classes. For each class, we */
    /* step through each node, add its GID to the Coverage table, and its   */
    /* info to the mark record list.                                        */
    uint32_t numMarkGlyphs = 0;
    cac->coverageBegin();
    for (auto mc : si.markClassList) {
        GNode *nextNode = mc.gnode;
        while (nextNode != NULL) {
            cac->coverageAddGlyph(nextNode->gid);
            numMarkGlyphs++;
            nextNode = nextNode->nextCl;
        }
    }
    MarkCoverage = cac->coverageEnd(); /* coverageEnd does the sort by GID */

    /* Now we know how many mark nodes there are, we can build the MarkArray */
    /* table, and get its size. We will keep things simple, and write the    */
    /* MarkArray right after the MarkLigature subtable, followed by the      */
    /* LigatureArray table, followed by a list of the anchor tables.         */
    MarkOffset = (Offset)size; /* offset from the start of the MarkToBase subtable */
    uint16_t i = 0;
    for (auto mc : si.markClassList) {
        for (GNode *nextNode = mc.gnode; nextNode != NULL; nextNode = nextNode->nextCl) {
            MarkRecords.emplace_back(nextNode->gid,
                                     getAnchorOffset(nextNode->markClassAnchorInfo),
                                     i);
        }
        i++;
    }
    size += sizeof(uint16_t) + (numMarkGlyphs * (2 * sizeof(uint16_t)));

    std::sort(MarkRecords.begin(), MarkRecords.end());

    /* Build ligature glyph coverage list from rules. First, we need to    */
    /* sort the base record list by GID, and make sure there are no errors */
    /* Then we can simply step through it to make the base coverage table. */
    /* sort the recs by base gid value, then by component index. */
    h.checkBaseLigatureConflict(si.baseList);

    LigatureOffset = (Offset)size;
    cac->coverageBegin();
    int32_t ligArraySize = sizeof(uint16_t); /* size of LigatureArray.LigatureCount */
    LigatureAttach la;
    la.offset = (Offset)ligArraySize;
    int32_t prevGID = -1;
    for (auto &baseRec : si.baseList) {
        if (prevGID != baseRec.gid) {
            if (la.components.size() > 0) {
                LigatureAttaches.emplace_back(std::move(la));
                la.components.clear();
                ligArraySize += sizeof(uint16_t);
                la.offset = (Offset)ligArraySize;
            }
            cac->coverageAddGlyph(baseRec.gid);
            prevGID = baseRec.gid;
        }
        std::vector<LOffset> clo(ClassCount, 0xFFFFFFFFL);
        ligArraySize += ClassCount * sizeof(uint16_t);

        /* Check uniqueness of anchors in the baseRec, and fill in the ligature attach table */
        for (auto &am : baseRec.anchorMarkInfo) {
            if (clo[am.markClassIndex] != 0xFFFFFFFFL) {
                /*it has already been filled in !*/
                featGlyphDump(h.g, baseRec.gid, '\0', 0);
                hotMsg(h.g, hotERROR, "MarkToLigature statement error in %s. Glyph '%s' contains a duplicate mark class assignment for one of the ligature components. [previous at %s]",
                    h.g->error_id_text.c_str(), h.g->note.array, baseRec.locDesc.c_str());
            } else {
                if (am.format != 0) {
                    /* Skip anchor if the format is 0 aka NULL anchor */
                    clo[am.markClassIndex] = getAnchorOffset(am);
                }
            }
        }
        la.components.emplace_back(std::move(clo));
    }
    if (la.components.size() > 0) {
        LigatureAttaches.emplace_back(std::move(la));
        ligArraySize += sizeof(uint16_t);
    }
    LigatureCoverage = cac->coverageEnd(); /* coverageEnd does the sort by GID */

    ligArraySize += sizeof(uint16_t) * LigatureAttaches.size();
    size += ligArraySize;
    endArrays = size;

    /* Now add the size of the anchor list*/
    auto &anchorRec = anchorList.back();
    size += anchorRec.offset;
    if (anchorRec.anchor.format == 2) {
        size += sizeof(uint16_t) * 4;
    } else {
        size += sizeof(uint16_t) * 3;
    }

    if (isExt()) {
        MarkCoverage += size;     /* Adjust offset */
        LigatureCoverage += size; /* Adjust offset */
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }

    h.checkOverflow("lookup subtable", h.subOffset(), "mark to ligature positioning");
}

void GPOS::MarkLigaturePos::fill(GPOS &h, GPOS::SubtableInfo &si) {
    h.AddSubtable(std::move(std::make_unique<MarkLigaturePos>(h, si)));
}

void GPOS::MarkLigaturePos::write(OTL *h) {
    LOffset adjustment = 0; /* (Linux compiler complains) */
    if (!isExt())
        adjustment = h->subOffset() - offset;

    MarkCoverage += adjustment;     /* Adjust offset */
    LigatureCoverage += adjustment; /* Adjust offset */

    OUT2(subformat());
    h->checkOverflow("mark coverage table", MarkCoverage, "mark to ligature positioning");
    OUT2((Offset)MarkCoverage);
    h->checkOverflow("ligature coverage table", LigatureCoverage, "mark to ligature positioning");
    OUT2((Offset)LigatureCoverage);
    OUT2(ClassCount);
    OUT2((Offset)MarkOffset);
    OUT2((Offset)LigatureOffset);
    /* Now write out MarkArray */
    OUT2((uint16_t)MarkRecords.size());
    /* Now write out MarkRecs */
    LOffset anchorListOffset = endArrays - MarkOffset;
    for (auto &mr : MarkRecords) {
        OUT2(mr.cls);
        OUT2((Offset)(mr.anchor + anchorListOffset));
    }

    /* Now write out LigatureArray */
    OUT2((uint16_t)LigatureAttaches.size());
    LOffset ligAttachOffsetSize = sizeof(uint16_t) * LigatureAttaches.size();
    /* fmt->endArrays is the position of the start of the anchor List. */
    /* fmt->endArrays - fmt->LigatureArray is the offset from the fmt->LigatureArray to the start of the anchor table. */
    anchorListOffset = endArrays - LigatureOffset;

    /*Now write out the offsets to the LigatureAttach recs*/
    for (auto &la : LigatureAttaches) {
        la.offset += ligAttachOffsetSize;
        OUT2((Offset) la.offset);
    }

    /* Now write the Ligature Attach recs */
    for (auto &la : LigatureAttaches) {
        OUT2((uint16_t)la.components.size());
        for (auto &cv : la.components) {
            for (auto o : cv) {
                if (o == 0xFFFFFFFFL) {
                    OUT2(0);
                } else {
                    /* anchorOffset[k] + anchorListOffset is the offset   */
                    /* from the start of the LigatureArray to the offset. */
                    /* We then need to subtract the offset from the start */
                    /* of the LigatureArray table to the LigatureAttach   */
                    /* record.                                            */
                    OUT2((Offset)(o + (anchorListOffset - la.offset)));
                }
            }
        }
    }

    /* Now write out the  anchor list */
    for (auto &an : anchorList) {
        auto &anchorRec = an.anchor;
//        if (anchorRec.format == 0) {
//            anchorRec.format = 1; /* This is an anchor record that never got filled in, or was specified as a NULL anchor. */
//                                   /* In either case, it is written as a default anchor with x=y =0                         */
//        }
        OUT2(anchorRec.format); /*offset to anchor */
        OUT2(anchorRec.x);      /*offset to anchor */
        OUT2(anchorRec.y);      /*offset to anchor */
        if (anchorRec.format == 2) {
            OUT2(anchorRec.contourpoint); /*offset to anchor */
        }
    }

    if (isExt())
        cac->coverageWrite();
}

/* ------------------------ Cursive Attachment ------------------------ */

void GPOS::addCursive(SubtableInfo &si, GNode *targ,
                     std::vector<AnchorMarkInfo> &anchorMarkInfo,
                     std::string &locDesc) {
    GNode *nextNode = targ;

    while (nextNode != NULL) {
        BaseGlyphRec baseRec;
        baseRec.gid = nextNode->gid;

        for (auto am : anchorMarkInfo)
            baseRec.anchorMarkInfo.push_back(am);
        baseRec.locDesc = locDesc;
        si.baseList.emplace_back(std::move(baseRec));

        nextNode = nextNode->nextCl;
    }
}

GPOS::CursivePos::CursivePos(GPOS &h, GPOS::SubtableInfo &si) : AnchorPosBase(h, si) {
    LOffset size = cursive1Size();

    std::sort(si.baseList.begin(), si.baseList.end());

    GPOS::BaseGlyphRec *prevRec = NULL;
    cac->coverageBegin();
    for (auto &baseRec : si.baseList) {
        if (prevRec && (prevRec->gid == baseRec.gid)) {
            featGlyphDump(h.g, baseRec.gid, '\0', 0);
            hotMsg(h.g, hotERROR, "Cursive statement error in %s. A previous statement has already referenced glyph '%s'. [current at %s, previous at %s]",
                       h.g->error_id_text.c_str(), h.g->note.array, baseRec.locDesc.c_str(), prevRec->locDesc.c_str());
        } else {
            EntryExitRecord eeRec;
            cac->coverageAddGlyph(baseRec.gid);
            if (baseRec.anchorMarkInfo[0].format == 0) {
                eeRec.EntryAnchor = 0xFFFF; /* Can't use 0 as a flag, as 0 is the offset to the first anchor record */
            } else {
                eeRec.EntryAnchor = getAnchorOffset(baseRec.anchorMarkInfo[0]); /* this returns the offset from the start of the anchor list. To be adjusted later*/
            }
            if (baseRec.anchorMarkInfo[1].format == 0) {
                eeRec.ExitAnchor = 0xFFFF; /* Can't use 0 as a flag, as 0 is he offset to the first anchor record */
            } else {
                eeRec.ExitAnchor = getAnchorOffset(baseRec.anchorMarkInfo[1]);
            }
            entryExitRecords.push_back(eeRec);
        }
        prevRec = &baseRec;
    }
    Coverage = cac->coverageEnd(); /* coverageEnd does the sort by GID */
    size += entryExitRecords.size() * (2 * sizeof(uint16_t));
    endArrays = size;

    /* Now add the size of the anchor list*/
    auto &anchorRec = anchorList.back();
    size += anchorRec.offset;
    if (anchorRec.anchor.format == 2) {
        size += sizeof(uint16_t) * 4;
    } else {
        size += sizeof(uint16_t) * 3;
    }

    if (isExt()) {
        Coverage += size; /* Adjust offset */
        h.incExtOffset(size + cac->coverageSize());
    } else {
        h.incSubOffset(size);
    }

    h.checkOverflow("cursive attach table", h.subOffset(), "cursive positioning");
}

void GPOS::CursivePos::fill(GPOS &h, GPOS::SubtableInfo &si) {
    h.AddSubtable(std::move(std::make_unique<CursivePos>(h, si)));
}

void GPOS::CursivePos::write(OTL *h) {
    LOffset adjustment = 0; /* (Linux compiler complains) */

    if (!isExt())
        adjustment = h->subOffset() - offset;

    Coverage += adjustment; /* Adjust offset */

    LOffset anchorListOffset = endArrays;

    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2((uint16_t)entryExitRecords.size());
    h->checkOverflow("cursive coverage table", h->subOffset(), "cursive positioning");
    for (auto &eeRec : entryExitRecords) {
        if (eeRec.EntryAnchor == 0xFFFF) {
            OUT2((Offset)0);
        } else {
            OUT2((Offset)(eeRec.EntryAnchor + anchorListOffset));
        }

        if (eeRec.ExitAnchor == 0xFFFF) {
            OUT2((Offset)0);
        } else {
            OUT2((Offset)(eeRec.ExitAnchor + anchorListOffset));
        }
    }

    /* Now write out the anchor list */
    for (auto &an : anchorList) {
        auto &anchorRec = an.anchor;
        if (anchorRec.format == 0)
            continue; /* This is an anchor record that never got filled in, or was specified as a NULL anchor. */
        OUT2(anchorRec.format);
        OUT2(anchorRec.x);
        OUT2(anchorRec.y);
        if (anchorRec.format == 2) {
            OUT2(anchorRec.contourpoint);
        }
    }

    if (isExt())
        cac->coverageWrite();
}
