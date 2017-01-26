/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	sfnt table.
 */

#include <stdlib.h>

#include "BASE.h"
#include "CFF_.h"
#include "GDEF.h"
#include "GPOS.h"
#include "GSUB.h"
#include "MMFX.h"
#include "MMSD.h"
#include "OS_2.h"
#include "VORG.h"
#include "anon.h"
#include "cmap.h"
#include "fvar.h"
#include "head.h"
#include "hhea.h"
#include "hmtx.h"
#include "maxp.h"
#include "name.h"
#include "post.h"
#include "sfnt.h"
#include "vhea.h"
#include "vmtx.h"

typedef struct {
	Tag tag;                    /* Table tag */
	void (*new)(hotCtx g);      /* New table */
	int (*fill)(hotCtx g);      /* Fill table */
	void (*write)(hotCtx g);    /* Writes table */
	void (*reuse)(hotCtx g);    /* Prepare table for reuse */
	void (*free)(hotCtx g);     /* Free table */
	char fillOrder;             /* Table filling order (lowest first) */
	char writeOrder;            /* Table writing order (lowest first) */
	short flags;                /* Status flags */
#define FUNC_WRITE  (1 << 0)      /* Table selected for writing */
} Funcs;

/* Function table. Some sfnt tables require that other tables are filled before
   them, e.g. name, and are therefore ordered using the fillOrder field.
   Tables inside the sfnt are ordered using the writeOrder field so as to
   optimize access during font loading. Order fields with the same values are
   ordered by tag. */
static Funcs funcs[] = {
	{ head_, headNew, headFill, headWrite, headReuse, headFree, 1,  1, 0 },
	{ hhea_, hheaNew, hheaFill, hheaWrite, hheaReuse, hheaFree, 2,  2, 0 },
	{ maxp_, maxpNew, maxpFill, maxpWrite, maxpReuse, maxpFree, 1,  3, 0 },
	{ OS_2_, OS_2New, OS_2Fill, OS_2Write, OS_2Reuse, OS_2Free, 1,  4, 0 },
	{ name_, nameNew, nameFill, nameWrite, nameReuse, nameFree, 3,  5, 0 },
	{ cmap_, cmapNew, cmapFill, cmapWrite, cmapReuse, cmapFree, 1,  6, 0 },
	{ post_, postNew, postFill, postWrite, postReuse, postFree, 1,  7, 0 },
	{ fvar_, fvarNew, fvarFill, fvarWrite, fvarReuse, fvarFree, 1,  8, 0 },
	{ MMSD_, MMSDNew, MMSDFill, MMSDWrite, MMSDReuse, MMSDFree, 1,  9, 0 },
	{ MMFX_, MMFXNew, MMFXFill, MMFXWrite, MMFXReuse, MMFXFree, 1, 10, 0 },
	{ CFF__, CFF_New, CFF_Fill, CFF_Write, CFF_Reuse, CFF_Free, 1, 11, 0 },
	{ hmtx_, hmtxNew, hmtxFill, hmtxWrite, hmtxReuse, hmtxFree, 1, 12, 0 },
	{ vhea_, vheaNew, vheaFill, vheaWrite, vheaReuse, vheaFree, 2, 13, 0 },
	{ vmtx_, vmtxNew, vmtxFill, vmtxWrite, vmtxReuse, vmtxFree, 1, 14, 0 },
	{ GDEF_, GDEFNew, GDEFFill, GDEFWrite, GDEFReuse, GDEFFree, 1, 15, 0 },
	{ GSUB_, GSUBNew, GSUBFill, GSUBWrite, GSUBReuse, GSUBFree, 1, 16, 0 },
	{ GPOS_, GPOSNew, GPOSFill, GPOSWrite, GPOSReuse, GPOSFree, 1, 17, 0 },
	{ BASE_, BASENew, BASEFill, BASEWrite, BASEReuse, BASEFree, 1, 18, 0 },
	{ VORG_, VORGNew, VORGFill, VORGWrite, VORGReuse, VORGFree, 1, 19, 0 },
};
#define SFNT_TABLE_CNT  ARRAY_LEN(funcs)

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	int32_t tag;
	uint32_t checksum;
	uint32_t offset;
	uint32_t length;
} Entry;
#define ENTRY_SIZE (uint32 * 4)

typedef struct {
	Fixed version;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
	dnaDCL(Entry, directory);   /* [numTables] */
} sfntTbl;
#define DIR_HDR_SIZE (int32 + uint16 * 4)

struct sfntCtx_ {
	dnaDCL(Funcs, funcs);
	sfntTbl tbl;                /* Table data */
	char *next;                 /* Next byte available in input buffer */
	long left;                  /* Number of bytes available in input buffer */
	int anonOrder;              /* Anonymous client table fill/write order */
	hotCtx g;                   /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void sfntNew(hotCtx g) {
	sfntCtx h = MEM_NEW(g, sizeof(struct sfntCtx_));
	unsigned int i;

	/* Link contexts */
	h->g = g;
	g->ctx.sfnt = h;

	/* Initialize tables */
	dnaINIT(g->dnaCtx, h->tbl.directory, SFNT_TABLE_CNT, 5);

	dnaINIT(g->dnaCtx, h->funcs, SFNT_TABLE_CNT + 2, 5);
	dnaSET_CNT(h->funcs, SFNT_TABLE_CNT);
	COPY(h->funcs.array, funcs, SFNT_TABLE_CNT);
	for (i = 0; i < SFNT_TABLE_CNT; i++) {
		h->funcs.array[i].new(g);
	}

	anonNew(g);

	/* Force anonymous client tables to fill and write last */
	h->anonOrder = SFNT_TABLE_CNT + 1;
}

/* Compare fillOrder fields */
static int CDECL cmpFillOrders(const void *first, const void *second) {
	const Funcs *a = first;
	const Funcs *b = second;
	if (a->fillOrder < b->fillOrder) {
		return -1;
	}
	else if (a->fillOrder > b->fillOrder) {
		return 1;
	}
	else if (a->tag < b->tag) {
		return -1;
	}
	else if (a->tag > b->tag) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Fill tables */
void sfntFill(hotCtx g) {
	sfntCtx h = g->ctx.sfnt;
	int i;

	/* Sort into fill order */
	qsort(h->funcs.array, h->funcs.cnt, sizeof(Funcs), cmpFillOrders);

	h->tbl.version = TAG('O', 'T', 'T', 'O');
	h->tbl.numTables = 0;
	for (i = 0; i < h->funcs.cnt; i++) {
		Funcs *funcs = &h->funcs.array[i];
		if (funcs->fill(g)) {
			funcs->flags |= FUNC_WRITE;
			h->tbl.numTables++;
		}
	}

	hotCalcSearchParams(ENTRY_SIZE, h->tbl.numTables, &h->tbl.searchRange,
	                    &h->tbl.entrySelector, &h->tbl.rangeShift);
}

/* Compare writeOrder fields */
static int CDECL cmpWriteOrders(const void *first, const void *second) {
	const Funcs *a = first;
	const Funcs *b = second;
	if (a->writeOrder < b->writeOrder) {
		return -1;
	}
	else if (a->writeOrder > b->writeOrder) {
		return 1;
	}
	else if (a->tag < b->tag) {
		return -1;
	}
	else if (a->tag > b->tag) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Write all the tables */
static void writeTables(hotCtx g, unsigned long start) {
	sfntCtx h = g->ctx.sfnt;
	int i;
	unsigned long before;

	/* Sort into write order */
	qsort(h->funcs.array, h->funcs.cnt, sizeof(Funcs), cmpWriteOrders);

	SEEK(start + DIR_HDR_SIZE + ENTRY_SIZE * h->tbl.numTables);

	/* Write tables */
	h->tbl.directory.cnt = 0;
	before = TELL();
	for (i = 0; i < h->funcs.cnt; i++) {
		Funcs *funcs = &h->funcs.array[i];
		if (funcs->flags & FUNC_WRITE) {
			int pad;
			unsigned long after;
			Entry *entry = dnaNEXT(h->tbl.directory);

			funcs->write(g);
			after = TELL();

			/* Pad to 4-byte boundary */
			pad = (~(after & 0x3) + 1) & 0x3;
			if (pad != 0) {
				OUTN(pad, "\0\0");
			}

			entry->tag = funcs->tag;
			entry->offset = before - start;
			entry->length = after - before;

			before = after + pad;
		}
	}
}

/* Compare tag fields */
static int CDECL cmpTags(const void *first, const void *second) {
	const Entry *a = first;
	const Entry *b = second;
	if (a->tag < b->tag) {
		return -1;
	}
	else if (a->tag > b->tag) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Write directory */
static void writeDirectory(sfntCtx h, unsigned long start) {
	int i;

	SEEK(start);

	OUT4(h->tbl.version);
	OUT2(h->tbl.numTables);
	OUT2(h->tbl.searchRange);
	OUT2(h->tbl.entrySelector);
	OUT2(h->tbl.rangeShift);

	/* Sort directory into tag order */
	qsort(h->tbl.directory.array, h->tbl.numTables, sizeof(Entry), cmpTags);

	/* Write directory */
	for (i = 0; i < h->tbl.numTables; i++) {
		Entry *entry = &h->tbl.directory.array[i];

		OUT4(entry->tag);
		OUT4(entry->checksum);
		OUT4(entry->offset);
		OUT4(entry->length);
	}
}

/* Replenish input buffer using refill function */
static char fillbuf(sfntCtx h) {
	hotCtx g = h->g;
	h->next = g->cb.otfRefill(g->cb.ctx, &h->left);
	if (h->left-- == 0) {
		hotMsg(g, hotFATAL, "premature end of input");
	}
	return *h->next++;
}

/* Read byte from OTF data */
#define read1(h)    ((unsigned char)((h->left-- == 0) ? fillbuf(h) : *h->next++))

/* Input OTF data as 4-byte number in big-endian order */
static unsigned long read4(sfntCtx h) {
	unsigned long result;
	if (h->left > 3) {
		/* All bytes bytes in current block */
		result  = (unsigned long)((unsigned char)*h->next++) << 24;
		result |= (unsigned long)((unsigned char)*h->next++) << 16;
		result |= (unsigned long)((unsigned char)*h->next++) << 8;
		result |= (unsigned long)((unsigned char)*h->next++);
		h->left -= 4;
	}
	else {
		/* Bytes split between blocks */
		result  = (unsigned long)read1(h) << 24;
		result |= (unsigned long)read1(h) << 16;
		result |= (unsigned long)read1(h) << 8;
		result |= (unsigned long)read1(h);
	}
	return result;
}

void sfntWrite(hotCtx g) {
	sfntCtx h = g->ctx.sfnt;
	int i;
	long j;
	unsigned offset = 0;    /* Suppress optimizer warning */
	unsigned long sum;
	unsigned long start = TELL();   /* xxx this may always be 0 */
	unsigned long dirSize = DIR_HDR_SIZE + ENTRY_SIZE * h->tbl.numTables;

	writeTables(g, start);

	/* Seek to first table */
	SEEK(start + dirSize);
	h->left = 0;

	/* Compute table checksums */
	for (i = 0; i < h->tbl.numTables; i++) {
		Entry *entry = &h->tbl.directory.array[i];
		int nLongs = (entry->length + 3) / 4;

		if (entry->tag == head_) {
			offset = entry->offset + HEAD_ADJUST_OFFSET;
		}

		sum = 0;
		for (j = 0; j < nLongs; j++) {
			sum += read4(h);
		}
		entry->checksum = sum;
	}

	writeDirectory(h, start);

	/* Compute header checksum */
	SEEK(start);
	h->left = 0;
	sum = 0;
	for (j = 0; j < (long)dirSize; j += 4) {
		sum += read4(h);
	}

	/* Add in table checksums */
	for (i = 0; i < h->tbl.numTables; i++) {
		sum += h->tbl.directory.array[i].checksum;
	}

	/* Write head table checksum adjustment */
	SEEK(start + offset);
	OUT4(0xb1b0afba - sum);
}

void sfntReuse(hotCtx g) {
	sfntCtx h = g->ctx.sfnt;
	unsigned int i;

	for (i = 0; i < SFNT_TABLE_CNT; i++) {
		Funcs *funcs = &h->funcs.array[i];
		if (funcs->flags & FUNC_WRITE) {
			funcs->reuse(g);
		}
		funcs->flags = 0;
	}

	/* Trim anonymous client table entries from the end of list */
	h->funcs.cnt = SFNT_TABLE_CNT;
	h->anonOrder = SFNT_TABLE_CNT + 1;
	anonReuse(g);
}

void sfntFree(hotCtx g) {
	sfntCtx h = g->ctx.sfnt;

	if (h != NULL) {
		unsigned int i;
		for (i = 0; i < SFNT_TABLE_CNT; i++) {
			h->funcs.array[i].free(g);
		}
		dnaFREE(h->funcs);
		dnaFREE(h->tbl.directory);
	}
	MEM_FREE(g, g->ctx.sfnt);

	anonFree(g);
}

/* Add anonymous client table */
void sfntAddAnonTable(hotCtx g, unsigned long tag, hotAnonRefill refill) {
	sfntCtx h = g->ctx.sfnt;
	int i;
	Funcs *funcs;

	/* Check that table hasn't already been added */
	for (i = 0; i < h->funcs.cnt; i++) {
		if (h->funcs.array[i].tag == tag) {
			hotMsg(g, hotFATAL, "attempt to add duplicate table");
		}
	}

	/* Initialize function table entry */
	funcs               = dnaNEXT(h->funcs);
	funcs->tag          = tag;
	funcs->new          = anonNew;
	funcs->fill         = anonFill;
	funcs->write        = anonWrite;
	funcs->reuse        = anonReuse;
	funcs->free         = anonFree;
	funcs->fillOrder    = h->anonOrder;
	funcs->writeOrder   = h->anonOrder++;
	funcs->flags        = 0;

	anonAddTable(g, tag, refill);
}