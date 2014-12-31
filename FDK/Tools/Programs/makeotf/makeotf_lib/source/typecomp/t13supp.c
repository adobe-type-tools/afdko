/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "t13.h"
#include "parse.h"
#include "sindex.h"

#include "txops.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/*
 * Type 13  charstring operators.
 *
 * The Type 13 operator set was derived from the Type 2 set.
 *
 * The sole purpose of Type 13 as differentiated from Type 2 is to
 * provide a mechanism to implement "Outline" protection.
 *
 * The Type 13 definition is an unpublished Trade Secret of Adobe
 * Systems Inc.
 *
 * Macro prefixes:
 *
 * t13_ Type 13
 */

#define t13_reserved0       0   /* Reserved */
/*							1-215  Number: -107..107 */
#define t13_hvcurveto       216
#define t13_rrcurveto       217
#define t13_vstem           218
#define t13_rmoveto         219
/*							220	   Number: -1131..-876 */
#define t13_rlineto         221
#define t13_cntrmask        222
#define t13_shftshort       223
/*							224    Number: -875..-620 */
#define t13_hstem           225
#define t13_callsubr        226
#define t13_escape          227
/*							228    Number: -619..-364 */
#define t13_hintmask        229
#define t13_vmoveto         230
#define t13_hlineto         231
/*							232    Number: -363..-108 */
#define t13_shortint        233
#define t13_hhcurveto       234
/*							235    Number: 1131..876 */
#define t13_reserved236     236 /* Reserved (Subroutinizer cstr separator) */
#define t13_reserved237     237 /* Reserved */
#define t13_vlineto         238
/*							239    Number: 619..364 */
#define t13_return          240
/*							241	   Number: 363..108 */
#define t13_endchar         242
#define t13_blend           243
#define t13_reserved244     244
#define t13_hstemhm         245
/*							246	   Number: 620..875 */
#define t13_hmoveto         247
#define t13_vstemhm         248
#define t13_rcurveline      249
#define t13_rlinecurve      250
#define t13_vvcurveto       251
#define t13_callgsubr       252
#define t13_vhcurveto       253
/*							254    Number: +/- 32k (Fixed) */
#define t13_reserved255     255 /* Reserved */

/* --- Two byte operators --- */

/* Make escape operator value; may be redefined to suit implementation */
#ifndef t13_ESC
#define t13_ESC(op)          (t13_escape << 8 | (op))
#endif

/*									0-27  Reserved */
#define t13_dotsection      t13_ESC(28)
/*									29-56 Reserved */
#define t13_flex            t13_ESC(57)
#define t13_hflex           t13_ESC(58)
#define t13_exch            t13_ESC(59)
#define t13_sqrt            t13_ESC(60)
#define t13_roll            t13_ESC(61)
#define t13_cntron          t13_ESC(62)
#define t13_hflex1          t13_ESC(63)
/*									64-94 Reserved */
#define t13_mul             t13_ESC(95)
#define t13_dup             t13_ESC(96)
#define t13_index           t13_ESC(97)
#define t13_flex1           t13_ESC(98)
/*									99-126 Reserved */
#define t13_random          t13_ESC(127)
/*									128-209 Reserved */
#define t13_ifelse          t13_ESC(210)
#define t13_get             t13_ESC(211)
#define t13_put             t13_ESC(212)
#define t13_drop            t13_ESC(213)
#define t13_eq              t13_ESC(214)
#define t13_neg             t13_ESC(215)
#define t13_load            t13_ESC(216)
#define t13_div             t13_ESC(217)
#define t13_sub             t13_ESC(218)
#define t13_add             t13_ESC(219)
/*									220-249 Reserved */
#define t13_abs             t13_ESC(250)
#define t13_store           t13_ESC(251)
#define t13_not             t13_ESC(252)
#define t13_or              t13_ESC(253)
#define t13_and             t13_ESC(254)
#define t13_reservedESC255  t13_ESC(255)    /* Reserved */

/* --- Type 13 interpreter limits/definitions --- */
#define T13_MAX_OP_STACK     96  /* Max operand stack depth */
#define T13_MAX_CALL_STACK   20  /* Max callsubr stack depth */
#define T13_MAX_STEMS        96  /* Max stems */
#define T13_MAX_MASTERS      16  /* Max master designs */
#define T13_MAX_AXES         15  /* Max design axes */
#define T13_STD_FLEX_HEIGHT  50  /* Standard flex height (100ths/pixel) */
#define T13_MAX_BLUE_VALUES  14  /* 14 values (7 pairs) */
#define T13_MAX_OTHER_BLUES  10  /* 10 values (5 pairs) */
#define T13_MAX_BLUES        24  /* 24 values (12 pairs) */

/* load/store registry ids */
#define T13_REG_WV          0   /* WeightVector */
#define T13_REG_NDV         1   /* NormalizedDesignVector */
#define T13_REG_UDV         2   /* UserDesignVector */

/* In order to avoid matching repeats across charstring boundaries it is
   necessary to add a unique separator between charstrings. This is achieved by
   inserting the t13_separator operator after each endchar. The 1-byte operator
   value is followed by a 24-bit number that is guaranteed to be different for
   each charstring thereby removing any possibility of a match spanning an
   endchar operator. These operators are inserted by the recode module and
   removed by the subroutinizer */
#define t13_separator   t13_reserved236
#define t13_noop        t13_reserved0   /* Noop flag */

typedef struct                  /* Character record */
{
	long icstr;                 /* Index of charstring start */
	short width;                /* Original width */
} Char;

typedef Fixed Blend[T13_MAX_MASTERS];   /* Blendable array of values */

/* Save Type 13 operator without args */
#define T13SAVEOP(op, da) \
	do { if ((op) & 0xff00) { *da_NEXT(da) = (unsigned char)t13_escape; } \
		 *da_NEXT(da) = (unsigned char)(op); } \
	while (0)

/* Calulate number size in bytes */
#define NUMSIZE(v) (-107 <= (v) && (v) <= 107 ? 1 : (-1131 <= (v) && (v) <= 1131 ? 2 : 3))

/* Charstring buffer */
typedef da_DCL (char, CSTR);

/* --- Stack access macros --- */

/* Check for under/overflow before POPs, PUSHes, or INDEXing */
#define CHKUFLOW(c) do { if (h->stack.cnt < (c)) { badChar(h); } \
} \
	while (0)
#define CHKOFLOW(c) \
	do { if (h->stack.cnt + (c) > T13_MAX_OP_STACK) { badChar(h); } \
	} \
	while (0)

/* Stack access macros for single-valued elements */
#define INDEX(i)        (h->stack.array[i][0])
#define SET(i, v)        (INDEX(i) = (v), h->stack.flags[i] = 0)
#define PUSH_FIX(v) do { CHKOFLOW(1); INDEX(h->stack.cnt) = (v); \
		                 h->stack.flags[h->stack.cnt++] = 0; } \
	while (0)
#define PUSH_INT(i)     PUSH_FIX(INT2FIX(i))
#define POP_FIX()       INDEX(--h->stack.cnt)
#define POP_INT()       (POP_FIX() >> 16)
#define STK_BLEND(i)    (h->stack.flags[i] & S_BLEND)

/* --- Stem hints --- */

#define MAX_MASK_BYTES  ((T13_MAX_STEMS + 7) / 8)
typedef char HintMask[MAX_MASK_BYTES];

#define CLEAR_ID_MASK(a)    memset(a, 0, MAX_MASK_BYTES)
#define SET_ID_BIT(a, i)     ((a)[(i) / 8] |= 1 << ((i) % 8))
#define TEST_ID_BIT(a, i)    ((a)[(i) / 8] & 1 << ((i) % 8))

#define CLEAR_HINT_MASK(a)  memset(a, 0, h->hint.size)
#define SET_HINT_BIT(a, i)   ((a)[(i) / 8] |= 1 << (7 - (i) % 8))
#define SAVE_HINT_MASK(a)   \
	memcpy(da_EXTEND(h->cstrs, h->hint.size), a, h->hint.size)

typedef struct              /* Stem record */
{
	char type;
#define STEM_VERT   (1 << 0)  /* Flags vertical stem */
#define STEM_CNTR   (1 << 1)  /* Counter attribute (h/vstem3) */
	unsigned char id;
	Blend edge;
	Blend delta;
} Stem;

/* --- Widths --- */

/* Width optimization seeks to find two values called the default and nominal
   width. Glyphs with the default width omit the width altogether and all other
   widths are reconstituted by subtracting the nominal width from the encoded
   width found in the charstring. By carefully choosing a nominal width many
   widths will shrink by one byte because smaller numbers are encoded in fewer
   bytes.

   However, two factors complicate this process. The first is that the choice
   of nominal width and default width interact. For example, choosing a nominal
   width near a default width will make the potential default width savings
   smaller. The second is that the optimal choice of nominal width can make
   some encoded widths larger and since the optimal values can't be chosen
   until all the glyphs have been processed that can lead to storage allocation
   difficulties since the charstrings are concatenated and the larger width
   would overwrite the end of the previous charstring.

   These problems are overcome by allocating three bytes at the beginning of
   every charstring for width storage which is sufficient to store a width of
   32767. The t13GetChar() function reads the non-optimized width from the
   chars array, performs the optimization, and then writes the result to the
   charstring so that the remaining charstring bytes immediately follow the
   optimized width.

   Multiple master and fractional widths fonts aren't optimized because the
   implementation would be more complicated and these kinds of fonts are rare.
 */

#define DUMMY_WIDTH_SIZE 3

typedef struct          /* Width accumulator */
{
	short width;
	unsigned short count;
} Width;

typedef struct          /* Width data per-FD */
{
	short dflt;         /* Default width */
	short nominal;      /* Nominal width */
	da_DCL(Width, total); /* Unique width totals */
} WidthFD;

/* --- Path --- */

/* Segment types */
enum {
	seg_move,   /* x1 y1 moveto (2 args) */
	seg_line,   /* x1 y1 lineto (2 args) */
	seg_curve   /* x1 y1 x2 y2 x3 y3 curveto (6 args) */
};

typedef struct  /* Segment */
{
	unsigned short iArgs;   /* Coordinate argument index */
	unsigned short flags;   /* Type and status flags */
#define SEG_TYPE_MASK   0x0003
} Segment;

typedef struct  /* Operator */
{
	unsigned short nArgs;   /* Number of arguments */
	unsigned short iArgs;   /* Argment index */
	unsigned short iSeg;    /* Next segment index */
	unsigned short type;    /* Operator type */
} Operator;

/* --- Output args --- */

/* Access macros */
#define ARG_ZERO(i)     (h->args.flag[i] & A_ZERO)
#define ARG_BLND(i)     (h->args.flag[i] & A_BLND)
#define ARG_SAVE(i)     (h->args.flag[i] & A_SAVE)
#define ARG_SELECT(i)   (h->args.flag[i] |= A_SAVE)

/* --- Warning messages --- */

enum                    /* See initializer in recodeNew() for message */
{
	iHint0,
	iHint1,
	iHint2,
	iHint3,
	iHint4,
	iHint5,
	iHint6,

	iFlex0,
	iFlex1,
	iFlex2,

	iMove0,
	iMove1,
	iMove2,

	iMisc0,
	iMisc1,

	WARN_COUNT          /* Warning count */
};

/* Warning initializer */
#define IWARN(i, m)  h->warn.message[i] = (m); h->warn.count[i] = 0

/* --- Module context --- */

typedef enum                    /* Char id type */
{
	SIDType,
	CIDType,
	SubrType
} IdType;

struct t13Ctx_ {
	Font *font;                 /* Current font */
	IdType idType;              /* Char id type */
	unsigned id;                /* Current char id (SID/CID/Subr) */
	CSTR cstrs;                 /* Recoded and concatenated charstrings */
	da_DCL(Char, chars);        /* Character records */
	int intDiv;                 /* Flags div operator had integer dividend */
	int eCCRunSeen;             /* Flags /eCCRun seen in Private dict */
	struct                      /* --- Type 1 input and Type 2 output stack */
	{
		int cnt;                /* Element count */
		int limit;              /* Output stack depth limit */
		int max;                /* Max output stack (allowing for subr calls)*/
		Blend array[T13_MAX_OP_STACK];  /* Values */
		char flags[T13_MAX_OP_STACK];   /* Flags */
#define S_BLEND     (1 << 0)      /* Blend element */
	}
	stack;
	struct                      /* --- Stem hints */
	{
		HintMask initmask;      /* Initial stem id mask */
		HintMask subsmask;      /* Hint subsitution id mask */
		HintMask lastmask;      /* Last hint substitution mask */
		da_DCL(Stem, list);     /* Stem list */
	}
	stem;
	struct                      /* --- Othersubrs 12/13 counters */
	{
		int insert;             /* Flags extra compatibility stem inserted */
		da_DCL(Blend, list);    /* Original charstring list */
		da_DCL(HintMask, idmask);   /* Id masks for counter groups */
	}
	cntr;
	struct                      /* --- Hint substitution */
	{
		int initial;            /* Flags initial h/vstem list */
		int subs;               /* Flags within hint subs (1 3 callother) */
		int seen;               /* Cstr has hint subs (seen 1 3 callother) */
		int cntr;               /* Charstring has counter hints */
		int size;               /* Hint mask size (bytes) */
	}
	hint;
	struct                      /* --- Pending operators */
	{
		int op;                 /* Pending operator */
		int flex;               /* Flags flex pending */
		int move;               /* Pending move */
	}
	pend;
	struct                      /* --- Path accumulator */
	{
		Blend x;                /* Current x coordinate */
		Blend y;                /* Current y coordinate */
		struct {
			Blend x;            /* Left side-bearing x */
			Blend y;            /* Left side-bearing y */
		}
		lsb;
		Blend width;            /* Glyph width */
		da_DCL(Segment, segs);
		da_DCL(Operator, ops);
		da_DCL(Fixed, args);    /* Operator and segment args */
		da_DCL(char, idmask);   /* Stem id mask */
	}
	path;
	struct                      /* --- Widths */
	{
		int opt;                /* Flags width optimization */
		da_DCL(WidthFD, fd);    /* FD width data */
		WidthFD *curr;          /* Current width */
	}
	width;
	int nMasters;               /* Local copy of g->nMasters */
	struct                      /* --- Output argument accumulator */
	{
		int cnt;
		Blend array[13];
		char flag[13];
#define A_ZERO  (1 << 0)          /* Value(s) zero */
#define A_BLND  (1 << 1)          /* Multiple-valued arg */
#define A_SAVE  (1 << 2)          /* Save this arg */
	}
	args;
	unsigned long unique;       /* Unique subroutinizer separator value */
	struct                      /* --- Temporary file input */
	{
		char *next;             /* Next byte available in input buffer */
		long left;              /* Number of bytes available in input buffer */
	}
	tmpFile;
	struct                      /* --- Message handling */
	{
		unsigned total;         /* Total warnings for char */
		unsigned count[WARN_COUNT]; /* Totals for different warnings */
		char *message[WARN_COUNT];  /* Warning messages */
	}
	warn;
#if TC_DEBUG
	struct {
		int input;              /* Dump input charstrings */
		int output;             /* Dump output charstrings */
	}
	debug;
#endif /* TC_DEBUG */
	tcCtx g;                    /* Package context */
};

static void add2Coords(t13Ctx h, int idx, int idy);

#if TC_DEBUG
static unsigned t13Dump(long length, unsigned char *cstr, int nMasters, int t1);

#endif /* TC_DEBUG */

/* Initialize width data */
static int initWidths(WidthFD *wfd) {
	da_INIT(wfd->total, 50, 50);
	return 0;
}

/* Initialize module */
void t13New(tcCtx g) {
	t13Ctx h = MEM_NEW(g, sizeof(struct t13Ctx_));

	h->eCCRunSeen = 0;

	da_INIT(h->cstrs, 25000, 10000);
	da_INIT(h->chars, 250, 50);
	da_INIT(h->stem.list, 48, 48);
	da_INIT(h->cntr.list, 96, 96);
	da_INIT(h->cntr.idmask, 4, 4);
	da_INIT(h->path.segs, 50, 100);
	da_INIT(h->path.ops, 20, 100);
	da_INIT(h->path.args, 150, 150);
	da_INIT(h->path.idmask, 30, 100);
	da_INIT(h->width.fd, 1, 10);
	h->width.fd.init = initWidths;
	h->args.cnt = 0;
	h->unique = 0;

	/* Initialize warning messages */
	IWARN(iHint0, "stem/stem3 conflict (resolved)");
	IWARN(iHint1, "negative non-edge stem width (fixed)");
	IWARN(iHint2, "negative non-edge stem width (can't fix)");
	IWARN(iHint3, "consecutive hint substitutions (collapsed)");
	IWARN(iHint4, "hint overlap");
	IWARN(iHint5, "repeat hint substitution (discarded)");
	IWARN(iHint6, "unhinted");
	IWARN(iFlex0, "inconsistent flex");
	IWARN(iFlex1, "non-perpendicular flex");
	IWARN(iFlex2, "suspect flex args");
	IWARN(iMove0, "no move after closepath (inserted)");
	IWARN(iMove1, "moveto precedes closepath (discarded)");
	IWARN(iMove2, "moveto sequence (collapsed)");
	IWARN(iMisc0, "argcnt != 22 (12 callother)");
	IWARN(iMisc1, "illegal setcurrentpt (converted to move)");
	h->warn.total = 0;

#if TC_DEBUG
	h->debug.input = 0;
	h->debug.output = 0;
#endif /* TC_DEBUG */

	/* Link contexts */
	h->g = g;
	g->ctx.t13 = h;
}

/* Free resources */
void t13Free(tcCtx g) {
	t13Ctx h = g->ctx.t13;
	int i;

	da_FREE(h->cstrs);
	da_FREE(h->chars);
	da_FREE(h->stem.list);
	da_FREE(h->cntr.list);
	da_FREE(h->cntr.idmask);
	da_FREE(h->path.segs);
	da_FREE(h->path.ops);
	da_FREE(h->path.args);
	da_FREE(h->path.idmask);
	for (i = 0; i < h->width.fd.size; i++) {
		da_FREE(h->width.fd.array[i].total);
	}
	da_FREE(h->width.fd);
	MEM_FREE(g, h);
}

#if TC_SUBR_SUPPORT
/* Return operator length from opcode */
static int t13oplen(unsigned char *cstr) {
	switch (cstr[0]) {
		default:
			return 1;

		case t13_escape:
		case 220:
		case 224:
		case 228:
		case 232:
		case 235:
		case 239:
		case 241:
		case 246:
			return 2;

		case t13_shortint:
		case t13_shftshort:
			return 3;

		case t13_separator:
			return 4;

		case 254:
			return 5;

		case t13_hintmask:
		case t13_cntrmask:
			return cstr[1];
	}
}

/* Copy and edit cstr by removing length bytes from mask operators. Return
   advanced destination buffer pointer */
static unsigned char *t13cstrcpy(unsigned char *pdst, unsigned char *psrc,
                                 unsigned length) {
	int left;
	unsigned char *pend = psrc + length;

	while (psrc < pend) {
		switch (*psrc) {
			case t13_hintmask:
			case t13_cntrmask:          /* Mask ops; remove length byte */
				*pdst++ = *psrc++;
				left = *psrc++ - 2;
				while (left--) {
					*pdst++ = *psrc++;
				}
				length--;
				break;

			case 254:                   /* 5-byte number */
				*pdst++ = *psrc++;
				*pdst++ = *psrc++;
			/* Fall through */

			case t13_shortint:          /* 3-byte number */
			case t13_shftshort:
				*pdst++ = *psrc++;
			/* Fall through */

			case t13_escape:            /* 2-byte number/esc operator */
			case 220:
			case 224:
			case 228:
			case 232:
			case 235:
			case 239:
			case 241:
			case 246:
				*pdst++ = *psrc++;
			/* Fall through */

			default:                    /* 1-byte number/operator */
				*pdst++ = *psrc++;
				break;
		}
	}

	return pdst;
}

#endif /* TC_SUBR_SUPPORT */

/* Encode integer and return length */
static int t13EncInteger(long i, char *t) {
	if (-107 <= i && i <= 107) {
		/* Single byte number */
		t[0] = (unsigned char)(108 - i);
		return 1;
	}
	else if (108 <= i && i <= 363) {
		/* Count down */
		t[0] = (unsigned char)241;
		t[1] = (unsigned char)(363 - i);
		return 2;
	}
	else if (364 <= i && i <= 619) {
		/* Count down */
		t[0] = (unsigned char)239;
		t[1] = (unsigned char)(619 - i);
		return 2;
	}
	else if (620 <= i && i <= 875) {
		/* Count up! */
		t[0] = (unsigned char)246;
		t[1] = (unsigned char)(i - 620);
		return 2;
	}
	else if (876 <= i && i <= 1131) {
		/* Count down */
		t[0] = (unsigned char)235;
		t[1] = (unsigned char)(1131 - i);
		return 2;
	}
	else if (-363 <= i && i <= -108) {
		/* Count up */
		t[0] = (unsigned char)232;
		t[1] = (unsigned char)(i + 363);
		return 2;
	}
	else if (-619 <= i && i <= -364) {
		/* Count up */
		t[0] = (unsigned char)228;
		t[1] = (unsigned char)(i + 619);
		return 2;
	}
	else if (-875 <= i && i <= -620) {
		/* Count up */
		t[0] = (unsigned char)224;
		t[1] = (unsigned char)(i + 875);
		return 2;
	}
	else if (-1131 <= i && i <= -876) {
		/* Count up */
		t[0] = (unsigned char)220;
		t[1] = (unsigned char)(i + 1131);
		return 2;
	}
	else if (SHRT_MIN <= i && i <= SHRT_MAX) {
		/* Positive/negative 3-byte number (shared with dict ops) */
		t[0] = (unsigned char)t13_shortint;
		t[1] = (unsigned char)(i >> 8);
		t[2] = (unsigned char)i;
		return 3;
	}
	return 0;   /* Suppress compiler warning */
}

/* Encode 16.16 fixed point number and return length */
static int t13EncFixed(Fixed f, char *t) {
	if ((f & 0x0000ffff) == 0) {
		return t13EncInteger(f >> 16, t); /* No fraction, use integer */
	}
	else if ((((f & 0xc0000000) == 0xc0000000) ||
	          ((f & 0xc0000000) == 0x00000000)) &&
	         (((f & 0x0000ffff) | 0x8000) == 0x8000)) {
		/* Type shftshort; top 2 bits are either on or off and decimal portion
		   is 0.5. Retain 15 bits integer and 1 bit fraction */
		t[0] = (unsigned char)t13_shftshort;
		t[1] = (unsigned char)(f >> 23);
		t[2] = (unsigned char)(f >> 15);
		return 3;
	}
	else {
		/* 5-byte fixed point */
		t[0] = (unsigned char)254;
		t[1] = (unsigned char)(f >> 24);
		t[2] = (unsigned char)(f >> 16);
		t[3] = (unsigned char)(f >> 8);
		t[4] = (unsigned char)f;
		return 5;
	}
}

/* Initialize for new font */
static void t13NewFont(tcCtx g, Font *font) {
	t13Ctx h = g->ctx.t13;
	int i;

	if (font->flags & FONT_CID && h->cstrs.size < 3000000) {
		/* Resize da's for CID fonts */
		if (!(g->flags & TC_SMALLMEMORY)) {
			/* xxx what if we've aready allocated cstrs to another font */
			da_FREE(h->cstrs);
			da_INIT(h->cstrs, 3000000, 1000000);
		}
		da_FREE(h->chars);
		da_INIT(h->chars, 9000, 1000);
	}

	if (h->eCCRunSeen) {
		/* Save CharstringType 13 */
		dictSaveInt(&font->dict, 13);
		DICTSAVEOP(font->dict, cff_CharstringType);
	}

	h->font = font;
	h->nMasters = g->nMasters;
	h->cstrs.cnt = 0;
	h->chars.cnt = 0;
	for (i = 0; i < h->width.fd.size; i++) {
		h->width.fd.array[i].total.cnt = 0;
	}
	h->width.fd.cnt = 0;
	h->width.opt = g->nMasters == 1;
	h->tmpFile.left = 0;

#if TC_SUBR_SUPPORT
	/* Initialize subr parse data */
	if (g->flags & TC_SUBRIZE) {
		static SubrParseData spd = {
			t13oplen,
			t13cstrcpy,
			t13EncInteger,
			T13_MAX_CALL_STACK,
			t13_hintmask,
			t13_cntrmask,
			t13_callsubr,
			t13_callgsubr,
			t13_return,
			t13_endchar,
			t13_separator,
		};
		if (g->spd != NULL && g->spd != &spd) {
			parseFatal(g, "can't mix charstring types when subroutinizing");
		}
		g->spd = &spd;
	}
	h->stack.max = T13_MAX_OP_STACK - ((g->flags & TC_SUBRIZE) != 0);
#else /* TC_SUBR_SUPPORT */
	h->stack.max = T13_MAX_OP_STACK;
#endif /* TC_SUBR_SUPPORT */
}

/* Bad charstring; print fatal message with glyph and FontName appended */
static void badChar(t13Ctx h) {
	tcCtx g = h->g;
	switch (h->idType) {
		case SIDType:
			parseFatal(g, "bad charstring <%s>", sindexGetString(g, (SID)h->id));

		case CIDType:
			parseFatal(g, "bad charstring cid#%hu", h->id);

		case SubrType:
			parseFatal(g, "bad charstring subr#%hu", h->id);
	}
}

/* Register warning for glyph */
static void warnChar(t13Ctx h, int iWarn) {
	h->warn.count[iWarn]++;
	h->warn.total++;
}

/* Print warning message with glyph and FontName appended */
static void showWarn(t13Ctx h) {
	int i;
	char id[128];

	/* Construct glyph identifier */
	switch (h->idType) {
		case SIDType:
			sprintf(id, "<%s>", sindexGetString(h->g, (SID)h->id));
			break;

		case CIDType:
			sprintf(id, "cid#%hu", h->id);
			break;

		case SubrType:
			sprintf(id, "subr#%hu", h->id);
			break;
	}

	for (i = 0; i < WARN_COUNT; i++) {
		if (h->warn.count[i] > 0) {
			/* Display warning message */
			if (h->warn.count[i] == 1) {
				parseWarning(h->g, "%s %s", h->warn.message[i], id);
			}
			else {
				parseWarning(h->g, "%s %s (X%d)",
				             h->warn.message[i], id, h->warn.count[i]);
			}
			h->warn.count[i] = 0;
		}
	}
	h->warn.total = 0;
}

/* -------------- Blend (array of per-master values) functions ------------- */

/* Copy blend array */
#define copyBlend(d, s) COPY(d, s, h->nMasters)

/* Set blend array to single value */
static void setBlend(t13Ctx h, Blend a, Fixed value) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		a[i] = value;
	}
}

/* Make blend array from first element */
static void makeBlend(t13Ctx h, Blend a) {
	int i;
	for (i = 1; i < h->nMasters; i++) {
		a[i] = a[0];
	}
}

/* Add 2 blend arrays */
static void addBlend(t13Ctx h, Blend result, Blend a, Blend b) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		result[i] = a[i] + b[i];
	}
}

/* Subtract 2 blend arrays */
static void subBlend(t13Ctx h, Blend result, Blend a, Blend b) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		result[i] = a[i] - b[i];
	}
}

/* Negate blend array */
static void negBlend(t13Ctx h, Blend a) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		a[i] = -a[i];
	}
}

/* Compare blend array against supplied value. Return 1 if same else 0 */
static int cmpBlend(t13Ctx h, Blend a, Fixed value) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		if (a[i] != value) {
			return 0;
		}
	}
	return 1;
}

/* Return stack space required by blend value */
static int spaceBlend(t13Ctx h, Blend value) {
	int i;
	for (i = 1; i < h->nMasters; i++) {
		if (value[i] != value[0]) {
			return h->nMasters + 1;
		}
	}
	return 1;
}

/* ------------------ Stack element manipulation functions ----------------- */

/* Move stack element */
static void moveElement(t13Ctx h, int iDst, int iSrc) {
	h->stack.flags[iDst] = h->stack.flags[iSrc];
	if (STK_BLEND(iDst)) {
		copyBlend(h->stack.array[iDst], h->stack.array[iSrc]);
	}
	else {
		INDEX(iDst) = INDEX(iSrc);
	}
}

/* Copy element from stack */
static void saveElement(t13Ctx h, Blend dst, int index) {
	if (STK_BLEND(index)) {
		copyBlend(dst, h->stack.array[index]);
	}
	else {
		int i;
		for (i = 0; i < h->nMasters; i++) {
			dst[i] = INDEX(index);
		}
	}
}

/* Add src to dst stack element */
static void addElement(t13Ctx h, int iDst, int iSrc) {
	if (h->nMasters == 1 || !(STK_BLEND(iDst) || STK_BLEND(iSrc))) {
		INDEX(iDst) += INDEX(iSrc);
	}
	else {
		if (!STK_BLEND(iSrc)) {
			makeBlend(h, h->stack.array[iSrc]);
		}
		if (!STK_BLEND(iDst)) {
			makeBlend(h, h->stack.array[iDst]);
		}
		addBlend(h, h->stack.array[iDst], h->stack.array[iDst],
		         h->stack.array[iSrc]);
		h->stack.flags[iDst] = S_BLEND;
	}
}

/* Subtract src from dst stack element */
static void subElement(t13Ctx h, int iDst, int iSrc) {
	if (h->nMasters == 1 || !(STK_BLEND(iDst) || STK_BLEND(iSrc))) {
		INDEX(iDst) -= INDEX(iSrc);
	}
	else {
		if (!STK_BLEND(iSrc)) {
			makeBlend(h, h->stack.array[iSrc]);
		}
		if (!STK_BLEND(iDst)) {
			makeBlend(h, h->stack.array[iDst]);
		}
		subBlend(h, h->stack.array[iDst], h->stack.array[iDst],
		         h->stack.array[iSrc]);
		h->stack.flags[iDst] = S_BLEND;
	}
}

/* Lookup width in array and return its index. Returns 1 if found else 0.
   Found or insertion position returned via index parameter. */
static int lookupWidth(t13Ctx h, int width, int *index) {
	int lo = 0;
	int hi = h->width.curr->total.cnt - 1;

	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		int try = h->width.curr->total.array[mid].width;
		if (width < try) {
			hi = mid - 1;
		}
		else if (width > try) {
			lo = mid + 1;
		}
		else {
			*index = mid;
			return 1;   /* Found */
		}
	}
	*index = lo;
	return 0;   /* Not found */
}

/* Add width to accumulator */
static void addWidth(t13Ctx h, Fixed width) {
	if (width & 0xffff) {
		/* Fractional width char (very rare but found in Tex fonts) */
		int i;
		long icstr;
		char *dst;

		parseWarning(h->g, "font has fractional width(s)");
		h->width.opt = 0;

		if (h->cstrs.size == 0) {
			return; /* First char; nothing to do */
		}
		/* Rewrite charstrings with dummy bytes replaced by real width */
		icstr = 0;
		dst = h->cstrs.array;
		for (i = 0; i < h->chars.cnt - 1; i++) {
			int bytes;
			Char *this = &h->chars.array[i];
			int length = ((i + 1 == h->chars.cnt) ? h->cstrs.cnt :
			              (this + 1)->icstr - this->icstr) - DUMMY_WIDTH_SIZE;

			/* Write width and copy rest of charstring */
			bytes = t13EncInteger(this->width, dst);
			memcpy(dst + bytes,
			       &h->cstrs.array[this->icstr + DUMMY_WIDTH_SIZE], length);

			length += bytes;
			dst += length;

			/* Save new index */
			this->icstr = icstr;
			icstr += length;
		}
		h->cstrs.cnt = h->chars.array[h->chars.cnt - 1].icstr = icstr;
	}
	else {
		int index;

		if (!lookupWidth(h, width >>= 16, &index)) {
			/* Insert new width */
			Width *new = &da_GROW(h->width.curr->total,
			                      h->width.curr->total.cnt)[index];
			COPY(new + 1, new, h->width.curr->total.cnt++ - index);
			new->width = (short)width;
			new->count = 1;
		}
		else {
			/* Bump width count */
			h->width.curr->total.array[index].count++;
		}

		/* Save width */
		h->chars.array[h->chars.cnt - 1].width = (short)width;
	}
}

/* Calculate the width bytes saved by default and nominal width optimization
   (see comment at head of file) */
static long sizeFDWidths(t13Ctx h, int fd) {
	int minsize;
	WidthFD *wfd = &h->width.fd.array[fd];
	DICT *Private = (h->font->flags & FONT_CID) ?
	    &h->font->fdInfo[fd].Private : &h->font->Private;

	if (wfd->total.cnt == 0) {
		/* Unused FD */
		return 0;
	}
	else if (wfd->total.cnt == 1) {
		/* Fixed-pitch font */
		Width *width = &wfd->total.array[0];
		wfd->dflt = width->width;
		wfd->nominal = 0;
		minsize = 0;
	}
	else {
		int i;
		int nonoptsize;
		int iNominal = 0;   /* Suppress optimizer warning */
		int iDefault = 0;   /* Suppress optimizer warning */

		/* Find non-optimized size */
		nonoptsize = 0;
		for (i = 0; i < wfd->total.cnt; i++) {
			Width *width = &wfd->total.array[i];
			nonoptsize += NUMSIZE(width->width) * width->count;
		}

		/* Find best combination of nominal and default widths */
		minsize = SHRT_MAX;
		for (i = 0; i < wfd->total.cnt; i++) {
			int j;
			int nomsize;
			int nomwidth = wfd->total.array[i].width + 107;

			/* Compute total size for this nominal width */
			nomsize = 0;
			for (j = 0; j < wfd->total.cnt; j++) {
				Width *width = &wfd->total.array[j];
				nomsize += NUMSIZE(width->width - nomwidth) * width->count;
			}

			/* Try this nominal width against all possible defaults */
			for (j = 0; j < wfd->total.cnt; j++) {
				Width *dflt = &wfd->total.array[j];
				int totsize = nomsize -
				    NUMSIZE(dflt->width - nomwidth) * dflt->count;
				if (totsize < minsize) {
					minsize = totsize;
					iNominal = i;
					iDefault = j;
				}
			}
		}

		/* Optimized savings; set optimization values */
		wfd->dflt = wfd->total.array[iDefault].width;
		wfd->nominal = wfd->total.array[iNominal].width + 107;

		if (minsize +
		    ((wfd->dflt == 0) ?
		     0 : NUMSIZE(wfd->dflt) + DICTOPSIZE(cff_defaultWidthX)) +
		    ((wfd->nominal == 0) ?
		     0 : NUMSIZE(wfd->nominal) + DICTOPSIZE(cff_nominalWidthX)) >=
		    nonoptsize) {
			/* No optimization savings; deactivate optimization values */
			wfd->dflt = -1;
			wfd->nominal = 0;
			return nonoptsize;
		}
	}

	/* Save default and nominal widths in Private dict */
	if (wfd->dflt != 0) {
		dictSaveInt(Private, wfd->dflt);
		DICTSAVEOP(*Private, cff_defaultWidthX);
	}
	if (wfd->nominal != 0) {
		dictSaveInt(Private, wfd->nominal);
		DICTSAVEOP(*Private, cff_nominalWidthX);
	}

	return minsize;
}

/* Post-process recoded font */
static long t13EndFont(tcCtx g) {
	t13Ctx h = g->ctx.t13;
	int i;
	long size = 0;
	long total = (g->flags & TC_SMALLMEMORY) ?
	    h->chars.array[h->chars.cnt - 1].icstr + h->cstrs.cnt : h->cstrs.cnt;

	if (!h->width.opt) {
		return total;   /* No optimization */
	}
	for (i = 0; i <= h->width.fd.cnt; i++) {
		size += sizeFDWidths(h, i);
	}

	return total - h->chars.cnt * DUMMY_WIDTH_SIZE + size;
}

/* Compare stems */
static int cmpStems(t13Ctx h, Stem *pat, Stem *stem) {
	int i;
	int cmp = (pat->type & STEM_VERT) - (stem->type & STEM_VERT);

	if (cmp != 0) {
		return cmp;
	}

	/* Compare edges */
	for (i = 0; i < h->nMasters; i++) {
		if (pat->edge[i] < stem->edge[i]) {
			return -1;
		}
		else if (pat->edge[i] > stem->edge[i]) {
			return 1;
		}
	}

	/* Compare deltas */
	for (i = 0; i < h->nMasters; i++) {
		if (pat->delta[i] < stem->delta[i]) {
			return -1;
		}
		else if (pat->delta[i] > stem->delta[i]) {
			return 1;
		}
	}

	return 0;
}

/* Lookup stem in array and return its index. Returns 1 if found else 0.
   Found or insertion position returned via index parameter. */
static int lookupStem(t13Ctx h, Stem *pat, int *index) {
	int lo = 0;
	int hi = h->stem.list.cnt - 1;

	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		Stem *stem = &h->stem.list.array[mid];
		int cmp = cmpStems(h, pat, stem);
		if (cmp > 0) {
			lo = mid + 1;
		}
		else if (cmp < 0) {
			hi = mid - 1;
		}
		else {
			if (pat->type ^ stem->type) {
				warnChar(h, iHint0);
			}
			stem->type |= pat->type;    /* Add in CNTR bit if present */
			*index = mid;
			return 1;   /* Found */
		}
	}
	*index = lo;
	return 0;   /* Not found */
}

/* Add stem hints */
static void addStems(t13Ctx h, int vert, int cntr) {
	int i;
	Stem stem;

	stem.type = (vert ? STEM_VERT : 0) | (cntr ? STEM_CNTR : 0);

	if (h->stack.cnt < 2 || h->stack.cnt & 1) {
		badChar(h);
	}

	for (i = 0; i < h->stack.cnt; i += 2) {
		int j;
		int index;

		saveElement(h, stem.edge, i);
		saveElement(h, stem.delta, i + 1);

		for (j = 0; j < h->nMasters; j++) {
			if (stem.delta[j] < 0 &&
			    stem.delta[j] != INT2FIX(-20) && stem.delta[j] != INT2FIX(-21)) {
				/* Negative non-edge stem width */
				if (h->nMasters == 1) {
					/* Fix problem by reversing edges */
					warnChar(h, iHint1);
					stem.edge[0] += stem.delta[0];
					stem.delta[0] = -stem.delta[0];
				}
				else {
					/* Just report problem */
					warnChar(h, iHint2);
				}
				break;
			}
		}
		if (!lookupStem(h, &stem, &index)) {
			/* Insert stem */
			Stem *new;

			if (h->stem.list.cnt == T13_MAX_STEMS) {
				break;  /* Limit exceeded; discard remaining stems */
			}
			stem.id = (unsigned char)h->stem.list.cnt;
			new = &da_GROW(h->stem.list, h->stem.list.cnt)[index];
			COPY(new + 1, new, h->stem.list.cnt++ - index);
			*new = stem;

			/* Remember if we have cntr hints. Test here in case of discard */
			h->hint.cntr |= cntr;
		}
		if (h->hint.initial && h->path.segs.cnt > 1) {
			/* Unsubstuted hints after drawing ops; insert hint subs */
			h->hint.initial = 0;
			h->hint.seen = 1;
			h->hint.subs = 1;
		}
		SET_ID_BIT(h->hint.initial ? h->stem.initmask : h->stem.subsmask,
		           h->stem.list.array[index].id);
	}
	h->stack.cnt = 0;
}

/* Fold "vertical" stack values into 1 or more "horizontal" stack values */
static void foldStack(t13Ctx h, int nBlends) {
	int i;
	int nElements = nBlends * h->nMasters;
	int iDst = h->stack.cnt - nElements;
	int j = iDst + nBlends;

	CHKUFLOW(nElements);
	for (i = 0; i < nBlends; i++) {
		int k;
		Fixed *dst = h->stack.array[iDst];
		Fixed first = dst[0];
		for (k = 1; k < h->nMasters; k++) {
			dst[k] = h->stack.array[j++][0] + first;
		}
		h->stack.flags[iDst++] = S_BLEND;
	}
	h->stack.cnt += nBlends - nElements;
}

/* Add operator to path. */
static void addPathOp(t13Ctx h, int nArgs, int iArgs, int type) {
	Operator *p = da_NEXT(h->path.ops);
	p->nArgs = nArgs;
	p->iArgs = iArgs;
	p->iSeg = (unsigned short)h->path.segs.cnt;
	p->type = type;
}

/* Add segment to path. */
static void addPathSeg(t13Ctx h, int type) {
	Segment *p = da_NEXT(h->path.segs);
	p->iArgs = (unsigned short)h->path.args.cnt;
	p->flags = type;
}

/* Handle pending hint state */
static void pendOp(t13Ctx h, int newop) {
	if (h->hint.subs && !(h->g->flags & TC_NOHINTS)) {
		/* Add hint subs id mask */
		int idbytes = (h->stem.list.cnt + 7) / 8;
		addPathOp(h, idbytes, h->path.idmask.cnt, t13_hintmask);
		memcpy(da_EXTEND(h->path.idmask, idbytes), h->stem.subsmask, idbytes);

		CLEAR_ID_MASK(h->stem.subsmask);
		h->hint.subs = 0;
	}
	else if (h->pend.op == tx_dotsection && newop != tx_endchar &&
	         !(h->g->flags & (TC_ROM | TC_NOOLDOPS))) {
		addPathOp(h, 0, 0, t13_dotsection);
	}

	switch (newop) {
		case tx_rlineto:
		case tx_rrcurveto:
		case t2_flex:
			if (h->pend.move) {
				/* Insert missing move at beginning of contour */
				warnChar(h, iMove0);
				addPathSeg(h, seg_move);
				add2Coords(h, -1, -1);
				h->stack.cnt = 0;
				h->pend.move = 0;
			}
			break;

		case t1_closepath:
			h->pend.move = 1;
			break;

		case tx_rmoveto:
			h->pend.move = 0;
			break;
	}
	h->pend.op = newop;
}

/* Add blend x-coordinate */
static void addXCoord(t13Ctx h, Blend coord, int idx) {
	int i;
	if (idx == -1) {
		copyBlend(coord, h->path.x);
	}
	else if (STK_BLEND(idx)) {
		for (i = 0; i < h->nMasters; i++) {
			coord[i] = (h->path.x[i] += h->stack.array[idx][i]);
		}
	}
	else {
		for (i = 0; i < h->nMasters; i++) {
			coord[i] = (h->path.x[i] += INDEX(idx));
		}
	}
}

/* Add blend y-coordinate value */
static void addYCoord(t13Ctx h, Blend coord, int idy) {
	int i;
	if (idy == -1) {
		copyBlend(coord, h->path.y);
	}
	else if (STK_BLEND(idy)) {
		for (i = 0; i < h->nMasters; i++) {
			coord[i] = (h->path.y[i] += h->stack.array[idy][i]);
		}
	}
	else {
		for (i = 0; i < h->nMasters; i++) {
			coord[i] = (h->path.y[i] += INDEX(idy));
		}
	}
}

/* Add 2 coordinates */
static void add2Coords(t13Ctx h, int idx, int idy) {
	if (h->nMasters == 1) {
		Fixed *coord = da_EXTEND(h->path.args, 2);
		coord[0] = (h->path.x[0] += ((idx == -1) ? 0 : INDEX(idx)));
		coord[1] = (h->path.y[0] += ((idy == -1) ? 0 : INDEX(idy)));
	}
	else {
		addXCoord(h, da_EXTEND(h->path.args, h->nMasters), idx);
		addYCoord(h, da_EXTEND(h->path.args, h->nMasters), idy);
	}
}

/* Add moveto op */
static void addMove(t13Ctx h, int idx, int idy) {
	pendOp(h, tx_rmoveto);
	addPathSeg(h, seg_move);
	add2Coords(h, idx, idy);
	h->stack.cnt = 0;
}

/* Add lineto op */
static void addLine(t13Ctx h, int idx, int idy) {
	pendOp(h, tx_rlineto);
	addPathSeg(h, seg_line);
	add2Coords(h, idx, idy);
	h->stack.cnt = 0;
}

/* Add 6 coordinates */
static void add6Coords(t13Ctx h, int idx1, int idy1,
                       int idx2, int idy2, int idx3, int idy3) {
	if (h->nMasters == 1) {
		Fixed *coord = da_EXTEND(h->path.args, 6);
		coord[0] = (h->path.x[0] += ((idx1 == -1) ? 0 : INDEX(idx1)));
		coord[1] = (h->path.y[0] += ((idy1 == -1) ? 0 : INDEX(idy1)));
		coord[2] = (h->path.x[0] += ((idx2 == -1) ? 0 : INDEX(idx2)));
		coord[3] = (h->path.y[0] += ((idy2 == -1) ? 0 : INDEX(idy2)));
		coord[4] = (h->path.x[0] += ((idx3 == -1) ? 0 : INDEX(idx3)));
		coord[5] = (h->path.y[0] += ((idy3 == -1) ? 0 : INDEX(idy3)));
	}
	else {
		addXCoord(h, da_EXTEND(h->path.args, h->nMasters), idx1);
		addYCoord(h, da_EXTEND(h->path.args, h->nMasters), idy1);
		addXCoord(h, da_EXTEND(h->path.args, h->nMasters), idx2);
		addYCoord(h, da_EXTEND(h->path.args, h->nMasters), idy2);
		addXCoord(h, da_EXTEND(h->path.args, h->nMasters), idx3);
		addYCoord(h, da_EXTEND(h->path.args, h->nMasters), idy3);
	}
}

/* Add curve to operation */
static void addCurve(t13Ctx h, int idx1, int idy1,
                     int idx2, int idy2, int idx3, int idy3) {
	pendOp(h, tx_rrcurveto);
	addPathSeg(h, seg_curve);
	add6Coords(h, idx1, idy1, idx2, idy2, idx3, idy3);
	h->stack.cnt = 0;
}

/* Add single value without changing current x or y coordinate */
static void addValue(t13Ctx h, int iv) {
	if (h->nMasters == 1) {
		*da_NEXT(h->path.args) = INDEX(iv);
	}
	else {
		int i;
		Fixed *coord = da_EXTEND(h->path.args, h->nMasters);
		if (STK_BLEND(iv)) {
			copyBlend(coord, h->stack.array[iv]);
		}
		else {
			for (i = 0; i < h->nMasters; i++) {
				coord[i] = INDEX(iv);
			}
		}
	}
}

/* Add operator and args (if any) and adjust stack */
static void addOp(t13Ctx h, int op, int t13op) {
	int i;

	pendOp(h, op);
	addPathOp(h, h->stack.cnt, h->path.args.cnt, t13op);

	for (i = 0; i < h->stack.cnt; i++) {
		addValue(h, i);
	}
	h->stack.cnt = 0;
}

/* Perform various kinds of division */
static void doDiv(t13Ctx h, int shift) {
	if (h->stack.cnt < 2) {
		/* This div operator depended on computed results; just save it */
		addOp(h, tx_div, t13_div);
	}
	else if (shift) {
		/* Strange (12 12) div with funky shifting behavior */
		double a = h->intDiv ? FIX2DBL(POP_FIX()) : POP_FIX();
		double b = POP_FIX();
		PUSH_FIX(DBL2FIX(b / a));
		h->intDiv = 0;
	}
	else {
		/* Well-behaved div2 (12 25) or otherDiv (23) */
		double a = POP_FIX();
		double b = POP_FIX();
		PUSH_FIX(DBL2FIX(b / a));
	}
}

/* Add flex */
static void addFlex(t13Ctx h) {
	pendOp(h, t2_flex);
	CHKUFLOW(17);

	/* Add referece point and first control point relative moves */
	addElement(h, 2, 0);
	addElement(h, 3, 1);

	addPathOp(h, 1, h->path.args.cnt, t2_flex);
	addValue(h, 14);    /* Flex height */

	addPathSeg(h, seg_curve);
	add6Coords(h, 2, 3,  4,  5,  6,  7);
	addPathSeg(h, seg_curve);
	add6Coords(h, 8, 9, 10, 11, 12, 13);

	/* Check that the current point agrees with the redundant one in flex op.
	   In the event of mismatch report it and update the current point from the
	   value stored with the flex op. This mimics the way that the rasterizer
	   functions and ensures that incorrectly constructed fonts will be
	   faithfully reproduced even when wrong! The major source of these errors
	   is a result of a BuildFont off-by-one bug when moving from a closepoint
	   contour to a new contour in MM fonts. */
	if (h->nMasters == 1) {
		if (h->path.x[0] != INDEX(15) || h->path.y[0] != INDEX(16)) {
			warnChar(h, iFlex0);
			h->path.x[0] = INDEX(15);
			h->path.y[0] = INDEX(16);
		}
	}
	else {
		int i;
		if (!STK_BLEND(15)) {
			setBlend(h, h->stack.array[15], INDEX(15));
		}
		if (!STK_BLEND(16)) {
			setBlend(h, h->stack.array[16], INDEX(16));
		}
		for (i = 0; i < h->nMasters; i++) {
			if (h->path.x[i] != h->stack.array[15][i] ||
			    h->path.y[i] != h->stack.array[16][i]) {
				warnChar(h, iFlex0);
				copyBlend(h->path.x, h->stack.array[15]);
				copyBlend(h->path.y, h->stack.array[16]);
				break;
			}
		}
	}

	h->stack.cnt = 0;
}

/* Add counter control list (othersubrs 12/13) */
static void addCntrCntl(t13Ctx h, int op, int argcnt) {
	int i;
	Blend *ctr;

	if (h->stack.cnt != argcnt || h->stack.cnt < 2) {
		badChar(h);
	}

	if (op == t1_otherCntr1 && argcnt != 22) {
		warnChar(h, iMisc0);
	}

	ctr = da_EXTEND(h->cntr.list, argcnt);
	for (i = h->stack.cnt - 1; i >= 0; i--) {
		saveElement(h, *ctr++, i);
	}

	h->stack.cnt = 0;
}

/* Parse type 1 charstring. Called recursively. Return 1 on endchar else 0 */
static int t1parse(t13Ctx h, unsigned length, unsigned char *cstr,
                   unsigned nSubrs, Charstring *subrs) {
	tcCtx g = h->g;
	unsigned i = 0;
#if TC_STATISTICS
	g->stats.flatSize += length;
#endif /* TC_STATISTICS */
	while (i < length) {
		switch (cstr[i]) {
			case tx_hstem:
				addStems(h, 0, 0);
				break;

			case t1_compose:
				parseFatal(g, "unsupported operator [compose]");

			case tx_vstem:
				addStems(h, 1, 0);
				break;

			case tx_vmoveto:
				CHKUFLOW(1);
				if (h->pend.flex) {
					/* Insert zero dx coordinate */
					if (h->nMasters == 1) {
						PUSH_FIX(INDEX(h->stack.cnt - 1));
						SET(h->stack.cnt - 2, 0);
					}
					else {
						CHKOFLOW(1);
						moveElement(h, h->stack.cnt, h->stack.cnt - 1);
						SET(h->stack.cnt - 1, 0);
						h->stack.cnt++;
					}
				}
				else {
					addMove(h, -1, 0);
				}
				break;

			case tx_rlineto:
				CHKUFLOW(2);
				addLine(h, 0, 1);
				break;

			case tx_hlineto:
				CHKUFLOW(1);
				addLine(h, 0, -1);
				break;

			case tx_vlineto:
				CHKUFLOW(1);
				addLine(h, -1, 0);
				break;

			case tx_rrcurveto:
				CHKUFLOW(6);
				addCurve(h, 0, 1, 2, 3, 4, 5);
				break;

			case t1_closepath:
				pendOp(h, t1_closepath);
				break;

			case tx_callsubr:
				CHKUFLOW(1);
				{
					unsigned isubr = POP_INT();
					if (isubr >= nSubrs) {
						badChar(h);
					}
#if TC_DEBUG
					if (h->debug.input) {
						printf("--- subr[%d]\n", isubr);
						csDump(subrs[isubr].length,
						       (unsigned char *)subrs[isubr].cstr, h->nMasters, 1);
					}
#endif /* TC_DEBUG */
					if (t1parse(h, subrs[isubr].length,
					            (unsigned char *)subrs[isubr].cstr, nSubrs, subrs)) {
						return 1;
					}
				}
				break;

			case tx_return:
				return 0;

			case t1_hsbw:
				CHKUFLOW(2);
				if (h->nMasters == 1) {
					h->path.lsb.x[0] = INDEX(0);
					h->path.lsb.y[0] = 0;
					h->path.x[0] = INDEX(0);
					h->path.y[0] = 0;
					h->path.width[0] = INDEX(1);
					if (h->width.opt) {
						addWidth(h, INDEX(1));
					}
				}
				else {
					saveElement(h, h->path.lsb.x, 0);
					setBlend(h, h->path.lsb.y, 0);
					saveElement(h, h->path.x, 0);
					setBlend(h, h->path.y, 0);
					saveElement(h, h->path.width, 1);
				}
				h->stack.cnt = 0;
				break;

			case tx_endchar:
				pendOp(h, tx_endchar);
				addPathOp(h, 0, 0, tx_endchar);
				return 1;

			case t1_moveto:
				CHKUFLOW(2);
				addMove(h, 0, 1);
				break;

			case t1_curveto:
				parseFatal(g, "unsupported operator [curveto]");
				break;

			case tx_rmoveto:
				CHKUFLOW(2);
				if (!h->pend.flex) {
					addMove(h, 0, 1);
				}
				break;

			case tx_hmoveto:
				CHKUFLOW(1);
				if (h->pend.flex) {
					PUSH_INT(0);
				}
				else {
					addMove(h, 0, -1);
				}
				break;

			case tx_vhcurveto:
				CHKUFLOW(4);
				addCurve(h, -1, 0, 1, 2, 3, -1);
				break;

			case tx_hvcurveto:
				CHKUFLOW(4);
				addCurve(h, 0, -1, 1, 2, -1, 3);
				break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				PUSH_INT(108 + 256 * (cstr[i] - 247) + cstr[i + 1]);
				i += 2;
				continue;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				PUSH_INT(-108 - 256 * (cstr[i] - 251) - cstr[i + 1]);
				i += 2;
				continue;

			case 255:
			{
				/* 5 byte number */
				long value = (long)cstr[i + 1] << 24 | (long)cstr[i + 2] << 16 |
				    (long)cstr[i + 3] << 8 | (long)cstr[i + 4];
				if (-32000 <= value && value <= 32000) {
					value <<= 16;
				}
				else {
					h->intDiv = 1;
				}
				PUSH_FIX(value);
				i += 5;
				continue;
			}

			default:
				/* 1 byte number */
				PUSH_INT(cstr[i] - 139);
				break;

			case tx_reserved0:
			case t1_reserved16:
			case t1_reserved18:
			case t1_reserved19:
			case t1_reserved20:
			case t1_reserved23:
			case t1_reserved24:
			case t1_reserved25:
			case t1_reserved26:
			case t1_reserved27:
			case t1_reserved28:
			case t1_reserved29:
				badChar(h);
				break;

			case tx_escape:
				/* Process escaped operator */
				switch (tx_ESC(cstr[i + 1])) {
					case tx_dotsection:
						pendOp(h, tx_dotsection);
						break;

					case t1_vstem3:
						addStems(h, 1, 1);
						break;

					case t1_hstem3:
						addStems(h, 0, 1);
						break;

					case tx_and:
						addOp(h, tx_and, t13_and);
						break;

					case tx_or:
						addOp(h, tx_or, t13_or);
						break;

					case tx_not:
						addOp(h, tx_not, t13_not);
						break;

					case t1_seac:
						CHKUFLOW(5);
						if (h->nMasters == 1) {
							INDEX(1) += -INDEX(0) + h->path.lsb.x[0];
						}
						else {
							subElement(h, 1, 0);
							addBlend(h, h->stack.array[1], h->stack.array[1],
							         h->path.lsb.x);
						}
						addPathOp(h, 4, h->path.args.cnt, tx_endchar);
						addValue(h, 1);
						addValue(h, 2);
						addValue(h, 3);
						addValue(h, 4);
						if (h->g->status & TC_SUBSET) {
							/* Add components to subset glyph list */
							if (parseAddComponent(h->g, INDEX(3) >> 16) ||
							    parseAddComponent(h->g, INDEX(4) >> 16)) {
								badChar(h);
							}
						}
						return 1;

					case t1_sbw:
						CHKUFLOW(4);
						if ((!STK_BLEND(3) && INDEX(3) != 0) ||
						    (STK_BLEND(3) && !cmpBlend(h, h->stack.array[2], 0))) {
							parseFatal(g, "sbw with vertical width unsupported");
						}
						if (h->nMasters == 1) {
							h->path.lsb.x[0] = INDEX(0);
							h->path.lsb.y[0] = INDEX(1);
							h->path.x[0] = INDEX(0);
							h->path.y[0] = INDEX(1);
							h->path.width[0] = INDEX(2);
							if (h->width.opt) {
								addWidth(h, INDEX(2));
							}
						}
						else {
							saveElement(h, h->path.lsb.x, 0);
							saveElement(h, h->path.lsb.y, 1);
							saveElement(h, h->path.x, 0);
							saveElement(h, h->path.y, 1);
							saveElement(h, h->path.width, 2);
						}
						h->stack.cnt = 0;
						break;

					case tx_store:
						addOp(h, tx_store, t13_store);
						break;

					case tx_abs:
						addOp(h, tx_abs, t13_abs);
						break;

					case tx_add:
						addOp(h, tx_add, t13_add);
						break;

					case tx_sub:
						addOp(h, tx_sub, t13_sub);
						break;

					case tx_div:
						doDiv(h, 1);
						break;

					case tx_load:
						addOp(h, tx_load, t13_load);
						break;

					case tx_neg:
						addOp(h, tx_neg, t13_neg);
						break;

					case tx_eq:
						addOp(h, tx_eq, t13_eq);
						break;

					case t1_callother:
						CHKUFLOW(2);
						{
							int othersubr = POP_INT();
							int argcnt = POP_INT();
							switch (othersubr) {
								case t1_otherFlex:
									addFlex(h);
									break;

								case t1_otherPreflex1:
									h->pend.flex = 1;
									break;

								case t1_otherPreflex2:
									break; /* Discard */

								case t1_otherHintSubs:
									if (h->hint.initial) {
										h->hint.initial = 0;
										h->hint.seen = 1;
									}
									if (h->hint.subs) {
										warnChar(h, iHint3);
										CLEAR_ID_MASK(h->stem.subsmask);
									}
									else {
										h->hint.subs = 1;
									}
									break;

								case t1_otherGlobalColor:
									addOp(h, t2_cntron, t13_cntron);
									break;

								case t1_otherCntr1:
									addCntrCntl(h, t1_otherCntr1, argcnt);
									break;

								case t1_otherCntr2:
									addCntrCntl(h, t1_otherCntr2, argcnt);
									break;

								case t1_otherBlend1:
									foldStack(h, 1);
									break;

								case t1_otherBlend2:
									foldStack(h, 2);
									break;

								case t1_otherBlend3:
									foldStack(h, 3);
									break;

								case t1_otherBlend4:
									foldStack(h, 4);
									break;

								case t1_otherBlend6:
									foldStack(h, 6);
									break;

								case t1_otherStoreWV:
									CHKUFLOW(1);
									{
										/* Convert op to (0 0 offset count store) */
										Fixed offset = POP_FIX();
										PUSH_INT(TX_REG_WV);
										PUSH_INT(0);
										PUSH_FIX(offset);
										PUSH_INT(h->nMasters);
										addOp(h, tx_store, t13_store);
									}
									break;

								case t1_otherAdd:
									addOp(h, tx_add, t13_add);
									break;

								case t1_otherSub:
									addOp(h, tx_sub, t13_add);
									break;

								case t1_otherMul:
									addOp(h, tx_mul, t13_mul);
									break;

								case t1_otherDiv:
									doDiv(h, 0);
									break;

								case t1_otherPut:
									addOp(h, tx_put, t13_put);
									break;

								case t1_otherGet:
									addOp(h, tx_get, t13_get);
									break;

								case t1_otherIfelse:
									addOp(h, tx_ifelse, t13_ifelse);
									break;

								case t1_otherRandom:
									addOp(h, tx_random, t13_random);
									break;

								case t1_otherDup:
									addOp(h, tx_dup, t13_dup);
									break;

								case t1_otherExch:
									addOp(h, tx_exch, t13_exch);
									break;

								case t1_otherPSPut:
								default:
									badChar(h);
									break;
							}
							break;
						} /* End of othersubrs */

					case t1_pop:
						break; /* Discard */

					case tx_drop:
						addOp(h, tx_drop, t13_drop);
						break;

					case t1_setwv:
						parseFatal(g, "unsupported operator [setwv]");
						break;

					case tx_put:
						addOp(h, tx_put, t13_put);
						break;

					case tx_get:
						addOp(h, tx_get, t13_get);
						break;

					case tx_ifelse:
						addOp(h, tx_ifelse, t13_ifelse);
						break;

					case tx_random:
						addOp(h, tx_random, t13_random);
						break;

					case tx_mul:
						addOp(h, tx_mul, t13_mul);
						break;

					case t1_div2:
						doDiv(h, 0);
						break;

					case tx_sqrt:
						addOp(h, tx_sqrt, t13_sqrt);
						break;

					case tx_dup:
						addOp(h, tx_dup, t13_dup);
						break;

					case tx_exch:
						addOp(h, tx_exch, t13_exch);
						break;

					case tx_index:
						addOp(h, tx_index, t13_index);
						break;

					case tx_roll:
					{
						Fixed n;
						CHKUFLOW(2);
						n = INDEX(h->stack.cnt - 2) >> 16;
						if (n < 0) {
							badChar(h);
						}
						addOp(h, tx_roll, t13_roll);
						break;
					}

					case t1_setcurrentpt:
						if (h->pend.flex) {
							h->pend.flex = 0;
						}
						else {
							/* Convert out-of-context setcurrentpt to moveto */
							warnChar(h, iMisc1);
							CHKUFLOW(2);
							saveElement(h, h->path.x, 0);
							saveElement(h, h->path.y, 0);
							h->stack.cnt = 0;
						}
						break;

					case tx_reservedESC31:
					case tx_reservedESC32:
					default:
						badChar(h);
				}
				i += 2;
				continue;
		}
		i++;
	}
	return 1;
}

/* Push single or blended value on stack */
static void pushValue(t13Ctx h, Blend value) {
	if (h->nMasters != 1) {
		int i;
		for (i = 0; i < h->nMasters; i++) {
			if (value[i] != value[0]) {
				copyBlend(h->stack.array[h->stack.cnt], value);
				h->stack.flags[h->stack.cnt++] = S_BLEND;
				if (h->stack.cnt + h->nMasters + 1 > h->stack.limit) {
					h->stack.limit = h->stack.cnt + h->nMasters + 1;
				}
				return;
			}
		}
	}
	h->stack.array[h->stack.cnt][0] = value[0];
	h->stack.flags[h->stack.cnt++] = 0;
	if (h->stack.limit < h->stack.cnt) {
		h->stack.limit = h->stack.cnt;
	}
}

/* Save group of blend or non-blend elements */
static void saveElements(t13Ctx h, int iFirst, int cnt) {
	char *t;
	char *end;
	int max;
	int i;

	if (STK_BLEND(iFirst)) {
		/* Save blend group */
		max = cnt * h->nMasters * 5 + 2;
		t = da_EXTEND(h->cstrs, max);
		end = t + max;

		/* Save initial values */
		for (i = 0; i < cnt; i++) {
			t += t13EncFixed(h->stack.array[iFirst + i][0], t);
		}

		/* Save deltas */
		for (i = 0; i < cnt; i++) {
			int j;
			Fixed *blend = h->stack.array[iFirst + i];
			for (j = 1; j < h->nMasters; j++) {
				t += t13EncFixed(blend[j] - blend[0], t);
			}
		}

		/* save blend count and op */
		t += t13EncInteger(cnt, t);
		*t++ = (unsigned char)t13_blend;
	}
	else {
		/* Save non-blend group */
		max = cnt * 5;
		t = da_EXTEND(h->cstrs, max);
		end = t + max;

		for (i = 0; i < cnt; i++) {
			t += t13EncFixed(INDEX(iFirst + i), t);
		}
	}
	h->cstrs.cnt -= end - t;    /* Only count bytes used */
}

/* Save stack elements */
static void saveStack(t13Ctx h) {
	if (h->stack.cnt == 0) {
		return; /* Stack empty */
	}
	else if (h->nMasters == 1) {
		saveElements(h, 0, h->stack.cnt);
	}
	else {
		/* Divide stack into blended and non-blended groups */
		int i;
		int iGroup = 0;
		for (i = 1; i <= h->stack.cnt; i++) {
			if (i == h->stack.cnt ||
			    STK_BLEND(iGroup) != STK_BLEND(i) ||
			    (STK_BLEND(i) &&
			     i + (i - iGroup + 1) * h->nMasters > T13_MAX_OP_STACK)) {
				saveElements(h, iGroup, i - iGroup);
				iGroup = i;
			}
		}
	}
	h->stack.cnt = 0;
	h->stack.limit = 0;
}

/* Save stem args and ops observing stack limit */
static void saveStemOp(t13Ctx h,
                       int iStart, int iEnd, int vert, int optimize) {
	Blend last;
	int i;
	int op = h->hint.seen ?
	    (vert ? t13_vstemhm : t13_hstemhm) : (vert ? t13_vstem : t13_hstem);

	if (iStart == iEnd) {
		return; /* No stems in this direction */
	}
	setBlend(h, last, 0);
	for (i = iStart; i < iEnd; i++) {
		int space;
		Blend edge;
		Stem *stem = &h->stem.list.array[i];

		/* Compute stem args */
retry:
		if (h->nMasters == 1) {
			edge[0] = stem->edge[0] - last[0];
			last[0] = stem->edge[0] + stem->delta[0];
			edge[0] += (vert ? h->path.lsb.x[0] : h->path.lsb.y[0]);
			last[0] += (vert ? h->path.lsb.x[0] : h->path.lsb.y[0]);
			space = 2;
		}
		else {
			int dspace;

			subBlend(h, edge, stem->edge, last);
			addBlend(h, last, stem->edge, stem->delta);
			addBlend(h, edge, edge, vert ? h->path.lsb.x : h->path.lsb.y);
			addBlend(h, last, last, vert ? h->path.lsb.x : h->path.lsb.y);

			/* Determine stack depth for args */
			space = spaceBlend(h, edge);
			dspace = spaceBlend(h, stem->delta);
			if (space <= dspace) {
				space = 1 + dspace;
			}
		}

		if (h->stack.limit + space > T13_MAX_OP_STACK) {
			/* Stack full; save stack and stem op */
			saveStack(h);
			if (!optimize || i != iEnd) {
				T13SAVEOP(op, h->cstrs);
			}
			setBlend(h, last, 0);
			goto retry;
		}
		else {
			pushValue(h, edge);
			pushValue(h, stem->delta);
		}
	}
	saveStack(h);
	if (!optimize || i != iEnd) {
		T13SAVEOP(op, h->cstrs);
	}
}

/* Set counter control group id mask */
static Blend *setCntrMask(t13Ctx h, Blend *p, int vert, HintMask cntrmask) {
	int index;
	int i;
	int last;
	Stem stem;

	setBlend(h, stem.edge, 0);
	setBlend(h, stem.delta, 0);
	stem.type = vert ? STEM_VERT : 0;

	last = 0;
	do {
		/* Get next stem */
		if (h->nMasters == 1) {
			stem.edge[0] += stem.delta[0] + (*p--)[0];
			stem.delta[0] = (*p--)[0];
			if (stem.delta[0] < 0) {
				/* Last stem in group */
				stem.edge[0] += stem.delta[0];
				stem.delta[0] = -stem.delta[0];
				last = 1;
			}
		}
		else {
			addBlend(h, stem.edge, stem.edge, stem.delta);
			addBlend(h, stem.edge, stem.edge, *p--);
			copyBlend(stem.delta, *p);
			p--;
			if (stem.delta[0] < 0) {
				/* Last stem in group */
				addBlend(h, stem.edge, stem.edge, stem.delta);
				negBlend(h, stem.delta);
				last = 1;
			}
		}

		if (lookupStem(h, &stem, &index)) {
			stem.id = h->stem.list.array[index].id;
		}
		else {
#if TC_GC_MATCH
			/* Stem not found. This occurs because the global coloring data is
			   derived from a stem list that may not be the same as the stem
			   data in the charstring due to stem edge mapping that is tolerant
			   to within 2 charspace units */
			if (h->stem.list.cnt != T13_MAX_STEMS) {
				/* Insert dummy stem so that the global coloring data exactly
				   matches the original and glyph bitmap comparisons succeed */
				Stem *new = &da_GROW(h->stem.list, h->stem.list.cnt)[index];

				stem.id = (unsigned char)h->stem.list.cnt;
				COPY(new + 1, new, h->stem.list.cnt++ - index);
				*new = stem;
			}
			else
#endif
			{
				/* Stem list full; just find the best match */
				Fixed smallest = LONG_MAX;

				for (i = 0; i < h->stem.list.cnt; i++) {
					int j;
					Fixed delta = 0;
					Stem *cand = &h->stem.list.array[i];

					if (stem.type != cand->type) {
						continue;
					}

					for (j = 0; j < h->nMasters; j++) {
						delta += ABS(stem.edge[j] - cand->edge[j]) +
						    ABS((stem.edge[j] + stem.delta[j]) -
						        (cand->edge[j] + cand->delta[j]));
					}

					if (delta < smallest) {
						/* Update smallest delta */
						smallest = delta;
						stem.id = cand->id;
					}
				}
			}
		}
		/* Add stem to id mask */
		SET_ID_BIT(cntrmask, stem.id);
	}
	while (!last);

	return p;
}

/* Set counter control mask groups. The original counter list from the
   charstring is contructed in reverse order and read back to front. */
static void setCntrMaskGroups(t13Ctx h) {
	int i;
	HintMask *cntrmask;
	Blend *pVert;
	int nVert;
	Blend *pHoriz = &h->cntr.list.array[h->cntr.list.cnt - 1]; /* Last element*/
	int nHoriz = (*pHoriz--)[0] >> 16;

	/* Search backwards for start of vertical data */
	pVert = pHoriz;
	for (i = 0; i < nHoriz; i++) {
		pVert--;
		while ((*pVert)[0] > 0) {
			pVert -= 2;
		}
		pVert--;
	}
	nVert = (*pVert--)[0] >> 16;

	if (nHoriz == 0 && pHoriz == 0) {
		/* Save zero cntrmask to turn off counter control */
		cntrmask = da_NEXT(h->cntr.idmask);
		CLEAR_ID_MASK(*cntrmask);
	}
	else {
		do {
			/* Each iteration saves the group(s) for one priority */
			cntrmask = da_NEXT(h->cntr.idmask);
			CLEAR_ID_MASK(*cntrmask);

			if (nHoriz-- > 0) {
				pHoriz = setCntrMask(h, pHoriz, 0, *cntrmask);
			}
			if (nVert-- > 0) {
				pVert = setCntrMask(h, pVert, 1, *cntrmask);
			}
		}
		while (nHoriz > 0 || nVert > 0);
	}
}

#if TC_HINT_CHECK
/* Test for hint overlap */
static void checkOverlap(t13Ctx h, HintMask hintmask) {
	int i;
	int j;
	int bitsleft = h->stem.list.cnt;
	Stem *last = NULL;

	if (h->cntr.insert) {
		return; /* Reject masks if compatibility stems inserted */
	}
	/* Find bits set and compare corresponding stems */
	for (i = 0; i < h->hint.size; i++) {
		if (hintmask[i] != 0) {
			int lastbit = (bitsleft > 7) ? 0 : 8 - bitsleft;
			for (j = 7; j >= lastbit; j--) {
				if (hintmask[i] & 1 << j) {
					Stem *curr = &h->stem.list.array[i * 8 + 7 - j];
					if (last != NULL &&
					    ((last->type ^ curr->type) & STEM_VERT) == 0 &&
					    ((last->delta[0] >= 0 &&
					      last->edge[0] + last->delta[0] >= curr->edge[0]) ||
					     (last->delta[0] < 0 &&
					      last->edge[0] == curr->edge[0]))) {
						warnChar(h, iHint4);
					}
					last = curr;
				}
			}
		}
		bitsleft -= 8;
	}
}

#endif /* TC_HINT_CHECK */

/* Save hint mask op */
static void saveHintMaskOp(t13Ctx h, unsigned char *map, int op,
                           int idsize, HintMask idmask, int check) {
	int i;
	HintMask hintmask;

	/* Construct hintmask from stem ids */
	CLEAR_HINT_MASK(hintmask);
	for (i = 0; i < idsize * 8; i++) {
		if (TEST_ID_BIT(idmask, i)) {
			SET_HINT_BIT(hintmask, map[i]);
		}
	}

	if (check && memcmp(h->stem.lastmask, hintmask, h->hint.size) == 0) {
		warnChar(h, iHint5);
		return;
	}
	memcpy(h->stem.lastmask, hintmask, h->hint.size);

	/* Save mask op */
	T13SAVEOP(op, h->cstrs);

#if TC_SUBR_SUPPORT
	if (h->g->flags & TC_SUBRIZE) {
		/* Save mask op size in second byte for subroutinizer */
		*da_NEXT(h->cstrs) = h->hint.size + 2;
	}
#endif /* TC_SUBR_SUPPORT */

	SAVE_HINT_MASK(hintmask);

#if TC_HINT_CHECK
	/* Counter masks (counter control list) aren't checked because it yields
	   too many warnings due to stem merging and integerization of list */
	if (op == t13_hintmask) {
		checkOverlap(h, hintmask);
	}
#endif /* TC_HINT_CHECK */
}

/* Save stem hints */
static void saveStems(t13Ctx h, unsigned char *map) {
	int i;
	int ivstem;
	int superset;   /* Flags if stem list is superset of initial hints */

	if (h->stem.list.cnt == 0) {
		/* Charstring has no stem hints */
		if (h->path.segs.cnt > 1) {
			/* Warn of unhinted glyphs with a path. I test for > 1 operators
			   so as not to select the glyphs with a move followed by endchar
			   that are common in space glyphs from older fonts */
			warnChar(h, iHint6);
		}
		return;
	}

	if (h->cntr.list.cnt) {
		/* Construct counter control masks */
		setCntrMaskGroups(h);
	}

	h->hint.size = (h->stem.list.cnt + 7) / 8;

	/* Construct stems id map */
	for (i = 0; i < h->stem.list.cnt; i++) {
		map[h->stem.list.array[i].id] = i;
	}

	/* Check if stems list is superset of initial hints */
	superset = 0;
	if (!(h->g->flags & TC_ROM) || h->path.ops.array[0].type != t2_hintmask) {
		for (i = 0; i < h->stem.list.cnt; i++) {
			if (!TEST_ID_BIT(h->stem.initmask, i)) {
				superset = 1;
				break;
			}
		}
	}

	/* Get hstem index */
	ivstem = h->stem.list.cnt;
	for (i = 0; i < h->stem.list.cnt; i++) {
		if (h->stem.list.array[i].type & STEM_VERT) {
			ivstem = i;
			break;
		}
	}

	/* Save hstems then vstems */
	saveStemOp(h, 0, ivstem, 0, 0);
	saveStemOp(h, ivstem, h->stem.list.cnt, 1,
	           h->hint.cntr || superset || h->cntr.idmask.cnt);

	if (h->hint.cntr) {
		/* Save h/vstem3 counter mask */
		HintMask cntrmask;

		CLEAR_HINT_MASK(cntrmask);
		for (i = 0; i < h->stem.list.cnt; i++) {
			if (h->stem.list.array[i].type & STEM_CNTR) {
				SET_HINT_BIT(cntrmask, i);
			}
		}

		T13SAVEOP(t13_cntrmask, h->cstrs);

#if TC_SUBR_SUPPORT
		if (h->g->flags & TC_SUBRIZE) {
			/* Save mask op size in second byte for subroutinizer */
			*da_NEXT(h->cstrs) = h->hint.size + 2;
		}
#endif /* TC_SUBR_SUPPORT */

		SAVE_HINT_MASK(cntrmask);
	}
	else {
		/* Save counter control masks (if any) */
		for (i = 0; i < h->cntr.idmask.cnt; i++) {
			saveHintMaskOp(h, map, t13_cntrmask,
			               h->hint.size, h->cntr.idmask.array[i], 0);
		}
	}

	if (superset) {
		/* Save initial hints */
		saveHintMaskOp(h, map, t13_hintmask, h->hint.size, h->stem.initmask, 0);
	}

#if TC_HINT_CHECK
	if (!h->hint.seen) {
		/* No hint substitution so check all hints for overlap */
		HintMask hintmask;

		/* Set all hint bits */
		for (i = 0; i < h->hint.size; i++) {
			hintmask[i] = (unsigned char)0xff;
		}

		checkOverlap(h, hintmask);
	}
#endif /* TC_HINT_CHECK */
}

/* Compute and load delta args from path coords and update current point */
static void loadDeltaArgs(t13Ctx h, unsigned index, int nDeltas) {
	int i;
	int j;
	Fixed *coord = &h->path.args.array[index];

	/* Compute x-coord deltas */
	for (i = 0; i < nDeltas; i += 2) {
		Fixed *x = &coord[i * h->nMasters];
		Fixed *dx = h->args.array[i];

		/* Set deltas */
		for (j = 0; j < h->nMasters; j++) {
			dx[j] = x[j] - h->path.x[j];
			h->path.x[j] = x[j];
		}

		/* Set flags */
		for (j = 1; j < h->nMasters; j++) {
			if (dx[j] != dx[0]) {
				h->args.flag[i] = A_BLND;
				goto diff_x;
			}
		}
		h->args.flag[i] = (dx[0] == 0) ? A_ZERO : 0;
diff_x:;
	}

	/* Compute y-coord deltas */
	for (i = 1; i < nDeltas; i += 2) {
		Fixed *y = &coord[i * h->nMasters];
		Fixed *dy = h->args.array[i];

		/* Set deltas */
		for (j = 0; j < h->nMasters; j++) {
			dy[j] = y[j] - h->path.y[j];
			h->path.y[j] = y[j];
		}

		/* Set flags */
		for (j = 1; j < h->nMasters; j++) {
			if (dy[j] != dy[0]) {
				h->args.flag[i] = A_BLND;
				goto diff_y;
			}
		}
		h->args.flag[i] = (dy[0] == 0) ? A_ZERO : 0;
diff_y:;
	}

	h->args.cnt += nDeltas;
}

/* Load args from path coords */
static void loadArgs(t13Ctx h, unsigned index, int nArgs) {
	int i;
	int j;
	char *flag = &h->args.flag[h->args.cnt];
	Fixed *coord = &h->path.args.array[index];

	/* Compute x-coord deltas */
	for (i = 0; i < nArgs; i++) {
		Fixed *arg = h->args.array[h->args.cnt + i];

		copyBlend(arg, &coord[i * h->nMasters]);

		/* Set flags */
		for (j = 1; j < h->nMasters; j++) {
			if (arg[j] != arg[0]) {
				flag[i] = A_BLND;
				goto diff;
			}
		}
		flag[i] = (arg[0] == 0) ? A_ZERO : 0;
diff:;
	}
	h->args.cnt += nArgs;
}

/* Swap 2 args */
static void swapArgs(t13Ctx h, int i, int j) {
	Fixed value[T13_MAX_MASTERS];
	char flag;
	if (h->nMasters == 1) {
		value[0] = h->args.array[i][0];
		h->args.array[i][0] = h->args.array[j][0];
		h->args.array[j][0] = value[0];
	}
	else {
		copyBlend(value, h->args.array[i]);
		copyBlend(h->args.array[i], h->args.array[j]);
		copyBlend(h->args.array[j], value);
	}
	flag = h->args.flag[i];
	h->args.flag[i] = h->args.flag[j];
	h->args.flag[j] = flag;
}

/* Recode flex. Curve 1 in args 0-5 and curve 2 in args 8-11. Arg 12 is flex
   height which is 50/100 pixels by default. The flex type (hflex, hflex1,
   flex1, and flex) is selected according to the values of the 2 curve args and
   the flex height arg:

   [Note: X marks saved arg]

   args  | 0   1   2   3   4   5 | 6   7   8   9  10  11 |12|
   values|dx1 dy1 dx2 dy2 dx3 dy3|dx4 dy4 dx5 dy5 dx6 dy6|fh|type
   ------+-----------------------+-----------------------+--+----
           X   0   X   X   X   0   X   0   X       X   0     hflex
           X   X   X   X   X   0   X   0   X   X   X         hflex1
           X   X   X   X   X   X   X   X   X   X   X         flex1 (h)
           X   X   X   X   X   X   X   X   X   X       X     flex1 (v)
           X   X   X   X   X   X   X   X   X   X   X   X   X flex
 */
static int doFlex(t13Ctx h, int iCoords, int iHeight) {
	int dxz;        /* Flags zero-valued dx */
	int dyz;        /* Flags zero-valued dy */
	int stdHeight;  /* Flags standard flex height */
	Blend dx;       /* Combined curves dx */
	Blend dy;       /* Combined curves dy */
	Blend pendx;    /* Penultimate combined curves dx */
	Blend pendy;    /* Penultimate combined curves dy */
	Fixed *coord = &h->path.args.array[iCoords];
	Fixed *height = &h->path.args.array[iHeight];

	/* Compute spanning deltas */
	if (h->nMasters == 1) {
		pendx[0] = coord[8] - h->path.x[0];
		pendy[0] = coord[9] - h->path.y[0];
		dx[0] = coord[10] - h->path.x[0];
		dy[0] = coord[11] - h->path.y[0];
		dxz = dx[0] == 0;
		dyz = dy[0] == 0;
		stdHeight = height[12] == DBL2FIX(T13_STD_FLEX_HEIGHT);
	}
	else {
		subBlend(h, pendx, &coord[8 * h->nMasters], h->path.x);
		subBlend(h, pendy, &coord[9 * h->nMasters], h->path.y);
		subBlend(h, dx, &coord[10 * h->nMasters], h->path.x);
		subBlend(h, dy, &coord[11 * h->nMasters], h->path.y);
		dxz = cmpBlend(h, dx, 0);
		dyz = cmpBlend(h, dy, 0);
		stdHeight = cmpBlend(h, &height[0], DBL2FIX(T13_STD_FLEX_HEIGHT));
	}

	/* Load deltas */
	loadDeltaArgs(h, iCoords, 12);
	loadArgs(h, iHeight, 1);

	/* Select args common to all flex ops */
	ARG_SELECT(0);
	ARG_SELECT(2);
	ARG_SELECT(3);
	ARG_SELECT(4);
	ARG_SELECT(6);
	ARG_SELECT(8);

	if (!(dxz || dyz) || !stdHeight) {
		/* flex -- the worst case */
		if (!dxz && !dyz) {
			warnChar(h, iFlex1);
		}
flexLabel:
		ARG_SELECT(1);
		ARG_SELECT(5);
		ARG_SELECT(7);
		ARG_SELECT(9);
		ARG_SELECT(10);
		ARG_SELECT(11);
		ARG_SELECT(12);
		return t13_flex;
	}
	else if (ARG_ZERO(1) && ARG_ZERO(5) &&
	         ARG_ZERO(7) && ARG_ZERO(11)) {
		/* hflex */
		ARG_SELECT(10);
		return t13_hflex;
	}
	else if (ARG_ZERO(5) && ARG_ZERO(7)) {
		/* hflex1 */
		ARG_SELECT(1);
		ARG_SELECT(9);
		ARG_SELECT(10);
		return t13_hflex1;
	}
	else {
		/* flex1 */
		ARG_SELECT(1);
		ARG_SELECT(5);
		ARG_SELECT(7);
		ARG_SELECT(9);

		if (dyz == (ABS(pendx[0]) <= ABS(pendy[0]))) {
			warnChar(h, iFlex2);
			goto flexLabel;
		}

		ARG_SELECT(dyz ? 10 : 11);
		return t13_flex1;
	}
}

/* Calulate max stack depth for new args */
static int maxArgsDepth(t13Ctx h) {
	int i;
	int max = 0;
	for (i = 0; i < h->args.cnt; i++) {
		if (ARG_SAVE(i)) {
			int cnt = i + (ARG_BLND(i) ? h->nMasters + 1 : 1);
			if (cnt > max) {
				max = cnt;
			}
		}
	}
	return max;
}

/* Fill stack from args accumulator */
static void fillStack(t13Ctx h) {
	int i;

	/* Fill stack elements */
	for (i = 0; i < h->args.cnt; i++) {
		if (ARG_SAVE(i)) {
			if (ARG_BLND(i)) {
				copyBlend(&h->stack.array[h->stack.cnt], h->args.array[i]);
				h->stack.flags[h->stack.cnt++] = S_BLEND;
			}
			else {
				h->stack.array[h->stack.cnt][0] = h->args.array[i][0];
				h->stack.flags[h->stack.cnt++] = 0;
			}
		}
	}
	h->args.cnt = 0;

	/* Determine max stack */
	if (h->nMasters != 1) {
		int min =
		    (h->stack.cnt > h->nMasters) ? h->stack.cnt - h->nMasters : 0;
		for (i = h->stack.cnt - 1; i > min; i--) {
			if (STK_BLEND(i)) {
				h->stack.limit = i + h->nMasters + 1;
				return;
			}
		}
		if (h->stack.limit < h->stack.cnt) {
			h->stack.limit = h->stack.cnt;
		}
	}
	else {
		h->stack.limit = h->stack.cnt;
	}
}

/* Save pending stack plus new args and new op */
static void saveNewOp(t13Ctx h, int newop) {
	fillStack(h);
	saveStack(h);
	T13SAVEOP(newop, h->cstrs);
	h->pend.op = t13_noop;
}

/* Save pending stack args and op */
static void savePendOp(t13Ctx h, int pendop) {
	saveStack(h);
	T13SAVEOP(pendop, h->cstrs);
	h->pend.op = t13_noop;
}

/* Recode char path */
static void t13Path(t13Ctx h) {
	unsigned char map[T13_MAX_STEMS];   /* Stem id map (id -> index) */
	int iSeg;
	Operator *nextop;
	int seqop = 0;      /* Initial op in alternating h/v sequence */

	/* Initialize */
	h->stack.cnt = 0;
	h->stack.limit = 0;
	h->pend.op = t13_noop;
	setBlend(h, h->path.x, 0);
	setBlend(h, h->path.y, 0);

	/* Handle charstring width */
	if (h->path.width[0] == LONG_MIN) {
		if (h->idType != SubrType) {
			badChar(h); /* Charstring has no width! */
		}
	}
	else {
		if (h->width.opt) {
			/* Save dummy width (see comment at head of file) */
			Blend dummy;
			dummy[0] = INT2FIX(SHRT_MAX);
			pushValue(h, dummy);
		}
		else {
			pushValue(h, h->path.width); /* Save glyph width */
		}
	}

	saveStems(h, map);  /* Save stems with cntr and initial hint masks */

	/* Create optimized Type 13 charstring from operators and segments */
	iSeg = 0;
	nextop = h->path.ops.array;
	for (;; ) {
		int newop;
		int fourArgs = 0;       /* Suppress optimizer warning */
		Operator *op = NULL;    /* Suppress optimizer warning */

		if (iSeg < nextop->iSeg) {
			/* Add next segment */
			Segment *seg = &h->path.segs.array[iSeg++];

			if ((seg->flags & SEG_TYPE_MASK) == seg_curve) {
				/* Add curve segment; optimizations:
				 *
				 * dx1 dy1 dx2 dy2 dx3 dy3
				 *  0   1   2   3   4   5  args type
				 * ----------------------- ---- ----
				 *  0                   0  4    vhcurveto
				 *  0                      5    vhcurveto
				 *  0               0      4    vvcurveto
				 *                  0      5    vvcurveto
				 *      0           0      4    hvcurveto
				 *      0                  5    hvcurveto (output ... dy3 dx3)
				 *      0               0  4    hhcurveto
				 *                      0  5    hhcurveto (output dy1 dx1 ...)
				 *                         6    rrcurveto
				 */
				fourArgs = 0;
				loadDeltaArgs(h, seg->iArgs, 6);

				/* Select args common to all curve ops */
				ARG_SELECT(2);
				ARG_SELECT(3);

				if (ARG_ZERO(0)) {
					ARG_SELECT(1);
					if (ARG_ZERO(5)) {
						ARG_SELECT(4);
						fourArgs = 1;
						newop = t13_vhcurveto;
					}
					else if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						fourArgs = 1;
						newop = t13_vvcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						newop = t13_vhcurveto;
					}
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						fourArgs = 1;
						newop = t13_hvcurveto;
					}
					else if (ARG_ZERO(5)) {
						ARG_SELECT(4);
						fourArgs = 1;
						newop = t13_hhcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						swapArgs(h, 4, 5);
						newop = t13_hvcurveto;
					}
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						newop = t13_vvcurveto;
					}
					else if (ARG_ZERO(5)) {
						ARG_SELECT(4);
						swapArgs(h, 0, 1);
						newop = t13_hhcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						newop = t13_rrcurveto;
					}
				}
			}
			else if ((seg->flags & SEG_TYPE_MASK) == seg_line) {
				/* Add line segment; optimizations:
				 *
				 * dx1 dy1
				 *  0   1   args type
				 * -------- ---- ----
				 *  0		1	 vlineto
				 *      0   1    hlineto
				 *          2    rlineto
				 */
				loadDeltaArgs(h, seg->iArgs, 2);
				if (ARG_ZERO(0)) {
					ARG_SELECT(1);
					newop = t13_vlineto;
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					newop = t13_hlineto;
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					newop = t13_rlineto;
				}
			}
			else {
				/* Add move; optimizations:
				 *
				 * dx1 dy1
				 *  0   1   args type
				 * -------- ---- ----
				 *  0		1	 vmoveto
				 *      0   1    hmoveto
				 *          2    rmoveto
				 */
				if (iSeg == h->path.segs.cnt) {
					/* Move at end of path; discard */
					warnChar(h, iMove1);
					continue;
				}
				else if (((seg + 1)->flags & SEG_TYPE_MASK) == seg_move) {
					/* Move sequence; collapse */
					warnChar(h, iMove2);
					continue;
				}

				loadDeltaArgs(h, seg->iArgs, 2);
				if (ARG_ZERO(0)) {
					ARG_SELECT(1);
					newop = t13_vmoveto;
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					newop = t13_hmoveto;
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					newop = t13_rmoveto;
				}
			}
		}
		else {
			/* Add next operator */
			op = nextop++;
			switch (op->type) {
				case t2_flex:
				{
					Segment *seg = &h->path.segs.array[iSeg];
					newop = doFlex(h, seg->iArgs, op->iArgs);
					iSeg += 2; /* Skip flex curves */
				}
				break;

				case t13_hintmask:
					newop = t13_hintmask;
					break;

				case tx_endchar:
					loadArgs(h, op->iArgs, op->nArgs);
					ARG_SELECT(0);
					ARG_SELECT(1);
					ARG_SELECT(2);
					ARG_SELECT(3);
					newop = t13_endchar;
					break;

				default:
				{
					/* 2-byte args ops */
					int j;
					if (op->nArgs > 13) {
						badChar(h);
					}
					loadArgs(h, op->iArgs, op->nArgs);
					for (j = 0; j < op->nArgs; j++) {
						ARG_SELECT(j);
					}
					newop = t13_escape;
				}
				break;
			}
		}

		/* Save pending op if new op's args don't fit on stack */
		if (maxArgsDepth(h) + h->stack.limit > T13_MAX_OP_STACK) {
			switch (h->pend.op) {
				case t13_vlineto:
				case t13_hlineto:
				case t13_vhcurveto:
				case t13_hvcurveto:
					savePendOp(h, seqop);
					break;

				default:
					savePendOp(h, h->pend.op);
					break;
			}
		}

		/* Check pending operator */
		switch (h->pend.op) {
			case t13_vlineto:
				if (newop == t13_hlineto) {
					goto pending;
				}
				savePendOp(h, seqop);
				break;

			case t13_hlineto:
				if (newop == t13_vlineto) {
					goto pending;
				}
				savePendOp(h, seqop);
				break;

			case t13_rlineto:
				if (newop == t13_rlineto) {
					goto pending;
				}
				else if (newop == t13_rrcurveto) {
					saveNewOp(h, t13_rlinecurve);
					continue;
				}
				savePendOp(h, t13_rlineto);
				break;

			case t13_vhcurveto:
				if (newop == t13_hvcurveto) {
					if (fourArgs) {
						goto pending;
					}
					saveNewOp(h, seqop);
					continue;
				}
				savePendOp(h, seqop);
				break;

			case t13_vvcurveto:
				if (newop == t13_vvcurveto && fourArgs) {
					goto pending;
				}
				savePendOp(h, t13_vvcurveto);
				break;

			case t13_hvcurveto:
				if (newop == t13_vhcurveto) {
					if (fourArgs) {
						goto pending;
					}
					saveNewOp(h, seqop);
					continue;
				}
				savePendOp(h, seqop);
				break;

			case t13_hhcurveto:
				if (newop == t13_hhcurveto && fourArgs) {
					goto pending;
				}
				savePendOp(h, t13_hhcurveto);
				break;

			case t13_rrcurveto:
				if (newop == t13_rrcurveto) {
					goto pending;
				}
				else if (newop == t13_rlineto) {
					saveNewOp(h, t13_rcurveline);
					continue;
				}
				savePendOp(h, t13_rrcurveto);
				break;
		}
		h->pend.op = t13_noop;

		/* See if new op should become pending */
		switch (newop) {
			case t13_vmoveto:
			case t13_hmoveto:
			case t13_rmoveto:
				saveNewOp(h, newop);
				continue;

			case t13_vlineto:
			case t13_hlineto:
				seqop = newop;
				break;

			case t13_vhcurveto:
				if (!fourArgs) {
					saveNewOp(h, t13_vhcurveto);
					continue;
				}
				seqop = t13_vhcurveto;
				break;

			case t13_hvcurveto:
				if (!fourArgs) {
					saveNewOp(h, t13_hvcurveto);
					continue;
				}
				seqop = t13_hvcurveto;
				break;

			case t13_hintmask:
				saveHintMaskOp(h, map, t13_hintmask,
				               op->nArgs, &h->path.idmask.array[op->iArgs], 1);
				continue;

			case t13_endchar:
				fillStack(h);
				saveStack(h);
				T13SAVEOP(t13_endchar, h->cstrs);

#if TC_SUBR_SUPPORT
				if (h->g->flags & TC_SUBRIZE) {
					/* Add unique separator to charstring for subroutinizer */
					unsigned char *cstr = (unsigned char *)da_EXTEND(h->cstrs, 4);
					cstr[0] = (unsigned char)t13_separator;
					cstr[1] = (unsigned char)h->unique >> 16;
					cstr[2] = (unsigned char)h->unique >> 8;
					cstr[3] = (unsigned char)h->unique++;
				}
#endif /* TC_SUBR_SUPPORT */

				return;

			case t13_escape:
				saveNewOp(h, op->type);
				continue;

			case t13_callsubr:
			case t13_hflex:
			case t13_flex:
			case t13_hflex1:
			case t13_flex1:
				saveNewOp(h, newop);
				continue;
		}

pending:
		fillStack(h);
		h->pend.op = newop;
	}
}

/* Recode type 1 charstring */
static void cstrRecode(tcCtx g, unsigned length, unsigned char *cstr,
                       unsigned nSubrs, Charstring *subrs) {
	t13Ctx h = g->ctx.t13;

	/* Initialize */
	h->stack.cnt = 0;
	h->intDiv = 0;

	h->hint.initial = 1;
	h->hint.subs = 0;
	h->hint.seen = 0;
	h->hint.cntr = 0;

	h->stem.list.cnt = 0;
	h->cntr.insert = 0;
	h->cntr.list.cnt = 0;
	h->cntr.idmask.cnt = 0;
	CLEAR_ID_MASK(h->stem.initmask);
	CLEAR_ID_MASK(h->stem.subsmask);
	CLEAR_ID_MASK(h->stem.lastmask);

	h->pend.op = t13_noop;
	h->pend.flex = 0;
	h->pend.move = 1;

	h->path.width[0] = LONG_MIN;
	h->path.segs.cnt = 0;
	h->path.ops.cnt = 0;
	h->path.args.cnt = 0;
	h->path.idmask.cnt = 0;

	/* Parse and recode charstring */
	(void)t1parse(h, length, cstr, nSubrs, subrs);
	t13Path(h);

	if (g->flags & TC_SMALLMEMORY) {
		/* Save charstring in tmp file */
		g->cb.tmpWriteN(g->cb.ctx, h->cstrs.cnt, (char *)h->cstrs.array);
	}

	if (h->warn.total > 0) {
		showWarn(h);
	}
}

/* Remove double layer of encryption (CJK fonts) in place */
static void t13Decrypt(unsigned length, unsigned char *cstr) {
	unsigned i;
	unsigned short r1 = 4330;   /* Single decryption key */
	unsigned short r2 = 54261;  /* Double decryption key */

	for (i = 0; i < length; i++) {
		unsigned char single = cstr[i] ^ (r2 >> 8);
		r2 = (cstr[i] + r2) * 16477 + 21483;
		cstr[i] = single ^ (r1 >> 8);
		r1 = (single + r1) * 52845 + 22719;
	}
}

/* Recode type 1 charstring */
static void t13AddChar(tcCtx g, unsigned length, char *cstr, unsigned id,
                       unsigned nSubrs, Charstring *subrs, int fd) {
	t13Ctx h = g->ctx.t13;

	/* Save charstring index */
	if (g->flags & TC_SMALLMEMORY) {
		long prev =
		    (h->chars.cnt > 0) ? h->chars.array[h->chars.cnt - 1].icstr : 0;
		da_NEXT(h->chars)->icstr = prev + h->cstrs.cnt;
		h->cstrs.cnt = 0;
	}
	else {
		da_NEXT(h->chars)->icstr = h->cstrs.cnt;
	}

#if TC_DEBUG
	if (h->debug.input || h->debug.output) {
		if (h->font->flags & FONT_CID) {
			printf("--- glyph[%hu]\n", id);
		}
		else {
			printf("--- glyph[%s]\n", sindexGetString(g, (SID)id));
		}
	}
	if (h->debug.input) {
		printf("=== orig[%d]: ", length);
		(void)csDump(length, (unsigned char *)cstr, g->nMasters, 1);
	}
#endif /* TC_DEBUG */

	/* Initialize context */
	h->idType = (h->font->flags & FONT_CID) ? CIDType : SIDType;
	h->id = id;
	h->width.curr = da_INDEXS(h->width.fd, fd);

	cstrRecode(g, length, (unsigned char *)cstr, nSubrs, subrs);

#if TC_DEBUG
	if (h->debug.output) {
		long icstr = h->chars.array[h->chars.cnt - 1].icstr;
		unsigned length = h->cstrs.cnt - icstr;
		printf("==== recode[%hd]: ", length);
		t13Dump(length, (unsigned char *)&h->cstrs.array[icstr], g->nMasters, 0);
	}
#endif /* TC_DEBUG */

#if TC_STATISTICS
	h->font->flatSize += h->cstrs.cnt - h->chars.array[h->chars.cnt - 1].icstr;
#endif /* TC_STATISTICS */
}

/* Get recoded char. Return charstring pointer and length after applying width
   optimizations. If TC_SMALLMEMORY just return length since charstrings are
   saved in temporary file */
static char *t13GetChar(tcCtx g, unsigned iChar, int fd, unsigned *length) {
	t13Ctx h = g->ctx.t13;
	char t[5];
	Char *this = &h->chars.array[iChar];
	long iSrc = this->icstr;
	long iEnd = ((long)(iChar + 1) == h->chars.cnt) ?
	    ((g->flags & TC_SMALLMEMORY) ? h->cstrs.cnt + iSrc : h->cstrs.cnt) :
	    (this + 1)->icstr;

	if (!h->width.opt) {
		/* No width optimization (return unmodified charstring) */
		*length = iEnd - iSrc;
		return (g->flags & TC_SMALLMEMORY) ? NULL : &h->cstrs.array[iSrc];
	}

	if (g->flags & TC_SMALLMEMORY) {
		*length = iEnd - iSrc - DUMMY_WIDTH_SIZE +
		    ((this->width == h->width.fd.array[fd].dflt) ? 0 :
		     t13EncInteger(this->width - h->width.fd.array[fd].nominal, t));
		return NULL;
	}
	else {
		int bytes;
		char *src = &h->cstrs.array[iSrc] + DUMMY_WIDTH_SIZE;

		if (this->width != h->width.fd.array[fd].dflt) {
			/* Compute and save optimized width */
			bytes =
			    t13EncInteger(this->width - h->width.fd.array[fd].nominal, t);
			src -= bytes;
			memmove(src, t, bytes);
		}
		else {
			bytes = 0;
		}

		*length = iEnd - iSrc - DUMMY_WIDTH_SIZE + bytes;
		return src;
	}
}

/* Replenish input buffer using tmp refill function */
static char fillbuf(tcCtx g) {
	t13Ctx h = g->ctx.t13;
	h->tmpFile.next = g->cb.tmpRefill(g->cb.ctx, &h->tmpFile.left);
	if (h->tmpFile.left-- == 0) {
		parseFatal(g, "premature end of tmp file");
	}
	return *h->tmpFile.next++;
}

/* Write recoded char in TC_SMALLMEMORY mode */
static void t13WriteChar(tcCtx g, Font *font, unsigned iChar) {
	t13Ctx h = g->ctx.t13;
	unsigned i;
	int fd = font->fdIndex[iChar];
	char *buf = h->cstrs.array;
	Char *this = &h->chars.array[iChar];
	unsigned length = ((long)(iChar + 1) == h->chars.cnt) ?
	    h->cstrs.cnt : ((this + 1)->icstr - this->icstr);

	/* Read charstring */
	for (i = 0; i < length; i++) {
		buf[i] = (h->tmpFile.left-- == 0) ? fillbuf(g) : *h->tmpFile.next++;
	}

	if (!h->width.opt) {
		/* No width optimization (write unmodified charstring) */
		OUTN(length, buf);
	}
	else {
		if (this->width != h->width.fd.array[fd].dflt) {
			/* Compute and save optimized width */
			char t[5];
			int bytes =
			    t13EncInteger(this->width - h->width.fd.array[fd].nominal, t);
			OUTN(bytes, t);
		}
		OUTN(length - DUMMY_WIDTH_SIZE, &buf[DUMMY_WIDTH_SIZE]);
	}
}

/* Read RunInt from CIDFont Private dict. This is set up like lenIV. No /RunInt
   entry or a value of "/CCRun" means single (or no) encryption (see lenIV
   value). An value of "/eCCRun" means double encryption, as found in Morisawa
   fonts. Returns 1 on failure else 0 */
int t13CheckConv(tcCtx g, psCtx ps, csDecrypt *decrypt) {
	t13Ctx h = g->ctx.t13;
	psToken *token = psGetToken(ps);

	if (token->type != PS_LITERAL) {
		return 1;
	}

	if (psMatchValue(ps, token, "/eCCRun")) {
		/* Set decrypter for doubly-encrypted charstrings */
		*decrypt = t13Decrypt;

		if (!h->eCCRunSeen) {
			static csConvProcs procs = {
				t13NewFont,
				t13EndFont,
				t13AddChar,
				t13GetChar,
				t13WriteChar,
			};

			if (!(g->flags & TC_EMBED)) {
				/* Set Type 13 conversion procs */
				csSetConvProcs(g, &procs);
				h->eCCRunSeen = 1;
			}
		}
	}
	return 0;
}

/* Add authentication string as last entry in string INDEX. Note hard-coded
   string definition since I don't want any public definitions of this
   mechanism. Return 1 if reserved space for auth. string else 0 */
int t13CheckAuth(tcCtx g, Font *font) {
	t13Ctx h = g->ctx.t13;
	if (h->eCCRunSeen ||
	    (g->flags & TC_ADDAUTHAREA && font->flags & FONT_CID)) {
		/* Seen Type 13 font or authentication area requested for non-Type 13
		   CID-keyed font; initialize authentication string with random
		   obscure bytes */
		static unsigned char auth[44] = {
			0x8f, 0x20, 0x10, 0x11, 0x3c, 0x05, 0xf3, 0x7f, 0xfe, 0x56,
			0x6d, 0x90, 0x12, 0x21, 0x3f, 0x80, 0x8f, 0x20, 0x10, 0x22,
			0x80, 0x00, 0x35, 0x80, 0x8f, 0x20, 0x01, 0x11, 0x3c, 0x05,
			0xf3, 0x7f, 0xfe, 0x56, 0x67, 0x90, 0x12, 0x21, 0x5a, 0x65,
			0xa9, 0xf0, 0x00, 0x50,
		};
		(void)sindexGetId(g, sizeof(auth), (char *)auth);

		h->eCCRunSeen = 0;
		return 1;
	}
	else {
		return 0;
	}
}

#if TC_DEBUG
/* -------------------------------- Debug ---------------------------------- */
static void dbcstrs(t13Ctx h, unsigned iChar) {
	long iStart = h->chars.array[iChar].icstr;
	unsigned length = (((long)(iChar + 1) == h->chars.cnt) ?
	                   h->cstrs.cnt : h->chars.array[iChar + 1].icstr) - iStart;
	csDump(length, (unsigned char *)&h->cstrs.array[iStart], h->nMasters, 0);
}

static void dbvalue(t13Ctx h, int n, Fixed *value) {
	int i;
	if (n > 1) {
		printf("{");
	}
	for (i = 0; i < n; i++) {
		printf("%g%s", value[i] / 65536.0, (i == n - 1) ? "" : ",");
	}
	if (n > 1) {
		printf("}");
	}
}

static void dbstack(t13Ctx h) {
	if (h->nMasters == 1) {
		printf("--- stack[index]=value\n");
	}
	else {
		printf("--- stack[index]={blend,value}\n");
	}
	if (h->stack.cnt == 0) {
		printf("empty\n");
	}
	else {
		int i;
		for (i = 0; i < h->stack.cnt; i++) {
			printf("[%d]=", i);
			dbvalue(h, STK_BLEND(i) ? h->nMasters : 1, h->stack.array[i]);
			printf(" ");
		}
		printf("\n");
	}
}

static void dbstemid(t13Ctx h, int idsize, char *idmask) {
	int i;
	for (i = 0; i < idsize; i++) {
		printf("%02x%s", idmask[i] & 0xff, (i == idsize - 1) ? "" : ",");
	}
}

static void dbstems(t13Ctx h) {
	int i;
	printf("--- stems[index]={type,id,edge,delta}\n");
	if (h->stem.list.cnt == 0) {
		printf("empty\n");
	}
	else {
		int idsize = (h->stem.list.cnt + 7) / 8;
		for (i = 0; i < h->stem.list.cnt; i++) {
			Stem *stem = &h->stem.list.array[i];
			printf("[%d]={%hx,%hd,", i, stem->type, stem->id);
			dbvalue(h, h->nMasters, stem->edge);
			printf(",");
			dbvalue(h, h->nMasters, stem->delta);
			printf("} ");
		}
		printf("\n");
		if (h->stem.list.cnt > 0) {
			printf("--- h->stem.initmask: ");
			dbstemid(h, idsize, h->stem.initmask);
			printf("\n");
			printf("--- h->stem.subsmask: ");
			dbstemid(h, idsize, h->stem.subsmask);
			printf("\n");
		}
	}
	if (h->cntr.list.cnt) {
		printf("--- cntrs[index]=value\n");
		for (i = h->cntr.list.cnt - 1; i >= 0; i--) {
			printf("[%ld]=", h->cntr.list.cnt - i - 1);
			dbvalue(h, h->nMasters, h->cntr.list.array[i]);
			printf(" ");
		}
		printf("\n");
	}
}

static void dbwidths(t13Ctx h, int fd) {
	if (h->nMasters != 1) {
		return;
	}
	printf("--- widths[index]={width,count}\n");
	if (h->width.fd.array[fd].total.cnt == 0) {
		printf("empty\n");
	}
	else {
		int i;
		for (i = 0; i < h->width.fd.array[fd].total.cnt; i++) {
			Width *width = &h->width.fd.array[fd].total.array[i];
			printf("[%d]={%hd,%hu} ", i, width->width, width->count);
		}
		printf("\n");
	}
}

static void dbcoordop(t13Ctx h, char *op, int nCoords, unsigned index) {
	while (nCoords-- > 0) {
		dbvalue(h, h->nMasters, &h->path.args.array[index]);
		printf(" ");
		index += h->nMasters;
	}
	printf("%s/", op);
}

static char *escopname(int escop) {
	switch (escop) {
		case t13_dotsection:
			return "dotsection";

		case t13_flex:
			return "flex";

		case t13_hflex:
			return "hflex";

		case t13_exch:
			return "exch";

		case t13_sqrt:
			return "sqrt";

		case t13_roll:
			return "roll";

		case t13_cntron:
			return "cntron";

		case t13_hflex1:
			return "hflex1";

		case t13_mul:
			return "mul";

		case t13_dup:
			return "dup";

		case t13_index:
			return "index";

		case t13_flex1:
			return "flex1";

		case t13_random:
			return "random";

		case t13_ifelse:
			return "ifelse";

		case t13_get:
			return "get";

		case t13_put:
			return "put";

		case t13_drop:
			return "drop";

		case t13_eq:
			return "eq";

		case t13_neg:
			return "neg";

		case t13_load:
			return "load";

		case t13_div:
			return "div";

		case t13_sub:
			return "sub";

		case t13_add:
			return "add";

		case t13_abs:
			return "abs";

		case t13_store:
			return "store";

		case t13_not:
			return "not";

		case t13_or:
			return "or";

		case t13_and:
			return "and";

		case t13_reservedESC255:
			return "reservedESC255";

		default:
			return "?";
	}
}

static void dbpath(t13Ctx h) {
	int iSeg;
	Operator *op;

	printf("--- path\n");
	printf("x    =");
	dbvalue(h, h->nMasters, h->path.x);
	printf("\n");
	printf("y    =");
	dbvalue(h, h->nMasters, h->path.y);
	printf("\n");
	printf("lsb.x=");
	dbvalue(h, h->nMasters, h->path.lsb.x);
	printf("\n");
	printf("lsb.y=");
	dbvalue(h, h->nMasters, h->path.lsb.y);
	printf("\n");
	printf("width=");
	dbvalue(h, h->nMasters, h->path.width);
	printf("\n");
	printf("--- ops\n");
	if (h->path.segs.cnt == 0) {
		printf("empty\n");
		return;
	}

	iSeg = 0;
	op = h->path.ops.array;
	for (;; ) {
		if (iSeg < op->iSeg) {
			/* Dump next segment */
			Segment *seg = &h->path.segs.array[iSeg++];

			if ((seg->flags & SEG_TYPE_MASK) == seg_curve) {
				dbcoordop(h, "curve", 6, seg->iArgs);
			}
			else if ((seg->flags & SEG_TYPE_MASK) == seg_line) {
				dbcoordop(h, "line", 2, seg->iArgs);
			}
			else {
				dbcoordop(h, "move", 2, seg->iArgs);
			}
		}
		else {
			/* Dump next operator */
			switch (op->type) {
				case tx_endchar:
					dbcoordop(h, "end", op->nArgs, op->iArgs);
					printf("\n");
					return;

				case t13_hintmask:
					printf("hint[");
					dbstemid(h, op->nArgs, &h->path.idmask.array[op->iArgs]);
					printf("]/");
					break;

				default:
					if (op->type >> 8 != t13_escape) {
						printf("? ");
					}
					else {
						dbcoordop(h, escopname(op->type), op->nArgs, op->iArgs);
					}
			}
			op++;
		}
	}
	printf("\n");
}

static void dbargs(t13Ctx h) {
	printf("--- args[index]={zero/blnd/save,value}\n");
	if (h->args.cnt == 0) {
		printf("empty\n");
	}
	else {
		int i;
		for (i = 0; i < h->args.cnt; i++) {
			printf("[%d]={%d/%d/%d,", i,
			       ARG_ZERO(i) != 0, ARG_BLND(i) != 0, ARG_SAVE(i) != 0);
			dbvalue(h, h->nMasters, h->args.array[i]);
			printf("} ");
		}
		printf("\n");
	}
}

/* Dump charstring for debugging purposes. If called with -ve length function
   will dump until endchar and then return the length of the charstring */
static unsigned t13Dump(long length, unsigned char *cstr, int nMasters, int t1) {
	static char *opname[] = {
		/*   0 */ "reserved0",
		/* 216 */ "hvcurveto",
		/* 217 */ "rrcurveto",
		/* 218 */ "vstem",
		/* 219 */ "rmoveto",
		/* 220 */ "?",          /* Number */
		/* 221 */ "rlineto",
		/* 222 */ "cntrmask",
		/* 223 */ "shftshort",
		/* 224 */ "?",          /* Number */
		/* 225 */ "hstem",
		/* 226 */ "callsubr",
		/* 227 */ "escape",
		/* 228 */ "?",          /* Number */
		/* 229 */ "hintmask",
		/* 230 */ "vmoveto",
		/* 231 */ "hlineto",
		/* 232 */ "?",          /* Number */
		/* 233 */ "shortint",
		/* 234 */ "hhcurveto",
		/* 235 */ "?",          /* Number */
		/* 236 */ "reserved226",
		/* 237 */ "reserved227",
		/* 238 */ "vlineto",
		/* 239 */ "?",          /* Number */
		/* 240 */ "return",
		/* 241 */ "?",          /* Number */
		/* 242 */ "endchar",
		/* 243 */ "blend",
		/* 244 */ "reserved244",
		/* 245 */ "hstemhm",
		/* 246 */ "?",          /* Number */
		/* 247 */ "hmoveto",
		/* 248 */ "vstemhm",
		/* 249 */ "rcurveline",
		/* 250 */ "rlinecurve",
		/* 251 */ "vvcurveto",
		/* 252 */ "callgsubr",
		/* 253 */ "vhcurveto",
		/* 254 */ "?",          /* Number */
		/* 255 */ "reserved255",
	};
	int single = 0; /* Suppress optimizer warning */
	int stems = 0;
	int args = 0;
	int i = 0;

	while (i < length || length < 0) {
		int op = cstr[i];
		switch (op) {
			case t13_endchar:
				printf("%s ", opname[op - 215]);
				if (length < 0) {
					return i + 1;
				}
				i++;

			case t13_reserved0:
				printf("%s ", opname[op]);
				args = 0;
				i++;

			case t13_reserved244:
			case t13_reserved236:
			case t13_reserved237:
			case t13_reserved255:
			case t13_vmoveto:
			case t13_rlineto:
			case t13_hlineto:
			case t13_vlineto:
			case t13_rrcurveto:
			case t13_callsubr:
			case t13_return:
			case t13_rmoveto:
			case t13_hmoveto:
			case t13_rcurveline:
			case t13_rlinecurve:
			case t13_vvcurveto:
			case t13_hhcurveto:
			case t13_callgsubr:
			case t13_vhcurveto:
			case t13_hvcurveto:
				printf("%s ", opname[op - 215]);
				args = 0;
				i++;

			case t13_hstem:
			case t13_vstem:
			case t13_hstemhm:
			case t13_vstemhm:
				printf("%s ", opname[op - 215]);
				stems += args / 2;
				args = 0;
				i++;

			case t13_hintmask:
			case t13_cntrmask:
			{
				int bytes;
				if (args > 0) {
					stems += args / 2;
				}
				bytes = (stems + 7) / 8;
				printf("%s[", opname[op - 215]);
				i++;
				while (bytes--) {
					printf("%02x", cstr[i++]);
				}
				printf("] ");
				args = 0;
			}

			case t13_escape:
				/* Process escaped operator */
				printf("%s ", escopname(t13_ESC(cstr[i + 1])));
				args = 0;
				i += 2;

			case t13_blend:
				printf("%s ", opname[op - 215]);
				args -= single * (nMasters - 1) + 1;
				i++;

			case t13_shortint:
				printf("%d ", cstr[i + 1] << 8 | cstr[i + 2]);
				args++;
				i += 3;

			case t13_shftshort:
			{
				Fixed f = ((long)cstr[i + 1] << 24 | (long)cstr[i + 2] << 16) >> 1;
				if ((signed char)cstr[i + 1] < 0) {
					f |= 0x80000000;
				}
				printf("%g ", FIX2DBL(f));
				args++;
				i += 3;
			}

			case 241:
				printf("%d ", 363 - cstr[i + 1]);
				args++;
				i += 2;

			case 239:
				printf("%d ", 619 - cstr[i + 1]);
				args++;
				i += 2;

			case 246:
				printf("%d ", cstr[i + 1] + 620);
				args++;
				i += 2;

			case 235:
				printf("%d ", 1131 - cstr[i + 1]);
				args++;
				i += 2;

			case 232:
				printf("%d ", cstr[i + 1] - 363);
				i += 2;
				args++;

			case 228:
				printf("%d ", cstr[i + 1] - 619);
				i += 2;
				args++;

			case 224:
				printf("%d ", cstr[i + 1] - 875);
				i += 2;
				args++;

			case 220:
				printf("%d ", cstr[i + 1] - 1131);
				i += 2;
				args++;

			case 254:
			{
				/* 5 byte number */
				long value = (long)cstr[i + 1] << 24 | (long)cstr[i + 2] << 16 |
				    (long)cstr[i + 3] << 8 | (long)cstr[i + 4];
				if (t1) {
					printf("%ld ", value);
				}
				else {
					printf("%.4g ", value / 65536.0);
				}
				args++;
				i += 5;
			}

			default:
				/* 1 byte number */
				single = 108 - cstr[i];
				printf("%d ", single);
				args++;
				i++;
		}
	}
	printf("\n");
	return length;
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dbcstrs, dbstack, dbstems, dbwidths, dbpath, dbargs);
}

#endif /* TC_DEBUG */