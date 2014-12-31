/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Compact Font Format (CFF) parser. (Standalone library.)
 */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "cffread.h"
#include "dictops.h"

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CDECL
#define CDECL
#endif

/* Predefined encodings */
static short stdenc[] = {
#include "stdenc0.h"
};
static short exenc[] = {
#include "exenc0.h"
};

#ifndef NULL
#define NULL 0
#endif

#define ARRAY_LEN(t)    (sizeof(t) / sizeof((t)[0]))
#define ABS(v)            ((v) < 0 ? -(v) : (v))

/* 16.16 fixed point arithmetic */
typedef long Fixed;
#define fixedScale    65536.0
#define FixedMax    ((Fixed)0x7FFFFFFF)
#define FixedMin    ((Fixed)0x80000000)
#define FixedOne    ((Fixed)0x00010000)
#define FixedHalf    ((Fixed)0x00008000)

/* Type conversions */
#define INT(f)        (((f) & 0xffff0000) >> 16)
#define FRAC(f)        ((f) & 0x0000ffff)
#define INT2FIX(i)    ((Fixed)(i) << 16)
#define FIX2INT(f)    ((short)(((f) + FixedHalf) >> 16))
#define FIX2DBL(f)    ((double)((f) / fixedScale))
#define DBL2FIX(d)    ((Fixed)((d) * fixedScale + ((d) < 0 ? -0.5 : 0.5)))
#define DBL2INT(d)    ((long)((d) + ((d) < 0 ? -0.5 : 0.5)))
#define CEIL(f)        ((Fixed)(((f) + 0xffff) & 0xffff0000))
#define FLOOR(f)    ((Fixed)((f) & 0xffff0000))
#define PATH_FUNC_DEFINED(func) (h->path.cb != NULL && h->path.cb->func != NULL)
#define PATH_FUNC_CALL(func, args) \
	do { if (PATH_FUNC_DEFINED(func)) { h->path.cb->func args; } \
	} \
	while (0)

typedef unsigned char OffSize;
/* Offset size indicator */
typedef long Offset;            /* 1, 2, 3, or 4-byte offset */
#define SID_SIZE    2            /* SID byte size */

typedef struct {              /* INDEX */
	unsigned short count;
	/* Element count */
	OffSize offSize;
	/* Offset size */
	Offset offset;
	/* Offset start */
	Offset data;
	/* Data start - 1 */
	unsigned bias;                /* Subr number bias */
} INDEX;

typedef struct {                  /* FDArray element */
	INDEX Subrs;
	Fixed defaultWidthX;
	Fixed nominalWidthX;
	double *FontMatrix;
	/* FD FontMatrix (NULL if default) */
	double FDMatrix[6];
} FDElement;

typedef union {                   /* Stack element */
	double d;
	Fixed f;
	long l;
} StkElement;

typedef enum {                   /* Stack element type */
	STK_DOUBLE,
	STK_FIXED,
	STK_LONG
} StkType;

typedef struct {                  /* Per-glyph data */
	unsigned short id;
	/* SID/CID */
	short code;
	/* Encoding */
	short fd;
	/* FD index */
	cffSupCode *sup;            /* Supplementary encodings (linked list) */
} Glyph;

struct cffCtx_ {                  /* Context */
	short flags;                /* Parse flags */
#define MM_FONT        (1 << 0)        /* MM Font */
#define CID_FONT    (1 << 1)        /* CID Font */
#define    FONT_INIT    (1 << 8)        /* Global font data has been parsed */
#define    GLYPHS_INIT    (1 << 9)        /* Global glyph data has been parsed */
#define    UDV_SET        (1 << 10)        /* UDV explicitly set */
#define    WV_SET        (1 << 11)        /* WV explicitly set */
#define    JUST_WIDTH    (1 << 12)        /* Stop parse after width found */
	struct {                       /* CFF data */
		char *next;
		/* Next byte available */
		long left;
		/* Number of bytes available */
		Offset nextoff;            /* Offset of next input buffer */
	} data;
	struct {                       /* CFF header */
		unsigned char major;
		unsigned char minor;
		unsigned char hdrSize;
		OffSize offSize;
	} hdr;
	struct {                        /* INDEXes */
		INDEX name;
		INDEX top;
		INDEX string;
		INDEX global;
		INDEX CharStrings;
		INDEX Subrs;
		INDEX FDArray;
	} index;
	cffFontInfo font;
	/* Font descriptor (returned to client) */
	cffGlyphInfo glyph;
	/* Glyph descriptor (returned to client) */
	struct {                        /* DICT values */
		Offset Encoding;
		Offset charset;
		Offset CharStrings;
		struct {
			unsigned long length;
			Offset offset;
		} Private;
		Offset Subrs;
		Offset FDArray;
		Offset FDSelect;
		Fixed defaultWidthX;
		Fixed nominalWidthX;
		struct {                   /* Vertical origin vector */
			Fixed x;
			Fixed y;
		} vOrig;
		FDElement *fd;
		/* Active FD (NULL indicates top DICT) */
		double *FontMatrix;
		/* Active matrix (NULL indicates default) */
		double topDICTMatrix[6]; /* FontMatrix read from top DICT */
	}
	dict;
	struct {                       /* DICT/charstring operand stack */
		int cnt;
		int max;
		StkElement *array;
		char *type;
	} stack;
	struct {                       /* Glyph outline path */
		short flags;
#define GET_WIDTH    (1 << 0)        /* Flags to get width value */
#define FIRST_MOVE    (1 << 1)        /* Flags first move */
#define FIRST_MASK    (1 << 2)        /* Flags first hint/cntrmask */
#define DICT_CSTR    (1 << 3)        /* Flags DICT charstring */
#define SEEN_END    (1 << 4)        /* Flags endchar operator seen */
		short nStems;
		/* Stem count */
		Fixed hstem;
		/* Horizontal hint stem accumulator */
		Fixed vstem;
		/* Vertical hint stem accumulator */
		Fixed hAdv;
		/* Horizontal advance width */
		Fixed vAdv;
		/* Vertical advance width */
		Fixed x;
		/* Current x-coord */
		Fixed y;
		/* Current y-coord */
		Fixed left;
		/* BBox */
		Fixed bottom;
		/* BBox */
		Fixed right;
		/* BBox */
		Fixed top;
		/* BBox */
		cffPathCallbacks *cb;    /* Path callback functions (if defined) */
	} path;
	unsigned short stdGlyphs[256];
	/* Standard encoded GIDs indexed by code */
	Glyph *glyphs;
	/* Glyph names and encoding */
	struct {                       /* BuildCharArray */
		int size;
		Fixed *array;
	} BCA;
	Fixed WV[TX_MAX_MASTERS];
	/* Weight vector */
	Fixed NDV[TX_MAX_AXES];
	/* Normalized design vector */
	Fixed UDV[TX_MAX_AXES];
	/* User design vector */
	FDElement *fd;
	/* FD info array */
	short lastfd;
	/* Last FD index */
	Offset start;
	/* CFF data offset */
	cffStdCallbacks cb;

	/* Standard callback functions */
	void (*cstrRead)(cffCtx h, Offset offset, int init);
};

enum {           /* Charstring initialization types for use with cstrRead() */
	cstr_NONE,
	cstr_GLYPH,
	cstr_DICT,
	cstr_METRIC
};

static void t2Read(cffCtx h, Offset offset, int init);

static void t13Read(cffCtx h, Offset offset, int init);

static int t13Support(cffCtx h);

#define MEM_NEW(h, s)    (h)->cb.malloc((h)->cb.ctx, (s))
#define MEM_FREE(h, p)    (h)->cb.free((h)->cb.ctx, (p))

/* Create new instance */
cffCtx cffNew(cffStdCallbacks *cb, Offset offset) {
	cffCtx h = cb->malloc(cb->ctx, sizeof(struct cffCtx_));

	if (h == NULL) {
		if (cb->message != NULL) {
			cb->message(cb->ctx, cffFATAL, "out of memory");
		}
		cb->fatal(cb->ctx);
	}

	/* Initialize */
	h->cb = *cb;
	h->flags = 0;

	h->font.version = CFF_SID_UNDEF;
	h->font.Notice = CFF_SID_UNDEF;
	h->font.Copyright = CFF_SID_UNDEF;
	h->font.FamilyName = CFF_SID_UNDEF;
	h->font.FullName = CFF_SID_UNDEF;
	h->font.FontBBox.left = 0;
	h->font.FontBBox.bottom = 0;
	h->font.FontBBox.right = 0;
	h->font.FontBBox.top = 0;
	h->font.unitsPerEm = 1000;
	h->font.isFixedPitch = 0;
	h->font.ItalicAngle = 0;
	h->font.UnderlinePosition = -100;
	h->font.UnderlineThickness = 50;
	h->font.mm.nAxes = 0;
	h->font.mm.nMasters = 1;
	h->font.mm.lenBuildCharArray = 0;
	h->font.mm.NDV = CFF_SID_UNDEF;
	h->font.mm.CDV = CFF_SID_UNDEF;
	h->font.cid.version = 0;
	h->font.cid.registry = CFF_SID_UNDEF;
	h->font.cid.vOrig.x = 500;
	h->font.cid.vOrig.y = 880;

	h->dict.Encoding = CFF_STD_ENC;
	h->dict.charset = CFF_ISO_CS;
	h->dict.CharStrings = 0;
	h->dict.Private.length = 0;
	h->dict.Subrs = 0;
	h->dict.FDArray = 0;
	h->dict.FDSelect = 0;
	h->dict.defaultWidthX = 0;
	h->dict.nominalWidthX = 0;
	h->dict.FontMatrix = NULL;
	h->dict.fd = NULL;

	/* Initialize for Type 2 charstrings */
	h->stack.max = T2_MAX_OP_STACK;
	h->stack.array = MEM_NEW(h, T2_MAX_OP_STACK * sizeof(StkElement));
	h->stack.type = MEM_NEW(h, T2_MAX_OP_STACK);
	h->cstrRead = t2Read;

	memset(h->stdGlyphs, 0, sizeof(h->stdGlyphs));
	h->BCA.array = NULL;
	h->fd = NULL;
	h->lastfd = -1;
	h->glyphs = NULL;
	h->start = offset;
	h->path.cb = NULL;

	return h;
}

/* Free instance */
void cffFree(cffCtx h) {
	if (h->fd != NULL) {
		MEM_FREE(h, h->fd);
	}
	if (h->BCA.array != NULL) {
		MEM_FREE(h, h->BCA.array);
	}
	if (h->glyphs != NULL) {
		int i;
		for (i = 0; i < h->index.CharStrings.count; i++) {
			cffSupCode *sup = h->glyphs[i].sup;
			while (sup != NULL) {
				cffSupCode *next = sup->next;
				MEM_FREE(h, sup);
				sup = next;
			}
		}
		MEM_FREE(h, h->glyphs);
	}
	MEM_FREE(h, h->stack.array);
	MEM_FREE(h, h->stack.type);
	MEM_FREE(h, h);
}

/* Quit on error */
static void fatal(cffCtx h, char *reason) {
	void *ctx = h->cb.ctx;
	void (*fatal)(void *ctx) = h->cb.fatal;

	if (h->cb.message) {
		h->cb.message(h->cb.ctx, cffFATAL, reason);
	}
	cffFree(h);    /* Free cff lib resources */

	fatal(ctx);
}

/* The stack is used for both dictionary and charstring parsing. All charstring
   stack operations are performed using the fixed point type as are most
   dictionary stack operations. However, dictionaries may specify long and BCD
   operands that cannot be represented accurately in fixed point. Therefore,
   a stack element is capable of storing all 3 types of operand: BCD, fixed,
   and long, and the following functions deal with storing and retrieving these
   types to and from stack elements. */

/* Push double number on stack (as double) */
static void pushDbl(cffCtx h, double d) {
	if (h->stack.cnt >= h->stack.max) {
		fatal(h, "stack overflow");
	}
	h->stack.array[h->stack.cnt].d = d;
	h->stack.type[h->stack.cnt++] = STK_DOUBLE;
}

/* Push fixed point number on stack (as fixed) */
static void pushFix(cffCtx h, Fixed f) {
	if (h->stack.cnt >= h->stack.max) {
		fatal(h, "stack overflow");
	}
	h->stack.array[h->stack.cnt].f = f;
	h->stack.type[h->stack.cnt++] = STK_FIXED;
}

/* Push integer number on stack (as fixed) */
static void pushInt(cffCtx h, long l) {
	if (h->stack.cnt >= h->stack.max) {
		fatal(h, "stack overflow");
	}
	h->stack.array[h->stack.cnt].f = INT2FIX(l);
	h->stack.type[h->stack.cnt++] = STK_FIXED;
}

/* Push long number on stack (as long) */
static void pushLong(cffCtx h, long l) {
	if (h->stack.cnt >= h->stack.max) {
		fatal(h, "stack overflow");
	}
	h->stack.array[h->stack.cnt].l = l;
	h->stack.type[h->stack.cnt++] = STK_LONG;
}

/* Pop number from stack (return double) */
static double popDbl(cffCtx h) {
	if (h->stack.cnt < 1) {
		fatal(h, "stack underflow");
	}
	switch (h->stack.type[--h->stack.cnt]) {
		case STK_DOUBLE:
			return h->stack.array[h->stack.cnt].d;

		case STK_FIXED:
			return FIX2DBL(h->stack.array[h->stack.cnt].f);

		case STK_LONG:
			return h->stack.array[h->stack.cnt].l;
	}
	return 0;    /* For quiet compilation */
}

/* Pop number from stack (return fixed) */
static Fixed popFix(cffCtx h) {
	if (h->stack.cnt < 1) {
		fatal(h, "stack underflow");
	}
	switch (h->stack.type[--h->stack.cnt]) {
		case STK_DOUBLE: {
			double d = h->stack.array[h->stack.cnt].d;
			if (d < FIX2DBL(FixedMin) || d > FIX2DBL(FixedMax)) {
				fatal(h, "range check\n");
			}
			else {
				return DBL2FIX(d);
			}
		}

		case STK_FIXED:
			return h->stack.array[h->stack.cnt].f;

		case STK_LONG: {
			unsigned long l = h->stack.array[h->stack.cnt].l;
			if (l < INT(FixedMin) || l > INT(FixedMax)) {
				fatal(h, "range check\n");
			}
			else {
				return INT2FIX(l);
			}
		}
	}
	return 0;    /* For quiet compilation */
}

/* Pop number from stack (return long) */
static long popInt(cffCtx h) {
	if (h->stack.cnt < 1) {
		fatal(h, "stack underflow");
	}
	switch (h->stack.type[--h->stack.cnt]) {
		case STK_DOUBLE: {
			double d = h->stack.array[h->stack.cnt].d;
			if (d < LONG_MIN || d > LONG_MAX) {
				fatal(h, "range check\n");
			}
			else {
				return DBL2INT(d);
			}
		}

		case STK_FIXED:
			return FIX2INT(h->stack.array[h->stack.cnt].f);

		case STK_LONG:
			return h->stack.array[h->stack.cnt].l;
	}
	return 0;    /* For quiet compilation */
}

/* Return indexed stack element (as fixed) */
static Fixed indexFix(cffCtx h, int i) {
	if (i < 0 || i >= h->stack.cnt) {
		fatal(h, "stack check");
	}
	switch (h->stack.type[i]) {
		case STK_DOUBLE: {
			double d = h->stack.array[i].d;
			if (d < FIX2DBL(FixedMin) || d > FIX2DBL(FixedMax)) {
				fatal(h, "range check\n");
			}
			else {
				return DBL2FIX(d);
			}
		}

		case STK_FIXED:
			return h->stack.array[i].f;

		case STK_LONG:
			return INT2FIX(h->stack.array[i].l);
	}
	return 0;    /* For quiet compilation */
}

/* Return indexed stack element (as long) */
static long indexInt(cffCtx h, int i) {
	if (i < 0 || i >= h->stack.cnt) {
		fatal(h, "stack check");
	}
	switch (h->stack.type[i]) {
		case STK_DOUBLE: {
			double d = h->stack.array[i].d;
			if (d < LONG_MIN || d > LONG_MAX) {
				fatal(h, "range check\n");
			}
			else {
				return DBL2INT(d);
			}
		}

		case STK_FIXED:
			return FIX2INT(h->stack.array[i].f);

		case STK_LONG:
			return h->stack.array[i].l;
	}
	return 0;    /* For quiet compilation */
}

/* Return indexed stack element (as double) */
static double indexDbl(cffCtx h, int i) {
	if (i < 0 || i >= h->stack.cnt) {
		fatal(h, "stack check");
	}
	switch (h->stack.type[i]) {
		case STK_DOUBLE:
			return h->stack.array[i].d;

		case STK_FIXED:
			return FIX2DBL(h->stack.array[i].f);

		case STK_LONG:
			return h->stack.array[i].l;
	}
	return 0;    /* For quiet compilation */
}

/* Replenish data buffer using refill function and handle end of data */
static char fillbuf(cffCtx h) {
	h->data.next = h->cb.cffRefill(h->cb.ctx, &h->data.left);
	h->data.nextoff += h->data.left;
	if (h->data.left-- == 0) {
		fatal(h, "premature end of data");
	}
	return *h->data.next++;
}

/* Seek to data byte */
static void seekbyte(cffCtx h, Offset offset) {
	h->data.next = h->cb.cffSeek(h->cb.ctx, offset, &h->data.left);
	if (h->data.left == 0) {
		fatal(h, "premature end of data");
	}
	h->data.nextoff = offset + h->data.left;
}

/* Report current data offset */
#define TELL(h)        ((h)->data.nextoff - (h)->data.left)

/* Get input byte */
#define GETBYTE(h)    \
	((unsigned char)(((h)->data.left-- == 0) ? fillbuf(h) : *(h)->data.next++))

/* Get integer of specified size starting at buffer pointer */
static unsigned long getnum(cffCtx h, int size) {
	unsigned long value;
	switch (size) {
		case 4:
			value = (unsigned long)GETBYTE(h) << 24;
			value |= (unsigned long)GETBYTE(h) << 16;
			value |= GETBYTE(h) << 8;
			return value | GETBYTE(h);

		case 3:
			value = (unsigned long)GETBYTE(h) << 16;
			value |= GETBYTE(h) << 8;
			return value | GETBYTE(h);

		case 2:
			value = GETBYTE(h) << 8;
			return value | GETBYTE(h);

		case 1:
			return GETBYTE(h);
	}
	return 0;    /* Suppress compiler warning */
}

/* Read INDEX structure parameters and return offset of byte following the
   INDEX. */
static Offset INDEXRead(cffCtx h, INDEX *index, Offset offset) {
	Offset last;

	seekbyte(h, offset);

	index->count = (unsigned short)getnum(h, 2);     /* Get count */

	if (index->count == 0) {
		return TELL(h);
	}

	index->offSize = GETBYTE(h);    /* Get offset size */
	index->offset = TELL(h);        /* Get offset array base */

	/* Get last offset */
	seekbyte(h, index->offset + index->count * index->offSize);
	last = getnum(h, index->offSize);

	index->data = TELL(h) - 1;        /* Set data reference */

	return index->data + last;
}

/* Get an element (offset and length) from INDEX structure */
static Offset INDEXGet(cffCtx h, INDEX *index,
                       unsigned element, unsigned *length) {
	Offset offset;

	if (index->count < element) {
		fatal(h, "INDEX bounds");    /* Requested element out-of-bounds */
	}
	/* Get offset */
	seekbyte(h, index->offset + (unsigned long)element * index->offSize);
	offset = getnum(h, index->offSize);

	/* Compute length */
	*length = getnum(h, index->offSize) - offset;

	return index->data + offset;
}

/* Get of string corresponding to SID from String INDEX. Return 1 if SID
   represented a standard string and 0 for custom strings. Standard strings are
   returned as a pointer via the ptr parameter and custom strings are returned
   as an data offset that the client must convert to a data pointer. In both
   cases the length is returned via the length parameter */
int cffGetString(cffCtx h, cffSID sid,
                 unsigned *length, char **ptr, long *offset) {
	static char *std[] = {
#include "stdstr1.h"
	};
#define STD_STR_CNT ARRAY_LEN(std)

	if (sid < STD_STR_CNT) {
		*length = strlen(std[sid]);
		*ptr = std[sid];
		return 1;
	}
	else {
		*offset = INDEXGet(h, &h->index.string, sid - STD_STR_CNT, length);
		return 0;
	}
}

/* Get NDV/CDV charstring offset */
static Offset getProcOffset(cffCtx h, cffSID sid, unsigned *length) {
	char *ptr;
	long offset;
	if (cffGetString(h, sid, length, &ptr, &offset)) {
		fatal(h, "bad NDV/CDV proc");
	}
	return offset;
}

/* Set bounds for seac component */
static void setComponentBounds(cffCtx h, int accent,
                               int code, Fixed x, Fixed y) {
	unsigned length;
	unsigned gid = h->stdGlyphs[code];

	if (gid == 0) {
		fatal(h, "bad seac\n");
	}

	h->stack.cnt = 0;
	h->path.nStems = 0;

	if (accent) {
		/* Accent component */
		h->path.flags = FIRST_MASK;

		/* Set component origin */
		h->path.x = x;
		h->path.y = y;
	}
	else {
		/* Base component */
		h->path.flags = FIRST_MOVE | FIRST_MASK;
	}

	h->cstrRead(h,
	            INDEXGet(h, &h->index.CharStrings, gid, &length), cstr_NONE);
}

/* Compute and save glyph advances */
static int setAdvance(cffCtx h) {
	h->path.hAdv = (h->stack.cnt & 1) ?
	    indexFix(h, 0) + h->dict.nominalWidthX : h->dict.defaultWidthX;
	h->path.vAdv = INT2FIX(-1000);    /* xxx set from VMetrics */
	if (h->dict.FontMatrix != NULL) {
		/* Adjust values */
		h->path.hAdv = DBL2FIX(h->dict.FontMatrix[0] * FIX2DBL(h->path.hAdv));
		h->path.vAdv = DBL2FIX(h->dict.FontMatrix[3] * FIX2DBL(h->path.vAdv));
	}
	h->path.flags &= ~GET_WIDTH;
	if (h->flags & JUST_WIDTH) {
		h->path.flags |= SEEN_END;
		return 1;
	}
	else {
		return 0;
	}
}

/* Check point against current bounds */
static void checkPoint(cffCtx h, Fixed x, Fixed y) {
	if (x < h->path.left) {
		h->path.left = x;
	}
	else if (x > h->path.right) {
		h->path.right = x;
	}
	if (y < h->path.bottom) {
		h->path.bottom = y;
	}
	else if (y > h->path.top) {
		h->path.top = y;
	}
}

/* Find bound(s) on Bezier curve.

   This function operates by flattening the curve into lines and submitting the
   resulting line end-points for bounds checking. The curve is flattened by
   recursively subdividing the curve into two curves joined at a common
   point. For a curve specified by the points: x0, y0, x1, y1, x2, y2, x3, y3
   the points on the subdivided curves are given by:

   x0,                            y0,
   (x0 + x1) / 2,                 (y0 + y1) / 2,
   (x0 + 2 * x1 + x2) / 4,        (y0 + 2 * y1 + y2) / 4,
   (x0 + 3 * (x1 + x2) + x3) / 8, (y0 + 3 * (y1 + y2) + y3) / 8,

   and:

   (x0 + 3 * (x1 + x2) + x3) / 8, (y0 + 3 * (y1 + y2) + y3) / 8,
   (x1 + 2 * x2 + x3) / 4,        (y1 + 2 * y2 + y3) / 4,
   (x2 + x3) / 2,				  (y2 + y3) / 2,
   x3,							  y3,

   The stopping condition is governed by the bounding box on the curve
   baseline joining x0, y0 to x3, y3. If the control points fall within this
   bounding box (expanded by half a unit in all directions) then the curve's
   maximum or minimum must be within half a unit of the end points. Therefore,
   in this case, the end points need only be tested and no further subdivision
   is necessary. This is rather efficient since subdivision is sparse except in
   the regions of the curve close to maximums or minimums. In particular this
   stopping condition is much more efficient that subdivision for drawing
   curves where subdivision density is fairly evenly spread.

   One further optimization can be made by observing that only one of the end
   points needs testing after the splitting terminates because the other end
   point will be tested in the other subdivided curve. This is true except for
   one end of the first curve which must be tested outside of this function.
 */
static void flatten(cffCtx h,
                    Fixed x0, Fixed y0, Fixed x1, Fixed y1,
                    Fixed x2, Fixed y2, Fixed x3, Fixed y3) {
	for (;; ) {
		Fixed left;
		Fixed bottom;
		Fixed right;
		Fixed top;

		/* Compute end-point bounding box */
		if (x0 < x3) {
			left = x0 - FixedHalf;
			right = x3 + FixedHalf;
		}
		else {
			left = x3 - FixedHalf;
			right = x0 + FixedHalf;
		}
		if (y0 < y3) {
			bottom = y0 - FixedHalf;
			top = y3 + FixedHalf;
		}
		else {
			bottom = y3 - FixedHalf;
			top = y0 + FixedHalf;
		}

		if (x1 <= left || x1 >= right || y1 <= bottom || y1 >= top ||
		    x2 <= left || x2 >= right || y2 <= bottom || y2 >= top) {
			/* Control points outside of end-point bounding box; split curve */
			Fixed sx = (x2 + x3) / 2;
			Fixed sy = (y2 + y3) / 2;
			Fixed tx = (x1 + x2) / 2;
			Fixed ty = (y1 + y2) / 2;
			Fixed ux = (x0 + x1) / 2;
			Fixed uy = (y0 + y1) / 2;
			Fixed cx = (sx + 2 * tx + ux) / 4;
			Fixed cy = (sy + 2 * ty + uy) / 4;

			/* Compute first curve and flatten */
			flatten(h, x0, y0, ux, uy, (tx + ux) / 2, (ty + uy) / 2, cx, cy);

			/* Compute second curve (x3, y3 unchanged) */
			x0 = cx;
			y0 = cy;
			x1 = (sx + tx) / 2;
			y1 = (sy + ty) / 2;
			x2 = sx;
			y2 = sy;
		}
		else {
			/* Check end point */
			checkPoint(h, x3, y3);
			return;
		}
	}
}

/* Transform x-coord by by FontMatrix */
static Fixed transX(cffCtx h, Fixed x, Fixed y) {
	return DBL2FIX(h->dict.FontMatrix[0] * FIX2DBL(x) +
	               h->dict.FontMatrix[2] * FIX2DBL(y) +
	               h->dict.FontMatrix[4]);
}

/* Transform y-coord by by FontMatrix */
static Fixed transY(cffCtx h, Fixed x, Fixed y) {
	return DBL2FIX(h->dict.FontMatrix[1] * FIX2DBL(x) +
	               h->dict.FontMatrix[3] * FIX2DBL(y) +
	               h->dict.FontMatrix[5]);
}

/* Add point to path */
static void addPoint(cffCtx h, Fixed dx, Fixed dy) {
	/* Update path */
	h->path.x += dx;
	h->path.y += dy;

	if (h->dict.FontMatrix != NULL) {
		/* Check transformed point */
		checkPoint(h,
		           transX(h, h->path.x, h->path.y),
		           transY(h, h->path.x, h->path.y));
	}
	else {
		checkPoint(h, h->path.x, h->path.y);
	}
}

/* Add line to path */
static void addLine(cffCtx h, Fixed dx, Fixed dy) {
	addPoint(h, dx, dy);
	if (h->dict.FontMatrix != NULL) {
		PATH_FUNC_CALL(lineto, (h->cb.ctx,
		                        transX(h, h->path.x, h->path.y),
		                        transY(h, h->path.x, h->path.y)));
	}
	else {
		PATH_FUNC_CALL(lineto, (h->cb.ctx, h->path.x, h->path.y));
	}
}

/* Add curve to path */
static void addCurve(cffCtx h, int flex, Fixed dx1, Fixed dy1,
                     Fixed dx2, Fixed dy2, Fixed dx3, Fixed dy3) {
	Fixed x0 = h->path.x;
	Fixed y0 = h->path.y;
	Fixed x1 = x0 + dx1;
	Fixed y1 = y0 + dy1;
	Fixed x2 = x1 + dx2;
	Fixed y2 = y1 + dy2;
	Fixed x3 = h->path.x = x2 + dx3;
	Fixed y3 = h->path.y = y2 + dy3;

	if (h->dict.FontMatrix != NULL) {
		/* Transform curve */
		Fixed xt;
		xt = x0;
		x0 = transX(h, xt, y0);
		y0 = transY(h, xt, y0);
		xt = x1;
		x1 = transX(h, xt, y1);
		y1 = transY(h, xt, y1);
		xt = x2;
		x2 = transX(h, xt, y2);
		y2 = transY(h, xt, y2);
		xt = x3;
		x3 = transX(h, xt, y3);
		y3 = transY(h, xt, y3);
	}

	PATH_FUNC_CALL(curveto, (h->cb.ctx, flex, x1, y1, x2, y2, x3, y3));

	if (x1 < h->path.left || x1 > h->path.right ||
	    x2 < h->path.left || x2 > h->path.right ||
	    x3 < h->path.left || x3 > h->path.right ||
	    y1 < h->path.bottom || y1 > h->path.top ||
	    y2 < h->path.bottom || y2 > h->path.top ||
	    y3 < h->path.bottom || y3 > h->path.top) {
		/* Bounding trapezium extends outside bounding box; flatten curve */
		checkPoint(h, x0, y0);
		flatten(h, x0, y0, x1, y1, x2, y2, x3, y3);
	}
}

/* Add move to path */
static int addMove(cffCtx h, Fixed x, Fixed y) {
	if (h->path.flags & FIRST_MOVE) {
		if ((h->path.flags & GET_WIDTH) && setAdvance(h)) {
			return 1;    /* Stop after getting width */
		}
		h->path.x = x;
		h->path.y = y;
		h->path.flags = 0;

		/* Set bound to first point */
		if (h->dict.FontMatrix != NULL) {
			h->path.left = h->path.right = transX(h, x, y),
			h->path.bottom = h->path.top = transY(h, x, y);
		}
		else {
			h->path.left = h->path.right = x;
			h->path.bottom = h->path.top = y;
		}
		h->path.flags = 0;
	}
	else {
		addPoint(h, x, y);
		PATH_FUNC_CALL(closepath, (h->cb.ctx));
	}
	PATH_FUNC_CALL(newpath, (h->cb.ctx));
	if (h->dict.FontMatrix != NULL) {
		PATH_FUNC_CALL(moveto, (h->cb.ctx,
		                        transX(h, h->path.x, h->path.y),
		                        transY(h, h->path.x, h->path.y)));
	}
	else {
		PATH_FUNC_CALL(moveto, (h->cb.ctx, h->path.x, h->path.y));
	}
	return 0;
}

/* Select registry item */
static Fixed *selRegItem(cffCtx h, int reg, int *regSize) {
	switch (reg) {
		case TX_REG_WV:
			*regSize = TX_MAX_MASTERS;
			return h->WV;

		case TX_REG_NDV:
			*regSize = TX_MAX_AXES;
			return h->NDV;

		case TX_REG_UDV:
			*regSize = TX_MAX_AXES;
			return h->UDV;

		default:
			fatal(h, "unknown registry item\n");
	}
	return 0;    /* Suppress compiler warning */
}

/* Park and Miller RNG from the "C Programming FAQs". Returns Fixed point
   result in the range (0,1] */
static Fixed PMRand(void) {
#define A 48271
#define M 2147483647
#define Q (M / A)
#define R (M % A)
	static long seed = 1;
	long hi = seed / Q;
	long lo = seed % Q;
	long test = A * lo - R * hi;
	seed = (test > 0) ? test : test + M;
	return seed % FixedOne + 1;
#undef A
#undef M
#undef Q
#undef R
}

/* Fixed point (16.16) square root using the "binary restoring square root
   extraction" method. This function is adapted from the algorithm described at
   http://www.research.apple.com/research/reports/TR96/TR96.html . Curiously,
   the algorithm listed there doesn't round up the remainder which is required
   in order to obtain a result that is correct to the nearest bit. */
static Fixed FixedSqrt(Fixed radicand) {
	unsigned long radical = 0;
	unsigned long remHi = 0;
	unsigned long remLo = radicand;
	int count = 23;    /* = 15+FracBits/2; adjust for other precisions */

	if (radicand <= 0) {
		return 0;
	}

	do {
		unsigned long term;

		/* Get next 2 bits of the radicand */
		remHi = remHi << 2 | remLo >> 30;
		remLo <<= 2;

		radical <<= 1;    /* Prepare for next bit */

		/* Compute and test the next term */
		term = (radical << 1) + 1;
		if (remHi >= term) {
			remHi -= term;
			radical++;
		}
	}
	while (count-- != 0);

	if (remHi > radical) {
		radical++;    /* Round up remainder */
	}
	return radical;
}

/* Reverse stack elements between index i and j (i < j). */
static void reverse(cffCtx h, int i, int j) {
	while (i < j) {
		Fixed tmp = h->stack.array[i].f;
		h->stack.array[i++].f = h->stack.array[j].f;
		h->stack.array[j--].f = tmp;
	}
}

/* Blend stack values */
static void blendValues(cffCtx h) {
	int i;
	int n = popInt(h);
	int iBase = h->stack.cnt - n * h->font.mm.nMasters;
	int k = iBase + n;

	if (iBase < 0) {
		fatal(h, "bounds check (blend)\n");
	}

	for (i = 0; i < n; i++) {
		int j;
		double x = indexDbl(h, iBase + i);

		for (j = 1; j < h->font.mm.nMasters; j++) {
			x += indexDbl(h, k++) * FIX2DBL(h->WV[j]);
		}

		h->stack.array[iBase + i].f = DBL2FIX(x);
		/* Stack element type is Fixed */
	}
	h->stack.cnt = iBase + n;
}

/* Read Type 2 charstring. Initial seek to offset performed if offset != -1 */
static void t2Read(cffCtx h, Offset offset, int init) {
	unsigned length;

	switch (init) {
		case cstr_GLYPH:
			/* Initialize glyph charstring */
			h->stack.cnt = 0;
			h->path.nStems = 0;
			h->path.flags = GET_WIDTH | FIRST_MOVE | FIRST_MASK;
			break;

		case cstr_DICT:
			/* Initialize DICT charstring */
			h->path.flags = DICT_CSTR;
			break;

		case cstr_METRIC:
			/* Initialize metric charstring */
			h->stack.cnt = 0;
			h->path.flags = 0;
			break;
	}

	if (offset != -1) {
		/* Seek to position of charstring if not already positioned */
		seekbyte(h, offset);
	}

	for (;; ) {
		int j;
		Fixed a;
		Fixed b;
		int byte0 = GETBYTE(h);
		switch (byte0) {
			case tx_endchar:
				if (!(h->path.flags & DICT_CSTR)) {
					if (h->path.flags & FIRST_MOVE) {
						/* Non-marking glyph; set bounds */
						h->path.left = h->path.right = 0;
						h->path.bottom = h->path.top = 0;
					}
					if ((h->path.flags & GET_WIDTH) && setAdvance(h)) {
						return;    /* Stop after getting width */
					}
					if (h->stack.cnt > 1) {
						/* Process seac components */
						int base;
						int accent;
						j = (h->stack.cnt == 4) ? 0 : 1;
						a = indexFix(h, j++);
						b = indexFix(h, j++);
						base = indexInt(h, j++);
						accent = indexInt(h, j);
						setComponentBounds(h, 0, base, 0, 0);
						setComponentBounds(h, 1, accent, a, b);
					}
				}
				h->path.flags |= SEEN_END;
				PATH_FUNC_CALL(endchar, (h->cb.ctx));
				return;

			case tx_reserved0:
			case t2_reserved2:
			case t2_reserved9:
			case t2_reserved13:
			case t2_reserved15:
			case t2_reserved17:
				fatal(h, "reserved charstring op");

			case tx_rlineto:
				for (j = 0; j < h->stack.cnt; j += 2) {
					addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
				}
				h->stack.cnt = 0;
				break;

			case tx_hlineto:
			case tx_vlineto: {
				int horz = byte0 == tx_hlineto;
				for (j = 0; j < h->stack.cnt; j++) {
					if (horz++ & 1) {
						addLine(h, indexFix(h, j), 0);
					}
					else {
						addLine(h, 0, indexFix(h, j));
					}
				}
			}
				h->stack.cnt = 0;
				break;

			case tx_rrcurveto:
				for (j = 0; j < h->stack.cnt; j += 6) {
					addCurve(h, 0,
					         indexFix(h, j + 0), indexFix(h, j + 1),
					         indexFix(h, j + 2), indexFix(h, j + 3),
					         indexFix(h, j + 4), indexFix(h, j + 5));
				}
				h->stack.cnt = 0;
				break;

			case tx_callsubr: {
				Offset save = TELL(h);
				t2Read(h, INDEXGet(h, &h->index.Subrs, popInt(h) +
				                   h->index.Subrs.bias, &length), cstr_NONE);
				if (h->path.flags & SEEN_END) {
					return;        /* endchar operator terminated subr */
				}
				seekbyte(h, save);
			}
			break;

			case tx_return:
				return;

			case t2_rcurveline:
				for (j = 0; j < h->stack.cnt - 2; j += 6) {
					addCurve(h, 0,
					         indexFix(h, j + 0), indexFix(h, j + 1),
					         indexFix(h, j + 2), indexFix(h, j + 3),
					         indexFix(h, j + 4), indexFix(h, j + 5));
				}
				addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
				h->stack.cnt = 0;
				break;

			case t2_rlinecurve:
				for (j = 0; j < h->stack.cnt - 6; j += 2) {
					addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
				}
				addCurve(h, 0,
				         indexFix(h, j + 0), indexFix(h, j + 1),
				         indexFix(h, j + 2), indexFix(h, j + 3),
				         indexFix(h, j + 4), indexFix(h, j + 5));
				h->stack.cnt = 0;
				break;

			case t2_vvcurveto:
				if (h->stack.cnt & 1) {
					/* Add initial curve */
					addCurve(h, 0,
					         indexFix(h, 0), indexFix(h, 1),
					         indexFix(h, 2), indexFix(h, 3),
					         0, indexFix(h, 4));
					j = 5;
				}
				else {
					j = 0;
				}

				/* Add remaining curve(s) */
				for (; j < h->stack.cnt; j += 4) {
					addCurve(h, 0,
					         0, indexFix(h, j + 0),
					         indexFix(h, j + 1), indexFix(h, j + 2),
					         0, indexFix(h, j + 3));
				}
				h->stack.cnt = 0;
				break;

			case t2_hhcurveto:
				if (h->stack.cnt & 1) {
					/* Add initial curve */
					addCurve(h, 0,
					         indexFix(h, 1), indexFix(h, 0),
					         indexFix(h, 2), indexFix(h, 3),
					         indexFix(h, 4), 0);
					j = 5;
				}
				else {
					j = 0;
				}

				/* Add remaining curve(s) */
				for (; j < h->stack.cnt; j += 4) {
					addCurve(h, 0,
					         indexFix(h, j + 0), 0,
					         indexFix(h, j + 1), indexFix(h, j + 2),
					         indexFix(h, j + 3), 0);
				}
				h->stack.cnt = 0;
				break;

			case t2_callgsubr: {
				Offset save = TELL(h);
				t2Read(h, INDEXGet(h, &h->index.global, popInt(h) +
				                   h->index.global.bias, &length), cstr_NONE);
				if (h->path.flags & SEEN_END) {
					return;        /* endchar operator terminated subr */
				}
				seekbyte(h, save);
			}
			break;

			case tx_vhcurveto:
			case tx_hvcurveto: {
				int adjust = (h->stack.cnt & 1) ? 5 : 0;
				int horz = byte0 == tx_hvcurveto;

				/* Add initial curve(s) */
				for (j = 0; j < h->stack.cnt - adjust; j += 4) {
					if (horz++ & 1) {
						addCurve(h, 0,
						         indexFix(h, j + 0), 0,
						         indexFix(h, j + 1), indexFix(h, j + 2),
						         0, indexFix(h, j + 3));
					}
					else {
						addCurve(h, 0,
						         0, indexFix(h, j + 0),
						         indexFix(h, j + 1), indexFix(h, j + 2),
						         indexFix(h, j + 3), 0);
					}
				}

				if (j < h->stack.cnt) {
					/* Add last curve */
					if (horz & 1) {
						addCurve(h, 0,
						         indexFix(h, j + 0), 0,
						         indexFix(h, j + 1), indexFix(h, j + 2),
						         indexFix(h, j + 4), indexFix(h, j + 3));
					}
					else {
						addCurve(h, 0,
						         0, indexFix(h, j + 0),
						         indexFix(h, j + 1), indexFix(h, j + 2),
						         indexFix(h, j + 3), indexFix(h, j + 4));
					}
				}
				h->stack.cnt = 0;
			}
			break;

			case tx_rmoveto:
				b = popFix(h);
				a = popFix(h);
				if (addMove(h, a, b)) {
					return;    /* Stop after getting width */
				}
				h->stack.cnt = 0;
				break;

			case tx_hmoveto:
				if (addMove(h, popFix(h), 0)) {
					return;    /* Stop after getting width */
				}
				h->stack.cnt = 0;
				break;

			case tx_vmoveto:
				if (addMove(h, 0, popFix(h))) {
					return;    /* Stop after getting width */
				}
				h->stack.cnt = 0;
				break;

			case tx_hstem:
			case tx_vstem:
			case t2_hstemhm:
			case t2_vstemhm:
				if ((h->path.flags & GET_WIDTH) && setAdvance(h)) {
					return;    /* Stop after getting width */
				}
				h->path.nStems += h->stack.cnt / 2;
				if (PATH_FUNC_DEFINED(hintstem)) {
					int vert = byte0 == tx_vstem || byte0 == t2_vstemhm;
					b = vert ? h->path.vstem : h->path.hstem;
					for (j = h->stack.cnt & 1; j < h->stack.cnt; j += 2) {
						a = b + indexFix(h, j);
						b = a + indexFix(h, j + 1);
						h->path.cb->hintstem(h->cb.ctx, vert, a, b);
					}
					if (vert) {
						h->path.vstem = b;
					}
					else {
						h->path.hstem = b;
					}
				}
				h->stack.cnt = 0;
				break;

			case t2_hintmask:
			case t2_cntrmask: {
				int cnt;
				if ((h->path.flags & GET_WIDTH) && setAdvance(h)) {
					return; /* Stop after getting width */
				}
				if (h->path.flags & FIRST_MASK) {
					/* The vstem(hm) op may be omitted if stem list is followed by
					   a mask op. In this case count the additional stems */
					if (PATH_FUNC_DEFINED(hintstem)) {
						b = h->path.vstem;
						for (j = h->stack.cnt & 1; j < h->stack.cnt; j += 2) {
							a = b + indexFix(h, j);
							b = a + indexFix(h, j + 1);
							h->path.cb->hintstem(h->cb.ctx, 1, a, b);
						}
					}
					h->path.nStems += h->stack.cnt / 2;
					h->stack.cnt = 0;
					h->path.flags &= ~FIRST_MASK;
				}
				cnt = (h->path.nStems + 7) / 8;
				if (PATH_FUNC_DEFINED(hintmask)) {
					char mask[CFF_MAX_MASK_BYTES];
					for (j = 0; j < cnt; j++) {
						mask[j] = GETBYTE(h);
					}
					h->path.cb->hintmask(h->cb.ctx,
					                     byte0 == t2_cntrmask, cnt, mask);
				}
				else {
					while (cnt--) {
						(void)GETBYTE(h);     /* Discard mask bytes */
					}
				}
			}
			break;

			case tx_escape: {
				double x;
				double y;
				switch (tx_ESC(GETBYTE(h))) {
					case tx_dotsection:
						break;

					default:
					case t2_reservedESC1:
					case t2_reservedESC2:
					case t2_reservedESC6:
					case t2_reservedESC7:
					case t2_reservedESC16:
					case t2_reservedESC17:
					case tx_reservedESC25:
					case tx_reservedESC31:
					case tx_reservedESC32:
					case t2_reservedESC33:
					case t2_reservedESC19:
					case t2_cntron:
						fatal(h, "reserved charstring op");

					case tx_and:
						b = popFix(h);
						a = popFix(h);
						pushInt(h, a && b);
						break;

					case tx_or:
						b = popFix(h);
						a = popFix(h);
						pushInt(h, a || b);
						break;

					case tx_not:
						a = popFix(h);
						pushInt(h, !a);
						break;

					case tx_store: {
						int count = popInt(h);
						int i = popInt(h);
						int j = popInt(h);
						int iReg = popInt(h);
						int regSize;
						Fixed *reg = selRegItem(h, iReg, &regSize);
						if (i < 0 || i + count - 1 >= h->BCA.size ||
						    j < 0 || j + count - 1 >= regSize) {
							fatal(h, "bounds check (store)\n");
						}
						memcpy(&reg[j], &h->BCA.array[i], sizeof(Fixed) * count);
					}
					break;

					case tx_abs:
						a = popFix(h);
						pushFix(h, (a < 0) ? -a : a);
						break;

					case tx_add:
						b = popFix(h);
						a = popFix(h);
						pushFix(h, a + b);
						break;

					case tx_sub:
						b = popFix(h);
						a = popFix(h);
						pushFix(h, a - b);
						break;

					case tx_div:
						y = popDbl(h);
						x = popDbl(h);
						if (y == 0.0) {
							fatal(h, "divide by zero (div)");
						}
						pushFix(h, DBL2FIX(x / y));
						break;

					case tx_load: {
						int regSize;
						int count = popInt(h);
						int i = popInt(h);
						int iReg = popInt(h);
						Fixed *reg = selRegItem(h, iReg, &regSize);
						if (i < 0 || i + count - 1 >= h->BCA.size || count > regSize) {
							fatal(h, "bounds check (load)\n");
						}
						memcpy(&h->BCA.array[i], reg, sizeof(Fixed) * count);
					}
					break;

					case tx_neg:
						a = popFix(h);
						pushFix(h, -a);
						break;

					case tx_eq:
						b = popFix(h);
						a = popFix(h);
						pushInt(h, a == b);
						break;

					case tx_drop:
						(void)popFix(h);
						break;

					case tx_put: {
						int i = popInt(h);
						if (i < 0 || i >= h->BCA.size) {
							fatal(h, "bounds check (put)\n");
						}
						h->BCA.array[i] = popFix(h);
					}
					break;

					case tx_get: {
						int i = popInt(h);
						if (i < 0 || i >= h->BCA.size) {
							fatal(h, "bounds check (get)\n");
						}
						pushFix(h, h->BCA.array[i]);
					}
					break;

					case tx_ifelse: {
						Fixed v2 = popFix(h);
						Fixed v1 = popFix(h);
						Fixed s2 = popFix(h);
						Fixed s1 = popFix(h);
						pushFix(h, (v1 > v2) ? s2 : s1);
					}
					break;

					case tx_random:
						pushFix(h, PMRand());
						break;

					case tx_mul:
						y = popDbl(h);
						x = popDbl(h);
						pushFix(h, DBL2FIX(x * y));
						break;

					case tx_sqrt:
						pushFix(h, FixedSqrt(popFix(h)));
						break;

					case tx_dup:
						pushFix(h, h->stack.array[h->stack.cnt - 1].f);
						break;

					case tx_exch:
						b = popFix(h);
						a = popFix(h);
						pushFix(h, b);
						pushFix(h, a);
						break;

					case tx_index: {
						int i = popInt(h);
						if (i < 0) {
							i = 0;    /* Duplicate top element */
						}
						if (i >= h->stack.cnt) {
							fatal(h, "limit check (index)");
						}
						pushFix(h, h->stack.array[h->stack.cnt - 1 - i].f);
					}
					break;

					case tx_roll: {
						int j = popInt(h);
						int n = popInt(h);
						int iTop = h->stack.cnt - 1;
						int iBottom = h->stack.cnt - n;

						if (n < 0 || iBottom < 0) {
							fatal(h, "limit check (roll)");
						}

						/* Constrain j to [0,n) */
						if (j < 0) {
							j = n - (-j % n);
						}
						j %= n;

						reverse(h, iTop - j + 1, iTop);
						reverse(h, iBottom, iTop - j);
						reverse(h, iBottom, iTop);
					}
					break;

					case t2_hflex:
						addCurve(h, 1,
						         indexFix(h, 0), 0,
						         indexFix(h, 1), indexFix(h, 2),
						         indexFix(h, 3), 0);
						addCurve(h, 1,
						         indexFix(h, 4), 0,
						         indexFix(h, 5), -indexFix(h, 2),
						         indexFix(h, 6), 0);
						h->stack.cnt = 0;
						break;

					case t2_flex:
						addCurve(h, 1,
						         indexFix(h, 0), indexFix(h, 1),
						         indexFix(h, 2), indexFix(h, 3),
						         indexFix(h, 4), indexFix(h, 5));
						addCurve(h, 1,
						         indexFix(h, 6), indexFix(h, 7),
						         indexFix(h, 8), indexFix(h, 9),
						         indexFix(h, 10), indexFix(h, 11));
						h->stack.cnt = 0;
						break;

					case t2_hflex1: {
						Fixed dy1 = indexFix(h, 1);
						Fixed dy2 = indexFix(h, 3);
						Fixed dy5 = indexFix(h, 7);
						addCurve(h, 1,
						         indexFix(h, 0), dy1,
						         indexFix(h, 2), dy2,
						         indexFix(h, 4), 0);
						addCurve(h, 1,
						         indexFix(h, 5), 0,
						         indexFix(h, 6), dy5,
						         indexFix(h, 8), -(dy1 + dy2 + dy5));
					}
						h->stack.cnt = 0;
						break;

					case t2_flex1: {
						Fixed dx1 = indexFix(h, 0);
						Fixed dy1 = indexFix(h, 1);
						Fixed dx2 = indexFix(h, 2);
						Fixed dy2 = indexFix(h, 3);
						Fixed dx3 = indexFix(h, 4);
						Fixed dy3 = indexFix(h, 5);
						Fixed dx4 = indexFix(h, 6);
						Fixed dy4 = indexFix(h, 7);
						Fixed dx5 = indexFix(h, 8);
						Fixed dy5 = indexFix(h, 9);
						Fixed dx = dx1 + dx2 + dx3 + dx4 + dx5;
						Fixed dy = dy1 + dy2 + dy3 + dy4 + dy5;
						if (ABS(dx) > ABS(dy)) {
							dx = indexFix(h, 10);
							dy = -dy;
						}
						else {
							dx = -dx;
							dy = indexFix(h, 10);
						}
						addCurve(h, 1, dx1, dy1, dx2, dy2, dx3, dy3);
						addCurve(h, 1, dx4, dy4, dx5, dy5, dx, dy);
					}
						h->stack.cnt = 0;
						break;
				}
			}
			break;

			case t2_blend:
				blendValues(h);
				break;

			case t2_shortint: {
				/* 2 byte number */
				long byte1 = GETBYTE(h);
				pushInt(h, byte1 << 8 | GETBYTE(h));
			}
			break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* Positive 2 byte number */
				pushInt(h, 108 + 256 * (byte0 - 247) + GETBYTE(h));
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* Negative 2 byte number */
				pushInt(h, -108 - 256 * (byte0 - 251) - GETBYTE(h));
				break;

			case 255: {
				/* 5 byte number */
				long byte1 = GETBYTE(h);
				long byte2 = GETBYTE(h);
				long byte3 = GETBYTE(h);
				pushFix(h, byte1 << 24 | byte2 << 16 | byte3 << 8 | GETBYTE(h));
			}
			break;

			default:
				/* 1 byte number */
				pushInt(h, byte0 - 139);
				break;
		}
	}
}

/* Save font transformation matrix */
static void saveMatrix(cffCtx h) {
	int i;
	double max;
	double *FM;

	if (h->stack.cnt != 6) {
		fatal(h, "stack check");
	}

	/* Find the largest of a, b, c, and d */
	max = 0.0;
	for (i = 0; i < 4; i++) {
		double value = indexDbl(h, i);
		if (value < 0.0) {
			value = -value;
		}
		if (value > max) {
			max = value;
		}
	}
	if (max == 0.0) {
		fatal(h, "bad FontMatrix");
	}

	h->font.unitsPerEm = (unsigned short)DBL2INT(1.0 / max);

	if (h->dict.fd == NULL) {
		FM = h->dict.FontMatrix = h->dict.topDICTMatrix;
	}
	else {
		FM = h->dict.fd->FontMatrix = h->dict.fd->FDMatrix;
	}

	for (i = 0; i < 6; i++) {
		FM[i] = indexDbl(h, i) * h->font.unitsPerEm;
	}

	if (h->dict.fd != NULL && h->dict.FontMatrix != NULL) {
		/* Multiply top DICT matrix and FD matrix */
		double *A = h->dict.FontMatrix;
		double B[6];

		memcpy(B, FM, sizeof(B));

		FM[0] = A[0] * B[0] + A[1] * B[2];
		FM[1] = A[0] * B[1] + A[1] * B[3];
		FM[2] = A[2] * B[0] + A[3] * B[2];
		FM[3] = A[2] * B[1] + A[3] * B[3];
		FM[4] = A[4] * B[0] + A[5] * B[2] + B[4];
		FM[5] = A[4] * B[1] + A[5] * B[3] + B[5];
	}
}

/* Convert BCD number to double */
static double convBCD(cffCtx h) {
	char buf[64];
	int byte = 0;    /* Suppress optimizer warning */
	unsigned int i = 0;
	int count = 0;

	/* Construct string representation of number */
	for (;; ) {
		int nibble;

		if (count++ & 1) {
			nibble = byte & 0xf;
		}
		else {
			byte = GETBYTE(h);
			nibble = byte >> 4;
		}
		if (nibble == 0xf) {
			break;
		}
		else if (nibble == 0xd) {
			fatal(h, "reserved nibble code (BCD op)");
		}
		else if (i < sizeof(buf) - 2) {
			buf[i++] = "0123456789.EE?-?"[nibble];
			if (nibble == 0xc) {
				buf[i++] = '-';    /* Negative exponent */
			}
		}
		else {
			fatal(h, "BCD buffer overflow");
		}
	}
	buf[i] = '\0';

	return strtod(buf, NULL);
}

/* Read DICT data */
static void DICTRead(cffCtx h, int length, Offset offset, int enable) {
	if (length == 0) {
		fatal(h, "empty DICT");
	}

	seekbyte(h, offset);

	h->stack.cnt = 0;
	while (TELL(h) - offset < length) {
		int byte0 = GETBYTE(h);
		switch (byte0) {
			case cff_Weight:
			case cff_BlueValues:
			case cff_OtherBlues:
			case cff_FamilyBlues:
			case cff_FamilyOtherBlues:
			case cff_StdHW:
			case cff_StdVW:
			case cff_UniqueID:
			case cff_XUID:
			case cff_reserved22:
			case cff_reserved23:
			case cff_reserved24:
			case cff_reserved25:
			case cff_reserved26:
			case cff_reserved27:
				h->stack.cnt = 0;
				break;

			case cff_version:
				if (enable) {
					h->font.version = (unsigned short)indexInt(h, 0);
				}
				h->stack.cnt = 0;
				break;

			case cff_Encoding:
				h->dict.Encoding = indexInt(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_charset:
				h->dict.charset = indexInt(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_CharStrings:
				h->dict.CharStrings = indexInt(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_Private:
				h->dict.Private.length = indexInt(h, 0);
				h->dict.Private.offset = indexInt(h, 1);
				h->stack.cnt = 0;
				break;

			case cff_Subrs:
				h->dict.Subrs = indexInt(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_Notice:
				if (enable) {
					h->font.Notice = (unsigned short)indexInt(h, 0);
				}
				h->stack.cnt = 0;
				break;

			case cff_FullName:
				if (enable) {
					h->font.FullName = (unsigned short)indexInt(h, 0);
				}
				h->stack.cnt = 0;
				break;

			case cff_FamilyName:
				if (enable) {
					h->font.FamilyName = (unsigned short)indexInt(h, 0);
				}
				h->stack.cnt = 0;
				break;

			case cff_FontBBox:
				if (enable) {
					h->font.FontBBox.left = FIX2INT(FLOOR(indexFix(h, 0)));
					h->font.FontBBox.bottom = FIX2INT(FLOOR(indexFix(h, 1)));
					h->font.FontBBox.right = FIX2INT(CEIL(indexFix(h, 2)));
					h->font.FontBBox.top = FIX2INT(CEIL(indexFix(h, 3)));
				}
				h->stack.cnt = 0;
				break;

			case cff_defaultWidthX:
				h->dict.defaultWidthX = indexFix(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_nominalWidthX:
				h->dict.nominalWidthX = indexFix(h, 0);
				h->stack.cnt = 0;
				break;

			case cff_escape: {
				/* Process escaped operator */
				switch (cff_ESC(GETBYTE(h))) {
					case cff_PaintType:
					case cff_StrokeWidth:
					case cff_BlueScale:
					case cff_BlueShift:
					case cff_BlueFuzz:
					case cff_StemSnapH:
					case cff_StemSnapV:
					case cff_ForceBold:
					case cff_ForceBoldThreshold:
					case cff_lenIV:
					case cff_LanguageGroup:
					case cff_ExpansionFactor:
					case cff_initialRandomSeed:
					case cff_PostScript:
					case cff_BaseFontName:
					case cff_BaseFontBlend:
					case cff_CIDFontRevision:
					case cff_CIDFontType:
					case cff_CIDCount:
					case cff_UIDBase:
						h->stack.cnt = 0;
						break;

					case cff_CIDFontVersion:
						h->font.cid.version = indexDbl(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_FontMatrix:
						saveMatrix(h);
						h->stack.cnt = 0;
						break;

					case cff_isFixedPitch:
						h->font.isFixedPitch = (unsigned short)indexInt(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_UnderlinePosition:
						h->font.UnderlinePosition = (unsigned short)indexInt(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_UnderlineThickness:
						h->font.UnderlineThickness = (unsigned short)indexInt(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_Copyright:
						if (enable) {
							h->font.Copyright = (unsigned short)indexInt(h, 0);
						}
						h->stack.cnt = 0;
						break;

					case cff_ItalicAngle:
						h->font.ItalicAngle = indexFix(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_CharstringType:
						if (indexInt(h, 0) == 2) {
						}
						else if (indexInt(h, 0) == 13 && t13Support(h)) {
							h->cstrRead = t13Read;
						}
						else {
							fatal(h, "unsupported charstring type");
						}
						h->stack.cnt = 0;
						break;

					case cff_ROS:
						if (enable) {
							h->font.cid.registry = (unsigned short)indexInt(h, 0);
							h->font.cid.ordering = (unsigned short)indexInt(h, 1);
							h->font.cid.supplement = (unsigned short)indexInt(h, 2);
						}
						h->flags |= CID_FONT;
						h->stack.cnt = 0;
						break;

					case cff_FDArray:
						h->dict.FDArray = indexInt(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_FDSelect:
						h->dict.FDSelect = indexInt(h, 0);
						h->stack.cnt = 0;
						break;

					case cff_FontName:
						h->stack.cnt = 0;
						break;

					case cff_MultipleMaster:
						if (!(h->flags & FONT_INIT)) {
							/* Initialize MM parameters */
							int iStack = 0;

							h->font.mm.nAxes = h->stack.cnt - 4;
							h->font.mm.nMasters = (unsigned short)indexInt(h, iStack++);

							/* Set UDV if not already set */
							if (h->flags & UDV_SET) {
								iStack += h->font.mm.nAxes;
							}
							else {
								int i;
								for (i = 0; i < h->font.mm.nAxes; i++) {
									h->UDV[i] = h->font.mm.UDV[i] =
									        indexFix(h, iStack++);
								}
							}

							h->font.mm.lenBuildCharArray =
							    h->BCA.size = indexInt(h, iStack++);
							h->BCA.array = MEM_NEW(h, h->BCA.size * sizeof(Fixed));
							h->font.mm.NDV = (unsigned short)indexInt(h, iStack++);
							h->font.mm.CDV = (unsigned short)indexInt(h, iStack);

							h->flags |= MM_FONT;
						}
						h->stack.cnt = 0;
						if (!(h->flags & WV_SET)) {
							/* Convert UDV to WV if WV not already set */
							unsigned length;
							Offset save = TELL(h);
							t2Read(h, getProcOffset(h, h->font.mm.NDV, &length),
							       cstr_DICT);
							t2Read(h, getProcOffset(h, h->font.mm.CDV, &length),
							       cstr_DICT);
							h->flags |= WV_SET;
							seekbyte(h, save);
						}
						break;

					case cff_BlendAxisTypes: {
						int i;
						for (i = 0; i < h->stack.cnt; i++) {
							h->font.mm.axisTypes[i] = (unsigned short)indexInt(h, i);
						}
						h->stack.cnt = 0;
					}
					break;

					case cff_SyntheticBase:
					case cff_Chameleon:
						fatal(h, "unsupported font type");

					default:
						fatal(h, "reserved DICT op");
				}
				break;
			}

			case cff_shortint: {
				/* 2 byte number */
				long byte1 = GETBYTE(h);
				pushInt(h, byte1 << 8 | GETBYTE(h));
			}
			break;

			case cff_longint: {
				/* 5 byte number */
				long byte1 = GETBYTE(h);
				long byte2 = GETBYTE(h);
				long byte3 = GETBYTE(h);
				pushLong(h, byte1 << 24 | byte2 << 16 | byte3 << 8 | GETBYTE(h));
			}
			break;

			case cff_BCD:
				pushDbl(h, convBCD(h));
				break;

			case cff_T2:
				t2Read(h, -1, cstr_DICT);
				break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				pushInt(h, 108 + 256 * (byte0 - 247) + GETBYTE(h));
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				pushInt(h, -108 - 256 * (byte0 - 251) - GETBYTE(h));
				break;

			case cff_reserved255:
				fatal(h, "reserved charstring op");

			default:
				/* 1 byte number */
				pushInt(h, byte0 - 139);
				break;
		}
	}
}

/* Read subroutine index and calculate number bias */
static void subrINDEXRead(cffCtx h, INDEX *index, Offset offset) {
	(void)INDEXRead(h, index, offset);
	if (index->count < 1240) {
		index->bias = 107;
	}
	else if (index->count < 33900) {
		index->bias = 1131;
	}
	else {
		index->bias = 32768;
	}
}

/* Read CID structures */
static void CIDRead(cffCtx h) {
	int i;

	/* Read FDArray INDEX */
	if (h->dict.FDArray == 0) {
		fatal(h, "missing FDArray operator");
	}
	INDEXRead(h, &h->index.FDArray, h->dict.FDArray);

	/* Read FDArray */
	h->fd = MEM_NEW(h, h->index.FDArray.count * sizeof(FDElement));
	for (i = 0; i < h->index.FDArray.count; i++) {
		unsigned length;
		Offset offset = INDEXGet(h, &h->index.FDArray, i, &length);

		h->dict.fd = &h->fd[i];

		/* Read Font DICT */
		h->dict.Private.length = 0;
		h->dict.fd->FontMatrix = NULL;
		DICTRead(h, length, offset, 0);

		if (h->dict.Private.length == 0) {
			fatal(h, "FD missing Private operator");
		}

		/* Read Private DICT */
		h->dict.Subrs = 0;
		h->dict.defaultWidthX = 0;
		h->dict.nominalWidthX = 0;
		DICTRead(h, h->dict.Private.length, h->dict.Private.offset, 0);

		/* Read Subrs index */
		if (h->dict.Subrs != 0) {
			subrINDEXRead(h, &h->dict.fd->Subrs,
			              h->dict.Private.offset + h->dict.Subrs);
		}
		else {
			h->index.Subrs.count = 0;
		}

		/* Save CharSring parsing values */
		h->dict.fd->defaultWidthX = h->dict.defaultWidthX;
		h->dict.fd->nominalWidthX = h->dict.nominalWidthX;
	}
	h->dict.fd = NULL;
}

/* Parse cff data */
static void cffRead(cffCtx h) {
	Offset offset;
	unsigned length;

	seekbyte(h, h->start);

	h->hdr.major = GETBYTE(h);
	h->hdr.minor = GETBYTE(h);
	h->hdr.hdrSize = GETBYTE(h);
	h->hdr.offSize = GETBYTE(h);

	if (h->hdr.major != 1) {
		fatal(h, "unknown CFF version");
	}

	/* Read fixed INDEXes */
	offset = INDEXRead(h, &h->index.name, h->hdr.hdrSize);
	offset = INDEXRead(h, &h->index.top, offset);
	offset = INDEXRead(h, &h->index.string, offset);
	subrINDEXRead(h, &h->index.global, offset);

	if (h->index.name.count != 1) {
		fatal(h, "multiple-font FontSet");
	}

	/* Set FontName */
	h->font.FontName.offset = INDEXGet(h, &h->index.name, 0, &length);
	h->font.FontName.length = length;

	/* Read top DICT */
	offset = INDEXGet(h, &h->index.top, 0, &length);
	DICTRead(h, length, offset, 1);

	if (h->flags & CID_FONT) {
		CIDRead(h);
	}
	else {
		/* Read Private DICT */
		DICTRead(h, h->dict.Private.length, h->dict.Private.offset, 1);

		/* Read Subrs index */
		if (h->dict.Subrs != 0) {
			subrINDEXRead(h, &h->index.Subrs,
			              h->dict.Private.offset + h->dict.Subrs);
		}
		else {
			h->index.Subrs.count = 0;
		}
	}

	/* Read CharStrings INDEX */
	if (h->dict.CharStrings == 0) {
		fatal(h, "no CharStrings!");
	}

	INDEXRead(h, &h->index.CharStrings, h->dict.CharStrings);

	h->font.nGlyphs = h->index.CharStrings.count;
	h->font.Encoding =
	    (short)((h->dict.Encoding < CFF_CUSTOM_ENC) ? h->dict.Encoding : CFF_CUSTOM_ENC);
	h->font.charset =
	    (short)((h->dict.charset < CFF_CUSTOM_CS) ? h->dict.charset : CFF_CUSTOM_CS);

	h->flags |= FONT_INIT;
}

/* Add SID to charset */
static void addSID(cffCtx h, unsigned gid, cffSID sid) {
	h->glyphs[gid].id = sid;

	if (sid < ARRAY_LEN(stdenc)) {
		/* Add GID of standard encoded glyph to table */
		int code = stdenc[sid];
		if (code != -1) {
			h->stdGlyphs[code] = gid;
		}
	}
}

/* Initialize from predefined charset */
static void predefCharset(cffCtx h, int cnt, cffSID *cs) {
	int gid;

	if (cnt > h->index.CharStrings.count) {
		/* Adjust for case where nGlyphs < predef charset length */
		cnt = h->index.CharStrings.count;
	}

	for (gid = 0; gid < cnt; gid++) {
		addSID(h, gid, cs[gid]);
	}
}

/* Read charset */
static void charsetRead(cffCtx h) {
	/* Predefined charsets */
	static cffSID isocs[] = {
		0,                        /* .notdef */
#include "isocs0.h"
	};
	static cffSID excs[] = {
		0,                        /* .notdef */
#include "excs0.h"
	};
	static cffSID exsubcs[] = {
		0,                        /* .notdef */
#include "exsubcs0.h"
	};

	switch (h->dict.charset) {
		case CFF_ISO_CS:
			predefCharset(h, ARRAY_LEN(isocs), isocs);
			break;

		case CFF_EXP_CS:
			predefCharset(h, ARRAY_LEN(excs), excs);
			break;

		case CFF_EXPSUB_CS:
			predefCharset(h, ARRAY_LEN(exsubcs), exsubcs);
			break;

		default: {
			/* Custom charset */
			long gid;
			int nGlyphs = h->index.CharStrings.count;

			seekbyte(h, h->dict.charset);

			gid = 0;
			addSID(h, gid++, 0);    /* .notdef */

			switch (GETBYTE(h)) {
				case 0:
					for (; gid < nGlyphs; gid++) {
						addSID(h, gid, (unsigned short)getnum(h, SID_SIZE));
					}
					break;

				case 1:
					while (gid < nGlyphs) {
						int i;
						cffSID sid = (unsigned short)getnum(h, SID_SIZE);
						int nLeft = GETBYTE(h);
						for (i = 0; i <= nLeft; i++) {
							addSID(h, gid++, sid++);
						}
					}
					break;

				case 2:
					while (gid < nGlyphs) {
						int i;
						cffSID sid = (unsigned short)getnum(h, SID_SIZE);
						int nLeft = (int)getnum(h, 2);
						for (i = 0; i <= nLeft; i++) {
							addSID(h, gid++, sid++);
						}
					}
					break;

				default:
					fatal(h, "reserved charset format");
			}
		}
		break;
	}
}

/* Initialize from predefined encoding */
static void predefEncoding(cffCtx h, int cnt, short *enc) {
	long gid;
	for (gid = 0; gid < h->index.CharStrings.count; gid++) {
		Glyph *glyph = &h->glyphs[gid];
		if (glyph->id >= cnt) {
			break;
		}
		glyph->code = enc[glyph->id];
	}
}

/* Read encoding */
static void EncodingRead(cffCtx h) {
	switch (h->dict.Encoding) {
		case CFF_STD_ENC:
			predefEncoding(h, ARRAY_LEN(stdenc), stdenc);
			break;

		case CFF_EXP_ENC:
			predefEncoding(h, ARRAY_LEN(exenc), exenc);
			break;

		default: {
			/* Custom encoding */
			int i;
			int j;
			int cnt;
			int fmt;
			long gid;

			seekbyte(h, h->dict.Encoding);

			fmt = GETBYTE(h);

			gid = 0;
			h->glyphs[gid++].code = -1;    /* .notdef */

			switch (fmt & 0x7f) {
				case 0:
					cnt = GETBYTE(h);
					while (gid <= cnt) {
						h->glyphs[gid++].code = GETBYTE(h);
					}
					break;

				case 1:
					cnt = GETBYTE(h);
					for (i = 0; i < cnt; i++) {
						int code = GETBYTE(h);
						int nLeft = GETBYTE(h);
						for (j = 0; j <= nLeft; j++) {
							h->glyphs[gid++].code = code++;
						}
					}
					break;

				default:
					fatal(h, "reserved Encoding format");
			}
			if (fmt & 0x80) {
				/* Read supplementary encoding */
				cnt = GETBYTE(h);

				for (i = 0; i < cnt; i++) {
					cffSID sid;
					cffSupCode *sup = MEM_NEW(h, sizeof(cffSupCode));

					sup->code = GETBYTE(h);

					/* Search for glyph with this SID. Glyphs with multiple
					   encodings are rare so linear search is acceptable */
					sid = (unsigned short)getnum(h, 2);
					for (gid = 0; gid < h->index.CharStrings.count; gid++) {
						if (sid == h->glyphs[gid].id) {
							Glyph *glyph = &h->glyphs[gid];
							sup->next = glyph->sup;
							glyph->sup = sup;
							goto next;
						}
					}

					fatal(h, "supplement SID not found");

next:;
				}
			}
		}
		break;
	}
}

/* Read FDSelect */
static void FDSelectRead(cffCtx h) {
	unsigned gid;

	if (h->dict.FDSelect == 0) {
		fatal(h, "missing FDSelect operator");
	}

	seekbyte(h, h->dict.FDSelect);
	switch (GETBYTE(h)) {
		case 0:
			for (gid = 0; gid < h->index.CharStrings.count; gid++) {
				h->glyphs[gid].fd = GETBYTE(h);
			}
			break;

		case 3: {
			int nRanges = getnum(h, 2);

			gid = getnum(h, 2);
			while (nRanges--) {
				int fd = GETBYTE(h);
				unsigned next = getnum(h, 2);

				while (gid < next) {
					h->glyphs[gid++].fd = fd;
				}
			}
		}
		break;

		default:
			fatal(h, "reserved FDSelect format");
	}
}

/* Perform global glyph initialization */
static void initGlyphs(cffCtx h) {
	long i;
	long nGlyphs;

	if (!(h->flags & FONT_INIT)) {
		cffRead(h);
	}

	/* Allocate and initialize glyphs array */
	nGlyphs = h->index.CharStrings.count;
	h->glyphs = MEM_NEW(h, nGlyphs * sizeof(Glyph));
	for (i = 0; i < nGlyphs; i++) {
		Glyph *glyph = &h->glyphs[i];
		glyph->code = -1;
		glyph->sup = NULL;
	}

	/* Fill glyph array */
	charsetRead(h);
	if (h->flags & CID_FONT) {
		FDSelectRead(h);
	}
	else {
		EncodingRead(h);
	}

	h->flags |= GLYPHS_INIT;
}

/* Read charstring and return metrics */
static void glyphRead(cffCtx h, unsigned gid) {
	int newfd;
	unsigned length;

	if (gid >= h->index.CharStrings.count) {
		fatal(h, "gid out-of-range");
	}

	if (h->flags & CID_FONT && (newfd = h->glyphs[gid].fd) != h->lastfd) {
		/* FD change; copy new fd values */
		FDElement *fd = &h->fd[newfd];

		h->index.Subrs = fd->Subrs;
		h->dict.defaultWidthX = fd->defaultWidthX;
		h->dict.nominalWidthX = fd->nominalWidthX;
		h->dict.FontMatrix = fd->FontMatrix;
		h->lastfd = newfd;
	}
	h->cstrRead(h,
	            INDEXGet(h, &h->index.CharStrings, gid, &length), cstr_GLYPH);
}

/* Get glyph information for given gid */
cffGlyphInfo *cffGetGlyphInfo(cffCtx h, unsigned gid, cffPathCallbacks *cb) {
	Glyph *glyph;

	if (!(h->flags & GLYPHS_INIT)) {
		initGlyphs(h);
	}

	h->path.cb = cb;
	h->path.vstem = h->path.hstem = 0;
	h->flags &= ~JUST_WIDTH;
	glyphRead(h, gid);
	h->path.cb = NULL;

	glyph = &h->glyphs[gid];
	h->glyph.id = glyph->id;
	h->glyph.code = glyph->code;
	h->glyph.hAdv = FIX2INT(h->path.hAdv);
	h->glyph.vAdv = FIX2INT(h->path.vAdv);
	h->glyph.bbox.left = FIX2INT(FLOOR(h->path.left));
	h->glyph.bbox.bottom = FIX2INT(FLOOR(h->path.bottom));
	h->glyph.bbox.right = FIX2INT(CEIL(h->path.right));
	h->glyph.bbox.top = FIX2INT(CEIL(h->path.top));
	h->glyph.sup = glyph->sup;

	return &h->glyph;
}

/* Get glyph width only */
void cffGetGlyphWidth(cffCtx h, unsigned gid, cffFWord *hAdv, cffFWord *vAdv) {
	if (!(h->flags & GLYPHS_INIT)) {
		initGlyphs(h);
	}

	h->flags |= JUST_WIDTH;
	glyphRead(h, gid);

	if (hAdv != NULL) {
		*hAdv = FIX2INT(h->path.hAdv);
	}
	if (vAdv != NULL) {
		*vAdv = FIX2INT(h->path.vAdv);
	}
}

/* Get glyph organization only */
void cffGetGlyphOrg(cffCtx h, unsigned gid,
                    unsigned short *id, short *code, cffSupCode **sup) {
	Glyph *glyph;

	if (!(h->flags & GLYPHS_INIT)) {
		initGlyphs(h);
	}

	glyph = &h->glyphs[gid];
	*id = glyph->id;
	*code = glyph->code;
	*sup = glyph->sup;
}

/* Set UDV */
void cffSetUDV(cffCtx h, int nAxes, cffFixed *UDV) {
	memcpy(h->UDV, UDV,
	       sizeof(cffFixed) * ((nAxes > TX_MAX_AXES) ? TX_MAX_AXES : nAxes));
	h->flags |= UDV_SET;
	h->flags &= ~WV_SET;
	if (!(h->flags & FONT_INIT)) {
		cffRead(h);
	}
	else {
		/* Convert UDV to WV */
		unsigned length;
		t2Read(h, getProcOffset(h, h->font.mm.NDV, &length), cstr_DICT);
		t2Read(h, getProcOffset(h, h->font.mm.CDV, &length), cstr_DICT);
		h->flags |= WV_SET;
	}
}

/* Set WV */
void cffSetWV(cffCtx h, int nMasters, cffFixed *WV) {
	memcpy(h->WV, WV, sizeof(cffFixed) *
	       ((nMasters > TX_MAX_MASTERS) ? TX_MAX_MASTERS : nMasters));
	h->flags |= WV_SET;
}

/* Initialize font */
static void initFont(cffCtx h) {
	if (h->flags & FONT_INIT) {
		if (h->flags & MM_FONT) {
			/* Reparse Top DICT for new instance values */
			unsigned length;
			Offset offset = INDEXGet(h, &h->index.top, 0, &length);
			DICTRead(h, length, offset, 1);
		}
	}
	else {
		cffRead(h);
	}
}

/* Get font information */
cffFontInfo *cffGetFontInfo(cffCtx h) {
	initFont(h);
	return &h->font;
}

/* Execute metric charstring at offset (result[0] is stack bottom) */
int cffExecMetric(cffCtx h, long offset, Fixed *result) {
	int i;

	if (!(h->flags & WV_SET)) {
		initFont(h);
	}

	/* Process string */
	t2Read(h, offset, cstr_METRIC);

	/* Return results */
	for (i = 0; i < h->stack.cnt; i++) {
		result[i] = indexFix(h, i);
	}
	return h->stack.cnt;
}

/* [cffread callback] Refill local data from current position */
static char *localRefill(void *ctx, long *count) {
	*count = 0;
	return NULL;
}

/* Execute client-supplied metric charstring cstr. length indicates the number
   of bytes of data available. (result[0] is stack bottom) */
int cffExecLocalMetric(cffCtx h, char *cstr, long length, cffFixed *result) {
	int i;
	struct {
		char *next;
		long left;
		Offset nextoff;

		char * (*cffRefill)(void *ctx, long *count);
	}
	save;

	if (!(h->flags & WV_SET)) {
		initFont(h);
	}

	/* Save data input state (cffSeek not needed) */
	save.next = h->data.next;
	save.left = h->data.left;
	save.nextoff = h->data.nextoff;
	save.cffRefill = h->cb.cffRefill;

	/* Set local data input */
	h->data.next = cstr;
	h->data.left = h->data.nextoff = length;
	h->cb.cffRefill = localRefill;

	/* Process string */
	t2Read(h, -1, cstr_METRIC);

	/* Restore data input state */
	h->data.next = save.next;
	h->data.left = save.left;
	h->data.nextoff = save.nextoff;
	h->cb.cffRefill = save.cffRefill;

	/* Return results */
	for (i = 0; i < h->stack.cnt; i++) {
		result[i] = indexFix(h, i);
	}
	return h->stack.cnt;
}

/*
 * Type 13 charstring support.
 *
 * The code may be compiled to support CJK protected fonts (via Type 13
 * charstrings) or to fail on such fonts. Support is enabled by defining the
 * macros CFF_T13_SUPPORT to a non-zero value. Note, however, that Type 13 is
 * proprietary to Adobe and not all clients of this library will be supplied
 * with the module that supports Type 13. In such cases these clients should
 * not define the CFF_T13_SUPPORT macro in order to avoid compiler complaints
 * about a missing t13supp.c file.
 */

#if CFF_T13_SUPPORT
#include "t13supp.c"
#else

#include "t13fail.c"

#endif /* CFF_T13_SUPPORT */

#if CFFREAD_DEBUG

#include <stdio.h>

static void dbstack(cffCtx h) {
	if (h->stack.cnt == 0) {
		printf("empty\n");
	}
	else {
		int i;
		for (i = 0; i < h->stack.cnt; i++) {
			switch (h->stack.type[i]) {
				case STK_DOUBLE:
					printf("[%d]=%.4g ", i, h->stack.array[i].d);
					break;

				case STK_FIXED:
					printf("[%d]=%.4g ", i, h->stack.array[i].f / 65536.0);
					break;

				case STK_LONG:
					printf("[%d]=%ld ", i, h->stack.array[i].l);
					break;
			}
		}
		printf("\n");
	}
}

static void dbpath(cffCtx h) {
	printf("--- path\n");
	printf("flags =%04hx\n", h->path.flags);
	printf("nStems=%hd\n", h->path.nStems);
	printf("hAdv  =%g\n", FIX2DBL(h->path.hAdv));
	printf("vAdv  =%g\n", FIX2DBL(h->path.vAdv));
	printf("x     =%g\n", FIX2DBL(h->path.x));
	printf("y     =%g\n", FIX2DBL(h->path.y));
	printf("left  =%g\n", FIX2DBL(h->path.left));
	printf("bottom=%g\n", FIX2DBL(h->path.bottom));
	printf("right =%g\n", FIX2DBL(h->path.right));
	printf("top   =%g\n", FIX2DBL(h->path.top));
}

static void dbvecs(cffCtx h, int hex) {
	int i;

	printf("--- UDV\n");
	for (i = 0; i < h->font.mm.nAxes; i++) {
		if (hex) {
			printf("[%d]=%08lx ", i, h->UDV[i]);
		}
		else {
			printf("[%d]=%.4g ", i, h->UDV[i] / 65536.0);
		}
	}
	printf("\n");

	printf("--- NDV\n");
	for (i = 0; i < h->font.mm.nAxes; i++) {
		if (hex) {
			printf("[%d]=%08lx ", i, h->NDV[i]);
		}
		else {
			printf("[%d]=%.4g ", i, h->NDV[i] / 65536.0);
		}
	}
	printf("\n");

	printf("--- WV\n");
	for (i = 0; i < h->font.mm.nMasters; i++) {
		if (hex) {
			printf("[%d]=%08lx ", i, h->WV[i]);
		}
		else {
			printf("[%d]=%.4g ", i, h->WV[i] / 65536.0);
		}
	}
	printf("\n");
}

static void dbsetudv(cffCtx h, int u0, int u1, int u2, int u3) {
	h->UDV[0] = INT2FIX(u0);
	h->UDV[1] = INT2FIX(u1);
	h->UDV[2] = INT2FIX(u2);
	h->UDV[3] = INT2FIX(u3);
	h->flags |= UDV_SET;
	h->flags &= ~WV_SET;
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dbstack, dbpath, dbvecs, dbsetudv);
}

#endif /* CFFREAD_DEBUG */