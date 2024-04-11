
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Glyph substitution table.
 */
#include "GSUB.h"

#include <cassert>
#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "hotmap.h"
#include "name.h"

/* --------------------------- Standard Functions -------------------------- */

void GSUBNew(hotCtx g) {
    g->ctx.GSUBp = new GSUB(g);
}

int GSUBFill(hotCtx g) {
    return g->ctx.GSUBp->Fill();
}

int GSUB::Fill() {
    return fillOTL(g->convertFlags & HOT_ALLOW_STUB_GSUB);
}

void GSUBWrite(hotCtx g) {
    g->ctx.GSUBp->Write();
}

void GSUB::Write() {
    writeOTL();
}

void GSUBReuse(hotCtx g) {
    delete g->ctx.GSUBp;
    g->ctx.GSUBp = new GSUB(g);
}

void GSUBFree(hotCtx g) {
    delete g->ctx.GSUBp;
    g->ctx.GSUBp = nullptr;
}

/* ------------------------ Supplementary Functions ------------------------ */

#define DUMP_CH(ch, print)             \
    do {                               \
        if (print)                     \
            fprintf(stderr, "%c", ch); \
        else                           \
            g->note.push_back(ch);     \
    } while (0)

void GSUB::LigatureTarg::dumpAsPattern(hotCtx g, int ch, bool print) const {
    DUMP_CH('{', print);
    bool first = true;
    for (auto &gid : gids) {
        if (!first)
            DUMP_CH(' ', print);
        first = false;
        g->ctx.feat->dumpGlyph(gid, -1, print);
    }
    DUMP_CH('}', print);
    if (ch >= 0)
        DUMP_CH(ch, print);
}

/* Begin new feature (can be called multiple times for same feature) */

void GSUB::FeatureBegin(Tag script, Tag language, Tag feature) {
    DF(2, (stderr, "\n"));
#if 1
    DF(1, (stderr, "{ GSUB '%c%c%c%c', '%c%c%c%c', '%c%c%c%c'\n",
           TAG_ARG(script), TAG_ARG(language), TAG_ARG(feature)));
#endif
    nw.script = script;
    nw.language = language;
    nw.feature = feature;
}

GSUB::Subtable::Subtable(GSUB &h, SubtableInfo &si) :
    OTL::Subtable(&h, &si, h.g->error_id_text,
                  si.lkpType == GSUBFeatureNameParam || si.lkpType == GSUBCVParam) {}

#if 0

static void rulesDump(hotCtx g, GSUB::SubtableInfo &si) {
    fprintf(stderr, "# Dump lookupType %d rules:\n", si.lkpType);
    for (uint32_t i = 0; i < si.rules.size(); i++) {
        auto &rule = si.rules[i];

        fprintf(stderr, "  [%d] ", i);
        g->ctx.feat->dumpPattern(*rule.targ, ' ', 1);
    }
}

#endif

/* Begin new lookup */

void GSUB::LookupBegin(uint32_t lkpType, uint32_t lkpFlag, Label label,
                       bool useExtension, uint16_t useMarkSetIndex) {
    DF(2, (stderr,
           " { GSUB lkpType=%s%d lkpFlag=%d label=%x\n",
           useExtension ? "EXTENSION:" : "", lkpType, lkpFlag, label));

    nw.reset(lkpType, lkpFlag, label, useExtension, useMarkSetIndex);
}

/* End lookup */

void GSUB::LookupEnd(SubtableInfo *si) {
    DF(2, (stderr, " } GSUB\n"));

    if (si == nullptr)
        si = &nw;

    /* Return if simply a reference */
    if (IS_REF_LAB(si->label)) {
        AddSubtable(std::move(std::make_unique<Subtable>(*this, *si)));
        return;
    }

    if (g->hadError)
        return;

    switch (si->lkpType) {
        case GSUBSingle:
            SingleSubst::fill(*this, *si);
            break;

        case GSUBMultiple:
            MultipleSubst::fill(*this, *si);
            break;

        case GSUBAlternate:
            AlternateSubst::fill(*this, *si);
            break;

        case GSUBLigature:
            LigatureSubst::fill(*this, *si);
            break;

        case GSUBChain:
            ChainSubst::fill(*this, *si);
            break;

        case GSUBReverse:
            ReverseSubst::fill(*this, *si);
            break;

        case GSUBFeatureNameParam:
            FeatureNameParam::fill(*this, *si);
            break;

        case GSUBCVParam:
            CVParam::fill(*this, *si);
            break;

        default:
            /* Can't get here, but it is a useful check for future development. */
            g->logger->log(sFATAL, "unknown GSUB lkpType <%d> in %s.", si->lkpType, g->error_id_text.c_str());
    }

    checkOverflow("lookup subtable", subOffset(), "substitution");
    // XXX recycle rules
    /* This prevents the rules from being re-used unintentionally in the  */
    /* case where an empty GSUB feature is called for; because it is      */
    /* empty, the table type doesn't get correctly assigned, and the code */
    /* comes through here.                                                */
    si->rules.clear();
}

/* Performs no action but brackets feature calls */
void GSUB::FeatureEnd() {
    DF(2, (stderr, "} GSUB\n"));
}

/* Add rule (enumerating if necessary) to subtable si */

void GSUB::addSubstRule(SubtableInfo &si, GPat::SP targ, GPat::SP repl) {
#if HOT_DEBUG
    if (DF_LEVEL(g) >= 2) {
        DF(2, (stderr, "  * GSUB RuleAdd "));
        g->ctx.feat->dumpPattern(*targ, ' ', 1);
        if (repl != NULL) {
            g->ctx.feat->dumpPattern(*repl, '\n', 1);
        }
    }
#endif

    /* Add rule(s), enumerating if not supported by the OT format */
    if (si.lkpType == GSUBSingle) {
        auto &rglyphs = repl->classes[0].glyphs;
        auto ri = rglyphs.begin();
        for (GID tg : targ->classes[0].glyphs) {
            auto [i, b] = si.singles.emplace(tg, ri->gid);
            if (!b) {
                if (i->second == ri->gid) {
                    g->ctx.feat->dumpGlyph(tg, ',', 0);
                    g->note.push_back(' ');
                    g->ctx.feat->dumpGlyph(ri->gid, '\0', 0);
                    g->logger->log(sINFO,
                                   "Removing duplicate single substitution "
                                   "in %s: %s",
                                   g->error_id_text.c_str(),
                                   g->getNote());
                } else {
                    g->ctx.feat->dumpGlyph(tg, '\0', 0);
                    g->logger->log(sFATAL,
                                   "Duplicate target glyph for single "
                                   "substitution in %s: %s",
                                   g->error_id_text.c_str(), g->getNote());
                }
            }
            // If repl is a glyph use it for all entries in targ
            if (rglyphs.size() > 1)
                ri++;
        }
        return;
    } else if (si.lkpType == GSUBLigature) {
        assert(repl->is_glyph());
        auto rgid = repl->classes[0].glyphs[0].gid;
        std::vector<GPat::ClassRec*> tcp;
        for (auto &cr : targ->classes)
            tcp.push_back(&cr);
        GPat::CrossProductIterator cpi {tcp};
        std::vector<GID> gids;
        while (cpi.next(gids)) {
            auto [i, b] = si.ligatures.emplace(std::move(gids), rgid);
            if (!b) {
                if (i->second == rgid) {
                    i->first.dumpAsPattern(g, ' ', false);
                    g->note.push_back(' ');
                    g->ctx.feat->dumpGlyph(rgid, '\0', 0);
                    g->logger->log(sINFO,
                                   "Removing duplicate ligature substitution "
                                   "in %s: %s",
                                   g->error_id_text.c_str(),
                                   g->getNote());
                } else {
                    i->first.dumpAsPattern(g, '\0', false);
                    g->logger->log(sFATAL,
                                   "Duplicate target sequence but different replacement "
                                   "glyphs in ligature substitutions in %s: %s",
                                   g->error_id_text.c_str(), g->getNote());
                }
            }
        }
    } else {
        /* Add whole rule intact (no enumeration needed) */
        si.rules.emplace_back(std::move(targ), std::move(repl));
    }
}

void GSUB::RuleAdd(GPat::SP targ, GPat::SP repl) {
    if (g->hadError)
        return;

    addSubstRule(nw, std::move(targ), std::move(repl));
}

/* Break the subtable at this point. Return 0 if successful, else 1. */

bool GSUB::SubtableBreak() {
    return true;
}

void GSUB::SetFeatureNameID(Tag feat, uint16_t nameID) {
    featNameID.insert({feat, nameID});
}

void GSUB::AddFeatureNameParam(uint16_t nameID) {
    nw.paramNameID = nameID;
}

GSUB::FeatureNameParam::FeatureNameParam(GSUB &h, SubtableInfo &si,
                                         uint16_t nameID) :
                                         Subtable(h, si), nameID(nameID) {}

void GSUB::FeatureNameParam::fill(GSUB &h, SubtableInfo &si) {
    uint16_t ssNumber = ((si.feature >> 8 & 0xFF) - (int)'0') * 10;
    ssNumber += (si.feature & 0xFF) - (int)'0';
    if (((si.feature >> 24 & 0xFF) == (int)'s') &&
        ((si.feature >> 16 & 0xFF) == (int)'s') &&
        (ssNumber <= 99)) {
        if (si.paramNameID != 0) {
            uint16_t nameIDPresent = verifyDefaultNames(h.g, si.paramNameID);
            if (nameIDPresent && (nameIDPresent & MISSING_WIN_DEFAULT_NAME)) {
                h.g->logger->log(sFATAL,
                                 "Missing Windows default name for 'featureNames' nameid %i in %s.",
                                 si.paramNameID, h.g->error_id_text.c_str());
            }
        }
    } else {
        h.g->logger->log(sFATAL,
                         "A 'featureNames' block is only allowed in Stylistic Set (ssXX) features; it is being used in %s.",
                         h.g->error_id_text.c_str());
    }
    std::unique_ptr<GSUB::Subtable> s = std::make_unique<FeatureNameParam>(h, si, si.paramNameID);
    h.incFeatParamOffset(FeatureNameParam::size());
    h.AddSubtable(std::move(s));
}

void GSUB::FeatureNameParam::write(OTL *h) {
    OUT2(subformat());
    OUT2(nameID);
}

GSUB::CVParam::CVParam(GSUB &h, SubtableInfo &si, CVParameterFormat &&p) :
                                   Subtable(h, si), params(std::move(p)) {}

void GSUB::CVParam::fill(GSUB &h, SubtableInfo &si) {
    uint16_t nameIDs[4];
    uint16_t cvNumber = ((si.feature >> 8 & 0xFF) - (int)'0') * 10;
    cvNumber += (si.feature & 0xFF) - (int)'0';
    if (((si.feature >> 24 & 0xFF) == (int)'c') &&
        ((si.feature >> 16 & 0xFF) == (int)'v') &&
        (cvNumber <= 99)) {
        nameIDs[0] = si.cvParams.FeatUILabelNameID;
        nameIDs[1] = si.cvParams.FeatUITooltipTextNameID;
        nameIDs[2] = si.cvParams.SampleTextNameID;
        nameIDs[3] = si.cvParams.FirstParamUILabelNameID;
        for (int i = 0; i < 4; i++) {
            uint16_t nameid = nameIDs[i];
            if (nameid != 0) {
                uint16_t nameIDPresent = verifyDefaultNames(h.g, nameid);
                if (nameIDPresent && nameIDPresent & MISSING_WIN_DEFAULT_NAME) {
                    h.g->logger->log(sFATAL,
                                     "Missing Windows default name for 'cvParameters' nameid %i in %s.",
                                     nameid, h.g->error_id_text.c_str());
                }
            }
        }
    } else {
        h.g->logger->log(sFATAL,
                         "A 'cvParameters' block is only allowed in Character Variant (cvXX) features; it is being used in %s.",
                         h.g->error_id_text.c_str());
    }
    auto sz = si.cvParams.size();
    h.AddSubtable(std::move(std::make_unique<CVParam>(h, si, std::move(si.cvParams))));
    h.incFeatParamOffset(sz);
}

void GSUB::CVParam::write(OTL *h) {
    OUT2(subformat());
    OUT2(params.FeatUILabelNameID);
    OUT2(params.FeatUITooltipTextNameID);
    OUT2(params.SampleTextNameID);
    OUT2(params.NumNamedParameters);
    OUT2(params.FirstParamUILabelNameID);
    OUT2((uint16_t)params.charValues.size());
    for (auto cv : params.charValues) {
        uint32_t uv = cv;
        int8_t val1 = (int8_t)(uv >> 16);
        uint16_t val2 = (uint16_t)(uv & 0x0000FFFF);
        OUT1(val1);
        OUT2(val2);
    }
}

void GSUB::AddCVParam(CVParameterFormat &&params) {
    nw.cvParams.swap(params);
}

/* -------------------------- Single Substitution -------------------------- */

GSUB::SingleSubst::SingleSubst(GSUB &h, SubtableInfo &si) : Subtable(h, si) {}

Offset GSUB::SingleSubst::fillSingleCoverage(SubtableInfo &si) {
    cac->coverageBegin();
    for (auto [t, _] : si.singles) {
        cac->coverageAddGlyph(t);
    }
    return cac->coverageEnd();
}

GSUB::SingleSubst::Format1::Format1(GSUB &h, SubtableInfo &si, int delta) : SingleSubst(h, si) {
    LOffset sz = size();
    Coverage = fillSingleCoverage(si); /* Adjusted later */
    DeltaGlyphID = delta;

    if (isExt()) {
        Coverage += sz; /* Final value */
        h.incExtOffset(sz + cac->coverageSize());
    } else {
        h.incSubOffset(sz);
    }
}

GSUB::SingleSubst::Format2::Format2(GSUB &h, SubtableInfo &si) : SingleSubst(h, si) {
    LOffset sz = size(si.singles.size());

    Coverage = fillSingleCoverage(si); /* Adjusted later */
    for (auto [_, r] : si.singles)
        gids.push_back(r);

    if (isExt()) {
        Coverage += sz; /* Final value */
        h.incExtOffset(sz + cac->coverageSize());
    } else {
        h.incSubOffset(sz);
    }
}

void GSUB::SingleSubst::fill(GSUB &h, SubtableInfo &si) {
    h.updateMaxContext(1);

    if (si.feature == vrt2_) {
        h.g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;

        for (auto [t, r] : si.singles) {
            auto &hotgi = h.g->glyphs[r];
            if (!hotgi.vAdv.isInitialized()) {
                /* don't set it if it has already been set, as with vmtx overrides */
                // XXX this is wrong -- need the whole hAdv here from hmtx.
                hotgi.vAdv.addValue(h.g->glyphs[t].hAdv);
            }
        }
    }

    /* Determine format and fill subtable */
    bool first = true;
    int32_t delta;
    for (auto [t, r] : si.singles) {
        if (first) {
            delta = r - t;
            first = false;
        } else if (delta != r - t) {
            h.AddSubtable(std::move(std::make_unique<Format2>(h, si)));
            return;
        }
    }
    h.AddSubtable(std::move(std::make_unique<Format1>(h, si, delta)));
}

void GSUB::SingleSubst::Format1::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "single substitution");

    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2(DeltaGlyphID);

    if (isExt())
        cac->coverageWrite();
}

void GSUB::SingleSubst::Format2::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "single substitution");

    OUT2(subformat());
    OUT2((Offset)(Coverage));
    OUT2((uint16_t)gids.size());
    for (auto gid : gids)
        OUT2((GID)gid);

    if (isExt())
        cac->coverageWrite();
}

/* ------------------------- Multiple Substitution ------------------------- */

GSUB::MultipleSubst::MultipleSubst(GSUB &h, SubtableInfo &si, int64_t beg,
                                   int64_t end, uint32_t sz, uint32_t nSubs) : Subtable(h, si) {
#define AALT_STATS 1 /* Print aalt statistics with debug output */
#if HOT_DEBUG
    auto g = h.g;
    if (beg != 0 || end != (int64_t)si.rules.size() - 1) {
        DF(1, (stderr, "fillMultiple1() from %ld->%ld; totNumRules=%ld\n", beg, end,
               si.rules.size()));
    }
#endif /* HOT_DEBUG */

    int nSequences = end - beg + 1;

    LOffset offst = headerSize(nSequences);
    cac->coverageBegin();
    for (int i = 0; i < nSequences; i++) {
        auto &rule = si.rules[i + beg];
        MultSequence seq;

        assert(rule.targ->is_glyph());
        cac->coverageAddGlyph(rule.targ->classes[0].glyphs[0].gid);

        if (rule.repl != nullptr) {
            for (auto &cr : rule.repl->classes) {
                assert(cr.glyphs.size() == 1);
                seq.gids.push_back(cr.glyphs[0].gid);
            }
        }

        seq.offset = offst;
        offst += seq.size();
        sequences.emplace_back(std::move(seq));
    }

#if HOT_DEBUG
    if (offst != sz)
        g->logger->log(sFATAL, "[internal] fillSubstitute() size miscalculation");
#if AALT_STATS
    if (si.feature == aalt_) {
        DF(1, (stderr,
               "# aalt lkptype 3 subtbl: average %.2f repl gids "
               "per rule for %d rules. subtbl size: %hx\n",
               (double)nSubs / nSequences, nSequences, offst));
    }
#endif
#endif /* HOT_DEBUG */

    Coverage = cac->coverageEnd(); /* Adjusted later */
    if (isExt()) {
        Coverage += offst; /* Final value */
        h.incExtOffset(offst + cac->coverageSize());
    } else {
        h.incSubOffset(offst);
    }
    h.updateMaxContext(1);
}

/* Fill the currently accumulated alternate substitution subtable, auto-
   breaking into several subtables if needed. */

void GSUB::MultipleSubst::fill(GSUB &h, SubtableInfo &si) {
    int32_t sz = 0;
    uint32_t nSubs = 0;

    std::sort(si.rules.begin(), si.rules.end());
    int i = 0;
    for (size_t j = 0; j < si.rules.size(); j++) {
        auto &rule = si.rules[j];
        /* Check for duplicates */
        if (j != 0 && rule.targ->classes[0].glyphs[0] == si.rules[j-1].targ->classes[0].glyphs[0]) {
            h.g->ctx.feat->dumpGlyph(rule.targ->classes[0].glyphs[0].gid, '\0', 0);
            h.g->logger->log(sFATAL,
                             "Duplicate target glyph for multiple substitution in "
                             "%s: %s",
                             h.g->error_id_text.c_str(),
                             h.g->getNote());
        }

        /* Calculate nw size if this rule were included: */
        uint32_t nSubsNew = nSubs;
        nSubsNew += rule.repl != nullptr ? rule.repl->patternLen() : 0;
        int32_t sizeNew = size(j - i + 1, nSubsNew);

        if (sizeNew > 0xFFFF) {
            /* Just overflowed size; back up one rule */
            h.AddSubtable(std::move(std::make_unique<MultipleSubst>(h, si, i, j-1, sz, nSubs)));
            /* Initialize for next subtable */
            sz = nSubs = 0;
            i = j--;
        } else if (j == si.rules.size() - 1) {
            /* At end of array */
            h.AddSubtable(std::move(std::make_unique<MultipleSubst>(h, si, i, j, sizeNew, nSubsNew)));
        } else {
            /* Not ready for a subtable break yet */
            sz = sizeNew;
            nSubs = nSubsNew;
        }
    }
}

void GSUB::MultipleSubst::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "multiple substitution");

    OUT2(subformat());
    OUT2((Offset)(Coverage));
    OUT2((uint32_t)sequences.size());

    for (auto &seq : sequences)
        OUT2((uint16_t)seq.offset);
    for (auto &seq : sequences) {
        OUT2((uint16_t)seq.gids.size());
        for (auto gid : seq.gids)
            OUT2(gid);
    }

    if (isExt())
        cac->coverageWrite();
}

/* ------------------------- Alternate Substitution ------------------------ */

/* Create a subtable with rules from index [beg] to [end]. size: total size
   (excluding actual coverage). numAlts: total number of replacement glyphs. */

GSUB::AlternateSubst::AlternateSubst(GSUB &h, SubtableInfo &si,
                                     int64_t beg, int64_t end, uint32_t size,
                                     uint16_t numAlts) : Subtable(h, si) {
#define AALT_STATS 1 /* Print aalt statistics with debug output */

#if HOT_DEBUG
    auto g = h.g;
    if (beg != 0 || end != (int64_t) si.rules.size() - 1) {
        DF(1, (stderr, "fillAlt() from %ld->%ld; totNumRules=%ld\n", beg, end,
               si.rules.size()));
    }
#endif /* HOT_DEBUG */

    uint32_t nAltSets = end - beg + 1;

    LOffset offst = headerSize(nAltSets);
    cac->coverageBegin();
    for (uint32_t i = 0; i < nAltSets; i++) {
        auto &rule = si.rules[i + beg];
        AlternateSet altSet;

        assert(rule.targ->is_glyph());
        cac->coverageAddGlyph(rule.targ->classes[0].glyphs[0].gid);

        /* --- Fill an AlternateSet --- */
        for (GID gid : rule.repl->classes[0].glyphs)
            altSet.gids.push_back(gid);

        altSet.offset = offst;
        offst += altSet.size();
        altSets.emplace_back(std::move(altSet));
    }

#if HOT_DEBUG
    if (offst != size)
        g->logger->log(sFATAL, "[internal] fillAlternate() size miscalculation");
#if AALT_STATS
    if (si.feature == aalt_) {
        DF(1, (stderr,
               "# aalt lkptype 3 subtbl: average %.2f repl gids "
               "per rule for %d rules. subtbl size: %hx\n",
               (double)numAlts / nAltSets, nAltSets, offst));
    }
#endif
#endif /* HOT_DEBUG */

    Coverage = cac->coverageEnd(); /* Adjusted later */
    if (isExt()) {
        Coverage += offst; /* Final value */
        h.incExtOffset(offst + cac->coverageSize());
    } else {
        h.incSubOffset(offst);
    }
    h.updateMaxContext(1);
}

/* Fill the currently accumulated alternate substitution subtable, auto-
   breaking into several subtables if needed. */

void GSUB::AlternateSubst::fill(GSUB &h, SubtableInfo &si) {
    long sz = 0;
    uint32_t numAlts = 0;

    /* Sort by target glyph */
    std::sort(si.rules.begin(), si.rules.end());
#if 0
    aaltDump(h, si);
#endif
    int i = 0;
    for (size_t j = 0; j < si.rules.size(); j++) {
        auto &rule = si.rules[j];

        /* Check for duplicates */
        if (j != 0 && rule.targ->classes[0].glyphs[0] == si.rules[j-1].targ->classes[0].glyphs[0]) {
            h.g->ctx.feat->dumpGlyph(rule.targ->classes[0].glyphs[0].gid, '\0', 0);
            h.g->logger->log(sFATAL,
                             "Duplicate target glyph for alternate substitution in "
                             "%s: %s",
                             h.g->error_id_text.c_str(),
                             h.g->getNote());
        }

        /* Calculate new size if this rule were included: */
        uint32_t numAltsNew = numAlts;
        numAltsNew += rule.repl->classSize();
        long sizeNew = size(j - i + 1, numAltsNew);

        if (sizeNew > 0xFFFF) {
            /* Just overflowed size; back up one rule */
            h.AddSubtable(std::move(std::make_unique<AlternateSubst>(h, si, i, j-1, sz, numAlts)));
            /* Initialize for next subtable */
            sz = numAlts = 0;
            i = j--;
        } else if (j == si.rules.size() - 1) {
            /* At end of array */
            h.AddSubtable(std::move(std::make_unique<AlternateSubst>(h, si, i, j, sizeNew, numAltsNew)));
        } else {
            /* Not ready for a subtable break yet */
            sz = sizeNew;
            numAlts = numAltsNew;
        }
    }
}

void GSUB::AlternateSubst::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "alternate substitution");
    OUT2(subformat());
    OUT2((Offset)(Coverage));
    OUT2((uint16_t)altSets.size());

    for (auto &set : altSets)
        OUT2((uint16_t)set.offset);
    for (auto &set : altSets) {
        OUT2((uint16_t)set.gids.size());
        for (auto gid : set.gids)
            OUT2(gid);
    }

    if (isExt())
        cac->coverageWrite();
}

/* ------------------------- Ligature Substitution ------------------------- */

GSUB::LigatureSubst::LigatureSubst(GSUB &h, SubtableInfo &si) : Subtable(h, si) {
    LigatureSet ligSet;
    int32_t lastgid = -1;
    cac->coverageBegin();
    // Add coverage and populate ligatureSets (without offsets)
    for (auto [l, rg] : si.ligatures) {
        GID gid = l.gids[0];
        if (gid != lastgid) {
            if (lastgid != -1) {
                ligatureSets.emplace_back(std::move(ligSet));
                ligSet.reset();
            }
            cac->coverageAddGlyph(gid);
            lastgid = gid;
        }
        LigatureGlyph lg {rg};
        lg.components.insert(lg.components.begin(), l.gids.begin() + 1, l.gids.end());
        h.updateMaxContext(lg.components.size() + 1);
        ligSet.ligatures.emplace_back(std::move(lg));
    }
    ligatureSets.emplace_back(std::move(ligSet));

    // Calculate offsets
    LOffset offst = headerSize(ligatureSets.size());
    for (auto &ls : ligatureSets) {
        LOffset offLig = ls.size();
        for (auto &lg : ls.ligatures) {
            lg.offset = offLig;
            offLig += lg.size();
        }
        ls.offset = offst;
        offst += offLig;
    }

    h.checkOverflow("lookup subtable", offst, "ligature substitution");
    Coverage = cac->coverageEnd(); /* Adjusted later */
    if (isExt()) {
        Coverage += offst; /* Final value */
        h.incExtOffset(offst + cac->coverageSize());
    } else {
        h.incSubOffset(offst);
    }
}

void GSUB::LigatureSubst::fill(GSUB &h, SubtableInfo &si) {
    h.AddSubtable(std::move(std::make_unique<LigatureSubst>(h, si)));
}

void GSUB::LigatureSubst::write(OTL *h) {
    if (!isExt())
        Coverage += h->subOffset() - offset; /* Adjust offset */

    h->checkOverflow("coverage table", Coverage, "ligature substitution");
    OUT2(subformat());
    OUT2((Offset)Coverage);
    OUT2((uint16_t)ligatureSets.size());

    for (auto &ls : ligatureSets)
        OUT2((Offset)ls.offset);
    for (auto &ls : ligatureSets) {
        OUT2((uint16_t)ls.ligatures.size());
        for (auto &l : ls.ligatures)
            OUT2(l.offset);
        for (auto &l : ls.ligatures) {
            OUT2(l.ligGlyph);
            OUT2((uint16_t)l.components.size() + 1);  // first component in Coverage
            for (auto gid : l.components)
                OUT2(gid);
        }
    }

    if (isExt())
        cac->coverageWrite();
}

/* ------------------ Chaining Contextual Substitution --------------------- */

/* Tries to add rule to current anon subtbl. If successful, returns 1, else 0.
   If rule already exists in subtbl, recycles targ and repl */

bool GSUB::addSingleToAnonSubtbl(SubtableInfo &si, GPat::ClassRec &tcr,
                                 GPat::ClassRec &rcr) {
    std::map<GID, GID> needed;

    assert(si.lkpType == GSUBSingle);
    /* Determine which rules already exist in the subtable */
    auto ri = rcr.glyphs.begin();
    for (GID tg : tcr.glyphs) {
        auto i = si.singles.find(tg);
        if (i != si.singles.end()) {
            if (i->second != ri->gid)
                return false;
        } else {
            // XXX warn about dups?
            needed.insert({tg, ri->gid});
        }
        if (rcr.glyphs.size() > 1)
            ri++;
    }

    for (auto [tgid, rgid] : needed)
        si.singles.emplace(tgid, rgid);

    return true;
}

bool GSUB::addLigatureToAnonSubtbl(SubtableInfo &si, GPat::SP &targ, GPat::SP &repl) {
    assert(repl->is_glyph());
    auto rgid = repl->classes[0].glyphs[0].gid;

    std::vector<GPat::ClassRec*> tcp;
    for (auto &cr : targ->classes)
        tcp.push_back(&cr);
    GPat::CrossProductIterator cpi {tcp};
    std::vector<GID> gids;
    std::set<LigatureTarg> needed;

    while (cpi.next(gids)) {
        LigatureTarg lt {std::move(gids)};
        auto i = si.ligatures.find(lt);
        if (i != si.ligatures.end()) {
            if (i->second != rgid)
                return false;
        } else {
            // XXX What about dups?
            needed.insert(std::move(lt));
        }
    }

    for (auto &lt : needed)
        si.ligatures.emplace(lt, rgid);

    return true;
}

/* Add the "anonymous" rule that occurs in a substitution within a chaining
   contextual rule. Return the label of the anonymous lookup */

Label GSUB::addAnonRule(SubtableInfo &cur_si, GPat::SP targ, GPat::SP repl) {
    int lkpType;

    if (targ->patternLen() == 1) {
        if (repl->patternLen() > 1) {
            lkpType = GSUBMultiple;
        } else {
            lkpType = GSUBSingle;
        }
    } else {
        lkpType = GSUBLigature;
    }

    if (anonSubtable.size() > 0) {
        auto &si = anonSubtable.back();
        if ((si.lkpType == lkpType) && (si.lkpFlag == cur_si.lkpFlag) &&
            (si.markSetIndex == cur_si.markSetIndex) &&
            (si.parentFeatTag == cur_si.feature)) {
            if (lkpType == GSUBSingle &&
                addSingleToAnonSubtbl(si, targ->classes[0], repl->classes[0])) {
                return si.label;
            } else if (lkpType == GSUBLigature &&
                       addLigatureToAnonSubtbl(si, targ, repl)) {
                return si.label;
            }
        }
    }

    SubtableInfo asi;

    /* Must create new anon subtable */
    asi.script = cur_si.script;
    asi.language = cur_si.language;
    asi.lkpType = lkpType;
    asi.lkpFlag = cur_si.lkpFlag;
    asi.markSetIndex = cur_si.markSetIndex;
    asi.label = g->ctx.feat->getNextAnonLabel();
    asi.parentFeatTag = nw.feature;
    asi.useExtension = cur_si.useExtension;

    addSubstRule(asi, std::move(targ), std::move(repl));

    anonSubtable.emplace_back(std::move(asi));

    return anonSubtable.back().label;
}

/* Create anonymous lookups (referred to only from within chain ctx lookups) */

void GSUB::createAnonLookups() {
    for (auto &si : anonSubtable) {
        DF(2, (stderr, "\n"));
#if 1
        DF(1, (stderr, "{ GSUB '%c%c%c%c', '%c%c%c%c', '%c%c%c%c'\n",
               TAG_ARG(si.script), TAG_ARG(si.language), TAG_ARG(si.feature)));
#endif
        DF(2, (stderr,
               " { GSUB lkpType=%s%d lkpFlag=%d label=%x\n",
               si.useExtension ? "EXTENSION:" : "", si.lkpType, si.lkpFlag, si.label));
        // so that these will sort to the end of the subtable array
        // and will not be considered for adding to the FeatureList table
        g->error_id_text = std::string("feature '") + TAG_STR(si.parentFeatTag) + std::string("'");
        si.script = si.language = si.feature = TAG_UNDEF;
#if HOT_DEBUG
        if (DF_LEVEL(g) >= 2) {
            for (auto [t, r] : si.singles) {
                DF(2, (stderr, "  * GSUB RuleAdd "));
                g->ctx.feat->dumpGlyph(t, ' ', true);
                g->ctx.feat->dumpGlyph(r, '\n', true);
            }
            for (auto [l, r] : si.ligatures) {
                DF(2, (stderr, "  * GSUB RuleAdd "));
                l.dumpAsPattern(g, ' ', true);
                g->ctx.feat->dumpGlyph(r, '\n', true);
            }
            for (auto &rule : si.rules) {
                DF(2, (stderr, "  * GSUB RuleAdd "));
                g->ctx.feat->dumpPattern(*rule.targ, ' ', 1);
                if (rule.repl != NULL) {
                    g->ctx.feat->dumpPattern(*rule.repl, '\n', 1);
                }
            }
        }
#endif
        LookupEnd(&si);
        FeatureEnd();
    }
}

/* Fill chaining contextual subtable format 3 */

GSUB::ChainSubst::ChainSubst(GSUB &h, SubtableInfo &si, SubstRule &rule) : Subtable(h, si) {
    std::vector<GPat::ClassRec*> backs, inputs, looks;
    GPat::SP marks = std::make_unique<GPat>();
    int iSeq = 0;  // offset into inputs of first mark
    uint32_t nSubst = (rule.repl != nullptr) ? 1 : 0;

    /* Set counts of and pointers to Back, Input, Look, Marked areas */
    for (auto &cr : rule.targ->classes) {
        if (cr.backtrack)
            backs.push_back(&cr);
        else if (cr.input) {
            if (cr.marked) {
                if (marks->patternLen() == 0)
                    iSeq = inputs.size();
                nSubst += cr.lookupLabels.size();
                marks->addClass(cr);
            }
            inputs.push_back(&cr);
        } else if (cr.lookahead)
            looks.push_back(&cr);
    }

    LOffset sz = chain3size(backs.size(), inputs.size(), looks.size(), nSubst);
    LOffset o = isExt() ? sz : 0;

    h.setCoverages(backtracks, cac, backs, o);
    h.setCoverages(inputGlyphs, cac, inputs, o);
    h.setCoverages(lookaheads, cac, looks, o);

    if (nSubst > 0) {
        if (rule.repl != NULL) {
            /* There is only a single replacement rule, not using direct lookup references */
            lookupRecords.emplace_back(iSeq, h.addAnonRule(si, std::move(marks), std::move(rule.repl)));
        } else {
            lookupRecords.reserve(nSubst);
            int i = 0;
            for (auto &mcr : marks->classes) {
                for (auto ll : mcr.lookupLabels)
                    lookupRecords.emplace_back(i, ll);
                i++;
            }
        }
    }

    h.updateMaxContext(inputs.size() + looks.size());

    if (isExt())
        h.incExtOffset(sz + cac->coverageSize());
    else
        h.incSubOffset(sz);
}

void GSUB::ChainSubst::fill(GSUB &h, SubtableInfo &si) {
    for (auto &rule : si.rules) {
        h.AddSubtable(std::move(std::make_unique<ChainSubst>(h, si, rule)));
        h.checkOverflow("lookup subtable", h.subOffset(), "chain contextual substitution");
    }
}

void GSUB::ChainSubst::write(OTL *h) {
    LOffset adjustment = 0; /* (Linux compiler complains) */
    if (!isExt())
        adjustment = h->subOffset() - offset;

    OUT2(subformat());

    OUT2((uint16_t)backtracks.size());

    if (h->g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
        /* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
        for (auto &bt : backtracks) {
            if (!isExt())
                bt += adjustment;
            h->checkOverflow("backtrack coverage table", bt, "chain contextual substitution");
            OUT2((uint16_t)bt);
        }
    } else {
        /* do it per OpenType spec 1.5 */
        for (auto ri = backtracks.rbegin(); ri != backtracks.rend(); ri++) {
            if (!isExt())
                *ri += adjustment;
            h->checkOverflow("backtrack coverage table", *ri, "chain contextual substitution");
            OUT2((uint16_t)*ri);
        }
    }

    OUT2((uint16_t)inputGlyphs.size());
    for (auto &ig : inputGlyphs) {
        if (!isExt())
            ig += adjustment;
        h->checkOverflow("input coverage table", ig, "chain contextual substitution");
        OUT2((uint16_t)ig);
    }

    OUT2((uint16_t)lookaheads.size());
    for (auto &la : lookaheads) {
        if (!isExt())
            la += adjustment;
        h->checkOverflow("lookahead coverage table", la, "chain contextual substitution");
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

GSUB::ReverseSubst::ReverseSubst(GSUB &h, SubtableInfo &si, SubstRule &rule) : Subtable(h, si) {
    std::vector<GPat::ClassRec*> backs, looks, inputs;

    for (auto &cr : rule.targ->classes) {
        if (cr.backtrack)
            backs.push_back(&cr);
        else if (cr.input)
            inputs.push_back(&cr);
        else if (cr.lookahead)
            looks.push_back(&cr);
    }

    // When we call CoverageEnd, the input coverage will be sorted in
    // GID order. The replacement glyph list must also be sorted in that
    // order, so we sort both lists in advance so CoverageEnd won't
    // change the correspondence
    std::vector<std::pair<GID, GID>> subs;
    if (rule.repl != NULL) {
        auto &rglyphs = rule.repl->classes[0].glyphs;
        assert(inputs[0]->glyphs.size() == rglyphs.size());
        auto rgi = rglyphs.begin();
        for (GID gid : inputs[0]->glyphs) {
            subs.emplace_back(gid, rgi->gid);
            rgi++;
        }
        std::sort(subs.begin(), subs.end());
    }

    cac->coverageBegin();
    for (auto [tg, _] : subs)
        cac->coverageAddGlyph(tg);

    InputCoverage = cac->coverageEnd(); /* Adjusted later */

    LOffset sz = rchain1size(backs.size(), looks.size(), subs.size());
    LOffset o = isExt() ? sz : 0;

    h.setCoverages(backtracks, cac, backs, o);
    h.setCoverages(lookaheads, cac, looks, o);

    for (auto [_, rg] : subs)
        substitutes.push_back(rg);

    h.updateMaxContext(inputs.size() + looks.size());

    if (isExt())
        h.incExtOffset(sz + cac->coverageSize());
    else
        h.incSubOffset(sz);
}

void GSUB::ReverseSubst::fill(GSUB &h, SubtableInfo &si) {
    for (auto &rule : si.rules) {
        h.AddSubtable(std::move(std::make_unique<ReverseSubst>(h, si, rule)));
        h.checkOverflow("lookup subtable", h.subOffset(), "reverse chain contextual substitution");
    }
}

void GSUB::ReverseSubst::write(OTL *h) {
    LOffset adjustment = 0; /* (Linux compiler complains) */

    if (!isExt())
        adjustment = h->subOffset() - offset;

    OUT2(subformat());

    if (!isExt())
        InputCoverage += adjustment;

    OUT2((uint16_t)InputCoverage);
    OUT2((uint16_t)backtracks.size());

    if (h->g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
        /* do it per OpenType spec 1.4 and earlier,as In Design 2.0 and earlier requires. */
        for (auto &bt : backtracks) {
            if (!isExt())
                bt += adjustment;
            h->checkOverflow("backtrack coverage table", bt, "reverse chain contextual substitution");
            OUT2((uint16_t)bt);
        }
    } else {
        /* do it per OpenType spec 1.5 */
        for (auto ri = backtracks.rbegin(); ri != backtracks.rend(); ri++) {
            if (!isExt())
                *ri += adjustment;
            h->checkOverflow("backtrack coverage table", *ri, "reverse chain contextual substitution");
            OUT2((uint16_t)*ri);
        }
    }

    OUT2((uint16_t)lookaheads.size());
    for (auto &la : lookaheads) {
        if (!isExt())
            la += adjustment;
        h->checkOverflow("lookahead coverage table", la, "reverse chain contextual substitution");
        OUT2((uint16_t)la);
    }

    OUT2((uint16_t)substitutes.size());
    for (auto gid : substitutes)
        OUT2(gid);

    if (isExt())
        cac->coverageWrite();
}
