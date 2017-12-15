/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Glyph metrics support.
 */

#include "absfont.h"

#include <math.h>

#define SEEN_MOVE	(1UL<<31)	/* Flags seen move operator */

/* Transform x and y coordinates by matrix. */
#define TX(x, y)	(h->matrix[0]*x + h->matrix[2]*y + h->matrix[4])
#define TY(x, y)	(h->matrix[1]*x + h->matrix[3]*y + h->matrix[5])

typedef struct 	/* Rectangle */
	{
	float left;	
	float bottom;
	float right;
	float top;
	} Rect;

/* Begin glyph path. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;

	cb->info = info;
	h->err_code = abfSuccess;
	
	/* Initialize bounding box */
	h->real_mtx.left	= 0;
	h->real_mtx.bottom	= 0;
	h->real_mtx.right	= 0;
	h->real_mtx.top 	= 0;

	h->flags &= ~SEEN_MOVE;

	return ABF_CONT_RET;
	}

/* Save glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;
	h->real_mtx.hAdv = (h->flags & ABF_MTX_TRANSFORM)? h->matrix[0]*hAdv: hAdv;
	}

/* Add point to current bounds. */
static void boundPoint(abfMetricsCtx h, float x, float y)
	{
	if (h->real_mtx.left > x)
		h->real_mtx.left = x;
	else if (h->real_mtx.right < x)
		h->real_mtx.right = x;
	if (h->real_mtx.bottom > y)
		h->real_mtx.bottom = y;
	else if (h->real_mtx.top < y)
		h->real_mtx.top = y;
	}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;
	float x;
	float y;

	if (h->flags & ABF_MTX_TRANSFORM)
		{
		x = TX(x0, y0);
		y = TY(x0, y0);
		}
	else
		{
		x = x0;
		y = y0;
		}

	if (h->flags & SEEN_MOVE)
		boundPoint(h, x, y);
	else
		{
		/* Set bounds to first point */
		h->real_mtx.left = h->real_mtx.right = x;
		h->real_mtx.bottom = h->real_mtx.top = y;
		}

	h->x = x0;
	h->y = y0;
	h->flags |= SEEN_MOVE;
	}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;

	if (h->flags & ABF_MTX_TRANSFORM)
		boundPoint(h, TX(x1, y1), TY(x1, y1));
	else
		boundPoint(h, x1, y1);

	h->x = x1;
	h->y = y1;
	}

/* Set bounds on line from x0,y0 to x1,y1. */
static void setLineBounds(Rect *r, float x0, float y0, float x1, float y1)
	{
	if (x0 < x1)
		{
		r->left = x0;
		r->right = x1;
		}
	else
		{
		r->left = x1;
		r->right = x0;
		}
	if (y0 < y1)
		{
		r->bottom = y0;
		r->top = y1;
		}
	else
		{
		r->bottom = y1;
		r->top = y0;
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
		if (r < 0)
			return;	/* Equation has no solutions */
		r = (float)sqrt(r);
		t[i++] = (-b + r)/a;
		t[i++] = (-b - r)/a;
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

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;
	Rect ep;	/* End-point bounds */
	Rect cp;	/* Control-point bounds */
	float x0 = h->x;
	float y0 = h->y;

	if (h->flags & ABF_MTX_TRANSFORM)
		{
		/* Transform curve */
		float xt;
		xt = x0; x0 = TX(xt, y0); y0 = TY(xt, y0);
		xt = x1; x1 = TX(xt, y1); y1 = TY(xt, y1);
		xt = x2; x2 = TX(xt, y2); y2 = TY(xt, y2);
		xt = x3; x3 = TX(xt, y3); y3 = TY(xt, y3);
		}

	setLineBounds(&ep, x0, y0, x3, y3);
	setLineBounds(&cp, x1, y1, x2, y2);

	if (ep.left   < h->real_mtx.left ||
		ep.bottom < h->real_mtx.bottom ||
		ep.right  > h->real_mtx.right ||
		ep.top    > h->real_mtx.top ||
		cp.left   < h->real_mtx.left ||
		cp.bottom < h->real_mtx.bottom ||
		cp.right  > h->real_mtx.right ||
		cp.top    > h->real_mtx.top)
		{
		/* Curve may extend bounds */
		if (cp.left < ep.left || cp.right > ep.right)
			/* Grow left and/or right bounds */
			setBezLimits(x0, x1, x2, x3, &ep.left, &ep.right);

		if (cp.bottom < ep.bottom || cp.top > ep.top)
			/* Grow top and/or bottom bounds */
			setBezLimits(y0, y1, y2, y3, &ep.bottom, &ep.top);

		boundPoint(h, ep.left, ep.bottom);
		boundPoint(h, ep.right, ep.top);
		}

	h->x = x3;
	h->y = y3;
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
	abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;

	/* Compute integer metrics */
	h->int_mtx.left		= (long)floor(h->real_mtx.left);
	h->int_mtx.bottom	= (long)floor(h->real_mtx.bottom);
	h->int_mtx.right	= (long)ceil(h->real_mtx.right);
	h->int_mtx.top		= (long)ceil(h->real_mtx.top);
	h->int_mtx.hAdv		= (long)(h->real_mtx.hAdv + 0.5);
	}

/* Glyph metrics callbacks template. */
const abfGlyphCallbacks abfGlyphMetricsCallbacks =
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

