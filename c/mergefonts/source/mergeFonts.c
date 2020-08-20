/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * mergeFonts. A minor modification of the tx code base.
 */

#include "tx_shared.h"

#define MERGEFONTS_VERSION CTL_MAKE_VERSION(1, 2, 3) /* derived from tx */

#ifdef __cplusplus
extern "C" {
#endif

#define ABF_DICTS_MATCH 0

typedef long (*CompareFDArrayCall)(void *dest_ctx, abfTopDict *srcTop);
typedef long (*MergeFDArrayCall)(void *dest_ctx, abfTopDict *topDict, int *newFDIndex);

#ifdef __cplusplus
}
#endif

#define MAX_PS_NAME_LEN 128

typedef struct {
    char srcName[MAX_PS_NAME_LEN];
    char dstName[MAX_PS_NAME_LEN];
    long srcCID;
    long dstCID;
} GAEntry;

enum /* Option enumeration */
{
    gafBothCID = 0, /* both src and dst are CID values .*/
    gafSrcCID,      /*  src is a CID value, dst is a name string. */
    gafDstCID,      /*  src is a name string, dst is a CID value. */
    gafBothName     /* both src and dst are name strings. */
};

#define MAX_DICT_ENTRY_LEN 128
#define MAX_COPYRIGHT_LEN 32000

#define MAX_MERGE_FILES 400
typedef struct {
    int gaType;
    char FontName[MAX_DICT_ENTRY_LEN];
    long LanguageGroup;
    dnaDCL(GAEntry, gaEntrySet);       // This holds the original and new glyph names
    dnaDCL(abfGlyphInfo, seenGlyphs);  // this holds the new abfGlyphInfo info for each glyph, so I can free them at the end of the merge.
} GAFileInfo;

typedef struct {
    char CIDFontName[MAX_DICT_ENTRY_LEN];
    char FullName[MAX_DICT_ENTRY_LEN];
    char FamilyName[MAX_DICT_ENTRY_LEN];
    char AdobeCopyright[MAX_COPYRIGHT_LEN];
    char Trademark[MAX_COPYRIGHT_LEN];
    char Weight[MAX_DICT_ENTRY_LEN];
    long isFixedPitch;
    long fsType;
    char Registry[MAX_DICT_ENTRY_LEN];
    char Ordering[MAX_DICT_ENTRY_LEN];
    long Supplement;
    float fontVersion;
} CIDInfo;

/* For merging fonts:
dna's  for holding all the source font ctx's which we need to free AFTER dest->endFont is called.
*/
typedef struct {
    dnaDCL(cfrCtx, cfr);
    dnaDCL(t1rCtx, t1r);
    dnaDCL(svrCtx, svr);
    dnaDCL(ufoCtx, ufr);
} sourceCtx;

typedef struct
{
    abfTopDict *srcTopDict;            /* current source font top dict */
    CompareFDArrayCall compareFDArray; /* function to compare source font dicts with destination font */
    MergeFDArrayCall mergeFDArray;     /* function to merge all of source FDArray font dicts into destination font */
    abfGlyphBegCallback mergeGlyphBeg; /* save original glyph beg() function */
    dnaDCL(int, newiFDArray);          /* Array holding the destination font iFD for each source font dict. */
    dnaDCL(int, glyph);                /* Array holding the destination font iFD for each source font dict. */
    dnaDCL(GAFileInfo, glyphAliasSet); /* Array of dynamic arrays, one for each font. */
    CIDInfo cidinfo;                   /* info used to convert a non-CID font to a CID font. */
    unsigned short fileIndex;
    unsigned short curGAEIndex; /* current index of the GAE entry  Must be set in */
    bool seenNotdef;
    int mode;                /* the file type of the first font. This is what sets the output font file type. */
    bool hintsOnly;       /* copy hints only */
    bool compareNameOnly; /* compare hint dict to source font dict by name only. Not yet used - at the moment, mergeFonts compares the fontName only. */
    struct
    {
        long cnt; /* ABF_EMPTY_ARRAY */
        long array[16];
    } XUID;
} MergeInfo;

/* -------------------------------- Options -------------------------------- */

/* Note: options.h must be ascii sorted by option string */

enum /* Option enumeration */
{
    opt_None, /* Not an option */
#define DCL_OPT(string, index) index,
#include "options.h"
    opt_Count
};

static const char *options[] =
    {
#undef DCL_OPT
#define DCL_OPT(string, index) string,
#include "options.h"
};

/* ------------------------------ Help Text -------------------------------- */

static void afm_Help(txCtx h) {
    static char *text[] = {
#include "afm.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void cef_Help(txCtx h) {
    static char *text[] = {
#include "cef.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void cff_Help(txCtx h) {
    static char *text[] = {
#include "cff.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void dcf_Help(txCtx h) {
    static char *text[] = {
#include "dcf.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void dump_Help(txCtx h) {
    static char *text[] = {
#include "dump.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void mtx_Help(txCtx h) {
    static char *text[] = {
#include "mtx.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void path_Help(txCtx h) {
    static char *text[] = {
#include "path.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void pdf_Help(txCtx h) {
    static char *text[] = {
#include "pdf.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void ps_Help(txCtx h) {
    static char *text[] = {
#include "ps.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void svg_Help(txCtx h) {
    static char *text[] = {
#include "svg.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void t1_Help(txCtx h) {
    static char *text[] = {
#include "t1.h"
    };
    printText(ARRAY_LEN(text), text);
}

static void ufo_Help(txCtx h) {
    static char *text[] = {
#include "ufo.h"
    };
    printText(ARRAY_LEN(text), text);
}

/* --------------------------------- t1 mode ------------------------------- */

static bool applyCIDFontInfo(txCtx h, bool fontisCID) {
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;
    if (mergeInfo->cidinfo.CIDFontName[0] != 0) /* if the users has specified a cidfontinfo file, override the first font's cid info. */
    {
        fontisCID = 1;
        h->top->sup.flags |= ABF_CID_FONT;
        h->top->cid.CIDFontName.ptr = mergeInfo->cidinfo.CIDFontName;
        h->top->FullName.ptr = mergeInfo->cidinfo.FullName;
        h->top->FamilyName.ptr = mergeInfo->cidinfo.FamilyName;
        h->top->Copyright.ptr = mergeInfo->cidinfo.AdobeCopyright;
        if ((mergeInfo->cidinfo.AdobeCopyright[0] != ABF_EMPTY_ARRAY) ||
            (mergeInfo->cidinfo.Trademark[0] != ABF_EMPTY_ARRAY)) {
            h->top->Copyright.ptr = ABF_UNSET_PTR;
            h->top->Notice.ptr = ABF_UNSET_PTR;
        }
        if (mergeInfo->cidinfo.AdobeCopyright[0] != ABF_EMPTY_ARRAY) {
            h->top->Notice.ptr = mergeInfo->cidinfo.AdobeCopyright;
        }
        if (mergeInfo->cidinfo.Trademark[0] != ABF_EMPTY_ARRAY) {
            if (mergeInfo->cidinfo.AdobeCopyright[0] == ABF_EMPTY_ARRAY)
                h->top->Notice.ptr = mergeInfo->cidinfo.Trademark;
            else {
                strcat(h->top->Notice.ptr, " ");
                strcat(h->top->Notice.ptr, mergeInfo->cidinfo.Trademark);
            }
        }
        h->top->Weight.ptr = mergeInfo->cidinfo.Weight;
        if (mergeInfo->cidinfo.isFixedPitch != ABF_UNSET_INT)
            h->top->isFixedPitch = mergeInfo->cidinfo.isFixedPitch;
        if (mergeInfo->cidinfo.fsType != ABF_UNSET_INT)
            h->top->FSType = mergeInfo->cidinfo.fsType;
        if (mergeInfo->XUID.cnt > 0) {
            int x;
            h->top->XUID.cnt = mergeInfo->XUID.cnt;
            for (x = 0; x < mergeInfo->XUID.cnt; x++)
                h->top->XUID.array[x] = mergeInfo->XUID.array[x];
        }
        h->top->cid.Registry.ptr = mergeInfo->cidinfo.Registry;
        h->top->cid.Ordering.ptr = mergeInfo->cidinfo.Ordering;
        h->top->cid.Supplement = mergeInfo->cidinfo.Supplement;
        h->top->cid.CIDFontVersion = mergeInfo->cidinfo.fontVersion;
        h->top->version.ptr = NULL;
    }

    return fontisCID;
}

static GAFileInfo *checkIFParentCIDCompatible(txCtx h, abfTopDict *local_top, bool parentIsCID, bool localFontIsCID) {
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;
    unsigned short fileIndex = mergeInfo->fileIndex;

    /* If there is a glyph alias file for the first font, make sure it is the right type. */
    if ((mergeInfo->glyphAliasSet.cnt > 0) && (mergeInfo->glyphAliasSet.array[fileIndex].gaEntrySet.cnt > 0)) {
        gaf = &mergeInfo->glyphAliasSet.array[fileIndex];
        if (parentIsCID) {
            int ifd;
            if ((gaf->gaType == gafBothName) || (gaf->gaType == gafSrcCID))
                fatal(h, "Parent font is not a CID font, but its matching glyph alias file maps the glyphs to names rather than CID values");

            for (ifd = 0; ifd < local_top->FDArray.cnt; ifd++) {
                if (gaf->FontName[0] != 0) /* if the GA file supplies an alternate FontName, use it .*/
                    local_top->FDArray.array[ifd].FontName.ptr = gaf->FontName;

                if (gaf->LanguageGroup != -1) /* if the GA file supplies a LanguageGroup, use it .*/
                    local_top->FDArray.array[ifd].Private.LanguageGroup = gaf->LanguageGroup;
            }
        } else {
            if ((gaf->gaType == gafBothCID) || (gaf->gaType == gafDstCID))
                fatal(h, "Parent font is not a CID font, but its matching glyph alias file maps the glyph names to CID values.");
        }
    } else {
        if (parentIsCID != localFontIsCID) {
            if (parentIsCID)
                fatal(h, "First font is CID, current font is name-keyed.");
            else
                fatal(h, "First font is name-keyed, current font is CID.");
        }
    }
    return gaf;
}

static void mergeFDArray(txCtx h, abfTopDict *local_top) {
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;
    dnaSET_CNT(mergeInfo->newiFDArray, local_top->FDArray.cnt);
    /* This will merge the new font's new FDArray into the output font's FDArray,
       and will fill in the mergeInfo->newiFDArray.array */
    /* Note that fonts from different hint dirs will end up in different
       FDArray dicts, because the FDArray dicts only compare as equal if
       the font Dict's FontName match */
    mergeInfo->mergeFDArray(h->cb.glyph.direct_ctx, local_top, mergeInfo->newiFDArray.array);

    /* Usually happens when last glyph in the font is a duplicate; this leaves
       an error state set in the cff write context, that is not cleared until
       the next call to glyphBegin. */
    if (mergeInfo->newiFDArray.array[0] < 0)
        fatal(h, "Error. Bad return from attempt to merge font dict for fonts %s.", h->src.stm.filename);
}

/* ---------------------------- Subset Creation ---------------------------- */

static void callbackMergeGASubset(txCtx h, GAFileInfo *gaf) {
    int seltype;
    int i;
    long numGAEEntries = gaf->gaEntrySet.cnt;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    if ((gaf->gaType == gafSrcCID) || (gaf->gaType == gafBothCID))
        seltype = sel_by_cid;
    else
        seltype = sel_by_name;

    i = 0;
    while (i < numGAEEntries) {
        GAEntry *gae;
        mergeInfo->curGAEIndex = i;
        gae = dnaINDEX(gaf->gaEntrySet, i);
        callbackGlyph(h, seltype, (unsigned short)gae->srcCID, gae->srcName);
        i++;
    }

    /* Unlike the regular callbackSubset function, we do NOT add a not def glyph is one is missing - we add only the specified glyphs in the mapping file. */
}

/* ----------------------------- t1read Library ---------------------------- */

static void updateFontBBox(abfTopDict *top, abfTopDict *mergeTop) {
    float *topBBOX = top->FontBBox;
    float *mergeBBOX = mergeTop->FontBBox;
    if (mergeBBOX[0] < topBBOX[0])
        topBBOX[0] = mergeBBOX[0];
    if (mergeBBOX[1] < topBBOX[1])
        topBBOX[1] = mergeBBOX[1];
    if (mergeBBOX[2] > topBBOX[2])
        topBBOX[2] = mergeBBOX[2];
    if (mergeBBOX[3] > topBBOX[3])
        topBBOX[3] = mergeBBOX[3];
}

/* Merge font with t1read library. */
static void t1rMergeFont(txCtx h, long origin, bool isFirstFont, sourceCtx *srcCtx) {
    bool parentIsCID = 0;
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    if (isFirstFont) {
        mergeInfo->mode = mode_t1;
        if (h->t1r.ctx == NULL) {
            /* Initialize library */
            h->t1r.ctx = t1rNew(&h->cb.mem, &h->cb.stm, T1R_CHECK_ARGS);
            if (h->t1r.ctx == NULL)
                fatal(h, "(t1r) can't init lib");
        }

        dnaNext(&srcCtx->t1r, sizeof(t1rCtx));
        srcCtx->t1r.array[srcCtx->t1r.cnt - 1] = h->t1r.ctx;

        h->t1r.flags |= T1R_UPDATE_OPS; /* Convert seac for subsets */

        if (t1rBegFont(h->t1r.ctx, h->t1r.flags, origin, &h->top, getUDV(h)))
            fatal(h, NULL);

        parentIsCID = h->top->sup.flags & ABF_CID_FONT;
        if (mergeInfo->hintsOnly && parentIsCID)
            fatal(h, "Error. The -hints option cannot be used with CID fonts.");

        /* Apply the CIDFontInfo file, if one was specified. */
        parentIsCID = applyCIDFontInfo(h, parentIsCID); /* can set parentIsCID true, if it was false */

        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, h->top, parentIsCID, parentIsCID);

        prepSubset(h);

        h->dst.begfont(h, h->top);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */
        mergeInfo->srcTopDict = h->top;
        if (parentIsCID)
            mergeFDArray(h, h->top);

        h->cb.glyph.indirect_ctx = h;

        if (h->arg.g.cnt != 0)
            callbackSubset(h);
        else if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else if (t1rIterateGlyphs(h->t1r.ctx, &h->cb.glyph))
            fatal(h, NULL);

        /* h->dst.endfont is called after the all fonts have been merged, by
        doMergeFontSet
        */

        if (t1rEndFont(h->t1r.ctx))
            fatal(h, NULL);
        /* Any subset options other than from the GA file are applied to only the first font. */
        h->flags &= ~SUBSET_OPT;
        h->flags &= ~SUBSET__EXCLUDE_OPT;
        h->arg.g.cnt = 0; /* we don't want to subset twice if a glyph list was specified */
        gaf = NULL;
        /* end if is first font */
    } else {
        unsigned int fileIndex = mergeInfo->fileIndex;
        t1rCtx local_t1r_ctx;
        abfTopDict *local_top = NULL;
        bool localFontIsCID = 0;
        parentIsCID = h->top->sup.flags & ABF_CID_FONT;

        local_t1r_ctx = t1rNew(&h->cb.mem, &h->cb.stm, T1R_CHECK_ARGS);

        if (local_t1r_ctx == NULL)
            fatal(h, "(t1r) can't init lib");

        h->t1r.flags |= T1R_SEEN_GLYPH; /* make sure we don't copy over gsubrs more than once.*/
        dnaNext(&srcCtx->t1r, sizeof(t1rCtx));
        srcCtx->t1r.array[srcCtx->t1r.cnt - 1] = local_t1r_ctx;

        h->t1r.ctx = local_t1r_ctx;

        if (h->flags & SUBSET_OPT && h->mode != mode_dump)
            h->t1r.flags |= T1R_UPDATE_OPS; /* Convert seac for subsets */

        if (t1rBegFont(local_t1r_ctx, h->t1r.flags, origin, &local_top, getUDV(h)))
            fatal(h, NULL);

        localFontIsCID = local_top->sup.flags & ABF_CID_FONT;
        if (mergeInfo->hintsOnly) {
            int k;
            if (fileIndex > 1)
                fatal(h, "Error. When merging hint dict only, only two font arguments are allowed, the source font for the hints and the source font for the glyphs..");
            /* The modified font inherits the entire font dict of the source hint dict. We just copy back the original font names. */
            h->top->FDArray.array[0].FontName.ptr = local_top->FDArray.array[0].FontName.ptr;
            h->top->FullName.ptr = local_top->FullName.ptr;
            h->top->FamilyName.ptr = local_top->FullName.ptr;
            for (k = 0; k < 4; k++)
                h->top->FontBBox[k] = local_top->FontBBox[k];
        }

        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, local_top, parentIsCID, localFontIsCID);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */

        mergeInfo->srcTopDict = local_top;
        if (parentIsCID)
            mergeFDArray(h, local_top);

        updateFontBBox(h->top, local_top);

        h->cb.glyph.indirect_ctx = h;

        if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else if (t1rIterateGlyphs(local_t1r_ctx, &h->cb.glyph))
            fatal(h, NULL);

        /* If this is the last font, and we haven't yet seen the .notdef, add it. */

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */

        if (t1rEndFont(local_t1r_ctx))
            fatal(h, NULL);

        gaf = NULL;
    } /* end of second or later font */

    /* the accumulated local_t1r_ctx's are freed in doMergeFontSet */
}

/* ----------------------------- svread Library ---------------------------- */

/* Merge font with svread library. */
static void svrMergeFont(txCtx h, long origin, bool isFirstFont, sourceCtx *srcCtx) {
    GAFileInfo *gaf = NULL;
    if (isFirstFont)
        fatal(h, "An SVG font cannot be the first font in a merge list - it doesn't have a complete top dict.");
    else {
        svrCtx local_svr_ctx;
        abfTopDict *local_top = NULL;
        bool parentIsCID = h->top->sup.flags & ABF_CID_FONT;
        bool localFontIsCID = 0;
        MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

        local_svr_ctx = svrNew(&h->cb.mem, &h->cb.stm, SVR_CHECK_ARGS);

        if (local_svr_ctx == NULL)
            fatal(h, "(svr) can't init lib");

        dnaNext(&srcCtx->svr, sizeof(svrCtx));
        srcCtx->svr.array[srcCtx->svr.cnt - 1] = local_svr_ctx;

        h->svr.ctx = local_svr_ctx;

        if (svrBegFont(local_svr_ctx, h->svr.flags, &local_top))
            fatal(h, NULL);

        localFontIsCID = local_top->sup.flags & ABF_CID_FONT;
        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, local_top, parentIsCID, localFontIsCID);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */

        mergeInfo->srcTopDict = local_top;

        if (parentIsCID)
            mergeFDArray(h, local_top);

        updateFontBBox(h->top, local_top);

        h->cb.glyph.indirect_ctx = h;

        if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else {
            if (parentIsCID)
                fatal(h, "The first font is CID. You must provide a glyph alias file that converts this svg file to CID.");

            if (svrIterateGlyphs(local_svr_ctx, &h->cb.glyph))
                fatal(h, NULL);
        }

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */

        if (svrEndFont(local_svr_ctx))
            fatal(h, NULL);
    } /* end of second or later font */

    /* the accumulated local_svr_ctx's are freed in doMergeFontSet */
}

/* ----------------------------- ufo read Library ---------------------------- */

/* Merge font with uforead library. */
static void ufoMergeFont(txCtx h, long origin, bool isFirstFont, sourceCtx *srcCtx) {
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    if (isFirstFont) {
        bool parentIsCID = 0;

        mergeInfo->mode = mode_ufow;
        if (h->ufr.ctx == NULL) {
            /* Initialize library */
            h->ufr.ctx = ufoNew(&h->cb.mem, &h->cb.stm, UFO_CHECK_ARGS);

            if (h->ufr.ctx == NULL)
                fatal(h, "(ufr) can't init lib");
        }

        dnaNext(&srcCtx->ufr, sizeof(ufoCtx));
        srcCtx->ufr.array[srcCtx->ufr.cnt - 1] = h->ufr.ctx;

        if (ufoBegFont(h->ufr.ctx, h->ufr.flags, &h->top, h->ufr.altLayerDir))
            fatal(h, NULL);

        parentIsCID = h->top->sup.flags & ABF_CID_FONT;

        /* Apply the CIDFontInfo file, if one was specified. */
        parentIsCID = applyCIDFontInfo(h, parentIsCID); /* can set parentIsCID true, if it was false */

        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, h->top, parentIsCID, parentIsCID);

        prepSubset(h);

        h->dst.begfont(h, h->top);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */
        mergeInfo->srcTopDict = h->top;

        if (parentIsCID)
            mergeFDArray(h, h->top);

        h->cb.glyph.indirect_ctx = h;

        if (h->arg.g.cnt != 0)
            callbackSubset(h);
        else if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else if (ufoIterateGlyphs(h->ufr.ctx, &h->cb.glyph))
            fatal(h, NULL);

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */

        if (ufoEndFont(h->ufr.ctx))
            fatal(h, NULL);
        /* Any subset options other than from the GA file are applied to only the first font. */
        h->flags &= ~SUBSET_OPT;
        h->flags &= ~SUBSET__EXCLUDE_OPT;
        h->arg.g.cnt = 0; /* we don't want to subset twice if a glyph list was specified */
        gaf = NULL;
    } else {
        ufoCtx local_ufr_ctx;
        abfTopDict *local_top = NULL;
        bool parentIsCID = h->top->sup.flags & ABF_CID_FONT;
        bool localFontIsCID;

        local_ufr_ctx = ufoNew(&h->cb.mem, &h->cb.stm, UFO_CHECK_ARGS);

        if (local_ufr_ctx == NULL)
            fatal(h, "(ufr) can't init lib");

        dnaNext(&srcCtx->ufr, sizeof(ufoCtx));
        srcCtx->ufr.array[srcCtx->ufr.cnt - 1] = local_ufr_ctx;

        h->ufr.ctx = local_ufr_ctx;
        if (ufoBegFont(local_ufr_ctx, h->ufr.flags, &local_top, h->ufr.altLayerDir))
            fatal(h, NULL);

        localFontIsCID = local_top->sup.flags & ABF_CID_FONT;
        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, local_top, parentIsCID, localFontIsCID);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */

        mergeInfo->srcTopDict = local_top;

        if (parentIsCID)
            mergeFDArray(h, local_top);

        updateFontBBox(h->top, local_top);

        h->cb.glyph.indirect_ctx = h;

        if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else {
            if (parentIsCID)
                fatal(h, "The first font is CID. You must provide a glyph alias file that converts this ufo font to CID.");

            if (ufoIterateGlyphs(local_ufr_ctx, &h->cb.glyph))
                fatal(h, NULL);
        }

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */

        if (ufoEndFont(local_ufr_ctx))
            fatal(h, NULL);
    }
    /* the accumulated local_svr_ctx's are freed in doMergeFontSet */
}

/* ---------------------------- cffread Library ---------------------------- */

/* Read font with cffread library. */
static void cfrReadFont(txCtx h, long origin, int ttcIndex) {
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    if (h->cfr.ctx == NULL) {
        h->cfr.ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
        if (h->cfr.ctx == NULL)
            fatal(h, "(cfr) can't init lib");
    }

    if (h->flags & SUBSET_OPT && h->mode != mode_dump)
        h->cfr.flags |= CFR_UPDATE_OPS; /* Convert seac for subsets */

    if (cfrBegFont(h->cfr.ctx, h->cfr.flags, origin, ttcIndex, &h->top, NULL))
        fatal(h, NULL);

    prepSubset(h);

    h->dst.begfont(h, h->top);

    if (h->mode != mode_cef && h->mode != mode_dcf) {
        if (h->cfr.flags & CFR_NO_ENCODING)
            /* OTF font */
            prepOTF(h);

        if (h->arg.g.cnt != 0)
            callbackSubset(h);
        else if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph))
            fatal(h, NULL);

        if (h->cfr.flags & CFR_NO_ENCODING) {
            /* OTF font; restore callback */
            h->cb.glyph.beg = h->cb.saveGlyphBeg;
            h->cfr.flags &= ~CFR_NO_ENCODING;
        }
    }
    /* CFF does not contain fsType, so I need to set it again is the source is CFF */
    if (mergeInfo->cidinfo.fsType != ABF_UNSET_INT) {
        h->top->FSType = mergeInfo->cidinfo.fsType;
    }
    h->dst.endfont(h);

    if (cfrEndFont(h->cfr.ctx))
        fatal(h, NULL);
}

/* Merge font with cffread library. */
static void cfrMergeFont(txCtx h, long origin, bool isFirstFont, sourceCtx *srcCtx) {
    bool parentIsCID = 0;
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    if (isFirstFont) {
        mergeInfo->mode = mode_cff;

        if (h->cfr.ctx == NULL) /* In this case, we must be processing the first font. */
        {
            h->cfr.ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
            if (h->cfr.ctx == NULL)
                fatal(h, "(cfr) can't init lib");
        }

        dnaNext(&srcCtx->cfr, sizeof(cfrCtx));
        srcCtx->cfr.array[srcCtx->cfr.cnt - 1] = h->cfr.ctx;

        h->cfr.flags |= CFR_UPDATE_OPS; /* Convert seac for subsets */

        if (cfrBegFont(h->cfr.ctx, h->cfr.flags, origin, 0, &h->top, NULL))
            fatal(h, NULL);

        parentIsCID = h->top->sup.flags & ABF_CID_FONT;
        /* Apply the CIDFontInfo file, if one was specified. */
        parentIsCID = applyCIDFontInfo(h, parentIsCID); /* can set parentIsCID true, if it was false */

        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, h->top, parentIsCID, parentIsCID);

        /* For first font, apply the glyph selection options. */
        prepSubset(h);

        h->dst.begfont(h, h->top);

        /* provide data for mergeGlyphBegin function,
           which is patched over the &h->cb.glyph.beg() function ptr */
        mergeInfo->srcTopDict = h->top;
        if (parentIsCID)
            mergeFDArray(h, h->top);
        h->cb.glyph.indirect_ctx = h;

        if (h->arg.g.cnt != 0)
            callbackSubset(h);
        else if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph))
            fatal(h, NULL);

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */

        if (cfrEndFont(h->cfr.ctx)) /* source stream gets closed here. */
            fatal(h, NULL);
        /* Any subset options  are applied to only the first font. */
        h->flags &= ~SUBSET_OPT;
        h->flags &= ~SUBSET__EXCLUDE_OPT;
        h->arg.g.cnt = 0; /* we don't want to subset twice if a glyph list was specified */
        /* end if is first font */
    } else {
        cfrCtx local_cfr_ctx;
        abfTopDict *local_top = NULL;
        bool localFontIsCID = 0;

        parentIsCID = h->top->sup.flags & ABF_CID_FONT;
        local_cfr_ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
        if (local_cfr_ctx == NULL)
            fatal(h, "(cfr) can't init lib");
        h->cfr.flags |= CFR_SEEN_GLYPH; /* make sure we don't copy over gsubrs more than once.*/

        dnaNext(&srcCtx->cfr, sizeof(cfrCtx));
        srcCtx->cfr.array[srcCtx->cfr.cnt - 1] = local_cfr_ctx;

        h->cfr.ctx = local_cfr_ctx;

        if (cfrBegFont(local_cfr_ctx, h->cfr.flags, origin, 0, &local_top, NULL))
            fatal(h, NULL);

        localFontIsCID = local_top->sup.flags & ABF_CID_FONT;

        /* provide data for mergeGlyphBegin function,
        which is patched over the &h->cb.glyph.beg() function ptr */

        /* If there is a glyph alias file for the first font, make sure it is the right type, or that both fonts are name keyed or CID. */
        gaf = checkIFParentCIDCompatible(h, local_top, parentIsCID, localFontIsCID);

        mergeInfo->srcTopDict = local_top;
        if (parentIsCID)
            mergeFDArray(h, local_top);
        updateFontBBox(h->top, local_top);

        h->cb.glyph.indirect_ctx = h;

        if (gaf != NULL) {
            callbackMergeGASubset(h, gaf);
        } else if (cfrIterateGlyphs(local_cfr_ctx, &h->cb.glyph))
            fatal(h, NULL);

        if (cfrEndFont(local_cfr_ctx)) /* source stream gets closed here. */
            fatal(h, NULL);

        /* h->dst.endfont is called after the all fonts have been merged, by doMergeFontSet */
    } /* end of second or later font */

    /* the accumulated local_cfr_ctx's are freed in doMergeFontSet */
}

/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information. */
static void usage(txCtx h) {
    static char *text[] =
        {
#include "usage.h"
        };
    printText(ARRAY_LEN(text), text);
    exit(0);
}

/* Show help information. */
static void help(txCtx h) {
    if (h->flags & SEEN_MODE) {
        /* Mode-specific help */
        switch (h->mode) {
            case mode_dump:
                dump_Help(h);
                break;
            case mode_ps:
                ps_Help(h);
                break;
            case mode_pdf:
                pdf_Help(h);
                break;
            case mode_afm:
                afm_Help(h);
                break;
            case mode_cff:
                cff_Help(h);
                break;
            case mode_cef:
                cef_Help(h);
                break;
            case mode_path:
                path_Help(h);
                break;
            case mode_mtx:
                mtx_Help(h);
                break;
            case mode_t1:
                t1_Help(h);
                break;
            case mode_svg:
                svg_Help(h);
                break;
            case mode_dcf:
                dcf_Help(h);
                break;
            case mode_ufow:
                ufo_Help(h);
                break;
        }
    } else {
        /* General help */
        static char *text[] =
            {
#include "help.h"
            };
        printText(ARRAY_LEN(text), text);
    }
    exit(0);
}

/* Add arguments from script file. */
static void addArgs(txCtx h, char *filename) {
    int state;
    long i;
    size_t length;
    FILE *fp;
    char *start = NULL; /* Suppress optimizer warning */

    /* Open script file */
    if ((fp = fopen(filename, "rb")) == NULL ||
        fseek(fp, 0, SEEK_END) == -1)
        fileError(h, filename);

    /* Size file and allocate buffer */
    length = ftell(fp) + 1;
    h->script.buf = memNew(h, length);

    /* Read whole file into buffer and close file */
    if (fseek(fp, 0, SEEK_SET) == -1 ||
        fread(h->script.buf, 1, length, fp) != length - 1 ||
        fclose(fp) == EOF)
        fileError(h, filename);

    h->script.buf[length - 1] = '\n'; /* Ensure termination */

    /* Parse buffer into args */
    state = 0;
    for (i = 0; i < (long)length; i++) {
        int c = h->script.buf[i] & 0xff;
        switch (state) {
            case 0:
                switch (c) {
                    case ' ':
                    case '\n':
                    case '\t':
                    case '\r':
                    case '\f':
                        break;
                    case '#':
                        state = 1;
                        break;
                    case '"':
                        start = &h->script.buf[i + 1];
                        state = 2;
                        break;
                    default:
                        start = &h->script.buf[i];
                        state = 3;
                        break;
                }
                break;
            case 1: /* Comment */
                if (c == '\n' || c == '\r')
                    state = 0;
                break;
            case 2: /* Quoted string */
                if (c == '"') {
                    h->script.buf[i] = '\0'; /* Terminate string */
                    *dnaNEXT(h->script.args) = start;
                    state = 0;
                }
                break;
            case 3: /* Space-delimited string */
                if (isspace(c)) {
                    h->script.buf[i] = '\0'; /* Terminate string */
                    *dnaNEXT(h->script.args) = start;
                    state = 0;
                }
                break;
        }
    }
}

/* Get version callback function. */
static void getversion(ctlVersionCallbacks *cb, int version, char *libname) {
    char version_buf[MAX_VERSION_SIZE + 1];
    printf("    %-10s%s\n", libname, CTL_SPLIT_VERSION(version_buf, version));
}

/* Print library version numbers. */
static void printVersions(txCtx h) {
    ctlVersionCallbacks cb;
    enum { MAX_VERSION_SIZE = 100 };
    char version_buf[MAX_VERSION_SIZE + 1];

    printf(
        "Versions:\n"
        "    mergefonts        %s\n",
        CTL_SPLIT_VERSION(version_buf, MERGEFONTS_VERSION));

    cb.ctx = NULL;
    cb.called = 0;
    cb.getversion = getversion;

    abfGetVersion(&cb);
    cefGetVersion(&cb);
    cfrGetVersion(&cb);
    cfwGetVersion(&cb);
    ctuGetVersion(&cb);
    dnaGetVersion(&cb);
    pdwGetVersion(&cb);
    sfrGetVersion(&cb);
    t1rGetVersion(&cb);
    svrGetVersion(&cb);
    ttrGetVersion(&cb);
    t1wGetVersion(&cb);
    svwGetVersion(&cb);
    ufoGetVersion(&cb);
    ufwGetVersion(&cb);

    exit(0);
}

/* Match options. */
static int CTL_CDECL matchOpts(const void *key, const void *value) {
    return strcmp((char *)key, *(char **)value);
}

/* Return option index from key or opt_None if not found. */
static int getOptionIndex(char *key) {
    const char **optstr =
        (const char **)bsearch(key, options, ARRAY_LEN(options),
                               sizeof(options[0]), matchOpts);
    return (int)((optstr == NULL) ? opt_None : optstr - options + 1);
}

/* Process file. */
static void doFile(txCtx h, char *srcname) {
    long i;
    char *p;
    struct stat fileStat;
    int statErrNo;

    /* Set src name */
    if (h->file.sr != NULL) {
        sprintf(h->file.src, "%s/", h->file.sr);
        p = &h->file.src[strlen(h->file.src)];
    } else
        p = h->file.src;
    if (h->file.sd != NULL)
        sprintf(p, "%s/%s", h->file.sd, srcname);
    else
        strcpy(p, srcname);

    /* Open file */

    /* Need to first check if it is a directory-based font format, like UFO. */
    statErrNo = stat(h->src.stm.filename, &fileStat);
    if (strcmp(h->src.stm.filename, "-") == 0)
        h->src.stm.fp = stdin;
    else if ((statErrNo == 0) && ((fileStat.st_mode & S_IFDIR) != 0)) {
        /* maybe it is a dir-based font, like UFO. Will verify this in buildFontList(h) */
        h->src.stm.fp = NULL;
    } else {
        h->src.stm.fp = fopen(h->src.stm.filename, "rb");
        if (h->src.stm.fp == NULL)
            fileError(h, h->src.stm.filename);
    }

    h->src.print_file = 1;

    if (h->flags & SHOW_NAMES) {
        fflush(stdout);
        fprintf(stderr, "--- Filename: %s\n", h->src.stm.filename);
    }

    /* The font file we are reading may contain multiple fonts, e.g. a TTC or
       multiple sfnt resources, so keep open until the last font processed */
    h->src.stm.flags |= STM_DONT_CLOSE;

    /* Read file and build font list */
    buildFontList(h);

    /* Process font list */
    for (i = 0; i < h->fonts.cnt; i++) {
        FontRec *rec = &h->fonts.array[i];

        if (i + 1 == h->fonts.cnt)
            h->src.stm.flags &= ~STM_DONT_CLOSE;

        h->src.type = rec->type;

        if (h->seg.refill != NULL) {
            /* Prep source filter */
            h->seg.left = 0;
            h->src.next = h->src.end;
        }

        /* Process font according to type */
        switch (h->src.type) {
            case src_Type1:
                t1rReadFont(h, rec->offset);
                break;
            case src_OTF:
                h->cfr.flags |= CFR_NO_ENCODING;
                /* Fall through */
            case src_CFF:
                cfrReadFont(h, rec->offset, rec->iTTC);
                break;
            case src_TrueType:
                ttrReadFont(h, rec->offset, rec->iTTC);
                break;
            case src_SVG:
                svrReadFont(h, rec->offset);
                break;
            case src_UFO:
                ufoReadFont(h, rec->offset);
                break;
        }
    }

    h->arg.i = NULL;
    h->flags |= DONE_FILE;
}

/* Process multi-file set. Return index of last used arg. */
static int doMultiFileSet(txCtx h, int argc, char *argv[], int i) {
    int filecnt = 0;

    h->dst.begset(h);
    for (; i < argc; i++)
        switch (getOptionIndex(argv[i])) {
            case opt_None:
                doFile(h, argv[i]);
                filecnt++;
                break;
            case opt_sd:
                if (++i == argc)
                    fatal(h, "no argument for option (-sd)");
                h->file.sd = argv[i];
                break;
            case opt_sr:
                if (++i == argc)
                    fatal(h, "no argument for option (-sr)");
                h->file.sr = argv[i];
                break;
            case opt_dd:
                if (++i == argc)
                    fatal(h, "no argument for option (-dd)");
                h->file.dd = argv[i];
                break;
            default:
                goto finish;
        }

finish:
    if (filecnt == 0)
        fatal(h, "empty list (-f)");

    h->dst.endset(h);
    return i - 1;
}

/* Process single file set. */
static void doSingleFileSet(txCtx h, char *srcname) {
    h->dst.begset(h);
    doFile(h, srcname);
    h->dst.endset(h);
}

/* Process auto-file set. Return index of last used arg. */
static int doAutoFileSet(txCtx h, int argc, char *argv[], int i) {
    int filecnt = 0;

    for (; i < argc; i++)
        switch (getOptionIndex(argv[i])) {
            case opt_None:
                doSingleFileSet(h, argv[i]);
                filecnt++;
                break;
            case opt_sd:
                if (++i == argc)
                    fatal(h, "no argument for option (-sd)");
                h->file.sd = argv[i];
                break;
            case opt_sr:
                if (++i == argc)
                    fatal(h, "no argument for option (-sr)");
                h->file.sr = argv[i];
                break;
            case opt_dd:
                if (++i == argc)
                    fatal(h, "no argument for option (-dd)");
                h->file.dd = argv[i];
                break;
            default:
                goto finish;
        }

finish:
    if (filecnt == 0)
        fatal(h, "empty list (-a/-A)");

    return i - 1;
}

static void stringStrip(char *str) {
    int start = 0;
    int end = (int)strlen(str) - 1;
    if (end == 0)
        return;

    while (start < MAX_DICT_ENTRY_LEN) {
        if ((str[start] == ' ') || (str[start] == '\t') || (str[start] == '(') || (str[start] == '\r') || (str[start] == '\n'))
            start++;
        else
            break;
    }

    while (end >= 0) {
        if ((str[end] == ' ') || (str[end] == '\t') || (str[end] == ')') || (str[end] == '\r') || (str[end] == '\n'))
            end--;
        else
            break;
    }

    str[end + 1] = 0;
    if (start > 0)
        memmove(str, &str[start], (end - start) + 2);
    if (strlen(str) == 0) {
        str[0] = ' '; /* so that the string is not empty. The  mergeInfo->cidinfo must not be empty strings in order to override the src fonts values.*/
        str[1] = 0;
    }
}

static void readCIDFontInfo(txCtx h, char *filePath) {
    int lineno;
    FILE *fp;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    fp = fopen(filePath, "rb");
    if (fp == NULL)
        fatal(h, "Failed to open cid fontinfo file %s.", filePath);

    lineno = 1;
    mergeInfo->cidinfo.CIDFontName[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.FullName[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.FamilyName[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.AdobeCopyright[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.Weight[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.Trademark[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.isFixedPitch = ABF_UNSET_INT;
    mergeInfo->cidinfo.fsType = -1;
    mergeInfo->cidinfo.Registry[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.Ordering[0] = ABF_EMPTY_ARRAY;
    mergeInfo->cidinfo.Supplement = ABF_UNSET_INT;
    mergeInfo->cidinfo.fontVersion = 0.0;
    mergeInfo->XUID.cnt = ABF_EMPTY_ARRAY;

    while (1) {
        char buf[MAX_DICT_ENTRY_LEN + MAX_DICT_ENTRY_LEN];
        char key[MAX_DICT_ENTRY_LEN];
        char value[MAX_DICT_ENTRY_LEN];
        int i, len;

        char *ret;
        ret = fgets(buf, MAX_DICT_ENTRY_LEN + MAX_DICT_ENTRY_LEN, fp);
        if (ret == NULL)
            break;
        i = 0;
        while (i < MAX_DICT_ENTRY_LEN) {
            key[i] = 0;
            value[i++] = 0;
        }
        lineno++;
        len = sscanf(buf, "%127s %127[^\n]", key, value);
        if (len != 2) {
            if (len == -1)
                continue;
            if ((len == 1) && (0 == strcmp(key, "CopyrightNotice"))) {
                while (1) {
                    ret = fgets(buf, MAX_DICT_ENTRY_LEN + MAX_DICT_ENTRY_LEN, fp);
                    if (strlen(buf) == 0)
                        continue;
                    if (ret == NULL)
                        fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. The CopyrightNotice key is not followed by the EndNotice key.", filePath, lineno - 1);
                    if (0 == strncmp(buf, "EndNotice", 9))
                        break;
                    strcat(mergeInfo->cidinfo.AdobeCopyright, buf);
                }
                continue;
            }
            fatal(h, "Parse error in the CID fontinfo file \"%s\", line number %d.", filePath, lineno - 1);
        } else {
            value[MAX_DICT_ENTRY_LEN - 1] = 0; /* very unlikely that a value would be over 128 chars, but just in case */
            /* strip leading and following spaces  and parentheses from values */
            stringStrip(key);
            stringStrip(value);
            if (0 == strcmp(key, "FontName"))
                strcpy(mergeInfo->cidinfo.CIDFontName, value);
            else if (0 == strcmp(key, "FullName"))
                strcpy(mergeInfo->cidinfo.FullName, value);
            else if (0 == strcmp(key, "FamilyName"))
                strcpy(mergeInfo->cidinfo.FamilyName, value);
            else if (0 == strcmp(key, "Weight"))
                strcpy(mergeInfo->cidinfo.Weight, value);
            else if (0 == strcmp(key, "AdobeCopyright"))
                strcat(mergeInfo->cidinfo.AdobeCopyright, value);
            else if (0 == strcmp(key, "Trademark"))
                strcpy(mergeInfo->cidinfo.Trademark, value);
            else if (0 == strcmp(key, "FSType")) {
                mergeInfo->cidinfo.fsType = atoi(value);
                if ((0 != strcmp("0", value)) && (mergeInfo->cidinfo.fsType == 0))
                    fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. The fsType value could not be parsed as an integer number.", filePath, lineno - 1);
            } else if (0 == strcmp(key, "isFixedPitch")) {
                if (0 == strcmp(value, "true"))
                    mergeInfo->cidinfo.isFixedPitch = 1;
                else if (0 == strcmp(value, "false"))
                    mergeInfo->cidinfo.isFixedPitch = 0;
                else
                    fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. The isFixedPitch value could not be parsed as either \"true\" or \"false\".", filePath, lineno - 1);
            } else if (0 == strcmp(key, "Registry"))
                strcpy(mergeInfo->cidinfo.Registry, value);
            else if (0 == strcmp(key, "Ordering"))
                strcpy(mergeInfo->cidinfo.Ordering, value);
            else if (0 == strcmp(key, "Supplement")) {
                mergeInfo->cidinfo.Supplement = atoi(value);
                if ((0 != strcmp("0", value)) && (mergeInfo->cidinfo.Supplement == 0))
                    fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. The Supplement value could not be parsed as a integer number.", filePath, lineno - 1);
            } else if (0 == strcmp(key, "version")) {
                mergeInfo->cidinfo.fontVersion = (float)atof(value);
                if (mergeInfo->cidinfo.fontVersion == 0.0)
                    fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. The fontVersion value could not be parsed as a fractional number.", filePath, lineno - 1);
            } else if (0 == strcmp(key, "XUID")) {
                char *p = value;
                long cnt = 0;
                long lenString = strlen(value);
                long i = 0;
                char *startp = NULL;
                while (i < lenString) {
                    if ((*p == ' ') || (*p == '\t') || (*p == '[') || (*p == ']')) {
                        *p = '\0';
                        if (startp != NULL) {
                            if (cnt > 15)
                                fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. There are more than 16 values in the array.", filePath, lineno - 1);
                            mergeInfo->XUID.array[cnt] = atoi(startp);
                            if ((0 != strcmp("0", startp)) && (mergeInfo->XUID.array[cnt] == 0))
                                fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. XUID value no. %ld (%s) could not be parsed as an integer number.", filePath, lineno - 1, cnt + 1, p);
                            startp = NULL;
                            cnt++;
                        }
                    } else if (startp == NULL)
                        startp = p;

                    i++;
                    p++;
                }

                if (startp != NULL) {
                    if (cnt > 15)
                        fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. There are more than 16 values in the array.", filePath, lineno - 1);
                    mergeInfo->XUID.array[cnt] = atoi(startp);
                    if ((0 != strcmp("0", startp)) && (mergeInfo->XUID.array[cnt] == 0))
                        fatal(h, "Parse error in the CID fontinfo file \"%s\" at line no %d. XUID value no. %ld (%s) could not be parsed as an integer number.", filePath, lineno - 1, cnt + 1, p);
                    cnt++;
                }
                mergeInfo->XUID.cnt = cnt;
            }
        }
    }

    fclose(fp);
    if (lineno < 9)
        fatal(h, "Parse error in the CID fontinfo file \"%s\", there are not enough key/value pairs. Make sure line-endings are correct.", filePath);

    {
        int missingKey = 0;
        if (mergeInfo->cidinfo.CIDFontName[0] == 0) {
            fprintf(stderr, "Parse error in the CID fontinfo file \"%s\": missing required key \"FontName\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Registry[0] == 0) {
            fprintf(stderr, "Parse error in the CID fontinfo file \"%s\": missing required key \"Registry\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Ordering[0] == 0) {
            fprintf(stderr, "Parse error in the CID fontinfo file \"%s\": missing required key \"Ordering\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Supplement == -1) {
            fprintf(stderr, "Parse error in the CID fontinfo file \"%s\": missing required key \"Supplement\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.fontVersion == 0.0) {
            fprintf(stderr, "Parse error in the CID fontinfo file \"%s\": missing required key \"version\".", filePath);
            missingKey = 1;
        }
        if (missingKey)
            fatal(h, "");
    }

    return;
}

/* Merge the source font dict into the destination font, and then
call glyphBegin */
static int mergeGlyphBegin(abfGlyphCallbacks *cb, abfGlyphInfo *srcInfo) {
    int result;
    txCtx h = cb->indirect_ctx;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;
    GAFileInfo *gaf = &mergeInfo->glyphAliasSet.array[mergeInfo->fileIndex];
    GAEntry *gae;
    abfGlyphInfo *info = srcInfo;

    if (mergeInfo->hintsOnly && (mergeInfo->fileIndex == 0))
        return ABF_SKIP_RET; /* If we are copying only the font dict hint info, then we take no glyphs from the source.*/

    /* If we have  GA file for the current font, apply it.
       If there is no match in the GA file, then skip the glyph */

    /* Since the t1r library will always add a  notdef to the glyph list, even
       if the font doesn't have it we will often see a notdef when the
       GAFileInfo does not contain a reference to it. We want to accept the
       first one we see, whether it is in the GAFileInfo or not, and let
       another one in only if it is specified by the GA file. */
    if (gaf->gaEntrySet.cnt > 0) {
        int gaeIndex = mergeInfo->curGAEIndex;

        /* We need a new unique abfGlyphInfo, to hold the new name. Doesn't
           need to be in any order. Note that seenGlyphs.array not grow and
           change locations while we are using it, as the output libraries,
           like cffwrite, save a pointer to info elements in
           seenGlyphs.array.*/
        info = dnaNEXT(gaf->seenGlyphs);

        /* We need a copy, because we can't afford to change the original
           srcInfo, as at a higher level, there is a binary search going on to
           match find the original name in the list of font glyph names. Note
           that this copies only some of the info. The rest are pointers.
           Fortunately, we don't need a deep copy. */
        *info = *srcInfo;

        gae = dnaINDEX(gaf->gaEntrySet, gaeIndex);
        if ((gaf->gaType == gafBothName) || (gaf->gaType == gafDstCID)) {
            /* if we are changing the name, we need to set the encoding to unknown */
            if (!((gaf->gaType == gafBothName) && (0 == strcmp(info->gname.ptr, gae->dstName))))
                info->encoding.code = ABF_GLYPH_UNENC;
            if (gaf->gaType == gafBothName)
                info->gname.ptr = gae->dstName;
            else {
                info->flags |= ABF_GLYPH_CID; /* set CID flag in  glyph info */
                info->cid = (unsigned short)gae->dstCID;
                info->gname.ptr = NULL;
            }
        } else /* src is keyed by  CID */
        {
            if (gaf->gaType == gafSrcCID) /* the glyph is being changed from CID to name-keyed */
            {
                /* if we are changing the name, we need to set the encoding to unknown */
                info->encoding.code = ABF_GLYPH_UNENC;
                info->flags &= ~ABF_GLYPH_CID; /* clear CID flag in glyph info */
                info->gname.ptr = gae->dstName;
            } else
                info->cid = (unsigned short)gae->dstCID;
        }
    } else if ((mergeInfo->cidinfo.CIDFontName[0] != ABF_EMPTY_ARRAY) && (!(info->flags & ABF_GLYPH_CID))) {
        /* else - file does not have GA file; everything gets copied through.
           However, if a cidfontinfo file has been specified, and the source is
           name-keyed, and the names are in the form cidXXXX, then the new
           names are the CID values. notdef then becomes cid0. */
        if (strncmp(info->gname.ptr, "cid", 3) == 0) {
            char *p = info->gname.ptr + 3;
            int cnt = sscanf(p, "%hu", &(info->cid));

            if (cnt == 1) {
                info->flags |= ABF_GLYPH_CID; /* set CID flag in  glyph info */
            } else {
                fatal(h, "Bad glyph name '%s'. When converting a name-keyed font to CID, all glyphs must be either .notdef, or have a name in the form 'cidXXXXX'.", info->gname.ptr);
            }
        } else if (strcmp(info->gname.ptr, ".notdef") == 0) {
            info->flags |= ABF_GLYPH_CID; /* set CID flag in  glyph info */
            info->cid = 0;
        } else {
            /* Skip the glyph, and emit a warning. */
            fprintf(stderr, "%s: Warning: Skipping glyph '%s'. When converting to CID, only glyphs with a name in the form 'cidXXXXX' will be copied.\n", h->progname, info->gname.ptr);
            return ABF_SKIP_RET; /* If we are copying only the font dict hint info, then we take no glyphs from the source.*/
        }

        h->flags |= SEEN_MODE;
    }

    if (!mergeInfo->seenNotdef) {
        if (((info->flags & ABF_GLYPH_CID) && (info->cid == 0)) || (((~info->flags) & ABF_GLYPH_CID) && (strcmp(info->gname.ptr, ".notdef") == 0))) {
            mergeInfo->seenNotdef = 1;
            h->flags |= SUBSET_HAS_NOTDEF;
        }
    }

    /* if the output font is a CID-keyed font, we need to collect the iFD value. */
    if (info->flags & ABF_GLYPH_CID) {
        info->iFD = mergeInfo->newiFDArray.array[info->iFD];
    }

    result = mergeInfo->mergeGlyphBeg(cb, info);
    return result;
}

/* Merge file. */
static void mergeFile(txCtx h, char *srcname, bool isFirstFont, sourceCtx *srcCtx) {
    long i;
    char *p;
    struct stat fileStat;
    int statErrNo;

    /* Set src name */
    p = h->file.src;
    strcpy(p, srcname);

    /* Need to first check if it is a directory-based font format, like UFO. */
    statErrNo = stat(h->src.stm.filename, &fileStat);
    if (strcmp(h->src.stm.filename, "-") == 0)
        h->src.stm.fp = stdin;
    else if ((statErrNo == 0) && ((fileStat.st_mode & S_IFDIR) != 0)) {
        /* maybe it is a dir-based font, like UFO. Will verify this in buildFontList(h) */
        h->src.stm.fp = NULL;
    } else {
        h->src.stm.fp = fopen(h->src.stm.filename, "rb");
        if (h->src.stm.fp == NULL)
            fileError(h, h->src.stm.filename);
    }

    h->src.print_file = 1;

    /* The font file we are reading may contain multiple fonts, e.g. a TTC or
       multiple sfnt resources, so keep open until the last font processed */
    h->src.stm.flags |= STM_DONT_CLOSE;

    /* Read file and build font list */
    buildFontList(h);

    /* Process font list */
    for (i = 0; i < h->fonts.cnt; i++) {
        FontRec *rec = &h->fonts.array[i];

        if (i + 1 == h->fonts.cnt)
            h->src.stm.flags &= ~STM_DONT_CLOSE;

        h->src.type = rec->type;

        if (h->seg.refill != NULL) {
            /* Prep source filter */
            h->seg.left = 0;
            h->src.next = h->src.end;
        }

        switch (h->src.type) {
            case src_Type1:
                t1rMergeFont(h, rec->offset, isFirstFont, srcCtx);
                break;
            case src_OTF:
                h->cfr.flags |= CFR_NO_ENCODING;
                /* Fall through */
            case src_CFF:
                cfrMergeFont(h, rec->offset, isFirstFont, srcCtx);
                break;
            case src_SVG:
                svrMergeFont(h, rec->offset, isFirstFont, srcCtx);
                break;
            case src_UFO:
                ufoMergeFont(h, rec->offset, isFirstFont, srcCtx);
                break;
            case src_TrueType:
                fatal(h, "Merging not supported for TT fonts");
                break;
        }
    }

    h->arg.i = NULL;
    h->flags |= DONE_FILE;
}

#if 0  /* see corresponding if'd out calls in readGlyphAliasFile below */
static int CTL_CDECL cmpGAEBySrcName(const void *first, const void *second) {
    return strcmp(((GAEntry *)first)->srcName,
                  ((GAEntry *)second)->srcName);
}

static int CTL_CDECL cmpGAEBySrcCID(const void *first, const void *second) {
    long firstCID = ((GAEntry *)first)->srcCID;
    long secondCID = ((GAEntry *)second)->srcCID;
    if (firstCID == secondCID)
        return 0;
    if (firstCID < secondCID)
        return -1;
    return 1;
}
#endif

static bool readGlyphAliasFile(txCtx h, int fileIndex, char *filePath) {
    FILE *ga_fp;
    bool isGA = 0;
    char lineBuffer[MAX_DICT_ENTRY_LEN];
    char progName[MAX_DICT_ENTRY_LEN];
    int len;
    char *ret;
    unsigned short lineno = 0;
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;
    char *p;

    /* Always make a  glyphAliasSet entry - even if we are skipping a GA file
       and the current file turns out to be a font. we still need an empty
       glyphAliasSet entry for it.*/
    while (fileIndex >= mergeInfo->glyphAliasSet.cnt) {
        gaf = dnaNEXT(mergeInfo->glyphAliasSet);
        dnaINIT(h->ctx.dna, gaf->gaEntrySet, 1, 300);
        dnaINIT(h->ctx.dna, gaf->seenGlyphs, 1, 300);
    }

    if (mergeInfo->glyphAliasSet.cnt > MAX_MERGE_FILES)
        fatal(h, "Error. This program cannot merge more than %d files.", MAX_MERGE_FILES);
    /* This is because the cffWrite context gets a pointer to the
       gaf->FontName. If we allow resizing of the glyphAliasSet array,
       it can move in memory, making the saved pointer invalid */
    gaf->LanguageGroup = -1;
    gaf->FontName[0] = 0;

    ga_fp = fopen(filePath, "rb");
    if (ga_fp == NULL)
        fatal(h, "Failed to open file %s.", filePath);
    ret = fgets(lineBuffer, MAX_DICT_ENTRY_LEN, ga_fp);
    if ((lineBuffer[0] == '%') || (lineBuffer[0] == 0) || (lineBuffer[0] == 1) || (sig_OTF == (*(long *)lineBuffer))) {
        fclose(ga_fp);
        return isGA;
    }

    len = sscanf(lineBuffer, "%127s %127s %ld", progName, gaf->FontName, &gaf->LanguageGroup);
    if (len == 0) {
        fclose(ga_fp);
        return isGA;
    }

    /* convert to lower case to allow for old uses of "mergeFonts" */
    for (p = progName; *p != 0; p++)
        *p = tolower(*p);

    if (strcmp(progName, "mergefonts")) {
        fclose(ga_fp);
        return isGA;
    }

    isGA = 1;
    lineno = 1;
    gaf->gaType = -1;
    while (1) {
        int gaType = -1;
        GAEntry *gae = NULL;

        ret = fgets(lineBuffer, MAX_DICT_ENTRY_LEN, ga_fp);
        if (ret == NULL)
            break;
        if (lineBuffer[1] == 0) /* contains only a new-line */
            continue;
        if (lineBuffer[0] == '#')
            continue;

        gae = dnaNEXT(gaf->gaEntrySet);
        len = sscanf(lineBuffer, "%127s %127s", gae->dstName, gae->srcName);
        if (len != 2) {
            fatal(h, "Parse error in glyph alias file \"%s\": there was not an even number of src/dst names, in line %d.", filePath, lineno - 1);
            dnaSET_CNT(gaf->gaEntrySet, gaf->gaEntrySet.cnt - 1);
            break;
        }
        lineno++;
        gae->srcCID = -1;
        gae->dstCID = -1;
        if ((gae->srcName[0] <= '9') && (gae->srcName[0] >= '0')) /* glyph names may not begin with a number */
            gae->srcCID = atoi(gae->srcName);
        if ((gae->dstName[0] <= '9') && (gae->dstName[0] >= '0')) /* glyph names may not begin with a number */
            gae->dstCID = atoi(gae->dstName);

        if ((gae->srcCID >= 0) && (gae->dstCID >= 0))
            gaType = gafBothCID; /* both are CID */
        else if ((gae->srcCID >= 0) && (gae->dstCID < 0))
            gaType = gafSrcCID; /*src is CID, dst is not */
        else if ((gae->dstCID >= 0) && (gae->srcCID < 0))
            gaType = gafDstCID; /*dst is CID, src is not  */
        else
            gaType = gafBothName; /*dst is is not CID, src is not  */

        if (gaf->gaType >= 0) {
            if (gaf->gaType != gaType)
                fatal(h, "Parse error in glyph alias file \"%s\": line %d is a different mapping type than the first line.", filePath, lineno);
        } else
            gaf->gaType = gaType;
    }
    if (gaf->gaEntrySet.cnt == 0) {
        printf("Warning: the glyph alias file %s contained no glyph names. This may be a problem with file line-endings.\n", filePath);
    } else {
        dnaGROW(gaf->seenGlyphs, gaf->gaEntrySet.cnt);  // We need to do this, as the seenGlyphs.array cannot grow and change locations while it is being used.
    }

#if 0
    /* sort list */
    if ((gaf->gaType == 2) || (gaf->gaType == 3))
        qsort(gaf->gaEntrySet.array, gaf->gaEntrySet.cnt, sizeof(gaf->gaEntrySet.array[0]), cmpGAEBySrcName);
    else
        qsort(gaf->gaEntrySet.array, gaf->gaEntrySet.cnt, sizeof(gaf->gaEntrySet.array[0]), cmpGAEBySrcCID);
#endif

    fclose(ga_fp);
    return isGA;
}

/* Process merging multi-file set. Return index of last used arg. */
/* Currently implemented only for mode_cff */
static int doMergeFileSet(txCtx h, int argc, char *argv[], int i) {
    sourceCtx srcCtx;
    int fileIndex = 0;
    int fileCount = 0;
    int curMaxFD = -1;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    /* allocate a list to hold  the source font ctx's */
    dnaINIT(h->ctx.dna, srcCtx.cfr, 5, 5);
    dnaINIT(h->ctx.dna, srcCtx.t1r, 5, 5);
    dnaINIT(h->ctx.dna, srcCtx.svr, 5, 5);
    dnaINIT(h->ctx.dna, srcCtx.ufr, 5, 5);

    dnaINIT(h->ctx.dna, mergeInfo->newiFDArray, MAX_MERGE_FILES, MAX_MERGE_FILES);
    dnaINIT(h->ctx.dna, mergeInfo->glyphAliasSet, MAX_MERGE_FILES, MAX_MERGE_FILES);

    h->dst.begset(h); /* dest stream gets opened */

    /* patch mergeGlyphBegin function over the &h->cb.glyph.beg() function ptr */
    mergeInfo->mergeGlyphBeg = h->cb.glyph.beg;
    h->cb.glyph.beg = mergeGlyphBegin;
    switch (h->mode) {
        case mode_cff:

            mergeInfo->compareFDArray = (CompareFDArrayCall)cfwCompareFDArrays;
            mergeInfo->mergeFDArray = (MergeFDArrayCall)cfwMergeFDArray;
            break;
        default:
            fatal(h, "Merging not allowed in destination font mode <%s>.", h->modename);
    }

    /* a glyph alias file applies to the following font file. */
    while (i < argc) {
        bool isGA;
        char *filePath = argv[i++];
        int j;

        /* try and see if the  file is a glyph alias file */
        isGA = readGlyphAliasFile(h, fileIndex, filePath);
        if (isGA) {
            if (i >= argc)
                fatal(h, "Missing final font path after glyph alias path argument %s.", filePath);
            filePath = argv[i++];
        }

        mergeInfo->fileIndex = fileIndex;
        mergeFile(h, filePath, (fileIndex == 0), &srcCtx);
        for (j = 0; j < mergeInfo->newiFDArray.cnt; j++) {
            if (curMaxFD < mergeInfo->newiFDArray.array[j]) {
                curMaxFD = mergeInfo->newiFDArray.array[j];
                fprintf(stderr, "Adding font dict %d from %s.\n", curMaxFD, h->src.stm.filename);
            }
        }

        fileIndex++;
    }

    if (fileIndex == 0)
        fatal(h, "empty file list.\n");
    h->dst.endfont(h);
    h->dst.endset(h); /* dest stream gets closed */

    /* restore original h->cb.glyph.beg */
    h->cb.glyph.beg = mergeInfo->mergeGlyphBeg;

    /* free the source font ctx's */
    for (fileIndex = 0; fileIndex < srcCtx.cfr.cnt; fileIndex++)
        cfrFree(srcCtx.cfr.array[fileIndex]);
    for (fileIndex = 0; fileIndex < srcCtx.t1r.cnt; fileIndex++)
        t1rFree(srcCtx.t1r.array[fileIndex]);
    for (fileIndex = 0; fileIndex < srcCtx.ufr.cnt; fileIndex++)
        ufoFree(srcCtx.ufr.array[fileIndex]);
    for (fileIndex = 0; fileIndex < srcCtx.svr.cnt; fileIndex++)
        svrFree(srcCtx.svr.array[fileIndex]);
    dnaFREE(srcCtx.cfr);
    h->cfr.ctx = NULL;
    dnaFREE(srcCtx.t1r);
    h->t1r.ctx = NULL;
    dnaFREE(srcCtx.ufr);
    h->ufr.ctx = NULL;
    dnaFREE(srcCtx.svr);
    h->svr.ctx = NULL;

    for (fileIndex = 0; fileIndex < mergeInfo->glyphAliasSet.cnt; fileIndex++) {
        if (mergeInfo->glyphAliasSet.array[fileCount].gaEntrySet.cnt != 0) {
            dnaFREE(mergeInfo->glyphAliasSet.array[fileCount].gaEntrySet);
            dnaFREE(mergeInfo->glyphAliasSet.array[fileCount].seenGlyphs);
        }
    }

    dnaFREE(mergeInfo->glyphAliasSet);
    dnaFREE(mergeInfo->newiFDArray);

    return i - 1;
}

/* Parse argument list. */
static void parseArgs(txCtx h, int argc, char *argv[]) {
    int i;
    char *arg;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    h->t1r.flags = 0; /* I initialize these here,as I need to set the std Encoding flags before calling setMode. */
    h->cfr.flags = 0;
    h->cfw.flags = 0;
    h->dcf.flags = DCF_AllTables | DCF_BreakFlowed;
    h->dcf.level = 5;
    h->svr.flags = 0;
    h->ufr.flags = 0;
    h->ufow.flags = 0;

    for (i = 0; i < argc; i++) {
        int argsleft = argc - i - 1;
        arg = argv[i];
        switch (getOptionIndex(arg)) {
            case opt_None:
                /* In mergeFonts, act like 'tx' if seen mode, else enter mergeFonts mode. */
                if (h->flags & SEEN_MODE) {
                    /* Not option, assume filename */
                    if (argsleft > 0) {
                        char *dstname = argv[i + 1];
                        if (getOptionIndex(dstname) == opt_None) {
                            if (argsleft > 1 && getOptionIndex(argv[i + 2]) == opt_None)
                                fatal(h, "too many file args [%s]", argv[i + 2]);
                            dstFileSetName(h, dstname);
                            i++; /* Consume arg */
                        }
                    }
                    doSingleFileSet(h, arg);
                } else {
                    char *dstPath;
                    setMode(h, mode_cff);

                    if (h->mode != mode_cff)
                        goto wrongmode;
                    if (argsleft < 1)
                        fatal(h, "too few file args: need at least <dest file> <parent src> ");
                    dstPath = argv[i];
                    dstFileSetName(h, argv[i]);
                    i++; /* Consume arg */
                    h->cfw.flags |= CFW_CHECK_IF_GLYPHS_DIFFER;
                    h->cfw.flags |= CFW_PRESERVE_GLYPH_ORDER;
                    i = doMergeFileSet(h, argc, argv, i);
                    if (mergeInfo->mode != mode_cff) { /* output font 'h->file.dst" is cff; we need to convert to t1. */
                        setMode(h, mergeInfo->mode);
                        strcpy(h->file.src, dstPath);
                        strcpy(h->file.dst, dstPath);
                        strcat(h->file.dst, ".temp");
                        doSingleFileSet(h, dstPath);
                        remove(h->file.src); /* Under Windows, rename fails if dst file exists. */
                        rename(h->file.dst, h->file.src);
                    }
                }
                break;
            case opt_dump: /* mode selection options */
                setMode(h, mode_dump);
                break;
            case opt_ps:
                setMode(h, mode_ps);
                break;
            case opt_afm:
                setMode(h, mode_afm);
                break;
            case opt_path:
                setMode(h, mode_path);
                break;
            case opt_cff:
                setMode(h, mode_cff);
                break;
            case opt_cef:
                setMode(h, mode_cef);
                break;
            case opt_cefsvg:
                if (h->mode != mode_cef)
                    goto wrongmode;
                h->arg.cef.flags |= CEF_WRITE_SVG;
                break;
            case opt_pdf:
                setMode(h, mode_pdf);
                break;
            case opt_mtx:
                setMode(h, mode_mtx);
                break;
            case opt_t1:
                setMode(h, mode_t1);
                break;
            case opt_svg:
                setMode(h, mode_svg);
                break;
            case opt_ufo:
                setMode(h, mode_ufow);
                break;
            case opt_bc:
                goto bc_gone;
            case opt_dcf:
                setMode(h, mode_dcf);
                break;
            case opt_altLayer:
                h->ufr.altLayerDir = argv[++i];
                break;
            case opt_l:
                switch (h->mode) {
                    case mode_t1:
                        h->t1w.flags &= ~T1W_ENCODE_MASK;
                        h->t1w.flags |= T1W_ENCODE_ASCII;
                        break;
                    case mode_ps:
                        h->abf.draw.flags |= ABF_NO_LABELS;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_0: /* dump/ps/path mode options */
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 0;
                        break;
                    case mode_ps:
                        h->abf.draw.level = 0;
                        break;
                    case mode_path:
                        h->arg.path.level = 0;
                        break;
                    case mode_mtx:
                        h->mtx.level = 0;
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_TYPE_MASK;
                        h->t1w.flags |= T1W_TYPE_HOST;
                        break;
                    case mode_dcf:
                        h->dcf.level = 0;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_1:
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 1;
                        break;
                    case mode_ps:
                        h->abf.draw.level = 1;
                        break;
                    case mode_path:
                        h->arg.path.level = 1;
                        break;
                    case mode_pdf:
                        h->pdw.level = 1;
                        break;
                    case mode_mtx:
                        h->mtx.level = 1;
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_TYPE_MASK;
                        h->t1w.flags |= T1W_TYPE_BASE;
                        break;
                    case mode_dcf:
                        h->dcf.level = 1;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_2:
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 2;
                        break;
                    case mode_mtx:
                        h->mtx.level = 2;
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_TYPE_MASK;
                        h->t1w.flags |= T1W_TYPE_ADDN;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_3: /* dump mode options */
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 3;
                        break;
                    case mode_mtx:
                        h->mtx.level = 3;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_4:
                if (h->mode == mode_dump)
                    h->abf.dump.level = 4;
                else
                    goto wrongmode;
                break;
            case opt_5:
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 5;
                        break;
                    case mode_dcf:
                        h->dcf.level = 5;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_6:
                switch (h->mode) {
                    case mode_dump:
                        h->abf.dump.level = 6;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_n:
                switch (h->mode) {
                    case mode_ps:
                    case mode_t1:
                    case mode_cff:
                    case mode_cef:
                    case mode_svg:
                    case mode_ufow:
                    case mode_dump:
                        h->cb.glyph.stem = NULL;
                        h->cb.glyph.flex = NULL;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt__E:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags |= CFW_EMBED_OPT;
                        break;
                    case mode_t1:
                        h->t1w.options |= T1W_WAS_EMBEDDED;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_E:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags &= ~CFW_EMBED_OPT;
                        break;
                    case mode_t1:
                        h->t1w.options &= ~T1W_WAS_EMBEDDED;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt__F: /* cff mode options */
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags &= ~CFW_NO_FAMILY_OPT;
                break;
            case opt__O:
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags |= CFW_ROM_OPT;
                break;
            case opt_O:
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags &= ~CFW_ROM_OPT;
                break;
            case opt__S:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags |= CFW_SUBRIZE;
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_OTHERSUBRS_MASK;
                        h->t1w.flags |= T1W_OTHERSUBRS_PROCSET;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_S:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags &= ~CFW_SUBRIZE;
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_OTHERSUBRS_MASK;
                        h->t1w.flags |= T1W_OTHERSUBRS_PRIVATE;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt__T:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags &= ~(CFW_NO_FAMILY_OPT | CFW_ENABLE_CMP_TEST);
                        break;
                    case mode_t1:
                        h->t1w.flags &= ~T1W_ENABLE_CMP_TEST;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_Z:
                if (h->mode != mode_cff)
                    goto wrongmode;
#if 0
                /* Although CFW_NO_DEP_OPS is defined in cffwrite.h,
                   it is not used anywhere. */
                h->cfw.flags |= CFW_NO_DEP_OPS;
#endif
                h->t1r.flags |= T1R_UPDATE_OPS;
                h->cfr.flags |= CFR_UPDATE_OPS;
                break;
            case opt__Z:
                h->t1r.flags &= ~T1R_UPDATE_OPS;
                h->cfr.flags &= ~CFR_UPDATE_OPS;
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags &= ~CFW_NO_DEP_OPS;
                break;
            case opt__d:
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags |= CFW_WARN_DUP_HINTSUBS;
                break;
            case opt_d:
                switch (h->mode) {
                    case mode_dump:
                        h->t1r.flags |= T1R_UPDATE_OPS;
                        h->cfr.flags |= CFR_UPDATE_OPS;
                        break;
                    case mode_ps:
                        h->abf.draw.flags |= ABF_DUPLEX_PRINT;
                        break;
                    case mode_cff:
                        h->cfw.flags &= ~CFW_WARN_DUP_HINTSUBS;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_q: /* t1 mode options */
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.options |= T1W_NO_UID;
                break;
            case opt__q:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.options &= ~T1W_NO_UID;
                break;
            case opt_w:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.flags &= ~T1W_WIDTHS_ONLY;
                break;
            case opt__w:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.flags |= T1W_WIDTHS_ONLY;
                break;
            case opt_lf:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_NEWLINE_MASK;
                    h->svw.flags |= SVW_NEWLINE_UNIX;
                } else if (h->mode == mode_t1) {
                    if (h->t1w.options & T1W_REFORMAT)
                        goto t1clash;
                    h->t1w.flags &= ~T1W_NEWLINE_MASK;
                    h->t1w.flags |= T1W_NEWLINE_UNIX;
                } else
                    goto wrongmode;
                break;
            case opt_cr:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_NEWLINE_MASK;
                    h->svw.flags |= SVW_NEWLINE_MAC;
                } else if (h->mode == mode_t1) {
                    if (h->t1w.options & T1W_REFORMAT)
                        goto t1clash;
                    h->t1w.flags &= ~T1W_NEWLINE_MASK;
                    h->t1w.flags |= T1W_NEWLINE_MAC;
                } else
                    goto wrongmode;
                break;
            case opt_crlf:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_NEWLINE_MASK;
                    h->svw.flags |= SVW_NEWLINE_WIN;
                } else if (h->mode == mode_t1) {
                    if (h->t1w.options & T1W_REFORMAT)
                        goto t1clash;
                    h->t1w.flags &= ~T1W_NEWLINE_MASK;
                    h->t1w.flags |= T1W_NEWLINE_WIN;
                } else
                    goto wrongmode;
                break;
            case opt_cid:
                readCIDFontInfo(h, argv[++i]);
                break;
            case opt_hints:
                mergeInfo->hintsOnly = 1;
                break;
            case opt_decid:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.options |= T1W_DECID;
                break;
            case opt_usefd:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (!argsleft)
                    goto noarg;
                else {
                    char *p;
                    h->t1w.fd = strtol(argv[++i], &p, 0);
                    if (*p != '\0' || h->t1w.fd < 0)
                        goto badarg;
                }
                h->t1w.options |= T1W_USEFD;
                break;
            case opt_pfb:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.flags = (T1W_TYPE_HOST |
                                T1W_ENCODE_BINARY |
                                T1W_OTHERSUBRS_PRIVATE |
                                T1W_NEWLINE_WIN);
                h->t1w.lenIV = 4;
                h->t1w.options |= T1W_REFORMAT;
                break;
            case opt_LWFN:
                if (h->mode != mode_t1)
                    goto wrongmode;
                else if (h->t1w.options & T1W_REFORMAT)
                    goto t1clash;
                h->t1w.flags = (T1W_TYPE_HOST |
                                T1W_ENCODE_BINARY |
                                T1W_OTHERSUBRS_PRIVATE |
                                T1W_NEWLINE_MAC);
                h->t1w.lenIV = 4;
                h->t1w.options |= T1W_REFORMAT;
                break;
            case opt_z: /* bc mode options */
                goto bc_gone;
                break;
            case opt_sha1:
                goto bc_gone;
                break;
            case opt_cmp:
                goto bc_gone;
                break;
            case opt_c:
                switch (h->mode) {
                    case mode_t1:
                        h->t1w.flags &= ~T1W_ENCODE_MASK;
                        h->t1w.flags |= T1W_ENCODE_ASCII85;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_F: /* Shared options */
                switch (h->mode) {
                    case mode_cef:
                        if (!argsleft)
                            goto noarg;
                        h->arg.cef.F = argv[++i];
                        break;
                    case mode_cff:
                        h->cfw.flags |= CFW_NO_FAMILY_OPT;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_T:
                switch (h->mode) {
                    case mode_dcf:
                        if (!argsleft)
                            goto noarg;
                        dcf_ParseTableArg(h, argv[++i]);
                        break;
                    case mode_cff:
                        h->cfw.flags |= CFW_NO_FAMILY_OPT | CFW_ENABLE_CMP_TEST;
                        break;
                    case mode_t1:
                        h->t1w.flags |= T1W_ENABLE_CMP_TEST;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt__b:
                if (h->mode != mode_cff)
                    goto wrongmode;
                h->cfw.flags |= CFW_PRESERVE_GLYPH_ORDER;
                break;
            case opt_b:
                switch (h->mode) {
                    case mode_t1:
                        h->t1w.flags &= ~T1W_ENCODE_MASK;
                        h->t1w.flags |= T1W_ENCODE_BINARY;
                        break;
                    case mode_cff:
                        h->cfw.flags &= ~CFW_PRESERVE_GLYPH_ORDER;
                        break;
                    case mode_dcf:
                        h->dcf.flags &= ~DCF_BreakFlowed;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_e:
                switch (h->mode) {
                    case mode_ps:
                        h->abf.draw.flags |= ABF_SHOW_BY_ENC;
                        break;
                    case mode_t1:
                        if (h->t1w.options & T1W_REFORMAT)
                            goto t1clash;
                        else if (!argsleft)
                            goto noarg;
                        else {
                            char *p;
                            h->t1w.lenIV = (int)strtol(argv[++i], &p, 0);
                            if (*p != '\0')
                                goto badarg;
                            switch (h->t1w.lenIV) {
                                case -1:
                                case 0:
                                case 1:
                                case 4:
                                    break;
                                default:
                                    goto badarg;
                            }
                        }
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_gx:
                if ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT))
                    goto subsetclash;
                h->flags |= SUBSET__EXCLUDE_OPT;
            case opt_g:
                if (!argsleft)
                    goto noarg;
                else {
                    char *p;

                    if ((h->arg.g.cnt > 0) && ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT)))
                        goto subsetclash;

                    /* Convert comma-terminated substrings to null-terminated*/
                    h->arg.g.cnt = 1;
                    h->arg.g.substrs = argv[++i];
                    for (p = strchr(h->arg.g.substrs, ',');
                         p != NULL;
                         p = strchr(p, ',')) {
                        *p++ = '\0';
                        h->arg.g.cnt++;
                    }
                }
                h->flags |= SUBSET_OPT;
                break;
            case opt_gn0:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
                    h->svw.flags |= SVW_GLYPHNAMES_NONE;
                } else
                    goto wrongmode;
                break;
            case opt_gn1:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
                    h->svw.flags |= SVW_GLYPHNAMES_NONASCII;
                } else
                    goto wrongmode;
                break;
            case opt_gn2:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
                    h->svw.flags |= SVW_GLYPHNAMES_ALL;
                } else
                    goto wrongmode;
                break;
            case opt_abs:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG))) {
                    h->svw.flags |= SVW_ABSOLUTE;
                } else
                    goto wrongmode;
                break;
            case opt_sa:
                if ((h->mode == mode_svg) ||
                    ((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
                    h->svw.flags |= SVW_STANDALONE;
                else
                    goto wrongmode;
                break;
            case opt_N:
                h->flags |= SHOW_NAMES;
                break;
            case opt_p:
                if (!argsleft)
                    goto noarg;
                else if (h->flags & SUBSET_OPT)
                    goto subsetclash;
                h->arg.p = argv[++i];
                srand(0);
                h->flags |= SUBSET_OPT;
                break;
            case opt_pg:
                h->flags |= PRESERVE_GID;
                break;
            case opt_P:
                if (!argsleft)
                    goto noarg;
                else if (h->flags & SUBSET_OPT)
                    goto subsetclash;
                h->arg.P = argv[++i];
                seedtime();
                h->flags |= SUBSET_OPT;
                break;
            case opt_U:
                if (!argsleft)
                    goto noarg;
                h->arg.U = argv[++i];
                break;

            case opt_UNC:
                h->flags |= NO_UDV_CLAMPING;
                break;

            case opt_fdx:
                if ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT))
                    goto subsetclash;
                h->flags |= SUBSET__EXCLUDE_OPT;
            case opt_fd:
                if (!argsleft)
                    goto noarg;
                else if ((h->arg.g.cnt > 0) && ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT)))
                    goto subsetclash;
                else {
                    /* Convert comma-terminated substrings to null-terminated*/
                    char *p;
                    h->arg.g.cnt = 1;
                    h->arg.g.substrs = argv[++i];
                    for (p = strchr(h->arg.g.substrs, ',');
                         p != NULL;
                         p = strchr(p, ',')) {
                        *p++ = '\0';
                        h->arg.g.cnt++;
                    }
                }
                h->flags |= SUBSET_OPT;
                // Parse FD argument.
                parseFDSubset(h);
                break;
            case opt_i:
                if (!argsleft)
                    goto noarg;
                h->arg.i = argv[++i];
                break;
            case opt_X:
                h->ttr.flags |= TTR_BOTH_PATHS;
                break;
            case opt_x:
                h->ttr.flags |= TTR_EXACT_PATH;
                break;
            case opt_o: /* file options */
                if (!argsleft)
                    goto noarg;
                dstFileSetName(h, argv[++i]);
                break;
            case opt_f:
                if (!argsleft)
                    goto noarg;
                i = doMultiFileSet(h, argc, argv, i + 1);
                break;
            case opt_a:
                if (!argsleft)
                    goto noarg;
                h->flags |= AUTO_FILE_FROM_FILE;
                i = doAutoFileSet(h, argc, argv, i + 1);
                h->flags &= ~AUTO_FILE_FROM_FILE;
                break;
            case opt_A:
                if (!argsleft)
                    goto noarg;
                h->flags |= AUTO_FILE_FROM_FONT;
                i = doAutoFileSet(h, argc, argv, i + 1);
                h->flags &= ~AUTO_FILE_FROM_FONT;
                break;
            case opt_dd:
                if (!argsleft)
                    goto noarg;
                h->file.dd = argv[++i];
                break;
            case opt_sd:
                if (!argsleft)
                    goto noarg;
                h->file.sd = argv[++i];
                break;
            case opt_sr:
                if (!argsleft)
                    goto noarg;
                h->file.sr = argv[++i];
                break;
            case opt_std:
                h->t1w.flags |= T1W_FORCE_STD_ENCODING;
                h->cfw.flags |= CFW_FORCE_STD_ENCODING;
            case opt_r:
                h->flags |= DUMP_RES;
                break;
            case opt_R:
                h->flags |= DUMP_ASD;
                break;
            case opt_s:
                if (h->script.buf != NULL)
                    fatal(h, "nested scripts not allowed (-s)");
                else
                    fatal(h, "option must be last (-s)");
            case opt_t:
                h->t1r.flags |= T1R_DUMP_TOKENS;
                break;
            case opt_m: /* Memory failure simulator */
                if (!argsleft)
                    goto noarg;
                else {
                    char *p = argv[++i];
                    char *q;
                    long cnt = strtol(p, &q, 0);
                    if (*q != '\0')
                        goto badarg;
                    else if (*p == '-')
                        /* Fail on specified call */
                        h->failmem.iFail = -cnt;
                    else if (cnt == 0)
                        /* Report number of calls */
                        h->failmem.iFail = FAIL_REPORT;
                    else {
                        /* Fail on random call */
                        seedtime();
                        h->failmem.iFail = randrange(cnt - 1);
                    }
                }
                break;
            case opt_u:
                usage(h);
            case opt_h:
            case opt_h1:
            case opt_h2:
            case opt_h3:
                help(h);
            case opt_v:
                printVersions(h);
            case opt_y:
                h->flags |= EVERY_FONT;
                break;
        }
    }

    if (!(h->flags & DONE_FILE))
        doSingleFileSet(h, "-");
    return;

wrongmode:
    fatal(h, "wrong mode (%s) for option (%s)", h->modename, arg);
noarg:
    fatal(h, "no argument for option (%s)", arg);
badarg:
    fatal(h, "bad arg (%s)", arg);
subsetclash:
    fatal(h, "options -g, -gx, -p, -P, or -fd are mutually exclusive");
t1clash:
    fatal(h, "options -pfb or -LWFN may not be used with other options");
bc_gone:
    fatal(h, "options -bc is no longer supported.");
}

/* Return tail component of path. */
static char *tail(char *path) {
    char *p = strrchr(path, '/');
    if (p == NULL)
        p = strrchr(path, '\\');
    return (p == NULL) ? path : p + 1;
}

/* Initialize local subr info element. */
static void initLocal(void *ctx, long cnt, SubrInfo *info) {
    txCtx h = ctx;
    while (cnt--) {
        dnaINIT(h->ctx.dna, info->stemcnt, 300, 2000);
        info++;
    }
}

/* Initialize context. */
static void txNew(txCtx h, char *progname) {
    ctlMemoryCallbacks cb;
    MergeInfo *mergeInfo = (MergeInfo *)h->appSpecificInfo;

    h->progname = progname;
    h->flags = 0;
    h->script.buf = NULL;

    h->arg.p = NULL;
    h->arg.P = NULL;
    h->arg.U = NULL;
    h->arg.i = NULL;
    h->arg.g.cnt = 0;
    h->arg.path.level = 0;

    h->src.print_file = 0;
    h->t1r.ctx = NULL;
    h->cfr.ctx = NULL;
    h->ttr.ctx = NULL;
    h->ttr.flags = 0;
    h->cfw.ctx = NULL;
    h->cef.ctx = NULL;
    h->abf.ctx = NULL;
    h->pdw.ctx = NULL;
    h->t1w.ctx = NULL;
    h->svw.ctx = NULL;
    h->svw.flags = 0;
    h->svr.ctx = NULL;
    h->svr.flags = 0;
    h->ufr.ctx = NULL;
    h->ufr.flags = 0;
    h->ufow.ctx = NULL;
    h->ufow.flags = 0;
    h->ufr.altLayerDir = NULL;
    mergeInfo->cidinfo.CIDFontName[0] = 0;
    mergeInfo->seenNotdef = 0;
    mergeInfo->mode = 0;
    mergeInfo->hintsOnly = 0;
    mergeInfo->compareNameOnly = 0;
    h->ctx.sfr = NULL;

    memInit(h);
    stmInit(h);

    /* Initialize dynarr library */
    cb.ctx = h;
    cb.manage = safeManage;
    h->ctx.dna = dnaNew(&cb, DNA_CHECK_ARGS);
    if (h->ctx.dna == NULL)
        fatal(h, "can't init dynarr lib");

    h->failmem.iCall = 0; /* Reset call index */

    dnaINIT(h->ctx.dna, h->src.glyphs, 256, 768);
    dnaINIT(h->ctx.dna, h->src.exclude, 256, 768);
    dnaINIT(h->ctx.dna, h->src.widths, 256, 768);
    dnaINIT(h->ctx.dna, h->src.streamStack, 10, 10);
    dnaINIT(h->ctx.dna, h->fonts, 1, 10);
    dnaINIT(h->ctx.dna, h->subset.glyphs, 256, 768);
    dnaINIT(h->ctx.dna, h->subset.args, 250, 500);
    dnaINIT(h->ctx.dna, h->res.map, 30, 30);
    dnaINIT(h->ctx.dna, h->res.names, 50, 100);
    dnaINIT(h->ctx.dna, h->asd.entries, 10, 10);
    dnaINIT(h->ctx.dna, h->script.args, 200, 3000);
    dnaINIT(h->ctx.dna, h->cef.subset, 256, 768);
    dnaINIT(h->ctx.dna, h->cef.gnames, 256, 768);
    dnaINIT(h->ctx.dna, h->cef.lookup, 256, 768);
    dnaINIT(h->ctx.dna, h->t1w.gnames, 2000, 80000);
    dnaINIT(h->ctx.dna, h->dcf.global.stemcnt, 300, 2000);
    dnaINIT(h->ctx.dna, h->dcf.local, 1, 15);
    h->dcf.local.func = initLocal;
    dnaINIT(h->ctx.dna, h->cmap.encoding, 1, 1);
    dnaINIT(h->ctx.dna, h->fd.fdIndices, 16, 16);
    dnaINIT(h->ctx.dna, h->cmap.segment, 1, 1);
    dnaINIT(h->ctx.dna, h->dcf.glyph, 256, 768);

    setMode(h, mode_dump);

    /* Clear the SEEN_MODE bit after setting the default mode */
    h->flags = 0;
}

/* Free context. */
static void txFree(txCtx h) {
    long i;

    memFree(h, h->script.buf);
    dnaFREE(h->src.glyphs);
    dnaFREE(h->src.exclude);
    dnaFREE(h->src.widths);
    dnaFREE(h->src.streamStack);
    dnaFREE(h->fonts);
    dnaFREE(h->subset.glyphs);
    dnaFREE(h->subset.args);
    dnaFREE(h->res.map);
    dnaFREE(h->res.names);
    dnaFREE(h->asd.entries);
    dnaFREE(h->script.args);
    dnaFREE(h->cef.subset);
    dnaFREE(h->cef.gnames);
    dnaFREE(h->cef.lookup);
    dnaFREE(h->t1w.gnames);
    dnaFREE(h->dcf.global.stemcnt);
    for (i = 0; i < h->dcf.local.size; i++)
        dnaFREE(h->dcf.local.array[i].stemcnt);
    dnaFREE(h->dcf.local);
    dnaFREE(h->dcf.glyph);
    dnaFREE(h->cmap.encoding);
    dnaFREE(h->fd.fdIndices);
    dnaFREE(h->cmap.segment);
    if (h->t1r.ctx != NULL)
        t1rFree(h->t1r.ctx);
    cfrFree(h->cfr.ctx);
    ttrFree(h->ttr.ctx);
    cfwFree(h->cfw.ctx);
    cefFree(h->cef.ctx);
    pdwFree(h->pdw.ctx);
    t1wFree(h->t1w.ctx);
    svwFree(h->svw.ctx);
    svrFree(h->svr.ctx);
    ufoFree(h->ufr.ctx);
    ufwFree(h->ufow.ctx);
    sfrFree(h->ctx.sfr);

    stmFree(h, &h->src.stm);
    stmFree(h, &h->dst.stm);
    stmFree(h, &h->cef.src);
    stmFree(h, &h->cef.tmp0);
    stmFree(h, &h->cef.tmp1);
    stmFree(h, &h->t1r.tmp);
    stmFree(h, &h->cfw.tmp);
    stmFree(h, &h->t1w.tmp);
    /* Don't close debug streams because they use stderr */

    dnaFree(h->ctx.dna);
    free(h->appSpecificInfo);
    free(h);
}

/* Main program. */
int CTL_CDECL main(int argc, char *argv[]) {
    txCtx h;
    char *progname;
#if PLAT_MAC
    argc = ccommand(&argv);
    (void)__reopen(stdin); /* Change stdin to binary mode */
#endif                     /* PLAT_MAC */

#if PLAT_WIN
    /* The Microsoft standard C-Library opens stderr in buffered mode in
       contravention of the C standard. The following code establishes the
       correct unbuffered mode */
    (void)setvbuf(stderr, NULL, _IONBF, 0);
#endif /* PLAT_WIN */

    /* Get program name */
    progname = tail(argv[0]);
    --argc;
    ++argv;

    /* Allocate program context */
    h = malloc(sizeof(struct txCtx_));
    if (h == NULL) {
        fprintf(stderr, "%s: out of memory\n", progname);
        return EXIT_FAILURE;
    }
    memset(h, 0, sizeof(struct txCtx_));

    h->app = APP_MERGEFONTS;
    h->appSpecificInfo = malloc(sizeof(MergeInfo));
    if (h == NULL) {
        fprintf(stderr, "%s: out of memory\n", progname);
        exit(1);
    }
    memset(h->appSpecificInfo, 0, sizeof(MergeInfo));

    h->appSpecificFree = txFree;

    txNew(h, progname);

    if (argc > 1 && getOptionIndex(argv[argc - 2]) == opt_s) {
        /* Option list ends with script option */
        int i;

        /* Copy args preceding -s */
        for (i = 0; i < argc - 2; i++)
            *dnaNEXT(h->script.args) = argv[i];

        /* Add args from script file */
        addArgs(h, argv[argc - 1]);

        parseArgs(h, (int)h->script.args.cnt, h->script.args.array);
    } else
        parseArgs(h, argc, argv);

    if (h->failmem.iFail == FAIL_REPORT) {
        fflush(stdout);
        fprintf(stderr, "mem_manage() called %ld times in this run.\n",
                h->failmem.iCall);
    }
    txFree(h);

    return 0;
}
