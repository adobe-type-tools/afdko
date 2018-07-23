/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)glyf.c	1.33
 * Changed:    5/19/99 23:50:33
 ***********************************************************************/

#if OSX
	#include <sys/time.h>
#else
	#include <time.h>
#endif
#include <math.h>
#include <string.h>

#include "glyf.h"
#include "sfnt_glyf.h"

#include "maxp.h"
#include "loca.h"
#include "head.h"
#include "hmtx.h"
#include "vmtx.h"
#include "sfnt.h"
#include "proof.h"
#include "sys.h" /* sysOurtime() */
#include "TTdumpinstrs.h"

extern time_t time(time_t *tptr);
extern size_t strftime(char *s, size_t maxsize, const char *format,
						   const struct tm *timeptr);

static void drawCompound(GlyphId glyphId, IntX marks, IntX fill);
static void proofCompound(GlyphId glyphId, ProofContextPtr ctx);
#define NUMERIC_LABEL_SIZE 5	/* Numeric label size (points) */

static glyfTbl *glyf = NULL;
static IntX loaded = 0;
static Card16 nGlyphs;
static Card16 maxComponents;
static Card16 unitsPerEm = 0;
static struct
	{
	Int16 xMin;
	Int16 yMin;
	Int16 xMax;
	Int16 yMax;
	} font;
IdList glyphs;

static ProofContextPtr proofctx;

/* Coordinate point */
typedef struct
	{
	Int16 on;
	Int16 x;
	Int16 y;
	} Point;

/* Target glyph bounding box */
TargetBBoxType target;

/* Scale factor */
ScaleType scale = {1.0, 1.0};

static struct
	{
	Int16 std;	/* Output in standard units */
	Int16 rnd;	/* Round mapped result */
	} cntl;

/* The following macros handle conversion between different coordinate spaces.
   STD designate the Adobe standard 1000 units/em space. FNT designates the
   font's built-in unit/em space from head.unitsPerEm. The outline drawing
   space is scaled so that 1 em = 500 pts and the filled drawing space is
   scaled so that 1 em = 5 cm. */
#define STD2FNT(a,c)	((c)*unitsPerEm/(1000.0*scale.a))
#define RND(c) 			(cntl.rnd?(Int32)((c)<0?(c)-0.5:(c)+0.5):(c))
#define FNT2STD(a,c)	(RND((c)*1000.0/unitsPerEm)*scale.a)
#define OUTPUT(a,c) 	(cntl.std?FNT2STD(a,c):(c)*scale.a)
#define PTS(a,c) 		(STD2FNT(a,c)*2.0)

/* Macros for tiled glyphs */
#define INCH(i)			((i)*72.0)
#define PAGE_WIDTH		INCH(8)
#define PAGE_HEIGHT		INCH(10.2)
#define TILE_WIDTH		36.0
#define TILE_HEIGHT		36.0
#define	TEXT_SIZE		5 	/* Tile text point size */
#define TEXT_BASE		((2 * TEXT_SIZE) / 3.0)
#define GLYPH_SIZE		24.0
#define GLYPH_BASE		((2 * GLYPH_SIZE) / 3.0)
#define MARGIN_WIDTH	((TILE_WIDTH - GLYPH_SIZE) / 2.0)
#define MARGIN_HEIGHT	((TILE_HEIGHT - GLYPH_SIZE) / 2.0)

/* Font synopsis */
static struct
	{
	Byte8 *title;	/* Page title */
	double hTile;	/* Horizontal tile origin (left) */
	double vTile;	/* Vertical tile origin (top) */
	Card16 page;	/* Page number */
	} synopsis;

static Byte8 *workstr = NULL;


/* Read simple glyph */
static Simple *readSimple(IntX nContours)
	{
	IntX i;
	IntX nPoints;
	Int16 coord;
	Simple *simple = memNew(sizeof(Simple));

	/* Read countour end point indices */
	simple->endPtsOfContours =
		memNew(sizeof(simple->endPtsOfContours[0]) * nContours);
	for (i = 0; i < nContours; i++)
		IN1(simple->endPtsOfContours[i]);

	/* Read instructions */
	IN1(simple->instructionLength);
	simple->instructions = memNew(simple->instructionLength);
	IN_BYTES(simple->instructionLength, simple->instructions);

	/* Read flags */
	nPoints = simple->endPtsOfContours[nContours - 1] + 1;
	simple->flags = memNew(sizeof(simple->flags[0]) * nPoints);
	for (i = 0; i < nPoints; i++)
		{
		Card8 flag;

		IN1(flag);
		simple->flags[i] = flag;

		if (flag & REPEAT)
			{
			Int8 cnt;
			
			IN1(cnt);
			while (cnt-- > 0)
				simple->flags[++i] = flag;
			}
		}

	/* Read x-coords */
	coord = 0;
	simple->xCoordinates = memNew(sizeof(simple->xCoordinates[0]) * nPoints);
	for (i = 0; i < nPoints; i++)
		{
		Card8 flag = simple->flags[i];

		if (flag & XSHORT)
			{
			Card8 C8Xvalue = 0;

			IN1(C8Xvalue);
			coord += (flag & SHORT_X_IS_POS) ? C8Xvalue : -C8Xvalue;
			}
		else if (!(flag & NEXT_X_IS_ZERO))
			{
			Int16 I16Xvalue = 0;

			IN1(I16Xvalue);
			coord += I16Xvalue;
			}
		simple->xCoordinates[i] = coord;
		}

	/* Read y-coords */
	coord = 0;
	simple->yCoordinates = memNew(sizeof(simple->yCoordinates[0]) * nPoints);
	for (i = 0; i < nPoints; i++)
		{
		Card8 flag = simple->flags[i];

		if (flag & YSHORT)
			{
			Card8 C8Yvalue = 0;

			IN1(C8Yvalue);
			coord += (flag & SHORT_Y_IS_POS) ? C8Yvalue : -C8Yvalue;
			}
		else if (!(flag & NEXT_Y_IS_ZERO))
			{
			Int16 I16Yvalue = 0;

			IN1(I16Yvalue);
			coord += I16Yvalue;
			}
		simple->yCoordinates[i] = coord;
		}	

	return simple;
	}

/* Read compound glyph */
static Compound *readCompound(void)
	{
	IntX i;
	Compound *compound = memNew(sizeof(Compound));
	Component *component;

	compound->component = memNew(sizeof(Component) * maxComponents);
	for (i = 0;; i++)
		{
		component = &compound->component[i];


		if (i >= maxComponents)
			{
			/* Exceeded maximum component count */
			memFree(compound->component);
			memFree(compound);
			return NULL;
			}
		/* init */
		component->instructionLength = 0;
		component->instructions = NULL;

		IN1(component->flags);
		IN1(component->glyphIndex);

		if (component->flags & ARG_1_AND_2_ARE_WORDS)
			{
			/* Short word arguments */
			IN1(component->arg1);
			IN1(component->arg2);
			}
		else
			{
			/* Byte arguments */
			Int8 byte1;
			Int8 byte2;

			IN1(byte1);
			IN1(byte2);

			if (component->flags & ARGS_ARE_XY_VALUES)
				{
				/* Signed offsets */
				component->arg1 = byte1;
				component->arg2 = byte2;
				}
			else
				{
				/* Unsigned anchor points */
				component->arg1 = (Card8)byte1;
				component->arg2 = (Card8)byte2;
				}
			}
			
		if (component->flags & WE_HAVE_A_SCALE)
			{
			IN1(component->transform[0][0]);
			component->transform[0][1] = DBL2F2Dot14(0.0);
			component->transform[1][0] = DBL2F2Dot14(0.0);
			component->transform[1][1] = component->transform[0][0];
			}
		else if (component->flags & WE_HAVE_AN_X_AND_Y_SCALE)
			{
			IN1(component->transform[0][0]);
			component->transform[0][1] = DBL2F2Dot14(0.0);
			component->transform[1][0] = DBL2F2Dot14(0.0);
			IN1(component->transform[1][1]);
			}
		else if (component->flags & WE_HAVE_A_TWO_BY_TWO)
			{
			IN1(component->transform[0][0]);
			IN1(component->transform[0][1]);
			IN1(component->transform[1][0]);
			IN1(component->transform[1][1]);
			}
		else 
			{
			component->transform[0][0] = DBL2F2Dot14(1.0);
			component->transform[0][1] = DBL2F2Dot14(0.0);
			component->transform[1][0] = DBL2F2Dot14(0.0);
			component->transform[1][1] = DBL2F2Dot14(1.0);
			}
	
		if (!(component->flags & MORE_COMPONENTS))
			break;
		}

	if (component->flags & WE_HAVE_INSTRUCTIONS)
		{
		/* Read instructions */
		IN1(component->instructionLength);
		component->instructions = memNew(component->instructionLength);
		IN_BYTES(component->instructionLength, component->instructions);
		}

	return compound;
	}

void glyfRead(LongN start, Card32 length)
	{
	IntX i;
	LongN offset;
	Card32 datalen;

	if (loaded)
		return;
	glyf = (glyfTbl *)memNew(sizeof(glyfTbl));

	/* Get data from other tables */
	if (maxpGetNGlyphs(&nGlyphs, glyf_) ||
		maxpGetMaxComponents(&maxComponents, glyf_) ||
		headGetUnitsPerEm(&unitsPerEm, glyf_) ||
		getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax) ||
		locaGetOffset(0, &offset, &datalen, glyf_))
		return;

	glyf->glyph = memNew(sizeof(Glyph) * nGlyphs);
	for (i = 0; i < nGlyphs; i++)
		{
		Glyph *glyph = &glyf->glyph[i];
		
		(void)locaGetOffset(i, &offset, &datalen, glyf_);
		if (offset > (LongN)(length - datalen)) /* offset can be equal to length if the last few glyphs are non-marking. */
			{
			warning(SPOT_MSG_glyfBADLOCA, i);
			glyph->numberOfContours = 0;
			continue;
			}
		if (datalen == 0)
			{
			glyph->numberOfContours = 0;
			glyph->xMin = glyph->yMin = glyph->xMax = glyph->yMax = 0;
			continue;
			}

		SEEK_ABS(start + offset);

		IN1(glyph->numberOfContours);
		IN1(glyph->xMin);
		IN1(glyph->yMin);
		IN1(glyph->xMax);
		IN1(glyph->yMax);

		if (glyph->numberOfContours > 0)
			glyph->data = readSimple(glyph->numberOfContours);
		else if (glyph->numberOfContours == -1)
			{
			glyph->data = readCompound();
			if (glyph->data == NULL)
				{
				glyph->numberOfContours = 0;
				warning(SPOT_MSG_glyfMAXCMP, i);
				}
			}
		else 
		  {
			warning(SPOT_MSG_glyfUNSCOMP, glyph->numberOfContours);
		  }
		}

	workstr = (Byte8 *)memNew(sizeof(Byte8) * 256);
	loaded = 1;
	}

/* Dump simple glyph */
static void dumpSimple(Simple *simple, IntX nContours, IntX level)
	{
	IntX i;
	IntX nPoints;

	/* Dump contour end point indices */
	DL(3, (OUTPUTBUFF, "--- endPtsOfContours[index]=value\n"));
	for (i = 0; i < nContours; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, simple->endPtsOfContours[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	/* Dump instructions */
	DLu(4, "instructionLength=", simple->instructionLength);
	/*DL(4, (OUTPUTBUFF, "--- instructions[index]=value\n"));*/
	DL(4, (OUTPUTBUFF, "--- instructions\n"));
	if(level==4)
		dumpInstrs(simple->instructionLength, (char *)(simple->instructions));
	/*for (i = 0; i < simple->instructionLength; i++)
		DL(4, (OUTPUTBUFF, "[%d]=%02hx ", i, simple->instructions[i]));*/
	DL(4, (OUTPUTBUFF, "\n"));
	   
	/* Dump flags */
	DL(3, (OUTPUTBUFF, "--- flags[index]=value\n"));
	nPoints = simple->endPtsOfContours[nContours - 1] + 1;
	for (i = 0; i < nPoints; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%02hx ", i, (Card16)simple->flags[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	/* Dump x-coords */
	DL(3, (OUTPUTBUFF, "--- xCoordinates[index]=value\n"));
	for (i = 0; i < nPoints; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hd ", i, simple->xCoordinates[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	/* Dump y-coords */
	DL(3, (OUTPUTBUFF, "--- yCoordinates[index]=value\n"));
	for (i = 0; i < nPoints; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hd ", i, simple->yCoordinates[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

/* Dump compound glyph */
static void dumpCompound(Compound *compound, IntX level)
	{
	IntX i;
	Component *component;

	for (i = 0;; i++)
		{
		component = &compound->component[i];

		DL(2, (OUTPUTBUFF, "--- component[%d]\n", i));
		
		DLx(2, "flags     =", component->flags);
		DLu(2, "glyphIndex=", component->glyphIndex);
		DLs(2, "arg1      =", component->arg1);
		DLs(2, "arg2      =", component->arg2);
		if (component->flags & WE_HAVE_A_SCALE)
			{
			DL(2, (OUTPUTBUFF, "--- WE_HAVE_A_SCALE\n"));
			DL(2, (OUTPUTBUFF, "scale=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[0][0]),
				   component->transform[0][0]));
			}
		else if (component->flags & WE_HAVE_AN_X_AND_Y_SCALE)
			{
			DL(2, (OUTPUTBUFF, "--- WE_HAVE_AN_X_AND_Y_SCALE\n"));
			DL(2, (OUTPUTBUFF, "xscale=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[0][0]),
				   component->transform[0][0]));
			DL(2, (OUTPUTBUFF, "yscale=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[1][1]),
				   component->transform[1][1]));
			}
		else if (component->flags & WE_HAVE_A_TWO_BY_TWO)
			{
			DL(2, (OUTPUTBUFF, "--- WE_HAVE_A_TWO_BY_TWO\n"));
			DL(2, (OUTPUTBUFF, "transform[0][0]=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[0][0]),
				   component->transform[0][0]));
			DL(2, (OUTPUTBUFF, "transform[0][1]=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[0][1]),
				   component->transform[0][1]));
			DL(2, (OUTPUTBUFF, "transform[1][0]=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[1][0]),
				   component->transform[1][0]));
			DL(2, (OUTPUTBUFF, "transform[1][1]=%1.3f (%04hx)\n", 
				   F2Dot142DBL(component->transform[1][1]),
				   component->transform[1][1]));
			}
			
		if (!(component->flags & MORE_COMPONENTS))
			break;
		}

	if (component->flags & WE_HAVE_INSTRUCTIONS)
		{
		/* Dump instructions */
		DLu(4, "instructionLength=", component->instructionLength);
		DL(4, (OUTPUTBUFF, "--- instructions\n"));
		if(level==4)
			dumpInstrs(component->instructionLength, (char *)(component->instructions));
		DL(4, (OUTPUTBUFF, "\n"));
		}
	}

static void dumpGlyph(GlyphId glyphId, IntX level)
	{
	Glyph *glyph = &glyf->glyph[glyphId];

	DL(2, (OUTPUTBUFF, "--- glyph[%d]\n", glyphId));
	DLs(2, "numberOfContours=", glyph->numberOfContours);
	DLs(2, "xMin            =", glyph->xMin);
	DLs(2, "yMin            =", glyph->yMin);
	DLs(2, "xMax            =", glyph->xMax);
	DLs(2, "yMax            =", glyph->yMax);

	if (glyph->numberOfContours > 0)
		dumpSimple(glyph->data, glyph->numberOfContours, level);
	else if (glyph->numberOfContours == -1)
		dumpCompound(glyph->data, level);
	}

/* Dump simple glyph points */
static void dumpSimplePoints(GlyphId glyphId)
	{
	IntX i;
	IntX j;
	Glyph *glyph = &glyf->glyph[glyphId];
	Simple *simple;

	if (!glyph) return;
	simple = glyph->data;
	if (!simple) return;

	fprintf(OUTPUTBUFF,  "--- glyph[%hu]\n", glyphId);
	j = 0;
	for (i = 0; i < glyph->numberOfContours; i++)
		{
		IntX iStart = j;
		IntX iEnd = simple->endPtsOfContours[i];

		fprintf(OUTPUTBUFF,  "--- contour[%d]\n", i);
		fprintf(OUTPUTBUFF,  "--- point[index]={off/on,x,y}\n");
		for (; j <= iEnd; j++)
			fprintf(OUTPUTBUFF,  "[%d]={%s,%hd,%hd} ", j - iStart, 
				   (simple->flags[j] & ONCURVE) ? "on" : "off",
				   simple->xCoordinates[j], simple->yCoordinates[j]);
		fprintf(OUTPUTBUFF,  "\n");
		}
	}

/* Dump compound glyph points */
static void dumpCompoundPoints(GlyphId glyphId)
	{
	warning(SPOT_MSG_glyfUNSDCOMP, glyphId);
	}

/* Dump points */
static void dumpPoints(GlyphId glyphId)
	{
	if (glyf->glyph[glyphId].numberOfContours > 0)
		dumpSimplePoints(glyphId);
	else if (glyf->glyph[glyphId].numberOfContours == -1)
		dumpCompoundPoints(glyphId);
	}

/* Initialize for single glyph PostScript drawing */
static void drawSingleInit(void)
	{
	  initGlyphNames();
	  if (unitsPerEm < 1)
		{
		  headGetUnitsPerEm(&unitsPerEm, glyf_);
		  getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
		}
	  
	  proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
								  STDPAGE_TOP, STDPAGE_BOTTOM,
								  NULL,
								  STDPAGE_GLYPH_PTSIZE,
								  10.0, unitsPerEm,
								  1, 1, "glyf");

	  workstr[0] = '\0';
	  sprintf(workstr, "/cntlpt {gsave newpath %g 0 360 arc fill grestore} bind def\n"
		   "/arrow {\n"
		   "newpath 0 0 moveto -15 -5 rlineto 0 10 rlineto closepath fill\n"
		   "} bind def\n",
		   PTS(h, 0.75));
	  proofPSOUT(proofctx, workstr);
	  proofPSOUT(proofctx, "/box{\n"
				 "newpath\n"
				 "-5 -5 moveto 5 -5 lineto 5 5 lineto -5 5 lineto closepath\n"
				 "gsave 2 setlinewidth stroke grestore 0 setgray fill\n"
				 "0 -5 moveto 5 -5 lineto 5 5 lineto 0 5 lineto closepath\n"
				 "1 setgray fill\n"
				 "}bind def\n"
				 "/circle{\n"
				 "newpath\n"
				 "0 0 7 0 360 arc 1 setlinewidth stroke\n"
				 "0 0 moveto 0 0 7 135 225 arc 0 0 moveto 0 setgray fill\n"
				 "}bind def\n");
	}

typedef struct
	{
	double x;
	double y;
	} Vector;

/* Normalize vector */
static void norm(Vector *v)
	{
	double d = sqrt(v->x * v->x + v->y * v->y);

	if (d == 0.0)
		v->x = v->y = 1.0;
	else
		{
		v->x /= d;
		v->y /= d;
		}	
	}

/* Draw end-point tic mark */
static void drawTic(IntX marks, Point *A, double Bx, double By, Point *C)
	{
	if (marks)
		{
		Vector s;
		Vector t;
		Vector u;
		Vector v;
		double y;

		s.x = Bx - A->x;
		s.y = By - A->y;
		t.x = C->x - Bx;
		t.y = C->y - By;

		norm(&s);				/* Normalized vector A->B */
		norm(&t);				/* Normalized vector B->C */

		u.x = s.x + t.x;
		u.y = s.y + t.y;
		norm(&u);				/* Normalized vector A->C */ 

		/* Scaled vector 90 degrees counter clockwise from A->C */
		v.x = -u.y * PTS(h, 5.5);
		v.y = u.x * PTS(v, 5.5);
		
		workstr[0] = '\0';
		sprintf(workstr, "gsave\n"
			   "newpath\n"
			   "%g %g moveto\n"
			   "%g %g rlineto\n", Bx, By, v.x, v.y);
		proofPSOUT(proofctx, workstr);

		y = (v.y > 0.0) ? 0.0 : -(PTS(v, NUMERIC_LABEL_SIZE) * 2.0) / 3.0;
		if (v.x >= 0.0) 
		  {
			workstr[0] = '\0';
			sprintf(workstr, "0 %g rmoveto\n", y);
			proofPSOUT(proofctx, workstr);
		  }
		else
		  {
			workstr[0] = '\0';
			sprintf(workstr, "(%.0f %.0f) stringwidth pop neg %g rmoveto\n",
				   OUTPUT(h, Bx), OUTPUT(v, By), y);
			proofPSOUT(proofctx, workstr);
		  }

		workstr[0] = '\0';
		sprintf(workstr, "(%.0f %.0f) show\n"
			   "0 setlinewidth stroke\n"
			   "grestore\n", OUTPUT(h, Bx), OUTPUT(v, By));
		proofPSOUT(proofctx, workstr);
		}
	}

/* Set transformation matrix */
static void setMatrix(Point *last, double x, double y)
	{
	double dx = x - last->x;
	double dy = y - last->y;
	double hyp = sqrt(dx * dx + dy * dy);

	workstr[0] = '\0';
	sprintf(workstr, "[%g %g %g %g %g %g] concat\n",
		   STD2FNT(h, dx / hyp), STD2FNT(v, dy / hyp), 
		   -STD2FNT(v, dy / hyp), STD2FNT(h, dx / hyp), x, y);
	proofPSOUT(proofctx, workstr);
	}

/* Draw line close path. This will never be called when dx = dy = 0 so hyp can
   never be 0 either */
static void drawLineClosePath(IntX marks, Point *last, Point *p)
	{
	if (marks)
		{
		proofPSOUT(proofctx, "gsave\n");
		setMatrix(last, p->x, p->y);
		proofPSOUT(proofctx, "arrow grestore\n");
		}
	}

#if 0
/* Draw point close path. This will never be called when dx = dy = 0 so hyp can
   never be 0 either */
static void drawPointClosePath(IntX marks, Point *last, double x, double y)
	{
	if (marks)
		{
		fprintf(OUTPUTBUFF,  "gsave\n");
		setMatrix(last, x, y);
		fprintf(OUTPUTBUFF,  "box grestore\n");
		}
	}
#endif

static void drawCurveClosePath(IntX marks, Point *last, double x, double y)
	{
	if (marks)
		{
		proofPSOUT(proofctx, "gsave\n");
		setMatrix(last, x, y);
		proofPSOUT(proofctx, "circle grestore\n");
		}
	}

/* Draw control point */
static void drawCntlPoint(IntX marks, Point *p)
	{
	if (marks)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "%d %d cntlpt\n", p->x, p->y);
		proofPSOUT(proofctx, workstr);
	  }
	}

/* Draw path. This algorithm uses the point before (p0) and the current point
   (p1) in order to determine what PostScript path operators to emit. Any point
   can be on or off the curve giving 4 cases:

   p0   p1		Action
   --   --		------
   off	off		End one bezier curve and begin another.
   off	on		End bezier curve.
   on	off		Begin bezier curve.
   on	on		Draw line.

   The point after the current point (p2) is only used for determining tic mark
   orientation. */
static void drawPath(GlyphId glyphId, IntX marks, IntX fill)
	{
	IntX i;
	Point p0;		/* Previous point */
	Point p1;		/* Current point */
	Point p2;		/* Next point */
	IntX iStart;	/* Start coordinate index */
	Glyph *glyph = &glyf->glyph[glyphId];
	Simple *simple;

	if (!glyph) return;
	simple = glyph->data;
	if (!simple) return;

	if (glyph->numberOfContours > 0)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "%% drawPath(%hu)\n"
				"newpath\n", 
				glyphId);
		proofPSOUT(proofctx, workstr);

		iStart = 0;
		for (i = 0; i < glyph->numberOfContours; i++)
		  {
			IntX iEnd = simple->endPtsOfContours[i];	/* End coordinate index */
			
			p0.on = simple->flags[iEnd] & ONCURVE;
			p0.x = simple->xCoordinates[iEnd];
			p0.y = simple->yCoordinates[iEnd];
			
			p1.on = simple->flags[iStart] & ONCURVE;
			p1.x = simple->xCoordinates[iStart];
			p1.y = simple->yCoordinates[iStart];
			
			if (iStart == iEnd)
			  drawCntlPoint(marks, &p1);
			else
			  {
				IntX nSegs = iEnd - iStart + 1;
				IntX i2 = iStart + 1; /* Next coordinate index */
				
				p2.on = simple->flags[i2] & ONCURVE;
				p2.x = simple->xCoordinates[i2];
				p2.y = simple->yCoordinates[i2];
				
				/* Start contour and draw closepath */
				if (p1.on)
				  {
					workstr[0] = '\0';
					sprintf(workstr, "%d %d moveto\n", p1.x, p1.y);
					proofPSOUT(proofctx, workstr);

					if (p0.on)
					  drawLineClosePath(marks, &p0, &p1);
					else
					  drawCurveClosePath(marks, &p0, p1.x, p1.y);
				  }
				else
				  {
					drawCurveClosePath(marks, &p0, p1.x, p1.y);
					if (p0.on)
					  {
						/* Back up to previous point */
						p2 = p1;
						p1 = p0;
						
						i2 = iStart;

						workstr[0] = '\0';						
						sprintf(workstr, "%d %d moveto\n", p1.x, p1.y);
						proofPSOUT(proofctx, workstr);
					  }
					else
					  {
						workstr[0] = '\0';
						sprintf(workstr, "%g %g moveto\n"
								"%g %g ", 
								(p0.x + p1.x) / 2.0, (p0.y + p1.y) / 2.0,
								(p0.x + 5 * p1.x) / 6.0, (p0.y + 5 * p1.y) / 6.0);
						proofPSOUT(proofctx, workstr);
					  }
				  }
				
				/* Output segments */
				while (nSegs--)
				  {
					p0 = p1;
					p1 = p2;
					
					/* Get next point */
					if (++i2 > iEnd)
					  i2 = iStart; /* Wrap round */
					
					p2.on = simple->flags[i2] & ONCURVE;
					p2.x = simple->xCoordinates[i2];
					p2.y = simple->yCoordinates[i2];
					
					/* Draw annotation */
					if (p1.on)
					  drawTic(marks, &p0, p1.x, p1.y, &p2);
					drawCntlPoint(marks, &p1);
					
					if (p0.on)
					  {
						if (p1.on)
						  {
							/* on on */
							workstr[0] = '\0';
							sprintf(workstr, "%d %d lineto\n", p1.x, p1.y);
							proofPSOUT(proofctx, workstr);
						  }
						else
						  {
							/* on off */
							workstr[0] = '\0';
							sprintf(workstr, "%g %g ", 
									(p0.x + 2 * p1.x) / 3.0, 
									(p0.y + 2 * p1.y) / 3.0);
							proofPSOUT(proofctx, workstr);
						  }
					  }
					else
					  {
						if (p1.on)
						  {
							/* off on */
							workstr[0] = '\0';
							sprintf(workstr, "%g %g %d %d curveto\n", 
									(2 * p0.x + p1.x) / 3.0, 
									(2 * p0.y + p1.y) / 3.0, p1.x, p1.y);
							proofPSOUT(proofctx, workstr);
						  }
						else
						  {
							/* off off */
							double x = (p0.x + p1.x) / 2.0;
							double y = (p0.y + p1.y) / 2.0;
							
							workstr[0] = '\0';
							sprintf(workstr, "%g %g %g %g curveto\n", 
									(5 * p0.x + p1.x) / 6.0, 
									(5 * p0.y + p1.y) / 6.0, x, y);
							proofPSOUT(proofctx, workstr);
							drawTic(marks, &p0, x, y, &p1);
							if (nSegs != 0)
							  {
								workstr[0] = '\0';
								sprintf(workstr, "%g %g ", 
										(p0.x + 5 * p1.x) / 6.0, 
										(p0.y + 5 * p1.y) / 6.0);
								proofPSOUT(proofctx, workstr);
							  }
						  }
					  }
				  }
			  }
			iStart = iEnd + 1;
		  }

		workstr[0] = '\0';
		sprintf(workstr, "0 setlinewidth %s\n", fill ? "fill" : "stroke");
		proofPSOUT(proofctx, workstr);
	  }
	else
	  {
		if (glyph->numberOfContours == -1)	/* a recursive Compound */
		  drawCompound (glyphId, marks, fill);
	  }
	}
	
	
static void proofPath(GlyphId glyphId, ProofContextPtr ctx)
	{
	IntX i;
	Point p0;		/* Previous point */
	Point p1;		/* Current point */
	Point p2;		/* Next point */
	Point stor;
	IntX iStart;	/* Start coordinate index */
	Glyph *glyph = &glyf->glyph[glyphId];
	Simple *simple;

	if (!glyph) return;
	simple = glyph->data;
	if (!simple) return;

	if (glyph->numberOfContours > 0)
	  {
		iStart = 0;
		for (i = 0; i < glyph->numberOfContours; i++)
		  {
			IntX iEnd = simple->endPtsOfContours[i];	/* End coordinate index */
			
			p0.on = simple->flags[iEnd] & ONCURVE;
			p0.x = simple->xCoordinates[iEnd];
			p0.y = simple->yCoordinates[iEnd];
			
			p1.on = simple->flags[iStart] & ONCURVE;
			p1.x = simple->xCoordinates[iStart];
			p1.y = simple->yCoordinates[iStart];
			
			if (iStart != iEnd)
			  {
				IntX nSegs = iEnd - iStart + 1;
				IntX i2 = iStart + 1; /* Next coordinate index */
				
				p2.on = simple->flags[i2] & ONCURVE;
				p2.x = simple->xCoordinates[i2];
				p2.y = simple->yCoordinates[i2];
				
				/* Start contour and draw closepath */
				if (p1.on)
				  {
					proofGlyphMT(ctx, p1.x, p1.y);
				  }
				else
				  {
					if (p0.on)
					  {
						/* Back up to previous point */
						p2 = p1;
						p1 = p0;
						
						i2 = iStart;
						
						proofGlyphMT(ctx, p1.x, p1.y);
					  }
					else {
					  proofGlyphMT(ctx, (p0.x + p1.x) / 2.0, (p0.y + p1.y) / 2.0);
					  stor.x = (Int16)((p0.x + 5 * p1.x) / 6.0);
					  stor.y = (Int16)((p0.y + 5 * p1.y) / 6.0);
				    }
				  }
				
				/* Output segments */
				while (nSegs--)
				  {
					p0 = p1;
					p1 = p2;
					
					/* Get next point */
					if (++i2 > iEnd)
					  i2 = iStart; /* Wrap round */
					
					p2.on = simple->flags[i2] & ONCURVE;
					p2.x = simple->xCoordinates[i2];
					p2.y = simple->yCoordinates[i2];
					
					if (p0.on)
					  {
						if (p1.on)
						  /* on on */
						  proofGlyphLT(ctx, p1.x, p1.y);
						else {
						  /* on off */
						  stor.x = (Int16)((p0.x + 2 * p1.x) / 3.0);
						  stor.y = (Int16)((p0.y + 2 * p1.y) / 3.0);
						}
					  }
					else
					  {
						if (p1.on)
						  {
							/* off on */
							proofGlyphCT(ctx, stor.x, stor.y,
										 (2 * p0.x + p1.x) / 3.0,(2 * p0.y + p1.y) / 3.0, 
										 p1.x, p1.y);
						  }
						else
						  {
							/* off off */
							proofGlyphCT(ctx, stor.x, stor.y,
										 (5 * p0.x + p1.x) / 6.0, (5 * p0.y + p1.y) / 6.0, 
										 (p0.x + p1.x) / 2.0, (p0.y + p1.y) / 2.0);
							
							if (nSegs != 0) {					
							  stor.x = (Int16)((p0.x + 5 * p1.x) / 6.0);
							  stor.y = (Int16)((p0.y + 5 * p1.y) / 6.0);
						    }
						  }
					  }
				  }
			  }
			iStart = iEnd + 1;
		  }
		
		proofGlyphClosePath(ctx);
	  }
	else
	  {
		if (glyph->numberOfContours == -1) /* recursive Compound */
		  proofCompound(glyphId, ctx);
	  }
	}

/* Draw cross at origin or width */
static void drawCross(IntX dataOrig, double x, double y)
	{
	double length = PTS(h, 25.0);

	workstr[0] = '\0';
	sprintf(workstr, "%% width cross\n"
		   "gsave\n"
		   "newpath\n"
		   "%g %g moveto\n"
		   "0 %g rlineto\n"
		   "%g %g moveto\n"
		   "%g 0 rlineto\n",
		   x, y - length / 2,
		   length,
		   x - length / 2, y,
		   length);
	proofPSOUT(proofctx, workstr);
   
	if (dataOrig)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "[%g %g] 0 setdash\n",
			   PTS(h, 0.3), PTS(h, 2.0));
		proofPSOUT(proofctx, workstr);
	  }

	proofPSOUT(proofctx, "0 setlinewidth\n stroke\n grestore\n");
	}

/* Draw width marks */
static void drawWidth(IntX origShift, IntX width)
	{
	drawCross(0, -origShift, 0);
	drawCross(0, width - origShift, 0);
	drawCross(1, 0, 0);
	}

/* Draw compound glyph */
static void drawCompound(GlyphId glyphId, IntX marks, IntX fill)
	{
	IntX i;
	IntX gripe = 0;
	Compound *compound = glyf->glyph[glyphId].data;

	workstr[0] = '\0';
	sprintf(workstr, "%% drawCompound(%hu)\n", glyphId);
	proofPSOUT(proofctx, workstr);

	for (i = 0;; i++)
		{
		Component *component = &compound->component[i];

		if (component->flags & ARGS_ARE_XY_VALUES)
			{
			IntX noShift =  component->arg1 == 0 && component->arg2 == 0;

			proofPSOUT(proofctx, "gsave\n");

			if (component->flags & 
				(WE_HAVE_A_SCALE|WE_HAVE_AN_X_AND_Y_SCALE|
				 WE_HAVE_A_TWO_BY_TWO))
			  {
				/* Apply transformation */
				workstr[0] = '\0';
				sprintf(workstr, "[%g %g %g %g %hd %hd] concat\n",
					   F2Dot142DBL(component->transform[0][0]),
					   F2Dot142DBL(component->transform[0][1]),
					   F2Dot142DBL(component->transform[1][0]),
					   F2Dot142DBL(component->transform[1][1]),
					   component->arg1, component->arg2);
				proofPSOUT(proofctx, workstr);
			  }
			else if (!noShift)
			  {
				workstr[0] = '\0';
				sprintf(workstr, "%hd %hd translate\n",
					   component->arg1, component->arg2);
				proofPSOUT(proofctx, workstr);
			  }
		
			drawPath(component->glyphIndex, marks && noShift, fill);

			proofPSOUT(proofctx, "grestore\n");
			}
		else if (!gripe)
			{
			warning(SPOT_MSG_glyfUNSCANCH, glyphId);
			gripe = 1;
			}
			
		if (!(component->flags & MORE_COMPONENTS))
			break;
		}
	}

static void proofCompound(GlyphId glyphId, ProofContextPtr ctx)
	{
	IntX i;
	Compound *compound = glyf->glyph[glyphId].data;
	Byte8 str[255];
	
	sprintf(str, "%% proofCompound(%hu)\n", glyphId);
	proofPSOUT(ctx, str);
	
	for (i = 0;; i++)
		{
		Component *component = &compound->component[i];

		if (component->flags & ARGS_ARE_XY_VALUES)
			{
			IntX noShift =  component->arg1 == 0 && component->arg2 == 0;

			proofPSOUT(ctx, "gsave\n");

			if (component->flags & 
				(WE_HAVE_A_SCALE|WE_HAVE_AN_X_AND_Y_SCALE|
				 WE_HAVE_A_TWO_BY_TWO)) 
				 {
				/* Apply transformation */
				
				sprintf(str,"[%g %g %g %g %hd %hd] concat\n",
					   F2Dot142DBL(component->transform[0][0]),
					   F2Dot142DBL(component->transform[0][1]),
					   F2Dot142DBL(component->transform[1][0]),
					   F2Dot142DBL(component->transform[1][1]),
					   component->arg1, component->arg2);
				proofPSOUT(ctx, str);
				}
			else if (!noShift) {
				sprintf(str, "%hd %hd translate\n", component->arg1, component->arg2);
				proofPSOUT(ctx, str);
			}
			proofPath(component->glyphIndex, ctx);

			proofPSOUT(ctx, "fill grestore\n");
			}
			
		if (!(component->flags & MORE_COMPONENTS))
			break;
		}
	}

static void drawGlyph(GlyphId glyphId, IntX marks, IntX fill)
	{
	if (glyf->glyph[glyphId].numberOfContours > 0)
		/* Draw simple glyph */
		drawPath(glyphId, marks, fill);
	else if (glyf->glyph[glyphId].numberOfContours == -1)
		drawCompound(glyphId, marks, fill);
	}


void glyfProofGlyph(GlyphId glyphId, void *ctx)
	{
	if (glyf->glyph[glyphId].numberOfContours > 0)
		/* Draw simple glyph */
		proofPath(glyphId, (ProofContextPtr)ctx);
	else if (glyf->glyph[glyphId].numberOfContours == -1)
		proofCompound(glyphId, (ProofContextPtr)ctx);
	}

static void glyfCallProof(GlyphId glyphId)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX width, vwidth, yorig;
	proofOptions options;
	Byte8 *name = getGlyphName(glyphId, 1);

	width = 0;
	glyfgetMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
	proofClearOptions(&options);
	options.lsb = lsb; options.rsb = rsb;

	proofDrawGlyph(proofctx, 
				   glyphId, ANNOT_SHOWIT|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
				   name, ANNOT_SHOWIT|((glyphId%2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
				   NULL, 0, /* altlabel,altlabelflags */
				   0,0, /* originDx,originDy */
				   0,ANNOT_SHOWIT|ANNOT_DASHEDLINE, /* origin,originflags */
				   width,ANNOT_SHOWIT|ANNOT_DASHEDLINE|ANNOT_ATBOTTOM, /* width,widthflags */
				   &options, yorig, "");
 
	}


/* Draw outline glyph */
static void drawOutline(GlyphId glyphId, IntX origShift, IntX width)
	{
	/* Scale to 500 pts/em */
	double h = scale.h * 500.0 / unitsPerEm;
	double v = scale.v * 500.0 / unitsPerEm;

	workstr[0] = '\0';
	sprintf(workstr, "gsave\n"
		   "%g 300 translate\n"
		   "%g %g scale\n"
		   "/Courier findfont %g scalefont setfont\n",
		   72 + origShift * h,
		   h, v,
		   PTS(v, NUMERIC_LABEL_SIZE));
	proofPSOUT(proofctx, workstr);
	
	drawGlyph(glyphId, !opt_Present("-a"), 0);
	drawWidth(origShift, width);

	proofPSOUT(proofctx, "grestore\n");
	}

/* Draw filled glyph */
static void drawFilled(GlyphId glyphId, IntX origShift, IntX width)
	{
	/* Scale to 5 cm/em */
	double h = (scale.h * 5.0 * 72) / (2.54 * unitsPerEm);
	double v = (scale.v * 5.0 * 72) / (2.54 * unitsPerEm);

	workstr[0] = '\0';
	sprintf(workstr, "gsave\n"
		   "%g 60 translate\n"
		   "%g %g scale\n",
		   72 + origShift * h,
		   h, v);
	proofPSOUT(proofctx, workstr);

	drawGlyph(glyphId, 0, 1);
	drawWidth(origShift, width);

	proofPSOUT(proofctx, "grestore\n");
	}

/* Count on- and off-curve points */
static void countPoints(GlyphId glyphId, IntX *onCurve, IntX *offCurve)
	{
	IntX i;
	IntX j;
	Glyph *glyph = &glyf->glyph[glyphId];
	Simple *simple;

	*onCurve = *offCurve = 0;
    if (!glyph) return;
    simple = glyph->data;
    if (!simple) return;

	j = 0;
	for (i = 0; i < glyph->numberOfContours; i++)
		for (; j <= simple->endPtsOfContours[i]; j++)
			if (simple->flags[j] & ONCURVE)
				(*onCurve)++;
			else
				(*offCurve)++;
	}

static void drawText(GlyphId glyphId, IntX lsb, IntX rsb, IntX width)
	{
	IntX onCurve;
	IntX offCurve;
	Glyph *glyph = &glyf->glyph[glyphId];	
	Byte8 *name = getGlyphName(glyphId, 1);
	Byte8 *nowstr;

	countPoints(glyphId, &onCurve, &offCurve);
	nowstr = sysOurtime();

	workstr[0] = '\0';
	sprintf(workstr, "/Helvetica findfont 12 scalefont setfont\n"
		   "72 764 moveto (Outline Instructions:  TrueType) show\n"
		   "318 764 moveto (%s) show\n",
			nowstr);
	proofPSOUT(proofctx, workstr);
	workstr[0] = '\0';
	sprintf(workstr, "72 750 moveto (%s  %s  [%u]) show\n"
		   "gsave\n"
		   "newpath 72 745 moveto 504 0 rlineto 2 setlinewidth stroke\n"
		   "grestore\n",
		   fileName(), (name[0] == '@') ? "--no name--" : name, glyphId);
	proofPSOUT(proofctx, workstr);
	workstr[0] = '\0';
	sprintf(workstr, "318 96 moveto (BBox:  min = %.0f, %.0f  max = %.0f, %.0f) show\n"
		   "318 84 moveto "
		   "(SideBearings:  L = %.0f  R = %.0f  Width = %.0f) show\n",
		   OUTPUT(h, glyph->xMin), OUTPUT(v, glyph->yMin),
		   OUTPUT(h, glyph->xMax), OUTPUT(v, glyph->yMax),
		   OUTPUT(h, lsb), OUTPUT(h, rsb), OUTPUT(h, width));
	proofPSOUT(proofctx, workstr);
	workstr[0] = '\0';
	sprintf(workstr, "318 72 moveto "
		   "(Points:  on-curve = %d  off-curve = %d  Total = %d) show\n"
		   "318 60 moveto (Paths:  %d  Labels:  %s%d units/em) show\n"
		   "318 48 moveto (H-scale = %f  V-scale = %f) show\n",
		   onCurve, offCurve, onCurve + offCurve,
		   glyph->numberOfContours, cntl.std ? 
		   (opt_Present("-R") ? "scaled&rounded to" : "scaled to ") : "",
		   cntl.std ? 1000 : unitsPerEm,
		   scale.h, scale.v);
	proofPSOUT(proofctx, workstr);
	}

/* Get glyph's metrics */
void glyfgetMetrics(GlyphId glyphId, 
					IntX *origShift, 
					IntX *lsb, IntX *rsb, IntX *hwidth, 
					IntX *tsb, IntX *bsb, IntX *vwidth, IntX *yorig)
	{
	FWord tlsb, ttsb;
	uFWord twidth, tvadv;
	Card16 setLsb;
	Glyph *glyph;

	if (!loaded)
	  {
		/* Ensure that glyf table has already been read */
		if (sfntReadTable(glyf_)) 
		  {
			*origShift = 0;
			*lsb = 0;
			*rsb = 0;
			*hwidth = 0;
			*vwidth = 0;
			*tsb = 0;
			*bsb = 0;
            if (yorig != NULL)
                *yorig = DEFAULT_YORIG_KANJI;
			return;
		  }
	  }

	glyph = &glyf->glyph[glyphId];
	if (hmtxGetMetrics(glyphId, &tlsb, &twidth, glyf_))
		{
		/* hmtx table missing, fake values */
		tlsb = 0;
		twidth = 0;
		}

	/* Adjust lsb if required */
	(void)headGetSetLsb(&setLsb, glyf_);
	if (setLsb)
		*origShift = glyph->xMin - tlsb;
	else
		{
		*origShift = tlsb - glyph->xMin;
		if (*origShift)
			warning(SPOT_MSG_glyfLSBXMIN, glyphId);
		}

	*lsb = tlsb;
	*rsb = twidth - (tlsb + (glyph->xMax - glyph->xMin));
	*hwidth = twidth;

	if (vmtxGetMetrics(glyphId, &ttsb, &tvadv, glyf_))
		{
		/* vmtx table missing, fake values */
		ttsb = 0;
		tvadv = 0;
		}

	*vwidth = tvadv; /* ??? */
	*tsb = ttsb;
	*bsb = tvadv - (ttsb + (glyph->yMax - glyph->yMin)); /* ??? */
    if (yorig != NULL)
        *yorig = ttsb + glyph->yMax;
     /*fprintf(stderr,"tsb %d, glyph yMx %d yorig %d.\n", ttsb, glyph->yMax, *yorig);*/
	}

/* Draw large one-page plot of glyph */
static void drawSingle(GlyphId glyphId)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX hwidth, vwidth, yorig;

	glyfgetMetrics(glyphId, &origShift, &lsb, &rsb, &hwidth, &tsb, &bsb, &vwidth, &yorig);

	drawOutline(glyphId, origShift, hwidth);
	drawFilled(glyphId, origShift, hwidth);
	drawText(glyphId, lsb, rsb, hwidth);
	proofOnlyNewPage(proofctx);
	}

static IntX donedate = 0;

static Byte8 *thedate(void)
	{
	static Byte8 date[64];
	if (!donedate)
	  {
		time_t now = time(NULL);
		strftime(date, 64, "%m/%d/%y %H:%M", localtime(&now));
		donedate = 1;
	  }
	return date;
	}

/* Start new page */
static void newPage(IntX page)
	{
	Byte8 *date = thedate();
	char * filename, *shortfilename;
	float fontRevision;
	int i;

	synopsis.hTile = 0;
	synopsis.vTile = PAGE_HEIGHT;
	synopsis.page = page;

	if (page > 1)
	  proofOnlyNewPage(proofctx); /* Output previous page */

	/* Initialize synopsis mode */
	workstr[0] = '\0';
	sprintf(workstr, "%% page %hu\n"
		   "%g %g translate\n"
		   "/Helvetica findfont 12 scalefont setfont\n"
		   "0 %g moveto ",
		   synopsis.page,
		   INCH(0.25), INCH(0.25),
		   PAGE_HEIGHT + 9);
	proofPSOUT(proofctx, workstr);
	workstr[0] = '\0';
	
	filename=fileName();
	headGetFontRevision(&fontRevision, 0);
	
	/*Remove path from filename*/
	shortfilename=filename;
	for (i=0; filename[i]!='\0'; i++)
		if (filename[i]==':' || filename[i]=='/' || filename[i]=='\\')
			shortfilename=filename+i+1;
	
	if (opt_Present("-d"))
		sprintf(workstr, 
				"(Widths: %.0f units/em   [%s] ) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   OUTPUT(h, unitsPerEm), synopsis.title, 
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
	else
		sprintf(workstr, 
				"(TrueType: %s  head vers: %.3f  Widths: %.0f units/em   %s) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   shortfilename, fontRevision, OUTPUT(h, unitsPerEm),  date,
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
	proofPSOUT(proofctx, workstr);
	}

static IntX initTable_and_Names(void)
	{
	if (!loaded)
		{
		/* Ensure that glyf table has already been read */
		if (sfntReadTable(glyf_))
			return 1;
		}
	initGlyphNames();
	return 0;
	}

/* Initialize for font synopsis */
ProofContextPtr glyfSynopsisInit(Byte8 *title, Card32 opt_tag)
	{
	  Byte8 t[5];
	  if (initTable_and_Names()) 
		return NULL;

	  synopsis.title = title;
	  if (unitsPerEm < 1)
		{
		  headGetUnitsPerEm(&unitsPerEm, glyf_);
		  getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
		}
	  if (opt_tag)
	  	{
	  	 IntX i;
	  	 sprintf(t, "%c%c%c%c", TAG_ARG(opt_tag));
	  	 for (i = 0; i < 4; i++)
	  	 	if (t[i] == ' ') t[i]= '_';
	  	}
	  else
	  	strcpy(t, "glyf");
	  proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
								  STDPAGE_TOP, STDPAGE_BOTTOM,
								  title,
								  GLYPH_SIZE,
								  10.0, unitsPerEm,
								  1, 1, 
								  t);
	  newPage(1);
	  
	  return proofctx;
	}


void  glyfSynopsisFinish(void)
	{
	  synopsis.title = NULL;
	}

/* Draw glyph tile for font sysnopsis pages */
void glyfDrawTile(GlyphId glyphId, Byte8 *code)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX width, vwidth, yorig;
	Byte8 *name = getGlyphName(glyphId, 1);
	double s;

	if (unitsPerEm < 1)
	  {
		headGetUnitsPerEm(&unitsPerEm, glyf_);
		getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
	  }

	s = GLYPH_SIZE / unitsPerEm;
	if (synopsis.hTile + TILE_WIDTH > PAGE_WIDTH)
		{
		/* Advance to next row */
		synopsis.hTile = 0;
		synopsis.vTile -= TILE_HEIGHT;
		}
	if (synopsis.vTile - TILE_WIDTH < 0)
		{
		/* Advance to next page */
		newPage(++synopsis.page);
		synopsis.vTile = PAGE_HEIGHT;
		}

	if (synopsis.title == NULL)
	  synopsis.title = " ";

	glyfgetMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		
	/* Draw box and width */
	workstr[0] = '\0';
	sprintf(workstr, "newpath\n"
		   "%g %g moveto %g 0 rlineto 0 -%g rlineto -%g 0 rlineto\n",
		   synopsis.hTile, synopsis.vTile, 
		   TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH);
	proofPSOUT(proofctx, workstr);
	workstr[0] = '\0';
	sprintf(workstr, "closepath 0 setlinewidth stroke\n"
		   "%g (%.0f) stringwidth pop sub %g moveto (%.0f) show\n",
		   synopsis.hTile + TILE_WIDTH - 1, OUTPUT(h, width), 
		   synopsis.vTile - (TEXT_BASE + 1), OUTPUT(h, width));
	proofPSOUT(proofctx, workstr);

	/* Draw [code/]glyphId */
	workstr[0] = '\0';
	sprintf(workstr, "%g %g moveto\n",
		   synopsis.hTile + 1, synopsis.vTile - (TEXT_BASE + 1));
	proofPSOUT(proofctx, workstr);

	if (code == NULL)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "(%hu) show\n", glyphId);
		proofPSOUT(proofctx, workstr);
	  }
	else
	  {
		workstr[0] = '\0';
		sprintf(workstr, "(%s/%hu) show\n", code, glyphId);
		proofPSOUT(proofctx, workstr);
	  }

	if (name[0] != '@')
	  {
		/* Draw glyph name */
		workstr[0] = '\0';
		sprintf(workstr, "%g %g moveto (%s) show\n",
			   synopsis.hTile + 1, 
			   synopsis.vTile - TILE_HEIGHT + TEXT_SIZE / 3.0,
			   name);
		proofPSOUT(proofctx, workstr);
	  }

	/* Draw glyph */
	workstr[0] = '\0';
	sprintf(workstr, "gsave\n"
		   "%g %g translate\n"
		   "%g %g scale\n",
		   synopsis.hTile + 
		   (TILE_WIDTH - width * s) / 2.0 + origShift * s,
		   synopsis.vTile - 
		   (MARGIN_HEIGHT + 
			(double)font.yMax / (font.yMax - font.yMin) * GLYPH_SIZE),
		   s, s);
	proofPSOUT(proofctx, workstr);

	drawGlyph(glyphId, 0, 1);

	/* Draw width */
	drawWidth(origShift, width);

	proofPSOUT(proofctx, "grestore\n");

	synopsis.hTile += TILE_WIDTH;
	}

/* Select dump type */
static void selectDump(GlyphId glyphId, IntX level)
	{
	if (glyphId < nGlyphs)
		switch (level)
			{
		case 1: case 2: case 3: case 4:
			dumpGlyph(glyphId, level);
			break;
		case 5:
			dumpPoints(glyphId);
			break;
		case 6:
			drawSingle(glyphId);
			break;
		case 7:
			glyfDrawTile(glyphId, NULL);
			break;
		case 8:
			glyfCallProof(glyphId);
			break;
			}
	else
		warning(SPOT_MSG_GIDTOOLARGE, glyphId);
	}

void glyfDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [glyf] (%08lx)\n", start));

	if (unitsPerEm < 1)
	  {
		headGetUnitsPerEm(&unitsPerEm, glyf_);
		getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
	  }

	if (opt_Present("-b"))
		{
		Glyph *glyph;
		
		/* Set coordinate scale so that bounding boxes are equivalent */
		if (glyphs.cnt != 1)
			fatal(SPOT_MSG_BOPTION);

		glyph = &glyf->glyph[glyphs.array[0]];	

		scale.h = ((target.right - target.left) * unitsPerEm) /
			((glyph->xMax - glyph->xMin) * 1000.0);
		scale.v = ((target.top - target.bottom) * unitsPerEm) /
			((glyph->yMax - glyph->yMin) * 1000.0);
		}
	if (opt_Present("-R"))
		cntl.rnd = 1;
	else
		cntl.rnd = 0;

	if (opt_Present("-c"))
		/* Output coordinates in Adobe units */
		cntl.std = 1;
	else
		cntl.std = 0;
		
	switch (level)
		{
	case 8:
	  initTable_and_Names();
	  proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
								  STDPAGE_TOP, STDPAGE_BOTTOM,
								  "glyf (Glyph shapes)",
								  GLYPH_SIZE,
								  10.0, unitsPerEm,
								  0, 1, "glyf");
	  break;
	case 7:
		glyfSynopsisInit("glyf", glyf_); /* creates proofctx */
		break;
	case 6:
		drawSingleInit();/* creates proofctx */
		break;
		}

	if (glyphs.size == 0)
		/* No glyphs specified so dump them all */
		for (i = 0; i < nGlyphs; i++)
			selectDump(i, level);
	else
		/* Dump specified glyphs */
		for (i = 0; i < glyphs.cnt; i++)
			selectDump(glyphs.array[i], level);

	if (level == 7)
	  glyfSynopsisFinish();

	if (proofctx)
		{
		proofDestroyContext(&proofctx); /* set proofctx to null */
		}
  }

/* Free simple glyph */
static void freeSimple(Simple *simple)
	{
	if (!simple) return;
	memFree(simple->yCoordinates);
	memFree(simple->xCoordinates);
	memFree(simple->flags);
	memFree(simple->instructions);
	memFree(simple->endPtsOfContours);
	memFree(simple);
	}

/* Free compound glyph */
static void freeCompound(Compound *compound)
	{
	IntX i;
	Component *component;

	if (!compound) return;
	for (i = 0;; i++)
		{
		component = &compound->component[i];
		if (!component) continue;
		if (!(component->flags & MORE_COMPONENTS))
			break;
		}
	if (component->flags & WE_HAVE_INSTRUCTIONS)
			memFree(component->instructions);

	memFree(compound->component);
	memFree(compound);
	}

IntX glyfLoaded(void)
{
  return loaded;
}

void glyfFree(void)
	{
	IntX i;

    if (!loaded)
		return;

	for (i = 0; i < nGlyphs; i++)
		{
		Glyph *glyph = &glyf->glyph[i];
		
		if (glyph->numberOfContours > 0)
			freeSimple(glyph->data);
		else if (glyph->numberOfContours == -1)
			freeCompound(glyph->data);
		}
	memFree(glyf->glyph);
	memFree(glyf); glyf = NULL;
	memFree(workstr);	workstr = NULL;
	unitsPerEm = 0;
	loaded = 0;
	}

/* Handle glyph list argument */
int glyfGlyphScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0;	/* Initialization not required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		Byte8 *arg = argv[argi++];

		da_INIT_ONCE(glyphs, 50, 20);
		if (parseIdList(arg, &glyphs))
			opt_Error(opt_Format, opt, arg);
		}
	return argi;
	}

/* Handle glyph bounding box argument */
int glyfBBoxScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0;	/* Initialization not required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		Byte8 *arg = argv[argi++];

		if (opt_Present("-s") || opt_Present("-o"))
			opt_Error(opt_Exclusive, opt, arg);
		else if (sscanf(arg, "%hd,%hd,%hd,%hd", &target.left, &target.bottom, 
						&target.right, &target.top) != 4)
			opt_Error(opt_Format, opt, arg);
		}
	return argi;
	}

/* Handle scale argument */
int glyfScaleScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0;	/* Initialization not required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		Byte8 *arg = argv[argi++];

		if (opt_Present("-b"))
			opt_Error(opt_Exclusive, opt, arg);
		else if (sscanf(arg, "%lf,%lf", &scale.h, &scale.v) != 2)
			opt_Error(opt_Format, opt, arg);
		}
	return argi;
	}

/* Glyph table specific usage */
void glyfUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- glyf\n"
		   "=5  Print coordinate points\n"
		   "=6  Proof glyph plot\n"
		   "    Options: [-a] [-c] [-R] [-g<list>] [-bleft,bottom,right,top]\n"
		   "             [-shoriz,vert]\n"
		   "    -a  don't show outline annotation\n"
		   "    -c  convert labeled points to Adobe units\n"
		   "    -R  apply intermediate rounding\n"
		   "    -g  comma separated list of glyphs. Each element is of the\n"
		   "        following form:\n"
		   "            N    glyph N\n"
		   "            N-M  glyphs N through M\n"
		   "        e.g. -g22,24,36-39\n"
		   "    -s  glyph scaling factor\n"
		   "    -b  scale glyph to this bounding box\n"
		   "=7  Proof glyph synopsis\n"
		   "    Options: [-g<list>]\n"
		   "    -g  as above\n"
		   "=8  Proof alternate type of glyph synopsis\n"
		   "    Options: [-g<list>]\n"
		   "    -g  as above\n");
	}

