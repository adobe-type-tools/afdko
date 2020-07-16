/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * rotateFont. A minor modification of the tx code base.
 */

#include "tx_shared.h"

#define ROTATE_VERSION CTL_MAKE_VERSION(1, 2, 3) /* derived from tx */

#define kHADV_NOT_SPECIFIED 11111.0 /* If the value is this, we don't overwrite the glyphs original advance width*/
#define kkHADV_NOT_SPECIFIED_NAME "None"

typedef struct
{
#define MAX_PS_NAMELEN 128
    char srcName[MAX_PS_NAMELEN];
    char dstName[MAX_PS_NAMELEN];
    long srcCID;
    long dstCID;
    float xOffset;
    float yOffset;
    float hAdv;
} RotateGlyphEntry;

typedef struct
{
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

    unsigned short flags;
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

/* ---------------------------- cffread Library ---------------------------- */

/* Read font with cffread library. */
static void cfrReadFont(txCtx h, long origin, int ttcIndex) {
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

    h->dst.endfont(h);

    if (cfrEndFont(h->cfr.ctx))
        fatal(h, NULL);
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
    char version_buf[MAX_VERSION_SIZE];
    printf("    %-10s%s\n", libname, CTL_SPLIT_VERSION(version_buf, version));
}

/* Print library version numbers. */
static void printVersions(txCtx h) {
    ctlVersionCallbacks cb;
    char version_buf[MAX_VERSION_SIZE];

    printf(
        "Versions:\n"
        "    rotatefont        %s\n",
        CTL_SPLIT_VERSION(version_buf, ROTATE_VERSION));

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
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
        abfPrivateDict *private = &top->FDArray.array[i].Private;
        if (rotateInfo->flags & ROTATE_0) /* just scale and transform in place */
        {
            cnt = private->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
               private->BlueValues.array[j] = TY(0, private->BlueValues.array[j]);

            cnt = private->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->OtherBlues.array[j] = TY(0, private->OtherBlues.array[j]);

            cnt = private->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->FamilyBlues.array[j] = TY(0, private->FamilyBlues.array[j]);

            cnt = private->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->FamilyOtherBlues.array[j] = TY(0, private->FamilyOtherBlues.array[j]);

            if (private->StdHW != ABF_UNSET_REAL)
               private->StdHW = SY(private->StdHW);

            if (private->StdVW != ABF_UNSET_REAL)
               private->StdVW = SX(private->StdVW);

            cnt = private->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapH.array[j] = SY(private->StemSnapH.array[j]);

            cnt = private->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapV.array[j] = SX(private->StemSnapV.array[j]);
        } else if (rotateInfo->flags & ROTATE_180) /* just scale and transform in place, with reverse sort, */
        {
            cnt = private->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, private->BlueValues.array[j]);
            for (j = 0; j < cnt; j++)
               private->BlueValues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = private->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, private->OtherBlues.array[j]);
            for (j = 0; j < cnt; j++)
               private->OtherBlues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = private->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, private->FamilyBlues.array[j]);
            for (j = 0; j < cnt; j++)
               private->FamilyBlues.array[cnt - (j + 1)] = tempArrray[j];

            cnt = private->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
                tempArrray[j] = TY(0, private->FamilyOtherBlues.array[j]);
            for (j = 0; j < cnt; j++)
               private->FamilyOtherBlues.array[cnt - (j + 1)] = tempArrray[j];

            if (private->StdHW != ABF_UNSET_REAL)
               private->StdHW = -SY(private->StdHW);

            if (private->StdVW != ABF_UNSET_REAL)
               private->StdVW = -SX(private->StdVW);

            cnt = private->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapH.array[j] = -SY(private->StemSnapH.array[j]);

            cnt = private->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapV.array[j] = -SX(private->StemSnapV.array[j]);
        } else if (rotateInfo->flags & ROTATE_90) /* just scale and transform in place. Swap V and H. The new H ints need a reverse sort. */
        {
            /* We no longer have sensible values for alignment zones. Use hard-coded value */
           private->BlueValues.cnt = 4;
           private->BlueValues.array[0] = -250;
           private->BlueValues.array[1] = -250;
           private->BlueValues.array[2] = 1100;
           private->BlueValues.array[3] = 1100;
           private->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;

            tempArrray[0] = private->StdVW;
            if (private->StdHW != ABF_UNSET_REAL)
               private->StdVW = S2(private->StdHW);

            if (tempArrray[0] != ABF_UNSET_REAL)
               private->StdHW = -S1(tempArrray[0]);

            tempCnt = private->StemSnapV.cnt;
            for (j = 0; j < tempCnt; j++)
                tempArrray[j] = private->StemSnapV.array[j];

            for (j = 0; j < private->StemSnapH.cnt; j++)
               private->StemSnapV.array[j] = S2(private->StemSnapH.array[j]);
           private->StemSnapV.cnt = private->StemSnapH.cnt;

            for (j = 0; j < tempCnt; j++)
               private->StemSnapH.array[j] = -S1(tempArrray[j]);
           private->StemSnapH.cnt = tempCnt;
        } else if (rotateInfo->flags & ROTATE_270) /* just scale and transform in place. Swap V and H. The new V ints need a reverse sort. */
        {
            /* We no longer have sensible values for alignment zones. Use  hard-coded value */
           private->BlueValues.cnt = 4;
           private->BlueValues.array[0] = -250;
           private->BlueValues.array[1] = -250;
           private->BlueValues.array[2] = 1100;
           private->BlueValues.array[3] = 1100;
           private->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;

            tempArrray[0] = private->StdVW;
            if (private->StdHW != ABF_UNSET_REAL)
               private->StdVW = -S2(private->StdHW);

            if (tempArrray[0] != ABF_UNSET_REAL)
               private->StdHW = S1(tempArrray[0]);

            tempCnt = private->StemSnapV.cnt;
            for (j = 0; j < tempCnt; j++)
                tempArrray[j] = private->StemSnapV.array[j];

            for (j = 0; j < private->StemSnapH.cnt; j++)
               private->StemSnapV.array[j] = -S2(private->StemSnapH.array[j]);
           private->StemSnapV.cnt = private->StemSnapH.cnt;

            for (j = 0; j < tempCnt; j++)
               private->StemSnapH.array[j] = S1(tempArrray[j]);
           private->StemSnapH.cnt = tempCnt;
        } else if (rotateInfo->curMatrix[1] == 0) /* shear, scale and transform in place */
        {
            cnt = private->BlueValues.cnt;
            for (j = 0; j < cnt; j++)
               private->BlueValues.array[j] = TY(0, private->BlueValues.array[j]);

            cnt = private->OtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->OtherBlues.array[j] = TY(0, private->OtherBlues.array[j]);

            cnt = private->FamilyBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->FamilyBlues.array[j] = TY(0, private->FamilyBlues.array[j]);

            cnt = private->FamilyOtherBlues.cnt;
            for (j = 0; j < cnt; j++)
               private->FamilyOtherBlues.array[j] = TY(0, private->FamilyOtherBlues.array[j]);

            if (private->StdHW != ABF_UNSET_REAL)
               private->StdHW = SY(private->StdHW);

            if (private->StdVW != ABF_UNSET_REAL)
               private->StdVW = SX(private->StdVW);

            cnt = private->StemSnapH.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapH.array[j] = SY(private->StemSnapH.array[j]);

            cnt = private->StemSnapV.cnt;
            for (j = 0; j < cnt; j++)
               private->StemSnapV.array[j] = SX(private->StemSnapV.array[j]);
        } else { /* rotation is not a multiple of 90 degrees. We have discarded all hints.*/
           private->BlueValues.cnt = 4;
           private->BlueValues.array[0] = -250;
           private->BlueValues.array[1] = -250;
           private->BlueValues.array[2] = 1100;
           private->BlueValues.array[3] = 1100;
           private->OtherBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyBlues.cnt = ABF_EMPTY_ARRAY;
           private->FamilyOtherBlues.cnt = ABF_EMPTY_ARRAY;
           private->StdVW = ABF_UNSET_REAL;
           private->StdHW = ABF_UNSET_REAL;
           private->StemSnapH.cnt = ABF_EMPTY_ARRAY;
           private->StemSnapV.cnt = ABF_EMPTY_ARRAY;
            fprintf(stderr, "Have discarded all global hint values, and set alignment zones arbitrarily. Please fix.\n");
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    long srcCID = info->cid;
    char *srcName = info->gname.ptr;
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    x = TX(x0, y0);
    y = TY(x0, y0);
    rotateInfo->savedGlyphCB.move(&rotateInfo->savedGlyphCB, x, y);
}

static void rotate_line(abfGlyphCallbacks *cb, float x1, float y1) {
    float x, y;
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    x = TX(x1, y1);
    y = TY(x1, y1);
    rotateInfo->savedGlyphCB.line(&rotateInfo->savedGlyphCB, x, y);
}

static void rotate_curve(abfGlyphCallbacks *cb,
                         float x1, float y1,
                         float x2, float y2,
                         float x3, float y3) {
    float xn1, yn1, xn2, yn2, xn3, yn3;
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    float *farray = rotateInfo->curMatrix;
    float temp = edge0;
    float offset, scale;
    /* this function gets called only if the rotation matrix is some multiple of 90 degrees */
    if (rotateInfo->flags & ROTATE_0) /* 0 degrees, scaling and translation only*/
    {
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
    } else if (rotateInfo->flags & ROTATE_180) /* 180 degrees.*/
    {
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
    } else if (rotateInfo->flags & ROTATE_90) /* 90 degrees.*/
    {
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
    } else if (rotateInfo->flags & ROTATE_270) /* -90 degrees.*/
    {
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
        if (farray[1] == 0) /* X-scaling, Y shear and translation only. Can keep hhints. */
        {
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
            fatal(h, "Program error - glyph stem hint callback called when rotation is not a multiple of 90 degrees.\n");
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    rotateInfo->savedGlyphCB.genop(&rotateInfo->savedGlyphCB, cnt, args, op);
}

static void rotate_seac(abfGlyphCallbacks *cb,
                        float adx, float ady, int bchar, int achar) {
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    rotateInfo->savedGlyphCB.seac(&rotateInfo->savedGlyphCB, adx, ady, bchar, achar);
}

static void rotate_end(abfGlyphCallbacks *cb) {
    txCtx h = cb->indirect_ctx;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
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
        if ((rge->srcName[0] <= '9') && (rge->srcName[0] >= '0')) /* glyph names may not begin with a number; must be a CID */
        {
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

static void setupRotationCallbacks(txCtx h) {
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    rotateInfo->savedGlyphCB = h->cb.glyph;
    h->cb.glyph.indirect_ctx = h;
    h->cb.glyph.beg = rotate_beg;
    h->cb.glyph.width = rotate_width;
    h->cb.glyph.move = rotate_move;
    h->cb.glyph.line = rotate_line;
    h->cb.glyph.curve = rotate_curve;
    if ((rotateInfo->flags & ROTATE_KEEP_HINTS) || (rotateInfo->origMatrix[1] == 0)) /* rotation is some multiple of 90 degrees */
    {
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

static int setRotationMatrix(txCtx h, int argc, char **argv, int i, bool isMatrix) {
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;
    float *farray = rotateInfo->origMatrix;
    char *arg = argv[i];
    int j;
    /* arguments may be either "degrees xoffset yoffset" or  the full 6 valued font transform matrix */
    if (isMatrix) {
        if (argc <= i + 6)
            fatal(h, "Not enough arguments for  rotation matrix. Need 6 decimal values.\n", arg);
        for (j = 0; j < 6; j++) {
            arg = argv[i + j];
            farray[j] = (float)atof(arg);
            if ((arg[0] != '0') && (farray[j] == 0.0))
                fatal(h, "Bad argument for rotation matrix: %s. Must be a decimal number.\n", arg);
        }
        i += 5;
    } else {
        float degrees = 0;
        if (argc <= i + 3)
            fatal(h, "Not enough arguments for rotation matrix. Need three decimal values: degrees, and x,y offset.\n", arg);
        for (j = 0; j < 3; j++) {
            double val;
            arg = argv[i + j];
            val = atof(arg);
            if ((arg[0] != '0') && (val == 0.0))
                fatal(h, "Bad argument for rotation matrix: %s. Must be a decimal number.\n", arg);
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
        i += 2;
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

/* Process file. */
static void doFile(txCtx h, char *srcname) {
    long i;
    char *p;
    struct stat fileStat;
    int statErrNo;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;

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

        /* insert the rotation call-backs. */
        if (rotateInfo->flags & ROTATE_MATRIX_SET)
            setupRotationCallbacks(h);

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

/* Parse argument list. */
static void parseArgs(txCtx h, int argc, char *argv[]) {
    int i;
    char *arg;
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;

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
            case opt_row:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                break;
            case opt_rtw:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                rotateInfo->flags |= ROT_TRANSFORM_WIDTHS;
                break;
            case opt_rew:
                rotateInfo->flags |= ROT_OVERRIDE_WIDTH_FLAGS;
                rotateInfo->flags |= ROT_SET_WIDTH_EM;
                break;
            case opt_rt:
                if (rotateInfo->flags & ROTATE_MATRIX_SET)
                    fatal(h, "-rt option must precede -rtf option, and cannot be specified twice or with -matrix");
                i = setRotationMatrix(h, argc, argv, ++i, 0);
                break;
            case opt_rtf:
                if (!argsleft)
                    goto noarg;
                strcpy(rotateInfo->rtFile, argv[++i]);
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
                break;
            case opt_matrix:
                if (rotateInfo->flags & ROTATE_MATRIX_SET)
                    fatal(h, "-matrix option must precede -rtf option, and cannot be specified twice or with -rt");
                i = setRotationMatrix(h, argc, argv, ++i, 1);
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
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;

    h->progname = progname;
    h->flags = 0;
    rotateInfo->flags = 0;
    rotateInfo->rtFile[0] = 0;
    rotateInfo->curRGE = NULL;
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

    dnaINIT(h->ctx.dna, rotateInfo->rotateGlyphEntries, 256, 768);
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
    RotateInfo *rotateInfo = (RotateInfo *)h->appSpecificInfo;

    memFree(h, h->script.buf);
    dnaFREE(rotateInfo->rotateGlyphEntries);
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


    h->app = APP_ROTATEFONT;
    h->appSpecificInfo = malloc(sizeof(RotateInfo));
    if (h == NULL) {
        fprintf(stderr, "%s: out of memory\n", progname);
        exit(1);
    }
    memset(h->appSpecificInfo, 0, sizeof(RotateInfo));

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
