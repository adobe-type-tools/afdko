/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Adobe Font Metrics (AFM) file support.
 */

#include "absfont.h"

#include <time.h>
#include <string.h>

/* Print key name and string if set. */
static void writeString(abfAFMCtx h, char *key, abfString *value)
	{
	if (value->ptr == ABF_UNSET_PTR)
		return;
	fprintf(h->fp, "%s %s\n", key, value->ptr);
	}

void abfAFMBegFont(abfAFMCtx h, abfTopDict *top)
	{
	time_t now = time(NULL);

	/* Initialize context */
	h->err_code = abfSuccess;
	h->metrics.cb = abfGlyphMetricsCallbacks;
	h->metrics.cb.direct_ctx = &h->metrics.ctx;
	h->metrics.ctx.flags = 0;

	if (top->sup.flags & ABF_CID_FONT)
		fprintf(h->fp, "StartFontMetrics 4.1\n");
	else
		fprintf(h->fp, "StartFontMetrics 2.0\n");

	fprintf(h->fp, "Comment Copyright %d Adobe Systems Incorporated. "
			"All Rights Reserved.\n", localtime(&now)->tm_year + 1900);
	fprintf(h->fp, "Comment Creation Date: %s", ctime(&now));
	if (top->UniqueID != ABF_UNSET_INT)
		fprintf(h->fp, "Comment UniqueID %ld\n", top->UniqueID);
	if (top->sup.UnitsPerEm != 1000)
		fprintf(h->fp, "Comment UnitsPerEm %ld\n", top->sup.UnitsPerEm);

	if (top->sup.flags & ABF_CID_FONT)
		{
		/* Write header for cid-keyed font */
		fprintf(h->fp, "MetricsSets 2\n");
		writeString(h, "FontName", &top->cid.CIDFontName);
		writeString(h, "Weight", &top->Weight);
		fprintf(h->fp, "FontBBox %g %g %g %g\n",
				top->FontBBox[0], top->FontBBox[1], 
				top->FontBBox[2], top->FontBBox[3]);
		fprintf(h->fp, "Version %.3f\n", top->cid.CIDFontVersion);
		writeString(h, "Notice", &top->Notice);
		fprintf(h->fp, "CharacterSet %s-%s-%ld\n", top->cid.Registry.ptr, 
				top->cid.Ordering.ptr, top->cid.Supplement);
		fprintf(h->fp, "Characters %ld\n", top->sup.nGlyphs);
		fprintf(h->fp, "IsBaseFont true\n");
		fprintf(h->fp, "IsCIDFont true\n");
		fprintf(h->fp, "StartDirection 2\n");
		fprintf(h->fp, "UnderlinePosition %g\n", top->UnderlinePosition);
		fprintf(h->fp, "UnderlineThickness %g\n", top->UnderlineThickness);
		fprintf(h->fp, "ItalicAngle %g\n", top->ItalicAngle);
		fprintf(h->fp, "IsFixedPitch %s\n", 
				top->isFixedPitch? "true": "false");
		fprintf(h->fp, "EndDirection\n");
		fprintf(h->fp, "StartCharMetrics %ld\n", top->sup.nGlyphs);
		}
	else
		{
		/* Write header for name-keyed font */
		writeString(h, "FontName", &top->FDArray.array[0].FontName);
		writeString(h, "FullName", &top->FullName);
		writeString(h, "FamilyName", &top->FamilyName);
		writeString(h, "Weight", &top->Weight);
		fprintf(h->fp, "ItalicAngle %g\n", top->ItalicAngle);
		fprintf(h->fp, "IsFixedPitch %s\n", 
				top->isFixedPitch? "true": "false");
		fprintf(h->fp, "FontBBox %g %g %g %g\n",
				top->FontBBox[0], top->FontBBox[1], 
				top->FontBBox[2], top->FontBBox[3]);
		fprintf(h->fp, "UnderlinePosition %g\n", top->UnderlinePosition);
		fprintf(h->fp, "UnderlineThickness %g\n", top->UnderlineThickness);
		writeString(h, "Version", &top->version);
		writeString(h, "Notice", &top->Notice);
		fprintf(h->fp, "StartCharMetrics %ld\n", top->sup.nGlyphs - 1L);
		}
	}

void abfAFMEndFont(abfAFMCtx h)
	{
	fprintf(h->fp, "EndCharMetrics\n");
	fprintf(h->fp, "EndFontMetrics\n");
	}

/* Begin glyph path. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	cb->info = info;
	return h->metrics.cb.beg(&h->metrics.cb, info);
	}

/* Save glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	h->metrics.cb.width(&h->metrics.cb, hAdv);
	}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	h->metrics.cb.move(&h->metrics.cb, x0, y0);
	}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	h->metrics.cb.line(&h->metrics.cb, x1, y1);
	}

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	h->metrics.cb.curve(&h->metrics.cb, x1, y1, x2, y2, x3, y3);
	}

/* Ignore stem operator. */
static void glyphStem(abfGlyphCallbacks *cb,
					  int flags, float edge0, float edge1)
	{
	/* Nothing to do */
	}

/* Convert flex operator. */
static void glyphFlex(abfGlyphCallbacks *cb, float depth,
					  float x1, float y1, 
					  float x2, float y2, 
					  float x3, float y3,
					  float x4, float y4, 
					  float x5, float y5, 
					  float x6, float y6)
	{
	glyphCurve(cb, x1, y1, x2, y2, x3, y3);
	glyphCurve(cb, x4, y4, x5, y5, x6, y6);
	}

/* Ignore general glyph operator. */
static void glyphGenop(abfGlyphCallbacks *cb, 
					   int cnt, float *args, int op)
	{
	/* Nothing to do */
	}

/* Handle seac operator. */
static void glyphSeac(abfGlyphCallbacks *cb, 
					  float adx, float ady, int bchar, int achar)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;
	h->err_code = abfErrGlyphSeac; 
	}

/* End glyph path. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	abfAFMCtx h = (abfAFMCtx)cb->direct_ctx;
	abfMetricsCtx g = &h->metrics.ctx;
	abfGlyphInfo *info = cb->info;
    unsigned long code = (info->encoding.code == ABF_GLYPH_UNENC)?
		(unsigned long)-1: (unsigned long)info->encoding.code;
	h->metrics.cb.end(&h->metrics.cb);

	/* Print glyph metrics */
	if (info->flags & ABF_GLYPH_CID)
		fprintf(h->fp, "C %ld ; W0X %ld ; N %hu ; B %ld %ld %ld %ld ;\n", 
				code, g->int_mtx.hAdv, info->cid,
				g->int_mtx.left, g->int_mtx.bottom, 
				g->int_mtx.right, g->int_mtx.top);
	else if (strcmp(info->gname.ptr, ".notdef") != 0)
		fprintf(h->fp, "C %ld ; WX %ld ; N %s ; B %ld %ld %ld %ld ;\n",
				code, g->int_mtx.hAdv, info->gname.ptr,
				g->int_mtx.left, g->int_mtx.bottom, 
				g->int_mtx.right, g->int_mtx.top);
	}

/* AFM callbacks template. */
const abfGlyphCallbacks abfGlyphAFMCallbacks =
	{
	NULL,
	NULL,
	NULL,
	glyphBeg,
	glyphWidth,
	glyphMove,
	glyphLine,
	glyphCurve,
	glyphStem,
	glyphFlex,
	glyphGenop,
	glyphSeac,
	glyphEnd,
	};

