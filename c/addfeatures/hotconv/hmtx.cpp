/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 *  Horizontal metrics table.
 */

#include "hmtx.h"

#include <cassert>
#include <cfloat>

#include "hotmap.h"

/* --------------------------- Standard Functions -------------------------- */

void hmtxNew(hotCtx g) {
    if (g->ctx.hmtx == nullptr)
        g->ctx.hmtx = new var_hmtx();
}

int hmtxFill(hotCtx g) {
    assert(g->ctx.hmtx != nullptr);

    // If there is already data it's because we read the tables
    // from the file already
    if (g->ctx.hmtx->lsb.size() > 0)
        return g->ctx.hmtx->Fill();

    auto &hmtx = *g->ctx.hmtx;
    /* Fill table */
    uint32_t glyphCount = g->glyphs.size();

    hmtx.advanceWidth.reserve(glyphCount);
    hmtx.lsb.reserve(glyphCount);

    BBox bbox;
    for (size_t i = 0; i < g->glyphs.size(); i++) {
        auto &gl = g->glyphs[i];
        hmtx.advanceWidth.push_back(gl.hAdv);
        // This should only execute for non-variable fonts
        bbox = hotDefaultGlyphBBox(g, i);
        hmtx.lsb.push_back(bbox.left);
    }

    /* Optimize metrics */
    FWord width = hmtx.advanceWidth.back();
    size_t i;
    for (i = hmtx.advanceWidth.size() - 2; i >= 0; i--) {
        if (hmtx.advanceWidth[i] != width)
            break;
    }
    if (i + 2 != hmtx.advanceWidth.size())
        hmtx.advanceWidth.resize(i+2);

    return hmtx.Fill();
}

void hmtxWrite(hotCtx g) {
    g->ctx.hmtx->write(g->vw);
}

void hmtxReuse(hotCtx g) {
    delete g->ctx.hmtx;
    g->ctx.hmtx = nullptr;
}

void hmtxFree(hotCtx g) {
    delete g->ctx.hmtx;
    g->ctx.hmtx = nullptr;
}

static short calcCaretOffset(hotCtx g);

#if HOT_DEBUG
#if 0
static void printCaretOffset(hotCtx g, short caretoff);
#endif
#endif

// hhea data is part of the var_hmtx object
void hheaNew(hotCtx g) {
}

int hheaFill(hotCtx g) {
    auto &header = g->ctx.hmtx->header;

    if (OVERRIDE(g->font.hheaAscender)) {
        header.ascender = g->font.hheaAscender;
    } else {
        // XXX maybe just override if not read in from CFF2 otf
        header.ascender = g->font.TypoAscender.getDefault();
    }

    if (OVERRIDE(g->font.hheaDescender)) {
        header.descender = g->font.hheaDescender;
    } else {
        // XXX maybe just override if not read in from CFF2 otf
        header.descender = g->font.TypoDescender.getDefault();
    }

    if (OVERRIDE(g->font.hheaLineGap)) {
        header.lineGap = g->font.hheaLineGap;
    } else {
        // XXX maybe just override if not read in from CFF2 otf
        header.lineGap = g->font.TypoLineGap.getDefault();
    }

    header.advanceWidthMax = g->font.maxAdv.h;
    header.minLeftSideBearing = g->font.minBearing.left;
    header.minRightSideBearing = g->font.minBearing.right;
    header.xMaxExtent = g->font.maxExtent.h;

    if (g->font.hheaCaretOffset.isInitialized()) {
        header.caretOffset = g->font.hheaCaretOffset.getDefault();
        g->ctx.MVAR->addValue(MVAR_hcof_tag, *g->ctx.locMap, g->font.hheaCaretOffset, g->logger);
    } else {
        header.caretOffset = (g->font.ItalicAngle == 0) ? 0 : calcCaretOffset(g);
    }

    if (g->font.hheaCaretSlopeRise.isInitialized()) {
        header.caretSlopeRise = g->font.hheaCaretSlopeRise.getDefault();
        g->ctx.MVAR->addValue(MVAR_hcrs_tag, *g->ctx.locMap, g->font.hheaCaretSlopeRise, g->logger);
    } else {
        header.caretSlopeRise = (g->font.ItalicAngle == 0) ? 1 : 1000;
    }

    if (g->font.hheaCaretSlopeRun.isInitialized()) {
        header.caretSlopeRun = g->font.hheaCaretSlopeRun.getDefault();
        g->ctx.MVAR->addValue(MVAR_hcrn_tag, *g->ctx.locMap, g->font.hheaCaretSlopeRun, g->logger);
    } else {
        header.caretSlopeRun = (g->font.ItalicAngle == 0) ? 0 : (int16_t)RND(tan(FIX2DBL(-g->font.ItalicAngle) / RAD_DEG) * 1000);
    }

    return 1;
}

void hheaWrite(hotCtx g) {
    g->ctx.hmtx->write_hhea(g->vw);
}

void hheaReuse(hotCtx g) {
}

void hheaFree(hotCtx g) {
}

void hheaSetCaretOffset(hotCtx g, int16_t caretOffset) {
    assert(g->ctx.hmtx != nullptr);
    g->ctx.hmtx->header.caretOffset = caretOffset;
}

// hhea data is part of the var_hmtx object
void HVARNew(hotCtx g) {
}

int HVARFill(hotCtx g) {
    return g->ctx.hmtx->ivs != nullptr && g->ctx.hmtx->ivs->numSubtables() > 0;
}

void HVARWrite(hotCtx g) {
    g->ctx.hmtx->write_HVAR(g->vw);
}

void HVARReuse(hotCtx g) {
}

void HVARFree(hotCtx g) {
}

/* ---- hhea caret offset calculation ---- */

struct costate {      /* Used in caret offset calculation */
    double bboxLeft;  /* Bounds of italic glyph sheared upright */
    double bboxRight;
    double tangent;   /* Tangent of italic angle */
};

/* Check point against current bounds */
static void checkPoint(costate *state, float x, float y) {
    double xSheared = x - y * state->tangent;

    if (xSheared < state->bboxLeft)
        state->bboxLeft = xSheared;
    if (xSheared > state->bboxRight)
        state->bboxRight = xSheared;
}

static int caretoffBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
    cb->info = info;
    return ABF_CONT_RET;
}

/* [cffread path callback] Add moveto to path */
static void caretoffMoveto(abfGlyphCallbacks *cb, float x1, float y1) {
    checkPoint((costate *) cb->direct_ctx, x1, y1);
}

/* [cffread path callback] Add lineto to path */
static void caretoffLineto(abfGlyphCallbacks *cb, float x1, float y1) {
    checkPoint((costate *) cb->direct_ctx, x1, y1);
}

/* [cffread path callback] Add curveto to path */
static void caretoffCurveto(abfGlyphCallbacks *cb, float x1, float y1,
                            float x2, float y2, float x3, float y3) {
    checkPoint((costate *) cb->direct_ctx, x3, y3);
}

/* Calculate caret offset for an italic font. Shear heuristic glyph upright,
   calculate its left (a) and right (b) side-bearings. Return a-(a+b)/2 */
static short calcCaretOffset(hotCtx g) {
    costate state;
    unsigned int i;
    double a;
    double b;
    short caretoff;
    GID gid = GID_UNDEF;
    static abfGlyphCallbacks glyphcb = {
        &state,
        NULL,
        NULL,
        caretoffBeg,
        NULL,
        caretoffMoveto,
        caretoffLineto,
        caretoffCurveto,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    UV uv[] = {
        UV_CARET_OFFSET_1,
        UV_CARET_OFFSET_2,
        UV_CARET_OFFSET_3,
    };

    /* Determine heuristic glyph */
    for (i = 0; i < ARRAY_LEN(uv); i++) {
        if ((gid = mapUV2GID(g, uv[i])) != GID_UNDEF) {
            break;
        }
    }
    if (gid == GID_UNDEF) {
        return 0;
    }

    state.tangent = tan(FIX2DBL(-g->font.ItalicAngle) / RAD_DEG);
    state.bboxLeft = DBL_MAX;
    state.bboxRight = 0;

    if (cfrGetGlyphByTag(g->ctx.cfr, gid, &glyphcb) == cfrErrNoGlyph)
        return 0;

    FWord hAdv = g->glyphs[gid].hAdv;

    if (state.bboxLeft == DBL_MAX) {
        return 0;
    }

    a = state.bboxLeft;
    b = hAdv - state.bboxRight;
    caretoff = (short)RND(a - (a + b) / 2);

#if HOT_DEBUG
#if 0
    printCaretOffset(g, caretoff);
#endif
#endif

    return caretoff;
}

/* ------------------------- Caret offset dump ----------------------------- */

#if HOT_DEBUG
#if 0
#define FIX2INT(f) ((short)(((f) + 0x00008000) >> 16)) /* xxx Doesn't round correctly for -ve numbers, */
/* [cffread path callback] Add moveto to path */
static void dumpMoveto(void *ctx, cffFixed x1, cffFixed y1) {
    printf("%d %d _MT\n", FIX2INT(x1), FIX2INT(y1));
}

/* [cffread path callback] Add lineto to path */
static void dumpLineto(void *ctx, cffFixed x1, cffFixed y1) {
    printf("%d %d _LT\n", FIX2INT(x1), FIX2INT(y1));
}

/* [cffread path callback] Add curveto to path */
static void dumpCurveto(void *ctx, int flex, cffFixed x1, cffFixed y1,
                        cffFixed x2, cffFixed y2, cffFixed x3, cffFixed y3) {
    printf("%d %d %d %d %d %d _CT\n", FIX2INT(x1), FIX2INT(y1), FIX2INT(x2),
           FIX2INT(y2), FIX2INT(x3), FIX2INT(y3));
}

/* [cffread path callback] Close path */
static void dumpClosepath(void *ctx) {
    printf("_CP\n");
}

/* [cffread path callback] End char */
static void dumpEndchar(void *ctx) {
    printf("0 setlinewidth fill\n");
}

static void printGlyphRun(hotCtx g, int *xLine, int lineMax, int caretoff,
                          int doOffset) {
    unsigned int i;
    static cffPathCallbacks pathcb = {
        NULL,
        dumpMoveto,
        dumpLineto,
        dumpCurveto,
        dumpClosepath,
        dumpEndchar,
        NULL,
        NULL,
    };
    const char *sample[] = {
        "O", "H", "e", "k",
        "y", "o", "h", "f", "onesuperior",
        "l", "semicolon", "ampersand", "five", "c",
        "exclam", "P",
        "A", "bullet", "g"};

    if (g->font.Encoding == FI_EXP_ENC) {
        sample[1] = "Osmall";
        sample[2] = "Hsmall";
        sample[3] = "Asmall";
        sample[4] = "Fsmall";
        sample[5] = "threeoldstyle";
        sample[6] = "twooldstyle";
        sample[7] = "onesuperior";
        sample[8] = "ampersandsmall";
        sample[9] = "comma";
        sample[10] = "figuredash";
        sample[11] = "onequarter";
        sample[11] = "ffi";
        sample[11] = "sixinferior";
        sample[11] = "sevenoldstyle";
        sample[11] = "nineoldstyle";
    }

    for (i = 0; i < ARRAY_LEN(sample); i++) {
        GID gid = mapName2GID(g, sample[i], NULL);
        if (gid != GID_UNDEF) {
            FWord hAdv;

            cffGetGlyphWidth(g->ctx.cff, gid, &hAdv, NULL);
            if (*xLine + hAdv < lineMax) {
                *xLine += hAdv;
                printf("_NP\n");
                cffGetGlyphInfo(g->ctx.cff, gid, &pathcb);
                if (doOffset) {
                    printf(
                        "%d 0 translate\n"
                        "caret\n"
                        "%d 0 translate\n\n",
                        hAdv + caretoff, -caretoff);
                } else {
                    printf(
                        "%d 0 translate\n"
                        "caret\n",
                        hAdv);
                }
            } else {
                return;
            }
        }
    }
}

/* Print proofs at the current UDV */
static void printCaretOffset(hotCtx g, short caretoff) {
#define IN2PIX(a) ((a)*72)
#define MARGIN IN2PIX(.25)
#define PTSIZE (60)
#define YTOP IN2PIX(11.4)
#define LINEGAP IN2PIX(1.3)
#define SCALE ((double)PTSIZE / 1000)
#define GAP (.15)

    hheaCtx h = g->ctx.hhea;
    int xLine;
    char msg[1024];
    static int caretPrint = 0;
    static double xCurr = MARGIN;
    static double yCurr = YTOP;

    if (caretPrint == 0) {
        printf(
            "%%!\n"
            "/_MT{moveto}bind def\n"
            "/_NP{newpath}bind def\n"
            "/_LT{lineto}bind def\n"
            "/_CT{curveto}bind def\n"
            "/_CP{closepath}bind def\n");
        printf("/Times-Roman findfont 9 scalefont setfont\n\n");
        caretPrint = 1;
    }

    printf(
        "/caret {_NP "
        "%hd %f mul %hd _MT "
        "%hd %f mul %hd _LT\n"
        " .1 setlinewidth stroke 0 0 _MT}bind def\n\n",
        g->font.TypoAscender, h->caretoff.tangent, g->font.TypoAscender,
        g->font.TypoDescender, h->caretoff.tangent, g->font.TypoDescender);

    /* Set up for a line */
    printf("gsave\n");
    yCurr -= LINEGAP;
    if (yCurr < MARGIN + IN2PIX(.3)) {
        printf("showpage\n");
        yCurr = YTOP - LINEGAP;
    }

    printf("%f %f translate\n", xCurr, yCurr);

    /* Draw baselines */
    printf("0 0 _MT %d 0 _LT ", IN2PIX(5));
    printf("%f 0 _MT %f 0 _LT\n", IN2PIX(5 + GAP), (double)IN2PIX(8));
    printf(".1 setlinewidth stroke\n");

    /* Prepare message */
    snprintf(msg, sizeof(msg), "%d %s", caretoff, g->font.FontName.c_str());

    printf("0 -20 _MT (%s) show\n", msg);
    fprintf(stderr, "%s\n", msg);

    printf("%.3f %.3f scale\n", SCALE, SCALE);

    /* Print glyphs with offset */
    xLine = 0;
    printGlyphRun(g, &xLine, (int)(IN2PIX(5) / SCALE), caretoff, 1);

    /* Print some glyphs without the offset, for contrast */
    printf("%f 0 translate\n", IN2PIX(5 + GAP) / SCALE - xLine);
    xLine = (int)(IN2PIX(5 + GAP) / SCALE);
    printGlyphRun(g, &xLine, (int)(IN2PIX(8) / SCALE), caretoff, 0);

    printf("grestore\n");
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
    dbuse(0, printCaretOffset);
}

#endif
#endif
