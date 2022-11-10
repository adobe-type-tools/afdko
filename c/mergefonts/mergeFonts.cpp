/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * mergeFonts. A minor modification of the tx code base.
 */

#include <string>

#include "tx_shared.h"

#define MERGEFONTS_VERSION CTL_MAKE_VERSION(1, 3, 0) /* derived from tx */

typedef long (*CompareFDArrayCall)(void *dest_ctx, abfTopDict *srcTop);
typedef long (*MergeFDArrayCall)(void *dest_ctx, abfTopDict *topDict, int *newFDIndex);

#define MAX_PS_NAME_LEN 128

typedef struct {
    char srcName[MAX_PS_NAME_LEN];
    char dstName[MAX_PS_NAME_LEN];
    long srcCID;
    long dstCID;
} GAEntry;

enum {  // Option enumeration
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

typedef struct {
    txCtx tx;
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
    struct {
        long cnt; /* ABF_EMPTY_ARRAY */
        long array[16];
    } XUID;
} MergeInfo;

/* -------------------------------- Options -------------------------------- */

/* Note: options.h must be ascii sorted by option string */

enum {  // Option enumeration
    mfopt_None, /* Not an option */
#define DCL_OPT(string, index) index,
#include "mf_options.h"
    mfopt_Count
};

static const char *mf_options[] = {
#undef DCL_OPT
#define DCL_OPT(string, index) string,
#include "mf_options.h"  // NOLINT(build/include)
};

/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information. */
static void mf_usage(txExtCtx h) {
    static const char *text[] = {
#include "mf_usage.h"
    };
    printText(ARRAY_LEN(text), text);
}

/* Show help information. */
static void mf_help(txExtCtx h) {
    /* General help */
    static const char *text[] = {
#include "mf_help.h"
    };
    printText(ARRAY_LEN(text), text);
}

static int mfGetOptionIndex(const char *key) {
    return getOptionIndex(key, mf_options, ARRAY_LEN(mf_options));
}

/* --------------------------------- t1 mode ------------------------------- */

static bool applyCIDFontInfo(txCtx h, bool fontisCID) {
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;
    if (mergeInfo->cidinfo.CIDFontName[0] != 0) {  // if the users has specified a cidfontinfo file, override the first font's cid info.
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
        } else if (mergeInfo->cidinfo.Trademark[0] != ABF_EMPTY_ARRAY) {
/* XXX never would have worked -- wait for std::string to fix
 *            if (mergeInfo->cidinfo.AdobeCopyright[0] == ABF_EMPTY_ARRAY)
 */
            h->top->Notice.ptr = mergeInfo->cidinfo.Trademark;
/*            else {
                STRCAT_S(h->top->Notice.ptr, MAX_COPYRIGHT_LEN, " ");
                STRCAT_S(h->top->Notice.ptr, MAX_COPYRIGHT_LEN,
                         mergeInfo->cidinfo.Trademark);
            }
 */
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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;
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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;
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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

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
        MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

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

/* tx cffread callback. */
static void mf_cfr_ReadFontCB(txExtCtx e) {
    MergeInfo *mergeInfo = (MergeInfo *) e->extData;

    if (mergeInfo->cidinfo.fsType != ABF_UNSET_INT) {
        mergeInfo->tx->top->FSType = mergeInfo->cidinfo.fsType;
    }
}

/* Merge font with cffread library. */
static void cfrMergeFont(txCtx h, long origin, bool isFirstFont, sourceCtx *srcCtx) {
    bool parentIsCID = 0;
    GAFileInfo *gaf = NULL;
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

    if (isFirstFont) {
        mergeInfo->mode = mode_cff;

        if (h->cfr.ctx == NULL) {  // In this case, we must be processing the first font.
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

static void readCIDFontInfo(MergeInfo *mergeInfo, const char *filePath) {
    int lineno;
    FILE *fp;
    txCtx h = mergeInfo->tx;

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
                    STRCAT_S(mergeInfo->cidinfo.AdobeCopyright, MAX_COPYRIGHT_LEN, buf);
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
                STRCPY_S(mergeInfo->cidinfo.CIDFontName, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "FullName"))
                STRCPY_S(mergeInfo->cidinfo.FullName, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "FamilyName"))
                STRCPY_S(mergeInfo->cidinfo.FamilyName, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "Weight"))
                STRCPY_S(mergeInfo->cidinfo.Weight, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "AdobeCopyright"))
                STRCPY_S(mergeInfo->cidinfo.AdobeCopyright, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "Trademark"))
                STRCPY_S(mergeInfo->cidinfo.Trademark, MAX_DICT_ENTRY_LEN, value);
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
                STRCPY_S(mergeInfo->cidinfo.Registry, MAX_DICT_ENTRY_LEN, value);
            else if (0 == strcmp(key, "Ordering"))
                STRCPY_S(mergeInfo->cidinfo.Ordering, MAX_DICT_ENTRY_LEN, value);
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
            h->logger->log(sFATAL, "Parse error in the CID fontinfo file \"%s\": missing required key \"FontName\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Registry[0] == 0) {
            h->logger->log(sFATAL, "Parse error in the CID fontinfo file \"%s\": missing required key \"Registry\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Ordering[0] == 0) {
            h->logger->log(sFATAL, "Parse error in the CID fontinfo file \"%s\": missing required key \"Ordering\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.Supplement == -1) {
            h->logger->log(sFATAL, "Parse error in the CID fontinfo file \"%s\": missing required key \"Supplement\".", filePath);
            missingKey = 1;
        } else if (mergeInfo->cidinfo.fontVersion == 0.0) {
            h->logger->log(sFATAL, "Parse error in the CID fontinfo file \"%s\": missing required key \"version\".", filePath);
            missingKey = 1;
        }
        if (missingKey)
            fatal(h, NULL);
    }

    return;
}

/* Merge the source font dict into the destination font, and then
call glyphBegin */
static int mergeGlyphBegin(abfGlyphCallbacks *cb, abfGlyphInfo *srcInfo) {
    int result;
    txCtx h = (txCtx) cb->indirect_ctx;
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;
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
        } else {  // src is keyed by CID
            if (gaf->gaType == gafSrcCID) {  // the glyph is being changed from CID to name-keyed
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
            const char *p = info->gname.ptr + 3;
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
            h->logger->log(sWARNING, "Skipping glyph '%s'. When converting to CID, only glyphs with a name in the form 'cidXXXXX' will be copied.", info->gname.ptr);
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
    struct stat fileStat;
    int statErrNo;

    /* Set src name */
    STRCPY_S(h->file.src, sizeof(h->file.src), srcname);

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

    std::string s = TX_PROGNAME(h);
    s += ": --- ";
    s += h->src.stm.filename;
    h->logger->set_context("txfile", sINFO, s.c_str());

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

    h->logger->clear_context("txfile");

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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;
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
    MergeInfo *mergeInfo = (MergeInfo *)h->ext->extData;

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
                h->logger->log(sINFO, "Adding font dict %d from %s.",
                               curMaxFD, h->src.stm.filename);
            }
        }

        fileIndex++;
    }

    if (fileIndex == 0)
        fatal(h, "empty file list.");
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

    return i;
}

/* Parse argument list. */
static int mf_processOption(txExtCtx e, int argc, char *argv[], int i) {
    MergeInfo *mergeInfo = (MergeInfo *)e->extData;
    txCtx h = mergeInfo->tx;
    int argsleft = argc - i - 1;

    const char *arg = argv[i];
    switch (mfGetOptionIndex(arg)) {
        case mfopt_None:
            /* Act like 'tx' if seen mode, else enter mergeFonts mode. */
            if (txGetOptionIndex(arg) == txopt_None && !(h->flags & SEEN_MODE)) {
                const char *dstPath;
                setMode(h, mode_cff);

                if (h->mode != mode_cff)
                    goto wrongmode;
                if (argsleft < 1)
                    fatal(h, "too few file args: need at least <dest file> <parent src> ");
                dstPath = arg;
                dstFileSetName(h, arg);
                i++; /* Consume arg */
                h->cfw.flags |= CFW_CHECK_IF_GLYPHS_DIFFER;
                h->cfw.flags |= CFW_PRESERVE_GLYPH_ORDER;
                i = doMergeFileSet(h, argc, argv, i);
                if (mergeInfo->mode != mode_cff) { /* output font 'h->file.dst" is cff; we need to convert to t1. */
                    setMode(h, mergeInfo->mode);
                    STRCPY_S(h->file.src, sizeof(h->file.src), dstPath);
                    STRCPY_S(h->file.dst, sizeof(h->file.dst), dstPath);
                    STRCAT_S(h->file.dst, sizeof(h->file.dst), ".temp");
                    doSingleFileSet(h, dstPath);
                    remove(h->file.src); /* Under Windows, rename fails if dst file exists. */
                    rename(h->file.dst, h->file.src);
                }
                return i;
            } else
                return i;
            break;
            case mfopt_cid:
                readCIDFontInfo(mergeInfo, argv[i+1]);
                return i+2;
                break;
            case mfopt_hints:
                mergeInfo->hintsOnly = 1;
                return i+1;
                break;
    }
    return i;
wrongmode:
    fatal(h, "wrong mode (%s) for option (%s)", h->modename, arg);
    return -1;
}

/* Main program. */
extern "C" int CTL_CDECL main__mergefonts(int argc, char *argv[]) {
    MergeInfo mi;
    struct txExtCtx_ e;
    struct txCtx_ h;

    memset(&mi, 0, sizeof(MergeInfo));
    memset(&e, 0, sizeof(struct txExtCtx_));
    memset(&h, 0, sizeof(struct txCtx_));

    // No free function because we're allocating statically
    // (the mergeInfo contents get cleaned up in doMergeFileSet)
    e.extData = &mi;
    e.progname = "mergefonts";
    e.version = MERGEFONTS_VERSION;
    e.processOption = &mf_processOption;
    e.usage = &mf_usage;
    e.help = &mf_help;
    e.cfr_ReadFontCB = &mf_cfr_ReadFontCB;

    mi.tx = &h;

    txNew(&h, &e);
    run_tx(&h, argc, argv);
    txFree(&h);

    return 0;
}
