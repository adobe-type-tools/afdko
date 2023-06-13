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

    if ((tableSize = glyphClassTable.fill(g, offset)) > 0) {
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

    if ((tableSize = markAttachClassTable.fill(g, offset)) > 0) {
        offset += (Offset)tableSize;
        haveData = true;
    }

    if ((tableSize = markSetClassTable.fill(g, offset)) > 0) {
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

void GDEF::GlyphClassTable::set(GNode *simple, GNode *ligature,
                                GNode *mark, GNode *component, hotCtx g) {
    // Mark is first because glyphs there take presidence.
    glyphClasses[0] = mark;
    glyphClasses[1] = simple;
    glyphClasses[2] = ligature;
    glyphClasses[3] = component;

    std::unordered_map<GID, uint16_t> seen;
    bool hadConflictingClassDef {false};

    /* Check if the same glyph has ended up in more than one class, keep only first. */
    int16_t i = -1;
    for (auto &gc : glyphClasses) {
        i++;
        GNode *prev {NULL}, *cur {gc};
        while (cur != NULL) {
            auto [seeni, b] = seen.emplace(cur->gid, i);
            if (!b) {
                // Don't complain if overriding mark class
                if (seeni->second != 0) {
                    hadConflictingClassDef = true;
                    if (g->convertFlags & HOT_VERBOSE) {
                        featGlyphDump(g, cur->gid, 0, 0);
                        hotMsg(g, hotWARNING, "GDEF GlyphClass. Glyph '%s' "
                               "gid '%d'. glyph class '%s' overrides "
                               "class '%s'.", g->note.array, cur->gid,
                               names[seeni->second], names[i]);
                    }
                }
                if (prev == NULL) {
                    if (gc->nextCl == NULL) {
                        gc = NULL;
                    } else {
                        // some flags are set only in the head node
                        gc->nextCl->flags |= gc->flags;  // XXX do we want the | here?
                        gc = gc->nextCl;
                    }
                    cur->nextCl = NULL;
                    featRecycleNodes(g, cur);
                    cur = gc;
                } else {
                    prev->nextCl = prev->nextCl->nextCl;
                    cur->nextCl = NULL;
                    featRecycleNodes(g, cur);
                    cur = prev->nextCl;
                }
            } else {
                prev = cur;
                cur = cur->nextCl;
            }
        }
    }
    if (hadConflictingClassDef && (g->convertFlags & HOT_VERBOSE)) {
        hotMsg(g, hotWARNING, "GDEF Glyph Class. Since there were conflicting "
               "GlyphClass assignments, you should examine this GDEF table, "
               "and make sure that the glyph class assignments are as "
               "needed.\n");
    }
}

uint16_t GDEF::MarkSetFilteringTable::add(GNode *markClassNode) {
    uint16_t i = 0;
    for (auto markNode : markSetClasses) {
        if (markClassNode == markNode)
            return i;
        i++;
    }

    markSetClasses.emplace_back(markClassNode);
    return (uint16_t) (markSetClasses.size() - 1);
}

// GDEF MarkAttachment class index starts at 1, not 0.
uint16_t GDEF::MarkAttachClassTable::add(GNode *markClassNode) {
    uint16_t i = 0;
    for (auto markNode : glyphClasses) {
        if (markClassNode == markNode)
            return i + 1;
        i++;
    }

    uint32_t cnt = glyphClasses.size();
    if (cnt > kMaxMarkAttachClasses)
        return kMaxMarkAttachClasses + 1;

    glyphClasses.emplace_back(markClassNode);
    return cnt + 1;
}

bool GDEF::AttachTable::add(GNode *glyphNode, uint16_t contour) {
    /* See if we can find matching GID entry in the attach point list.*/
    for (auto &ae : attachEntries) {
        if (ae.gid != glyphNode->gid)
            continue;
        /* Check if the contour is already in the list. */
        for (auto ci : ae.contourIndices)
            if (ci == contour)
                return true;
        ae.contourIndices.emplace_back(contour);
        return false;
    }
    AttachEntry ae {glyphNode->gid};
    ae.contourIndices.emplace_back(contour);
    attachEntries.emplace_back(std::move(ae));
    return false;
}

void GDEF::LigCaretTable::add(GNode *glyphNode, std::vector<uint16_t> &caretValues,
                              uint16_t format, hotCtx g) {
    /* First, make sure that there is not another LGE for the same glyph
     We don't yet support format 3. */
    for (auto &lge : ligCaretEntries) {
        if (lge.gid == glyphNode->gid) {
            featGlyphDump(g, lge.gid, 0, 0);
            hotMsg(g, hotWARNING, "GDEF Ligature Caret List Table. Glyph '%s' gid '%d'.\n A glyph can have at most one ligature glyph entry. Skipping entry for format '%d'.", g->note.array, lge.gid, format);
            return;
        }
    }
    LigGlyphEntry lge {glyphNode->gid, format};

    for (auto cv : caretValues)
        lge.caretTables.emplace_back(cv, format);

    ligCaretEntries.emplace_back(std::move(lge));
}

Offset GDEF::GlyphClassTable::fill(hotCtx g, Offset o) {
    /* There are always either none, or 4 glyph classes.
     * First glyph class is class index 1. */
    assert(o);
    int i = -1;
    for (auto &gc : glyphClasses) {
        i++;
        if (gc == NULL)
            continue;
        if (!offset) {
            offset = o;
            otl.classBegin();
        }
        for (GNode *p = gc; p != NULL; p = p->nextCl)
            otl.classAddMapping(p->gid, classIDmap[i]);
        featRecycleNodes(g, gc); /* Don't need this class any more*/
        gc = NULL;
    }
    if (offset) {
        otl.classEnd();
        return otl.getClassSize();
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

    otl.coverageBegin();
    for (auto &ae : attachEntries) {
        ae.offset = sz;
        sz += ae.size(ae.contourIndices.size());
        otl.coverageAddGlyph(ae.gid);
        std::sort(ae.contourIndices.begin(), ae.contourIndices.end());
    }
    otl.coverageEnd();

    Coverage = sz;
    sz += (Offset)otl.getCoverageSize();
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

    otl.coverageBegin();
    for (auto &lge : ligCaretEntries) {
        lge.offset = sz;
        LOffset caretOffset = lge.size(lge.caretTables.size());
        sz += caretOffset;
        /* we will write the caret tables right after each lge entry */
        std::sort(lge.caretTables.begin(), lge.caretTables.end());
        for (auto &ct : lge.caretTables) {
            ct.offset = caretOffset;
            caretOffset += ct.size();
            sz += ct.size();
        }
        otl.coverageAddGlyph(lge.gid);
    }
    otl.coverageEnd();
    Coverage = sz;
    sz += (Offset)otl.getCoverageSize();
    return sz;
}

void GDEF::MarkAttachClassTable::validate(hotCtx g) {
    bool hadConflictingClassDef {false};

    std::unordered_map<GID, const char *> seen;

    /* Check and warn if the same glyph has ended up in more than one class */
    for (auto gc : glyphClasses) {
        const char *className = gc->markClassName;
        if (className == NULL)
            className = "[unnamed mark attachment class]";

        for (GNode *p = gc; p != NULL; p = p->nextCl) {
            auto [seeni, b] = seen.emplace(p->gid, className);
            if (!b) {
                hadConflictingClassDef = true;
                if (g->convertFlags & HOT_VERBOSE) {
                    featGlyphDump(g, p->gid, 0, 0);
                    hotMsg(g, hotWARNING,
                           "GDEF MarkAttachment. Glyph '%s' gid '%d'. "
                           "previous glyph class '%s' conflicts with new "
                           "class '%s'.", g->note.array, p->gid,
                           seeni->second, className);
                }
            }
        }
    }
    if (hadConflictingClassDef && (g->convertFlags & HOT_VERBOSE)) {
        hotMsg(g, hotWARNING, "GDEF MarkAttachment Classes. There are "
                              "conflicting MarkAttachment assignments.");
    }
}

// Classes start numbering from 1
Offset GDEF::MarkAttachClassTable::fill(hotCtx g, Offset o) {
    uint32_t cnt = glyphClasses.size();
    if (cnt == 0)
        return 0;

    offset = o;
    // Copy Sort the glyph classes.
    for (auto &srcNode : glyphClasses) {
        if (srcNode == NULL) {
            continue;
        }

        /* The mark attach glyph classes are all named classes. Named class */
        /* glyphs get recycled when hashFree() is called, so we need to     */
        /* make a copy of these classes here, and recycle the copy after    */
        /* use. This is because we delete glyphs from within the class      */
        /* lists to eliminate duplicates. If we operate on a named class    */
        /* list, then any deleted duplicated glyphs gets deleted again when */
        /* hashFree() is called. Also, the hash element head points to the  */
        /* original first glyph, which may be sorted further down the list. */
        GNode *headNode;
        featGlyphClassCopy(g, &headNode, srcNode);
        featGlyphClassSort(g, &headNode, 1, 1);
        srcNode = headNode;
    }

    validate(g);
    // Add the to the OTL table.
    otl.classBegin();
    int i = -1;
    for (auto &srcNode : glyphClasses) {
        i++;
        if (srcNode == NULL)
            continue;

        for (GNode *p = srcNode; p != NULL; p = p->nextCl)
            otl.classAddMapping(p->gid, i + 1);

        featRecycleNodes(g, srcNode); /* Don't need this class any more*/
        srcNode = NULL;
    }
    otl.classEnd();
    return otl.getClassSize();
}

Offset GDEF::MarkSetFilteringTable::fill(hotCtx g, Offset o) {
    uint32_t cnt = markSetClasses.size();
    if (cnt == 0)
        return 0;

    offset = o;
    Offset sz = (Offset)size(cnt);
    markSetTableFormat = 1;
    for (auto &srcNode : markSetClasses) {
        MarkSetEntry mse {g};
        otlTbl &otl = mse.otl;

        otl.coverageBegin();
        for (GNode *p = srcNode; p != NULL; p = p->nextCl)
            otl.coverageAddGlyph(p->gid, true);
        otl.coverageEnd();

        mse.MarkSetCoverage = sz;
        sz += (Offset)otl.getCoverageSize();
        markSetEntries.emplace_back(std::move(mse));
        srcNode = NULL;   // XXX Figure out memory use later
    }
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
    otl.coverageWrite();
}

void GDEF::LigCaretTable::write(GDEF *h) {
    if (!offset)
        return;
    OUT2(Coverage);
    OUT2((uint16_t) ligCaretEntries.size());

    for (auto lge : ligCaretEntries)
        OUT2(lge.offset);

    for (auto lge : ligCaretEntries) {
        OUT2((uint16_t)lge.caretTables.size());
        /* write offsets */
        for (auto ct : lge.caretTables)
            OUT2(ct.offset);
        /* then write caret tables for this lge */
        for (auto ct : lge.caretTables) {
            OUT2(ct.format);
            OUT2(ct.CaretValue);
        }
    }
    otl.coverageWrite();
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
        mse.otl.coverageWrite();
}
