/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * rotateFont. A minor modification of the tx code base.
 */

#include "tx_shared.h"

#define ROTATE_VERSION CTL_MAKE_VERSION(1, 3, 0) /* derived from tx */

#define kHADV_NOT_SPECIFIED 11111.0 /* If the value is this, we don't overwrite the glyphs original advance width*/
#define kkHADV_NOT_SPECIFIED_NAME "None"

typedef struct {
#define MAX_PS_NAMELEN 128
    char srcName[MAX_PS_NAMELEN];
    char dstName[MAX_PS_NAMELEN];
    long srcCID;
    long dstCID;
    float xOffset;
    float yOffset;
    float hAdv;
} RotateGlyphEntry;

typedef struct {
    txCtx tx;
    unsigned short flags;
#define ROTATE_MATRIX_SET 1              /* if the rotation option was set. */
#define ROTATE_KEEP_HINTS 1 << 1         /* if the rotation is some multiple of 90, then we can keep the hints in the charstring. */
#define ROTATE_0 1 << 2                  /* rotation is 0 degrees */
#define ROTATE_90 1 << 3                 /* rotation is 90 degrees */
#define ROTATE_180 1 << 4                /* rotation is 180 degrees */
#define ROTATE_270 1 << 5                /* rotation is 270 degrees */
#define ROT_OVERRIDE_WIDTH_FLAGS 1 << 6  /* User said which of the next two flags they want applied. */
#define ROT_ORIG_WIDTHS 1 << 7           /* set undefined widths to the original width times the transform */
#define ROT_TRANSFORM_WIDTHS 1 << 8      /* set undefined widths to the original width times the transform */
#define ROT_SET_WIDTH_EM 1 << 9          /* Set undefined widths to the em-size */
#define ROTATE_UPDATE_FONTMATRIX 1 << 10 /* Scale FontMatrix as well as coordinates. */
    float origMatrix[6];            /* rotation/translation matrix specified by user. */
    float curMatrix[6];             /* rotation/translation matrix currently in use. */
    abfGlyphCallbacks savedGlyphCB; /* originally chosen writer call-backs */
    void (*endfont)(txCtx h);       /* override to fix up hint dicts */
    char rtFile[FILENAME_MAX];      /* text file containing per-glyph entries */
    dnaDCL(RotateGlyphEntry, rotateGlyphEntries);
    RotateGlyphEntry *curRGE; /* index of RotateGlyphEntry for the current glyph */
} RotateInfo;

/* -------------------------------- Options -------------------------------- */

/* Note: options.h must be ascii sorted by option string */

enum {  // Option enumeration
    rfopt_None, /* Not an option */
#define DCL_OPT(string, index) index,
#include "rf_options.h"
    rfopt_Count
};

static const char *rf_options[] = {
#undef DCL_OPT
#define DCL_OPT(string, index) string,
#include "rf_options.h"  // NOLINT(build/include)
};

/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information. */
static void rf_usage(txExtCtx h) {
    static const char *text[] = {
#include "rf_usage.h"
    };
    printText(ARRAY_LEN(text), text);
}

/* Show help information. */
static void rf_help(txExtCtx h) {
    /* General help */
    static const char *text[] = {
#include "rf_help.h"
    };
    printText(ARRAY_LEN(text), text);
}

/* Return option index from key or opt_None if not found. */
static int rfGetOptionIndex(const char *key) {
    return getOptionIndex(key, rf_options, ARRAY_LEN(rf_options));
}

#define RND(v) ((float)floor((v) + 0.5))
#define RNDFLT(v) ((float)floor((v) + 0.5))

#define TX(x, y) \
    RND(rotateInfo->curMatrix[0] * (x) + rotateInfo->curMatrix[2] * (y) + rotateInfo->curMatrix[4])
#define TY(x, y) \
    RND(rotateInfo->curMatrix[1] * (x) + rotateInfo->curMatrix[3] * (y) + rotateInfo->curMatrix[5])

/* Save font transformation matrix. */
static void scaleDicts(abfTopDict *top, float invScaleFactor) {
    int i = 0;
    float scaleFactor = 1 / invScaleFactor;
    float ppem = RND(top->sup.UnitsPerEm * scaleFactor);

    while (i < top->FDArray.cnt) {
        /*  Adjust the FDArray font Dict FontMatrix  and PrivateDict values.*/
        abfFontDict *fdict = &top->FDArray.array[i++];
        float *b = fdict->FontMatrix.array;

        if (fdict->FontMatrix.cnt != ABF_EMPTY_ARRAY) {
            int j;
            for (j = 0; j < 6; j++)
                b[j] = b[j] * scaleFactor;
            if (ppem == 1000.0) {
                b[0] = (float)0.001; /* to avoid setting the value to 0.00099999999 due to rounding error */
                b[3] = (float)0.001;
            }
        } else {
            float invEm = (float)0.001 * scaleFactor;
            b[0] = invEm;
            b[3] = invEm;
            b[1] = 0.0;
            b[2] = 0.0;
            b[4] = 0.0;
            b[5] = 0.0;
        }
        fdict->FontMatrix.cnt = 6;
        /* Unlike IS, we don't scale the private dict values here,
           as they are always scaled by code in rotateEndFont */
    }

    /*  Adjust the other top dict values .*/
    top->UnderlinePosition = RNDFLT(top->UnderlinePosition * invScaleFactor);
    top->UnderlineThickness = RNDFLT(top->UnderlineThickness * invScaleFactor);

    /* Unlike IS, we don't scale the FontBBox values here,
       as they are always scaled by code in rotateEndFont */

    return;
}

/* Scale coordinates by matrix. */
#define SX(x) RND(rotateInfo->curMatrix[0] * (x))
#define SY(y) RND(rotateInfo->curMatrix[3] * (y))
#define S1(x) RND(rotateInfo->curMatrix[1] * (x))
#define S2(y) RND(rotateInfo->curMatrix[2] * (y))

static void rotateEndFont(txCtx h) {
    abfTopDict *top = h->top;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    float FontBBox[8];
    float tempArrray[14];
    float *topFontBBox = top->FontBBox;
    int i, j;
    long cnt, tempCnt;

    /* restore matrix values */
    for (i = 0; i < 6; i++)
        rotateInfo->curMatrix[i] = rotateInfo->origMatrix[i];

    /* If requested, change the FontMatrix */
    if (rotateInfo->flags & ROTATE_UPDATE_FONTMATRIX) {
        scaleDicts(top, rotateInfo->origMatrix[0]);
        top->sup.UnitsPerEm = (long)RND(top->sup.UnitsPerEm * rotateInfo->origMatrix[0]);
    }
    /* Fix the font BBox. */

    /* If the rotation is a multiple of 90 degrees, just rotate/scale/transform
       the BBOX. Else calculate a BBox that will contain the rotated BBox. */
    FontBBox[0] = TX(topFontBBox[0], topFontBBox[1]);
    FontBBox[1] = TY(topFontBBox[0], topFontBBox[1]);
    FontBBox[2] = TX(topFontBBox[2], topFontBBox[3]);
    FontBBox[3] = TY(topFontBBox[2], topFontBBox[3]);

    for (i = 0; i < 2; i++) {
        topFontBBox[i] = FontBBox[i];
        if (topFontBBox[i] > FontBBox[i + 2])
            topFontBBox[i] = FontBBox[i + 2];

        topFontBBox[i + 2] = FontBBox[i];
        if (topFontBBox[i + 2] < FontBBox[i + 2])
            topFontBBox[i + 2] = FontBBox[i + 2];
    }

    /* Rotation is not a multiple of 90 degrees. We need to examine the other
       two corners of the fontBBox to see if they now define new
       maxima/minima */
    if (!(rotateInfo->flags & ROTATE_KEEP_HINTS)) {
        FontBBox[4] = TX(topFontBBox[0], topFontBBox[3]);
        FontBBox[5] = TY(topFontBBox[0], topFontBBox[3]);
        FontBBox[6] = TX(topFontBBox[2], topFontBBox[1]);
        FontBBox[7] = TY(topFontBBox[2], topFontBBox[1]);

        for (i = 0; i < 2; i++) {
            if (topFontBBox[i] > FontBBox[i + 4])
                topFontBBox[i] = FontBBox[i + 4];
            if (topFontBBox[i] > FontBBox[i + 6])
                topFontBBox[i] = FontBBox[i + 6];

            if (topFontBBox[i + 2] < FontBBox[i + 4])
                topFontBBox[i + 2] = FontBBox[i + 4];
            if (topFontBBox[i + 2] < FontBBox[i + 6])
                topFontBBox[i + 2] = FontBBox[i + 6];
        }

        /* This new Box is now probably bigger than needed, but it certainly contains all glyphs. */
    }

    /* Now for the hint data */
    for (i = 0; i < top->FDArray.cnt; i++) {
        abfPrivateDict *priv = &top->FDArray.array[i].Private;
        if (rotateInfo->flags & ROTATE_0) {  // just scale and transform in place
            cnt = priv->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
               priv->BlueValues.array[j] = TY(0, priv->BlueValues.array[j]);

            cnt = priv->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->OtherBlues.array[j] = TY(0, priv->OtherBlues.array[j]);

            cnt = priv->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->FamilyBlues.array[j] = TY(0, priv->FamilyBlues.array[j]);

            cnt = priv->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->FamilyOtherBlues.array[j] = TY(0, priv->FamilyOtherBlues.array[j]);

            if (priv->StdHW != ABF_UNSET_REAL)
               priv->StdHW = SY(priv->StdHW);

            if (priv->StdVW != ABF_UNSET_REAL)
               priv->StdVW = SX(priv->StdVW);

            cnt = priv->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapH.array[j] = SY(priv->StemSnapH.array[j]);

            cnt = priv->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapV.array[j] = SX(priv->StemSnapV.array[j]);
        } else if (rotateInfo->flags & ROTATE_180) {  // just scale and transform in place, with reverse sort
            cnt = priv->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, priv->BlueValues.array[j]);
            for (j = 0; j < cnt; j++)
               priv->BlueValues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = priv->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, priv->OtherBlues.array[j]);
            for (j = 0; j < cnt; j++)
               priv->OtherBlues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = priv->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, priv->FamilyBlues.array[j]);
            for (j = 0; j < cnt; j++)
               priv->FamilyBlues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = priv->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, priv->FamilyOtherBlues.array[j]);
            for (j = 0; j < cnt; j++)
               priv->FamilyOtherBlues.array[cnt - (j + 1)] = tempArrray[j];

            if (priv->StdHW != ABF_UNSET_REAL)
               priv->StdHW = -SY(priv->StdHW);

            if (priv->StdVW != ABF_UNSET_REAL)
               priv->StdVW = -SX(priv->StdVW);

            cnt = priv->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapH.array[j] = -SY(priv->StemSnapH.array[j]);

            cnt = priv->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapV.array[j] = -SX(priv->StemSnapV.array[j]);
        } else if (rotateInfo->flags & ROTATE_90) {  // just scale and transform in place. Swap V and H. The new H ints need a reverse sort.
            /* We no longer have sensible values for alignment zones. Use hard-coded value */
           priv->BlueValues.cnt = 4;
           priv->BlueValues.array[0] = -250;
           priv->BlueValues.array[1] = -250;
           priv->BlueValues.array[2] = 1100;
           priv->BlueValues.array[3] = 1100;
           priv->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;

            tempArrray[0] = priv->StdVW;
            if (priv->StdHW != ABF_UNSET_REAL)
               priv->StdVW = S2(priv->StdHW);

            if (tempArrray[0] != ABF_UNSET_REAL)
               priv->StdHW = -S1(tempArrray[0]);

            tempCnt = priv->StemSnapV.cnt;
            for (j = 0; j < tempCnt; j++)
                tempArrray[j] = priv->StemSnapV.array[j];

            for (j = 0; j < priv->StemSnapH.cnt; j++)
               priv->StemSnapV.array[j] = S2(priv->StemSnapH.array[j]);
           priv->StemSnapV.cnt = priv->StemSnapH.cnt;

            for (j = 0; j < tempCnt; j++)
               priv->StemSnapH.array[j] = -S1(tempArrray[j]);
           priv->StemSnapH.cnt = tempCnt;
        } else if (rotateInfo->flags & ROTATE_270) {  // just scale and transform in place. Swap V and H. The new V ints need a reverse sort.
            /* We no longer have sensible values for alignment zones. Use  hard-coded value */
           priv->BlueValues.cnt = 4;
           priv->BlueValues.array[0] = -250;
           priv->BlueValues.array[1] = -250;
           priv->BlueValues.array[2] = 1100;
           priv->BlueValues.array[3] = 1100;
           priv->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;

            tempArrray[0] = priv->StdVW;
            if (priv->StdHW != ABF_UNSET_REAL)
               priv->StdVW = -S2(priv->StdHW);

            if (tempArrray[0] != ABF_UNSET_REAL)
               priv->StdHW = S1(tempArrray[0]);

            tempCnt = priv->StemSnapV.cnt;
            for (j = 0; j < tempCnt; j++)
                tempArrray[j] = priv->StemSnapV.array[j];

            for (j = 0; j < priv->StemSnapH.cnt; j++)
               priv->StemSnapV.array[j] = -S2(priv->StemSnapH.array[j]);
           priv->StemSnapV.cnt = priv->StemSnapH.cnt;

            for (j = 0; j < tempCnt; j++)
               priv->StemSnapH.array[j] = S1(tempArrray[j]);
           priv->StemSnapH.cnt = tempCnt;
        } else if (rotateInfo->curMatrix[1] == 0) {  // shear, scale and transform in place
            cnt = priv->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
               priv->BlueValues.array[j] = TY(0, priv->BlueValues.array[j]);

            cnt = priv->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->OtherBlues.array[j] = TY(0, priv->OtherBlues.array[j]);

            cnt = priv->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->FamilyBlues.array[j] = TY(0, priv->FamilyBlues.array[j]);

            cnt = priv->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               priv->FamilyOtherBlues.array[j] = TY(0, priv->FamilyOtherBlues.array[j]);

            if (priv->StdHW != ABF_UNSET_REAL)
               priv->StdHW = SY(priv->StdHW);

            if (priv->StdVW != ABF_UNSET_REAL)
               priv->StdVW = SX(priv->StdVW);

            cnt = priv->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapH.array[j] = SY(priv->StemSnapH.array[j]);

            cnt = priv->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               priv->StemSnapV.array[j] = SX(priv->StemSnapV.array[j]);
        } else { /* rotation is not a multiple of 90 degrees. We have discarded all hints.*/
           priv->BlueValues.cnt = 4;
           priv->BlueValues.array[0] = -250;
           priv->BlueValues.array[1] = -250;
           priv->BlueValues.array[2] = 1100;
           priv->BlueValues.array[3] = 1100;
           priv->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           priv->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;
           priv->StdVW = ABF_UNSET_REAL;
           priv->StdHW = ABF_UNSET_REAL;
           priv->StemSnapH.cnt = ABF_EMPTY_ARRAY;
           priv->StemSnapV.cnt = ABF_EMPTY_ARRAY;
           h->logger->msg(sWARNING, "Have discarded all global hint values, and set alignment zones arbitrarily. Please fix.");
        }
    }

    rotateInfo->endfont(h); /* call original  h->dst.endfont */
}

static int CTL_CDECL matchRGE(const void *key, const void *value) {
    long firstCID = *(long *)key;
    char *secondName = ((RotateGlyphEntry *)value)->srcName;
    long secondCID = ((RotateGlyphEntry *)value)->srcCID;

    if (secondName[0] != 0)
        return strcmp((char *)key, secondName);

    return (firstCID > secondCID) ? 1 : (firstCID < secondCID) ? -1 : 0;
}

static int rotate_beg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    long srcCID = info->cid;
    const char *srcName = info->gname.ptr;
    RotateGlyphEntry *rge;
    rotateInfo->savedGlyphCB.info = info;
    cb->info = info;

    if (rotateInfo->rotateGlyphEntries.cnt > 0) {
        if (info->flags & ABF_GLYPH_CID)
            rge = (RotateGlyphEntry *)bsearch(&srcCID, rotateInfo->rotateGlyphEntries.array, rotateInfo->rotateGlyphEntries.cnt,
                                              sizeof(RotateGlyphEntry), matchRGE);
        else
            rge = (RotateGlyphEntry *)bsearch(srcName, rotateInfo->rotateGlyphEntries.array, rotateInfo->rotateGlyphEntries.cnt,
                                              sizeof(RotateGlyphEntry), matchRGE);

        rotateInfo->curRGE = rge;
        if (rge == NULL)
            return ABF_SKIP_RET;

        if (info->flags & ABF_GLYPH_CID)
            info->cid = (unsigned short)rge->dstCID;
        else {
            if (0 != strcmp(info->gname.ptr, rge->dstName))
                info->encoding.code = ABF_GLYPH_UNENC;
            info->gname.ptr = rge->dstName;
        }
        rotateInfo->curMatrix[4] = rge->xOffset;
        rotateInfo->curMatrix[5] = rge->yOffset;
    }
    return rotateInfo->savedGlyphCB.beg(&rotateInfo->savedGlyphCB, info);
}

static void rotate_width(abfGlyphCallbacks *cb, float hAdv) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    int seenRGEVal = 0;

    if (rotateInfo->curRGE != NULL) {
        float tmp = (float)rotateInfo->curRGE->hAdv;
        if (tmp != kHADV_NOT_SPECIFIED) {
            hAdv = tmp;
            seenRGEVal = 1;
        }
    }
    if (!seenRGEVal) {
        if (rotateInfo->flags & ROT_TRANSFORM_WIDTHS)
            hAdv = RND(rotateInfo->curMatrix[0] * (hAdv) + rotateInfo->curMatrix[4]);
        else if (rotateInfo->flags & ROT_SET_WIDTH_EM)
            hAdv = (float)h->top->sup.UnitsPerEm;
    }

    rotateInfo->savedGlyphCB.width(&rotateInfo->savedGlyphCB, hAdv);
}

static void rotate_move(abfGlyphCallbacks *cb, float x0, float y0) {
    float x, y;
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    x = TX(x0, y0);
    y = TY(x0, y0);
    rotateInfo->savedGlyphCB.move(&rotateInfo->savedGlyphCB, x, y);
}

static void rotate_line(abfGlyphCallbacks *cb, float x1, float y1) {
    float x, y;
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    x = TX(x1, y1);
    y = TY(x1, y1);
    rotateInfo->savedGlyphCB.line(&rotateInfo->savedGlyphCB, x, y);
}

static void rotate_curve(abfGlyphCallbacks *cb,
                         float x1, float y1,
                         float x2, float y2,
                         float x3, float y3) {
    float xn1, yn1, xn2, yn2, xn3, yn3;
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    xn1 = TX(x1, y1);
    yn1 = TY(x1, y1);
    xn2 = TX(x2, y2);
    yn2 = TY(x2, y2);
    xn3 = TX(x3, y3);
    yn3 = TY(x3, y3);

    rotateInfo->savedGlyphCB.curve(&rotateInfo->savedGlyphCB, xn1, yn1, xn2, yn2, xn3, yn3);
}

static void rotate_stem(abfGlyphCallbacks *cb,
                        int flags, float edge0, float edge1) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    float *farray = rotateInfo->curMatrix;
    float temp = edge0;
    float offset, scale;
    /* this function gets called only if the rotation matrix is some multiple of 90 degrees */
    if (rotateInfo->flags & ROTATE_0) {  // 0 degrees, scaling and translation only
        /* H hints remain H hints. */
        if (flags & ABF_VERT_STEM) {
            offset = farray[4];
            scale = farray[0];
        } else {
            offset = farray[5];
            scale = farray[3];
        }

        edge0 = offset + RND(scale * edge0);
        edge1 = offset + RND(scale * edge1);
    } else if (rotateInfo->flags & ROTATE_180) {  // 180 degrees
        /* H hints remain H hints. Since positive values become negative, edge0 and edge1 need to be swapped */
        if (flags & ABF_VERT_STEM) {
            offset = farray[4];
            scale = farray[0];
        } else {
            offset = farray[5];
            scale = farray[3];
        }

        edge0 = offset + RND(scale * edge1); /* both X and Y scale factors are negative */
        edge1 = offset + RND(scale * temp);
    } else if (rotateInfo->flags & ROTATE_90) {  // 90 degrees
        /* H hints (y0,y1)  become V hints (x0,x1). V hints become negative H hints, so edge0 and edge1 need to be swapped  */

        if (flags & ABF_VERT_STEM) {
            flags &= ~ABF_VERT_STEM; /* clear vert stem bit */
            offset = farray[5];
            scale = farray[1];
            edge0 = offset + RND(scale * edge1);
            edge1 = offset + RND(scale * temp);
        } else {
            flags |= ABF_VERT_STEM; /* add vert stem bit */
            offset = farray[4];
            scale = farray[2];
            edge0 = offset + RND(scale * edge0);
            edge1 = offset + RND(scale * edge1);
        }
    } else if (rotateInfo->flags & ROTATE_270) {  // -90 degrees
        /* H hints (y0,y1)  become negative V hints (x0,x1), so edge0 and edge1 needs to be swapped. V hints become H hints. */

        if (flags & ABF_VERT_STEM) {
            flags &= ~ABF_VERT_STEM; /* clear vert stem bit */
            offset = farray[5];
            scale = farray[1];
            edge0 = offset + RND(scale * edge0);
            edge1 = offset + RND(scale * edge1);
        } else {
            flags |= ABF_VERT_STEM; /* add vert stem bit */
            offset = farray[4];
            scale = farray[2];
            edge0 = offset + RND(scale * edge1);
            edge1 = offset + RND(scale * temp);
        }
    } else {
        if (farray[1] == 0) {  // X-scaling, Y shear and translation only. Can keep hhints.
            /* H hints remain H hints. */
            if (flags & ABF_VERT_STEM) {
                return; /* Because of the shear, vertical stems can't be applied. */
            } else {
                offset = farray[5];
                scale = farray[3];
            }

            edge0 = offset + RND(scale * edge0);
            edge1 = offset + RND(scale * edge1);
        } else
            fatal(h, "Program error - glyph stem hint callback called when rotation is not a multiple of 90 degrees.");
    }

    rotateInfo->savedGlyphCB.stem(&rotateInfo->savedGlyphCB, flags, edge0, edge1);
}

static void rotate_flex(abfGlyphCallbacks *cb, float depth,
                        float x1, float y1,
                        float x2, float y2,
                        float x3, float y3,
                        float x4, float y4,
                        float x5, float y5,
                        float x6, float y6) {
    float xn1, yn1, xn2, yn2, xn3, yn3, xn4, yn4, xn5, yn5, xn6, yn6;
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    xn1 = TX(x1, y1);
    yn1 = TY(x1, y1);
    xn2 = TX(x2, y2);
    yn2 = TY(x2, y2);
    xn3 = TX(x3, y3);
    yn3 = TY(x3, y3);
    xn4 = TX(x4, y4);
    yn4 = TY(x4, y4);
    xn5 = TX(x5, y5);
    yn5 = TY(x5, y5);
    xn6 = TX(x6, y6);
    yn6 = TY(x6, y6);

    rotateInfo->savedGlyphCB.flex(&rotateInfo->savedGlyphCB, depth,
                                  xn1, yn1,
                                  xn2, yn2,
                                  xn3, yn3,
                                  xn4, yn4,
                                  xn5, yn5,
                                  xn6, yn6);
}

static void rotate_genop(abfGlyphCallbacks *cb, int cnt, float *args, int op) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    rotateInfo->savedGlyphCB.genop(&rotateInfo->savedGlyphCB, cnt, args, op);
}

static void rotate_seac(abfGlyphCallbacks *cb,
                        float adx, float ady, int bchar, int achar) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    rotateInfo->savedGlyphCB.seac(&rotateInfo->savedGlyphCB, adx, ady, bchar, achar);
}

static void rotate_end(abfGlyphCallbacks *cb) {
    txCtx h = (txCtx) cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    rotateInfo->savedGlyphCB.end(&rotateInfo->savedGlyphCB);
}

static int CTL_CDECL cmpRGE(const void *first, const void *second) {
    int ret;
    if (((RotateGlyphEntry *)first)->srcName[0] != 0) /* font is name-keyed */
        ret = strcmp(((RotateGlyphEntry *)first)->srcName,
                     ((RotateGlyphEntry *)second)->srcName);
    else
        ret = (((RotateGlyphEntry *)first)->srcCID > ((RotateGlyphEntry *)second)->srcCID) ? 1 : (((RotateGlyphEntry *)first)->srcCID < ((RotateGlyphEntry *)second)->srcCID) ? -1 : 0;

    return ret;
}

static void rotateLoadGlyphList(txCtx h, char *filePath) {
    RotateInfo *rotateInfo = (RotateInfo *)h->ext->extData;
    int namesAreCID = -1;
    FILE *rtf_fp;
    char lineBuffer[MAX_PS_NAMELEN * 2];
    int lineno, len;

    rtf_fp = fopen(filePath, "rb");
    if (rtf_fp == NULL)
        fatal(h, "Failed to open glyph list file %s.", filePath);
    lineno = 1;
    while (1) {
        char *ret;
        RotateGlyphEntry *rge = NULL;
        char *cp;

        ret = fgets(lineBuffer, MAX_PS_NAMELEN, rtf_fp);
        if (ret == NULL)
            break;

        if (lineBuffer[1] == 0) /* contains only a new-line */
            continue;
        if (lineBuffer[0] == '#') /* comment line */
            continue;

        cp = lineBuffer;
        while (((*cp == ' ') || (*cp == '\t')) && (cp < &lineBuffer[MAX_PS_NAMELEN]))
            cp++;

        if ((*cp == '#') || (*cp == '\r') || (*cp == '\n'))
            continue;

        rge = dnaNEXT(rotateInfo->rotateGlyphEntries);
        {
            char advText[128];
            len = sscanf(lineBuffer, "%127s %127s %127s %f %f", rge->srcName, rge->dstName, advText, &rge->xOffset, &rge->yOffset);
            if (strcmp(advText, kkHADV_NOT_SPECIFIED_NAME) != 0)
                rge->hAdv = (float)atof(advText);
            else
                rge->hAdv = kHADV_NOT_SPECIFIED;
        }
        if (len != 5) {
            fatal(h, "Parse error in glyph alias file \"%s\" at line number %d: there were not five fields.", filePath, lineno - 1);
            dnaSET_CNT(rotateInfo->rotateGlyphEntries, rotateInfo->rotateGlyphEntries.cnt - 1);
            break;
        }
        lineno++;
        rge->srcCID = -1;
        rge->dstCID = -1;
        if ((rge->srcName[0] <= '9') && (rge->srcName[0] >= '0')) {  // glyph names may not begin with a number; must be a CID
            if (namesAreCID < 0)
                namesAreCID = 1;
            else if (namesAreCID != 1)
                fatal(h, "Error in rotate info file %s: first entry uses CID values, entry %s at line %d does not.", filePath, lineBuffer, lineno - 1);
            rge->srcCID = atoi(rge->srcName);
            rge->srcName[0] = 0;
            rge->dstCID = atoi(rge->dstName);
            rge->dstName[0] = 0;
        } else if (namesAreCID < 0)
            namesAreCID = 0;
        else if (namesAreCID != 0)
            fatal(h, "Error in rotate info file %s: first entry uses name-keyed values, entry %s at line %d uses CID value.", filePath, lineBuffer, lineno - 1);
    }

    /* sort list */
    if (rotateInfo->rotateGlyphEntries.cnt > 0)
        qsort(rotateInfo->rotateGlyphEntries.array, rotateInfo->rotateGlyphEntries.cnt, sizeof(rotateInfo->rotateGlyphEntries.array[0]), cmpRGE);

    fclose(rtf_fp);
    return;
}

static void rf_doFileCB(txExtCtx e) {
    RotateInfo *rotateInfo = (RotateInfo *)e->extData;
    txCtx h = (txCtx) rotateInfo->tx;

    if (!(rotateInfo->flags & ROTATE_MATRIX_SET))
        return;

    rotateInfo->savedGlyphCB = h->cb.glyph;
    h->cb.glyph.indirect_ctx = h;
    h->cb.glyph.beg = rotate_beg;
    h->cb.glyph.width = rotate_width;
    h->cb.glyph.move = rotate_move;
    h->cb.glyph.line = rotate_line;
    h->cb.glyph.curve = rotate_curve;
    if ((rotateInfo->flags & ROTATE_KEEP_HINTS) || (rotateInfo->origMatrix[1] == 0)) {  // rotation is some multiple of 90 degrees
        h->cb.glyph.stem = rotate_stem;
        h->cb.glyph.flex = rotate_flex;
    } else {
        h->cb.glyph.stem = NULL;
        h->cb.glyph.flex = NULL;
    }
    h->cb.glyph.genop = rotate_genop;
    h->cb.glyph.seac = rotate_seac;
    h->cb.glyph.end = rotate_end;
    rotateInfo->endfont = h->dst.endfont;
    h->dst.endfont = rotateEndFont;
    if (rotateInfo->rtFile[0] != 0)
        rotateLoadGlyphList(h, rotateInfo->rtFile);
}

static int setRotationMatrix(RotateInfo *rotateInfo, int argc, char **argv, int i, bool isMatrix) {
    txCtx h = (txCtx) rotateInfo->tx;
    float *farray = rotateInfo->origMatrix;
    char *arg = argv[i];
    int j;
    /* arguments may be either "degrees xoffset yoffset" or  the full 6 valued font transform matrix */
    if (isMatrix) {
        if (argc <= i + 6)
            fatal(h, "Not enough arguments for  rotation matrix. Need 6 decimal values.", arg);
        for (j = 0; j < 6; j++) {
            arg = argv[i + j];
            farray[j] = (float)atof(arg);
            if ((arg[0] != '0') && (farray[j] == 0.0))
                fatal(h, "Bad argument for rotation matrix: %s. Must be a decimal number.", arg);
        }
        i += 6;
    } else {
        float degrees = 0;
        if (argc <= i + 3)
            fatal(h, "Not enough arguments for rotation matrix. Need three decimal values: degrees, and x,y offset.", arg);
        for (j = 0; j < 3; j++) {
            double val;
            arg = argv[i + j];
            val = atof(arg);
            if ((arg[0] != '0') && (val == 0.0))
                fatal(h, "Bad argument for rotation matrix: %s. Must be a decimal number.", arg);
        }
        degrees = (float)atof(argv[i]);
        farray[4] = (float)atof(argv[i + 1]);
        farray[5] = (float)atof(argv[i + 2]);
        if ((degrees == 90.0) || (degrees == -270.0)) {
            farray[0] = 0;
            farray[1] = -1.0;
            farray[2] = 1.0;
            farray[3] = 0;
        } else if ((degrees == 180.0) || (degrees == -180.0)) {
            farray[0] = -1.0;
            farray[1] = 0;
            farray[2] = 0;
            farray[3] = -1.0;
        } else if ((degrees == 270.0) || (degrees == -90.0)) {
            farray[0] = 0;
            farray[1] = 1.0;
            farray[2] = -1.0;
            farray[3] = 0;
        } else if ((degrees == 0.0) || (degrees == 360.0)) {
            farray[0] = 1.0;
            farray[1] = 0;
            farray[2] = 0;
            farray[3] = 1.0;
        } else {
            degrees = (float)(3.14159 * degrees / 360.0);
            farray[0] = (float)cos(degrees);
            farray[1] = (float)-sin(degrees);
            farray[2] = (float)sin(degrees);
            farray[3] = (float)cos(degrees);
        }
        i += 3;
    }
    rotateInfo->flags |= ROTATE_MATRIX_SET;

    if (((farray[0] == 0) && (farray[3] == 0)) || ((farray[1] == 0) && (farray[2] == 0))) {
        rotateInfo->flags |= ROTATE_KEEP_HINTS; /* this is a rotation by some multiple of 90 degrees */
        if ((farray[0] < 0.0) && (farray[3] < 0.0))
            rotateInfo->flags |= ROTATE_180;
        else if ((farray[0] > 0.0) && (farray[3] > 0.0))
            rotateInfo->flags |= ROTATE_0;
        else if ((farray[1] < 0.0) && (farray[2] > 0.0)) /* 90 degrees.*/
            rotateInfo->flags |= ROTATE_90;
        else if ((farray[1] > 0.0) && (farray[2] < 0)) /* -90 degrees.*/
            rotateInfo->flags |= ROTATE_270;
    }

    if (!(rotateInfo->flags & ROT_OVERRIDE_WIDTH_FLAGS)) {
        if (farray[1] == 0)
            rotateInfo->flags |= ROT_TRANSFORM_WIDTHS; /* transform widths for no rotation, 180 mirror, or shearing */
        else
            rotateInfo->flags |= ROT_SET_WIDTH_EM;
    }

    for (j = 0; j < 6; j++)
        rotateInfo->curMatrix[j] = rotateInfo->origMatrix[j];

    return i;
}

/* Parse argument list. */
static int rf_processOption(txExtCtx e, int argc, char *argv[], int i) {
    RotateInfo *rotateInfo = (RotateInfo *)e->extData;
    txCtx h = (txCtx) rotateInfo->tx;
    int argsleft = argc - i - 1;

    const char *arg = argv[i];
    switch (rfGetOptionIndex(arg)) {
            case rfopt_row:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                return i+1;
                break;
            case rfopt_rtw:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                rotateInfo->flags |= ROT_TRANSFORM_WIDTHS;
                return i+1;
                break;
            case rfopt_rew:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                rotateInfo->flags |= ROT_SET_WIDTH_EM;
                return i+1;
                break;
            case rfopt_rt:
                if (rotateInfo->flags & ROTATE_MATRIX_SET)
                    fatal(h, "-rt option must precede -rtf option, and cannot be specified twice or with -matrix");
                return setRotationMatrix(rotateInfo, argc, argv, i+1, 0);
                break;
            case rfopt_rtf:
                if (!argsleft)
                    goto noarg;
                STRCPY_S(rotateInfo->rtFile, sizeof(rotateInfo->rtFile),
                         argv[i+1]);
                if (!(rotateInfo->flags & ROTATE_MATRIX_SET)) {
                    float *farray = rotateInfo->origMatrix;
                    int j;
                    farray[0] = 1.0;
                    farray[1] = 0;
                    farray[2] = 0;
                    farray[3] = 1.0;
                    farray[4] = 0;
                    farray[5] = 0;
                    for (j = 0; j < 6; j++)
                        rotateInfo->curMatrix[j] = rotateInfo->origMatrix[j];
                    rotateInfo->flags |= ROTATE_MATRIX_SET;
                    rotateInfo->flags |= ROTATE_KEEP_HINTS;
                    rotateInfo->flags |= ROTATE_0;
                }
                return i+2;
                break;
            case rfopt_matrix:
                if (rotateInfo->flags & ROTATE_MATRIX_SET)
                    fatal(h, "-matrix option must precede -rtf option, and cannot be specified twice or with -rt");
                return setRotationMatrix(rotateInfo, argc, argv, i+1, 1);
                break;
    }
    return i;
noarg:
    fatal(h, "no argument for option (%s)", arg);
    return 0;
}

/* Main program. */
extern "C" int CTL_CDECL main__rotatefont(int argc, char *argv[]) {
    RotateInfo ri;
    struct txExtCtx_ e;
    struct txCtx_ h;

    memset(&ri, 0, sizeof(RotateInfo));
    memset(&e, 0, sizeof(struct txExtCtx_));
    memset(&h, 0, sizeof(struct txCtx_));

    // No free function because we're allocating statically
    // (the mergeInfo contents get cleaned up in doMergeFileSet)
    e.extData = &ri;
    e.progname = "rotatefont";
    e.version = ROTATE_VERSION;
    e.processOption = &rf_processOption;
    e.usage = &rf_usage;
    e.help = &rf_help;
    e.doFileCB = &rf_doFileCB;

    ri.tx = &h;

    txNew(&h, &e);
    dnaINIT(h.ctx.dna, ri.rotateGlyphEntries, 256, 768);
    run_tx(&h, argc, argv);
    dnaFREE(ri.rotateGlyphEntries);
    txFree(&h);

    return 0;
}
