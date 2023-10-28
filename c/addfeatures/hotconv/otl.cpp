/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Shared OpenType Layout support.
 */

#include "otl.h"

#include <cassert>
#include <algorithm>
#include <utility>

#include <stdio.h>
#include <stdlib.h>


#define FEAT_PARAM_ZERO 1

#if HOT_DEBUG
/* Initialize subtable. Not needed except for debug dumps */
static void valDump(int16_t val, int16_t excep, bool isRef) {
    if (val == excep) {
        fprintf(stderr, "   * ");
    } else if (isRef) {
        fprintf(stderr, "   ->%hd ", val);
    } else {
        fprintf(stderr, "%4hd ", val);
    }
}

static void tagDump(Tag tag, char ch) {
    if (tag == TAG_UNDEF) {
        fprintf(stderr, "****");
    } else {
        fprintf(stderr, "%c%c%c%c", TAG_ARG(tag));
    }
    if (ch != 0) {
        fprintf(stderr, "%c", ch);
    }
}

void otlTbl::Subtable::dump(std::vector<Subtable>::iterator sb) {
    tagDump(script, ' ');
    tagDump(language, ' ');
    tagDump(feature, ' ');
    fprintf(stderr, "   %2u/%hu|%-3x %4hx ",
            lookup >> 16,
            fmt,
            lookup & 0xffff,
            offset);
    fprintf(stderr, "%4x ", label);

    std::vector<Subtable>::iterator uninit;
    valDump(span.script - sb, uninit - sb, 0);
    valDump(span.language - sb, uninit - sb, 0);
    valDump(span.feature - sb, uninit - sb, 0);
    valDump(span.lookup - sb, uninit - sb, 0);
    valDump(index.feature, -1, false);
    valDump(index.lookup, -1, isRef());
}

void otlTbl::dumpSubtables() {
    fprintf(stderr,
            "# nCov: %ld, nClass: %ld; nCovReused: %d, nClassReused: %d\n"
            "# [offs relative to start of subtable section]\n",
            coverage.records.size(), classObj.records.size(),
            coverage.reused, classObj.reused);
    fprintf(stderr,
            "      -----tag------ --look-----            -------span--------  --index--\n"
            "      scri lang feat typ/fmt|flg offs  lab  scri lang feat look  feat look"
            "\n");
    uint32_t i = 0;
    for (auto &sub : subtables) {
        fprintf(stderr, "[%3d] ", i++);
        sub.dump(subtables.begin());
        fprintf(stderr, "\n");
    }
}

/* Must call only after otlTableFill(). Returns total length of the Script-,
   Feature-, and Lookup Lists, with all their associated structures (Script,
   LangSys, Feature, Lookup). */
LOffset otlTbl::getHeaderSize() {
    return header.lookupOffset + lookupSize;
}

void otlTbl::dumpSizes(LOffset subtableSize, LOffset extensionSectionSize) {
    LOffset s, tot = 0;

    s = getHeaderSize();
    tot += s;
    DF(1, (stderr, "  Scr,Fea,Lkp lists:     size %5u, tot %5u\n", s, tot));

    s = subtableSize;
    tot += s;
    DF(1, (stderr, "  Main subtable section: size %5u, tot %5u\n", s, tot));

    s = getCoverageSize();
    tot += s;
    DF(1, (stderr, "  Main coverage section: size %5u, tot ", s));
    if (s != 0) {
        DF(1, (stderr, "%5u\n", tot));
    } else {
        DF(1, (stderr, "    \"\n"));
    }

    s = getClassSize();
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

/* --------------------------- Standard Functions -------------------------- */

otlTbl::CoverageRecord::CoverageRecord(Offset o, std::set<GID> &gl) : offset(o) {
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

/* Write format 1 table */
void otlTbl::CoverageRecord::write(otlTbl &t) {
    if (ranges.size() > 0) {  // format 2
        t.o2(2);
        t.o2(ranges.size());
        for (auto &range : ranges) {
            t.o2(range.Start);
            t.o2(range.End);
            t.o2(range.startCoverageIndex);
        }
    } else {  // format 1
        t.o2(1);
        t.o2(glyphs.size());
        for (auto gid : glyphs)
            t.o2(gid);
    }
}

/* Fill coverage table */
Offset otlTbl::Coverage::fill(otlTbl &t) {
    records.emplace_back(size, current);
    size += records.back().size();

    if (size > 0xFFFF)
        hotMsg(t.g, hotFATAL, "coverage section too large (%0x)", size);

    return records.back().offset;
}

void otlTbl::Coverage::write(otlTbl &t) {
    for (auto &record : records)
        record.write(t);
}

/* Write all coverage tables */
void otlTbl::coverageWrite() {
    coverage.write(*this);
}

/* Begin new coverage table */
void otlTbl::coverageBegin() {
    coverage.current.clear();
}

/* Add coverage glyph */
void otlTbl::coverageAddGlyph(GID gid, bool warn) {
    auto [i, b] = coverage.current.insert(gid);
    if (!b) {
        if (warn) {
            featGlyphDump(g, gid, 0, 0);
            hotMsg(g, hotNOTE, "Removing duplicate glyph <%s>", g->note.array);
        }
#if HOT_DEBUG
        printf("duplicated glyph ['%d'] in coverage.\n", gid);
#endif
    }
}

/* End coverage table; uniqueness of GIDs up to client. Sorting done here. */
Offset otlTbl::coverageEnd() {
    for (auto &record : coverage.records) {
        if (record.glyphs == coverage.current) {
#if HOT_DEBUG
            coverage.reused++;
#if 0
            if (DF_LEVEL >= 2) {
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

    return coverage.fill(*this);
}

/* Returns total length of the coverage section, for all coverages currently
   defined. */
LOffset otlTbl::getCoverageSize() {
    return coverage.size;
}

/* ---------------------------- Class Functions ---------------------------- */

otlTbl::ClassRecord::ClassRecord(Offset o, std::map<GID, uint16_t> &m) : offset(o) {
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

void otlTbl::ClassRecord::write(otlTbl &t) {
    if (values.empty()) {  // Format 2
        t.o2(2);
        t.o2(ranges.size());
        for (auto &range : ranges) {
            t.o2(range.Start);
            t.o2(range.End);
            t.o2(range.classId);
        }
    } else {  // Format 1
        assert(startGlyph != GID_UNDEF && ranges.size() == 0);
        t.o2(1);
        t.o2(startGlyph);
        t.o2(values.size());
        for (auto classIndex : values)
            t.o2(classIndex);
    }
}

/* Fill coverage table */
Offset otlTbl::Class::fill(otlTbl &t) {
    records.emplace_back(size, current);
    size += records.back().size();

    if (size > 0xFFFF) {
        hotMsg(t.g, hotFATAL, "class section too large (%0x)", size);
    }

    return records.back().offset;
}

void otlTbl::Class::write(otlTbl &t) {
    for (auto &record : records)
        record.write(t);
}

/* Write all coverage tables */
void otlTbl::classWrite() {
    classObj.write(*this);
}

/* Begin new coverage table */
void otlTbl::classBegin() {
    classObj.current.clear();
}

/* Add class mapping; ignore class 0's */
void otlTbl::classAddMapping(GID gid, uint32_t classId) {
    if (classId == 0)
        return;
    auto [i, b] = classObj.current.insert({gid, classId});
    if (!b) {
#if HOT_DEBUG
        printf("duplicated glyph ['%d'] in class mapping.\n", gid);
#endif
    }
}

/* End class table */
Offset otlTbl::classEnd() {
    for (auto &record : classObj.records) {
        if (record.map == classObj.current) {
#if HOT_DEBUG
            classObj.reused++;
#if 0
            if (DF_LEVEL >= 2) {
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

    return classObj.fill(*this);
}

/* Returns total length of the class section, for all classes currently
   defined. */
LOffset otlTbl::getClassSize() {
    return classObj.size;
}

/* ---------------------------- Table Functions ---------------------------- */

/* Returns the lookup index associated with baselabel (which should not have
   the reference bit set). May be called from outside this module only after
   otlTableFill() */
int32_t otlTbl::label2LookupIndex(Label baseLabel) {
    auto li = labelMap.find(baseLabel);
    if (li == labelMap.end())
        hotMsg(g, hotFATAL, "(internal) label 0x%x not found", baseLabel);
    else
        li->second.used = true;

    return li->second.lookupInx;
}

/* Calculate LookupList indexes. Granularity of lookups has already been
   determined by the client of this module by labels. Prepare the label ->
   lkpInx mapping */
void otlTbl::calcLookupListIndices() {
    std::stable_sort(subtables.begin(), subtables.end(),
                     Subtable::ltLookupList);

    /* Process all regular subtables and subtables with feature parameters.          */
    /* We want the lookups indices to be ordered in the order that they were created */
    /* by feature file definitions.                                                  */
    int prevLabel = -1;
    int indexCnt = 0;
    auto si = subtables.begin();
    for (; si != subtables.end() && !si->isRef(); si++) {
        if (si->label != prevLabel) {
            /* Label i.e. lookup change. Store lab -> lookupInx info in this first subtable of the span of tables with the same label.  */
            if (si->isFeatParam) {
                si->index.lookup = -1;
            } else {
                si->index.lookup = indexCnt++;
            }
            auto [li, b] = labelMap.emplace(si->label, si->index.lookup);
            if (!b) {
                hotMsg(g, hotFATAL, "[internal] duplicate subtable label encountered");
            }
        } else {
            si->index.lookup = indexCnt - 1;
        }
        prevLabel = si->label;
    }

    /* Fill the index of any reference subtables (sorted at end) */
    for (; si != subtables.end(); si++)
        si->index.lookup = label2LookupIndex(si->label & ~REF_LAB);
}

/* Calculate feature indexes */
void otlTbl::calcFeatureListIndices() {
    /* Determine granularity of features */
    std::stable_sort(subtables.begin(), subtables.end(),
                     Subtable::ltScriptList);

    /* Assign temporary feature indexes according to feature granularity (i.e. */
    /* same script, language & feature), disregarding whether reference or has */
    /* parameters or not                                                       */
    auto previ = subtables.begin();
    previ->index.feature = 0;
    for (auto si = previ + 1; si != subtables.end() && !si->isAnon() &&
                              !si->isStandAlone();
                              previ = si, si++) {
        if (si->script != previ->script || si->language != previ->language ||
              si->feature != previ->feature) {
            si->index.feature = previ->index.feature + 1;
        } else {
            si->index.feature = previ->index.feature;
        }
    }

    /* Sort in final feature order */
    std::stable_sort(subtables.begin(), subtables.end());

    /* Assign final feature indexes */
    int prevIndex = -2;
    int curIndex = -1;
    for (auto &s : subtables) {
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
void otlTbl::prepScriptList() {
    std::stable_sort(subtables.begin(), subtables.end(),
                     Subtable::ltScriptList);

    auto prevs = subtables.begin();
    for (auto ss = prevs + 1; ss <= subtables.end(); ss++) {
        if (ss == subtables.end() || ss->isAnon() || ss->isStandAlone() ||
            ss->script != prevs->script) {
            // script change
            prevs->span.script = ss;

            auto prevl = prevs;
            for (auto sl = prevl + 1; sl <= ss ; sl++) {
                if (sl == ss || sl->language != prevl->language) {
                    // language change
                    prevl->span.language = sl;

                    auto prevf = prevl;
                    for (auto sf = prevf + 1; sf <= sl ; sf++) {
                        if (sf == sl ||
                            sf->index.feature != prevf->index.feature) {
                            // feature index change
                            prevf->span.feature = sf;
                            prevf = sf;
                        }
                    }
                    prevl = sl;
                }
            }
            prevs = ss;
        }
        if (ss->isAnon() || ss->isStandAlone())
            break;
    }
}

/* Fill language system record */
Offset otlTbl::LangSys::fill(Offset o, std::vector<Subtable>::iterator sl) {
    /* Fill record */
    LookupOrder = NULL_OFFSET;
    ReqFeatureIndex = 0xffff; /* xxx unsupported at present */
    offset = o;
    LangSysTag = sl->language;

    for (auto sf = sl; sf != sl->span.language; sf = sf->span.feature)
        featureIndices.push_back(sf->index.feature);

    return size(featureIndices.size());
}

/* Fill ScriptList */
Offset otlTbl::fillScriptList() {
    auto spanEnd = subtables.end() - (nAnonSubtables + nStandAloneSubtables);
    if (subtables.size() == 0 || spanEnd == subtables.begin())
        return Script::listSize(0);

    int nScripts {0};
    for (auto ss = subtables.begin(); ss != spanEnd; ss = ss->span.script)
        nScripts++;

    Offset oScriptList = Script::listSize(nScripts);

    /* Build table */
    for (auto ss = subtables.begin(); ss != spanEnd; ss = ss->span.script) {
        Script script {oScriptList, ss->script};

        int nLanguages = 0;
        for (auto sl = ss; sl != ss->span.script; sl = sl->span.language)
            nLanguages++;

        Offset oScript {0};
        auto slb = ss;
        if (slb->language == dflt_) {
            nLanguages--;
            oScript = script.size(nLanguages);
            /* Fill default language system record */
            oScript += script.defaultLangSys.fill(oScript, slb);
            slb = slb->span.language;
        } else {
            oScript = script.size(nLanguages);
        }

        /* Fill languages system records */
        for (auto sl = slb; sl != ss->span.script; sl = sl->span.language) {
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
void otlTbl::prepFeatureList() {
    std::stable_sort(subtables.begin(), subtables.end());

    auto prevf = subtables.begin();
    for (auto sf = prevf + 1; sf <= subtables.end(); sf++) {
        /* This logic steps through the  t->subtable.array.                                                         */
        /* Whenever it encounters a new feature subtable.array[fea].index.feature index,                            */
        /* it stores the current subtable index in the first subtable of the sequence of subtables that             */
        /* had the previous subtable index. The array is this divided into sequences of subtables with the          */
        /* same index.feature, and the span.feature of the first subtable in the index gives the index of the first */
        /* subtable in the next run.                                                                                */
        /* The same is then done for sequences of subtables with the same index.lookup                              */
        /* within the previous sequence of subtables with the same same index.feature.                              */
        if (sf == subtables.end() || sf->isStandAlone() || sf->isAnon() ||
            sf->index.feature != prevf->index.feature) {
            // feature index change
            prevf->span.feature = sf;

            auto prevl = prevf;
            for (auto sl = prevl + 1; sl <= sf; sl++) {
                if (sl == sf || sl->index.lookup != prevl->index.lookup) {
                    // lookup index change
                    prevl->span.lookup = sl;
                    prevl = sl;
                }
            }
            prevf = sf;
        }
        if (sf->isStandAlone() || sf->isAnon())
            break;
    }
}

/* Fill FeatureList */
Offset otlTbl::findFeatParamOffset(Tag featTag, Label featLabel) {
    Label matchLabel = (Label)(featLabel & ~REF_LAB);

    for (auto &s : subtables) {
        if (s.feature == featTag && s.label == matchLabel)
            return s.offset;
    }

    return 0;
}

Offset otlTbl::fillFeatureList() {
    // This works because prepFeature has sorted the subtables so that
    // Anonymous subtables are last, preceded by Stand-Alone subtables
    auto spanEnd = subtables.end() - (nAnonSubtables + nStandAloneSubtables);
    if (spanEnd == subtables.begin())
        return Feature::listSize(0);

    auto last = spanEnd - 1;
    int nFeatures = last->index.feature + 1;

    /* Allocate features */
    header.features.reserve(nFeatures);
    Offset oFeatureList = Feature::listSize(nFeatures);

    /* Build table */
    for (auto sf = subtables.begin(); sf != spanEnd; sf = sf->span.feature) {
        Feature f {oFeatureList, sf->feature};

        /* sub is is the first subtable in a run of subtables with the same feature table index.                       */
        /* sub->span.feature is the array index for the first subtable with a different feature table index.           */
        /* This field is NOT set in any of the other subtables in the current run.                                     */
        /* Within the current run, the first subtable of a sequence with the same lookup table index                   */
        /* span.lookup set to the first subtable of the next sequence with a different lookup index or feature index.  */
        for (auto sl = sf; sl != sf->span.feature; sl = sl->span.lookup) {
            if (sl->isParam()) {
                if (f.featureParams != NULL_OFFSET) {
                    hotMsg(g, hotFATAL, "GPOS feature '%c%c%c%c' has more "
                                        "than one FeatureParameter subtable! ",
                                        TAG_ARG(f.FeatureTag));
                }
                if (sl->isRef())
                    f.featureParams = findFeatParamOffset(sl->feature, sl->label);
                else
                    f.featureParams = sl->offset; /* Note! this is only the offset from the start of the subtable block that follows the lookupList */
                if (f.featureParams == 0)
                    f.featureParams = FEAT_PARAM_ZERO;
            } else {
                f.lookupIndices.push_back(sl->index.lookup);
            }
        }

        oFeatureList += f.size(f.lookupIndices.size());
        header.features.emplace_back(std::move(f));
    }

    return oFeatureList;
}

void otlTbl::fixFeatureParamOffsets(Offset shortfeatureParamBaseOffset) {
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
                hotMsg(g, hotFATAL, "feature parameter offset too large (%0x)",
                       f.featureParams);
            }
        }
    }
}

void otlTbl::prepLookupList() {
    std::stable_sort(subtables.begin(), subtables.end(),
                     Subtable::ltLookupList);

    auto prevl = subtables.begin();
    for (auto sl = prevl + 1; sl <= subtables.end(); sl++) {
        if (sl == subtables.end() || sl->isRef() || sl->isParam() ||
            sl->index.lookup != prevl->index.lookup) {
            /* Lookup index change */
            prevl->span.lookup = sl;
            prevl = sl;
        }
        if (sl->isRef() || sl->isParam())
            break;
    }
}

Offset otlTbl::fillLookupList() {
    int nLookups = 0;
    auto spanEnd = subtables.end() - (nRefLookups + nFeatParams);

    if (spanEnd != subtables.begin()) {
        // can  have 0 lookups when there is only a GPOS 'size' feature,
        // and no other features.
        auto last = spanEnd - 1;
        nLookups = last->index.lookup + 1;
    }

    DF(2, (stderr, ">OTL: %d lookup%s allocated (%d ref%s skipped)\n",
           nLookups, nLookups == 1 ? "" : "s",
           nRefLookups, nRefLookups == 1 ? "" : "s"));

    /* Allocate lookups */
    header.lookups.reserve(nLookups);
    Offset oLookupList = Lookup::listSize(nLookups);

    /* Build table */
    for (auto sl = subtables.begin(); sl < spanEnd; sl = sl->span.lookup) {
        Lookup lookup {oLookupList, sl->lookup};

        for (auto si = sl; si != sl->span.lookup; si++)
            lookup.subtableOffsets.push_back(si->offset - oLookupList);

        oLookupList += lookup.size(lookup.subtableOffsets.size());
        if (lookup.Flag & otlUseMarkFilteringSet) {
            lookup.useMarkSetIndex = sl->markSetIndex;
            oLookupList += sizeof(lookup.useMarkSetIndex);
        }
        header.lookups.emplace_back(std::move(lookup));
    }

    return oLookupList;
}

void otlTbl::checkStandAloneRefs() {
    std::map<int32_t, bool> revUsedMap;
    for (auto li : labelMap)
        revUsedMap.emplace(li.second.lookupInx, li.second.used);

    // Now go back through all the real subtables, and check that
    // the stand-alone ones have been referenced.
    for (auto s : subtables) {
        if (s.isAnon() || s.isRef())
            continue;
        if (s.seenInFeature)
            continue;
        auto rli = revUsedMap.find(s.index.lookup);
        if (rli == revUsedMap.end())
            hotMsg(g, hotFATAL, "Base lookup %d not found", s.index.lookup);
        s.seenInFeature = rli->second;
        if (!s.seenInFeature)
            hotMsg(g, hotWARNING, "Stand-alone lookup with Lookup Index %d "
                   "was not referenced from within any feature, and will "
                   "never be used.", s.index.lookup);
    }
}

void otlTbl::fill(LOffset params_size) {
    Offset offset = header.size();

    calcLookupListIndices();
    calcFeatureListIndices();

    prepScriptList();
    Offset size = fillScriptList();
    header.scriptOffset = offset;
    offset += size;

#if HOT_DEBUG
    if (DF_LEVEL >= 1)
        dumpSubtables();
#endif

    prepFeatureList();
    size = fillFeatureList();
    header.featureOffset = offset;
    offset += size;
    if (nFeatParams > 0) {
        /* The feature table FeatureParam offsets are currently
         * from the start of the featureParam block, starting right
         * after the FeatureList. featureParamBaseOffset is the size
         * of the FeatureList and its records..
         */
        fixFeatureParamOffsets(size);
        offset += params_size;
    }

    prepLookupList();
    header.lookupOffset = offset;
    lookupSize = fillLookupList();
}

void otlTbl::LangSys::write(otlTbl &t) {
    t.o2(LookupOrder);
    t.o2(ReqFeatureIndex);
    t.o2(featureIndices.size());

    for (auto fi : featureIndices)
        t.o2(fi);
}

void otlTbl::Header::write(otlTbl &t) {
    t.o4(Version);
    t.o2(scriptOffset);
    t.o2(featureOffset);
    t.o2(lookupOffset);

    if (scriptOffset == 0)
        return;

    // Scripts
    t.o2(scripts.size());
    for (auto &s : scripts) {
        t.o4(s.ScriptTag);
        t.o2(s.offset);
    }
    for (auto &s : scripts) {
        t.o2(s.defaultLangSys.offset);
        t.o2(s.langSystems.size());

        for (auto &ls : s.langSystems) {
            t.o4(ls.LangSysTag);
            t.o2(ls.offset);
        }

        if (s.defaultLangSys.offset != 0)
            s.defaultLangSys.write(t);

        for (auto &ls : s.langSystems)
            ls.write(t);
    }

    // Features
    t.o2(features.size());
    for (auto &f : features) {
        t.o4(f.FeatureTag);
        t.o2(f.offset);
    }
    for (auto &f : features) {
        t.o2(f.featureParams);
        t.o2(f.lookupIndices.size());

        for (auto li : f.lookupIndices)
            t.o2(li);
    }
}

void otlTbl::Header::lookupListWrite(otlTbl &t, Offset lookupSize) {
    t.o2(lookups.size());

    for (auto &l : lookups)
        t.o2(l.offset);

    int i = 0;
    for (auto &l : lookups) {
        t.o2(l.Type);
        t.o2(l.Flag);
        t.o2(l.subtableOffsets.size());
        for (auto so : l.subtableOffsets) {
            LOffset subTableOffset = so + lookupSize;
            if (subTableOffset > 0xFFFF) {
                hotMsg(t.g, hotFATAL, "subtable offset too large (%0lx) "
                       "in lookup %i type %i",
                       subTableOffset, i, l.Type);
            }
            t.o2(subTableOffset);
        }
        if (l.Flag & otlUseMarkFilteringSet)
            t.o2(l.useMarkSetIndex);
        i++;
    }
}

/* Add subtable to list. Assumes label has been set for intended granularity of
   lookups (i.e. all subtables of the same lookup should have the same
   label). */
void otlTbl::subtableAdd(Tag script, Tag language, Tag feature,
                         int32_t lkpType, int32_t lkpFlag,
                         uint16_t markSetIndex, uint16_t extensionLookupType,
                         LOffset offset, Label label, uint16_t fmt,
                         bool isFeatParam) {
    subtables.emplace_back(script, language, feature, lkpType, lkpFlag,
                           markSetIndex, extensionLookupType, offset, label,
                           fmt, isFeatParam);

    auto &sub = subtables.back();

    if (sub.isAnon())
        nAnonSubtables++;
    if (sub.isStandAlone())
        nStandAloneSubtables++;

     /* FeatParam subtables may be labeled, but should NOT be added */
     /* to the list of real look ups.                               */
    if (sub.isRef())
        nRefLookups++;
    else if (sub.isParam()) {
        // Really counting non-ref isParams here
        nFeatParams++;
    }
}
