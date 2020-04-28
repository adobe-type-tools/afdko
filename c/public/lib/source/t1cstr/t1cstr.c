/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type 1 charstring support.
 */

#if defined(_WIN32) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES /* Needed to define M_PI under Windows */
#endif

#include "t1cstr.h"
#include "txops.h"
#include "ctutil.h"

#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

/* Make up operator for internal use */
#define t2_cntroff t2_reservedESC33
enum /* seac operator conversion phase */
{
    seacNone,          /* No seac conversion */
    seacBase,          /* In base char */
    seacAccentPreMove, /* In accent char before first moveto */
    seacAccentPostMove /* In accent char after first moveto */
};

/* Module context */
typedef struct t1cCtx *t1cCtx;
struct t1cCtx {
    long flags;                   /* Control flags */
#define IN_FLEX           (1<<0)  /* In flex sequence */
#define PEND_MOVETO       (1<<1)  /* Pending moveto */
#define PEND_HINTSUBS     (1<<2)  /* Pending hint substitution */
#define SEEN_HINTSUBS     (1<<3)  /* Seen hint substitution */
#define SEEN_MOVETO       (1<<4)  /* Seen moveto (path data) */
#define SEEN_ENDCHAR      (1<<5)  /* Seen endchar operator */
#define SEEN_BLEND        (1<<6)  /* Seen blend operator: need to round position after each op. */
#define NEW_GROUP         (1<<7)  /* Flags new counter group */
#define USE_MATRIX        (1<<12) /* Apply transform to */
#define USE_GLOBAL_MATRIX (1<<13) /* Transform is applied to entire charstring */
    struct /* Operand stack */
    {
        int cnt;
        float array[T1_MAX_OP_STACK];
    } stack;
    float x; /* Path x-coord */
    float y; /* Path y-coord */
    long maxOpStack;
    int subrDepth;
    float transformMatrix[6];
    struct
    {
        int useLEStart;
        float x; /* start x-coord for previous LE  */
        float y; /* start y-coord for previous LE */
    } le_start;
    struct /* Left side bearing from hsbw/sbw */
    {
        float x;
        float y;
    } lsb;
    struct /* Counter control args */
    {
        int cnt;
        float array[T2_MAX_STEMS * 2 + 2];
    } cntr;
    struct /* seac conversion data */
    {
        float adx;
        float ady;
        float bsb_x;  /* base component hsb */
        float bsb_y;  /* base component hsb */
        float asb_x;  /* accent component hsb */
        float asb_y;  /* accent component hsb */
        float xshift; /* shift  = ((seac.adx + seac.bsb)  - seac.asb)- accent component hsb */
        int phase;
    } seac;
    float BCA[TX_BCA_LENGTH]; /* BuildCharArray */
    struct                    /* Flex args */
    {
        long cnt;
        float args[17];
    } flex;
    struct /* Source data */
    {
        char *buf;   /* Buffer */
        long length; /* Buffer length */
        long offset; /* Buffer offset */
    } src;
    t1cAuxData *aux;          /* Auxiliary parse data */
    abfGlyphCallbacks *glyph; /* Glyph callbacks */
};

/* Check stack contains at least n elements. */
#define CHKUFLOW(n)                      \
    do {                                 \
        if (h->stack.cnt < (n)) {        \
            dummyDebug(h->stack.cnt);    \
            return t1cErrStackUnderflow; \
        }                                \
    } while (0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n)                                                         \
    do {                                                                    \
        if (h->stack.cnt + (n) > h->maxOpStack) return t1cErrStackOverflow; \
    } while (0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++] = (float)(v))

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))
#define RND(v) ((float)floor((v) + 0.50001))
/* The 0.000005 is added to allow for differences in OS's and other mathlibs
when blending LE's or MM's. The problem is that the differences can lead to 
a final value being just a hair above .5 on one platform and a hair below on
another,leading, to off by one differences depending on which system is being
 used to build the output font.
The logic 'floor(x + 0.5)' implements std Java rounding, where x.5 rounds
 to (x+1), -x.5) rounds to (-x), and everything else rounds to the nearest
 whole integer. The addition of 0.00001 captures everything 'close enough'
 to +/- x.5 to be treated as it if were on the boundary. This number is derived
 empirically, as being the smallest number that is still larger than precision
 differences between Flex/Flash in TWB2, and the 64 bit C math lib.
 */

/* Transform coordinates by matrix. */
#define TX(x, y) \
    RND(h->transformMatrix[0] * (x) + h->transformMatrix[2] * (y) + h->transformMatrix[4])
#define TY(x, y) \
    RND(h->transformMatrix[1] * (x) + h->transformMatrix[3] * (y) + h->transformMatrix[5])

/* Scale coordinates by matrix. */
#define SX(x) RND(h->transformMatrix[0] * (x))
#define SY(y) RND(h->transformMatrix[3] * (y))

/* Flex arguments */
#define CHKFLEX(n)                                             \
    do {                                                       \
        if (h->flex.cnt + (n) > (long)ARRAY_LEN(h->flex.args)) \
            return t1cErrFlex;                                 \
    } while (0)
#define PUSHFLEX(v) (h->flex.args[h->flex.cnt++] = (float)(v))

static int t1Decode(t1cCtx h, long offset);

/* ------------------------------- Data Input ------------------------------ */

/* Refill input buffer. Return NULL on stream error else pointer to buffer. */
static unsigned char *refill(t1cCtx h, unsigned char **end) {
    /* Read buffer */
    /* 64-bit warning fixed by cast here HO */
    h->src.length = (long)h->aux->stm->read(h->aux->stm, h->aux->src, &h->src.buf);
    if (h->src.length == 0)
        return NULL; /* Stream error */

    /* Update offset of next read */
    h->src.offset += h->src.length;

    /* Set buffer limit */
    *end = (unsigned char *)h->src.buf + h->src.length;

    return (unsigned char *)h->src.buf;
}

/* Seek to offset on source stream. */
static int srcSeek(t1cCtx h, long offset) {
    if (h->aux->stm->seek(h->aux->stm, h->aux->src, offset))
        return 1;
    h->src.offset = offset;
    return 0;
}

/* Check next byte available and refill buffer if not. */
#define CHKBYTE(h)                      \
    do                                  \
        if (next == end) {              \
            next = refill(h, &end);     \
            if (next == NULL)           \
                return t1cErrSrcStream; \
        }                               \
    while (0)

static int dummyDebug(int debugErr) {
    return debugErr;
}

/* ------------------------------- Callbacks ------------------------------- */

/* Callback char width. Return non-0 to end parse else 0. */
static int callbackWidth(t1cCtx h, float width) {
    if (width < -32000.0f || width > 32000.0f)
        /* This is an rare case where a number in the charstring was specified
           as a 5-byte number but was never followed by a "div" operator which
           would have reduced its magnitude. The value is erroneous and cannot
           be represented in Type 2 so we reduce to a safe range so as not to
           cause subsequent processing problems. */
        width /= 65536.0f;

    if (h->flags & USE_MATRIX)
        width = SX(width);
    else if (h->flags & SEEN_BLEND)
        width = RND(width);
    h->glyph->width(h->glyph, width);
    return h->aux->flags & T1C_WIDTH_ONLY;
}

/* Callback path move. */
static void callbackMove(t1cCtx h, float dx, float dy) {
    float x;
    float y;

    h->flags &= ~PEND_MOVETO;
    h->flags |= SEEN_MOVETO;

    x = h->x + dx;
    y = h->y + dy;

    x = (float)RND_ON_READ(x);
    y = (float)RND_ON_READ(y);

    if (h->seac.phase == seacAccentPreMove) {
        x += h->seac.xshift;
        y += h->seac.ady;
        h->seac.phase = seacAccentPostMove;
    }

    h->x = x;
    h->y = y;

    if (h->flags & USE_MATRIX) {
        /* if START_PATH_MATRIX is not set, then this either global, or left over from a previous path.  */
        if (h->flags & USE_GLOBAL_MATRIX) {
            int i;
            /* restore the global matrix values */
            for (i = 0; i < 6; i++) {
                h->transformMatrix[i] = h->aux->matrix[i];
            }
        } else {
            /* If USE_GLOBAL_MATRIX is not set, then the   USE_MATRIX is left over from the previous path. Clear it. */
            h->flags &= ~USE_MATRIX;
            h->transformMatrix[0] = h->transformMatrix[3] = 1.0;
            h->transformMatrix[1] = h->transformMatrix[2] = h->transformMatrix[4] = h->transformMatrix[5] = 1.0;
        }
    }

    if (h->flags & USE_MATRIX)
        h->glyph->move(h->glyph, TX(x, y), TY(x, y));
    else if (h->flags & SEEN_BLEND)
        h->glyph->move(h->glyph, RND(x), RND(y));
    else
        h->glyph->move(h->glyph, x, y);
}

/* Callback path line. */
static void callbackLine(t1cCtx h, float dx, float dy) {
    float x;
    float y;

    if (h->flags & PEND_MOVETO)
        callbackMove(h, 0, 0); /* Insert missing move */

    x = h->x + dx;
    y = h->y + dy;

    x = (float)RND_ON_READ(x);
    y = (float)RND_ON_READ(y);

    h->x = x;
    h->y = y;

    if (h->flags & USE_MATRIX)
        h->glyph->line(h->glyph, TX(x, y), TY(x, y));
    else if (h->flags & SEEN_BLEND)
        h->glyph->line(h->glyph, RND(x), RND(y));
    else
        h->glyph->line(h->glyph, x, y);
}

/* Callback path curve. */
static void callbackCurve(t1cCtx h,
                          float dx1, float dy1,
                          float dx2, float dy2,
                          float dx3, float dy3) {
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;

    if (h->flags & PEND_MOVETO)
        callbackMove(h, 0, 0); /* Insert missing move */

    x1 = h->x + dx1;
    y1 = h->y + dy1;
    x2 = x1 + dx2;
    y2 = y1 + dy2;
    x3 = x2 + dx3;
    y3 = y2 + dy3;
    x3 = (float)RND_ON_READ(x3);
    y3 = (float)RND_ON_READ(y3);
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

/* Callback stem hint. */
static void callbackStem(t1cCtx h, int flags, float edge, float width) {
    float edge0;
    float edge1;

    if (h->glyph->stem == NULL)
        return;

    if (!(h->flags & SEEN_HINTSUBS) && h->flags & SEEN_MOVETO)
        /* Initial unsubstituted hints after drawing ops; force substitution */
        h->flags |= (PEND_HINTSUBS | SEEN_HINTSUBS);

    /* Adjust offset */
    if (h->seac.phase == seacBase) {
        edge += (flags & ABF_VERT_STEM) ? h->seac.bsb_x : h->seac.bsb_y;
    } else if (h->seac.phase > seacBase) {
        float shift = ((flags & ABF_VERT_STEM) ? h->seac.xshift : h->seac.ady) + ((flags & ABF_VERT_STEM) ? h->seac.asb_x : h->seac.asb_y);
        edge += (flags & ABF_VERT_STEM) ? shift : h->seac.ady;
    } else {
        edge += (flags & ABF_VERT_STEM) ? h->lsb.x : h->lsb.y;
    }

    edge0 = edge;
    edge1 = edge + width;

    if (h->flags & USE_MATRIX) {
        /* Apply matrix */
        if (flags & ABF_VERT_STEM) {
            edge0 = SX(edge0);
            edge1 = SX(edge1);
        } else {
            edge0 = SY(edge0);
            edge1 = SY(edge1);
        }
    } else if (h->flags & SEEN_BLEND) {
        edge0 = RND(edge0);
        edge1 = RND(edge1);
    }

    /* Set flags */
    if (h->flags & PEND_HINTSUBS) {
        flags |= ABF_NEW_HINTS;
        h->flags &= ~PEND_HINTSUBS;
    }
    if (h->flags & NEW_GROUP) {
        flags |= ABF_NEW_GROUP;
        h->flags &= ~NEW_GROUP;
    }

    h->glyph->stem(h->glyph, flags, edge0, edge1);
}

/* Callback path flex. */
static void callbackFlex(t1cCtx h) {
    float *args = h->flex.args;
    float dx1 = args[0] + args[2];
    float dy1 = args[1] + args[3];
    float dx2 = args[4];
    float dy2 = args[5];
    float dx3 = args[6];
    float dy3 = args[7];
    float dx4 = args[8];
    float dy4 = args[9];
    float dx5 = args[10];
    float dy5 = args[11];
    float dx6 = args[12];
    float dy6 = args[13];
    float depth = args[14];

    if (h->flags & PEND_MOVETO)
        callbackMove(h, 0, 0); /* Insert missing move */

    if (h->glyph->flex == NULL) {
        /* Callback as 2 curves */
        callbackCurve(h,
                      dx1, dy1,
                      dx2, dy2,
                      dx3, dy3);
        callbackCurve(h,
                      dx4, dy4,
                      dx5, dy5,
                      dx6, dy6);
    } else {
        /* Callback as flex hint; convert to absolute coordinates */
        float x1 = h->x + dx1;
        float y1 = h->y + dy1;
        float x2 = x1 + dx2;
        float y2 = y1 + dy2;
        float x3 = x2 + dx3;
        float y3 = y2 + dy3;
        float x4 = x3 + dx4;
        float y4 = y3 + dy4;
        float x5 = x4 + dx5;
        float y5 = y4 + dy5;
        float x6 = x5 + dx6;
        float y6 = y5 + dy6;
        x6 = (float)RND_ON_READ(x6);
        y6 = (float)RND_ON_READ(y6);
        h->x = x6;
        h->y = y6;

        if (h->flags & USE_MATRIX) {
            x1 = TX(x1, y1);
            y1 = TY(x1, y1);
            x2 = TX(x2, y2);
            y2 = TY(x2, y2);
            x3 = TX(x3, y3);
            y3 = TY(x3, y3);
            x4 = TX(x4, y4);
            y4 = TY(x4, y4);
            x5 = TX(x5, y5);
            y5 = TY(x5, y5);
            x6 = TX(x6, y6);
            y6 = TY(x6, y6);
            /* depth not in char space units so not transformed! */
        } else if (h->flags & SEEN_BLEND) {
            x1 = RND(x1);
            y1 = RND(y1);
            x2 = RND(x2);
            y2 = RND(y2);
            x3 = RND(x3);
            y3 = RND(y3);
            x4 = RND(x4);
            y4 = RND(y4);
            x5 = RND(x5);
            y5 = RND(y5);
            x6 = RND(x6);
            y6 = RND(y6);
            depth = RND(depth);
        }

        h->glyph->flex(h->glyph, depth,
                       x1, y1,
                       x2, y2,
                       x3, y3,
                       x4, y4,
                       x5, y5,
                       x6, y6);
    }

    /* The Type 1 flex mechanism specifies the new current point as a pair of
       absolute coordinates via operands 15 and 16. This information is
       redundant since it could be calculated by adding all the deltas but I
       have found that there is sometimes disagreement between this calculation
       and the specified operands, the operands being in error. I have chosen
       to use the operands to reset the current point because it mimics the way
       that the rasterizer functions and ensures that incorrectly constructed
       fonts will be faithfully reproduced even when wrong! The major source of
       these errors is a result of a BuildFont off-by-one bug when moving from
       a closepoint-terminated subpath to a new subpath in MM fonts. */

    if (h->seac.phase <= seacBase) {
        h->x = (float)RND_ON_READ(args[15]);
        h->y = (float)RND_ON_READ(args[16]);
    }
}

/* Callback operator and args. */
static void callbackOp(t1cCtx h, int op) {
    h->glyph->genop(h->glyph, h->stack.cnt, h->stack.array, op);
}

/* Callback seac operator. */
static void callback_seac(t1cCtx h, float adx, float ady, int bchar, int achar) {
    if (h->flags & USE_MATRIX) {
        adx = TX(adx, ady);
        ady = TY(adx, ady);
    } else if (h->flags & SEEN_BLEND) {
        adx = RND(adx);
        ady = RND(ady);
    }
    h->glyph->seac(h->glyph, adx, ady, bchar, achar);
}

/* ---------------------------- Charstring Parse --------------------------- */

/* Execute "and" op. Return 0 on success else error code. */
static int do_and(t1cCtx h) {
    int b;
    int a;
    CHKUFLOW(2);
    b = POP() != 0.0;
    a = POP() != 0.0;
    PUSH(a && b);
    return 0;
}

/* Execute "or" op. Return 0 on success else error code. */
static int do_or(t1cCtx h) {
    int b;
    int a;
    CHKUFLOW(2);
    b = POP() != 0.0;
    a = POP() != 0.0;
    PUSH(a || b);
    return 0;
}

/* Execute "not" op. Return 0 on success else error code. */
static int do_not(t1cCtx h) {
    int a;
    CHKUFLOW(1);
    a = POP() == 0.0;
    PUSH(a);
    return 0;
}

/* Select registry item. Return NULL on invalid selector. */
static float *selRegItem(t1cCtx h, int reg, int *size) {
    switch (reg) {
        case T1_REG_WV:
            *size = T1_MAX_MASTERS;
            return h->aux->WV;
        case T1_REG_NDV:
            *size = T1_MAX_AXES;
            return h->aux->NDV;
        case T1_REG_UDV:
            *size = T1_MAX_AXES;
            return h->aux->UDV;
    }
    return NULL;
}

/* Execute "store" op. Return 0 on success else error code. */
static int do_store(t1cCtx h) {
    int size;
    int count;
    int i;
    int j;
    int reg;
    float *array;
    int temp_length;
    int temp_size;

    CHKUFLOW(4);

    count = (int)POP();
    i = (int)POP();
    j = (int)POP();
    reg = (int)POP();
    array = selRegItem(h, reg, &size);

    temp_length = i + count + 1;
    temp_size = j + count + 1;
    if (array == NULL || count < 0 ||
        i < 0 || temp_length < 0 || temp_length >= TX_BCA_LENGTH ||
        j < 0 || temp_size < 0 || temp_size >= size)
        return t1cErrStoreBounds;

    memcpy(&array[j], &h->BCA[i], sizeof(float) * count);
    return 0;
}

/* Execute "abs" op. Return 0 on success else error code. */
static int do_abs(t1cCtx h) {
    float a;
    CHKUFLOW(1);
    a = POP();
    PUSH((a < 0.0) ? -a : a);
    return 0;
}

/* Execute "add" op. Return 0 on success else error code. */
static int do_add(t1cCtx h) {
    float b;
    float a;
    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(a + b);
    return 0;
}

/* Execute "sub" op. Return 0 on success else error code. */
static int do_sub(t1cCtx h) {
    float b;
    float a;
    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(a - b);
    return 0;
}

/* Execute "load" op. Return 0 on success else error code. */
static int do_load(t1cCtx h) {
    int size;
    int count;
    int i;
    int reg;
    float *array;

    CHKUFLOW(3);

    count = (int)POP();
    i = (int)POP();
    reg = (int)POP();
    array = selRegItem(h, reg, &size);

    if (array == NULL || count < 0 ||
        i < 0 || i + count - 1 >= TX_BCA_LENGTH || count > size)
        return t1cErrLoadBounds;

    memcpy(&h->BCA[i], array, sizeof(float) * count);

    return 0;
}

/* Execute "neg" op. Return 0 on success else error code. */
static int do_neg(t1cCtx h) {
    float a;
    CHKUFLOW(1);
    a = POP();
    PUSH(-a);
    return 0;
}

/* Execute "eq" op. Return 0 on success else error code. */
static int do_eq(t1cCtx h) {
    float b;
    float a;
    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(a == b);
    return 0;
}

/* Execute "put" op. Return 0 on success else error code. */
static int do_put(t1cCtx h) {
    int i;
    CHKUFLOW(2);
    i = (int)POP();
    if (i < 0 || i >= TX_BCA_LENGTH)
        return t1cErrPutBounds;
    h->BCA[i] = POP();
    return 0;
}

/* Execute "get" op. Return 0 on success else error code. */
static int do_get(t1cCtx h) {
    int i;
    CHKUFLOW(1);
    i = (int)POP();
    if (i < 0 || i >= TX_BCA_LENGTH)
        return t1cErrGetBounds;
    PUSH(h->BCA[i]);
    return 0;
}

/* Execute "ifelse" op. Return 0 on success else error code. */
static int do_ifelse(t1cCtx h) {
    float v2;
    float v1;
    float s2;
    float s1;
    CHKUFLOW(4);
    v2 = POP();
    v1 = POP();
    s2 = POP();
    s1 = POP();
    PUSH((v1 <= v2) ? s1 : s2);
    return 0;
}

/* Execute "random" op. Return 0 on success else error code. */
static int do_random(t1cCtx h) {
    CHKOFLOW(1);
    PUSH(((float)rand() + 1) / ((float)RAND_MAX + 1));
    return 0;
}

/* Execute "mul" op. Return 0 on success else error code. */
static int do_mul(t1cCtx h) {
    float b;
    float a;
    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(a * b);
    return 0;
}

/* Execute "dup" op. Return 0 on success else error code. */
static int do_dup(t1cCtx h) {
    float a;
    CHKUFLOW(1);
    a = POP();
    CHKOFLOW(2);
    PUSH(a);
    PUSH(a);
    return 0;
}

/* Execute "exch" op. Return 0 on success else error code. */
static int do_exch(t1cCtx h) {
    float b;
    float a;
    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(b);
    PUSH(a);
    return 0;
}

/* Reverse stack elements between index i and j (i < j). */
static void reverse(t1cCtx h, int i, int j) {
    while (i < j) {
        float tmp = h->stack.array[i];
        h->stack.array[i++] = h->stack.array[j];
        h->stack.array[j--] = tmp;
    }
}

/* Callback h/vstem3 stems. */
static int callbackStem3(t1cCtx h, int flags) {
    CHKUFLOW(6);
    callbackStem(h, flags, INDEX(0), INDEX(1));
    callbackStem(h, flags, INDEX(2), INDEX(3));
    callbackStem(h, flags, INDEX(4), INDEX(5));
    return 0;
}

/* Parse seac component glyph. */
static int parseSeacComponent(t1cCtx h, int stdcode) {
    long offset;

    if (stdcode < 0 || stdcode > 255 ||
        (offset = h->aux->getStdEncGlyphOffset(h->aux->ctx, stdcode)) == -1)
        return t1cErrBadSeacComp;

    h->stack.cnt = 0;
    return t1Decode(h, offset);
}

/* Execute "div" op. Return 0 on success else error code. */
static int do_div(t1cCtx h) {
    float b;
    float a;

    CHKUFLOW(2);
    b = POP();
    a = POP();
    PUSH(a / b);

    return 0;
}

/* Callback stems of one direction in group. Return index of start of next
   group. */
static int addPartialGroup(t1cCtx h, int flags, int i) {
    float edge1 = 0;
    for (;;) {
        float edge0 = edge1 + h->cntr.array[i--];
        float width = h->cntr.array[i--];
        edge1 = edge0 + width;

        if (width < 0) {
            /* End of group */
            callbackStem(h, flags, edge1, -width);
            break;
        } else
            callbackStem(h, flags, edge0, width);
    }
    return i;
}

/* Add counter control list (othersubrs 12/13). Return 1 on error else 0. */
static int addCntrCntl(t1cCtx h, int last, int argcnt) {
    int i;
    int j;
    int nHoriz;
    int iHoriz;
    int nVert;
    int iVert;

    if (h->stack.cnt != argcnt ||
        h->cntr.cnt + argcnt > (int)ARRAY_LEN(h->cntr.array))
        return 1;

    /* Copy stack into argument list reversing order */
    for (i = h->stack.cnt - 1; i >= 0; i--)
        h->cntr.array[h->cntr.cnt++] = INDEX(i);

    if (!last)
        return 0; /* More counter ops to follow */

    if (h->cntr.cnt < 2 || h->cntr.cnt & 1)
        return 1;

    /* Search backwards for start of vertical stems (if any) */
    i = h->cntr.cnt - 1;
    nHoriz = (int)h->cntr.array[i];
    iHoriz = i - 1;
    for (j = 0; j < nHoriz; j++)
        do {
            i -= 2;
            if (i < 1)
                return 1;
        } while (h->cntr.array[i] >= 0);
    nVert = (int)h->cntr.array[--i];
    iVert = i - 1;

    /* Validate vertical groups */
    for (j = 0; j < nVert; j++)
        do {
            i -= 2;
            if (i < 0)
                return 1;
        } while (h->cntr.array[i] >= 0);

    if (nHoriz == 0 && nVert == 0)
        h->glyph->genop(h->glyph, 0, NULL, t2_cntroff);
    else
        do {
            h->flags |= NEW_GROUP;
            if (nHoriz-- > 0)
                iHoriz = addPartialGroup(h, ABF_CNTR_STEM, iHoriz);
            if (nVert-- > 0)
                iVert = addPartialGroup(h, ABF_CNTR_STEM | ABF_VERT_STEM, iVert);
        } while (nHoriz > 0 || nVert > 0);

    return 0;
}

/* Execute "blend" op. Return 0 on success else error code. */
static int do_blend(t1cCtx h, int nBlends) {
    int i;
    int nElements = nBlends * h->aux->nMasters;
    int iBase = h->stack.cnt - nElements;
    int k = iBase + nBlends;

    if (h->aux->nMasters <= 1)
        return t1cErrInvalidWV;
    CHKUFLOW(nElements);

    for (i = 0; i < nBlends; i++) {
        int j;
        float x = INDEX(iBase + i);
        for (j = 1; j < h->aux->nMasters; j++)
            x += INDEX(k++) * h->aux->WV[j];
        INDEX(iBase + i) = x;
    }

    h->stack.cnt = iBase + nBlends;

    h->flags |= SEEN_BLEND;
    return 0;
}

/* Decode type 1 charstring. Called recursively. Return 1 on endchar else 0. */
static int t1Decode(t1cCtx h, long offset) {
    unsigned char *end;
    unsigned char *next;

    /* Fetch charstring */
    if (srcSeek(h, offset))
        return t1cErrSrcStream;
    next = refill(h, &end);
    if (next == NULL)
        return t1cErrSrcStream;

    for (;;) {
        int result;
        int byte0;
        CHKBYTE(h);
        byte0 = *next++;
        switch (byte0) {
            case tx_hstem:
                CHKUFLOW(2);
                callbackStem(h, 0, INDEX(0), INDEX(1));
                break;
            case tx_vstem:
                CHKUFLOW(2);
                callbackStem(h, ABF_VERT_STEM, INDEX(0), INDEX(1));
                break;
            case tx_vmoveto:
                CHKUFLOW(1);
                if (h->flags & (long)IN_FLEX) {
                    CHKFLEX(2);
                    PUSHFLEX(0);
                    PUSHFLEX(INDEX(0));
                } else
                    callbackMove(h, 0, INDEX(0));
                break;
            case tx_rlineto:
                CHKUFLOW(2);
                callbackLine(h, INDEX(0), INDEX(1));
                break;
            case tx_hlineto:
            case tx_vlineto:
                CHKUFLOW(1);
                {
                    int horz = byte0 == tx_hlineto;
                    if (horz & 1)
                        callbackLine(h, INDEX(0), 0);
                    else
                        callbackLine(h, 0, INDEX(0));
                }
                break;
            case tx_rrcurveto:
                CHKUFLOW(6);
                callbackCurve(h,
                              INDEX(0), INDEX(1),
                              INDEX(2), INDEX(3),
                              INDEX(4), INDEX(5));
                break;
            case t1_closepath:
                h->flags |= PEND_MOVETO;
                break;
            case tx_callsubr:
                CHKUFLOW(1);
                {
                    long saveoff = h->src.offset - (end - next);
                    long num = (long)POP();

                    if (num < 0 || num >= h->aux->subrs.cnt)
                        return t1cErrCallsubr;

                    h->subrDepth++;
                    if (h->subrDepth > TX_MAX_SUBR_DEPTH) {
                        printf("subr depth: %d\n", h->subrDepth);
                        return t1cErrSubrDepth;
                    }
                    result = t1Decode(h, h->aux->subrs.offset[num]);
                    h->subrDepth--;

                    if (result || h->flags & SEEN_ENDCHAR)
                        return result;
                    else if (srcSeek(h, saveoff))
                        return t1cErrSrcStream;
                    next = refill(h, &end);
                    if (next == NULL)
                        return t1cErrSrcStream;
                }
                continue;
            case tx_return:
                return 0;
            case tx_escape: {
                /* Process escaped operator */
                int escop;
                CHKBYTE(h);
                escop = tx_ESC(*next++);
                switch (escop) {
                    case tx_dotsection:
                        if (!(h->aux->flags & T1C_UPDATE_OPS ||
                              h->flags & PEND_MOVETO ||
                              h->glyph->stem == NULL))
                            callbackOp(h, tx_dotsection);
                        break;
                    case tx_and:
                        result = do_and(h);
                        if (result)
                            return result;
                        continue;
                    case tx_or:
                        result = do_or(h);
                        if (result)
                            return result;
                        continue;
                    case tx_not:
                        result = do_not(h);
                        if (result)
                            return result;
                        continue;
                    case t1_store:
                        result = do_store(h);
                        if (result)
                            return result;
                        continue;
                    case tx_abs:
                        result = do_abs(h);
                        if (result)
                            return result;
                        continue;
                    case tx_add:
                        result = do_add(h);
                        if (result)
                            return result;
                        continue;
                    case tx_sub:
                        result = do_sub(h);
                        if (result)
                            return result;
                        continue;
                    case t1_load:
                        result = do_load(h);
                        if (result)
                            return result;
                        continue;
                    case tx_neg:
                        result = do_neg(h);
                        if (result)
                            return result;
                        continue;
                    case tx_eq:
                        result = do_eq(h);
                        if (result)
                            return result;
                        continue;
                    case tx_drop:
                        CHKUFLOW(1);
                        h->stack.cnt--;
                        continue;
                    case tx_put:
                        result = do_put(h);
                        if (result)
                            return result;
                        continue;
                    case tx_get:
                        result = do_get(h);
                        if (result)
                            return result;
                        continue;
                    case tx_ifelse:
                        result = do_ifelse(h);
                        if (result)
                            return result;
                        continue;
                    case tx_random:
                        result = do_random(h);
                        if (result)
                            return result;
                        continue;
                    case tx_mul:
                        result = do_mul(h);
                        if (result)
                            return result;
                        continue;
                    case tx_sqrt:
                        CHKUFLOW(1);
                        {
                            float a = POP();
                            if (a < 0.0)
                                return t1cErrSqrtDomain;
                            PUSH(sqrt(a));
                        }
                        continue;
                    case tx_dup:
                        result = do_dup(h);
                        if (result)
                            return result;
                        continue;
                    case tx_exch:
                        result = do_exch(h);
                        if (result)
                            return result;
                        continue;
                    case tx_index:
                        CHKUFLOW(1);
                        {
                            int i = (int)POP();
                            if (i < 0)
                                i = 0; /* Duplicate top element */
                            if (i >= h->stack.cnt)
                                return t1cErrIndexBounds;
                            {
                                float t = h->stack.array[h->stack.cnt - 1 - i];
                                PUSH(t);
                            }
                        }
                        continue;
                    case tx_roll:
                        CHKUFLOW(2);
                        {
                            int j = (int)POP();
                            int n = (int)POP();
                            int iTop = h->stack.cnt - 1;
                            int iBottom = h->stack.cnt - n;

                            if (n <= 0 || iBottom < 0)
                                return t1cErrRollBounds;

                            /* Constrain j to [0,n) */
                            if (j < 0)
                                j = n - (-j % n);
                            j %= n;

                            reverse(h, iTop - j + 1, iTop);
                            reverse(h, iBottom, iTop - j);
                            reverse(h, iBottom, iTop);
                        }
                        continue;
                    case t1_vstem3:
                        result = callbackStem3(h, ABF_STEM3_STEM | ABF_VERT_STEM);
                        if (result)
                            return result;
                        break;
                    case t1_hstem3:
                        result = callbackStem3(h, ABF_STEM3_STEM);
                        if (result)
                            return result;
                        break;
                    case t1_seac:
                        CHKUFLOW(5);

                        /* Save component character codes */
                        h->aux->bchar = (unsigned char)INDEX(3);
                        h->aux->achar = (unsigned char)INDEX(4);

                        if (h->aux->flags & T1C_UPDATE_OPS) {
                            h->seac.asb_x = INDEX(0);
                            h->seac.asb_y = 0;
                            h->seac.adx = INDEX(1);
                            h->seac.ady = INDEX(2);

                            h->seac.xshift = h->seac.adx + h->lsb.x - h->seac.asb_x;

                            /* Parse base component */
                            h->seac.phase = seacBase;
                            result = parseSeacComponent(h, h->aux->bchar);
                            if (result)
                                return result;

                            h->flags &= ~SEEN_ENDCHAR;
                            h->flags |= PEND_HINTSUBS;

                            /* Parse accent component */
                            h->seac.phase = seacAccentPreMove;
                            result = parseSeacComponent(h, h->aux->achar);
                            if (result)
                                return result;
                        } else {
                            INDEX(1) += -INDEX(0) + h->lsb.x;
                            callback_seac(h, INDEX(1), INDEX(2),
                                          h->aux->bchar, h->aux->achar);
                        }
                        return 0;
                    case t1_sbw:
                        if (h->seac.phase == seacNone) {
                            CHKUFLOW(4);
                            h->x = h->lsb.x = INDEX(0);
                            h->y = h->lsb.y = INDEX(1);
                            if (callbackWidth(h, INDEX(2)))
                                return t1cSuccess;
                        } else if (h->seac.phase == seacAccentPreMove) {
                            /* shift  == (seac.adx + seac.bsb) - accent component hsb. path.x is now equal to accent hsb.*/
                            /* We should now set the current path ' h->path.x' to (path.x + (( h->lsb.x  +   h->seac.adx) - accent hsb))
                               Since path.x is now == to  h->seac.asb, this reduces to setting ' h->path.x' to (h->seac.bsb + h->seac.adx) */
                            CHKUFLOW(4);
                            h->x = INDEX(0); /* h->x is now the accent hsb. */
                            h->y = INDEX(1);
                        } else if (h->seac.phase == seacBase) {
                            CHKUFLOW(4);
                            h->x = h->seac.bsb_x = INDEX(0);
                            h->y = h->seac.bsb_y = INDEX(1);
                        }
                        h->x = (float)RND_ON_READ(h->x);
                        h->y = (float)RND_ON_READ(h->y);
                        break;
                    case tx_div:
                        result = do_div(h);
                        if (result)
                            return result;
                        continue;
                    case t1_callother:
                        /* Process othersubr */
                        CHKUFLOW(2);
                        {
                            int othersubr = (int)POP();
                            int argcnt = (int)POP();
                            switch (othersubr) {
                                case t1_otherFlex:
                                    CHKUFLOW(3);
                                    if (h->flex.cnt != 14)
                                        return t1cErrFlex;
                                    PUSHFLEX(INDEX(0));
                                    PUSHFLEX(INDEX(1));
                                    PUSHFLEX(INDEX(2));
                                    callbackFlex(h);
                                    break;
                                case t1_otherPreflex1:
                                    h->flex.cnt = 0;
                                    h->flags |= IN_FLEX;
                                    continue;
                                case t1_otherPreflex2:
                                    continue; /* Discard */
                                case t1_otherHintSubs:
                                    h->flags |= (PEND_HINTSUBS | SEEN_HINTSUBS);
                                    continue;
                                case t1_otherGlobalColor:
                                    callbackOp(h, t2_cntron);
                                    break;
                                case t1_otherCntr1:
                                    if (addCntrCntl(h, 0, argcnt))
                                        return t1cErrBadCntrCntl;
                                    break;
                                case t1_otherCntr2:
                                    if (addCntrCntl(h, 1, argcnt))
                                        return t1cErrBadCntrCntl;
                                    break;
                                case t1_otherBlend1:
                                    result = do_blend(h, 1);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherBlend2:
                                    result = do_blend(h, 2);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherBlend3:
                                    result = do_blend(h, 3);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherBlend4:
                                    result = do_blend(h, 4);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherBlend6:
                                    result = do_blend(h, 6);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherAdd:
                                    result = do_add(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherSub:
                                    result = do_sub(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherMul:
                                    result = do_mul(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherDiv:
                                    result = do_div(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherPut:
                                    result = do_put(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherGet:
                                    result = do_get(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherIfelse:
                                    result = do_ifelse(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherRandom:
                                    result = do_random(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherDup:
                                    result = do_dup(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherExch:
                                    result = do_exch(h);
                                    if (result)
                                        return result;
                                    continue;
                                case t1_otherStoreWV: {
                                    int iBase = (int)INDEX(0);
                                    if (iBase < 0 || iBase + h->aux->nMasters > TX_BCA_LENGTH)
                                        return t1cErrStoreWVBounds;
                                    memcpy(&h->BCA[iBase], h->aux->WV,
                                           h->aux->nMasters * sizeof(h->aux->WV[0]));
                                } break;
                                case t1_otherReserved4:
                                case t1_otherReserved7:
                                case t1_otherReserved8:
                                case t1_otherReserved9:
                                case t1_otherReserved10:
                                case t1_otherReserved11:
                                case t1_otherReserved26:
                                default:
                                    return t1cErrInvalidOp;
                            } /* End: switch(othersubr) */
                        }     /* End: case t1_callother: */
                        break;
                    case t1_pop:
                        continue; /* Discard */
                    case t1_div2:
                        result = do_div(h);
                        if (result)
                            return result;
                        continue;
                    case t1_setcurrentpt:
                        if (h->flags & IN_FLEX)
                            h->flags &= ~IN_FLEX;
                        else {
                            /* Use illegal setcurrentpt operator (Apple driver)
                               to set the current point */
                            CHKUFLOW(2);
                            h->x = INDEX(0);
                            h->y = INDEX(1);
                            h->x = (float)RND_ON_READ(h->x);
                            h->y = (float)RND_ON_READ(h->y);
                        }
                        break;
                    default:
                        return t1cErrInvalidOp;
                } /* End: switch (escop) */
            }     /* End: case tx_escape: */
            break;

            case t1_hsbw:
                if (h->seac.phase == seacNone) {
                    CHKUFLOW(2);
                    h->x = h->lsb.x = INDEX(0);
                    h->y = h->lsb.y = 0;
                    if (callbackWidth(h, INDEX(1)))
                        return t1cSuccess;
                } else if (h->seac.phase == seacAccentPreMove) {
                    /* shift == (seac.adx + composite glyph hsb) - accent component hsb.
                       path.x is now equal to accent hsb. */
                    /* We should now set the current path 
                       'h->path.x' to (path.x + ((h->lsb.x + h->seac.adx) - accent hsb))
                       Since path.x is now == to h->seac.asb, this reduces to setting
                       'h->path.x' to (h->seac.bsb + h->seac.adx) */
                    CHKUFLOW(2);
                    h->x = INDEX(0); /* h->x is now the accent hsb. */
                    h->y = 0;
                } else if (h->seac.phase == seacBase) {
                    CHKUFLOW(2);
                    h->x = h->seac.bsb_x = INDEX(0);
                    h->y = h->seac.bsb_y = 0;
                }
                h->x = (float)RND_ON_READ(h->x);
                h->y = (float)RND_ON_READ(h->y);
                break;
            case tx_endchar:
                h->flags |= SEEN_ENDCHAR;
                return t1cSuccess;
            case t1_moveto:
                /* Deprecated absolute moveto; convert to rmoveto */
                CHKUFLOW(2);
                INDEX(0) -= h->x;
                INDEX(1) -= h->y;
                /* Fall through */
            case tx_rmoveto:
                CHKUFLOW(2);
                if (h->flags & (long)IN_FLEX) {
                    CHKFLEX(2);
                    PUSHFLEX(INDEX(0));
                    PUSHFLEX(INDEX(1));
                } else
                    callbackMove(h, INDEX(0), INDEX(1));
                break;
            case tx_hmoveto:
                CHKUFLOW(1);
                if (h->flags & (long)IN_FLEX) {
                    CHKFLEX(2);
                    PUSHFLEX(INDEX(0));
                    PUSHFLEX(0);
                } else
                    callbackMove(h, INDEX(0), 0);
                break;
            case tx_vhcurveto:
                CHKUFLOW(4);
                callbackCurve(h,
                              0, INDEX(0),
                              INDEX(1), INDEX(2),
                              INDEX(3), 0);
                break;
            case tx_hvcurveto:
                CHKUFLOW(4);
                callbackCurve(h,
                              INDEX(0), 0,
                              INDEX(1), INDEX(2),
                              0, INDEX(3));
                break;
            default:
                /* 1 byte number */
                CHKOFLOW(1);
                PUSH(byte0 - 139);
                continue;
            case 247:
            case 248:
            case 249:
            case 250:
                /* Positive 2 byte number */
                CHKOFLOW(1);
                CHKBYTE(h);
                PUSH(108 + 256 * (byte0 - 247) + *next++);
                continue;
            case 251:
            case 252:
            case 253:
            case 254:
                /* Negative 2 byte number */
                CHKOFLOW(1);
                CHKBYTE(h);
                PUSH(-108 - 256 * (byte0 - 251) - *next++);
                continue;
            case 255:
                /* 5 byte number */
                CHKOFLOW(1);
                {
                    long value;
                    CHKBYTE(h);
                    value = *next++;
                    CHKBYTE(h);
                    value = value << 8 | *next++;
                    CHKBYTE(h);
                    value = value << 8 | *next++;
                    CHKBYTE(h);
                    value = value << 8 | *next++;
#if LONG_MAX > 2147483647
                    /* long greater that 4 bytes; handle negative range */
                    if (value > 2417483647)
                        value -= 4294967296;
#endif
                    PUSH(value);
                }
                continue;
        }                 /* End: switch (byte0) */
        h->stack.cnt = 0; /* Clear stack */
    }                     /* End: while (cstr < end) */
}

/* Parse Type 1 charstring. */
int t1cParse(long offset, t1cAuxData *aux, abfGlyphCallbacks *glyph) {
    struct t1cCtx h = {0};
    int retVal;

    /* Initialize */
    h.flags = PEND_MOVETO;
    h.stack.cnt = 0;
    h.x = h.lsb.x = 0;
    h.y = h.lsb.y = 0;
    h.le_start.useLEStart = 0;
    h.le_start.x = 0;
    h.le_start.y = 0;
    h.cntr.cnt = 0;
    h.seac.phase = seacNone;
    h.aux = aux;
    h.glyph = glyph;
    h.maxOpStack = T1_MAX_OP_STACK;

    aux->bchar = 0;
    aux->achar = 0;

    if (aux->flags & T1C_USE_MATRIX) {
        if ((fabs(1 - aux->matrix[0]) > 0.0001) ||
            (fabs(1 - aux->matrix[3]) > 0.0001) ||
            (aux->matrix[1] != 0) ||
            (aux->matrix[2] != 0) ||
            (aux->matrix[4] != 0) ||
            (aux->matrix[5] != 0))

        {
            int i;
            h.flags |= USE_MATRIX;
            h.flags |= USE_GLOBAL_MATRIX;
            for (i = 0; i < 6; i++) {
                h.transformMatrix[i] = aux->matrix[i];
            }
        }
    }

    retVal = t1Decode(&h, offset);

    // Clear transform flag,
    if (h.flags & USE_MATRIX)
        h.flags &= ~USE_MATRIX;

    return retVal;
}

/* ------------------------- Charstring Decryption ------------------------- */

/* Decrypt Type 1 charstring. */
int t1cDecrypt(int lenIV, long *length, char *cipher, char *plain) {
    if (lenIV < 0 || lenIV > *length)
        /* Invalid lenIV */
        return t1cErrBadLenIV;
    else {
        /* Encrypted */
        unsigned char *c = (unsigned char *)cipher;
        unsigned char *p = (unsigned char *)plain;
        unsigned short r = 4330; /* Initial state */
        long cnt = *length -= lenIV;

        /* Prime state from random initial bytes */
        while (lenIV--)
            r = (*c++ + r) * 52845 + 22719;

        /* Decrypt and copy bytes */
        while (cnt--) {
            unsigned char c1 = *c++;
            *p++ = c1 ^ (r >> 8);
            r = (c1 + r) * 52845 + 22719;
        }
    }
    return t1cSuccess;
}

/* Decrypt protected (doubly-encrypted CJK fonts) charstring. */
int t1cUnprotect(int lenIV, long *length, char *cipher, char *plain) {
    if (lenIV < 0 || lenIV > *length)
        /* Invalid lenIV */
        return t1cErrBadLenIV;
    else {
        /* Doubly-encrypted */
        unsigned char single;
        unsigned char *c = (unsigned char *)cipher;
        unsigned char *p = (unsigned char *)plain;
        unsigned short r1 = 4330;  /* Single decryption key */
        unsigned short r2 = 54261; /* Double decryption key */
        long cnt = *length -= lenIV;

        /* Prime state from random initial bytes */
        while (lenIV--) {
            single = *c ^ (r2 >> 8);
            r2 = (*c++ + r2) * 16477 + 21483;
            r1 = (single + r1) * 52845 + 22719;
        }

        /* Double decrypt and copy bytes */
        while (cnt--) {
            unsigned char c1 = *c++;
            single = c1 ^ (r2 >> 8);
            r2 = (c1 + r2) * 16477 + 21483;
            *p++ = single ^ (r1 >> 8);
            r1 = (single + r1) * 52845 + 22719;
        }
    }
    return t1cSuccess;
}

/* Get version numbers of libraries. */
void t1cGetVersion(ctlVersionCallbacks *cb) {
    if (cb->called & 1 << T1C_LIB_ID)
        return; /* Already enumerated */

    /* This library */
    cb->getversion(cb, T1C_VERSION, "t1cstr");

    /* Record this call */
    cb->called |= 1 << T1C_LIB_ID;
}

/* Map error code to error string. */
char *t1cErrStr(int err_code) {
    static char *errstrs[] =
        {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string) string,
#include "t1cerr.h"
        };
    return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs)) ? "unknown error" : errstrs[err_code];
}

/* ----------------------------- Debug Support ----------------------------- */

#if T1C_DEBUG

#include <stdio.h>

static void dbt1cstack(t1cCtx h) {
    printf("--- stack\n");
    if (h->stack.cnt == 0)
        printf("empty\n");
    else {
        int i;
        for (i = 0; i < h->stack.cnt; i++)
            printf("[%d]=%g ", i, h->stack.array[i]);
        printf("\n");
    }
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
    dbuse(0, dbt1cstack);
}

#endif /* T1C_DEBUG */
