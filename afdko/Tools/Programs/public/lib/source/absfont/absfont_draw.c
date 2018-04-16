/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Abstract font dump.
 */

#include "absfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))
#define PTS_TO_GLYPH(v)	((v)/h->glyph.scale)

#define TILE_SIZE		35
#define TILE_COLS		16
#define TILE_ROWS		20
#define TILE_TOP		(TILE_ROWS*TILE_SIZE)
#define TILE_RIGHT		(TILE_COLS*TILE_SIZE)
#define TILE_TEXT_FONT	"Helvetica-Narrow"
#define TILE_TEXT_SIZE	5.0f
#define TILE_TEXT_BASE	(TILE_TEXT_SIZE*0.7f)
#define TILE_GLYPH_SIZE	24.0f
#define TILE_GLYPH_BASE	(TILE_GLYPH_SIZE*0.7f)

#define PAGE_RIGHT		TILE_RIGHT
#define PAGE_TOP		747
#define PAGE_TEXT_FONT	"Helvetica-Narrow"
#define PAGE_TEXT_SIZE	10.0f
#define	PAGE_TEXT_BASE	(PAGE_TEXT_SIZE*0.7f)

#define LABEL_TEXT_SIZE	4.0f
#define LABEL_TEXT_FONT	"Courier"
#define LABEL_TEXT_BASE	(LABEL_TEXT_SIZE*0.7f)

/* One-glyph-per-page origin */
#define EM_ORIG_H		72.0
#define EM_ORIG_V		252.0

/* Map value to em coordinate space */
#define TO_EM(v)	((float)(v)/h->glyph.scale)

/* Express value in ems */
#define IN_EM(v)	((float)(v)*h->top->sup.UnitsPerEm)

/* Flags bits */
#define NEW_PAGE	(1UL<<31)

typedef struct
	{
	float x;
	float y;
	} Vector;

/* Draw and label tile. */
static void drawTile(abfDrawCtx h, float x, float y,
					 char *tag, char *hAdv, char *gname)
	{
	/* Draw tile box */
	fprintf(h->fp,
			"newpath\n"
			"%g %g moveto %d 0 rlineto 0 %d rlineto %d 0 rlineto\n"
			"closepath 0 setlinewidth stroke\n",
			x, y, TILE_SIZE, -TILE_SIZE, -TILE_SIZE);

	/* Draw tag and gname labels */
	fprintf(h->fp,
			"%g %g moveto (%s) show\n"
			"%g %g moveto (%s) show\n",
			x + 1, y - 1 - TILE_TEXT_BASE, tag,
			x + 1, y - TILE_SIZE + TILE_TEXT_SIZE - TILE_TEXT_BASE, gname);

	/* Draw hAdv label */
	fprintf(h->fp,
			"%g (%s) stringwidth pop sub %g moveto (%s) show\n",
			x + TILE_SIZE - 1, hAdv, y - 1 - TILE_TEXT_BASE, hAdv);
	}

/* Begin page. */
static void pageBeg(abfDrawCtx h, abfGlyphInfo *info)
	{
	char datestr[20];
	char timestr[20];
	time_t now;
	struct tm *tm;
	float y;
	char *fontname;

	if (h->top->sup.flags & ABF_CID_FONT)
		fontname = h->top->cid.CIDFontName.ptr;
	else
		fontname = h->top->FDArray.array[0].FontName.ptr;
	if (fontname == ABF_UNSET_PTR)
		fontname = "<unknown>";

	/* Make formatted date and time string */
	now = time(NULL);
	tm = localtime(&now);
	strftime(datestr, sizeof(datestr), "Date: %m/%d/%y", tm);
	strftime(timestr, sizeof(timestr), "Time: %H:%M", tm);

	fprintf(h->fp, 
			"%% page: %d\n"
			"18 18 translate\n"
			"/%s findfont %g scalefont setfont\n",
			h->pageno,
			PAGE_TEXT_FONT, PAGE_TEXT_SIZE);

	y = (int)(PAGE_TOP - PAGE_TEXT_BASE + 0.5);
	fprintf(h->fp, 
			"0 %g moveto (Filename:  ",
			y);
	if (h->top->sup.filename == ABF_UNSET_PTR)
		fprintf(h->fp, "<unknown>");
	else
		{
		char *p = h->top->sup.filename;
		while (*p != '\0')
			{
			int c = *p++;
			fputc(c, h->fp);
			if (c == '\\')
				fputc(c, h->fp);
			}
		}
	fprintf(h->fp, 
			") show\n"
			"%d (%s) stringwidth pop sub %g moveto (%s) show\n",
			PAGE_RIGHT, datestr, y, datestr);

	y -= (PAGE_TEXT_SIZE + 1);
	fprintf(h->fp, 
			"0 %g moveto (FontName:  %s) show\n",
			y, fontname);
	fprintf(h->fp,
			"%d (%s) stringwidth pop sub %g moveto (%s) show\n",
			PAGE_RIGHT, timestr, y, timestr);

	y -= (PAGE_TEXT_SIZE + 1);
	fprintf(h->fp,
			"0 %g moveto (Em:  %ld units) show\n",
			y, h->top->sup.UnitsPerEm);
	fprintf(h->fp, 
			"%d (Page: %d) stringwidth pop sub %g moveto (Page: %d) show\n",
			PAGE_RIGHT, h->pageno, y, h->pageno);
			
	if (h->level == 0)
		{
		fprintf(h->fp, 
				"/%s findfont %g scalefont setfont\n",
				TILE_TEXT_FONT, TILE_TEXT_SIZE);
		if (info->flags & ABF_CID_FONT)
			drawTile(h, (TILE_COLS - 3)*TILE_SIZE, PAGE_TOP, 
					 "tag,fd", "hAdv", "cid");
		else
			drawTile(h, (TILE_COLS - 3)*TILE_SIZE, PAGE_TOP, 
					 "tag,enc", "hAdv", "gname");

		if (h->flags & ABF_SHOW_BY_ENC)
			{
			/* Draw encoding grid */
			int i;

			fprintf(h->fp, "gsave\n");

			/* Draw horizontal grid lines */
			for (i = 0; i < 17; i++)
				{
				int v = TILE_TOP - i*TILE_SIZE;
				fprintf(h->fp, 
						"%d %d moveto\n"
						"%d %d lineto\n",
						0, v,
						TILE_RIGHT, v);
				}
		
			/* Draw vertical grid lines */
			for (i = 0; i < 17; i++)
				fprintf(h->fp, 
						"%d %d moveto\n"
						"%d %d lineto\n",
						i*TILE_SIZE, TILE_TOP, 
						i*TILE_SIZE, TILE_TOP - 16*TILE_SIZE);

			fprintf(h->fp,
					"0 setlinewidth\n"
					".4 setgray\n"
					"stroke\n"
					"grestore\n");
			}
		}
	else
		fprintf(h->fp, 
				"/%s findfont %g scalefont setfont\n",
				LABEL_TEXT_FONT, TO_EM(LABEL_TEXT_SIZE));

	h->flags &= ~NEW_PAGE;
	}

/* End page. */
static void pageEnd(abfDrawCtx h)
	{
	fprintf(h->fp, "showpage\n");
	h->pageno++;
	h->flags |= NEW_PAGE;
	}

/* Begin drawing new font. */
void abfDrawBegFont(abfDrawCtx h, abfTopDict *top)
	{
	if (h->level > 1 || h->level < 0)
		h->level = 0;	/* Set invalid level to tile mode */

	h->flags |= NEW_PAGE;

	fprintf(h->fp, "%%!\n");
	if (h->flags & ABF_DUPLEX_PRINT)
		fprintf(h->fp, 
				"mark\n"
				"{2 dict dup /Duplex true put\n"
				"dup /Tumble false put setpagedevice} stopped\n"
				"cleartomark\n");

	if (h->level == 0)
		{
		h->glyph.scale = TILE_GLYPH_SIZE / top->sup.UnitsPerEm;
		h->tile.h = 0;
		h->tile.v = (int)TILE_TOP;
		}
	else
		{
		h->glyph.scale = 500.0f/top->sup.UnitsPerEm;
		fprintf(h->fp,
				"/cntlpt{gsave newpath %g 0 360 arc fill grestore}bind def\n"
				"/arrow{newpath\n"
				"0 0 moveto %g %g rlineto 0 %g rlineto closepath fill\n"
				"}bind def\n"
				"/closept{newpath 0 0 %g 0 360 arc fill}bind def\n",
				TO_EM(0.5),
				TO_EM(-6), TO_EM(-1.5), TO_EM(3),
				TO_EM(1));
		h->metrics.cb = abfGlyphMetricsCallbacks;
		h->metrics.cb.direct_ctx = &h->metrics.ctx;
		h->metrics.ctx.flags = 0;
		}

	h->pageno = 1;
	h->top = top;
	}

/* End drawing new font. */
void abfDrawEndFont(abfDrawCtx h)
	{
	if (h->level != 0)
		return;

	if (h->flags & ABF_SHOW_BY_ENC || 
		(h->tile.h != 0 || h->tile.v != TILE_TOP))
		pageEnd(h);	/* Show last page */
	}

/* ------------------------------- Glyph Draw ------------------------------ */

/* Draw glyph beginning. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;

	cb->info = info;
	if (h->flags & NEW_PAGE)
		pageBeg(h, info);

	if (h->level == 0 && h->flags & ABF_SHOW_BY_ENC)
		{
		unsigned long code = info->encoding.code;
		if (code > 255)
			h->showglyph = 0;
		else
			{
			h->tile.h = (code&0xf)*TILE_SIZE;
			h->tile.v = TILE_TOP - (code>>4&0xf)*TILE_SIZE;
			h->showglyph = 1;
			}
		}
	else
		h->showglyph = 1;

	if (h->showglyph)
		{
		if (info->flags & ABF_GLYPH_CID)
			fprintf(h->fp, "%% glyph: \\%hu\n", info->cid);
		else
			fprintf(h->fp, "%% glyph: %s\n", info->gname.ptr);

		h->path.moves = 0;
		h->path.lines = 0;
		h->path.curves = 0;

		if (h->level != 0)
			h->metrics.cb.beg(&h->metrics.cb, info);
		}

	return ABF_CONT_RET;
	}

/* Draw glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;

	if (!h->showglyph)
		return;

	if (h->level != 0)
		{
		/* Draw ruler */
		int i;

		fprintf(h->fp,
				"gsave\n"
				"%g %g scale\n"
				"0 0 moveto\n"
				"100 0 lineto\n",
				h->glyph.scale, h->glyph.scale);

		for (i = 0; i <= 100; i += 10)
			fprintf(h->fp, 
					"%d 0 moveto\n"
					"%d 10 lineto\n",
					i, i);
		for (i = 5; i < 100; i += 10)
			fprintf(h->fp, 
					"%d 0 moveto\n"
					"%d 7 lineto\n",
					i, i);

		fprintf(h->fp, 
				"0 setlinewidth stroke\n"
				"110 0 moveto (100 units) show\n"
				"grestore\n");
		}

	/* Prepare em space */
	fprintf(h->fp, "gsave\n");
	if (h->level == 0)
		fprintf(h->fp,
				"%g %g translate\n"
				"%g %g scale\n",
				h->tile.h + 
				(TILE_SIZE - TILE_GLYPH_SIZE*hAdv/h->top->sup.UnitsPerEm)/2, 
				h->tile.v - TILE_SIZE*0.7,
				h->glyph.scale, h->glyph.scale);
	else
		fprintf(h->fp, 
				"%g %g translate\n"
				"%g %g scale\n", 
				EM_ORIG_H, EM_ORIG_V,
				h->glyph.scale, h->glyph.scale);

	/* Draw origin */
	fprintf(h->fp,
			"%% origin tic\n"
			"%g 0 moveto\n"
			"%g 0 rlineto\n"
			"0 %g rlineto\n",
			IN_EM(-0.03), IN_EM(0.03), IN_EM(-0.03));

	/* Draw width */
	fprintf(h->fp,
			"%% width tic\n"
			"%g 0 moveto\n"
			"%g 0 rlineto\n"
			"0 %g rlineto\n",
			hAdv + IN_EM(0.03), IN_EM(-0.03), IN_EM(-0.03));

	fprintf(h->fp, "0 setlinewidth stroke\n");
	if (h->level != 0)
		h->metrics.cb.width(&h->metrics.cb, hAdv);

	h->glyph.hAdv = hAdv;
	}

/* Make normalized vector. */
static Vector makeVec(float x, float y)
	{
	Vector v;
	float d = (float)sqrt(x*x + y*y);
	if (d == 0)
		{
		v.x = 1;	
		v.y = 1;
		}
	else
		{
		v.x = x/d;
		v.y = y/d;
		}
	return v;
	}

/* Save label point. */
static void savePt(abfDrawCtx h, float x, float y)
	{
	if (h->path.cnt == 1)
		{
		h->path.sx = x;
		h->path.sy = y;
		}
	h->path.cx = h->path.bx;
	h->path.cy = h->path.by;
	h->path.bx = x;
	h->path.by = y;
	h->path.cnt++;
	}

/* Draw numeric label on last point. */
static void drawLabel(abfDrawCtx h, float ax, float ay)
	{
	if (h->level == 0 || (h->flags & ABF_NO_LABELS))
		return;	/* Tile mode */

	if (h->path.cnt >= 2)
		{
		/* Make vector 90 degress counter clockwise to a->c */
		Vector s = makeVec(h->path.bx - ax, h->path.by - ay);
		Vector t = makeVec(h->path.cx - h->path.bx, h->path.cy - h->path.by);
		Vector u = makeVec(s.x + t.x, s.y + t.y);
		Vector v;
		v.x = -u.y*TO_EM(5.5);
		v.y = u.x*TO_EM(5.5);

		if (h->flags & ABF_FLIP_TICS)
			{
			v.x = -v.x;
			v.y = -v.y;
			}

		fprintf(h->fp,
				"%% draw label\n"
				"gsave\n"
				"%g %g moveto\n"
				"%g %g rlineto\n",
				h->path.bx, h->path.by,
				v.x, v.y);

		/* Adjust position for quadrant */
		if (v.x < 0 && v.y >= 0)
			fprintf(h->fp, "(%.0f %.0f) stringwidth pop neg 0 rmoveto\n",
					h->path.bx, h->path.by);
		else if (v.x <= 0 && v.y < 0)
			fprintf(h->fp, "(%.0f %.0f) stringwidth pop neg %g rmoveto\n",
					h->path.bx, h->path.by, TO_EM(-LABEL_TEXT_BASE));
		else if (v.x > 0 && v.y <= 0)
			fprintf(h->fp, "0 %g rmoveto\n", TO_EM(-LABEL_TEXT_BASE));

		fprintf(h->fp,
				"(%.0f %.0f) show\n"
				"0 setlinewidth stroke\n"
				"grestore\n",
				h->path.bx, h->path.by);
		}
	savePt(h, ax, ay);
	}

/* Draw closepath. */
static void drawClose(abfDrawCtx h, int closepoint)
	{
	float dx;
	float dy;
	float l;

	if (closepoint)
		{
		dx = h->path.fx - h->path.cx;
		dy = h->path.fy - h->path.cy;
		}
	else
		{
		dx = h->path.fx - h->path.bx;
		dy = h->path.fy - h->path.by;
		}
	l = (float)sqrt(dx*dx + dy*dy);

	fprintf(h->fp,
			"%% draw close\n"
			"gsave\n");
	if (l == 0)
		fprintf(h->fp, 
				"[1 0 0 1 %g %g] concat\n", 
				h->path.fx, h->path.fy);
	else
		fprintf(h->fp,
				"[%g %g %g %g %g %g] concat\n",
				dx/l, dy/l, -dy/l, dx/l, h->path.fx, h->path.fy);
	fprintf(h->fp, "arrow\n");

	if (closepoint)
		fprintf(h->fp, "closept\n");
	fprintf(h->fp, "grestore\n");
	}

/* End current path. */
static void endPath(abfDrawCtx h)
	{
	fprintf(h->fp, "closepath\n");

	if (h->level == 0 || (h->flags & ABF_NO_LABELS) || h->path.moves == 0)
		return;

	if (h->path.bx == h->path.fx && h->path.by == h->path.fy)
		drawClose(h, 1);
	else
		{
		drawClose(h, 0);
		drawLabel(h, h->path.fx, h->path.fy);
		h->path.lines++;
		}
	drawLabel(h, h->path.sx, h->path.sy);
	}

/* Draw glyph move. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;

	if (!h->showglyph)
		return;

	if (h->path.moves > 0)
		endPath(h);
	else
		fprintf(h->fp, "%% path\n");
	fprintf(h->fp, "%g %g moveto\n", x0, y0);

	h->path.cnt = 1;
	if (h->level != 0)
		{
		/* Initialize point accumulator */
		h->path.bx = h->path.fx = x0;
		h->path.by = h->path.fy = y0;
		h->metrics.cb.move(&h->metrics.cb, x0, y0);
		}

	h->path.moves++;
	}

/* Draw control point. */
static void drawCntlPt(abfDrawCtx h, float x, float y)
	{
	if (h->level == 0 || (h->flags & ABF_NO_LABELS))
		return;
	fprintf(h->fp, "%g %g cntlpt\n", x, y);
	}

/* Draw glyph line. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;

	if (!h->showglyph)
		return;

	fprintf(h->fp, "%g %g lineto\n", x1, y1);	
	drawLabel(h, x1, y1);

	if (h->level != 0)
		h->metrics.cb.line(&h->metrics.cb, x1, y1);

	h->path.lines++;
	}

/* Draw glyph curve. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;

	if (!h->showglyph)
		return;

	fprintf(h->fp, "%g %g %g %g %g %g curveto\n", x1, y1, x2, y2, x3, y3);

	drawCntlPt(h, x1, y1);
	drawCntlPt(h, x2, y2);

	drawLabel(h, x1, y1);
	savePt(h, x2, y2);
	savePt(h, x3, y3);

	if (h->level != 0)
		h->metrics.cb.curve(&h->metrics.cb, x1, y1, x2, y2, x3, y3);

	h->path.curves++;
	}

/* Draw glyph stem. */
static void glyphStem(abfGlyphCallbacks *cb,
					  int flags, float edge0, float edge1)
	{
	/* xxx want to optionally draw hints */
	}

/* Draw glyph flex. */
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
	/* xxx want to optionally draw flex */
	}

/* Draw glyph general operator. */
static void glyphGenop(abfGlyphCallbacks *cb, 
					   int cnt, float *args, int op)
	{
	}

/* Draw glyph seac. */
static void glyphSeac(abfGlyphCallbacks *cb, 
					  float adx, float ady, int bchar, int achar)
	{
	}

/* Draw numeric key/value. */
static float drawNumber(abfDrawCtx h, float y, char *key, float value)
	{
	fprintf(h->fp, "%d %g moveto (%s) show\n", PAGE_RIGHT - (54 + 30), y, key);
	fprintf(h->fp, "%d %g moveto (%.0f) show\n", PAGE_RIGHT - 54, y, value);
	return y + PAGE_TEXT_SIZE + 1;
	}

/* Draw string key/value. */
static float drawString(abfDrawCtx h, float y, char *key, char *value)
	{
	fprintf(h->fp, "%d %g moveto (%s) show\n", PAGE_RIGHT - (54 + 30), y, key);
	fprintf(h->fp, "%d %g moveto (%s) show\n", PAGE_RIGHT - 54, y, value);
	return y + PAGE_TEXT_SIZE + 1;
	}

/* Draw metrics. */
static void drawMetrics(abfDrawCtx h, abfGlyphInfo *info)
	{
	char buf[50];
    const size_t bufLen = sizeof(buf);
	float y;

	fprintf(h->fp, "/%s findfont %g scalefont setfont\n",
			PAGE_TEXT_FONT, PAGE_TEXT_SIZE);

	y = (int)(PAGE_TEXT_SIZE - PAGE_TEXT_BASE + 0.5);
	y = drawNumber(h, y, "total", 
			   (float)h->path.moves + h->path.lines + h->path.curves);
	y = drawNumber(h, y, "curves", 	(float)h->path.curves);
	y = drawNumber(h, y, "lines", 	(float)h->path.lines);
	y = drawNumber(h, y, "moves",	(float)h->path.moves);
	
	y += PAGE_TEXT_SIZE/2;
	y = drawNumber(h, y, "top", 	(float)h->metrics.ctx.int_mtx.top);
	y = drawNumber(h, y, "right", 	(float)h->metrics.ctx.int_mtx.right);
	y = drawNumber(h, y, "bottom", 	(float)h->metrics.ctx.int_mtx.bottom);
	y = drawNumber(h, y, "left", 	(float)h->metrics.ctx.int_mtx.left);

	y += PAGE_TEXT_SIZE/2;
	y = drawNumber(h, y, "hAdv", 	h->glyph.hAdv);
	if (info->flags & ABF_GLYPH_CID)
		y = drawNumber(h, y, "fd", info->iFD);
	else
		{
		abfEncoding *enc = &info->encoding;
		if (enc->code == ABF_GLYPH_UNENC)
			SPRINTF_S(buf, bufLen, "-");
		else
			{
			int cnt = 0;
			char *p = buf;
            size_t remainingBufferLength = bufLen;
			char *sep = "";
                size_t printStringLength;
			do
				{
				if (info->flags & ABF_GLYPH_UNICODE)
					{
					if (enc->code < 0x10000)
						SPRINTF_S(p, remainingBufferLength, "%sU+%04lX", sep, enc->code);
					else
						SPRINTF_S(p, remainingBufferLength, "%sU+%lX", sep, enc->code);
					}
				else
					SPRINTF_S(p, remainingBufferLength, "%s0x%02lx", sep, enc->code);
                printStringLength = strnlen(p, remainingBufferLength);
				p += printStringLength;
                remainingBufferLength -= printStringLength;
				sep = "+";
				enc = enc->next;
				}
			while (enc != NULL && ++cnt < 2);
			if (enc != NULL)
				SPRINTF_S(p, remainingBufferLength, "...");
			}
		y = drawString(h, y, "enc", buf);
		}
	y = drawNumber(h, y, "tag", info->tag);
	if (info->flags & ABF_GLYPH_CID)
		SPRINTF_S(buf, bufLen, "\\\\%hu", info->cid);
	else
		SPRINTF_S(buf, bufLen, "%s", info->gname.ptr);
	y =	drawString(h, y, "glyph", buf);
	}

/* Draw glyph end. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	abfDrawCtx h = (abfDrawCtx)cb->direct_ctx;
	char gname[50];
    const size_t gnameLen = sizeof(gname);
	abfGlyphInfo *info = cb->info;

	if (!h->showglyph)
		return;

	endPath(h);

	if (h->level == 0)
		FPRINTF_S(h->fp, "fill\n");
	else 
		fprintf(h->fp, "0 setlinewidth stroke\n");
	FPRINTF_S(h->fp, "grestore\n");

	/* Set glyph name */
	if (info->flags & ABF_GLYPH_CID)
		SPRINTF_S(gname, gnameLen, "\\\\%hu", info->cid);
	else
		SPRINTF_S(gname, gnameLen, "%s", info->gname.ptr);

	if (h->level == 0)
		{
		char tag[20];
        const size_t tagLen = sizeof(tag);
		char hAdv[20];
        const size_t hAdvLen = sizeof(hAdv);

		if (info->flags & ABF_GLYPH_CID)
			SPRINTF_S(tag, tagLen, "%hu,%u", info->tag, info->iFD);
		else
			{
			abfEncoding *enc = &info->encoding;
			SPRINTF_S(tag, tagLen, "%hu,", info->tag);
			if (enc->code != ABF_GLYPH_UNENC)
				{
				int cnt = 0;
				char *p = tag;
                size_t remainingTagLength = tagLen;
				char *sep = "";
				do
					{
                    size_t printStringLength = strnlen(p, remainingTagLength);
					p += printStringLength;
                    remainingTagLength -= printStringLength;
					if (info->flags & ABF_GLYPH_UNICODE)
						{
						if (enc->code < 0x10000)
							SPRINTF_S(p, remainingTagLength, "%sU+%04lX", sep, enc->code);
						else
							SPRINTF_S(p, remainingTagLength, "%sU+%lX", sep, enc->code);
						}
					else
						SPRINTF_S(p, remainingTagLength, "%s0x%02lX", sep, enc->code);
					sep = "+";
					enc = enc->next;
					}
				while (enc != NULL && ++cnt < 2);
				if (enc != NULL)
					{
                    size_t printStringLength = strnlen(p, remainingTagLength);
                    p += printStringLength;
                    remainingTagLength -= printStringLength;
					SPRINTF_S(p, remainingTagLength, "...");
					}
				}
			}
		SPRINTF_S(hAdv, hAdvLen, "%g", h->glyph.hAdv);
		drawTile(h, (float)h->tile.h, (float)h->tile.v, tag, hAdv, gname);

		if (!(h->flags & ABF_SHOW_BY_ENC))
			{
			h->tile.h += TILE_SIZE;
			if (h->tile.h == TILE_RIGHT)
				{
				/* Row full */
				h->tile.v -= TILE_SIZE;
				if (h->tile.v == 0)
					{
					/* Page full */
					pageEnd(h);
					h->tile.v = TILE_TOP;
					}
				h->tile.h = 0;
				}
			}
		}
	else
		{
		h->metrics.cb.end(&h->metrics.cb);
		drawMetrics(h, cb->info);
		pageEnd(h);
		}
	}

/* Draw glyph callbacks */
const abfGlyphCallbacks abfGlyphDrawCallbacks =
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
