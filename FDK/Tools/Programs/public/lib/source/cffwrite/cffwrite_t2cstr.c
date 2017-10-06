/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Charstring support.
 */

#include "cffwrite_t2cstr.h"
#include "cffwrite_subr.h"
#include "txops.h"
#include "ctutil.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <stdlib.h>
#include <math.h>

/* Make up operators for internal use */
#define tx_noop     tx_reserved0
#define t2_cntroff  t2_reservedESC33

/* Push value on stack */
#define PUSH(v) (h->stack.array[h->stack.cnt++] = (float)(v))
#define PUSH_DELTA(v) (h->deltaStack.array[h->deltaStack.cnt++] = (float)(v))

#define TC_MAX_WARNINGS 5

/* ----------------------------- Module Context ---------------------------- */

typedef struct                  /* Stem hint */
{
    union
    {
        float edge0;
        abfBlendArg edge0v;
    };
    union {
        float edge1;
        abfBlendArg edge1v;
    };
	unsigned char id;           /* Unique identifier */
	char flags;
#define STEM_VERT   (1 << 0)      /* Flags vertical stem */
#define STEM_CNTR   (1 << 1)      /* Flags counter stem */
} Stem;

typedef struct                  /* Hint record */
{
	long iCstr;                 /* Hint/cntrmask operator index */
	long iMask;                 /* Index of first mask byte */
	long size;                  /* Size of mask */
} Hint;

typedef struct                  /* Cntr record */
{
	long iMask;                 /* Index of first mask byte */
	long size;                  /* Size of mask */
} Cntr;

enum                            /* Warning types */
{
	warn_move0,
	warn_move1,
	warn_move2,
	warn_hint0,
	warn_hint1,
	warn_hint2,
	warn_hint3,
	warn_hint4,
	warn_hint5,
	warn_hint6,
	warn_hint7,
	warn_flex0,
	warn_flex1,
	warn_dup0,
	warn_dup1,
	warn_cnt
};

/* Hintmask */
#define MAX_MASK_BYTES  ((T2_MAX_STEMS + 7) / 8)
typedef char HintMask[MAX_MASK_BYTES];
#define SET_HINT_BIT(a, i)   ((a)[(i) / 8] |= 1 << (7 - (i) % 8))
#define CLEAR_MASK(a)       memset(a, 0, MAX_MASK_BYTES)

/* Stem id mask */
#define SET_ID_BIT(a, i)     ((a)[(i) / 8] |= 1 << ((i) % 8))
#define TEST_ID_BIT(a, i)    ((a)[(i) / 8] & 1 << ((i) % 8))

struct cstrCtx_ {
	long flags;                 /* Control flags */
#define SEEN_MOVETO     (1 << 0)  /* Seen moveto operator */
#define SEEN_HINT       (1 << 1)  /* Seen hint subs */
#define SEEN_CNTR       (1 << 2)  /* Seen counter */
#define SEEN_WARN       (1 << 3)  /* Seen cstr warning */
    int pendop;                 /* Pending op */
	int seqop;                  /* Pending sequence op */
    struct                      /* Operand stack */
    {
        int cnt;
        float array[CFF2_MAX_OP_STACK];
    }
    stack;
    struct                      /* Operand stack */
    {
        int cnt;
        float array[CFF2_MAX_OP_STACK];
    }
    deltaStack;
    int numBlends;
    unsigned short maxstack;
    float x;                    /* Current x-coordinate */
    float y;                    /* Current y-coordinate */
    float start_x;             /*  x-coordinate of current path initial moveto */
    float start_y;              /*  y-coordinate  of current path initial moveto */
	dnaDCL(char, cstr);         /* Charstring accumulator */
	struct                      /* Hint stems */
	{
		int cnt;
		Stem array[T2_MAX_STEMS];
	}
	stems;
	long iCstr;                 /* Current hintmask charstring index */
	dnaDCL(char, masks);        /* Idmasks */
	dnaDCL(Hint, hints);        /* Hint records */
	dnaDCL(Cntr, cntrs);        /* Counter records */
	HintMask lastmask;          /* Last hintmask */
	HintMask initmask;          /* Initial idmask */
	HintMask hintmask;          /* Hint idmask */
	HintMask cntrmask;          /* Counter idmask */
	int masksize;               /* Final hint/cntr mask size (bytes) */
	struct                      /* Glyph data */
	{
		abfGlyphInfo *info;
		float hAdv;
	}
	glyph;
	long tmpoff;                /* Temporary file offset */
	unsigned long unique;       /* Unique subroutinizer separator value */
	unsigned short warning[warn_cnt]; /* Warning accumulator */
	cfwCtx g;                   /* Package context */
};


static void flushBlends(cstrCtx h);
static void tmp_savefixed(cfwCtx g, Fixed f);
static Fixed float2Fixed(float r);
static void tmp_saveop(cfwCtx g, int op);

/* Initialize module. */
void cfwCstrNew(cfwCtx g) {
	cstrCtx h = (cstrCtx)cfwMemNew(g, sizeof(struct cstrCtx_));

	memset(h, 0, sizeof(*h));

    /* Review with Ariza */
	DURING_EX(g->err.env)
		/* Link contexts */
		h->g = g;
		g->ctx.cstr = h;
		if (g->flags & CFW_WRITE_CFF2)
			h->maxstack = CFF2_MAX_OP_STACK;
		else
			h->maxstack = T2_MAX_OP_STACK;

		dnaINIT(g->ctx.dnaFail, h->cstr, 500, 5000);
		dnaINIT(g->ctx.dnaFail, h->masks, 30, 60);
		dnaINIT(g->ctx.dnaFail, h->hints, 10, 40);
		dnaINIT(g->ctx.dnaFail, h->cntrs, 1, 10);

		/* Open tmp stream */
		g->stm.tmp = g->cb.stm.open(&g->cb.stm, CFW_TMP_STREAM_ID, 0);
		if (g->stm.tmp == NULL) {
			cfwFatal(g, cfwErrTmpStream, NULL);
		}
		h->tmpoff = 0;

		/* Initialize warning accumulator */
		memset(h->warning, 0, sizeof(h->warning));

		/* Used as charstring separator for subroutinizer */
		h->unique = 0;
	HANDLER
		cfwMemFree(g, h);
	RERAISE;
	END_HANDLER
}

/* Prepare module for reuse. */
void cfwCstrReuse(cfwCtx g) {
	cstrCtx h = g->ctx.cstr;
	if (g->cb.stm.seek(&g->cb.stm, g->stm.tmp, 0)) {
		cfwFatal(g, cfwErrTmpStream, NULL);
	}
	h->tmpoff = 0;
}

/* Free resources. */
void cfwCstrFree(cfwCtx g) {
	cstrCtx h = g->ctx.cstr;

	if (h == NULL) {
		return;
	}
	dnaFREE(h->cstr);
	dnaFREE(h->masks);
	dnaFREE(h->hints);
	dnaFREE(h->cntrs);

	/* Close tmp stream */
	if (g->cb.stm.close(&g->cb.stm, g->stm.tmp)) {
		cfwFatal(g, cfwErrTmpStream, NULL);
	}

	cfwMemFree(g, g->ctx.cstr);
	g->ctx.cstr = NULL;
}

/* Print charstring warning. */
static void addWarn(cstrCtx h, int type) {
	h->warning[type]++;
	h->flags |= SEEN_WARN;
}

/* Begin new glyph definition. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

	cb->info = info;
	if (g->err.code != 0) {
		if (g->flags & CFW_CHECK_IF_GLYPHS_DIFFER) {
			if (g->err.code & cfwErrGlyphPresent) {
				g->err.code = g->err.code & ~cfwErrGlyphPresent;
			}

			if (g->err.code & cfwErrGlyphDiffers) {
				g->err.code = g->err.code & ~cfwErrGlyphDiffers;
			}

			if (g->err.code != 0) {
				return ABF_FAIL_RET;    /* Error on last glyph */
			}
		}
		else {
			return ABF_FAIL_RET;    /* Error on last glyph */
		}
	}
	else if ((info->flags & ABF_GLYPH_SEEN) && (!(g->flags & CFW_CHECK_IF_GLYPHS_DIFFER))) {
		return ABF_SKIP_RET;    /* Ignore duplicate glyph */
	}
	else if (!(info->flags & ABF_GLYPH_CID) &&
	         (info->gname.ptr == NULL || info->gname.ptr[0] == '\0')) {
		return ABF_FAIL_RET;    /* No glyph name in name-keyed font */
	}
	/* Initialize */
	h->flags = 0;
	h->pendop = tx_noop;
	h->seqop = tx_noop;
	h->x = 0;
	h->y = 0;
    h->stack.cnt = 0;
    h->deltaStack.cnt = 0;
    h->numBlends = 0;
	h->cstr.cnt = 0;
	h->stems.cnt = 0;
	h->iCstr = -1;
	h->masks.cnt = 0;
	h->hints.cnt = 0;
	h->cntrs.cnt = 0;
	CLEAR_MASK(h->initmask);
	CLEAR_MASK(h->hintmask);
	CLEAR_MASK(h->cntrmask);
	h->glyph.info = info;

	return ABF_CONT_RET;
}

/* Add horizontal advance width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	h->glyph.hAdv = roundf(hAdv);
}

/* Save number in charstring. */
static void cstr_savenum(cstrCtx h, float r) {
	long i;
	unsigned char *t;
	if (h->cstr.cnt + 4 >= h->cstr.size) {
		/* Grow buffer to accomodate maximum number length */
		if (dnaGrow(&h->cstr, 1, h->cstr.cnt + 4)) {
			h->g->err.code = cfwErrNoMemory;
			return;
		}
	}
	t = (unsigned char *)&h->cstr.array[h->cstr.cnt];
	i = (long)r;
	h->cstr.cnt += ((i == r) ? cfwEncInt(i, t) : cfwEncReal(r, t));
}

/* Save op code in charstring. */
static void cstr_saveop(cstrCtx h, int op) {
	if (h->cstr.cnt + 1 >= h->cstr.size) {
		/* Grow buffer to accomodate maximum op size */
		if (dnaGrow(&h->cstr, 1, h->cstr.cnt + 1)) {
			h->g->err.code = cfwErrNoMemory;
			return;
		}
	}
	if (op & 0xff00) {
		h->cstr.array[h->cstr.cnt++] = tx_escape;
	}
	h->cstr.array[h->cstr.cnt++] = (unsigned char)op;
}

/* Save operator and args to charstring. Return 1 on error else 0. */
static void saveop(cstrCtx h, int op) {
    
    if (h->numBlends > 0)
        flushBlends(h);
    
	else if (h->stack.cnt != 0) {
		/* Save args */
		int i;
		for (i = 0; i < h->stack.cnt; i++) {
			cstr_savenum(h, h->stack.array[i]);
		}
		h->stack.cnt = 0;
	}

	/* Save op */
	switch (op) {
        case t2_blend:
            cfwMessage(h->g, "CFF2 error - unexpected blend op  <%s>%s",
                       h->glyph.info->gname.ptr, 0);

             break;
        case tx_hlineto:
		case tx_vlineto:
		case tx_hvcurveto:
		case tx_vhcurveto:
			cstr_saveop(h, h->seqop);
            h->pendop = tx_noop;    /* Clear pending op */
			break;

		default:
			cstr_saveop(h, op);
            h->pendop = tx_noop;    /* Clear pending op */
			break;
	}

}

static void saveopDirect(cstrCtx h, int op) {
    if (h->numBlends > 0)
        flushBlends(h);
    
    else if (h->stack.cnt != 0) {
		/* Save args */
		int i;
		for (i = 0; i < h->stack.cnt; i++) {
			cstr_savenum(h, h->stack.array[i]);
		}
		h->stack.cnt = 0;
	}

    cstr_saveop(h, op);

	h->pendop = tx_noop;    /* Clear pending op */
}

/* Check stack headroom and flush pending op if next op causes overflow.
   When subroutinizer is enabled, reserve one extra space for a subr number.
   Return 1 on error else 0. */
static void flushop(cstrCtx h, int argcnt) {
 	if (h->stack.cnt + argcnt + ((h->g->flags & CFW_SUBRIZE) ? 1 : 0) > h->maxstack) {
		saveop(h, h->pendop);
	}
}

/* Discard pending operator. */
static void clearop(cstrCtx h) {
    h->stack.cnt = 0;
	h->pendop = tx_noop;
}


/* Discard pending moveto operator. */
static void clearmoveto(cstrCtx h, float dx, float dy) {
	/* Restore current point */
	h->x -= dx;
	h->y -= dy;
    h->x = (float)RND_ON_WRITE(h->x);
    h->y = (float)RND_ON_WRITE(h->y);

	addWarn(h, warn_move2);
	clearop(h);
}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

    float dx0;
    float dy0;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);

    x0 = (float)RND_ON_WRITE(x0);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y0 = (float)RND_ON_WRITE(y0);
	dx0 = x0 - h->x;
	dy0 = y0 - h->y;

	/* Check pending op */
	switch (h->pendop) {
		case tx_vmoveto:
			clearmoveto(h, 0, h->stack.array[0]);
			break;

		case tx_hmoveto:
			clearmoveto(h, h->stack.array[0], 0);
			break;

		case tx_rmoveto:
			clearmoveto(h, h->stack.array[0], h->stack.array[1]);
			break;

		case tx_dotsection:
			/* Discard dotsection before closepath */
			clearop(h);
			break;

		case tx_noop:
			break;

		default:
			saveop(h, h->pendop);
			break;
	}

    if ((!doOptimize) && (h->flags & SEEN_MOVETO)) /* add closing line-to, if last op is not the same as the starting moveto. */
    {
      if ((h->x != h->start_x) || (h->y != h->start_y))
      {
          dx0 = h->start_x - h->x;
          dy0 = h->start_y - h->y;
          PUSH(dx0);
          PUSH(dy0);
          h->pendop = tx_rlineto;
          saveop(h, h->pendop);
          h->x = h->start_x;
          h->y = h->start_y;
      }
    }
    
	/* Compute delta and update current point */
	dx0 = x0 - h->x;
	dy0 = y0 - h->y;
	h->x = h->start_x = x0;
	h->y = h->start_y = y0;

	/* Choose format */
	if ((dx0 == 0.0) && doOptimize) {
		/* - dy0 vmoveto */
		/*  0  */ PUSH(dy0);
		h->pendop = tx_vmoveto;
	}
	else if ((dy0 == 0.0) && doOptimize) {
		/* dx0 - hmoveto */
		PUSH(dx0); /*  0  */
		h->pendop = tx_hmoveto;
	}
	else {
		/* dx0 dy0 rmoveto */
		PUSH(dx0);
		PUSH(dy0);
		h->pendop = tx_rmoveto;
	}
	h->flags |= SEEN_MOVETO;
}

static void pushBlendDeltas(cstrCtx h,abfBlendArg* blendArg)
{
    int j;
    for (j = 0; j < h->glyph.info->blendInfo.numRegions; j++)
    {
        float val = blendArg->blendValues[j];
        PUSH_DELTA(val);
    }
    
}

static void flushBlends(cstrCtx h)
{
    int i;
    if ((h->deltaStack.cnt + h->stack.cnt) > 513)
    {
        cfwCtx g = h->g;
        cfwFatal(g, cfwErrStackOverflow, "Blend overflow");
    }
    
    for (i = 0; i < h->deltaStack.cnt; i++)
        PUSH(h->deltaStack.array[i]);
    PUSH(h->numBlends);
    h->deltaStack.cnt = 0;
    h->numBlends = 0;
   
    if (h->stack.cnt != 0) {
        /* Save args */
        int i;
        for (i = 0; i < h->stack.cnt; i++) {
            cstr_savenum(h, h->stack.array[i]);
        }
        h->stack.cnt = 0;
    }
    cstr_saveop(h, t2_blend);
}

static void flushStemBlends(cstrCtx h)
{
    cfwCtx g = h->g;
    int i;
    if ((h->deltaStack.cnt + h->stack.cnt) > 513)
    {
        cfwCtx g = h->g;
        cfwFatal(g, cfwErrStackOverflow, "Blend overflow");
    }
    
    for (i = 0; i < h->deltaStack.cnt; i++)
        tmp_savefixed(g, float2Fixed(h->deltaStack.array[i]));

    tmp_savefixed(g, float2Fixed(h->numBlends));
    h->deltaStack.cnt = 0;
    h->numBlends = 0;
    
    tmp_saveop(g, t2_blend);
    
}

static void pushBlend(cstrCtx h, abfBlendArg* arg1)
{
    if ((arg1 == NULL) || (!arg1->hasBlend))
    {
        if (h->numBlends > 0)
            flushBlends(h);
    }
    else
    {
        if ( ((h->numBlends+1) * h->glyph.info->blendInfo.numRegions + h->stack.cnt) > 513)
        {
            flushBlends(h);
        }
        pushBlendDeltas(h,arg1);
        h->numBlends++;
    }
    return;
}

static void pushStemBlends(cstrCtx h, abfBlendArg* arg1)
{
    if ((arg1 == NULL) || (!arg1->hasBlend))
    {
        if (h->numBlends > 0)
            flushStemBlends(h);
    }
    else
    {
        if ( ((h->numBlends+1) * h->glyph.info->blendInfo.numRegions + h->stack.cnt) > 513)
        {
            flushStemBlends(h);
        }
        pushBlendDeltas(h,arg1);
        h->numBlends++;
    }
    return;
}


static int blendIsZero(cstrCtx h, float dv, abfBlendArg* blendArg)
{
    /* Validate that all the blend args are zero. */
    int i;
    if (dv != 0)
        return 0;
    
    if (!blendArg->hasBlend)
        return 1;
    
    i = 0;
    while (i< h->glyph.info->blendInfo.numRegions)
    {
        float delta = (float)RND_ON_WRITE(blendArg->blendValues[i] - blendArg->value);
        if (delta != 0)
            return 0;
        i++;
    }
    return 1;
}


static void glyphMoveVF(abfGlyphCallbacks *cb, abfBlendArg* argX, abfBlendArg* argY) {
    cfwCtx g = (cfwCtx)cb->direct_ctx;
    cstrCtx h = g->ctx.cstr;
    float x0, dx0;
    float y0, dy0;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    
    x0 = (float)RND_ON_WRITE(argX->value);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y0 = (float)RND_ON_WRITE(argY->value);
    dx0 = x0 - h->x;
    dy0 = y0 - h->y;

    /* Check pending op */
    switch (h->pendop) {
        case tx_vmoveto:
            clearmoveto(h, 0, h->stack.array[0]);
            break;
            
        case tx_hmoveto:
            clearmoveto(h, h->stack.array[0], 0);
            break;
            
        case tx_rmoveto:
            clearmoveto(h, h->stack.array[0], h->stack.array[1]);
            break;
            
        case tx_dotsection:
            /* Discard dotsection before closepath */
            clearop(h);
            break;
            
        case tx_noop:
            break;
            
        default:
            saveop(h, h->pendop);
            break;
    }

    /* h->pendop is now tx_noop */
    h->x = x0;
    h->y = y0;

    if (doOptimize && blendIsZero(h, dx0, argX)) {
        pushBlend(h, argY);
        PUSH(dy0);
        h->pendop = tx_vmoveto;
    }
    else if (doOptimize && blendIsZero(h, dy0, argY)) {
        pushBlend(h, argX);
        PUSH(dx0);
        h->pendop = tx_hmoveto;
    }
    else {
        pushBlend(h, argX);
        PUSH(dx0);
        pushBlend(h, argY);
        PUSH(dy0);
        h->pendop = tx_rmoveto;
    }
    h->flags |= SEEN_MOVETO;
    
}

/* Insert missing initial moveto op. */
static void insertMove(abfGlyphCallbacks *cb) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	addWarn(h, warn_move0);
	glyphMove(cb, 0, 0);
}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

    float dx1;
    float dy1;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    
    x1 = (float)RND_ON_WRITE(x1);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    dx1 = x1 - h->x;
    dy1 = y1 - h->y;

    h->x = x1;
	h->y = y1;

	if (!(h->flags & SEEN_MOVETO)) {
		insertMove(cb);
	}

	/* Choose format */
	if ((dx1 == 0.0) && doOptimize) {
		/* - dy1 vlineto */
		flushop(h, 1);
		switch (h->pendop) {
			case tx_hlineto:
				/*  0  */ PUSH(dy1);
				h->pendop = tx_vlineto;
				break;

			default:
				saveop(h, h->pendop);

			/* Fall through */
			case tx_noop:
				/*  0  */ PUSH(dy1);
				h->pendop = h->seqop = tx_vlineto;
		}
	}
	else if ((dy1 == 0.0) && doOptimize) {
		/* dx1 - hlineto */
		flushop(h, 1);
		switch (h->pendop) {
			case tx_vlineto:
				PUSH(dx1); /*  0  */
				h->pendop = tx_hlineto;
				break;

			default:
				saveop(h, h->pendop);

			/* Fall through */
			case tx_noop:
				PUSH(dx1); /*  0  */
				h->pendop = h->seqop = tx_hlineto;
				break;
		}
	}
	else {
		/* dx1 dy1 rlineto */
		flushop(h, 2);
        if (doOptimize)
        {
            switch (h->pendop) {
                case tx_rlineto:
                    PUSH(dx1);
                    PUSH(dy1);
                    break;
                    
                case tx_rrcurveto:
                    PUSH(dx1);
                    PUSH(dy1);
                    saveop(h, t2_rcurveline);
                    break;
                    
                default:
                   saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    PUSH(dx1);
                    PUSH(dy1);
                    h->pendop = tx_rlineto;
                    break;
            }
        }
        else
        {
            if (h->pendop != tx_noop)
                saveop(h, h->pendop);
            PUSH(dx1);
            PUSH(dy1);
            h->pendop = tx_rlineto;
        }
	}
}

static void glyphLineVF(abfGlyphCallbacks *cb, abfBlendArg* argX, abfBlendArg* argY) {
    cfwCtx g = (cfwCtx)cb->direct_ctx;
    cstrCtx h = g->ctx.cstr;
    float x0, dx0;
    float y0, dy0;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    

    x0 = (float)RND_ON_WRITE(argX->value);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y0 = (float)RND_ON_WRITE(argY->value);
    dx0 = x0 - h->x;
    dy0 = y0 - h->y;

    h->x = x0;
    h->y = y0;

    if (!(h->flags & SEEN_MOVETO)) {
        insertMove(cb);
    }

    if (doOptimize && blendIsZero(h, dx0, argX)) {
        flushop(h, 1);
        switch (h->pendop) {
            case tx_hlineto:
                pushBlend(h, argY);
                PUSH(dy0);
                h->pendop = tx_vlineto;
                break;

            default:
                saveop(h, h->pendop);
                /* Fall through */

            case tx_noop:
                pushBlend(h, argY);
                PUSH(dy0);
               h->pendop = h->seqop = tx_vlineto;
                
        }
    }
    else if (doOptimize && blendIsZero(h, dy0, argY)) {
        flushop(h, 1);
        switch (h->pendop) {
            case tx_vlineto:
                pushBlend(h, argX);
                PUSH(dx0);
                h->pendop = tx_hlineto;
                break;
                
            default:
                saveop(h, h->pendop);
                /* Fall through */
                
            case tx_noop:
                pushBlend(h, argX);
                PUSH(dx0);
                 h->pendop = h->seqop = tx_hlineto;
                
        }
    }
    else {
        flushop(h, 2);
        if (doOptimize)
        {
            switch (h->pendop) {
                case tx_rlineto:
                    pushBlend(h, argX);
                    PUSH(dx0);
                    pushBlend(h, argY);
                    PUSH(dy0);
                    break;
                    
                case tx_rrcurveto:
                    pushBlend(h, argX);
                    PUSH(dx0);
                    pushBlend(h, argY);
                    PUSH(dy0);
                    saveop(h, t2_rcurveline);
                    break;
                    
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, argX);
                    PUSH(dx0);
                    pushBlend(h, argY);
                    PUSH(dy0);
                    h->pendop = tx_rlineto;
                    break;
            }
        }
        else
        {
            if (h->pendop != tx_noop)
                saveop(h, h->pendop);
            pushBlend(h, argX);
            PUSH(dx0);
            pushBlend(h, argY);
            PUSH(dy0);
            h->pendop = tx_rlineto;
        }
    }

}

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
                       float x1, float y1,
                       float x2, float y2,
                       float x3, float y3) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

    float dx1;
    float dy1;
    float dx2;
    float dy2;
    float dx3;
    float dy3;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    
    x1 = (float)RND_ON_WRITE(x1); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    x2 = (float)RND_ON_WRITE(x2);
    y2 = (float)RND_ON_WRITE(y2);
    x3 = (float)RND_ON_WRITE(x3);
    y3 = (float)RND_ON_WRITE(y3);
    
    dx1 = x1 - h->x;
    dy1 = y1 - h->y;
    dx2 = x2 - x1;
    dy2 = y2 - y1;
    dx3 = x3 - x2;
    dy3 = y3 - y2;

    h->x = x3;
	h->y = y3;

	if (!(h->flags & SEEN_MOVETO)) {
		insertMove(cb);
	}

	/* Choose format */
	if ((dx1 == 0.0) && doOptimize) {
		if (dy3 == 0.0) {
			/* - dy1 dx2 dy2 dx3 - vhcurveto */
			flushop(h, 4);
			switch (h->pendop) {
				case tx_hvcurveto:
					/*  0  */ PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3); /*  0  */
					h->pendop = tx_vhcurveto;
					break;

				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					/*  0  */ PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3); /*  0  */
					h->pendop = h->seqop = tx_vhcurveto;
			}
		}
		else if (dx3 == 0.0) {
			/* - dy1 dx2 dy2 - dy3 vvcurveto */
			flushop(h, 4);
			switch (h->pendop) {
				case t2_vvcurveto:
					/*  0  */ PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					/*  0  */ PUSH(dy3);
					break;

				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					/*  0  */ PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					/*  0  */ PUSH(dy3);
					h->pendop = t2_vvcurveto;
					break;
			}
		}
		else {
			/* - dy1 dx2 dy2 dx3 dy3 vhcurveto (odd args) */
			flushop(h, 5);
			switch (h->pendop) {
				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					h->seqop = tx_vhcurveto;

				/* Fall through */
				case tx_hvcurveto:
					/*  0  */ PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3);
					PUSH(dy3);
					saveop(h, tx_vhcurveto);
					break;
			}
		}
	}
	else if ((dy1 == 0.0) && doOptimize) {
		if (dx3 == 0.0) {
			/* dx1 - dx2 dy2 - dy3 hvcurveto */
			flushop(h, 4);
			switch (h->pendop) {
				case tx_vhcurveto:
					PUSH(dx1); /*  0  */
					PUSH(dx2);
					PUSH(dy2);
					/*  0  */ PUSH(dy3);
					h->pendop = tx_hvcurveto;
					break;

				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					PUSH(dx1); /*  0  */
					PUSH(dx2);
					PUSH(dy2);
					/*  0  */ PUSH(dy3);
					h->pendop = h->seqop = tx_hvcurveto;
					break;
			}
		}
		else if (dy3 == 0.0) {
			/* dx1 - dx2 dy2 dx3 - hhcurveto */
			flushop(h, 4);
			switch (h->pendop) {
				case t2_hhcurveto:
					PUSH(dx1); /*  0  */
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3); /*  0  */
					break;

				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					PUSH(dx1); /*  0  */
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3); /*  0  */
					h->pendop = t2_hhcurveto;
					break;
			}
		}
		else {
			/* dx1 - dx2 dy2 dy3 dx3 hvcurveto (odd args) */
			flushop(h, 5);
			switch (h->pendop) {
				default:
 					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					h->seqop = tx_hvcurveto;

				/* Fall through */
				case tx_vhcurveto:
					PUSH(dx1); /*  0  */
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dy3);
					PUSH(dx3);          /* Note arg swap */
					saveop(h, tx_hvcurveto);
					break;
			}
		}
	}
	else {
		if ((dx3 == 0.0) && doOptimize) {
			/* dx1 dy1 dx2 dy2 - dy3 vvcurveto (odd args) */
			flushop(h, 5);
			switch (h->pendop) {
				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					PUSH(dx1);
					PUSH(dy1);
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dy3); /*  0  */
					h->pendop = t2_vvcurveto;
					break;
			}
		}
		else if ((dy3 == 0.0) && doOptimize) {
			/* dx1 dy1 dx2 dy2 dx3 - hhcurveto (odd args) */
			flushop(h, 5);
			switch (h->pendop) {
				default:
					saveop(h, h->pendop);

				/* Fall through */
				case tx_noop:
					PUSH(dy1);
					PUSH(dx1);          /* Note arg swap */
					PUSH(dx2);
					PUSH(dy2);
					PUSH(dx3); /*  0  */
					h->pendop = t2_hhcurveto;
			}
		}
        else {
            /* dx1 dy1 dx2 dy2 dx3 dy3 rrcurveto */
            flushop(h, 6);
            if (doOptimize) {
                switch (h->pendop) {
                    case tx_rlineto:
                        PUSH(dx1);
                        PUSH(dy1);
                        PUSH(dx2);
                        PUSH(dy2);
                        PUSH(dx3);
                        PUSH(dy3);
                        saveop(h, t2_rlinecurve);
                        break;
                        
                    case tx_rrcurveto:
                        PUSH(dx1);
                        PUSH(dy1);
                        PUSH(dx2);
                        PUSH(dy2);
                        PUSH(dx3);
                        PUSH(dy3);
                        break;
                        
                    default:
                        saveop(h, h->pendop);
                        
                        /* Fall through */
                    case tx_noop:
                        PUSH(dx1);
                        PUSH(dy1);
                        PUSH(dx2);
                        PUSH(dy2);
                        PUSH(dx3);
                        PUSH(dy3);
                        h->pendop = tx_rrcurveto;
                        break;
                }
            }
            else
            {
                if (h->pendop != tx_noop)
                    saveop(h, h->pendop);
                PUSH(dx1);
                PUSH(dy1);
                PUSH(dx2);
                PUSH(dy2);
                PUSH(dx3);
                PUSH(dy3);
                h->pendop = tx_rrcurveto;
            }
        }
	}
}

static void glyphCurveVF(abfGlyphCallbacks *cb,
                         abfBlendArg* arg_x1, abfBlendArg* arg_y1,
                         abfBlendArg* arg_x2, abfBlendArg* arg_y2,
                         abfBlendArg* arg_x3, abfBlendArg* arg_y3) {
    cfwCtx g = (cfwCtx)cb->direct_ctx;
    cstrCtx h = g->ctx.cstr;
    
    float x1, dx1;
    float y1, dy1;
    float x2, dx2;
    float y2, dy2;
    float x3, dx3;
    float y3, dy3;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    
    x1 = (float)RND_ON_WRITE(arg_x1->value); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(arg_y1->value);
    x2 = (float)RND_ON_WRITE(arg_x2->value);
    y2 = (float)RND_ON_WRITE(arg_y2->value);
    x3 = (float)RND_ON_WRITE(arg_x3->value);
    y3 = (float)RND_ON_WRITE(arg_y3->value);
    
    dx1 = x1 - h->x;
    dy1 = y1 - h->y;
    dx2 = x2 - x1;
    dy2 = y2 - y1;
    dx3 = x3 - x2;
    dy3 = y3 - y2;
    
    h->x = x3;
    h->y = y3;
    
    if (!(h->flags & SEEN_MOVETO)) {
        insertMove(cb);
    }
    /* Choose format */
    if (blendIsZero(h, dx1, arg_x1) && doOptimize) {
        if (blendIsZero(h, dy3, arg_y3)) {
            /* - dy1 dx2 dy2 dx3 - vhcurveto */
            flushop(h, 4);
            switch (h->pendop) {
                case tx_hvcurveto:
                    pushBlend(h, arg_y1);
                    /*  0  */ PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3); /*  0  */
                    h->pendop = tx_vhcurveto;
                    break;
                    
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_y1);
                    /*  0  */ PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3); /*  0  */
                    h->pendop = h->seqop = tx_vhcurveto;
            }
        }
        else if (blendIsZero(h, dx3, arg_x3)) {
            /* - dy1 dx2 dy2 - dy3 vvcurveto */
            flushop(h, 4);
            switch (h->pendop) {
                case t2_vvcurveto:
                    pushBlend(h, arg_y1);
                    /*  0  */ PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    /*  0  */ PUSH(dy3);
                    break;
                    
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_y1);
                    /*  0  */ PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    /*  0  */ PUSH(dy3);
                    h->pendop = t2_vvcurveto;
                    break;
            }
        }
        else {
            /* - dy1 dx2 dy2 dx3 dy3 vhcurveto (odd args) */
            flushop(h, 5);
            switch (h->pendop) {
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    h->seqop = tx_vhcurveto;
                    
                    /* Fall through */
                case tx_hvcurveto:
                    pushBlend(h, arg_y1);
                    /*  0  */ PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3);
                    pushBlend(h, arg_y3);
                    PUSH(dy3);
                    saveop(h, tx_vhcurveto);
                    break;
            }
        }
    }
    else if (blendIsZero(h, dy1, arg_y1) && doOptimize) {
        if (blendIsZero(h, dx3, arg_x3)) {
            /* dx1 - dx2 dy2 - dy3 hvcurveto */
            flushop(h, 4);
            switch (h->pendop) {
                case tx_vhcurveto:
                    pushBlend(h, arg_x1);
                    PUSH(dx1); /*  0  */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    /*  0  */ PUSH(dy3);
                    h->pendop = tx_hvcurveto;
                    break;
                    
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_x1);
                    PUSH(dx1); /*  0  */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    /*  0  */ PUSH(dy3);
                    h->pendop = h->seqop = tx_hvcurveto;
                    break;
            }
        }
        else if (blendIsZero(h, dy3, arg_y3)) {
            /* dx1 - dx2 dy2 dx3 - hhcurveto */
            flushop(h, 4);
            switch (h->pendop) {
                case t2_hhcurveto:
                    pushBlend(h, arg_x1);
                    PUSH(dx1); /*  0  */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3); /*  0  */
                    break;
                    
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_x1);
                    PUSH(dx1); /*  0  */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3); /*  0  */
                    h->pendop = t2_hhcurveto;
                    break;
            }
        }
        else {
            /* dx1 - dx2 dy2 dy3 dx3 hvcurveto (odd args) */
            flushop(h, 5);
            switch (h->pendop) {
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    h->seqop = tx_hvcurveto;
                    
                    /* Fall through */
                case tx_vhcurveto:
                    pushBlend(h, arg_x1);
                    PUSH(dx1); /*  0  */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    PUSH(dy3);
                    pushBlend(h, arg_x3);
                    PUSH(dx3);          /* Note arg swap */
                    saveop(h, tx_hvcurveto);
                    break;
            }
        }
    }
    else {
        if (blendIsZero(h, dx3, arg_x3) && doOptimize) {
            /* dx1 dy1 dx2 dy2 - dy3 vvcurveto (odd args) */
            flushop(h, 5);
            switch (h->pendop) {
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_x1);
                    PUSH(dx1);
                    pushBlend(h, arg_y1);
                    PUSH(dy1);
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_y3);
                    PUSH(dy3); /*  0  */
                    h->pendop = t2_vvcurveto;
                    break;
            }
        }
        else if (blendIsZero(h, dy3, arg_y3) && doOptimize) {
            /* dx1 dy1 dx2 dy2 dx3 - hhcurveto (odd args) */
            flushop(h, 5);
            switch (h->pendop) {
                default:
                    saveop(h, h->pendop);
                    
                    /* Fall through */
                case tx_noop:
                    pushBlend(h, arg_y1);
                    PUSH(dy1);
                    pushBlend(h, arg_x1);
                    PUSH(dx1);          /* Note arg swap */
                    pushBlend(h, arg_x2);
                    PUSH(dx2);
                    pushBlend(h, arg_y2);
                    PUSH(dy2);
                    pushBlend(h, arg_x3);
                    PUSH(dx3); /*  0  */
                    h->pendop = t2_hhcurveto;
            }
        }
        else {
            /* dx1 dy1 dx2 dy2 dx3 dy3 rrcurveto */
            flushop(h, 6);
            if (doOptimize) {
                switch (h->pendop) {
                    case tx_rlineto:
                        pushBlend(h, arg_x1);
                        PUSH(dx1);
                        pushBlend(h, arg_y1);
                        PUSH(dy1);
                        pushBlend(h, arg_x2);
                        PUSH(dx2);
                        pushBlend(h, arg_y2);
                        PUSH(dy2);
                        pushBlend(h, arg_x3);
                        PUSH(dx3);
                        pushBlend(h, arg_y3);
                        PUSH(dy3);
                        saveop(h, t2_rlinecurve);
                        break;
                        
                    case tx_rrcurveto:
                        pushBlend(h, arg_x1);
                        PUSH(dx1);
                        pushBlend(h, arg_y1);
                        PUSH(dy1);
                        pushBlend(h, arg_x2);
                        PUSH(dx2);
                        pushBlend(h, arg_y2);
                        PUSH(dy2);
                        pushBlend(h, arg_x3);
                        PUSH(dx3);
                        pushBlend(h, arg_y3);
                        PUSH(dy3);
                        break;
                        
                    default:
                        saveop(h, h->pendop);
                        
                        /* Fall through */
                    case tx_noop:
                        pushBlend(h, arg_x1);
                        PUSH(dx1);
                        pushBlend(h, arg_y1);
                        PUSH(dy1);
                        pushBlend(h, arg_x2);
                        PUSH(dx2);
                        pushBlend(h, arg_y2);
                        PUSH(dy2);
                        pushBlend(h, arg_x3);
                        PUSH(dx3);
                        pushBlend(h, arg_y3);
                        PUSH(dy3);
                        h->pendop = tx_rrcurveto;
                        break;
                }
            }
            else
            {
                if (h->pendop != tx_noop)
                    saveop(h, h->pendop);
                pushBlend(h, arg_x1);
                PUSH(dx1);
                pushBlend(h, arg_y1);
                PUSH(dy1);
                pushBlend(h, arg_x2);
                PUSH(dx2);
                pushBlend(h, arg_y2);
                PUSH(dy2);
                pushBlend(h, arg_x3);
                PUSH(dx3);
                pushBlend(h, arg_y3);
                PUSH(dy3);
                h->pendop = tx_rrcurveto;
            }
        }
    }
}

static void glyphCurveVF2(abfGlyphCallbacks *cb,
                        abfBlendArg* arg_x1, abfBlendArg* arg_y1,
                        abfBlendArg* arg_x2, abfBlendArg* arg_y2,
                        abfBlendArg* arg_x3, abfBlendArg* arg_y3) {
    cfwCtx g = (cfwCtx)cb->direct_ctx;
    cstrCtx h = g->ctx.cstr;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);
    float x1, dx1;
    float y1, dy1;
    float x2, dx2;
    float y2, dy2;
    float x3, dx3;
    float y3, dy3;
    
    x1 = (float)RND_ON_WRITE(arg_x1->value); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(arg_y1->value);
    x2 = (float)RND_ON_WRITE(arg_x2->value);
    y2 = (float)RND_ON_WRITE(arg_y2->value);
    x3 = (float)RND_ON_WRITE(arg_x3->value);
    y3 = (float)RND_ON_WRITE(arg_y3->value);
    
    dx1 = x1 - h->x;
    dy1 = y1 - h->y;
    dx2 = x2 - x1;
    dy2 = y2 - y1;
    dx3 = x3 - x2;
    dy3 = y3 - y2;

    if (!(h->flags & SEEN_MOVETO)) {
        insertMove(cb);
    }
    
    h->x = x3;
    h->y = y3;
    
    
    if (1) {
        if (h->pendop != tx_noop)
            saveop(h, h->pendop);
        pushBlend(h, arg_x1);
        PUSH(dx1);
        pushBlend(h, arg_y1);
        PUSH(dy1);
        pushBlend(h, arg_x2);
        PUSH(dx2);
        pushBlend(h, arg_y2);
        PUSH(dy2);
        pushBlend(h, arg_x3);
        PUSH(dx3);
        pushBlend(h, arg_y3);
        PUSH(dy3);
        h->pendop = tx_rrcurveto;
    }
}

/* Save hint/cntr mask. */
static int saveMask(cstrCtx h, HintMask mask) {
	int size = (h->stems.cnt + 7) / 8;
	long index = dnaExtend(&h->masks, 1, size);
	if (index == -1) {
		h->g->err.code = cfwErrNoMemory;
		return 0;
	}
	memcpy(&h->masks.array[index], mask, size);
	memset(mask, 0, size);
	return size;
}

/* Save hint. */
static void saveHint(cstrCtx h) {
	Hint *hint;
	long index = dnaNext(&h->hints, sizeof(Hint));
	if (index == -1) {
		h->g->err.code = cfwErrNoMemory;
		return;
	}
	hint = &h->hints.array[index];
	hint->iCstr = h->iCstr;
	hint->iMask = h->masks.cnt;
	hint->size = saveMask(h, h->hintmask);
}

/* Start new hint. */
static void newHint(cstrCtx h) {
	if (h->pendop == tx_noop && h->iCstr == h->cstr.cnt) {
		/* Consecutive hintsubs; clear mask */
		addWarn(h, warn_hint6);
		CLEAR_MASK(h->hintmask);
	}
	else {
		if (h->pendop != tx_noop) {
			saveop(h, h->pendop);   /* Save pending op */
		}
		if (h->flags & SEEN_HINT) {
			saveHint(h);
		}
		else {
			h->flags |= SEEN_HINT;
		}

		h->iCstr = h->cstr.cnt;
	}
}

/* Save counter. */
static void saveCntr(cstrCtx h) {
	Cntr *cntr;
	long index = dnaNext(&h->cntrs, sizeof(Cntr));
	if (index == -1) {
		h->g->err.code = cfwErrNoMemory;
		return;
	}
	cntr = &h->cntrs.array[index];
	cntr->iMask = h->masks.cnt;
	cntr->size = saveMask(h, h->cntrmask);
}

/* Start new counter. */
static void newCntr(cstrCtx h) {
	if (h->pendop != tx_noop) {
		saveop(h, h->pendop);   /* Save pending op */
	}
	if (h->flags & SEEN_CNTR) {
		saveCntr(h);
	}
	else {
		h->flags |= SEEN_CNTR;
	}
}

/* Match stems. */
static int CTL_CDECL matchStems(const void *key, const void *value, void *ctx) {
	Stem *a = (Stem *)key;
	Stem *b = (Stem *)value;
	int cmp = (a->flags & STEM_VERT) - (b->flags & STEM_VERT);
	if (cmp != 0) {
		return cmp;
	}
	else if (a->edge0 < b->edge0) {
		return -1;
	}
	else if (a->edge0 > b->edge0) {
		return 1;
	}
	else if (a->edge1 < b->edge1) {
		return -1;
	}
	else if (a->edge1 > b->edge1) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Match stems if both edges are within 2 units. */
static int approxMatch(Stem *_new, Stem *old) {
	float delta0 = _new->edge0 - old->edge0;
	float delta1 = _new->edge1 - old->edge1;
	return ((old->flags & STEM_CNTR) &&
	        (_new->flags & STEM_VERT) == (old->flags & STEM_VERT) &&
	        -2.0f < delta0 && delta0 < 2.0f &&
	        -2.0f < delta1 && delta1 < 2.0f);
}

/* Add stem to list. Return stem id. */
static unsigned char addStem(cstrCtx h, int flags, float edge0, float edge1) {
	int found;
	size_t index;
	Stem _new;
	Stem *stem;
#if PLAT_WIN || PLAT_LINUX
	/* On the Pentium architecture in an optimized build, float variables
	   are kept in 80-bit registers which is more precision than is needed and
	   in fact causes the comparisons against the ABF_EDGE_HINT_LO and
	   ABF_EDGE_HINT_HI integers to fail. The "voilatile" qualifier forces
	   intermediate stores and loads which circumvents this problem. */
	volatile float delta;
#else
	float delta;
#endif

	delta = edge1 - edge0;
	if (delta < 0 && delta != ABF_EDGE_HINT_LO && delta != ABF_EDGE_HINT_HI) {
		addWarn(h, warn_hint0);
		_new.edge0 = edge1;
		_new.edge1 = edge0;
	}
	else {
		_new.edge0 = edge0;
		_new.edge1 = edge1;
	}
	_new.flags = (flags & ABF_VERT_STEM) ? STEM_VERT : 0;
	if (flags & ABF_CNTR_STEM) {
		_new.flags |= STEM_CNTR;
	}

	/* Check if stem already in list */
	found = ctuLookup(&_new, h->stems.array, h->stems.cnt, sizeof(Stem),
	                  matchStems, &index, NULL);
	stem = &h->stems.array[index];
	if (!found) {
		if (!(h->g->flags & CFW_ENABLE_CMP_TEST) &&
		    (h->flags & SEEN_CNTR) && !(flags & ABF_CNTR_STEM) &&
		    h->stems.cnt != (int)index) {
			/* We are trying to match a stem against a stem list containing
			   global coloring data. The global coloring stems can have a
			   2-unit error on each edge so we have to perform approximate
			   match. This is the correct conversion behavior but can cause
			   rendering differences between the source font and the converted
			   font so we disable this if are trying to compare bitmaps. */
			if (approxMatch(&_new, stem)) {
				return stem->id;
			}
			if (index > 0 && approxMatch(&_new, stem - 1)) {
				return (stem - 1)->id;
			}
		}

		/* New stem */
		if (h->stems.cnt == T2_MAX_STEMS) {
			addWarn(h, warn_hint3);
		}
		else {
			/* Insert new stem in list */
			memmove(stem + 1, stem, (h->stems.cnt - index) * sizeof(Stem));
			_new.id = (unsigned char)h->stems.cnt++;
			*stem = _new;
		}
	}

	return stem->id;
}

static unsigned char addStemVF(cstrCtx h, int flags, abfBlendArg* edge0v, abfBlendArg* edge1v) {
    int found;
    size_t index;
    Stem _new;
    Stem *stem;
    float edge0 = edge0v->value;
    float edge1 = edge1v->value;
    
#if PLAT_WIN || PLAT_LINUX
    /* On the Pentium architecture in an optimized build, float variables
     are kept in 80-bit registers which is more precision than is needed and
     in fact causes the comparisons against the ABF_EDGE_HINT_LO and
     ABF_EDGE_HINT_HI integers to fail. The "voilatile" qualifier forces
     intermediate stores and loads which circumvents this problem. */
    volatile float delta;
#else
    float delta;
#endif
    
    delta = edge1 - edge0;
    if (delta < 0 && delta != ABF_EDGE_HINT_LO && delta != ABF_EDGE_HINT_HI) {
        addWarn(h, warn_hint0);
        _new.edge0v = *edge1v;
        _new.edge1v = *edge0v;
    }
    else {
        _new.edge0v = *edge0v;
        _new.edge1v = *edge1v;
    }
    _new.flags = (flags & ABF_VERT_STEM) ? STEM_VERT : 0;
    if (flags & ABF_CNTR_STEM) {
        _new.flags |= STEM_CNTR;
    }
    
    /* Check if stem already in list */
    found = ctuLookup(&_new, h->stems.array, h->stems.cnt, sizeof(Stem),
                      matchStems, &index, NULL);
    stem = &h->stems.array[index];
    if (!found) {
        if (!(h->g->flags & CFW_ENABLE_CMP_TEST) &&
            (h->flags & SEEN_CNTR) && !(flags & ABF_CNTR_STEM) &&
            h->stems.cnt != (int)index) {
            /* We are trying to match a stem against a stem list containing
             global coloring data. The global coloring stems can have a
             2-unit error on each edge so we have to perform approximate
             match. This is the correct conversion behavior but can cause
             rendering differences between the source font and the converted
             font so we disable this if are trying to compare bitmaps. */
            if (approxMatch(&_new, stem)) {
                return stem->id;
            }
            if (index > 0 && approxMatch(&_new, stem - 1)) {
                return (stem - 1)->id;
            }
        }
        
        /* New stem */
        if (h->stems.cnt == T2_MAX_STEMS) {
            addWarn(h, warn_hint3);
        }
        else {
            /* Insert new stem in list */
            memmove(stem + 1, stem, (h->stems.cnt - index) * sizeof(Stem));
            _new.id = (unsigned char)h->stems.cnt++;
            *stem = _new;
        }
    }
    
    return stem->id;
}

/* Add stem hint. */
static void glyphStem(abfGlyphCallbacks *cb,
                      int flags, float edge0, float edge1) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	unsigned char stemid;

	if (h->flags & SEEN_MOVETO) {
		if (flags & ABF_NEW_HINTS || h->stems.cnt == 0) {
			newHint(h);
		}
	}
	else {
		if (flags & ABF_NEW_HINTS && h->stems.cnt > 0) {
			newHint(h);
		}
	}

	stemid = addStem(h, flags, edge0, edge1);

	if (flags & (ABF_CNTR_STEM | ABF_STEM3_STEM)) {
		/* Add counter-controlled stem */
		if (flags & ABF_NEW_GROUP || !(h->flags & SEEN_CNTR)) {
			newCntr(h);
		}
		SET_ID_BIT(h->cntrmask, stemid);
	}

	if (!(flags & ABF_CNTR_STEM)) {
		/* Add stem or stem3 hint */
		if (h->flags & SEEN_HINT) {
			SET_ID_BIT(h->hintmask, stemid);
		}
		else {
			SET_ID_BIT(h->initmask, stemid);
		}
	}
}

static void glyphStemVF(abfGlyphCallbacks *cb,
                      int flags, abfBlendArg*  edge0v, abfBlendArg*  edge1v) {
    cfwCtx g = (cfwCtx)cb->direct_ctx;
    cstrCtx h = g->ctx.cstr;
    unsigned char stemid;
    
    if (h->flags & SEEN_MOVETO) {
        if (flags & ABF_NEW_HINTS || h->stems.cnt == 0) {
            newHint(h);
        }
    }
    else {
        if (flags & ABF_NEW_HINTS && h->stems.cnt > 0) {
            newHint(h);
        }
    }
    
    stemid = addStemVF(h, flags, edge0v, edge1v);
    
    if (flags & (ABF_CNTR_STEM | ABF_STEM3_STEM)) {
        /* Add counter-controlled stem */
        if (flags & ABF_NEW_GROUP || !(h->flags & SEEN_CNTR)) {
            newCntr(h);
        }
        SET_ID_BIT(h->cntrmask, stemid);
    }
    
    if (!(flags & ABF_CNTR_STEM)) {
        /* Add stem or stem3 hint */
        if (h->flags & SEEN_HINT) {
            SET_ID_BIT(h->hintmask, stemid);
        }
        else {
            SET_ID_BIT(h->initmask, stemid);
        }
    }
}

/* Add flex hint. */
static void glyphFlex(abfGlyphCallbacks *cb, float depth,
                      float x1, float y1,
                      float x2, float y2,
                      float x3, float y3,
                      float x4, float y4,
                      float x5, float y5,
                      float x6, float y6) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	float x0 = h->x;
	float y0 = h->y;
	h->x = x6;
	h->y = y6;

	if (h->pendop != tx_noop) {
		saveop(h, h->pendop);
	}

    x1 = (float)RND_ON_WRITE(x1); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    x2 = (float)RND_ON_WRITE(x2);
    y2 = (float)RND_ON_WRITE(y2);
    x3 = (float)RND_ON_WRITE(x3);
    y3 = (float)RND_ON_WRITE(y3);
    x4 = (float)RND_ON_WRITE(x4);
    y4 = (float)RND_ON_WRITE(y4);
    x5 = (float)RND_ON_WRITE(x5);
    y5 = (float)RND_ON_WRITE(y5);
    x6 = (float)RND_ON_WRITE(x6);
    y6 = (float)RND_ON_WRITE(y6);

	if (depth != TX_STD_FLEX_DEPTH) {
		/* dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 dx6 dy6 depth flex */
flex:
		PUSH(x1 - x0);
		PUSH(y1 - y0);
		PUSH(x2 - x1);
		PUSH(y2 - y1);
		PUSH(x3 - x2);
		PUSH(y3 - y2);
		PUSH(x4 - x3);
		PUSH(y4 - y3);
		PUSH(x5 - x4);
		PUSH(y5 - y4);
		PUSH(x6 - x5);
		PUSH(y6 - y5);
		PUSH(depth);
		saveop(h, t2_flex);
	}
	else {
		int horiz;
		if (y0 == y6) {
			if (y2 == y3 && y3 == y4) {
				if (y0 == y1 && y5 == y6) {
					/* dx1 - dx2 dy2 dx3 - dx4 - dx5 - dx6 - - hflex */
					PUSH(x1 - x0); /*    0    */
					PUSH(x2 - x1);
					PUSH(y2 - y1);
					PUSH(x3 - x2); /*    0    */
					PUSH(x4 - x3); /*    0    */
					PUSH(x5 - x4); /*    0    */
					PUSH(x6 - x5); /*    0    */
					/*    50    */
					saveop(h, t2_hflex);
				}
				else {
					/* dx1 dy1 dx2 dy2 dx3 - dx4 - dx5 dy5 dx6 - - hflex1 */
					PUSH(x1 - x0);
					PUSH(y1 - y0);
					PUSH(x2 - x1);
					PUSH(y2 - y1);
					PUSH(x3 - x2); /*    0    */
					PUSH(x4 - x3); /*    0    */
					PUSH(x5 - x4);
					PUSH(y5 - y4);
					PUSH(x6 - x5); /*    0    */
					/*    50    */
					saveop(h, t2_hflex1);
				}
				return;
			}
			horiz = 1;
		}
		else if (x0 == x6) {
			horiz = 0;
		}
		else {
			addWarn(h, warn_flex0);
			goto flex;
		}

		if ((fabs(x5 - x0) > fabs(y5 - y0)) != horiz) {
			addWarn(h, warn_flex1);
			goto flex;
		}

		/* dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 dx6 - - flex1 (h) */
		/* dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 - dy6 - flex1 (v) */
		PUSH(x1 - x0);
		PUSH(y1 - y0);
		PUSH(x2 - x1);
		PUSH(y2 - y1);
		PUSH(x3 - x2);
		PUSH(y3 - y2);
		PUSH(x4 - x3);
		PUSH(y4 - y3);
		PUSH(x5 - x4);
		PUSH(y5 - y4);
		PUSH(horiz ? x6 - x5 : y6 - y5);
		saveop(h, t2_flex1);
	}
}

/* Add generic operator. */
static void glyphGenop(abfGlyphCallbacks *cb, int cnt, float *args, int op) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

	if (h->glyph.info->flags & ABF_GLYPH_CUBE_GSUBR) {
		switch (op) {
			case tx_return:
				if (cnt) {
					/* use use tx_return to signal the end of a subr when there are numbers
					   on the stack */
					float *p = &h->stack.array[h->stack.cnt];
					memmove(p, args, cnt * sizeof(float));
					h->stack.cnt += cnt;
				}
				break;

			case tx_SETWV1:
			case tx_SETWV2:
			case tx_SETWV3:
			case tx_SETWV4:
			case tx_SETWV5:
				if (cnt) {
					float *p = &h->stack.array[h->stack.cnt];
					memmove(p, args, cnt * sizeof(float));
					h->stack.cnt += cnt;
				}
				cfwSeenLEIndex(g, -1);
				saveopDirect(h, op);
				break;

			case tx_SETWVN:
				if (cnt) {
					float *p = &h->stack.array[h->stack.cnt];
					memmove(p, args, cnt * sizeof(float));
					h->stack.cnt += cnt;
				}
				cfwSeenLEIndex(g, -1);
				saveopDirect(h, op);
				break;

			case tx_callgrel:
			{
				float *p = &h->stack.array[h->stack.cnt];
				memmove(p, args, cnt * sizeof(float));
				h->stack.cnt += cnt;
				cfwSeenLEIndex(g, (long)args[cnt - 1]);
				saveopDirect(h, op);
				break;
			}

			default:
			{
				if (cnt) {
					float *p = &h->stack.array[h->stack.cnt];
					memmove(p, args, cnt * sizeof(float));
					h->stack.cnt += cnt;
				}
				saveopDirect(h, op);
				break;
			}
		}
	}
	else {
		if (h->pendop != tx_noop) {
			saveop(h, h->pendop);
		}

		switch (op) {
			case t2_cntron:
				h->pendop = t2_cntron;
				break;

			case t2_cntroff:
				/* Create all clear cntrmask */
				newCntr(h);
				break;

			default:
				if (cnt) {
					memmove(h->stack.array, args, cnt * sizeof(float));
					h->stack.cnt = cnt;
				}
				if (op == tx_dotsection) {
					h->pendop = tx_dotsection;
				}
				else {
					saveop(h, op);
				}
				break;
		}
	}
}

/* Add seac operator. */
static void glyphSeac(abfGlyphCallbacks *cb,
                      float adx, float ady, int bchar, int achar) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	if (h->pendop != tx_noop) {
		saveop(h, h->pendop);
	}
	PUSH(adx);
	PUSH(ady);
	PUSH(bchar);
	PUSH(achar);
}

/* Save op code in tmp file. */
static void tmp_saveop(cfwCtx g, int op) {
	unsigned char t[2];
	size_t count;
	if (op & 0xff00) {
		t[0] = tx_escape;
		t[1] = (unsigned char)op;
		count = 2;
	}
	else {
		t[0] = (unsigned char)op;
		count = 1;
	}
	if (g->cb.stm.write(&g->cb.stm, g->stm.tmp, count, (char *)t) == 0) {
		g->err.code = cfwErrTmpStream;
	}
}

/* Save fixed-point in tmp file. */
static void tmp_savefixed(cfwCtx g, Fixed f) {
	unsigned char t[5];
	size_t count;
	if (f & 0xffff) {
		t[0] = (unsigned char)255;
		t[1] = (unsigned char)(f >> 24);
		t[2] = (unsigned char)(f >> 16);
		t[3] = (unsigned char)(f >> 8);
		t[4] = (unsigned char)f;
		count = 5;
	}
	else {
		count = cfwEncInt(f >> 16, t);
	}
	if (g->cb.stm.write(&g->cb.stm, g->stm.tmp, count, (char *)t) == 0) {
		g->err.code = cfwErrTmpStream;
	}
}

/* This function converts a real number to a fixed point number. However, the
   precision of the conversion is restricted to 2 decimal digits of fraction.
   This permits round-tripping of fractional numbers in Type 1 charstrings
   that were implemented by division by 10 or 100. */
static Fixed float2Fixed(float r) {
	float half = (r < 0) ? -0.5f : 0.5f;
	Fixed x = (Fixed)(r * 100 + half);
	return (x / 100) * 65536 + (Fixed)((x % 100) * 655.36f + half);
}

/* Save stem args and ops observing stack limit and possible width arg. The
   arithmetic is performed in fixed point notation so that cumulative rounding
   errors in the delta stem list can be eliminated. */
static void saveStemOp(cstrCtx h, int iBeg, int iEnd, int op, int optimize) {
    unsigned short MAX_STEMS = (h->maxstack - 1) / 2;
	cfwCtx g = h->g;
	int i = iBeg;
	int stems = iEnd - iBeg;
	int ops = (stems + MAX_STEMS - 1) / MAX_STEMS;
	stems = stems - (ops - 1) * MAX_STEMS;
	if (g->flags & CFW_ENABLE_CMP_TEST) {
		/* For testing; produces result compatible with typecomp CFF */
        while (ops--) {
            Fixed last = 0;
            while (stems--) {
                Stem *stem = &h->stems.array[i++];
                Fixed edge0 = float2Fixed(stem->edge0);
                Fixed edge1 = float2Fixed(stem->edge1);
                tmp_savefixed(g, edge0 - last);
                tmp_savefixed(g, edge1 - edge0);
                last = edge1;
            }
            if (ops > 0 || !optimize) {
                tmp_saveop(g, op);
            }
            stems = MAX_STEMS;
		}
	}
	else {
		/* For production; produces "correct" result */
		while (ops--) {
           if (g->flags & CFW_WRITE_CFF2)
            {
                float lastf = 0;
                /* First, write the default values */
                while (stems--) {
                    Stem *stem = &h->stems.array[i++];
                    pushStemBlends(h, &stem->edge0v);
                    pushStemBlends(h, &stem->edge1v);
                    tmp_savefixed(g, float2Fixed(stem->edge0 - lastf));
                    tmp_savefixed(g, float2Fixed(stem->edge1 - stem->edge0));
                    lastf = stem->edge1;
                }
               if (h->numBlends > 0)
                    flushStemBlends(h);
            }
            else
            {
                Fixed last = 0;
                while (stems--) {
                    Stem *stem = &h->stems.array[i++];
                    tmp_savefixed(g, float2Fixed(stem->edge0 - last));
                    tmp_savefixed(g, float2Fixed(stem->edge1 - stem->edge0));
                    last = stem->edge1;
                }
            }
			if (ops > 0 || !optimize) {
				tmp_saveop(g, op);
			}
			stems = MAX_STEMS;
		}
	}
}

/* Test for hint overlap */
static void checkOverlap(cstrCtx h, HintMask hintmask) {
	int i;
	Stem *last;

	if (h->g->stm.dbg == NULL) {
		return; /* Debug stream deactivated */
	}
	/* Find bits set and compare corresponding stems */
	last = NULL;
	for (i = 0; i < h->masksize; i++) {
		int j;
		unsigned char byte = hintmask[i];
		for (j = i * 8; byte != 0; j++) {
			if (byte & 0x80) {
				Stem *curr = &h->stems.array[j];
				if (last != NULL &&
				    ((last->flags ^ curr->flags) & STEM_VERT) == 0 &&
				    last->edge1 >= curr->edge0) {
					addWarn(h, warn_hint4);
				}
				last = curr;
			}
			byte <<= 1;
		}
	}
}

/* Save hint/cntrmask op in tmp stream. */
static void saveHintMaskOp(cstrCtx h, unsigned char *map,
                           int idsize, char *idmask, int op) {
	cfwCtx g = h->g;
	int i;
	char hintop[MAX_MASK_BYTES + 2];    /* One extra length byte consumed by subroutinizer */
	char *hintmask = &hintop[1];

	hintop[0] = (char)op;

	/* Add length byte for subroutinizer */
	if (g->flags & CFW_SUBRIZE) {
		*hintmask++ = (char)h->masksize;
	}

	/* Construct hintmask from stem ids */
	memset(hintmask, 0, h->masksize);
	for (i = 0; i < idsize * 8; i++) {
		if (TEST_ID_BIT(idmask, i)) {
			SET_HINT_BIT(hintmask, map[i]);
		}
	}

	if (op == t2_hintmask) {
		if (memcmp(h->lastmask, hintmask, h->masksize) == 0) {
			if (!(g->flags & CFW_ENABLE_CMP_TEST)) {
				/* This hintmask is a duplicate of the initial mask or is a
				   duplicate of the last hintmask. Normally we can remove it
				   without causing rendering differences between the source
				   font and the converted font, but if all the hintmasks are
				   duplicates of the initial mask the rendering behavior can
				   change because this causes fixupmap code to be executed by
				   the render, hence the above test. */
				addWarn(h, warn_hint1);
				return;
			}
		}
		memcpy(h->lastmask, hintmask, h->masksize);
	}

	/* Save mask */
	if (g->flags & CFW_SUBRIZE) {
		/* Save mask op size in second byte for subroutinizer */
		char sizebyte = (char)(h->masksize + 2);
		if ((g->cb.stm.write(&g->cb.stm, g->stm.tmp, 1, (char *)hintop) == 0)
		    || (g->cb.stm.write(&g->cb.stm, g->stm.tmp, 1, &sizebyte) == 0)
		    || (g->cb.stm.write(&g->cb.stm, g->stm.tmp, h->masksize, hintmask) == 0)) {
			g->err.code = cfwErrTmpStream;
		}
	}
	else {
		if (g->cb.stm.write(&g->cb.stm, g->stm.tmp,
		                    1 + h->masksize, (char *)hintop) == 0) {
			g->err.code = cfwErrTmpStream;
		}
	}

	/* Counter masks (counter control list) aren't checked because it yields
	   too many warnings due to stem merging and integerization of list */
	if (op == t2_hintmask) {
		checkOverlap(h, hintmask);
	}
}

/* Save stem hints. */
static void saveStems(cstrCtx h, unsigned char *map) {
	cfwCtx g = h->g;
	int i;
	int ivstem;     /* Index of first vertical stem */
	int hasinit;    /* Charstring has initial mask? */
	int hasmask;    /* Charstring contains a hintmask? */
	int maskfirst;  /* Maskop first? */
	char *last;

	if (h->stems.cnt == 0) {
		if (h->flags & SEEN_MOVETO) {
			/* Marking glyph has no stem hints */
			addWarn(h, warn_hint2);
		}
		return;
	}

	/* Set final hintmask size */
	h->masksize = (h->stems.cnt + 7) / 8;

	/* Initialize lastmask to all stems */
	memset(h->lastmask, 0xff, h->masksize - 1);
	h->lastmask[h->masksize - 1] = 0xff >> (8 - h->stems.cnt % 8) % 8;

	/* Determine if charstring has initial mask */
	hasinit = memcmp(h->lastmask, h->initmask, h->masksize) != 0;

	/* Check for all duplicate hintmasks */
	last = h->initmask;
	for (i = 0; i < h->hints.cnt; i++) {
		Hint *hint = &h->hints.array[i];
		char *curr = &h->masks.array[hint->iMask];
		if (hint->size != h->masksize || memcmp(last, curr, h->masksize) != 0) {
			maskfirst = hint->iCstr == 0;
			hasmask = 1;
			goto check_opt;
		}
		last = curr;
	}

	if (!hasinit) {
		if (h->hints.cnt > 0) {
			/* All hintsubs removed! */
			addWarn(h, warn_hint5);
		}
	}
	else {
		/* Unused stems */
		addWarn(h, warn_hint7);
	}

	maskfirst = 0;
	hasmask = 0;

	/* Apply ROM optimization */
check_opt:
	if (maskfirst && (g->flags & CFW_ROM_OPT)) {
		hasinit = 0;
	}

	if (hasinit || h->cntrs.cnt > 0) {
		maskfirst = 1;  /* The charstring will begin with a hint/cntrmask */
	}
	/* Will a hintmask exist in the charstring? */
	hasmask |= hasinit;

	/* Set vstem index */
	ivstem = h->stems.cnt;
	for (i = 0; i < h->stems.cnt; i++) {
		if (h->stems.array[i].flags & STEM_VERT) {
			ivstem = i;
			break;
		}
	}

	if (ivstem > 0) {
		/* Save hstems */
		saveStemOp(h, 0, ivstem, hasmask ? t2_hstemhm : tx_hstem, 0);
	}

	if (ivstem < h->stems.cnt) {
		/* Save vstems */
		saveStemOp(h, ivstem, h->stems.cnt, hasmask ? t2_vstemhm : tx_vstem,
		           maskfirst);
	}

	/* Construct stem id to stem order mapping */
	for (i = 0; i < h->stems.cnt; i++) {
		map[h->stems.array[i].id] = (unsigned char)i;
	}

	/* Save counter masks */
	for (i = 0; i < h->cntrs.cnt; i++) {
		Cntr *cntr = &h->cntrs.array[i];
		saveHintMaskOp(h, map,
		               cntr->size, &h->masks.array[cntr->iMask], t2_cntrmask);
	}

	/* Initialize lastmask to all stems */
	memset(h->lastmask, 0xff, h->masksize - 1);
	h->lastmask[h->masksize - 1] = 0xff << (8 - h->stems.cnt % 8) % 8;

	if (hasinit) {
		/* Save initial hints */
		saveHintMaskOp(h, map, h->masksize, h->initmask, t2_hintmask);
	}
	else {
		/* No initial hints so check all hints for overlap */
		checkOverlap(h, h->lastmask);
	}
}

/* Convert warning number to string. */
static char *warnMsg(int warning) {
	switch (warning) {
		case warn_move0:
			return "no initial moveto (inserted)";

		case warn_move1:
			return "moveto preceeds closepath (discarded)";

		case warn_move2:
			return "moveto sequence (collapsed)";

		case warn_hint0:
			return "negative hint (reversed)";

		case warn_hint1:
			return "duplicate hintsubs (discarded)";

		case warn_hint2:
			return "unhinted";

		case warn_hint3:
			return "stem list overflow (discarded)";

		case warn_hint4:
			return "hint overlap";

		case warn_hint5:
			return "all hintsubs removed (fixupmap enabled)";

		case warn_hint6:
			return "consecutive hintsubs (discarded)";

		case warn_hint7:
			return "unused hints";

		case warn_flex0:
			return "non-perpendicular flex";

		case warn_flex1:
			return "suspect flex args";

		case warn_dup0:
			return "glyph skipped - duplicate of glyph in font";

		case warn_dup1:
			return "glyph skipped - same name, different charstring as glyph in font";

		default:
			return "unknown warning!";
	}
}

/* Print charstring warning. */
static void printWarn(cstrCtx h) {
	cfwCtx g = h->g;
	int i;

	if (g->stm.dbg == NULL) {
		return;
	}

	for (i = 0; i < warn_cnt; i++) {
		if (h->warning[i] > 0 &&
		    ((g->flags & CFW_WARN_DUP_HINTSUBS) || i != warn_hint1)) {
			char *msg = warnMsg(i);

			if (h->warning[i] > 1) {
                if (h->warning[i] > TC_MAX_WARNINGS)
                    continue;
			}

			if (h->glyph.info->flags & ABF_GLYPH_CID) {
				cfwMessage(h->g,
				           "%s <cid-%hu>", msg, h->glyph.info->cid);
			}
			else {
				cfwMessage(h->g, "%s <%s>",
				           msg, h->glyph.info->gname.ptr);
			}
		}
	}
}

/* Print charstring warning. */
void printFinalWarn(cfwCtx g) {
    cstrCtx h = g->ctx.cstr;
    int i;
    
    if (g->stm.dbg == NULL) {
        return;
    }
    
    for (i = 0; i < warn_cnt; i++) {
        if (h->warning[i] > TC_MAX_WARNINGS) {
            char *msg = warnMsg(i);
            cfwMessage(h->g, "There are %d additional reports of '%s'.\n", h->warning[i] - TC_MAX_WARNINGS, msg);
        }
        h->warning[i] = 0;
    }
}


/* End glyph definition. */
static void glyphEnd(abfGlyphCallbacks *cb) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	int i;
	long iCstr;
	unsigned char map[T2_MAX_STEMS];
	long cstroff = h->tmpoff;
    int doOptimize = !(g->flags & CFW_NO_OPTIMIZATION);

	switch (h->pendop) {
		case tx_vmoveto:
		case tx_hmoveto:
		case tx_rmoveto:
			/* Path finished with moveto; discard it */
			addWarn(h, warn_move1);

		/* Fall through */
		case tx_dotsection:
			/* Discard dotsection before closepath */
			clearop(h);
			break;

		case tx_noop:
			break;

		default:
			saveop(h, h->pendop);
			break;
	}

    if ((!doOptimize) && (h->flags & SEEN_MOVETO)) /* add closing line-to, if last op is not the same as the starting moveto. */
    {
        if ((h->x != h->start_x) || (h->y != h->start_y))
        {
            float dx0;
            float dy0;
            dx0 = h->start_x - h->x;
            dy0 = h->start_y - h->y;
            PUSH(dx0);
            PUSH(dy0);
            h->pendop = tx_rlineto;
            saveop(h, h->pendop);
            h->x = h->start_x;
            h->y = h->start_y;
        }
    }

    
    if (g->flags & (CFW_IS_CUBE | CFW_WRITE_CFF2)) {
		clearop(h);
	}
	else {
		saveop(h, tx_endchar);
	}

	/* Save last hint and counter */
	newHint(h);
	newCntr(h);

	/* Save stems, counters, and initial hintmask at head of charstring */
	saveStems(h, map);

	if (h->glyph.info->flags & ABF_GLYPH_CUBE_GSUBR) {
		cfwAddCubeGSUBR(g, h->cstr.array,  h->cstr.cnt);
		return;
	}
	/* Write rest of charstring */
	iCstr = 0;
	for (i = 0; i < h->hints.cnt; i++) {
		Hint *hint = &h->hints.array[i];
		size_t count;

		/* Write charstring segment */
		count = hint->iCstr - iCstr;
		if (g->cb.stm.write(&g->cb.stm, g->stm.tmp,
		                    count, &h->cstr.array[iCstr]) != count) {
			g->err.code = cfwErrTmpStream;
		}

		/* Save hintmask */
		saveHintMaskOp(h, map,
		               hint->size, &h->masks.array[hint->iMask], t2_hintmask);
		iCstr = hint->iCstr;
	}

	if (g->flags & CFW_SUBRIZE) {
		/* Add unique separator to charstring for subroutinizer */
		cstr_saveop(h, t2_separator);
		cstr_saveop(h, (h->unique >> 16) & 0xff);
		cstr_saveop(h, (h->unique >> 8) & 0xff);
		cstr_saveop(h, (h->unique++) & 0xff);
	}

	/* Write last segment */
	if ((h->cstr.cnt - iCstr) > 0) {
		if (g->cb.stm.write(&g->cb.stm, g->stm.tmp,
		                    h->cstr.cnt - iCstr, &h->cstr.array[iCstr]) == 0) {
			g->err.code = cfwErrTmpStream;
		}
	}

	h->tmpoff = g->cb.stm.tell(&g->cb.stm, g->stm.tmp);
	if (h->tmpoff == -1) {
		g->err.code = cfwErrTmpStream;
	}

	{
		/* Check if new glyph is same as old, if merging fonts */
		long seen_index = 0;
		int errorCode = 0;
		if (g->flags & CFW_CHECK_IF_GLYPHS_DIFFER) {
			/* check and see if glyph has been already seen */
			seen_index = cfwSeenGlyph(g, h->glyph.info, &errorCode, cstroff, h->tmpoff);
			if (errorCode) {
				/* set the cfwCtx error code.*/
				g->err.code |= errorCode;
				if (errorCode == cfwErrGlyphPresent) {
					addWarn(h, warn_dup0);
				}
				else if (errorCode == cfwErrGlyphDiffers) {
					addWarn(h, warn_dup1);
				}

				/* rewind temp stream to start of last glyph */
				g->cb.stm.seek(&g->cb.stm, g->stm.tmp, cstroff);
				h->tmpoff = g->cb.stm.tell(&g->cb.stm, g->stm.tmp);
			}
		}
		if (errorCode == 0) {
			cfwAddGlyph(g, h->glyph.info, h->glyph.hAdv, h->tmpoff - cstroff, cstroff, seen_index);
		}
	}

	if (h->flags & SEEN_WARN) {
		printWarn(h);
	}
}

static void glyphCubeBlend(abfGlyphCallbacks *cb, unsigned int nBlends, unsigned int numVals, float *blendVals) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	unsigned int i = 0;

	if (h->pendop != tx_noop) {
		saveop(h, h->pendop);
	}

	while (i < numVals) {
		PUSH(blendVals[i]);
	}
	switch (nBlends) {
		case 1:
			saveop(h, tx_BLEND1);
			break;

		case 2:
			saveop(h, tx_BLEND2);
			break;

		case 3:
			saveop(h, tx_BLEND3);
			break;

		case 4:
			saveop(h, tx_BLEND4);
			break;

		case 6:
			saveop(h, tx_BLEND6);
			break;
	}
	h->pendop = tx_noop;
}

static void glyphCubeSetWV(abfGlyphCallbacks *cb,  unsigned int numDV) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

	if (h->pendop != tx_noop) {
		saveop(h, h->pendop);
	}

	switch (numDV) {
		case 1:
			saveop(h, tx_SETWV1);
			break;

		case 2:
			saveop(h, tx_SETWV2);
			break;

		case 3:
			saveop(h, tx_SETWV3);
			break;

		case 4:
			saveop(h, tx_SETWV4);
			break;

		case 5:
			saveop(h, tx_SETWV5);
			break;

		default:
			saveop(h, tx_SETWVN);
	}
	h->pendop = tx_noop;
}

static void glyphCubeCompose(abfGlyphCallbacks *cb,  int cubeLEIndex, float x0, float y0, int numDV, float *ndv) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;
	int i = 0;
	float dx0;
	float dy0;

	if (!(h->flags & SEEN_MOVETO)) {
		glyphMove(cb, 0, 0); // same as insertMoce(), but without the warning.
	}
	switch (h->pendop) {
		case tx_compose:
			flushop(h, 3 + numDV); /* make sure we don't exceed the stack depth */
			break;

		case tx_noop:
			break;

		default:
			saveop(h, h->pendop);
			break;
	}


	PUSH(cubeLEIndex); /* library element value */
	/* Compute delta and update current point */
	dx0 = x0 - h->x;
	dy0 = y0 - h->y;
	h->x = x0;
	h->y = y0;
	if (g->flags & CFW_CUBE_RND) {
		PUSH(dx0 / 4);
		PUSH(dy0 / 4);
	}
	else {
		PUSH(dx0);
		PUSH(dy0);
	}

	while (i < numDV) {
		PUSH(ndv[i++]);
	}
	h->pendop = tx_compose;
}

static void cubeTransform(abfGlyphCallbacks *cb,  float rotate, float scaleX, float scaleY, float skewX, float skewY) {
	cfwCtx g = (cfwCtx)cb->direct_ctx;
	cstrCtx h = g->ctx.cstr;

	switch (h->pendop) {
		case tx_noop:
			break;

		default:
			saveop(h, h->pendop);
			break;
	}

	/* Rotation is expressed in degrees. Rotation is *100,
	   and the scale/skew are *1000 to avoid the use of the div
	   oeprator in the charstring.  */
	PUSH(rotate * 100);
	PUSH(scaleX * 1000);
	PUSH(scaleY * 1000);
	PUSH(skewX * 1000);
	PUSH(skewY * 1000);
	h->pendop = tx_transform; // This is not a typo. The idea is to allow chaining a transform op and a compose op.
}

/* Public callback set */
const abfGlyphCallbacks cfwGlyphCallbacks = {
	NULL,
	NULL,
	0,
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
	glyphCubeBlend,
	glyphCubeSetWV,
	glyphCubeCompose,
	cubeTransform,
    glyphMoveVF,
    glyphLineVF,
    glyphCurveVF,
    glyphStemVF,
};

/* ----------------------------- Debug Support ----------------------------- */

#if CFW_DEBUG

/* Debug mask. */
static void dbmask(int size, char *mask) {
	if (size == 0) {
		printf("empty");
	}
	else {
		int j;
		char *sep = "";
		for (j = 0; j < size * 8; j++) {
			if (TEST_ID_BIT(mask, j)) {
				printf("%s%d", sep, j);
				sep = "+";
			}
		}
	}
}

/* Debug hints. */
static void dbhints(cstrCtx h) {
	long i;
	printf("--- stems[index]={edge0,edge1,id,flags}\n");
	if (h->stems.cnt == 0) {
		printf("empty\n");
	}
	else {
		long i;
		for (i = 0; i < h->stems.cnt; i++) {
			Stem *stem = &h->stems.array[i];
			printf("[%ld]={%g,%g,%u,%c}\n", i,
			       stem->edge0, stem->edge1, stem->id,
			       (stem->flags & STEM_VERT) ? 'v' : 'h');
		}
	}
	printf("--- initial mask\n");
	dbmask((h->stems.cnt + 7) / 8, h->initmask);
	printf("\n");
	printf("--- hints[index]={iCstr,iMask,size}\n");
	if (h->hints.cnt == 0) {
		printf("empty\n");
	}
	else {
		for (i = 0; i < h->hints.cnt; i++) {
			Hint *hint = &h->hints.array[i];
			printf("[%ld]={%ld,", i, hint->iCstr);
			dbmask(hint->size, &h->masks.array[hint->iMask]);
			printf(",%ld}\n", hint->size);
		}
	}
	printf("--- cntrs[index]={iMask,size}\n");
	if (h->cntrs.cnt == 0) {
		printf("empty\n");
	}
	else {
		for (i = 0; i < h->cntrs.cnt; i++) {
			Cntr *cntr = &h->cntrs.array[i];
			printf("[%ld]={", i);
			dbmask(cntr->size, &h->masks.array[cntr->iMask]);
			printf(",%ld}\n", cntr->size);
		}
	}
}

/* Debug stack. */
static void dbstack(cstrCtx h) {
	printf("--- stack[index]=value\n");
	if (h->stack.cnt == 0) {
		printf("empty\n");
	}
	else {
		int i;
		for (i = 0; i < h->stack.cnt; i++) {
			printf("[%d]=%g ", i, h->stack.array[i]);
		}
		printf("\n");
	}
}

/* Debug Type 2 charstring. */
static void dbt2cstr(long length, unsigned char *cstr) {
	static char *opname[32] = {
		/*  0 */ "reserved0",
		/*  1 */ "hstem",
		/*  2 */ "compose",
		/*  3 */ "vstem",
		/*  4 */ "vmoveto",
		/*  5 */ "rlineto",
		/*  6 */ "hlineto",
		/*  7 */ "vlineto",
		/*  8 */ "rrcurveto",
		/*  9 */ "reserved9",
		/* 10 */ "callsubr",
		/* 11 */ "return",
		/* 12 */ "escape",
		/* 13 */ "reserved13",
		/* 14 */ "endchar",
		/* 15 */ "vsindex",
		/* 16 */ "blend",
		/* 17 */ "callgrel",
		/* 18 */ "hstemhm",
		/* 19 */ "hintmask",
		/* 20 */ "cntrmask",
		/* 21 */ "rmoveto",
		/* 22 */ "hmoveto",
		/* 23 */ "vstemhm",
		/* 24 */ "rcurveline",
		/* 25 */ "rlinecurve",
		/* 26 */ "vvcurveto",
		/* 27 */ "hhcurveto",
		/* 28 */ "shortint",
		/* 29 */ "callgsubr",
		/* 30 */ "vhcurveto",
		/* 31 */ "hvcurveto",
	};
	static char *escopname[] = {
		/*  0 */ "dotsection",
		/*  1 */ "reservedESC1",
		/*  2 */ "reservedESC2",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "reservedESC6",
		/*  7 */ "reservedESC7",
		/*  8 */ "reservedESC8",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "reservedESC13",
		/* 14 */ "neg",
		/* 15 */ "eq",
		/* 16 */ "reservedESC16",
		/* 17 */ "reservedESC17",
		/* 18 */ "drop",
		/* 19 */ "reservedESC19",
		/* 20 */ "put",
		/* 21 */ "get",
		/* 22 */ "ifelse",
		/* 23 */ "random",
		/* 24 */ "mul",
		/* 25 */ "reservedESC25",
		/* 26 */ "sqrt",
		/* 27 */ "dup",
		/* 28 */ "exch",
		/* 29 */ "index",
		/* 30 */ "roll",
		/* 31 */ "reservedESC31",
		/* 32 */ "reservedESC32",
		/* 33 */ "reservedESC33",
		/* 34 */ "hflex",
		/* 35 */ "flex",
		/* 36 */ "hflex1",
		/* 37 */ "flex1",
		/* 38 */ "cntron",

		/* 39 */ "blend1",
		/* 40 */ "blend2",
		/* 41 */ "blend3",
		/* 42 */ "blend4",
		/* 43 */ "blend6",
		/* 44 */ "setwv1",
		/* 45 */ "setwv2",
		/* 46 */ "setwv3",
		/* 47 */ "setwv4",
		/* 48 */ "setwv5",
		/* 49 */ "setwvn",
		/* 50 */ "transform",
	};
	int single = 0; /* Suppress optimizer warning */
	int stems = 0;
	int args = 0;
	long i = 0;

	while (i < length) {
		int op = cstr[i];
		switch (op) {
			case tx_endchar:
				printf("%s ", opname[op]);
				i++;
				break;

			case tx_reserved0:
			case tx_compose:
			case tx_vmoveto:
			case tx_rlineto:
			case tx_hlineto:
			case tx_vlineto:
			case tx_rrcurveto:
			case t2_reserved9:
			case tx_callsubr:
			case tx_return:
			case t2_reserved13:
			case t2_vsindex:
			case t2_blend:
			case tx_callgrel:
			case tx_rmoveto:
			case tx_hmoveto:
			case t2_rcurveline:
			case t2_rlinecurve:
			case t2_vvcurveto:
			case t2_hhcurveto:
			case t2_callgsubr:
			case tx_vhcurveto:
			case tx_hvcurveto:
				printf("%s ", opname[op]);
				args = 0;
				i++;
				break;

			case tx_hstem:
			case tx_vstem:
			case t2_hstemhm:
			case t2_vstemhm:
				printf("%s ", opname[op]);
				stems += args / 2;
				args = 0;
				i++;
				break;

			case t2_hintmask:
			case t2_cntrmask:
			{
				int bytes;
				if (args > 0) {
					stems += args / 2;
				}
				bytes = (stems + 7) / 8;
				printf("%s+", opname[op]);
				i++;
				while (bytes--) {
					printf("%02X", cstr[i++]);
				}
				printf(" ");
				args = 0;
				break;
			}

			case tx_escape:
			{
				/* Process escaped operator */
				int escop = cstr[i + 1];
				if (escop > (int)ARRAY_LEN(escopname) - 1) {
					printf("? ");
				}
				else {
					printf("%s ", escopname[escop]);
				}
				args = 0;
				i += 2;
				break;
			}

			case t2_shortint:
				printf("%d ", cstr[i + 1] << 8 | cstr[i + 2]);
				args++;
				i += 3;
				break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				printf("%d ", 108 + 256 * (cstr[i] - 247) + cstr[i + 1]);
				args++;
				i += 2;
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				printf("%d ", -108 - 256 * (cstr[i] - 251) - cstr[i + 1]);
				i += 2;
				args++;
				break;

			case 255:
			{
				/* 5 byte number */
				long value = (long)cstr[i + 1] << 24 | (long)cstr[i + 2] << 16 |
				    (long)cstr[i + 3] << 8 | (long)cstr[i + 4];
				printf("%g ", value / 65536.0);
				args++;
				i += 5;
				break;
			}

			default:
				/* 1 byte number */
				single = cstr[i] - 139;
				printf("%d ", single);
				args++;
				i++;
				break;
		}
	}
	printf("\n");
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
	dbuse(0, dbhints, dbstack, dbt2cstr);
}

#endif /* CFW_DEBUG */
