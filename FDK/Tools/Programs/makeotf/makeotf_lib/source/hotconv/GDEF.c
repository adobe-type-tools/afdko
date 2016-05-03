/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Font GDEFer table.
 */
#include <stdlib.h>
#include "GDEF.h"
#include "otl.h"
#include "feat.h"

/* --------------------------- Context Definition -------------------------- */
typedef struct {            /* Subtable record */
	LOffset offset;         /* From beginning of first subtable */
	void *tbl;              /* Format-specific subtable data */
} Subtable;

/* ---------------------------- Table Definition --------------------------- */


typedef struct {
	unsigned long version;
	Offset GlyphClassDefOffset;
	Offset AttachTableOffset;
	Offset LigCaretTableOffset;
	Offset MarkClassDefOffset;
	Offset MarkGlyphSetsDefOffset;
} GDEFTbl;

#define GDEF_HEADER_VERSION10_SIZE (sizeof(Fixed) + (4 * sizeof(Offset)))

typedef struct {
	unsigned short PointCount;
	unsigned short gid;
	Offset offset;
	dnaDCL(unsigned short, contourIndices);
} AttachEntry;
#define ATTACH_ENTRY_SIZE(numContours) (uint16 + uint16 * (numContours))

typedef struct {
	Offset Coverage;
	unsigned short GlyphCount;
	Offset AttachPoint;
	otlTbl otl;
} AttachTable;
#define ATTACH_TABLE_SIZE(glyphCount)   (uint16 * 2 + uint16 * (glyphCount))


typedef struct {
	unsigned short Coverage;
	unsigned short LigGlyphCount;
	Offset LigGlyph;
	otlTbl otl;
} LigCaretTable;
#define LIG_CARET_TABLE_SIZE(numGlyphs)  (uint16 * 2 + uint16 * (numGlyphs))

typedef struct {
	unsigned short format;
	unsigned short CaretValue;
	unsigned short offset;
} CaretTable;
#define CARET_TABLE_SIZE (uint16 * 2)

typedef struct {
	unsigned short CaretCount;
	unsigned short gid;
	dnaDCL(CaretTable, caretTables);
	Offset offset;
} LigGlyphEntry;
# define LIG_GLYPH_ENTRY_SIZE(numCarets)  (uint16 + uint16 * (numCarets))


typedef struct {
	otlTbl otl;
	LOffset MarkSetCoverage;
} MarkSetEntry;

typedef struct {
	unsigned short MarkSetTableFormat;
	unsigned short MarkSetCount;
	dnaDCL(MarkSetEntry, markSetEntries);
} MarkSetFilteringTable;

#define MARK_SET_TABLE_SIZE(MarkSetCount)   (uint16 * 2 + uint32 * (MarkSetCount))


static LOffset createGlyphClassDef(GDEFCtx h);
static Offset createAttachTableDef(GDEFCtx h);
static Offset createLigCaretTableDef(GDEFCtx h);
static LOffset createMarkAttachClassDef(GDEFCtx h);
static LOffset createMarkSetClassDef(GDEFCtx h, hotCtx g);
static void writeAttachTable(hotCtx g, GDEFCtx h);
static void writeLigCaretTable(hotCtx g, GDEFCtx h);
static void writeMarkSetClassDef(hotCtx g, GDEFCtx h);
static void writeMarkSetClassTable(hotCtx g, GDEFCtx h);

/* --------------------------- Context Definition -------------------------- */
struct GDEFCtx_ {
	GDEFTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
	dnaDCL(GNode *, glyphClasses);
	dnaDCL(AttachEntry, attachEntries);
	dnaDCL(LigGlyphEntry, ligCaretEntries);
	dnaDCL(GNode *, markAttachClasses);
	dnaDCL(GNode *, markSetClasses);

	otlTbl glyphClassTable;
	AttachTable attachTable;
	LigCaretTable ligCaretTable;
	otlTbl markAttachClassTable;
	MarkSetFilteringTable markSetClassTable;
	Offset offset;
};

/* --------------------------- Standard Functions -------------------------- */

void GDEFNew(hotCtx g) {
	GDEFCtx h = MEM_NEW(g, sizeof(struct GDEFCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.GDEF = h;
	dnaINIT(g->dnaCtx, h->glyphClasses, 50, 200);
	dnaINIT(g->dnaCtx, h->attachEntries, 50, 200);
	dnaINIT(g->dnaCtx, h->ligCaretEntries, 50, 200);
	dnaINIT(g->dnaCtx, h->markAttachClasses, 50, 200);
	dnaINIT(g->dnaCtx, h->markSetClasses, 50, 200);

	h->offset = 0;
	h->tbl.GlyphClassDefOffset = 0;
	h->tbl.AttachTableOffset = 0;
	h->tbl.LigCaretTableOffset = 0;
	h->tbl.MarkClassDefOffset = 0;
	h->tbl.MarkGlyphSetsDefOffset = 0;

	h->glyphClassTable = NULL;
	h->attachTable.otl = NULL;
	h->attachTable.GlyphCount = 0;
	h->ligCaretTable.otl = NULL;
	h->ligCaretTable.LigGlyphCount = 0;
	h->markAttachClassTable = NULL;
}

int GDEFFill(hotCtx g) {
	GDEFCtx h = g->ctx.GDEF;
	LOffset tableSize;
	int haveData = 0;

	h->tbl.version  = 0x00010000;
	h->offset = GDEF_HEADER_VERSION10_SIZE;

	if (h->markSetClasses.cnt > 0) {
		h->offset += sizeof(Offset);
		h->tbl.version  = 0x00010002;
	}

	if (h->glyphClasses.cnt > 0) {
		h->tbl.GlyphClassDefOffset = h->offset;
		h->glyphClassTable =  otlTableNew(g);
		tableSize = createGlyphClassDef(h);
		h->offset += (Offset)tableSize;
		haveData = 1;
	}
	else {
		h->tbl.GlyphClassDefOffset = 0;
	}

	if (h->attachEntries.cnt > 0) {
		h->tbl.AttachTableOffset = h->offset;
		h->attachTable.otl =  otlTableNew(g);
		tableSize = createAttachTableDef(h);
		h->offset += (Offset)tableSize;
		haveData = 1;
	}
	else {
		h->tbl.AttachTableOffset = 0;
	}

	if (h->ligCaretEntries.cnt > 0) {
		h->tbl.LigCaretTableOffset = h->offset;
		h->ligCaretTable.otl =  otlTableNew(g);
		tableSize = createLigCaretTableDef(h);
		h->offset += (Offset)tableSize;
		haveData = 1;
	}
	else {
		h->tbl.LigCaretTableOffset = 0;
	}

	if (h->markAttachClasses.cnt > 0) {
		h->tbl.MarkClassDefOffset = h->offset;
		h->markAttachClassTable =  otlTableNew(g);
		tableSize = createMarkAttachClassDef(h);
		h->offset += (Offset)tableSize;
		haveData = 1;
	}
	else {
		h->tbl.MarkClassDefOffset = 0;
	}

	if (h->markSetClasses.cnt > 0) {
		h->tbl.MarkGlyphSetsDefOffset = h->offset;
		tableSize = createMarkSetClassDef(h, g);
		h->offset += (Offset)tableSize;
		haveData = 1;
	}
	else {
		h->tbl.MarkGlyphSetsDefOffset = 0;
	}

	return haveData;
}

void GDEFWrite(hotCtx g) {
	GDEFCtx h = g->ctx.GDEF;

	/* Perform checksum adjustment */
	OUT4(h->tbl.version);
	OUT2(h->tbl.GlyphClassDefOffset);
	OUT2(h->tbl.AttachTableOffset);
	OUT2(h->tbl.LigCaretTableOffset);
	OUT2(h->tbl.MarkClassDefOffset);
	if (h->tbl.MarkGlyphSetsDefOffset) {
		/* If this is zero, then we write a Verion 1,0 table, whcih doesn't have this offset. */
		OUT2(h->tbl.MarkGlyphSetsDefOffset);
	}

	if (h->tbl.GlyphClassDefOffset) {
		otlClassWrite(g, h->glyphClassTable);
	}
	if ((h->tbl.AttachTableOffset)) {
		writeAttachTable(g, h);
	}
	if ((h->tbl.LigCaretTableOffset)) {
		writeLigCaretTable(g, h);
	}
	if (h->tbl.MarkClassDefOffset) {
		otlClassWrite(g, h->markAttachClassTable);
	}
	if (h->tbl.MarkGlyphSetsDefOffset) {
		writeMarkSetClassTable(g, h);
	}
}

void GDEFReuse(hotCtx g) {
}

void GDEFFree(hotCtx g) {
	int i;
	GDEFCtx h = g->ctx.GDEF;

	dnaFREE(h->glyphClasses);
	if (h->glyphClassTable != NULL) {
		otlTableFree(g, h->glyphClassTable);
	}

	for (i = 0; i < h->attachEntries.cnt; i++) {
		dnaFREE(h->attachEntries.array[i].contourIndices);
	}
	dnaFREE(h->attachEntries);
	if (h->attachTable.otl != NULL) {
		otlTableFree(g, h->attachTable.otl);
	}

	for (i = 0; i < h->ligCaretEntries.cnt; i++) {
		dnaFREE(h->ligCaretEntries.array[i].caretTables);
	}
	dnaFREE(h->ligCaretEntries);
	if (h->ligCaretTable.otl != NULL) {
		otlTableFree(g, h->ligCaretTable.otl);
	}

	dnaFREE(h->markAttachClasses);
	if (h->markAttachClassTable != NULL) {
		otlTableFree(g, h->markAttachClassTable);
	}

	dnaFREE(h->markSetClasses);
	if (h->tbl.MarkGlyphSetsDefOffset != 0) {
		for (i = 0; i < h->markSetClassTable.markSetEntries.cnt; i++) {
			MarkSetEntry *markSetEntry = dnaINDEX(h->markSetClassTable.markSetEntries, i);
			otlTableFree(g, markSetEntry->otl);
		}
		dnaFREE(h->markSetClassTable.markSetEntries);
	}

	MEM_FREE(g, g->ctx.GDEF);
}

static void removeNodeFromClass(GDEFCtx h, GNode **glyphClass, GNode **headNode, GNode **prevNode, GNode **curNode) {
	if (*curNode == *headNode) {
		/* move the second GNode to the start of the class. */
		if ((*curNode)->nextCl == NULL) {
			*glyphClass = NULL;
			*curNode = NULL;
			*headNode = NULL;
		}
		else {
			(*curNode)->nextCl->flags = (*curNode)->flags;                         /* some flags are set only in the head node */
			(*curNode) = (*curNode)->nextCl;
			*glyphClass = *curNode;
			*headNode =  *curNode;
			*prevNode = NULL;
		}
	}
	else {
		if ((*prevNode) != NULL) {
			/* prev remains the same, p2 is set to the next class, and gets tested on the next loop. */
			(*prevNode)->nextCl = (*curNode)->nextCl;
			(*curNode) = (*curNode)->nextCl;
		}
	}
	return;
}

void setGlyphClassGDef(hotCtx g, GNode *simple, GNode *ligature, GNode *mark,
                       GNode *component) {
	GDEFCtx h = g->ctx.GDEF;
	GNode **simpleClass;
	GNode **ligClass;
	GNode **markClass;
	GNode **compClass;
	int hadConflictingClassDef = 0;

    if (h->glyphClasses.cnt > 0)
    {
         /* Have seen previous GlyphClassDef. Can index into list */
        simpleClass = h->glyphClasses.array[0];
        ligClass = h->glyphClasses.array[1];
        markClass = h->glyphClasses.array[2];
        compClass = h->glyphClasses.array[3];
    }
    else
    {
        simpleClass = dnaNEXT(h->glyphClasses);
        ligClass = dnaNEXT(h->glyphClasses);
        markClass = dnaNEXT(h->glyphClasses);
        compClass = dnaNEXT(h->glyphClasses);
    }
        
	char *glyphClassNames[] = {
		"Base", "Ligature", "Mark", "Component"
	};

	long i, j;

	if (simple == NULL) {
		*simpleClass = NULL;
	}
	else {
		*simpleClass = simple;
	}
	if (ligature == NULL) {
		*ligClass = NULL;
	}
	else {
		*ligClass = ligature;
	}

	if (mark == NULL) {
		*markClass = NULL;
	}
	else {
		*markClass = mark;
	}

	if (component == NULL) {
		*compClass = NULL;
	}
	else {
		*compClass = component;
	}


	/* Check and warn if the same glyph has ended up in more than one class */
	for (i = 0; i < h->glyphClasses.cnt; i++) {
		GNode *p1;
		GNode *prev1 = NULL;
		GNode *headNode1 = h->glyphClasses.array[i];
		if (headNode1 == NULL) {
			continue;
		}

		for (j = i + 1; j < h->glyphClasses.cnt; j++) {
			GNode *p2;
			GNode *headNode2 = h->glyphClasses.array[j];
			if (headNode2 == NULL) {
				continue;
			}
			for (p1 = headNode1; p1 != NULL; ) {
				GNode *prev2 = NULL;
				unsigned int removedP1 = 0;
				for (p2 = headNode2; p2 != NULL; ) {
					if (p1->gid == p2->gid) {
						if (j != 2) {
							// remove glyph from current class.
							if (i != 2) {
								/* If the gid in class j is being overridden by the mark class definition, then we do not complain */
								hadConflictingClassDef = 1;
								featGlyphDump(g, p1->gid, 0, 0);
								hotMsg(g, hotWARNING, "GDEF GlyphClass. Glyph '%s' gid '%d'. previous glyph class '%s' overrides new class '%s'.", g->note.array, p1->gid, glyphClassNames[i],  glyphClassNames[j]);
							}
							removeNodeFromClass(h, &(h->glyphClasses.array[j]), &headNode2, &prev2, &p2);
						}
						else {
							// class j is the mark class; we need to remove p1 from the glyph class i. We do this silently, as the mark class always supersedes other classes.
							removeNodeFromClass(h, &(h->glyphClasses.array[i]), &headNode1, &prev1, &p1);
							removedP1 = 1;
						}
					}
					else {
						prev2 = p2;
						p2 = p2->nextCl;
					}

					if (p1 == NULL) {
						break; /* This can happen because all the p1's get removed by mark class overrides above */
					}
				}
				if (!removedP1) {
					prev1 = p1;
					if (p1 != NULL) {
						p1 = p1->nextCl;
					}
				}
			}
		}
	}
	if (hadConflictingClassDef) {
		hotMsg(g, hotWARNING, "GDEF Glyph Class. Since there were conflicting GlyphClass assignments, you should examine this GDEF table, and make sure that the glyph class assignments are as needed.\n");
	}
}

unsigned short addMarkSetClassGDEF(hotCtx g, GNode *markClassNode) {
	int i;
	unsigned char classIndex = 0;
	GNode *markNode;
	GDEFCtx h = g->ctx.GDEF;

	for (i = 0; i < h->markSetClasses.cnt; i++) {
		markNode = *((GNode **)dnaINDEX(h->markSetClasses, i));
		if (markClassNode == markNode) {
			classIndex = i + 1; /* so that 0 means no value was assigned. */
			break;
		}
	}

	if (classIndex == 0) {
		GNode **nextMarkClass = dnaNEXT(h->markSetClasses);
		*nextMarkClass = markClassNode;
		classIndex = (unsigned char)h->markSetClasses.cnt;
	}
	return classIndex - 1;
}

unsigned short addGlyphMarkClassGDEF(hotCtx g, GNode *markClassNode) {
	int i;
	unsigned char classIndex = 0;
	GNode *markNode;
	GDEFCtx h = g->ctx.GDEF;

	for (i = 0; i < h->markAttachClasses.cnt; i++) {
		markNode = *((GNode **)dnaINDEX(h->markAttachClasses, i));
		if (markClassNode == markNode) {
			classIndex = i + 1; /* GDEF MarkAttachment class index starts at 1, not 0. */
			break;
		}
	}

	if (classIndex == 0) {
		GNode **nextMarkClass;
		if (h->markAttachClasses.cnt > kMaxMarkAttachClasses) {
			return kMaxMarkAttachClasses + 1; /* trigger error msg in caller of this function */
		}
		nextMarkClass = dnaNEXT(h->markAttachClasses);
		*nextMarkClass = markClassNode;
		classIndex = (unsigned char)h->markAttachClasses.cnt;
	}

	return classIndex;
}

int addAttachEntryGDEF(hotCtx g, GNode *glyphNode, unsigned short contour) {
	int i;
	AttachEntry *attachEntry = NULL;
	GID gid = glyphNode->gid;
	GDEFCtx h = g->ctx.GDEF;
	int seenContour = 0;
	/* See if we can find matching GID entry in the attach point list.*/
	i = 0;
	while (i <  h->attachEntries.cnt) {
		if (h->attachEntries.array[i].gid == gid) {
			unsigned short j, *indexEntry;
			attachEntry = &h->attachEntries.array[i];
			/* make sure the contour isn't already in the list. */
			for (j = 0; j < attachEntry->contourIndices.cnt; j++) {
				if (attachEntry->contourIndices.array[j] == contour) {
					seenContour = 1;
					break;
				}
			}
			if (!seenContour) {
				indexEntry = dnaNEXT(attachEntry->contourIndices);
				*indexEntry = contour;
			}
			break;
		}
		i++;
	}
	if (attachEntry == NULL) {
		unsigned short *indexEntry;
		attachEntry = dnaNEXT(h->attachEntries);
		attachEntry->gid = gid;
		dnaINIT(g->dnaCtx, attachEntry->contourIndices, 10, 10);
		indexEntry = dnaNEXT(attachEntry->contourIndices);
		*indexEntry = contour;
	}
	return seenContour;
}

int addLigCaretEntryGDEF(hotCtx g, GNode *glyphNode, unsigned short caretValue, unsigned short format) {
    int i;
    LigGlyphEntry *lge = NULL;
    GID gid = glyphNode->gid;
    GDEFCtx h = g->ctx.GDEF;
    int seenCaretValue = 0;
    CaretTable *ct;
    
    if (h->ligCaretEntries.cnt == 0)
    {
        lge = dnaNEXT(h->ligCaretEntries);
        lge->gid = gid;
        dnaINIT(g->dnaCtx, lge->caretTables, 10, 10);
        ct = dnaNEXT(lge->caretTables);
        ct->format = format;
        ct->CaretValue = caretValue;
    }
    else
    {
        /* We allow only one device entry and either one byPos or byIndex entry */
        i = 0;
        while (i <  h->ligCaretEntries.cnt) {
            if (h->ligCaretEntries.array[i].gid == gid) {
                unsigned short j;
                CaretTable *ct;
                lge = &h->ligCaretEntries.array[i];
                for (j = 0; j < lge->caretTables.cnt; j++) {
                    if (lge->caretTables.array[j].format == format) {
                        seenCaretValue = 1;
                        break;
                    }
                    if (((lge->caretTables.array[j].format == 1) &&
                         (lge->caretTables.array[j].format == 2))
                        ||
                        ((lge->caretTables.array[j].format == 2) &&
                         (lge->caretTables.array[j].format == 1)))
                    {
                        featGlyphDump(g, gid, 0, 0);
                        seenCaretValue = 1;
                        break;
                    }
                }
                if (!seenCaretValue) {
                    ct = dnaNEXT(lge->caretTables);
                    ct->format = format;
                    ct->CaretValue = caretValue;
                }
                break;
            }
            i++;
        }
    }
    if (seenCaretValue) {
        featGlyphDump(g, gid, 0, 0);
        hotMsg(g, hotWARNING, "GDEF Ligature Caret List Table. Glyph '%s' gid '%d'.\n A glyph can have at most one ligature caret device  statement and one of either\n ligature caret by position or ligature caret by index statetment. Skipping entry for format '%d'.", g->note.array, gid, format);
        return seenCaretValue;
    }
}

static LOffset createGlyphClassDef(GDEFCtx h) {
	/* There are always either none, or 4 glyph classes. First glyph class is  class index 1. */

	otlTbl otl = h->glyphClassTable;
	int i;
	otlClassBegin(h->g, otl);
	for (i = 0; i < h->glyphClasses.cnt; i++) {
		GNode *p;
		GNode *headNode = h->glyphClasses.array[i];
		if (headNode == NULL) {
			continue;
		}

		for (p = headNode; p != NULL; p = p->nextCl) {
			otlClassAddMapping(h->g, otl, p->gid, i + 1);
		}
		featRecycleNodes(h->g, headNode); /* Don't need this class any more*/
		h->glyphClasses.array[i] = NULL;
	}
	otlClassEnd(h->g, otl);
	return otlGetClassSize(otl);
}

static int cmpAttachEntries(const void *first, const void *second) {
	GID a = ((AttachEntry *)first)->gid;
	GID b = ((AttachEntry *)second)->gid;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	return 0;
}

static int cmpContourIndicies(const void *first, const void *second) {
	unsigned short a = *(unsigned short *)first;
	unsigned short b = *(unsigned short *)second;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	return 0;
}

static Offset createAttachTableDef(GDEFCtx h) {
	/* Classes start numbering from 1 */

	int i;
	otlTbl otl = h->attachTable.otl;
	Offset size = (Offset)ATTACH_TABLE_SIZE(h->attachEntries.cnt);
	hotCtx g = h->g;
	h->attachTable.GlyphCount = (unsigned short)h->attachEntries.cnt;
	qsort(h->attachEntries.array, h->attachEntries.cnt, sizeof(AttachEntry), cmpAttachEntries);
	otlCoverageBegin(g, otl);
	for (i = 0; i < h->attachEntries.cnt; i++) {
		AttachEntry *attachEntry = &h->attachEntries.array[i];
		attachEntry->offset = size;
		attachEntry->PointCount = (unsigned short)attachEntry->contourIndices.cnt;
		size += ATTACH_ENTRY_SIZE(attachEntry->PointCount);
		otlCoverageAddGlyph(g, otl,  attachEntry->gid);
		qsort(attachEntry->contourIndices.array, attachEntry->contourIndices.cnt, sizeof(GID), cmpContourIndicies);
	}
	otlCoverageEnd(g, otl);
	h->attachTable.Coverage = size;
	size  += (Offset)otlGetCoverageSize(otl);
	return size;
}

static int cmpLigGlyphEntries(const void *first, const void *second) {
	GID a = ((LigGlyphEntry *)first)->gid;
	GID b = ((LigGlyphEntry *)second)->gid;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	return 0;
}

static int cmpCaretTables(const void *first, const void *second) {
	unsigned short format1 = ((CaretTable *)first)->format;
	unsigned short format2 = ((CaretTable *)second)->format;
    /* Sort device table entries format == 3) before By Pos or By Index entries (formats 1 and 2).
    Not allowed to have both by Pos and by Index entries */
	if (format1 < format2) {
		return 1;
	}
	else if (format1 > format2) {
		return -1;
	}

	/* both are the same format */
	if (format1 == 1) {
		short a = (short)((CaretTable *)first)->CaretValue;
		short b = (short)((CaretTable *)second)->CaretValue;
		if (a < b) {
			return -1;
		}
		else if (a > b) {
			return 1;
		}
		return 0;
	}
	else {
		unsigned short a = ((CaretTable *)first)->CaretValue;
		unsigned short b = ((CaretTable *)second)->CaretValue;
		if (a < b) {
			return -1;
		}
		else if (a > b) {
			return 1;
		}
		return 0;
	}
}

static Offset createLigCaretTableDef(GDEFCtx h) {
	/* Classes start numbering from 1 */

	int i;
	otlTbl otl = h->ligCaretTable.otl;
	Offset size = (Offset)LIG_CARET_TABLE_SIZE(h->ligCaretEntries.cnt);
	hotCtx g = h->g;
	h->ligCaretTable.LigGlyphCount = (unsigned short)h->ligCaretEntries.cnt;
	qsort(h->ligCaretEntries.array, h->ligCaretEntries.cnt, sizeof(LigGlyphEntry), cmpLigGlyphEntries);
	otlCoverageBegin(g, otl);
	for (i = 0; i < h->ligCaretEntries.cnt; i++) {
		int j;
		unsigned int caretOffset;
		unsigned int numCarets;
		LigGlyphEntry *lge = &h->ligCaretEntries.array[i];
		lge->offset = size;
		numCarets = lge->caretTables.cnt;
		lge->CaretCount = numCarets;
		caretOffset = LIG_GLYPH_ENTRY_SIZE(numCarets);
		size += LIG_GLYPH_ENTRY_SIZE(numCarets);
		/* we will write the caret tables right after each lge entry */
		qsort(lge->caretTables.array, lge->caretTables.cnt, sizeof(CaretTable), cmpCaretTables);
		for (j = 0; j < lge->caretTables.cnt; j++) {
			lge->caretTables.array[j].offset = caretOffset;
			caretOffset += CARET_TABLE_SIZE;
			size += CARET_TABLE_SIZE;
		}
		otlCoverageAddGlyph(g, otl,  lge->gid);
	}
	otlCoverageEnd(g, otl);
	h->ligCaretTable.Coverage = size;
	size  += (Offset)otlGetCoverageSize(otl);
	return size;
}

static void validateGlyphClasses(GDEFCtx h, GNode **classList, long numClasses) {
	int i, j;
	int hadConflictingClassDef = 0;
	hotCtx g = h->g;

	/* Check and warn if the same glyph has ended up in more than one class */
	for (i = 0; i < numClasses; i++) {
		GNode *p1;
		GNode *headNode1 = classList[i];
		char *className1;
		if (headNode1 == NULL) {
			continue;
		}

		className1 = headNode1->markClassName;
		if (className1 == NULL) {
			className1 = "mark attachment class 1";
		}
		for (j = i + 1; j < numClasses; j++) {
			GNode *p2;
			GNode *headNode2 = classList[j];
			char *className2;
			if (headNode2 == NULL) {
				continue;
			}
			className2 = headNode2->markClassName;
			if (className2 == NULL) {
				className2 = "mark attachment class 2";
			}
			for (p1 = headNode1; p1 != NULL; ) {
				GNode *prev = NULL;
				for (p2 = headNode2; p2 != NULL; ) {
					if (p1->gid == p2->gid) {
						hadConflictingClassDef = 1;
						featGlyphDump(g, p1->gid, 0, 0);
						hotMsg(g, hotWARNING, "GDEF MarkAttachment. Glyph '%s' gid '%d'. previous glyph class '%s' conflicts with new class '%s'.", g->note.array, p1->gid, className1,  className2);
					}
					prev = p2;
					p2 = p2->nextCl;
				}
				p1 = p1->nextCl;
			}
		}
	}
	if (hadConflictingClassDef) {
		hotMsg(g, hotWARNING, "GDEF MarkAttachment Classes. There are conflicting MarkAttachment assignments.");
	}
}

static LOffset createMarkAttachClassDef(GDEFCtx h) {
	/* Classes start numbering from 1 */

	otlTbl otl = h->markAttachClassTable;
	int i;

	//Copy Sort the glyph classes.
	for (i = 0; i < h->markAttachClasses.cnt; i++) {
		GNode *srcNode = h->markAttachClasses.array[i];
		GNode *headNode;
		if (srcNode == NULL) {
			continue;
		}

		/* The mark attach glyph classes are all named classes. Named class glyphs get recycled when hashFree() is called,
		   so we need to make a copy of these classes here, and recycle the copy after use. This is because we delete
		   glyphs from within the class lists to eliminate duplicates. If we operate on a named class list, then any deleted duplicated glyphs
		   gets deleted again when hashFree() is called. Also, the hash element head points to the original first glyph, which may be sorted further
		   down the list. */
		featGlyphClassCopy(h->g, &headNode, srcNode);
		featGlyphClassSort(h->g, &headNode, 1, 1);
		h->markAttachClasses.array[i] = headNode;
	}

	validateGlyphClasses(h, (GNode **)h->markAttachClasses.array, h->markAttachClasses.cnt);
	// Add the to the OTL table.
	otlClassBegin(h->g, otl);
	for (i = 0; i < h->markAttachClasses.cnt; i++) {
		GNode *p;
		GNode *srcNode = h->markAttachClasses.array[i];
		if (srcNode == NULL) {
			continue;
		}
		for (p = srcNode; p != NULL; p = p->nextCl) {
			otlClassAddMapping(h->g, otl, p->gid, i + 1);
		}
		featRecycleNodes(h->g, srcNode); /* Don't need this class any more*/
		h->markAttachClasses.array[i] = NULL;
	}
	otlClassEnd(h->g, otl);
	return otlGetClassSize(otl);
}

static LOffset createMarkSetClassDef(GDEFCtx h, hotCtx g) {
	int i;
	Offset size = (Offset)MARK_SET_TABLE_SIZE(h->markSetClasses.cnt);
	h->markSetClassTable.MarkSetTableFormat = 1;
	h->markSetClassTable.MarkSetCount = (unsigned short)h->markSetClasses.cnt;
	dnaINIT(g->dnaCtx, h->markSetClassTable.markSetEntries, h->markSetClasses.cnt, 10);
	for (i = 0; i < h->markSetClasses.cnt; i++) {
		GNode *p;
		otlTbl otl;
		MarkSetEntry *markSetEntry;
		GNode *srcNode = h->markSetClasses.array[i];
		GNode *headNode;
		markSetEntry = dnaNEXT(h->markSetClassTable.markSetEntries);
		otl = otlTableNew(g);
		markSetEntry->otl = otl;

		featGlyphClassCopy(g, &headNode, srcNode);
		featGlyphClassSort(g, &headNode, 1, 1);
		otlCoverageBegin(g, otl);

		for (p = headNode; p != NULL; p = p->nextCl) {
			otlCoverageAddGlyph(g, otl,  p->gid);
		}
		otlCoverageEnd(g, otl);

		markSetEntry->MarkSetCoverage = size;
		size  += (Offset)otlGetCoverageSize(otl);
		featRecycleNodes(g, headNode); /* Don't need this class any more*/
		h->markSetClasses.array[i] = NULL;
	}

	return size;
}

static void writeAttachTable(hotCtx g, GDEFCtx h) {
	int i;
	OUT2(h->attachTable.Coverage);
	OUT2(h->attachTable.GlyphCount);
	for (i = 0; i < h->attachEntries.cnt; i++) {
		AttachEntry *apt = &h->attachEntries.array[i];
		OUT2(apt->offset);
	}
	for (i = 0; i < h->attachEntries.cnt; i++) {
		unsigned int j;
		AttachEntry *apt = &h->attachEntries.array[i];
		OUT2(apt->PointCount);
		for (j = 0; j < apt->PointCount; j++) {
			OUT2(apt->contourIndices.array[j]);
		}
	}
	otlCoverageWrite(g, h->attachTable.otl);
}

static void writeLigCaretTable(hotCtx g, GDEFCtx h) {
	int i;
	OUT2(h->ligCaretTable.Coverage);
	OUT2(h->ligCaretTable.LigGlyphCount);
	for (i = 0; i < h->ligCaretEntries.cnt; i++) {
		LigGlyphEntry *lge = &h->ligCaretEntries.array[i];
		OUT2(lge->offset);
	}
	for (i = 0; i < h->ligCaretEntries.cnt; i++) {
		unsigned int j;
		LigGlyphEntry *lge = &h->ligCaretEntries.array[i];
		OUT2(lge->CaretCount);
		/* write offsets */
		for (j = 0; j < lge->CaretCount; j++) {
			OUT2(lge->caretTables.array[j].offset);
		}
		/* then write caret tables for this lge */
		for (j = 0; j < lge->CaretCount; j++) {
			CaretTable *ct = &lge->caretTables.array[j];
			OUT2(ct->format);
			OUT2(ct->CaretValue);
		}
	}
	otlCoverageWrite(g, h->ligCaretTable.otl);
}

static void writeMarkSetClassTable(hotCtx g, GDEFCtx h) {
	int i;
	OUT2(h->markSetClassTable.MarkSetTableFormat);
	OUT2(h->markSetClassTable.MarkSetCount);

	/* first write the offsets to the coverage tables */
	for (i = 0; i < h->markSetClassTable.markSetEntries.cnt; i++) {
		MarkSetEntry *markSetEntry = dnaINDEX(h->markSetClassTable.markSetEntries, i);
		OUT4(markSetEntry->MarkSetCoverage);
	}

	/* Now write the coverage tables */
	for (i = 0; i < h->markSetClassTable.markSetEntries.cnt; i++) {
		MarkSetEntry *markSetEntry = dnaINDEX(h->markSetClassTable.markSetEntries, i);
		otlCoverageWrite(g, markSetEntry->otl);
	}
}