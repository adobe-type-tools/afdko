/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Font GDEFer table.
 */
#include "GDEF.h"

#include <assert.h>

#include <unordered_map>
#include <algorithm>
#include <utility>

const char *GDEF::GlyphClassTable::names[4] = { "Mark", "Base", "Ligature",
                                                "Component" };
const uint16_t GDEF::GlyphClassTable::classIDmap[4] = { 3, 1, 2, 4 };

void GDEFNew(hotCtx g) {
    g->ctx.GDEFp = new GDEF(g);
}

int GDEFFill(hotCtx g) {
    return g->ctx.GDEFp->Fill();
}

int GDEF::Fill() {
    Offset tableSize;
    bool haveData {false};

    uint32_t markSetClassCount = markSetClassTable.markSetClasses.size();

    version = markSetClassCount ? 0x00010002 : 0x00010000;
    offset = headerSize(markSetClassCount);

    if ((tableSize = glyphClassTable.fill(offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    if ((tableSize = attachTable.fill(offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    if ((tableSize = ligCaretTable.fill(offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    if ((tableSize = markAttachClassTable.fill(offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    if ((tableSize = markSetClassTable.fill(offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    return haveData ? 1 : 0;
}

void GDEFWrite(hotCtx g) {
    g->ctx.GDEFp->Write();
}

void GDEF::Write() {
    /* Perform checksum adjustment */
    auto h = this;
    OUT4(version);
    OUT2(glyphClassTable.offset);
    OUT2(attachTable.offset);
    OUT2(ligCaretTable.offset);
    OUT2(markAttachClassTable.offset);
    if (markSetClassTable.offset) {
        /* If this is zero, then we write a Version 1,0 table, which doesn't have this offset. */
        OUT2(markSetClassTable.offset);
    }

    glyphClassTable.write();
    attachTable.write(h);
    ligCaretTable.write(h);
    markAttachClassTable.write();
    markSetClassTable.write(h);
}

void GDEFReuse(hotCtx g) {
    delete g->ctx.GDEFp;
    g->ctx.GDEFp = new GDEF(g);
}

void GDEFFree(hotCtx g) {
    delete g->ctx.GDEFp;
    g->ctx.GDEFp = nullptr;
}

void GDEF::GlyphClassTable::set(GPat::ClassRec &simple, GPat::ClassRec &ligature,
                                GPat::ClassRec &mark, GPat::ClassRec &component) {
    // Mark is first because glyphs there take presidence.
    glyphClasses[0] = mark;
    glyphClasses[1] = simple;
    glyphClasses[2] = ligature;
    glyphClasses[3] = component;

    std::unordered_map<GID, uint16_t> seen;
    bool hadConflictingClassDef {false};

    /* Check if the same glyph has ended up in more than one class, keep only first. */
    for (int8_t i = 0; i < 4; i++) {
        auto &glyphs = glyphClasses[i].glyphs;
        uint16_t j = 0;
        while (j < glyphs.size()) {
            GID gid = glyphs[j];
            auto [seeni, b] = seen.emplace(gid, i);
            if (!b) {
                // Don't complain if overriding mark class
                if (seeni->second != 0) {
                    hadConflictingClassDef = true;
                    if (h.g->convertFlags & HOT_VERBOSE) {
                        h.g->ctx.feat->dumpGlyph(gid, 0, 0);
                        h.g->logger->log(sWARNING, "GDEF GlyphClass. Glyph '%s' "
                                         "gid '%d'. glyph class '%s' overrides "
                                         "class '%s'.", h.g->getNote(), gid,
                                         names[seeni->second], names[i]);
                    }
                }
                glyphs.erase(glyphs.begin() + j);
            } else {
                j++;
            }
        }
    }

    if (hadConflictingClassDef && (h.g->convertFlags & HOT_VERBOSE)) {
        h.g->logger->log(sWARNING, "GDEF Glyph Class. Since there were conflicting "
                         "GlyphClass assignments, you should examine this GDEF table, "
                         "and make sure that the glyph class assignments are as "
                         "needed.\n");
    }
}

uint16_t GDEF::MarkSetFilteringTable::add(GPat::ClassRec &&markClassNode) {
    uint16_t i = 0;
    for (auto markNode : markSetClasses) {
        if (markClassNode == markNode)
            return i;
        i++;
    }

    markSetClasses.emplace_back(std::move(markClassNode));
    return (uint16_t) (markSetClasses.size() - 1);
}

// GDEF MarkAttachment class index starts at 1, not 0.
uint16_t GDEF::MarkAttachClassTable::add(GPat::ClassRec &&markClassNode) {
    uint16_t i = 0;
    for (auto markNode : glyphClasses) {
        if (markClassNode == markNode)
            return i + 1;
        i++;
    }

    uint32_t cnt = glyphClasses.size();
    if (cnt > kMaxMarkAttachClasses)
        return kMaxMarkAttachClasses + 1;

    glyphClasses.emplace_back(std::move(markClassNode));
    return cnt + 1;
}

bool GDEF::AttachTable::add(GID gid, uint16_t contour) {
    /* See if we can find matching GID entry in the attach point list.*/
    for (auto &ae : attachEntries) {
        if (ae.gid != gid)
            continue;
        /* Check if the contour is already in the list. */
        for (auto ci : ae.contourIndices)
            if (ci == contour)
                return true;
        ae.contourIndices.emplace_back(contour);
        return false;
    }
    AttachEntry ae {gid};
    ae.contourIndices.emplace_back(contour);
    attachEntries.emplace_back(std::move(ae));
    return false;
}

bool GDEF::LigCaretTable::warnGid(GID gid) {
    /* First, make sure that there is not another LGE for the same glyph
     We don't yet support format 3. */
    for (auto &lge : ligCaretEntries) {
        if (lge.gid == gid) {
            h.g->ctx.feat->dumpGlyph(lge.gid, 0, 0);
            h.g->logger->log(sWARNING, "GDEF Ligature Caret List Table. Glyph '%s' gid '%d'.\n A glyph can have at most one ligature glyph entry. Skipping entry.", h.g->getNote(), lge.gid);
            return true;
        }
    }
    return false;
}

void GDEF::LigCaretTable::addCoords(GID gid, ValueVector &coords,
                                    ValueVector &values) {
    if (warnGid(gid))
        return;

    LigGlyphEntry lge {gid};

    for (auto &c : coords) {
        auto cctp = std::make_unique<LigGlyphEntry::CoordCaretTable>(values.size());
        lge.caretTables.emplace_back(std::move(cctp));
        values.emplace_back(std::move(c));
    }

    ligCaretEntries.emplace_back(std::move(lge));
}

void GDEF::LigCaretTable::addPoints(GID gid, std::vector<uint16_t> &points) {
    if (warnGid(gid))
        return;

    LigGlyphEntry lge {gid};

    for (auto &p : points)
        lge.caretTables.emplace_back(std::make_unique<LigGlyphEntry::PointCaretTable>(p));

    ligCaretEntries.emplace_back(std::move(lge));
}

Offset GDEF::GlyphClassTable::fill(Offset o) {
    /* There are always either none, or 4 glyph classes.
     * First glyph class is class index 1. */
    assert(o);
    int i = -1;
    for (int j = 0; j < 4; j++) {
        auto &gc = glyphClasses[j];
        i++;
        if (gc.glyphs.size() == 0)
            continue;
        if (!offset) {
            offset = o;
            cac.classBegin();
        }
        for (GID gid : gc.glyphs)
            cac.classAddMapping(gid, classIDmap[i]);
    }
    if (offset) {
        cac.classEnd();
        return cac.classSize();
    } else
        return 0;
}

Offset GDEF::AttachTable::fill(Offset o) {
    /* Classes start numbering from 1 */
    uint32_t cnt = attachEntries.size();
    if (cnt == 0)
        return 0;

    offset = o;
    Offset sz = (Offset)size(cnt);
    std::sort(attachEntries.begin(), attachEntries.end());

    cac.coverageBegin();
    for (auto &ae : attachEntries) {
        ae.offset = sz;
        sz += ae.size(ae.contourIndices.size());
        cac.coverageAddGlyph(ae.gid);
        std::sort(ae.contourIndices.begin(), ae.contourIndices.end());
    }
    cac.coverageEnd();

    Coverage = sz;
    sz += (Offset)cac.coverageSize();
    return sz;
}

// Classes start numbering from 1
Offset GDEF::LigCaretTable::fill(Offset o) {
    uint32_t cnt = ligCaretEntries.size();
    if (cnt == 0)
        return 0;

    offset = o;
    Offset sz = (Offset)size(cnt);
    std::sort(ligCaretEntries.begin(), ligCaretEntries.end());

    cac.coverageBegin();
    LigGlyphEntry::CaretTable::comparator ctc {h.values};
    for (auto &lge : ligCaretEntries) {
        lge.offset = sz;
        LOffset caretOffset = lge.size(lge.caretTables.size());
        sz += caretOffset;
        /* we will write the caret tables right after each lge entry */
        std::stable_sort(lge.caretTables.begin(), lge.caretTables.end(), ctc);
        for (auto &ct : lge.caretTables) {
            ct->offset = caretOffset;
            caretOffset += ct->size();
            sz += ct->size();
        }
        cac.coverageAddGlyph(lge.gid);
    }
    cac.coverageEnd();
    Coverage = sz;
    sz += (Offset)cac.coverageSize();
    return sz;
}

void GDEF::MarkAttachClassTable::validate() {
    bool hadConflictingClassDef {false};

    std::unordered_map<GID, const char *> seen;

    /* Check and warn if the same glyph has ended up in more than one class */
    for (auto &gc : glyphClasses) {
        const char *className = gc.markClassName.c_str();
        if (className == NULL)
            className = "[unnamed mark attachment class]";

        for (GID gid : gc.glyphs) {
            auto [seeni, b] = seen.emplace(gid, className);
            if (!b) {
                hadConflictingClassDef = true;
                if (h.g->convertFlags & HOT_VERBOSE) {
                    h.g->ctx.feat->dumpGlyph(gid, 0, 0);
                    h.g->logger->log(sWARNING,
                                     "GDEF MarkAttachment. Glyph '%s' gid '%d'. "
                                     "previous glyph class '%s' conflicts with new "
                                     "class '%s'.", h.g->getNote(), gid,
                                     seeni->second, className);
                }
            }
        }
    }
    if (hadConflictingClassDef && (h.g->convertFlags & HOT_VERBOSE)) {
        h.g->logger->log(sWARNING, "GDEF MarkAttachment Classes. There are "
                         "conflicting MarkAttachment assignments.");
    }
}

// Classes start numbering from 1
Offset GDEF::MarkAttachClassTable::fill(Offset o) {
    uint32_t cnt = glyphClasses.size();
    if (cnt == 0)
        return 0;

    offset = o;
    for (auto &gc : glyphClasses)
        gc.makeUnique(h.g, true);

    validate();
    // Add the to the OTL table.
    cac.classBegin();
    int i = -1;
    for (auto &cr : glyphClasses) {
        i++;
        for (GID gid : cr.glyphs)
            cac.classAddMapping(gid, i + 1);
    }
    cac.classEnd();
    return cac.classSize();
}

Offset GDEF::MarkSetFilteringTable::fill(Offset o) {
    uint32_t cnt = markSetClasses.size();
    if (cnt == 0)
        return 0;

    offset = o;
    Offset sz = (Offset)size(cnt);
    markSetTableFormat = 1;
    for (auto &cr : markSetClasses) {
        MarkSetEntry mse {h};
        auto &cac = mse.cac;

        cac.coverageBegin();
        for (GID gid : cr.glyphs)
            cac.coverageAddGlyph(gid, true);
        cac.coverageEnd();

        mse.MarkSetCoverage = sz;
        sz += (Offset)cac.coverageSize();
        markSetEntries.emplace_back(std::move(mse));
    }
    markSetClasses.clear();
    return sz;
}

void GDEF::AttachTable::write(GDEF *h) {
    if (!offset)
        return;
    OUT2(Coverage);
    OUT2((uint16_t) attachEntries.size());

    for (auto &ae : attachEntries)
        OUT2(ae.offset);

    for (auto &ae : attachEntries) {
        OUT2((uint16_t) ae.contourIndices.size());
        for (auto ci : ae.contourIndices)
            OUT2(ci);
    }
    cac.coverageWrite();
}

void GDEF::LigCaretTable::write(GDEF *h) {
    if (!offset)
        return;
    OUT2(Coverage);
    OUT2((uint16_t) ligCaretEntries.size());

    for (auto &lge : ligCaretEntries)
        OUT2(lge.offset);

    for (auto &lge : ligCaretEntries) {
        OUT2((uint16_t)lge.caretTables.size());
        /* write offsets */
        for (auto &ct : lge.caretTables)
            OUT2(ct->offset);
        /* then write caret tables for this lge */
        for (auto &ct : lge.caretTables)
            ct->write(h, h->values);
    }
    cac.coverageWrite();
}

void GDEF::MarkSetFilteringTable::write(GDEF *h) {
    if (!offset)
        return;
    OUT2(markSetTableFormat);
    OUT2((uint16_t)markSetEntries.size());

    /* first write the offsets to the coverage tables */
    for (auto mse : markSetEntries)
        OUT4(mse.MarkSetCoverage);

    /* Now write the coverage tables */
    for (auto mse : markSetEntries)
        mse.cac.coverageWrite();
}
