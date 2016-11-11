/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type 2 charstring support.
 */
#ifdef _WIN32
#define _USE_MATH_DEFINES /* Needed to define M_PI under Windows */
#endif

#include "t2cstr.h"
#include "txops.h"
#include "ctutil.h"

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>

#if PLAT_SUN4
#include "sun4/fixstdlib.h"
#else /* PLAT_SUN4 */
#include <stdlib.h>
#endif /* PLAT_SUN4 */

#include <errno.h>


/* Make uzoperator for internal use */
#define t2_cntroff	t2_reservedESC33

enum							/* seac operator conversion phase */
	{
	seacNone,					/* No seac conversion */
	seacBase,					/* In base char */
	seacAccentPreMove,			/* In accent char before first moveto */
	seacAccentPostMove			/* In accent char after first moveto */
	};

typedef struct					/* Stem data */
	{
	float edge0;
	float edge1;
	short flags;				/* Uses stem flags defined in absfont.h */
	} Stem;

/* Module context */
typedef struct t2cCtx *t2cCtx;
struct t2cCtx
	{
	long flags;					/* Control flags */
#define PEND_WIDTH		(1<<0)	/* Flags width pending */
#define PEND_MASK		(1<<1)	/* Flags hintmask pending */
#define SEEN_ENDCHAR	(1<<2)	/* Flags endchar operator seen */
#define START_COMPOSE  	(1<<3)	/* currently flattening a Cube library element: need to add the compose (dx, dy) to the LE moveto */
#define FLATTEN_CUBE	(1<<4)  /* when process a compose operator, flatten it rather than report it. */
#define NEW_HINTS		(1<<5)  /* Needed for cube gsubrs that report simple simple and vstems, Type1 style. */
#define IS_CUBE			(1<<6) /* Current font is a cube font. Allow no endchar/return ops, increase stack sizes. */
#define CUBE_RND		(1<<7) /* start (x,y) for a Cube element is rounded to a multiple of 4: used when building real Cube host fonts. Required to be rasterized by PFR. */
#define SEEN_BLEND		(1<<8) /* Seen blend operator: need to round position after each op. */
#define USE_MATRIX		(1<<9)	/* Apply transform*/
#define USE_GLOBAL_MATRIX (1<<10)	/* Transform is applied to entire charstring */
#define START_PATH_MATRIX	 (1<<11) /* Transform has just been defined, and shoould be applied to the foloowing path (from next move-to tup o the following move-to). */
#define FLATTEN_COMPOSE (1<<12)	/* Flag that we are flattening a compose operation. Used to rest the USE_MATRIX flag when done. */
#define FLATTEN_BLEND (1<<13)	/* Flag that we are flattening a CFF2 charstring. */
#define SEEN_CFF2_BLEND (1<<14)	/* Flag that we are flattening a CFF2 charstring. */
	struct						/* Operand stack */
		{
		long cnt;
		float array[TX_MAX_OP_STACK_CUBE];
        unsigned short numRegions;
        long blendCnt;
        abfOpEntry blendArray[TX_MAX_OP_STACK_CUBE];
		} stack;
	long maxOpStack;
	float BCA[TX_BCA_LENGTH];	/* BuildCharArray */
	float x;					/* Path x-coord */
	float y;					/* Path y-coord */
	int subrDepth;
	int cubeStackDepth;
    float transformMatrix[6];
	struct						/* Stem hints */
		{
		float start_x;			/* Path x-coord at start of Cube library element processing */
		float start_y;			/* Path y-coord at start of Cube library element processing */
		float offset_x;		/* cube offset, to add to first moveto in cube library element (LE) */
		float offset_y;		/* cube offset, to add to first moveto in cube library element (LE)  */
		int nMasters;
		int leIndex;
		int composeOpCnt;
		float composeOpArray[TX_MAX_OP_STACK_CUBE];
        double WV[kMaxCubeMasters]; /* Was originally just 4, to support substitution MM fonts. Note: the PFR rasterizer can support only up to 5 axes */
		} cube[CUBE_LE_STACKDEPTH];
	struct						/* Stem hints */
		{
		long cnt;
		Stem array[T2_MAX_STEMS];
		} stems;
	struct						/* hint/cntrmask */
		{
		short state;			/* cntrmask state */
		short length;			/* Number of bytes in mask op */
		short unused;			/* Mask unused bits in last byte of mask */
		unsigned char bytes[T2_MAX_STEMS/8];	/* Current mask */
		} mask;
	struct						/* seac conversion data */
		{
		float adx;
		float ady;
		int phase;
		} seac;
	struct						/* Source data */
		{
		char *buf;				/* Buffer */
		long length;			/* Buffer length */
		long offset;			/* offset in file */
		long endOffset;			/* offset in file fo end of charstring */
		long left;				/* Bytes remaining in charstring */
		} src;
	short LanguageGroup;
	t2cAuxData *aux;			/* Auxiliary parse data */
    abfGlyphCallbacks *glyph;	/* Glyph callbacks */
    ctlMemoryCallbacks *mem;	/* Glyph callbacks */
	};

/* Check stack contains at least n elements. */
#define CHKUFLOW(n) \
	do{if(h->stack.cnt<(n))return t2cErrStackUnderflow;}while(0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n) \
	do{if((h->stack.cnt)+(n)>h->maxOpStack)return t2cErrStackOverflow;}while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define INDEX_BLEND(i) (h->stack.blendArray[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) {if (h->aux->flags & T2C_IS_CFF2) h->stack.blendArray[h->stack.blendCnt++].value =(float)(v);  h->stack.array[h->stack.cnt++]=(float)(v);}

#define ABS(v)	(((v)<0)?-(v):(v))
#define ARRAY_LEN(a)	(sizeof(a)/sizeof(a[0]))

/* Transform coordinates by matrix. */
#define RND(v)	((float)floor((v)+0.5))
#define TX(x, y) \
RND(h->transformMatrix[0]*(x) + h->transformMatrix[2]*(y) + h->transformMatrix[4])
#define TY(x, y) \
RND(h->transformMatrix[1]*(x) + h->transformMatrix[3]*(y) + h->transformMatrix[5])

/* Scale coordinates by matrix. */
#define SX(x)	RND(h->transformMatrix[0]*(x))
#define SY(y)	RND(h->transformMatrix[3]*(y))

static int t2Decode(t2cCtx h, long offset);

/* ------------------------------- Error handling -------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(t2cCtx h, char *fmt, va_list ap)
	{
	char text[500];

	if (h->aux->dbg == NULL)
		return;	/* Debug stream not available */

	vsprintf(text, fmt, ap);
	(void)h->aux->stm->write(h->aux->stm, h->aux->dbg, strlen(text), text);
	}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(t2cCtx h, char *fmt, ...)
	{
	va_list ap;
	va_start(ap, fmt);
	vmessage(h, fmt, ap);
	va_end(ap);
	}
/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(t2cCtx h, size_t size)
{
    void *ptr = h->mem->manage(h->mem, NULL, size);
    return ptr;
}

/* Free memory. */
static void memFree(t2cCtx h, void *ptr)
{
    h->mem->manage(h->mem, ptr, 0);
}

/* ------------------------------- Data Input ------------------------------ */

/* Refill input buffer. Return NULL on stream error else pointer to buffer. */
static unsigned char *refill(t2cCtx h, unsigned char **end)
	{
	long int offset;
    char * errMsg;
	/* Read buffer */
	/* 64-bit warning fixed by cast here HO */
	h->src.length = (long) h->aux->stm->read(h->aux->stm, h->aux->src, &h->src.buf);
	if (h->src.length == 0)
    {
        errMsg = strerror(errno);
        printf("%s", errMsg);
		return NULL;	/* Stream error */
    }

	/* Update offset of next read */
	offset = h->src.offset + h->src.length;
	if (offset >= h->src.endOffset)
		{
		 h->src.length = h->src.endOffset - h->src.offset;
		 h->src.offset = h->src.offset + h->src.length;
		}
	else
		h->src.offset  = offset;

	/* Set buffer limit */
	*end = (unsigned char *)h->src.buf + h->src.length;

	return (unsigned char *)h->src.buf;
	}

/* Seek to offset on source stream. */
static int srcSeek(t2cCtx h, long offset)
	{
	if (h->aux->stm->seek(h->aux->stm, h->aux->src, offset))
		return 1;
	h->src.offset = offset;
	return 0;
	}

/* Check next byte available and refill buffer if not. */
/* only CUBE fonts have no endchar/return operators in teh charstring or gsubrs. */


#define CHKBYTE(h) \
	do \
		if (next == end) \
			{ \
			if (h->src.offset >= h->src.endOffset) \
				{\
                if (h->aux->flags & T2C_IS_CFF2) \
                    goto do_endchar;\
				if (!(h->aux->flags & T2C_IS_CUBE)) \
						return t2cErrSrcStream;\
				if ((h->subrDepth > 0) || (h->cubeStackDepth >= 0))\
					return t2cSuccess;\
				else\
					goto do_endchar;\
				}\
			next = refill(h, &end); \
			if (next == NULL) \
				  return t2cErrSrcStream; \
			} \
	while (0)

#define CHKSUBRBYTE(h) \
	do \
		if (next == end) \
			{ \
			if (h->src.offset >= h->src.endOffset) \
				{\
                if (h->aux->flags & T2C_IS_CFF2) \
                    goto do_subr_return;\
				if (!(h->aux->flags & T2C_IS_CUBE)) \
						return t2cErrSrcStream;\
				goto do_subr_return;\
				}\
			next = refill(h, &end); \
			if (next == NULL) \
				  return t2cErrSrcStream; \
			} \
	while (0)

/* ------------------------------- Callbacks ------------------------------- */

/* Compute and callback width. Return non-0 to end parse else 0. */
static int callbackWidth(t2cCtx h, int odd_args)
	{
	if ((h->flags & PEND_WIDTH) && (h->flags & ~T2C_IS_CFF2))
		{		
		float width = (odd_args == (h->stack.cnt & 1))?
			INDEX(0) + h->aux->nominalWidthX: h->aux->defaultWidthX;
		if (h->flags & USE_MATRIX)
			width = SX(width);
		else if (h->flags & SEEN_BLEND)
			width = RND(width);
		h->glyph->width(h->glyph, width);
		h->flags &= ~PEND_WIDTH;
		if (h->aux->flags & T2C_WIDTH_ONLY)
			{
			/* Only width required; pretend we've seen endchar operator */
			h->flags |= SEEN_ENDCHAR;
			return 1;
			}
		}
	return 0;
	}

/* Compute and callback width. Return non-0 to end parse else 0. */
static int callbacDfltkWidth(t2cCtx h)
	{
	if (h->flags & PEND_WIDTH)
		{		
		float width = h->aux->defaultWidthX;
		if (h->flags & USE_MATRIX)
			width = SX(width);
		else if (h->flags & SEEN_BLEND)
			width = RND(width);
			
		h->glyph->width(h->glyph, width);
		h->flags &= ~PEND_WIDTH;
		if (h->aux->flags & T2C_WIDTH_ONLY)
			{
			/* Only width required; pretend we've seen endchar operator */
			h->flags |= SEEN_ENDCHAR;
			return 1;
			}
		}
	return 0;
	}

/* Test if cntrmask contains valid stem3 counters. */
static int validStem3(t2cCtx h)
	{
	int i;
	int hcntrs = 0;
	int vcntrs = 0;

	/* Count counters */
	for (i = 0; i < h->mask.length; i++)
		{
		int j;
		unsigned char byte = h->mask.bytes[i];
		for (j = i*8; byte; j++)
			{
			if (byte & 0x80)
				{
				if (h->stems.array[j].flags & ABF_VERT_STEM)
					vcntrs++;
				else
					hcntrs++;
				}
			byte <<= 1;
			}
		}

	/* Do we have valid stem3s? */
	return ((hcntrs == 0 && vcntrs == 3) ||
			(hcntrs == 3 && vcntrs == 0) ||
			(hcntrs == 3 && vcntrs == 3));
	}

/* Callback operator and args. */
static void callbackOp(t2cCtx h, int op)
	{
	h->glyph->genop(h->glyph, h->stack.cnt, h->stack.array, op);
	}

/* Callback stems in mask. */
static void callbackStems(t2cCtx h, int cntr)
	{
	int i;
	int flags;

	if (cntr)
		{
		for (i = 0; i < h->mask.length; i++)
			if (h->mask.bytes[i] != 0)
				goto set_flags;

		/* Null cntrmask; turn off global coloring */
		callbackOp(h, t2_cntroff);
		return;

	set_flags:
		flags = ABF_NEW_GROUP;
		}
	else 
		flags = (h->flags & PEND_MASK)? 0: ABF_NEW_HINTS;

	for (i = 0; i < h->mask.length; i++)
		{
		int j;
		unsigned char byte = h->mask.bytes[i];
		for (j = i*8; byte; j++)
			{
			if (byte & 0x80)
				{
				/* Callback stem */
				Stem *stem = &h->stems.array[j];
				float edge0 = stem->edge0;
				float edge1 = stem->edge1;
				if (h->flags & USE_MATRIX)
					{
					if (stem->flags & ABF_VERT_STEM)
						{
						edge0 = SX(edge0);
						edge1 = SX(edge1);
						}
					else 
						{
						edge0 = SY(edge0);
						edge1 = SY(edge1);
						}
					}
				else if (h->flags & SEEN_BLEND)
					{
					edge0 = RND(edge0);
					edge1 = RND(edge1);
					}
					
				if (cntr)
					flags |= ABF_CNTR_STEM;
				flags |= stem->flags;
				h->glyph->stem(h->glyph, flags, edge0, edge1);
				flags = 0;
				}
			byte <<= 1;
			}
		}
	if (!cntr)
		h->flags &= ~PEND_MASK;
	}

/* Callback stems in mask. */
static void callbackCubeStem(t2cCtx h, int stemFlags)
	{
	int flags;
	float lastEdge = 0;
	float edge0;
	float edge1;
	int i;
	if (h->glyph->stem == NULL)
		return;
		
	flags = (h->flags & NEW_HINTS)? ABF_NEW_HINTS: 0;
	h->flags &= ~NEW_HINTS;
	
	flags |= stemFlags;
	for (i = h->stack.cnt & 1; i < h->stack.cnt - 1; i += 2)
		{
		edge0 = INDEX(i);
		edge1 = INDEX(i+1); /* is actually width */
		edge0 += lastEdge;
		edge1 += edge0;
		lastEdge = edge1;
		if (h->flags & USE_MATRIX)
			{
			if (stemFlags & ABF_VERT_STEM)
				{
				edge0 = SX(edge0);
				edge1 = SX(edge1);
				}
			else 
				{
				edge0 = SY(edge0);
				edge1 = SY(edge1);
				}
			}
		else if (h->flags & SEEN_BLEND)
			{
			edge0 = RND(edge0);
			edge1 = RND(edge1);
			}
		
		h->glyph->stem(h->glyph, flags, edge0, edge1);
		if (flags & ABF_NEW_HINTS)
			flags &= ~ABF_NEW_HINTS;
		}
	}


/* Determine how to handle first cntrmask and save it. */
static void savePendCntr(t2cCtx h, int cntr)
	{
	if (cntr || h->LanguageGroup == 1 || !validStem3(h))
		/* We have:
		   - 2 or more cntrmasks
		   - font sets LanguageGroup 1
		   - saw a cntron operator
		   - don't have valid stem3 cntrs;
		   do global coloring */
		callbackStems(h, 1);
	else
		{
		/* Do stem3; mark stem3 stems */
		int i;
		for (i = 0; i < h->mask.length; i++)
			{
			int j;
			unsigned char byte = h->mask.bytes[i];
			for (j = i*8; byte; j++)
				{
				if (byte & 0x80)
					h->stems.array[j].flags |= ABF_STEM3_STEM;
				byte <<= 1;
				}
			}
		}

	h->mask.state = 2;	/* Don't do this again */
	}

static void clearHintStack(t2cCtx h)
	{
	int flags;

	if (h->mask.state == 1)
		savePendCntr(h, 0);
	
	if (h->glyph->stem != NULL && (h->flags & PEND_MASK))
		{
		/* No mask before first move; callback initial hints */
		int i;
		for (i = 0; i < h->stems.cnt; i++)
			{
			/* Callback stem */
			Stem *stem = &h->stems.array[i];
			float edge0 = stem->edge0;
			float edge1 = stem->edge1;
			if (h->flags & USE_MATRIX)
				{
				if (stem->flags & ABF_VERT_STEM)
					{
					edge0 = SX(edge0);
					edge1 = SX(edge1);
					}
				else 
					{
					edge0 = SY(edge0);
					edge1 = SY(edge1);
					}
				}
			else if (h->flags & SEEN_BLEND)
				{
				edge0 = RND(edge0);
				edge1 = RND(edge1);
				}
				
			flags |= stem->flags;
			h->glyph->stem(h->glyph, flags, edge0, edge1);
			flags = 0;
			}
		h->flags &= ~PEND_MASK;
		}
	}
	
/* Callback path move. */
static void callbackMove(t2cCtx h, float dx, float dy)
	{
	int flags;
	float x; float y;


	if (h->flags & START_COMPOSE)
    {
        /* 	We can tell that this is the first move-to of a flattened compare operator
         with the  START_COMPOSE flag.
         dx and dy are the initial moveto values in the LE, usually 0 or a small value.
         h->x and h->y are the current absolute position of the last point in the last path.
         h->le_start.x,y are the LE absolute start position.
         */
		x = dx + h->cube[h->cubeStackDepth].offset_x;
		y = dy + h->cube[h->cubeStackDepth].offset_y;
		h->cube[h->cubeStackDepth].offset_x = 0;
		h->cube[h->cubeStackDepth].offset_y = 0;
        if (h->flags & START_PATH_MATRIX)
        {
            /* Set the tx and ty such that the start point remains the same. */
            float x2 = TX(x,y);
            float y2 = TY(x,y);
            // Path specific matrix scales and rotates around start point. Set tx and ty accordingly.
            h->transformMatrix[4] = x- x2; // tx
            h->transformMatrix[5] = y - y2;  //ty
            
            /* set USE_MATRIX. The transform op already set the new matrix values in h->transfromMatrix */
            h->flags |= USE_MATRIX;
            h->flags &= ~START_PATH_MATRIX;
            /* we will clear USE_MATRIX if START_PATH_MATRIX is set at the end of flattening the compose operator. */
        }
		h->flags &= ~START_COMPOSE;
    }
	else
	{
		x = h->x + dx;
		y = h->y + dy;
        if (!(h->flags & FLATTEN_COMPOSE))
        {
            /* We are starting a regular path, not flattening a compose operator.*/
            if (h->flags & START_PATH_MATRIX)
            {
                /* set USE_MATRIX. The transform op already set the new matrix values in h->transfromMatrix */
                h->flags |= USE_MATRIX;
                h->flags &= ~START_PATH_MATRIX;
            }
            else if (h->flags & USE_MATRIX)
            {
                /* if START_PATH_MATRIX is not set, then this either global, or left over from a previous path.  */
                if (h->flags & USE_GLOBAL_MATRIX)
                {
                    int i;
                    /* restore the global matrix values */
                    for (i = 0; i < 6; i++)
                    {
                        h->transformMatrix[i] = h->aux->matrix[i];
                    }
                }
                else
                {
                    /* If USE_GLOBAL_MATRIX is not set, then the   USE_MATRIX is left over from the previous path. Clear it. */
                    h->flags &= ~USE_MATRIX;
                    h->transformMatrix[0] =  h->transformMatrix[3] = 1.0;
                    h->transformMatrix[1] =  h->transformMatrix[2] = h->transformMatrix[4] = h->transformMatrix[5] = 1.0;
                }
                
                
            }
        }
	}

	
	if (h->mask.state == 1)
		savePendCntr(h, 0);
		
	h->flags |= NEW_HINTS;
	if (h->seac.phase == seacAccentPreMove)
		{
		/* Accent component moveto; if no mask force hintsubs */
		flags = ABF_NEW_HINTS;
		h->seac.phase = seacAccentPostMove;
		}
	else
		flags = 0;

	if (h->glyph->stem != NULL && (h->flags & PEND_MASK))
		{
		/* No mask before first move; callback initial hints */
		int i;
		for (i = 0; i < h->stems.cnt; i++)
			{
			/* Callback stem */
			Stem *stem = &h->stems.array[i];
			float edge0 = stem->edge0;
			float edge1 = stem->edge1;
			if (h->flags & USE_MATRIX)
				{
				if (stem->flags & ABF_VERT_STEM)
					{
					edge0 = SX(edge0);
					edge1 = SX(edge1);
					}
				else 
					{
					edge0 = SY(edge0);
					edge1 = SY(edge1);
					}
				}
			else if (h->flags & SEEN_BLEND)
				{
				edge0 = RND(edge0);
				edge1 = RND(edge1);
				}
			
			flags |= stem->flags;
			h->glyph->stem(h->glyph, flags, edge0, edge1);
			flags = 0;
			}
		h->flags &= ~PEND_MASK;
		}


    x = roundf(x*100)/100;
    y = roundf(y*100)/100;
    h->x = x;
    h->y = y;
	
	if (h->flags & USE_MATRIX)
		h->glyph->move(h->glyph, TX(h->x, h->y), TY(h->x, h->y));
	else if (h->flags & SEEN_BLEND)
		h->glyph->move(h->glyph, RND(h->x), RND(h->y));
	else
		h->glyph->move(h->glyph, h->x, h->y);
	}

/* Callback path line. */
static void callbackLine(t2cCtx h, float dx, float dy)
	{
	h->flags |= NEW_HINTS;

	if (h->flags & START_COMPOSE)
		{
		callbackMove(h, h->cube[h->cubeStackDepth].offset_x , h->cube[h->cubeStackDepth].offset_y);
		h->cube[h->cubeStackDepth].offset_x = 0;
		h->cube[h->cubeStackDepth].offset_y = 0;
		h->flags &= ~START_COMPOSE;
		}

	h->x += dx;	h->y += dy;
    h->x = roundf(h->x*100)/100;
    h->y = roundf(h->y*100)/100;
        
	if (h->flags & USE_MATRIX)
		h->glyph->line(h->glyph, TX(h->x, h->y), TY(h->x, h->y));
	else if (h->flags & SEEN_BLEND)
		h->glyph->line(h->glyph, RND(h->x), RND(h->y));
	else
		h->glyph->line(h->glyph, h->x, h->y);
	}

/* Callback path curve. */
static void callbackCurve(t2cCtx h,
						  float dx1, float dy1, 
						  float dx2, float dy2, 
						  float dx3, float dy3)
	{
	float x1, y1, x2, y2, x3, y3;

	h->flags |= NEW_HINTS;

	if (h->flags & START_COMPOSE)
		{
		callbackMove(h, h->cube[h->cubeStackDepth].offset_x , h->cube[h->cubeStackDepth].offset_y);
		h->cube[h->cubeStackDepth].offset_x = 0;
		h->cube[h->cubeStackDepth].offset_y = 0;
		h->flags &= ~START_COMPOSE;
		}

	x1 = h->x + dx1;	y1 = h->y + dy1;
	x2 = x1 + dx2;		y2 = y1 + dy2; 
	x3 = x2 + dx3;		y3 = y2 + dy3;

    x3 = roundf(x3*100)/100;
    y3 = roundf(y3*100)/100;
    h->x = x3;
    h->y = y3;

	if (h->flags & USE_MATRIX)
		h->glyph->curve(h->glyph, 
						TX(x1, y1), TY(x1, y1),
						TX(x2, y2), TY(x2, y2),
						TX(x3, y3), TY(x3, y3));
	else if (h->flags & SEEN_BLEND)
		h->glyph->curve(h->glyph, 
						RND(x1), RND(y1),
						RND(x2), RND(y2),
						RND(x3), RND(y3));
	else
		h->glyph->curve(h->glyph, x1, y1, x2, y2, x3, y3);
	}

/* Callback path flex. */
static void callbackFlex(t2cCtx h,
						 float dx1, float dy1,
						 float dx2, float dy2,
						 float dx3, float dy3,
						 float dx4, float dy4,
						 float dx5, float dy5,
						 float dx6, float dy6,
						 float depth)
	{

	if (h->flags & START_COMPOSE)
		{
		callbackMove(h, h->cube[h->cubeStackDepth].offset_x , h->cube[h->cubeStackDepth].offset_y);
		h->cube[h->cubeStackDepth].offset_x = 0;
		h->cube[h->cubeStackDepth].offset_y = 0;
		h->flags &= ~START_COMPOSE;
		}


	if (h->glyph->flex == NULL)
		{
		/* Callback as 2 curves */
		callbackCurve(h, 
					  dx1, dy1,
					  dx2, dy2,
					  dx3, dy3);
		callbackCurve(h,
					  dx4, dy4,
					  dx5, dy5,
					  dx6, dy6);
		}
	else
		{
		/* Callback as flex hint */
		float x1 = h->x + dx1;	float y1 = h->y + dy1;
		float x2 = x1 + dx2;	float y2 = y1 + dy2;
		float x3 = x2 + dx3;	float y3 = y2 + dy3;
		float x4 = x3 + dx4;	float y4 = y3 + dy4;
		float x5 = x4 + dx5;	float y5 = y4 + dy5;
		float x6 = x5 + dx6;	float y6 = y5 + dy6;
        x6 = roundf(x6*100)/100;
        y6 = roundf(y6*100)/100;
        h->x = x6;
        h->y = y6;

		if (h->flags & USE_MATRIX)
			h->glyph->flex(h->glyph, depth,
						   TX(x1, y1), TY(x1, y1),
						   TX(x2, y2), TY(x2, y2),
						   TX(x3, y3), TY(x3, y3),
						   TX(x4, y4), TY(x4, y4),
						   TX(x5, y5), TY(x5, y5),
						   TX(x6, y6), TY(x6, y6));
		if (h->flags & SEEN_BLEND)
			h->glyph->flex(h->glyph, depth,
						RND(x1), RND(y1),
						RND(x2), RND(y2),
						RND(x3), RND(y3),
						RND(x4), RND(y4),
						RND(x5), RND(y5),
						RND(x6), RND(y6));
		else
			h->glyph->flex(h->glyph, depth,
						   x1, y1,
						   x2, y2,
						   x3, y3,
						   x4, y4,
						   x5, y5,
						   x6, y6);
		}
	}

/* Add stems to stem list. Return 1 on stem overflow else 0. */
/* Note! Make sure that the width value has been cleared from
the stack befor this is called! */
static int addStems(t2cCtx h, int vert)
	{
	int i;
	float lastedge1 = 0;
	if (h->stems.cnt + h->stack.cnt/2 > T2_MAX_STEMS)
		return 1;
	/* The "i = h->stack.cnt & 1" clause lets it skip the width value, if there is one. */
	for (i = h->stack.cnt & 1; i < h->stack.cnt - 1; i += 2)
		{
		Stem *stem = &h->stems.array[h->stems.cnt++];

		stem->edge0 = lastedge1 + INDEX(i + 0);
		if (h->seac.phase > seacBase)
			stem->edge0 += vert? h->seac.adx: h->seac.ady;
		stem->edge1 = stem->edge0 + INDEX(i + 1);
		stem->flags = vert? ABF_VERT_STEM: 0;

		lastedge1 = stem->edge1;
		}

	/* Compute mask length and unused bit mask */
	h->mask.length = (short)((h->stems.cnt + 7)/8);
	h->mask.unused = ~(~0<<(h->mask.length*8 - h->stems.cnt));

	return 0;
	}

/* Callback hint/cntrmask. Return 0 on success else error code. */
static int callbackMask(t2cCtx h, int cntr, 
						unsigned char **next, unsigned char **end)
	{
	int i;

	if (h->mask.state == 1)
		savePendCntr(h, cntr);
	
	if (h->stack.cnt > 1)
		{
		/* Stem args on stack; must be omitted vstem(hm) op */
		if (addStems(h, 1))
			return t2cErrStemOverflow;
		}

	/* Check for invalid mask */
	if (h->mask.length == 0 || h->mask.length > T2_MAX_STEMS/8)
		return t2cErrHintmask;

	/* Read mask */
	for (i = 0; i < h->mask.length; i++)
		{
		if (*next == *end)
			{
			*next = refill(h, end);
			if (*next == NULL)
				return t2cErrSrcStream;
			}
		h->mask.bytes[i] = *(*next)++;
		}

	if (h->mask.bytes[h->mask.length - 1] & h->mask.unused)
	{
		message(h, "invalid hint/cntr mask. Correcting...");
		h->mask.bytes[h->mask.length - 1] &= ~h->mask.unused; /* clear the unused bits */
	}

	if (h->glyph->stem == NULL)
		return 0;	/* No stem callback */

	if (cntr && h->mask.state == 0)
		{
		/* Save first cntrmask */
		h->mask.state = 1;
		return 0;
		}

	callbackStems(h, cntr);
	return 0;
	}

/* Callback seac operator. */
static void callback_seac(t2cCtx h, float adx, float ady, int bchar, int achar)
	{
	if (h->flags & USE_MATRIX)
		{
		adx = TX(adx, ady);
		ady = TY(adx, ady);
		}
	else if (h->flags & SEEN_BLEND)
		{
		adx = RND(adx);
		ady = RND(ady);
		}
		
	h->glyph->seac(h->glyph, adx, ady, bchar, achar);
	}

/* Callback setwv operator. */
static void callback_blend_cube(t2cCtx h, unsigned int nBlends, unsigned int numVals, float* blendVals)
	{
	if (h->glyph->cubeBlend == NULL)
		return;
		
	h->glyph->cubeBlend(h->glyph, nBlends, numVals, blendVals);
	return;
	}

/* Callback blend operator. */
static void callback_setwv_cube(t2cCtx h, unsigned int numDV)
	{
	if (h->glyph->cubeSetwv == NULL)
		return;
		
	h->glyph->cubeSetwv(h->glyph, numDV);
	return;
	}

/* Callback compose operator. */
static void callback_compose(t2cCtx h, int cubeLEIndex, float dx, float dy, int numDV, float *ndv)
	{
	h->x += dx;	h->y += dy;
    h->x = roundf(h->x*100)/100;
    h->y = roundf(h->y*100)/100;
	if (h->glyph->cubeCompose == NULL)
		return;
		
	h->glyph->cubeCompose(h->glyph, cubeLEIndex, h->x, h->y, numDV, ndv);
	}

/* Callback transform operator. */
static void callback_transform(t2cCtx h, float rotate, float scaleX, float scaleY, float skewX, float skewY)
	{
	if (h->glyph->cubeTransform == NULL)
		return;
		
	h->glyph->cubeTransform(h->glyph, rotate, scaleX, scaleY, skewX, skewY);
	}

/* ---------------------------- Charstring Parse --------------------------- */

/* Parse seac component glyph. */
static int parseSeacComponent(t2cCtx h, int stdcode)
	{
	long offset;
	
	if (stdcode < 0 || stdcode > 255)
			return t2cErrBadSeacComp;
	offset = h->aux->getStdEncGlyphOffset(h->aux->ctx, stdcode);
	if (offset == -1)
		return t2cErrBadSeacComp;

	h->stack.cnt = 0;
	return t2Decode(h, offset);
	}

/* Unbias subroutine number. Return -1 on error else subroutine number. */
static long unbias(long arg, long nSubrs)
	{
	if (nSubrs < 1240)
		arg += 107;
	else if (nSubrs < 33900)
		arg += 1131;
	else 
		arg += 32768;
	return (arg < 0 || arg >= nSubrs)? -1: arg;
	}

/* Unbias Library element gsub value. 
 -107 is is the last gsubrm -106, the second to last, and so on.
 Return -1 on error else subroutine number. */
static long unbiasLE(long arg, long nSubrs)
{
	long subrIndex = nSubrs - (107 + arg + 1);
	return (subrIndex < 0 || subrIndex >= nSubrs)? -1: subrIndex;
}


static long setWV(t2cCtx h, int i)
{
 return 1;
}

/* Reverse stack elements between index i and j (i < j). */
static void reverse(t2cCtx h, int i, int j)
	{
	while (i < j)
		{
		float tmp = h->stack.array[i];
		h->stack.array[i++] = h->stack.array[j];
		h->stack.array[j--] = tmp;
		}
	}

static int do_set_weight_vector_cube(t2cCtx h, int nAxes)
	{
	float dx, dy;
	int i = 0;
	int j= 0;
	int nMasters = 1<<nAxes;
	float NDV[kMaxCubeAxes];
	int popCnt = nAxes + 3;
	int composeCnt = h->cube[h->cubeStackDepth].composeOpCnt;
	float*composeOps = h->cube[h->cubeStackDepth].composeOpArray;

	h->flags |= NEW_HINTS;
	
	if (h->flags & CUBE_RND)
	{
		dx = (float)4*(long)composeOps[1];
		dy = (float)4*(long)composeOps[2];
	}
	else 
	{
		dx = (float)composeOps[1];
		dy = (float)composeOps[2];
	}

	/* Several complications for processing the Cube library element (x,y) offset.
	1) It acts as an rmoveto. However, since the LE may have an rmoveto, we need to save the dx, and dy
	value and either add it to the LE's rmoveto, or add it as an explicit rmoveto before the LE's first marking operator.
	2) The LE path ops have no effect on the current point. This means that after the LE is processed, we need to restore the
	current point to the what it was after the compose (dx,dy0 is added.
	All this matters only when we are flattening compose operators, as otherwise, the values are just passed through.
	*/
	h->cube[h->cubeStackDepth].start_x = h->x + dx;
	h->cube[h->cubeStackDepth].start_y = h->y + dy;
	h->cube[h->cubeStackDepth].offset_x = dx;
	h->cube[h->cubeStackDepth].offset_y = dy;
	
	if (composeCnt < (nAxes + 3))
		return t2cErrStackUnderflow;
	h->cube[h->cubeStackDepth].nMasters = nMasters;
	
	if (!(h->flags & FLATTEN_CUBE))
		{
		callback_compose(h, h->cube[h->cubeStackDepth].leIndex, dx, dy, nAxes, &composeOps[3]);
		/* Pop all the current COMPOSE args off the stack. */
		for (i=popCnt; i< composeCnt; i++)
			composeOps[i-popCnt] = composeOps[i];
		h->cube[h->cubeStackDepth].composeOpCnt -= popCnt;
		return t2cSuccess;
		}

	while (i < nAxes)
		{
		NDV[i] = (float)((100 + (long)composeOps[3+i])/200.0);
		i++;
		}
		
	/* Compute Weight Vector */
	for (i = 0; i < nMasters; i++)
		{
		h->cube[h->cubeStackDepth].WV[i] = 1;
		for (j = 0; j < nAxes; j++)
			h->cube[h->cubeStackDepth].WV[i] *= (i & 1<<j)? NDV[j]: 1 - NDV[j];
		}
	/* Pop all the current COMPOSE args off the stack. */
	for (i=popCnt; i< composeCnt; i++)
		composeOps[i-popCnt] = composeOps[i];
	h->cube[h->cubeStackDepth].composeOpCnt -= popCnt;

	return t2cSuccess;
	}
	
/* Execute "blend" op. Return 0 on success else error code. */
static int do_blend_cube(t2cCtx h, int nBlends)
	{
	int i;
	int nElements = nBlends * h->cube[h->cubeStackDepth].nMasters;
	int iBase = h->stack.cnt - nElements;
	int k = iBase + nBlends;
	
	if (h->cube[h->cubeStackDepth].nMasters <= 1)
		return t2cErrInvalidWV;
	CHKUFLOW(nElements);

	if  (h->flags & FLATTEN_CUBE)
		{
		for (i = 0; i < nBlends; i++)
			{
			int j;
			double x = INDEX(iBase + i);
			for (j = 1; j < h->cube[h->cubeStackDepth].nMasters; j++)
				x += INDEX(k++)*h->cube[h->cubeStackDepth].WV[j];
			INDEX(iBase + i) = (float)x;
			}
		}
	else
		{
		float blendVals[kMaxCubeMasters*kMaxBlendOps];
		for (i = 0; i < nElements; i++)
			{
			blendVals[i] = INDEX(iBase + i);
			}
		callback_blend_cube(h, nBlends, nElements, blendVals);
		}
		
	h->stack.cnt = iBase + nBlends;

	return 0;
	}

/* Support undocumented blend operator. Retained for multiple master font
   substitution. Assumes 4 masters. Return 0 on success else error code. */
/* Note: there is no command line support to set a WV for Type 2, but you can
   compile one for testing by editing t2cParse(), below */

static int handleBlend(t2cCtx h)
{
    /* When the 'blend operator is encountered, the last items on the stack are
     the blend operands. These are, in order:
     numMaster operands from the default font
     (numMaster-1)* numBlends blend delta operands
     numBlends value
     
     What we do is to find the first blend operand item, which is the first value from the
     default font. We assign numBlends to the field of the same name in this struct_elem, and pop
     it off the stack. We then allocate memory to hold the (numMaster-1)* numBlends blend delta operands,
     and copy them to the blendValues field. We then pop the (numMaster-1)* numBlends blend delta operands,
     leaving the numMaster operands.
     When an operator is encountered, a handler that does not know about the blend values will
     simply see the usual stack values from the default fonts. A handler which does know about
     the blend values can use the blend values to fill the blendable fields.
     
     Alternatively, the blend handler can use the blend operands to actually produce blended values,
     and replace the  numMaster operands from the defaultfont with the blended values.
     */
    int result = 0;
    int numBlends = h->glyph->info->blendInfo.numBlends = (int)INDEX(h->stack.cnt -1);
    h->stack.cnt--;
    h->stack.blendCnt--;
    
    if (h->flags & T2C_FLATTEN_BLEND)
    {
        /* this is where we should blend the values. For the moment, we will just pop off the the blend delta operands,
         leaving the the default values on the stack, which has the same result as making a snapshot at the center of
         the design space.
         */
        h->stack.cnt -= numBlends*h->stack.numRegions;
    }
    else
    {
        abfOpEntry* firstItem;
        int i = 0;
        int numDeltaBlends = numBlends*h->stack.numRegions;
        int firstItemIndex = (h->stack.blendCnt - (numBlends + numDeltaBlends));
        
        firstItem = &(h->stack.blendArray[firstItemIndex]);
        firstItem->numBlends = numBlends;
        firstItem->blendValues = memNew(h, sizeof(float)*numDeltaBlends);
        while (i < numDeltaBlends)
        {
            int index = i + (h->stack.cnt - numDeltaBlends);
            float val= INDEX_BLEND(index).value;
            firstItem->blendValues[i] = val;
            i++;
        }
        h->stack.cnt -= numDeltaBlends;
        h->stack.blendCnt -= numDeltaBlends;
    }
    return result;
}

static void clearBlendStack(t2cCtx h)
{
    if (h->flags & SEEN_CFF2_BLEND)
    {
        int j = 0;
        while (j < h->stack.blendCnt)
        {
            if (h->stack.blendArray[j].numBlends > 0)
            {
                memFree(h, h->stack.blendArray[j].blendValues);
                h->stack.blendArray[j].numBlends = 0;
            }
            j++;
        }
        h->flags &= ~SEEN_CFF2_BLEND;
    }
    h->stack.blendCnt = 0;
}

static void setNumMasters(t2cCtx h)
{
    unsigned int vsindex = h->glyph->info->blendInfo.vsindex;
    h->stack.numRegions = h->aux->varStore->varDataList[vsindex].regionCount;
}

/* Decode Type 2 charstring. Return 0 to continue else error code. */
static int t2Decode(t2cCtx h, long offset)
{
	unsigned char *end;
	unsigned char *next;
	/* Fetch charstring */
	if (srcSeek(h, offset))
		return t2cErrSrcStream;
	next = refill(h, &end);
	if (next == NULL)
		return t2cErrSrcStream;
	
	for (;;)
		{
		int i;
		int result;
		int byte0;
            
        CHKBYTE(h);
        //CHKBYTE2(h, &next, &end);
		byte0 = *next++;
		switch (byte0)
			{
				case tx_reserved0:
				case t2_reserved9:
				case t2_reserved13:
                    return t2cErrInvalidOp;
                case t2_vsindex:
                    h->glyph->info->blendInfo.vsindex = POP();
                    setNumMasters(h);
                    break;
				case tx_callgrel:
				{
				int result;
				long saveoff = h->src.offset - (end - next);
				long saveEndOff = h->src.endOffset;
				long num = unbiasLE((long)POP(), h->aux->gsubrs.cnt);
				if (num == -1)
					return t2cErrCallgsubr;
				if ((num +1) < h->aux->gsubrs.cnt)
					h->src.endOffset = h->aux->gsubrs.offset[num+1];
				else
					h->src.endOffset = h->aux->gsubrsEnd;
				
				result = t2Decode(h, h->aux->gsubrs.offset[num]);
				h->src.endOffset = saveEndOff;
				if (result || h->flags & SEEN_ENDCHAR)
					return result;
				else if (srcSeek(h, saveoff))
					return t2cErrSrcStream;
				next = refill(h, &end);
				if (next == NULL)
					return t2cErrSrcStream;
				}
				continue;
				
				case tx_compose:
				
				CHKUFLOW(4);
                /* If there is a transform left over from a previous path op, clear it. */
                if (h->flags & USE_MATRIX)
                {
                    h->flags &= ~USE_MATRIX;
                }
				
				if ((h->flags & PEND_MASK) && (callbacDfltkWidth(h))) /* we ony need to do callbackWidth is this is the initial hint */
					return t2cSuccess;
				
				clearHintStack(h); 
				/* If hstm/vstem have been called but no mask operators, we need to report them now, as any new hint ops we meet in the library elements
				 will have to be treated as hint substitution blocks, so these will othewise get thrown away. */
				
				/* The op stack at this point looks like:
				 library element num
				 dx
				 dy
				 design vector val 1-n, where n is a max of 5
				 .. repeated.
				 
				 However, we don't know how many masters there are.
				 I just call the LE routine, which will pop the stack items
				 which it needs. When it returns, if there are any items on the stack,
				 we assume that the items are for the next LE, and we call it.
				 I still need to do this even if I am not flattening the Cube LE, as
				 I can can't call the compose glyph call back until I know how many design vectors there are,
				 which I don't know until I see the setwv<n> operator. As a result, I call the compose glyph callback from
				 within the setwv routine.
				 */
				
				
				/*  compose operators must follow all regular charstring operators, but do not inherit any hints from the parent charstring.
				 This means that any hints seen in the library elements must be new hint blocks. */
				h->flags &= ~PEND_MASK; /* any hints seen in the library element need to be treated as a hint-sub.*/
				
				if (h->stack.cnt < 4)
					return t2cErrStackUnderflow;
				
				h->cubeStackDepth++;
				/* copy compose ops to h->cubeOpArray */
				h->cube[h->cubeStackDepth].composeOpCnt = h->stack.cnt;
				while ( h->stack.cnt-- > 0)
					h->cube[h->cubeStackDepth].composeOpArray[h->stack.cnt] = h->stack.array[h->stack.cnt];
                    h->stack.cnt = 0;
				while (h->cube[h->cubeStackDepth].composeOpCnt >= 4)
					{
					int result;
					long saveoff = h->src.offset - (end - next);
					long saveEndOff = h->src.endOffset;
					float *composeOpArray = h->cube[h->cubeStackDepth].composeOpArray;
					long int leIndex = (long)composeOpArray[0];
					long num = unbiasLE(leIndex, h->aux->gsubrs.cnt);
					h->cube[h->cubeStackDepth].leIndex = leIndex;
					if ((num == -1) || (h->aux->gsubrs.cnt == 0))
						{
						/* maybe this is a font with the Cube compose ops, but without the subrs. we get this
						 when converting a svg font to a cff font. 
						 In this case, Assume that each compose op calls just one LE. */
						
						if ( (!(h->flags & FLATTEN_CUBE)) && (h->aux->subrs.cnt <=5) )
							{
							int nAxes = h->cube[h->cubeStackDepth].composeOpCnt -3;
							
							float dx;
							float dy;
							if (h->flags & CUBE_RND)
								{
								dx = (float)4*(long)composeOpArray[1];
								dy = (float)4*(long)composeOpArray[2];
								}
							else 
								{
								dx = (float)composeOpArray[1];
								dy = (float)composeOpArray[2];
								}
							callback_compose(h, leIndex, dx, dy, nAxes, &composeOpArray[3]);
							result = 0;
							h->stack.cnt = 0;
							h->cube[h->cubeStackDepth].composeOpCnt = 0;
							}
						else
							return t2cErrCallgsubr;
						}
					else
						{ /* we have the LE subr we need */
						if ((num +1) < h->aux->gsubrs.cnt)
							h->src.endOffset = h->aux->gsubrs.offset[num+1];
						else
							h->src.endOffset = h->aux->gsubrsEnd;
						
						if (h->flags & FLATTEN_CUBE)
						{
							h->flags |= START_COMPOSE;
							h->flags |= FLATTEN_COMPOSE;
						}
						h->flags |= SEEN_BLEND; // Apply rounding when updating the final position after a drawing op
						result = t2Decode(h, h->aux->gsubrs.offset[num]);
                        if (result)
                            return result;
						h->flags &= ~SEEN_BLEND;
						h->src.endOffset = saveEndOff;
						if (h->flags & FLATTEN_CUBE)
						{
							h->flags &= ~START_COMPOSE;
							h->flags &= ~FLATTEN_COMPOSE;
						}
						h->x = h->cube[h->cubeStackDepth].start_x; /* these get set in do_set_weight_vector_cube */
						h->y = h->cube[h->cubeStackDepth].start_y;
						}
					
                        /* Clear the path-specific transform matrix, if any. */
                        if (h->flags & USE_MATRIX)
                        {
                            /* if START_PATH_MATRIX is not set, then this either global, or left over from a previous path.  */
                            if (h->flags & USE_GLOBAL_MATRIX)
                            {
                                int i;
                                /* restore the global matrix values */
                                for (i = 0; i < 6; i++)
                                {
                                    h->transformMatrix[i] = h->aux->matrix[i];
                                }
                            }
                            else
                            {
                                /* If USE_GLOBAL_MATRIX is not set, then the   USE_MATRIX is left over from the previous path. Clear it. */
                                h->flags &= ~USE_MATRIX;
                                h->transformMatrix[0] =  h->transformMatrix[3] = 1.0;
                                h->transformMatrix[1] =  h->transformMatrix[2] = h->transformMatrix[4] = h->transformMatrix[5] = 1.0;
                            }
                        }
                        
					if (result) /* ignore endChar flag, if there is one in the LE. */
						{
						h->cubeStackDepth--;
						return result;
						}
					else if (srcSeek(h, saveoff))
						{
						h->cubeStackDepth--;
						return t2cErrSrcStream;
						}
					next = refill(h, &end);
					if (next == NULL)
						{
						h->cubeStackDepth--;
						return t2cErrSrcStream;
						}
					}
				h->cubeStackDepth--;
				
				if (h->flags & SEEN_ENDCHAR)
					return t2cSuccess;
				break;
				case tx_hstem:
				if ((h->flags & IS_CUBE) && (h->cubeStackDepth >= 0))
					{
					callbackCubeStem(h, 0); /* calls stem call back immediately, instead of waiting for endchar. mask ops not supported.*/
					break;
					}
				/* if not in a library element, falls through to t2_hstemhm */
				case t2_hstemhm:
				if (callbackWidth(h, 1))
					return t2cSuccess;
				if (addStems(h, 0))
					return t2cErrStemOverflow;
				break;
				case tx_vstem:
				if ((h->flags & IS_CUBE) &&  (h->cubeStackDepth  >= 0))
					{
					callbackCubeStem(h, ABF_VERT_STEM); /* calls stem call back immediately, instead of waiting for endchar. mask ops not supported.r*/
					break;
					}
				/* if not in a library element, falls through to t2_vstemhm */
				case t2_vstemhm:
				if (callbackWidth(h, 1))
					return t2cSuccess;
				if (addStems(h, 1))
					return t2cErrStemOverflow;
				break;
				case tx_rmoveto:
				if (callbackWidth(h, 1))
					return t2cSuccess;
				{
				float y = POP();
				float x = POP();
				callbackMove(h, x, y);
				}
				break;
				case tx_hmoveto:
				if (callbackWidth(h, 0))
					return t2cSuccess;
				callbackMove(h, POP(), 0);
				break;
				case tx_vmoveto:
				if (callbackWidth(h, 0))
					return t2cSuccess;
				callbackMove(h, 0, POP());
				break;
				case tx_rlineto:
				CHKUFLOW(2);
				for (i = 0; i < h->stack.cnt - 1; i += 2)
					callbackLine(h, INDEX(i + 0), INDEX(i + 1));
				break;
				case tx_hlineto:
				case tx_vlineto:
				CHKUFLOW(1);
				{
				int horz = byte0 == tx_hlineto;
				for (i = 0; i < h->stack.cnt; i++)
					if (horz++ & 1)
						callbackLine(h, INDEX(i), 0);
					else
						callbackLine(h, 0, INDEX(i));
				}
				break;
				case tx_rrcurveto:
				CHKUFLOW(6);
				for (i = 0; i < h->stack.cnt - 5; i += 6)
					callbackCurve(h, 
								  INDEX(i + 0), INDEX(i + 1), 
								  INDEX(i + 2), INDEX(i + 3), 
								  INDEX(i + 4), INDEX(i + 5));
				break;
				case tx_callsubr:
				CHKUFLOW(1);
				{
				int result;
				long saveoff = h->src.offset - (end - next);
				long saveEndOff = h->src.endOffset;
				long num = unbias((long)POP(), h->aux->subrs.cnt);
				
				if (num == -1)
					return t2cErrCallsubr;
				if ((num +1) < h->aux->subrs.cnt)
					h->src.endOffset = h->aux->subrs.offset[num+1];
				else
					h->src.endOffset = h->aux->subrsEnd;
				h->subrDepth++;
                if (h->subrDepth > TX_MAX_SUBR_DEPTH)
                {
                    printf("subr depth: %d\n", h->subrDepth);
                    return t2cErrSubrDepth;
                }
                    
				result = t2Decode(h, h->aux->subrs.offset[num]);
				h->src.endOffset = saveEndOff;
				h->subrDepth--;
				
				if (result || h->flags & SEEN_ENDCHAR)
					return result;
				else if (srcSeek(h, saveoff))
					return t2cErrSrcStream;
				next = refill(h, &end);
				if (next == NULL)
					return t2cErrSrcStream;
				}
				continue;
				case tx_return:
				return 0;
				case tx_escape:
				/* Process escaped operator */
				{
				int escop;
				CHKBYTE(h);
				escop = tx_ESC(*next++);
				switch (escop)
					{
						case tx_dotsection:
						if (!(h->aux->flags & T2C_UPDATE_OPS ||
							  h->glyph->stem == NULL))
							callbackOp(h, tx_dotsection);
						break;
						case tx_and:
						CHKUFLOW(2);
						{
						int b = POP() != 0.0f;
						int a = POP() != 0.0f;
						PUSH(a && b);
						}
						continue;
						case tx_or:
						CHKUFLOW(2);
						{
						int b = POP() != 0.0f;
						int a = POP() != 0.0f;
						PUSH(a || b);
						}
						continue;
						case tx_not:
						CHKUFLOW(1);
						{
						int a = POP() == 0.0f;
						PUSH(a);
						}
						continue;
						case tx_abs:
						CHKUFLOW(1);
						{
						float a = POP();
						PUSH((a < 0.0f)? -a: a);
						}
						continue;
						case tx_add:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(a + b);
						}
						continue;
						case tx_sub:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(a - b);
						}
						continue;
						case tx_div:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(a/b);
						}
						continue;
						case tx_neg:
						CHKUFLOW(1);
						{
						float a = POP();
						PUSH(-a);
						}
						continue;
						case tx_eq:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(a == b);
						}
						continue;
						case tx_drop:
						CHKUFLOW(1);
						h->stack.cnt--;
						continue;
						case tx_put:
						CHKUFLOW(2);
						{
						int i = (int)POP();
						if (i < 0 || i >= TX_BCA_LENGTH)
							return t2cErrPutBounds;
						h->BCA[i] = POP();
						}
						continue;
						case tx_get:
						CHKUFLOW(1);
						{
						int i = (int)POP();
						if (i < 0 || i >= TX_BCA_LENGTH)
							return t2cErrGetBounds;
						PUSH(h->BCA[i]);
						}
						continue;
						case tx_ifelse:
						CHKUFLOW(4);
						{
						float v2 = POP();
						float v1 = POP();
						float s2 = POP();
						float s1 = POP();
						PUSH((v1 <= v2)? s1: s2);
						}
						continue;
						case tx_random:	
						CHKOFLOW(1);
						PUSH(((float)rand() + 1) / ((float)RAND_MAX + 1));
						continue;
						case tx_mul:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(a*b);
						}
						continue;
						case tx_sqrt:
						CHKUFLOW(1);
						{
						float a = POP();
						if (a < 0.0f)
							return t2cErrSqrtDomain;
						PUSH(sqrt(a));
						}
						continue;
						case tx_dup:
						CHKUFLOW(1);
						CHKOFLOW(1);
						{
						float a = POP();
						PUSH(a);
						PUSH(a);
						}
						continue;
						case tx_exch:
						CHKUFLOW(2);
						{
						float b = POP();
						float a = POP();
						PUSH(b);
						PUSH(a);
						}
						continue;
						case tx_index:
						CHKUFLOW(1);
						{
						int i = (int)POP();
						if (i < 0)
							i = 0;	/* Duplicate top element */
						if (i >= h->stack.cnt)
							return t2cErrIndexBounds;
						PUSH(h->stack.array[h->stack.cnt - 1 - i]);
						}
						continue;
						case tx_roll:
						CHKUFLOW(2);
						{
						int j = (int)POP();
						int n = (int)POP();
						int iTop = h->stack.cnt - 1;
						int iBottom = h->stack.cnt - n;
						
						if (n < 0 || iBottom < 0)
							return t2cErrRollBounds;
						
						/* Constrain j to [0,n) */
						if (j < 0)
							j = n - (-j % n);
						j %= n;
						
						reverse(h, iTop - j + 1, iTop);
						reverse(h, iBottom, iTop - j);
						reverse(h, iBottom, iTop);
						}
						continue;
						case t2_hflex:
						CHKUFLOW(7);
						callbackFlex(h,	
									 INDEX(0),  0,
									 INDEX(1),  INDEX(2),
									 INDEX(3),  0,
									 INDEX(4),  0,
									 INDEX(5), -INDEX(2),
									 INDEX(6),  0,
									 TX_STD_FLEX_DEPTH);
						break;
						case t2_flex:
						CHKUFLOW(13);
						callbackFlex(h, 							 
									 INDEX(0),  INDEX(1),
									 INDEX(2),  INDEX(3),
									 INDEX(4),  INDEX(5),
									 INDEX(6),  INDEX(7),
									 INDEX(8),  INDEX(9),
									 INDEX(10), INDEX(11),
									 INDEX(12));
						break;
						case t2_hflex1:
						CHKUFLOW(9);
						{
						float dy1 = INDEX(1);
						float dy2 = INDEX(3);
						float dy5 = INDEX(7);
						callbackFlex(h,
									 INDEX(0),  dy1,
									 INDEX(2),  dy2,
									 INDEX(4),  0,
									 INDEX(5),  0,		  
									 INDEX(6),  dy5,
									 INDEX(8),  -(dy1 + dy2 + dy5), 
									 TX_STD_FLEX_DEPTH);
						}
						break;
						case t2_flex1:
						CHKUFLOW(11);
						{
						float dx1 = INDEX(0);
						float dy1 = INDEX(1);
						float dx2 = INDEX(2);
						float dy2 = INDEX(3);
						float dx3 = INDEX(4);
						float dy3 = INDEX(5);
						float dx4 = INDEX(6);
						float dy4 = INDEX(7);
						float dx5 = INDEX(8);
						float dy5 = INDEX(9);
						float dx = dx1 + dx2 + dx3 + dx4 + dx5;
						float dy = dy1 + dy2 + dy3 + dy4 + dy5;
						if (ABS(dx) > ABS(dy))
							{
							dx = INDEX(10);
							dy = -dy;
							}
						else
							{
							dx = -dx;
							dy = INDEX(10);
							}
						callbackFlex(h,
									 dx1, dy1, 
									 dx2, dy2, 
									 dx3, dy3,
									 dx4, dy4, 
									 dx5, dy5,  
									 dx,  dy, 
									 TX_STD_FLEX_DEPTH);
						}
						break;
						case t2_cntron:
						h->LanguageGroup = 1;	/* Turn on global coloring */
						callbackOp(h, t2_cntron);
						continue;
						case t2_reservedESC1:
						case t2_reservedESC2:
						case t2_reservedESC6:
						case t2_reservedESC7:
						case t2_reservedESC8:
						case t2_reservedESC13:
						case t2_reservedESC16:
						case t2_reservedESC17:
						case tx_reservedESC19:
						case tx_reservedESC25:
						case tx_reservedESC31:
						case tx_reservedESC32:
						case t2_reservedESC33:
						break;
						case tx_BLEND1:
						result = do_blend_cube(h, 1);
						if (result)
							return result;
						continue; /* not break, since we don;t want ot clear the stack */
						case tx_BLEND2:
						result = do_blend_cube(h, 2);
						if (result)
							return result;
						continue; /* not break, since we don;t want ot clear the stack */
						case tx_BLEND3:
						result = do_blend_cube(h, 3);
						if (result)
							return result;
						continue; /* not break, since we don;t want ot clear the stack */
						case tx_BLEND4:
						result = do_blend_cube(h, 4);
						if (result)
							return result;
						continue; /* not break, since we don;t want ot clear the stack */
						case tx_BLEND6:
						result = do_blend_cube(h, 6);
						if (result)
							return result;
						continue; /* not break, since we don;t want ot clear the stack */
						case tx_SETWV1:
						{
						int numAxes = 1;
						result = do_set_weight_vector_cube(h, numAxes);
						if (result || !(h->flags & FLATTEN_CUBE))
							return result;
						}
						break;
						case tx_SETWV2:
						{
						int numAxes = 2;
						result = do_set_weight_vector_cube(h, numAxes);
						if (result || !(h->flags & FLATTEN_CUBE))
							return result;
						}
						break;
						case tx_SETWV3:
						{
						int numAxes = 3;
						result = do_set_weight_vector_cube(h, numAxes);
						if (result || !(h->flags & FLATTEN_CUBE))
							return result;
						}
						break;
						case tx_SETWV4:
						{
						int numAxes = 4;
						result = do_set_weight_vector_cube(h, numAxes);
						if (result || !(h->flags & FLATTEN_CUBE))
							return result;
						}
						break;
						case tx_SETWV5:
						{
                            int numAxes = 5;
                            result = do_set_weight_vector_cube(h, numAxes);
                            if (result || !(h->flags & FLATTEN_CUBE))
                                return result;
						}
                            break;
                            
						case tx_SETWVN:
						{
                            int numAxes = (int)POP();
                            result = do_set_weight_vector_cube(h, numAxes);
                            if (result || !(h->flags & FLATTEN_CUBE))
                                return result;
						}
                            break;
                            
						case tx_transform:
						/* This set a transform matrix to be applied to all path data until it is cleared
						 by seeing another t1_transform, or a move-to or tx_compose which follows another 
						 a move-to or tx_compose. */
						{
                        float rotate;
                        float radians;
						float scaleX;
						float scaleY;
                        float skewX;
                        float skewY;
                        float sinVal;
                        float cosVal;
						
						CHKUFLOW(5);
                        // rotate is ezpressed in degrees. All are expressed in val*100 or *1000, to avoid using "div" operator in charstring.
                        rotate = (float)(INDEX(0)/100.0);
                        radians = (float)(M_PI*rotate/180.0);
                        scaleX = (float)(INDEX(1)/1000.0);
                        scaleY = (float)(INDEX(2)/1000.0);
                        skewX = (float)(INDEX(3)/1000.0);
                        skewY = (float)(INDEX(4)/1000.0);
						
						if (scaleX == 0)
							scaleX = 1.0;
						if (scaleY == 0)
							scaleY = 1.0;
						
						if (h->flags & FLATTEN_CUBE)
							{
                            h->flags |= START_PATH_MATRIX; // flag that we have added a path specific transform.*/
                        
                            sinVal = (float)sin(radians);
                            cosVal = (float)cos(radians);
                            
                            h->transformMatrix[0] = scaleX * cosVal - skewX * sinVal;
                            h->transformMatrix[1] = scaleX * sinVal + skewX * cosVal;
                            h->transformMatrix[2] = skewY * cosVal - scaleY * sinVal;
                            h->transformMatrix[3] = skewY * sinVal + scaleY * cosVal;
							
							h->transformMatrix[4] = 0;
							h->transformMatrix[5] = 0;
							// The offsets (matrix[4] and matrix[5]) will be set at the first move-to, so that the rotation is around the
							// first start point.
							}
						else
							{
							callback_transform(h, rotate, scaleX, scaleY, skewX, skewY);
							}
						
						break;
						}
						
						default:
						return t2cErrInvalidOp;
					} 		/* End: switch (escop) */
				}			/* End: case tx_escape: */
				break;
				
				
				case tx_endchar:
				do_endchar:
				if (h->cubeStackDepth > -1) /* Ignore if we are in a CUBE library element. Early test CFF fonts
											 erroneously had enchar ops.*/
					return t2cSuccess;
				if (callbackWidth(h, 1))
					return t2cSuccess;
				if (h->stack.cnt > 1)
					{
					CHKUFLOW(4);
					
					/* Save arguments */
					h->aux->achar = (unsigned char)POP();
					h->aux->bchar = (unsigned char)POP();
					h->seac.ady = POP();
					h->seac.adx = POP();
					
					if (h->aux->flags & T2C_UPDATE_OPS)
						{
						/* Parse base component */
						h->seac.phase = seacBase;
						result = parseSeacComponent(h, h->aux->bchar);
						if (result)
							return result;
						
						h->flags &= ~SEEN_ENDCHAR;
						h->stems.cnt = 0;
						h->mask.length = 0;
						h->mask.state = 0;
						h->x = h->seac.adx;
						h->y = h->seac.ady;
                        h->x = roundf(h->x*100)/100;
                        h->y = roundf(h->y*100)/100;
						
						/* Parse accent component */
						h->seac.phase = seacAccentPreMove;
						result = parseSeacComponent(h, h->aux->achar);
						if (result)
							return result;
						}
					else
						/* Callback seac data */
						callback_seac(h, h->seac.adx, h->seac.ady, 
									  h->aux->bchar, h->aux->achar);
					}
				h->flags |= SEEN_ENDCHAR;
				return 0;
                case t2_blend:
                {
                h->flags &= SEEN_CFF2_BLEND;
                if (h->stack.numRegions == 0)
                    setNumMasters(h);
                 result = handleBlend(h);
                if (result)
                    return result;
                }
                    continue;
                case t2_hintmask:
				if (callbackWidth(h, 1))
					return t2cSuccess;
				result = callbackMask(h, 0, &next, &end);
				if (result)
					return result;
				break;
				case t2_cntrmask:
				if (callbackWidth(h, 1))
					return t2cSuccess;
				result = callbackMask(h, 1, &next, &end);
				if (result)
					return result;
				break;
				case t2_rcurveline:
				CHKUFLOW(8);
				for (i = 0; i < h->stack.cnt - 5; i += 6)
					callbackCurve(h, 
								  INDEX(i + 0), INDEX(i + 1), 
								  INDEX(i + 2), INDEX(i + 3), 
								  INDEX(i + 4), INDEX(i + 5));
				if (i < h->stack.cnt - 1)
					callbackLine(h, INDEX(i + 0), INDEX(i + 1));
				break;
				case t2_rlinecurve:
				CHKUFLOW(8);
				for (i = 0; i < h->stack.cnt - 6; i += 2)
					callbackLine(h, INDEX(i + 0), INDEX(i + 1));
				callbackCurve(h, 
							  INDEX(i + 0), INDEX(i + 1), 
							  INDEX(i + 2), INDEX(i + 3), 
							  INDEX(i + 4), INDEX(i + 5));
				break;
				case t2_vvcurveto:
				if ((h->stack.cnt) & 1)
					{
					/* Add initial curve */
					CHKUFLOW(5);
					callbackCurve(h, 
								  INDEX(0), INDEX(1), 
								  INDEX(2), INDEX(3), 
								  0, 	    INDEX(4));
					i = 5;
					}
				else
					{
					CHKUFLOW(4);
					i = 0;
					}
				
				/* Add remaining curve(s) */
				for (; i < h->stack.cnt - 3; i += 4)
					callbackCurve(h,
								  0, 		    INDEX(i + 0), 
								  INDEX(i + 1), INDEX(i + 2), 
								  0, 		    INDEX(i + 3));
				break;
				case t2_hhcurveto:
				if ((h->stack.cnt ) & 1)
					{
					/* Add initial curve */
					CHKUFLOW(5);
					callbackCurve(h,
								  INDEX(1), INDEX(0), 
								  INDEX(2), INDEX(3), 
								  INDEX(4), 0);
					i = 5;
					}
				else
					{
					CHKUFLOW(4);
					i = 0;
					}
				
				/* Add remaining curve(s) */
				for (; i < h->stack.cnt - 3; i += 4)
					callbackCurve(h,
								  INDEX(i + 0), 0,
								  INDEX(i + 1), INDEX(i + 2), 
								  INDEX(i + 3), 0);
				break;
				case t2_callgsubr:
				CHKUFLOW(1);
				{
				int result;
				long saveoff = h->src.offset - (end - next);
				long saveEndOff = h->src.endOffset;
				long num = unbias((long)POP(), h->aux->gsubrs.cnt);
				if (num == -1)
					return t2cErrCallgsubr;
				if ((num +1) < h->aux->gsubrs.cnt)
					h->src.endOffset = h->aux->gsubrs.offset[num+1];
				else
					h->src.endOffset = h->aux->gsubrsEnd;
				
				h->subrDepth++;
                if (h->subrDepth > TX_MAX_SUBR_DEPTH)
                {
                    printf("subr depth: %d\n", h->subrDepth);
                    return t2cErrSubrDepth;
                }
                    
				result = t2Decode(h, h->aux->gsubrs.offset[num]);
				h->src.endOffset = saveEndOff;
				h->subrDepth--;
				
				if (result || h->flags & SEEN_ENDCHAR)
					return result;
				else if (srcSeek(h, saveoff))
					return t2cErrSrcStream;
				next = refill(h, &end);
				if (next == NULL)
					return t2cErrSrcStream;
				}
				continue;
				case tx_vhcurveto:
				case tx_hvcurveto:
				CHKUFLOW(4);
				{
				int adjust = ((h->stack.cnt) & 1)? 5: 0;
				int horz = byte0 == tx_hvcurveto;
				
				/* Add initial curve(s) */
				for (i = 0; i < h->stack.cnt - adjust - 3; i += 4)
					if (horz++ & 1)
						callbackCurve(h,
									  INDEX(i + 0), 0,
									  INDEX(i + 1), INDEX(i + 2), 
									  0,			INDEX(i + 3));
					else
						callbackCurve(h,
									  0,			INDEX(i + 0),
									  INDEX(i + 1), INDEX(i + 2), 
									  INDEX(i + 3), 0);
				
				if (adjust)
					{
					/* Add last curve */
					if (horz & 1)
						callbackCurve(h,
									  INDEX(i + 0), 0,
									  INDEX(i + 1), INDEX(i + 2), 
									  INDEX(i + 4), INDEX(i + 3));
					else
						callbackCurve(h,
									  0,	   	    INDEX(i + 0),
									  INDEX(i + 1), INDEX(i + 2), 
									  INDEX(i + 3), INDEX(i + 4));
					}
				}
				break;
				case t2_shortint:
				/* 3 byte number */
				CHKOFLOW(1);
				{
				short value;
				CHKBYTE(h);
				value = *next++;
				CHKBYTE(h);
				value = value<<8 | *next++;
#if SHRT_MAX > 32767
				/* short greater that 2 bytes; handle negative range */
				if (value > 32767)
					value -= 65536;
#endif
				PUSH(value);
				}
				continue;
				default:
				/* 1 byte number */
				CHKOFLOW(1);
				PUSH(byte0 - 139);
				continue;
				case 247: case 248: case 249: case 250:
				/* Positive 2 byte number */
				CHKOFLOW(1);
				CHKBYTE(h);
				PUSH(108 + 256*(byte0 - 247) + *next);
                next++;
				continue;
				case 251: case 252: case 253: case 254:
				/* Negative 2 byte number */
				CHKOFLOW(1);
				CHKBYTE(h);
				PUSH(-108 - 256*(byte0 - 251) - *next);
                next++;
				continue;
				case 255:
				/* 5 byte number */
				CHKOFLOW(1);
				{
				long value;
				CHKBYTE(h);
				value = *next++;
				CHKBYTE(h);
				value = value<<8 | *next++;
				CHKBYTE(h);
				value = value<<8 | *next++;
				CHKBYTE(h);
				value = value<<8 | *next++;
#if LONG_MAX > 2147483647
				/* long greater that 4 bytes; handle negative range */
				if (value > 2417483647)
					value -= 4294967296;
#endif
				PUSH(value/65536.0);
				}
				continue;
			} 				/* End: switch (byte0) */
        clearBlendStack(h);
		h->stack.cnt = 0;	/* Clear stack */
		} 					/* End: while (cstr < end) */
 
    /* for CFF2 Charstrings, we hit the end of the charstring without having seen endchar or return yet. Add it now. */
    if ((h->aux->flags & T2C_IS_CFF2)  && !(h->flags & SEEN_ENDCHAR))
    
    {
        /* if this was a subr, do return. */
        if (h->subrDepth > 0)
            return 0;
        else
            goto do_endchar;
     }
}

/* Decode Type 2 charstring. Return 0 to continue else error code.
Note: I do not try and convert the T2 only ops to ops common to T2 and T1 becuase the 
Cube library elements can only have the common ops.
*/
static int t2DecodeSubr(t2cCtx h, long offset)
	{
	/* This would better named "passThroughSubr", as it makes no attempt to
	covert between t2 and T2 and abs font ops. This is just fine for the purpose of 
	Cube fonts, as the regular global subroutines from subroutinization get thrown away,
	and only the Cube library elements and the gsubs called by tme are kept. These can have oly
	the ops that are common to T2 and T1 - no hint mask operators, no vvcurveto, etc.
	*/
	unsigned char *end;
	unsigned char *next;

	/* Fetch charstring */
	if (srcSeek(h, offset))
		return t2cErrSrcStream;
	next = refill(h, &end);
	if (next == NULL)
		return t2cErrSrcStream;

	for (;;)
		{
		int byte0;
		CHKSUBRBYTE(h);
		byte0 = *next++;
		switch (byte0)
			{
		case tx_reserved0:
		case t2_reserved9:
		case t2_reserved13:
			return t2cErrInvalidOp;
        case t2_vsindex:
		case t2_hintmask:
		case t2_cntrmask: /* can't process these in the context of subrs - no way to know how long the mask byte is. Skip it. */
			return 0;
		case tx_callgrel:
		case tx_compose:
		case tx_hstem:
		case t2_hstemhm:
		case tx_vstem:
		case t2_vstemhm:
		case tx_rmoveto:
		case tx_hmoveto:
		case tx_vmoveto:
		case tx_rlineto:
		case tx_hlineto:
		case tx_vlineto:
		case tx_rrcurveto:
		case tx_callsubr:
			callbackOp(h, byte0);
			break;
		case tx_return:
			do_subr_return:
			if (h->stack.cnt > 0)
				callbackOp(h, tx_return); /* allow numbers at end of subroutine */
			return 0;
		case tx_escape:
			/* Process escaped operator */
			{
			int escop;
			CHKSUBRBYTE(h);
			escop = tx_ESC(*next++);
			switch (escop)
				{
			case tx_dotsection:
			case tx_and:
			case tx_or:
			case tx_not:
			case tx_abs:
			case tx_add:
			case tx_sub:
			case tx_div:
			case tx_neg:
			case tx_eq:
			case tx_drop:
			case tx_put:
			case tx_get:
			case tx_ifelse:
			case tx_random:	
			case tx_mul:
			case tx_sqrt:
			case tx_dup:
			case tx_exch:
			case tx_index:
			case tx_roll:
			case t2_hflex:
			case t2_flex:
			case t2_hflex1:
			case t2_flex1:
			case t2_cntron:
			case t2_reservedESC1:
			case t2_reservedESC2:
			case t2_reservedESC6:
			case t2_reservedESC7:
			case t2_reservedESC8:
			case t2_reservedESC13:
			case t2_reservedESC16:
			case t2_reservedESC17:
			case tx_reservedESC19:
			case tx_reservedESC25:
			case tx_reservedESC31:
			case tx_reservedESC32:
			case t2_reservedESC33:
			case tx_BLEND1:
			case tx_BLEND2:
			case tx_BLEND3:
			case tx_BLEND4:
			case tx_BLEND6:
				callbackOp(h, escop);
				break;
			case tx_SETWV1:
			case tx_SETWV2:
			case tx_SETWV3:
			case tx_SETWV4:
            case tx_SETWV5:
            case tx_SETWVN:
				callbackOp(h, escop);
				break;
			default:
				return t2cErrInvalidOp;
				} 		/* End: switch (escop) */
			}			/* End: case tx_escape: */
			break;
		case tx_endchar:
			return 0;
		case t2_blend:
            {
                int result = 0;
                h->flags &= SEEN_CFF2_BLEND;
                if (h->stack.numRegions == 0)
                    setNumMasters(h);
                result = handleBlend(h);
                if (result)
                    return result;
            }
            continue;
		case t2_rcurveline:
		case t2_rlinecurve:
		case t2_vvcurveto:
		case t2_hhcurveto:
		case t2_callgsubr:
		case tx_vhcurveto:
		case tx_hvcurveto:
			callbackOp(h, byte0);
			break;
		case t2_shortint:
			/* 3 byte number */
			CHKOFLOW(1);
			{
			short value;
			CHKSUBRBYTE(h);
			value = *next++;
			CHKSUBRBYTE(h);
			value = value<<8 | *next++;
#if SHRT_MAX > 32767
			/* short greater that 2 bytes; handle negative range */
			if (value > 32767)
				value -= 65536;
#endif
			PUSH(value);
			}
			continue;
		default:
			/* 1 byte number */
			CHKOFLOW(1);
			PUSH(byte0 - 139);
			continue;
		case 247: case 248: case 249: case 250:
			/* Positive 2 byte number */
			CHKOFLOW(1);
			CHKSUBRBYTE(h);
			PUSH(108 + 256*(byte0 - 247) + *next);
            next++;
			continue;
		case 251: case 252: case 253: case 254:
			/* Negative 2 byte number */
			CHKOFLOW(1);
			CHKSUBRBYTE(h);
			PUSH(-108 - 256*(byte0 - 251) - *next);
            next++;
			continue;
		case 255:
			/* 5 byte number */
			CHKOFLOW(1);
			{
			long value;
			CHKSUBRBYTE(h);
			value = *next++;
			CHKSUBRBYTE(h);
			value = value<<8 | *next++;
			CHKSUBRBYTE(h);
			value = value<<8 | *next++;
			CHKSUBRBYTE(h);
			value = value<<8 | *next++;
#if LONG_MAX > 2147483647
			/* long greater that 4 bytes; handle negative range */
			if (value > 2417483647)
				value -= 4294967296;
#endif
			PUSH(value/65536.0);
			}
			continue;
			} 				/* End: switch (byte0) */
        clearBlendStack(h);
		h->stack.cnt = 0;	/* Clear stack */
		} 					/* End: while (cstr < end) */
	}

/* Parse Type 2 charstring. */
int t2cParse(long offset, long endOffset, t2cAuxData *aux, abfGlyphCallbacks *glyph, ctlMemoryCallbacks *mem)
	{
	struct t2cCtx h;
	int retVal;
	/* Initialize */
	h.flags = PEND_WIDTH|PEND_MASK;
	h.stack.cnt = 0;
	h.x = 0;
	h.y = 0;
	h.cubeStackDepth = -1;
	h.subrDepth = 0;
	h.stems.cnt = 0;
	h.mask.state = 0;
	h.mask.length = 0;
	h.seac.phase = seacNone;
	h.aux = aux;
	h.aux->WV[0] = 0.25f;	/* use center of design space by default */  
	h.aux->WV[1] = 0.25f;
	h.aux->WV[2] = 0.25f;
	h.aux->WV[3] = 0.25f;
    h.glyph = glyph;
    h.mem = mem;
	h.LanguageGroup = (glyph->info->flags & ABF_GLYPH_LANG_1) != 0;
	h.maxOpStack = (aux->flags & T2C_IS_CUBE) ? TX_MAX_OP_STACK_CUBE : T2_MAX_OP_STACK;
	aux->bchar = 0;
	aux->achar = 0;

	if (aux->flags & T2C_USE_MATRIX)
    {
        int i;
        if ((fabs(1 - aux->matrix[0]) > 0.0001) ||
            (fabs(1 - aux->matrix[3]) > 0.0001) ||
            (aux->matrix[1] != 0) ||
            (aux->matrix[2] != 0) ||
            (aux->matrix[4] != 0) ||
            (aux->matrix[5] != 0))
        {
            h.flags |= USE_MATRIX;
            h.flags |= USE_GLOBAL_MATRIX;
            for (i = 0; i < 6; i++)
            {
                h.transformMatrix[i] = aux->matrix[i];
            }
        }
    }
    if (aux->flags & T2C_FLATTEN_CUBE)
        h.flags |= FLATTEN_CUBE;
    if (aux->flags & T2C_FLATTEN_BLEND)
        h.flags |= FLATTEN_BLEND;
	if (aux->flags & T2C_IS_CUBE)
		h.flags |= IS_CUBE;
	if (aux->flags & T2C_CUBE_RND)
		h.flags |= CUBE_RND;
	h.src.endOffset = endOffset;
	if (aux->flags & T2C_CUBE_GSUBR)
		retVal = t2DecodeSubr(&h, offset);
	else
		retVal = t2Decode(&h, offset);

	// Clear path=specifc transform if last over from last path of the charstring,
	if 	(h.flags & USE_MATRIX)
		h.flags &= ~USE_MATRIX;

	return retVal;
	}

/* Get version numbers of libraries. */
void t2cGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<T2C_LIB_ID)
		return;	/* Already enumerated */

	/* This library */
	cb->getversion(cb, T2C_VERSION, "t2cstr");

	/* Record this call */
	cb->called |= 1<<T2C_LIB_ID;
	}

/* Convert error code to error string. */
char *t2cErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "t2cerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
    }
