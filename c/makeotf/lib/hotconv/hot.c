/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Hatch OpenType.
 */

#include "sfnt.h"
#include "hotmap.h"
#include "feat.h"
#include "otl.h"
#include "name.h"
#include "GPOS.h"
#include "OS_2.h"
#include "dictops.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>

/* Windows-specific macros */
#define FAMILY_UNSET 255 /* Flags unset Windows Family field */
#define ANSI_CHARSET 0
#define SYMBOL_CHARSET 2

void hotAddAnonTable(hotCtx g, unsigned long tag, hotAnonRefill refill);

/* Initialize character name */
static void initCharName(void *ctx, long count, CharName *charname) {
    hotCtx g = ctx;
    long i;
    for (i = 0; i < count; i++) {
        dnaINIT(g->DnaCTX, *charname, 16, 16);
        charname++;
    }
    return;
}

static void initOverrides(hotCtx g) {
    FontInfo_ *font = &g->font;

    font->TypoAscender = font->hheaAscender = font->hheaDescender
        = font->hheaLineGap = font->TypoDescender = font->TypoLineGap
        = font->VertTypoAscender = font->VertTypoDescender
        = font->VertTypoLineGap = font->winAscent = font->winDescent
        = font->win.XHeight = font->win.CapHeight
        = SHRT_MAX;
}

static ctlMemoryCallbacks hot_dna_memcb;

static void *hot_manage(ctlMemoryCallbacks *cb, void *old, size_t size) {
    hotCtx h = (hotCtx)cb->ctx;
    void *p = NULL;
    if (size > 0) {
        if (old == NULL) {
            p = hotMemNew(h, size);
            return (p);
        } else {
            p = hotMemResize(h, old, size);
            return (p);
        }
    } else {
        if (old == NULL) {
            return NULL;
        } else {
            hotMemFree(h, old);
            return NULL;
        }
    }
}

hotCtx hotNew(hotCallbacks *hotcb) {
    tcCallbacks tccb;
    time_t now;
    hotCtx g = malloc(sizeof(struct hotCtx_));

    if (g == NULL) {
        if (hotcb->message != NULL) {
            hotcb->message(hotcb->ctx, hotFATAL, "out of memory");
        }
        hotcb->fatal(hotcb->ctx);
    }

    g->hadError = 0;
    g->convertFlags = 0;

    /* Set version numbers. The hot library version serves to identify the      */
    /* software version that built an OTF font and is saved in the Version name */
    /* in the name table. The font version serves to identify the OTF font data */
    /* and is saved in the head table and the Unique and Version names in the   */
    /* name table. */
    g->version = HOT_VERSION;
    g->font.version.otf = (1 << 16); /* 1.0 in Fixed */
    g->font.vendId = "";
    g->font.licenseID = NULL;

    /* Get current time */

    time(&now);
    SAFE_LOCALTIME(&now, &g->time);

    /* Initialize hot library callbacks */
    g->cb = *hotcb;
    hot_dna_memcb.ctx = g;
    hot_dna_memcb.manage = hot_manage;

    /* Initialize contexts for safe freeing */
    g->DnaCTX = NULL;
    g->ctx.cff = NULL;
    g->ctx.tc = NULL;
    g->ctx.map = NULL;
    g->ctx.feat = NULL;
    g->ctx.otl = NULL;
    g->ctx.BASE = NULL;
    g->ctx.CFF_ = NULL;
    g->ctx.GDEF = NULL;
    g->ctx.GPOS = NULL;
    g->ctx.GSUB = NULL;
    g->ctx.OS_2 = NULL;
    g->ctx.cmap = NULL;
    g->ctx.head = NULL;
    g->ctx.hhea = NULL;
    g->ctx.hmtx = NULL;
    g->ctx.name = NULL;
    g->ctx.post = NULL;
    g->ctx.sfnt = NULL;
    g->ctx.vhea = NULL;
    g->ctx.vmtx = NULL;
    g->ctx.VORG = NULL;

    g->DnaCTX = dnaNew(&hot_dna_memcb, DNA_CHECK_ARGS);
    dnaINIT(g->DnaCTX, g->data, 250, 500);
    dnaINIT(g->DnaCTX, g->tmp, 250, 500);
    dnaINIT(g->DnaCTX, g->note, 1024, 1024);

    /* Initialize font information */
#if HOT_DEBUG
    g->font.debug = 0;
#endif /* HOT_DEBUG */
    g->font.flags = 0;
    g->font.fsSelectionMask_on = -1;
    g->font.fsSelectionMask_off = -1;
    g->font.os2Version = 0;
    dnaINIT(g->DnaCTX, g->font.FontName, 64, 32);
    dnaINIT(g->DnaCTX, g->font.kern.pairs, 1500, 1000);
    dnaINIT(g->DnaCTX, g->font.kern.values, 1500, 8500);
    dnaINIT(g->DnaCTX, g->font.unenc, 30, 70);
    g->font.unenc.func = initCharName;
    dnaINIT(g->DnaCTX, g->font.glyphs, 315, 350);

    initOverrides(g);

    /* Initialize tc library */
    tccb.ctx = hotcb->ctx;
    tccb.fatal = hotcb->fatal;
    tccb.message = hotcb->message; /* Suppress messages from library */
    tccb.psId = hotcb->psId;
    tccb.psRefill = hotcb->psRefill;
    tccb.psSize = NULL;
    tccb.cffId = hotcb->cffId;
    tccb.cffWrite1 = hotcb->cffWrite1;
    tccb.cffWriteN = hotcb->cffWriteN;
    tccb.cffSize = hotcb->cffSize;
    tccb.tmpOpen = hotcb->tmpOpen;
    tccb.tmpWriteN = hotcb->tmpWriteN;
    tccb.tmpRewind = hotcb->tmpRewind;
    tccb.tmpRefill = hotcb->tmpRefill;
    tccb.tmpClose = hotcb->tmpClose;
    tccb.glyphMap = NULL;
    tccb.getAliasAndOrder = hotcb->getAliasAndOrder;
    g->ctx.tc = tcNew(&tccb);

    /* Initialize modules */
    sfntNew(g);
    mapNew(g);
    otlNew(g);
    featNew(g);

    return g;
}

void setVendId_str(hotCtx g, const char *vend) {
    char *id;

    id = (char *)hotMemNew(g, strlen(vend) + 1);
    strcpy(id, vend);
    g->font.vendId = id;
}

/* Try to set vendor id by matching against copyright strings in the font */
static void setVendId(hotCtx g) {
    typedef struct {
        char *string;
        char *id;
    } Match;
    static Match vendor[] = /* Arranged by most likely first */
        {
            {"Adobe", "ADBE"},
        };
    unsigned int i;
    char *p;
    char *notice;
    unsigned length;

    if (g->font.vendId[0] != '\0') {
        return; /*Must have been set by feature file*/
    }
    /* Get notice string */
    if (g->font.Notice != SID_UNDEF) {
        p = hotGetString(g, g->font.Notice, &length);
    } else {
        goto dflt;
    }

    /* Make null-terminated  copy */
    notice = dnaGROW(g->tmp, (int)length);
    for (i = 0; i < length; i++) {
        notice[i] = p[i];
    }
    notice[length] = '\0';

    /* Match vendor */
    for (i = 0; i < ARRAY_LEN(vendor); i++) {
        if (strstr(notice, vendor[i].string) != NULL) {
            g->font.vendId = vendor[i].id;
            return;
        }
    }
dflt:
    /* No information; set default */
    g->font.vendId = "UKWN"; /* xxx Is this correct? */
}

/* Return pointer to CFF data that's guaranteed to be length bytes long.
   WARNING, this function may return a pointer to temporary storage that may be
   overwritten by the next function call. Therefore, if you need to have call
   this multiple times and have each return valid concurrently you must make
   copies of the data */
static char *getCFFData(hotCtx g, long offset, long length) {
    long count;
    char *ptr = g->cb.cffSeek(g->cb.ctx, offset, &count);

    if (count >= length) {
        return ptr; /* All data within client buffer */
    } else {
        /* Not enough data in block so copy blocks to temporary */
        g->data.cnt = 0;
        for (;;) {
            long left = length - g->data.cnt;

            if (count == 0) {
                hotMsg(g, hotFATAL, "invalid CFF data offset");
            } else if (left > count) {
                COPY(dnaEXTEND(g->data, count), ptr, count);
                ptr = g->cb.cffRefill(g->cb.ctx, &count);
            } else {
                COPY(dnaEXTEND(g->data, left), ptr, left);
                break;
            }
        }
        return g->data.array;
    }
}

/* [cffread callback] Fatal exception handler */
static void cbFatal(void *ctx) {
    hotCtx g = ctx;
    g->cb.fatal(g->cb.ctx);
}

/* [cffread callback] Print error message */
static void cbMessage(void *ctx, int type, char *text) {
    hotCtx g = ctx;
    g->cb.message(g->cb.ctx, type, text);
}

/* [cffread callback] Allocate memory */
static void *cbMalloc(void *ctx, size_t size) {
    hotCtx g = ctx;
    return hotMemNew(g, size);
}

/* [cffread callback] Free memory */
static void cbFree(void *ctx, void *ptr) {
    hotCtx g = ctx;
    hotMemFree(g, ptr);
}

/* [cffread callback] Seek to offset and return data. */
static char *cbSeek(void *ctx, long offset, long *count) {
    hotCtx g = ctx;
    return g->cb.cffSeek(g->cb.ctx, offset, count);
}

/* [cffread callback] Refill data buffer from current position */
static char *cbRefill(void *ctx, long *count) {
    hotCtx g = ctx;
    return g->cb.cffRefill(g->cb.ctx, count);
}

/* Convert PostScript font to CFF and read result */
char *hotReadFont(hotCtx g, int flags, int *psinfo, hotReadFontOverrides *fontOverride) {
    static cffStdCallbacks cb = {
        NULL,
        cbFatal,
        cbMessage,
        cbMalloc,
        cbFree,
        cbSeek,
        cbRefill};
    cffFontInfo *fi;
    int gid;
    long tcflags;

    /* Copy conversion flags */
    g->font.flags = 0;
#if HOT_DEBUG
    g->font.debug = flags & HOT_DB_MASK;
#endif /* HOT_DEBUG */

    /* Convert to CFF */
    tcflags = 0;
    tcflags |= TC_DO_WARNINGS;
    if (flags & HOT_ADD_AUTH_AREA) {
        tcflags |= TC_ADDAUTHAREA;
    }
    if (flags & HOT_SUBRIZE) {
        tcflags |= TC_SUBRIZE;
    }
    if (flags & HOT_ADD_EURO) {
        tcflags |= TC_ADDEURO;
    }
    if (flags & HOT_IS_SERIF) {
        tcflags |= TC_IS_SERIF;
    }
    if (flags & HOT_IS_SANSSERIF) {
        tcflags |= TC_IS_SANSSERIF;
    }
    if (flags & HOT_SUPRESS_HINT_WARNINGS) {
        tcflags |= TC_SUPPRESS_HINT_WARNINGS;
    }
    if (flags & HOT_SUPRESS__WIDTH_OPT) {
        tcflags |= TC_SUPPRESS_WIDTH_OPT;
    }

    if (flags & HOT_NO_OLD_OPS) {
        tcflags |= TC_NOOLDOPS;
    }
    if (flags & HOT_FORCE_NOTDEF) {
        tcflags |= TC_FORCE_NOTDEF;
    }
    if (flags & HOT_RENAME) {
        tcflags |= TC_RENAME; /* turn on  renaming in typecomp */
    } else {
        g->cb.getFinalGlyphName = NULL; /* suppresses renaming in feature file */
    }
    if (flags & HOT_SUBSET) {
        tcflags |= TC_SUBSET; /* turn on subsetting to GOADB list  in typecomp */
    }
    if (flags & HOT_VERBOSE) {
        tcflags |= TC_VERBOSE; /* turn on all warnings and notes */
    }
    tcSetMaxNumSubrsOverride(g->ctx.tc, fontOverride->maxNumSubrs);
    tcSetWeightOverride(g->ctx.tc, fontOverride->syntheticWeight);
    tcCompactFont(g->ctx.tc, tcflags);

    if (g->cb.tmpClose) {
        g->cb.tmpClose(g->cb.ctx); /* temporary hack to write out tmp cff file. */
    }

    /* Parse CFF data and get global font information */
    cb.ctx = g;
    g->ctx.cff = cffNew(&cb, 0);
    fi = cffGetFontInfo(g->ctx.cff);

    /* Create and copy FontName */
    COPY(dnaGROW(g->font.FontName, fi->FontName.length),
         getCFFData(g, fi->FontName.offset, fi->FontName.length),
         fi->FontName.length);
    g->font.FontName.array[fi->FontName.length] = '\0';
    g->font.FontName.cnt = fi->FontName.length + 1;
    hotMsg(g, hotHEADING, "processing font <%s>", g->font.FontName.array);

    /* Copy basic font information */
    g->font.Notice = fi->Notice;
    g->font.Copyright = fi->Copyright;
    g->font.FamilyName = fi->FamilyName;
    g->font.FullName = fi->FullName;
    g->font.bbox.left = fi->FontBBox.left;
    g->font.bbox.bottom = fi->FontBBox.bottom;
    g->font.bbox.right = fi->FontBBox.right;
    g->font.bbox.top = fi->FontBBox.top;
    g->font.unitsPerEm = fi->unitsPerEm;
    if (fi->isFixedPitch) {
        g->font.flags |= FI_FIXED_PITCH;
    }
    g->font.ItalicAngle = fi->ItalicAngle;
    g->font.UnderlinePosition = fi->UnderlinePosition;
    g->font.UnderlineThickness = fi->UnderlineThickness;
    g->font.Encoding = fi->Encoding;
    g->font.charset = fi->charset;

    if (fi->cid.registry != CFF_SID_UNDEF) {
        /* Copy CIDFont data */
        g->font.cid.registry = fi->cid.registry;
        g->font.cid.ordering = fi->cid.ordering;
        g->font.cid.supplement = fi->cid.supplement;
        g->font.flags |= FI_CID;

        sprintf(g->font.version.PS, "%g", fi->cid.version);
    } else if (fi->version != CFF_SID_UNDEF) {
        unsigned length;
        char *p = hotGetString(g, fi->version, &length);
        if (length > sizeof(g->font.version.PS) - 1) {
            length = sizeof(g->font.version.PS) - 1;
        }
        strncpy(g->font.version.PS, p, length);
        g->font.version.PS[length] = '\0';
    } else {
        strcpy(g->font.version.PS, "(version unavailable)");
    }

    /* Get and copy glyph information */
    dnaSET_CNT(g->font.glyphs, fi->nGlyphs);
    for (gid = 0; gid < fi->nGlyphs; gid++) {
        cffGlyphInfo *cffgi = cffGetGlyphInfo(g->ctx.cff, gid, NULL);
        hotGlyphInfo *hotgi = &g->font.glyphs.array[gid];

        hotgi->id = cffgi->id;
        hotgi->code = cffgi->code;
        hotgi->hAdv = cffgi->hAdv;
        hotgi->vAdv = SHRT_MAX;
        hotgi->bbox = cffgi->bbox;
        hotgi->sup = cffgi->sup;
        hotgi->vOrigY = SHRT_MAX;
        hotgi->srcName = NULL;
    }

    setVendId(g);

    /* Prepare result */
    *psinfo = IS_CID(g) ? hotCID : hotSingleMaster;
    if (g->font.Encoding == FI_STD_ENC) {
        *psinfo |= HOT_STD_ENC;
    }

    return g->font.FontName.array;
}

/* Add glyph's vertical origin y-value to glyph info. (Not an API.) */
void hotAddVertOriginY(hotCtx g, GID gid, short value) {
    hotGlyphInfo *hotgi = &g->font.glyphs.array[gid]; /* gid already validated */

    if (!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) {
        g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;
    }

    if (hotgi->vOrigY != SHRT_MAX) {
        featGlyphDump(g, gid, '\0', 0);
        if (hotgi->vOrigY == value) {
            hotMsg(g, hotNOTE,
                   "Ignoring duplicate VertOriginY entry for "
                   "glyph %s", g->note.array);
        } else {
            hotMsg(g, hotFATAL,
                   "VertOriginY redefined for "
                   "glyph %s", g->note.array);
        }
    } else {
        hotgi->vOrigY = value;
    }
}

/* Add glyph's vertical advance width to the  glyph info. (Not an API.) */
void hotAddVertAdvanceY(hotCtx g, GID gid, short value) {
    hotGlyphInfo *hotgi = &g->font.glyphs.array[gid]; /* gid already validated */
    if (!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) {
        g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;
    }

    if (hotgi->vAdv != SHRT_MAX) {
        featGlyphDump(g, gid, '\0', 0);
        if (hotgi->vAdv == value) {
            hotMsg(g, hotNOTE,
                   "Ignoring duplicate VertAdvanceY entry for "
                   "glyph %s", g->note.array);
        } else {
            hotMsg(g, hotFATAL,
                   "VertAdvanceY redefined for "
                   "glyph %s", g->note.array);
        }
    } else {
        hotgi->vAdv = -value;
    }
}

/* Set font bounding metrics from glyph bounding boxes */
static void setBounds(hotCtx g) {
    int i;
    FontInfo_ *font = &g->font;
    long nOrigGlyphs = font->glyphs.cnt;
    BBox *bbox = &font->bbox;
    BBox *minBearing = &font->minBearing;
    hvMetric *maxAdv = &font->maxAdv;
    hvMetric *maxExtent = &font->maxExtent;

    /* Initialize */
    bbox->left = bbox->bottom = SHRT_MAX;
    bbox->right = bbox->top = SHRT_MIN;
    minBearing->left = minBearing->right = SHRT_MAX;
    maxAdv->h = 0;
    maxExtent->h = 0;

    if (g->font.flags & HOT_EURO_ADDED) {
        /* Euro glyph added; exclude from BBox calculation. */
        nOrigGlyphs--;
    }

    /* Compute horizontal extents and font bbox */
    for (i = 0; i < nOrigGlyphs; i++) {
        hotGlyphInfo *glyph = &font->glyphs.array[i];

        if (glyph->bbox.left != 0 || glyph->bbox.bottom != 0 ||
            glyph->bbox.right != 0 || glyph->bbox.top != 0) {
            /* Marking glyph; compute bounds */
            if (maxAdv->h < glyph->hAdv) {
                maxAdv->h = glyph->hAdv;
            }

            if (glyph->bbox.left < minBearing->left) {
                minBearing->left = glyph->bbox.left;
            }

            if (glyph->hAdv - glyph->bbox.right < minBearing->right) {
                minBearing->right = glyph->hAdv - glyph->bbox.right;
            }

            if (glyph->bbox.right > maxExtent->h) {
                maxExtent->h = glyph->bbox.right;
            }

#if 1
            if (glyph->bbox.left < bbox->left) {
                bbox->left = glyph->bbox.left;
            }
            if (glyph->bbox.right > bbox->right) {
                bbox->right = glyph->bbox.right;
            }
            if (glyph->bbox.bottom < bbox->bottom) {
                bbox->bottom = glyph->bbox.bottom;
            }
            if (glyph->bbox.top > bbox->top) {
                bbox->top = glyph->bbox.top;
            }
#else
            if (glyph->bbox.left < bbox->left) {
                bbox->left = glyph->bbox.left;
                printf("left  =%4hd [%4d]\n", bbox->left, i);
            }
            if (glyph->bbox.right > bbox->right) {
                bbox->right = glyph->bbox.right;
                printf("right =%4hd [%4d]\n", bbox->right, i);
            }
            if (glyph->bbox.bottom < bbox->bottom) {
                bbox->bottom = glyph->bbox.bottom;
                printf("bottom=%4hd [%4d]\n", bbox->bottom, i);
            }
            if (glyph->bbox.top > bbox->top) {
                bbox->top = glyph->bbox.top;
                printf("top   =%4hd [%4d]\n", bbox->top, i);
            }
#endif
        }
    }
#if 0
    printf(
        "--- FontBBox\n"
        "left  =%4hd\n"
        "bottom=%4hd\n"
        "right =%4hd\n"
        "top   =%4hd\n",
        bbox->left, bbox->bottom, bbox->right, bbox->top);
#endif

    if (bbox->left == SHRT_MAX) {
        /* Marking glyph not seen; reset values */
        bbox->left = bbox->bottom = bbox->right = bbox->top = 0;
        minBearing->left = minBearing->bottom =
            minBearing->right = minBearing->top = 0;
    }
}

/* Check if encoded on platform; else use WinANSI */
static UV pfmChar2UV(hotCtx g, int code) {
    hotGlyphInfo *gi = (IS_CID(g)) ? NULL : mapPlatEnc2Glyph(g, code);

    if (gi != NULL && gi->uv != UV_UNDEF) {
        return gi->uv;
    } else {
        /* WinNT file c_1252.nls maps undef UVs in WinANSI to C0/C1. */
        UV uv = mapWinANSI2UV(g, code);
        return (uv == UV_UNDEF) ? (unsigned int)code : uv;
    }
}

/* Prepare Windows-specific data. */
static void prepWinData(hotCtx g) {
    hotGlyphInfo *gi;
    long i;
    long sum;
    long count;
    int intLeading;
    FontInfo_ *font = &g->font;
    if (g->convertFlags & HOT_OLD_SPACE_DEFAULT_CHAR) {
        font->win.DefaultChar = 32; /* QuarkXPress6.5 wants this to be space, or it spits notdefs */
    } else {
        font->win.DefaultChar = 0; /* if 0, this indicates GID 0. The previous use of the UV for space */
                                   /* was a leftover from when this module was written to change as    */
                                   /* little as possible when converting legacy PS fonts to OTF.       */
    }
    if (font->win.Family == FAMILY_UNSET) {
        /* Converting from non-Windows data; synthesize makepfm algorithm */
        int StdEnc = font->Encoding == CFF_STD_ENC;
        int spacePresent = mapUV2Glyph(g, UV_SPACE) != NULL;

        font->win.Family = StdEnc ? HOT_ROMAN : HOT_DECORATIVE;
        /* xxx should this change to the new pcmaster algorithm */
        font->win.CharSet =
            (strcmp(font->FontName.array, "Symbol") == 0) ? SYMBOL_CHARSET : ANSI_CHARSET;

        font->win.BreakChar = spacePresent ? UV_SPACE : UV_UNDEF;
    } else {
        /* Convert default and break char to Unicode */
        font->win.DefaultChar = (UV_BMP)pfmChar2UV(g, font->win.DefaultChar);
        font->win.BreakChar = (UV_BMP)pfmChar2UV(g, font->win.BreakChar);
    }

    {
        /* Production font; synthesize Windows values using ideal algorithm */

        /* Set subscript scale to x=65% and y=60% lowered by 7.5% of em */
        font->win.SubscriptXSize = (FWord)EM_SCALE(650);
        font->win.SubscriptYSize = (FWord)EM_SCALE(600);
        font->win.SubscriptYOffset = (FWord)EM_SCALE(75);

        /* Set superscript scale to x=65% and y=60% raised by 35% of em */
        font->win.SuperscriptXSize = font->win.SubscriptXSize;
        font->win.SuperscriptYSize = font->win.SubscriptYSize;
        font->win.SuperscriptYOffset = (FWord)EM_SCALE(350);

        /* Use o-height and O-height (adjusted for overshoot) for x-height and
           cap-height, respectively, since this should give better results for
           italic and swash fonts as well as handling Roman */
        if (!OVERRIDE(font->win.XHeight)) {
            gi = mapUV2Glyph(g, UV_PRO_X_HEIGHT_1 /* o */);
            if (gi == NULL) {
                gi = mapUV2Glyph(g, UV_PRO_X_HEIGHT_2 /* Osmall */);
            }
            font->win.XHeight =
                (gi != NULL) ? gi->bbox.top + gi->bbox.bottom : 0;
        }
        if (!OVERRIDE(font->win.CapHeight)) {
            gi = mapUV2Glyph(g, UV_PRO_CAP_HEIGHT /* O */);
            font->win.CapHeight =
                (gi != NULL) ? gi->bbox.top + gi->bbox.bottom : 0;
        }

        /* Set strikeout size to underline thickness and position it at 60% of
           x-height, if non-zero, else 22% of em.  */
        font->win.StrikeOutSize = font->UnderlineThickness;
        font->win.StrikeOutPosition =
            (font->win.XHeight != 0) ? font->win.XHeight * 6L / 10
                                     : (FWord)EM_SCALE(220);
    }

    if (font->ItalicAngle == 0) {
        font->win.SubscriptXOffset = 0;
        font->win.SuperscriptXOffset = 0;
    } else {
        /* Adjust position for italic angle */
        double tangent = tan(FIX2DBL(-font->ItalicAngle) / RAD_DEG);
        font->win.SubscriptXOffset =
            (short)RND(-font->win.SubscriptYOffset * tangent);
        font->win.SuperscriptXOffset =
            (short)RND(font->win.SuperscriptYOffset * tangent);
    }

    /* Set win ascent/descent */
    if (OVERRIDE(font->winAscent)) {
        font->win.ascent = font->winAscent;
    } else {
        font->win.ascent = 0;
        if (font->bbox.top > 0) {
            font->win.ascent = font->bbox.top;
        }
    }
    if (OVERRIDE(font->winDescent)) {
        font->win.descent = font->winDescent;
    } else {
        font->win.descent = 0;
        if (font->bbox.bottom < 0) {
            font->win.descent = -font->bbox.bottom;
        }
    }
    intLeading = font->win.ascent + font->win.descent - font->unitsPerEm;
    if (intLeading < 0) {
        /* Warn about negative internal leading */
        hotMsg(g, hotWARNING, "Negative internal leading: win.ascent + win.descent < unitsPerEm");
    }

    /* Set typo ascender/descender/linegap */
    if (IS_CID(g)) {
        if (!OVERRIDE(font->TypoAscender) || !OVERRIDE(font->TypoDescender)) {
            hotGlyphInfo *gi = mapUV2Glyph(g, UV_VERT_BOUNDS);
            if (!OVERRIDE(font->TypoAscender)) {
                font->TypoAscender = (gi != NULL) ? gi->bbox.top : (short)EM_SCALE(880);
            }
            if (!OVERRIDE(font->TypoDescender)) {
                font->TypoDescender = (gi != NULL) ? gi->bbox.bottom : (short)EM_SCALE(-120);
            }
        }
    } else {
        if (!OVERRIDE(font->TypoAscender)) {
            /* try to use larger of height of lowercase d ascender, or CapHeight. */
            /* Fall back to font bbox top, but make sure it is not greater than embox height */

            hotGlyphInfo *gi = mapUV2Glyph(g, UV_ASCENT); /* gi for lower-case d */
            short dHeight = (gi == NULL) ? 0 : gi->bbox.top;

            if (dHeight > font->win.CapHeight) {
                font->TypoAscender = dHeight;
            } else {
                font->TypoAscender = font->win.CapHeight;
            }

            if (font->TypoAscender == 0) {
                font->TypoAscender = ABS(font->bbox.top);
            }

            if (font->TypoAscender > font->unitsPerEm) {
                font->TypoAscender = font->unitsPerEm;
            }
        }

        if (!OVERRIDE(font->TypoDescender)) {
            font->TypoDescender = font->TypoAscender - font->unitsPerEm;
        }
    }

    /* warn if the override values don't sum correctly. */
    if ((font->TypoAscender - font->TypoDescender) != font->unitsPerEm) {
        /* can happen only if overrides are used */
        hotMsg(g, hotWARNING, "The feature file OS/2 overrides TypoAscender and TypoDescender do not sum to the font em-square.");
    }

    if (!OVERRIDE(font->TypoLineGap)) {
        font->TypoLineGap = IS_CID(g)
                                ? font->TypoAscender - font->TypoDescender
                                : EM_SCALE(1200) - font->TypoAscender + font->TypoDescender;
    }

    /* warn if the line gap is negative. */
    if (font->TypoAscender < 0) {
        hotMsg(g, hotWARNING, "The feature file OS/2 override TypoLineGap value is negative!");
    }

    /* No need to duplicate PFM avgCharWidth algorithm: Euro, Zcarons added to
       WinANSI. This algorithm is encoding-dependent. */
    sum = 0;
    count = 0;
    for (i = 0; i < font->glyphs.cnt; i++) {
        if (font->glyphs.array[i].hAdv > 0) {
            sum += font->glyphs.array[i].hAdv;
            count++;
        }
    }
    if (count > 0) {
        g->font.win.AvgWidth = (short)(sum / count);
    } else {
        g->font.win.AvgWidth = 0;
    }
}

/* Must be called after featFill(), since TypoAscender/TypoDescender could have
   been overwritten */
static void setVBounds(hotCtx g) {
    long i;
    FontInfo_ *font = &g->font;
    BBox *minBearing = &font->minBearing;
    hvMetric *maxAdv = &font->maxAdv;
    hvMetric *maxExtent = &font->maxExtent;
    cffFWord dfltVAdv = -font->TypoAscender + font->TypoDescender;
    short dfltVOrigY = font->TypoAscender;

    if (!OVERRIDE(font->VertTypoAscender)) {
        font->VertTypoAscender = font->unitsPerEm / 2;
    }
    if (!OVERRIDE(font->VertTypoDescender)) {
        font->VertTypoDescender = -font->VertTypoAscender;
    }
    if (!OVERRIDE(font->VertTypoLineGap)) {
        font->VertTypoLineGap = font->VertTypoAscender -
                                font->VertTypoDescender;
    }

    /* Initialize */
    minBearing->bottom = minBearing->top = SHRT_MAX;
    maxAdv->v = 0;
    maxExtent->v = 0;

    /* Compute vertical extents */
    for (i = 0; i < g->font.glyphs.cnt; i++) {
        hotGlyphInfo *glyph = &g->font.glyphs.array[i];

        /* If glyph is a repl in the 'vrt2' feature, its vAdv has already been
           set appropriately from the GSUB module */
        if (glyph->vAdv == SHRT_MAX) {
            glyph->vAdv = dfltVAdv;
        }
        if (glyph->vOrigY == SHRT_MAX) {
            glyph->vOrigY = dfltVOrigY;
        }

        if (glyph->bbox.left != 0 && glyph->bbox.bottom != 0 &&
            glyph->bbox.right != 0 && glyph->bbox.top != 0) {
            /* Marking glyph; compute bounds. */
            FWord tsb = glyph->vOrigY - glyph->bbox.top;
            FWord bsb = glyph->bbox.bottom -
                        (glyph->vOrigY + glyph->vAdv);

            if (maxAdv->v < -glyph->vAdv) {
                maxAdv->v = -glyph->vAdv;
            }

            if (tsb < minBearing->top) {
                minBearing->top = tsb;
            }

            if (bsb < minBearing->bottom) {
                minBearing->bottom = bsb;
            }

            if (glyph->vOrigY - glyph->bbox.bottom > maxExtent->v) {
                maxExtent->v = glyph->vOrigY - glyph->bbox.bottom;
            }
        }
    }
}

static void hotReuse(hotCtx g) {
    g->hadError = 0;
    g->convertFlags = 0;

    initOverrides(g);
    sfntReuse(g);
    mapReuse(g);
    featReuse(g);
}

char *refillDSIG(void *ctx, long *count, unsigned long tag) {
    static const char data[] = "\x00\x00\x00\x01\x00\x00\x00";
    static unsigned int dsigCnt = 0;
    if (dsigCnt == 0) {
        *count = 8;
        dsigCnt = 1;
        return (char *)(&data);
    } else {
        *count = 0;
        return (char *)NULL;
    }
}

/* ------------------------------------------------------------------------- */

int bbox_changed(BBox *x, BBox *y) {
    return (   (x->left   != y->left)
            || (x->right  != y->right)
            || (x->top    != y->top)
            || (x->bottom != y->bottom));
}

void patch_cff_fontbbox(hotCtx g) {
    long count;
    char *data;
    long offset, top_dict_start, top_dict_end;
    uint8_t b0 = 0;

    data = g->cb.cffSeek(g->cb.ctx, 0, &count);

    offset = 8 + data[8]; /* get past end of name string to top dict index */
    top_dict_end = 3 + offset + data[offset + 4]; /* calc end index of top dict */
    offset += 5; /* get past top dict index to start of top dict itself */

    /* parse the top dict to find the FontBBox key */
    top_dict_start = offset;
    while (offset <= top_dict_end)
    {
        b0 = data[offset];
        if (b0 == cff_FontBBox) {
            break;
        } else if (b0 == cff_BCD) {
            while (((b0 & 0x0F) != 0x0F)
                  && ((b0 & 0xF0) != 0xF0)
                  && offset < top_dict_end) {
                offset++;
                b0 = data[offset];
            }
            continue;
        } else if (b0 == cff_longint) {
            offset += 5;
            continue;
        } else if (b0 == cff_shortint) {
            offset += 3;
            continue;
        } else if ((247 <= b0) && (b0 <= 254)) {
            offset += 2;
            continue;
        } else if ((32 <= b0) && (b0 <= 246)) {
            offset += 1;
            continue;
        } else if (b0 == cff_escape) {
            offset += 2;
            continue;
        } else {
            offset += 1;
            continue;
        }
    }

    /* make sure we found the key and that we have room for the values */
    if ((b0 != cff_FontBBox) || (offset < top_dict_start + 12))
    {
        hotMsg(g, hotWARNING, "unable to patch FontBBox in CFF table");
        return;
    }

    /* back up to start of FontBBox data */
    offset -= 12;

    /* make sure we see four short (3-byte) ints and a FontBBox key */
    if (   (data[offset] != cff_shortint)
        || (data[offset + 3] != cff_shortint)
        || (data[offset + 6] != cff_shortint)
        || (data[offset + 9] != cff_shortint)
        || (data[offset + 12] != cff_FontBBox)) {
        hotMsg(g, hotWARNING, "unable to patch FontBBox in CFF table");
        return;
    }

    data[offset + 1] = (g->font.bbox.left & 0xFF00) >> 8;
    data[offset + 2] = (g->font.bbox.left & 0xFF);

    data[offset + 4] = (g->font.bbox.bottom & 0xFF00) >> 8;
    data[offset + 5] = (g->font.bbox.bottom & 0xFF);

    data[offset + 7] = (g->font.bbox.right & 0xFF00) >> 8;
    data[offset + 8] = (g->font.bbox.right & 0xFF);

    data[offset + 10] = (g->font.bbox.top & 0xFF00) >> 8;
    data[offset + 11] = (g->font.bbox.top & 0xFF);
}

/* Convert to OTF */
void hotConvert(hotCtx g) {
    BBox old_bbox;

    old_bbox = g->font.bbox;
    setBounds(g);
    if (bbox_changed(&old_bbox, &g->font.bbox)) {
        patch_cff_fontbbox(g);
    }
    mapFill(g);
    featFill(g);

    prepWinData(g);

    setVBounds(g);

    if (g->convertFlags & HOT_ADD_STUB_DSIG)
        hotAddAnonTable(g, TAG('D', 'S', 'I', 'G'), refillDSIG);

    sfntFill(g);
    sfntWrite(g);

#if HOT_DEBUG
    if (g->font.debug & HOT_DB_AFM) {
        mapPrintAFM(g);
    }
#endif /* HOT_DEBUG */

    cffFree(g->ctx.cff);

    hotReuse(g);
}

void hotFree(hotCtx g) {
    int i;

    tcFree(g->ctx.tc);
    sfntFree(g);
    mapFree(g);
    otlFree(g);
    featFree(g);

    dnaFREE(g->data);
    dnaFREE(g->tmp);
    dnaFREE(g->note);
    dnaFREE(g->font.FontName);
    dnaFREE(g->font.kern.pairs);
    dnaFREE(g->font.kern.values);

    if (g->font.unenc.size != 0) {
        for (i = 0; i < g->font.unenc.size; i++) {
            dnaFREE(g->font.unenc.array[i]);
        }
        dnaFREE(g->font.unenc);
    }
    dnaFree(g->DnaCTX);
    MEM_FREE(g, g);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Save Windows-specific data */
static void saveWinData(hotCtx g, hotWinData *win) {
    if (win != NULL) {
        /* Data supplied from Windows-specific metrics file */
        g->font.win.Family = win->Family;
        g->font.win.CharSet = win->CharSet;
        g->font.win.DefaultChar = win->DefaultChar;
        g->font.win.BreakChar = win->BreakChar;
    } else {
        /* Converting from non-Windows data; mark for latter processing */
        g->font.win.Family = FAMILY_UNSET;
    }
}

/* */
void hotSetConvertFlags(hotCtx g, unsigned long hotConvertFlags) {
    g->convertFlags = hotConvertFlags;
}

/* Add miscellaneous data */
void hotAddMiscData(hotCtx g,
                    hotCommonData *common, hotWinData *win, hotMacData *mac) {
    /* Copy attribute flags and client version */
    g->font.flags |= common->flags & FI_MISC_FLAGS_MASK;
    g->font.version.client = common->clientVers;
    g->font.fsSelectionMask_on = common->fsSelectionMask_on;
    g->font.fsSelectionMask_off = common->fsSelectionMask_off;
    g->font.os2Version = common->os2Version;
    if (common->licenseID != NULL) {
        g->font.licenseID = common->licenseID;
    }

    g->font.flags |= common->flags & FI_MISC_FLAGS_MASK;

    saveWinData(g, win);
    g->font.mac.cmapScript = mac->cmapScript;
    g->font.mac.cmapLanguage = mac->cmapLanguage;

    /* Initialize data arrays */
    dnaSET_CNT(g->font.kern.pairs, common->nKernPairs);
    dnaSET_CNT(g->font.kern.values, common->nKernPairs);
    dnaSET_CNT(g->font.unenc, win->nUnencChars);

    mapApplyReencoding(g, common->encoding, mac->encoding);
}

/* Prepare Windows name by converting \-format numbers to UTF-8. Return 1 on
   syntax error else 0. */
static int prepWinName(hotCtx g, const char *src) {
    /* Next state table */
    static unsigned char next[5][6] = {
        /*  \       0-9     a-f     A-F     *       \0       index */
        /* -------- ------- ------- ------- ------- -------- ----- */
        {   1,      0,      0,      0,      0,      0 },    /* [0] */
        {   0,      2,      2,      2,      0,      0 },    /* [1] */
        {   0,      3,      3,      3,      0,      0 },    /* [2] */
        {   0,      4,      4,      4,      0,      0 },    /* [3] */
        {   0,      0,      0,      0,      0,      0 },    /* [4] */
    };

    /* Action table */

#define E_ (1 << 0) /* Report syntax error */
#define A_ (1 << 1) /* Accumulate hexadecimal bytes */
#define B_ (1 << 2) /* Save regular byte */
#define H_ (1 << 3) /* Save hexadecimal bytes */
#define Q_ (1 << 4) /* Quit on end-of-string */

    static unsigned char action[5][6] = {
        /*  \       0-9     a-f     A-F     *       \0       index */
        /* -------- ------- ------- ------- ------- -------- ----- */
        {   0,      B_,     B_,     B_,     B_,     B_|Q_ },/* [0] */
        {   E_,     A_,     A_,     A_,     E_,     E_ },   /* [1] */
        {   E_,     A_,     A_,     A_,     E_,     E_ },   /* [2] */
        {   E_,     A_,     A_,     A_,     E_,     E_ },   /* [3] */
        {   E_,     A_|H_,  A_|H_,  A_|H_,  E_,     E_ },   /* [4] */
    };

    char *dst = dnaGROW(g->tmp, (long)strlen((char *)src));
    int state = 0;
    unsigned value = 0;

    for (;;) {
        int hexdig = 0; /* Converted hex digit (suppress optimizer warning) */
        int class;      /* Character class */
        int actn;       /* Action flags */
        int c = *src++;

        switch (c) {
            case '\\':
                class = 0;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                hexdig = c - '0';
                class = 1;
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                hexdig = c - 'a' + 10;
                class = 2;
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                hexdig = c - 'A' + 10;
                class = 3;
                break;

            default:
                class = 4;
                break;

            case '\0': /* End of string */
                class = 5;
                break;
        }

        actn = action[state][class];
        state = next[state][class];

        /* Perform actions */
        if (actn == 0) {
            continue;
        }
        if (actn & E_) {
            return 1; /* Syntax error */
        }
        if (actn & B_) {
            *dst++ = c;
        }
        if (actn & A_) {
            value = value << 4 | hexdig;
        }
        if (actn & H_) {
            /* Save 16-bit value in UTF-8 format */
            if (value == 0) {
                return 1; /* Syntax error */
            } else if (value < 0x80) {
                *dst++ = value;
            } else if (value < 0x800) {
                *dst++ = 0xc0 | value >> 6;
                *dst++ = 0x80 | (value & 0x3f);
            } else {
                *dst++ = 0xe0 | value >> 12;
                *dst++ = 0x80 | (value >> 6 & 0x3f);
                *dst++ = 0x80 | (value & 0x3f);
            }
            value = 0;
        }
        if (actn & Q_) {
            return 0;
        }
    }
#undef E_
#undef A_
#undef B_
#undef H_
#undef Q_
}

/* Prepare Macintosh name by converting \-format numbers to bytes. Return 1 on
   syntax error else 0. */
static int prepMacName(hotCtx g, const char *src) {
    /* Next state table */
    static unsigned char next[3][6] = {
        /*  \       0-9     a-f     A-F     *       \0       index */
        /* -------- ------- ------- ------- ------- -------- ----- */
        {   1,      0,      0,      0,      0,      0 },    /* [0] */
        {   0,      2,      2,      2,      0,      0 },    /* [1] */
        {   0,      0,      0,      0,      0,      0 },    /* [2] */
    };

    /* Action table */

#define E_ (1 << 0) /* Report syntax error */
#define A_ (1 << 1) /* Accumulate hexadecimal bytes */
#define B_ (1 << 2) /* Save regular byte */
#define H_ (1 << 3) /* Save hexadecimal bytes */
#define Q_ (1 << 4) /* Quit on end-of-string */

    static unsigned char action[3][6] = {
        /*  \       0-9     a-f     A-F     *       \0       index */
        /* -------- ------- ------- ------- ------- -------- ----- */
        {   0,      B_,     B_,     B_,     B_,     B_|Q_ },/* [0] */
        {   E_,     A_,     A_,     A_,     E_,     E_ },   /* [1] */
        {   E_,     A_|H_,  A_|H_,  A_|H_,  E_,     E_ },   /* [2] */
    };

    char *dst = dnaGROW(g->tmp, (long)strlen((char *)src));
    int state = 0;
    unsigned value = 0;

    for (;;) {
        int hexdig = 0; /* Converted hex digit (suppress optimizer warning) */
        int class;      /* Character class */
        int actn;       /* Action flags */
        int c = *src++;

        /* Direct UTF-8 input is not supported. */
        if (c < 0)
            return 1;

        switch (c) {
            case '\\':
                class = 0;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                hexdig = c - '0';
                class = 1;
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                hexdig = c - 'a' + 10;
                class = 2;
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                hexdig = c - 'A' + 10;
                class = 3;
                break;

            default:
                class = 4;
                break;

            case '\0': /* End of string */
                class = 5;
                break;
        }

        actn = action[state][class];
        state = next[state][class];

        /* Perform actions */
        if (actn == 0) {
            continue;
        }
        if (actn & E_) {
            return 1; /* Syntax error */
        }
        if (actn & B_) {
            *dst++ = c;
        }
        if (actn & A_) {
            value = value << 4 | hexdig;
        }
        if (actn & H_) {
            if (value == 0) {
                return 1; /* Syntax error */
            }
            *dst++ = value;
            value = 0;
        }
        if (actn & Q_) {
            return 0;
        }
    }
#undef E_
#undef A_
#undef B_
#undef H_
#undef Q_
}

/* Add name to name table. Return 1 on validation error else 0. */
int hotAddName(hotCtx g,
               unsigned short platformId, unsigned short platspecId,
               unsigned short languageId, unsigned short nameId,
               const char *str) {
    if ((platformId == HOT_NAME_MS_PLATFORM) ? prepWinName(g, str) : prepMacName(g, str)) {
        return 1;
    }
    nameAddReg(g, platformId, platspecId, languageId, nameId, g->tmp.array);
    return 0;
}

/* Add encoded kern pair to accumulator */
void hotAddKernPair(hotCtx g, long iPair, unsigned first, unsigned second) {
    if (iPair >= g->font.kern.pairs.cnt) {
        hotMsg(g, hotFATAL,
               "invalid kern pair index: %ld; expecting maximum "
               "index: %ld",
               iPair, g->font.kern.pairs.cnt - 1);
    } else {
        KernPair_ *pair = &g->font.kern.pairs.array[iPair];
        pair->first = first;
        pair->second = second;
    }
}

/* Add encoded kern value to accumulator */
void hotAddKernValue(hotCtx g, long iPair, short value) {
    if (iPair >= g->font.kern.pairs.cnt) {
        hotMsg(g, hotFATAL,
               "invalid kern value index: %ld; expecting maximum "
               "index: %ld",
               iPair, g->font.kern.pairs.cnt - 1);
    }
    g->font.kern.values.array[iPair] = value;
}

/* Add unencoded char */
void hotAddUnencChar(hotCtx g, int iChar, char *name) {
    if (iChar >= g->font.unenc.cnt) {
        hotMsg(g, hotFATAL, "invalid unencoded char");
    } else {
        int length = strlen(name);
        strcpy(dnaGROW(g->font.unenc.array[iChar], length), name);
    }
}

/* Add Adobe CMap */
void hotAddCMap(hotCtx g, hotCMapId id, hotCMapRefill refill) {
    mapAddCMap(g, id, refill);
}

/* Add Unicode variation Selector cmap subtable.  */
void hotAddUVSMap(hotCtx g, char *uvsName) {
    mapAddUVS(g, uvsName);
}

/* Map platform encoding to GID */
unsigned short hotMapPlatEnc2GID(hotCtx g, int code) {
    return mapPlatEnc2GID(g, code);
}

/* Map glyph name to GID */
unsigned short hotMapName2GID(hotCtx g, char *gname) {
    return mapName2GID(g, gname, NULL);
}

/* Map CID to GID */
unsigned short hotMapCID2GID(hotCtx g, unsigned short cid) {
    return mapCID2GID(g, cid);
}

/* Add anonymous table to otf font */
void hotAddAnonTable(hotCtx g, unsigned long tag, hotAnonRefill refill) {
    sfntAddAnonTable(g, tag, refill);
}

/* ---------------------------- Utility Functions --------------------------- */

void *hotMemNew(hotCtx g, size_t size) {
    void *ptr = malloc(size);
    if ( ptr == NULL )
        hotMsg(g, hotFATAL, "out of memory");
    return ptr;
}

void *hotMemResize(hotCtx g, void *old, size_t size) {
    void *ptr = realloc(old, size);
    if ( ptr == NULL )
        hotMsg(g, hotFATAL, "out of memory");
    return ptr;
}

void hotMemFree(hotCtx g, void *ptr) {
    free(ptr);
}

/* Call fatal if hadError is set (this is set by a hotMsg() hotERROR call) */
void hotQuitOnError(hotCtx g) {
    if (g->hadError) {
        hotMsg(g, hotFATAL, "aborting because of errors");
    }
}

/* xxx length notes:
   hotCMapID should have some max num chars, or else can't predict how long
   message would be. */
/* Print note, error, warning, or fatal message (from note buffer is fmt is
   NULL). If note used, handle reuse of g->note. */
void CDECL hotMsg(hotCtx g, int level, const char *fmt, ...) {
    if (g->cb.message != NULL) {
        // int lenName = g->font.FontName.cnt + 2;

        if (fmt == NULL) {
            /*if (g->font.FontName.cnt != 0) {
                int lenNote = g->note.cnt;
                dnaEXTEND(g->note, lenName);
                MOVE(&g->note.array[lenName], g->note.array, lenNote);
                sprintf(g->note.array, "<%s>", g->font.FontName.array);
                g->note.array[lenName - 1] = ' ';
            } */
            g->cb.message(g->cb.ctx, level, g->note.array);
        } else {
            va_list ap;
#define MAX_NOTE_LEN 1024
            char message[MAX_NOTE_LEN + 1024];
            char *p;
            size_t p_size;

            p_size = sizeof(message);
            /* if ((g->font.FontName.cnt != 0) && (lenName < MAX_NOTE_LEN)) {
                sprintf(message, "<%s> ", g->font.FontName.array);
                p = &message[lenName];
                p_size -= lenName;
            } else { */
                p = message;
            // }
            if ( level != hotHEADING ) {  // Don't print headings for headings
                char *premsg, *prefix;
                featMsgPrefix(g, &premsg, &prefix);
                // Usually information that the file has changed
                if ( premsg != NULL )
                    g->cb.message(g->cb.ctx, hotHEADING, premsg);

                if ( prefix != NULL ) {
                    // Usually feature file line and character numbers
                    int l = snprintf(p, p_size, "%s", prefix);
                    p += l;
                    p_size -= l;
                }
            }

            /* xxx If note is used, crop it to MAX_NOTE_LEN. */
            if (g->note.cnt > MAX_NOTE_LEN) {
                g->note.array[MAX_NOTE_LEN - 1] = '\0';
                g->note.array[MAX_NOTE_LEN - 2] = '.';
                g->note.array[MAX_NOTE_LEN - 3] = '.';
                g->note.array[MAX_NOTE_LEN - 4] = '.';
            }

            va_start(ap, fmt);
            VSPRINTF_S(p, p_size, fmt, ap);
            va_end(ap);
            g->cb.message(g->cb.ctx, level, message);
        }
    }

    g->note.cnt = 0;

    if (level == hotFATAL) {
        g->cb.fatal(g->cb.ctx);
    } else if (level == hotERROR && !g->hadError) {
        g->hadError = 1;
    }
}

/* Output OTF data as 2-byte number in big-endian order */
void hotOut2(hotCtx g, int16_t value) {
    g->cb.otfWrite1(g->cb.ctx, value >> 8);
    g->cb.otfWrite1(g->cb.ctx, value);
}

/* Output OTF data as 3-byte number in big-endian order */
void hotOut3(hotCtx g, int32_t value) {
    g->cb.otfWrite1(g->cb.ctx, value >> 16);
    g->cb.otfWrite1(g->cb.ctx, value >> 8);
    g->cb.otfWrite1(g->cb.ctx, value);
}

/* Output OTF data as 4-byte number in big-endian order */
void hotOut4(hotCtx g, int32_t value) {
    g->cb.otfWrite1(g->cb.ctx, value >> 24);
    g->cb.otfWrite1(g->cb.ctx, value >> 16);
    g->cb.otfWrite1(g->cb.ctx, value >> 8);
    g->cb.otfWrite1(g->cb.ctx, value);
}

/* Calculates the values of binary search table parameters */
void hotCalcSearchParams(unsigned unitSize, long nUnits,
                         unsigned short *searchRange,
                         unsigned short *entrySelector,
                         unsigned short *rangeShift) {
    long nextPwr;
    int pwr2;
    int log2;

    nextPwr = 2;
    for (log2 = 0; nextPwr <= nUnits; log2++) {
        nextPwr *= 2;
    }
    pwr2 = nextPwr / 2;

    *searchRange = unitSize * pwr2;
    *entrySelector = log2;
    *rangeShift = (unsigned short)(unitSize * (nUnits - pwr2));
}

/* Write Pascal string from null-terminated string */
void hotWritePString(hotCtx g, char *string) {
    int length = strlen(string);
    if (length > 255) {
        hotMsg(g, hotFATAL, "string too long");
    }
    g->cb.otfWrite1(g->cb.ctx, length);
    g->cb.otfWriteN(g->cb.ctx, length, string);
}

/* Get string from SID */
char *hotGetString(hotCtx g, SID sid, unsigned *length) {
    char *ptr;
    long offset;
    if (cffGetString(g->ctx.cff, sid, length, &ptr, &offset)) {
        return ptr; /* Standard string */
    } else {
        /* Non-standard string; fetch pointer to CFF data */
        return getCFFData(g, offset, *length);
    }
}

/* Encode integer and return length */
#if 0
static int encInteger(short i, unsigned char *cstr) {
    if (-107 <= i && i <= 107) {
        /* Single byte number */
        *cstr = (unsigned char)i + 139;
        return 1;
    } else if (108 <= i && i <= 1131) {
        /* +ve 2-byte number */
        i -= 108;
        *cstr++ = (unsigned char)((i >> 8) + 247);
        *cstr = (unsigned char)i;
        return 2;
    } else if (-1131 <= i && i <= -108) {
        /* -ve 2-byte number */
        i += 108;
        *cstr++ = (unsigned char)((-i >> 8) + 251);
        *cstr = (unsigned char)-i;
        return 2;
    } else {
        /* +ve/-ve 3-byte number */
        *cstr++ = (unsigned char)t2_shortint;
        *cstr++ = (unsigned char)(i >> 8);
        *cstr = (unsigned char)i;
        return 3;
    }
}
#endif

/* --------------------- Temporary Debugging Functions --------------------- */

#if 0
static void testmetrics(hotCtx g);

testmetrics(g);

static void testmetrics(hotCtx g) {
    static unsigned char metrics[] = {
        0xf9, 0x5a, 0x8b, 0x97, 0x91, 0x81, 0x81, 0x8b,
        0x8b, 0x8c, 0x10, 0x0e, 0xfb, 0x84, 0x99, 0x81,
        0x91, 0x8f, 0x8f, 0x85, 0x85, 0x8c, 0x10, 0x0e,
        0xde, 0x0e, 0xfa, 0x7c, 0xf7, 0x10, 0xf7, 0x3b,
        0xf7, 0x9b, 0x8b, 0xb3, 0xc7, 0xf7, 0x24, 0x8c,
        0x10, 0x0e, 0xf8, 0x11, 0xa7, 0xb1, 0xc8, 0x4d,
        0x77, 0x86, 0xa1, 0x8c, 0x10, 0x0e,
    };
    static unsigned long offsets[] = {
        0x0, 0xc, 0x18, 0x1a, 0x2a
    };
    int i;

    g->cff.data = (char *)metrics;
    g->cff.length = ARRAY_LEN(metrics);

    for (i = 0; i < ARRAY_LEN(offsets); i++) {
        int j;
        Fixed results[T2_MAX_OP_STACK];
        int cnt = cffExecMetric(g->ctx.cff, offsets[i], results);

        printf("metrics[%d]: ", i);
        for (j = 0; j < cnt; j++) {
            printf("%.4g ", results[j] / 65536.0);
        }
        printf("\n");
    }
}

#endif

#if 0
static BBox calcFontBBox(hotCtx g, unsigned *left, unsigned *bottom,
                         unsigned *right, unsigned *top) {
    int gid;
    BBox bbox;

    bbox.left = bbox.bottom = bbox.right = bbox.top = 0;
    *left = *bottom = *right = *top = 0;

    /* Get info on all glyphs */
    for (gid = 0; gid < g->font.glyphs.cnt; gid++) {
        cffGlyphInfo *gi = cffGetGlyphInfo(g->ctx.cff, gid);

        if (gi->bbox.left < bbox.left) {
            bbox.left = gi->bbox.left;
            *left = gid;
        }
        if (gi->bbox.right > bbox.right) {
            bbox.right = gi->bbox.right;
            *right = gid;
        }
        if (gi->bbox.bottom < bbox.bottom) {
            bbox.bottom = gi->bbox.bottom;
            *bottom = gid;
        }
        if (gi->bbox.top > bbox.top) {
            bbox.top = gi->bbox.top;
            *top = gid;
        }
#if 0
        printf("id=%hu, bbox={%4hd,%4hd,%4hd,%4hd}\n",
               gi->id,
               gi->bbox.left, gi->bbox.bottom,
               gi->bbox.right, gi->bbox.top);
#endif
    }

    return bbox;
}

static void newpath(void *ctx) {
    printf("newpath ");
}

static void moveto(void *ctx, cffFixed x1, cffFixed y1) {
    printf("%g %g moveto ", x1 / 65536.0, y1 / 65536.0);
}

static void lineto(void *ctx, cffFixed x1, cffFixed y1) {
    printf("%g %g lineto ", x1 / 65536.0, y1 / 65536.0);
}

static void curveto(void *ctx, int flex,
                    cffFixed x1, cffFixed y1,
                    cffFixed x2, cffFixed y2,
                    cffFixed x3, cffFixed y3) {
    printf("%g %g %g %g %g %g %s ",
           x1 / 65536.0, y1 / 65536.0,
           x2 / 65536.0, y2 / 65536.0,
           x3 / 65536.0, y3 / 65536.0,
           flex ? "flex" : "curveto");
}

static void closepath(void *ctx) {
    printf("closepath ");
}

static void endchar(void *ctx) {
    printf("endchar ");
}

static void hintstem(void *ctx, int vert, cffFixed edge0, cffFixed edge1) {
    printf("%g %g %cstem ", edge0 / 65536.0, edge1 / 65536.0, vert ? 'v' : 'h');
}

static void hintmask(void *ctx, int cntr, int n, char mask[CFF_MAX_MASK_BYTES]) {
    int i;
    printf("%smask[", cntr ? "cntr" : "hint");
    for (i = 0; i < n; i++) {
        printf("%02x", mask[i] & 0xff);
    }
    printf("] ");
}

static void testpathcb(hotCtx g) {
    int gid;
    cffPathCallbacks cb;

    cb.newpath = NULL;
    cb.moveto = moveto;
    cb.lineto = lineto;
    cb.curveto = curveto;
    cb.closepath = NULL;
    cb.endchar = endchar;
    cb.hintstem = hintstem;
    cb.hintmask = hintmask;

    for (gid = 0; gid < g->font.glyphs.cnt; gid++) {
        printf("--- gid %d\n", gid);
        (void)cffGetGlyphInfo(g->ctx.cff, gid, &cb);
        printf("\n");
    }
}

#endif
