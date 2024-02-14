/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Hatch OpenType.
 */

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits>

#include "head.h"
#include "hhea.h"
#include "hmtx.h"
#include "post.h"
#include "sfnt.h"
#include "sfntread.h"
#include "supportfp.h"
#include "hotmap.h"
#include "FeatCtx.h"
#include "name.h"
#include "GPOS.h"
#include "OS_2.h"
#include "dictops.h"
#include "varsupport.h"
#include "varvalue.h"

/* Windows-specific macros */
#define FAMILY_UNSET 255 /* Flags unset Windows Family field */
#define ANSI_CHARSET 0
#define SYMBOL_CHARSET 2

void hotAddAnonTable(hotCtx g, unsigned long tag, hotAnonRefill refill);

/* Initialize character name */
static void initCharName(void *ctx, long count, CharName *charname) {
    hotCtx g = (hotCtx)ctx;
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

static ctlMemoryCallbacks hot_memcb;

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

// ctlSharedStmCallbacks adapter for varsupport (terrible but temporary)
static void *sscbMemNew(ctlSharedStmCallbacks *h, size_t size) {
    return hotMemNew((hotCtx) h->direct_ctx, size);
}

static void sscbMemFree(ctlSharedStmCallbacks *h, void *ptr) {
    hotMemFree((hotCtx) h->direct_ctx, ptr);
}

static void sscbSeek(ctlSharedStmCallbacks *h, long offset) {
    hotCtx g = (hotCtx) h->direct_ctx;
    g->cb.stm.seek(&g->cb.stm, g->in_stream, offset);
    g->bufleft = 0;
}

static long sscbTell(ctlSharedStmCallbacks *h) {
    hotCtx g = (hotCtx) h->direct_ctx;
    return g->cb.stm.tell(&g->cb.stm, g->in_stream) - g->bufleft;
}

static void sscbRead(ctlSharedStmCallbacks *h, size_t count, char *ptr) {
    hotCtx g = (hotCtx) h->direct_ctx;
    hotMsg(g, hotFATAL, "General reads not supported");
}

static uint8_t sscbRead1(ctlSharedStmCallbacks *h) {
    return hin1((hotCtx) h->direct_ctx);
}

static uint16_t sscbRead2(ctlSharedStmCallbacks *h) {
    return hotIn2((hotCtx) h->direct_ctx);
}

static uint32_t sscbRead4(ctlSharedStmCallbacks *h) {
    return hotIn4((hotCtx) h->direct_ctx);
}

static void sscbMessage(ctlSharedStmCallbacks *h, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    hotCtx g = (hotCtx) h->direct_ctx;
    g->logger->vlog(sWARNING, msg, ap);
    va_end(ap);
}

void hotMakeSSC(hotCtx g, ctlSharedStmCallbacks &c) {
    c.direct_ctx = (void *) g;
    c.dna = g->DnaCTX;
    c.memNew = sscbMemNew;
    c.memFree = sscbMemFree;
    c.seek = sscbSeek;
    c.tell = sscbTell;
    c.read = sscbRead;
    c.read1 = sscbRead1;
    c.read2 = sscbRead2;
    c.read4 = sscbRead4;
    c.message = sscbMessage;
}

hotCtx hotNew(hotCallbacks *hotcb, std::shared_ptr<GOADB> goadb,
              std::shared_ptr<slogger> logger) {
    time_t now;
    hotCtx g = new struct hotCtx_;

    if (g == NULL) {
        if (logger)
            logger->msg(sFATAL, "out of memory");
        hotcb->fatal(hotcb->ctx);
    }

    g->hadError = false;
    g->convertFlags = 0;

    /* Set version numbers. The hot library version serves to identify the      */
    /* software version that built an OTF font and is saved in the Version name */
    /* in the name table. The font version serves to identify the OTF font data */
    /* and is saved in the head table and the Unique and Version names in the   */
    /* name table. */
    g->version = HOT_VERSION;
    g->font.version.otf = (1 << 16); /* 1.0 in Fixed */
    g->font.vendId = "";

    /* Get current time */

    time(&now);
    SAFE_LOCALTIME(&now, &g->time);

    /* Initialize hot library callbacks */
    g->cb = *hotcb;
    hot_memcb.ctx = g;
    hot_memcb.manage = hot_manage;

    /* Initialize contexts for safe freeing */
    g->DnaCTX = NULL;
    g->ctx.map = NULL;
    g->ctx.feat = NULL;
    g->ctx.cfr = NULL;
    g->ctx.BASE = NULL;
    g->ctx.GDEFp = nullptr;
    g->ctx.GPOSp = nullptr;
    g->ctx.GSUBp = nullptr;
    g->ctx.OS_2 = NULL;
    g->ctx.cmap = NULL;
    g->ctx.head = NULL;
    g->ctx.hhea = NULL;
    g->ctx.hmtxp = nullptr;
    g->ctx.name = NULL;
    g->ctx.post = NULL;
    g->ctx.sfnt = NULL;
    g->ctx.vhea = NULL;
    g->ctx.vmtxp = nullptr;
    g->ctx.VORG = NULL;
    g->ctx.axes = nullptr;
    g->ctx.locMap = nullptr;

    g->DnaCTX = dnaNew(&hot_memcb, DNA_CHECK_ARGS);
    dnaINIT(g->DnaCTX, g->note, 1024, 1024);

    hotMakeSSC(g, g->sscb);

    /* Initialize font information */
#if HOT_DEBUG
    g->font.debug = 0;
#endif /* HOT_DEBUG */
    g->font.flags = 0;
    g->font.fsSelectionMask_on = -1;
    g->font.fsSelectionMask_off = -1;
    g->font.os2Version = 0;
    dnaINIT(g->DnaCTX, g->font.kern.pairs, 1500, 1000);
    dnaINIT(g->DnaCTX, g->font.kern.values, 1500, 8500);
    dnaINIT(g->DnaCTX, g->font.unenc, 30, 70);
    g->font.unenc.func = initCharName;
    g->bufnext = NULL;
    g->bufleft = 0;

    initOverrides(g);

    /* Initialize modules */
    sfntNew(g);
    mapNew(g);
    g->ctx.feat = new FeatCtx(g);

    if (logger)
        g->logger = logger;
    else
        g->logger = slogger::getLogger("hotconv");

    g->goadb = goadb;

    return g;
}

void setVendId_str(hotCtx g, const char *vend) {
    char *id;

    int l = strlen(vend) + 1;
    id = (char *)hotMemNew(g, l);
    STRCPY_S(id, l, vend);
    g->font.vendId = id;
}

/* Try to set vendor id by matching against copyright strings in the font */
static void setVendId(hotCtx g) {
    typedef struct {
        const char *string;
        const char *id;
    } Match;
    static Match vendor[] = {  // Arranged by most likely first
        {"Adobe", "ADBE"},
    };

    if (!g->font.vendId.empty())
        return; /*Must have been set by feature file*/

    /* Match vendor */
    for (uint32_t i = 0; i < ARRAY_LEN(vendor); i++) {
        std::string vid = vendor[i].string;
        if (g->font.Notice.find(vid) != std::string::npos) {
            g->font.vendId = vendor[i].id;
            return;
        }
    }
    /* No information; set default */
    g->font.vendId = "UKWN"; /* xxx Is this correct? */
}

static void hotGlyphEnd(abfGlyphCallbacks *cb) {
    hotCtx g = (hotCtx)cb->indirect_ctx;
    abfMetricsCtx h = (abfMetricsCtx)cb->direct_ctx;

    abfGlyphMetricsCallbacks.end(cb);

    uint16_t gid = cb->info->tag;
    auto &glyph = g->glyphs[gid];

    if (cb->info->gname.ptr != NULL)
        glyph.gname = cb->info->gname.ptr;
    if (IS_CID(g))
        glyph.id = cb->info->cid;
    else
        glyph.id = cb->info->gname.impl;  // The SID (when CFF 1)
    glyph.code = cb->info->encoding.code;
    abfEncoding *e = cb->info->encoding.next;
    while (e != NULL) {
        glyph.sup.push_back(e->code);
        e = e->next;
    }
    glyph.hAdv = (FWord) h->int_mtx.hAdv;
    glyph.vAdv = SHRT_MAX;
    glyph.bbox.left = h->int_mtx.left;
    glyph.bbox.bottom = h->int_mtx.bottom;
    glyph.bbox.right = h->int_mtx.right;
    glyph.bbox.top = h->int_mtx.top;
    glyph.vOrigY = SHRT_MAX;
    if (glyph.bbox.left != 0 || glyph.bbox.right != 0 ||
        glyph.bbox.bottom != 0 || glyph.bbox.top != 0) {
        if (g->font.maxAdv.h < glyph.hAdv)
            g->font.maxAdv.h = glyph.hAdv;

        if (glyph.bbox.left < g->font.minBearing.left)
            g->font.minBearing.left = glyph.bbox.left;

        if (glyph.hAdv - glyph.bbox.right < g->font.minBearing.right)
            g->font.minBearing.right = glyph.hAdv - glyph.bbox.right;

        if (glyph.bbox.right > g->font.maxExtent.h)
            g->font.maxExtent.h = glyph.bbox.right;
    }
}

static void hotReadTables(hotCtx g) {
    g->in_stream = g->cb.stm.open(&g->cb.stm, CFR_SRC_STREAM_ID, 0);

    sfrCtx sfr = sfrNew(&hot_memcb, &g->cb.stm, SFR_CHECK_ARGS);
    ctlTag sfnt_tag;

    int result = sfrBegFont(sfr, g->in_stream, 0, &sfnt_tag);
    if (result == sfrSuccess) {
        if (sfnt_tag == sfr_OTTO_tag) {
            sfrTable *table = sfrGetTableByTag(sfr, CFF__);
            if (table != NULL) {
                sfntOverrideTable(g, CFF__, table->offset, table->length);
            } else {
                table = sfrGetTableByTag(sfr, CFF2_);
                if (table != NULL) {
                    sfntOverrideTable(g, CFF2_, table->offset, table->length);
                    table = sfrGetTableByTag(sfr, head_);
                    if (table == NULL)
                        hotMsg(g, hotFATAL, "CFF2 sfnt must contain head table");
                    headRead(g, table->offset, table->length);
                    table = sfrGetTableByTag(sfr, hhea_);
                    if (table == NULL)
                        hotMsg(g, hotFATAL, "CFF2 sfnt must contain hhea table");
                    hheaRead(g, table->offset, table->length);
                    table = sfrGetTableByTag(sfr, hmtx_);
                    if (table == NULL)
                        hotMsg(g, hotFATAL, "CFF2 sfnt must contain hmtx table");
                    sfntOverrideTable(g, hmtx_, table->offset, table->length);
                    table = sfrGetTableByTag(sfr, name_);
                    if (table == NULL)
                        hotMsg(g, hotFATAL, "CFF2 sfnt must contain name table");
                    nameRead(g, table->offset, table->length);
                    table = sfrGetTableByTag(sfr, post_);
                    if (table != NULL) {
                        postRead(g, table->offset, table->length);
                    } else {
                        hotMsg(g, hotWARNING, "CFF2 sfnt does not include post table");
                    }
                    table = sfrGetTableByTag(sfr, fvar_);
                    if (table != NULL) {
                        sfntOverrideTable(g, fvar_, table->offset, table->length);
                        g->ctx.axes = new var_axes(sfr, &g->sscb);
                    }
                    table = sfrGetTableByTag(sfr, avar_);
                    if (table != NULL) {
                        sfntOverrideTable(g, avar_, table->offset, table->length);
                    }
                    table = sfrGetTableByTag(sfr, HVAR_);
                    if (table != NULL) {
                        sfntOverrideTable(g, HVAR_, table->offset, table->length);
                    }
                    // temporary, will read in and write back out
                    table = sfrGetTableByTag(sfr, MVAR_);
                    if (table != NULL)
                        sfntOverrideTable(g, MVAR_, table->offset, table->length);
                }
            }
            // call name function to read in extra labels
        } else {
            hotMsg(g, hotFATAL, "sfnt file must have CFF or CFF2 table");
        }
    } else {
        g->cb.stm.seek(&g->cb.stm, g->in_stream, 0);
        char first = hotFillBuf(g);
        if (g->bufleft < 3)
            hotMsg(g, hotFATAL, "Bad font file");
        else if (first == 2)
            hotMsg(g, hotFATAL, "Bad font file (or bare CFF2 table)");
        else if (first != 1)
            hotMsg(g, hotFATAL, "Bad font file");
        else
            sfntOverrideTable(g, CFF__, 0, -1);
    }
    g->cb.stm.seek(&g->cb.stm, g->in_stream, 0);
    sfrFree(sfr);
}

/* Convert PostScript font to CFF and read result */
const char *hotReadFont(hotCtx g, int flags, bool &isCID) {
    abfTopDict *top;

    hotReadTables(g);

    if (g->ctx.axes != nullptr)
        g->ctx.locMap = new VarLocationMap(g->ctx.axes->getAxisCount());

    /* Copy conversion flags */
    g->font.flags = 0;
#if HOT_DEBUG
    g->font.debug = flags & HOT_DB_MASK;
#endif /* HOT_DEBUG */

    /* Parse CFF data and get global font information */
    g->ctx.cfr = cfrNew(&hot_memcb, &g->cb.stm, CFR_CHECK_ARGS, g->logger);
    cfrBegFont(g->ctx.cfr, 0, 0, 0, &top, NULL);

    /* Create and copy font strings */
    if ((top->sup.flags & ABF_CID_FONT) && top->cid.CIDFontName.ptr) {
        g->font.FontName = top->cid.CIDFontName.ptr;
    } else if (top->FDArray.cnt > 0 && top->FDArray.array[0].FontName.ptr) {
        // XXX what about CID?
        g->font.FontName = top->FDArray.array[0].FontName.ptr;
    }
    if (top->Notice.ptr)
        g->font.Notice = top->Notice.ptr;
    if (top->FamilyName.ptr)
        g->font.FamilyName = top->FamilyName.ptr;
    if (top->FullName.ptr)
        g->font.FullName = top->FullName.ptr;
    if (top->version.ptr)
        g->font.version.PS = top->version.ptr;
    if (g->font.version.PS.empty())
        g->font.version.PS = "(version unavailable)";
    std::string &n = g->font.FontName.empty() ? g->font.FullName : g->font.FontName;
    hotMsg(g, hotFLUSH, "%s processing font <%s>", "addfeatures", n.c_str());

    /* Copy basic font information */
    g->font.bbox.left = top->FontBBox[0];
    g->font.bbox.bottom = top->FontBBox[1];
    g->font.bbox.right = top->FontBBox[2];
    g->font.bbox.top = top->FontBBox[3];
    g->font.minBearing.left = g->font.minBearing.right = SHRT_MAX;
    g->font.maxAdv.h = g->font.maxAdv.v = 0;
    g->font.maxExtent.h = g->font.maxExtent.v = 0;
    if (top->isFixedPitch) {
        g->font.flags |= FI_FIXED_PITCH;
    }
    g->font.ItalicAngle = pflttofix(&top->ItalicAngle);
    g->font.UnderlinePosition = top->UnderlinePosition;
    g->font.UnderlineThickness = top->UnderlineThickness;

    if (top->sup.flags & ABF_CID_FONT) {
        /* Copy CIDFont data */
        if (top->cid.Registry.ptr)
            g->font.cid.registry = top->cid.Registry.ptr;
        if (top->cid.Ordering.ptr)
            g->font.cid.ordering = top->cid.Ordering.ptr;
        g->font.cid.supplement = top->cid.Supplement;
        g->font.flags |= FI_CID;
    }

    g->font.unitsPerEm = top->sup.UnitsPerEm;

    const cfrSingleRegions *cfrSR = cfrGetSingleRegions(g->ctx.cfr);
    if (cfrSR->Encoding.begin == cff_StandardEncoding)
        g->font.Encoding = FI_STD_ENC;
    else if (cfrSR->Encoding.begin == cff_ExpertEncoding)
        g->font.Encoding = FI_EXP_ENC;
    else
        g->font.Encoding = FI_CUSTOM_ENC;

    g->glyphs.resize(top->sup.nGlyphs);

    abfGlyphCallbacks cb = abfGlyphMetricsCallbacks;
    struct abfMetricsCtx_ metricsCtx;
    memset(&metricsCtx, 0, sizeof(metricsCtx));
    cb.direct_ctx = &metricsCtx;
    cb.indirect_ctx = g;
    cb.end = hotGlyphEnd;
    cfrIterateGlyphs(g->ctx.cfr, &cb);

    if (g->font.minBearing.left == SHRT_MAX)
        g->font.minBearing.left = 0;
    if (g->font.minBearing.right == SHRT_MAX)
        g->font.minBearing.right = 0;

    setVendId(g);

    isCID = IS_CID(g);

    return g->font.FontName.c_str();
}

/* Add glyph's vertical origin y-value to glyph info. (Not an API.) */
void hotAddVertOriginY(hotCtx g, GID gid, short value) {
    hotGlyphInfo *hotgi = &g->glyphs[gid]; /* gid already validated */

    if (!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) {
        g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;
    }

    if (hotgi->vOrigY != SHRT_MAX) {
        g->ctx.feat->dumpGlyph(gid, '\0', 0);
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
    hotGlyphInfo &hotgi = g->glyphs[gid]; /* gid already validated */
    if (!(g->convertFlags & HOT_SEEN_VERT_ORIGIN_OVERRIDE)) {
        g->convertFlags |= HOT_SEEN_VERT_ORIGIN_OVERRIDE;
    }

    if (hotgi.vAdv != SHRT_MAX) {
        g->ctx.feat->dumpGlyph(gid, '\0', 0);
        if (hotgi.vAdv == value) {
            hotMsg(g, hotNOTE,
                   "Ignoring duplicate VertAdvanceY entry for "
                   "glyph %s", g->note.array);
        } else {
            hotMsg(g, hotFATAL,
                   "VertAdvanceY redefined for "
                   "glyph %s", g->note.array);
        }
    } else {
        hotgi.vAdv = -value;
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
    font->win.DefaultChar = 0; /* if 0, this indicates GID 0. The previous use of the UV for space */
                               /* was a leftover from when this module was written to change as    */
                               /* little as possible when converting legacy PS fonts to OTF.       */
    if (font->win.Family == FAMILY_UNSET) {
        /* Converting from non-Windows data; synthesize makepfm algorithm */
        int StdEnc = font->Encoding == FI_STD_ENC;
        int spacePresent = mapUV2Glyph(g, UV_SPACE) != NULL;

        font->win.Family = StdEnc ? HOT_ROMAN : HOT_DECORATIVE;
        /* xxx should this change to the new pcmaster algorithm */
        font->win.CharSet =
            (strcmp(font->FontName.c_str(), "Symbol") == 0) ? SYMBOL_CHARSET : ANSI_CHARSET;

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
    if (font->TypoLineGap < 0) {
        hotMsg(g, hotWARNING, "The feature file OS/2 override TypoLineGap value is negative!");
    }

    /* No need to duplicate PFM avgCharWidth algorithm: Euro, Zcarons added to
       WinANSI. This algorithm is encoding-dependent. */
    sum = 0;
    count = 0;
    for (i = 0; i < g->glyphs.size(); i++) {
        if (g->glyphs[i].hAdv > 0) {
            sum += g->glyphs[i].hAdv;
            count++;
        }
    }
    if (count > 0) {
        g->font.win.AvgWidth = (short)(sum / count);
    } else {
        g->font.win.AvgWidth = 0;
    }
}

/* Must be called after FeatCtx::fill(), since TypoAscender/TypoDescender could have
   been overwritten */
static void setVBounds(hotCtx g) {
    FontInfo_ *font = &g->font;
    BBox *minBearing = &font->minBearing;
    hvMetric *maxAdv = &font->maxAdv;
    hvMetric *maxExtent = &font->maxExtent;
    short dfltVAdv = -font->TypoAscender + font->TypoDescender;
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
    for (size_t i = 0; i < g->glyphs.size(); i++) {
        hotGlyphInfo *glyph = &g->glyphs[i];
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

static unsigned int dsigCnt = 0;

static void hotReuse(hotCtx g) {
    g->ctx.feat->dumpLocationDefs();
    if (g->ctx.locMap != nullptr)
        g->ctx.locMap->toerr();

    g->hadError = false;
    g->convertFlags = 0;
    delete g->ctx.locMap;
    g->ctx.locMap = nullptr;
    delete g->ctx.axes;
    g->ctx.axes = nullptr;

    initOverrides(g);
    sfntReuse(g);
    mapReuse(g);
    delete g->ctx.feat;
    g->ctx.feat = new FeatCtx(g);
    dsigCnt = 0;
}

char *refillDSIG(void *ctx, long *count, unsigned long tag) {
    static const char data[] = "\x00\x00\x00\x01\x00\x00\x00";
    if (dsigCnt == 0) {
        *count = 8;
        dsigCnt = 1;
        return (char *)(&data);
    } else {
        *count = 0;
        return (char *)NULL;
    }
}

/* Convert to OTF */
void hotConvert(hotCtx g) {
    mapFill(g);
    g->ctx.feat->fill();

    prepWinData(g);

    setVBounds(g);

    if (g->convertFlags & HOT_ADD_STUB_DSIG)
        hotAddAnonTable(g, TAG('D', 'S', 'I', 'G'), refillDSIG);

    g->out_stream = g->cb.stm.open(&g->cb.stm, OTF_DST_STREAM_ID, 0);
    sfntFill(g);
    cfrFree(g->ctx.cfr);
    sfntWrite(g);

    hotReuse(g);
}

void hotFree(hotCtx g) {
    int i;

    delete g->ctx.locMap;
    delete g->ctx.axes;
    sfntFree(g);
    mapFree(g);
    delete g->ctx.feat;

    dnaFREE(g->note);
    dnaFREE(g->font.kern.pairs);
    dnaFREE(g->font.kern.values);

    if (g->font.unenc.size != 0) {
        for (i = 0; i < g->font.unenc.size; i++) {
            dnaFREE(g->font.unenc.array[i]);
        }
        dnaFREE(g->font.unenc);
    }
    dnaFree(g->DnaCTX);
    delete g;
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
    if (g->convertFlags & HOT_LOOKUP_FINAL_NAMES)
        g->goadb->useFinalNames();
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

    g->tmp.clear();
    int state = 0;
    unsigned value = 0;

    for (;;) {
        int hexdig = 0; /* Converted hex digit (suppress optimizer warning) */
        int cls;      /* Character cls */
        int actn;       /* Action flags */
        int c = *src++;

        switch (c) {
            case '\\':
                cls = 0;
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
                cls = 1;
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                hexdig = c - 'a' + 10;
                cls = 2;
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                hexdig = c - 'A' + 10;
                cls = 3;
                break;

            default:
                cls = 4;
                break;

            case '\0': /* End of string */
                cls = 5;
                break;
        }

        actn = action[state][cls];
        state = next[state][cls];

        /* Perform actions */
        if (actn == 0) {
            continue;
        }
        if (actn & E_) {
            return 1; /* Syntax error */
        }
        if (actn & B_) {
            g->tmp.push_back(c);
        }
        if (actn & A_) {
            value = value << 4 | hexdig;
        }
        if (actn & H_) {
            /* Save 16-bit value in UTF-8 format */
            if (value == 0) {
                return 1; /* Syntax error */
            } else if (value < 0x80) {
                g->tmp.push_back(value);
            } else if (value < 0x800) {
                g->tmp.push_back(0xc0 | value >> 6);
                g->tmp.push_back(0x80 | (value & 0x3f));
            } else {
                g->tmp.push_back(0xe0 | value >> 12);
                g->tmp.push_back(0x80 | (value >> 6 & 0x3f));
                g->tmp.push_back(0x80 | (value & 0x3f));
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

    g->tmp.clear();
    int state = 0;
    unsigned value = 0;

    for (;;) {
        int hexdig = 0; /* Converted hex digit (suppress optimizer warning) */
        int cls;      /* Character cls */
        int actn;       /* Action flags */
        int c = *src++;

        /* Direct UTF-8 input is not supported. */
        if (c < 0)
            return 1;

        switch (c) {
            case '\\':
                cls = 0;
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
                cls = 1;
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                hexdig = c - 'a' + 10;
                cls = 2;
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                hexdig = c - 'A' + 10;
                cls = 3;
                break;

            default:
                cls = 4;
                break;

            case '\0': /* End of string */
                cls = 5;
                break;
        }

        actn = action[state][cls];
        state = next[state][cls];

        /* Perform actions */
        if (actn == 0) {
            continue;
        }
        if (actn & E_) {
            return 1; /* Syntax error */
        }
        if (actn & B_) {
            g->tmp.push_back(c);
        }
        if (actn & A_) {
            value = value << 4 | hexdig;
        }
        if (actn & H_) {
            if (value == 0) {
                return 1; /* Syntax error */
            }
            g->tmp.push_back(value);
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
    nameAddReg(g, platformId, platspecId, languageId, nameId, g->tmp.c_str());
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
        int l = strlen(name) + 1;
        STRCPY_S(dnaGROW(g->font.unenc.array[iChar], l), l, name);
    }
}

/* Add Adobe CMap */
void hotAddCMap(hotCtx g, hotCMapId id, hotCMapRefill refill) {
    mapAddCMap(g, id, refill);
}

/* Add Unicode variation Selector cmap subtable.  */
void hotAddUVSMap(hotCtx g, const char *uvsName) {
    mapAddUVS(g, uvsName);
}

/* Map platform encoding to GID */
unsigned short hotMapPlatEnc2GID(hotCtx g, int code) {
    return mapPlatEnc2GID(g, code);
}

/* Map glyph name to GID */
unsigned short hotMapName2GID(hotCtx g, const char *gname) {
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

/* Print note, error, warning, or fatal message (from note buffer is fmt is
   NULL). If note used, handle reuse of g->note. */
void CTL_CDECL hotMsg(hotCtx g, int level, const char *fmt, ...) {
    int slevel = sINFO;

    switch (level) {
        case hotINDENT:
            slevel = sINDENT;
            break;
        case hotFLUSH:
            slevel = sFLUSH;
            break;
        case hotWARNING:
            slevel = sWARNING;
            break;
        case hotERROR:
            slevel = sERROR;
            break;
        case hotFATAL:
            slevel = sFATAL;
            break;
    }

    if (fmt == NULL) {
        g->logger->msg(slevel, g->note.array);
    } else {
        va_list ap;
#define MAX_NOTE_LEN 1024
        std::string prefix = g->ctx.feat->msgPrefix(), sfmt(fmt);

        sfmt.insert(0, prefix);
        /* xxx If note is used, crop it to MAX_NOTE_LEN. */
        if (g->note.cnt > MAX_NOTE_LEN) {
            g->note.array[MAX_NOTE_LEN - 1] = '\0';
            g->note.array[MAX_NOTE_LEN - 2] = '.';
            g->note.array[MAX_NOTE_LEN - 3] = '.';
            g->note.array[MAX_NOTE_LEN - 4] = '.';
        }

        va_start(ap, fmt);
        g->logger->vlog(slevel, sfmt.c_str(), ap);
        va_end(ap);
    }

    g->note.cnt = 0;

    if (level == hotFATAL) {
        g->cb.fatal(g->cb.ctx);
    } else if (level == hotERROR && !g->hadError) {
        g->hadError = true;
    }
}

/* Replenish input buffer using refill function */
char hotFillBuf(hotCtx g) {
    g->bufleft = g->cb.stm.read(&g->cb.stm, g->in_stream, &g->bufnext);
    if (g->bufleft-- == 0) {
        hotMsg(g, hotFATAL, "premature end of input");
    }
    return *g->bufnext++;
}

/* Input OTF data as 2-byte number in big-endian order */
int16_t hotIn2(hotCtx g) {
    uint16_t result;
    result = (uint8_t)hin1(g) << 8;
    result |= (uint8_t)hin1(g);
    return (int16_t) result;
}

/* Input OTF data as 4-byte number in big-endian order */
int32_t hotIn4(hotCtx g) {
    uint32_t result;
    result = (uint8_t)hin1(g) << 24;
    result |= (uint8_t)hin1(g) << 16;
    result |= (uint8_t)hin1(g) << 8;
    result |= (uint8_t)hin1(g);
    return (int32_t)result;
}

/* Output OTF data as 2-byte number in big-endian order */
void hotOut2(hotCtx g, int16_t value) {
    hout1(g, value >> 8);
    hout1(g, value);
}

/* Output OTF data as 3-byte number in big-endian order */
void hotOut3(hotCtx g, int32_t value) {
    hout1(g, value >> 16);
    hout1(g, value >> 8);
    hout1(g, value);
}

/* Output OTF data as 4-byte number in big-endian order */
void hotOut4(hotCtx g, int32_t value) {
    hout1(g, value >> 24);
    hout1(g, value >> 16);
    hout1(g, value >> 8);
    hout1(g, value);
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
    hout1(g, length);
    g->cb.stm.write(&g->cb.stm, g->out_stream, length, string);
}
