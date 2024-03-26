/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Shared OpenType Layout support.
 */

#include "otl.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "OS_2.h"
#include "GDEF.h"

CoverageAndClass::CoverageRecord::CoverageRecord(Offset o, std::set<GID> &gl) : offset(o) {
    glyphs.swap(gl);

    uint16_t i = 0;
    RangeRecord range;
    for (auto gid : glyphs) {
        if (gid == range.End + 1) {
            range.End = gid;
        } else {
            if (range.Start != GID_UNDEF)
                ranges.emplace_back(range);
            range.Start = range.End = gid;
            range.startCoverageIndex = i;
        }
        i++;
    }
    if (range.Start != GID_UNDEF)
        ranges.emplace_back(range);

    if (cov1size() <= cov2size())  // Decide on format
        ranges.clear();
}

void CoverageAndClass::CoverageRecord::write(hotCtx g) {
    if (ranges.size() > 0) {  // format 2
        hotOut2(g, 2);
        hotOut2(g, ranges.size());
        for (auto &range : ranges) {
            hotOut2(g, range.Start);
            hotOut2(g, range.End);
            hotOut2(g, range.startCoverageIndex);
        }
    } else {  // format 1
        hotOut2(g, 1);
        hotOut2(g, glyphs.size());
        for (auto gid : glyphs)
            hotOut2(g, gid);
    }
}

Offset CoverageAndClass::coverageFill() {
    coverage.records.emplace_back(coverage.size, coverage.current);
    coverage.size += coverage.records.back().size();

    if (coverage.size > 0xFFFF)
        g->logger->log(sFATAL, "coverage section too large (%0x)", coverage.size);

    return coverage.records.back().offset;
}

void CoverageAndClass::coverageWrite() {
    for (auto &record : coverage.records)
        record.write(g);
}

void CoverageAndClass::coverageBegin() {
    coverage.current.clear();
}

void CoverageAndClass::coverageAddGlyph(GID gid, bool warn) {
    auto [i, b] = coverage.current.insert(gid);
    if (!b) {
        if (warn) {
            g->ctx.feat->dumpGlyph(gid, 0, 0);
            g->logger->log(sINFO, "Removing duplicate glyph <%s>", g->getNote());
        }
#if HOT_DEBUG
        printf("duplicated glyph ['%d'] in coverage.\n", gid);
#endif
    }
}

Offset CoverageAndClass::coverageEnd() {
    for (auto &record : coverage.records) {
        if (record.glyphs == coverage.current) {
#if HOT_DEBUG
            coverage.reused++;
#if 0
            if (DF_LEVEL(g) >= 2) {
                auto lasti = coverage.current.rbegin();
                fprintf(stderr, "# Using coverage already present:");
                for (auto gid : coverage.current)
                    gidDump(g, gid, (gid != *lasti) ? ' ' : 0);
                fprintf(stderr, "\n");
            }
#endif
#endif
            return record.offset;
        }
    }

    return coverageFill();
}

CoverageAndClass::ClassRecord::ClassRecord(Offset o, std::map<GID, uint16_t> &m) : offset(o) {
    map.swap(m);

    if (map.empty())
        return;

    GID first = map.begin()->first;
    int32_t nGlyphs = map.rbegin()->first - first + 1;

    ClassRangeRecord range;

    for (auto i : map) {
        if (i.first == range.End + 1 && i.second == range.classId) {
            range.End = i.first;
        } else {
            if (range.Start != GID_UNDEF)
                ranges.emplace_back(range);
            range.Start = range.End = i.first;
            range.classId = i.second;
        }
    }
    if (range.Start != GID_UNDEF)
        ranges.emplace_back(range);

    if (cls1size(nGlyphs) < cls2size()) {
        startGlyph = first;
        values.insert(values.begin(), nGlyphs, (uint16_t) 0);
        for (auto i : map)
            values[i.first - startGlyph] = i.second;
        ranges.clear();
    }
}

void CoverageAndClass::ClassRecord::write(hotCtx g) {
    if (values.empty()) {  // Format 2
        hotOut2(g, 2);
        hotOut2(g, ranges.size());
        for (auto &range : ranges) {
            hotOut2(g, range.Start);
            hotOut2(g, range.End);
            hotOut2(g, range.classId);
        }
    } else {  // Format 1
        assert(startGlyph != GID_UNDEF && ranges.size() == 0);
        hotOut2(g, 1);
        hotOut2(g, startGlyph);
        hotOut2(g, values.size());
        for (auto classIndex : values)
            hotOut2(g, classIndex);
    }
}

Offset CoverageAndClass::classFill() {
    cls.records.emplace_back(cls.size, cls.current);
    cls.size += cls.records.back().size();

    if (cls.size > 0xFFFF) {
        g->logger->log(sFATAL, "class section too large (%0x)", cls.size);
    }

    return cls.records.back().offset;
}

void CoverageAndClass::classWrite() {
    for (auto &record : cls.records)
        record.write(g);
}

void CoverageAndClass::classBegin() {
    cls.current.clear();
}

void CoverageAndClass::classAddMapping(GID gid, uint32_t classId) {
    if (classId == 0)
        return;
    auto [i, b] = cls.current.insert({gid, classId});
    if (!b) {
#if HOT_DEBUG
        printf("duplicated glyph ['%d'] in class mapping.\n", gid);
#endif
    }
}

Offset CoverageAndClass::classEnd() {
    for (auto &record : cls.records) {
        if (record.map == cls.current) {
#if HOT_DEBUG
            cls.reused++;
#if 0
            if (DF_LEVEL(g) >= 2) {
                auto lasti = classObj.current.rbegin();
                fprintf(stderr, "# Using class already present:");
                for (auto i : classObj.current)
                    gidDump(g, i->first, 0);
                    fprintf(stderr, ":%u", i->second);
                fprintf(stderr, "\n");
            }
#endif
#endif
            return record.offset;
        }
    }

    return classFill();
}

#if HOT_DEBUG

void CoverageAndClass::dump() {
    fprintf(stderr,
            "# nCov: %ld, nClass: %ld; nCovReused: %d, nClassReused: %d\n"
            "# [offs relative to start of subtable section]\n",
            coverage.records.size(), cls.records.size(),
            coverage.reused, cls.reused);
}
#endif

#define FEAT_PARAM_ZERO 1

#if HOT_DEBUG
/* Initialize subtable. Not needed except for debug dumps */
void OTL::valDump(int16_t val, int16_t excep, bool isRef) {
    if (val == excep) {
        fprintf(stderr, "   * ");
    } else if (isRef) {
        fprintf(stderr, "   ->%hd ", val);
    } else {
        fprintf(stderr, "%4hd ", val);
    }
}

void OTL::tagDump(Tag tag, char ch) {
    if (tag == TAG_UNDEF) {
        fprintf(stderr, "****");
    } else {
        fprintf(stderr, "%c%c%c%c", TAG_ARG(tag));
    }
    if (ch != 0) {
        fprintf(stderr, "%c", ch);
    }
}

void OTL::Subtable::dump(typename std::vector<std::unique_ptr<Subtable>>::iterator sb,
                         uint32_t extLkpType) {
    tagDump(script, ' ');
    tagDump(language, ' ');
    tagDump(feature, ' ');
    fprintf(stderr, "   %2u/%hu|%-3x %4hx ",
            isExt() ? extLkpType : (uint32_t)lkpType,
            fmt(),
            (uint32_t)lkpFlag,
            offset);
    fprintf(stderr, "%4x ", label);

    typename std::vector<std::unique_ptr<Subtable>>::iterator uninit;
    valDump(span.script - sb, uninit - sb, false);
    valDump(span.language - sb, uninit - sb, false);
    valDump(span.feature - sb, uninit - sb, false);
    valDump(span.lookup - sb, uninit - sb, false);
    valDump(index.feature, -1, false);
    valDump(index.lookup, -1, isRef());
}

void OTL::dumpSubtables() {
    cac->dump();
    fprintf(stderr,
            "      -----tag------ --look-----            -------span--------  --index--\n"
            "      scri lang feat typ/fmt|flg offs  lab  scri lang feat look  feat look"
            "\n");
    uint32_t i = 0;
    for (auto &sub : subtables) {
        fprintf(stderr, "[%3d] ", i++);
        sub->dump(subtables.begin(), extLkpType);
        fprintf(stderr, "\n");
    }
}

/* Must call only after otlTableFill(). Returns total length of the Script-,
   Feature-, and Lookup Lists, with all their associated structures (Script,
   LangSys, Feature, Lookup). */
LOffset OTL::getHeaderSize() {
    return header.lookupOffset + lookupSize;
}

void OTL::dumpSizes(LOffset subtableSize, LOffset extensionSectionSize) {
    LOffset s, tot = 0;

    s = getHeaderSize();
    tot += s;
    DF(1, (stderr, "  Scr,Fea,Lkp lists:     size %5u, tot %5u\n", s, tot));

    s = subtableSize;
    tot += s;
    DF(1, (stderr, "  Main subtable section: size %5u, tot %5u\n", s, tot));

    s = cac->coverageSize();
    tot += s;
    DF(1, (stderr, "  Main coverage section: size %5u, tot ", s));
    if (s != 0) {
        DF(1, (stderr, "%5u\n", tot));
    } else {
        DF(1, (stderr, "    \"\n"));
    }

    s = cac->classSize();
    tot += s;
    DF(1, (stderr, "  Main class section:    size %5u, tot ", s));
    if (s != 0) {
        DF(1, (stderr, "%5u\n", tot));
    } else {
        DF(1, (stderr, "    \"\n"));
    }

    s = extensionSectionSize;
    tot += s;
    DF(1, (stderr, "  Extension section:     size %5u, tot ", s));
    if (s != 0) {
        DF(1, (stderr, "%5u\n", tot));
    } else {
        DF(1, (stderr, "    \"\n"));
    }
}

#endif /* HOT_DEBUG */

void OTL::Subtable::ExtensionFormat1::write(hotCtx g, uint16_t lkpType, LOffset adjust) {
    offset += adjust;
    DF(1, (stderr, "  Extension: fmt=%1d, lkpType=%2d, offset=%08ux\n",
           subformat(), lkpType, offset));

    hotOut2(g, subformat());
    hotOut2(g, lkpType);
    hotOut4(g, offset);
}

OTL::Subtable::Subtable(OTL *otl, SubtableInfo *si, std::string &id_text,
                        bool isFeatParam) :
                          script(si->script), language(si->language),
                          feature(si->feature), useExtension(si->useExtension),
                          lkpType(si->lkpType), lkpFlag(si->lkpFlag),
                          markSetIndex(si->markSetIndex),
                          offset(IS_REF_LAB(si->label) ? 0 : isFeatParam ? otl->offset.featParam : otl->offset.subtable),
                          label(si->label),
                          seenInFeature(feature != TAG_STAND_ALONE),
                          isFeatParam(isFeatParam), id_text(id_text) {
    if (isExt() && !isRef()) {
        cac = std::make_shared<CoverageAndClass>(otl->g);
        extension.offset = otl->extOffset();
        otl->incSubOffset(extension.size());
    } else {
        cac = otl->cac;
    }
}

/* ---------------------------- Table Functions ---------------------------- */

/* Returns the lookup index associated with baselabel (which should not have
   the reference bit set). May be called from outside this module only after
   otlTableFill() */
int32_t OTL::label2LookupIndex(Label baseLabel) {
    auto li = labelMap.find(baseLabel);
    if (li == labelMap.end())
        g->logger->log(sFATAL, "(internal) label 0x%x not found", baseLabel);
    else
        li->second.used = true;

    return li->second.lookupInx;
}

/* Calculate LookupList indexes. Granularity of lookups has already been
   determined by the client of this module by labels. Prepare the label ->
   lkpInx mapping */
void OTL::calcLookupListIndices() {
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltLookupList);

    /* Process all regular subtables and subtables with feature parameters.          */
    /* We want the lookups indices to be ordered in the order that they were created */
    /* by feature file definitions.                                                  */
    int prevLabel = -1;
    int indexCnt = 0;
    auto si = subtables.begin();
    for (; si != subtables.end() && !(*si)->isRef(); si++) {
        auto &s = **si;
        if (s.label != prevLabel) {
            /* Label i.e. lookup change. Store lab -> lookupInx info in this first subtable of the span of tables with the same label.  */
            if (s.isParam()) {
                s.index.lookup = -1;
            } else {
                s.index.lookup = indexCnt++;
            }
            auto [li, b] = labelMap.emplace(s.label, s.index.lookup);
            if (!b)
                g->logger->log(sFATAL, "[internal] duplicate subtable label encountered");
        } else {
            s.index.lookup = indexCnt - 1;
        }
        prevLabel = s.label;
    }

    /* Fill the index of any reference subtables (sorted at end) */
    for (; si != subtables.end(); si++) {
        auto &s = **si;
        s.index.lookup = label2LookupIndex(s.label & ~REF_LAB);
    }
}

/* Calculate feature indexes */
void OTL::calcFeatureListIndices() {
    /* Determine granularity of features */
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltScriptList);

    /* Assign temporary feature indexes according to feature granularity (i.e. */
    /* same script, language & feature), disregarding whether reference or has */
    /* earameters or not                                                       */
    auto previ = subtables.begin();
    (*previ)->index.feature = 0;
    for (auto si = previ + 1; si != subtables.end() && !(*si)->isAnon() &&
                              !(*si)->isStandAlone();
                              previ = si, si++) {
        auto &s = **si;
        auto &p = **previ;
        if (s.script != p.script || s.language != p.language || s.feature != p.feature) {
            s.index.feature = p.index.feature + 1;
        } else {
            s.index.feature = p.index.feature;
        }
    }

    /* Sort in final feature order */
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltFeatureList);

    /* Assign final feature indexes */
    int prevIndex = -2;
    int curIndex = -1;
    for (auto &sub : subtables) {
        auto &s = *sub;
        if (s.isAnon() || s.isStandAlone())
            continue;
        if (s.index.feature != prevIndex) {
            /* -- Feature change --  */
            prevIndex = s.index.feature;
            s.index.feature = ++curIndex;
        } else {
            s.index.feature = curIndex;
        }
    }
}

/* Calculate script, language, and index.feature spans */
void OTL::prepScriptList() {
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltScriptList);

    auto prevs = subtables.begin();
    for (auto ss = prevs + 1; ss <= subtables.end(); ss++) {
        if (ss == subtables.end() || (*ss)->isAnon() || (*ss)->isStandAlone() ||
            (*ss)->script != (*prevs)->script) {
            // script change
            (*prevs)->span.script = ss;

            auto prevl = prevs;
            for (auto sl = prevl + 1; sl <= ss ; sl++) {
                if (sl == ss || (*sl)->language != (*prevl)->language) {
                    // language change
                    (*prevl)->span.language = sl;

                    auto prevf = prevl;
                    for (auto sf = prevf + 1; sf <= sl ; sf++) {
                        if (sf == sl ||
                            (*sf)->index.feature != (*prevf)->index.feature) {
                            // feature index change
                            (*prevf)->span.feature = sf;
                            prevf = sf;
                        }
                    }
                    prevl = sl;
                }
            }
            prevs = ss;
        }
        if (ss != subtables.end() && ((*ss)->isAnon() || (*ss)->isStandAlone()))
            break;
    }
}

/* Fill language system record */
Offset OTL::LangSys::fill(Offset o, typename std::vector<std::unique_ptr<Subtable>>::iterator sl) {
    /* Fill record */
    LookupOrder = NULL_OFFSET;
    ReqFeatureIndex = 0xffff; /* xxx unsupported at present */
    offset = o;
    LangSysTag = (*sl)->language;

    for (auto sf = sl; sf != (*sl)->span.language; sf = (*sf)->span.feature)
        featureIndices.push_back((*sf)->index.feature);

    return size(featureIndices.size());
}

Offset OTL::fillScriptList() {
    auto spanEnd = subtables.end() - (nAnonSubtables + nStandAloneSubtables);
    if (subtables.size() == 0 || spanEnd == subtables.begin())
        return Script::listSize(0);

    int nScripts {0};
    for (auto ss = subtables.begin(); ss != spanEnd; ss = (*ss)->span.script)
        nScripts++;

    Offset oScriptList = Script::listSize(nScripts);

    /* Build table */
    for (auto ss = subtables.begin(); ss != spanEnd; ss = (*ss)->span.script) {
        Script script {oScriptList, (*ss)->script};

        int nLanguages = 0;
        for (auto sl = ss; sl != (*ss)->span.script; sl = (*sl)->span.language)
            nLanguages++;

        Offset oScript {0};
        auto slb = ss;
        if ((*slb)->language == dflt_) {
            nLanguages--;
            oScript = script.size(nLanguages);
            /* Fill default language system record */
            oScript += script.defaultLangSys.fill(oScript, slb);
            slb = (*slb)->span.language;
        } else {
            oScript = script.size(nLanguages);
        }

        /* Fill languages system records */
        for (auto sl = slb; sl != (*ss)->span.script; sl = (*sl)->span.language) {
            LangSys ls;
            oScript += ls.fill(oScript, sl);
            script.langSystems.emplace_back(std::move(ls));
        }
        /* Advance to next script */
        oScriptList += oScript;
        header.scripts.emplace_back(std::move(script));
    }

    return oScriptList;
}

/* Sort; span by feature index and lookup index. Caution: overwrites
   span.feature from prepScriptList().
 */
void OTL::prepFeatureList() {
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltFeatureList);

    auto prevf = subtables.begin();
    for (auto sf = prevf + 1; sf <= subtables.end(); sf++) {
        // Whenever we encounter new feature index, store the current subtable
        // index in the first subtable of the sequence of subtables that
        // had the previous subtable index. The array is this divided into
        // sequences of subtables with the same index.feature, and the
        // span.feature of the first subtable in the index gives the index of the first
        /* subtable in the next run.                                                                                */
        /* The same is then done for sequences of subtables with the same index.lookup                              */
        /* within the previous sequence of subtables with the same same index.feature.                              */
        if (sf == subtables.end() || (*sf)->isStandAlone() || (*sf)->isAnon() ||
            (*sf)->index.feature != (*prevf)->index.feature) {
            // feature index change
            (*prevf)->span.feature = sf;

            auto prevl = prevf;
            for (auto sl = prevl + 1; sl <= sf; sl++) {
                if (sl == sf || (*sl)->index.lookup != (*prevl)->index.lookup) {
                    // lookup index change
                    (*prevl)->span.lookup = sl;
                    prevl = sl;
                }
            }
            prevf = sf;
        }
        if (sf != subtables.end() && ((*sf)->isStandAlone() || (*sf)->isAnon()))
            break;
    }
}

Offset OTL::findFeatParamOffset(Tag featTag, Label featLabel) {
    Label matchLabel = (Label)(featLabel & ~REF_LAB);

    for (auto &s : subtables) {
        if (s->feature == featTag && s->label == matchLabel)
            return s->offset;
    }

    return 0;
}

Offset OTL::fillFeatureList() {
    // This works because prepFeature has sorted the subtables so that
    // Anonymous subtables are last, preceded by Stand-Alone subtables
    auto spanEnd = subtables.end() - (nAnonSubtables + nStandAloneSubtables);
    if (spanEnd == subtables.begin())
        return Feature::listSize(0);

    auto last = spanEnd - 1;
    int nFeatures = (*last)->index.feature + 1;

    /* Allocate features */
    header.features.reserve(nFeatures);
    Offset oFeatureList = Feature::listSize(nFeatures);

    /* Build table */
    for (auto sf = subtables.begin(); sf != spanEnd; sf = (*sf)->span.feature) {
        Feature f {oFeatureList, (*sf)->feature};

        /* sub is is the first subtable in a run of subtables with the same feature table index.                       */
        /* sub->span.feature is the array index for the first subtable with a different feature table index.           */
        /* This field is NOT set in any of the other subtables in the current run.                                     */
        /* Within the current run, the first subtable of a sequence with the same lookup table index                   */
        /* span.lookup set to the first subtable of the next sequence with a different lookup index or feature index.  */
        for (auto sl = sf; sl != (*sf)->span.feature; sl = (*sl)->span.lookup) {
            auto &slr = *sl;
            if (slr->isParam()) {
                if (f.featureParams != NULL_OFFSET) {
                    g->logger->log(sFATAL, "%s feature '%c%c%c%c' has more "
                                   "than one FeatureParameter subtable! ",
                                   objName(), TAG_ARG(f.FeatureTag));
                }
                if (slr->isRef())
                    f.featureParams = findFeatParamOffset(slr->feature, slr->label);
                else
                    f.featureParams = slr->offset; /* Note! this is only the offset from the start of the subtable block that follows the lookupList */
                if (f.featureParams == 0)
                    f.featureParams = FEAT_PARAM_ZERO;
            } else {
                f.lookupIndices.push_back(slr->index.lookup);
            }
        }

        oFeatureList += f.size(f.lookupIndices.size());
        header.features.emplace_back(std::move(f));
    }

    return oFeatureList;
}

void OTL::fixFeatureParamOffsets(Offset shortfeatureParamBaseOffset) {
    for (auto &f : header.features) {
        if (f.featureParams != 0) {
            /* Undo fix so we can tell the diff between feature->FeatureParam==NULL_OFFSET, */
            /* and a real 0 offset from the start of the subtable array.                    */
            if (f.featureParams == FEAT_PARAM_ZERO)
                f.featureParams = 0;

            /* feature->FeatureParams is now: (offset from start of featureParams block that follows the FeatureList and feature records) */
            /* featureParamBaseOffset is (size of featureList).                     */
            f.featureParams += shortfeatureParamBaseOffset - f.offset;
            if (f.featureParams > 0xFFFF) {
                g->logger->log(sFATAL, "feature parameter offset too large (%0x)",
                               f.featureParams);
            }
        }
    }
}

void OTL::prepLookupList() {
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltLookupList);

    auto prevl = subtables.begin();
    for (auto sl = prevl + 1; sl <= subtables.end(); sl++) {
        auto &slr = *sl;
        auto &prevlr = *prevl;
        if (sl == subtables.end() || slr->isRef() || slr->isParam() ||
            slr->index.lookup != prevlr->index.lookup) {
            /* Lookup index change */
            prevlr->span.lookup = sl;
            prevl = sl;
        }
        if (sl != subtables.end() && (slr->isRef() || slr->isParam()))
            break;
    }
}

Offset OTL::fillLookupList() {
    int nLookups = 0;
    auto spanEnd = subtables.end() - (nRefLookups + nFeatParams);

    if (spanEnd != subtables.begin()) {
        // can have 0 lookups when there is only a 'size' feature,
        // and no other features.
        auto last = spanEnd - 1;
        nLookups = (*last)->index.lookup + 1;
    }

    DF(2, (stderr, ">OTL: %d lookup%s allocated (%d ref%s skipped)\n",
           nLookups, nLookups == 1 ? "" : "s",
           nRefLookups, nRefLookups == 1 ? "" : "s"));

    /* Allocate lookups */
    header.lookups.reserve(nLookups);
    Offset oLookupList = Lookup::listSize(nLookups);

    /* Build table */
    for (auto sl = subtables.begin(); sl < spanEnd; sl = (*sl)->span.lookup) {
        auto &l = *sl;
        Lookup lookup {oLookupList, l->isExt() ? extLkpType : l->lkpType, l->lkpFlag};

        for (auto si = sl; si != (*sl)->span.lookup; si++)
            lookup.subtableOffsets.push_back((*si)->offset - oLookupList);

        oLookupList += lookup.size(lookup.subtableOffsets.size());
        if (lookup.Flag & otlUseMarkFilteringSet) {
            lookup.useMarkSetIndex = (*sl)->markSetIndex;
            oLookupList += sizeof(lookup.useMarkSetIndex);
        }
        header.lookups.emplace_back(std::move(lookup));
    }

    return oLookupList;
}

void OTL::checkStandAloneRefs() {
    std::map<int32_t, bool> revUsedMap;
    for (auto li : labelMap)
        revUsedMap.emplace(li.second.lookupInx, li.second.used);

    // Now go back through all the real subtables, and check that
    // the stand-alone ones have been referenced.
    for (auto &s : subtables) {
        if (s->isAnon() || s->isRef())
            continue;
        if (s->seenInFeature)
            continue;
        auto rli = revUsedMap.find(s->index.lookup);
        if (rli == revUsedMap.end())
            g->logger->log(sFATAL, "Base lookup %d not found", s->index.lookup);
        s->seenInFeature = rli->second;
        if (!s->seenInFeature)
            g->logger->log(sWARNING, "Stand-alone lookup with Lookup Index %d "
                           "was not referenced from within any feature, and will "
                           "never be used.", s->index.lookup);
    }
}

void OTL::checkOverflow(const char* offsetType, long offset, const char* posType) {
    if (offset > 0xFFFF) {
        g->logger->log(sFATAL,
                       "In %s %s rules cause an offset overflow (0x%lx) to a %s",
                       g->error_id_text.c_str(), posType, offset, offsetType);
    }
}

    /* The font tables are in the order:
     ScriptList
     FeatureList
     FeatureParams
     LookupList
     lookup subtables (with aalt subtables written last)
     anon subtables (lookup subtables created by contextual rules)
     coverage definition tables
     class definition tables
     extension sections.
     Notes:
     All directly defined lookup subtables are added in the order that they are
     created by the feature file. The only exceptions are the subtables for
     the aalt lookups, and anonymous subtables. 'aalt'subtables' are created
     after the end of feature file parsing, in feat.c:featFill(), since the
     aalt feature references can be used only after all the other features are
     defined. Anonymous subtables, those implied by contextual rules rather
     than being explicitly defined, are added at the end of the subtable list,
     in createAnonLookups() above.

     coverage and class subtables are seperately accumulated in otlTable
     t->coverage.tables and t->class.tables, and are written after all the
     lookup subtables, first coverage, then class subtables.

     For featparams and lookup subtables, there are two parallel sets of arrays
     of subtables. The GSUB arrays (h->*) contain the actual data to be
     written, and is where the offsets are set. The other set is in the otl
     table, and exists so that the GPOS and GSUB can share code for ordering
     and writing feature and lookup indices. The latter inherit offset and
     other data from the GSUB arrays. The GSUB arrays are created when the
     feature file is processed, by all the fill* functions.  The otl table
     arrays are created below.
    */

int OTL::fillOTL(bool force) {
    if (subtables.size() == 0 && !force)
        return 0;

    if (g->hadError)
        g->logger->log(sFATAL, "aborting because of errors");

    createAnonLookups();

    DF(1, (stderr, "### %s:\n", objName()));

    Offset offst = header.size();

    calcLookupListIndices();
    calcFeatureListIndices();

    prepScriptList();
    Offset size = fillScriptList();
    header.scriptOffset = offst;
    offst += size;

#if HOT_DEBUG
    if (DF_LEVEL(g) >= 1)
        dumpSubtables();
#endif

    prepFeatureList();
    size = fillFeatureList();
    header.featureOffset = offst;
    offst += size;
    if (nFeatParams > 0) {
        /* The feature table FeatureParam offsets are currently
         * from the start of the featureParam block, starting right
         * after the FeatureList. featureParamBaseOffset is the size
         * of the FeatureList and its records..
         */
        fixFeatureParamOffsets(size);
        offst += offset.featParam;
    }

    prepLookupList();
    header.lookupOffset = offst;
    lookupSize = fillLookupList();

    offset.extensionSection = offset.subtable + cac->coverageSize() + cac->classSize();
#if HOT_DEBUG
    dumpSizes(offset.subtable, offset.extension);
#endif /* HOT_DEBUG */

    /* setAnonLookupIndices marks as used not only the anonymous lookups, */
    /* but also all lookups that were referenced from chain sub rules,    */
    /* including the stand-alone lookups. This is why                     */
    /* checkStandAloneTablRefs has to follow setAnonLookupIndices.        */
    setAnonLookupIndices();

    checkStandAloneRefs();

    OS_2SetMaxContext(g, maxContext);

    // Put the subtables back on offset order
    std::stable_sort(subtables.begin(), subtables.end(), Subtable::ltOffset);

    return 1;
}

void OTL::writeOTL() {
    header.write(g);

    // Feature Parameters first
    for (auto &sub : subtables) {
        g->error_id_text = sub->id_text;

        if (!sub->isRef() && sub->isParam())
            sub->write(this);
    }

    lookupListWrite();

    // Write main subtable section
    for (auto &sub : subtables) {
        if (sub->isRef())
            continue;

        g->error_id_text = sub->id_text;

        if (sub->isExt())
            sub->writeExt(this, offset.extensionSection);
        else if (!sub->isParam())
            sub->write(this);
    }

    // Write main coverage and class tables
    cac->coverageWrite();
    cac->classWrite();

    // Write extension subtables section
    // Each subtable is immediately followed by its coverages and classes
    for (auto &sub : subtables) {
        if (!sub->isRef() && sub->isExt()) {
            g->error_id_text = sub->id_text;
            sub->write(this);
        }
    }
}

void OTL::LangSys::write(hotCtx g) {
    hotOut2(g, LookupOrder);
    hotOut2(g, ReqFeatureIndex);
    hotOut2(g, featureIndices.size());

    for (auto fi : featureIndices)
        hotOut2(g, fi);
}

void OTL::Header::write(hotCtx g) {
    hotOut4(g, Version);
    hotOut2(g, scriptOffset);
    hotOut2(g, featureOffset);
    hotOut2(g, lookupOffset);

    if (scriptOffset == 0)
        return;

    // Scripts
    hotOut2(g, scripts.size());
    for (auto &s : scripts) {
        hotOut4(g, s.ScriptTag);
        hotOut2(g, s.offset);
    }
    for (auto &s : scripts) {
        hotOut2(g, s.defaultLangSys.offset);
        hotOut2(g, s.langSystems.size());

        for (auto &ls : s.langSystems) {
            hotOut4(g, ls.LangSysTag);
            hotOut2(g, ls.offset);
        }

        if (s.defaultLangSys.offset != 0)
            s.defaultLangSys.write(g);

        for (auto &ls : s.langSystems)
            ls.write(g);
    }

    // Features
    hotOut2(g, features.size());
    for (auto &f : features) {
        hotOut4(g, f.FeatureTag);
        hotOut2(g, f.offset);
    }
    for (auto &f : features) {
        hotOut2(g, f.featureParams);
        hotOut2(g, f.lookupIndices.size());

        for (auto li : f.lookupIndices)
            hotOut2(g, li);
    }
}

void OTL::Header::lookupListWrite(hotCtx g, Offset lookupSize) {
    hotOut2(g, lookups.size());

    for (auto &l : lookups)
        hotOut2(g, l.offset);

    int i = 0;
    for (auto &l : lookups) {
        hotOut2(g, l.Type);
        hotOut2(g, l.Flag);
        hotOut2(g, l.subtableOffsets.size());
        for (auto so : l.subtableOffsets) {
            LOffset subTableOffset = so + lookupSize;
            if (subTableOffset > 0xFFFF) {
                g->logger->log(sFATAL, "subtable offset too large (%0lx) "
                               "in lookup %i type %i",
                               subTableOffset, i, l.Type);
            }
            hotOut2(g, subTableOffset);
        }
        if (l.Flag & otlUseMarkFilteringSet)
            hotOut2(g, l.useMarkSetIndex);
        i++;
    }
}

void OTL::setAnonLookupIndices() {
    for (auto &sub : subtables) {
        auto lv = sub->getLookups();
        if (lv == nullptr)
            continue;
        for (auto &lr : *lv) {
            DF(2, (stderr, "lr: Label 0x%x", lr.LookupListIndex));
            lr.LookupListIndex = label2LookupIndex(lr.LookupListIndex);
            DF(2, (stderr, " -> LookupListIndex %u\n", lr.LookupListIndex));
        }
    }
}

void OTL::AddSubtable(typename std::unique_ptr<Subtable> s) {
    subtables.emplace_back(std::move(s));
    auto &sub = subtables.back();

    if (sub->isAnon())
        nAnonSubtables++;
    if (sub->isStandAlone())
        nStandAloneSubtables++;

    // FeatParam subtables may be labeled, but should NOT be added
    // to the list of real look ups.
    if (sub->isRef())
        nRefLookups++;
    else if (sub->isParam())
        nFeatParams++;
}

const VarTrackVec &OTL::getValues() {
    return g->ctx.GDEFp->getValues();
}

ValueIndex OTL::nextValueIndex() {
    return getValues().size();
}

ValueIndex OTL::addTrackerValue(const VarValueRecord &vvr) {
    return g->ctx.GDEFp->addTrackerValue(vvr);
}

void OTL::setDevOffset(ValueIndex vi, LOffset o) {
    g->ctx.GDEFp->setDevOffset(vi, o);
}


void OTL::setCoverages(std::vector<LOffset> &covs,
                       std::shared_ptr<CoverageAndClass> &cac,
                       std::vector<GPat::ClassRec*> classes, LOffset o) {
    if (classes.size() == 0)
        return;

    covs.reserve(classes.size());
    for (auto cr : classes) {
        cac->coverageBegin();
        for (auto &g : cr->glyphs)
            cac->coverageAddGlyph(g);

        covs.push_back(cac->coverageEnd() + o);
    }
}
