/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CFF_.c	1.25
 * Changed:    5/19/99 23:50:38
 ***********************************************************************/
#if OSX
#include <time.h>
#else
#include <time.h>
#endif
#include <sys/types.h>

#include <string.h>
#include <math.h>
#include "CFF_.h"
#include "sfnt.h"
#include "cffread.h"
#include "proof.h"
#include "head.h"
#include "glyf.h" /* for scale and target */
#include "pathbuild.h"
#include "vmtx.h"
#include "VORG.h"
#include "dump.h"
#include "sys.h" /* sysOurtime() */
#include "name.h"

extern char VORGfound;
extern double sqrt(double);
extern double fabs(double);
#if WIN32 || MACINTOSH
#define rint(d) ceil((d))
#else
extern double rint(double);
#endif
extern size_t strftime(char *s, size_t maxsize, const char *format,
						   const struct tm *timeptr);
extern time_t time(time_t *tptr);

struct /* must NOT be static for some weird gcc issue */
	{
	  cffCtx cff;
	  cffStdCallbacks cb;
	  cffPathCallbacks proofcb;
	  cffPathCallbacks hintedcb;
	  cffPathCallbacks dumpcb;
	  cffFontInfo *fi;
	} CFF_;

static IntX loaded = 0;
static Card32 CFFbufflen = 0;
static Card8 *CFFbuffer = NULL;
ProofContextPtr CFFproofctx = NULL;/* placeholder */
ProofContextPtr cffproofctx;

static Card16 unitsPerEm = 0;
static struct
	{
	Int16 xMin;
	Int16 yMin;
	Int16 xMax;
	Int16 yMax;
	} font;

static struct
	{
	Int16 std;	/* Output in standard units */
	Int16 rnd;	/* Round mapped result */
	} cntl;

#define NUMERIC_LABEL_SIZE 5

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
#define	TEXT_SIZE		5 	/* Tile text point size */
#define TEXT_BASE		((2 * TEXT_SIZE) / 3.0)
float  GLYPH_SIZE	=	24.0;
#define GLYPH_BASE		((2 * GLYPH_SIZE) / 3.0)
#define TILE_WIDTH		(GLYPH_SIZE+12)
#define TILE_HEIGHT		(GLYPH_SIZE+12)
#define MARGIN_WIDTH	((TILE_WIDTH - GLYPH_SIZE) / 2.0)
#define MARGIN_HEIGHT	((TILE_HEIGHT - GLYPH_SIZE) / 2.0)
#define NAME_SPACE		INCH(1)
/*#define MAX_PS_PAGE     14400*/
#define MAX_PS_PAGE     14400
#define THIN_SPACE		10

/* Font synopsis */
static struct
	{
	Byte8 *title;	/* Page title */
	double hTile;	/* Horizontal tile origin (left) */
	double vTile;	/* Vertical tile origin (top) */
	Card16 page;	/* Page number */
	Card32 pagewidth;  /*These entries for Glyph Complement Report*/
	Card32 pageheight;
	} synopsis;


GlyphComplementReportT gcr;

static char *syntheticGlyphs[]={"Delta","Euro", "Omega" ,"approxequal", "asciicircum", "asciitilde", "at", "backslash", "bar",
						"brokenbar","currency","dagger","daggerdbl","degree","divide","equal","estimated",  "fraction",
						"greater", "greaterequal", "infinity", "integral", "less", "lessequal", "afii61289", "logicalnot",
						"lozenge", "minus", "multiply", "notequal", "numbersign", "onehalf", "onequarter",
						"paragraph", "partialdiff", "perthousand", "pi", "plus", "plusminus", "product", "quotedbl",
						"quotesingle", "radical", "section", "summation", "threequarters", ""};
#define NUM_SYN_GLYPHS 46

static Byte8 *workstr = NULL;

static void CFFfatal(void *ctx)
	{
	  fatal(SPOT_MSG_CFFPARSING);
	}
static void *CFFmalloc(void *ctx, size_t size)
	{
	  return memNew(size);
	}
static void CFFfree(void *ctx, void *ptr)
	{
	  memFree(ptr);
	}
static char *CFFcffSeek(void *ctx, long offset, long *count)
	{	
	  if ( (offset >= (long)CFFbufflen) || (offset < 0))
		{ /* illegal seek */
		  *count = 0;
		  return NULL;
		}
	  else 
		{
		  *count = CFFbufflen - offset;
		  return (char*)&(CFFbuffer[offset]);
		}
	}
static char *CFFcffRefill(void *ctx, long *count)
	{
	  *count = 0; /* never called because all data is in memory (CFFbuffer) */
	  return NULL;
	}

static double fixtodbl(cffFixed x)
{
  return (x / 65536.0);
}

static int justsawEC = 0;

static void CFFproofGlyphMT(void *ctx, cffFixed x1, cffFixed y1)
{
  double x, y;  
  x = fixtodbl(x1);
  y = fixtodbl(y1);
  proofGlyphMT(*((ProofContextPtr *)ctx), x, y);
  justsawEC=0;
}
static void CFFproofGlyphLT(void *ctx, cffFixed x1, cffFixed y1)
{
  double x, y;  
  x = fixtodbl(x1);
  y = fixtodbl(y1);
  proofGlyphLT(*((ProofContextPtr *)ctx), x, y);
}
static void CFFproofGlyphCT(void *ctx, int flex,
                cffFixed X1, cffFixed Y1, 
				cffFixed X2, cffFixed Y2, 
				cffFixed X3, cffFixed Y3)
{
  double x1, y1, x2, y2, x3, y3;
  x1 = fixtodbl(X1);
  y1 = fixtodbl(Y1);
  x2 = fixtodbl(X2);
  y2 = fixtodbl(Y2);
  x3 = fixtodbl(X3);
  y3 = fixtodbl(Y3);
  proofGlyphCT(*((ProofContextPtr *)ctx), x1, y1, x2, y2, x3, y3);
}
static void CFFproofGlyphClosePath(void *ctx)
{
  proofGlyphClosePath(*((ProofContextPtr *)ctx));
  justsawEC=0;
}
static void CFFproofGlyphEndChar(void *ctx)
{
  if (justsawEC)
	proofGlyphClosePath(*((ProofContextPtr *)ctx)); 
  else
	justsawEC = 1;
}

static IntX MTcount, DTcount, CTcount, CPcount;
static double initMTx, initMTy;

Outline dumpOutline;

static void resetPathCounts(void)
{
  MTcount = DTcount = CTcount = CPcount = 0;
}

#define NEARNESSEPSILON (0.01)
static boolean notnearlyequal (double a, double b)
{
  double d;
  d = a - b;
  d = fabs(d);
  return (d > NEARNESSEPSILON);
}


static void CFFdumpGlyphMT(void *ctx, cffFixed x1, cffFixed y1)
{
  POutline O = (POutline)(*((ProofContextPtr *)ctx));

  initMTx = fixtodbl(x1);
  initMTy = fixtodbl(y1);
  MTcount++;
  addmoveto(initMTx, initMTy, O);
  justsawEC = 0;
}
static void CFFdumpGlyphLT(void *ctx, cffFixed x1, cffFixed y1)
{
  double x, y;  
  POutline O = (POutline)(*((ProofContextPtr *)ctx));

  x = fixtodbl(x1);
  y = fixtodbl(y1);
  DTcount++;

  addlineto(currx, curry, x, y, 0, O);
}
static void CFFdumpGlyphCT(void *ctx, int flex,
                cffFixed X1, cffFixed Y1, 
				cffFixed X2, cffFixed Y2, 
				cffFixed X3, cffFixed Y3)
{
  double x1, y1, x2, y2, x3, y3;
  POutline O = (POutline)(*((ProofContextPtr *)ctx));

  x1 = fixtodbl(X1);
  y1 = fixtodbl(Y1);
  x2 = fixtodbl(X2);
  y2 = fixtodbl(Y2);
  x3 = fixtodbl(X3);
  y3 = fixtodbl(Y3);
  CTcount++;

  addcurveto(currx, curry, x1, y1, x2, y2, x3, y3, O);
}
static void CFFdumpGlyphClosePath(void *ctx)
{
  boolean shouldsnap;
  POutline O = (POutline)(*((ProofContextPtr *)ctx));

  shouldsnap = notnearlyequal(initMTx, currx) || notnearlyequal(initMTy, curry);
  if (shouldsnap) {
	addlineto(currx, curry, initMTx, initMTy, 1, O);
  }
  CPcount++;
  addclosepath(initMTx, initMTy, O, shouldsnap);
  O->numsubpaths += 1;
  justsawEC=0;
}
static void CFFdumpGlyphEndChar(void *ctx)
{
  if (justsawEC) 
	CFFdumpGlyphClosePath(ctx);  
  else
	justsawEC = 1;
}



void CFF_Read(LongN start, Card32 length)
	{
	if (loaded)
		return;

	CFFbuffer = memNew(length);
	if (CFFbuffer != NULL) 
	  CFFbufflen = length;
	else
	  return;	
	SEEK_SURE(start);
	IN_BYTES(length, CFFbuffer);

	CFF_.cb.message		= NULL;
	CFF_.cb.fatal		= CFFfatal;
	CFF_.cb.malloc		= CFFmalloc;
	CFF_.cb.free		= CFFfree;
	CFF_.cb.cffSeek		= CFFcffSeek;
	CFF_.cb.cffRefill	= CFFcffRefill;
	CFF_.cb.ctx 		= (void *)(& CFFproofctx);

	CFF_.cff = cffNew(&CFF_.cb, 0, CFFREAD_SMALL_FONT); /* On dumping, doesn't hurt to have the max stack limits larger than usual */
	CFF_.fi = cffGetFontInfo(CFF_.cff);

	CFF_.proofcb.newpath = NULL;
	CFF_.proofcb.moveto = CFFproofGlyphMT;
	CFF_.proofcb.lineto = CFFproofGlyphLT;
	CFF_.proofcb.curveto = CFFproofGlyphCT;
	CFF_.proofcb.closepath = CFFproofGlyphClosePath;
	CFF_.proofcb.endchar = CFFproofGlyphEndChar;
	CFF_.proofcb.hintstem = NULL;
	CFF_.proofcb.hintmask = NULL;

	CFF_.dumpcb.newpath = NULL;
	CFF_.dumpcb.moveto = CFFdumpGlyphMT;
	CFF_.dumpcb.lineto = CFFdumpGlyphLT;
	CFF_.dumpcb.curveto = CFFdumpGlyphCT;
	CFF_.dumpcb.closepath = CFFdumpGlyphClosePath;
	CFF_.dumpcb.endchar = CFFdumpGlyphEndChar;
	CFF_.dumpcb.hintstem = NULL;
	CFF_.dumpcb.hintmask = NULL;

	workstr = (Byte8 *)memNew(sizeof(Byte8) * 256);
	loaded = 1;
	}


IntX CFF_isCID(void)
	{
	  if (!loaded) 
		{
		  if (sfntReadTable(CFF__))
			return  0;
		}
	  return (CFF_.fi->cid.registry != CFF_SID_UNDEF);
	}

/* Initialize for single glyph PostScript drawing */
static void drawSingleInit(void)
	{
	  if (unitsPerEm < 1)
		{
		  headGetUnitsPerEm(&unitsPerEm, CFF__);
		  getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
		}
		GLYPH_SIZE=(float)proofCurrentGlyphSize();
	  cffproofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									 STDPAGE_TOP, STDPAGE_BOTTOM,
									 NULL,
									 GLYPH_SIZE,
									 10.0, unitsPerEm,
									 1,  1, "CFF_");
	  workstr[0] = '\0';
	  sprintf(workstr, "/cntlpt {gsave newpath %g 0 360 arc fill grestore} bind def\n"
		   "/arrow {\n"
		   "newpath 0 0 moveto -15 -5 rlineto 0 10 rlineto closepath fill\n"
		   "} bind def\n",
		   PTS(h, 0.75));
	  proofPSOUT(cffproofctx, workstr);

	  proofPSOUT(cffproofctx, "/box{\n"
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


static void drawText(GlyphId glyphId, IntX lsb, IntX rsb, IntX width)
	{
	  cffGlyphInfo *cffgi;
	  Byte8 *name = getGlyphName(glyphId, 1);
	  Byte8 *nowstr;

	  nowstr = sysOurtime();
	  cffgi = cffGetGlyphInfo(CFF_.cff, glyphId, NULL);
	  workstr[0] = '\0';
	  sprintf(workstr, "/Helvetica findfont 12 scalefont setfont\n"
			 "72 764 moveto (Outline Instructions:  CFF/Type2) show\n"
			 "318 764 moveto (%s) show\n",
			nowstr);
	  proofPSOUT(cffproofctx, workstr);
	  workstr[0] = '\0';
	  sprintf(workstr, "72 750 moveto (%s  %s  [@%u]) show\n"
			 "gsave\n"
			 "newpath 72 745 moveto 504 0 rlineto 2 setlinewidth stroke\n"
			  "grestore\n",			
			 fileName(), (name[0] == '@') ? "--no name--" : name, glyphId);
	  proofPSOUT(cffproofctx, workstr);

	  workstr[0] = '\0';
	  sprintf(workstr, "318 96 moveto (BBox:  min = %.0f, %.0f  max = %.0f, %.0f) show\n"
			 "318 84 moveto "
			 "(SideBearings:  L = %.0f  R = %.0f  Width = %.0f) show\n",
			 OUTPUT(h, cffgi->bbox.left), OUTPUT(v, cffgi->bbox.bottom),
			 OUTPUT(h, cffgi->bbox.right), OUTPUT(v, cffgi->bbox.top),
			 OUTPUT(h, lsb), OUTPUT(h, rsb), OUTPUT(h, width));
	  proofPSOUT(cffproofctx, workstr);
	  workstr[0] = '\0';
	  sprintf(workstr, "318 72 moveto "
			 "(Parts:  mt = %d  dt = %d  ct = %d Total = %d) show\n"
			 "318 60 moveto (Paths:  %d  Labels:  %s%d units/em) show\n"
			 "318 48 moveto (H-scale = %f  V-scale = %f) show\n",
			 MTcount, DTcount, CTcount, MTcount+DTcount+CTcount, 
			 CPcount, cntl.std ? 
			 (opt_Present("-R") ? "scaled&rounded to" : "scaled to ") : "",
			 cntl.std ? 1000 : unitsPerEm,
			 scale.h, scale.v);
	  proofPSOUT(cffproofctx, workstr);
	}


static IntX donedate = 0;
static Byte8 date[64];

 Byte8 *getthedate(void)
	{
	
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
	Byte8 *date = getthedate();
	char * filename, *shortfilename;
	float fontRevision;
	int i;

	if (opt_Present("-br"))
	{
		synopsis.hTile = 0;
		synopsis.vTile = TILE_HEIGHT;
		synopsis.page = page;
	}else{
		synopsis.hTile = 0;
		synopsis.vTile = PAGE_HEIGHT;
		synopsis.page = page;
	}

	if (page > 1)
	  proofOnlyNewPage(cffproofctx); /* Output previous page */

	/* Initialize synopsis mode */
	workstr[0] = '\0';
	if (opt_Present("-br"))
	{
		sprintf(workstr, "%% page %hu\n"
			   "<</PageSize [%g %g]>> setpagedevice\n"
			   "%g %g translate\n"
			   "/Helvetica findfont 12 scalefont setfont\n"
			   "0 %g moveto ",
			   synopsis.page,
			   TILE_WIDTH+INCH(0.25), TILE_HEIGHT+INCH(0.25),
			   INCH(0.125), INCH(0.125),
			   TILE_HEIGHT + 9);
	}else{
		sprintf(workstr, "%% page %hu\n"
			   "%g %g translate\n"
			   "/Helvetica findfont 12 scalefont setfont\n"
			   "0 %g moveto ",
			   synopsis.page,
			   INCH(0.25), INCH(0.25),
			   PAGE_HEIGHT + 9);
	}
	proofPSOUT(cffproofctx, workstr);
	workstr[0] = '\0';
	
	filename=fileName();
	headGetFontRevision(&fontRevision, 0);
	
	/*Remove path from filename*/
	shortfilename=filename;
	for (i=0; filename[i]!='\0'; i++)
		if (filename[i]==':' || filename[i]=='/' || filename[i]=='\\')
			shortfilename=filename+i+1;
	
	if(OUTPUT(h, unitsPerEm)==1000)
	{
		if (opt_Present("-br"))
			sprintf(workstr, "%g (%hu) stringwidth pop sub %g moveto\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9,
			   TEXT_SIZE);
		else if (opt_Present("-d"))
			sprintf(workstr, "(  [%s] ) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			    synopsis.title, 
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
		else		
			sprintf(workstr, "(CFF:%s  head vers: %.3f   %s) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   shortfilename, fontRevision, date,
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
	}else{
		if (opt_Present("-br"))
			sprintf(workstr, "%g (%hu) stringwidth pop sub %g moveto\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9,
			   TEXT_SIZE);
		else if (opt_Present("-d"))
			sprintf(workstr, "(Widths: %.0f units/em   [%s] ) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   OUTPUT(h, unitsPerEm), synopsis.title, 
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
		else		
			sprintf(workstr, "(CFF:%s  head vers: %.3f  Widths: %.0f units/em   %s) show\n"
			   "%g (%hu) stringwidth pop sub %g moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   shortfilename, fontRevision, OUTPUT(h, unitsPerEm), date,
			   PAGE_WIDTH, synopsis.page, PAGE_HEIGHT + 9, synopsis.page,
			   TEXT_SIZE);
	}
	
	proofPSOUT(cffproofctx, workstr);
	}

/* Start new page for Glyph Complement Report */
static void gcrnewPage(IntX page)
	{
	synopsis.hTile = NAME_SPACE;
	synopsis.vTile = synopsis.pageheight;
	synopsis.page = page;
	
	if (page > 1)
	  proofOnlyNewPage(cffproofctx); /* Output previous page */
	
	/* Initialize synopsis mode */
	workstr[0] = '\0';
	sprintf(workstr, "%% page %hu\n"
			   "<</PageSize [%d %d]>> setpagedevice\n"
			   "%g %g translate\n"
			   "/Helvetica findfont 12 scalefont setfont\n"
			   "0 %d moveto ",
			   synopsis.page,
			   synopsis.pagewidth+((Card32)INCH(0.25)), synopsis.pageheight+((Card32)INCH(0.25)),
			   INCH(0.125), INCH(0.125),
			   synopsis.pageheight + ((Card32)9));
			   
	proofPSOUT(cffproofctx, workstr);
	workstr[0] = '\0';
	
	sprintf(workstr, "%d (%hu) stringwidth pop sub %d moveto (%hu) show\n"
			   "/Helvetica-Narrow findfont %d scalefont setfont\n",
			   synopsis.pagewidth, synopsis.page, synopsis.pageheight +((Card32)9), synopsis.page,
			   TEXT_SIZE);
	
	proofPSOUT(cffproofctx, workstr);
	}

/*Prints the fonts' psname at the beginning of each line for the Glyph Complement Report*/	
extern char *infilename;

static void CFFproofFontName()
{
	Byte8 *psname;
	Byte8 *str2;
	int i, length;
	float fontRevision;
	
	workstr[0] = '\0';
	psname = namePostScriptName();
	if (!psname)
		psname=nameFontName();
	if (!psname){
		if (infilename!=NULL)
		{
			int loc=-1;
			
			for(i=0; i<(int)strlen(infilename); i++)
				if(infilename[i]=='/' || infilename[i]=='\\' || infilename[i]==':')
					loc=i;
						
			psname=	(char *)memNew(strlen(infilename)+1-loc);
			strcpy(psname, infilename+loc+1);
		}
	}
	if(!psname)
	{
		psname=(char *)memNew(8);
		strcpy(psname, "Unknown");
	}
	
	length=strlen(psname);
	str2=(char *)memNew(length+1);
	str2[0]='\0';
	for(i=0; i<length; i++)
		if(psname[i]=='-' || psname[i]=='_' || psname[i]=='.')
		{
			strcpy(str2, psname+i+1);
			psname[i+1]='\0';
			break;
		}
	
	headGetFontRevision(&fontRevision, 0);
	
	sprintf(workstr, "/Helvetica-Narrow findfont %d scalefont setfont\n", 10);
	
	proofPSOUT(cffproofctx, workstr);
	
	sprintf (workstr, "%d %g moveto (%s) show\n"
					  "%d %g moveto (%s) show\n"
					  "%d %g moveto (Rev.%.3f) show\n",
					  0, synopsis.vTile-(TILE_HEIGHT/3.0), psname, 
					  0, synopsis.vTile-(2*TILE_HEIGHT/3.0), str2, 
					  0, synopsis.vTile-(TILE_HEIGHT),fontRevision);

	proofPSOUT(cffproofctx, workstr);

	sprintf(workstr, "/Helvetica-Narrow findfont %d scalefont setfont\n", TEXT_SIZE);

	proofPSOUT(cffproofctx, workstr);
	
	memFree(psname);
	memFree(str2);
}

/* Normalize vector */
static void norm(Vector *v)
	{
	double d = sqrt(v->x * v->x + v->y * v->y);

	if (d == 0.0)
	  {
		v->x = v->y = 1.0;
	  }
	else
		{
		v->x /= d;
		v->y /= d;
		}	
	}

/* Draw end-point tic mark at the end of "A" */
static void drawTic(IntX marks, Pelt A)
	{
	if (marks)
		{
		Vector s;
		Vector t;
		Vector u;
		Vector v;
		double y;
		double Bx, By;
		Pelt C;

		Bx =  A->coord[XP3];
		By =  A->coord[YP3];

		end_vector(A, &s);
		C = SUCC(A);
		init_vector(C, &t);

		u.x = s.x + t.x;
		u.y = s.y + t.y;
		norm(&u);				/* Normalized vector A->C */ 

		/* Scaled vector 90 degrees counter clockwise from A->C */
		v.x = u.y * PTS(h, 5.5);
		v.y = -u.x * PTS(v, 5.5);
		
		workstr[0] = '\0';
		sprintf(workstr, "gsave %% tic\n"
			   "newpath\n"
			   "%g %g moveto\n"
			   "%g %g rlineto\n", 
				Bx, By, v.x, v.y);
		proofPSOUT(cffproofctx, workstr);

		y = (v.y > 0.0) ? 0.0 : -(PTS(v, NUMERIC_LABEL_SIZE) * 2.0) / 3.0;
		if (v.x >= 0.0)
		  {
			workstr[0] = '\0';			
			sprintf(workstr, "0 %g rmoveto\n", y);
			proofPSOUT(cffproofctx, workstr);
		  }
		else
		  {
			workstr[0] = '\0';
			sprintf(workstr, "(%.0f %.0f) stringwidth pop neg %g rmoveto\n",
				   OUTPUT(h, Bx), OUTPUT(v, By), y);
			proofPSOUT(cffproofctx, workstr);
		  }

		workstr[0] = '\0';
		sprintf(workstr, "(%.0f %.0f) show\n"
			   "0 setlinewidth stroke\n"
			   "grestore %% tic\n", 
				OUTPUT(h, Bx), OUTPUT(v, By));
		proofPSOUT(cffproofctx, workstr);
		}
	}

/* Set transformation matrix */
static void setMatrix(Pelt last, double x, double y)
	{
	double dx = x - last->coord[XP0];
	double dy = y - last->coord[YP0];
	double hyp = sqrt(dx * dx + dy * dy);

	workstr[0] = '\0';
	sprintf(workstr, "[%g %g %g %g %g %g] concat\n",
		   STD2FNT(h, dx / hyp), STD2FNT(v, dy / hyp), 
		   -STD2FNT(v, dy / hyp), STD2FNT(h, dx / hyp), x, y);		   
	proofPSOUT(cffproofctx, workstr);
	}

/* Draw line close path. This will never be called when dx = dy = 0 so hyp can
   never be 0 either */
static void drawLineClosePath(IntX marks, Pelt last, Pelt p)
	{
	if (marks)
		{
		proofPSOUT(cffproofctx, "gsave\n");
		setMatrix(last, p->coord[XP3], p->coord[YP3]);
		proofPSOUT(cffproofctx, "arrow grestore\n");
		}
	}

static void drawCurveClosePath(IntX marks, Pelt last, Pelt p)
	{
	if (marks)
		{
		proofPSOUT(cffproofctx, "gsave\n");
		setMatrix(last, p->coord[XP3], p->coord[YP3]);
		proofPSOUT(cffproofctx, "circle grestore\n");
		}
	}

/* Draw control point */
static void drawCntlPoint(IntX marks, IntX x, IntX y)
	{
	if (marks)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "%d %d cntlpt\n", x, y);
		proofPSOUT(cffproofctx, workstr);
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
	proofPSOUT(cffproofctx, workstr);

	if (dataOrig)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "[%g %g] 0 setdash\n",
			   PTS(h, 0.3), PTS(h, 2.0));
		proofPSOUT(cffproofctx, workstr);
	  }

	proofPSOUT(cffproofctx, "0 setlinewidth\n stroke\n grestore\n");
	}

/* Draw width marks */
static void drawWidth(IntX origShift, IntX width)
	{
	drawCross(0, -origShift, 0);
	drawCross(0, width - origShift, 0);
	drawCross(1, 0, 0);
	}

static void CFFdrawGlyph(GlyphId glyphId, IntX marks, IntX fill)
	{
	  int i;
	  Pelt e;
	  PPath p;
	  int dx1, dy1, dx2, dy2;

	  workstr[0] = '\0';
	  sprintf(workstr, "%% drawPath(%hu)\n"
			 "newpath\n", 
			 glyphId);
	  proofPSOUT(cffproofctx, workstr);

	  /* build up the Outline data */
	  resetPathCounts();
	  init_Outlines(&dumpOutline);
	  CFFproofctx = (ProofContextPtr)((void *)&dumpOutline);
	  
	  if (glyphId >= CFF_.fi->nGlyphs)
	  	glyphId = 0; /* avoid out-of-bounds */
	  	
	  cffGetGlyphInfo(CFF_.cff, glyphId, &CFF_.dumpcb);
	  if (justsawEC)
		  CFFdumpGlyphClosePath((void *)&CFFproofctx);
	  CFFproofctx = NULL;

	  /* walk through it and output it */
	  for (i = 0; i < dumpOutline.numsubpaths; i++) {
		if ((p = dumpOutline.subpath[i]) == (PPath)NULL) break;
		for (e = p->elements;
			 (e != NULL) && (!ISCPorLCPTYPE(e));
			 e=e->nextelt) {
		  switch (e->elttype) {
		  case MTtype : 
			proofGlyphMT(cffproofctx, e->coord[XP0], e->coord[YP0]);
			break;
		  case DTtype :
			if (e->nextelt->elttype != LCPtype)
			  {
				proofGlyphLT(cffproofctx, e->coord[XP3], e->coord[YP3]);
				drawTic(marks, e);
			  }
			break;
		  case CTtype : 
			proofGlyphCT(cffproofctx, e->coord[XP1], e->coord[YP1], e->coord[XP2], e->coord[YP2], e->coord[XP3], e->coord[YP3]);
			drawTic(marks, e);
			dx1 = (int)rint(e->coord[XP1]);
			dy1 = (int)rint(e->coord[YP1]);
			drawCntlPoint(marks, dx1, dy1);
			dx2 = (int)rint(e->coord[XP2]);
			dy2 = (int)rint(e->coord[YP2]);
			drawCntlPoint(marks, dx2, dy2);
          break;
		  }
		}
		proofGlyphClosePath(cffproofctx);

		e = p->lastelement;
		drawTic(marks, e);

		if(e->prevelt!=NULL)
		{
			if (e->prevelt->elttype == DTtype)
			  {
				drawLineClosePath(marks, e->prevelt, e);
			  }
			else
			  {
				drawCurveClosePath(marks, e->prevelt, e);
			  }
		}
	  }

	  workstr[0] = '\0';
	  sprintf(workstr, "0 setlinewidth %s\n", fill ? "fill" : "stroke");
	  proofPSOUT(cffproofctx, workstr);
	  free_Outlines(&dumpOutline);
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
	proofPSOUT(cffproofctx, workstr);
	CFFdrawGlyph(glyphId, !opt_Present("-a"), 0);
	drawWidth(origShift, width);

	proofPSOUT(cffproofctx, "grestore\n");
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
	proofPSOUT(cffproofctx, workstr);

	CFFdrawGlyph(glyphId, 0, 1);
	drawWidth(origShift, width);

	proofPSOUT(cffproofctx, "grestore\n");
	}

/* Draw large one-page plot of glyph */
static void drawSingle(GlyphId glyphId)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX hwidth, vwidth;
	Byte8 *name = getGlyphName(glyphId, 1);

	workstr[0] = '\0';
	sprintf(workstr, "%% SINGLE PLOT OF:  %s\n", name);
	proofPSOUT(cffproofctx, workstr);
	CFF_getMetrics(glyphId, &origShift, &lsb, &rsb, &hwidth, &tsb, &bsb, &vwidth, NULL);

	drawOutline(glyphId, origShift, hwidth);
	drawFilled(glyphId, origShift, hwidth);
	drawText(glyphId, lsb, rsb, hwidth);
	proofOnlyNewPage(cffproofctx);
	}

static IntX initTable_and_Names(void)
	{
	if (!loaded)
		{
		/* Ensure that CFF_ table has already been read */
		if (sfntReadTable(CFF__))
			return 1;
		}
	initGlyphNames();
	return 0;
	}

/* Tests whether a font should go in a reduced glyph complement reporter*/

int IsSynthetic(GlyphId glyphId)
{
	Byte8 *name = getGlyphName(glyphId, 1);
	int i;
	
	if(syntheticGlyphs[NUM_SYN_GLYPHS][0]!='\0'){
		fprintf(OUTPUTBUFF, "%s: WARNING: NUM_SYN_GLYPHS is not set correctly. ", global.progname);
		for(i=0; i<NUM_SYN_GLYPHS*2; i++)
			if(syntheticGlyphs[i][0]=='\0')
			{
				fprintf(OUTPUTBUFF, "Should be %d.\n", i);
				return 0;
			}
	}
		
	for(i=0; i<NUM_SYN_GLYPHS; i++)
		if (!strcmp(name, syntheticGlyphs[i])) return 1;
	
	return 0;
}

int IsH(GlyphId glyphId)
{
	Byte8 *name = getGlyphName(glyphId, 1);
	return (strcmp(name, "H")==0);

}

int IsZero(GlyphId glyphId)
{
	Byte8 *name = getGlyphName(glyphId, 1);
	return (strcmp(name, "zero")==0);

}


/* Initialize for font synopsis */
ProofContextPtr CFF_SynopsisInit(Byte8 *title, Card32 opt_tag)
	{
	  Byte8 t[5];
	  Card16 numglyphs;
	  if (initTable_and_Names()) 
		return NULL;

		GLYPH_SIZE=(float)proofCurrentGlyphSize();
	  if (unitsPerEm < 1)
		{
		  headGetUnitsPerEm(&unitsPerEm, CFF__);
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
	  	strcpy(t, "CFF_");
	  synopsis.title = title;
	  if(gcr.reportNumber) 
	  {
	  		if(gcr.reportNumber==1) /*Only the first time*/
	  		{
	  			int numRows;
	  			
	  			if(gcr.synOnly)
	  				gcr.numGlyphs=numglyphs=NUM_SYN_GLYPHS+2;
	  			else{
	  				gcr.numGlyphs=numglyphs=gcr.maxNumGlyphs;
	  			}
	  			
	  			synopsis.pagewidth=(Card32)((TILE_WIDTH*numglyphs)+NAME_SPACE);
	  			
	  			numRows=(int)(synopsis.pagewidth/((int) MAX_PS_PAGE-INCH(0.25)));
	  			if(numRows*(MAX_PS_PAGE-INCH(0.25))<synopsis.pagewidth) numRows++;
	  			
	  			if (synopsis.pagewidth+INCH(0.25)>MAX_PS_PAGE)
					synopsis.pagewidth=(Card32)(MAX_PS_PAGE-INCH(0.25));
	  			
	  			synopsis.pageheight=(Card32)((TILE_HEIGHT*gcr.numFonts*numRows)+THIN_SPACE*numRows);
	  			if (synopsis.pageheight+INCH(0.25)>MAX_PS_PAGE)
					synopsis.pageheight=(Card32)(MAX_PS_PAGE-INCH(0.25));
	  			
	  			/*fprintf(OUTPUTBUFF, "%ld %ld\n", synopsis.pagewidth, synopsis.pageheight);*/
	  			cffproofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_LEFT+synopsis.pagewidth,
										 STDPAGE_BOTTOM+synopsis.pageheight, STDPAGE_BOTTOM,
										 "Glyph Complement Report",
										 GLYPH_SIZE,
										 10.0, unitsPerEm,
										 1, 1, t);

		  		gcrnewPage(1);
		  	}else{
		  		if(gcr.synOnly)
	  				numglyphs=NUM_SYN_GLYPHS+2;
	  			else
	  				CFF_GetNGlyphs(&numglyphs, CFF__);
	  				
		  		/*if(numglyphs!=gcr.numGlyphs && gcr.reportNumber<=gcr.numFonts+1)
		  			fprintf(OUTPUTBUFF, "%s: WARNING: font number %d does not have the same number of glyphs as number 1. (%d vs %d)\n",
		  								global.progname, gcr.reportNumber, numglyphs, gcr.numGlyphs);
		  		*/
		  		synopsis.hTile = NAME_SPACE;
				/*synopsis.vTile = synopsis.pageheight-(gcr.reportNumber-1)*TILE_HEIGHT;*/
				if (synopsis.vTile - TILE_WIDTH < 0)
					{
					/* Advance to next page */
					gcrnewPage(++synopsis.page);
					}				
			}
		CFFproofFontName();
	  }else{
		  cffproofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										 STDPAGE_TOP, STDPAGE_BOTTOM,
										 title,
										 GLYPH_SIZE,
										 10.0, unitsPerEm,
										 (title == NULL), 1, t);

		  newPage(1);
	 }	  
	  return cffproofctx;
	}

void CFF_SynopsisFinish(void)
	{
	  synopsis.title = NULL;
	}


/* Draw glyph tile for font sysnopsis pages */
int CFF_DrawTile(GlyphId glyphId, Byte8 *code)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX width, vwidth;
	Byte8 *name = getGlyphName(glyphId, 1);

	double s, delt;

	if (unitsPerEm < 1)
	  {
		headGetUnitsPerEm(&unitsPerEm, CFF__);
		getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
	  }

	s = GLYPH_SIZE / unitsPerEm;

	if (gcr.reportNumber!=0){
		if (synopsis.hTile + TILE_WIDTH > synopsis.pagewidth)
			{
			/* Reached end of row, prepare for next font */
			synopsis.hTile = 0;
			if(gcr.endGlyph<glyphId)
				gcr.endGlyph=glyphId;
			return 1;
			}
		if (synopsis.vTile - TILE_WIDTH < 0)
		{
		/* Advance to next page */
		gcrnewPage(++synopsis.page);
		}
	}
	else
	{
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
		else if (opt_Present("-br"))
			{ 
				if(synopsis.page==1)
				{
					synopsis.page++;
				}else{
					newPage(++synopsis.page);
					synopsis.vTile = TILE_HEIGHT;
				}
			}
	}
	if (synopsis.title == NULL)
	  synopsis.title = " ";

	CFF_getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, NULL);
		
	/* Draw box and width */
	workstr[0] = '\0';
	sprintf(workstr, "newpath\n"
		   "%g %g moveto %g 0 rlineto 0 -%g rlineto -%g 0 rlineto\n",
		   synopsis.hTile, synopsis.vTile, 
		   TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH);
	proofPSOUT(cffproofctx, workstr);
	workstr[0] = '\0';
	sprintf(workstr, "closepath 0 setlinewidth stroke\n"
		   "%g (%.0f) stringwidth pop sub %g moveto (%.0f) show\n",
		   synopsis.hTile + TILE_WIDTH - 1, OUTPUT(h, width), 
		   synopsis.vTile - (TEXT_BASE + 1), OUTPUT(h, width));
	proofPSOUT(cffproofctx, workstr);

	/* Draw [code/]glyphId */
	workstr[0] = '\0';
	sprintf(workstr, "%g %g moveto\n",
		   synopsis.hTile + 1, synopsis.vTile - (TEXT_BASE + 1));
	proofPSOUT(cffproofctx, workstr);

	if (code == NULL)
	  {
		workstr[0] = '\0';
		sprintf(workstr, "(%hu) show\n", glyphId);
		proofPSOUT(cffproofctx, workstr);
	  }
	else
	  {
		workstr[0] = '\0';
		sprintf(workstr, "(%s/%hu) show\n", code, glyphId);
		proofPSOUT(cffproofctx, workstr);
	  }

	if (name[0] != '@')
	  {
		/* Draw glyph name */
		workstr[0] = '\0';
		sprintf(workstr, "%g %g moveto (%s) show\n",
			   synopsis.hTile + 1, 
			   synopsis.vTile - TILE_HEIGHT + TEXT_SIZE / 3.0,
			   name);
		proofPSOUT(cffproofctx, workstr);
	  }

	delt = (MARGIN_HEIGHT + (double)font.yMax / (font.yMax - font.yMin) * GLYPH_SIZE);
	
	workstr[0] = '\0';
	sprintf(workstr, "gsave\n"
		   "%g %g translate\n"
		   "%g %g scale\n",
		   synopsis.hTile + 
		   (TILE_WIDTH - width * s) / 2.0 + origShift * s,
		   synopsis.vTile - delt,
		   s, s);
	proofPSOUT(cffproofctx, workstr);

	CFFdrawGlyph(glyphId, 0, 1);

	/* Draw width */
	drawWidth(origShift, width);

	proofPSOUT(cffproofctx, "grestore\n");

	synopsis.hTile += TILE_WIDTH;
	return 0;
	}


static void CFF_CallProof(GlyphId glyphId)
	{
	IntX origShift;
	IntX lsb, rsb, tsb, bsb;
	IntX width, vwidth, yorig;
	proofOptions options;
	Byte8 *name = getGlyphName(glyphId, 1);

	width = 0;
	CFF_getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
	proofClearOptions(&options);
	options.lsb = lsb; options.rsb = rsb;
	proofDrawGlyph(cffproofctx, 
				   glyphId, ANNOT_SHOWIT|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
				   name, ANNOT_SHOWIT|((glyphId%2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
				   NULL, 0, /* altlabel,altlabelflags */
				   0,0, /* originDx,originDy */
				   0,ANNOT_SHOWIT|ANNOT_DASHEDLINE, /* origin,originflags */
				   width,ANNOT_SHOWIT|ANNOT_DASHEDLINE|ANNOT_ATBOTTOM, /* width,widthflags */
				   &options, yorig, "");
	}

static Card8 *getCFFstring(cffSID id, unsigned *len)
{
  long offset = (-1);
  long cnt = (-1);
  IntX isstd;
  char *cptr;

  *len = 0;
  if (id == CFF_SID_UNDEF) {
	return NULL;
  }

  isstd = cffGetString(CFF_.cff, id, len, &cptr, &offset);
  if (isstd != 1) 
	{
	  cptr = CFFcffSeek(CFF_.cff, offset, &cnt);
	}
  return (Card8 *)cptr;
}

static void dumpCharstring(cffSID id)
{
  Card8 *ptr;
  unsigned len = 0;

  ptr = getCFFstring(id, &len);

  if ((ptr != NULL) && (len > 0))
	{
	  IntX nMasters;
	  if ((nMasters = CFF_.fi->mm.nMasters) < 1) nMasters = 1;
	  dump_csDump(len, ptr, nMasters);
	}
  else
	{
	  fprintf(OUTPUTBUFF, " ");
	}
}


/* Select dump type */
static int selectDump(GlyphId glyphId, IntX level)
	{
	if (glyphId < CFF_.fi->nGlyphs)
		switch (level)
			{
		case 6:
			drawSingle(glyphId);
			break;
		case 7:
			return CFF_DrawTile(glyphId, NULL);
			break;
		case 8:
			CFF_CallProof(glyphId);
			break;
		case 9:
			return CFF_DrawTile(glyphId, NULL);
			break;
		
		}
	else
		warning(SPOT_MSG_GIDTOOLARGE, glyphId);
	return 0;
	}



static void CFF_dumpfi(IntX level)
{
  Card8 *ptr;
  unsigned len;
  cffFontInfo *fi = CFF_.fi;
  IntX i;

  {
	long cnt;
	Card8 nam[100];
	ptr = (Card8 *)CFFcffSeek(CFF_.cff, fi->FontName.offset, &cnt);
	strncpy((char *)nam, (char *)ptr, fi->FontName.length);
	nam[fi->FontName.length] = '\0';
	DL(2, (OUTPUTBUFF, "FontName    =<%s>\n", nam));
  }

  ptr = getCFFstring(fi->version, &len);  	DL(2, (OUTPUTBUFF, "version     =<%.*s>\n", (int)len, ptr));
  ptr = getCFFstring(fi->Notice, &len);  	DL(2, (OUTPUTBUFF, "Notice      =<%.*s>\n", (int)len, ptr));
  ptr = getCFFstring(fi->Copyright, &len);	DL(2, (OUTPUTBUFF, "Copyright   =<%.*s>\n", (int)len, ptr));
  ptr = getCFFstring(fi->FamilyName, &len);	DL(2, (OUTPUTBUFF, "FamilyName  =<%.*s>\n", (int)len, ptr));
  ptr = getCFFstring(fi->FullName, &len);	DL(2, (OUTPUTBUFF, "FullName    =<%.*s>\n", (int)len, ptr));

  DL(2, (OUTPUTBUFF, "FontBBox    =[%d %d %d %d]\n", 
		 fi->FontBBox.left, 
		 fi->FontBBox.bottom, 
		 fi->FontBBox.right, 
		 fi->FontBBox.top)); 

  DLs(2, "unitsPerEm  =",fi->unitsPerEm);
  DLs(2, "isFixedPitch=", fi->isFixedPitch);
  DLF(2, "ItalicAngle =", (Fixed)fi->ItalicAngle);
  DLs(2, "UnderlinePosition =", fi->UnderlinePosition);
  DLs(2, "UnderlineThickness=", fi->UnderlineThickness);
  if (fi->UniqueID!=4000000)
  	DLU(2, "UniqueID    =", (Card32)fi->UniqueID);
  if (fi->XUID[0]!=0)
  {
  	DL(2, (OUTPUTBUFF, "XUID        ="));
  	for (i=0; i<fi->XUID[0]; i++)
  		DL(2, (OUTPUTBUFF, "%ld ", fi->XUID[i+1]));
  	DL(2, (OUTPUTBUFF, "\n"));
  }

  DLs(2, "Encoding    =", fi->Encoding);
  DLs(2, "charset     =", fi->charset);

  if (fi->mm.nAxes > 0)
  {
	DLs(2, "nAxes       =", fi->mm.nAxes);
	DLs(2, "nMasters    =", fi->mm.nMasters);
	DLs(2, "lenBuildCharArray=", fi->mm.lenBuildCharArray);
	DL(2, (OUTPUTBUFF, "NDV         =<")); dumpCharstring(fi->mm.NDV); DL(2, (OUTPUTBUFF, ">\n"));
	DL(2, (OUTPUTBUFF, "CDV         =<")); dumpCharstring(fi->mm.CDV); DL(2, (OUTPUTBUFF, ">\n"));
	

	DL(2, (OUTPUTBUFF, "UDV         =["));
	for (i = 0; i < fi->mm.nAxes; i++)
	  {
		if (fi->mm.UDV[i] > 0)
		  {
			DLF(2, " ", (Fixed)fi->mm.UDV[i]);
		  }
		else
		  DL(2, (OUTPUTBUFF, " ? "));
	  }
	DL(2, (OUTPUTBUFF, "]\n"));

	DL(2, (OUTPUTBUFF, "axisTypes   =[ "));
	for (i = 0; i < fi->mm.nAxes; i++)
	  {
		ptr = getCFFstring(fi->mm.axisTypes[i], &len);
		DL(2, (OUTPUTBUFF, "<%.*s> ", (int)len, ptr));
	  }
	DL(2, (OUTPUTBUFF, "]\n"));
  }

  if (fi->cid.registry != CFF_SID_UNDEF)
	{
	  DL(2, (OUTPUTBUFF, "version     =%g\n", fi->cid.version));
	  ptr = getCFFstring(fi->cid.registry, &len);  DL(2, (OUTPUTBUFF, "registry    =<%.*s>\n", (int)len, ptr));
	  ptr = getCFFstring(fi->cid.ordering, &len);  DL(2, (OUTPUTBUFF, "ordering    =<%.*s>\n", (int)len, ptr));
	  DLs(2, "supplement  =", fi->cid.supplement);
	  DL(2, (OUTPUTBUFF, "vOrig       =(%d, %d)\n", fi->cid.vOrig.x, fi->cid.vOrig.y)); 
	}

  DLu(2, "nGlyphs     =", fi->nGlyphs);
}

typedef struct {
	char *name;
	IntX id;
	} GlyphnameT; 

int GlyphnameCmp(const void *p1, const void *p2)
{
	return strcmp( ((GlyphnameT *)p1)->name,((GlyphnameT *)p2)->name );
}

void dumpByName()
{
	GlyphnameT *list = (GlyphnameT *)memNew(sizeof(GlyphnameT)*CFF_.fi->nGlyphs);
	int i;
	
	for(i=0; i<CFF_.fi->nGlyphs; i++)
	{
		Byte8 *name = getGlyphName(i, 1);
		
		list[i].name=(Byte8 *)memNew(strlen(name)+1);
		strcpy(list[i].name, name);
		list[i].id=i;
	}

	qsort(list, CFF_.fi->nGlyphs, sizeof(GlyphnameT), GlyphnameCmp);
	

	for (i = gcr.startGlyph; i < CFF_.fi->nGlyphs; i++)
		if(selectDump(list[i].id, 9))
		{
			gcr.endGlyph=i;
			break;
		}
	for(i=0; i<CFF_.fi->nGlyphs; i++)
		memFree(list[i].name);
	memFree(list);
	

}

void CFF_Dump(IntX level, LongN start)
	{
	  IntX i;
	  
	  if (!loaded) 
		{
		  if (sfntReadTable(CFF__))
			{
			  return;
			}
		}
	  DL(1, (OUTPUTBUFF, "### [CFF_] (%08lx)\n", start));

	  initGlyphNames();
	  headGetUnitsPerEm(&unitsPerEm, CFF__);
	  getFontBBox(&font.xMin, &font.yMin, &font.xMax, &font.yMax);
		GLYPH_SIZE=(float)proofCurrentGlyphSize();
	  if (level < 5)
		{
		  CFF_dumpfi(level);
		  return;
		}

	  if (opt_Present("-b"))
		{
		  cffGlyphInfo *cffgi;
		  
		  /* Set coordinate scale so that bounding boxes are equivalent */
		  if (glyphs.cnt != 1)
			fatal(SPOT_MSG_BOPTION);
		  
		  cffgi = cffGetGlyphInfo(CFF_.cff, glyphs.array[0], NULL);
		  
		  scale.h = ((target.right - target.left) * unitsPerEm) /
			((cffgi->bbox.right - cffgi->bbox.left) * 1000.0);
		  scale.v = ((target.top - target.bottom) * unitsPerEm) /
			((cffgi->bbox.top - cffgi->bbox.bottom) * 1000.0);
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

	  dumpOutline.subpath = (PPath *)NULL;
	  
	  switch (level)
		{
		case 9: 
			gcr.reportNumber++;
			CFF_SynopsisInit("CFF_", CFF__); /* creates cffproofctx */
			break;
		case 8:
		  cffproofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										 STDPAGE_TOP, STDPAGE_BOTTOM,
										 "CFF_ (Glyph shapes)",
										 GLYPH_SIZE,
										 10.0, unitsPerEm,
										 0, 1, "CFF_");
		  break;
		case 7: 
		  CFF_SynopsisInit("CFF_", CFF__); /* creates cffproofctx */
		  break;
		case 6:
		  drawSingleInit(); /* creates cffproofctx */
		  break;
		default:
		break;
		}
	  
	  if (level==9)
	  	if(gcr.synOnly)
		  {
		  	for (i = 0; i < CFF_.fi->nGlyphs; i++)
			{
			  	if(IsH(i))
			  	{
					  selectDump(i, level);
					  break;
				}
			}
			for (i = 0; i < CFF_.fi->nGlyphs; i++)
			{
			  	if(IsZero(i))
			  	{
					  selectDump(i, level);
					  break;
				}
			}
		  	for (i = gcr.startGlyph; i < CFF_.fi->nGlyphs; i++)
			{
			  	if(IsSynthetic(i))
					  if(selectDump(i, level))
					  	break;
			}  
			
		  }else if (gcr.byname){
		  	dumpByName();
		  		
		  }else{
		  	for (i = gcr.startGlyph; i < CFF_.fi->nGlyphs; i++)
			  if(selectDump(i, level))
			  	break;
		 }
	  else if (glyphs.size == 0)
		/* No glyphs specified so dump them all */
		for (i = 0; i < CFF_.fi->nGlyphs; i++)
		  selectDump(i, level);
	  else
		/* Dump specified glyphs */
		for (i = 0; i < glyphs.cnt; i++)
			selectDump(glyphs.array[i], level);
	  
	  if (level == 7)
		CFF_SynopsisFinish();
	  else if (level == 9)
	  {
	  	synopsis.vTile -= TILE_HEIGHT;
	  	if (gcr.reportNumber%gcr.numFonts==0)
		  	synopsis.vTile-=THIN_SPACE;
	  }

	  if (cffproofctx)
		{
		proofDestroyContext(&cffproofctx); /* set cffproofctx to null */
		}	
	  
	}

void CFF_Free(void)
	{
    if (!loaded)
		return;
	memFree(CFFbuffer);
	if (CFF_.cff)
	  cffFree(CFF_.cff); 
	CFF_.cff = NULL;
	memFree(workstr);	workstr = NULL;
	loaded = 0;
	unitsPerEm = 0;
	CFFbufflen = 0;
	}


IntX CFF_Loaded(void)
	{
	  return loaded;
	}


void CFF_ProofGlyph(GlyphId glyphId, void *myctx)
	{
	  if (myctx != NULL)
		{
		  CFFproofctx = (ProofContextPtr)myctx;
		  cffGetGlyphInfo(CFF_.cff, glyphId, &CFF_.proofcb);
		  if (justsawEC)
			proofGlyphClosePath((void *)CFFproofctx); /* removed & before CFFproofctx */
		  CFFproofctx = NULL;
		}
	}


void CFF_getMetrics(GlyphId glyphId, 
					IntX *origShift, 
					IntX *lsb, IntX *rsb, IntX *hwidth, 
					IntX *tsb, IntX *bsb, IntX *vwidth, IntX *yorig)
	{
	  cffGlyphInfo *cffgi = NULL;

	   *origShift = 0; /*This is needed only for TrueType fonts, where the xMin of in the glyf
	   							table glyph record may contain whitesepace,and this needs to be added to the left side bearing.*/

	  if (!loaded)
		{
		  if (sfntReadTable(CFF__))
			{
			  *hwidth = 0;
			  *vwidth = 0;
			  *lsb = 0;
			  *rsb = 0;
			  *tsb = 0;
			  *bsb = 0;
			  *origShift = 0;
			  if (yorig!=NULL)
				*yorig=DEFAULT_YORIG_KANJI;
			  return;
			}
		}

	  if (glyphId >= CFF_.fi->nGlyphs)
	  	glyphId = 0; /* avoid out-of-bounds */
	  	
	  cffgi = cffGetGlyphInfo(CFF_.cff, glyphId, NULL);

	  if (cffgi != NULL) {
	  	FWord ttsb;
		uFWord tvadv;
		Int16 vertOriginY;
		
		*hwidth = cffgi->hAdv;
		*vwidth = cffgi->vAdv;
		*lsb = cffgi->bbox.left;
		*rsb = *hwidth - cffgi->bbox.right;
		
			
		if (!vmtxGetMetrics(glyphId, &ttsb, &tvadv, CFF__)){
			*vwidth = tvadv;
			*tsb = ttsb;
			*bsb = 0; /* TT opnly */
			if (yorig!=NULL){
				if(!VORGGetVertOriginY(glyphId, &vertOriginY, CFF__)){
					*yorig=vertOriginY;
				}
				else{
					*yorig=ttsb+cffgi->bbox.top;
					VORGfound=0;
				}
			return;
			}
		}else
			if (yorig!=NULL)
				*yorig=DEFAULT_YORIG_KANJI;
		
		if (*vwidth == 0)
		  {
			/*if (vmtxGetMetrics(glyphId, &ttsb, &tvadv, CFF__))
			  {*/ /* ??? */
				*tsb = cffgi->bbox.top;
				*bsb = *vwidth - cffgi->bbox.bottom;
			  /*}
			else
			  {
				*vwidth = tvadv;
				*tsb = ttsb;
				*bsb = 0; 
			  }*/
		  }
		else if (*vwidth < 0) 
		  {
			*tsb = (- *vwidth) - cffgi->bbox.top;
			*bsb = cffgi->bbox.bottom;
		  }
		else 
		  {
			*tsb = cffgi->bbox.top;
			*bsb = *vwidth - cffgi->bbox.bottom;
		  }
		
		
	  }
	  else {
		*hwidth = 0;
		*vwidth = 0;
		*lsb = 0;
		*rsb = 0;
		*tsb = 0;
		*bsb = 0;
		*origShift = 0;
	  }
	}

IntX CFF_GetNGlyphs(Card16 *nGlyphs, Card32 client)
	{
	  if (!loaded)
		{
		  if (sfntReadTable(CFF__))
			return tableMissing(CFF__, client);
		}
	  *nGlyphs = CFF_.fi->nGlyphs;
	  return 0;
	}


IntX CFF_GetNMasters(Card16 *nMasters, Card32 client)
	{
	  if (!loaded)
		{
		  if (sfntReadTable(CFF__))
			return tableMissing(CFF__, client);
		}
	  *nMasters = CFF_.fi->mm.nMasters;
	  if (*nMasters < 1) *nMasters = 1;
	  return 0;
	}
	 
IntX CFF_GetBBox(Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax, Card32 client)
	{
	  if (!loaded)
		{
		  if (sfntReadTable(CFF__))
			{
			  *xMin = 0;
			  *yMin = 0;
			  *xMax = 0;
			  *yMax = 0;
			  return 1;
			}
		}
	  *xMin = CFF_.fi->FontBBox.left;
	  *yMin = CFF_.fi->FontBBox.bottom;
	  *xMax = CFF_.fi->FontBBox.right;
	  *yMax = CFF_.fi->FontBBox.top;
	  return 0;
	}


IntX CFF_InitName(void)
	{
	  if (loaded)  return 1;
  else
	{
	  if (sfntReadTable(CFF__))
		{
		  return 0;
		}
	  return 1;
	}
}

Byte8 *CFF_GetName(GlyphId glyphId, IntX *length, IntX forProofing)
	{
	  cffGlyphInfo *cffgi;
	  char *namptr = NULL;
	  IntX isstd;
	  long offset = (-1);
	  static Byte8 cidname[16];
	  int nglyphs =0;

	  if (!loaded)
		{
		  if (sfntReadTable(CFF__))
			{
			  *length = 0;
			  return NULL;
			}
		}

	  cffgi = cffGetGlyphInfo(CFF_.cff, glyphId, NULL);

	  nglyphs = CFF_.fi->nGlyphs;

	  if (cffgi != NULL) 
		{
		  if (CFF_isCID()) 
			{
			  if (nglyphs < 100)
				{
				  if (forProofing)
					sprintf(cidname,"\\\\%02hu", cffgi->id);
				  else
					sprintf(cidname,"\\%hu", cffgi->id);
				}
			  else if (nglyphs < 1000)
				{
				  if (forProofing)
					sprintf(cidname,"\\\\%03hu", cffgi->id);
				  else
					sprintf(cidname,"\\%hu", cffgi->id);
				}
			  else if (nglyphs < 10000)
				{
				  if (forProofing)
					sprintf(cidname,"\\\\%04hu", cffgi->id);
				  else
					sprintf(cidname,"\\%hu", cffgi->id);
				}
			  
			  else if (nglyphs < 100000)
				{
				  if (forProofing)
					sprintf(cidname,"\\\\%05hu", cffgi->id);
				  else
					sprintf(cidname,"\\%hu", cffgi->id);
				}
			  else
				{
				  if (forProofing)
					sprintf(cidname,"\\\\%hu", cffgi->id);
				  else
					sprintf(cidname,"\\%hu", cffgi->id);
				}
			  *length = strlen(cidname);
			  return (cidname);
			}

		  isstd = cffGetString(CFF_.cff, cffgi->id, (unsigned *)length, &namptr, &offset);
		  if (isstd == 1) 
			{
			  return namptr;
			}
		  else 
			{/* get the string from the CFF data itself */
			  long cnt;
			  namptr = CFFcffSeek(CFF_.cff, offset, &cnt);
			  return namptr;
			}
		}
	  else 
		{
		  *length = 0;
		  return NULL;
		}
	}


void CFF_Usage(void)
	{
	printf( "--- CFF_\n"

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
		   "=8  Proof alternate glyph synopsis\n"
		   "    Options: [-g<list>]\n"
		   "    -g  as above\n"
		   );
	}


