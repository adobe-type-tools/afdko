/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "pdfwrite.h"
#include "dynarr.h"
#include "supportexcept.h"

#if PLAT_SUN4
#include "sun4/fixtime.h"
#else /* PLAT_SUN4 */
#include <time.h>
#endif  /* PLAT_SUN4 */

#include <stdarg.h>
#include <string.h>
#include <math.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))

typedef struct
	{
	float x;
	float y;
	} Vector;

/* --------------------------------- Layout -------------------------------- */

/* Tile layout */
#define TILE_SIZE		35
#define TILE_COLS		16
#define TILE_ROWS		20
#define	TILES_PER_PAGE	(TILE_ROWS*TILE_COLS)
#define TILE_TOP		(TILE_ROWS*TILE_SIZE)
#define TILE_RIGHT		(TILE_COLS*TILE_SIZE)
#define TILE_TEXT_SIZE	5.0f
#define TILE_GLYPH_SIZE	24.0f

/* Page layout */
#define PAGE_ORIG_H		18.0f
#define PAGE_ORIG_V		18.0f
#define PAGE_RIGHT		TILE_RIGHT
#define PAGE_TOP		747
#define PAGE_TEXT_SIZE	10.0f

/* Metrics layout (2 columns: keys/values; 3 groups) */
#define MTX_VALUES_X	(PAGE_RIGHT - 54)
#define MTX_KEYS_X		(MTX_VALUES_X - 30)
#define MTX_LEADING		(PAGE_TEXT_SIZE + 1)
#define MTX_GROUP0_Y	(12*MTX_LEADING + PAGE_TEXT_SIZE)
#define MTX_GROUP1_Y	(8*MTX_LEADING + PAGE_TEXT_SIZE/2)
#define MTX_GROUP2_Y	(4*MTX_LEADING)

/* Coordinate labels */
#define COORD_TEXT_SIZE	4.0f

/* Outline glyph origin */
#define EM_ORIG_H		72.0
#define EM_ORIG_V		252.0

/* Map pts to em coordinate space */
#define TO_EM(v)	((float)(v)/h->glyph.scale)

/* Map em value to font coordinate space */
#define IN_EM(v)	((float)(v)*h->top->sup.UnitsPerEm)

/* Round value to specified number of digits of precision */
#define RND(p,v)	(floor((v)*10*(p)+0.5)/10.0*(p))

/* ------------------------------ Font Metrics ----------------------------- */

typedef struct			/* Font metrics (1000 units/em assumed) */
	{
	char *FontName;		/* FontName */
	short baseline;		/* Text baseline from em top */
	short capheight;
	short widths[128];	/* ASCII character widths */
	} Font;

enum
	{
	FONT_TEXT,			/* Text font */
	FONT_COORD,			/* Coord font */
	FONT_CNT
	};

/* Font metrics */
static Font fonts[FONT_CNT] =
	{
	{					/* [0] Text font */
	"Helvetica",
	755,
	718,
	{
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	228, 228, 291, 456, 456, 729, 547, 182, 
	273, 273, 319, 479, 228, 273, 228, 228, 
	456, 456, 456, 456, 456, 456, 456, 456, 
	456, 456, 228, 228, 479, 479, 479, 456, 
	832, 547, 547, 592, 592, 547, 501, 638, 
	592, 228, 410, 547, 456, 683, 592, 638, 
	547, 638, 592, 547, 501, 592, 547, 774, 
	547, 547, 501, 228, 228, 228, 385, 456,
	182, 456, 456, 410, 456, 456, 228, 456, 
	456, 182, 182, 410, 182, 683, 456, 456, 
	456, 456, 273, 410, 228, 456, 410, 592, 
	410, 410, 410, 274, 213, 274, 479, 0,
	},
	},
	{					/* [1] Coord font */
	"Courier",
	689,
	572,
	{
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 600, 
	600, 600, 600, 600, 600, 600, 600, 0, 
	},
	},
	};

/* ---------------------------- Library Context ---------------------------- */

typedef long OBJ;			/* Object number */
typedef dnaDCL(char, STM);	/* Object stream */

typedef struct				/* Hint layer */
	{
	OBJ hint;
	OBJ path;
	} Layer;

typedef struct				/* Per-glyph objects */
	{
	struct					/* Hint layers */
		{
		long cnt;			/* Number of layers */
		long index;			/* Index to start */
		} layer;
	struct					/* Palette tile objects */
		{
		OBJ mtx_value;
		OBJ annot;
		} tile;
	struct					/* Glyph page objects */
		{
		OBJ coords;
		OBJ tics;
		OBJ flex;
		OBJ links;
		OBJ mtx_value;
		OBJ obj;
		} page;
	OBJ bearings;			/* Sidebearing tics */
	} Glyph;

enum						/* Object streams */
	{
	STM_MISC,
	STM_PATH,
	STM_TICS,				/* Coord tics */
	STM_COORDS,				/* Coord text */
	STM_HINT,
	STM_FLEX,
	STM_LINKS,
	STM_CNT
	};

struct pdwCtx_
	{
	long flags;				/* Control flags */
	long level;				/* Dump level */
	abfTopDict *top;		/* Top Dict data */
	char *FontName;			/* Current FontName */
	char date[10];			/* Today's date: dd mmm yy */
	char time[6];			/* Current time: hh:mm */
	char pdfdate[24];		/* Today's date and time in PDF date format */
	struct					/* Document objects */
		{
		OBJ init_page;		/* Initialize page state */
		OBJ root;			/* Catolog */
		OBJ header;			/* Page header */
		OBJ info;			/* Document info */
		OBJ tile_mtx_key;	/* Tile metrics key */
		OBJ page_mtx_key;	/* Page metrics key */
		OBJ text_font;		/* Text font */
		OBJ coord_font;		/* Coord font */
		} obj;
	STM stms[STM_CNT];		/* Object streams */
	dnaDCL(long, xref);		/* Object offsets */
	dnaDCL(OBJ, pages);		/* Page objects */
	struct					/* Text parameters */
		{
		short iStm;			/* Text stream */
		short iFont;		/* Base font */
		float size;			/* Point size */
		float leading;		/* Line spacing */
		float x;			/* Current line x */
		float y;			/* Current line y */
		} text;
	struct					/* Path data */
		{
        float bx;	       	/* Last point */
        float by;	
        float cx;   	    /* Last but one point */
        float cy;
        float fx;       	/* First point in path */
        float fy;
        float sx;       	/* Second point in path */
        float sy;
        int cnt;        	/* Point count */
		int moves;
		int lines;
		int curves;
		} path;
	dnaDCL(Layer, layers);	/* Hint layers */
	dnaDCL(Glyph, glyphs);	/* Glyph data */
    struct              	/* Temorary glyph data */
        {
        float hAdv; 
		float scale;
		Glyph *rec;
		Layer *layer;
        } glyph;
	struct					/* Metric data */
        {
        struct abfMetricsCtx_ ctx;
        abfGlyphCallbacks cb;
        } metrics;
	struct
		{
		void *stm;			/* Destination stream */
		long start;			/* Start offset */
		} dst;
	struct					/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	dnaCtx dna;				/* dynarr context */
	struct					/* Error handling */
    {
		_Exc_Buf env;
    } err;
	};

static void writePDFBeg(pdwCtx h);
static void writePDFEnd(pdwCtx h);
static void writePages(pdwCtx h);

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(pdwCtx h, int err_code)
	{
  RAISE(&h->err.env, err_code, NULL);
	}

/* ---------------------- Error Handling dynarr Context -------------------- */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	pdwCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, pdwErrNoMemory);
	return ptr;
	}

/* Initialize error handling dynarr context. Return 1 on failure else 0. */
static int dna_init(pdwCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx = h;
	cb.manage = dna_manage;
	h->dna = dnaNew(&cb, DNA_CHECK_ARGS);
	return h->dna == NULL;
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
pdwCtx pdwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	pdwCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(PDW_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct pdwCtx_));
	if (h == NULL)
		return NULL;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Initialize service library */
	if (dna_init(h))
		{
		/* Initialize failed; clean up */
		mem_cb->manage(mem_cb, h, 0);
		return NULL;
		}

	dnaINIT(h->dna, h->xref, 1500, 6000);
	dnaINIT(h->dna, h->pages, 10, 50);
	dnaINIT(h->dna, h->layers, 750, 2500);
	dnaINIT(h->dna, h->glyphs, 250, 750);
	dnaINIT(h->dna, h->stms[STM_MISC], 200, 500);
	dnaINIT(h->dna, h->stms[STM_PATH], 200, 500);
	dnaINIT(h->dna, h->stms[STM_TICS], 200, 500);
	dnaINIT(h->dna, h->stms[STM_COORDS], 200, 500);
	dnaINIT(h->dna, h->stms[STM_HINT], 200, 500);
	dnaINIT(h->dna, h->stms[STM_FLEX], 200, 500);
	dnaINIT(h->dna, h->stms[STM_LINKS], 200, 500);

	return h;
	}

/* Free context. */
int pdwFree(pdwCtx h)
	{
	if (h == NULL)
		return pdwSuccess;

	/* Close destination stream */
	if (h->dst.stm != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->dst.stm);

	dnaFREE(h->xref);
	dnaFREE(h->pages);
	dnaFREE(h->layers);
	dnaFREE(h->glyphs);
	dnaFREE(h->stms[STM_MISC]);
	dnaFREE(h->stms[STM_PATH]);
	dnaFREE(h->stms[STM_TICS]);
	dnaFREE(h->stms[STM_COORDS]);
	dnaFREE(h->stms[STM_HINT]);
	dnaFREE(h->stms[STM_FLEX]);
	dnaFREE(h->stms[STM_LINKS]);

	dnaFree(h->dna);

	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);

	return pdwSuccess;
	}

/* Set time fields. */
static void settime(pdwCtx h)
	{
	time_t now = time(NULL);
	struct tm utc = *gmtime(&now);
	struct tm local = *localtime(&now);
	double tzoff = difftime(mktime(&utc), mktime(&local));
	long tzmins = (long)fabs(tzoff)/60;

	strftime(h->date, sizeof(h->date), "%d %b %y", &local);
	strftime(h->time, sizeof(h->time), "%H:%M", &local);

	strftime(h->pdfdate, sizeof(h->pdfdate), "D:%Y%m%d%H%M%S", &local);
	sprintf(&h->pdfdate[strlen(h->pdfdate)], "%c%02ld'%02ld'", 
			(tzoff == 0)? 'Z': (tzoff < 0)? '-': '+', tzmins/60L, tzmins%60L);
	}

/* Begin font. */
int pdwBegFont(pdwCtx h, long flags, long level, abfTopDict *top)
	{
	/* Set error handler */
  DURING_EX(h->err.env)

    /* Initialize */
    h->flags = flags;
    if (level < 0)
      h->level = 0;
    else if (level > 2)
      h->level = 2;
    else
      h->level = level;
    h->top = top;
    h->glyph.scale = 500.0f/top->sup.UnitsPerEm;
    settime(h);

    /* Set FontName */
    if (h->top->sup.flags & ABF_CID_FONT)
      h->FontName = h->top->cid.CIDFontName.ptr;
    else
      h->FontName = h->top->FDArray.array[0].FontName.ptr;
    if (h->FontName == ABF_UNSET_PTR)
      h->FontName = "<unknown>";

    if (h->level > 0)
      {
      /* Set up metrics facility */
      h->metrics.cb = abfGlyphMetricsCallbacks;
      h->metrics.cb.direct_ctx = &h->metrics.ctx;
      h->metrics.ctx.flags = 0;
      }

    /* Open dst stream and get start offset */
    h->dst.stm = h->cb.stm.open(&h->cb.stm, PDW_DST_STREAM_ID, 0);
    h->dst.start = h->cb.stm.tell(&h->cb.stm, h->dst.stm);
    if (h->dst.start == -1)
      return pdwErrDstStream;
      
    /* Initialize xref list */
    h->xref.cnt = 0;
    *dnaNEXT(h->xref) = 0;

    writePDFBeg(h);

	HANDLER
  	return Exception.Code;
  END_HANDLER

	return pdwSuccess;
	}

/* Finish reading font. */
int pdwEndFont(pdwCtx h)
	{
	/* Set error handler */
  DURING_EX(h->err.env)

    writePages(h);
    writePDFEnd(h);

    /* Close dst stream */
    if (h->cb.stm.close(&h->cb.stm, h->dst.stm) == -1)
      return pdwErrDstStream;

	HANDLER
  	return Exception.Code;
  END_HANDLER

	return pdwSuccess;
	}

/* Get version numbers of libraries. */
void pdwGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<PDW_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	abfGetVersion(cb);
	dnaGetVersion(cb);

	/* This library */
	cb->getversion(cb, PDW_VERSION, "pdfwrite");

	/* Record this call */
	cb->called |= 1<<PDW_LIB_ID;
	}

/* Map error code to error string. */
char *pdwErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "pdwerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

/* --------------------------- Destination Stream -------------------------- */

/* Return position of destination stream. */
static long dstTell(pdwCtx h)
	{
	long offset = h->cb.stm.tell(&h->cb.stm, h->dst.stm);
	if (offset == -1)
		fatal(h, pdwErrDstStream);
	return offset;
	}

/* Write buffer to dst stream. */
static void dstWrite(pdwCtx h, size_t count, char *ptr)
	{
	if (h->cb.stm.write(&h->cb.stm, h->dst.stm, count, ptr) != count)
		fatal(h, pdwErrDstStream);
	}

/* Write formatted string to dst stream. */
static void CTL_CDECL dstPrint(pdwCtx h, char *fmt, ...)
	{
	char buf[500];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	dstWrite(h, strlen(buf), buf);
	va_end(ap);
	}

/* Cross-reference object about to be written to stream and allocate number. */
static OBJ newObj(pdwCtx h)
	{
	OBJ num = h->xref.cnt;
	*dnaNEXT(h->xref) = dstTell(h) - h->dst.start;
	return num;
	}

/* Write beginning of object to dst stream. */
static long dstBegObj(pdwCtx h)
	{
	OBJ num = newObj(h);
	dstPrint(h, 
			 "%ld 0 obj\n"
			 "<<\n", 
			 num);
	return num;
	}

/* Write end of object to dst stream. */
static void dstEndObj(pdwCtx h)
	{
	dstPrint(h,
			 ">>\n"
			 "endobj\n");
	}

/* ------------------------- Stream Object Support ------------------------- */

/* Save formatted string to stream object. */
static void CTL_CDECL stmPrint(pdwCtx h, long iStm, char *fmt, ...)
	{
	char buf[500];
	long length;
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	length = (long)strlen(buf);
	memcpy(dnaEXTEND(h->stms[iStm], length), buf, length);
	}

/* Write stream object stream to dst stream. */
static long stmWrite(pdwCtx h, long iStm)
	{
	OBJ num;
	STM *stm = &h->stms[iStm];

	if (stm->cnt == 0)
		return -1;	/* Empty stream */

	num = newObj(h);
	dstPrint(h,
			 "%ld 0 obj\n"
			 "<< /Length %ld >>\n"
			 "stream\n",
			 num, stm->cnt);
	dstWrite(h, (size_t)stm->cnt, stm->array);
	dstPrint(h,
			 "endstream\n"
			 "endobj\n");

	stm->cnt = 0;	/* Reset stream */

	return num;
	}

/* ------------------------------ Text Support ----------------------------- */

/* Begin text object. No leading set if leading parameter is 0. */
static void textBeg(pdwCtx h, int iStm, int iFont, float size, float leading)
	{
	/* Select font */
	stmPrint(h, iStm,
			 "BT\n"
			 "/F%d %.2f Tf\n",
			 iFont, RND(1, size));

	stmPrint(h, iStm, "%.2f TL\n", RND(1, leading));
	stmPrint(h, iStm, "%d Tz\n", (iFont == FONT_TEXT)? 82: 100);

	/* Save text parameters */
	h->text.iStm = iStm;
	h->text.iFont = iFont;
	h->text.size = size;
	h->text.leading = leading;
	h->text.x = 0;
	h->text.y = 0;
	}

/* End text object. */
static void textEnd(pdwCtx h)
	{
	stmPrint(h, h->text.iStm, "ET\n");
	}	

/* Set text position. */
static void textSetPos(pdwCtx h, float x, float y)
	{
	y += h->text.leading;
	stmPrint(h, h->text.iStm, "%.2f %.2f Td\n", 
			 RND(1, x - h->text.x), RND(1, y - h->text.y));
	h->text.x = x;
	h->text.y = y;
	}

/* Show text string. */
static void CTL_CDECL textShow(pdwCtx h, char *fmt, ...)
	{
	char src[500];
	char dst[500];
	char *p;
	char *q;
	va_list ap;

	/* Format text */
	va_start(ap, fmt);
	vsprintf(src, fmt, ap);
	va_end(ap);

	/* Double backslashes */
	p = src;
	q = dst;
	for (;;)
		{
		int c = *p++;
		*q++ = c;
		if (c == '\0')
			break;
		else if (c == '\\')
			*q++ = c;
		}

	/* Show text */
	if (h->text.leading != 0)
		{
		h->text.y -= h->text.leading;
		stmPrint(h, h->text.iStm, "(%s)'\n", dst);
		}
	else
		stmPrint(h, h->text.iStm, "(%s)Tj\n", dst);
	}

/* Return text baseline from em top in current font. */
static float textBaseline(pdwCtx h)
	{
	Font *font = &fonts[h->text.iFont];
	return h->text.size*font->baseline/1000;
	}

/* Return text cap height in current font. */
static float textCapHeight(pdwCtx h)
	{
	Font *font = &fonts[h->text.iFont];
	return h->text.size*font->capheight/1000;
	}

/* Return text size in current font. */
static float textSize(pdwCtx h)
	{
	return h->text.size;
	}
	
/* Return length of text when shown in current font. */
static float textLength(pdwCtx h, char *text)
	{
	Font *font = &fonts[h->text.iFont];
	long total = 0;
	while (*text != '\0')
		total += font->widths[*text++ & 0x7f];
	return total*h->text.size/1000;
	}

/* ------------------------------- Glyph Draw ------------------------------ */

/* Draw glyph beginning. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	pdwCtx h = cb->direct_ctx;

	cb->info = info;

	h->glyph.rec = dnaNEXT(h->glyphs);
	h->glyph.rec->layer.index = h->layers.cnt;

	if (h->level > 0)
		{
		stmPrint(h, STM_TICS, "s\n");
		textBeg(h, STM_COORDS, FONT_COORD, TO_EM(COORD_TEXT_SIZE), 0);
		h->path.moves = 0;
		h->path.lines = 0;
		h->path.curves = 0;
		h->path.cnt = 0;

		h->metrics.cb.beg(&h->metrics.cb, info);

		h->glyph.rec->page.coords = 0;
		h->glyph.rec->page.tics = 0;
		h->glyph.rec->page.flex = 0;
		h->glyph.rec->page.links = 0;
		}

	return ABF_CONT_RET;
	}

/* Draw glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	pdwCtx h = cb->direct_ctx;

	h->glyph.hAdv = hAdv;
	if (h->level > 0)
		h->metrics.cb.width(&h->metrics.cb, hAdv);

	/* Write bearings object */
	stmPrint(h, STM_MISC,
			 "%.2f 0 m\n"
			 "0 0 l\n"
			 "0 %.2f l\n"
			 "%.2f 0 m\n"
			 "%.2f 0 l\n"
			 "%.2f %.2f l\n"
			 "S\n",
			 IN_EM(-0.03), 
			 IN_EM(-0.03),
			 hAdv + IN_EM(0.03), 
			 hAdv, 
			 hAdv, IN_EM(-0.03));
	h->glyph.rec->bearings = stmWrite(h, STM_MISC);
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
static void savePt(pdwCtx h, float x, float y)
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

/* Draw coordinate for last point. */
static void drawCoord(pdwCtx h, float ax, float ay)
	{
	if (h->level == 0)
		return;

	if (h->path.cnt >= 2)
		{
		float x;
		float y;
		char buf[15];

		/* Make vector 90 degress counter clockwise to a->c */
		Vector s = makeVec(h->path.bx - ax, h->path.by - ay);
		Vector t = makeVec(h->path.cx - h->path.bx, h->path.cy - h->path.by);
		Vector u = makeVec(s.x + t.x, s.y + t.y);
		Vector v;
		v.x = -u.y*TO_EM(5.5);
		v.y = u.x*TO_EM(5.5);

		if (h->flags & PDW_FLIP_TICS)
			{
			v.x = -v.x;
			v.y = -v.y;
			}

		/* Set off-curve end of tic */
		x = (float)RND(1, h->path.bx + v.x);
		y = (float)RND(1, h->path.by + v.y);

		/* Draw tic */
		stmPrint(h, STM_TICS,
				 "%.2f %.2f m\n"
				 "%.2f %.2f l\n",
				 h->path.bx, h->path.by,
				 x, y);

		/* Make label */
		sprintf(buf, "%.0f %.0f", h->path.bx, h->path.by);

		/* Adjust position for quadrant */
		if (v.x < 0 && v.y >= 0)
			x -= textLength(h, buf);
		else if (v.x <= 0 && v.y < 0)
			{
			x -= textLength(h, buf);
			y -= textBaseline(h);
			}
		else if (v.x > 0 && v.y <= 0)
			y -= textBaseline(h);

		/* Show label */
		textSetPos(h, x, y);
		textShow(h, buf);
		}

	savePt(h, ax, ay);
	}

/* Draw circle of radius r center x,y using 4 bezier curves. */
static void drawCircle(pdwCtx h, int iStm, float x, float y, float r)
	{
	double left 	= RND(1, x - r);
	double bottom 	= RND(1, y - r);
	double right 	= RND(1, x + r);
	double top 		= RND(1, y + r);
	double l 		= r*0.552285;	/* Control arm length */
	double xp		= RND(1, x + l);
	double xm		= RND(1, x - 1);
	double yp		= RND(1, y + l);
	double ym		= RND(1, y - l);
	stmPrint(h, iStm,
			 "%.2f %.2f m\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
			 "h\n",
			 x, top,
			 xp, top, right, yp, right, y,
			 right, ym, xp, bottom, x, bottom,
			 xm, bottom, left, ym, left, y,
			 left, yp, xm, top, x, top);
	}

/* Draw closepath. */
static void drawClose(pdwCtx h, int closepoint)
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

	/* Draw close arrow */
	stmPrint(h, STM_TICS,
			 "f\n"
			 "q\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f cm\n"
			 "0 0 m\n"
			 "%.2f %.2f l\n"
			 "%.2f %.2f l\n"
			 "h\n",
			 dx/l, dy/l, -dy/l, dx/l, h->path.fx, h->path.fy,
			 TO_EM(-6), TO_EM(-1.5),
			 TO_EM(-6), TO_EM(1.5));

	if (closepoint)
		drawCircle(h, STM_TICS, 0, 0, TO_EM(1));

	stmPrint(h, STM_TICS,
			 "f\n"
			 "Q\n");

	stmPrint(h, STM_PATH, "h\n");
	}

/* End current path. */
static void endPath(pdwCtx h)
	{
	if (h->level == 0 || h->path.moves == 0)
		return;

	if (h->path.bx == h->path.fx && h->path.by == h->path.fy)
		drawClose(h, 1);
	else
		{
		drawClose(h, 0);
		drawCoord(h, h->path.fx, h->path.fy);
		h->path.lines++;
		}
	drawCoord(h, h->path.sx, h->path.sy);
	}

/* Draw glyph move. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	pdwCtx h = cb->direct_ctx;

	if (h->path.moves != 0)
		endPath(h);

	stmPrint(h, STM_PATH, "%.2f %.2f m\n", x0, y0);

	h->path.cnt = 1;
	if (h->level > 0)
		{
		/* Initialize point accumulator */
		h->path.bx = h->path.fx = x0;
		h->path.by = h->path.fy = y0;
		h->metrics.cb.move(&h->metrics.cb, x0, y0);
		}

	h->path.moves++;
	}

/* Draw control point. */
static void drawCntlPt(pdwCtx h, float x, float y)
	{
	float side;

	if (h->level == 0)
		return;

	side = (float)RND(1, TO_EM(0.6));
	stmPrint(h, STM_TICS,
			 "%.2f %.2f %.2f %.2f re\n",
			 RND(1, x - TO_EM(0.3)), RND(1, y - TO_EM(0.3)),
			 side, side);
	}

/* Draw glyph line. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	pdwCtx h = cb->direct_ctx;

	stmPrint(h, STM_PATH, "%.2f %.2f l\n", x1, y1);	
	drawCoord(h, x1, y1);

	if (h->level > 0)
		h->metrics.cb.line(&h->metrics.cb, x1, y1);

	h->path.lines++;
	}

/* Draw glyph curve. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	pdwCtx h = cb->direct_ctx;

	stmPrint(h, STM_PATH, "%.2f %.2f %.2f %.2f %.2f %.2f c\n", x1, y1, x2, y2, x3, y3);

	drawCntlPt(h, x1, y1);
	drawCntlPt(h, x2, y2);

	drawCoord(h, x1, y1);
	savePt(h, x2, y2);
	savePt(h, x3, y3);

	if (h->level > 0)
		h->metrics.cb.curve(&h->metrics.cb, x1, y1, x2, y2, x3, y3);

	h->path.curves++;
	}

/* Draw horizontal stem. */
static void drawHStem(pdwCtx h, 
					  float left, float bottom, float right, float top)
	{
	}

/* Draw vertical stem. */
static void drawVStem(pdwCtx h, 
					  float left, float bottom, float right, float top)
	{
	if (left == right)
		stmPrint(h, STM_MISC, 
				 "%.2f %.2f m\n"
				 "%.2f %.2f l\n",
				 left, bottom, left, top);
	else
		stmPrint(h, STM_MISC,
				 "%.2f %.2f %.2f %.2f re\n",
				 left, bottom, right - left, top - bottom);
	}

/* Draw horizontal stem coordinates. */
static void drawHCoords(pdwCtx h, float left, float top, float bottom)
	{
	char buf[20];
	float baseshift = -textCapHeight(h)/2;
		
	sprintf(buf, "%.2f", top);
	left -= textLength(h, buf);
	textSetPos(h, left, top + baseshift);
	textShow(h, buf);

	if (bottom == top)
		return;

	sprintf(buf, "%.2f", bottom);
	left -= textLength(h, buf);
	textSetPos(h, left, bottom + baseshift);
	textShow(h, buf);
	}

/* Draw glyph stem. */
static void glyphStem(abfGlyphCallbacks *cb,
					  int flags, float edge0, float edge1)
	{
	pdwCtx h = cb->direct_ctx;
	float left;
	float bottom;
	float right;
	float top;

	if (h->level < 2)
		return;

	if (flags & ABF_VERT_STEM)
		{
		left = edge0;
		bottom = TO_EM(24);
		right = edge1;
		top = TO_EM(TILE_TOP - 24);

		}
	else
		{
		/* Draw horizontal stem */
		left = TO_EM(24 - EM_ORIG_H);
		bottom = edge0;
		right = TO_EM(PAGE_RIGHT - EM_ORIG_H - 48);
		top = edge1;

		if (bottom == top)
			stmPrint(h, STM_HINT,
					 "%.2f %.2f m\n"
					 "%.2f %.2f l\n",
					 left, top, right, top);
		else
			stmPrint(h, STM_MISC,
					 "%.2f %.2f %.2f %.2f re\n",
					 left, bottom, right - left, top - bottom);
		}
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
	pdwCtx h = cb->direct_ctx;

	glyphCurve(cb, x1, y1, x2, y2, x3, y3);
	glyphCurve(cb, x4, y4, x5, y5, x6, y6);

	if (h->level < 2)
		return;

	stmPrint(h, STM_FLEX,
			 "%.2f %.2f m\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
			 "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			 h->path.bx, h->path.by,
			 x1, y1, x2, y2, x3, y3,
			 x4, y4, x5, y5, x6, y6);
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

/* Make encoding string. Return buffer argument. */
static char *makeEncStr(pdwCtx h, char *buf, abfGlyphInfo *info)
	{
	abfEncoding *enc = &info->encoding;
	if (enc->code == ABF_GLYPH_UNENC)
		sprintf(buf, "-");
	else
		{
		char *p = buf;
		char *sep = "";
		do
			{
			if (info->flags & ABF_GLYPH_UNICODE)
				{
				if (enc->code < 0x10000)
					sprintf(p, "%sU+%04lX", sep, enc->code);
				else
					sprintf(p, "%sU+%lX", sep, enc->code);
				}
			else
				sprintf(p, "%sx%02lX", sep, enc->code);
			p += strlen(p);
			sep = "+";
			enc = enc->next;
			}
		while (enc != NULL);
		}
	return buf;
	}

/* Draw tile box and labels to stream. The x and y parameters specify the 
   top left corner of the tile. */
static void drawTile(pdwCtx h, int iStm,
					 float x, float y, char *tag, char *hAdv, char *gname)
	{
	float left;
	float right;
	float top;
	float bottom;

	/* Draw tile rectangle */
	stmPrint(h, iStm, 
			 "%.2f %.2f %d %d re\n"
			 "s\n",
			 x, y - TILE_SIZE, TILE_SIZE, TILE_SIZE);

	/* Show labels */
	textBeg(h, iStm, FONT_TEXT, TILE_TEXT_SIZE, TILE_TEXT_SIZE);

	left = x + 1;
	right = x + TILE_SIZE - 1;
	top = y - 1 - textBaseline(h);
	bottom = y - TILE_SIZE + 0.2f + textSize(h) - textBaseline(h);

	textSetPos(h, left, top);
	textShow(h, tag);

	textSetPos(h, left, bottom);
	textShow(h, gname);

	textSetPos(h, right - textLength(h, hAdv), top);
	textShow(h, hAdv);

	textEnd(h);
	}

/* Write tile metrics value object. */
static long writeTileMtxValueObj(pdwCtx h, abfGlyphInfo *info)
	{
	char tag[20];
	char enc[50];
	char hAdv[20];
	char gname[127];
	long iTile = (h->glyphs.cnt - 1)%TILES_PER_PAGE;
	float x = (float)(iTile%TILE_COLS*TILE_SIZE);
	float y = (float)(TILE_ROWS - iTile/TILE_COLS)*TILE_SIZE;
	float scale = TILE_GLYPH_SIZE/h->top->sup.UnitsPerEm;

	/* Set glyph name */
	if (info->flags & ABF_GLYPH_CID)
		{
		sprintf(tag, "%hu,%u", info->tag, info->iFD);
		sprintf(gname, "\\%hu", info->cid);
		}
	else
		{
		sprintf(tag, "%hu,%s", info->tag, makeEncStr(h, enc, info));
		sprintf(gname, "%s", info->gname.ptr);
		}
	sprintf(hAdv, "%.2f", h->glyph.hAdv);

	drawTile(h, STM_MISC, x, y, tag, hAdv, gname);
	stmPrint(h, STM_MISC, 
			 "q\n"
			 "%.2f 0 0 %.2f %.2f %.2f cm\n",
			 scale, scale,
			 RND(1, x + (TILE_SIZE - TILE_GLYPH_SIZE*
						   h->glyph.hAdv/h->top->sup.UnitsPerEm)/2),
			 RND(1, y - TILE_SIZE*0.7));
	return stmWrite(h, STM_MISC);
	}

/* Write page metrics value object. */
static long writePageMtxValueObj(pdwCtx h, abfGlyphInfo *info)
	{
	char buf[50];

	textBeg(h, STM_MISC, FONT_TEXT, PAGE_TEXT_SIZE, MTX_LEADING);

	textSetPos(h, MTX_VALUES_X, MTX_GROUP0_Y);
	if (info->flags & ABF_GLYPH_CID)
		textShow(h, "\\%hu", info->cid);
	else
		textShow(h, info->gname.ptr);
	textShow(h, "%hd", info->tag);
	if (info->flags & ABF_GLYPH_CID)
		textShow(h, "%u", info->iFD);
	else
		textShow(h, makeEncStr(h, buf, info));
	textShow(h, "%g", h->glyph.hAdv);

	textSetPos(h, MTX_VALUES_X, MTX_GROUP1_Y);
	textShow(h, "%ld", h->metrics.ctx.int_mtx.left);
	textShow(h, "%ld", h->metrics.ctx.int_mtx.bottom);
	textShow(h, "%ld", h->metrics.ctx.int_mtx.right);
	textShow(h, "%ld", h->metrics.ctx.int_mtx.top);

	textSetPos(h, MTX_VALUES_X, MTX_GROUP2_Y);
	textShow(h, "%d", h->path.moves);
	textShow(h, "%d", h->path.lines);
	textShow(h, "%d", h->path.curves);
	textShow(h, "%d", h->path.moves + h->path.lines + h->path.curves);

	textEnd(h);

	return stmWrite(h, STM_MISC);
	}

/* Save hint layer. */
static void saveLayer(pdwCtx h)
	{
	Layer *layer;

	if (h->stms[STM_HINT].cnt == 0 && h->stms[STM_PATH].cnt == 0)
		return;
		
	layer = dnaNEXT(h->layers);
	layer->hint = stmWrite(h, STM_HINT);
	layer->path = stmWrite(h, STM_PATH);
	}

/* Draw glyph end. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	pdwCtx h = cb->direct_ctx;
	Glyph *glyph = h->glyph.rec;

	endPath(h);

	if (h->path.cnt == 0)
		h->stms[STM_TICS].cnt = 0;
	else
		stmPrint(h, STM_TICS, "S\n");

	if (h->level > 0)
		textEnd(h);	/* End coord text */

	saveLayer(h);

	/* Save glyph objects */
	glyph->layer.cnt		= h->layers.cnt - h->glyph.rec->layer.index;
	glyph->tile.mtx_value	= writeTileMtxValueObj(h, cb->info);
	glyph->page.coords		= stmWrite(h, STM_COORDS);
	glyph->page.tics		= stmWrite(h, STM_TICS);
	glyph->page.flex		= stmWrite(h, STM_FLEX);
	/* page.links */
	if (h->level > 0)
		{
		h->metrics.cb.end(&h->metrics.cb);
		glyph->page.mtx_value = writePageMtxValueObj(h, cb->info);
		}
	}

/* Draw glyph callbacks */
const abfGlyphCallbacks pdwGlyphCallbacks =
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

/* ------------------------------------------------- */

/* Write page initialization object. */
static long writeInitPageObj(pdwCtx h)
	{
	stmPrint(h, STM_MISC,
			 "1 0 0 1 %g %g cm\n"
			 "0 w\n",
			 PAGE_ORIG_H, PAGE_ORIG_V);
	return stmWrite(h, STM_MISC);
	}

/* Write info object. */
static long writeInfoObj(pdwCtx h)
	{
	enum { MAX_VERSION_SIZE = 100 };
	char version[MAX_VERSION_SIZE + 1];
	char version_buf[MAX_VERSION_SIZE];
	OBJ num;

	/* Make font version */
	if (h->top->sup.flags & ABF_CID_FONT)
		sprintf(version, "%g", h->top->cid.CIDFontVersion);
	else if (h->top->version.ptr != ABF_UNSET_PTR)
		{
		char format[20];
		sprintf(format, "%%.%ds", MAX_VERSION_SIZE);
		sprintf(version, format, h->top->version.ptr);
		}
	else
		version[0] = '\0';

	/* Write information dictionary */
	num = dstBegObj(h);
	dstPrint(h, 
			 "/Title (%s %s)\n"
			 "/Creator (absfont %s)\n" 
			 "/Producer (pdfwrite %s)\n"
			 "/CreationDate (%s)\n"
			 "/ModDate (%s)\n",
			 h->FontName, version,
			 CTL_SPLIT_VERSION(version_buf, ABF_VERSION),
			 CTL_SPLIT_VERSION(version_buf, PDW_VERSION),
			 h->pdfdate,
			 h->pdfdate);
	dstEndObj(h);

	return num;
	}

/* Write font object. Return object number. */
static long writeFontObj(pdwCtx h, int iFont)
	{
	OBJ num = dstBegObj(h);
	dstPrint(h,
			 "/Type /Font\n"
			 "/Subtype /Type1\n"
			 "/BaseFont /%s\n",
			 fonts[iFont].FontName);
	dstEndObj(h);
	return num;
	}

/* Write header object used by every page. */
static long writeHeaderObj(pdwCtx h)
	{
	char *p;
	float y;
	char buf[30];

	textBeg(h, STM_MISC, FONT_TEXT, PAGE_TEXT_SIZE, PAGE_TEXT_SIZE);
	y =	PAGE_TOP - textBaseline(h);

	/* Show filename */
	if (h->top->sup.filename == ABF_UNSET_PTR)
		p = "<unknown>";
	else
		p = h->top->sup.filename;
	textSetPos(h, 0, y);
	textShow(h, "Filename:  %s", p);

	/* Show FontName */
	textShow(h, "FontName:  %s", h->FontName);

	/* Show units-per-em */
	textShow(h, "Em:  %ld units", h->top->sup.UnitsPerEm);

	/* Show date */
	sprintf(buf, "Date:  %s", h->date);
	textSetPos(h, PAGE_RIGHT - textLength(h, buf), y);
	textShow(h, buf);

	/* Show time */
	y -= textSize(h);
	sprintf(buf, "Time:  %s", h->time); 
	textSetPos(h, PAGE_RIGHT - textLength(h, buf), y);
	textShow(h, buf);

	textEnd(h);

	return stmWrite(h, STM_MISC);
	}

/* Write palette tile key object. */
static long writeTileKeyObj(pdwCtx h)
	{
	float x = (TILE_COLS - 3)*TILE_SIZE;
	float y = PAGE_TOP;

	if (h->top->sup.flags & ABF_CID_FONT)
		drawTile(h, STM_MISC, x, y, "tag,fd", "hAdv", "cid");
	else
		drawTile(h, STM_MISC, x, y, "tag,enc", "hAdv", "gname");

    return stmWrite(h, STM_MISC);
	}

/* Write metrics key object. */
static long writeMtxKeyObj(pdwCtx h)
	{
	textBeg(h, STM_MISC, FONT_TEXT, PAGE_TEXT_SIZE, MTX_LEADING);

	textSetPos(h, MTX_KEYS_X, MTX_GROUP0_Y);
	textShow(h, "glyph");
	textShow(h, "tag");
	textShow(h, "enc");
	textShow(h, "hAdv");

	textSetPos(h, MTX_KEYS_X, MTX_GROUP1_Y);
	textShow(h, "left");
	textShow(h, "bottom");
	textShow(h, "right");
	textShow(h, "top");

	textSetPos(h, MTX_KEYS_X, MTX_GROUP2_Y);
	textShow(h, "moves");
	textShow(h, "lines");
	textShow(h, "curves");
	textShow(h, "total");

	textEnd(h);

	return stmWrite(h, STM_MISC);
	}

/* Write beginning PDF file. */
static void writePDFBeg(pdwCtx h)
	{
	dstPrint(h, "%%PDF-1.1\n");

	h->obj.init_page	= writeInitPageObj(h);
	h->obj.info 		= writeInfoObj(h);
	h->obj.text_font 	= writeFontObj(h, FONT_TEXT);
	h->obj.header		= writeHeaderObj(h);
	h->obj.tile_mtx_key = writeTileKeyObj(h);
	if (h->level > 0)
		{
		h->obj.coord_font = writeFontObj(h, FONT_COORD);
		h->obj.page_mtx_key = writeMtxKeyObj(h);
		}
	}

/* Write end of PDF file (xref + trailer). */
static void writePDFEnd(pdwCtx h)
	{
	long i;
	long offset = dstTell(h) - h->dst.start;	

	/* Write xref table */
	dstPrint(h, 
			 "xref\n"
			 "0 %ld\n"
			 "0000000000 65535 f \n",
			 h->xref.cnt);
	for (i = 1; i < h->xref.cnt; i++)
		dstPrint(h, "%010ld 00000 n \n", h->xref.array[i]);

	/* Write trailer */
	dstPrint(h, 
			 "trailer\n"
			 "<<\n"
			 "/Size %ld\n"
			 "/Root %ld 0 R\n"
			 "/Info %ld 0 R\n"
			 ">>\n",
			 h->xref.cnt, 
			 h->obj.root,
			 h->obj.info);

	/* Write tail */
	dstPrint(h,
			 "startxref\n"
			 "%ld\n"
			 "%%%%EOF\n",
			 offset);
	}

/* Write catalog object. Return object number. */
static OBJ writeCatalogObj(pdwCtx h, OBJ pages)
	{
	OBJ num = dstBegObj(h);
	dstPrint(h,
			 "/Type /Catalog\n"
			 "/Pages %ld 0 R\n",
			 pages);
	dstEndObj(h);
	return num;
	}

/* Write annotation object. */
static OBJ writeAnnotObj(pdwCtx h, OBJ page,
						 float left, float bottom, 
						 float right, float top)
	{
	OBJ num = dstBegObj(h);
	dstPrint(h, 
			 "/Type /Annot\n"
			 "/Subtype /Link\n"
			 "/Border [0 0 0]\n"
			 "/Rect [%g %g %g %g]\n"
			 "/Dest [%ld 0 R /Fit]\n",
			 left, bottom, right, top,
			 page);
	dstEndObj(h);
	return num;
	}

/* Write tile annotation dictionaries. */
static void writeTileAnnots(pdwCtx h, long iBase, long iNext)
	{
	long i;
	for (i = iBase; i < iNext; i++)
		{
		Glyph *glyph = &h->glyphs.array[i];
		long iTile = i - iBase;
		float x = iTile%TILE_COLS*TILE_SIZE + PAGE_ORIG_H;
		float y = (TILE_ROWS - iTile/TILE_COLS)*TILE_SIZE + PAGE_ORIG_V;
		glyph->tile.annot = writeAnnotObj(h, glyph->page.obj, 
										  x, y - TILE_SIZE, x + TILE_SIZE, y);
		}
	}

/* Write object reference to dst stream. */
static void writeObjRef(pdwCtx h, OBJ obj)
	{
	dstPrint(h, "%ld 0 R\n", obj);
	}

/* Write palette pages. */
static void writePalettePages(pdwCtx h, OBJ parent)
	{
	OBJ fQ_obj;
	long i;
	long iBase;
	long pages = h->glyphs.cnt/TILES_PER_PAGE + 1;

	dnaSET_CNT(h->pages, pages);

	/* Write fill/restore object */
	stmPrint(h, STM_MISC,
			 "f\n"
			 "Q\n");
	fQ_obj = stmWrite(h, STM_MISC);
	
	iBase = 0;
	for (i = 0; i < pages; i++)
		{
		long j;
		long iNext = iBase + TILES_PER_PAGE;
		if (iNext > h->glyphs.cnt)
			iNext = h->glyphs.cnt;

		if (h->level > 0)
			writeTileAnnots(h, iBase, iNext);

		/* Begin page object */
		h->pages.array[i] = dstBegObj(h);
		dstPrint(h, 
				 "/Type /Page\n"
				 "/Parent %ld 0 R\n"
				 "/Contents [\n",
				 parent);

		/* Write contents objects */
		writeObjRef(h, h->obj.init_page);
		writeObjRef(h, h->obj.header);
		writeObjRef(h, h->obj.tile_mtx_key);

		for (j = iBase; j < iNext; j++)
			{
			long iLayer;
			long k;
			Glyph *glyph = &h->glyphs.array[j];

			writeObjRef(h, glyph->tile.mtx_value);
			writeObjRef(h, glyph->bearings);

			iLayer = glyph->layer.index;
			for (k = 0; k < glyph->layer.cnt; k++)
				writeObjRef(h, h->layers.array[iLayer++].path);

			writeObjRef(h, fQ_obj);
			}

		dstPrint(h, "]\n");

		if (h->level > 0)
			{
			/* Add annotation array */
			dstPrint(h, "/Annots [\n");
			for (j = iBase; j < iNext; j++)
				writeObjRef(h, h->glyphs.array[j].tile.annot);
			dstPrint(h, "]\n");
			}

		/* End page object */
		dstEndObj(h);

		iBase = iNext;
		}
	}

#if 0
/* Draw zone. */
static void drawZone(pdwCtx h, 
					 float left, float bottom, float right, float top, 
					 float fuzz, float baseshift)
	{
	float x;
	char buf[20];
	
	/* Draw fuzz */
	stmPrint(h, STM_HINT_STEMS,
			 "%g %g m\n"
			 "%g %g l\n"
			 "%g %g m\n"
			 "%g %g l\n",
			 left, top + fuzz, 
			 right, top + fuzz,
			 left, bottom - fuzz, 
			 right, bottom - fuzz);

	/* Draw zone */
	if (bottom == top)
		stmPrint(h, STM_HINT_STEMS,
				 "%g %g m\n"
				 "%g %g l\n",
				 left, top, 
				 right, top);
	else
		stmPrint(h, STM_HINT_STEMS,
				 "%g %g %g %g re\n",
				 left, bottom, right - left, top - bottom);

	sprintf(buf, "%g", top);
	textSetPos(h, right, top + baseshift);
	textShow(h, buf);

	if (bottom == top)
		return;

	x = right + textLength(h, buf);

	sprintf(buf, "%g", bottom);
	textSetPos(h, x, bottom + baseshift);
	textShow(h, buf);
	}

/* Write hint zone object. */
static OBJ writeZoneObj(pdwCtx h, long iFD)
	{
	abfPrivateDict *private = &h->top->FDArray.array[iFD].Private;

	
	for (i = 0; i < private->OtherBlues.cnt; i += 2)
		drawHStem(h, 
				  left, private->OtherBlues.array[i + 0],
				  right, private->OtherBlues.array[i + 1]);
	if (private->BlueValues.cnt > 0)
		drawHStem(h, 
				  left, private->BlueValues.array[i + 0],
				  right, private->BlueValues.array[i + 1]);

	



	if (private->BlueValues.cnt != 0)
		{
		drawZones(h, 2, private->BlueValues.array, private->BlueFuzz, 1);
		drawZones(h, private->BlueValues.cnt - 2, 
				  &private->BlueValues.array[2], private->BlueFuzz, 0);
		}
	drawZones(h, private->OtherBlues.cnt, private->OtherBlues.array,
			  private->BlueFuzz, 1);

	return stmWrite(h, STM_MISC);
	}
#endif

/* Draw zones. */
static void drawZones(pdwCtx h, 
					  long cnt, float *array, float fuzz, int bottom_zone)
	{
	long i;
	float baseshift;
	float left;
	float right;

	if (cnt == 0)
		return;

	left = TO_EM(-EM_ORIG_H);
	right = TO_EM(PAGE_RIGHT - EM_ORIG_H - 24);

	/* Draw zones */
	for (i = 0; i < cnt; i += 2)
		{
		float bottom = array[i + 0];
		float top = array[i + 1];

		/* Draw fuzz */
		stmPrint(h, STM_MISC,
				 "%g %g m\n"
				 "%g %g l\n"
				 "%g %g m\n"
				 "%g %g l\n",
				 left, top + fuzz, right, top + fuzz,
				 left, bottom - fuzz, right, bottom - fuzz);

		/* Draw zone */
		if (bottom == top)
			stmPrint(h, STM_MISC, 
					 "%g %g m\n"
					 "%g %g l\n",
					 left, top, right, top);
		else
			stmPrint(h, STM_MISC,
					 "%g %g %g %g re\n",
					 left, bottom, right - left, top - bottom);
		}

	if (bottom_zone)
		stmPrint(h, STM_MISC, "0 .6 1 rg\n");
	else
		stmPrint(h, STM_MISC, "0 .8 1 rg\n");

	/* Draw labels */
	stmPrint(h, STM_MISC, 
			 "f\n"
			 "0 0 0 rg\n");

	textBeg(h, STM_MISC, FONT_COORD, TO_EM(COORD_TEXT_SIZE), 0);
	baseshift = -textCapHeight(h)/2;

	for (i = 0; i < cnt; i += 2)
		{
		float x;
		char buf[20];
		float bottom = array[i + 0];
		float top = array[i + 1];
		
		sprintf(buf, "%g", top);
		textSetPos(h, right, top + baseshift);
		textShow(h, buf);

		if (bottom == top)
			continue;

		x = right + textLength(h, buf);

		sprintf(buf, "%g", bottom);
		textSetPos(h, x, bottom + baseshift);
		textShow(h, buf);
		}

	textEnd(h);
	}

/* Write hint zone object. */
static OBJ writeZoneObj(pdwCtx h, long iFD)
	{
	abfPrivateDict *private = &h->top->FDArray.array[iFD].Private;

#if 0	
	for (i = 0; i < private->OtherBlues.cnt; i += 2)
		drawHStem(h, 
				  left, private->OtherBlues.array[i + 0],
				  right, private->OtherBlues.array[i + 1]);
	if (private->BlueValues.cnt > 0)
		drawHStem(h, 
				  left, private->BlueValues.array[i + 0],
				  right, private->BlueValues.array[i + 1]);
#endif
	
	if (private->BlueValues.cnt != 0)
		{
		drawZones(h, 2, private->BlueValues.array, private->BlueFuzz, 1);
		drawZones(h, private->BlueValues.cnt - 2, 
				  &private->BlueValues.array[2], private->BlueFuzz, 0);
		}
	drawZones(h, private->OtherBlues.cnt, private->OtherBlues.array,
			  private->BlueFuzz, 1);

	return stmWrite(h, STM_MISC);
	}

/* Write glyph pages. */
static void writeGlyphPages(pdwCtx h, OBJ parent)
	{
	OBJ zone;
	OBJ transform_obj;
	long i;

	if (h->level == 0)
		return;

	zone = writeZoneObj(h, 0);

	/* Write path transformation object */
	stmPrint(h, STM_MISC,
			 "%g 0 0 %g %g %g cm\n",
			 h->glyph.scale, h->glyph.scale,
			 EM_ORIG_H, EM_ORIG_V);
	transform_obj = stmWrite(h, STM_MISC);
	
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		Glyph *glyph = &h->glyphs.array[i];

		/* Begin page object */
		glyph->page.obj = dstBegObj(h);
		dstPrint(h, 
				 "/Type /Page\n"
				 "/Parent %ld 0 R\n"
				 "/Contents [\n",
				 parent);

		/* Write contents objects */
		writeObjRef(h, h->obj.init_page);
		writeObjRef(h, h->obj.header);
		writeObjRef(h, h->obj.page_mtx_key);
		writeObjRef(h, glyph->page.mtx_value); 
		writeObjRef(h, transform_obj);
		if (zone != -1)
			writeObjRef(h, zone);
		writeObjRef(h, glyph->bearings);

		if (glyph->layer.cnt > 0)
			{
			/* Marking glyph; draw path, tics, and coords */
			long j;
			long iLayer = glyph->layer.index;
			
			for (j = 0; j < glyph->layer.cnt; j++)
				writeObjRef(h, h->layers.array[iLayer++].path);

			writeObjRef(h, glyph->page.tics);
			writeObjRef(h, glyph->page.coords);
			}

		/* End page object */
		dstPrint(h, "]\n");
		dstEndObj(h);
		}
	}

/* Write page objects. */
static void writePages(pdwCtx h)
	{
	long i;
	long pagecnt;
	OBJ pages;

	/* Get pages object number and allocate dummy xref */
	pages = h->xref.cnt;
	*dnaNEXT(h->xref) = 0;

	writeGlyphPages(h, pages);
	writePalettePages(h, pages);

	/* Replace dummy xref by file offset */
	h->xref.array[pages] = dstTell(h) - h->dst.start;

	/* Write pages object */
	dstPrint(h, 
			 "%ld 0 obj\n"
			 "<<\n"
			 "/Type /Pages\n"
			 "/MediaBox [0 0 612 792]\n"
			 "/Resources <<\n"
			 "/ProcSet [/PDF /Text]\n"
			 "/Font <<\n"
			 "/F0 %ld 0 R\n",
			 pages,
			 h->obj.text_font);

	pagecnt = h->pages.cnt;
	if (h->level > 0)
		{
		dstPrint(h, "/F1 %ld 0 R\n", h->obj.coord_font);
		pagecnt += h->glyphs.cnt;
		}

	dstPrint(h, 
			 ">>\n"
			 ">>\n"
			 "/Count %ld\n"
			 "/Kids [\n",
			 pagecnt);
	for (i = 0; i < h->pages.cnt; i++)
		writeObjRef(h, h->pages.array[i]);
	if (h->level > 0)
		for (i = 0; i < h->glyphs.cnt; i++)
			writeObjRef(h, h->glyphs.array[i].page.obj);
	dstPrint(h, "]\n");
	dstEndObj(h);

	/* Write catalog object */
	h->obj.root = writeCatalogObj(h, pages);
	}

