/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Compact Font Format Generation Library.
 */

#include "txops.h"
#include "dictops.h"
#include "supportexcept.h"

#include "cffwrite_charset.h"
#include "cffwrite_encoding.h"
#include "cffwrite_fdselect.h"
#include "cffwrite_sindex.h"
#include "cffwrite_dict.h"
#include "cffwrite_varstore.h"
#include "cffwrite_t2cstr.h"
#include "cffwrite_subr.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define CFF_HDR_SIZE 4          /* CFF v1.0 header size */
#define CFF2_HDR_SIZE 5          /* CFF v2.0 header size */

/* Width optimization seeks to find two values called the default and nominal
   width. Glyphs with the default width omit the width altogether and all other
   widths are reconstituted by subtracting the nominal width from the encoded
   width found in the charstring. The choice of nominal width and default width
   interact. For example, choosing a nominal width near a default width will
   make the potential default width savings smaller. By carefully choosing a
   nominal width many widths will shrink by one byte because smaller numbers
   are encoded in fewer bytes. */

typedef struct                  /* Width frequency record */
{
	float width;
	long count;
} WidthFreq;

typedef struct                  /* FDArray element */
{
	abfFontDict dict;           /* Font dict data (copied from client) */
	abfPrivateDict Private;     /* Private dict data (copied from client) */
	struct                      /* CFF DICT data */
	{
		DICT dict;
		DICT Private;
	} cff;
	INDEX Subrs;
	subr_CSData subrData;
	struct                      /* CFF data sizes */
	{
		long Private;
		long Subrs;
	} size;
	struct                      /* CFF data offsets */
	{
		Offset Private;
		Offset Subrs;
	} offset;
	struct                      /* Width analysis */
	{
		dnaDCL(WidthFreq, freqs);
		float dflt;
		float nominal;
	} width;
} FDInfo;

typedef struct Glyph_                   /* Glyph data */
{
	abfGlyphInfo *info;
	float hAdv;
	struct                      /* Charstring */
	{
		long length;
		long offset;
	} cstr;
	unsigned char iFD;      /* Modifiable and persistent copy of info->iFD */
} Glyph;

typedef struct                  /* Glyph data */
{
	long glyphsIndex; /* index of this glyph in Font.glyphs.array */
	abfGlyphInfo info;          /* if the entry exists, it has been seen. */
} SeenGlyph;

typedef struct                  /* sorted FontDict data */
{
	long fdIndex;           /* index of this dict in h->_new.FDArray  */
} SeenDict;

typedef struct                  /* Per-font data */
{
	long flags;                 /* Font-specific flags */
#define FONT_CID    (1 << 0)      /* CID font */
#define FONT_SYN    (1 << 1)      /* Synthetic font */
	abfTopDict top;             /* Top dict data (copied from client) */
	struct                      /* CFF DICT data */
	{
        DICT top;
        DICT varStore;
		DICT synthetic;         /* Synthetic Top DICT */
	} cff;
	dnaDCL(FDInfo, FDArray);    /* FD array */
	dnaDCL(Glyph, glyphs);      /* Per-glyph data */
	dnaDCL(SeenGlyph, seenGlyphs);  /* Per-glyph data */
	dnaDCL(SeenDict, seenDicts);    /* Per-fontdict data */

	INDEX CharStrings;          /* CharStrings INDEX data */
	struct                      /* Object indexes */
	{
		long charset;
		long Encoding;
		long FDSelect;
	} iObject;
	struct                      /* CFF data sizes */
	{
		long top;
        long CharStrings;
        long VarStore;
		long FDArray;           /* CID and CFF2 only */
		long Private;
		long Subrs;
		long widths;
	} size;
	struct                      /* CFF data offsets */
	{
		Offset charset;
		Offset Encoding;
        Offset VarStore;
        Offset FDSelect;
		Offset CharStrings;
		Offset FDArray;         /* CID and CFF2 only */
		Offset Private;
		Offset Subrs;
	} offset;
	cfwMapCallback *map;        /* Client-supplied map callbacks */
} cff_Font;

/* ----------------------------- Module Context ---------------------------- */

struct controlCtx_ {
	long flags;                 /* FontSet specific flags */
#define FONTSET_SUBRIZE (1 << 0)  /* Subroutinize */
#define FONTSET_EMBED   (1 << 1)  /* Peform embedding space optimization */
#define SEEN_NAME_KEYED_GLYPH (1 << 2)    /* Seen name-keyed glyph */
#define SEEN_CID_KEYED_GLYPH  (1 << 3)    /* Seen cid-keyed glyph */
#define FONTSET_CFF2  (1 << 4)  /* We are writing a CFF2 font. */
	dnaDCL(cff_Font, FontSet);      /* FontSet */
	cff_Font *_new;                  /* New font */
	INDEX name;                 /* Name INDEX */
	struct                      /* Global data sizes */
	{
		long header;
		long name;
        long top;
        long varStore;
		long string;
		long gsubr;
		long charset;
		long Encoding;
		long FDSelect;          /* CID only */
	}
	size;
	struct                      /* Global data offsets */
	{
		Offset name;
        Offset top;
        Offset varStore;
		Offset string;
		Offset gsubr;
		Offset charset;
		Offset Encoding;
		Offset FDSelect;        /* CID only */
		Offset end;             /* 1 byte beyond end of CFF data */
	}
	offset;
	struct                      /* Global data offsets */
	{
		char *oldGlyph; /* buffers used in merging glyphs */
		char *newGlyph;
	}
	mergeBuffers;
	int mergedDicts;            /* flag set if client has merged any dicts during glyph processing */
	cfwCtx g;                   /* Package context */
};

static void initSubrData(subr_CSData *subrData);

/* Initialize FDInfo. */
static void initFDInfo(void *ctx, long cnt, FDInfo *fd) {
	cfwCtx g = (cfwCtx)ctx;
	while (cnt--) {
		dnaINIT(g->ctx.dnaSafe, fd->cff.dict, 25, 50);
		dnaINIT(g->ctx.dnaSafe, fd->cff.Private, 100, 50);
		dnaINIT(g->ctx.dnaSafe, fd->width.freqs, 50, 50);
		initSubrData(&fd->subrData);
		fd++;
	}
}

/* Initialize font. */
static void initFont(void *ctx, long cnt, cff_Font *font) {
	cfwCtx g = (cfwCtx)ctx;
	while (cnt--) {
		memset(font, 0, sizeof(*font));
		dnaINIT(g->ctx.dnaSafe, font->cff.top, 50, 50);
		dnaINIT(g->ctx.dnaSafe, font->cff.varStore, 100, 100);
		dnaINIT(g->ctx.dnaSafe, font->cff.synthetic, 25, 50);
		dnaINIT(g->ctx.dnaSafe, font->FDArray, 1, 14);
		font->FDArray.func = initFDInfo;
		dnaINIT(g->ctx.dnaFail, font->glyphs, 256, 750);
		dnaINIT(g->ctx.dnaFail, font->seenGlyphs, 256, 256);
		dnaINIT(g->ctx.dnaFail, font->seenDicts, 2, 2);
		font++;
	}
}

/* Initialize module. */
static void cfwControlNew(cfwCtx g) {
	controlCtx h = (controlCtx)cfwMemNew(g, sizeof(struct controlCtx_));

	dnaINIT(g->ctx.dnaFail, h->FontSet, 1, 150);
	h->FontSet.func = initFont;
	h->mergeBuffers.newGlyph = NULL;
	h->mergeBuffers.oldGlyph = NULL;
	/* Link contexts */
	h->g = g;
	g->ctx.control = h;
}

static void freeSubrData(cfwCtx g, subr_CSData *subrData);

/* Free resources. */
static void cfwControlFree(cfwCtx g) {
	controlCtx h = g->ctx.control;
	int i;

	if (h == NULL) {
		return;
	}

	for (i = 0; i < h->FontSet.size; i++) {
		int j;
		cff_Font *font = &h->FontSet.array[i];
		for (j = 0; j < font->FDArray.size; j++) {
			FDInfo *fd = &font->FDArray.array[j];
			dnaFREE(fd->cff.dict);
			dnaFREE(fd->cff.Private);
			dnaFREE(fd->width.freqs);
			freeSubrData(g, &fd->subrData);
		}
        dnaFREE(font->cff.top);
        dnaFREE(font->cff.varStore);
		dnaFREE(font->cff.synthetic);
		dnaFREE(font->FDArray);
		dnaFREE(font->glyphs);
		dnaFREE(font->seenGlyphs);
		dnaFREE(font->seenDicts);
	}
	dnaFREE(h->FontSet);
	if (h->mergeBuffers.newGlyph != NULL) {
		g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.newGlyph, 0);
	}
	if (h->mergeBuffers.oldGlyph != NULL) {
		g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.oldGlyph, 0);
	}

	cfwMemFree(g, h);
	g->ctx.control = NULL;
}

/* ----------------------------- Glyph Merge Handling ---------------------------- */

static void fillbuf(cfwCtx g, long offset);



/* Copy count bytes from tmp stream. */
static void tmp2bufCopy(cfwCtx g, long length, long offset, char *ptr) {
	long left;
	long delta = offset - g->tmp.offset;

	if (delta >= 0 && (size_t)delta < g->tmp.length) {
		/* Offset within current buffer; reposition next byte */
		g->tmp.next = g->tmp.buf + delta;
	}
	else {
		/* Offset outside current buffer; seek to offset and fill buffer */
		if (g->cb.stm.seek(&g->cb.stm, g->stm.tmp, offset)) {
			cfwFatal(g, cfwErrTmpStream, NULL);
		}
		fillbuf(g, offset);
	}

	/* Compute bytes left in buffer */
	/* 64-bit warning fixed by cast here */
	left = (long)(g->tmp.end - g->tmp.next);
	while (left < length) {
		/* Copy buffer */
		memcpy(ptr, g->tmp.next, left);
		ptr += left;
		length -= left;

		/* Refill buffer */
		/* 64-bit warning fixed by cast here */
		fillbuf(g, (long)(g->tmp.offset + g->tmp.length));
		left = (long)g->tmp.length;
	}

	memcpy(ptr, g->tmp.next, length);
	g->tmp.next += length;
}

/* Match glyph name. */
static int CTL_CDECL matchSeenName(const void *key, const void *value, void *ctx) {
	char *compStr = ((SeenGlyph *)value)->info.gname.ptr;

	return strcmp((char *)key,
	              compStr);
}

/* Match glyph CID. */
static int CTL_CDECL matchSeenCID(const void *key, const void *value, void *ctx) {
	long match;
	match = *(unsigned short *)key -  ((SeenGlyph *)value)->info.cid;

	if (match == 0) {
		return match;
	}
	else {
		return (match > 0) ? 1 : -1;
	}
}

/* Match font FD dict FontName. */
static int CTL_CDECL matchFontName(const void *srcName, const void *value, void *ctx) {
	long fdIndex = ((SeenDict *)value)->fdIndex;
	controlCtx h  = (controlCtx)ctx;
	int result;
	char *destName = h->_new->FDArray.array[fdIndex].dict.FontName.ptr;

	/*  new dict with NULL FontName is always sorted last */
	/* We also treat NULL FontName as a unique individual name. */
	if ((srcName == NULL) || (destName == NULL)) {
		if (destName == srcName) {
			result = 0;
		}
		else {
			if (srcName == NULL) {
				result = 1;
			}
			else {
				result = -1;
			}
		}
	}
	else {
		result = strcmp((char *)srcName, destName);
	}

	return result;
}

/*
   Test if glyph has already been added to the font.
   The function sets the result code:
    0						glyph not yet seen
    cfwErrGlyphPresent,      "identical charstring is already present"
    cfwErrGlyphDiffers,       "different charstring of same name is already present"
   and returns the index of the glyph in the sorted array of glyph names in the CFF. If not
   yet in the font, the index is one greater than the index of the glyph which sorts before the new name.

 */
long cfwSeenGlyph(cfwCtx g, abfGlyphInfo *info, int *result, long startNew, long endNew) {
	controlCtx h = g->ctx.control;
	long seenIndex  =  0;
	int nameFound;
	int noMatch = 0;

	*result = 0;

	if (info->flags & ABF_GLYPH_CID) {
		nameFound = ctuLookup(&info->cid, h->_new->seenGlyphs.array, h->_new->seenGlyphs.cnt,
		                      sizeof(h->_new->seenGlyphs.array[0]), matchSeenCID, (size_t *)&seenIndex, h);
	}
	else {
		nameFound = ctuLookup(info->gname.ptr, h->_new->seenGlyphs.array, h->_new->seenGlyphs.cnt,
		                      sizeof(h->_new->seenGlyphs.array[0]), matchSeenName, (size_t *)&seenIndex, h);
	}

	if (nameFound) {
		long lenNewStr = endNew - startNew;
		long glyphIndex = h->_new->seenGlyphs.array[seenIndex].glyphsIndex;
		long lenOldStr = h->_new->glyphs.array[glyphIndex].cstr.length;



		if (lenNewStr != lenOldStr) {
			noMatch = 1;
		}
		else {
			h->mergeBuffers.newGlyph = (char *)g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.newGlyph, lenNewStr);
			h->mergeBuffers.oldGlyph = (char *)g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.oldGlyph, lenOldStr);

			/* get  strings */
			/* This is used in the context of the glyphEnd function,
			   where succesie writes are used to the tmp stream.
			   The g->tmp values may therefore be all out of date, and we have to
			   reset the g->tmp values.
			 */
			/* re-init temp buffer */
			g->cb.stm.seek(&g->cb.stm, g->stm.tmp, h->_new->glyphs.array[glyphIndex].cstr.offset);
			g->tmp.offset = h->_new->glyphs.array[glyphIndex].cstr.offset;
			g->tmp.next = g->tmp.buf; /* say that there is no current stream content in the tmp.buf */
			g->tmp.length = 0;

			tmp2bufCopy(g, lenOldStr,  h->_new->glyphs.array[glyphIndex].cstr.offset, h->mergeBuffers.oldGlyph);
			tmp2bufCopy(g, lenNewStr, startNew, h->mergeBuffers.newGlyph);

			/* fix temp file position */
			g->cb.stm.seek(&g->cb.stm, g->stm.tmp, endNew);

			if (g->flags & CFW_SUBRIZE) {
				lenNewStr -= 4; /* Skip the unique terminator for subroutinizer */
			}
			noMatch = strncmp(h->mergeBuffers.oldGlyph, h->mergeBuffers.newGlyph, lenNewStr);
		}

		if (noMatch == 0) {
			*result = cfwErrGlyphPresent;
		}
		else {
			*result = cfwErrGlyphDiffers;
		}
	}

	return seenIndex;
}

static long mergeDict(cfwCtx g, abfFontDict *srcDict) {
	controlCtx h = g->ctx.control;
	long seenIndex  =  -1;
	long newFDIndex;
	int dictFound;

	h->mergedDicts = 1;

	dictFound = ctuLookup(srcDict->FontName.ptr, h->_new->seenDicts.array, h->_new->seenDicts.cnt,
	                      sizeof(SeenDict), matchFontName, (size_t *)&seenIndex, h);


	/* if it is a new dict, then add it to the destination font */
	if (!dictFound) {
		int needHole = seenIndex < h->_new->seenDicts.cnt;
		SeenDict *newSeenDict;
		FDInfo *newFDInfo;
		long nDicts;


		/* grow h->_new.FDArray array, increment FDArray.cnt */
		if (dnaNext(&h->_new->FDArray, sizeof(FDInfo)) == -1) {
			g->err.code = cfwErrNoMemory;
			return ABF_UNSET_INT;
		}
		nDicts =  h->_new->FDArray.cnt;
		newFDInfo = &h->_new->FDArray.array[nDicts - 1];

		/* grow seenDicts array, increment SeenDict.cnt */
		if (dnaNext(&h->_new->seenDicts, sizeof(SeenDict)) == -1) {
			g->err.code = cfwErrNoMemory;
			return ABF_UNSET_INT;
		}

		/* ptr to insertion-point index */
		newSeenDict = &h->_new->seenDicts.array[seenIndex];


		/* Move higher elements (if there are any) up one index to make hole,
		    and fix all the glyph->iFD indices. */
		if (needHole) {
			memmove(newSeenDict + 1, newSeenDict,
			        ((h->_new->seenDicts.cnt - 1) - seenIndex) * sizeof(SeenDict));
		}

		/* fill new FD array entry */
		cfwDictCopyFont(g, &newFDInfo->dict, srcDict);
		cfwDictCopyPrivate(g, &newFDInfo->Private, &srcDict->Private);
		newFDInfo->Subrs.count = 0;
		memset(&newFDInfo->subrData, 0, sizeof(subr_CSData));
		newFDInfo->width.freqs.cnt = 0;

		/* Fill new seenDicts entry */
		newSeenDict->fdIndex = nDicts - 1;

		/* What we return is the index of the new font dict in the h->_new.FDArray.
		   A new dict is always the last dist in the array. */
		newFDIndex =  nDicts - 1;
	}

	newFDIndex = h->_new->seenDicts.array[seenIndex].fdIndex;

	return newFDIndex;
}

int cfwMergeFDArray(cfwCtx g, abfTopDict *top, int *newFDIndex) {
	abfFontDict *srcDict =  &top->FDArray.array[0];
	int i = 0;


	while (i++ < top->FDArray.cnt) {
		if (g->err.code) {
			*newFDIndex = ABF_UNSET_INT;
		}
		else {
			*newFDIndex = mergeDict(g, srcDict);
		}
		newFDIndex++;
		srcDict++;
	}

	return g->err.code;
}

int cfwCompareFDArrays(cfwCtx g, abfTopDict *srcTop) {
	controlCtx h = g->ctx.control;
	int srcCount = srcTop->FDArray.cnt;
	int destCount = h->_new->FDArray.cnt;
	int j = 0;

	if (destCount > 0) {
		while (j < srcCount) {
			long seenIndex = 0;
			int dictFound;
			abfFontDict *srcDict =  &srcTop->FDArray.array[j++];

			dictFound = ctuLookup(srcDict->FontName.ptr, h->_new->seenDicts.array, h->_new->seenDicts.cnt,
			                      sizeof(SeenDict), matchFontName, (size_t *)&seenIndex, h);


			if (dictFound) {
				int iFD = h->_new->seenDicts.array[seenIndex].fdIndex;
				abfFontDict *destDict =  &h->_new->FDArray.array[iFD].dict;

				if (abfCompareFontDicts(destDict, srcDict)) {
					return 1;
				}
			}
		}
	}

	return 0;
}

/* ----------------------------- Glyph Handling ---------------------------- */

/* Add new glyph. */
void cfwAddGlyph(cfwCtx g,
                 abfGlyphInfo *info, float hAdv, long length, long offset, long seen_index) {
	controlCtx h = g->ctx.control;
	Glyph *glyph = NULL;
	SeenGlyph *seenGlyph = NULL;
	long index = 0;

	if (info->flags & ABF_CID_FONT) {
		/* CID-keyed font */
		if (info->cid == 0) {
			/* Force CID 0 to GID 0 */
			glyph = &h->_new->glyphs.array[0];
		}
	}
	else {
		/* Name-keyed font */
		if (strcmp(info->gname.ptr, ".notdef") == 0) {
			/* Force .notdef glyph to GID 0 */
			glyph = &h->_new->glyphs.array[0];
		}

		/* Add glyph name to string index, if we have not already seen it. */
		info->gname.impl = cfwSindexAddString(g, info->gname.ptr);
	}

	/* h->_new->glyphs.array is initialized with one glyph entry reserved for notdef, so when we see the notdef glyph info,
	   we just copy it to that slot. We need to add a new glyph entry for all other glyphs. */
	if (glyph == NULL) {
		/* if it is anything but notdef */
		index = dnaNext(&h->_new->glyphs, sizeof(Glyph));
		if (index == -1) {
			g->err.code = cfwErrNoMemory;
			return;
		}
		glyph = &h->_new->glyphs.array[index];
	}

	/* Copy glyph details */
	glyph->info         = info;
	glyph->hAdv         = hAdv;
	glyph->cstr.length  = length;
	glyph->cstr.offset  = offset;
	glyph->iFD          = info->iFD;    /* Make modifiable/persistent copy */
	                                    /* When merging  CID fonts, this must specify the
	                                       the destination font FD array index, not the source font
	                                       See mergeDict() */




	/* If we will be merging glyphs from more then one font, update the seenGlyphs list. */
	/* We get to here only if the new glyph does NOT have the same name as a glyph which has
	   already been seen. It is therefore added to the list. 'seen_index' gives the index
	   of the new name in the alphabetic list of glyph names. */

	/* For CID glyphs, we also need to check if the FD is new to the dest font,
	   and if so add it, and we need to fix the glyph->iFD value. This is currently
	   an index into the  source font FD array */
	if (g->flags & CFW_CHECK_IF_GLYPHS_DIFFER) {
		int needHole = seen_index < h->_new->seenGlyphs.cnt;

		/* grow array, increment seenGlyphs.cnt */
		if (dnaNext(&h->_new->seenGlyphs, sizeof(SeenGlyph)) == -1) {
			g->err.code = cfwErrNoMemory;
			return;
		}

		/* ptr to insertion-point index */
		seenGlyph = &h->_new->seenGlyphs.array[seen_index];


		/* Move higher elements (if there are any) up one index to make hole. */
		if (needHole) {
			memmove(seenGlyph + 1, seenGlyph,
			        ((h->_new->seenGlyphs.cnt - 1) - seen_index) * sizeof(SeenGlyph));
		}

		seenGlyph->glyphsIndex = index;
		seenGlyph->info = *info; // I can't save a ptr to the info, as the original array of abfGlyphInfo moves when resized.
	}


	if (info->flags & ABF_GLYPH_UNICODE) {
		/* Remove Unicode encoding since it can't be represented */
		abfEncoding *enc = &info->encoding;
		do {
			enc->code = ABF_GLYPH_UNENC;
			enc = enc->next;
		}
		while (enc != NULL);
		info->flags &= ~ABF_GLYPH_UNICODE;
	}

	/* Accumulate total charstring size */
	h->_new->CharStrings.datasize += length;

	if (h->_new->map != NULL && g->flags & CFW_PRESERVE_GLYPH_ORDER) {
		h->_new->map->glyphmap(h->_new->map,
		                      (unsigned short)(glyph - h->_new->glyphs.array),
		                      glyph->info);
	}
}

/* ---------------------------- FontSet Filling ---------------------------- */

/* Fill CharStrings INDEX. */
static long fillCharStringsINDEX(cfwCtx g, cff_Font *font) {
    long size;
    font->CharStrings.count = (unsigned short)font->glyphs.cnt;
    if (!(g->flags & CFW_WRITE_CFF2)) {
        font->CharStrings.datasize += font->size.widths;
    }
    font->CharStrings.offSize = INDEX_OFF_SIZE(font->CharStrings.datasize);
    
    if (g->flags & CFW_WRITE_CFF2) {
        size = INDEX2_SIZE(font->CharStrings.count, font->CharStrings.datasize);
    }
    else {
        size = INDEX_SIZE(font->CharStrings.count, font->CharStrings.datasize);
    }
    return size;
}

/* Calculate initial font sizes. */
static void initFontSizes(controlCtx h, cff_Font *font) {
	int i;
    cfwCtx g = h->g;

    font->size.top = font->cff.top.cnt;

	/* Calculate CharStrings INDEX size */
	font->size.CharStrings = fillCharStringsINDEX(g, font);

    /* Calculate VarStore INDEX size */
    font->size.VarStore = font->cff.varStore.cnt;

    /* Calculate FDArray size */
	if ((font->flags & FONT_CID) || (g->flags & CFW_WRITE_CFF2)) {
		long size = 0;
		for (i = 0; i < font->FDArray.cnt; i++) {
			size += font->FDArray.array[i].cff.dict.cnt;
		}
        if (g->flags & CFW_WRITE_CFF2) {
            font->size.FDArray = INDEX2_SIZE(font->FDArray.cnt, size);
        }
        else {
            font->size.FDArray = INDEX_SIZE(font->FDArray.cnt, size);
        }
    }
	else {
		font->size.FDArray = 0;
	}

	/* Calculate Private DICT and Subr INDEX sizes */
	font->size.Private = 0;
	font->size.Subrs = 0;
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		font->size.Private += fd->size.Private = fd->cff.Private.cnt;
		font->size.Subrs += fd->size.Subrs = cfwSubrSizeLocal(g, &fd->subrData);
	}
}

/* Calculate font offsets. */
static Offset calcFontOffsets(controlCtx h, cff_Font *font, Offset offset) {
	cfwCtx g = h->g;
	int i;

	font->offset.charset =
	    cfwCharsetGetOffset(g, font->iObject.charset, h->offset.charset);
	font->offset.Encoding =
	    cfwEncodingGetOffset(g, font->iObject.Encoding, h->offset.Encoding);
    font->offset.FDSelect =
    cfwFdselectGetOffset(g, font->iObject.FDSelect, h->offset.FDSelect);

    if (font->size.VarStore > 0) {
        font->offset.VarStore = offset;
        offset += font->size.VarStore;
    }
    font->offset.CharStrings = offset;
    font->offset.FDArray = font->offset.CharStrings + font->size.CharStrings;
	font->offset.Private = font->offset.FDArray    + font->size.FDArray;
	font->offset.Subrs  = font->offset.Private    + font->size.Private;
	/* Compute individual Private DICT offsets */
	offset = font->offset.Private;
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		fd->offset.Private = offset;
		offset += fd->size.Private;
	}

	/* Compute individual Subrs INDEX offsets */
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		fd->offset.Subrs = offset;
		offset += fd->size.Subrs;
	}
	return offset;
}

/* Return size of int argument (in bytes) if formatted for DICT. */
static int intsize(long i) {
	if (-107 <= i && i <= 107) {
		return 1;
	}
	else if (-1131 <= i && i <= 1131) {
		return 2;
	}
	else if (-32768 <= i && i <= 32767) {
		return 3;
	}
	else {
		return 5;
	}
}

/* Calculate font sizes. */
static void calcFontSizes(controlCtx h, cff_Font *font) {
	int i;
	long size;
    cfwCtx g = h->g;

	/* Calculate Top DICT size */
    if (g->flags & CFW_WRITE_CFF2) {
        size = font->cff.top.cnt;
        size += intsize(font->offset.CharStrings) + DICT_OP_SIZE(cff_CharStrings);
        
        if (font->offset.VarStore > 0)
            size += (intsize(font->offset.VarStore) + DICT_OP_SIZE(cff_VarStore));
        
        if (font->offset.FDSelect > 0)
            size += (intsize(font->offset.FDSelect) + DICT_OP_SIZE(cff_FDSelect));
        
        size +=  (intsize(font->offset.FDArray) + DICT_OP_SIZE(cff_FDArray));
        font->size.top = size;
        
        /* Calculate FDArray size */
        size = 0;
        for (i = 0; i < font->FDArray.cnt; i++) {
            FDInfo *fd = &font->FDArray.array[i];
            size += (fd->cff.dict.cnt +
                     intsize(fd->size.Private) + intsize(fd->offset.Private) +
                     DICT_OP_SIZE(cff_Private));
        }
        
        font->size.FDArray = INDEX2_SIZE(font->FDArray.cnt, size);
        
        
        
     }
    else {
        size = font->cff.top.cnt;
        switch (font->offset.charset) {
            case cff_ISOAdobeCharset:
                break;
                
            case cff_ExpertCharset:
            case cff_ExpertSubsetCharset:
            default:
                size += intsize(font->offset.charset) + DICT_OP_SIZE(cff_charset);
                break;
        }
        
        switch (font->offset.Encoding) {
            case cff_StandardEncoding:
                break;
                
            case cff_ExpertEncoding:
            default:
                size += intsize(font->offset.Encoding) + DICT_OP_SIZE(cff_Encoding);
                break;
        }
        
        size += intsize(font->offset.CharStrings) + DICT_OP_SIZE(cff_CharStrings);
        
        if (font->flags & FONT_CID) {
            size += (intsize(font->offset.FDSelect) + DICT_OP_SIZE(cff_FDSelect) +
                     intsize(font->offset.FDArray) + DICT_OP_SIZE(cff_FDArray));
        }
        else {
            size += (intsize(font->size.Private) + intsize(font->offset.Private) +
                     DICT_OP_SIZE(cff_Private));
        }
        font->size.top = size;
        
        if (font->flags & FONT_CID) {
            /* Calculate FDArray size */
            size = 0;
            for (i = 0; i < font->FDArray.cnt; i++) {
                FDInfo *fd = &font->FDArray.array[i];
                size += (fd->cff.dict.cnt +
                         intsize(fd->size.Private) + intsize(fd->offset.Private) +
                         DICT_OP_SIZE(cff_Private));
            }
            
            font->size.FDArray = INDEX_SIZE(font->FDArray.cnt, size);
            
            
        }
    }


	/* Calculate total size of Private DICTs */
	font->size.Private = 0;
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		fd->size.Private = fd->cff.Private.cnt;
		if (fd->Subrs.count > 0) {
			/* Subrs offset is relative to start of the Private DICT */
			fd->size.Private +=
			    (intsize(fd->offset.Subrs - fd->offset.Private) +
			     DICT_OP_SIZE(cff_Subrs));
		}
		font->size.Private += fd->size.Private;
	}
}

/* Fill Name INDEX. */
static long fillNameINDEX(controlCtx h) {
	long i;

	h->name.datasize = 0;
	for (i = 0; i < h->FontSet.cnt; i++) {
		cff_Font *font = &h->FontSet.array[i];
		if (font->FDArray.cnt > 0) {
			char *name =
				cfwSindexGetString(h->g, (SRI)((font->flags & FONT_CID) ?
					font->top.cid.CIDFontName.impl :
					font->FDArray.array[0].dict.FontName.impl));
			/* 64-bit warning fixed by cast here */
			h->name.datasize += (long)strlen(name);
		}
	}

	h->name.count = (unsigned short)h->FontSet.cnt;
	h->name.offSize = INDEX_OFF_SIZE(h->name.datasize);

	return INDEX_SIZE(h->name.count, h->name.datasize);
}

/* Size top DICT INDEX. */
static long sizeTopDICT_INDEX(controlCtx h) {
    long i;
    long size = 0;
    for (i = 0; i < h->FontSet.cnt; i++) {
        size += h->FontSet.array[i].cff.top.cnt;
    }
    return INDEX_SIZE(h->FontSet.cnt, size);
}

static long sizeTopDICT_Data(controlCtx h) {
    return h->FontSet.array[0].cff.top.cnt;
}

static long sizeVarStore(controlCtx h) {
    return h->FontSet.array[0].cff.varStore.cnt;
}

/* Calculate initial FontSet sizes. */
static void initSetSizes(controlCtx h) {
	cfwCtx g = h->g;
	int i;

	/* Compute initial sizes */
    if (g->flags & CFW_WRITE_CFF2) {
        h->size.string      = 0;
        h->size.charset     = 0;
        h->size.Encoding    = 0;
        h->size.header      = CFF2_HDR_SIZE;
        h->size.top         = sizeTopDICT_Data(h);
        h->size.varStore    = sizeVarStore(h);
        h->size.gsubr       = cfwSubrSizeGlobal(g);
        h->size.FDSelect    = cfwFdselectFill(g);
    } else {
        h->size.header      = CFF_HDR_SIZE;
        h->size.name        = fillNameINDEX(h);
        h->size.top         = sizeTopDICT_INDEX(h);
        h->size.varStore    = 0;
        h->size.string      = cfwSindexSize(g);
        h->size.gsubr       = cfwSubrSizeGlobal(g);
        h->size.charset     = cfwCharsetFill(g);
        h->size.Encoding    = cfwEncodingFill(g);
        h->size.FDSelect    = cfwFdselectFill(g);
    }

	for (i = 0; i < h->FontSet.cnt; i++) {
		initFontSizes(h, &h->FontSet.array[i]);
	}
}

/* Calculate offsets of CFF "structures". Return 0 when offset calculation has
   stabilized else return 1. */
static int calcSetOffsets(controlCtx h) {
	int i;
	Offset lastend = h->offset.end;
    cfwCtx g = h->g;

    if (g->flags & CFW_WRITE_CFF2) {
        Offset	offset;
        h->offset.name      = 0;
        h->offset.string    = 0;
        h->offset.charset   = 0;
        h->offset.Encoding  = 0;
        
        h->offset.top       = h->size.header;
        h->offset.gsubr     = h->offset.top + h->size.top;
        offset = h->offset.gsubr + h->size.gsubr;
        if (h->size.varStore > 0) {
            h->offset.varStore  = offset;
            offset += h->size.varStore;
        }
        if (h->size.FDSelect > 0) {
            h->offset.FDSelect  = offset;
            offset += h->size.FDSelect;
        }
        h->offset.end = offset;
    } else {
        h->offset.varStore = 0;

        h->offset.name      = 0                  + h->size.header;
        h->offset.top       = h->offset.name     + h->size.name;
        h->offset.string    = h->offset.top      + h->size.top;
        h->offset.gsubr     = h->offset.string   + h->size.string;
        h->offset.charset   = h->offset.gsubr    + h->size.gsubr;
        h->offset.Encoding  = h->offset.charset  + h->size.charset;
        h->offset.FDSelect  = h->offset.Encoding + h->size.Encoding;
        h->offset.end       = h->offset.FDSelect + h->size.FDSelect;
    }

	/* Calculate offset for each font in FontSet */
	for (i = 0; i < h->FontSet.cnt; i++) {
		h->offset.end = calcFontOffsets(h, &h->FontSet.array[i], h->offset.end);
	}

	return h->offset.end != lastend;
}

/* Calculate sizes of dynamic CFF "structures". */
static void calcSetSizes(controlCtx h) {
	int i;
	long size = 0;
    cfwCtx g = h->g;
    
	for (i = 0; i < h->FontSet.cnt; i++) {
		cff_Font *font = &h->FontSet.array[i];
		calcFontSizes(h, font);
		size += font->size.top;
	}
    if (g->flags & CFW_WRITE_CFF2) {
        h->size.top = h->FontSet.array[0].size.top;
    }
    else {
        h->size.top = INDEX_SIZE(h->FontSet.cnt, size);
    }
}

/* Fill font. */
static void fillFont(controlCtx h, cff_Font *font) {
	int i;
    cfwCtx g = h->g;

    if (!(g->flags & CFW_WRITE_CFF2))
    {
        /* Fill top DICT with dynamic data */
        switch (font->offset.charset) {
            case cff_ISOAdobeCharset:
                break;
                
            case cff_ExpertCharset:
            case cff_ExpertSubsetCharset:
            default:
                cfwDictSaveIntOp(&font->cff.top, font->offset.charset, cff_charset);
                break;
        }
        
        switch (font->offset.Encoding) {
            case cff_StandardEncoding:
                break;
                
            case cff_ExpertEncoding:
            default:
                cfwDictSaveIntOp(&font->cff.top, font->offset.Encoding, cff_Encoding);
                break;
        }
    }

	cfwDictSaveIntOp(&font->cff.top, font->offset.CharStrings, cff_CharStrings);

    if (g->flags & CFW_WRITE_CFF2) {
        if (font->offset.VarStore != 0)
            cfwDictSaveIntOp(&font->cff.top, font->offset.VarStore, cff_VarStore);
        if (font->offset.FDSelect != 0)
            cfwDictSaveIntOp(&font->cff.top, font->offset.FDSelect, cff_FDSelect);
        cfwDictSaveIntOp(&font->cff.top, font->offset.FDArray, cff_FDArray);
 
        /* Fill font DICTs with dynamic data */
        for (i = 0; i < font->FDArray.cnt; i++) {
            FDInfo *fd = &font->FDArray.array[i];
            cfwDictSaveInt(&fd->cff.dict, fd->size.Private);
            cfwDictSaveInt(&fd->cff.dict, fd->offset.Private);
            cfwDictSaveOp(&fd->cff.dict, cff_Private);
        }
   }
	else if (font->flags & FONT_CID) {
		cfwDictSaveIntOp(&font->cff.top, font->offset.FDSelect, cff_FDSelect);
		cfwDictSaveIntOp(&font->cff.top, font->offset.FDArray, cff_FDArray);

		/* Fill font DICTs with dynamic data */
		for (i = 0; i < font->FDArray.cnt; i++) {
			FDInfo *fd = &font->FDArray.array[i];
			cfwDictSaveInt(&fd->cff.dict, fd->size.Private);
			cfwDictSaveInt(&fd->cff.dict, fd->offset.Private);
			cfwDictSaveOp(&fd->cff.dict, cff_Private);
		}
	}
	else {
		cfwDictSaveInt(&font->cff.top, font->size.Private);
		cfwDictSaveInt(&font->cff.top, font->offset.Private);
		cfwDictSaveOp(&font->cff.top, cff_Private);
	}

	/* Fill Private DICTs with dynamic data */
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		if (fd->Subrs.count > 0) {
			cfwDictSaveIntOp(&fd->cff.Private,
			                 fd->offset.Subrs - fd->offset.Private, cff_Subrs);
		}
	}
}

/* Fill FontSet. */
static void fillSet(controlCtx h) {
	cfwCtx g = h->g;
	int i;

	/* Fill DICTs with static data */
	for (i = 0; i < h->FontSet.cnt; i++) {
		long j;
		cff_Font *font = &h->FontSet.array[i];

		if (font->FDArray.cnt > 0)
	        cfwDictFillTop(g, &font->cff.top, &font->top,
    	                   &font->FDArray.array[0].dict, -1);
        
        if ((font->top.varStore != NULL) &&  (g->flags & CFW_WRITE_CFF2)) {
            cfwDictFillVarStore(g, &font->cff.varStore, &font->top);
        }
        
		for (j = 0; j < font->FDArray.cnt; j++) {
			FDInfo *fd = &font->FDArray.array[j];
			if ((font->flags & FONT_CID) || (g->flags & CFW_WRITE_CFF2)) {
				cfwDictFillFont(h->g, &fd->cff.dict, &fd->dict);
			}
			cfwDictFillPrivate(h->g, &fd->cff.Private, &fd->Private);
            if (!(g->flags & CFW_WRITE_CFF2))
            {
                if (fd->width.dflt != cff_DFLT_defaultWidthX) {
                    cfwDictSaveRealOp(&fd->cff.Private,
                                      fd->width.dflt, cff_defaultWidthX);
                }
                if (fd->width.nominal != cff_DFLT_nominalWidthX) {
                    cfwDictSaveRealOp(&fd->cff.Private,
                                      fd->width.nominal, cff_nominalWidthX);
                }
            }
		}
	}

	initSetSizes(h);

	/* Iterate offset calculation until stable */
	h->offset.end = 0;
	while (calcSetOffsets(h)) {
		calcSetSizes(h);
	}

	for (i = 0; i < h->FontSet.cnt; i++) {
		fillFont(h, &h->FontSet.array[i]);
	}
}

/* ---------------------------- FontSet Writing ---------------------------- */

/* Write CFF header. */
static void writeHeader(controlCtx h) {
	cfwCtx g = h->g;

    if (g->flags & CFW_WRITE_CFF2) {
        cfwWrite1(g, 2);                        /* major */
        cfwWrite1(g, 0);                        /* minor */
        cfwWrite1(g, CFF2_HDR_SIZE);             /* hdrSize */
        cfwWrite2(g, (unsigned short)h->FontSet.array[0].size.top);  /* topDict data length */
    }
    else {
        cfwWrite1(g, 1);                        /* major */
        cfwWrite1(g, 0);                        /* minor */
        cfwWrite1(g, CFF_HDR_SIZE);             /* hdrSize */
        cfwWrite1(g, OFF_SIZE(h->offset.end));  /* offSize */
    }

}

/* Write name INDEX. */
static void writeNameINDEX(controlCtx h) {
	cfwCtx g = h->g;
	int i;
	Offset offset;

	/* Write header */
	cfwWrite2(g, (unsigned short)h->name.count);
	cfwWrite1(g, h->name.offSize);

	/* Write offset array */
	offset = 1;
	cfwWriteN(g, h->name.offSize, offset);
	for (i = 0; i < h->FontSet.cnt; i++) {
		cff_Font *font = &h->FontSet.array[i];
		if (font->FDArray.cnt > 0) {
			char *name =
				cfwSindexGetString(g, (SRI)((font->flags & FONT_CID) ?
					font->top.cid.CIDFontName.impl :
					font->FDArray.array[0].dict.FontName.impl));
			/* 64-bit warning fixed by cast here */
			offset += (Offset)strlen(name);
			cfwWriteN(g, h->name.offSize, offset);
		}
	}

	/* Write object data */
	for (i = 0; i < h->FontSet.cnt; i++) {
		cff_Font *font = &h->FontSet.array[i];
		if (font->FDArray.cnt > 0) {
			char *name =
				cfwSindexGetString(g, (SRI)((font->flags & FONT_CID) ?
					font->top.cid.CIDFontName.impl :
					font->FDArray.array[0].dict.FontName.impl));
			cfwWrite(g, strlen(name), name);
		}
	}
}

/* Write top DICT INDEX */
static void writeTopDICT_INDEX(controlCtx h) {
	cfwCtx g = h->g;
	int i;
	INDEX index;
	Offset offset;

	/* Fill INDEX */
	index.datasize = 0;
	for (i = 0; i < h->FontSet.cnt; i++) {
		index.datasize += h->FontSet.array[i].size.top;
	}
	index.count = (unsigned short)h->FontSet.cnt;
	index.offSize = INDEX_OFF_SIZE(index.datasize);

	/* Write header */
	cfwWrite2(g, (unsigned short)index.count);
	cfwWrite1(g, index.offSize);

	/* Write offset array */
	offset = 1;
	cfwWriteN(g, index.offSize, offset);
	for (i = 0; i < h->FontSet.cnt; i++) {
		offset += h->FontSet.array[i].size.top;
		cfwWriteN(g, index.offSize, offset);
	}

	/* Write object data */
	for (i = 0; i < h->FontSet.cnt; i++) {
		DICT *top = &h->FontSet.array[i].cff.top;
		cfwWrite(g, top->cnt, top->array);
	}
}

static void writeTopDICT_Data(controlCtx h) {
    cfwCtx g = h->g;
    
    DICT *top = &h->FontSet.array[0].cff.top;
    cfwWrite(g, top->cnt, top->array);
}

static void writeVarStoreData_Data(controlCtx h) {
    cfwCtx g = h->g;
    
    DICT *varStore = &h->FontSet.array[0].cff.varStore;
    cfwWrite(g, varStore->cnt, varStore->array);
}

/* Return size of argument (in bytes) if formatted for charstring. */
static int numsize(float r) {
	long i = (long)r;
	if (i == r) {
		/* Integer */
		if (-107 <= i && i <= 107) {
			return 1;
		}
		else if (-1131 <= i && i <= 1131) {
			return 2;
		}
        else if (-32768 <= i && i <= 32767) {
			return 3;
		}
		else {
			return 5;
		}
	}
	else {
		/* Fractional */
		return 5;
	}
}

/* Fill tmp buffer. */
/* Assumes tmp stream file position == offset */
static void fillbuf(cfwCtx g, long offset) {
	g->tmp.length = g->cb.stm.read(&g->cb.stm, g->stm.tmp, &g->tmp.buf);
	if (g->tmp.length == 0) {
		cfwFatal(g, cfwErrTmpStream, NULL);
	}
	g->tmp.offset = offset;
	g->tmp.next = g->tmp.buf;
	g->tmp.end = g->tmp.buf + g->tmp.length;
}

/* Copy length bytes from tmp stream to dst stream. */
static void tmp2dstCopy(cfwCtx g, long length, long offset) {
	long left;
	long delta = offset - g->tmp.offset;
	if (delta >= 0 && (size_t)delta < g->tmp.length) {
		/* Offset within current buffer; reposition next byte */
		g->tmp.next = g->tmp.buf + delta;
	}
	else {
		/* Offset outside current buffer; seek to offset and fill buffer */
		if (g->cb.stm.seek(&g->cb.stm, g->stm.tmp, offset)) {
			cfwFatal(g, cfwErrTmpStream, NULL);
		}
		fillbuf(g, offset);
	}

	/* Compute bytes left in buffer */
	/* 64-bit warning fixed by cast here */
	left = (long)(g->tmp.end - g->tmp.next);
	while (left < length) {
		/* Write buffer */
		cfwWrite(g, left, g->tmp.next);
		length -= left;

		/* Refill buffer */
		/* 64-bit warning fixed by cast here */
		fillbuf(g, (long)(g->tmp.offset + g->tmp.length));
		left = (long)g->tmp.length;
	}

	/* Write buffer */
	cfwWrite(g, length, g->tmp.next);
	g->tmp.next += length;
}

/* Write CharStrings INDEX. */
static void writeCharStringsINDEX(controlCtx h, cff_Font *font) {
	cfwCtx g = h->g;
	long i;
	Offset offset;

	/* Write header */
    if (g->flags & CFW_WRITE_CFF2) {
        cfwWriteN(g, 4, font->CharStrings.count);
    }
    else {
        cfwWrite2(g, (unsigned short)font->CharStrings.count);
    }
	cfwWrite1(g, font->CharStrings.offSize);

	/* Write offset array */
	offset = 1;
	cfwWriteN(g, font->CharStrings.offSize, offset);
	for (i = 0; i < font->glyphs.cnt; i++) {
		Glyph *glyph = &font->glyphs.array[i];
		FDInfo *fd = &font->FDArray.array[glyph->iFD];

		if (glyph->cstr.length > 65535) {
			cfwFatal(g, cfwErrCstrTooLong, NULL);
		}

        if ((!(g->flags & CFW_WRITE_CFF2)) && (glyph->hAdv != fd->width.dflt)) {
			/* Add glyph width size */
			offset += numsize(glyph->hAdv - fd->width.nominal);
		}
		offset += glyph->cstr.length;

		cfwWriteN(g, font->CharStrings.offSize, offset);
	}

	/* Write charstring data */
	for (i = 0; i < font->glyphs.cnt; i++) {
		long length;
		Glyph *glyph = &font->glyphs.array[i];
		FDInfo *fd = &font->FDArray.array[glyph->iFD];

        if ((!(g->flags & CFW_WRITE_CFF2)) && (glyph->hAdv != fd->width.dflt)) {
			/* Write width */
			unsigned char t[5];
            long n;
			float r = glyph->hAdv - fd->width.nominal;
            if (r < -32768 || r >= 32768)
                cfwMessage(h->g, "out of numeric range width %g in glyph %ld", r, i);
			n = (long)r;
			length = (numsize(r) <= 3) ? cfwEncInt(n, t) : cfwEncReal(r, t);
			cfwWrite(g, length, (char *)t);
		}

		/* Copy charstring */
		if (glyph->cstr.length > 0)
			tmp2dstCopy(g, glyph->cstr.length, glyph->cstr.offset);
	}
}

/* Write FDArray. */
static void writeFDArray(controlCtx h, cff_Font *font) {
	cfwCtx g = h->g;
	int i;

	if ((font->flags & FONT_CID) || (g->flags & CFW_WRITE_CFF2)) {
		/* Compute FDArray offSize */
		INDEX index;
		Offset offset;

		/* Fill INDEX */
		index.datasize = 0;
		for (i = 0; i < font->FDArray.cnt; i++) {
			index.datasize += font->FDArray.array[i].cff.dict.cnt;
		}
		index.count = (unsigned short)font->FDArray.cnt;
		index.offSize = INDEX_OFF_SIZE(index.datasize);

		/* Write header */
        if (g->flags & CFW_WRITE_CFF2) {
            cfwWriteN(g, 4, index.count);
        }
        else {
            cfwWrite2(g, (unsigned short)index.count);
       }
		cfwWrite1(g, index.offSize);

		/* Write offset array */
		offset = 1;
		cfwWriteN(g, index.offSize, offset);
		for (i = 0; i < font->FDArray.cnt; i++) {
			offset += font->FDArray.array[i].cff.dict.cnt;
			cfwWriteN(g, index.offSize, offset);
		}

		/* Write object data */
		for (i = 0; i < font->FDArray.cnt; i++) {
			DICT *dict = &font->FDArray.array[i].cff.dict;
			cfwWrite(g, dict->cnt, dict->array);
		}
	}

	/* Write Private DICTs */
	for (i = 0; i < font->FDArray.cnt; i++) {
		DICT *Private = &font->FDArray.array[i].cff.Private;
		cfwWrite(g, Private->cnt, Private->array);
	}

	/* Write local Subrs INDEXes */
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		cfwSubrWriteLocal(g, &fd->subrData);
	}
}

/* Write FontSet. */
static void writeSet(controlCtx h) {
	cfwCtx g = h->g;
	int i;

	/* Initialize tmp file read */
	g->tmp.offset = 0;
	g->tmp.length = 0;

	/* Open output stream */
	g->stm.dst = g->cb.stm.open(&g->cb.stm, CFW_DST_STREAM_ID, h->offset.end);
	if (g->stm.dst == NULL) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}

	/* Write aggregate data structures */
	writeHeader(h);
    if (g->flags & CFW_WRITE_CFF2) {
        writeTopDICT_Data(h);
        cfwSubrWriteGlobal(g);
        writeVarStoreData_Data(h);
        cfwFdselectWrite(g);
    }
    else {
        writeNameINDEX(h);
        writeTopDICT_INDEX(h);
        cfwSindexWrite(g);
        cfwSubrWriteGlobal(g);
        cfwCharsetWrite(g);
        cfwEncodingWrite(g);
        cfwFdselectWrite(g);
    }

	/* Write per-font data structures */
	for (i = 0; i < h->FontSet.cnt; i++) {
		cff_Font *font = &h->FontSet.array[i];
		writeCharStringsINDEX(h, font);
		writeFDArray(h, font);
    }

	/* Close output stream */
	if (g->cb.stm.close(&g->cb.stm, g->stm.dst)) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}
}

/* Begin new FontSet. */
int cfwBegSet(cfwCtx g, long flags) {
	controlCtx h = g->ctx.control;
	g->flags = flags;
	h->FontSet.cnt = 0;
    if (flags & CFW_WRITE_CFF2)
        h->flags |= FONTSET_CFF2;
    
	/* Open debug stream */
	g->stm.dbg = g->cb.stm.open(&g->cb.stm, CFW_DBG_STREAM_ID, 0);

	return 0;
}

/* Begin new font. */
int cfwBegFont(cfwCtx g, cfwMapCallback *map, unsigned long maxNumSubrs) {
	controlCtx h = g->ctx.control;
	long index;

	/* set tmp buffer buffers safely */
	g->tmp.offset = 0;
	g->tmp.length = 0;
    g->maxNumSubrs = maxNumSubrs;

	/* Allocate and initialize new font */
	index = dnaNext(&h->FontSet, sizeof(cff_Font));
	if (index == -1) {
		return cfwErrNoMemory;
	}
	h->_new = &h->FontSet.array[index];
	h->_new->CharStrings.datasize = 0;
	h->_new->map = map;

	/* Allocate slot for .notdef glyph */
	index = dnaSetCnt(&h->_new->glyphs, sizeof(Glyph), 1);
	if (index == -1) {
		return cfwErrNoMemory;
	}
	h->_new->glyphs.array[0].info = NULL;

	/* For h->_new->seenGlyphs, we do NOT need to pre-allocate for .notdef
	   as we are not forcing it to the begining of the list */


	h->flags &= ~(SEEN_NAME_KEYED_GLYPH | SEEN_CID_KEYED_GLYPH);
	h->mergedDicts = 0;
 
	return cfwSuccess;
}

/* Order glyphs in CID-keyed font. */
static void orderCIDKeyedGlyphs(controlCtx h) {
	cfwCtx g = h->g;
	long i;
	unsigned char fdmap[256];
	long nGlyphs = h->_new->glyphs.cnt;
	Glyph *glyphs = h->_new->glyphs.array;

	if (glyphs[0].info == NULL) {
		cfwFatal(h->g, cfwErrNoCID0, NULL);
	}

	if (!(g->flags & CFW_PRESERVE_GLYPH_ORDER)) {
		/* Insertion sort glyphs by CID order */
		for (i = 2; i < nGlyphs; i++) {
			long j = i;
			Glyph tmp = glyphs[i];
			while (tmp.info->cid < glyphs[j - 1].info->cid) {
				glyphs[j] = glyphs[j - 1];
				j--;
			}
			if (j != i) {
				glyphs[j] = tmp;
			}
		}
	}

	/* Mark used FDs */
	memset(fdmap, 0, 256);
	for (i = 0; i < nGlyphs; i++) {
		fdmap[glyphs[i].iFD] = 1;
	}

	/* Remove unused FDs from FDArray */
    {
        long j = 0;
        for (i = 0; i < h->_new->FDArray.cnt; i++) {
            if (fdmap[i]) {
                if (i != j) {
                    /* Swap elements preserving dynamic arrays for later freeing */
                    FDInfo tmp = h->_new->FDArray.array[j];
                    h->_new->FDArray.array[j] = h->_new->FDArray.array[i];
                    h->_new->FDArray.array[i] = tmp;
                }
                fdmap[i] = (unsigned char)j++;
            }
        }
        
        if (i != j) {
            /* Unused FDs; remap glyphs to new FDArray */
            for (i = 0; i < nGlyphs; i++) {
                glyphs[i].iFD = fdmap[glyphs[i].iFD];
            }
            h->_new->FDArray.cnt = j;
        }
    }
    
	cfwCharsetBeg(g, 1);
	cfwFdselectBeg(g);

	cfwFdselectAddIndex(g, glyphs[0].iFD);
	for (i = 1; i < nGlyphs; i++) {
		Glyph *glyph = &glyphs[i];
		cfwCharsetAddGlyph(g, glyph->info->cid);
		cfwFdselectAddIndex(g, glyph->iFD);
	}

	h->_new->iObject.charset = cfwCharsetEnd(g);
	h->_new->iObject.Encoding = 0;
	h->_new->iObject.FDSelect = cfwFdselectEnd(g);

	h->_new->top.cid.CIDCount = glyphs[nGlyphs - 1].info->cid + 1;
}

/* Order glyphs in name-keyed font. */
static void orderNameKeyedGlyphs(controlCtx h) {
	cfwCtx g = h->g;
	int predef;
	int id;
	long i;
	long j;
	long k;
	Glyph tmp;
	long nGlyphs = h->_new->glyphs.cnt;
	Glyph *glyphs = h->_new->glyphs.array;

	if (glyphs[0].info == NULL) {
		cfwFatal(h->g, cfwErrNoNotdef, NULL);
	}

	if (!(g->flags & CFW_PRESERVE_GLYPH_ORDER)) {
		/* Set sentinel for insertion sort */
		glyphs[0].info->encoding.code = 0;

		/* Sort glyphs by encoding order using insertion sort */
		for (i = 2; i < nGlyphs; i++) {
			j = i;
			tmp = glyphs[i];
			while (tmp.info->encoding.code < glyphs[j - 1].info->encoding.code) {
				glyphs[j] = glyphs[j - 1];
				j--;
			}
			if (j != i) {
				glyphs[j] = tmp;
			}
		}
		glyphs[0].info->encoding.code = ABF_GLYPH_UNENC;
	}

	/* Assign SIDs and remember last encoded glyph */
	j = 0;
	for (i = 1; i < nGlyphs; i++) {
		abfGlyphInfo *info = glyphs[i].info;
		abfString *gname = &info->gname;

		gname->impl = cfwSindexAssignSID(g, (SRI)gname->impl);

		if (info->encoding.code != ABF_GLYPH_UNENC) {
			j = i;
		}
	}

	if (!(g->flags & CFW_PRESERVE_GLYPH_ORDER)) {
		if (++j < nGlyphs) {
			/* Font has unencoded glyphs; set sentinel for insetion sort */
			k = j;
			for (i = j + 1; i < nGlyphs; i++) {
				if (glyphs[i].info->gname.impl < glyphs[k].info->gname.impl) {
					k = i;
				}
			}
			if (k != j) {
				tmp = glyphs[j];
				glyphs[j] = glyphs[k];
				glyphs[k] = tmp;
			}
			/* Sort unencoded glyphs by SID order using insertion sort */
			for (i = j + 2; i < nGlyphs; i++) {
				j = i;
				tmp = glyphs[i];
				while (tmp.info->gname.impl < glyphs[j - 1].info->gname.impl) {
					glyphs[j] = glyphs[j - 1];
					j--;
				}
				if (j != i) {
					glyphs[j] = tmp;
				}
			}
		}
	}

	/* Check for predefined encoding */
	predef = 0;
	if (g->flags & CFW_FORCE_STD_ENCODING) {
		h->_new->iObject.Encoding = 0;
		predef = 1;
	}
	else {
		for (id = 0;; id++) {
			int cnt;
			const unsigned char *array = cfwEncodingGetPredef(id, &cnt);
			if (array == NULL) {
				break;
			}

			/* Check compatibility against predef encoding */
			for (i = 0; i < nGlyphs; i++) {
				abfGlyphInfo *info = glyphs[i].info;
				SID sid = (SID)info->gname.impl;
				short font_enc = (info->encoding.code == ABF_GLYPH_UNENC) ?
				    -1 : (short)info->encoding.code;
				short predef_enc = (sid >= cnt || array[sid] == 0) ? -1 : array[sid];

				if (font_enc != predef_enc) {
					goto incompat;
				}
			}

			/* Found compatible predef encoding */
			h->_new->iObject.Encoding = id;
			predef = 1;
			break;

incompat:;
		}
	}
	cfwCharsetBeg(g, 0);
	if (!predef) {
		cfwEncodingBeg(g);
	}

	for (i = 1; i < nGlyphs; i++) {
		abfGlyphInfo *info = glyphs[i].info;
		abfString *gname = &info->gname;

		/* Add glyph to charset */
		cfwCharsetAddGlyph(g, (unsigned short)gname->impl);

		if (!predef) {
			abfEncoding *enc = &info->encoding;
			if (enc->code != ABF_GLYPH_UNENC) {
				/* Add glyph to encoding */
				cfwEncodingAddCode(g, (unsigned char)enc->code);
				while ((enc = enc->next) != NULL) {
					cfwEncodingAddSupCode(g, (unsigned char)enc->code,
					                      (SID)gname->impl);
				}
			}
		}
	}

	h->_new->iObject.charset = cfwCharsetEnd(g);
	if (!predef) {
		h->_new->iObject.Encoding = cfwEncodingEnd(g);
	}
	h->_new->iObject.FDSelect = 0;
}

/* Assign the default and nominal widths (see comment at head of file). */
static int assignWidths(controlCtx h, FDInfo *fd) {
	/* Assign default values */
	fd->width.dflt = cff_DFLT_defaultWidthX;
	fd->width.nominal = cff_DFLT_nominalWidthX;

	if (fd->width.freqs.cnt == 0) {
		/* Unused FD */
		return 0;
	}
	else if (fd->width.freqs.cnt == 1) {
		/* Fixed-pitch font */
		WidthFreq *rec = &fd->width.freqs.array[0];
		fd->width.dflt = rec->width;
		return 0;
	}
	else {
		float nominal;
		int i;
		int minsize;
		int dictsize;
		int nonoptsize;
		int iNominal = 0;   /* Suppress optimizer warning */
		int iDefault = 0;   /* Suppress optimizer warning */

		/* Find non-optimized size */
		nonoptsize = 0;
		for (i = 0; i < fd->width.freqs.cnt; i++) {
			WidthFreq *rec = &fd->width.freqs.array[i];
			if (rec->width != 0) {
				nonoptsize += numsize(rec->width) * rec->count;
			}
		}

		/* Find best combination of nominal and default widths */
		minsize = nonoptsize;
		for (i = 0; i < fd->width.freqs.cnt; i++) {
			int j;
			int nomsize;
			float nomwidth = fd->width.freqs.array[i].width + 107;

			/* Compute total size for this nominal width */
			nomsize = 0;
			for (j = 0; j < fd->width.freqs.cnt; j++) {
				WidthFreq *rec = &fd->width.freqs.array[j];
				nomsize += numsize(rec->width - nomwidth) * rec->count;
			}

			/* Try this nominal width against all possible defaults */
			for (j = 0; j < fd->width.freqs.cnt; j++) {
				WidthFreq *dflt = &fd->width.freqs.array[j];
				int totsize =
				    nomsize - numsize(dflt->width - nomwidth) * dflt->count;
				if (totsize < minsize) {
					minsize = totsize;
					iNominal = i;
					iDefault = j;
				}
			}
		}

        {
             float dflt;
            dflt = fd->width.freqs.array[iDefault].width;
            nominal = fd->width.freqs.array[iNominal].width + 107;
            
            /* Compute size of dictionary entries */
            dictsize = 0;
            if (dflt != 0) {
                dictsize += numsize(dflt) + DICT_OP_SIZE(cff_defaultWidthX);
            }
            if (nominal != 0) {
                dictsize += numsize(nominal) + DICT_OP_SIZE(cff_nominalWidthX);
            }
            
            if (minsize + dictsize < nonoptsize) {
                /* Optimized savings; set optimization values */
                fd->width.dflt = dflt;
                fd->width.nominal = nominal;
                return minsize;
            }
            else {
                /* No savings; set non-optimized size */
                return nonoptsize;
            }
        }
	}
}

/* Match width. */
static int CTL_CDECL matchWidth(const void *key, const void *value, void *ctx) {
	float a = *(float *)key;
	float b = ((WidthFreq *)value)->width;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Analyse width frequencies and assign default and nominal widths. */
static void analyzeWidths(controlCtx h) {
	long i;

	/* Make width frequency records */
	for (i = 0; i < h->_new->glyphs.cnt; i++) {
		size_t index;
		Glyph *glyph = &h->_new->glyphs.array[i];
		FDInfo *fd = &h->_new->FDArray.array[glyph->iFD];

		/* Check if width already present */
		if (ctuLookup(&glyph->hAdv,
		              fd->width.freqs.array, fd->width.freqs.cnt,
		              sizeof(WidthFreq), matchWidth, &index, NULL)) {
			/* Match found; bump count */
			fd->width.freqs.array[index].count++;
		}
		else {
			/* Not found; add new record */
			WidthFreq *_new =
			    &dnaGROW(fd->width.freqs, fd->width.freqs.cnt)[index];

			/* Make hole for new record */
			_new = &fd->width.freqs.array[index];
			memmove(_new + 1, _new,
			        (fd->width.freqs.cnt++ - index) * sizeof(WidthFreq));

			/* Fill new record */
			_new->width = glyph->hAdv;
			_new->count = 1;
		}
	}

	/* Choose nominal and default widths */
	h->_new->size.widths = 0;
	for (i = 0; i < h->_new->FDArray.cnt; i++) {
		h->_new->size.widths += assignWidths(h, &h->_new->FDArray.array[i]);
	}
}

/* End font. */
int cfwEndFont(cfwCtx g, abfTopDict *top) {
	controlCtx h = g->ctx.control;
	long i;

	if (top == NULL) {
		/* Discard the last font */
		if (h->FontSet.cnt > 0) {
			h->FontSet.cnt--;
		}
		return cfwSuccess;
	}

	/* Set error handler */
    DURING_EX(g->err.env)

        /* If there are more warnings than get shown, write out how many there were. */
        printFinalWarn(g);

        /* Set font-wide flags */
        h->_new->flags = 0;
        if (top->sup.flags & ABF_CID_FONT) {
            /* Validate CID data */
            if (top->cid.Registry.ptr == ABF_UNSET_PTR ||
                top->cid.Ordering.ptr == ABF_UNSET_PTR ||
                top->cid.Supplement == ABF_UNSET_INT) {
                return cfwErrBadDict;
            }

            if (h->flags & SEEN_NAME_KEYED_GLYPH) {
                return cfwErrGlyphType;
            }
            if (top->FDArray.cnt < 1 || top->FDArray.cnt > 256) {
                return cfwErrBadFDArray;
            }

            h->_new->flags |= FONT_CID;
        }
        else {
            if (h->flags & SEEN_CID_KEYED_GLYPH) {
                return cfwErrGlyphType;
            }
            if (top->FDArray.cnt != 1) {
                return cfwErrBadFDArray;
            }

            if (top->sup.flags & ABF_SYN_FONT) {
                h->_new->flags |= FONT_SYN;
            }
        }

        /* Copy dict data from client */
        cfwDictCopyTop(g, &h->_new->top, top);

        if (!(h->mergedDicts && (g->flags & CFW_CHECK_IF_GLYPHS_DIFFER))) {
            /* Initialize FDArray */
            dnaSET_CNT(h->_new->FDArray, top->FDArray.cnt);
            for (i = 0; i < top->FDArray.cnt; i++) {
                abfFontDict *src = &top->FDArray.array[i];
                FDInfo *dst = &h->_new->FDArray.array[i];

                /* Copy font and Private dict data from client */
                cfwDictCopyFont(g, &dst->dict, src);
                cfwDictCopyPrivate(g, &dst->Private, &src->Private);

                dst->Subrs.count = 0;
                memset(&dst->subrData, 0, sizeof(subr_CSData));
                dst->width.freqs.cnt = 0;
            }
        }
        /* else
            {
            FDArray has already been added by client during glyph processing
            Warning: at this point, the  h->_new->FDArray is out of sync with the h->_new->top.FDArray!
            There may ne more dicts in h->_new->FDArray.
            }
         */

        if (h->_new->flags & FONT_CID) {
            orderCIDKeyedGlyphs(h);
        }
        else {
            orderNameKeyedGlyphs(h);
        }

        analyzeWidths(h);

        if (h->_new->map != NULL && !(g->flags & CFW_PRESERVE_GLYPH_ORDER)) {
            /* Callback glyph mapping */
            for (i = 0; i < h->_new->glyphs.cnt; i++) {
                h->_new->map->glyphmap(h->_new->map, (unsigned short)i,
                                      h->_new->glyphs.array[i].info);
            }
        }

        /* The "glyph->info" field points to the client stucture that was supplied
           to the glyphBeg() callback. This isn't required to be stable after
           returning from from this function so we set to null to ensure it isn't
           subsequently used by mistake. */
        for (i = 0; i < h->_new->glyphs.cnt; i++) {
            h->_new->glyphs.array[i].info = NULL;
        }

        /* release the merge copy buffers */
        if (h->mergeBuffers.newGlyph != NULL) {
            g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.newGlyph, 0);
            h->mergeBuffers.newGlyph = NULL;
        }
        if (h->mergeBuffers.oldGlyph != NULL) {
            g->cb.mem.manage(&g->cb.mem, h->mergeBuffers.oldGlyph, 0);
            h->mergeBuffers.oldGlyph = NULL;
        }

    HANDLER
        return g->err.code;
    END_HANDLER

	return cfwSuccess;
}

/* Preparse module for reuse. */
static void cfwControlReuse(cfwCtx g) {
}

static void initSubrData(subr_CSData *subrData) {
	subrData->data = 0;
	subrData->offset = 0;
}

static void freeSubrData(cfwCtx g, subr_CSData *subrData) {
	if (subrData->data) {
		cfwMemFree(g, subrData->data);
	}
	if (subrData->offset) {
		cfwMemFree(g, subrData->offset);
	}
}

/* Call subroutinizer with copies of charstrings */
static void cfwCallSubrizer(cfwCtx g) {
	controlCtx h = g->ctx.control;
	int nFonts =  h->FontSet.cnt;
	int iFont;
	int iGlyph;
	cff_Font *cffFont;
	subr_Font *subrFonts = (subr_Font*)cfwMemNew(g, sizeof(subr_Font) * nFonts);
	subr_Font *subrFont;
	Offset offset;

	/* Repackage font data in formats subroutinizer expects */
	memset(subrFonts, 0, sizeof(subr_Font) * nFonts);

	DURING_EX(g->err.env)

	for (iFont = 0; iFont < nFonts; iFont++) {
		cffFont = &h->FontSet.array[iFont];
		subrFont = &subrFonts[iFont];
			
		/* Set up FDInfo and FDIndex arrays */
		subrFont->fdCount = (short)cffFont->FDArray.cnt;
		subrFont->fdInfo = (subr_FDInfo *)cfwMemNew(g, sizeof(subr_FDInfo) * subrFont->fdCount);
		memset(subrFont->fdInfo, 0, sizeof(subr_FDInfo) * subrFont->fdCount);

		if ((cffFont->flags & FONT_CID) || (g->flags & CFW_WRITE_CFF2)) {
			subrFont->flags = SUBR_FONT_CID;
			subrFont->fdIndex = (subr_FDIndex *)cfwMemNew(g, sizeof(subr_FDIndex) * cffFont->glyphs.cnt);
			for (iGlyph = 0; iGlyph < cffFont->glyphs.cnt; iGlyph++) {
				Glyph *glyph = &cffFont->glyphs.array[iGlyph];
				subrFont->fdIndex[iGlyph] = glyph->iFD;
			}
		}

		/* Set up charoffsets and charstrings in memory */
		subrFont->chars.nStrings = (unsigned short)cffFont->glyphs.cnt;
		subrFont->chars.offset = (Offset *)cfwMemNew(g, sizeof(Offset) * (subrFont->chars.nStrings));
		subrFont->chars.data = (char *)cfwMemNew(g, g->cb.stm.tell(&g->cb.stm, g->stm.tmp));

		offset = 0;
		for (iGlyph = 0; iGlyph < cffFont->glyphs.cnt; iGlyph++) {
			Glyph *glyph = &cffFont->glyphs.array[iGlyph];
			tmp2bufCopy(g, glyph->cstr.length, glyph->cstr.offset, &subrFont->chars.data[offset]);
			offset += glyph->cstr.length;
			subrFont->chars.offset[iGlyph] = offset;
		}
	}

	/* Call the subroutinizer. The result replaces the charstring in the buffer in place. */
	cfwSubrSubrize(g, nFonts, &subrFonts[0]);

	/* Copy back charstrings back to tmp buffer */
	if (g->cb.stm.seek(&g->cb.stm, g->stm.tmp, 0)) {
		cfwFatal(g, cfwErrTmpStream, NULL);
	}

	for (iFont = 0; iFont < nFonts; iFont++) {
		long length;
		FDInfo *fd;
		int i;
		cffFont = &h->FontSet.array[iFont];
		subrFont = &subrFonts[iFont];

		length = subrFont->chars.offset[subrFont->chars.nStrings - 1];
		if ((int)g->cb.stm.write(&g->cb.stm, g->stm.tmp,
		                         length, subrFont->chars.data) != length) {
			cfwFatal(g, cfwErrTmpStream, NULL);
		}

		offset = 0;
		for (iGlyph = 0; iGlyph < cffFont->glyphs.cnt; iGlyph++) {
			Glyph *glyph = &cffFont->glyphs.array[iGlyph];
			Offset nextOffset = subrFont->chars.offset[iGlyph];
			glyph->cstr.offset = offset;
			glyph->cstr.length = nextOffset - offset;
			offset = nextOffset;
		}

		/* Copy back local subr info */
		for (i = 0; i < cffFont->FDArray.cnt; i++) {
			fd = &cffFont->FDArray.array[i];
			if ((cffFont->flags & FONT_CID) || (g->flags & CFW_WRITE_CFF2)) {
				fd->subrData = subrFont->fdInfo[i].subrs;
			}
			else {
				fd->subrData = subrFont->subrs;
			}
			fd->Subrs.count = fd->subrData.nStrings;
		}
	}

	h->_new->CharStrings.datasize = g->cb.stm.tell(&g->cb.stm, g->stm.tmp);

	HANDLER
	END_HANDLER

	/* Free temporary data used by subroutinizer */
	for (iFont = 0; iFont < nFonts; iFont++) {
		subrFont = &subrFonts[iFont];
		freeSubrData(g, &subrFont->chars);
		if (subrFont->fdInfo) {
			cfwMemFree(g, subrFont->fdInfo);
		}
		if (subrFont->fdIndex) {
			cfwMemFree(g, subrFont->fdIndex);
		}
	}

	if (subrFonts)
		cfwMemFree(g, subrFonts);
	}

/* End FontSet. */
int cfwEndSet(cfwCtx g) {
	controlCtx h = g->ctx.control;

	if (h->FontSet.cnt == 0) {
		return cfwSuccess;  /* Nothing to do */
	}
	/* Set error handler */
    DURING_EX(g->err.env)

        /* Subroutinize charstrings */
        if (g->flags & CFW_SUBRIZE) {
            cfwCallSubrizer(g);
        }

        if (g->flags & CFW_IS_CUBE) {
            cfwMergeCubeGSUBR(g);
        }



        fillSet(h); /* all subrs and charstrings must be inplace by the time this is called. */
        writeSet(h);

        /* Prepare for reuse */
        cfwControlReuse(g);
        cfwCharsetReuse(g);
        cfwEncodingReuse(g);
        cfwFdselectReuse(g);
        cfwSindexReuse(g);
        cfwDictReuse(g);
        cfwCstrReuse(g);
        cfwSubrReuse(g);

        /* Close debug stream */
        if (g->stm.dbg != NULL) {
            (void)g->cb.stm.close(&g->cb.stm, g->stm.dbg);
        }

    HANDLER
        return g->err.code;
    END_HANDLER

	return cfwSuccess;
}

/* -------------------------- Safe dynarr Context -------------------------- */

/* Manage memory and handle failure. */
static void *safeManage(ctlMemoryCallbacks *cb, void *old, size_t size) {
	cfwCtx g = (cfwCtx)cb->ctx;
	void *ptr = g->cb.mem.manage(&g->cb.mem, old, size);
	if (size > 0 && ptr == NULL) {
		cfwFatal(g, cfwErrNoMemory, NULL);
	}
	return ptr;
}

/* Initialize error handling dynarr context. */
static void dnaSafeInit(cfwCtx g) {
	ctlMemoryCallbacks cb;
	cb.ctx          = g;
	cb.manage       = safeManage;
	g->ctx.dnaSafe  = dnaNew(&cb, DNA_CHECK_ARGS);
}

/* -------------------------- Fail dynarr Context -------------------------- */

/* Manage memory and handle failure. */
static void *failManage(ctlMemoryCallbacks *cb, void *old, size_t size) {
	cfwCtx g = (cfwCtx)cb->ctx;
	return g->cb.mem.manage(&g->cb.mem, old, size);
}

/* Initialize error returning dynarr context. */
static void dnaFailInit(cfwCtx g) {
	ctlMemoryCallbacks cb;
	cb.ctx          = g;
	cb.manage       = failManage;
	g->ctx.dnaFail  = dnaNew(&cb, DNA_CHECK_ARGS);
	if (g->ctx.dnaFail == NULL) {
		cfwFatal(g, cfwErrNoMemory, NULL);
	}
}

/* ---------------------------- Library Context ---------------------------- */

/* Validate client and create context. */
cfwCtx cfwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL) {
	cfwCtx g;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(CFW_VERSION)) {
		return NULL;
	}

	/* Allocate context */
	g = (cfwCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct cfwCtx_));
	if (g == NULL) {
		return NULL;
	}

	/* Safety initialization */
	memset(g, 0, sizeof(*g));

	g->cb.mem = *mem_cb;
	g->cb.stm = *stm_cb;
	g->stm.dst = NULL;
	g->stm.tmp = NULL;
	g->stm.dbg = NULL;

	/* Set error handler */
    DURING_EX(g->err.env)

        /* Initialize service libraries */
        dnaSafeInit(g);
        dnaFailInit(g);

        /* Initialize modules */
        cfwControlNew(g);
        cfwCharsetNew(g);
        cfwEncodingNew(g);
        cfwFdselectNew(g);
        cfwSindexNew(g);
        cfwDictNew(g);
        cfwCstrNew(g);
        cfwSubrNew(g);

        g->err.code = cfwSuccess;

    HANDLER
        /* Initialization failed */
        cfwFree(g);
        g = NULL;
    END_HANDLER

	return g;
}

/* Free context. */
void cfwFree(cfwCtx g) {
	if (g == NULL) {
		return;
	}

	/* Free other modules */
	cfwControlFree(g);
	cfwCharsetFree(g);
	cfwEncodingFree(g);
	cfwFdselectFree(g);
	cfwSindexFree(g);
	cfwDictFree(g);
	cfwCstrFree(g);
	cfwSubrFree(g);

	/* Free service libraries */
	dnaFree(g->ctx.dnaSafe);
	dnaFree(g->ctx.dnaFail);

	/* Free library context */
	g->cb.mem.manage(&g->cb.mem, g, 0);
}

/* ----------------------------- Error Handling ---------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(cfwCtx g, char *fmt, va_list ap) {
	char text[500];
    const size_t textLen = sizeof(text);

	if (g->stm.dbg == NULL) {
		return; /* Debug stream not available */
	}
	VSPRINTF_S(text, textLen, fmt, ap);
	(void)g->cb.stm.write(&g->cb.stm, g->stm.dbg, strlen(text), text);
}

/* Write message to debug stream from varargs. */
void CTL_CDECL cfwMessage(cfwCtx g, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vmessage(g, fmt, ap);
	va_end(ap);
}

/* Handle fatal error. */
void CTL_CDECL cfwFatal(cfwCtx g, int err_code, char *fmt, ...) {
	if (fmt == NULL) {
		/* Write standard error message */
		cfwMessage(g, "%s", cfwErrStr(err_code));
	}
	else {
		/* Write font-specific error message */
		va_list ap;
		va_start(ap, fmt);
		vmessage(g, fmt, ap);
		va_end(ap);
	}
	g->err.code = (short)err_code;
  RAISE(&g->err.env, err_code, NULL);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
void *cfwMemNew(cfwCtx g, size_t size) {
	void *ptr = g->cb.mem.manage(&g->cb.mem, NULL, size);
	if (ptr == NULL) {
		cfwFatal(g, cfwErrNoMemory, NULL);
	}

	/* Safety initialization */
	memset(ptr, 0, size);

	return ptr;
}

/* Free memory. */
void cfwMemFree(cfwCtx g, void *ptr) {
	(void)g->cb.mem.manage(&g->cb.mem, ptr, 0);
}

/* ------------------------------ Data Output ------------------------------ */

/* Write 1-byte number. */
void cfwWrite1(cfwCtx g, unsigned char value) {
	if (g->cb.stm.write(&g->cb.stm, g->stm.dst, 1, (char *)&value) != 1) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}
}

/* Write 2-byte number. */
void cfwWrite2(cfwCtx g, unsigned short value) {
	unsigned char buf[2];
	buf[0] = (unsigned char)(value >> 8);
	buf[1] = (unsigned char)value;
	if (g->cb.stm.write(&g->cb.stm, g->stm.dst, 2, (char *)buf) != 2) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}
}

/* Write N-byte number. */
void cfwWriteN(cfwCtx g, int N, unsigned long value) {
	char buf[4];
	unsigned char *p = (unsigned char *)buf;
	switch (N) {
		case 4:
			*p++ = (unsigned char)(value >> 24);

		case 3:
			*p++ = (unsigned char)(value >> 16);

		case 2:
			*p++ = (unsigned char)(value >> 8);

		case 1:
			*p = (unsigned char)value;
	}
	if (g->cb.stm.write(&g->cb.stm, g->stm.dst, N, buf) != (size_t)N) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}
}

/* Write data buffer. */
void cfwWrite(cfwCtx g, size_t count, char *buf) {
	if (g->cb.stm.write(&g->cb.stm, g->stm.dst, count, buf) != count) {
		cfwFatal(g, cfwErrDstStream, NULL);
	}
}

/* Encode integer in array and return length. */
int cfwEncInt(long i, unsigned char *t) {
	if (-107 <= i && i <= 107) {
		/* Single byte number */
		t[0] = (unsigned char)(i + 139);
		return 1;
	}
	else if (108 <= i && i <= 1131) {
		/* Positive 2-byte number */
		i -= 108;
		t[0] = (unsigned char)((i >> 8) + 247);
		t[1] = (unsigned char)i;
		return 2;
	}
	else if (-1131 <= i && i <= -108) {
		/* Negative 2-byte number */
		i += 108;
		t[0] = (unsigned char)((-i >> 8) + 251);
		t[1] = (unsigned char)-i;
		return 2;
	}
	else if (-32768 <= i && i <= 32767) {
		/* Positive/negative 3-byte number (shared with dict ops) */
		t[0] = (unsigned char)t2_shortint;
		t[1] = (unsigned char)(i >> 8);
		t[2] = (unsigned char)i;
		return 3;
	}
	else {
		/* Positive/negative 5-byte number (dict ops only) */
		t[0] = (unsigned char)cff_longint;
		t[1] = (unsigned char)(i >> 24);
		t[2] = (unsigned char)(i >> 16);
		t[3] = (unsigned char)(i >> 8);
		t[4] = (unsigned char)i;
		return 5;
	}
}

/* Encode real number as 5-byte fixed point in array and return length. */
int cfwEncReal(float r, unsigned char *t) {
	long i = (long)(r * 65536.0 + ((r < 0) ? -0.5 : 0.5));
	t[0] = (unsigned char)255;
	t[1] = (unsigned char)(i >> 24);
	t[2] = (unsigned char)(i >> 16);
	t[3] = (unsigned char)(i >> 8);
	t[4] = (unsigned char)i;
	return 5;
}

/* Get version numbers of libraries. */
void cfwGetVersion(ctlVersionCallbacks *cb) {
	if (cb->called & 1 << CFW_LIB_ID) {
		return; /* Already enumerated */
	}
	/* Support libraries */
	dnaGetVersion(cb);
	ctuGetVersion(cb);

	/* This library */
	cb->getversion(cb, CFW_VERSION, "cffwrite");

	/* Record this call */
	cb->called |= 1 << CFW_LIB_ID;
}

/* ----------------------------- Error Support ----------------------------- */

/* Map error code to error string. */
char *cfwErrStr(int err_code) {
	static char *errstrs[] = {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string)    string,
#include "cfwerr.h"
	};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs)) ?
	       "unknown error" : errstrs[err_code];
}

/* return current error status. */
int cfwGetErrCode(cfwCtx h) {
	return h->err.code;
}

/* ----------------------------- Debug Support ----------------------------- */

#if CFW_DEBUG

/* Dump glyphs. */
static void dbglyphs(controlCtx h, cff_Font *font) {
	long i;
	if (h->flags & FONT_CID) {
		printf("--- glyph[index]={tag,cid,iFD,hAdv,length,offset}\n");
		for (i = 0; i < font->glyphs.cnt; i++) {
			Glyph *glyph = &font->glyphs.array[i];
			abfGlyphInfo *info = glyph->info;
			printf("[%3ld]={%hu,%hu,%u,%g,%ld,%ld}\n",
			       i, info->tag, info->cid, glyph->iFD,
			       glyph->hAdv, glyph->cstr.length, glyph->cstr.offset);
		}
	}
	else {
		printf("--- glyph[index]="
		       "{tag,hAdv,length,offset,gname,code[+code]+}\n");
		for (i = 0; i < font->glyphs.cnt; i++) {
			Glyph *glyph = &font->glyphs.array[i];
			abfGlyphInfo *info = glyph->info;
			abfEncoding *enc = &info->encoding;
			printf("[%3ld]={%3hu,%4g,%3ld,%5ld,",
			       i, info->tag, glyph->hAdv,
			       glyph->cstr.length, glyph->cstr.offset);
			if (enc->code == ABF_GLYPH_UNENC) {
				printf("   -");
			}
			else {
				printf("0x%02lX", enc->code);
				for (;; ) {
					enc = enc->next;
					if (enc == NULL) {
						break;
					}
					printf("+0x%02lX", enc->code);
				}
			}
			printf(",%s}\n", info->gname.ptr);
		}
	}
}

/* Dump width frequencies. */
static void dbwidths(controlCtx h, cff_Font *font) {
	int i;
	int j;
	for (i = 0; i < font->FDArray.cnt; i++) {
		FDInfo *fd = &font->FDArray.array[i];
		printf("--- FD[%d]\n", i);
		printf("default=%g\n", fd->width.dflt);
		printf("nominal=%g\n", fd->width.nominal);
		printf("--- freqs[index]={width,count}\n");
		if (fd->width.freqs.cnt == 0) {
			printf("empty\n");
		}
		else {
			for (j = 0; j < fd->width.freqs.cnt; j++) {
				WidthFreq *rec = &fd->width.freqs.array[j];
				printf("[%d]={%g,%ld}\n", j, rec->width, rec->count);
			}
		}
	}
}

/* Dump CFF layout. */
static void dblayout(controlCtx h) {
	int i;

	printf("object       offset     size\n");
	printf("----------  -------  -------\n");
	printf("header            0  %7ld\n",                   h->size.header);
	printf("name        %7ld  %7ld\n",  h->offset.name,     h->size.name);
	printf("top         %7ld  %7ld\n",  h->offset.top,      h->size.top);
	printf("string      %7ld  %7ld\n",  h->offset.string,   h->size.string);
	printf("gsubr       %7ld  %7ld\n",  h->offset.gsubr,    h->size.gsubr);
	printf("charset     %7ld  %7ld\n",  h->offset.charset,  h->size.charset);
	printf("Encoding    %7ld  %7ld\n",  h->offset.Encoding, h->size.Encoding);
	printf("FDSelect    %7ld  %7ld\n",  h->offset.FDSelect, h->size.FDSelect);
	printf("end         %7ld        -\n", h->offset.end);

	for (i = 0; i < h->FontSet.cnt; i++) {
		int j;
		cff_Font *font = &h->FontSet.array[i];

		printf("--- font[%d]\n", i);
		printf("top               -  %7ld\n", font->size.top);
		printf("charset     %7ld        -\n", font->offset.charset);
		printf("Encoding    %7ld        -\n", font->offset.Encoding);
		printf("FDSelect    %7ld        -\n", font->offset.FDSelect);
		printf("CharStrings %7ld  %7ld\n",
		       font->offset.CharStrings, font->size.CharStrings);
        printf("VarStore     %7ld  %7ld\n",
               font->offset.VarStore, font->size.VarStore);
        printf("FDArray     %7ld  %7ld\n",
               font->offset.FDArray, font->size.FDArray);
		printf("Private     %7ld  %7ld\n",
		       font->offset.Private, font->size.Private);
		printf("Subrs       %7ld  %7ld\n",
		       font->offset.Subrs, font->size.Subrs);

		for (j = 0; j < font->FDArray.cnt; j++) {
			FDInfo *fd = &font->FDArray.array[j];

			printf("-- FDArray[%d]\n", j);
			printf("Private     %7ld  %7ld\n",
			       fd->offset.Private, fd->size.Private);
			printf("Subrs       %7ld  %7ld\n",
			       fd->offset.Subrs, fd->size.Subrs);
		}
	}
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
	dbuse(0, dbglyphs, dbwidths, dblayout);
}

#endif /* CFW_DEBUG */
