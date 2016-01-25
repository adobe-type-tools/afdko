/* @(#)CM_VerSion recode.c atm08 1.2 16245.eco sum= 07086 atm08.002 */
/* @(#)CM_VerSion recode.c atm07 1.2 16164.eco sum= 47624 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/


#include "recode.h"
#include "sindex.h"
#include "parse.h"
#include "subr.h"

#include "txops.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */
#include "dynarr.h"

/* In order to avoid matching repeats across charstring boundaries it is
   necessary to add a unique separator between charstrings. This is achieved by
   inserting the t2_separator operator after each endchar. The 1-byte operator
   value is followed by a 24-bit number that is guaranteed to be different for
   each charstring thereby removing any possibility of a match spanning an
   endchar operator. These operators are inserted by the recode module and
   removed by the subroutinizer */
#define t2_separator    t2_reserved9
#define t2_noop         tx_reserved0    /* Noop flag */

typedef struct {                /* Character record */
	long icstr;                 /* Index of charstring start */
	short width;                /* Original width */
} Char;

typedef Fixed Blend[TX_MAX_MASTERS];    /* Blendable array of values */

/* Save Type 2 operator without args */
#define T2SAVEOP(op, da) do { if ((op) & 0xff00) { *dnaNEXT(da) = tx_escape; } \
		                      *dnaNEXT(da) = (unsigned char)(op); } \
	while (0)

/* Calulate number size in bytes */
#define NUMSIZE(v) (-107 <= (v) && (v) <= 107 ? 1 : (-1131 <= (v) && (v) <= 1131 ? 2 : 3))

/* Charstring buffer */
typedef dnaDCL (char, CSTR);

/* --- Stack access macros --- */

/* Check for under/overflow before POPs, PUSHes, or INDEXing */
#define CHKUFLOW(c) do { if (h->stack.cnt < (c)) { badChar(h); } \
} \
	while (0)
#define CHKOFLOW(c) do { if (h->stack.cnt + (c) > T2_MAX_OP_STACK) { badChar(h); } \
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

#define MAX_MASK_BYTES  ((T2_MAX_STEMS + 7) / 8)
typedef char HintMask[MAX_MASK_BYTES];

#define CLEAR_ID_MASK(a)    memset(a, 0, MAX_MASK_BYTES)
#define SET_ID_BIT(a, i)     ((a)[(i) / 8] |= 1 << ((i) % 8))
#define TEST_ID_BIT(a, i)    ((a)[(i) / 8] & 1 << ((i) % 8))

#define CLEAR_HINT_MASK(a)  memset(a, 0, h->hint.size)
#define SET_HINT_BIT(a, i)   ((a)[(i) / 8] |= 1 << (7 - (i) % 8))
#define SAVE_HINT_MASK(a)   \
	memcpy(dnaEXTEND(h->cstrs, h->hint.size), a, h->hint.size)

typedef struct {            /* Stem record */
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
   32767. The recodeGetChar() function reads the non-optimized width from the
   chars array, performs the optimization, and then writes the result to the
   charstring so that the remaining charstring bytes immediately follow the
   optimized width.

   Multiple master and fractional widths fonts aren't optimized because the

   implementation would be more complicated and these kinds of fonts are rare.
 */

#define DUMMY_WIDTH_SIZE 3

typedef struct {        /* Width accumulator */
	short width;
	unsigned long count;
} Width;

typedef struct {        /* Width data per-FD */
	short dflt;         /* Default width */
	short nominal;      /* Nominal width */
	dnaDCL(Width, total); /* Unique width totals */
} WidthFD;

/* --- Path --- */

/* Segment types */
enum {
	seg_move,   /* x1 y1 moveto (2 args) */
	seg_line,   /* x1 y1 lineto (2 args) */
	seg_curve   /* x1 y1 x2 y2 x3 y3 curveto (6 args) */
};

typedef struct { /* Segment */
	unsigned short iArgs;   /* Coordinate argument index */
	unsigned short flags;   /* Type and status flags */
#define SEG_TYPE_MASK   0x0003
} Segment;

typedef struct { /* Operator */
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

enum {                  /* See initializer in recodeNew() for message */
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
	iMisc2,

	WARN_COUNT          /* Warning count */
};

/* Warning initializer */
#define IWARN(i, m)  h->warn.message[i] = (m); h->warn.count[i] = 0

/* --- Transitional designs --- */

typedef struct {        /* Transitional char record */
	SID sid;
	Charstring cstr;
} TransChar;

typedef struct {        /* Transitional font record */
	struct {             /* Subrs */
		short cnt;
		short *array;   /* [cnt]; value= old subr #, index= new subr # */
	} subrs;
	struct {             /* Chars */
		short cnt;
		TransChar *array; /* [cnt] */
	} chars;
} TransFont;

/* --- NDV/CDV subrs --- */

typedef struct {         /* Conversion subrs record */
	char *FontName;
	Charstring *NDV;
	Charstring *CDV;
	TransFont *trans;
	short subs;
} ConvRecord;

/* --- Module context --- */

typedef enum {                  /* Char id type */
	SIDType,
	CIDType,
	SubrType
} IdType;

#if TC_EURO_SUPPORT

/* --------------------------- Euro Glyph Support -------------------------- */

#include <math.h>

#include "cffread.h"

/* Degrees to readians multiplying constant */
#define DEG_2_RAD   (3.141592653589793 / 180)

/* Round double and convert to Fixed */
#define RND2FIX(d)  ((Fixed)((long)((double)(d) + 0.5) * 65536.0))

/* --- template glyph data for auto glyph addition --- */
typedef struct {
	int seen;
	int alreadyWarnedonClampedUDV; /*  so we can only warn once about clamping the current font's UDV. */
	Blend width;
	struct {
		Blend left;
		Blend bottom;
		Blend right;
		Blend top;
	} bbox;
	Blend hstem;        /* First hstem width (0 if no hstems) */
	Blend vstem;        /* First vstem width (0 if no vstems) */
} TemplateGlyphData;

typedef struct {
	double c;
	double d;
	double tx;
	double ty;
	double figure_height_offset;
	double noslant_glyph_offset;
} FontMatrix;

typedef struct {
	char *data;
	long length;
	unsigned scaling_gid; /* the glyph in the fill-in source font,
	                         whose top and bottom is used for scaling the template font to match
	                         the top/bottom of the target font template glyph */
	double scaling_gid_bottom;
	double scaling_gid_top;
	FontMatrix matrix[TX_MAX_MASTERS];
	Fixed UDV[2][TX_MAX_MASTERS];
} FillinFontData;


#define kNumFillinFonts 2
#define kNUM_Fill_IN_MM_AXES 2

#endif /* if TC_EURO_SUPPORT */

struct recodeCtx_ {
	unsigned short flags;
	#define RECODE_SUPPRESS_HINT_WARNINGS (1 << 0)

	Font *font;                 /* Current font */
	IdType idType;              /* Char id type */
	unsigned id;                /* Current char id (SID/CID/Subr) */
	CSTR cstrs;                 /* Recoded and concatenated charstrings */
	dnaDCL(Char, chars);        /* Character records */
	int intDiv;                 /* Flags div operator had integer dividend */
	struct {                    /* --- Type 1 input and Type 2 output stack */
		int cnt;                /* Element count */
		int limit;              /* Output stack depth limit */
		int max;                /* Max output stack (allowing for subr calls)*/
		Blend array[T2_MAX_OP_STACK];   /* Values */
		char flags[T2_MAX_OP_STACK];    /* Flags */
#define S_BLEND     (1 << 0)      /* Blend element */
	} stack;
	struct {                    /* --- Stem hints */
		HintMask initmask;      /* Initial stem id mask */
		HintMask subsmask;      /* Hint subsitution id mask */
		HintMask lastmask;      /* Last hint substitution mask */
		dnaDCL(Stem, list);     /* Stem list */
	} stem;
	struct {                    /* --- Othersubrs 12/13 counters */
		int insert;             /* Flags extra compatibility stem inserted */
		dnaDCL(Blend, list);    /* Original charstring list */
		dnaDCL(HintMask, idmask);   /* Id masks for counter groups */
	} cntr;
	struct {                    /* --- Hint substitution */
		int initial;            /* Flags initial h/vstem list */
		int subs;               /* Flags within hint subs (1 3 callother) */
		int seen;               /* Cstr has hint subs (seen 1 3 callother) */
		int cntr;               /* Charstring has counter hints */
		int size;               /* Hint mask size (bytes) */
	} hint;
	struct {                    /* --- Pending operators */
		int op;                 /* Pending operator */
		int flex;               /* Flags flex pending */
		int move;               /* Pending move */
		int trans;              /* Pending transitional subrs conversion */
	} pend;
	struct {                    /* --- Path accumulator */
		Blend x;                /* Current x coordinate */
		Blend y;                /* Current y coordinate */
		struct {
			Blend x;            /* Left side-bearing x */
			Blend y;            /* Left side-bearing y */
		}
		lsb;
		Blend width;            /* Glyph width */
		dnaDCL(Segment, segs);
		dnaDCL(Operator, ops);
		dnaDCL(Fixed, args);    /* Operator and segment args */
		dnaDCL(char, idmask);   /* Stem id masks */
	} path;
	struct {                    /* --- Seac operator data */
		Blend bsb;
		Blend adx;
		Blend ady;
		int phase;              /* 0=no seac, 1=base,
		                           2=pre-move accent, 3=post-move accent */
		int hint;               /* 0=off, 1=init, 2=stem, 3=subs, 4=remove */
		int iHintmask;          /* Initial accent hintmask index */
	} seac;
	struct {                    /* --- Widths */
		int opt;                /* Flags width optimization */
		dnaDCL(WidthFD, fd);    /* FD width data */
		WidthFD *curr;          /* Current width */
	} width;
	int nMasters;               /* Local copy of g->nMasters */
	TransFont *trans;           /* Transitional font record */
	struct {                    /* --- Output argument accumulator */
		int cnt;
		Blend array[13];
		char flag[13];
#define A_ZERO  (1 << 0)          /* Value(s) zero */
#define A_BLND  (1 << 1)          /* Multiple-valued arg */
#define A_SAVE  (1 << 2)          /* Save this arg */
	} args;
	unsigned long unique;       /* Unique subroutinizer separator value */
	struct {                    /* --- Temporary file input */
		char *next;             /* Next byte available in input buffer */
		long left;              /* Number of bytes available in input buffer */
	} tmpFile;
	struct {                    /* --- Message handling */
		unsigned total;         /* Total warnings for char */
		unsigned count[WARN_COUNT]; /* Totals for different warnings */
		char *message[WARN_COUNT];  /* Warning messages */
	} warn;
#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
	double italic_angle;
	unsigned isFixedPitch;
	unsigned noslant; /* whether the current synthetic glyphs hould be slanted. */
	cffPathCallbacks *pathcb;
	cffPathCallbacks *metricsPathcb; /* used to get some metrics WITHOUT building the newGlyphPath
	                                    NOTE! Must be kept parallel to pathcb, with regard to metric calculations.
	                                  */
	cffStdCallbacks *stdcb;
	struct {                     /* --- Euro data */
		TemplateGlyphData templateGlyph[kNumFillinFonts]; /* --- Figure zero or zerooldstyle data for Euro
		                                                     Did this because I thought I was supposed to use cap O
		                                                     for FillinMM, hence needed a different template for each fill-in font
		                                                   */

		FillinFontData fill_in_cff[kNumFillinFonts];    /* --- all the available fill-in MM CFF data */
		/* 0 is EuroMM*/
		/* 1 is FillinMM */
		/* These are filled from the Sans or Serif faces of each font,
		   depending on the serif-ness of the target font. */

		FontMatrix matrix; /* current font matrix. Points to a fill_in_cff[i].matrix */

		struct {
			char *data;
			long length;
		} cff;       /* copied from the current fill_in_cff[i].cff */

		long tmlength;          /* Length of Type 2 trademark charstring */
		int iMaster;            /* Current master index */
		int iStem;              /* Current stem index */
		int iCoord;             /* Current coord index */
		Blend width;
		double max_x;
		double max_y;
	} newGlyph;

#endif /* TC_EURO_SUPPORT */
#if TC_DEBUG
	struct {
		int input;              /* Dump input charstrings */
		int output;             /* Dump output charstrings */
	}
	debug;
#endif /* TC_DEBUG */
	tcCtx g;                    /* Package context */
};

static void add2Coords(recodeCtx h, int idx, int idy);

#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void saveTemplateGlyphData(recodeCtx h, TemplateGlyphData *templateGlyph);

#endif /* TC_EURO_SUPPORT */

/* Initialize width data */
static void initWidths(void *ctx, long count, WidthFD *wfd) {
	tcCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->ctx.dnaCtx, wfd->total, 50, 50);
		wfd++;
	}
	return;
}

/* Initialize module */
void recodeNew(tcCtx g) {
	recodeCtx h = MEM_NEW(g, sizeof(struct recodeCtx_));

	dnaINIT(g->ctx.dnaCtx, h->cstrs, 25000, 10000);
	dnaINIT(g->ctx.dnaCtx, h->chars, 250, 50);
	dnaINIT(g->ctx.dnaCtx, h->stem.list, 48, 48);
	dnaINIT(g->ctx.dnaCtx, h->cntr.list, 96, 96);
	dnaINIT(g->ctx.dnaCtx, h->cntr.idmask, 4, 4);
	dnaINIT(g->ctx.dnaCtx, h->path.segs, 50, 100);
	dnaINIT(g->ctx.dnaCtx, h->path.ops, 20, 100);
	dnaINIT(g->ctx.dnaCtx, h->path.args, 150, 150);
	dnaINIT(g->ctx.dnaCtx, h->path.idmask, 30, 100);
	dnaINIT(g->ctx.dnaCtx, h->width.fd, 1, 10);
	h->width.fd.func = initWidths;
	h->pend.trans = 0;
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
	IWARN(iMisc2, "sbw with vertical width (ignored)");
	h->warn.total = 0;

#if TC_DEBUG
	h->debug.input = 0;
	h->debug.output = 0;
#endif /* TC_DEBUG */

	/* Link contexts */
	h->g = g;
	g->ctx.recode = h;
}

/* Free resources */
void recodeFree(tcCtx g) {
	recodeCtx h = g->ctx.recode;
	int i;

	dnaFREE(h->cstrs);
	dnaFREE(h->chars);
	dnaFREE(h->stem.list);
	dnaFREE(h->cntr.list);
	dnaFREE(h->cntr.idmask);
	dnaFREE(h->path.segs);
	dnaFREE(h->path.ops);
	dnaFREE(h->path.args);
	dnaFREE(h->path.idmask);
	for (i = 0; i < h->width.fd.size; i++) {
		dnaFREE(h->width.fd.array[i].total);
	}
	dnaFREE(h->width.fd);
	MEM_FREE(g, h);
}

#if TC_SUBR_SUPPORT
/* Return operator length from opcode */
static int t2oplen(unsigned char *cstr) {
	switch (cstr[0]) {
		default:
			return 1;

		case tx_escape:
		case 247:
		case 248:
		case 249:
		case 250:
		case 251:
		case 252:
		case 253:
		case 254:
			return 2;

		case t2_shortint:
			return 3;

		case t2_separator:
			return 4;

		case 255:
			return 5;

		case t2_hintmask:
		case t2_cntrmask:
			return cstr[1];
	}
}

/* Copy and edit cstr by removing length bytes from mask operators. Return
   advanced destination buffer pointer */
static unsigned char *t2cstrcpy(unsigned char *pdst, unsigned char *psrc,
                                unsigned length) {
	int left;
	unsigned char *pend = psrc + length;

	while (psrc < pend) {
		switch (*psrc) {
			case t2_hintmask:
			case t2_cntrmask:           /* Mask ops; remove length byte */
				*pdst++ = *psrc++;
				left = *psrc++ - 2;
				while (left--) {
					*pdst++ = *psrc++;
				}
				length--;
				break;

			case 255:                   /* 5-byte number */
				*pdst++ = *psrc++;
				*pdst++ = *psrc++;
			/* Fall through */

			case t2_shortint:           /* 3-byte number */
				*pdst++ = *psrc++;
			/* Fall through */

			case tx_escape:             /* 2-byte number/esc operator */
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
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

/* Initialize for new font */
static void recodeNewFont(tcCtx g, Font *font) {
	recodeCtx h = g->ctx.recode;
	int i;

	if (g->flags & TC_SUPPRESS_HINT_WARNINGS) {
		h->flags |= RECODE_SUPPRESS_HINT_WARNINGS;
	}

	if (font->flags & FONT_CID && h->cstrs.size < 3000000) {
		/* Resize da's for CID fonts */
		if (!(g->flags & TC_SMALLMEMORY)) {
			/* xxx what if we've aready allocated cstrs to another font */
			dnaFREE(h->cstrs);
			dnaINIT(g->ctx.dnaCtx, h->cstrs, 3000000, 1000000);
		}
		dnaFREE(h->chars);
		dnaINIT(g->ctx.dnaCtx, h->chars, 9000, 1000);
	}
	h->font = font;
	h->nMasters = g->nMasters;
	if (!h->pend.trans) {
		h->trans = NULL;
	}
	h->cstrs.cnt = 0;
	h->chars.cnt = 0;
	for (i = 0; i < h->width.fd.size; i++) {
		h->width.fd.array[i].total.cnt = 0;
	}
	h->width.fd.cnt = 0;
	if (g->flags & TC_SUPPRESS_WIDTH_OPT)
        h->width.opt = 0;
    else
        h->width.opt = g->nMasters == 1;
	h->tmpFile.left = 0;
#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
	h->newGlyph.tmlength = 0;
	for (i = 0; i < kNumFillinFonts; i++) {
		h->newGlyph.templateGlyph[i].seen = 0;
		h->newGlyph.templateGlyph[i].alreadyWarnedonClampedUDV = 0;
	}

#endif /* TC_EURO_SUPPORT */

#if TC_SUBR_SUPPORT
	/* Initialize subr parse data */
	if (g->flags & TC_SUBRIZE) {
		static SubrParseData spd = {
			t2oplen,
			t2cstrcpy,
			csEncInteger,
			TX_MAX_CALL_STACK,
			t2_hintmask,
			t2_cntrmask,
			tx_callsubr,
			t2_callgsubr,
			tx_return,
			tx_endchar,
			t2_separator,
		};
		if (g->spd != NULL && g->spd != &spd) {
			parseFatal(g, "can't mix charstring types when subroutinizing");
		}
		g->spd = &spd;
	}
	h->stack.max = T2_MAX_OP_STACK - ((g->flags & TC_SUBRIZE) != 0);
#else /* TC_SUBR_SUPPORT */
	h->stack.max = T2_MAX_OP_STACK;
#endif /* TC_SUBR_SUPPORT */
}

/* Bad charstring; print fatal message with glyph and FontName appended */
static void badChar(recodeCtx h) {
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
static void warnChar(recodeCtx h, int iWarn) {
	h->warn.count[iWarn]++;
	h->warn.total++;
}

/* Print warning message with glyph and FontName appended */
static void showWarn(recodeCtx h) {
	int i;
	char id[128];

	if (h->flags & RECODE_SUPPRESS_HINT_WARNINGS) {
		return;
	}

	/* Construct glyph identifier */
	switch (h->idType) {
		case SIDType:
			sprintf(id, "<%s>", sindexGetString(h->g, (SID)h->id));
			break;

		case CIDType:
			sprintf(id, "cid#%u", h->id);
			break;

		case SubrType:
			sprintf(id, "subr#%u", h->id);
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
static void setBlend(recodeCtx h, Blend a, Fixed value) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		a[i] = value;
	}
}

/* Make blend array from first element */
static void makeBlend(recodeCtx h, Blend a) {
	int i;
	for (i = 1; i < h->nMasters; i++) {
		a[i] = a[0];
	}
}

/* Add 2 blend arrays */
static void addBlend(recodeCtx h, Blend result, Blend a, Blend b) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		result[i] = a[i] + b[i];
	}
}

/* Subtract 2 blend arrays */
static void subBlend(recodeCtx h, Blend result, Blend a, Blend b) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		result[i] = a[i] - b[i];
	}
}

/* Negate blend array */
static void negBlend(recodeCtx h, Blend a) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		a[i] = -a[i];
	}
}

/* Compare blend array against supplied value. Return 1 if same else 0 */
static int cmpBlend(recodeCtx h, Blend a, Fixed value) {
	int i;
	for (i = 0; i < h->nMasters; i++) {
		if (a[i] != value) {
			return 0;
		}
	}
	return 1;
}

/* Return stack space required by blend value */
static int spaceBlend(recodeCtx h, Blend value) {
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
static void moveElement(recodeCtx h, int iDst, int iSrc) {
	h->stack.flags[iDst] = h->stack.flags[iSrc];
	if (STK_BLEND(iDst)) {
		copyBlend(h->stack.array[iDst], h->stack.array[iSrc]);
	}
	else {
		INDEX(iDst) = INDEX(iSrc);
	}
}

/* Copy element from stack */
static void saveElement(recodeCtx h, Blend dst, int index) {
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
static void addElement(recodeCtx h, int iDst, int iSrc) {
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
static void subElement(recodeCtx h, int iDst, int iSrc) {
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
static int lookupWidth(recodeCtx h, int width, int *index) {
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
static void addWidth(recodeCtx h, Fixed width) {
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
			bytes = csEncInteger(this->width, dst);
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
			Width *new = &dnaGROW(h->width.curr->total,
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
static long sizeFDWidths(recodeCtx h, int fd) {
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
static long recodeEndFont(tcCtx g) {
	recodeCtx h = g->ctx.recode;
	int i;
	long size = 0;
	long total = (g->flags & TC_SMALLMEMORY) ?
	    h->chars.array[h->chars.cnt - 1].icstr + h->cstrs.cnt : h->cstrs.cnt;

	if (!h->width.opt) {
		return total;   /* No optimization */
	}
	for (i = 0; i < h->width.fd.cnt; i++) {
		size += sizeFDWidths(h, i);
	}

	return total - h->chars.cnt * DUMMY_WIDTH_SIZE + size;
}

/* Compare stems */
static int cmpStems(recodeCtx h, Stem *pat, Stem *stem) {
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
static int lookupStem(recodeCtx h, Stem *pat, int *index) {
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
static void addStems(recodeCtx h, int vert, int cntr) {
	int i;
	Stem stem;

	/* If we are flattening an oblique synthetix font, omit all vertical stems. */
	if ((h->font->synthetic.oblique_term != 0.0) && (vert)) {
		h->stack.cnt = 0;
		return;
	}

	stem.type = (vert ? STEM_VERT : 0) | (cntr ? STEM_CNTR : 0);


	if (h->stack.cnt < 2 || h->stack.cnt & 1) {
		badChar(h);
	}

	if (h->seac.hint == 1) {
		/* In accent component of seac operator */
		if (h->hint.initial) {
			h->hint.initial = 0;
		}
		h->seac.hint++;
	}

	for (i = 0; i < h->stack.cnt; i += 2) {
		int j;
		int index;

		saveElement(h, stem.edge, i);
		if (h->seac.phase >= 2) {
			/* Accent component; adjust stem edge */
			addBlend(h, stem.edge, stem.edge,
			         vert ? h->seac.adx : h->seac.ady);
		}
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

			if (h->stem.list.cnt == T2_MAX_STEMS) {
				break;  /* Limit exceeded; discard remaining stems */
			}
			stem.id = (unsigned char)h->stem.list.cnt;
			new = &dnaGROW(h->stem.list, h->stem.list.cnt)[index];
			COPY(new + 1, new, h->stem.list.cnt++ - index);
			*new = stem;

			/* Remember if we have cntr hints. Test here in case of discard */
			h->hint.cntr |= cntr;
		}
		if (h->hint.initial && h->path.segs.cnt > 1) {
			/* Unsubstituted hints after drawing ops; insert hint subs */
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
static void foldStack(recodeCtx h, int nBlends) {
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
static void addPathOp(recodeCtx h, int nArgs, int iArgs, int type) {
	Operator *p = dnaNEXT(h->path.ops);
	p->nArgs = nArgs;
	p->iArgs = iArgs;
	p->iSeg = (unsigned short)h->path.segs.cnt;
	p->type = type;
}

/* Add segment to path. */
static void addPathSeg(recodeCtx h, int type) {
	Segment *p = dnaNEXT(h->path.segs);
	p->iArgs = (unsigned short)h->path.args.cnt;
	p->flags = type;
}

/* Handle pending hint state */
static void pendOp(recodeCtx h, int newop) {
	if (h->hint.subs && !(h->g->flags & TC_NOHINTS)) {
		/* Add hint subs id mask */
		int idbytes = (h->stem.list.cnt + 7) / 8;

		if (h->seac.hint == 3) {
			/* Seac accent synthetic hintmask; save index */
			h->seac.iHintmask = h->path.ops.cnt;
			h->seac.hint++;
		}
		else if (h->seac.hint == 4) {
			/* Seac accent real hintmask; remove synthetic hintmask */
			memmove(&h->path.ops.array[h->seac.iHintmask],
			        &h->path.ops.array[h->seac.iHintmask + 1],
			        (h->path.ops.cnt - h->seac.iHintmask + 1) *
			        sizeof(Operator));
			h->path.ops.cnt--;
			h->seac.hint++;
		}

		addPathOp(h, idbytes, h->path.idmask.cnt, t2_hintmask);
		memcpy(dnaEXTEND(h->path.idmask, idbytes), h->stem.subsmask, idbytes);

		CLEAR_ID_MASK(h->stem.subsmask);
		h->hint.subs = 0;
	}
	else if (h->pend.op == tx_dotsection && newop != tx_endchar &&
	         !(h->g->flags & (TC_ROM | TC_NOOLDOPS))) {
		addPathOp(h, 0, 0, tx_dotsection);
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
static void addXCoord(recodeCtx h, Blend coord, int idx) {
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

	/* fix synthetically obliqued font. */
	for (i = 0; i < h->nMasters; i++) {
		if (h->font->synthetic.oblique_term != 0.0) {
			coord[i] = DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[i])));
		}
	}
}

/* Add blend y-coordinate value */
static void addYCoord(recodeCtx h, Blend coord, int idy) {
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

static void synthesizeBlendCoords(recodeCtx h, Fixed *coord, int numMasters, int numCoords) {
	int pairIndex, masterIndex, numPairs = numCoords / 2;

	for (pairIndex = 0; pairIndex < numPairs; pairIndex++) {
		Fixed *pairCoord = &coord[pairIndex * numMasters];

		for (masterIndex = 0; masterIndex < numMasters; masterIndex++) {
			pairCoord[masterIndex] +=
			    DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[pairCoord[masterIndex + numMasters]])));
		}
	}
}

/* Add 2 coordinates */
static void add2Coords(recodeCtx h, int idx, int idy) {
	if (h->nMasters == 1) {
		Fixed *coord = dnaEXTEND(h->path.args, 2);
		coord[0] = (h->path.x[0] += ((idx == -1) ? 0 : INDEX(idx)));
		coord[1] = (h->path.y[0] += ((idy == -1) ? 0 : INDEX(idy)));

		if (h->font->synthetic.oblique_term != 0.0) {
			coord[0] += DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[1])));
		}
	}
	else {
		addXCoord(h, dnaEXTEND(h->path.args, h->nMasters), idx);
		addYCoord(h, dnaEXTEND(h->path.args, h->nMasters), idy);
		if (h->font->synthetic.oblique_term != 0.0) {
			synthesizeBlendCoords(h, h->path.args.array, h->nMasters, 2);
		}
	}
}

/* Add moveto op */
static void addMove(recodeCtx h, int idx, int idy) {
	pendOp(h, tx_rmoveto);
	addPathSeg(h, seg_move);
	if (h->seac.phase == 2) {
		/* Accent component moveto; set path to first accent coord */
		addBlend(h, h->path.x, h->seac.bsb, h->seac.adx);
		if (idx != -1) {
			addBlend(h, h->path.x, h->path.x, h->stack.array[idx]);
		}

		copyBlend(h->path.y, h->seac.ady);
		if (idy != -1) {
			addBlend(h, h->path.y, h->path.y, h->stack.array[idy]);
		}

		add2Coords(h, -1, -1);
		h->seac.phase = 3;

		if (h->seac.hint == 2) {
			/* Accent had hints; insert hintmask at beginning of subpath */
			h->hint.seen = 1;
			h->hint.subs = 1;
			h->seac.hint++;
		}
	}
	else {
		add2Coords(h, idx, idy);
	}
	h->stack.cnt = 0;
}

/* Add lineto op */
static void addLine(recodeCtx h, int idx, int idy) {
	pendOp(h, tx_rlineto);
	addPathSeg(h, seg_line);
	add2Coords(h, idx, idy);
	h->stack.cnt = 0;
}

/* Add 6 coordinates */
static void add6Coords(recodeCtx h, int idx1, int idy1,
                       int idx2, int idy2, int idx3, int idy3) {
	if (h->nMasters == 1) {
		Fixed *coord = dnaEXTEND(h->path.args, 6);
		coord[0] = (h->path.x[0] += ((idx1 == -1) ? 0 : INDEX(idx1)));
		coord[1] = (h->path.y[0] += ((idy1 == -1) ? 0 : INDEX(idy1)));
		coord[2] = (h->path.x[0] += ((idx2 == -1) ? 0 : INDEX(idx2)));
		coord[3] = (h->path.y[0] += ((idy2 == -1) ? 0 : INDEX(idy2)));
		coord[4] = (h->path.x[0] += ((idx3 == -1) ? 0 : INDEX(idx3)));
		coord[5] = (h->path.y[0] += ((idy3 == -1) ? 0 : INDEX(idy3)));

		if (h->font->synthetic.oblique_term != 0.0) {
			coord[0] += DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[1])));
			coord[2] += DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[3])));
			coord[4] += DBL2FIX(floor(h->font->synthetic.oblique_term * FIX2DBL(coord[5])));
		}
	}
	else {
		addXCoord(h, dnaEXTEND(h->path.args, h->nMasters), idx1);
		addYCoord(h, dnaEXTEND(h->path.args, h->nMasters), idy1);
		addXCoord(h, dnaEXTEND(h->path.args, h->nMasters), idx2);
		addYCoord(h, dnaEXTEND(h->path.args, h->nMasters), idy2);
		addXCoord(h, dnaEXTEND(h->path.args, h->nMasters), idx3);
		addYCoord(h, dnaEXTEND(h->path.args, h->nMasters), idy3);
		if (h->font->synthetic.oblique_term != 0.0) {
			synthesizeBlendCoords(h, h->path.args.array, h->nMasters, 6);
		}
	}
}

/* Add curve to operation */
static void addCurve(recodeCtx h, int idx1, int idy1,
                     int idx2, int idy2, int idx3, int idy3) {
	pendOp(h, tx_rrcurveto);
	addPathSeg(h, seg_curve);
	add6Coords(h, idx1, idy1, idx2, idy2, idx3, idy3);
	h->stack.cnt = 0;
}

/* Add single value without changing current x or y coordinate */
static void addValue(recodeCtx h, int iv) {
	if (h->nMasters == 1) {
		*dnaNEXT(h->path.args) = INDEX(iv);
	}
	else {
		int i;
		Fixed *coord = dnaEXTEND(h->path.args, h->nMasters);
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
static void addOp(recodeCtx h, int op) {
	int i;

	pendOp(h, op);
	addPathOp(h, h->stack.cnt, h->path.args.cnt, op);

	for (i = 0; i < h->stack.cnt; i++) {
		addValue(h, i);
	}
	h->stack.cnt = 0;
}

/* Perform various kinds of division */
static void doDiv(recodeCtx h, int shift) {
	if (h->stack.cnt < 2) {
		/* This div operator depended on computed results; just save it */
		addOp(h, tx_div);
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
static void addFlex(recodeCtx h) {
	pendOp(h, t2_flex);
	CHKUFLOW(17);

	/* Add reference point and first control point relative moves */
	addElement(h, 2, 0);
	addElement(h, 3, 1);

	addPathOp(h, 1, h->path.args.cnt, t2_flex);
	addValue(h, 14);    /* Flex height */

	addPathSeg(h, seg_curve);
	add6Coords(h, 2, 3,  4,  5,  6,  7);
	addPathSeg(h, seg_curve);
	add6Coords(h, 8, 9, 10, 11, 12, 13);

	/* obliquing a charstring with flex should mek problems that I haven't dealt with yet. */
	if (h->font->synthetic.oblique_term != 0.0) {
		parseWarning(h->g, "Error: flattened obliqued synthetic font uses flex. Should produce other flex errors.");
	}

	/* Check that the current point agrees with the redundant one in flex op.
	   In the event of mismatch report it and update the current point from the
	   value stored with the flex op. This mimics the way that the rasterizer
	   functions and ensures that incorrectly constructed fonts will be
	   faithfully reproduced even when wrong! The major source of these errors
	   is a result of a BuildFont off-by-one bug when moving from a closepoint
	   contour to a new contour in MM fonts. */

	if (h->nMasters == 1) {
		if (h->seac.phase != 3 &&
		    (h->path.x[0] != INDEX(15) || h->path.y[0] != INDEX(16))) {
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
			if (h->seac.phase != 3 &&
			    (h->path.x[i] != h->stack.array[15][i] ||
			     h->path.y[i] != h->stack.array[16][i])) {
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
static void addCntrCntl(recodeCtx h, int op, int argcnt) {
	int i;
	Blend *ctr;

	if (h->stack.cnt != argcnt || h->stack.cnt < 2) {
		badChar(h);
	}

	if (op == t1_otherCntr1 && argcnt != 22) {
		warnChar(h, iMisc0);
	}

	ctr = dnaEXTEND(h->cntr.list, argcnt);
	for (i = h->stack.cnt - 1; i >= 0; i--) {
		saveElement(h, *ctr++, i);
	}

	h->stack.cnt = 0;
}

/* Parse type 1 charstring. Called recursively. Return 1 on endchar else 0 */
static int t1parse(recodeCtx h, unsigned length, unsigned char *cstr,
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
				if (h->seac.phase == 0) {
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
				}
				h->stack.cnt = 0;
				break;

			case tx_endchar:
				if (h->seac.phase != 1) {
					pendOp(h, tx_endchar);
					addPathOp(h, 0, 0, tx_endchar);
				}
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

			case 255: {
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
						addOp(h, tx_and);
						break;

					case tx_or:
						addOp(h, tx_or);
						break;

					case tx_not:
						addOp(h, tx_not);
						break;

					case t1_seac:
						CHKUFLOW(5);
						if (h->g->flags & TC_NOOLDOPS) {
							/* Convert seac */
							int base;
							int accent;

							/* Save args */
							copyBlend(h->seac.bsb, h->path.x);
							saveElement(h, h->seac.adx, 1);
							saveElement(h, h->seac.ady, 2);
							base = INDEX(3) >> 16;
							accent = INDEX(4) >> 16;
							h->stack.cnt = 0;

							/* Process base component */
							if (parseGetComponent(g, base, &length, &cstr)) {
								badChar(h);
							}
							h->seac.phase = 1;
							(void)t1parse(h, length, cstr, nSubrs, subrs);

							/* Process accent component */
							if (parseGetComponent(g, accent, &length, &cstr)) {
								badChar(h);
							}
							h->seac.phase = 2;
							h->seac.hint = 1;
							(void)t1parse(h, length, cstr, nSubrs, subrs);
						}
						else {
							if (h->nMasters == 1) {
								INDEX(1) += -INDEX(0) + h->path.lsb.x[0];
							}
							else {
								subElement(h, 1, 0);
								addBlend(h, h->stack.array[1], h->stack.array[1],
								         h->path.lsb.x);
							}

							/* Save seac */
							addPathOp(h, 4, h->path.args.cnt, tx_endchar);
							addValue(h, 1);
							addValue(h, 2);
							addValue(h, 3);
							addValue(h, 4);

							if (h->g->status & TC_SUBSET) {
								/* Add components to subset glyph list */
								if (parseAddComponent(g, INDEX(3) >> 16) ||
								    parseAddComponent(g, INDEX(4) >> 16)) {
									badChar(h);
								}
							}
						}
						return 1;

					case t1_sbw:
						CHKUFLOW(4);
						if (h->seac.phase == 0) {
							if ((!STK_BLEND(3) && INDEX(3) != 0) ||
							    (STK_BLEND(3) && !cmpBlend(h, h->stack.array[2], 0))) {
								warnChar(h, iMisc2);
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
						}
						h->stack.cnt = 0;
						break;

					case tx_store:
						addOp(h, tx_store);
						break;

					case tx_abs:
						addOp(h, tx_abs);
						break;

					case tx_add:
						addOp(h, tx_add);
						break;

					case tx_sub:
						addOp(h, tx_sub);
						break;

					case tx_div:
						doDiv(h, 1);
						break;

					case tx_load:
						addOp(h, tx_load);
						break;

					case tx_neg:
						addOp(h, tx_neg);
						break;

					case tx_eq:
						addOp(h, tx_eq);
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
									addOp(h, t2_cntron);
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
										addOp(h, tx_store);
									}
									break;

								case t1_otherAdd:
									addOp(h, tx_add);
									break;

								case t1_otherSub:
									addOp(h, tx_sub);
									break;

								case t1_otherMul:
									addOp(h, tx_mul);
									break;

								case t1_otherDiv:
									doDiv(h, 0);
									break;

								case t1_otherPut:
									addOp(h, tx_put);
									break;

								case t1_otherGet:
									addOp(h, tx_get);
									break;

								case t1_otherIfelse:
									addOp(h, tx_ifelse);
									break;

								case t1_otherRandom:
									addOp(h, tx_random);
									break;

								case t1_otherDup:
									addOp(h, tx_dup);
									break;

								case t1_otherExch:
									addOp(h, tx_exch);
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
						addOp(h, tx_drop);
						break;

					case t1_setwv:
						parseFatal(g, "unsupported operator [setwv]");
						break;

					case tx_put:
						addOp(h, tx_put);
						break;

					case tx_get:
						addOp(h, tx_get);
						break;

					case tx_ifelse:
						addOp(h, tx_ifelse);
						break;

					case tx_random:
						addOp(h, tx_random);
						break;

					case tx_mul:
						addOp(h, tx_mul);
						break;

					case t1_div2:
						doDiv(h, 0);
						break;

					case tx_sqrt:
						addOp(h, tx_sqrt);
						break;

					case tx_dup:
						addOp(h, tx_dup);
						break;

					case tx_exch:
						addOp(h, tx_exch);
						break;

					case tx_index:
						addOp(h, tx_index);
						break;

					case tx_roll: {
						Fixed n;
						CHKUFLOW(2);
						n = INDEX(h->stack.cnt - 2) >> 16;
						if (n < 0) {
							badChar(h);
						}
						addOp(h, tx_roll);
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
static void pushValue(recodeCtx h, Blend value) {
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
static void saveElements(recodeCtx h, int iFirst, int cnt) {
	char *t;
	char *end;
	int max;
	int i;

	if (STK_BLEND(iFirst)) {
		/* Save blend group */
		max = cnt * h->nMasters * 5 + 2;
		t = dnaEXTEND(h->cstrs, max);
		end = t + max;

		/* Save initial values */
		for (i = 0; i < cnt; i++) {
			t += csEncFixed(h->stack.array[iFirst + i][0], t);
		}

		/* Save deltas */
		for (i = 0; i < cnt; i++) {
			int j;
			Fixed *blend = h->stack.array[iFirst + i];
			for (j = 1; j < h->nMasters; j++) {
				t += csEncFixed(blend[j] - blend[0], t);
			}
		}

		/* save blend count and op */
		t += csEncInteger(cnt, t);
		*t++ = t2_blend;
	}
	else {
		/* Save non-blend group */
		max = cnt * 5;
		t = dnaEXTEND(h->cstrs, max);
		end = t + max;

		for (i = 0; i < cnt; i++) {
			t += csEncFixed(INDEX(iFirst + i), t);
		}
	}
	h->cstrs.cnt -= end - t;    /* Only count bytes used */
}

/* Save stack elements */
static void saveStack(recodeCtx h) {
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
			     i + (i - iGroup + 1) * h->nMasters > h->stack.max)) {
				saveElements(h, iGroup, i - iGroup);
				iGroup = i;
			}
		}
	}
	h->stack.cnt = 0;
	h->stack.limit = 0;
}

/* Save stem args and ops observing stack limit */
static void saveStemOp(recodeCtx h,
                       int iStart, int iEnd, int vert, int optimize) {
	Blend last;
	int i;
	int op = h->hint.seen ?
	    (vert ? t2_vstemhm : t2_hstemhm) : (vert ? tx_vstem : tx_hstem);

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

		if (h->stack.limit + space > h->stack.max) {
			/* Stack full; save stack and stem op */
			saveStack(h);
			if (!optimize || i != iEnd) {
				T2SAVEOP(op, h->cstrs);
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
		T2SAVEOP(op, h->cstrs);
	}
}

/* Set counter control group id mask */
static Blend *setCntrMask(recodeCtx h, Blend *p, int vert, HintMask cntrmask) {
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
			if (h->stem.list.cnt != T2_MAX_STEMS) {
				/* Insert dummy stem so that the global coloring data exactly
				   matches the original and glyph bitmap comparisons succeed */
				Stem *new = &dnaGROW(h->stem.list, h->stem.list.cnt)[index];

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
static void setCntrMaskGroups(recodeCtx h) {
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
		cntrmask = dnaNEXT(h->cntr.idmask);
		CLEAR_ID_MASK(*cntrmask);
	}
	else {
		do {
			/* Each iteration saves the group(s) for one priority */
			cntrmask = dnaNEXT(h->cntr.idmask);
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
static void checkOverlap(recodeCtx h, HintMask hintmask) {
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
static void saveHintMaskOp(recodeCtx h, unsigned char *map, int op,
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
		/* warnChar(h, iHint5); */ /* Supressing this becuase it is so common. */
		/* Users end up ignoring the few more interesting cases. */
		return;
	}
	memcpy(h->stem.lastmask, hintmask, h->hint.size);

	/* Save mask op */
	T2SAVEOP(op, h->cstrs);

#if TC_SUBR_SUPPORT
	if (h->g->flags & TC_SUBRIZE) {
		/* Save mask op size in second byte for subroutinizer */
		*dnaNEXT(h->cstrs) = h->hint.size + 2;
	}
#endif /* TC_SUBR_SUPPORT */

	SAVE_HINT_MASK(hintmask);

#if TC_HINT_CHECK
	/* Counter masks (counter control list) aren't checked because it yields
	   too many warnings due to stem merging and integerization of list */
	if (op == t2_hintmask) {
		checkOverlap(h, hintmask);
	}
#endif /* TC_HINT_CHECK */
}

/* Save stem hints */
static void saveStems(recodeCtx h, unsigned char *map) {
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

		T2SAVEOP(t2_cntrmask, h->cstrs);

#if TC_SUBR_SUPPORT
		if (h->g->flags & TC_SUBRIZE) {
			/* Save mask op size in second byte for subroutinizer */
			*dnaNEXT(h->cstrs) = h->hint.size + 2;
		}
#endif /* TC_SUBR_SUPPORT */

		SAVE_HINT_MASK(cntrmask);
	}
	else {
		/* Save counter control masks (if any) */
		for (i = 0; i < h->cntr.idmask.cnt; i++) {
			saveHintMaskOp(h, map, t2_cntrmask,
			               h->hint.size, h->cntr.idmask.array[i], 0);
		}
	}

	if (superset) {
		/* Save initial hints */
		saveHintMaskOp(h, map, t2_hintmask, h->hint.size, h->stem.initmask, 0);
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
static void loadDeltaArgs(recodeCtx h, unsigned index, int nDeltas) {
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
static void loadArgs(recodeCtx h, unsigned index, int nArgs) {
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
static void swapArgs(recodeCtx h, int i, int j) {
	Fixed value[TX_MAX_MASTERS];
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
static int doFlex(recodeCtx h, int iCoords, int iHeight) {
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
		stdHeight = height[0] == DBL2FIX(TX_STD_FLEX_HEIGHT);
	}
	else {
		subBlend(h, pendx, &coord[8 * h->nMasters], h->path.x);
		subBlend(h, pendy, &coord[9 * h->nMasters], h->path.y);
		subBlend(h, dx, &coord[10 * h->nMasters], h->path.x);
		subBlend(h, dy, &coord[11 * h->nMasters], h->path.y);
		dxz = cmpBlend(h, dx, 0);
		dyz = cmpBlend(h, dy, 0);
		stdHeight = cmpBlend(h, &height[0], DBL2FIX(TX_STD_FLEX_HEIGHT));
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
		return t2_flex;
	}
	else if (dyz && ARG_ZERO(1) && ARG_ZERO(5) &&
	         ARG_ZERO(7) && ARG_ZERO(11)) {
		/* hflex */
		ARG_SELECT(10);
		return t2_hflex;
	}
	else if (dyz && ARG_ZERO(5) && ARG_ZERO(7)) {
		/* hflex1 */
		ARG_SELECT(1);
		ARG_SELECT(9);
		ARG_SELECT(10);
		return t2_hflex1;
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
		return t2_flex1;
	}
}

/* Calulate max stack depth for new args */
static int maxArgsDepth(recodeCtx h) {
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
static void fillStack(recodeCtx h) {
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
static void saveNewOp(recodeCtx h, int newop) {
	fillStack(h);
	saveStack(h);
	T2SAVEOP(newop, h->cstrs);
	h->pend.op = t2_noop;
}

/* Save pending stack args and op */
static void savePendOp(recodeCtx h, int pendop) {
	saveStack(h);
	T2SAVEOP(pendop, h->cstrs);
	h->pend.op = t2_noop;
}

/* Recode char path */
static void recodePath(recodeCtx h) {
	unsigned char map[T2_MAX_STEMS];    /* Stem id map (id -> index) */
	int iSeg;
	Operator *nextop;
	int seqop = 0;      /* Initial op in alternating h/v sequence */
#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
	long icstr = h->cstrs.cnt;
#endif /* TC_EURO_SUPPORT */

	/* Initialize */
	h->stack.cnt = 0;
	h->stack.limit = 0;
	h->pend.op = t2_noop;
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
		else if (h->idType != SubrType) {
			pushValue(h, h->path.width); /* Save glyph width */
		}
	}

	saveStems(h, map);  /* Save stems with cntr and initial hint masks */

	/* Create optimized Type 2 charstring from operators and segments */
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
						newop = tx_vhcurveto;
					}
					else if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						fourArgs = 1;
						newop = t2_vvcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						newop = tx_vhcurveto;
					}
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						fourArgs = 1;
						newop = tx_hvcurveto;
					}
					else if (ARG_ZERO(5)) {
						ARG_SELECT(4);
						fourArgs = 1;
						newop = t2_hhcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						swapArgs(h, 4, 5);
						newop = tx_hvcurveto;
					}
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					if (ARG_ZERO(4)) {
						ARG_SELECT(5);
						newop = t2_vvcurveto;
					}
					else if (ARG_ZERO(5)) {
						ARG_SELECT(4);
						swapArgs(h, 0, 1);
						newop = t2_hhcurveto;
					}
					else {
						ARG_SELECT(4);
						ARG_SELECT(5);
						newop = tx_rrcurveto;
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
					newop = tx_vlineto;
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					newop = tx_hlineto;
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					newop = tx_rlineto;
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
					newop = tx_vmoveto;
				}
				else if (ARG_ZERO(1)) {
					ARG_SELECT(0);
					newop = tx_hmoveto;
				}
				else {
					ARG_SELECT(0);
					ARG_SELECT(1);
					newop = tx_rmoveto;
				}
			}
		}
		else {
			/* Add next operator */
			op = nextop++;
			switch (op->type) {
				case t2_flex: {
					Segment *seg = &h->path.segs.array[iSeg];
					newop = doFlex(h, seg->iArgs, op->iArgs);
					iSeg += 2; /* Skip flex curves */
				}
				break;

				case t2_hintmask:
					newop = t2_hintmask;
					break;

				case tx_endchar:
					loadArgs(h, op->iArgs, op->nArgs);
					ARG_SELECT(0);
					ARG_SELECT(1);
					ARG_SELECT(2);
					ARG_SELECT(3);
					newop = tx_endchar;
					break;

				default: {
					/* 2-byte args ops */
					int j;
					if (op->nArgs > 13) {
						badChar(h);
					}
					loadArgs(h, op->iArgs, op->nArgs);
					for (j = 0; j < op->nArgs; j++) {
						ARG_SELECT(j);
					}
					newop = tx_escape;
				}
				break;
			}
		}

		/* Save pending op if new op's args don't fit on stack */
		if (maxArgsDepth(h) + h->stack.limit > h->stack.max) {
			switch (h->pend.op) {
				case tx_vlineto:
				case tx_hlineto:
				case tx_vhcurveto:
				case tx_hvcurveto:
					savePendOp(h, seqop);
					break;

				default:
					savePendOp(h, h->pend.op);
					break;
			}
		}

		/* Check pending operator */
		switch (h->pend.op) {
			case tx_vlineto:
				if (newop == tx_hlineto) {
					goto pending;
				}
				savePendOp(h, seqop);
				break;

			case tx_hlineto:
				if (newop == tx_vlineto) {
					goto pending;
				}
				savePendOp(h, seqop);
				break;

			case tx_rlineto:
				if (newop == tx_rlineto) {
					goto pending;
				}
				else if (newop == tx_rrcurveto) {
					saveNewOp(h, t2_rlinecurve);
					continue;
				}
				savePendOp(h, tx_rlineto);
				break;

			case tx_vhcurveto:
				if (newop == tx_hvcurveto) {
					if (fourArgs) {
						goto pending;
					}
					saveNewOp(h, seqop);
					continue;
				}
				savePendOp(h, seqop);
				break;

			case t2_vvcurveto:
				if (newop == t2_vvcurveto && fourArgs) {
					goto pending;
				}
				savePendOp(h, t2_vvcurveto);
				break;

			case tx_hvcurveto:
				if (newop == tx_vhcurveto) {
					if (fourArgs) {
						goto pending;
					}
					saveNewOp(h, seqop);
					continue;
				}
				savePendOp(h, seqop);
				break;

			case t2_hhcurveto:
				if (newop == t2_hhcurveto && fourArgs) {
					goto pending;
				}
				savePendOp(h, t2_hhcurveto);
				break;

			case tx_rrcurveto:
				if (newop == tx_rrcurveto) {
					goto pending;
				}
				else if (newop == tx_rlineto) {
					saveNewOp(h, t2_rcurveline);
					continue;
				}
				savePendOp(h, tx_rrcurveto);
				break;
		}
		h->pend.op = t2_noop;

		/* See if new op should become pending */
		switch (newop) {
			case tx_vmoveto:
			case tx_hmoveto:
			case tx_rmoveto:
				saveNewOp(h, newop);
				continue;

			case tx_vlineto:
			case tx_hlineto:
				seqop = newop;
				break;

			case tx_vhcurveto:
				if (!fourArgs) {
					saveNewOp(h, tx_vhcurveto);
					continue;
				}
				seqop = tx_vhcurveto;
				break;

			case tx_hvcurveto:
				if (!fourArgs) {
					saveNewOp(h, tx_hvcurveto);
					continue;
				}
				seqop = tx_hvcurveto;
				break;

			case t2_hintmask:
				saveHintMaskOp(h, map, t2_hintmask,
				               op->nArgs, &h->path.idmask.array[op->iArgs], 1);
				continue;

			case tx_endchar:
				fillStack(h);
				saveStack(h);
				T2SAVEOP(tx_endchar, h->cstrs);

#if TC_SUBR_SUPPORT
				if (h->g->flags & TC_SUBRIZE) {
					/* Add unique separator to charstring for subroutinizer */
					unsigned char *cstr = (unsigned char *)dnaEXTEND(h->cstrs, 4);
					cstr[0] = (unsigned char)t2_separator;
					cstr[1] = (unsigned char)(h->unique >> 16);
					cstr[2] = (unsigned char)(h->unique >> 8);
					cstr[3] = (unsigned char)h->unique++;
				}
#endif /* TC_SUBR_SUPPORT */

#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
				/* Collect information about path for euro matching. */
				if (h->g->flags & (TC_ADDEURO | TC_FORCE_NOTDEF) && h->idType == SIDType) {
					switch (h->id) {
						case SID_ZEROOLDSTYLE:
							if (h->newGlyph.templateGlyph[kEuroMMIndex].seen) {
								break; /* Glyph zero takes precedence */
							}

						/* Fall through */
						case SID_ZERO:
							saveTemplateGlyphData(h, &h->newGlyph.templateGlyph[kEuroMMIndex]);
							copyBlend(h->newGlyph.templateGlyph[kEuroMMIndex].width, h->path.width);
							h->newGlyph.templateGlyph[kEuroMMIndex].seen = 1;

							saveTemplateGlyphData(h, &h->newGlyph.templateGlyph[kFillinMMIndex]);
							copyBlend(h->newGlyph.templateGlyph[kFillinMMIndex].width, h->path.width);
							h->newGlyph.templateGlyph[kFillinMMIndex].seen = 2;

							break;

						case SID_TRADEMARK:
							h->newGlyph.tmlength = h->cstrs.cnt - icstr;
							break;

/*
                case SID_CAP_O:
                    saveTemplateGlyphData(h, &h->newGlyph.capO);
                    copyBlend(h->newGlyph.capO.width, h->path.width);
                    h->newGlyph.capO.seen = 1;
                    break;
 */
					}
				}

#endif /* TC_EURO_SUPPORT */


				return;

			case tx_escape:
				saveNewOp(h, op->type);
				continue;

			case tx_callsubr:
			case t2_hflex:
			case t2_flex:
			case t2_hflex1:
			case t2_flex1:
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
	recodeCtx h = g->ctx.recode;

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

	h->pend.op = t2_noop;
	h->pend.flex = 0;
	h->pend.move = 1;
	h->pend.trans = 0;

	h->path.width[0] = LONG_MIN;
	h->path.segs.cnt = 0;
	h->path.ops.cnt = 0;
	h->path.args.cnt = 0;
	h->path.idmask.cnt = 0;
	h->seac.phase = 0;
	h->seac.hint = 0;

	/* Parse and recode charstring */
	(void)t1parse(h, length, cstr, nSubrs, subrs);
	recodePath(h);

	if (g->flags & TC_SMALLMEMORY) {
		/* Save charstring in tmp file */
		g->cb.tmpWriteN(g->cb.ctx, h->cstrs.cnt, (char *)h->cstrs.array);
	}

	if (h->warn.total > 0) {
		showWarn(h);
	}
}

/* Convert transitional design subrs */
static void convTransSubrs(tcCtx g, unsigned nSubrs, Charstring *subrs) {
	recodeCtx h = g->ctx.recode;
	int i;
	int length;
	CSData *dst = &parseGetFont(g)->subrs;
	int icstrs = h->cstrs.cnt;

	/* Allocate subr offsets */
	dst->nStrings = h->trans->subrs.cnt;
	dst->offset = MEM_NEW(h->g, sizeof(Offset) * dst->nStrings);

	/* Convert subrs */
	h->idType = SubrType;
	for (i = 0; i < dst->nStrings; i++) {
		unsigned isubr = h->trans->subrs.array[i];
		if (isubr >= nSubrs) {
			badChar(h);
		}

		h->id = isubr;
		cstrRecode(g, subrs[isubr].length, (unsigned char *)subrs[isubr].cstr,
		           nSubrs, subrs);

		dst->offset[i] = h->cstrs.cnt - icstrs; /* Save subr offset */
	}

	/* Allocate/copy subr data */
	length = h->cstrs.cnt - icstrs;
	dst->data = MEM_NEW(h->g, length);
	memcpy(dst->data, &h->cstrs.array[icstrs], length);

	h->cstrs.cnt = icstrs;
	h->pend.trans = 0;
}

/* Recode type 1 charstring */
static void recodeAddChar(tcCtx g, unsigned length, char *cstr, unsigned id,
                          unsigned nSubrs, Charstring *subrs, int fd) {
	recodeCtx h = g->ctx.recode;

	/* Save charstring index */
	if (g->flags & TC_SMALLMEMORY) {
		long prev =
		    (h->chars.cnt > 0) ? h->chars.array[h->chars.cnt - 1].icstr : 0;
		dnaNEXT(h->chars)->icstr = prev + h->cstrs.cnt;
		h->cstrs.cnt = 0;
	}
	else {
		dnaNEXT(h->chars)->icstr = h->cstrs.cnt;
	}

	if (h->trans != NULL) {
		/* Transitional chars font; perform additional processing */
		int i;

		if (h->pend.trans) {
			/* Convert transitional subrs */
			convTransSubrs(g, nSubrs, subrs);
		}

		/* Check if transitional char */
		for (i = 0; i < h->trans->chars.cnt; i++) {
			TransChar *chr = &h->trans->chars.array[i];
			if (id == chr->sid) {
				/* Found it; save stored description */
				memcpy(dnaEXTEND(h->cstrs, chr->cstr.length),
				       chr->cstr.cstr, chr->cstr.length);
				return;
			}
		}
	}

#if TC_DEBUG
	if (h->debug.input || h->debug.output) {
		if (h->font->flags & FONT_CID) {
			printf("--- glyph[%u]\n", id);
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
	h->width.curr = dnaMAX(h->width.fd, fd);

	cstrRecode(g, length, (unsigned char *)cstr, nSubrs, subrs);

#if TC_DEBUG
	if (h->debug.output) {
		long icstr = h->chars.array[h->chars.cnt - 1].icstr;
		unsigned length = h->cstrs.cnt - icstr;
		printf("==== recode[%d]: ", length);
		csDump(length, (unsigned char *)&h->cstrs.array[icstr], g->nMasters, 0);
	}
#endif /* TC_DEBUG */

#if TC_STATISTICS
	h->font->flatSize += h->cstrs.cnt - h->chars.array[h->chars.cnt - 1].icstr;
#endif /* TC_STATISTICS */
}

/* Get recoded char. Return charstring pointer and length after applying width
   optimizations. If TC_SMALLMEMORY just return length since charstrings are
   saved in temporary file */
static char *recodeGetChar(tcCtx g, unsigned iChar, int fd, unsigned *length) {
	recodeCtx h = g->ctx.recode;
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
		     csEncInteger(this->width - h->width.fd.array[fd].nominal, t));
		return NULL;
	}
	else {
		int bytes;
		char *src = &h->cstrs.array[iSrc] + DUMMY_WIDTH_SIZE;

		if (this->width != h->width.fd.array[fd].dflt) {
			/* Compute and save optimized width */
			bytes =
			    csEncInteger(this->width - h->width.fd.array[fd].nominal, t);
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
	recodeCtx h = g->ctx.recode;
	h->tmpFile.next = g->cb.tmpRefill(g->cb.ctx, &h->tmpFile.left);
	if (h->tmpFile.left-- == 0) {
		parseFatal(g, "premature end of tmp file");
	}
	return *h->tmpFile.next++;
}

/* Write recoded char in TC_SMALLMEMORY mode */
static void recodeWriteChar(tcCtx g, Font *font, unsigned iChar) {
	recodeCtx h = g->ctx.recode;
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
			    csEncInteger(this->width - h->width.fd.array[fd].nominal, t);
			OUTN(bytes, t);
		}
		OUTN(length - DUMMY_WIDTH_SIZE, &buf[DUMMY_WIDTH_SIZE]);
	}
}

/* Return charstring conversion procs */
csConvProcs recodeGetConvProcs(tcCtx g) {
	static csConvProcs procs = {
		recodeNewFont,
		recodeEndFont,
		recodeAddChar,
		recodeGetChar,
		recodeWriteChar,
	};
	return procs;
}

/* Save integer in charstring */
static void cstrSaveInt(CSTR *cstr, long i) {
	char t[5];
	int length = csEncInteger(i, t);
	memcpy(dnaEXTEND(*cstr, length), t, length);
}

/* Save real or integer number in charstring */
static void cstrSaveNumber(CSTR *cstr, double d) {
	char t[5];
	int length = csEncFixed(DBL2FIX(d), t);
	memcpy(dnaEXTEND(*cstr, length), t, length);
}

/* Compile NDV charstring, add it to string table, and return its SID.

   BuildCharArray usage.

   BCA[0-14]	UDV[0-14]	Initially
   BCA[0-14]	NDV[0-14]	Finally
 */
static SID compileNDV(tcCtx g, int nAxes, double *BDM, int clamp) {
	SID sid;
	CSTR cstr;
	int i;
	int j;

	dnaINIT(g->ctx.dnaCtx, cstr, 100, 500);

	/* 2 0 nAxes load */
	cstrSaveInt(&cstr, 2);
	cstrSaveInt(&cstr, 0);
	cstrSaveInt(&cstr, nAxes);
	T2SAVEOP(tx_load, cstr);

	j = 0;
	for (i = 0; i < nAxes; i++) {
		double U0 = BDM[j++];
		double N0 = BDM[j++];

		for (;; ) {
			double U1 = BDM[j++];
			double N1 = BDM[j++];
			int first = N0 == 0.0;
			int last = N1 == 1.0;

			if (clamp && first) {
				/* First axis map; 0 */
				cstrSaveInt(&cstr, 0);
			}

			/* iAxis get U0 sub */
			cstrSaveInt(&cstr, i);
			T2SAVEOP(tx_get, cstr);
			cstrSaveNumber(&cstr, U0);
			T2SAVEOP(tx_sub, cstr);

			if (!first || !last) {
				/* (N1-N0) mul */
				cstrSaveNumber(&cstr, N1 - N0);
				T2SAVEOP(tx_mul, cstr);
			}
			if (U1 - U0 != 1.0) {
				/* (U1-U0) div */
				cstrSaveNumber(&cstr, U1 - U0);
				T2SAVEOP(tx_div, cstr);
			}
			if (!first) {
				/* N0 add */
				cstrSaveNumber(&cstr, N0);
				T2SAVEOP(tx_add, cstr);
			}
			if (clamp || !first) {
				/* iAxis get U0 ifelse */
				cstrSaveInt(&cstr, i);
				T2SAVEOP(tx_get, cstr);
				cstrSaveNumber(&cstr, U0);
				T2SAVEOP(tx_ifelse, cstr);
			}
			if (last) {
				if (clamp) {
					/* 1 iAxis get U1 ifelse */
					cstrSaveInt(&cstr, 1);
					cstrSaveInt(&cstr, i);
					T2SAVEOP(tx_get, cstr);
					cstrSaveNumber(&cstr, U1);
					T2SAVEOP(tx_ifelse, cstr);
				}
				/* iAxis put */
				cstrSaveInt(&cstr, i);
				T2SAVEOP(tx_put, cstr);
				break;
			}
			U0 = U1;
			N0 = N1;
		}
	}

	/* 1 0 0 nAxes store endchar */
	cstrSaveInt(&cstr, 1);
	cstrSaveInt(&cstr, 0);
	cstrSaveInt(&cstr, 0);
	cstrSaveInt(&cstr, nAxes);
	T2SAVEOP(tx_store, cstr);
	T2SAVEOP(tx_endchar, cstr);

	sid = sindexGetId(g, cstr.cnt, cstr.array);
	dnaFREE(cstr);
	return sid;
}

/* Compile CDV charstring, add it to string table, and return its SID. Only
   used when nMasters = 2 ^ nAxes.

   BuildCharArray usage.

   BCA[0-3]		NDV[0-3]
   BCA[1-7]		1-NDV[0],1-NDV[1],1-NDV[2],1-NDV[3]
   BCA[8-23]	WV[0-15]
 */
static SID compileCDV(tcCtx g, int nAxes, int *order) {
	SID sid;
	CSTR cstr;

	dnaINIT(g->ctx.dnaCtx, cstr, 200, 500);

	if (nAxes == 1) {
		/* Optimize 1-axis case */
		static unsigned char CDV1Axis[] = {
#include "cdv1axis.h"
		};
		memcpy(dnaEXTEND(cstr, (int)sizeof(CDV1Axis)),
		       CDV1Axis, sizeof(CDV1Axis));
	}
	else {
		int i;
		int j;

		/* 1 0 nAxes load */
		cstrSaveInt(&cstr, 1);
		cstrSaveInt(&cstr, 0);
		cstrSaveInt(&cstr, nAxes);
		T2SAVEOP(tx_load, cstr);

		for (i = 0; i < nAxes; i++) {
			/* 1 iAxis get sub (4+iAxis) put */
			cstrSaveInt(&cstr, 1);
			cstrSaveInt(&cstr, i);
			T2SAVEOP(tx_get, cstr);
			T2SAVEOP(tx_sub, cstr);
			cstrSaveInt(&cstr, 4 + i);
			T2SAVEOP(tx_put, cstr);
		}

		/* 1 */
		cstrSaveInt(&cstr, 1);

		for (i = 1; i < g->nMasters; i++) {
			for (j = 0; j < nAxes; j++) {
				if (order[i] & 1 << j) {
					/* iAxis */
					cstrSaveInt(&cstr, j);
				}
				else {
					/* (4+iAxis) */
					cstrSaveInt(&cstr, 4 + j);
				}
				T2SAVEOP(tx_get, cstr);

				if (j != 0) {
					/* mul */
					T2SAVEOP(tx_mul, cstr);
				}
			}

			/* dup (8+iMaster) put sub */
			T2SAVEOP(tx_dup, cstr);
			cstrSaveInt(&cstr, 8 + i);
			T2SAVEOP(tx_put, cstr);
			T2SAVEOP(tx_sub, cstr);
		}

		/* 8 put 0 0 8 nMasters store endchar */
		cstrSaveInt(&cstr, 8);
		T2SAVEOP(tx_put, cstr);
		cstrSaveInt(&cstr, 0);
		cstrSaveInt(&cstr, 0);
		cstrSaveInt(&cstr, 8);
		cstrSaveInt(&cstr, g->nMasters);
		T2SAVEOP(tx_store, cstr);
		T2SAVEOP(tx_endchar, cstr);
	}

	sid = sindexGetId(g, cstr.cnt, cstr.array);
	dnaFREE(cstr);
	return sid;
}

/* Remove single layer of encryption */
void recodeDecrypt(unsigned length, unsigned char *cstr) {
	unsigned i;
	unsigned short r1 = 4330;   /* Decryption key */

	/* Remove encryption */
	for (i = 0; i < length; i++) {
		unsigned char plain = cstr[i] ^ (r1 >> 8);
		r1 = (cstr[i] + r1) * 52845 + 22719;
		cstr[i] = plain;
	}
}

/* Convert NDV/CDV subr, add it to string table, and return its SID */
static SID convDVSubr(tcCtx g, ConvSubr *subr, int lenIV) {
	recodeCtx h = g->ctx.recode;
	SID sid;
	long icstr = h->cstrs.cnt;  /* Save charstring index */
	unsigned length = subr->data.length;
	unsigned char *cstr = (unsigned char *)subr->data.cstr;

	if (lenIV != -1) {
		/* Decrypt subr */
		recodeDecrypt(length, cstr);
		cstr += lenIV;
		length -= lenIV;
	}

	h->idType = SubrType;
	h->id = subr->iSubr;
	h->nMasters = g->nMasters;
	h->stack.max = T2_MAX_OP_STACK;

	cstrRecode(g, length, cstr, 0, NULL);

	sid = sindexGetId(g, h->cstrs.cnt - icstr, &h->cstrs.array[icstr]);
	h->cstrs.cnt = icstr;   /* Reset charstring index */

	return sid;
}

/* Match conversion subr records */
static int CDECL matchRecs(const void *key, const void *value) {
	return strcmp(*(char **)key, ((ConvRecord *)value)->FontName);
}

/* Save vector conversion and transitional design subrs */
void recodeSaveConvSubrs(tcCtx g, Font *font, int nAxes, double *BDM,
                         int *order, int lenIV, ConvSubr *NDV, ConvSubr *CDV) {
	recodeCtx h = g->ctx.recode;

	/* --- JensonMM CDV --- */
	static unsigned char JensonCDV_cstr[] = {
#include "jenson.h"
	};
	static Charstring JensonCDV = {
		sizeof(JensonCDV_cstr), (char *)JensonCDV_cstr
	};

	/* --- KeplerMM NDV --- */
	static unsigned char KeplerNDV_cstr[] = {
#include "kepler.h"
	};
	static Charstring KeplerNDV = {
		sizeof(KeplerNDV_cstr), (char *)KeplerNDV_cstr
	};

	/* --- JimboMM transitional chars --- */
	static unsigned char Jimbo0_cstr[] = {
#include "jimbo0.h"
	};
	static unsigned char Jimbo1_cstr[] = {
#include "jimbo1.h"
	};
	static unsigned char Jimbo2_cstr[] = {
#include "jimbo2.h"
	};
	static TransChar Jimbo_chars[] = {
		{ 50, { sizeof(Jimbo0_cstr), (char *)Jimbo0_cstr }
		},
		{ 56, { sizeof(Jimbo1_cstr), (char *)Jimbo1_cstr }
		},
		{ 88, { sizeof(Jimbo2_cstr), (char *)Jimbo2_cstr }
		},
	};
	static short Jimbo_subrs[] = {
		539, 540, 542, 538, 537, 541
	};
	static TransFont Jimbo = {
		{ TABLE_LEN(Jimbo_subrs), Jimbo_subrs },
		{ TABLE_LEN(Jimbo_chars), Jimbo_chars },
	};

	/* --- ITC Garamond (Roman) transitional chars --- */
	static unsigned char GaraRm0_cstr[] = {
#include "rgara0.h"
	};
	static unsigned char GaraRm1_cstr[] = {
#include "rgara1.h"
	};
	static unsigned char GaraRm2_cstr[] = {
#include "rgara2.h"
	};
	static unsigned char GaraRm3_cstr[] = {
#include "rgara3.h"
	};
	static TransChar GaraRm_chars[] = {
		{ 56, { sizeof(GaraRm0_cstr), (char *)GaraRm0_cstr }
		},
		{ 97, { sizeof(GaraRm1_cstr), (char *)GaraRm1_cstr }
		},
		{ 5, { sizeof(GaraRm2_cstr), (char *)GaraRm2_cstr }
		},
		{ 88, { sizeof(GaraRm3_cstr), (char *)GaraRm3_cstr }
		},
	};
	static short GaraRm_subrs[] = {
		343, 350, 349, 347, 346, 342, 344, 345, 348
	};
	static TransFont GaraRm = {
		{ TABLE_LEN(GaraRm_subrs), GaraRm_subrs },
		{ TABLE_LEN(GaraRm_chars), GaraRm_chars },
	};

	/* --- ITC Garamond (Italic) transitional chars --- */
	static unsigned char GaraIt0_cstr[] = {
#include "igara0.h"
	};
	static unsigned char GaraIt1_cstr[] = {
#include "igara1.h"
	};
	static unsigned char GaraIt2_cstr[] = {
#include "igara2.h"
	};
	static unsigned char GaraIt3_cstr[] = {
#include "igara3.h"
	};
	static unsigned char GaraIt4_cstr[] = {
#include "igara4.h"
	};
	static TransChar GaraIt_chars[] = {
		{ 56, { sizeof(GaraIt0_cstr), (char *)GaraIt0_cstr }
		},
		{ 97, { sizeof(GaraIt1_cstr), (char *)GaraIt1_cstr }
		},
		{  5, { sizeof(GaraIt2_cstr), (char *)GaraIt2_cstr }
		},
		{ 32, { sizeof(GaraIt3_cstr), (char *)GaraIt3_cstr }
		},
		{ 123, { sizeof(GaraIt4_cstr), (char *)GaraIt4_cstr }
		},
	};
	static short GaraIt_subrs[] = {
		448, 449, 456, 458, 457, 454, 452, 450, 451, 455, 453
	};
	static TransFont GaraIt = {
		{ TABLE_LEN(GaraIt_subrs), GaraIt_subrs },
		{ TABLE_LEN(GaraIt_chars), GaraIt_chars },
	};

	/* --- Special conversion font list --- */
	static ConvRecord fontList[] = {
		{ "AJensonMM",       NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-Alt",   NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-Ep",    NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-It",    NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-ItAlt", NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-ItEp",  NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-ItSC",  NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-SC",    NULL,       &JensonCDV, NULL,    0 },
		{ "AJensonMM-Sw",    NULL,       &JensonCDV, NULL,    0 },
		{ "AdobeSansMM",     NULL,       NULL,       NULL,    1 },
		{ "AdobeSansXMM",    NULL,       NULL,       NULL,    1 },
		{ "AdobeSerifMM",    NULL,       NULL,       NULL,    1 },
		{ "ITCGaramondMM",   NULL,       NULL,       &GaraRm, 0 },
		{ "ITCGaramondMM-It", NULL,       NULL,       &GaraIt, 0 },
		{ "JimboMM",         NULL,       NULL,       &Jimbo,  0 },
		{ "KeplMM",          &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-Ep",       &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-It",       &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-ItEp",     &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-ItSC",     &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-Or1",      NULL,       NULL,       NULL,    0 }, /* Remake */
		{ "KeplMM-Or2",      NULL,       NULL,       NULL,    0 }, /* Remake */
		{ "KeplMM-Or3",      NULL,       NULL,       NULL,    0 }, /* Remake */
		{ "KeplMM-SC",       &KeplerNDV, NULL,       NULL,    0 },
		{ "KeplMM-Sw",       &KeplerNDV, NULL,       NULL,    0 },
	};
	ConvRecord *rec = (ConvRecord *)bsearch(&font->FontName, fontList,
	                                        TABLE_LEN(fontList),
	                                        sizeof(ConvRecord), matchRecs);
	/* Save NDV */
	if (rec != NULL) {
		if (rec->NDV != NULL) {
			NDV->sid = sindexGetId(g, rec->NDV->length, rec->NDV->cstr);
		}
		else {
			NDV->sid = compileNDV(g, nAxes, BDM, !rec->subs);
		}
	}
	else if (NDV->data.length != 0) {
		NDV->sid = convDVSubr(g, NDV, lenIV);
	}
	else {
		NDV->sid = compileNDV(g, nAxes, BDM, 1);
	}

	/* Save CDV */
	if (rec != NULL) {
		if (rec->CDV != NULL) {
			CDV->sid = sindexGetId(g, rec->CDV->length, rec->CDV->cstr);
		}
		else {
			CDV->sid = compileCDV(g, nAxes, order);
		}
	}
	else if (NDV->data.length != 0) {
		CDV->sid = convDVSubr(g, CDV, lenIV);
	}
	else {
		CDV->sid = compileCDV(g, nAxes, order);
	}

	if (rec != NULL && rec->trans != NULL) {
		/* Save transitional font record */
		h->trans = rec->trans;
		h->pend.trans = 1;

#if TC_SUBR_SUPPORT
		if (g->flags & TC_SUBRIZE) {
			parseWarning(g, "transitional font; disabling subroutinizer");
			g->flags &= ~TC_SUBRIZE;
		}
#endif /* TC_SUBR_SUPPORT */
	}
}

#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

/* Check point against current bounds. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void newGlyphCheckPoint(recodeCtx h, Fixed x, Fixed y, TemplateGlyphData *templateGlyph) {
	if (x < templateGlyph->bbox.left[h->newGlyph.iMaster]) {
		templateGlyph->bbox.left[h->newGlyph.iMaster] = x;
	}
	else if (x > templateGlyph->bbox.right[h->newGlyph.iMaster]) {
		templateGlyph->bbox.right[h->newGlyph.iMaster] = x;
	}
	if (y < templateGlyph->bbox.bottom[h->newGlyph.iMaster]) {
		templateGlyph->bbox.bottom[h->newGlyph.iMaster] = y;
	}
	else if (y > templateGlyph->bbox.top[h->newGlyph.iMaster]) {
		templateGlyph->bbox.top[h->newGlyph.iMaster] = y;
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
   the regions of the curve close to maxima or minima. In particular this
   stopping condition is much more efficient that subdivision for drawing
   curves where subdivision density is fairly evenly spread.

   One further optimization can be made by observing that only one of the end
   points needs testing after the splitting terminates because the other end
   point will be tested in the other subdivided curve. This is true except for
   one end of the first curve which must be tested outside of this function.
 */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void newGlyphFlatten(recodeCtx h,
                            Fixed x0, Fixed y0, Fixed x1, Fixed y1,
                            Fixed x2, Fixed y2, Fixed x3, Fixed y3, TemplateGlyphData *templateGlyph) {
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
			newGlyphFlatten(h, x0, y0, ux, uy, (tx + ux) / 2, (ty + uy) / 2, cx, cy, templateGlyph);

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
			newGlyphCheckPoint(h, x3, y3, templateGlyph);
			return;
		}
	}
}

/* Add curve to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void newGlyphAddCurve(recodeCtx h, double c,
                             long index, Fixed *x0, Fixed *y0, TemplateGlyphData *templateGlyph) {
	Fixed y1;
	Fixed x1;
	Fixed y2;
	Fixed x2;
	Fixed y3;
	Fixed x3;
	Fixed *coord = &h->path.args.array[index];

	/* Fetch coords for one master */
	x1 = *coord;
	coord += h->nMasters;
	y1 = *coord;
	coord += h->nMasters;
	x2 = *coord;
	coord += h->nMasters;
	y2 = *coord;
	coord += h->nMasters;
	x3 = *coord;
	coord += h->nMasters;
	y3 = *coord;

	/* Add shear correction */
	x1 += DBL2FIX(c * FIX2DBL(y1));
	x2 += DBL2FIX(c * FIX2DBL(y2));
	x3 += DBL2FIX(c * FIX2DBL(y3));

	if (x1 < templateGlyph->bbox.left[h->newGlyph.iMaster] ||
	    x1 > templateGlyph->bbox.right[h->newGlyph.iMaster]  ||
	    x2 <templateGlyph->bbox.left[h->newGlyph.iMaster] ||
	        x2> templateGlyph->bbox.right[h->newGlyph.iMaster]  ||
	    x3 <templateGlyph->bbox.left[h->newGlyph.iMaster] ||
	        x3> templateGlyph->bbox.right[h->newGlyph.iMaster]  ||
	    y1 <templateGlyph->bbox.bottom[h->newGlyph.iMaster] ||
	        y1> templateGlyph->bbox.top[h->newGlyph.iMaster] ||
	    y2 <templateGlyph->bbox.bottom[h->newGlyph.iMaster] ||
	        y2> templateGlyph->bbox.top[h->newGlyph.iMaster] ||
	    y3 <templateGlyph->bbox.bottom[h->newGlyph.iMaster] ||
	        y3> templateGlyph->bbox.top[h->newGlyph.iMaster]) {
		/* Bounding trapezium extends outside bounding box; flatten curve */
		newGlyphFlatten(h, *x0, *y0, x1, y1, x2, y2, x3, y3, templateGlyph);
	}

	*x0 = x3;
	*y0 = y3;
}

/* Add point to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void newGlyphAddPoint(recodeCtx h,
                             double c, long index, Fixed *x0, Fixed *y0, TemplateGlyphData *templateGlyph) {
	Fixed *coord = &h->path.args.array[index];

	/* Fetch coords for one master */
	*x0 = *coord;
	coord += h->nMasters;
	*y0 = *coord;

	/* Add shear correction */
	*x0 += DBL2FIX(c * FIX2DBL(*y0));

	newGlyphCheckPoint(h, *x0, *y0, templateGlyph);
}

/* Determine height of current character by traversing its path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/* The template glyph is the glyph in the target font whose metrics are used to determine the font matrix
    and UDV to be applied to the fill-in font glyphs, when those are added to the target font.

    The code allows for the target font  to be an MM font.

    We currently are using the smae template glyph name with both the EuroMM and the FillinMM fonts;
    this code is here because David Lemon originally wanted a different template glyph for the FillinMM font.
 */
static void saveTemplateGlyphData(recodeCtx h, TemplateGlyphData *templateGlyph) {
	int i;
	int iSeg;
	Operator *op;
	double ItalicAngle = parseGetItalicAngle(h->g);
	double c = tan(ItalicAngle * DEG_2_RAD);
	Fixed x0 = 0;
	Fixed y0 = 0;

	for (h->newGlyph.iMaster = 0; h->newGlyph.iMaster < h->nMasters; h->newGlyph.iMaster++) {
		int iMaster = h->newGlyph.iMaster;
		templateGlyph->bbox.left[iMaster] = LONG_MAX;
		templateGlyph->bbox.bottom[iMaster] = 0;
		templateGlyph->bbox.right[iMaster] = 0;
		templateGlyph->bbox.top[iMaster] = 0;
		templateGlyph->hstem[iMaster] = 0;
		templateGlyph->vstem[iMaster] = 0;

		iSeg = 0;
		op = h->path.ops.array;
		for (;; ) {
			if (iSeg < op->iSeg) {
				/* Add next segment */
				Segment *seg = &h->path.segs.array[iSeg++];

				if ((seg->flags & SEG_TYPE_MASK) == seg_curve) {
					newGlyphAddCurve(h, c, seg->iArgs + iMaster, &x0, &y0, templateGlyph);
				}
				else {
					newGlyphAddPoint(h, c, seg->iArgs + iMaster, &x0, &y0, templateGlyph);
				}
			}
			else {
				/* Add next operator */
				if (op->type == tx_endchar) {
					break;
				}
				else if (op->type == t2_flex) {
					Segment *seg = &h->path.segs.array[iSeg];
					newGlyphAddCurve(h, c, seg[0].iArgs + iMaster,
					                 &x0, &y0, templateGlyph);
					newGlyphAddCurve(h, c, seg[1].iArgs + iMaster,
					                 &x0, &y0, templateGlyph);
					iSeg += 2;  /* Skip flex curves */
				}
				op++;
			}
		}
	}

	/* Fetch first hstem and vstem */
	for (i = 0; i < h->stem.list.cnt; i++) {
		Stem *stem = &h->stem.list.array[i];
		if (stem->delta[0] < 0) {
			/* Ignore edge hints or bad deltas */
		}
		else if (stem->type & STEM_VERT) {
			/* vstem */
			if (ItalicAngle == 0.0) {
				copyBlend(templateGlyph->vstem, stem->delta);
			}
			else {
				/* Compute italic optical adjustment to stem weight */
				double adjust = fabs(cos(ItalicAngle * DEG_2_RAD));
				int j;

				/* Handle stupid values of ItalicAngle */
				if (adjust < 0.5) {
					adjust = 0.5;
				}

				for (j = 0; j < h->nMasters; j++) {
					templateGlyph->vstem[j] =
					    RND2FIX(FIX2DBL(stem->delta[j]) / adjust);
				}
			}
			break;
		}
		else if (templateGlyph->hstem[0] == 0) {
			/* hstem; copy first one */
			copyBlend(templateGlyph->hstem, stem->delta);
		}
	}
}

/* [cff callback] Fatal error handler. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroFatal(void *ctx) {
	recodeCtx h = ctx;
	parseFatal(h->g, "euro addition failed");
}

/* [cff callback] Allocate memory. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void *euroMalloc(void *ctx, size_t size) {
	recodeCtx h = ctx;
	return MEM_NEW(h->g, size);
}

/* [cff callback] Free memory. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroFree(void *ctx, void *ptr) {
	recodeCtx h = ctx;
	MEM_FREE(h->g, ptr);
}

/* [cff callback] Seek to offset and return data. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static char *euroSeek(void *ctx, long offset, long *count) {
	recodeCtx h = ctx;
	if (offset >= h->newGlyph.cff.length || offset < 0) {
		/* Seek outside data bounds */
		*count = 0;
		return NULL;
	}
	else {
		*count = h->newGlyph.cff.length - offset;
		return &h->newGlyph.cff.data[offset];
	}
}

/* [cff callback] Refill data buffer from current postion. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static char *euroRefill(void *ctx, long *count) {
	/* Never called in this implementation since all data in memory */
	*count = 0;
	return NULL;
}

/* Add x-coordinate to list. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroAddCoord(recodeCtx h, Fixed x, Fixed y) {
	double yd;
	double xd;
	double txd;
	double dx_dy = 0.0;

	Fixed *coord;

	if (h->italic_angle == 0.0) {
		txd = 0.0;
	}
	else {
		if (h->noslant) {
			txd = h->newGlyph.matrix.noslant_glyph_offset;
		}
		else {
			txd = h->newGlyph.matrix.tx;
			dx_dy = h->newGlyph.matrix.c;
		}
	}

	yd = floor(FIX2DBL(y) * h->newGlyph.matrix.d + h->newGlyph.matrix.ty);
	xd = floor(FIX2DBL(x) * h->newGlyph.matrix.d + dx_dy * yd + txd);


	coord =
	    (h->newGlyph.iMaster == 0) ? dnaEXTEND(h->path.args, h->nMasters * 2) :
	    &h->path.args.array[h->newGlyph.iCoord * h->nMasters + h->newGlyph.iMaster];

	/*fprintf(stderr, "xd: %g x*d: %g c: %g tx: %g\n", xd,  FIX2DBL(x) * h->newGlyph.matrix.d,
	                h->newGlyph.matrix.c, h->newGlyph.matrix.tx);*/
	/* record right-most position, e.g right side of bbox. Needed for adjusting position of non-slanted glyphs. */
	if (h->newGlyph.max_x < xd) {
		h->newGlyph.max_x = xd;
	}

	if (h->newGlyph.max_y < yd) {
		h->newGlyph.max_y = yd;
	}


	*coord = RND2FIX(xd);
	coord += h->nMasters;
	*coord = RND2FIX(yd);
	h->newGlyph.iCoord += 2;
}

/* Add moveto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroMoveto(void *ctx, cffFixed x1, cffFixed y1) {
	recodeCtx h = ctx;
	if (h->newGlyph.iMaster == 0) {
		addPathSeg(h, seg_move);
	}
	euroAddCoord(h, x1, y1);
}

/* Add lineto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroLineto(void *ctx, cffFixed x1, cffFixed y1) {
	recodeCtx h = ctx;
	if (h->newGlyph.iMaster == 0) {
		addPathSeg(h, seg_line);
	}
	euroAddCoord(h, x1, y1);
}

/* Add curveto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroCurveto(void *ctx, int flex,
                        cffFixed x1, cffFixed y1,
                        cffFixed x2, cffFixed y2,
                        cffFixed x3, cffFixed y3) {
	recodeCtx h = ctx;
	if (h->newGlyph.iMaster == 0) {
		addPathSeg(h, seg_curve);
	}
	euroAddCoord(h, x1, y1);
	euroAddCoord(h, x2, y2);
	euroAddCoord(h, x3, y3);
}

/* Add end to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroEndchar(void *ctx) {
	recodeCtx h = ctx;
	if (h->newGlyph.iMaster == 0) {
		addPathOp(h, 0, 0, tx_endchar);
	}
}

/* Add stem hint to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void euroHintstem(void *ctx, int vert, cffFixed edge0, cffFixed edge1) {
	recodeCtx h = ctx;
	Stem *stem;

	if (vert && h->italic_angle != 0.0) {
		return; /* Ignore vstems for italic fonts */
	}
	stem = (h->newGlyph.iMaster == 0) ?
	    dnaNEXT(h->stem.list) : &h->stem.list.array[h->newGlyph.iStem++];
	stem->id = (unsigned char)(h->stem.list.cnt - 1);

	if (vert) {
		double txd;
		Fixed tx;
		Fixed diff;

		if (h->italic_angle == 0.0) {
			tx = 0;
		}
		else {
			if (h->noslant) {
				txd = h->newGlyph.matrix.noslant_glyph_offset;
			}
			else {
				txd = h->newGlyph.matrix.tx;
			}
			tx = DBL2FIX(txd);
		}

		diff = edge1 - edge0; /* We do NOT scale horizontal edge hints, which have a diff of  -20 */
		if (diff ==  INT2FIX(-20)) {
			edge0 = RNDFIX(DBL2FIX(FIX2DBL(edge0) * h->newGlyph.matrix.d) + tx);
			edge1 = edge0 + INT2FIX(-20);
		}
		else {
			edge0 =  RNDFIX(DBL2FIX(FIX2DBL(edge0) * h->newGlyph.matrix.d) + tx);
			edge1 =  RNDFIX(DBL2FIX(FIX2DBL(edge1) * h->newGlyph.matrix.d) + tx);
		}

		stem->type = STEM_VERT;
		stem->edge[h->newGlyph.iMaster] = edge0;
		stem->delta[h->newGlyph.iMaster] = edge1 - edge0;
	}
	else {
		Fixed diff = edge1 - edge0; /* We do NOT scale horizontal edge hints, which have a diff of  -21 */
		if (diff == INT2FIX(-21)) {
			edge0 = RND2FIX(FIX2DBL(edge0) * h->newGlyph.matrix.d + h->newGlyph.matrix.ty);
			edge1 = edge0 + INT2FIX(-21);
		}
		else {
			edge0 = RND2FIX(FIX2DBL(edge0) * h->newGlyph.matrix.d + h->newGlyph.matrix.ty);
			edge1 = RND2FIX(FIX2DBL(edge1) * h->newGlyph.matrix.d + h->newGlyph.matrix.ty);
		}
		stem->type = 0;
		stem->edge[h->newGlyph.iMaster] = edge0;
		stem->delta[h->newGlyph.iMaster] = edge1 - edge0;
	}
}

/* Run through path just to get metrics, Don't record anything!
 */

/* Add x-coordinate to list. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsAddCoord(recodeCtx h, Fixed x, Fixed y) {
	double yd;
	double xd;
	double txd;
	double dx_dy = 0.0;

	if (h->italic_angle == 0.0) {
		txd = 0.0;
	}
	else {
		if (h->noslant) {
			txd = h->newGlyph.matrix.noslant_glyph_offset;
		}
		else {
			txd = h->newGlyph.matrix.tx;
			dx_dy = h->newGlyph.matrix.c;
		}
	}

	yd = floor(FIX2DBL(y) * h->newGlyph.matrix.d + h->newGlyph.matrix.ty);
	xd = floor(FIX2DBL(x) * h->newGlyph.matrix.d + dx_dy * yd + txd);


	/*fprintf(stderr, "xd: %g x*d: %g c: %g tx: %g\n", xd,  FIX2DBL(x) * h->newGlyph.matrix.d,
	                    h->newGlyph.matrix.c, h->newGlyph.matrix.tx);*/
	/* record right-most position, e.g right side of bbox. Needed for adjusting position of non-slanted glyphs. */
	if (h->newGlyph.max_x < xd) {
		h->newGlyph.max_x = xd;
	}

	if (h->newGlyph.max_y < yd) {
		h->newGlyph.max_y = yd;
	}
}

/* Add moveto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsMoveto(void *ctx, cffFixed x1, cffFixed y1) {
	recodeCtx h = ctx;
	metricsAddCoord(h, x1, y1);
}

/* Add lineto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsLineto(void *ctx, cffFixed x1, cffFixed y1) {
	recodeCtx h = ctx;
	metricsAddCoord(h, x1, y1);
}

/* Add curveto to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsCurveto(void *ctx, int flex,
                           cffFixed x1, cffFixed y1,
                           cffFixed x2, cffFixed y2,
                           cffFixed x3, cffFixed y3) {
	recodeCtx h = ctx;
	metricsAddCoord(h, x1, y1);
	metricsAddCoord(h, x2, y2);
	metricsAddCoord(h, x3, y3);
}

/* Add end to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsEndchar(void *ctx) {
}

/* Add stem hint to path. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static void metricsHintstem(void *ctx, int vert, cffFixed edge0, cffFixed edge1) {
	/* I have no interest in stems when getting glyph bbox */
}

/* Add euro glyph with attributes that match arguments. Matching is achieved by
   taking an instance from a specially developed multiple master euro font that
   has a sans and a serif euro glyph with weight, width, and italic axes. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

/* Add an Adobe PostScript Std glyph with attributes that match arguments.
    Matching is achieved by
   taking an instance from a specially developed multiple master  font that
   has a sans and a serif version of the glyph with weight, width, and italic axes. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

static int FillinMM_font_clampUDV(recodeCtx h, int is_serif, cffFixed *UDV);

/*
   We need to calculate a UDV and a font transform, to be applied to the new glyph copied from the fill-in MM font, for
   each master design of the target font.

   The width and weight metrics to match are taken from the "templateGlyph" in the target font. For both the EuroMM and the FillinMM font,
   teh template glyph is "zero". The font transform is calculated by comparing the height of the font BBOX between the
   target font's template glyph, and a 'scaling' glyph in the fill-in MM font. For the EuroMM, the scaling glyph is the "Euro"; for the
   FillinMM font, the scaling glyph is "zero".
 */
void InitStaticFontData(tcCtx g, int font__serif_selector, double *StdVW, double *fontMatrix, unsigned isFixedPitch) {
	recodeCtx h = g->ctx.recode;
	int is_serif;

	static unsigned char eurosans[] = {
		#include "eurosans.h"
	};
	static unsigned char euroserf[] = {
		#include "euroserf.h"
	};

	static unsigned char fillinmm_sans[] = {
		#include "fillinmm_sans.h"
	};
	static unsigned char fillinmm_serif[] = {
		#include "fillinmm_serif.h"
	};

	/*
	   Used to build the synthetic glyph under newGlyphPath.
	 */
	static cffPathCallbacks pathcb = {
		NULL,
		euroMoveto,
		euroLineto,
		euroCurveto,
		NULL,
		euroEndchar,
		euroHintstem,
		NULL,           /* Hintmasks unsupported */
	};

	/* used to get some metrics WITHOUT building the newGlyphPath
	   NOTE! Must be kept parallel to pathcb, with regard to metric calculations.
	 */
	static cffPathCallbacks metricsPathcb = {
		NULL,
		metricsMoveto,
		metricsLineto,
		metricsCurveto,
		NULL,
		metricsEndchar,
		metricsHintstem,
		NULL,           /* Hintmasks unsupported */
	};


	static cffStdCallbacks stdcb;
	int font_index;

	h->pathcb = &pathcb;
	h->metricsPathcb = &metricsPathcb;

	stdcb.ctx       = h;
	stdcb.fatal     = euroFatal;
	stdcb.message   = NULL;
	stdcb.free      = euroFree;
	stdcb.malloc    = euroMalloc;
	stdcb.cffSeek   = euroSeek,
	stdcb.cffRefill = euroRefill;
	h->stdcb = &stdcb;

	h->italic_angle = parseGetItalicAngle(h->g);
	h->noslant = 0;

	h->isFixedPitch = isFixedPitch;

	if (fontMatrix[2] != 0.0) {
		/* Avoid obliquing the euro in an oblique font twice */
		h->italic_angle = 0.0;
	}

	/*
	   if font__serif_selector is 0, use heuristics, elese respect override.
	 */

	if (font__serif_selector == 1) {
		is_serif = 1;
	}
	else if (font__serif_selector == 2) {
		is_serif = 0;
	}
	else {
		if (isFixedPitch) {
			is_serif = 0;
		}
		else if (h->newGlyph.tmlength == 0) {
			is_serif = 1;
		}
		else {
			is_serif = (h->newGlyph.tmlength / g->nMasters) >= 118;
		}
		parseWarning(h->g, "Falling back on heuristic to choose new glyphs serif-ness: is_serif ==  %i", is_serif);
	}

	if (is_serif) {
		h->newGlyph.fill_in_cff[kEuroMMIndex].data = (char *)euroserf;
		h->newGlyph.fill_in_cff[kEuroMMIndex].length = sizeof(euroserf);
		h->newGlyph.fill_in_cff[kEuroMMIndex].scaling_gid = 2; /* euro for EuroMM */

		h->newGlyph.fill_in_cff[kFillinMMIndex].data = (char *)fillinmm_serif;
		h->newGlyph.fill_in_cff[kFillinMMIndex].length = sizeof(fillinmm_serif);
		h->newGlyph.fill_in_cff[kFillinMMIndex].scaling_gid = 4; /* zero for FillinMM */
	}
	else {
		h->newGlyph.fill_in_cff[kEuroMMIndex].data = (char *)eurosans;
		h->newGlyph.fill_in_cff[kEuroMMIndex].length = sizeof(eurosans);
		h->newGlyph.fill_in_cff[kEuroMMIndex].scaling_gid = 2; /* euro for EuroMM */

		h->newGlyph.fill_in_cff[kFillinMMIndex].data = (char *)fillinmm_sans;
		h->newGlyph.fill_in_cff[kFillinMMIndex].length = sizeof(fillinmm_sans);
		h->newGlyph.fill_in_cff[kFillinMMIndex].scaling_gid = 4; /* zero for FillinMM */
	}

	/*
	   For each fill-in font, there is a glyph whose top and bottom is used to scale
	   the fill-in font, by scaling this glyph's top/bottom to the target font's template glyph
	   top bottom.

	   After we get the top/bottom values for the fill-in MMfont scaling glyph,
	   then calculate the font transform and UDV for each target font master face.
	 */

	/* Initialize for new glyph addition. Affects the pathcb callbacks.  */

	for (font_index = 0; font_index < kNumFillinFonts; font_index++) {
		double scaling_gid_bottom;
		double scaling_gid_top;

		cffCtx ctx;
		cffGlyphInfo *gi;

		/* Get the scaling glyph's top and bottom from the fill-in font */
		ctx = cffNew(&stdcb, 0);

		/* Traverse glyph */
		h->newGlyph.matrix.c = 0; /* used in h->pathcb funtions*/
		h->newGlyph.matrix.d = 1.0;
		h->newGlyph.matrix.tx = 0.0;
		h->newGlyph.matrix.ty = 0.0;
		h->newGlyph.matrix.figure_height_offset = 0.0;
		h->newGlyph.matrix.noslant_glyph_offset = 0.0;


		h->idType           = SIDType;
		h->width.curr       = dnaMAX(h->width.fd, 0);
		h->path.segs.cnt    = 0;
		h->path.ops.cnt     = 0;
		h->path.args.cnt    = 0;
		h->path.idmask.cnt  = 0;
		h->hint.cntr        = 0;
		h->hint.initial     = 1;
		h->stem.list.cnt    = 0;
		h->cntr.list.cnt    = 0;
		setBlend(h, h->path.lsb.x, 0);

		h->newGlyph.iStem = 0; /* used in h->pathcb */
		h->newGlyph.iCoord = 0;

		h->newGlyph.cff.data = h->newGlyph.fill_in_cff[font_index].data;
		h->newGlyph.cff.length = h->newGlyph.fill_in_cff[font_index].length;

		/* doesn't matter what UDV I use for the fill-in MM fonts, as the top/bottom
		   are the same for all masters, in all the fill-in fonts. */
		gi = cffGetGlyphInfo(ctx, h->newGlyph.fill_in_cff[font_index].scaling_gid, h->metricsPathcb);

		cffFree(ctx);


		scaling_gid_bottom = gi->bbox.bottom;
		scaling_gid_top = gi->bbox.top;


		/*
		   Now get the font transform matrix and the UDV to be used with this master face of the target font,
		   for the new glyph from the fill-in MM font.
		   Note that we are stepping throught the master faces of the target font, not the fill-in MM Font.
		 */
		for (h->newGlyph.iMaster = 0; h->newGlyph.iMaster < h->nMasters; h->newGlyph.iMaster++) {
			if (h->newGlyph.templateGlyph[font_index].seen == 0) {
				/* The only time we should be here without having seen the template glyph,
				   and filled in the data,
				   is when we are forcing a nordef ona font without a "zero"
				 */
				if (g->flags & TC_FORCE_NOTDEF) {
					/*
					   Because the notdef is fixed in size in all the FillinMM base designs,
					   we can hardwire all the generic info.
					 */
					Fixed *UDV = &(h->newGlyph.fill_in_cff[font_index].UDV[0][h->newGlyph.iMaster]);
					UDV[0] = DBL2FIX(34.0);     /* a low but valid wieght */
					UDV[1] = DBL2FIX(226);      /* min width of MM design.*/
					h->newGlyph.matrix.c = 0;
					h->newGlyph.matrix.d = 1.0;
					h->newGlyph.matrix.tx = 0.0;
					h->newGlyph.matrix.ty = 0.0;
					h->newGlyph.matrix.figure_height_offset = 0.0;
					h->newGlyph.matrix.noslant_glyph_offset = 0.0;
					h->newGlyph.fill_in_cff[font_index].matrix[h->newGlyph.iMaster] = h->newGlyph.matrix;
				}
				else {
					parseFatal(g, " We are trying to add a synthetic glyph when template glyph is not in target font.");
				}
			}
			else {
				int iMaster = h->newGlyph.iMaster;
				int isUDVOverride = 0;

				Fixed *UDV = &(h->newGlyph.fill_in_cff[font_index].UDV[0][h->newGlyph.iMaster]);

				Fixed width     = h->newGlyph.templateGlyph[font_index].width[iMaster];
				Fixed left      = h->newGlyph.templateGlyph[font_index].bbox.left[iMaster];
				Fixed right     = h->newGlyph.templateGlyph[font_index].bbox.right[iMaster];
				Fixed bottom    = h->newGlyph.templateGlyph[font_index].bbox.bottom[iMaster];
				Fixed top       = h->newGlyph.templateGlyph[font_index].bbox.top[iMaster];
				Fixed hstem     = h->newGlyph.templateGlyph[font_index].hstem[iMaster];
				Fixed vstem     = h->newGlyph.templateGlyph[font_index].vstem[iMaster];

				/*
				   Now use the scaling glyph's top/bottom to get the font transform to apply when
				   copying glyphs from the fill-in font to the target font.
				 */
				h->newGlyph.matrix.c = tan(-(h->italic_angle) * DEG_2_RAD);
				/* ASP: This is the point where the matrix gets initialized */
				/*h->newGlyph.matrix.c = 0;*/
				h->newGlyph.matrix.d = FIX2DBL(top - bottom) / (scaling_gid_top - scaling_gid_bottom);
				h->newGlyph.matrix.ty = FIX2DBL(bottom) - scaling_gid_bottom * h->newGlyph.matrix.d;
				if (h->italic_angle == 0.0) {
					h->newGlyph.matrix.tx = 0.0;
					h->newGlyph.matrix.figure_height_offset = 0;
				}
				else {
					h->newGlyph.matrix.figure_height_offset = h->newGlyph.matrix.c * FIX2DBL(top); /* 'top' should be figure height. Since the template is '0', I hope this is close enough. */
					h->newGlyph.matrix.tx = FIX2DBL((left - (width - right)) / 2);
				}

				h->newGlyph.fill_in_cff[font_index].matrix[iMaster] = h->newGlyph.matrix;

				/* Determine weight value for UDV[0] */
				UDV[0] = INT2FIX(tcGetWeightOverride(g));
				if (UDV[0] == 0) {
					if (vstem != 0) {
						if (hstem != 0) {
							if (is_serif) {
								/* Serif design; use largest of vstem and hstem */
								UDV[0] = (vstem > hstem) ? vstem : hstem;
							}
							else {
								/* Sans design; use (v*v+h*h)/(v+h) */
								double n = FIX2DBL(vstem) / FIX2DBL(hstem);
								UDV[0] = DBL2FIX((n * FIX2DBL(vstem) + FIX2DBL(hstem)) /
								                 (n + 1));
							}
						}
						else {
							/* Sans/serif design and no hstem; use vstem */
							UDV[0] = vstem;
						}
						if (UDV[0] < width / 2) {
							goto setwidth;  /* Good weight; set width axis */
						}
					}

					if (StdVW[iMaster] != -1.0) {
						UDV[0] = DBL2FIX(StdVW[iMaster]);   /* Use StdVW */
					}
					else if (is_serif) {
						UDV[0] = INT2FIX(100);                      /* Use serif default */
					}
					else {
						UDV[0] = INT2FIX(68);                       /* Use sans default */
					}
				}     /* end if there is no weight override */
				else {
					isUDVOverride = 1;
				}

				/* Set width */
setwidth:
				UDV[1] = width;

				if (!isUDVOverride) {
					/* we do not scale the override value */
					UDV[0] =  DBL2FIX((FIX2DBL(UDV[0]) / h->newGlyph.matrix.d));
				}
				UDV[1] =  DBL2FIX((FIX2DBL(UDV[1]) / h->newGlyph.matrix.d));

				/* clamp UDV to extrapolation limits, if using FillinMM font */
				/* record the ratio between the FillInMM and target glyph width for the template glyph ( currently zer). */
				/* we use this later to scale up all the synthetic glyph widths */
				if (font_index == kFillinMMIndex) {
					FillinMM_font_clampUDV(h, is_serif, UDV);
				}
			}     /* end if -else target glyph not seen. */
		}    /* end for each iMaster */

		/* Until I'm sure this is working, let's printout the UDV values */
		{
			/*
			   char fontname[32];
			   Fixed *UDV = &(h->newGlyph.fill_in_cff[font_index].UDV[0][0]);

			   if (font_index == kEuroMMIndex)
			     strcpy(fontname, "EuroMM");
			   else
			     strcpy(fontname, "FillinMM");

			   if (is_serif)
			    strcat(fontname, "-Serif");
			   else
			    strcat(fontname, "-Sans");

			   parseNewGlyphReport(h->g, "Setting UDV for %s: %f %f",  fontname,  FIX2DBL(UDV[0]), FIX2DBL(UDV[1]));
			   parseNewGlyphReport(h->g, "Scaling glyph top/botton for %s: %f %f",  fontname,  scaling_gid_top, scaling_gid_bottom);
			   parseNewGlyphReport(h->g, "Font Matrix for for %s: c %f d %f tx %f ty %f",  fontname,  h->newGlyph.matrix.c,  h->newGlyph.matrix.d,h->newGlyph.matrix.tx,h->newGlyph.matrix.ty);
			 */
		}
	}     /* end for each fill-in font. */
}

/*
   Add the new glyph. It has the GID == gl_id in the fill-in font, and will be created with GID == id in the target font.

   The source glyph from the fill-in font is snapshotted to a single path, via the UDV and font transfrom, for
   each master design of the target font.
 */
void recodeAddNewGlyph(tcCtx g, unsigned id, unsigned fill_in_font_id, unsigned gl_id, unsigned noslant,  unsigned adjust_overshoot, char *char_name) {
	recodeCtx h = g->ctx.recode;

	cffCtx ctx;
	cffGlyphInfo *gi;
	int font_index = fill_in_font_id;


	/* Initialize for new glyph addition. Affects the pathcb callbacks.  */

	h->idType           = SIDType;
	h->noslant          = noslant;
	h->id               = id;
	h->width.curr       = dnaMAX(h->width.fd, 0);
	h->path.segs.cnt    = 0;
	h->path.ops.cnt     = 0;
	h->path.args.cnt    = 0;
	h->path.idmask.cnt  = 0;
	h->hint.cntr        = 0;
	h->hint.initial     = 1;
	h->stem.list.cnt    = 0;
	h->cntr.list.cnt    = 0;
	h->newGlyph.max_x   = 0.0;
	h->newGlyph.max_y = 0.0;

	setBlend(h, h->path.lsb.x, 0);
	dnaNEXT(h->chars)->icstr = h->cstrs.cnt;

	h->newGlyph.cff.data = h->newGlyph.fill_in_cff[font_index].data;
	h->newGlyph.cff.length = h->newGlyph.fill_in_cff[font_index].length;

	ctx = cffNew(h->stdcb, 0); /* the stdcb callbacks reference only h->newGlyph.cff.data/length */

	/* scale the new glyph to match the target font template glyph height, width, and weight,
	   for each master in the target font */
	for (h->newGlyph.iMaster = 0; h->newGlyph.iMaster < h->nMasters; h->newGlyph.iMaster++) {
		Fixed *UDV = &(h->newGlyph.fill_in_cff[font_index].UDV[0][h->newGlyph.iMaster]);
		int iMaster = h->newGlyph.iMaster;

		/* Note: the next line resets the h->newGlyph.matrix values, so I don't need to undo the following modifications
		   to the values for notdef and glyphs with a round bottom overshoot */
		h->newGlyph.matrix = h->newGlyph.fill_in_cff[font_index].matrix[iMaster];

		if (id == 0) {
			/* special case for the notdef glyph. No scaling. */
			h->newGlyph.matrix.d = 1.0;
		}

		if ((!adjust_overshoot) || (id == 0)) {
			h->newGlyph.matrix.ty = 0.0;  /* for not def and glyphs without a round bottom overshoot, which must stay on baseline. */
		}
		h->newGlyph.matrix.noslant_glyph_offset = 0.0;

		/* Traverse glyph */
		h->newGlyph.iStem = 0;
		h->newGlyph.iCoord = 0;


		cffSetUDV(ctx, 2, UDV);

		/* Section for adjusting letter spacing of non-slanted syntheetic glyphs in italic fonts.
		   I call cffGetGlyphInfo with the metricsPathcb solely in order to get the h->newGlyph.max_x and h->newGlyph.max_y WITHOUT
		   adding points to the NewGlyph structure. This allows me to set the h->newGlyph.matrix.noslant_glyph_offset before building
		   the synthetic glyph.

		   The idea is that we need to move non-slanted glyphs to the right, in order to avoid collisions with figure-height italic glyphs
		   with the left side of the non-slanted glyph. The most we need to shift the glyph is by the x-shift from slanting the figure height,
		   as this is the tallest glyph that is frequently on the left side. However, if the non-slanted glyph is smaller than the figure height,
		   then we can instead base the x-shift only on the non-slanted glyph height.

		   We then need to check the new RSB. IF the synthetic glyph max x is greater than the current glyph width, we extend the glyph width to be
		   be max X + the original RSB (all after scaling by the new font transform).


		 */
		if (h->noslant && (h->italic_angle != 0.0)) {
			/* for italic target fonts only, and only the non-slanted glyphs */
			cffGlyphInfo *metrics_gi;
			int max_x;
			int max_y;

			metrics_gi = cffGetGlyphInfo(ctx, gl_id, h->metricsPathcb);

			/* metrics_gi contains the unscaled metrics for the FillinMM glyphs at the current UDV */
			/* max_x and max-Y are the scaled metrics for the new synthetic glyph data */
			max_x = (int)h->newGlyph.max_x;
			max_y = (int)h->newGlyph.max_y;

			h->newGlyph.matrix.noslant_glyph_offset = h->newGlyph.matrix.c * h->newGlyph.max_y;

			if (h->newGlyph.matrix.noslant_glyph_offset > h->newGlyph.matrix.figure_height_offset) {
				h->newGlyph.matrix.noslant_glyph_offset = h->newGlyph.matrix.figure_height_offset;
			}

			/* adjust the no_slant glyph offset by the amount that the font has been shifted by the designer. */
			h->newGlyph.matrix.noslant_glyph_offset += h->newGlyph.matrix.tx;
		}



		/*
		   This call to cffGetGlyphInfo makes the synthetic glyph under h->newGlyph, and calls all the 'euroXXXX' call backs.
		   obliquing , scaling, and offsets all happen here.
		   The gi is for the  glyph info for the FillinMM font at the UDV, NOT the
		   scaled synthetic info, which is under  h->newGlyph area.
		 */
		gi = cffGetGlyphInfo(ctx, gl_id, h->pathcb);


		{
			/* Now to fix up the glyph width */
			double width = gi->hAdv * h->newGlyph.matrix.d;
			long iwidth;

			if (h->noslant && (h->italic_angle != 0.0)) {
				/* make sure the original RSB is preserved  in this case */
				double rsb = width - (gi->bbox.right * h->newGlyph.matrix.d);
				width = h->newGlyph.max_x + rsb;
			}

			iwidth = (long)floor(width + 0.5);

			h->newGlyph.width[iMaster] = INT2FIX(iwidth);

			/* Force Euro to the width of the target font zero. */

			if (!strcmp(char_name, "Euro")) {
				Fixed zeroWidth = h->newGlyph.templateGlyph[font_index].width[iMaster];
				int diff =  (zeroWidth - h->newGlyph.width[iMaster]) >> 16;

				diff = (diff > 0) ? diff : -diff;

				if (diff == 1) {
					parseWarning(g, "Zero width vs  calculated Euro width round-off difference: %f, %d", FIX2DBL(zeroWidth), iwidth);
				}
				if (diff > 1) {
					parseWarning(g, "Problem: Zero width of target font is not same as calculated Euro width: %f, %d", FIX2DBL(zeroWidth), iwidth);
				}
				h->newGlyph.width[iMaster] = zeroWidth;
			}
		} /* end width-fix-up */
	}

	cffFree(ctx);

	if (h->hint.initial) {
		/* No hintmasks in charstring; construct initial mask for all stems */
		int i;
		for (i = 0; i < h->stem.list.cnt; i++) {
			SET_ID_BIT(h->stem.initmask, i);
		}
	}

	/* Initialize width */
	copyBlend(h->path.width, h->newGlyph.width);
	if (h->width.opt) {
		addWidth(h, h->newGlyph.width[0]);
	}

	/* Save type 2 path */
	recodePath(h);

	g->status |= TC_EURO_ADDED;
}

/*
   Get the char name and the unicode name for the next glyph name in the FillinMM font, using the GID passed in.
   Return 0 if the GID is out of range.
 */
unsigned getNextStdGlyph(unsigned gl_id, char ***charListptr) {
	unsigned status = 0;

	/* Each gl_id name has up to 5 alternate names for the glyph. Last item in list of char ptrs must be NULL, */
	static char *std_char_names[][kMaxAlternateNames] = {
			#include "fill_in_mm.cs"
	};

	int length = sizeof(std_char_names) / (kMaxAlternateNames * sizeof(char *)); /* each pointer is 4 bytes */


	if (gl_id >= (unsigned)length) {
		*charListptr = NULL;
	}
	else {
		*charListptr = std_char_names[gl_id];
		status = 1;
	}

	return status;
}

/* the std FillinMM font has a bad area in its design space. We need to clamp
   to a line running through (width = 226, weight=108) and (width = 535, weight=220)
   aka (width = 0.0, weight=0.4) and    (width = ,67, weight=1.0) in normalized space,
   blocking off the corner at (width = 226, weight=220)

   We do this by calculating the vector from the current UDV to the nearest point on the line.
   If ew represent the line as:
   y = (dy/dx)x + c1

   then for current point (x1,y1), the nearest point (x2, y2) on the line is calculated as:
   x2 = ( (y1 + (dx/dy)*x1) - c1) / ( (dy/dx) + (dx/dy))
   y2 =( dy.dx)*x2 + c1

   if either (x1 - x2) or (y1 -y2) are negative, we are above the line, and should clamp
   the NDV to (x2, y2)
 */
static int FillinMM_font_clampUDV(recodeCtx h, int is_serif, cffFixed *UDV) {
	int status = 0; /* 0 means the UDV did not need to be clamped. */
	double min_width = 226;
	double min_weight = 30;
	double max_weight = 220;
	double width_limit = 535;
	double weight_limit = 108;


	/* current point */
	double x1 = FIX2DBL(UDV[0]);
	double y1 = FIX2DBL(UDV[1]);

	/* nearest point on line */
	double x2 = 0;
	double y2 = 0;

	double dy_dx;
	double dx_dy;

	double c1;
	double c2;

	if (is_serif) {
		min_weight = 34;
	}

	dy_dx = (width_limit - min_width) / (max_weight - weight_limit);
	dx_dy = 1.0 / dy_dx;

	c1 =  min_width - (dy_dx * weight_limit);
	c2 = y1 + (dx_dy * x1);

	x2 = (c2 - c1) / (dy_dx + dx_dy);
	y2 = c2 - dx_dy * x2;

	/* if clamped point is to left of x1, then we need to clamp */
	if ((x2 - x1) < 0) {
		status = 1;
		if (!h->newGlyph.templateGlyph[kFillinMMIndex].alreadyWarnedonClampedUDV) {
			h->newGlyph.templateGlyph[kFillinMMIndex].alreadyWarnedonClampedUDV = 1;
			parseNewGlyphReport(h->g, "Clamping UDV from %f %f to %f %f", x1, y1, x2, y2);
		}

		UDV[0] = DBL2FIX(x2);
		UDV[1] = DBL2FIX(y2);
	}

	return status;
}

#endif /* TC_EURO_SUPPORT */

#if TC_DEBUG
/* -------------------------------- Debug ---------------------------------- */
static void dbcstrs(recodeCtx h, unsigned iChar) {
	long iStart = h->chars.array[iChar].icstr;
	unsigned length = (((long)(iChar + 1) == h->chars.cnt) ?
	                   h->cstrs.cnt : h->chars.array[iChar + 1].icstr) - iStart;
	csDump(length, (unsigned char *)&h->cstrs.array[iStart], h->nMasters, 0);
}

static void dbvalue(recodeCtx h, int n, Fixed *value) {
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

static void dbstack(recodeCtx h) {
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

static void dbstemid(recodeCtx h, int idsize, char *idmask) {
	int i;
	for (i = 0; i < idsize; i++) {
		printf("%02x%s", idmask[i] & 0xff, (i == idsize - 1) ? "" : ",");
	}
}

static void dbstems(recodeCtx h) {
	int i;
	printf("--- stems[index]={type,id,edge,delta}\n");
	if (h->stem.list.cnt == 0) {
		printf("empty\n");
	}
	else {
		int idsize = (h->stem.list.cnt + 7) / 8;
		for (i = 0; i < h->stem.list.cnt; i++) {
			Stem *stem = &h->stem.list.array[i];
			printf("[%d]={%hhx,%hhu,", i, stem->type, stem->id);
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

static void dbwidths(recodeCtx h, int fd) {
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
			printf("[%d]={%hd,%lu} ", i, width->width, width->count);
		}
		printf("\n");
	}
}

static void dbcoordop(recodeCtx h, char *op, int nCoords, unsigned index) {
	while (nCoords-- > 0) {
		dbvalue(h, h->nMasters, &h->path.args.array[index]);
		printf(" ");
		index += h->nMasters;
	}
	printf("%s/", op);
}

static void dbpath(recodeCtx h) {
	static char *escopname[] = {
		/*  0 */ "dotsection",
		/*  1 */ "reservedESC1",
		/*  2 */ "reservedESC2",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "reservedESC6",
		/*  7 */ "reservedESC7",
		/*  8 */ "store",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "load",
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
	};
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
	if (h->path.ops.cnt == 0) {
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

				case t2_hintmask:
					printf("hint[");
					dbstemid(h, op->nArgs, &h->path.idmask.array[op->iArgs]);
					printf("]/");
					break;

				default:
					if (op->type >> 8 != tx_escape ||
					    (op->type & 0xff) >= TABLE_LEN(escopname)) {
						printf("? ");
					}
					else {
						dbcoordop(h, escopname[op->type & 0xff],
						          op->nArgs, op->iArgs);
					}
					break;
			}
			op++;
		}
	}
}

static void dbargs(recodeCtx h) {
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

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dbcstrs, dbstack, dbstems, dbwidths, dbpath, dbargs);
}

#endif /* TC_DEBUG */