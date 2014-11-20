/* @(#)CM_VerSion tc.c atm08 1.2 16245.eco sum= 11245 atm08.002 */
/* @(#)CM_VerSion tc.c atm07 1.2 16164.eco sum= 19567 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "common.h"

#include "encoding.h"
#include "sindex.h"
#include "charset.h"
#include "subr.h"
#include "parse.h"
#include "recode.h"
#include "t13.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* CFF header */
typedef struct {
	unsigned char major;        /* Major version number */
	unsigned char minor;        /* Minor version number */
	unsigned char hdrSize;      /* CFF header size */
	OffSize offSize;            /* Absolute offset (0) size */
} CFFHdr;
#define CFF_HEADER_SIZE (sizeCard8 * 4)

/* Module context */
struct tcprivCtx_ {
	CFFHdr cffHdr;              /* CFF header */
	INDEXHdr nameHdr;           /* FontName index header */
	INDEXHdr dictHdr;           /* Font dict index header */
	dnaDCL(Font, set);          /* Font set */
	struct {                    /* --- FontSet component offsets */
		Offset encodings;
		Offset charsets;
		Offset FDSelects;
		Offset copyright;
	} offset;
	struct {                    /* --- FontSet component size */
		long FontNames;
		long dicts;
		long nameINDEX;
		long dictINDEX;
		long strings;
		long gsubrs;
		long encodings;
		long charsets;
		long FDSelects;
	} size;
	struct {                    /* --- FontSet wrapper */
		char *name;
		char *version;
		char *master;           /* Chameleon master fontname */
		long size;
	} FontSet;
	char *copyright;            /* Font copyright notice */
	long syntheticWeight;       /* override for synthetic font UDV */
	tcCtx g;                    /* Package context */
};

/* Use previous sizes to compute new offsets */
static int calcOffsets(tcprivCtx h) {
	int i;
	Offset chars;
	Offset prevCopyright = h->offset.copyright;

	h->offset.encodings = CFF_HEADER_SIZE + h->size.nameINDEX +
	    h->size.dictINDEX + h->size.strings + h->size.gsubrs;

	h->offset.charsets = h->offset.encodings + h->size.encodings;
	h->offset.FDSelects = h->offset.charsets + h->size.charsets;

	chars = h->offset.FDSelects + h->size.FDSelects;

	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		font->offset.encoding =
		    encodingGetOffset(h->g, font->iEncoding, h->offset.encodings);

		if (!(font->flags & FONT_SYNTHETIC)) {
			font->offset.charset =
			    charsetGetOffset(h->g, font->iCharset, h->offset.charsets);
			font->offset.fdselect =
			    fdselectGetOffset(h->g, font->iFDSelect, h->offset.FDSelects);

			font->offset.CharStrings = chars;
			font->offset.FDArray = chars + font->size.CharStrings;
			font->offset.Private = font->offset.FDArray + font->size.FDArray;

			/* Subrs follow Private so set offset relative to Private dict */
			font->offset.Subrs = font->size.Private;

			chars =
			    font->offset.Private + font->size.Private + font->size.Subrs;

			if (font->flags & FONT_CID) {
				/* Update Private and Subr offset for CID font */
				int j;
				Offset privoff = font->offset.Private;
				Offset subroff = font->offset.Subrs + privoff;

				for (j = 0; j < font->fdCount; j++) {
					FDInfo *fdInfo = &font->fdInfo[j];
					if (fdInfo->seenChar) {
						fdInfo->offset.Private = privoff;
						fdInfo->offset.Subrs = subroff - privoff;

						privoff += fdInfo->size.Private;
						subroff += fdInfo->size.Subrs;
					}
				}
			}
		}
	}
	h->offset.copyright = chars;

	return h->offset.copyright != prevCopyright;
}

/* Use previous offsets to compute new sizes */
static void calcSizes(tcprivCtx h) {
	int i;
	char t[5];

	h->cffHdr.offSize = OFF_SIZE(h->offset.copyright);

	h->size.dicts = 0;
	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		if (font->flags & FONT_SYNTHETIC) {
			if (font->offset.encoding != 0) {
				font->size.dict =
				    (font->dict.cnt +
				     csEncInteger(font->offset.encoding, t) +
				     DICTOPSIZE(cff_Encoding));
			}
			font->size.CharStrings = 0;
			font->size.Private = 0;
		}
		else {
			if (font->flags & FONT_CID) {
				font->size.dict =
				    (font->dict.cnt +
				     csEncInteger(font->offset.CharStrings, t) +
				     DICTOPSIZE(cff_CharStrings) +
				     csEncInteger(font->offset.fdselect, t) +
				     DICTOPSIZE(cff_FDSelect) +
				     csEncInteger(font->offset.FDArray, t) +
				     DICTOPSIZE(cff_FDArray));
			}
			else if (font->flags & FONT_CHAMELEON) {
				font->size.dict =
				    (font->dict.cnt +
				     csEncInteger(font->size.Private, t) +
				     csEncInteger(font->offset.Private, t) +
				     DICTOPSIZE(cff_Private));
			}
			else {
				font->size.dict =
				    (font->dict.cnt +
				     csEncInteger(font->offset.CharStrings, t) +
				     DICTOPSIZE(cff_CharStrings) +
				     csEncInteger(font->size.Private, t) +
				     csEncInteger(font->offset.Private, t) +
				     DICTOPSIZE(cff_Private));
			}

			if (font->offset.encoding != 0) {
				font->size.dict +=
				    (csEncInteger(font->offset.encoding, t) +
				     DICTOPSIZE(cff_Encoding));
			}
			if (font->offset.charset != 0) {
				font->size.dict +=
				    (csEncInteger(font->offset.charset, t) +
				     DICTOPSIZE(cff_charset));
			}

			font->size.CharStrings =
			    (font->flags & FONT_CHAMELEON) ? 0 : csSizeChars(h->g, font);

			if (font->flags & FONT_CID) {
				/* Calculate FDArray, Private, and Subr sizes */
				int j;
				int fdCount = 0;
				long sizeFDs = 0;

				font->size.Private = 0;
				for (j = 0; j < font->fdCount; j++) {
					FDInfo *fdInfo = &font->fdInfo[j];
					if (fdInfo->seenChar) {
						if (fdInfo->size.Subrs != 0) {
							fdInfo->size.Private =
							    (fdInfo->Private.cnt +
							     csEncInteger(fdInfo->offset.Subrs, t) +
							     DICTOPSIZE(cff_Subrs));
						}

						font->size.Private += fdInfo->size.Private;

						sizeFDs += fdInfo->size.FD =
						        (fdInfo->FD.cnt +
						         csEncInteger(fdInfo->size.Private, t) +
						         csEncInteger(fdInfo->offset.Private, t) +
						         DICTOPSIZE(cff_Private));

						fdCount++;
					}
				}
				font->size.FDArray = INDEX_SIZE(fdCount, sizeFDs);
			}
			else if (font->size.Subrs != 0) {
				font->size.Private =
				    (font->Private.cnt +
				     csEncInteger(font->offset.Subrs, t) +
				     DICTOPSIZE(cff_Subrs));
			}
		}
		h->size.dicts += font->size.dict;
	}
	h->size.dictINDEX = INDEX_SIZE(h->set.cnt, h->size.dicts);
}

/* Fill initial sizes in font */
static void fillInitialSizes(tcprivCtx h, Font *font) {
	font->size.FontName = strlen(font->FontName);
	font->size.dict = font->dict.cnt;
	font->size.CharStrings =
	    (font->flags & FONT_CHAMELEON) ? 0 : csSizeChars(h->g, font);

	if (font->flags & FONT_CID) {
		int i;
		int fdCount = 0;
		long sizeFDs = 0;

		font->size.Private = 0;
		font->size.Subrs = 0;
		for (i = 0; i < font->fdCount; i++) {
			FDInfo *fdInfo = &font->fdInfo[i];
			if (fdInfo->seenChar) {
				sizeFDs += fdInfo->size.FD = fdInfo->FD.cnt;
				font->size.Private +=
				    fdInfo->size.Private = fdInfo->Private.cnt;
#if TC_SUBR_SUPPORT
				font->size.Subrs +=
				    fdInfo->size.Subrs = subrSizeLocal(&fdInfo->subrs);
#endif /* TC_SUBR_SUPPORT */
				fdCount++;
			}
		}
		font->size.FDArray = INDEX_SIZE(fdCount, sizeFDs);
	}
	else {
		font->size.FDArray = 0;
		if (!(font->flags & FONT_CHAMELEON)) {
			font->size.Private = font->Private.cnt;
		}
#if TC_SUBR_SUPPORT
		font->size.Subrs = subrSizeLocal(&font->subrs);
#else /* TC_SUBR_SUPPORT */
		font->size.Subrs = 0;
#endif /* TC_SUBR_SUPPORT */
	}
	h->size.FontNames += font->size.FontName;
	h->size.dicts += font->dict.cnt;
}

/* Calculate all offset in font set. Start by assuming the best possible case
   where all variable-length offsets are 1-byte long and calculate sizes and
   offsets. Then recalculate all sizes and offsets and iterate until there are
   no further changes. */
static void fillOffsets(tcprivCtx h) {
	int i;

	/* Initialize header */
	h->cffHdr.major = TC_MAJOR;
	h->cffHdr.minor = TC_MINOR;
	h->cffHdr.offSize = 1;

	/* Compute initial font sizes */
	h->size.FontNames = 0;
	h->size.dicts = 0;
	h->size.gsubrs = subrSizeGlobal(h->g);

	for (i = 0; i < h->set.cnt; i++) {
		fillInitialSizes(h, &h->set.array[i]);
	}

	h->size.nameINDEX = INDEX_SIZE(h->set.cnt, h->size.FontNames);
	h->size.dictINDEX = INDEX_SIZE(h->set.cnt, h->size.dicts);

	/* Iterate offset computation until final offset remains unchanged */
	h->offset.copyright = 0;
	while (calcOffsets(h)) {
		calcSizes(h);
	}

	h->cffHdr.hdrSize = CFF_HEADER_SIZE;

	/* Fill index headers */
	h->nameHdr.count = (unsigned short)h->set.cnt;
	h->nameHdr.offSize = INDEX_OFF_SIZE(h->size.FontNames);
	h->dictHdr.count = (unsigned short)h->set.cnt;
	h->dictHdr.offSize = INDEX_OFF_SIZE(h->size.dicts);

	/* Add offsets to dictionaries */
	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		if (font->offset.encoding != 0) {
			/* Save encoding offset */
			dictSaveInt(&font->dict, font->offset.encoding);
			DICTSAVEOP(font->dict, cff_Encoding);
		}

		if (!(font->flags & FONT_SYNTHETIC)) {
			if (font->offset.charset != 0) {
				/* Save charset offset */
				dictSaveInt(&font->dict, font->offset.charset);
				DICTSAVEOP(font->dict, cff_charset);
			}
			if (!(font->flags & FONT_CHAMELEON)) {
				/* Save CharString offset */
				dictSaveInt(&font->dict, font->offset.CharStrings);
				DICTSAVEOP(font->dict, cff_CharStrings);
			}
			if (font->flags & FONT_CID) {
				int j;

				/* Save FDSelect offset */
				dictSaveInt(&font->dict, font->offset.fdselect);
				DICTSAVEOP(font->dict, cff_FDSelect);

				/* Save FDArray offset */
				dictSaveInt(&font->dict, font->offset.FDArray);
				DICTSAVEOP(font->dict, cff_FDArray);

				for (j = 0; j < font->fdCount; j++) {
					FDInfo *fdInfo = &font->fdInfo[j];
					if (fdInfo->seenChar) {
						/* Save Private sizes/offsets */
						dictSaveInt(&fdInfo->FD, fdInfo->size.Private);
						dictSaveInt(&fdInfo->FD, fdInfo->offset.Private);
						DICTSAVEOP(fdInfo->FD, cff_Private);

						if (fdInfo->size.Subrs != 0) {
							/* Save Subrs offset */
							dictSaveInt(&fdInfo->Private, fdInfo->offset.Subrs);
							DICTSAVEOP(fdInfo->Private, cff_Subrs);
						}
					}
				}
			}
			else {
				/* Save Private size/offset */
				dictSaveInt(&font->dict, font->size.Private);
				dictSaveInt(&font->dict, font->offset.Private);
				DICTSAVEOP(font->dict, cff_Private);

				if (font->size.Subrs != 0) {
					/* Save Subrs offset */
					dictSaveInt(&font->Private, font->offset.Subrs);
					DICTSAVEOP(font->Private, cff_Subrs);
				}
			}
		}
	}
}

/* Compare FontNames */
static int CDECL cmpFontNames(const void *first, const void *second) {
	return strcmp(((Font *)first)->FontName, ((Font *)second)->FontName);
}

/* Match font name */
static int CDECL matchFontName(const void *key, const void *value) {
	return strcmp((char *)key, ((Font *)value)->FontName);
}

/* Fill font set from PostScript font files */
static void fillSet(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;
	int duplicate;
	int i;
	Font *last;

	/* Sort set by font name */
	qsort(h->set.array, h->set.cnt, sizeof(Font), cmpFontNames);

	/* Check for duplicate fonts */
	duplicate = 0;
	last = &h->set.array[0];
	for (i = 1; i < h->set.cnt; i++) {
		Font *curr = &h->set.array[i];
		if (strcmp(curr->FontName, last->FontName) == 0) {
			if (g->cb.message != NULL) {
				/* Report duplicate FontNames */
				char text[513];
				sprintf(text, "--- duplicate FontName: %s, files:",
				        curr->FontName);
				g->cb.message(g->cb.ctx, tcERROR, text);
				g->cb.message(g->cb.ctx, tcERROR, last->filename);
				g->cb.message(g->cb.ctx, tcERROR, curr->filename);
			}
			duplicate = 1;
		}
		last = curr;
	}
	if (duplicate) {
		if (g->cb.message != NULL) {
			g->cb.message(g->cb.ctx, tcFATAL, "aborting because of errors");
		}
		g->cb.fatal(g->cb.ctx);
	}

	/* Handle synthetic fonts */
	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		if (font->flags & FONT_SYNTHETIC) {
			Font *base = (Font *)bsearch(font->synthetic.baseName,
			                             h->set.array, h->set.cnt,
			                             sizeof(Font), matchFontName);
			font->iEncoding = font->synthetic.iEncoding;
			if (base == NULL) {
				/* No synthetic base font; make conventional font */
				dnaFREE(font->synthetic.dict);
				font->flags &= ~FONT_SYNTHETIC;
			}
			else {
				/* Synthetic base found: make synthetic font */
				csFreeFont(g, font);
				dnaFREE(font->Private);
				font->Private.cnt = 0;

				/* Build new dict with SyntheticBase op first */
				font->dict.cnt = 0;
				dictSaveInt(&font->dict, base - h->set.array);
				DICTSAVEOP(font->dict, cff_SyntheticBase);

				/* Append the other synthetic ops to dict */
				COPY(dnaEXTEND(font->dict, font->synthetic.dict.cnt),
				     font->synthetic.dict.array, font->synthetic.dict.cnt);

				dnaFREE(font->synthetic.dict);
			}
			MEM_FREE(g, font->synthetic.baseName);
		}
	}

#if TC_SUBR_SUPPORT
	if (g->flags & TC_SUBRIZE) {
		subrSubrize(g, h->set.cnt, h->set.array);
	}
#endif /* TC_SUBR_SUPPORT */

	if (t13CheckAuth(g, &h->set.array[0]) && h->set.cnt != 1) {
		tcFatal(g, "authentication applied to multiple fonts");
	}

	h->size.encodings = encodingFill(g);
	h->size.charsets = charsetFill(g);
	h->size.strings = sindexSize(g);

	h->size.FDSelects = fdselectFill(g);

	fillOffsets(h);

	h->FontSet.size = h->offset.copyright +
	    ((h->copyright == NULL) ? 0 : strlen(h->copyright));
}

/* Add PostScript wapper header */
static void fillWrapHeader(tcCtx g, char *wrapHeader) {
	tcprivCtx h = g->ctx.tcpriv;
	char call[128];

	if (h->FontSet.master == NULL) {
		sprintf(call, "/%s %lu StartData ", h->FontSet.name, h->FontSet.size);
	}
	else {
		sprintf(call, "/%s %lu /%s StartData ",
		        h->FontSet.name, h->FontSet.size, h->FontSet.master);
	}
	sprintf(wrapHeader,
	        "%%!PS-Adobe-3.0 Resource-FontSet\n"
	        "%%%%DocumentNeededResources: ProcSet (FontSetInit)\n"
	        "%%%%EndComments\n"
	        "%%%%IncludeResource: ProcSet (FontSetInit)\n"
	        "%%%%BeginResource: FontSet (%s)\n"
	        "%%%%Title: (FontSet/%s)\n"
	        "%%%%Version: %s\n"
	        "/FontSetInit /ProcSet findresource begin\n"
	        "%%%%BeginData: %lu Binary Bytes\n"
	        "%s",
	        h->FontSet.name,
	        h->FontSet.name,
	        (h->FontSet.version != NULL) ? h->FontSet.version : "1.000",
	        strlen(call) + h->FontSet.size,
	        call);
}

/* Add PostScript wapper trailer */
static void fillWrapTrailer(tcCtx g, char *wrapTrailer) {
	static char buf[] =
	    "\n"
	    "%%EndData\n"
	    "%%EndResource\n"
	    "%%EOF\n";
	strcpy(wrapTrailer, buf);
}

#if TC_STATISTICS
/* Show compression statistics */
static void showStats(tcprivCtx h) {
#define AVG(v, c) ((long)((c) ? (double)(v) / (c) + .5 : 0))
	tcCtx g = h->g;
	int i;
	struct {
		long nSubrs;
		long subrSize;
		long nChars;
		long charSize;
		long flatSize;
	}
	comp;
	long compOther;
	long compDataSize;

	comp.nSubrs = 0;
	comp.subrSize = 0;
	comp.nChars = 0;
	comp.charSize = 0;
	comp.flatSize = 0;

	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];
		if (!(font->flags & (FONT_SYNTHETIC | FONT_CHAMELEON))) {
			comp.nSubrs += font->subrs.nStrings;
			comp.subrSize += (font->subrs.nStrings == 0) ?
			    0 : font->subrs.offset[font->subrs.nStrings - 1];
			comp.nChars += font->chars.nStrings;
			comp.charSize += font->chars.offset[font->chars.nStrings - 1];
			comp.flatSize += font->flatSize;
		}
	}

	if (g->stats.gather == 1) {
		fprintf(stderr, "%8ld %8ld %.1f %s  (orig/comp/%%orig/FontName)\n",
		        g->stats.fontSize, h->FontSet.size,
		        h->FontSet.size * 100.0 / g->stats.fontSize,
		        (h->set.cnt == 1) ? h->set.array[0].FontName : "set");
	}
	else {
		long origOther;
		long origDataSize;

		printf("       ------------orig-----------"
		       "  ---------------comp----------------\n"
		       "          count      size avg.size"
		       "     count      size avg.size  orig %%\n"
		       "       ---------------------------"
		       "  -----------------------------------\n"
		       "flat   %8ld %9ld %8ld  %8ld#%9ld %8ld %6.1f%%\n",
		       g->stats.nChars,
		       g->stats.flatSize,
		       AVG(g->stats.flatSize, g->stats.nChars),
		       comp.nChars,
		       comp.flatSize,
		       AVG(comp.flatSize, comp.nChars),
		       ((double)comp.flatSize * g->stats.nChars * 100.0) /
		       ((double)comp.nChars * g->stats.flatSize));

		printf("subrs  %8ld %9ld*%8ld  %8ld %9ld %8ld     -\n",
		       g->stats.nSubrs,
		       g->stats.subrSize,
		       AVG(g->stats.subrSize, g->stats.nSubrs),
		       comp.nSubrs,
		       comp.subrSize,
		       AVG(comp.subrSize, comp.nSubrs));

		origDataSize = g->stats.subrSize + g->stats.charSize;
		compDataSize = comp.subrSize + comp.charSize;
		printf("chars  %8ld %9ld*%8ld  %8ld#%9ld %8ld %6.1f%%$\n",
		       g->stats.nChars,
		       g->stats.charSize,
		       AVG(g->stats.charSize, g->stats.nChars),
		       comp.nChars,
		       comp.charSize,
		       AVG(comp.charSize, comp.nChars),
		       (((double)g->stats.nChars *
		         (comp.subrSize + comp.charSize) * 100.0) /
		        ((double)comp.nChars *
		         (g->stats.subrSize + g->stats.charSize))));

		origOther = g->stats.fontSize -
		    (g->stats.subrSize + g->stats.charSize);
		compOther = h->FontSet.size - (comp.subrSize + comp.charSize);
		printf("other@ %8ld %9ld %8ld  %8ld %9ld %8ld %6.1f%%\n",
		       h->set.cnt,
		       origOther,
		       AVG(origOther, h->set.cnt),
		       h->set.cnt,
		       compOther,
		       AVG(compOther, h->set.cnt),
		       compOther * 100.0 / origOther);

		printf("fonts  %8ld %9ld %8ld  %8ld %9ld %8ld %6.1f%%\n"
		       "\n"
		       "* original subr and char sizes without lenIV bytes.\n"
		       "@ other.size=fonts.size-(subrs.size+chars.size)\n"
		       "# comp.count may be <orig.count if synthetic fonts included.\n"
		       "$ orig %%=(orig.count*(comp.subr.size+comp.char.size)*100)/\n"
		       "         (comp.count*(orig.subr.size+orig.char.size)).\n",
		       h->set.cnt,
		       g->stats.fontSize,
		       AVG(g->stats.fontSize, h->set.cnt),
		       h->set.cnt,
		       h->FontSet.size,
		       AVG(h->FontSet.size, h->set.cnt),
		       h->FontSet.size * 100.0 / g->stats.fontSize);
	}
#undef AVG
}

#endif /* TC_STATISTICS */

/* Write CID font dicts */
static void writeCIDDicts(tcCtx g, Font *font) {
	OffSize offSize;
	unsigned count;
	Offset offset;
	int i;
	long sizeFDs;

	/* Only output fd's which contained a character. */
	/* The FDSelect which is written syncs with this. See cidAddChars. */
	count = 0;
	sizeFDs = 0;
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		if (info->seenChar) {
			sizeFDs += info->FD.cnt;
			count++;
		}
	}

	/* Font dict index */
	OUT2(count);
	offSize = INDEX_OFF_SIZE(sizeFDs);
	OUT1(offSize);
	offset = 1;
	OUTOFF(offSize, offset);
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		if (info->seenChar) {
			offset += info->FD.cnt;
			OUTOFF(offSize, offset);
		}
	}
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		if (info->seenChar) {
			OUTN(info->FD.cnt, info->FD.array);
		}
	}

	/* Private dicts */
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		if (info->seenChar) {
			OUTN(info->Private.cnt, info->Private.array);
		}
	}

#if TC_SUBR_SUPPORT
	/* Subrs */
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		if (info->seenChar && info->size.Subrs != 0) {
			subrWriteLocal(g, &info->subrs);
		}
	}
#endif /* TC_SUBR_SUPPORT */

#if 0
	for (i = 0; i < font->fdCount; i++) {
		printf("--- FD/PD[%d]\n", i);
		dictDump(g, &font->fdInfo[i].FD);
		dictDump(g, &font->fdInfo[i].Private);
	}
#endif
}

/* Write font set file */
static void writeSet(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;
	int i;
	int nSynthetics;
	Offset offset;
	struct {
		char header[1024];
		char trailer[128];
	}
	wrap;

	if (h->FontSet.name != NULL) {
		/* Create wrappers */
		fillWrapHeader(g, wrap.header);
		fillWrapTrailer(g, wrap.trailer);
		h->FontSet.size += strlen(wrap.header) + strlen(wrap.trailer);
	}

	if (g->cb.cffSize != NULL) {
		/* Report FontSet size to client */
		g->cb.cffSize(g->cb.ctx, h->FontSet.size,
		              (g->status & TC_EURO_ADDED) != 0);
	}

	if (h->FontSet.name != NULL) {
		OUTN(strlen(wrap.header), wrap.header);
	}

	/* Write header */
	OUT1(h->cffHdr.major);
	OUT1(h->cffHdr.minor);
	OUT1(h->cffHdr.hdrSize);
	OUT1(h->cffHdr.offSize);

	/* Write name index */
	OUT2(h->nameHdr.count);
	OUT1(h->nameHdr.offSize);
	offset = 1;
	OUTOFF(h->nameHdr.offSize, offset);
	for (i = 0; i < h->set.cnt; i++) {
		offset += h->set.array[i].size.FontName;
		OUTOFF(h->nameHdr.offSize, offset);
	}
	for (i = 0; i < h->set.cnt; i++) {
		OUTN(h->set.array[i].size.FontName, h->set.array[i].FontName);
	}

	/* Write top dict index */
	OUT2(h->dictHdr.count);
	OUT1(h->dictHdr.offSize);
	offset = 1;
	OUTOFF(h->dictHdr.offSize, offset);
	for (i = 0; i < h->set.cnt; i++) {
		offset += h->set.array[i].size.dict;
		OUTOFF(h->dictHdr.offSize, offset);
	}
	for (i = 0; i < h->set.cnt; i++) {
		OUTN(h->set.array[i].dict.cnt, h->set.array[i].dict.array);
	}

#if 0
	for (i = 0; i < h->set.cnt; i++) {
		dictDump(g, &h->set.array[i].dict);
	}
#endif

	sindexWrite(g);     /* Write string table */
	subrWriteGlobal(g); /* Write global subrs */

	/* Write charsets, encodings, and fdIndexes */
	encodingWrite(g);
	charsetWrite(g);
	fdselectWrite(g);

	/* Write remainder of font data */
	nSynthetics = 0;
	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		if (font->flags & FONT_SYNTHETIC) {
			nSynthetics++;
		}
		else if (font->flags & FONT_CHAMELEON) {
			OUTN(font->size.Private, font->chameleon.data);
		}
		else {
			DICT *Private = &font->Private;

			csWriteChars(g, font);      /* Write CharsStrings */
			if (font->flags & FONT_CID) {
				/* Write font and Private dict indexes */
				writeCIDDicts(g, font);
			}
			else {
				/* Write Private dict */
				OUTN(Private->cnt, Private->array);
			}
#if TC_SUBR_SUPPORT
			subrWriteLocal(g, &font->subrs);    /* Write Subrs */
#endif /* TC_SUBR_SUPPORT */
		}
	}

	/* Write legal text */
	if (h->copyright != NULL) {
		OUTN(strlen(h->copyright), h->copyright);
	}
	else if (g->flags & TC_NONOTICE && h->set.cnt > 1) {
		tcWarning(g, "no copyright notice specified");
	}

	if (h->FontSet.name != NULL) {
		OUTN(strlen(wrap.trailer), wrap.trailer);
	}

#if TC_STATISTICS
	if (g->stats.gather) {
		showStats(h);
	}
	else {
		fprintf(stderr,
		        "Created: %s (fonts=%ld+%d=%ld, size=%ld, %ld/font)\n",
		        (g->cb.cffId == NULL) ? "?" : g->cb.cffId(g->cb.ctx),
		        h->set.cnt - nSynthetics, nSynthetics,
		        h->set.cnt, h->FontSet.size, h->FontSet.size / h->set.cnt);
	}
#endif /* TC_STATISTICS */
}

/* ------------------------------- Interface ------------------------------- */

/* Initialize font record */
static void fontInit(void *ctx, long count, Font *font) {
	long i;
	tcCtx g = ctx;
	for (i = 0; i < count; i++) {
		font->flags = 0;
		font->FontName = NULL;
		font->fdCount = 0;
		font->fdInfo = NULL;
		font->fdIndex = NULL;
		font->iFDSelect = 0;
		dnaINIT(g->ctx.dnaCtx, font->dict, 50, 50);
		dnaINIT(g->ctx.dnaCtx, font->Private, 100, 50);
		font->chars.nStrings = 0;
		font->subrs.nStrings = 0;
		/* Not used in low-memory mode. Avoid freeing them. */
		font->subrs.offset = NULL;
		font->subrs.data = NULL;
		font->chars.offset = NULL;
		font->chars.data = NULL;
		font->chameleon.data = NULL;
	#if TC_STATISTICS
		font->flatSize = 0;
	#endif /* TC_STATISTICS */
		font++;
	}
	return;
}

/* (Re)Initialize font set */
static void initSet(tcCtx g, tcprivCtx h) {
	g->spd = NULL;
#if TC_STATISTICS
	g->stats.gather = 0;
	g->stats.flatSize = 0;
	g->stats.fontSize = 0;
#endif /* TC_STATISTICS */
	h->copyright = NULL;
	h->FontSet.name = NULL;
}

static ctlMemoryCallbacks tc_dna_memcb;

static void *tc_manage(ctlMemoryCallbacks *cb, void *old, size_t size) {
	tcCtx h = (tcCtx)cb->ctx;
	tcCallbacks tcb =   h->cb;
	void *p = NULL;
	if (size > 0) {
		if (old == NULL) {
			p = tcb.malloc(tcb.ctx, size);
			return (p);
		}
		else {
			p = tcb.realloc(tcb.ctx, old, size);
			return (p);
		}
	}
	else {
		if (old == NULL) {
			return NULL;
		}
		else {
			tcb.free(tcb.ctx, old);
			return NULL;
		}
	}
}

/* Create new context */
tcCtx tcNew(tcCallbacks *cb) {
	tcCtx g = cb->malloc(cb->ctx, sizeof(struct tcCtx_));
	tcprivCtx h;

	g->cb = *cb;

	h = MEM_NEW(g, sizeof(struct tcprivCtx_));

	tc_dna_memcb.ctx = g;
	tc_dna_memcb.manage = tc_manage;
	g->ctx.dnaCtx = dnaNew(&tc_dna_memcb, DNA_CHECK_ARGS);

	dnaINIT(g->ctx.dnaCtx, h->set, 4, 120);
	h->set.func = fontInit;

	initSet(g, h);

	/* Initialize other library modules */
	g->ctx.sindex   = NULL;
	g->ctx.fdselect = NULL;
	g->ctx.subr     = NULL;
	g->ctx.cs       = NULL;
	g->ctx.encoding = NULL;
	g->ctx.charset  = NULL;
	g->ctx.recode   = NULL;
	g->ctx.parse    = NULL;
	g->ctx.tcpriv   = NULL;
	g->ctx.t13      = NULL;


	sindexNew(g);
	encodingNew(g);
	charsetNew(g);
	parseNew(g);
	csNew(g);
	recodeNew(g);
#if TC_SUBR_SUPPORT
	subrNew(g);
#endif /* TC_SUBR_SUPPORT */
	fdselectNew(g);
	t13New(g);

	/* Link contexts */
	h->g = g;
	g->ctx.tcpriv = h;

	return g;
}

/* Free font record */
static void freeFonts(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;
	int i;

	/* Free font data */
	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		MEM_FREE(g, font->FontName);
		dnaFREE(font->dict);
		dnaFREE(font->Private);
		csFreeFont(g, font);
		if (font->flags & FONT_CID) {
			int fdCount = font->fdCount;
			int fd;
			if (font->fdInfo) {
				for (fd = 0; fd < fdCount; fd++) {
					dnaFREE(font->fdInfo[fd].FD);
					dnaFREE(font->fdInfo[fd].Private);
				}
				MEM_FREE(g, font->fdInfo);
			}
			MEM_FREE(g, font->fdIndex);
		}
		MEM_FREE(g, font->chameleon.data);
		(void)fontInit(g, 1, font); /* Prepare to reuse */
	}
	h->set.cnt = 0;
}

/* Free context */
void tcFree(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;

	/* Free modules */
	sindexFree(g);
	encodingFree(g);
	charsetFree(g);
	parseFree(g);
	csFree(g);
	recodeFree(g);
#if TC_SUBR_SUPPORT
	subrFree(g);
#endif /* TC_SUBR_SUPPORT */
	fdselectFree(g);
	t13Free(g);

	freeFonts(g);

	dnaFREE(h->set);
	MEM_FREE(g, h);
	g->cb.free(g->cb.ctx, g);   /* Free context */
}

/* Add font to set */
void tcAddFont(tcCtx g, long flags) {
	tcprivCtx h = g->ctx.tcpriv;
	Font *font = dnaNEXT(h->set);

	if (flags & TC_SMALLMEMORY && g->cb.tmpOpen == NULL) {
		tcFatal(g, "callbacks not supplied for SMALLMEMORY mode");
	}

	g->flags = flags;
	font->filename = (g->cb.psId == NULL) ? "?" : g->cb.psId(g->cb.ctx);
	parseFont(g, font);
}

/* Add additional copyright notice to font set */
void tcAddCopyright(tcCtx g, char *copyright) {
	tcprivCtx h = g->ctx.tcpriv;
	h->copyright = copyright;
}

/* Set the max number of subrs*/
void tcSetMaxNumSubrsOverride(tcCtx g, unsigned long maxNumSubrs) {
	g->maxNumSubrs = maxNumSubrs;
}

/* Set the  weight value for the design vectore for making synthetic glyphs */
void tcSetWeightOverride(tcCtx g, long weight) {
	tcprivCtx h = g->ctx.tcpriv;
	h->syntheticWeight = weight;
}

/* Get the  weight value for the design vectore for making synthetic glyphs */
long tcGetWeightOverride(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;
	return (h->syntheticWeight);
}

/* Add PS wrapper to FontSet */
void tcWrapSet(tcCtx g, char *name, char *version, char *master) {
	tcprivCtx h = g->ctx.tcpriv;
	h->FontSet.name = name;
	h->FontSet.version = version;
	h->FontSet.master = master;
}

/* Compact font set */
void tcCompactSet(tcCtx g) {
	tcprivCtx h = g->ctx.tcpriv;
	fillSet(g);
	writeSet(g);
	freeFonts(g);
	initSet(g, h);
#if TC_SUBR_SUPPORT
	subrReuse(g);
#endif /* TC_SUBR_SUPPORT */
}

/* Compact single font */
void tcCompactFont(tcCtx g, long flags) {
	tcAddFont(g, flags);
	tcCompactSet(g);
}

/* ----------------------------- Error handling ---------------------------- */

/* Report fatal error message (with source filename appended) and quit */
void CDECL tcFatal(tcCtx g, char *fmt, ...) {
	if (g->cb.message != NULL) {
		/* Format message */
		char text[513];
		va_list ap;

		va_start(ap, fmt);
		vsprintf(text, fmt, ap);

		if (g->cb.psId != NULL) {
			/* Append source data id */
			sprintf(&text[strlen(text)], " [%s]", g->cb.psId(g->cb.ctx));
		}

		g->cb.message(g->cb.ctx, tcFATAL, text);
		va_end(ap);
	}
	g->cb.fatal(g->cb.ctx);
}

/* Report warning error message (with source filename appended) */
void CDECL tcWarning(tcCtx g, char *fmt, ...) {
	if (g->cb.message != NULL) {
		/* Format message */
		char text[513];
		va_list ap;

		va_start(ap, fmt);
		vsprintf(text, fmt, ap);

		if (g->cb.psId != NULL) {
			/* Append source data id */
			sprintf(&text[strlen(text)], " [%s]", g->cb.psId(g->cb.ctx));
		}

		g->cb.message(g->cb.ctx, tcWARNING, text);
		va_end(ap);
	}
}

/* Report informational message  */
void CDECL tcNote(tcCtx g, char *fmt, ...) {
	if (g->cb.message != NULL) {
		/* Format message */
		char text[513];
		va_list ap;

		va_start(ap, fmt);
		vsprintf(text, fmt, ap);

		if (g->cb.psId != NULL) {
			/* Append source data id */
			sprintf(&text[strlen(text)], " [%s]", g->cb.psId(g->cb.ctx));
		}

		g->cb.message(g->cb.ctx, tcNOTE, text);
		va_end(ap);
	}
}

/* -------------------------------- Output  -------------------------------- */

/* Output 2-byte number in big-endian order */
void tcOut2(tcCtx g, short value) {
	g->cb.cffWrite1(g->cb.ctx, value >> 8);
	g->cb.cffWrite1(g->cb.ctx, value);
}

/* Output 1, 2, 3, or 4-byte offset in big-endian order */
void tcOutOff(tcCtx g, int size, Offset value) {
	switch (size) {
		case 4: g->cb.cffWrite1(g->cb.ctx, value >> 24);

		case 3: g->cb.cffWrite1(g->cb.ctx, value >> 16);

		case 2: g->cb.cffWrite1(g->cb.ctx, value >> 8);

		case 1: g->cb.cffWrite1(g->cb.ctx, value);
	}
}

/* ----------------------------- Miscellaneous ----------------------------- */

/* strcmp function with one string specified by pointer and length */
int tc_strncmp(const char *s1, int length1, const char *s2) {
	while (length1--) {
		if (*s2 == '\0') {
			return 1;
		}
		else {
			int cmp = *s1++ - *s2++;
			if (cmp != 0) {
				return cmp;
			}
		}
	}
	return (*s2 != '\0') ? -1 : 0;
}

/* Clone null-terminated source string */
char *tc_dupstr(tcCtx g, char *src) {
	int length = strlen(src);
	char *dst = MEM_NEW(g, length + 1);
	strcpy(dst, src);
	return dst;
}

/* Clone source string of specified length */
char *tc_dupstrn(tcCtx g, char *src, int length) {
	char *dst = MEM_NEW(g, length + 1);
	memcpy(dst, src, length);
	dst[length] = '\0';
	return dst;
}

/* Gather compaction statistics */
int tcSetStats(tcCtx g, int gather) {
#if TC_STATISTICS
	g->stats.gather = gather;
	return 0;   /* Signal statistics enabled */
#else
	return 1;   /* Signal statistics disabled */
#endif /* TC_STATISTICS */
}

#if TC_DEBUG
/* -------------------------------- Debug ---------------------------------- */
static void dblayout(tcprivCtx h) {
	int i;

	printf("               size   offset\n");
	printf("FontNames   %7ld        -\n", h->size.FontNames);
	printf("dicts       %7ld        -\n", h->size.dicts);
	printf("nameINDEX   %7ld        -\n", h->size.nameINDEX);
	printf("dictINDEX   %7ld        -\n", h->size.dictINDEX);
	printf("strings     %7ld        -\n", h->size.strings);
	printf("gsubrs      %7ld        -\n", h->size.gsubrs);
	printf("FontNames   %7ld        -\n", h->size.FontNames);
	printf("encodings   %7ld  %7ld\n", h->size.encodings, h->offset.encodings);
	printf("charsets    %7ld  %7ld\n", h->size.charsets, h->offset.charsets);
	printf("FDSelects   %7ld  %7ld\n", h->size.FDSelects, h->offset.FDSelects);
	printf("copyright         -  %7ld\n", h->offset.copyright);

	for (i = 0; i < h->set.cnt; i++) {
		Font *font = &h->set.array[i];

		printf("=== font[%3d] ==============\n", i);
		printf("FontName    %7ld        -\n", font->size.FontName);
		printf("dict        %7ld        -\n", font->size.dict);
		printf("encoding          -  %7ld\n", font->offset.encoding);
		printf("charset           -  %7ld\n", font->offset.charset);
		printf("fdselect          -  %7ld\n", font->offset.fdselect);
		printf("CharStrings %7ld  %7ld\n",
		       font->size.CharStrings, font->offset.CharStrings);
		printf("FDArray     %7ld  %7ld\n",
		       font->size.FDArray, font->offset.FDArray);
		printf("Private     %7ld  %7ld\n",
		       font->size.Private, font->offset.Private);
		printf("Subrs       %7ld  %7ld\n",
		       font->size.Subrs, font->offset.Subrs);
		if (font->flags & FONT_CID) {
			int j;
			int iFD = 0;
			for (j = 0; j < font->fdCount; j++) {
				FDInfo *info = &font->fdInfo[j];
				if (info->seenChar) {
					printf("--- FD[%2d] -----------------\n", iFD++);
					printf("FD          %7ld        -\n", info->size.FD);
					printf("Private     %7ld  %7ld\n",
					       info->size.Private, info->offset.Private);
					printf("Subrs       %7ld  %7ld\n",
					       info->size.Subrs, info->offset.Subrs);
				}
			}
		}
	}
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dblayout);
}

#endif /* TC_DEBUG */