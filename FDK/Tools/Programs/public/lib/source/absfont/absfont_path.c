/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Path support.
 */

#include "absfont.h"
#include "dynarr.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <math.h>
#include <setjmp.h>
#include <stdlib.h>

#define MIN(a,b)	((a) < (b)? (a): (b))
#define MAX(a,b)	((a) > (b)? (a): (b))
#define RND(v)		((float)floor((v) + 0.5))

/* Scale to thousandths of an em */
#define MILLIEM(v)	((float)(h->top->sup.UnitsPerEm*(v)/1000.0))

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
	} Intersect;

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

typedef struct				/* Segment */
	{
	Rect bounds;			/* Segment bounds */
	Point p0;				/* Line: p0, p3; curve: p0, p1, p2, p3 */
	Point p1;
	Point p2;
	Point p3;
	long flags;				/* Status flags */
#define SEG_DELETE		(1<<0)	/* Segment deleted (must be bit zero) */
#define SEG_LINE		(1<<1)	/* Line segment (else curve segment) */
#define SEG_ISECT		(1<<2)	/* Segment intersected */
#define SEG_WIND_TEST	(1<<3)	/* Segment winding tested */
	long iPrev;				/* Previous segment index */
	long iNext;				/* Next segment index */
	long iPath;				/* Parent path */
	} Segment;

struct abfCtx_				/* Context */
	{
	long flags;
#define SELF_ISECT	(1<<0)	/* Flags self intersection test */
	abfTopDict *top;
	dnaDCL(Glyph, glyphs);
	dnaDCL(Path, paths);
	dnaDCL(Segment, segs);
	dnaDCL(Intersect, isects);
	long iGlyph;			/* Current glyph index */
	long iPath;				/* Current path index */
	long iSeg;				/* Current segment index */
	Point p;				/* Current point */
	ctlMemoryCallbacks mem;
	dnaCtx fail;			/* Failing dna context */
	dnaCtx safe;			/* Safe dna context */
	struct					/* Error handling */
		{
		jmp_buf env;
		int code;
		} err;
	};

static void isectGlyph(abfCtx h, long iGlyph);

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(abfCtx h, int err_code)
	{
	h->err.code = err_code;
	longjmp(h->err.env, 1);
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
	abfCtx h = cb->ctx;
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
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct abfCtx_));
	if (h == NULL)
		return NULL;

	/* Copy callbacks */
	h->mem = *mem_cb;

	/* Initialize service libraries */
	h->fail = NULL;
	h->safe = NULL;
	h->fail = dna_FailInit(h);
	h->safe = dna_SafeInit(h);
	if (h->fail == NULL || h->safe == NULL)
	  	{
		dnaFree(h->fail);
		dnaFree(h->safe);
		mem_cb->manage(mem_cb, h, 0);
		return NULL;
		}

	/* Initialize */
	h->flags = 0;
	dnaINIT(h->fail, h->glyphs, 1, 250);
	dnaINIT(h->fail, h->paths, 20, 500);
	dnaINIT(h->fail, h->segs, 100, 5000);
	dnaINIT(h->safe, h->isects, 10, 20);
	h->iGlyph = -1;
	h->iPath = -1;
	h->iSeg = -1;
	h->err.code = abfSuccess;

	return h;
	}

/* Free library context. */
int abfFree(abfCtx h)
	{
	dnaFREE(h->glyphs);
	dnaFREE(h->paths);
	dnaFREE(h->segs);
	dnaFREE(h->isects);
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
	if (setjmp(h->err.env))
		return h->err.code;
	
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
			return abfErrCstrQuit;
		case ABF_FAIL_RET:
			return abfErrCstrFail;
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

	return abfSuccess;
	}

/* ---------------------------- Glyph Callbacks ---------------------------- */

/* Begin new glyph. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfCtx h = cb->direct_ctx;
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
	abfCtx h = cb->direct_ctx;

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

	return seg;
	}

/* Add line segment to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	abfCtx h = cb->direct_ctx;
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
	abfCtx h = cb->direct_ctx;
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

	if (p->x != h->p.x || p->y != h->p.y)
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
	abfCtx h = cb->direct_ctx;
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
	abfCtx h = cb->direct_ctx;
	Segment *seg = newSegment(h);
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
	abfCtx h = cb->direct_ctx;
	h->err.code = abfErrGlyphGenop;
	}

/* Glyph path seac. */
static void glyphSeac(abfGlyphCallbacks *cb, 
					  float adx, float ady, int bchar, int achar)
	{
	abfCtx h = cb->direct_ctx;
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

/* Set segment bounds. */
static void setSegBounds(Segment *seg)
	{
	if (seg->flags & SEG_LINE)
		setLineBounds(&seg->bounds, &seg->p0, &seg->p3);
	else
		setCurveBounds(&seg->bounds, &seg->p0, &seg->p1, &seg->p2, &seg->p3);
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
	abfCtx h = cb->direct_ctx;
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

/* Save intersection. */
static void saveIsect(abfCtx h, Point *p, Segment *seg, float t, long id)
	{	
	Intersect *new;
	long i;

	/* Search for insertion position */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Intersect *isect = &h->isects.array[i];
		long iPath = h->segs.array[isect->iSeg].iPath;
		if (iPath == seg->iPath)
			{
			Point u;
			Point v;
			u.x = RND(p->x);
			v.x = RND(isect->p.x);
			if (v.x == u.x)
				{
				u.y = RND(p->y);
				v.y = RND(isect->p.y);
				if (v.y == u.y)
					{
					if (h->flags & SELF_ISECT)
						break;
					return;	/* Matches previous intersection; ignore */
					}
				else if (v.y > u.y)
					break;
				}
			else if (v.x > u.x)
				break;
			}
		else if (iPath > seg->iPath)
			break;
		}

	/* Insert new record */
	new = &dnaGROW(h->isects, h->isects.cnt)[i];
	memmove(new + 1, new, (h->isects.cnt++ - i)*sizeof(Intersect));
	new->t 		= t;
	new->iSeg 	= seg - h->segs.array;
	new->iSplit = -1;
	new->p 		= *p;
	new->id 	= id;

	/* Mark path as intersected */
	h->paths.array[seg->iPath].flags |= PATH_ISECT;
	}

/* Save intersection pair. */
static void saveIsectPair(abfCtx h, 
						  Segment *seg0, float t0, Segment *seg1, float t1)
	{
	float x;
	float y;
	Point p;
	long id = h->isects.cnt;

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

	/* If intersection is very close to end of segment, force it to	end */
	x = RND(p.x);
	y = RND(p.y);
	if (0 < t0 && t0 < 1 &&
		((x == seg0->p0.x && y == seg0->p0.y) ||
		 (x == seg0->p3.x && y == seg0->p3.y)))
		t0 = RND(t0);
	if (0 < t1 && t1 < 1 &&
		((x == seg1->p0.x && y == seg1->p0.y) ||
		 (x == seg1->p3.x && y == seg1->p3.y)))
		t1 = RND(t1);

	saveIsect(h, &p, seg0, t0, id);
	saveIsect(h, &p, seg1, t1, id);
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
	if (q->depth == 6)
		/* Split 6 times; bbox < .002 em; don't split */
		return 1;
	else if (fabs(q->bounds.right - q->bounds.left) > MILLIEM(127) ||
			 fabs(q->bounds.top - q->bounds.bottom) > MILLIEM(127))
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
	a->iSeg = s - h->segs.array;
	a->bounds = s->bounds;
	}

/* Make a Line record from line segment. */
static void makeLine(abfCtx h, Segment *s, Line *l)
	{
	l->p0 = s->p0;
	l->p1 = s->p3;
	l->t0 = 0;
	l->t1 = 1;
	l->iSeg = s - h->segs.array;
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

/* Self-intersect path segments. */
static void selfIsectPath(abfCtx h, Path *path)
	{
	long i;
	long j;
	long iEnd = h->segs.array[path->iSeg].iPrev;

	for (i = path->iSeg; i < iEnd; i++)
		{
		Segment *s0 = &h->segs.array[i];
		for (j = i + 2; j <= iEnd; j++)
			{
			Segment *s1 = &h->segs.array[j];
			if (s1->iNext != i && rectOverlap(&s0->bounds, &s1->bounds))
				isectSegPair(h, s0, s1);
			}
		}
	}

/* Compare intersections by segment. */
static int CTL_CDECL cmpSegs(const void *first, const void *second)
	{
	Intersect *a = (Intersect *)first;
	Intersect *b = (Intersect *)second;
	if (a->iSeg < b->iSeg)
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
	Segment *new;
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
	new = &h->segs.array[iNew];
	seg = &h->segs.array[iSeg];

	if (seg->flags & SEG_LINE)
		{
		/* Split line segment */
		new->p0 = isect->p;
		new->p3 = seg->p3;
		seg->p3 = isect->p;

		new->flags = SEG_LINE;
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
		new->p3 = p3;
		new->p2.x = t*(p2.x - p3.x) + new->p3.x;
		new->p2.y = t*(p2.y - p3.y) + new->p3.y;
		new->p1.x = t*t*(p1.x - 2*p2.x + p3.x) + 2*new->p2.x - new->p3.x;
		new->p1.y = t*t*(p1.y - 2*p2.y + p3.y) + 2*new->p2.y - new->p3.y;
		new->p0 = seg->p3;
		new->flags = 0;
		if (curveIsLine(h, new))
			new->flags |= SEG_LINE;	/* Make line segment */
		}

	/* Link new segment */
	new->iPrev = iSeg;
	new->iNext = seg->iNext;
	new->iPath = seg->iPath;

	seg->iNext = iNew;
	h->segs.array[new->iNext].iPrev = iNew;

	isect->iSplit = iNew;
	}

/* Round points in segment to nearest integer. */
static void roundSegment(abfCtx h, long iSeg)
	{
	Segment *seg = &h->segs.array[iSeg];
	seg->p0.x = RND(seg->p0.x);
	seg->p0.y = RND(seg->p0.y);
	seg->p3.x = RND(seg->p3.x);
	seg->p3.y = RND(seg->p3.y);
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

/* Split intersecting segments. */
static void splitIsectSegs(abfCtx h)
	{
	long i;
	Intersect *last;
	Intersect *isect;

	/* Sort list by segment */
	qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpSegs);

	/* Split segments */
	last = NULL;
	for (i = 0; i < h->isects.cnt; i++)
		{
		isect = &h->isects.array[i];
		splitSegment(h, last, isect);
		last = isect;
		}

	/* Update connection of multi-split segments */
	for (i = h->isects.cnt - 1; i > 0; i--)
		{
		last = &h->isects.array[i - 1];
		isect = &h->isects.array[i];
		if (isect->iSplit != -1 && 
			last != NULL && last->iSeg == isect->iSeg && last->iSplit != -1)
			{
			isect->iSeg = last->iSplit;
			isect->iSplit = h->segs.array[isect->iSeg].iNext;
			}
		}

	/* Round split segments and update unsplit segment connections */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Segment *seg;
		isect = &h->isects.array[i];
		seg = &h->segs.array[isect->iSeg];

		if (isect->iSplit == -1)
			isect->iSplit = (isect->t == 0)? seg->iPrev: seg->iNext;
		else
			{
			roundSegment(h, isect->iSeg);
			roundSegment(h, isect->iSplit);
			}

		/* Mark segments as intersected */
		seg->flags |= SEG_ISECT;
		h->segs.array[isect->iSplit].flags |= SEG_ISECT;
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

	t = 1 - t;
	b->p3 = p3;
	b->p2.x = t*(p2.x - p3.x) + b->p3.x;
	b->p2.y = t*(p2.y - p3.y) + b->p3.y;
	b->p1.x = t*t*(p1.x - 2*p2.x + p3.x) + 2*b->p2.x - b->p3.x;
	b->p1.y = t*t*(p1.y - 2*p2.y + p3.y) + 2*b->p2.y - b->p3.y;
	b->p0 = a->p3;
	}

/* Find extrema on bezier curve. Return count (max of 2) of solutions passed
   back via the "solns" parameter. */
static int findBezExtrema(float p0, float p1, float p2, float p3, 
						  float solns[2])
	{
	float t[2];
	int i = 0;
	int j = 0;
	float a = p3 - 3*(p2 - p1) - p0;
	float b = p2 - 2*p1 + p0;
	float c = p1 - p0;

	/* Solve dy/dx=0 */
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

	/* Select solutions in range 0 < t and t < 1 */
	while (i--)
		if (0 < t[i] && t[i] < 1)
			solns[j++] = t[i];

	/* Sort solutions */
	if (j == 2 && solns[0] > solns[1])
		{
		solns[0] = t[1];
		solns[1] = t[0];
		}
		
	return j;
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
	float t[2];
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

	/* Find extrema */
	cnt = findBezExtrema(q[0].p3.y, q[0].p2.y, q[0].p1.y, q[0].p0.y, t);

	/* Split curve(s) */
	if (cnt > 0)
		splitBez(&q[0], &q[1], t[0]);
	if (cnt > 1)
		splitBez(&q[1], &q[2], (t[1] - t[0])/(1 - t[0]));
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
	float t[2];
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

	/* Find extrema */
	cnt = findBezExtrema(q[0].p3.x, q[0].p2.x, q[0].p1.x, q[0].p0.x, t);

	/* Split curve(s) */
	if (cnt > 0)
		splitBez(&q[0], &q[1], t[0]);
	if (cnt > 1)
		splitBez(&q[1], &q[2], (t[1] - t[0])/(1 - t[0]));
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

/* Test segment winding and if non-zero delete it. */
static void windTestSeg(abfCtx h, long iTarget)
	{
	Point p;
	float icepts[3];
	long iPath;
	long iBegPath;
	int i;
	int wind[3];
	int testvert;
	int windtotal;
	int windtarget;
	Segment *target = &h->segs.array[iTarget];

	if (target->flags & (SEG_WIND_TEST|SEG_DELETE))
		return;	/* Ignore checked or deleted segments */

	/* Choose target point in middle of this segment */
	if (target->flags & SEG_LINE)
		{
		p.x = (target->p0.x + target->p3.x)/2;
		p.y = (target->p0.y + target->p3.y)/2;
		}
	else
		{
		p.x = (target->p3.x + 3*(target->p2.x + target->p1.x) +target->p0.x)/8;
		p.y = (target->p3.y + 3*(target->p2.y + target->p1.y) +target->p0.y)/8;
		}

	windtotal = 0;
	testvert = (fabs(target->p3.x - target->p0.x) > 
				fabs(target->p3.y - target->p0.y));
	if (testvert)
		windtarget = getWind(target->p0.x, target->p3.x);
	else
		windtarget = getWind(target->p0.y, target->p3.y);

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
			if (path->bounds.top < p.y ||
				path->bounds.left > p.x ||
				path->bounds.right <= p.x)
				goto nextpath;
			}
		else
			{
			if (path->bounds.left > p.x ||
				path->bounds.bottom > p.y ||
				path->bounds.top <= p.y)
				goto nextpath;
			}
		iBegSeg = path->iSeg; 
		iSeg = iBegSeg;
		do
			{
			Segment *seg = &h->segs.array[iSeg];
			if (iSeg == iTarget)
				;
			else if (testvert)
				{
				/* Consider only non-horizontal segments that are to the left
				   of the target point, and cross, or touch from above, the
				   horizontal line through the target point. */
				if (seg->bounds.top < p.y ||
					seg->bounds.left > p.x ||
					seg->bounds.right <= p.x ||
					seg->bounds.left == seg->bounds.right)
					;
				else if (seg->bounds.bottom > p.y)
					windtotal += getWindAtValue(seg->p0.x, seg->p3.x, p.x);
				else if ((seg->p0.x == target->p3.x &&
						  seg->p0.y == target->p3.y &&
						  seg->p3.x == target->p0.x &&
						  seg->p3.y == target->p0.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p2.x &&
						   seg->p1.y == target->p2.y &&
						   seg->p2.x == target->p1.x &&
						   seg->p2.y == target->p1.y)))
					{
					/* Coincident in opposite direction */
					target->flags |= (SEG_DELETE|SEG_WIND_TEST);
					return;
					}
				else if ((seg->p0.x == target->p0.x &&
						  seg->p0.y == target->p0.y &&
						  seg->p3.x == target->p3.x &&
						  seg->p3.y == target->p3.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p1.x &&
						   seg->p1.y == target->p1.y &&
						   seg->p2.x == target->p2.x &&
						   seg->p2.y == target->p2.y)))
					{
					/* Coincident in same direction */
					if (seg->iPath < target->iPath)
						target->flags |= SEG_DELETE;
					target->flags |= SEG_WIND_TEST;
					return;
					}
				else
					{
					int cnt = solveSegAtX(seg, p.x, icepts, wind);
					for (i = 0; i < cnt; i++)
						if (icepts[i] > p.y)
							windtotal += wind[i];
					}
				}
			else
				{
				/* Consider only non-horizontal segments that are to the left
				   of the target point, and cross, or touch from above, the
				   horizontal line through the target point. */
				if (seg->bounds.left > p.x ||
					seg->bounds.bottom > p.y ||
					seg->bounds.top <= p.y ||
					seg->bounds.top == seg->bounds.bottom)
					;
				else if (seg->bounds.right < p.x)
					windtotal += getWindAtValue(seg->p0.y, seg->p3.y, p.y);
				else if ((seg->p0.x == target->p3.x &&
						  seg->p0.y == target->p3.y &&
						  seg->p3.x == target->p0.x &&
						  seg->p3.y == target->p0.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p2.x &&
						   seg->p1.y == target->p2.y &&
						   seg->p2.x == target->p1.x &&
						   seg->p2.y == target->p1.y)))
					{
					/* Coincident in opposite direction */
					target->flags |= (SEG_DELETE|SEG_WIND_TEST);
					return;
					}
				else if ((seg->p0.x == target->p0.x &&
						  seg->p0.y == target->p0.y &&
						  seg->p3.x == target->p3.x &&
						  seg->p3.y == target->p3.y) &&
						 (seg->flags & SEG_LINE ||
						  (seg->p1.x == target->p1.x &&
						   seg->p1.y == target->p1.y &&
						   seg->p2.x == target->p2.x &&
						   seg->p2.y == target->p2.y)))
					{
					/* Coincident in same direction */
					if (seg->iPath < target->iPath)
						target->flags |= SEG_DELETE;
					target->flags |= SEG_WIND_TEST;
					return;
					}
				else
					{
					int cnt = solveSegAtY(seg, p.y, icepts, wind);
					for (i = 0; i < cnt; i++)
						if (icepts[i] < p.x)
							windtotal += wind[i];
					}
				}
			iSeg = seg->iNext;
			}
		while (iSeg != iBegSeg);
	nextpath:
		iPath = path->iNext;
		}
	while (iPath != iBegPath);

	if (windtotal && windtotal + windtarget)
		target->flags |= SEG_DELETE;

	target->flags |= SEG_WIND_TEST;
	}

/* Link segments. */
static void link(abfCtx h, long iBeg, long iEnd)
	{
	h->segs.array[iBeg].iNext = iEnd;
	h->segs.array[iEnd].iPrev = iBeg;
	}

/* Delete overlapped segments or those with bad winding. */
static void deleteBadSegs(abfCtx h)
	{
	long i;

	if (h->isects.cnt & 1)
		fatal(h, abfErrCantHandle);

	/* Sort list by id */
	qsort(h->isects.array, h->isects.cnt, sizeof(Intersect), cmpIds);

	/* Delete badly wound segments from each intersection */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Intersect *isect = &h->isects.array[i];
		windTestSeg(h, isect->iSeg);
		windTestSeg(h, isect->iSplit);
		}
	
	/* Link non-deleted segments from each intersection */
	for (i = 0; i < h->isects.cnt; i += 2)
		{
		long abeg;
		long aend;
		long bbeg;
		long bend;
		Intersect *a = &h->isects.array[i + 0];
		Intersect *b = &h->isects.array[i + 1];

		/* Order intersection path "a" from beg to end */
		if (h->segs.array[a->iSeg].iNext == a->iSplit)
			{
			abeg = a->iSeg;
			aend = a->iSplit;
			}
		else
			{
			abeg = a->iSplit;
			aend = a->iSeg;
			}

		/* Order intersection path "b" from beg to end */
		if (h->segs.array[b->iSeg].iNext == b->iSplit)
			{
			bbeg = b->iSeg;
			bend = b->iSplit;
			}
		else
			{
			bbeg = b->iSplit;
			bend = b->iSeg;
			}
		
		/* Link overlapping paths. There are 4 segments on 2 paths surrounding
		   an intersection. Some, or all, of these segments may have been
		   deleted because of bad winding. This switch determines how to link
		   the remaining undeleted segments. Apart from cases 0, 3, 12, and 15
		   which require no action the only other senisible cases are 6 and 9.
		   However, the remaining cases may be encountered if the winding test
		   fails due the numeric instability, which although rare, must be
		   handled. Fortunately, it is possible to determine where the failure
		   occured and fix it, yeilding a correct result. */
		switch ((h->segs.array[abeg].flags & SEG_DELETE)<<3 |
				(h->segs.array[aend].flags & SEG_DELETE)<<2 |
				(h->segs.array[bbeg].flags & SEG_DELETE)<<1 |
				(h->segs.array[bend].flags & SEG_DELETE)<<0)
			{
		case 0:
		case 3:
		case 12:
		case 15:
			/* Various cases requiring no action */
			break;
		case 5:
		case 10:
			/* These cases shouldn't happen; give up */
			fatal(h, abfErrCantHandle);
		case 1:
			h->segs.array[abeg].flags |= SEG_DELETE;
			link(h, bbeg, aend);
			break;
		case 2:
			h->segs.array[aend].flags |= SEG_DELETE;
			link(h, abeg, bend);
			break;
		case 4:
			h->segs.array[bbeg].flags |= SEG_DELETE;
			link(h, abeg, bend);
			break;
		case 6:
			link(h, abeg, bend);
			break;
		case 7:
			h->segs.array[bend].flags &= ~SEG_DELETE;
			link(h, abeg, bend);
			break;
		case 8:
			h->segs.array[bend].flags |= SEG_DELETE;
			link(h, bbeg, aend);
			break;
		case 9:
			link(h, bbeg, aend);
			break;
		case 11:
			h->segs.array[bbeg].flags &= ~SEG_DELETE;
			link(h, bbeg, aend);
			break;
		case 13:
			h->segs.array[aend].flags &= ~SEG_DELETE;
			link(h, bbeg, aend);
			break;
		case 14:
			h->segs.array[abeg].flags &= ~SEG_DELETE;
			link(h, abeg, bend);
			break;
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

/* Build new path from intersection segment. */
static void buildPath(abfCtx h, long iBegNewPaths, long iSeg)
	{
	Path *path;
	Segment *seg = &h->segs.array[iSeg];

	if (seg->flags & SEG_DELETE ||	/* Segment deleted */
		seg->iPath >= iBegNewPaths)	/* Segment already added to new path */
		return;
	
	path = newPath(h, iSeg);
	path->bounds = seg->bounds;

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

/* Build new path list. */
static void buildNewPaths(abfCtx h, long iPath, long iGlyph)
	{
	long i;
	long iBegPath = iPath;
	long iBegNewPaths = h->paths.cnt;
	long iEndNewPaths;

	/* Copy non-intersected paths to new list */
	do
		{
		Path *path = &h->paths.array[iPath];
		iPath = path->iNext;
		if (!(path->flags & PATH_ISECT))
			{
			/* Copy bounds since newPath() may invalidate path pointer */
			Rect bounds = path->bounds;
			newPath(h, path->iSeg)->bounds = bounds;
			}
		}
	while (iPath != iBegPath);

	/* Build new paths from intersection data */
	for (i = 0; i < h->isects.cnt; i++)
		{
		Intersect *isect = &h->isects.array[i];
		buildPath(h, iBegNewPaths, isect->iSeg);
		buildPath(h, iBegNewPaths, isect->iSplit);
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

	/* Check paths for self-intersection */
	h->flags |= SELF_ISECT;
	for (i = iBeg; i < iEnd; i++)
		selfIsectPath(h, &h->paths.array[i]);

	/* Check if different paths intersect each other */
	h->flags &= ~SELF_ISECT;
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
		deleteBadSegs(h);
		buildNewPaths(h, iBeg, iGlyph);
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
	do
		{
		Segment *seg = &h->segs.array[i];
		printf("--- segment[%ld]\n", i);
		dbbounds(&seg->bounds);
		printf("p0={%g,%g}\n", seg->p0.x, seg->p0.y);
		if (!(seg->flags & SEG_LINE))
			{
			printf("p1={%g,%g}\n", seg->p1.x, seg->p1.y);
			printf("p2={%g,%g}\n", seg->p2.x, seg->p2.y);
			}
		printf("p3={%g,%g}\n", seg->p3.x, seg->p3.y);
		printf("flags=%08lx\n", seg->flags);
		printf("iPrev=%ld\n", seg->iPrev);
		printf("iNext=%ld\n", seg->iNext);
		printf("iPath=%ld\n", seg->iPath);
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
	printf("--- isects[index]={t,p.x,p.y,iSeg,iSplit,id}\n");
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
			printf("[%2ld]={%-10g,%-10g,%-10g,%2ld%c,%2ld%c,%2ld}\n", i,
				   isect->t, isect->p.x, isect->p.y, 
				   isect->iSeg, delSeg, isect->iSplit, delSplit, isect->id);
			}
		}
	}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...)
	{
	dbuse(0, dbglyph, dbisects);
	}

#endif /* ABF_DEBUG */
