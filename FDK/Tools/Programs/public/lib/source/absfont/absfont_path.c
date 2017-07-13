/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Path support.
 */

#include "absfont.h"
#include "dynarr.h"
#include "supportexcept.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <math.h>
#include <stdlib.h>
#include <float.h>

#ifndef MAXFLOAT
    #define MAXFLOAT    FLT_MAX
#endif

#define RND(v)		((float)floor((v) + 0.5))

/* Scale to thousandths of an em */
#define MILLIEM(v)	((float)(h->top->sup.UnitsPerEm*(v)/1000.0))

/* Macro switches */
#define ROUND_ISECT_COORDS  0           /* Rounding intersection coordinates appear to have negative impact. Disabled */
#define CLAMP_ISECT_T       0           /* Clamp "t" if the calculated intersection is close to a segment end */

/* Heuristics constants */

#define BEZ_SPLIT_DEPTH     6           /* Split 6 times; bbox < .002 em; don't split */
#define BEZ_SPLIT_MILLIEM   MILLIEM(16) /* MILLIEM(64) is not small enough for FiraSans-TwoItalic */
#define ISECT_EPSILON       1.0f
#define WIND_TEST_EPSILON   1.0f
#define EXTREMUM_EPSILON    0.5f
#define MISMATCH_SEGMENT_PENALTY    0.5f
#define FIXED_SCORE_INCREMENT       0.1f
#define EXTRA_OUTLET_PENALTY        0.5f
#define MAX_SWEEP_FIX_JUNC          100
#define SHORT_LINE_LIMIT            1

#if ABF_DEBUG
static void dbglyph(abfCtx h, long iGlyph);
static void dbisects(abfCtx h);
static void dbjuncs(abfCtx h);
#endif

typedef struct				/* Coordinate point */
	{
	float x;
	float y;
	} Point;

typedef struct				/* Rectangle */
	{
	float left;
	float bottom;
	float right;
	float top;
	} Rect;

typedef struct				/* Straight line */
	{
	Point p0;
	Point p1;
	float t0;				/* Split start t */
	float t1;				/* Split end t */
	long iSeg;				/* Original segment */
	} Line;

typedef struct				/* Bezier curve */
	{
	Point p0;
	Point p1;
	Point p2;
	Point p3;
	float t0;				/* Split start t */
	float t1;				/* Split end t */
	long depth;				/* Recursion depth */
	long iSeg;				/* Original segment */
	Rect bounds;
	} Bezier;

typedef struct				/* Intersect */
	{
	float t;				/* Intercept on segment */
	Point p;				/* intersection point */
	long iSeg;				/* Segment index */
	long iSplit;			/* Split segment index */
	long id;				/* Pair identifier */
	long flags;				/* Status flags */
#define ISECT_DUP	(1<<0)	/* Duplicate intersection */
	} Intersect;

typedef struct              /* Outlet */
    {
    long    seg;            /* Segment index */
    long    otherJunc;      /* Junction index on the other side */
    long    otherSeg;       /* Segment index on the other side */
    float   angle;          /* angle */
    float   score;          /* segment score */
    long    flags;          /* Status flags */
#define OUTLET_DELETE   (1<<0)  /* Outlet deleted */
#define OUTLET_OUT      (1<<1)  /* Outlet outgoing */
#define OUTLET_VISITED  (1<<2)  /* Outlet visited */
    } Outlet;

typedef dnaDCL(Outlet, OutletList);

typedef struct				/* Junction */
	{
    Point p;                /* intersection point */
    OutletList  outlets;    /* list of incoming or outgoing segments */
    int inCount;            /* number of incoming segments */
    int outCount;           /* number of outgoing segments */
	} Junction;

typedef struct				/* Glyph */
	{
	abfGlyphInfo *info;		/* General info */
	float hAdv;				/* Horizontal advance width */
	long iPath;				/* First path index; -1 if no path */
	} Glyph;

typedef struct				/* Path */
	{
	Rect bounds;			/* Path bounds */
	long flags;				/* Status flags */
#define PATH_ISECT	(1<<0)	/* Path intersected */
	long iSeg;				/* First segment index */
	long iPrev;				/* Previous path index */
	long iNext;				/* Next path index */
	} Path;

typedef struct
    {
    int cnt;                /* Number of extrema (up to 2) */
    float t[2];             /* t corresponding to extrema on curve */
    float v[2];             /* coordinates at extrema */
    } Extrema;

typedef struct				/* Segment */
	{
	Rect bounds;			/* Segment bounds */
	Point p0;				/* Line: p0, p3; curve: p0, p1, p2, p3 */
	Point p1;
	Point p2;
	Point p3;
    Extrema xExtrema;       /* Curve extrema */
    Extrema yExtrema;
	long flags;				/* Status flags */
#define SEG_DELETE		(1<<0)	/* Segment deleted */
#define SEG_LINE		(1<<1)	/* Line segment (else curve segment) */
#define SEG_ISECT		(1<<2)	/* Segment intersected */
#define SEG_WIND_TEST	(1<<3)	/* Segment winding tested */
#define SEG_REVERSE     (1<<4)	/* Segment to be reversed */
#define SEG_VISITED     (1<<5)	/* Segment has been visited during path reconstruction */
	long iPrev;				/* Previous segment index */
	long iNext;				/* Next segment index */
	long iPath;				/* Parent path */
    long inJunc;            /* ingoing junction index */
    long outJunc;           /* outcoming junction index */
    float score;            /* Confidence level of delete/undelete status (non-negative) */
	} Segment;

typedef dnaDCL(float, ValueList);

struct abfCtx_				/* Context */
	{
	long flags;
	abfTopDict *top;
	dnaDCL(Glyph, glyphs);
	dnaDCL(Path, paths);
	dnaDCL(Segment, segs);
	dnaDCL(Intersect, isects);
	dnaDCL(Junction, juncs);
    ValueList xExtremaList;
    ValueList yExtremaList;
	long iGlyph;			/* Current glyph index */
	long iPath;				/* Current path index */
	long iSeg;				/* Current segment index */
	Point p;				/* Current point */
	ctlMemoryCallbacks mem;
	dnaCtx fail;			/* Failing dna context */
	dnaCtx safe;			/* Safe dna context */
	struct					/* Error handling */
		{
		int code;
		} err;
	};

static void isectGlyph(abfCtx h, long iGlyph);
static void splitBez(Bezier *a, Bezier *b, float t);

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(abfCtx h, int err_code)
	{
	h->err.code = err_code;
    RAISE(h->err.code, NULL);
	}

/* -------------------------- Fail dynarr Context -------------------------- */

/* Initialize failing dynarr context. Return 1 on failure else 0. */
static dnaCtx dna_FailInit(abfCtx h)
	{
	return dnaNew(&h->mem, DNA_CHECK_ARGS);
	}

/* -------------------------- Safe dynarr Context -------------------------- */

/* Manage memory. */
static void *dna_SafeManage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	abfCtx h = (abfCtx)cb->ctx;
	void *ptr = h->mem.manage(&h->mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, abfErrNoMemory);
	return ptr;
	}

/* Initialize safe dynarr context. Return 1 on failure else 0. */
static dnaCtx dna_SafeInit(abfCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx = h;
	cb.manage = dna_SafeManage;
	return dnaNew(&cb, DNA_CHECK_ARGS);
	}

/* --------------------------- Context Management -------------------------- */

/* Create new library context. */
abfCtx abfNew(ctlMemoryCallbacks *mem_cb, CTL_CHECK_ARGS_DCL)
	{
	abfCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(ABF_VERSION))
		return NULL;

	/* Allocate context */
	h = (abfCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct abfCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	memset(h, 0, sizeof(*h));
	h->err.code = abfErrNoMemory;

	DURING
		/* Copy callbacks */
		h->mem = *mem_cb;

		/* Initialize service libraries */
		h->fail = NULL;
		h->safe = NULL;
		h->fail = dna_FailInit(h);
		h->safe = dna_SafeInit(h);
		if (h->fail == NULL || h->safe == NULL)
	  		{
			goto cleanup;
			}

		/* Initialize */
		h->flags = 0;
		dnaINIT(h->fail, h->glyphs, 1, 250);
		dnaINIT(h->fail, h->paths, 20, 500);
		dnaINIT(h->fail, h->segs, 100, 5000);
		dnaINIT(h->safe, h->isects, 10, 20);
		dnaINIT(h->safe, h->juncs, 10, 20);
		dnaINIT(h->safe, h->xExtremaList, 100, 100);
		dnaINIT(h->safe, h->yExtremaList, 100, 100);
		h->iGlyph = -1;
		h->iPath = -1;
		h->iSeg = -1;
		h->err.code = abfSuccess;

cleanup:;
	HANDLER
	END_HANDLER
	if (h->err.code != abfSuccess) {
		abfFree(h);
		h = NULL;
	}

	return h;
	}

static void freeOutlets(abfCtx h)
	{
	long	i;

	for (i = 0; i < h->juncs.cnt; i++)
		dnaFREE(h->juncs.array[i].outlets);
	}

/* Free library context. */
int abfFree(abfCtx h)
	{
	if (h == NULL)
		return abfSuccess;

	dnaFREE(h->glyphs);
	dnaFREE(h->paths);
	dnaFREE(h->segs);
	dnaFREE(h->isects);
	freeOutlets(h);
	dnaFREE(h->juncs);
    dnaFREE(h->xExtremaList);
    dnaFREE(h->yExtremaList);
	dnaFree(h->fail);
	dnaFree(h->safe);
	h->mem.manage(&h->mem, h, 0);

	return abfSuccess;
	}

/* Begin new font. */
int abfBegFont(abfCtx h, abfTopDict *top)
	{
	h->top = top;
	h->glyphs.cnt = 0;
	h->paths.cnt = 0;
	h->segs.cnt = 0;

	return abfSuccess;
	}

/* End font. */
int abfEndFont(abfCtx h, long flags, abfGlyphCallbacks *glyph_cb)
	{
	long i;

	if (h->err.code == abfErrNoMemory)
		return abfErrNoMemory;

	/* Set error handler */
    DURING

        if (flags & ABF_PATH_REMOVE_OVERLAP)
            for (i = 0; i < h->glyphs.cnt; i++)
                isectGlyph(h, i);

        /* Callback glyphs */
        for (i = 0; i < h->glyphs.cnt; i++)
            {
            long iPath;
            Glyph *glyph = &h->glyphs.array[i];

            /* Callback glyph begin and handle return value */
            switch (glyph_cb->beg(glyph_cb, glyph->info))
                {
            case ABF_CONT_RET:
                break;
            case ABF_WIDTH_RET:
                glyph_cb->width(glyph_cb, glyph->hAdv);
                glyph_cb->end(glyph_cb);
                continue;
            case ABF_SKIP_RET:
                continue;
            case ABF_QUIT_RET:
                E_RETURN(abfErrCstrQuit);
            case ABF_FAIL_RET:
                E_RETURN(abfErrCstrFail);
                }

            /* Callback width */
            glyph_cb->width(glyph_cb, glyph->hAdv);

            iPath = glyph->iPath;
            if (iPath != -1)
                do
                    {
                    /* Callback path */
                    Path *path = &h->paths.array[iPath];
                    long iFirst = path->iSeg;
                    Segment *first = &h->segs.array[iFirst];
                    long iLast = first->iPrev;
                    long iStop = 
                        (h->segs.array[iLast].flags & SEG_LINE)? iLast: iFirst;
                    Segment *seg = first;
                    long segcnt = 0;

                    /* Callback initial move */
                    glyph_cb->move(glyph_cb, first->p0.x, first->p0.y);

                    /* Callback remaining segments */
                    for (;;)
                        {
                        long iSeg;

                        if (seg->flags & SEG_LINE)
                            glyph_cb->line(glyph_cb, seg->p3.x, seg->p3.y);
                        else
                            glyph_cb->curve(glyph_cb, 
                                            seg->p1.x, seg->p1.y,
                                            seg->p2.x, seg->p2.y,
                                            seg->p3.x, seg->p3.y);

                        if (++segcnt > h->segs.cnt)
                            /* Infinite loop! */
                            fatal(h, abfErrCantHandle);	

                        iSeg = seg->iNext;
                        if (iSeg == iStop)
                            break;
                        seg = &h->segs.array[iSeg];
                        }

                    iPath = path->iNext;
                    }
                while (iPath != glyph->iPath);

            /* Callback glyph end */
            glyph_cb->end(glyph_cb);
            }

    HANDLER
        return h->err.code;
    END_HANDLER

	return abfSuccess;
	}

/* ---------------------------- Glyph Callbacks ---------------------------- */

/* Begin new glyph. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	Glyph *glyph;

	cb->info = info;

	/* Allocate new glyph */
	h->iGlyph = dnaNext(&h->glyphs, sizeof(Glyph));
	if (h->iGlyph == -1)
		{
		h->err.code = abfErrNoMemory;
		return ABF_FAIL_RET;
		}

	/* Initialize */
	glyph = &h->glyphs.array[h->iGlyph];
	glyph->info = info;
	glyph->iPath = h->paths.cnt;

	h->iPath = -1;
	h->iSeg = 1;

	return ABF_CONT_RET;
	}

/* Add glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;

	if (h->iGlyph == -1)
		{
		h->err.code = abfErrBadPath;	
		return;
		}

	h->glyphs.array[h->iGlyph].hAdv = hAdv;
	}

/* Get new segment. */
static Segment *newSegment(abfCtx h)
	{
	Segment *seg;

	/* Allocate new segment */
	h->iSeg = dnaNext(&h->segs, sizeof(Segment));
	if (h->iSeg == -1)
		{
		h->err.code = abfErrNoMemory;
		return NULL;
		}

	/* Initialize */
	seg = &h->segs.array[h->iSeg];
	seg->iPrev = h->iSeg - 1;
	seg->iNext = h->iSeg + 1;
	seg->iPath = h->iPath;
    seg->inJunc = -1;
    seg->outJunc = -1;
    seg->score = 0;

	return seg;
	}

/* Add line segment to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	Segment *seg;

	if (x1 == h->p.x && y1 == h->p.y)
		return;	/* Ignore zero-length lines */

	seg = newSegment(h);
	if (seg == NULL)
		return;

	/* Fill segment */
	seg->flags	= SEG_LINE;
	seg->p0		= h->p;
	seg->p3.x	= x1;
	seg->p3.y	= y1;
	h->p		= seg->p3;
	}

/* Close the current path */
static void closepath(abfGlyphCallbacks *cb)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	long iBeg;
	long iEnd;
	Point *p;

	if (h->iPath == -1)
		return;	/* First move */

	/* Get first point on path */
	iBeg = h->paths.array[h->iPath].iSeg;
	if (iBeg == h->segs.cnt)
		{
		/* Remove empty path */
		h->paths.cnt--;
		return;
		}
	p = &h->segs.array[iBeg].p0;

    if (fabs(p->x - h->p.x) < 1 && fabs(p->y - h->p.y) < 1)
        {
        /* Avoid adding almost zero length line segment to close a path */
        *p = h->p;
        }
	else
		{
		/* Insert line segment to close path */
		glyphLine(cb, p->x, p->y);
		if (h->iSeg == -1)
			return;
		}

	/* Make loop */
	iEnd = h->iSeg;
	h->segs.array[iBeg].iPrev = iEnd;
	h->segs.array[iEnd].iNext = iBeg;

	h->iPath = -1;
	}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	Path *path;

	closepath(cb);
	if (h->iSeg == -1)
		return;

	/* Allocate new path */
	h->iPath = dnaNext(&h->paths, sizeof(Path));
	if (h->iPath == -1)
		{
		h->err.code = abfErrNoMemory;
		return;
		}

	/* Initialize */
	path = &h->paths.array[h->iPath];
	path->flags = 0;
	path->iSeg = h->segs.cnt;
	path->iPrev = h->iPath - 1;
	path->iNext = h->iPath + 1;

	/* Set current point */
	h->p.x = x0;
	h->p.y = y0;
	}
	
/* Add curve segment to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
    Segment *seg;

	if (x1 == h->p.x && y1 == h->p.y && x1 == x2 && y1 == y2 && x2 == x3 && y2 == y3)
		return;	/* Ignore zero-length curves */

	seg = newSegment(h);
	if (seg == NULL)
		return;

	/* Fill segment */
	seg->flags	= 0;
	seg->p0 	= h->p;
	seg->p1.x	= x1; 
	seg->p1.y	= y1;
	seg->p2.x	= x2; 
	seg->p2.y	= y2;
	seg->p3.x	= x3; 
	seg->p3.y	= y3;
	h->p 		= seg->p3;
	}

/* Glyph path general operator. */
static void glyphGenop(abfGlyphCallbacks *cb, 
					   int cnt, float *args, int op)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	h->err.code = abfErrGlyphGenop;
	}

/* Glyph path seac. */
static void glyphSeac(abfGlyphCallbacks *cb, 
					  float adx, float ady, int bchar, int achar)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	h->err.code = abfErrGlyphSeac;
	}

/* Set bounds on line segment. */
static void setLineBounds(Rect *r, Point *u0, Point *u1)
	{
	if (u0->x < u1->x)
		{
		r->left = u0->x;
		r->right = u1->x;
		}
	else
		{
		r->left = u1->x;
		r->right = u0->x;
		}
	if (u0->y < u1->y)
		{
		r->bottom = u0->y;
		r->top = u1->y;
		}
	else
		{
		r->bottom = u1->y;
		r->top = u0->y;
		}
	}

/* Set horizontal or vertical limits (other than end points) on Bezier curve.*/
static void setBezLimits(float p0, float p1, float p2, float p3,
						 float *lo, float *hi)
	{
	float t[2];
	int i = 0;
	float a = p3 - 3*(p2 - p1) - p0;
	float b = p2 - 2*p1 + p0;
	float c = p1 - p0;
	if (a == 0)
		{
		if (b != 0)
			t[i++] = -c/(2*b);
		}
	else
		{
		float r = b*b - a*c;
		if (r >= 0)
			{
			r = (float)sqrt(r);
			t[i++] = (-b + r)/a;
			t[i++] = (-b - r)/a;
			}
		}
	while (i--)
		if (t[i] > 0 && t[i] < 1)
			{
			float limit = t[i]*(t[i]*(t[i]*a + 3*b) + 3*c) + p0;
			if (limit < *lo)
				*lo = limit;
			else if (limit > *hi)
				*hi = limit;
			}
	}

/* Set bounds on Bezier curve. */
static void setCurveBounds(Rect *r, Point *p0, Point *p1, Point *p2, Point *p3)
	{
	/* Set initial bounds from end points */
	setLineBounds(r, p0, p3);

	if (p1->x < r->left || p1->x > r->right || 
		p2->x < r->left || p2->x > r->right)
		/* Grow left and/or right bounds */
		setBezLimits(p0->x, p1->x, p2->x, p3->x, &r->left, &r->right);

	if (p1->y < r->bottom || p1->y > r->top ||
		p2->y < r->bottom || p2->y > r->top)
		/* Grow top and/or bottom bounds */
		setBezLimits(p0->y, p1->y, p2->y, p3->y, &r->bottom, &r->top);
	}

/* Find curve extrema using sub-division of Bezier curve.
   The derivative quadratic equation may not necessarily produce accurate results
   with a curve with a sharp turn. */
static void findBezExtrema(float *min, float *max, float *mint, float *maxt,
                            float v0, float v1, float v2, float v3, float t0, float t1)
    {
    if (v0 < *min)
        {
        *min = v0;
        *mint = t0;
        }
    if (v3 < *min)
        {
        *min = v3;
        *mint = t1;
        }
    if (v0 > *max)
        {
        *max = v0;
        *maxt = t0;
        }
    if (v3 > *max)
        {
        *max = v3;
        *maxt = t1;
        }

    if (v1 < *min - EXTREMUM_EPSILON || v2 < *min - EXTREMUM_EPSILON ||
        v1 > *max + EXTREMUM_EPSILON || v2 > *max + EXTREMUM_EPSILON)
        {
        float s, t, u, c, ct;

        /* Compute intermediate point */
        s = (v2 + v3)/2;
        t = (v1 + v2)/2;
        u = (v0 + v1)/2;
        c = (s + 2*t + u)/4;
        ct = (t0 + t1)/2;

        /* First half of split */
        findBezExtrema(min, max, mint, maxt, v0, u, (t + u)/2, c, t0, ct);

        /* Second half of split */
        findBezExtrema(min, max, mint, maxt, c, (s + t)/2, s, v3, ct, t1);
        }
    else
        {
        if (v1 < *min)
            *min = v1;
        if (v2 < *min)
            *min = v2;
        if (v1 > *max)
            *max = v1;
        if (v2 > *max)
            *max = v2;
        }
    }

static void setBezExtrema(Extrema *ex, float *lo, float *hi, float v0, float v1, float v2, float v3)
    {
    float min, max;
    float mint, maxt;
    int i;

    ex->cnt = 0;
    if (v0 < v3)
        {
        min = v0;
        max = v3;
        }
    else
        {
        min = v3;
        max = v0;
        }
    
    mint = maxt = 0;
    findBezExtrema(&min, &max, &mint, &maxt, v0, v1, v2, v3, 0, 1);
    i = 0;
    if (mint > 0)
        {
        ex->t[i] = mint;
        ex->v[i] = min;
        i++;
        }
    if (maxt > 0)
        {
        ex->t[i] = maxt;
        ex->v[i] = max;
        i++;
        }
    ex->cnt = i;
    if (i == 2 && mint > maxt)
        {
        ex->t[0] = maxt;
        ex->v[0] = max;
        ex->t[1] = mint;
        ex->v[1] = min;
        }

    *lo = min;
    *hi = max;
    }

/* Set bounds on Bezier curve. */
static void setCurveBoundsAndExtrema(Segment *seg)
	{
    setBezExtrema(&seg->xExtrema, &seg->bounds.left, &seg->bounds.right, seg->p0.x, seg->p1.x, seg->p2.x, seg->p3.x);
    setBezExtrema(&seg->yExtrema, &seg->bounds.bottom, &seg->bounds.top, seg->p0.y, seg->p1.y, seg->p2.y, seg->p3.y);
	}

/* Set segment bounds and curve extrema. */
static void setSegBounds(Segment *seg)
	{
	if (seg->flags & SEG_LINE)
		setLineBounds(&seg->bounds, &seg->p0, &seg->p3);
	else
        setCurveBoundsAndExtrema(seg);
	}

/* Grow bounds of rect r to include bounds of rect r1. */
static void rectGrow(Rect *r, Rect *r1)
	{
	if (r->left > r1->left)
		r->left = r1->left;
	if (r->bottom > r1->bottom)
		r->bottom = r1->bottom;
	if (r->right < r1->right)
		r->right = r1->right;
	if (r->top < r1->top)
		r->top = r1->top;
	}

/* Set path bounds. */
static void setPathBounds(abfCtx h, Path *path)
	{
	Segment *seg = &h->segs.array[path->iSeg];

	/* Set and copy first segment bounds */
	setSegBounds(seg);
	path->bounds = seg->bounds;

	/* Grow bounds for remaining segments */
	for (;;)
		{
		long iSeg = seg->iNext;
		if (iSeg == path->iSeg)
			break;
		seg = &h->segs.array[iSeg];
		setSegBounds(seg);
		rectGrow(&path->bounds, &seg->bounds);
		}
	}

/* Glyph path end. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	abfCtx h = (abfCtx)cb->direct_ctx;
	long i;
	long iFirst;
	long iLast;

	/* Close last path */
	closepath(cb);
	if (h->iSeg == -1)
		return;

	iFirst = h->glyphs.array[h->iGlyph].iPath; 
	iLast = h->paths.cnt - 1;
	if (iFirst > iLast)
		{
		/* Non-marking glyph */
		h->glyphs.array[h->iGlyph].iPath = -1;
		return;
		}
	
	/* Make loop */
	h->paths.array[iFirst].iPrev = iLast;
	h->paths.array[iLast].iNext = iFirst;

	/* Set bounds */
	for (i = iFirst; i <= iLast; i++)
		setPathBounds(h, &h->paths.array[i]);	

	h->iGlyph = -1;
	}

/* Return 1 if rectangles intersect else 0. */
static int rectOverlap(Rect *r1, Rect *r2)
	{
	return !(r1->right < r2->left ||
			 r1->left > r2->right ||
			 r1->top < r2->bottom ||
			 r1->bottom > r2->top);
	}

/* Intersect rects r1 and r2 to produce rect r. */
static void rectIsect(Rect *r, Rect *r1, Rect *r2)
	{
	r->left = MAX(r1->left, r2->left);
	r->bottom = MAX(r1->bottom, r2->bottom);
	r->right = MIN(r1->right, r2->right);
	r->top 	= MIN(r1->top, r2->top);
	}

/* Intersect lines u0->u1 and v0->v1. Returns as follows:

   0	lines don't intersect
   1	lines intersect; line0 at t[0]
   						 line1 at t[1]
   2	lines intersect; line0 at t[0] and t[2]
						 line1 at t[1] and t[3]
   */
static int isectLines(Point *u0, Point *u1, Point *v0, Point *v1, float t[4])
	{
	float a0 = u1->y - u0->y;
	float b0 = u1->x - u0->x;
	float a1 = v1->y - v0->y;
	float b1 = v1->x - v0->x;
	float d = a1*b0 - b1*a0;
	float j = u0->y - v0->y;
	float k = u0->x - v0->x;
	float n0 = b1*j - a1*k;

	if (d == 0)
		{
		/* Lines are parallel */
		if (n0 == 0)
			{
			/* Lines are coincident; compute intersection rect */
			Rect r;
			Rect r1;
			Rect r2;

			setLineBounds(&r1, u0, u1);
			setLineBounds(&r2, v0, v1);
			if (!rectOverlap(&r1, &r2))
				return 0;

			rectIsect(&r, &r1, &r2);

			if (r.left == r.right && r.bottom == r.top)
				{
				/* Lines touch at end points */
				if (b0 != 0)
					{
					/* Compute t horizontally */
					t[0] = (float)(u1->x == r.left);
					t[1] = (float)(v1->x == r.left);
					}
				else
					{
					/* Compute t vertically */
					t[0] = (float)(u1->y == r.bottom);
					t[1] = (float)(v1->y == r.bottom);
					}
				return 1;
				}
			else if (r.right - r.left > r.top - r.bottom)
				{
				/* Compute t horizontally */
				t[0] = (r.left - u0->x)/b0;
				t[1] = (r.left - v0->x)/b1;
				t[2] = (r.right - u0->x)/b0;
				t[3] = (r.right - v0->x)/b1;
				return 2;
				}
			else
				{
				/* Compute t vertically */
				t[0] = (r.bottom - u0->y)/a0;
				t[1] = (r.bottom - v0->y)/a1;
				t[2] = (r.top - u0->y)/a0;
				t[3] = (r.top - v0->y)/a1;
				return 2;
				}
			}
		else
			return 0;
		}

	/* Non-parallel lines; check they both have intercepts */
	t[0] = n0/d;
	if (t[0] < 0 || t[0] > 1)
		return 0;

	t[1] = (b0*j - a0*k)/d;
	if (t[1] < 0 || t[1] > 1)
		return 0;

	return 1;
	}

/* Compute intersection point on line segment. */
static Point calcLineIsectPoint(float t, Segment *seg)
	{
	Point p;
	p.x = seg->p0.x + t*(seg->p3.x - seg->p0.x);
	p.y = seg->p0.y + t*(seg->p3.y - seg->p0.y);
	return p;
	}

/* Compute intersection point on bezier segment. */
static Point calcCurveIsectPoint(float t, Segment *seg)
	{
	Point p;
	p.x = t*(t*(t*(seg->p3.x - 3*(seg->p2.x - seg->p1.x) - seg->p0.x) +
				3*(seg->p2.x - 2*seg->p1.x + seg->p0.x)) +
			 3*(seg->p1.x - seg->p0.x)) + seg->p0.x;
	p.y = t*(t*(t*(seg->p3.y - 3*(seg->p2.y - seg->p1.y) - seg->p0.y) +
				3*(seg->p2.y - 2*seg->p1.y + seg->p0.y)) +
			 3*(seg->p1.y - seg->p0.y)) + seg->p0.y;
	return p;
	}

/* Search for insertion position. Returns an index where a new intersection point is added.
 * if the point exactly matches an existing intersection, its index is returned and match is set to 2
 * if a close match is found, match is set to 1
 */
static long checkIsect(abfCtx h, Point *p, Segment *seg, float t, int *match)
	{	
	long i;

	/* Search for insertion position for the same segment */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Intersect *isect = &h->isects.array[i];
        Segment *isectSeg = &h->segs.array[isect->iSeg];
		long iPath = isectSeg->iPath;

		if (iPath == seg->iPath)
			{
            if (seg == isectSeg && t == isect->t)
                {
                    *match = 2; /* Matches previous intersection */
					return i;
                }
            }
		}

	/* Search for an close enough intersection */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Intersect *isect = &h->isects.array[i];
        Point u;
        Point v;

        u = *p;
        v = isect->p;
        if ((fabs(v.x - u.x) <= ISECT_EPSILON) &&
            (fabs(v.y - u.y) <= ISECT_EPSILON))
            {
            *p = isect->p;
            *match = 1; /* Closely matches previous intersection */
            return i;
            }
		}
    *match = 0;
    return i;
}

/* Insert intersection at the index. */
static void insertIsect(abfCtx h, long i, Point *p, Segment *seg, float t, long id)
	{	
	Intersect *_new;

	/* Insert new record */
	_new = &dnaGROW(h->isects, h->isects.cnt)[i];
	memmove(_new + 1, _new, (h->isects.cnt++ - i)*sizeof(Intersect));
	_new->t 		= t;
	_new->iSeg 	= (long)(seg - h->segs.array);
	_new->iSplit = -1;
	_new->p 		= *p;
	_new->id 	= id;
    _new->flags  = 0;

	/* Mark path as intersected */
	h->paths.array[seg->iPath].flags |= PATH_ISECT;
	}

/* Save intersection pair. */
static void saveIsectPair(abfCtx h, 
						  Segment *seg0, float t0, Segment *seg1, float t1)
	{
	Point p;
	long id = h->isects.cnt;
    long i0, i1;
    int match0, match1;

    /* If the intersection point is at the split point for self-intersecting curve test, ignore it */
    if ((seg0 == seg1) && (t0 == t1))
        return;

	/* Compute intersection point */
	if (t0 == 0)
		p = seg0->p0;
	else if (t0 == 1)
		p = seg0->p3;
	else if (t1 == 0)
		p = seg1->p0;
	else if (t1 == 1)
		p = seg1->p3;
	else if (seg0->flags & SEG_LINE)
		p = calcLineIsectPoint(t0, seg0);
	else if (seg1->flags & SEG_LINE)
		p = calcLineIsectPoint(t1, seg1);
	else
		p = calcCurveIsectPoint(t0, seg0);

#if CLAMP_ISECT_T
    {
	/* If intersection is very close to end of segment, force it to	end */
	float x = RND(p.x);
	float y = RND(p.y);
	if (0 < t0 && t0 < 1 &&
		((x == seg0->p0.x && y == seg0->p0.y) ||
		 (x == seg0->p3.x && y == seg0->p3.y)))
		t0 = RND(t0);
	if (0 < t1 && t1 < 1 &&
		((x == seg1->p0.x && y == seg1->p0.y) ||
		 (x == seg1->p3.x && y == seg1->p3.y)))
		t1 = RND(t1);
    }
#endif

    /* If the intersection is actually the end point between two adjacent segments in the same path, ignore it */
    if (seg0->iPath == seg1->iPath)
        {
        long iSeg1 = (long)(seg1 - h->segs.array);
        if ((seg0->iNext == iSeg1) && (t0 == 1 || t1 == 0))
            return;
        if ((seg0->iPrev == iSeg1) && (t0 == 0 || t1 == 1))
            return;
        }

    i0 = checkIsect(h, &p, seg0, t0, &match0);
    i1 = checkIsect(h, &p, seg1, t1, &match1);

    /* assign the same id of an existing matching intersection if there is one */
    if (match0 || match1)
        {
        int m = match0;
        long i = i0;
        /* Prefer an exact match */
        if (match0 < match1)
            {
            m = match1;
            i = i1;
            }
        id = h->isects.array[i].id;

        /* If it was a close-match, unify the intersection points
           to prevent further close matches from floating away */
        if (m == 1)
            p = h->isects.array[i].p;

        /* If the two exact matches happen to have distict intersection IDs, unify them altogether */
        if (match0 == 2 && match1 == 2 && h->isects.array[i0].id != h->isects.array[i1].id)
            {
            long    id1 = h->isects.array[i1].id;
            long    j;
            for (j = 0; j < h->isects.cnt; j++)
                if (h->isects.array[j].id == id1)
                    h->isects.array[j].id = id;
            }
        }

    insertIsect(h, i0, &p, seg0, t0, id);
     /* recalculate insertion index as it might have changed */
    if (i0 <= i1)
        i1 = checkIsect(h, &p, seg1, t1, &match1);
    insertIsect(h, i1, &p, seg1, t1, id);
    }

/* Intersect a line/line segments. */
static void isectLineLineSegs(abfCtx h, Segment *s0, Segment *s1)
	{
	float t[4];
	switch (isectLines(&s0->p0, &s0->p3, &s1->p0, &s1->p3, t))
		{
	case 0:
		break;
	case 2:
		saveIsectPair(h, s0, t[2], s1, t[3]);
		/* Fall through */
	case 1:
		saveIsectPair(h, s0, t[0], s1, t[1]);
		break;
		}
	}

/* Check if line u0->u1 intersects with rectangle r. Return 1 if intersection
   else 0. Cohen-Sutherland algorithm. */
static int isectLineRect(Point *u0, Point *u1, Rect *r)
	{
	enum	/* Clip codes */
		{
		Left = 1,
		Right = 2,
		Bottom = 4,
		Top = 8
		};
	int c0;
	int c1;
	
	if (u0->x < r->left)
		c0 = Left;
	else if (u0->x > r->right)
		c0 = Right;
	else
		c0 = 0;

	if (u0->y > r->top)
		c0 |= Top;
	else if (u0->y < r->bottom)
		c0 |= Bottom;
	
	if (c0 == 0)
		return 1;

	if (u1->x < r->left)
		c1 = Left;
	else if (u1->x > r->right)
		c1 = Right;
	else
		c1 = 0;

	if (u1->y > r->top)
		c1 |= Top;
	else if (u1->y < r->bottom)
		c1 |= Bottom;

	if (c1 == 0)
		return 1;

	return (c0 & c1) == 0;
	}

/* Return 1 if curve flat enough else 0. Adapted from the flatten.c in Type 1
   rasterizer. */
static int bezFlatEnough(abfCtx h, Bezier *q)
	{
	if (q->depth == BEZ_SPLIT_DEPTH)
		return 1;
	else if (fabs(q->bounds.right - q->bounds.left) > BEZ_SPLIT_MILLIEM ||
			 fabs(q->bounds.top - q->bounds.bottom) > BEZ_SPLIT_MILLIEM)
		{
		/* BBox eighth em or bigger, split */
		q->depth = 0;
		return 0;
		}
	else if (((q->p0.x > q->p1.x || q->p1.x > q->p2.x || q->p2.x > q->p3.x) &&
			  (q->p3.x > q->p2.x || q->p2.x > q->p1.x || q->p1.x > q->p0.x)) ||
			 ((q->p0.y > q->p1.y || q->p1.y > q->p2.y || q->p2.y > q->p3.y) &&
			  (q->p3.y > q->p2.y || q->p2.y > q->p1.y || q->p1.y > q->p0.y)))
		/* Points not monotonic in x and y; split */
		return 0;
	else
		{
		/* Split if control points not spaced evenly within .003 em */
		float eps3 = MILLIEM(9);
		float c1x = (float)fabs(q->p1.x - q->p0.x);
		float c3x = (float)fabs(q->p3.x - q->p0.x);
		if (fabs(c3x - 3*c1x) > eps3)
			return 0;
		else
			{
			float c2x = (float)fabs(q->p2.x - q->p0.x);
			if (fabs(2*(c3x - c2x) - c2x) > eps3)
				return 0;
			else
				{
				float c1y = (float)fabs(q->p1.y - q->p0.y);
				float c3y = (float)fabs(q->p3.y - q->p0.y);
				if (fabs(c3y - 3*c1y) > eps3)
					return 0;
				else
					{
					float c2y = (float)fabs(q->p2.y - q->p0.y);
					if (fabs(2*(c3y - c2y) - c2y) > eps3)
						return 0;
					}
				}
			}
		}
	return 1;
	}

/* Split Bezier curve a at the midpoint into Bezier curves b and a. */
static void splitBezMid(Bezier *a, Bezier *b)
	{
	Point s;
	Point t;
	Point u;
	Point c;
	float ct;

	/* Compute intermediate points */
	s.x = (a->p2.x + a->p3.x)/2;
	s.y = (a->p2.y + a->p3.y)/2;
	t.x = (a->p1.x + a->p2.x)/2;
	t.y = (a->p1.y + a->p2.y)/2;
	u.x = (a->p0.x + a->p1.x)/2;
	u.y = (a->p0.y + a->p1.y)/2;
	c.x = (s.x + 2*t.x + u.x)/4;
	c.y = (s.y + 2*t.y + u.y)/4;
	ct = (a->t0 + a->t1)/2;

	/* Compute first half of split (curve b) */
	b->p0 = a->p0;
	b->p1 = u;
	b->p2.x = (t.x + u.x)/2;
	b->p2.y = (t.y + u.y)/2;
	b->p3 = c;
	b->t0 = a->t0;
	b->t1 = ct;
	b->iSeg = a->iSeg;
	b->depth = ++a->depth;
	setCurveBounds(&b->bounds, &b->p0, &b->p1, &b->p2, &b->p3);

	/* Compute second half of split (curve a) */
	a->p0 = c;
	a->p1.x = (s.x + t.x)/2;
	a->p1.y = (s.y + t.y)/2;
	a->p2 = s;
	a->t0 = ct;
	setCurveBounds(&a->bounds, &a->p0, &a->p1, &a->p2, &a->p3);
	/* p3, t1, and iSeg unchanged */
	}

/* Intersect line/curve by recursive subdivision of curve. */
static void isectLineCurve(abfCtx h, Line *l, Bezier a)
	{
	for (;;)
		if (bezFlatEnough(h, &a))
			{
			float t[4];
			switch (isectLines(&l->p0, &l->p1, &a.p0, &a.p3, t))
				{
			case 0:
				return;
			case 2:
				saveIsectPair(h, 
							  &h->segs.array[a.iSeg], 
							  a.t0 + t[3]*(a.t1 - a.t0),
							  &h->segs.array[l->iSeg], 
							  l->t0 + t[2]*(l->t1 - l->t0));
				/* Fall through */
			case 1:
				saveIsectPair(h, 
							  &h->segs.array[a.iSeg], 
							  a.t0 + t[1]*(a.t1 - a.t0),
							  &h->segs.array[l->iSeg], 
							  l->t0 + t[0]*(l->t1 - l->t0));
				return;
				}
			}
		else if (isectLineRect(&l->p0, &l->p1, &a.bounds))
			{
			/* Split a into b and a */
			Bezier b;
			splitBezMid(&a, &b);
			isectLineCurve(h, l, b);
			}
		else
			return;
	}

/* Make a Bezier record from curve segment. */
static void makeBez(abfCtx h, Segment *s, Bezier *a)
	{
	a->p0 = s->p0;
	a->p1 = s->p1;
	a->p2 = s->p2;
	a->p3 = s->p3;
	a->t0 = 0;
	a->t1 = 1;
	a->depth = 0;
	a->iSeg = (long)(s - h->segs.array);
	a->bounds = s->bounds;
	}

/* Make a Line record from line segment. */
static void makeLine(abfCtx h, Segment *s, Line *l)
	{
	l->p0 = s->p0;
	l->p1 = s->p3;
	l->t0 = 0;
	l->t1 = 1;
	l->iSeg = (long)(s - h->segs.array);
	}

/* Intersect a line/curve segments. */
static void isectLineCurveSegs(abfCtx h, Segment *l, Segment *c)
	{
	Line u;
	Bezier q;
	makeLine(h, l, &u);
	makeBez(h, c, &q);
	isectLineCurve(h, &u, q);
	}

/* Intersect curve/curve by alternate recursive subdivision of curves. */
static void isectCurveCurve(abfCtx h, Bezier a, Bezier b)
	{
	if (!rectOverlap(&a.bounds, &b.bounds))
		return;
	else if (bezFlatEnough(h, &a))
		{
		Line l;
		l.p0 = a.p0;
		l.p1 = a.p3;
		l.iSeg = a.iSeg;
		l.t0 = a.t0;
		l.t1 = a.t1;
		isectLineCurve(h, &l, b);
		}
	else
		{
		/* Split a into c and a */
		Bezier c;
		splitBezMid(&a, &c);
		isectCurveCurve(h, b, c);
		isectCurveCurve(h, b, a);
		}
	}

/* Intersect a curve/curve segments. */
static void isectCurveCurveSegs(abfCtx h, Segment *s0, Segment *s1)
	{
	if (s0->p0.x == s1->p0.x && s0->p0.y == s1->p0.y &&
		s0->p1.x == s1->p1.x && s0->p1.y == s1->p1.y &&
		s0->p2.x == s1->p2.x && s0->p2.y == s1->p2.y &&
		s0->p3.x == s1->p3.x && s0->p3.y == s1->p3.y)
		{
		/* Coincident identical curves; intersect endpoints */
		saveIsectPair(h, s0, 0, s1, 0);
		saveIsectPair(h, s0, 1, s1, 1);
		}
	else if (s0->p0.x == s1->p3.x && s0->p0.y == s1->p3.y &&
			 s0->p1.x == s1->p2.x && s0->p1.y == s1->p2.y &&
			 s0->p2.x == s1->p1.x && s0->p2.y == s1->p1.y &&
			 s0->p3.x == s1->p0.x && s0->p3.y == s1->p0.y)
		{
		/* Reversed coincident identical curves; intersect endpoints */
		saveIsectPair(h, s0, 0, s1, 1);
		saveIsectPair(h, s0, 1, s1, 0);
		}
	else
		{
		/* Complex intersection */
		Bezier a;
		Bezier b;
		makeBez(h, s0, &a);
		makeBez(h, s1, &b);
		isectCurveCurve(h, a, b);
		}
	}

/* Intersect pair of segments. */
static void isectSegPair(abfCtx h, Segment *s0, Segment *s1)
	{
	if (s0->flags & SEG_LINE)
		{
		if (s1->flags & SEG_LINE)
			isectLineLineSegs(h, s0, s1);
		else
			isectLineCurveSegs(h, s0, s1);
		}
	else
		{
		if (s1->flags & SEG_LINE)
			isectLineCurveSegs(h, s1, s0);
		else
			isectCurveCurveSegs(h, s0, s1);
		}
	}

/* Intersect pair of paths. */
static void isectPathPair(abfCtx h, Path *h0, Path *h1)
	{
	long i;
	long j;
	long iEnd0 = h->segs.array[h0->iSeg].iPrev;
	long iEnd1 = h->segs.array[h1->iSeg].iPrev;

	for (i = h0->iSeg; i <= iEnd0; i++)
		{
		Segment *s0 = &h->segs.array[i];
		for (j = h1->iSeg; j <= iEnd1; j++)
			{
			Segment *s1 = &h->segs.array[j];
			if (rectOverlap(&s0->bounds, &s1->bounds))
				isectSegPair(h, s0, s1);
			}
		}
	}

/* Check if a curve may be self-intersecting.
   If a curve is self-intersecting, lines from p0 to p1 and from p2 to p3 must be intersecting.
   Note that the opposite is not necessarily is true */
static int checkSelfIsectCurve(Point *p0, Point *p1, Point *p2, Point *p3)
{
    float t[4];
    if (p0->x == p3->x && p0->y == p3->y) return 0;
    if (p0->x == p1->x && p0->y == p1->y) return 0;
    if (p2->x == p2->x && p3->y == p3->y) return 0;
    return isectLines(p0, p1, p2, p3, t) != 0;
}

/* Find the intersection of a self-intersecting curve. */
static void selfIsectCurve(abfCtx h, Segment *seg)
{
    float   lo = 0, hi = 1;
    float   t;
    Bezier  a = {0}, b = {0}, c = {0};
    int     foundSplit = 0;

    /* First find a point to split the curve in two
       so that the split curves are no longer intersecting each other.
     */
    makeBez(h, seg, &c);
    b = c;  /* initialize bounds */
    while (lo < hi)
        {
        int isecta, isectb;

        t = (lo + hi) / 2;
        a = c;
        splitBez(&a, &b, t);
        isecta = checkSelfIsectCurve(&a.p0, &a.p1, &a.p2, &a.p3);
        isectb = checkSelfIsectCurve(&b.p0, &b.p1, &b.p2, &b.p3);

        if (isecta && isectb)
            fatal(h, abfErrCantHandle); /* shouldn't happen */
        else if (isecta)
            hi = t;
        else if (isectb)
            lo = t;
        else
            {
            foundSplit = 1;
            break;
            }
        }
    if (!foundSplit)
        fatal(h, abfErrCantHandle); /* shouldn't happen */

    /* the self-intersecting curve split in two as a and b which are no longer intersecting.
       find an intersection between two which must be the intersection of the original curve */
    isectCurveCurve(h, a, b);
}

/* Self-intersect path segments. */
static void selfIsectPath(abfCtx h, Path *path)
	{
	long i;
	long j;
	long iEnd = h->segs.array[path->iSeg].iPrev;

	for (i = path->iSeg; i < iEnd; i++)
		{
		Segment *s0 = &h->segs.array[i];
		for (j = i + 1; j <= iEnd; j++)
			{
			Segment *s1 = &h->segs.array[j];
			if (rectOverlap(&s0->bounds, &s1->bounds))
				isectSegPair(h, s0, s1);
			}
		}

    /* Check self-intersecting curves. */
	for (i = path->iSeg; i < iEnd; i++)
		{
		Segment *s = &h->segs.array[i];
        if (!(s->flags & SEG_LINE) &&
            checkSelfIsectCurve(&s->p0, &s->p1, &s->p2, &s->p3))
            {
            selfIsectCurve(h, s);
            }
		}
	}

/* Compare intersections by ISECT_DUP, segment, t. */
static int CTL_CDECL cmpSegs(const void *first, const void *second)
	{
	Intersect *a = (Intersect *)first;
	Intersect *b = (Intersect *)second;
    int da = (a->flags & ISECT_DUP) != 0;
    int db = (b->flags & ISECT_DUP) != 0;

	if (da < db)
		return -1;
	else if (da > db)
		return 1;
	else if (a->iSeg < b->iSeg)
		return -1;
	else if (a->iSeg > b->iSeg)
		return 1;
	else if (a->t < b->t)
		return -1;
	else if (a->t > b->t)
		return 1;
	else
		return 0;
	}

/* Return 1 if curve is can be converted into a line else 0. */
static int curveIsLine(abfCtx h, Segment *seg)
	{
	float a = seg->p3.y - seg->p0.y;
	float b = seg->p3.x - seg->p0.x;
	if (fabs(a) <= 1 && fabs(b) <= 1)
		return 1;	/* Segment within unit square */
	else
		{
		float s = a*(seg->p1.x - seg->p0.x) + b*(seg->p0.y - seg->p1.y);
		float t = a*(seg->p2.x - seg->p0.x) + b*(seg->p0.y - seg->p2.y);
		float tol = MILLIEM(1);
		return (s*s + t*t) < tol*tol*(a*a + b*b);
		}
	}

/* Split segment at intersection point. */
static void splitSegment(abfCtx h, Intersect *last, Intersect *isect)
	{
	long iNew;
	Segment *_new;
	Segment *seg;
	float t = isect->t;
	long iSeg = isect->iSeg;

	if (last != NULL && last->iSeg == isect->iSeg)
		{
		/* Second or subsequent intercept on same segment */
		if (t == last->t || t == 1)
			{
			isect->iSplit = last->iSplit;
			return;
			}
		else if (last->iSplit != -1)
			{
			t = (t - last->t)/(1 - last->t);
			iSeg = last->iSplit;
			seg = &h->segs.array[iSeg];
			}
		}
	else if (t == 0 || t == 1)
		return;

	/* Split segment; allocate new segment */
	iNew = dnaNext(&h->segs, sizeof(Segment));
	if (iNew == -1)
		fatal(h, abfErrNoMemory);
	_new = &h->segs.array[iNew];
	seg = &h->segs.array[iSeg];

	if (seg->flags & SEG_LINE)
		{
		/* Split line segment */
		_new->p0 = isect->p;
		_new->p3 = seg->p3;
		seg->p3 = isect->p;

		_new->flags = SEG_LINE;
		}
	else
		{
		/* Split curve segment */
		Point p0 = seg->p0;
		Point p1 = seg->p1;
		Point p2 = seg->p2;
		Point p3 = seg->p3;

		seg->p1.x = t*(p1.x - p0.x) + seg->p0.x;
		seg->p1.y = t*(p1.y - p0.y) + seg->p0.y;
		seg->p2.x = t*t*(p2.x - 2*p1.x + p0.x) + 2*seg->p1.x - seg->p0.x;
		seg->p2.y = t*t*(p2.y - 2*p1.y + p0.y) + 2*seg->p1.y - seg->p0.y;
		seg->p3 = isect->p; 
		if (curveIsLine(h, seg))
			seg->flags |= SEG_LINE;	/* Make line segment */

		t = 1 - t;
		_new->p3 = p3;
		_new->p2.x = t*(p2.x - p3.x) + _new->p3.x;
		_new->p2.y = t*(p2.y - p3.y) + _new->p3.y;
		_new->p1.x = t*t*(p1.x - 2*p2.x + p3.x) + 2*_new->p2.x - _new->p3.x;
		_new->p1.y = t*t*(p1.y - 2*p2.y + p3.y) + 2*_new->p2.y - _new->p3.y;
		_new->p0 = seg->p3;
		_new->flags = 0;
		if (curveIsLine(h, _new))
			_new->flags |= SEG_LINE;	/* Make line segment */
		}

	/* Link _new segment */
	_new->iPrev = iSeg;
	_new->iNext = seg->iNext;
	_new->iPath = seg->iPath;
    _new->inJunc = -1;
    _new->outJunc = -1;

    /* Set _new bounds */
    setSegBounds(seg);
    setSegBounds(_new);

	seg->iNext = iNew;
	h->segs.array[_new->iNext].iPrev = iNew;

	isect->iSplit = iNew;
	}

#if ROUND_ISECT_COORDS
/* Round points in segment to nearest integer. */
static void roundSegment(abfCtx h, long iSeg, int beg)
	{
	Segment *seg = &h->segs.array[iSeg];

    /* Don't round an unsplit end point otherwise it may make a gap */
    if (beg)
        {
        seg->p0.x = RND(seg->p0.x);
        seg->p0.y = RND(seg->p0.y);
        }
    else
        {
        seg->p3.x = RND(seg->p3.x);
        seg->p3.y = RND(seg->p3.y);
        }
	if (!(seg->flags & SEG_LINE))
		{
		seg->p1.x = RND(seg->p1.x);
		seg->p1.y = RND(seg->p1.y);
		seg->p2.x = RND(seg->p2.x);
		seg->p2.y = RND(seg->p2.y);
		if (curveIsLine(h, seg))
			seg->flags |= SEG_LINE;	/* Make line segment */
		}
	setSegBounds(seg);
	}
#endif

/* Squash a short negligible segment and update intersections. Returns 1 if any segment is squashed. */
static int squashShortSeg(abfCtx h, Intersect *isect, int isSplit, int atBeg, int *isectSquashed)
    {
    long    iSeg = isSplit? isect->iSplit: isect->iSeg;
    Segment *seg = &h->segs.array[iSeg];
    int segSquashed = 0;

    if ((seg->flags & SEG_LINE) &&
        (seg->bounds.right - seg->bounds.left <= SHORT_LINE_LIMIT) &&
        (seg->bounds.top - seg->bounds.bottom <= SHORT_LINE_LIMIT))
        {
        /* Remove this short segment at an intersection from linked segments, isect, and path */

        if (seg->iNext != iSeg && seg->iPrev != iSeg)
            {
            Path    *path;
            long    iRep;
            Segment *rep;
            long    i;

            segSquashed = 1;
            /* Replace the short segment with the next/previous segment in the link */
            if (isSplit ^ atBeg)
                {
                iRep = seg->iNext;
                rep = &h->segs.array[iRep];
                rep->p0 = isect->p;
                }
            else
                {
                iRep = seg->iPrev;
                rep = &h->segs.array[iRep];
                rep->p3 = isect->p;
                }
            if (isSplit)
                isect->iSplit = iRep;
            else
                isect->iSeg = iRep;
            rep->flags |= SEG_ISECT;

            /* Update segment link and bounds */
            h->segs.array[seg->iPrev].iNext = seg->iNext;
            h->segs.array[seg->iNext].iPrev = seg->iPrev;
            setSegBounds(rep);

            /* Update path if necessary */
            path = &h->paths.array[seg->iPath];
            if (path->iSeg == iSeg)
                path->iSeg = iRep;

            /* Delete the short segment */
            seg->flags |= SEG_DELETE;

            for (i = 0; i < h->isects.cnt; i++)
                {
                Intersect   *i2 = &h->isects.array[i];

                if (!(i2->flags & ISECT_DUP))
                    {
                    if (i2->iSeg == iSeg)
                        i2->iSeg = iRep;
                    if (i2->iSplit == iSeg)
                        i2->iSplit = iRep;

                    /* If removing the segment degenerates an intersection, squash it as well */
                    if (i2->iSeg == i2->iSplit)
                        {
                        i2->flags |= ISECT_DUP;
                        *isectSquashed = 1;
                        }
                    }
                }
            }
        }
    return segSquashed;
    }

/* Split intersecting segments. */
static void splitIsectSegs(abfCtx h)
	{
	long i;
	Intersect *last;
	Intersect *isect;
    int isectSquashed = 0;

	/* Sort list by segment */
	qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpSegs);

    /* Look for a segment intersecting at its end. If there is one,
       check if the next segment in the same path is intersecting at its beginning.
       Such an interseciton is a duplicate and marked as such and merged together
       with the same intersection ID. */
    for (i = 0; i < h->isects.cnt; i++)
        {
        long iSeg, iNextSeg, j;
        Intersect *next = NULL;
        int found = 0;

		isect = &h->isects.array[i];
        if ((isect->flags & ISECT_DUP) || isect->t != 1)
            continue;
        iSeg = isect->iSeg;
        iNextSeg = h->segs.array[iSeg].iNext;

        j = (iNextSeg > iSeg)? i+1: 0;
        for (; j < h->isects.cnt; j++)
            {
            next = &h->isects.array[j];
            if (next->iSeg == iNextSeg && next->t == 0)
                {
                found = 1;
                break;
                }
            else if (next->iSeg > iNextSeg)
                break;
            }
        if (found)
            {
            long origId = isect->id;
            isect->flags |= ISECT_DUP;
            if (isect->id != next->id)
                {
                for (j = 0; j < h->isects.cnt; j++)
                    {
                    Intersect *i1 = &h->isects.array[j];
                    if (i1->id == origId)
                        i1->id = next->id;
                    }
                }
            }
        }

	/* Split segments */
	last = NULL;
	for (i = 0; i < h->isects.cnt; i++)
		{
		isect = &h->isects.array[i];
        if (last && isect->id == last->id &&
            isect->iSeg == last->iSeg)
            {
            isect->flags |= ISECT_DUP;
            continue;
            }
		splitSegment(h, last, isect);
		last = isect;
		}

	/* Move duplicate intersections to end */
	qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpSegs);

	/* Update connection of multi-split segments */
	for (i = h->isects.cnt - 1; i > 0; i--)
		{
		last = &h->isects.array[i - 1];
		isect = &h->isects.array[i];
        
        /* Remove duplicate intersections from list */
        if (isect->flags & ISECT_DUP)
            {
            dnaSET_CNT(h->isects, i);
            continue;
            }
		if (isect->iSplit != -1 && 
			last->iSeg == isect->iSeg && last->iSplit != -1)
			{
			isect->iSeg = last->iSplit;
			isect->iSplit = h->segs.array[isect->iSeg].iNext;
			}
		}

	/* Update unsplit segment connections and round end coordinates */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Segment *seg;
        int atBeg;
		isect = &h->isects.array[i];
		seg = &h->segs.array[isect->iSeg];

#if ROUND_ISECT_COORDS
        isect->p.x = RND(isect->p.x);
        isect->p.y = RND(isect->p.y);
#endif
        atBeg = (isect->t == 0);
		if (isect->iSplit == -1)
			isect->iSplit = atBeg? seg->iPrev: seg->iNext;
#if ROUND_ISECT_COORDS
        roundSegment(h, isect->iSeg, atBeg);
        roundSegment(h, isect->iSplit, !atBeg);
#endif
		/* Mark segments as intersected */
		seg->flags |= SEG_ISECT;
		h->segs.array[isect->iSplit].flags |= SEG_ISECT;
		}
    /* Remove short segments from each intersection. */
retry:
	for (i = 0; i < h->isects.cnt; i++)
		{
		isect = &h->isects.array[i];

        if (!(isect->flags & ISECT_DUP))
            {
            int atBeg = (isect->t == 0);
            int segSquashed = 0;
            segSquashed |= squashShortSeg(h, isect, 0, atBeg, &isectSquashed);
            segSquashed |= squashShortSeg(h, isect, 1, atBeg, &isectSquashed);
            if (segSquashed)
                goto retry;  /* restart scan for short segments */
            }
        }

    /* If any intersections are squashed above, remove them from list */
    if (isectSquashed)
        {
        qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpSegs);
        for (i = h->isects.cnt - 1; i > 0; i--)
            {
            isect = &h->isects.array[i];
            
            /* Remove squashed intersections from list */
            if (!(isect->flags & ISECT_DUP))
                break;
            else
                dnaSET_CNT(h->isects, i);
            }
        }
	}

/* Compare intersections by id. */
static int CTL_CDECL cmpIds(const void *first, const void *second)
	{
	long a = ((Intersect *)first)->id;
	long b = ((Intersect *)second)->id;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Get winding for beg and end values. */
static int getWind(float beg, float end)
	{
	return (beg > end)? 1: -1;
	}

/* Split bezier curve at t. */
static void splitBez(Bezier *a, Bezier *b, float t)
	{
	Point p0 = a->p0;
	Point p1 = a->p1;
	Point p2 = a->p2;
	Point p3 = a->p3;

	a->p1.x = t*(p1.x - p0.x) + a->p0.x;
	a->p1.y = t*(p1.y - p0.y) + a->p0.y;
	a->p2.x = t*t*(p2.x - 2*p1.x + p0.x) + 2*a->p1.x - a->p0.x;
	a->p2.y = t*t*(p2.y - 2*p1.y + p0.y) + 2*a->p1.y - a->p0.y;
	a->p3.x = t*t*t*(p3.x - 3*(p2.x - p1.x) - p0.x) + 
		3*(a->p2.x - a->p1.x) + a->p0.x;
	a->p3.y = t*t*t*(p3.y - 3*(p2.y - p1.y) - p0.y) + 
		3*(a->p2.y - a->p1.y) + a->p0.y;
	b->t1 = a->t1;
    b->t0 = a->t1 = t;

	t = 1 - t;
	b->p3 = p3;
	b->p2.x = t*(p2.x - p3.x) + b->p3.x;
	b->p2.y = t*(p2.y - p3.y) + b->p3.y;
	b->p1.x = t*t*(p1.x - 2*p2.x + p3.x) + 2*b->p2.x - b->p3.x;
	b->p1.y = t*t*(p1.y - 2*p2.y + p3.y) + 2*b->p2.y - b->p3.y;
	b->p0 = a->p3;
	}

/* Find t value that yeilds the target value from Bezier equation. */
static float solveBezAtValue(float value, 
							 float p3, float p2, float p1, float p0)
	{	
	float a = p3 - 3*(p2 - p1) - p0;
	float b = 3*(p2 - 2*p1 + p0);
	float c = 3*(p1 - p0);
	float lo = 0;
	float hi = 1;
	float t;

	/* Binary search for solution */
	for (;;)
		{
		float delta;
		
		t = (lo + hi)/2;
		delta = t*(t*(t*a + b) + c) + p0 - value;
		if (fabs(delta) < 0.125)
			break;
		else if (delta < 0)
			lo = t;
		else
			hi = t;
		}
	return t;
	}

/* Solve segment for x value(s) at given y. */
static int solveSegAtY(Segment *seg, float y, float icepts[3], int wind[3])
	{
	Bezier q[3];
    Extrema *ex;
	int cnt;
	int i;
	int j;

	if (seg->flags & SEG_LINE)
		{
		icepts[0] = seg->p0.x + 
			(y - seg->p0.y)*(seg->p3.x - seg->p0.x)/(seg->p3.y - seg->p0.y);
		wind[0] = getWind(seg->p0.y, seg->p3.y);
		return 1;
		}

	/* Copy curve */
	q[0].p0 = seg->p0;
	q[0].p1 = seg->p1;
	q[0].p2 = seg->p2;
	q[0].p3 = seg->p3;

	/* Split curve at extrema */
    ex = &seg->yExtrema;
    cnt = ex->cnt;

	/* Split curve(s) */
	if (cnt > 0)
		splitBez(&q[0], &q[1], ex->t[0]);
	if (cnt > 1)
		splitBez(&q[1], &q[2], (ex->t[1] - ex->t[0])/(1 - ex->t[0]));
	cnt++;

	j = 0;
	for (i = 0; i < cnt; i++)
		{
		Point p0;
		Point p1;
		Point p2;
		Point p3;

		/* Orient curve */
		if (q[i].p3.y > q[i].p0.y)
			{
			p0 = q[i].p0;
			p1 = q[i].p1;
			p2 = q[i].p2;
			p3 = q[i].p3;
			}
		else
			{
			p0 = q[i].p3;
			p1 = q[i].p2;
			p2 = q[i].p1;
			p3 = q[i].p0;
			}

		wind[j] = getWind(q[i].p0.y, q[i].p3.y);

		if (y < p0.y || y > p3.y)
			continue;
		else if (y == p0.y)
			icepts[j++] = p0.x;
		else if (y == p3.y)
			icepts[j++] = p3.x;
		else
			{
			float t = solveBezAtValue(y, p3.y, p2.y, p1.y, p0.y);
			icepts[j++] = t*(t*(t*(p3.x - 3*(p2.x - p1.x) - p0.x) +
								3*(p2.x - 2*p1.x + p0.x)) +
							 3*(p1.x - p0.x)) + p0.x;
			}
		}

	return j;
	}

/* Solve segment for y value(s) at given x. */
static int solveSegAtX(Segment *seg, float x, float icepts[3], int wind[3])
	{
	Bezier q[3];
    Extrema *ex;
	int cnt;
	int i;
	int j;

	if (seg->flags & SEG_LINE)
		{
		icepts[0] = seg->p0.y + 
			(x - seg->p0.x)*(seg->p3.y - seg->p0.y)/(seg->p3.x - seg->p0.x);
		wind[0] = getWind(seg->p0.x, seg->p3.x);
		return 1;
		}

	/* Copy curve */
	q[0].p0 = seg->p0;
	q[0].p1 = seg->p1;
	q[0].p2 = seg->p2;
	q[0].p3 = seg->p3;

	/* Split curve at extrema */
    ex = &seg->xExtrema;
    cnt = ex->cnt;

	/* Split curve(s) */
	if (cnt > 0)
		splitBez(&q[0], &q[1], ex->t[0]);
	if (cnt > 1)
		splitBez(&q[1], &q[2], (ex->t[1] - ex->t[0])/(1 - ex->t[0]));
	cnt++;

	j = 0;
	for (i = 0; i < cnt; i++)
		{
		Point p0;
		Point p1;
		Point p2;
		Point p3;

		/* Orient curve */
		if (q[i].p3.x > q[i].p0.x)
			{
			p0 = q[i].p0;
			p1 = q[i].p1;
			p2 = q[i].p2;
			p3 = q[i].p3;
			}
		else
			{
			p0 = q[i].p3;
			p1 = q[i].p2;
			p2 = q[i].p1;
			p3 = q[i].p0;
			}

		wind[j] = getWind(q[i].p0.x, q[i].p3.x);

		if (x < p0.x || x > p3.x)
			continue;
		else if (x == p0.x)
			icepts[j++] = p0.y;
		else if (x == p3.x)
			icepts[j++] = p3.y;
		else
			{
			float t = solveBezAtValue(x, p3.x, p2.x, p1.x, p0.x);
			icepts[j++] = t*(t*(t*(p3.y - 3*(p2.y - p1.y) - p0.y) +
								3*(p2.y - 2*p1.y + p0.y)) +
							 3*(p1.y - p0.y)) + p0.y;
			}
		}

	return j;
	}

/* Get segment winding at specified value. */
static int getWindAtValue(float beg, float end, float value)
	{
	if ((value < beg && value < end) || (value > beg && value > end))
		return 0;
	else
		return (beg > end)? 1: -1;
	}

static float penalizeCloseCross(float v1, float v2)
    {
    float   diff = (float)fabs(v1 - v2);
    if (diff < WIND_TEST_EPSILON)
        return WIND_TEST_EPSILON - diff;
    else
        return 0;
    }

static long searchValueList(ValueList *list, float val)
    {
    long    lo = 0;
    long    hi = list->cnt - 1;
    long    t = 0;

	/* Binary search for value */
	while (lo <= hi)
		{
        float   test;

		t = (lo + hi)/2;
        test = list->array[t] - val;
        if (test == 0)
            return t;
		else if (test < 0)
			lo = t + 1;
		else
			hi = t - 1;
		}
	return t;
    }

/* Pick a coordinate value for a segment winding test.
   Any value between the two ends of the segment should work in theory,
   however in order to minimize the chance of math errors, avoid proximity of
   end points and extrema of other segments utilizing extrema lists collected earlier. */
static void chooseTargetPoint(abfCtx h, Segment *seg, Point *mid, int testvert)
    {
    float   v;
    float   min, max;
    float   gapVal;
    long    i, gapIndex = 0;
    ValueList   *list;

    if (testvert)
        {
        min = seg->p0.x;
        max = seg->p3.x;
        list = &h->xExtremaList;
        }
        else
        {
        min = seg->p0.y;
        max = seg->p3.y;
        list = &h->yExtremaList;
        }

    if (min == max)
        v = min;
    else
        {
        if (min > max)
            {
            float   temp = min;
            min = max;
            max = temp;
            }

        /* Search the extrema list for the widest gap between the segment ends. */
        i = searchValueList(list, min);
        gapVal = -MAXFLOAT;
        for (; list->array[i] < max; i++)
            {
            float   w = list->array[i+1] - list->array[i];

            /* penalize gaps at both ends of this segment */
            if (list->array[i] <= min || list->array[i+1] >= max)
                w -= 5.0f;
            if (gapVal < w)
                {
                gapVal = w;
                gapIndex = i;
                }
            }
        v = (list->array[gapIndex] + list->array[gapIndex+1])/2;
    #if ABF_DEBUG
        if (v <= min || v >= max)
            fatal(h, abfErrCantHandle);
    #endif
    }

    /* Determine the other coordinate value corresponding to v. */
    if (seg->flags & SEG_LINE)
        {
        if (testvert)
            {
            mid->x = v;
            mid->y = ((seg->p3.y - seg->p0.y) * (v - seg->p0.x) + (seg->p3.x - seg->p0.x) * seg->p0.y) / (seg->p3.x - seg->p0.x);
            }
        else
            {
            mid->x = ((seg->p3.x - seg->p0.x) * (v - seg->p0.y) + (seg->p3.y - seg->p0.y) * seg->p0.x) / (seg->p3.y - seg->p0.y);
            mid->y = v;
            }
        }
    else
        {
        int cnt;
        float   icepts[3];
        int     wind[3];    /* unused */

        if (testvert)
            {
            mid->x = v;
            cnt = solveSegAtX(seg, v, icepts, wind);
#if ABF_DEBUG
            if (cnt <= 0)
                fatal(h, abfErrCantHandle);
#endif
            mid->y = icepts[0];
            }
        else
            {
            mid->y = v;
            cnt = solveSegAtY(seg, v, icepts, wind);
#if ABF_DEBUG
            if (cnt <= 0)
                fatal(h, abfErrCantHandle);
#endif
            mid->x = icepts[0];
            }
        }
    }

/* Rough estimate of segment length for heuristics use. */
static float estSegLen(Segment *seg)
    {
    return (float)(fabs(seg->bounds.right - seg->bounds.left) + fabs(seg->bounds.top - seg->bounds.bottom));
    }

/* Set segment score from winding count, its confidence, and the segment length */
static void setWindScore(Segment *seg, int windCount, float conf)
    {
        float score = (float)windCount/2 + conf;
        float len = estSegLen(seg);

        score = (float)((score * 0.5) + 1.0);
        score = MAX(score, 0);

        /* Less confident with short segments */
        score *= log10f(len);

        seg->score = score;
    }

/* Test segment winding on both sides and set flags accordingly.
   If both sides are filled/unfilled, delete the segment.
   If the wrong side of the segment is filled, reverse its direction. */
static void windTestSeg(abfCtx h, long iTarget)
	{
    Point p;
	float icepts[3];
	long iPath;
	long iBegPath;
	int i;
	int wind[3];
	int testvert;
	int windTotalLo, windTotalHi;
    float score;
	int windtarget;
	Segment *target = &h->segs.array[iTarget];

	if (target->flags & (SEG_WIND_TEST|SEG_DELETE))
		return;	/* Ignore checked or deleted segments */

	windTotalLo = windTotalHi = 0;
    score = 0;
	testvert = (fabs(target->p3.x - target->p0.x) >
				fabs(target->p3.y - target->p0.y));
	if (testvert)
		windtarget = getWind(target->p0.x, target->p3.x);
	else
		windtarget = getWind(target->p0.y, target->p3.y);

	/* Choose a target point geometrically between the two ends of the segment */
    chooseTargetPoint(h, target, &p, testvert);

	iBegPath = target->iPath;
	iPath = iBegPath;
	do
		{
		long iBegSeg;
		long iSeg;
		Path *path = &h->paths.array[iPath];
		/* Ignore paths that can't affect winding */
		if (testvert)
			{
			if (path->bounds.left > p.x ||
				path->bounds.right <= p.x)
				goto nextpath;
			}
		else
			{
			if (path->bounds.bottom > p.y ||
				path->bounds.top <= p.y)
				goto nextpath;
			}
		iBegSeg = path->iSeg; 
		iSeg = iBegSeg;
		do
			{
			Segment *seg = &h->segs.array[iSeg];
			if (iSeg == iTarget && (seg->flags & SEG_LINE))
                ;   /* Winding test with the self-curve is taken care of below */
			else if (testvert)
				{
				/* Consider only non-vertical segments below or above. */
				if (seg->bounds.left > p.x ||
					seg->bounds.right <= p.x ||
					seg->bounds.left == seg->bounds.right)
					;
				else if (seg->bounds.top < p.y - WIND_TEST_EPSILON)
					windTotalLo += getWindAtValue(seg->p0.x, seg->p3.x, p.x);
				else if (seg->bounds.bottom > p.y + WIND_TEST_EPSILON)
					windTotalHi += getWindAtValue(seg->p0.x, seg->p3.x, p.x);
				else
                    {
                    /* Check coincident segments. Delete all except at most one undeleted. */
                    if ((iSeg != iTarget) &&
                        ((seg->flags & SEG_LINE) == (target->flags & SEG_LINE)) &&
                        /* Coincident in opposite direction */
                        (((seg->p0.x == target->p3.x &&
						  seg->p0.y == target->p3.y &&
						  seg->p3.x == target->p0.x &&
						  seg->p3.y == target->p0.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p2.x &&
						   seg->p1.y == target->p2.y &&
						   seg->p2.x == target->p1.x &&
						   seg->p2.y == target->p1.y)))
                        /* Coincident in same direction */
                        || ((seg->p0.x == target->p0.x &&
                          seg->p0.y == target->p0.y &&
                          seg->p3.x == target->p3.x &&
                          seg->p3.y == target->p3.y) &&
                         (seg->flags & SEG_LINE ||
                          (seg->p1.x == target->p1.x &&
                           seg->p1.y == target->p1.y &&
                           seg->p2.x == target->p2.x &&
                           seg->p2.y == target->p2.y)))))
                        {
                        if (!(seg->flags & SEG_DELETE))
                            {
                            target->flags |= (SEG_DELETE|SEG_WIND_TEST);
                            setWindScore(target, 1, score);
                            return;
                            }
                        }
                    else
                        {
                        int cnt = solveSegAtX(seg, p.x, icepts, wind);
                        int self = -1;
                        float mindiff = MAXFLOAT;
                        if (iTarget == iSeg)
                            {
                            if (cnt == 1)
                                self = 0;
                            else
                                for (i = 0; i < cnt; i++)
                                    {
                                    float d = (float)fabs(icepts[i] - p.y);
                                    if (d < mindiff)
                                        {
                                        self = i;
                                        mindiff = d;
                                        }
                                    }
                            }
                        for (i = 0; i < cnt; i++)
                            {
                            int w = wind[i];
                            if (i != self && w)
                                {
                                float iy = icepts[i];
                                if (iy != p.y)
                                    {
                                    if (iy < p.y)
                                        windTotalLo += w;
                                    else
                                        windTotalHi += w;

                                    score -= penalizeCloseCross(iy, p.y);
                                    }
                                else
                                    score -= 1.0f;  /* overlapping */
                                }
                            }
                        }
                    }
				}
			else
				{
				/* Consider only non-horizontal segments on the left or on the right */
				if (seg->bounds.bottom > p.y ||
					seg->bounds.top <= p.y ||
					seg->bounds.top == seg->bounds.bottom)
					;
				else if (seg->bounds.right < p.x - WIND_TEST_EPSILON)
					windTotalLo += getWindAtValue(seg->p0.y, seg->p3.y, p.y);
				else if (seg->bounds.left > p.x + WIND_TEST_EPSILON)
					windTotalHi += getWindAtValue(seg->p0.y, seg->p3.y, p.y);
                else
                    {
                    /* Check coincident segments. Delete all except at most one undeleted. */
                    if ((iSeg != iTarget) &&
                        ((seg->flags & SEG_LINE) == (target->flags & SEG_LINE)) &&
                        /* Coincident in opposite direction */
                        (((seg->p0.x == target->p3.x &&
						  seg->p0.y == target->p3.y &&
						  seg->p3.x == target->p0.x &&
						  seg->p3.y == target->p0.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p2.x &&
						   seg->p1.y == target->p2.y &&
						   seg->p2.x == target->p1.x &&
						   seg->p2.y == target->p1.y)))
                        /* Coincident in same direction */
                        || ((seg->p0.x == target->p0.x &&
						  seg->p0.y == target->p0.y &&
						  seg->p3.x == target->p3.x &&
						  seg->p3.y == target->p3.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p1.x &&
						   seg->p1.y == target->p1.y &&
						   seg->p2.x == target->p2.x &&
						   seg->p2.y == target->p2.y)))))
                        {
                        if (!(seg->flags & SEG_DELETE))
                            {
                            target->flags |= (SEG_DELETE|SEG_WIND_TEST);
                            setWindScore(target, 1, score);
                            return;
                            }
                        }
                    else
                        {
                        int cnt = solveSegAtY(seg, p.y, icepts, wind);
                        int self = -1;
                        float mindiff = MAXFLOAT;
                        if (iTarget == iSeg)
                            {
                            if (cnt == 1)
                                self = 0;
                            else
                                for (i = 0; i < cnt; i++)
                                    {
                                    float d = (float)fabs(icepts[i] - p.x);
                                    if (d < mindiff)
                                        {
                                        self = i;
                                        mindiff = d;
                                        }
                                    }
                            }
                        for (i = 0; i < cnt; i++)
                            {
                            int w = wind[i];
                            if (i != self && w)
                                {
                                float ix = icepts[i];
                                if (ix != p.x)
                                    {
                                    if (ix < p.x)
                                        windTotalLo += w;
                                    else
                                        windTotalHi += w;

                                    score -= penalizeCloseCross(ix, p.x);
                                    }
                                else
                                    score -= 1.0f;  /* overlapping */
                                }
                            }
                        }
                    }
				}
			iSeg = seg->iNext;
			}
		while (iSeg != iBegSeg);
	nextpath:
		iPath = path->iNext;
		}
	while (iPath != iBegPath);

    /* If both sides of the segment is to be filled or unfilled, delete it */
	if ((windTotalLo != 0) == (windTotalHi != 0))
        {
        /* A segment with (almost) overlapping segments is a candidate for undeletion later. */
        setWindScore(target, MIN(abs(windTotalLo), abs(windTotalHi)), score);
		target->flags |= SEG_DELETE;
        }
    else
        {
        /* A segment with (almost) overlapping segments is a candidate for deletion later */
        setWindScore(target, MAX(abs(windTotalLo), abs(windTotalHi)), score);

        /* If the wrong side (i.e., not on the left) of the segment is filled,
           set the flag so that the segment will be reversed */
        if ((testvert && ((windtarget > 0) == (windTotalHi != 0)))
            || (!testvert && ((windtarget < 0) == (windTotalHi != 0))))
            target->flags |= SEG_REVERSE;
        }

	target->flags |= SEG_WIND_TEST;
	}

static float segmentAngle(Segment *seg, int end)
    {
        Point   vec;

        if (seg->flags & SEG_LINE)
            {
            if (end)
                {
                vec.x = seg->p0.x - seg->p3.x;
                vec.y = seg->p0.y - seg->p3.y;
                }
            else
                {
                vec.x = seg->p3.x - seg->p0.x;
                vec.y = seg->p3.y - seg->p0.y;
                }
            }
        else
            {
            if (end)
                {
                vec.x = seg->p2.x - seg->p3.x;
                vec.y = seg->p2.y - seg->p3.y;
                }
            else
                {
                vec.x = seg->p1.x - seg->p0.x;
                vec.y = seg->p1.y - seg->p0.y;
                }
            }

        return atan2f(vec.y, vec.x);
    }

/* Connect a segment to a junction. */
static Junction *connectSegToJunction(abfCtx h, Intersect *isect, long iJunc, Junction *junc, long iSeg, int in)
    {
    Segment *seg = &h->segs.array[iSeg];
    int reverse;
    int del;
    int init = iJunc >= h->juncs.cnt;
    Outlet  *outlet;

    junc = dnaMAX(h->juncs, iJunc);

    if (init)
        {
        dnaINIT(h->safe, junc->outlets, 2, 5);
        junc->p = isect->p;
        junc->inCount = junc->outCount = 0;
        }

    del = (seg->flags & SEG_DELETE) != 0;
    reverse = (seg->flags & SEG_REVERSE) != 0;
    outlet = dnaNEXT(junc->outlets);
    outlet->seg = iSeg;
    outlet->otherJunc = -1;
    outlet->otherSeg = -1;
    outlet->score = 0;
    outlet->flags = 0;

    if (del)
        outlet->flags |= OUTLET_DELETE;
    if (in == !reverse)
        {
        if (!del)
            junc->inCount++;
        outlet->angle = segmentAngle(seg, !reverse);
        seg->inJunc = iJunc;
        }
    else
        {
        if (!del)
            junc->outCount++;
        outlet->flags |= OUTLET_OUT;
        outlet->angle = segmentAngle(seg, reverse);
        seg->outJunc = iJunc;
        }

    return junc;
    }

/* Compare outlets by angle. */
static int CTL_CDECL cmpAngles(const void *first, const void *second)
	{
	float a = ((Outlet *)first)->angle;
	float b = ((Outlet *)second)->angle;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Sort outlet segments at each junction by angle */
static void sortOutlets(abfCtx h)
    {
    long    i, j, k;
    Junction *junc;

    for (j = 0; j < h->juncs.cnt; j++)
        {
        OutletList  *list;
        junc = &h->juncs.array[j];
        list = &junc->outlets;
        qsort(list->array, junc->outlets.cnt, sizeof(Outlet), cmpAngles);

        /* Remove duplicate outlets if any */
        for (i = 0; i < list->cnt; i++)
            {
            long    flags = list->array[i].flags;
            for (k = i + 1; k < list->cnt; k++)
                {
                if (list->array[i].seg == list->array[k].seg &&
                    list->array[i].angle == list->array[k].angle &&
                    list->array[i].flags == list->array[k].flags)
                    {
                    memmove(&list->array[k], &list->array[k+1], sizeof(Outlet) * (list->cnt-k-1));
                    list->cnt--;
                    if (!(flags & OUTLET_DELETE))
                        {
                        if (flags & OUTLET_OUT)
                            junc->outCount--;
                        else
                            junc->inCount--;
                        }
                    }
                }
            }
        }
    }

/* Reset all path and segment flags */
static void resetSegFlags(abfCtx h, long iBeg, long pathFlags, long segFlags)
    {
    long    i;
    long    iPath = iBeg;
    do
        {
        Path *path = &h->paths.array[iPath];
        path->flags &= ~pathFlags;
        iPath = path->iNext;
        i = path->iSeg;
        do
            {
            Segment *seg = &h->segs.array[i];
            seg->flags &= ~segFlags;
            i = seg->iNext;
            }
        while (i != path->iSeg);
        }
    while (iPath != iBeg);
    }

static long lookupOutlet(abfCtx h, Junction *junc, long iSeg, long flags)
    {
    long    i, segMatch = -1;
    OutletList  *list = &junc->outlets;
    Outlet  *outlet = NULL;
    long    mask = OUTLET_OUT;

    for (i = 0; i < list->cnt; i++)
        {
        outlet = &list->array[i];
        if (outlet->seg == iSeg)
            {
            segMatch = i;
            if ((outlet->flags & mask) == (flags & mask))
                return i;
            }
        }
    return segMatch;
    }

static void modifyOutlet(abfCtx h, Junction *junc, long iSeg, long oldFlags, long newFlags, float adjustScore)
    {
    long    i;
    //OutletList  *list = &junc->outlets;
    Outlet  *outlet;
    long    mask = OUTLET_OUT|OUTLET_DELETE;

    i = lookupOutlet(h, junc, iSeg, oldFlags);
    if (i < 0)
        fatal(h, abfErrCantHandle);

    outlet = &junc->outlets.array[i];
    outlet->flags = (outlet->flags & ~mask) | newFlags;
    outlet->score += adjustScore;

    if (!(oldFlags & OUTLET_DELETE))
        {
        if (oldFlags & OUTLET_OUT)
            junc->outCount--;
        else
            junc->inCount--;
        }
    if (!(newFlags & OUTLET_DELETE))
        {
        if (newFlags & OUTLET_OUT)
            junc->outCount++;
        else
            junc->inCount++;
        }
    }

/* Modify flags of segments in a sequence. */
static void modifySegSeqFlags(abfCtx h, long iSeg, long iEndSeg, int forw, long mask, long flags)
    {
    for (;;)
        {
        Segment *seg = &h->segs.array[iSeg];
        if ((seg->flags & mask & SEG_REVERSE) != (flags & SEG_REVERSE))
            {
            long    inJunc = seg->inJunc;
            seg->inJunc = seg->outJunc;
            seg->outJunc = inJunc;
            }
        seg->flags = (seg->flags & ~mask) | flags;
        if (iSeg == iEndSeg)
            break;

        if (forw)
            iSeg = seg->iNext;
        else
            iSeg = seg->iPrev;
        }
}

/* Check consistency of a sequence of segments starting from this outlet. */
static void polishOutlet(abfCtx h, long iJunc, Outlet *begOutlet)
    {
    Junction    *begJunc = &h->juncs.array[iJunc];
    long    iBegSeg = begOutlet->seg;
    long    iSeg = iBegSeg;
    long    iEndSeg;
    Segment *begSeg = &h->segs.array[iBegSeg];
    Segment *seg;
    int out = (begOutlet->flags & OUTLET_OUT) != 0;
    long begRevFlag = (begSeg->flags & SEG_REVERSE);
    int forw = out ^ (begRevFlag != 0);
    long revFlag, endRevFlag;
    int segCount;
    int undelCount = 0;
    int delCount = 0;
    int revCount = 0;
    float totalRevScore = 0;
    float totalUndelScore = 0;
    float totalDelScore = 0;
    float totalScore;
    long iEndJunc;
    long iEndOutlet;
    Outlet *endOutlet;
    Junction *endJunc = NULL;
    float   score = 0;

    if (begOutlet->flags & OUTLET_VISITED)
        return;
    begOutlet->flags |= OUTLET_VISITED;

    seg = begSeg;
    for (;;)
        {
        int     revmis;

        /* Check consistency of segments in a sequence. */
        seg->flags |= SEG_VISITED;
        if (seg->flags & SEG_DELETE)
            {
            delCount++;
            totalDelScore += seg->score;
            }
        else
            {
            undelCount++;
            totalUndelScore += seg->score;
            }

        revFlag = (seg->flags & SEG_REVERSE);
        revmis = (revFlag != begRevFlag);
        if (revmis)
            {
            revCount++;
            totalRevScore += seg->score;
            }

        if (out ^ revmis)
            iEndJunc = seg->inJunc;
        else
            iEndJunc = seg->outJunc;
        if (iEndJunc >= 0)
            {
            endJunc = &h->juncs.array[iEndJunc];
            iEndSeg = iSeg;
            endRevFlag = revFlag;
            break;
            }

        if (forw)
            iSeg = seg->iNext;
        else
            iSeg = seg->iPrev;
        seg = &h->segs.array[iSeg];
        }

    segCount = delCount + undelCount;
    totalScore = totalDelScore + totalUndelScore;
    score = totalScore / segCount;

    /* Set other junction/outlet indices in outlets at both ends */
    begOutlet->otherJunc = iEndJunc;
    begOutlet->otherSeg = iEndSeg;
    iEndOutlet = lookupOutlet(h, endJunc, iEndSeg, begOutlet->flags ^ OUTLET_OUT);
    endOutlet = &endJunc->outlets.array[iEndOutlet];
    endOutlet->otherJunc = iJunc;
    endOutlet->otherSeg = iBegSeg;
    endOutlet->flags |= OUTLET_VISITED;

    if (delCount > 0 && undelCount <= 0)
        {
        /* All segments are deleted. */
        begOutlet->score = score;
        endOutlet->score = score;
        return;
        }

    /* Fix inconsistency problems */

    /* If deletion status is inconsistent among segments in a sequence,
     * delete or undelete the entire sequence depending the average scores between two. */
    if (undelCount > 0 && delCount > 0)
        {
        long segFlag = 0;
        long segMask = SEG_DELETE;
        
        if (totalDelScore > totalUndelScore)
            {
            segFlag = SEG_DELETE;
            segMask = (SEG_DELETE|SEG_REVERSE);
            score -= totalUndelScore / undelCount;
            }
        else
            score -= totalDelScore / delCount;

        /* Delete/undelete the outgoing outlet. */
        if ((h->segs.array[iBegSeg].flags & SEG_DELETE) != segFlag)
            {
            begOutlet->score += score;
            modifyOutlet(h, begJunc, iBegSeg, begOutlet->flags, begOutlet->flags ^ OUTLET_DELETE, 0);
            }

        /* Delete/undelete the incoming outlet. */
        if ((h->segs.array[iEndSeg].flags & SEG_DELETE) != segFlag)
            {
            endOutlet->score += score;
            modifyOutlet(h, endJunc, iEndSeg, endOutlet->flags, endOutlet->flags ^ OUTLET_DELETE, 0);
            }
        
        modifySegSeqFlags(h, iBegSeg, iEndSeg, forw, segMask, segFlag);
        if (segFlag)
            return;

        score = 0;
        }

    /* If any segments have inconsistent reverse flags, then correct minority segments */
    if (revCount > 0)
        {
        if (totalScore > totalRevScore * 2)
            {
            revFlag = begRevFlag;
            score -= totalRevScore / segCount;
            }
        else
            {
            revFlag = begRevFlag ^ SEG_REVERSE;
            score -= (totalScore - totalRevScore) / segCount;
            }

        modifySegSeqFlags(h, iBegSeg, iEndSeg, forw, SEG_REVERSE, revFlag);

        if (revFlag != begRevFlag)
            modifyOutlet(h, begJunc, iBegSeg, begOutlet->flags, begOutlet->flags ^ OUTLET_OUT, score);
        if (revFlag != endRevFlag)
            modifyOutlet(h, endJunc, iEndSeg, endOutlet->flags, endOutlet->flags ^ OUTLET_OUT, score);
        score = 0;
        }

    begOutlet->score += score;
    endOutlet->score += score;
    }

static int signCountDiff(Junction *junc)
    {
        if (junc->outCount > junc->inCount)
            return 1;
        else if (junc->outCount < junc->inCount)
            return -1;
        else
            return 0;
    }

/* Check consistency of all outlets and segments in between, and score outlets, and fix problems. */
static void polishOutlets(abfCtx h, long iBeg)
    {
    long    iJunc, i;
    Junction *junc;

    for (iJunc = 0; iJunc < h->juncs.cnt; iJunc++)
        {
        junc = &h->juncs.array[iJunc];
        for (i = 0; i < junc->outlets.cnt; i++)
            polishOutlet(h, iJunc, &junc->outlets.array[i]);
        }

    /* If the numbers of outlets and inlets at both end junctions of a segment are inconsistent
       in the opposite directions, penalize the outlets so that fixBadJunctions will consider
       the segment as a candidate for flipping its direction or deletion.  */
    for (iJunc = 0; iJunc < h->juncs.cnt; iJunc++)
        {
        junc = &h->juncs.array[iJunc];
        for (i = 0; i < junc->outlets.cnt; i++)
            {
            Outlet *begOutlet = &junc->outlets.array[i];
            Junction *endJunc = &h->juncs.array[begOutlet->otherJunc];
            int begDiff = signCountDiff(junc);
            int endDiff = signCountDiff(endJunc);

            if (begDiff && begDiff == -endDiff)
                {
                int out = (begOutlet->flags & OUTLET_OUT) != 0;
                if (((begOutlet->flags & OUTLET_DELETE) != 0) ||
                    (out && begDiff > 0 && endDiff < 0) ||
                    (!out && begDiff < 0 && endDiff > 0))
                    begOutlet->score -= MISMATCH_SEGMENT_PENALTY;
                }
            }
        }

    /* Reset visited flag */
    resetSegFlags(h, iBeg, 0, SEG_VISITED);
    }

/* try fixing junctions by deleting or undeleting an outlet with the least score
   Return 1 if all junctions become good (i.e., inlets and outlets are balanced) */
static int fixWorstOutlet(abfCtx h)
    {
    int     good = 1;
    long    i;
    long    iJunc, iWorstJunc = 0;
    Junction *endJunc = NULL, *worstJunc = NULL;
    float   worstScore;
    Outlet  *worstOutlet = NULL;
    long    badFlag = 0;
    long    iBegSeg, iEndSeg;
    Segment *seg;

    /* Find the worst outlet */
    worstScore = MAXFLOAT;
    for (iJunc = 0; iJunc < h->juncs.cnt; iJunc++)
        {
        Junction *junc = &h->juncs.array[iJunc];
        int sign = signCountDiff(junc);

        if (sign)
            {
            good = 0;

            for (i = 0; i < junc->outlets.cnt; i++)
                {
                Outlet *outlet = &junc->outlets.array[i];
                float  score;
                int otherSign;

                /* Don't choose an outlet with a segment sequence returning to the same junction */
                if (junc == &h->juncs.array[outlet->otherJunc])
                    continue;

                score = outlet->score;

                if (!(outlet->flags & OUTLET_DELETE))
                    {
                    if (((outlet->flags & OUTLET_OUT) != 0) != (sign > 0))
                        continue;

                    score -= EXTRA_OUTLET_PENALTY * (((sign > 0)? junc->outCount: junc->inCount) - 1);
                    }

                /* If the other junction has an opposite extra outlet, give it penalty */
                endJunc = &h->juncs.array[outlet->otherJunc];
                otherSign = signCountDiff(endJunc);
                if (otherSign && otherSign != sign)
                    score -= EXTRA_OUTLET_PENALTY;

                if (score < worstScore)
                    {
                    iWorstJunc = iJunc;
                    worstOutlet = outlet;
                    worstScore = score;
                    badFlag = (sign > 0)? OUTLET_OUT: 0;
                    }
                }
            }
        }

    if (good || !worstOutlet)
        return good;

    /* Fix a bad junction by flipping deletion flag of an outlet with the least score. */
    iBegSeg = worstOutlet->seg;
    endJunc = &h->juncs.array[worstOutlet->otherJunc];
    iEndSeg = worstOutlet->otherSeg;
    worstJunc = &h->juncs.array[iWorstJunc];

    if (worstOutlet->flags & OUTLET_DELETE)
        {
        /* Undelete a deleted outlet and the segment sequence starting from it */
        long    segFlag;
        int     forw;

        seg = &h->segs.array[iBegSeg];
        forw = (seg->outJunc == iWorstJunc) ^ ((seg->flags & SEG_REVERSE) != 0);
        segFlag = seg->flags & SEG_REVERSE;
        if ((seg->inJunc == iWorstJunc) == (badFlag == 0))
            segFlag ^= SEG_REVERSE;
        modifyOutlet(h, endJunc, iEndSeg, OUTLET_DELETE, badFlag, FIXED_SCORE_INCREMENT);
        modifyOutlet(h, worstJunc, iBegSeg, OUTLET_DELETE, badFlag ^ OUTLET_OUT, FIXED_SCORE_INCREMENT);
        modifySegSeqFlags(h, iBegSeg, iEndSeg, forw, SEG_REVERSE|SEG_DELETE, segFlag);
        }
    else
        {
        /* Delete an outlet and the segment sequence starting from it */
        int forw = ((worstOutlet->flags & OUTLET_OUT) != 0) ^ ((h->segs.array[iBegSeg].flags & SEG_REVERSE) != 0);

        modifyOutlet(h, endJunc, iEndSeg, worstOutlet->flags ^ OUTLET_OUT, OUTLET_DELETE, FIXED_SCORE_INCREMENT);
        modifyOutlet(h, worstJunc, iBegSeg, worstOutlet->flags, OUTLET_DELETE, FIXED_SCORE_INCREMENT);
        modifySegSeqFlags(h, iBegSeg, iEndSeg, forw, SEG_DELETE, SEG_DELETE);
        }

    return good;
    }

/* Each junction must have the same number of incoming and outgoing outlets.
   They may become inconsistent due to math errors during intersection and winding order tests
   with barely intersecting/overlapping segments.
   Fix such junctions heuristically if the numbers of outlets don't match. */
static void fixBadJunctions(abfCtx h, long iBeg)
    {
    int     sweep;
    int     good = 0;
    int     maxSweep = h->segs.cnt - h->paths.array[iBeg].iSeg;

    if (maxSweep > MAX_SWEEP_FIX_JUNC)
        maxSweep = MAX_SWEEP_FIX_JUNC;

    for (sweep = 0; !good && sweep < h->segs.cnt; sweep++)
        good = fixWorstOutlet(h);

    if (!good)
        {
        /* Last resort: Undelete all segments and restore the path direction. */
        resetSegFlags(h, iBeg, PATH_ISECT, SEG_ISECT|SEG_DELETE|SEG_REVERSE);
		freeOutlets(h);
		h->juncs.cnt = 0;
        }
    }

static void reverseSeg(Segment *seg)
    {
    Point   p0 = seg->p0;
    Point   p1 = seg->p1;
    Point   p2 = seg->p2;
    Point   p3 = seg->p3;
    long    next = seg->iNext;
    long    prev = seg->iPrev;

    seg->p0 = p3;
    seg->p1 = p2;
    seg->p2 = p1;
    seg->p3 = p0;
    seg->iNext = prev;
    seg->iPrev = next;
    }

static void windTestPaths(abfCtx h, long iBeg)
    {
	/* Check all segments for winding order. */
    long    iPath = iBeg;
    long    i;

	do
		{
		Path *path = &h->paths.array[iPath];
		iPath = path->iNext;
        i = path->iSeg;
        do
            {
            windTestSeg(h, i);
            i = h->segs.array[i].iNext;
            }
        while (i != path->iSeg);
		}
	while (iPath != iBeg);
    }

static void addPointToExtremaLists(abfCtx h, Point *p)
    {
    *dnaNEXT(h->xExtremaList) = p->x;
    *dnaNEXT(h->yExtremaList) = p->y;
    }

static void addExtremaToExtremaList(abfCtx h, Extrema *extrema, ValueList *extremaList)
    {
    long    i;
    for (i = 0; i < extrema->cnt; i++)
        *dnaNEXT(*extremaList) = extrema->v[i];
    }

/* Compare intersections by id. */
static int CTL_CDECL cmpVals(const void *first, const void *second)
	{
	float a = *(float *)first;
	float b = *(float *)second;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

static void sortValueList(ValueList *list)
    {
    float   lastVal = -MAXFLOAT;
    float   val;
    long    i, j;

	qsort(list->array, list->cnt, sizeof(list->array[0]), cmpVals);

    /* remove duplicates */
    for (i = j = 0; i < list->cnt; i++)
        {
        val = list->array[i];
        if (val != lastVal)
            {
            list->array[j++] = val;
            lastVal = val;
            }
        }
    list->cnt = j;
    }

/* Collect all x and y values of segment ends and extrema
   sort each list for later use in choosing a coordinate value
   for winding test with a segment */
static void collectExtrema(abfCtx h)
    {
    long    iBeg = h->glyphs.array[h->iGlyph].iPath;
    long    iPath = iBeg;
    long    iSeg;

	do
		{
		Path *path = &h->paths.array[iPath];
		iPath = path->iNext;
        iSeg = path->iSeg;
        do
            {
            Segment *seg = &h->segs.array[iSeg];

            addPointToExtremaLists(h, &seg->p0);
            addPointToExtremaLists(h, &seg->p3);
            addExtremaToExtremaList(h, &seg->xExtrema, &h->xExtremaList);
            addExtremaToExtremaList(h, &seg->yExtrema, &h->yExtremaList);

            iSeg = h->segs.array[iSeg].iNext;
            }
        while (iSeg != path->iSeg);
		}
	while (iPath != iBeg);

    /* sort both extrema list and remove duplicates */
    sortValueList(&h->xExtremaList);
    sortValueList(&h->yExtremaList);
    }

/* Delete overlapped segments
   Check each intersection for in and out non-deleted segments (their counts must match)
   Collect them in a junction list */
static void deleteBadSegs(abfCtx h, long iBeg)
	{
	long i, j;
    Intersect *firstisect, *lastisect;
    long beg, end;
    Segment *seg;
    long iJunc;

	freeOutlets(h);
    iJunc = h->juncs.cnt = 0;

	/* Sort list by id */
	qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpIds);

    windTestPaths(h, iBeg);

	/* Collect non-deleted segments intersecting at the same point in a junction list */
        {
        Junction *junc = NULL;
        firstisect = lastisect = NULL;
        for (i = 0; i < h->isects.cnt; i++)
            {
            Intersect *isect = &h->isects.array[i];

            /* Beginning of a series of intersections */
            if (firstisect == NULL)
                firstisect = isect;

            /* Order intersection path from beg to end.
               Test isect->t > 0 is to prevent confusion about a path consisting of
               only two path segments intersecting at both ends. */
            if (h->segs.array[isect->iSeg].iNext == isect->iSplit && isect->t > 0)
                {
                beg = isect->iSeg;
                end = isect->iSplit;
                }
            else
                {
                beg = isect->iSplit;
                end = isect->iSeg;
                }

            /* Connect segments to a junction. Count the number of incoming/outgoing segments (outlets). */
            junc = connectSegToJunction(h, isect, iJunc, junc, beg, 1);
            junc = connectSegToJunction(h, isect, iJunc, junc, end, 0);

            /* Check the end of a series of intersections sharing the same id */
            if (i + 1 >= h->isects.cnt || isect->id != isect[1].id)
                {
                lastisect = isect;

                /* There must be N*2 segments on N paths surrounding an intersection */
                /* Start a new series of intersections at the same point */
                if (junc)
                {
                    iJunc++;
                    junc = NULL;
                }
                firstisect = lastisect = NULL;
                }
            }
        }
    sortOutlets(h);
    polishOutlets(h, iBeg);
    fixBadJunctions(h, iBeg);

    /* relink segments starting from each junction
       Follow the original segment link possibly backwards if a segment is marked SEG_REVERSE
       At a junction, choose one of untaken segments in the opposite direction */
    for (j = 0; j < h->juncs.cnt; j++)
        {
        Junction *junc = &h->juncs.array[j];

        for (i = 0; i < junc->outlets.cnt; i++)
            {
            Outlet  *outlet1 = &junc->outlets.array[i];
            long    iSeg, iLastSeg;

            if ((outlet1->flags & OUTLET_DELETE) || !(outlet1->flags & OUTLET_OUT))
                continue;

            /* Pick the next segment to start from */
            beg = outlet1->seg;
            iLastSeg = -1;
            iSeg = beg;
            seg = &h->segs.array[iSeg];

            if (seg->flags & SEG_VISITED)
                continue;

            for (;;) {
                seg->flags |= SEG_VISITED;
                if (seg->flags & SEG_REVERSE)
                    reverseSeg(seg);
                seg->iPrev = iLastSeg;

                if (seg->inJunc == j)
                    {
                    /* reached the starting junction. link the beginning segment
                       and the ending segment closing a loop */
                    h->segs.array[beg].iPrev = iSeg;
                    seg->iNext = beg;
                    break;
                    }
                
                /* At a junction, choose an untaken outgoing segment as the next one */
                if (seg->inJunc >= 0)
                    {
                    Junction *junc1 = &h->juncs.array[seg->inJunc];
                    long    k;
                    float   angle = segmentAngle(seg, 1);
                    long    iOutSeg = -1;
                    long    iAnySeg = -1;
                    Segment *outSeg;

                    /* If there are multiple outgoing segments to choose from at the junction,
                       choose the closest one to the left so that newly built paths are
                       touching rather than intersecting at junctions */
                    for (k = 0; k < junc1->outlets.cnt; k++)
                        {
                        Outlet  *outlet = &junc1->outlets.array[k];

                        if ((outlet->flags & OUTLET_DELETE) || !(outlet->flags & OUTLET_OUT))
                            continue;
                        if (!(h->segs.array[outlet->seg].flags & SEG_VISITED))
                            {
                            iAnySeg = outlet->seg;
                            if (outlet->angle < angle)
                                iOutSeg = outlet->seg;
                            }
                        }

                    if (iOutSeg < 0)
                        iOutSeg = iAnySeg;
                    if (iOutSeg < 0)
                        fatal(h, abfErrCantHandle); /* shouldn't happen */

                    outSeg = &h->segs.array[iOutSeg];

                    /* link the segments */
                    seg->iNext = iOutSeg;
                    seg->iPrev = iLastSeg;
                    }

                iLastSeg = iSeg;
                iSeg = seg->iNext;
                seg = &h->segs.array[iSeg];
                if (seg->flags & (SEG_DELETE|SEG_VISITED))
                    fatal(h, abfErrCantHandle); /* shouldn't happen */
                }
            }
        }
	}

/* Create new path starting from iSeg. Path bounds and links must be set by
   caller. Returns pointer to new path. */
static Path *newPath(abfCtx h, long iSeg)
	{
	long segcnt;
	Path *path;
	long iPath = h->paths.cnt;
	long i = dnaNext(&h->paths, sizeof(Path));
	if (i == -1)
		fatal(h, abfErrNoMemory);

	path = &h->paths.array[i];
	path->flags = 0;
	path->iSeg = iSeg;
	segcnt = 0;
	do
		{
		Segment *seg = &h->segs.array[iSeg];
		seg->iPath = iPath;
		if (++segcnt > h->segs.cnt)
			/* Infinite loop! I've not seen this occur in practice but it seems
			   prudent to guard against it */
			fatal(h, abfErrCantHandle);	
		iSeg = seg->iNext;
		}
	while (iSeg != path->iSeg);
	return path;
	}

/* Build new path from junction segment. */
static void buildPath(abfCtx h, long iBegNewPaths, long iSeg)
	{
	Path *path;
	Segment *seg = &h->segs.array[iSeg];

	if (seg->flags & SEG_DELETE ||	/* Segment deleted */
		seg->iPath >= iBegNewPaths)	/* Segment already added to new path */
		return;

	path = newPath(h, iSeg);
	path->bounds = seg->bounds;

    /* Build a new path by walking along segments following the original link
       if a segment is marked SEG_REVERSE traverse the link backwards
       at a junction, choose one of remaining segments in the opposite direction  */

	/* Grow bounds for remaining segments */
	for (;;)
		{
		iSeg = seg->iNext;
		if (iSeg == path->iSeg)
			break;
		seg = &h->segs.array[iSeg];
		rectGrow(&path->bounds, &seg->bounds);
		}
	}

/* Returns 1 if a non-intersecting path should be preserved;
   Returns 0 if it should be deleted. */
static int isMarkingPath(abfCtx h, Path *path)
    {
    int segCount, delCount;
    long    iSeg, iStartSeg;
    Segment *seg;

    segCount = delCount = 0;
    iSeg = iStartSeg = path->iSeg;
    for (;;)
        {
        seg = &h->segs.array[iSeg];
        segCount++;
        if (seg->flags & SEG_DELETE)
            delCount++;

        iSeg = seg->iNext;
        if (iSeg == iStartSeg)
            break;
        }
    return (delCount * 2 <= segCount);
    }

/* Delete non-marking, non-intersecting paths */
static void deleteNonMarkingPaths(abfCtx h, long iBeg, long iGlyph)
    {
	long iEnd = h->paths.array[iBeg].iPrev;
    long iFirstPath = -1;
    long iPrevPath = -1;
    long i;
    Path    *path;

    for (i = iBeg; i <= iEnd; i++)
        {
        path = &h->paths.array[i];
#if ABF_DEBUG
        if (path->flags & PATH_ISECT)
            fatal(h, abfErrCantHandle);
#endif
        if (isMarkingPath(h, path))
            {
            if (iPrevPath != -1)
                {
                path->iPrev = iPrevPath;
                h->paths.array[iPrevPath].iNext = i;
                }
            iPrevPath = i;
            if (iFirstPath == -1)
                iFirstPath = i;
            }
        }
    if (iFirstPath != -1)
        {
        h->paths.array[iFirstPath].iPrev = iPrevPath;
        h->paths.array[iPrevPath].iNext = iFirstPath;
        h->glyphs.array[iGlyph].iPath = iFirstPath;
        }
    else    /* Unlikely but may happen */
		h->glyphs.array[iGlyph].iPath = -1;
    }

/* Build new path list. */
static void buildNewPaths(abfCtx h, long iPath, long iGlyph)
	{
	long i, j;
	long iBegPath = iPath;
	long iBegNewPaths = h->paths.cnt;
	long iEndNewPaths;

	/* Copy non-intersected, non-deleted paths to new list */
	do
		{
		Path *path = &h->paths.array[iPath];
		iPath = path->iNext;
		if (!(path->flags & PATH_ISECT)
            && isMarkingPath(h, path))
			{
			/* Copy bounds since newPath() may invalidate path pointer */
			Rect bounds = path->bounds;
			newPath(h, path->iSeg)->bounds = bounds;
			}
		}
	while (iPath != iBegPath);

	/* Build new paths from junctions */
	for (j = 0; j < h->juncs.cnt; j++)
		{
		Junction *junc = &h->juncs.array[j];
        for (i = 0; i < junc->outlets.cnt; i++)
            {
            Outlet  *outlet = &junc->outlets.array[i];
            if ((outlet->flags & OUTLET_DELETE) || !(outlet->flags & OUTLET_OUT))
                continue;
            buildPath(h, iBegNewPaths, outlet->seg);
            }
		}

	/* Use original path start points with new paths if possible */
	iPath = iBegPath;
	do
		{
		Path *path = &h->paths.array[iPath];
		if (path->flags & PATH_ISECT)
			{
			Segment *seg = &h->segs.array[path->iSeg];
			if (seg->iPath >= iBegNewPaths)
				h->paths.array[seg->iPath].iSeg = path->iSeg;
			}
		iPath = path->iNext;
		}
	while (iPath != iBegPath);

	/* Coalesce co-linear line segments */
	for (i = iBegNewPaths; i < h->paths.cnt; i++)
		{
		Path *path = &h->paths.array[i];
		int lastdir = 0;	/* Suppress optimizer warning */
		long iSeg = path->iSeg;
		long segcnt = 0;
		do
			{
			int thisdir = 0;
			Segment *seg = &h->segs.array[iSeg];
			if (seg->flags & SEG_LINE)
				{
				if (seg->p0.x == seg->p3.x)
					thisdir = 1;
				else if (seg->p0.y == seg->p3.y)
					thisdir = -1;
				else 
					goto nextseg;

				if (iSeg != path->iSeg && thisdir == lastdir)
					{
					/* Remove this segment from path */
					Segment *last = &h->segs.array[seg->iPrev];
					Segment *next = &h->segs.array[seg->iNext];
					last->iNext = seg->iNext;
					next->iPrev = seg->iPrev;
					last->p3 = seg->p3;
					setLineBounds(&last->bounds, &last->p0, &last->p3);
					}
				}
		nextseg:
			if (++segcnt > h->segs.cnt)
				/* Infinite loop! */
				fatal(h, abfErrCantHandle);	
			lastdir = thisdir;
			iSeg = seg->iNext;
			}
		while (iSeg != path->iSeg);
		}

	/* Link new paths */
	iEndNewPaths = h->paths.cnt - 1;
	if (iBegNewPaths > iEndNewPaths)
		{
		/* Non-marking glyph */
		h->glyphs.array[iGlyph].iPath = -1;
		return;
		}
	
	/* Make loop */
	h->paths.array[iBegNewPaths].iPrev = iEndNewPaths;
	h->paths.array[iEndNewPaths].iNext = iBegNewPaths;

	/* Link intermediate paths */
	for (i = iBegNewPaths; i < iEndNewPaths; i++)
		h->paths.array[i].iNext = i + 1;
	for (i = iEndNewPaths; i > iBegNewPaths; i--)
		h->paths.array[i].iPrev = i - 1;

	h->glyphs.array[iGlyph].iPath = iBegNewPaths; 
	}

/* Intersect glyph paths. */
static void isectGlyph(abfCtx h, long iGlyph)
	{
	long i;
	long j;
	long iEnd;
	long iBeg = h->glyphs.array[iGlyph].iPath;

	if (iBeg == -1)
		return;	/* No path */

	iEnd = h->paths.array[iBeg].iPrev;
	h->isects.cnt = 0;
    h->xExtremaList.cnt = 0;
    h->yExtremaList.cnt = 0;
    h->iGlyph = iGlyph;

	/* Check paths for self-intersection */
	for (i = iBeg; i <= iEnd; i++)
		selfIsectPath(h, &h->paths.array[i]);

	/* Check if different paths intersect each other */
	for (i = iBeg; i < iEnd; i++)
		{
		Path *h0 = &h->paths.array[i];
		for (j = i + 1; j <= iEnd; j++)
			{
			Path *h1 = &h->paths.array[j];
			if (rectOverlap(&h0->bounds, &h1->bounds))
				isectPathPair(h, h0, h1);
			}
		}

	if (h->isects.cnt != 0)
		{
		/* Glyph contained intersecting paths */
		splitIsectSegs(h);
        collectExtrema(h);
		deleteBadSegs(h, iBeg);
		buildNewPaths(h, iBeg, iGlyph);
		}
    else
        {
        collectExtrema(h);
        windTestPaths(h, iBeg);
        deleteNonMarkingPaths(h, iBeg, iGlyph);
        }
	}

/* Glyph path callbacks */
const abfGlyphCallbacks abfGlyphPathCallbacks =
	{
	NULL,
	NULL,
	NULL,
	glyphBeg,
	glyphWidth,
	glyphMove,
	glyphLine,
	glyphCurve,
	NULL,
	NULL,
	glyphGenop,
	glyphSeac,
	glyphEnd,
	NULL,
	NULL,
	NULL,
	};

/* ----------------------------- Debug Support ----------------------------- */

#if ABF_DEBUG

#include <stdio.h>

/* Debug bounds. */
static void dbbounds(Rect *r)
	{
	printf("bounds={%g,%g,%g,%g}\n", r->left, r->bottom, r->right, r->top);
	}

/* Debug segment. */
static void dbseg(abfCtx h, long iSeg)
	{
	long i = iSeg;
    long segCount = 0;
    int seenGoodSeg = 0;

	do
		{
		Segment *seg = &h->segs.array[i];
        if (segCount++ > h->segs.cnt)
            fatal(h, abfErrCantHandle);
		printf("--- segment[%ld]\n", i);
		dbbounds(&seg->bounds);
		printf("p0={%g,%g}\n", seg->p0.x, seg->p0.y);
		if (!(seg->flags & SEG_LINE))
			{
			printf("p1={%g,%g}\n", seg->p1.x, seg->p1.y);
			printf("p2={%g,%g}\n", seg->p2.x, seg->p2.y);
			}
		printf("p3={%g,%g}\n", seg->p3.x, seg->p3.y);
		printf("flags=%08lx%s\n", seg->flags, (seg->flags&SEG_DELETE)? " << DEL ": ((seg->flags&SEG_REVERSE)? " << REV ": ""));
		printf("score={%g}\n", seg->score);
		printf("iPrev=%ld\n", seg->iPrev);
		printf("iNext=%ld\n", seg->iNext);
		printf("iPath=%ld\n", seg->iPath);

        if (!seenGoodSeg && !(seg->flags & SEG_DELETE))
            {
            seenGoodSeg = 1;
            iSeg = i;
            }
        i = seg->iNext;
		}
	while (i != iSeg);
	}

/* Debug path. */
static void dbpath(abfCtx h, long iPath)
	{
	Path *path = &h->paths.array[iPath];
	printf("--- path[%ld]\n", iPath);
	dbbounds(&path->bounds);
	printf("flags=%08lx\n", path->flags);	
	printf("iSeg =%ld\n", path->iSeg);
	printf("iPrev=%ld\n", path->iPrev);
	printf("iNext=%ld\n", path->iNext);
	dbseg(h, path->iSeg);
	}

/* Debug glyph. */
static void dbglyph(abfCtx h, long iGlyph)
	{
	Glyph *glyph = &h->glyphs.array[iGlyph];
	long iPath = glyph->iPath;
	printf("--- glyph[%ld]\n", iGlyph);
	if (iPath == -1)
		printf("no path\n");
	else
		{
		long iFirst = iPath;
		do
			{
			dbpath(h, iPath);
			iPath = h->paths.array[iPath].iNext;
			}
		while (iPath != iFirst);
		}
	}

/* Debug intersects. */
static void dbisects(abfCtx h)
	{
	printf("--- isects[index]={t,p.x,p.y,iSeg,iSplit,id,flags}\n");
	if (h->isects.cnt == 0)
		printf("empty\n");
	else
		{
		long i;
		for (i = 0; i < h->isects.cnt; i++)
			{
			Intersect *isect = &h->isects.array[i];
			char delSeg = 
				(h->segs.array[isect->iSeg].flags & SEG_DELETE)? '*': ' ';
			char delSplit = (isect->iSplit == -1)? ' ':
				((h->segs.array[isect->iSplit].flags & SEG_DELETE)? '*': ' ');
			printf("[%2ld]={%-10g,%-10g,%-10g,%2ld%c,%2ld%c,%2ld,%2ld}\n", i,
				   isect->t, isect->p.x, isect->p.y, 
				   isect->iSeg, delSeg, isect->iSplit, delSplit, isect->id, isect->flags);
			}
		}
	}

/* Debug Junctions. */
static void dbjuncs(abfCtx h)
	{
	printf("--- juncs[index]={p.x,p.y,total/in/out} - {seg i/o/* => (other junc,seg), angle,score}, ...\n");
	if (h->juncs.cnt == 0)
		printf("empty\n");
	else
		{
		long j, i;
		for (j = 0; j < h->juncs.cnt; j++)
			{
			Junction *junc = &h->juncs.array[j];
			printf("[%2ld]={%-10g,%-10g,%-2ld,%-2d,%-2d} - ", j,
				   junc->p.x, junc->p.y, junc->outlets.cnt, junc->inCount, junc->outCount);
            for (i = 0; i < junc->outlets.cnt; i++)
                {
                Outlet  *outlet = &junc->outlets.array[i];
                char   type;
                if (outlet->flags & OUTLET_DELETE)
                    type = '*';
                else if (outlet->flags & OUTLET_OUT)
                    type = 'o';
                else
                    type = 'i';
                printf("%s{%ld%c => (%ld, %ld), %g, %g}", ((i == 0)? "": ", "),
                    outlet->seg, type, outlet->otherJunc, outlet->otherSeg, outlet->angle, outlet->score);
                }
            printf("\n");
			}
		}
	}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...)
	{
	dbuse(0, dbglyph, dbisects, dbjuncs);
	}

#endif /* ABF_DEBUG */
