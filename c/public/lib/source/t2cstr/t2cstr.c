/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type 2 charstring support.
 */

#if defined(_WIN32) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES /* Needed to define M_PI under Windows */
#endif

#include "t2cstr.h"
#include "txops.h"
#include "ctutil.h"
#include "varread.h"
#include "supportexcept.h"

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

/* Make operator for internal use */
#define t2_cntroff t2_reservedESC33

#define MAX_RECURSION_DEPTH 1000

enum /* seac operator conversion phase */
{
    seacNone,          /* No seac conversion */
    seacBase,          /* In base char */
    seacAccentPreMove, /* In accent char before first moveto */
    seacAccentPostMove /* In accent char after first moveto */
};

typedef struct /* Stem data */
{
    union {
        float edge0;
        abfBlendArg edge0v;
    };
    union {
        float edge1;
        abfBlendArg edge1v;
    };
    short flags; /* Uses stem flags defined in absfont.h */
} Stem;

/* Module context */
typedef struct _t2cCtx *t2cCtx;
struct _t2cCtx {
    long flags;                   /* Control flags */
#define PEND_WIDTH        (1<<0)  /* Flags width pending */
#define PEND_MASK         (1<<1)  /* Flags hintmask pending */
#define SEEN_ENDCHAR      (1<<2)  /* Flags endchar operator seen */
#define SEEN_BLEND        (1<<8)  /* Seen blend operator: need to round position after each op. */
#define USE_MATRIX        (1<<9)  /* Apply transform*/
#define USE_GLOBAL_MATRIX (1<<10) /* Transform is applied to entire charstring */
#define FLATTEN_BLEND     (1<<13) /* Flag that we are flattening a CFF2 charstring. */
#define SEEN_CFF2_BLEND   (1<<14) /* Seen CFF2 blend operator. */
#define IS_CFF2           (1<<15) /* CFF2 charstring */
    struct /* Operand stack */
    {
        long cnt;
        float array[CFF2_MAX_OP_STACK];
        unsigned short numRegions;
        long blendCnt;
        abfOpEntry blendArray[CFF2_MAX_OP_STACK];
        abfBlendArg blendArgs[T2_MAX_STEMS];
    } stack;
    long maxOpStack;
    float BCA[TX_BCA_LENGTH]; /* BuildCharArray */
    float x;                  /* Path x-coord */
    float y;                  /* Path y-coord */
    int subrDepth;
    float transformMatrix[6];
    struct /* Stem hints */
    {
        long cnt;
        Stem array[T2_MAX_STEMS];
    } stems;
    struct /* hint/cntrmask */
    {
        short state;                           /* cntrmask state */
        short length;                          /* Number of bytes in mask op */
        short unused;                          /* Mask unused bits in last byte of mask */
        unsigned char bytes[T2_MAX_STEMS / 8]; /* Current mask */
    } mask;
    struct /* seac conversion data */
    {
        float adx;
        float ady;
        int phase;
    } seac;
    struct /* Source data */
    {
        char *buf;      /* Buffer */
        long length;    /* Buffer length */
        long offset;    /* offset in file */
        long endOffset; /* offset in file for end of charstring */
        long left;      /* Bytes remaining in charstring */
    } src;
    short LanguageGroup;
    t2cAuxData *aux;                                /* Auxiliary parse data */
    unsigned short gid;                             /* glyph ID */
    unsigned short regionIndices[CFF2_MAX_MASTERS]; /* variable font region indices */
    cff2GlyphCallbacks *cff2;                       /* CFF2 font callbacks */
    abfGlyphCallbacks *glyph;                       /* Glyph callbacks */
    ctlMemoryCallbacks *mem;                        /* Glyph callbacks */
    struct                                          /* Error handling */
    {
        _Exc_Buf env;
    } err;
};

/* Check stack contains at least n elements. */
#define CHKUFLOW(h, n)                                       \
    do {                                                     \
        if (h->stack.cnt < (n)) return t2cErrStackUnderflow; \
    } while (0)

/* Check stack has room for n elements. */
#define CHKOFLOW(h, n)                                                                                                                                              \
    do {                                                                                                                                                            \
        if (((h->flags & IS_CFF2) && ((h->stack.blendCnt) + (n) > CFF2_MAX_OP_STACK)) || (h->stack.cnt) + (n) > h->maxOpStack) return t2cErrStackOverflow; \
    } while (0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define INDEX_BLEND(i) (h->stack.blendArray[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v)                                                                                       \
    {                                                                                                 \
        if (h->flags & IS_CFF2) h->stack.blendArray[h->stack.blendCnt++].value = (float)(v); \
        h->stack.array[h->stack.cnt++] = (float)(v);                                                  \
    }

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

/* Transform coordinates by matrix. */
#define RND(v) ((float)floor((v) + 0.5))
#define TX(x, y) \
    RND(h->transformMatrix[0] * (x) + h->transformMatrix[2] * (y) + h->transformMatrix[4])
#define TY(x, y) \
    RND(h->transformMatrix[1] * (x) + h->transformMatrix[3] * (y) + h->transformMatrix[5])

/* Scale coordinates by matrix. */
#define SX(x) RND(h->transformMatrix[0] * (x))
#define SY(y) RND(h->transformMatrix[3] * (y))

static int t2Decode(t2cCtx h, long offset, int depth);
static void convertToAbsolute(t2cCtx h, float x1, float y1, abfBlendArg *blendArgs, int num);
static void copyBlendArgs(t2cCtx h, abfBlendArg *blendArg, abfOpEntry *opEntry);

/* ------------------------------- Error handling -------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(t2cCtx h, char *fmt, va_list ap) {
    char text[500];

    if (h->aux->dbg == NULL)
        return; /* Debug stream not available */

    VSPRINTF_S(text, sizeof(text), fmt, ap);
    (void)h->aux->stm->write(h->aux->stm, h->aux->dbg, strlen(text), text);
}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(t2cCtx h, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vmessage(h, fmt, ap);
    va_end(ap);
}

/* Handle fatal error. */
static void fatal(t2cCtx h, int err_code) {
    message(h, "%s", t2cErrStr(err_code));
    RAISE(&h->err.env, err_code, NULL);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(t2cCtx h, size_t size) {
    void *ptr = h->mem->manage(h->mem, NULL, size);
    if (ptr == NULL) {
        fatal(h, t2cErrMemory);
    } else {
        /* Safety initialization */
        memset(ptr, 0, size);
    }
    return ptr;
}

/* Free memory. */
static void memFree(t2cCtx h, void *ptr) {
    h->mem->manage(h->mem, ptr, 0);
}

/* ------------------------------- Data Input ------------------------------ */

/* Refill input buffer. Return NULL on stream error else pointer to buffer. */
static unsigned char *refill(t2cCtx h, unsigned char **end) {
    long int offset;
    /* Read buffer */
    /* 64-bit warning fixed by cast here HO */
    h->src.length = (long)h->aux->stm->read(h->aux->stm, h->aux->src, &h->src.buf);
    if (h->src.length == 0 && !(h->flags & IS_CFF2)) {
        char *errMsg;
        errMsg = strerror(errno);
        message(h, "%s", errMsg);
        return NULL; /* Stream error */
    }

    /* Update offset of next read */
    offset = h->src.offset + h->src.length;
    if (offset >= h->src.endOffset) {
        h->src.length = h->src.endOffset - h->src.offset;
        h->src.offset = h->src.offset + h->src.length;
    } else
        h->src.offset = offset;

    /* Set buffer limit */
    *end = (unsigned char *)h->src.buf + h->src.length;

    return (unsigned char *)h->src.buf;
}

/* Seek to offset on source stream. */
static int srcSeek(t2cCtx h, long offset) {
    if (h->aux->stm->seek(h->aux->stm, h->aux->src, offset))
        return 1;
    h->src.offset = offset;
    return 0;
}

/* Check for the first of possibly multiple bytes for an operator or operand.
   If we're currently empty, but more bytes are available, refill from buffer.
   If this is a CFF2 font and we're completely out of bytes,
   treat this condition as equivalent to an endchar in CFF. */

#define CHECK_FIRST_BYTE(h)                                         \
    do                                                              \
        if (next == end) {                                          \
            if (h->src.offset >= h->src.endOffset) {                \
                if (h->flags & IS_CFF2)                             \
                    goto do_endchar;                                \
                return t2cErrSrcStream;                             \
            }                                                       \
            next = refill(h, &end);                                 \
            if (next == NULL)                                       \
                return t2cErrSrcStream;                             \
        }                                                           \
    while (0)

/* Check for a byte that follows the first byte of an operator or operand.
   If we're currently empty, but more bytes are available, refill from buffer.
   If we're completely out of bytes, throw an error.                         */

#define CHECK_SUBSEQUENT_BYTE(h)                                    \
    do                                                              \
        if (next == end) {                                          \
            if (h->src.offset >= h->src.endOffset) {                \
                return t2cErrSrcStream;                             \
            }                                                       \
            next = refill(h, &end);                                 \
            if (next == NULL)                                       \
                return t2cErrSrcStream;                             \
        }                                                           \
    while (0)

/* ------------------------------- Callbacks ------------------------------- */

/* Compute and callback width. Return non-0 to end parse else 0. */
static int callbackWidth(t2cCtx h, int odd_args) {
    if (h->flags & PEND_WIDTH) {
        float width;

        if (h->flags & IS_CFF2) {
            width = h->cff2->getWidth(h->cff2, h->gid);
        } else {
            width = (odd_args == (h->stack.cnt & 1)) ? INDEX(0) + h->aux->nominalWidthX : h->aux->defaultWidthX;
            if (h->flags & USE_MATRIX)
                width = SX(width);
            else if (h->flags & SEEN_BLEND)
                width = RND(width);
        }

        h->glyph->width(h->glyph, width);
        h->flags &= ~PEND_WIDTH;
        if (h->aux->flags & T2C_WIDTH_ONLY) {
            /* Only width required; pretend we've seen endchar operator */
            h->flags |= SEEN_ENDCHAR;
            return 1;
        }
    }
    return 0;
}

/* Test if cntrmask contains valid stem3 counters. */
static int validStem3(t2cCtx h) {
    int i;
    int hcntrs = 0;
    int vcntrs = 0;

    /* Count counters */
    for (i = 0; i < h->mask.length; i++) {
        int j;
        unsigned char byte = h->mask.bytes[i];
        for (j = i * 8; byte; j++) {
            if (byte & 0x80) {
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
static void callbackOp(t2cCtx h, int op) {
    h->glyph->genop(h->glyph, h->stack.cnt, h->stack.array, op);
}

static void callBackStem(t2cCtx h, Stem *stem, int cntr, int flags) {
    if ((h->glyph->stemVF != NULL) && (h->flags & IS_CFF2)) {
        if (cntr)
            flags |= ABF_CNTR_STEM;
        h->glyph->stemVF(h->glyph, flags, &stem->edge0v, &stem->edge1v);
    } else {
        float edge0 = stem->edge0;
        float edge1 = stem->edge1;
        if (h->flags & USE_MATRIX) {
            if (stem->flags & ABF_VERT_STEM) {
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

        if (cntr)
            flags |= ABF_CNTR_STEM;
        h->glyph->stem(h->glyph, flags, edge0, edge1);
    }
}

/* Callback stems in mask. */
static void callbackStems(t2cCtx h, int cntr) {
    int i;
    int flags;

    if (cntr) {
        for (i = 0; i < h->mask.length; i++)
            if (h->mask.bytes[i] != 0)
                goto set_flags;

        /* Null cntrmask; turn off global coloring */
        callbackOp(h, t2_cntroff);
        return;

    set_flags:
        flags = ABF_NEW_GROUP;
    } else
        flags = (h->flags & PEND_MASK) ? 0 : ABF_NEW_HINTS;

    for (i = 0; i < h->mask.length; i++) {
        int j;
        unsigned char byte = h->mask.bytes[i];
        for (j = i * 8; byte; j++) {
            if (byte & 0x80) {
                /* Callback stem */
                Stem *stem = &h->stems.array[j];
                flags |= stem->flags;
                callBackStem(h, stem, cntr, flags);
                flags = 0;
            }
            byte <<= 1;
        }
    }
    if (!cntr)
        h->flags &= ~PEND_MASK;
}

/* Determine how to handle first cntrmask and save it. */
static void savePendCntr(t2cCtx h, int cntr) {
    if (cntr || h->LanguageGroup == 1 || !validStem3(h))
        /* We have:
           - 2 or more cntrmasks
           - font sets LanguageGroup 1
           - saw a cntron operator
           - don't have valid stem3 cntrs;
             do global coloring */
        callbackStems(h, 1);
    else {
        /* Do stem3; mark stem3 stems */
        int i;
        for (i = 0; i < h->mask.length; i++) {
            int j;
            unsigned char byte = h->mask.bytes[i];
            for (j = i * 8; byte; j++) {
                if (byte & 0x80)
                    h->stems.array[j].flags |= ABF_STEM3_STEM;
                byte <<= 1;
            }
        }
    }

    h->mask.state = 2; /* Don't do this again */
}

/* Callback path move. */
static void callbackMove(t2cCtx h, float dx, float dy) {
    int flags;
    float x, y;

    x = h->x + dx;
    y = h->y + dy;
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

    if (h->mask.state == 1)
        savePendCntr(h, 0);

    if (h->seac.phase == seacAccentPreMove) {
        /* Accent component moveto; if no mask force hintsubs */
        flags = ABF_NEW_HINTS;
        h->seac.phase = seacAccentPostMove;
    } else
        flags = 0;

    if (h->glyph->stem != NULL && (h->flags & PEND_MASK)) {
        /* No mask before first move; callback initial hints */
        int i;
        for (i = 0; i < h->stems.cnt; i++) {
            /* Callback stem */
            Stem *stem = &h->stems.array[i];
            flags |= stem->flags;
            callBackStem(h, stem, 0, flags);
            flags = 0;
        }
        h->flags &= ~PEND_MASK;
    }

    x = roundf(x * 100) / 100;
    y = roundf(y * 100) / 100;
    h->x = x;
    h->y = y;

    if ((h->flags & IS_CFF2) && (h->glyph->moveVF != NULL)) {
        abfBlendArg *blendArgs = &(h->stack.blendArgs[0]);
        convertToAbsolute(h, x, y, blendArgs, 2);
        h->glyph->moveVF(h->glyph, blendArgs, blendArgs + 1);
    } else {
        if (h->flags & USE_MATRIX)
            h->glyph->move(h->glyph, TX(h->x, h->y), TY(h->x, h->y));
        else if (h->flags & SEEN_BLEND)
            h->glyph->move(h->glyph, RND(h->x), RND(h->y));
        else
            h->glyph->move(h->glyph, h->x, h->y);
    }
}

/* Callback path line. */
static void callbackLine(t2cCtx h, float dx, float dy) {
    h->x += dx;
    h->y += dy;
    h->x = roundf(h->x * 100) / 100;
    h->y = roundf(h->y * 100) / 100;

    if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL)) {
        abfBlendArg *blendArgs = &(h->stack.blendArgs[0]);
        convertToAbsolute(h, h->x, h->y, blendArgs, 2);
        h->glyph->lineVF(h->glyph, blendArgs, blendArgs + 1);
    } else {
        if (h->flags & USE_MATRIX)
            h->glyph->line(h->glyph, TX(h->x, h->y), TY(h->x, h->y));
        else if (h->flags & SEEN_BLEND)
            h->glyph->line(h->glyph, RND(h->x), RND(h->y));
        else
            h->glyph->line(h->glyph, h->x, h->y);
    }
}

/* Callback path curve. */
static void callbackCurve(t2cCtx h,
                          float dx1, float dy1,
                          float dx2, float dy2,
                          float dx3, float dy3) {
    float x1, y1, x2, y2, x3, y3;

    x1 = h->x + dx1;
    y1 = h->y + dy1;
    x2 = x1 + dx2;
    y2 = y1 + dy2;
    x3 = x2 + dx3;
    y3 = y2 + dy3;

    x3 = roundf(x3 * 100) / 100;
    y3 = roundf(y3 * 100) / 100;
    h->x = x3;
    h->y = y3;

    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))

    {
        abfBlendArg *blendArgs = &(h->stack.blendArgs[0]);
        convertToAbsolute(h, x1, y1, blendArgs, 6);
        h->glyph->curveVF(h->glyph, blendArgs, blendArgs + 1,
                          blendArgs + 2, blendArgs + 3,
                          blendArgs + 4, blendArgs + 5);
    } else {
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
}

/* Callback path flex. */
static void callbackFlex(t2cCtx h,
                         float dx1, float dy1,
                         float dx2, float dy2,
                         float dx3, float dy3,
                         float dx4, float dy4,
                         float dx5, float dy5,
                         float dx6, float dy6,
                         float depth) {
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
        /* Callback as flex hint */
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
        x6 = roundf(x6 * 100) / 100;
        y6 = roundf(y6 * 100) / 100;
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

static void convertStemToAbsolute(t2cCtx h, float lastEdge, abfBlendArg *blendArgs) {
    /* Convert only the default value. At some point, need to actually convert the region
     deltas to absolute values, but this requires referencing the current point and the region weight map.
     */

    blendArgs->value += lastEdge;
}

/* Add stems to stem list. Return 1 on stem overflow else 0;
 This is called only on hstem(hm), vstem(hm), or tx_mask, so all args on the stack are stems.
 */
static int addStems(t2cCtx h, int vert) {
    int i;
    Stem *stem;
    float lastedge = 0;

    if (h->stems.cnt + h->stack.cnt / 2 > T2_MAX_STEMS)
        return 1;

    /* The "i = h->stack.cnt & 1" clause lets it skip the width value, if there is one. */
    if ((h->flags & IS_CFF2) && (h->glyph->stemVF != NULL)) {
        for (i = h->stack.cnt & 1; i < h->stack.cnt - 1; i += 2) {
            abfBlendArg *blendArgs = &h->stack.blendArgs[i];
            abfOpEntry *opEntry = &h->stack.blendArray[i];
            stem = &h->stems.array[h->stems.cnt++];
            copyBlendArgs(h, blendArgs, opEntry);
            stem->edge0v = h->stack.blendArgs[i];

            blendArgs = &h->stack.blendArgs[i + 1];
            opEntry = &h->stack.blendArray[i + 1];
            copyBlendArgs(h, blendArgs, opEntry);
            stem->edge1v = h->stack.blendArgs[i + 1];

            convertStemToAbsolute(h, lastedge, &stem->edge0v);
            convertStemToAbsolute(h, stem->edge0v.value, &stem->edge1v);
            lastedge = stem->edge1v.value;
            stem->flags = vert ? ABF_VERT_STEM : 0;
        }
    } else {
        for (i = h->stack.cnt & 1; i < h->stack.cnt - 1; i += 2) {
            stem = &h->stems.array[h->stems.cnt++];
            stem->edge0 = lastedge + INDEX(i + 0);
            if (h->seac.phase > seacBase)
                stem->edge0 += vert ? h->seac.adx : h->seac.ady;
            stem->edge1 = stem->edge0 + INDEX(i + 1);
            lastedge = stem->edge1;
            stem->flags = vert ? ABF_VERT_STEM : 0;
        }
    }

    /* Compute mask length and unused bit mask */
    h->mask.length = (short)((h->stems.cnt + 7) / 8);
    h->mask.unused = ~(~0 << (h->mask.length * 8 - h->stems.cnt));

    return 0;
}

/* Callback hint/cntrmask. Return 0 on success else error code. */
static int callbackMask(t2cCtx h, int cntr,
                        unsigned char **next, unsigned char **end) {
    int i;

    if (h->mask.state == 1)
        savePendCntr(h, cntr);

    if (h->stack.cnt > 1) {
        /* Stem args on stack; must be omitted vstem(hm) op */
        if (addStems(h, 1))
            return t2cErrStemOverflow;
    }

    /* Check for invalid mask */
    if (h->mask.length <= 0 || h->mask.length > T2_MAX_STEMS / 8)
        return t2cErrHintmask;

    /* Read mask */
    for (i = 0; i < h->mask.length; i++) {
        if (*next == *end) {
            *next = refill(h, end);
            if (*next == NULL)
                return t2cErrSrcStream;
        }
        h->mask.bytes[i] = *(*next)++;
    }

    if (h->mask.bytes[h->mask.length - 1] & h->mask.unused) {
        message(h, "invalid hint/cntr mask. Correcting...");
        h->mask.bytes[h->mask.length - 1] &= ~h->mask.unused; /* clear the unused bits */
    }

    if (h->glyph->stem == NULL)
        return 0; /* No stem callback */

    if (cntr && h->mask.state == 0) {
        /* Save first cntrmask */
        h->mask.state = 1;
        return 0;
    }

    callbackStems(h, cntr);
    return 0;
}

/* Callback seac operator. */
static void callback_seac(t2cCtx h, float adx, float ady, int bchar, int achar) {
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

/* Parse seac component glyph. */
static int parseSeacComponent(t2cCtx h, int stdcode, int depth) {
    long offset;

    if (depth > MAX_RECURSION_DEPTH) {
        fatal(h, t2cErrMaxRecursion);
    }

    if (stdcode < 0 || stdcode > 255)
        return t2cErrBadSeacComp;
    offset = h->aux->getStdEncGlyphOffset(h->aux->ctx, stdcode);
    if (offset == -1)
        return t2cErrBadSeacComp;

    h->stack.cnt = 0;
    return t2Decode(h, offset, depth + 1);
}

/* Unbias subroutine number. Return -1 on error else subroutine number. */
static long unbias(long arg, long nSubrs) {
    if (nSubrs < 1240)
        arg += 107;
    else if (nSubrs < 33900)
        arg += 1131;
    else
        arg += 32768;
    return (arg < 0 || arg >= nSubrs) ? -1 : arg;
}

/* Reverse stack elements between index i and j (i < j). */
static void reverse(t2cCtx h, int i, int j) {
    while (i < j) {
        float tmp = h->stack.array[i];
        h->stack.array[i++] = h->stack.array[j];
        h->stack.array[j--] = tmp;
    }
}

/* Support undocumented blend operator. Retained for multiple master font
   substitution. Assumes 4 masters. Return 0 on success else error code. */
/* Note: there is no command line support to set a WV for Type 2, but you can
   compile one for testing by editing t2cParse(), below */

static int handleBlend(t2cCtx h) {
    /* When the 'blend operator is encountered, the last items on the stack are
     the blend operands. These are, in order:
     numMaster operands from the default font
     (numMaster-1)* numBlends blend delta operands
     numBlends value
     
     We find the first blend operand item, which is the first value from the
     default font. For each of the numBlends default font values, we allocate opeEntry.blendValues
     to hold the region font values for that operand index.
     We then copy the region delta values to opEntry.blendValues.      */
    int result = 0;
    abfOpEntry *opEntry;
    int i = 0;
    int numBlends;
    int numRegions = h->stack.numRegions;
    int numDeltaBlends;
    int numTotalBlends;
    int firstItemIndex;

    CHKUFLOW(h, 1);
    numBlends = (int)INDEX(h->stack.cnt - 1);
    numDeltaBlends = numBlends * numRegions;
    numTotalBlends = numBlends + numDeltaBlends;
    if (numBlends < 0 || numDeltaBlends < 0 || numTotalBlends < 0 /*check signed int overflow*/)
        return t2cErrStackUnderflow;

    // pop off the numBlends value
    h->stack.cnt--;
    h->stack.blendCnt--;

    CHKUFLOW(h, numTotalBlends);
    firstItemIndex = (h->stack.blendCnt - numTotalBlends);
    if (firstItemIndex < 0)
        return t2cErrStackUnderflow;

    opEntry = &(h->stack.blendArray[firstItemIndex]);

    if (h->stack.blendCnt < numDeltaBlends || h->stack.cnt < numDeltaBlends)
        return t2cErrStackUnderflow;

    if (h->flags & FLATTEN_BLEND) {
        /* Blend values on the blend stack and replace the default values on the regular stack with the results.
         */
        for (i = 0; i < numBlends; i++) {
            float val = opEntry[i].value;
            int r;

            for (r = 0; r < h->stack.numRegions; r++) {
                int index = (i * h->stack.numRegions) + r + (h->stack.blendCnt - numDeltaBlends);
                val += INDEX_BLEND(index).value * h->aux->scalars[h->regionIndices[r]];
            }

            h->stack.array[i + h->stack.cnt - (numBlends + numDeltaBlends)] = val;
        }

        h->stack.cnt -= numDeltaBlends;
        h->stack.blendCnt -= numDeltaBlends;
    } else {
        int stackIndex = h->stack.blendCnt - numDeltaBlends;

        for (i = 0; i < numBlends; i++) {
            int r;
            opEntry->blendValues = (float *)memNew(h, sizeof(float) * numRegions);
            opEntry->numBlends = 1;
            for (r = 0; r < numRegions; r++) {
                float val = INDEX_BLEND(stackIndex++).value;
                opEntry->blendValues[r] = val;
            }
            opEntry++;
        }

        h->stack.cnt -= numDeltaBlends;

        opEntry = &(h->stack.blendArray[h->stack.blendCnt - numDeltaBlends]);
        for (i = 0; i < numDeltaBlends; i++) {
            opEntry->numBlends = 0;
            opEntry++;
        }
        h->stack.blendCnt -= numDeltaBlends;
    }
    return result;
}

static void clearBlendStack(t2cCtx h) {
    if (h->flags & SEEN_CFF2_BLEND) {
        int j;
        for (j = 0; j < h->stack.blendCnt; j++) {
            abfOpEntry *blend = &h->stack.blendArray[j];
            if (blend->blendValues != NULL) {
                memFree(h, blend->blendValues);
                blend->blendValues = NULL;
            }
            blend->numBlends = 0;
        }
        h->flags &= ~SEEN_CFF2_BLEND;
    }
    /* The following needs to be always called, so that h->stack.blendCnt is
     reset to zero even if no blends have been seen. */
    h->stack.blendCnt = 0;
}

static void copyBlendArgs(t2cCtx h, abfBlendArg *blendArg, abfOpEntry *opEntry) {
    /* If the source opEntry is NULL, fill the blendArg with all zeros,
    else copy the values.
    */
    if (opEntry != NULL) {
        blendArg->value = opEntry->value;
        if (opEntry->numBlends) {
            int j;
            blendArg->hasBlend = 1;
            for (j = 0; j < h->stack.numRegions; j++) {
                blendArg->blendValues[j] = opEntry->blendValues[j];
            }
        } else {
            blendArg->hasBlend = 0;
        }
    } else {
        blendArg->hasBlend = 0;
        blendArg->value = 0;
    }
}
static void popBlendArgs2(t2cCtx h, abfOpEntry *dx, abfOpEntry *dy) {
    abfBlendArg *blendArgs = h->stack.blendArgs;
    copyBlendArgs(h, &blendArgs[0], dx);
    copyBlendArgs(h, &blendArgs[1], dy);
}

static void popBlendArgs6(t2cCtx h, abfOpEntry *dx1, abfOpEntry *dy1,
                          abfOpEntry *dx2, abfOpEntry *dy2,
                          abfOpEntry *dx3, abfOpEntry *dy3) {
    abfBlendArg *blendArgs = h->stack.blendArgs;
    copyBlendArgs(h, &blendArgs[0], dx1);
    copyBlendArgs(h, &blendArgs[1], dy1);
    copyBlendArgs(h, &blendArgs[2], dx2);
    copyBlendArgs(h, &blendArgs[3], dy2);
    copyBlendArgs(h, &blendArgs[4], dx3);
    copyBlendArgs(h, &blendArgs[5], dy3);
}

static void convertToAbsolute(t2cCtx h, float x1, float y1, abfBlendArg *blendArgs, int num) {
    /* Convert the delta values in place. */
    int isX = 1;
    float defaultX = x1;
    float defaultY = y1;
    int i;

    for (i = 0; i < num; i++) {
        if (isX) {
            if (i > 1)
                defaultX = blendArgs->value += defaultX;
            else
                blendArgs->value = defaultX;
        } else {
            if (i > 1)
                defaultY = blendArgs->value += defaultY;
            else
                blendArgs->value = defaultY;
        }
        isX = !isX;
        blendArgs++;
    }
}

static void setNumMasters(t2cCtx h) {
    unsigned short vsindex = h->glyph->info->blendInfo.vsindex;
    h->stack.numRegions = var_getIVDRegionCountForIndex(h->aux->varStore, vsindex);
    h->glyph->info->blendInfo.numRegions = h->stack.numRegions;
    if (h->aux->varStore && !var_getIVSRegionIndices(h->aux->varStore, vsindex, h->regionIndices, h->aux->varStore->regionList.regionCount)) {
        message(h, "inconsistent region indices detected in item variation store subtable %d", vsindex);
        h->stack.numRegions = 0;
    }
}

/* Decode Type 2 charstring. Return 0 to continue else error code. */
static int t2Decode(t2cCtx h, long offset, int depth) {
    unsigned char *end;
    unsigned char *next;

    if (depth > MAX_RECURSION_DEPTH) {
        fatal(h, t2cErrMaxRecursion);
    }

    /* Fetch charstring */
    if (srcSeek(h, offset))
        return t2cErrSrcStream;
    next = refill(h, &end);
    if (next == NULL)
        return t2cErrSrcStream;

    for (;;) {
        int i;
        int result;
        int byte0;

        CHECK_FIRST_BYTE(h);
        byte0 = *next++;
        switch (byte0) {
            case tx_reserved0:
            case t2_reserved9:
            case t2_reserved13:
            case tx_reserved17:
                return t2cErrInvalidOp;
            case t2_vsindex:
                CHKUFLOW(h, 1);
                h->glyph->info->blendInfo.vsindex = (unsigned short)POP();
                setNumMasters(h);
                break;
            case tx_hstem:
            /* if not in a library element, falls through to t2_hstemhm */
            case t2_hstemhm:
                if (callbackWidth(h, 1))
                    return t2cSuccess;
                if (addStems(h, 0))
                    return t2cErrStemOverflow;
                break;
            case tx_vstem:
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
                CHKUFLOW(h, 2);
                {
                    float y = POP();
                    float x = POP();
                    if ((h->flags & IS_CFF2) && (h->glyph->moveVF != NULL))
                        popBlendArgs2(h, &INDEX_BLEND(0), &INDEX_BLEND(1));
                    callbackMove(h, x, y);
                }
                break;
            case tx_hmoveto:
                if (callbackWidth(h, 0))
                    return t2cSuccess;
                if ((h->flags & IS_CFF2) && (h->glyph->moveVF != NULL))
                    popBlendArgs2(h, &INDEX_BLEND(0), NULL);
                CHKUFLOW(h, 1);
                callbackMove(h, POP(), 0);
                break;
            case tx_vmoveto:
                if (callbackWidth(h, 0))
                    return t2cSuccess;
                if ((h->flags & IS_CFF2) && (h->glyph->moveVF != NULL))
                    popBlendArgs2(h, NULL, &INDEX_BLEND(0));
                CHKUFLOW(h, 1);
                callbackMove(h, 0, POP());
                break;
            case tx_rlineto:
                CHKUFLOW(h, 2);
                for (i = 0; i < h->stack.cnt - 1; i += 2) {
                    if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL)) popBlendArgs2(h, &INDEX_BLEND(i), &INDEX_BLEND(i + 1));
                    callbackLine(h, INDEX(i + 0), INDEX(i + 1));
                }
                break;
            case tx_hlineto:
            case tx_vlineto:
                CHKUFLOW(h, 1);
                {
                    int horz = byte0 == tx_hlineto;
                    for (i = 0; i < h->stack.cnt; i++)
                        if (horz++ & 1) {
                            if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL))
                                popBlendArgs2(h, &INDEX_BLEND(i), NULL);
                            callbackLine(h, INDEX(i), 0);
                        } else {
                            if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL))
                                popBlendArgs2(h, NULL, &INDEX_BLEND(i));
                            callbackLine(h, 0, INDEX(i));
                        }
                }
                break;
            case tx_rrcurveto:
                CHKUFLOW(h, 6);
                for (i = 0; i < h->stack.cnt - 5; i += 6) {
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1),
                                      &INDEX_BLEND(i + 2), &INDEX_BLEND(i + 3),
                                      &INDEX_BLEND(i + 4), &INDEX_BLEND(i + 5));
                    callbackCurve(h,
                                  INDEX(i + 0), INDEX(i + 1),
                                  INDEX(i + 2), INDEX(i + 3),
                                  INDEX(i + 4), INDEX(i + 5));
                }
                break;
            case tx_callsubr:
                CHKUFLOW(h, 1);
                {
                    long saveoff = (long)(h->src.offset - (end - next));
                    long saveEndOff = h->src.endOffset;
                    long num = unbias((long)POP(), h->aux->subrs.cnt);
                    h->stack.blendCnt--;  // we do not blend subr indices.
                    if (num == -1)
                        return t2cErrCallsubr;
                    if ((num + 1) < h->aux->subrs.cnt)
                        h->src.endOffset = h->aux->subrs.offset[num + 1];
                    else
                        h->src.endOffset = h->aux->subrsEnd;
                    h->subrDepth++;
                    if (h->subrDepth > TX_MAX_SUBR_DEPTH) {
                        message(h, "subr depth: %d\n", h->subrDepth);
                        return t2cErrSubrDepth;
                    }

                    result = t2Decode(h, h->aux->subrs.offset[num], depth + 1);
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
                    CHECK_SUBSEQUENT_BYTE(h);
                    escop = tx_ESC(*next++);
                    switch (escop) {
                        case tx_dotsection:
                            if (!(h->aux->flags & T2C_UPDATE_OPS ||
                                  h->glyph->stem == NULL))
                                callbackOp(h, tx_dotsection);
                            break;
                        case tx_and:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                int b = POP() != 0.0f;
                                int a = POP() != 0.0f;
                                PUSH(a && b);
                            }
                            continue;
                        case tx_or:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                int b = POP() != 0.0f;
                                int a = POP() != 0.0f;
                                PUSH(a || b);
                            }
                            continue;
                        case tx_not:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                int a = POP() == 0.0f;
                                PUSH(a);
                            }
                            continue;
                        case tx_abs:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                float a = POP();
                                PUSH((a < 0.0f) ? -a : a);
                            }
                            continue;
                        case tx_add:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(a + b);
                            }
                            continue;
                        case tx_sub:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(a - b);
                            }
                            continue;
                        case tx_div:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(a / b);
                            }
                            continue;
                        case tx_neg:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                float a = POP();
                                PUSH(-a);
                            }
                            continue;
                        case tx_eq:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(a == b);
                            }
                            continue;
                        case tx_drop:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            h->stack.cnt--;
                            continue;
                        case tx_put:
                            CHKUFLOW(h, 2);
                            {
                                i = (int)POP();
                                if (i < 0 || i >= TX_BCA_LENGTH)
                                    return t2cErrPutBounds;
                                h->BCA[i] = POP();
                            }
                            continue;
                        case tx_get:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                i = (int)POP();
                                if (i < 0 || i >= TX_BCA_LENGTH)
                                    return t2cErrGetBounds;
                                PUSH(h->BCA[i]);
                            }
                            continue;
                        case tx_ifelse:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 4);
                            {
                                float v2 = POP();
                                float v1 = POP();
                                float s2 = POP();
                                float s1 = POP();
                                PUSH((v1 <= v2) ? s1 : s2);
                            }
                            continue;
                        case tx_random:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKOFLOW(h, 1);
                            PUSH(((float)rand() + 1) / ((float)RAND_MAX + 1));
                            continue;
                        case tx_mul:
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(a * b);
                            }
                            continue;
                        case tx_sqrt:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                float a = POP();
                                if (a < 0.0f)
                                    return t2cErrSqrtDomain;
                                PUSH(sqrt(a));
                            }
                            continue;
                        case tx_dup:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            CHKOFLOW(h, 1);
                            {
                                float a = POP();
                                PUSH(a);
                                PUSH(a);
                            }
                            continue;
                        case tx_exch:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                float b = POP();
                                float a = POP();
                                PUSH(b);
                                PUSH(a);
                            }
                            continue;
                        case tx_index:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 1);
                            {
                                i = (int)POP();
                                if (i < 0)
                                    i = 0; /* Duplicate top element */
                                if (i >= h->stack.cnt)
                                    return t2cErrIndexBounds;
                                {
                                    float t = h->stack.array[h->stack.cnt - 1 - i];
                                    PUSH(t);
                                }
                            }
                            continue;
                        case tx_roll:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            CHKUFLOW(h, 2);
                            {
                                int j = (int)POP();
                                int n = (int)POP();
                                int iTop = h->stack.cnt - 1;
                                int iBottom = h->stack.cnt - n;

                                if (n <= 0 || iBottom < 0)
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
                            CHKUFLOW(h, 7);
                            callbackFlex(h,
                                         INDEX(0), 0,
                                         INDEX(1), INDEX(2),
                                         INDEX(3), 0,
                                         INDEX(4), 0,
                                         INDEX(5), -INDEX(2),
                                         INDEX(6), 0,
                                         TX_STD_FLEX_DEPTH);
                            break;
                        case t2_flex:
                            CHKUFLOW(h, 13);
                            callbackFlex(h,
                                         INDEX(0), INDEX(1),
                                         INDEX(2), INDEX(3),
                                         INDEX(4), INDEX(5),
                                         INDEX(6), INDEX(7),
                                         INDEX(8), INDEX(9),
                                         INDEX(10), INDEX(11),
                                         INDEX(12));
                            break;
                        case t2_hflex1:
                            CHKUFLOW(h, 9);
                            {
                                float dy1 = INDEX(1);
                                float dy2 = INDEX(3);
                                float dy5 = INDEX(7);
                                callbackFlex(h,
                                             INDEX(0), dy1,
                                             INDEX(2), dy2,
                                             INDEX(4), 0,
                                             INDEX(5), 0,
                                             INDEX(6), dy5,
                                             INDEX(8), -(dy1 + dy2 + dy5),
                                             TX_STD_FLEX_DEPTH);
                            }
                            break;
                        case t2_flex1:
                            CHKUFLOW(h, 11);
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
                                if (ABS(dx) > ABS(dy)) {
                                    dx = INDEX(10);
                                    dy = -dy;
                                } else {
                                    dx = -dx;
                                    dy = INDEX(10);
                                }
                                callbackFlex(h,
                                             dx1, dy1,
                                             dx2, dy2,
                                             dx3, dy3,
                                             dx4, dy4,
                                             dx5, dy5,
                                             dx, dy,
                                             TX_STD_FLEX_DEPTH);
                            }
                            break;
                        case t2_cntron:
                            if (h->flags & IS_CFF2)
                                return t2cErrInvalidOp;
                            h->LanguageGroup = 1; /* Turn on global coloring */
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
                        default:
                            return t2cErrInvalidOp;
                    } /* End: switch (escop) */
                }     /* End: case tx_escape: */
                break;

            case tx_endchar:
            do_endchar:
                if (callbackWidth(h, 1))
                    return t2cSuccess;
                if ((h->subrDepth > 0) && (h->flags & IS_CFF2))
                    return t2cSuccess; /* CFF2 subrs don't have a return char. */

                if (h->stack.cnt > 1) {
                    CHKUFLOW(h, 4);

                    /* Save arguments */
                    h->aux->achar = (unsigned char)POP();
                    h->aux->bchar = (unsigned char)POP();
                    h->seac.ady = POP();
                    h->seac.adx = POP();

                    if (h->aux->flags & T2C_UPDATE_OPS) {
                        /* Parse base component */
                        h->seac.phase = seacBase;
                        result = parseSeacComponent(h, h->aux->bchar, depth + 1);
                        if (result)
                            return result;

                        h->flags &= ~SEEN_ENDCHAR;
                        h->stems.cnt = 0;
                        h->mask.length = 0;
                        h->mask.state = 0;
                        h->x = h->seac.adx;
                        h->y = h->seac.ady;
                        h->x = roundf(h->x * 100) / 100;
                        h->y = roundf(h->y * 100) / 100;

                        /* Parse accent component */
                        h->seac.phase = seacAccentPreMove;
                        result = parseSeacComponent(h, h->aux->achar, depth + 1);
                        if (result)
                            return result;
                    } else
                        /* Callback seac data */
                        callback_seac(h, h->seac.adx, h->seac.ady,
                                      h->aux->bchar, h->aux->achar);
                }
                h->flags |= SEEN_ENDCHAR;
                return 0;
            case t2_blend: {
                h->flags |= SEEN_CFF2_BLEND;
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
                CHKUFLOW(h, 8);
                for (i = 0; i < h->stack.cnt - 5; i += 6) {
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1),
                                      &INDEX_BLEND(i + 2), &INDEX_BLEND(i + 3),
                                      &INDEX_BLEND(i + 4), &INDEX_BLEND(i + 5));
                    callbackCurve(h,
                                  INDEX(i + 0), INDEX(i + 1),
                                  INDEX(i + 2), INDEX(i + 3),
                                  INDEX(i + 4), INDEX(i + 5));
                }
                if (i < h->stack.cnt - 1) {
                    if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL))
                        popBlendArgs2(h, &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1));
                    callbackLine(h, INDEX(i + 0), INDEX(i + 1));
                }
                break;
            case t2_rlinecurve:
                CHKUFLOW(h, 8);
                for (i = 0; i < h->stack.cnt - 6; i += 2) {
                    if ((h->flags & IS_CFF2) && (h->glyph->lineVF != NULL))
                        popBlendArgs2(h, &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1));
                    callbackLine(h, INDEX(i + 0), INDEX(i + 1));
                }
                if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                    popBlendArgs6(h,
                                  &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1),
                                  &INDEX_BLEND(i + 2), &INDEX_BLEND(i + 3),
                                  &INDEX_BLEND(i + 4), &INDEX_BLEND(i + 5));
                callbackCurve(h,
                              INDEX(i + 0), INDEX(i + 1),
                              INDEX(i + 2), INDEX(i + 3),
                              INDEX(i + 4), INDEX(i + 5));

                break;
            case t2_vvcurveto:
                if ((h->stack.cnt) & 1) {
                    CHKUFLOW(h, 5);
                    i = 0;
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      &INDEX_BLEND(i + 0), &INDEX_BLEND(i + 1),
                                      &INDEX_BLEND(i + 2), &INDEX_BLEND(i + 3),
                                      NULL, &INDEX_BLEND(i + 4));
                    callbackCurve(h,
                                  INDEX(0), INDEX(1),
                                  INDEX(2), INDEX(3),
                                  0, INDEX(4));
                    i = 5;
                } else {
                    CHKUFLOW(h, 4);
                    i = 0;
                }

                /* Add remaining curve(s) */
                for (; i < h->stack.cnt - 3; i += 4) {
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      NULL, &INDEX_BLEND(i + 0),
                                      &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                      NULL, &INDEX_BLEND(i + 3));
                    callbackCurve(h,
                                  0, INDEX(i + 0),
                                  INDEX(i + 1), INDEX(i + 2),
                                  0, INDEX(i + 3));
                }
                break;
            case t2_hhcurveto:
                if ((h->stack.cnt) & 1) {
                    /* Add initial curve */
                    CHKUFLOW(h, 5);
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      &INDEX_BLEND(1), &INDEX_BLEND(0),
                                      &INDEX_BLEND(2), &INDEX_BLEND(3),
                                      &INDEX_BLEND(4), NULL);
                    callbackCurve(h,
                                  INDEX(1), INDEX(0),
                                  INDEX(2), INDEX(3),
                                  INDEX(4), 0);
                    i = 5;
                } else {
                    CHKUFLOW(h, 4);
                    i = 0;
                }

                /* Add remaining curve(s) */
                for (; i < h->stack.cnt - 3; i += 4) {
                    if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                        popBlendArgs6(h,
                                      &INDEX_BLEND(i + 0), NULL,
                                      &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                      &INDEX_BLEND(i + 3), NULL);
                    callbackCurve(h,
                                  INDEX(i + 0), 0,
                                  INDEX(i + 1), INDEX(i + 2),
                                  INDEX(i + 3), 0);
                }
                break;
            case t2_callgsubr:
                CHKUFLOW(h, 1);
                {
                    long saveoff = (long)(h->src.offset - (end - next));
                    long saveEndOff = h->src.endOffset;
                    long num = unbias((long)POP(), h->aux->gsubrs.cnt);
                    h->stack.blendCnt--;  // we do not blend subr indices.
                    if (num == -1)
                        return t2cErrCallgsubr;
                    if ((num + 1) < h->aux->gsubrs.cnt)
                        h->src.endOffset = h->aux->gsubrs.offset[num + 1];
                    else
                        h->src.endOffset = h->aux->gsubrsEnd;

                    h->subrDepth++;
                    if (h->subrDepth > TX_MAX_SUBR_DEPTH) {
                        message(h, "subr depth: %d\n", h->subrDepth);
                        return t2cErrSubrDepth;
                    }

                    result = t2Decode(h, h->aux->gsubrs.offset[num], depth + 1);
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
                CHKUFLOW(h, 4);
                {
                    int adjust = ((h->stack.cnt) & 1) ? 5 : 0;
                    int horz = byte0 == tx_hvcurveto;

                    /* Add initial curve(s) */
                    for (i = 0; i < h->stack.cnt - adjust - 3; i += 4)
                        if (horz++ & 1) {
                            if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                                popBlendArgs6(h,
                                              &INDEX_BLEND(i + 0), NULL,
                                              &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                              NULL, &INDEX_BLEND(i + 3));
                            callbackCurve(h,
                                          INDEX(i + 0), 0,
                                          INDEX(i + 1), INDEX(i + 2),
                                          0, INDEX(i + 3));
                        } else {
                            if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                                popBlendArgs6(h,
                                              NULL, &INDEX_BLEND(i + 0),
                                              &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                              &INDEX_BLEND(i + 3), NULL);
                            callbackCurve(h,
                                          0, INDEX(i + 0),
                                          INDEX(i + 1), INDEX(i + 2),
                                          INDEX(i + 3), 0);
                        }

                    if (adjust) {
                        /* Add last curve */
                        if (horz & 1) {
                            if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                                popBlendArgs6(h,
                                              &INDEX_BLEND(i + 0), NULL,
                                              &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                              &INDEX_BLEND(i + 4), &INDEX_BLEND(i + 3));
                            callbackCurve(h,
                                          INDEX(i + 0), 0,
                                          INDEX(i + 1), INDEX(i + 2),
                                          INDEX(i + 4), INDEX(i + 3));
                        } else {
                            if ((h->flags & IS_CFF2) && (h->glyph->curveVF != NULL))
                                popBlendArgs6(h,
                                              NULL, &INDEX_BLEND(i + 0),
                                              &INDEX_BLEND(i + 1), &INDEX_BLEND(i + 2),
                                              &INDEX_BLEND(i + 3), &INDEX_BLEND(i + 4));
                            callbackCurve(h,
                                          0, INDEX(i + 0),
                                          INDEX(i + 1), INDEX(i + 2),
                                          INDEX(i + 3), INDEX(i + 4));
                        }
                    }
                }
                break;
            case t2_shortint:
                /* 3 byte number */
                CHKOFLOW(h, 1);
                {
                    short value;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = *next++;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = value << 8 | *next++;
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
                CHKOFLOW(h, 1);
                PUSH(byte0 - 139);
                continue;
            case 247:
            case 248:
            case 249:
            case 250:
                /* Positive 2 byte number */
                CHKOFLOW(h, 1);
                CHECK_SUBSEQUENT_BYTE(h);
                PUSH(108 + 256 * (byte0 - 247) + *next);
                next++;
                continue;
            case 251:
            case 252:
            case 253:
            case 254:
                /* Negative 2 byte number */
                CHKOFLOW(h, 1);
                CHECK_SUBSEQUENT_BYTE(h);
                PUSH(-108 - 256 * (byte0 - 251) - *next);
                next++;
                continue;
            case 255:
                /* 5 byte number (16.16 fixed) */
                CHKOFLOW(h, 1);
                {
                    int32_t value;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = *next++;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = value << 8 | *next++;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = value << 8 | *next++;
                    CHECK_SUBSEQUENT_BYTE(h);
                    value = value << 8 | *next++;
                    PUSH(value / 65536.0);
                }
                continue;
        } /* End: switch (byte0) */
        clearBlendStack(h);
        h->stack.cnt = 0; /* Clear stack */
    }                     /* End: while (cstr < end) */

#if 0
    /* for CFF2 Charstrings, we hit the end of the charstring without having seen endchar or return yet. Add it now. */
    if ((h->flags & IS_CFF2) && !(h->flags & SEEN_ENDCHAR))

    {
        /* if this was a subr, do return. */
        if (h->subrDepth > 0)
            return 0;
        else
            goto do_endchar;
    }
#endif
}

/* Parse Type 2 charstring. */
int t2cParse(long offset, long endOffset, t2cAuxData *aux, unsigned short gid, cff2GlyphCallbacks *cff2, abfGlyphCallbacks *glyph, ctlMemoryCallbacks *mem) {
    struct _t2cCtx *h;
    int retVal;

    h = malloc(sizeof(struct _t2cCtx));
    if (h == NULL) {
        retVal = t2cErrMemory;
        return retVal;
    }
    memset(h, 0, sizeof(struct _t2cCtx));

    /* Initialize */
    h->flags = PEND_WIDTH | PEND_MASK;
    if (aux->flags & T2C_IS_CFF2)
        h->flags |= IS_CFF2;
    h->stack.cnt = 0;
    h->stack.blendCnt = 0;
    memset(h->stack.blendArray, 0, sizeof(h->stack.blendArray));
    h->stack.numRegions = 0;
    h->x = 0;
    h->y = 0;
    h->subrDepth = 0;
    h->stems.cnt = 0;
    h->mask.state = 0;
    h->mask.length = 0;
    h->seac.phase = seacNone;
    h->aux = aux;
    h->aux->WV[0] = 0.25f; /* use center of design space by default */
    h->aux->WV[1] = 0.25f;
    h->aux->WV[2] = 0.25f;
    h->aux->WV[3] = 0.25f;
    h->glyph = glyph;
    h->mem = mem;
    h->LanguageGroup = (glyph->info->flags & ABF_GLYPH_LANG_1) != 0;
    h->gid = gid;
    h->cff2 = cff2;
    memset(&h->BCA, 0, sizeof(h->BCA));
    aux->bchar = 0;
    aux->achar = 0;
    if (h->flags & IS_CFF2)
        h->maxOpStack = glyph->info->blendInfo.maxstack;
    else
        h->maxOpStack = T2_MAX_OP_STACK;

    if (aux->flags & T2C_USE_MATRIX) {
        if ((fabs(1 - aux->matrix[0]) > 0.0001) ||
            (fabs(1 - aux->matrix[3]) > 0.0001) ||
            (aux->matrix[1] != 0) ||
            (aux->matrix[2] != 0) ||
            (aux->matrix[4] != 0) ||
            (aux->matrix[5] != 0)) {
            int i;
            h->flags |= USE_MATRIX;
            h->flags |= USE_GLOBAL_MATRIX;
            for (i = 0; i < 6; i++) {
                h->transformMatrix[i] = aux->matrix[i];
            }
        }
    }
    if (aux->flags & T2C_FLATTEN_BLEND) {
        h->flags |= FLATTEN_BLEND;
        h->flags |= SEEN_BLEND;
    }
    h->src.endOffset = endOffset;

    DURING_EX(h->err.env)

    retVal = t2Decode(h, offset, 0);

    HANDLER
    retVal = Exception.Code;
    END_HANDLER

    free(h);

    return retVal;
}

/* Get version numbers of libraries. */
void t2cGetVersion(ctlVersionCallbacks *cb) {
    if (cb->called & 1 << T2C_LIB_ID)
        return; /* Already enumerated */

    /* This library */
    cb->getversion(cb, T2C_VERSION, "t2cstr");

    /* Record this call */
    cb->called |= 1 << T2C_LIB_ID;
}

/* Convert error code to error string. */
char *t2cErrStr(int err_code) {
    static char *errstrs[] =
        {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string) string,
#include "t2cerr.h"
        };
    return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs)) ? "unknown error" : errstrs[err_code];
}
