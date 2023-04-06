/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "ufowrite.h"

#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>

#include "dynarr.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"
#include "supportexcept.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define PLIST_DTD_HEADER "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"

/* ---------------------------- Library Context ---------------------------- */

typedef struct {  // Glyph data
    char glyphName[FILENAME_MAX];
    char glifFileName[FILENAME_MAX];
    int cid;
    int iFD;
} Glyph;

typedef enum {
    movetoType,
    linetoType,
    curvetoType,
    initialCurvetoType,
    finalCurvetoType,
    closepathType,
} OpType;

typedef struct {
    OpType opType;
    float coords[6]; /* Float matrix */
    const char *pointName;
} OpRec;

struct ufwCtx_ {
    int state;             /* 0 == writing to tmp; 1 == writing to dst */
    abfTopDict *top;       /* Top Dict data */
    dnaDCL(Glyph, glyphs); /* Glyph data */
    int lastiFD;           /* The index into the FD array of the last glyph seen. Used only when the source is a CID font.*/
    struct {               /* Client-specified data */
        long flags; /* See ufowrite.h for flags */
        const char *glyphLayer;
    } arg;
    struct {  // Destination stream
        char buf[BUFSIZ];
        size_t cnt; /* Bytes in buf */
    } dst;
    struct {  // Temporary stream
        char buf[BUFSIZ];
        size_t cnt; /* Bytes in buf */
    } tmp;
    struct {  // Glyph path
        float x;
        float y;
        int state;
        dnaDCL(OpRec, opList);
    } path;
    struct {  // Streams
        void *dst;
        void *tmp;
    } stm;
    struct {  // Client callbacks
        ctlMemoryCallbacks mem;
        ctlStreamCallbacks stm;
    } cb;
    dnaCtx dna; /* dynarr context */
    struct {    /* Error handling */
        _Exc_Buf env;
        int code;
    } err;
    std::shared_ptr<slogger> logger;
};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(ufwCtx h, int err_code) {
    h->logger->msg(sFATAL, ufwErrStr(err_code));
    h->err.code = err_code;
    RAISE(&h->err.env, err_code, NULL);
}

/* --------------------------- Destination Stream -------------------------- */

/* Flush dst/tmp stream buffer. */
static void flushBuf(ufwCtx h) {
    void *stm;
    char *buf;
    size_t *cnt;
    int err;
    if (h->state == 0) {
        stm = h->stm.tmp;
        buf = h->tmp.buf;
        cnt = &h->tmp.cnt;
        err = ufwErrTmpStream;
    } else {  // h->state == 1
        stm = h->stm.dst;
        buf = h->dst.buf;
        cnt = &h->dst.cnt;
        err = ufwErrDstStream;
    }

    if (*cnt == 0)
        return; /* Nothing to do */

    DURING_EX(h->err.env)

    /* Write buffered bytes */
    if (h->cb.stm.write(&h->cb.stm, stm, *cnt, buf) != *cnt)
        fatal(h, err);

    HANDLER
    END_HANDLER

    *cnt = 0;
}

/* Write to dst/tmp stream buffer. */
static void writeBuf(ufwCtx h, size_t writeCnt, const char *ptr) {
    char *buf;
    size_t *cnt;
    size_t left;
    if (h->state == 0) {
        buf = h->tmp.buf;
        cnt = &h->tmp.cnt;
    } else {  // h->state == 1
        buf = h->dst.buf;
        cnt = &h->dst.cnt;
    }

    left = BUFSIZ - *cnt; /* Bytes left in buffer */
    while (writeCnt >= left) {
        memcpy(&buf[*cnt], ptr, left);
        *cnt += left;
        flushBuf(h);
        ptr += left;
        writeCnt -= left;
        left = BUFSIZ;
    }
    if (writeCnt > 0) {
        memcpy(&buf[*cnt], ptr, writeCnt);
        *cnt += writeCnt;
    }
}

/* Write integer value as ASCII to dst stream. */
static void writeInt(ufwCtx h, long value) {
    char buf[50];
    snprintf(buf, sizeof(buf), "%ld", value);
    writeBuf(h, strlen(buf), buf);
}

/* Convert a long into a string */
static void ufw_ltoa(char *buf, long val) {
    static char ascii_digit[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    char *position = buf;
    char *start = buf;

    int isneg = val < 0;
    if (isneg)
        val = -val;

    /* extract char's */
    do {
        int digit;
        digit = val % 10;
        val /= 10;
        *position = ascii_digit[digit];
        position++;
    } while (val);

    /* store sign */
    if (isneg) {
        *position = '-';
        position++;
    }

    /* terminate */
    *position = '\0';
    position--;

    /* reverse string */
    while (start < position) {
        char tmp;
        tmp = *position;
        *position = *start;
        *start = tmp;
        position--;
        start++;
    }
}

/* Write real number in ASCII to dst stream. */
#define TX_EPSILON 0.0003
/*In Xcode, FLT_EPSILON is 1.192..x10-7, but the diff between value-roundf(value) can be 3.05..x10-5, when the input value was a 24.8 Fixed. */
static void writeReal(ufwCtx h, float value) {
    char buf[50];
    /* if no decimal component, perform a faster to string conversion */
    if ((fabs(value - roundf(value)) < TX_EPSILON) && value > LONG_MIN && value < LONG_MAX)
        ufw_ltoa(buf, (long)roundf(value));
    else
        ctuDtostr(buf, sizeof(buf), value, 0, 2);
    writeBuf(h, strlen(buf), buf);
}

/* Write null-terminated string to dst steam. */
static void writeStr(ufwCtx h, const char *s) {
    writeBuf(h, strlen(s), s);
}

/* Write null-terminated string followed by newline. */
static void writeLine(ufwCtx h, const char *s) {
    writeStr(h, s);
    writeStr(h, "\n");
}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
ufwCtx ufwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL, std::shared_ptr<slogger> logger) {
    ufwCtx h;

    /* Check client/library compatibility */
    if (CTL_CHECK_ARGS_TEST(UFW_VERSION))
        return NULL;

    /* Allocate context */
    h = (ufwCtx) mem_cb->manage(mem_cb, NULL, sizeof(struct ufwCtx_));
    if (h == NULL)
        return NULL;

    /* Safety initialization */
    h->state = 0;
    h->top = NULL;
    h->glyphs.size = 0;
    h->path.opList.size = 0;

    h->dna = NULL;
    h->stm.dst = NULL;
    h->err.code = ufwSuccess;
    h->lastiFD = ABF_UNSET_INT;

    /* Copy callbacks */
    h->cb.mem = *mem_cb;
    h->cb.stm = *stm_cb;

    if (logger == nullptr)
        h->logger = slogger::getLogger("ufowrite");
    else
        h->logger = logger;

    /* Initialize service library */
    h->dna = dnaNew(&h->cb.mem, DNA_CHECK_ARGS);
    if (h->dna == NULL)
        goto cleanup;

    h->arg.glyphLayer = "glyphs";

    dnaINIT(h->dna, h->glyphs, 256, 750);
    dnaINIT(h->dna, h->path.opList, 256, 750);

    return h;

cleanup:
    /* Initialization failed */
    ufwFree(h);
    return NULL;
}

/* Free context. */
void ufwFree(ufwCtx h) {
    if (h == NULL)
        return;

    dnaFREE(h->glyphs);
    dnaFREE(h->path.opList);
    dnaFree(h->dna);

    // Needed until we're sure context isn't malloc-ed
    h->logger = nullptr;

    /* Free library context */
    h->cb.mem.manage(&h->cb.mem, h, 0);

    return;
}

/* Begin font. */
int ufwBegFont(ufwCtx h, long flags, const char *glyphLayerDir) {
    /* Initialize */
    h->arg.flags = flags;
    h->tmp.cnt = 0;
    h->dst.cnt = 0;
    h->glyphs.cnt = 0;
    h->path.opList.cnt = 0;
    h->path.state = 0;
    h->top = NULL;
    h->stm.dst = NULL;
    h->state = 1; /* Indicates writing to dst stream */

    if (glyphLayerDir != NULL)
        h->arg.glyphLayer = glyphLayerDir;

    /* Set error handler */
    DURING_EX(h->err.env)
    HANDLER
    END_HANDLER

    return ufwSuccess;
}

static void orderNameKeyedGlyphs(ufwCtx h) {
    abfTopDict *top = h->top;
    long i;
    long j;
    long nGlyphs = h->glyphs.cnt;
    Glyph *glyphs = h->glyphs.array;

    for (i = 0; i < nGlyphs; i++) {
        for (j = i + 1; j < nGlyphs; j++) {
        Glyph a = glyphs[i];
        Glyph b = glyphs[j];
        if (strcmp(a.glyphName, b.glyphName) > 0) {
            Glyph temp = a;
            glyphs[i] = glyphs[j];
            glyphs[j] = temp;
            }
        }
    }
}

static void orderCIDKeyedGlyphs(ufwCtx h) {
    long i;
    long nGlyphs = h->glyphs.cnt;
    Glyph *glyphs = h->glyphs.array;

    /* Insertion sort glyphs by CID order */
    for (i = 2; i < nGlyphs; i++) {
        long j = i;
        Glyph tmp = glyphs[i];
        while (tmp.cid < glyphs[j - 1].cid) {
            glyphs[j] = glyphs[j - 1];
            j--;
        }
        if (j != i) {
            glyphs[j] = tmp;
        }
    }
}

static void removeUnusedFDicts(ufwCtx h) {
    long i;
    long nGlyphs = h->glyphs.cnt;
    Glyph *glyphs = h->glyphs.array;
    abfTopDict *top = h->top;
    dnaDCL(uint16_t, fdmap);
    dnaINIT(h->dna, fdmap, 1, 1);
    dnaSET_CNT(fdmap, top->FDArray.cnt);
    memset(fdmap.array, 0, sizeof(uint16_t) * top->FDArray.cnt);

    /* Mark used FDs */
    for (i = 0; i < nGlyphs; i++) {
        fdmap.array[glyphs[i].iFD] = 1;
    }

    /* Remove unused FDs from FDArray */
    {
        long j = 0;
        for (i = 0; i < top->FDArray.cnt; i++) {
            if (fdmap.array[i]) {
                if (i != j) {
                    /* Swap elements preserving dynamic arrays for later freeing */
                    abfFontDict tmp = top->FDArray.array[j];
                    top->FDArray.array[j] = top->FDArray.array[i];
                    top->FDArray.array[i] = tmp;
                }
                fdmap.array[i] = (uint16_t)j++;
            }
        }

        if (i != j) {
            /* Unused FDs; remap glyphs to new FDArray */
            for (i = 0; i < nGlyphs; i++) {
                glyphs[i].iFD = fdmap.array[glyphs[i].iFD];
            }
            top->FDArray.cnt = j;
        }
    }
    dnaFREE(fdmap);
}

static void writeContents(ufwCtx h) {
    char buffer[FILENAME_MAX];
    int i;
    /* Set error handler */
    DURING_EX(h->err.env)

    h->state = 1; /* Indicates writing to dst stream */

    /* Open contents.plist file as dst stream */

    snprintf(buffer, sizeof(buffer), "%s/%s",
             h->arg.glyphLayer, "contents.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);

    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    for (i = 0; i < h->glyphs.cnt; i++) {
        Glyph *glyphRec;
        glyphRec = &h->glyphs.array[i];
        snprintf(buffer, sizeof(buffer), "\t<key>%s</key>",
                 glyphRec->glyphName);
        writeLine(h, buffer);
        snprintf(buffer, sizeof(buffer), "\t<string>%s</string>",
                 glyphRec->glifFileName);
        writeLine(h, buffer);
    }

    writeLine(h, "</dict>");
    writeLine(h, "</plist>");

    /* Close dst stream */
    flushBuf(h);
    h->cb.stm.close(&h->cb.stm, h->stm.dst);

    HANDLER

    if (h->stm.dst)
        h->cb.stm.close(&h->cb.stm, h->stm.dst);

    END_HANDLER

    return;
}

static void writeBlueValues(ufwCtx h, abfPrivateDict *privateDict) {
    char buffer[FILENAME_MAX];
    int i;

    /* All the Blue values */
    if (privateDict->BlueValues.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptBlueValues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->BlueValues.cnt; i++) {
            float stem = privateDict->BlueValues.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->OtherBlues.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptOtherBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->OtherBlues.cnt; i++) {
            float stem = privateDict->OtherBlues.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->FamilyBlues.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptFamilyBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->FamilyBlues.cnt; i++) {
            float stem = privateDict->FamilyBlues.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->FamilyOtherBlues.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptFamilyOtherBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->FamilyOtherBlues.cnt; i++) {
            float stem = privateDict->FamilyOtherBlues.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->StemSnapH.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptStemSnapH</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->StemSnapH.cnt; i++) {
            float stem = privateDict->StemSnapH.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->StdHW != ABF_UNSET_REAL) {
        float stem = privateDict->StdHW;
        writeLine(h, "\t<key>postscriptStdHW</key>");
        writeLine(h, "\t<array>");
        if (stem == ((int)stem))
            snprintf(buffer, sizeof(buffer),
                     "\t\t<integer>%d</integer>", (int)stem);
        else
            snprintf(buffer, sizeof(buffer),
                     "\t\t<real>%.2f</real>", stem);
        writeLine(h, buffer);
        writeLine(h, "\t</array>");
    }

    if (privateDict->StemSnapV.cnt != ABF_EMPTY_ARRAY) {
        writeLine(h, "\t<key>postscriptStemSnapV</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->StemSnapV.cnt; i++) {
            float stem = privateDict->StemSnapV.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, sizeof(buffer),
                         "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, sizeof(buffer),
                         "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }

    if (privateDict->StdVW != ABF_UNSET_REAL) {
        float stem = privateDict->StdVW;
        writeLine(h, "\t<key>postscriptStdVW</key>");
        writeLine(h, "\t<array>");
        if (stem == ((int)stem))
            snprintf(buffer, sizeof(buffer),
                     "\t\t<integer>%d</integer>", (int)stem);
        else
            snprintf(buffer, sizeof(buffer),
                     "\t\t<real>%.2f</real>", stem);
        writeLine(h, buffer);
        writeLine(h, "\t</array>");
    }

    if (privateDict->BlueScale != ABF_UNSET_REAL) {
        char *p;
        char valueBuffer[50];
        /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/
        snprintf(valueBuffer, sizeof(valueBuffer),
                 "%.8f", privateDict->BlueScale);
        p = strchr(valueBuffer, '.');
        if (p != NULL) {
            /* Have decimal point. Remove trailing zeros.*/
            size_t l = strlen(p);
            p += l - 1;
            while (*p == '0') {
                *p = '\0';
                p--;
            }
            if (*p == '.') {
                *p = '\0';
            }
        }

        writeLine(h, "\t<key>postscriptBlueScale</key>");
        snprintf(buffer, sizeof(buffer), "\t<real>%s</real>", valueBuffer);
        writeLine(h, buffer);
    }

    if (privateDict->BlueShift != ABF_UNSET_REAL) {
        writeLine(h, "\t<key>postscriptBlueShift</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>", (int)(0.5 + privateDict->BlueShift));
        writeLine(h, buffer);
    }

    if (privateDict->BlueFuzz != ABF_UNSET_REAL) {
        writeLine(h, "\t<key>postscriptBlueFuzz</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>", (int)(0.5 + privateDict->BlueFuzz));
        writeLine(h, buffer);
    }

    if ((privateDict->ForceBold != ABF_UNSET_INT) && (privateDict->ForceBold > 0)) {
        writeLine(h, "\t<key>postscriptForceBold</key>");
        writeLine(h, "\t<true/>");
    }

    if (privateDict->LanguageGroup != cff_DFLT_LanguageGroup) {
        writeLine(h, "\t<key>LanguageGroup</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>", (int)privateDict->LanguageGroup);
        writeLine(h, buffer);
    }

    if (privateDict->ExpansionFactor != cff_DFLT_ExpansionFactor) {
        writeLine(h, "\t<key>ExpansionFactor</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<real>%.2f</real>", privateDict->ExpansionFactor);
        writeLine(h, buffer);
    }
}

static void writeFDArray(ufwCtx h, abfTopDict *top, char *buffer, int buflen) {
    abfPrivateDict *privateDict;
    int i;
    int j;
    writeLine(h, "\t<key>com.adobe.type.postscriptFDArray</key>");
    writeLine(h, "\t<array>");
    for (j = 0; j < top->FDArray.cnt; j++) {
        writeLine(h, "\t<dict>");
        abfFontDict *fd = &h->top->FDArray.array[j];
        writeLine(h, "\t<key>FontName</key>");
        snprintf(buffer, buflen, "\t<string>%s</string>", fd->FontName.ptr);
        writeLine(h, buffer);
        writeLine(h, "\t<key>PaintType</key>");
        snprintf(buffer, buflen, "\t<integer>%ld</integer>", fd->PaintType);
        writeLine(h, buffer);
        if (fd->FontMatrix.cnt == ABF_EMPTY_ARRAY) {
            fd->FontMatrix.cnt = 6;
            fd->FontMatrix.array[0] = 0.001;
            fd->FontMatrix.array[1] = 0.0;
            fd->FontMatrix.array[2] = 0.0;
            fd->FontMatrix.array[3] = 0.001;
            fd->FontMatrix.array[4] = 0.0;
            fd->FontMatrix.array[5] = 0.0;
        }
        writeLine(h, "\t<key>FontMatrix</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < fd->FontMatrix.cnt; i++) {
            float stem = fd->FontMatrix.array[i];
            if (stem == ((int)stem))
                snprintf(buffer, buflen, "\t\t<integer>%d</integer>", (int)stem);
            else
                snprintf(buffer, buflen, "\t\t<real>%.3f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");

        privateDict = &(fd->Private);
        writeLine(h, "\t<key>PrivateDict</key>");
        writeLine(h, "\t<dict>");
        writeBlueValues(h, privateDict);
        writeLine(h, "\t</dict>");
        writeLine(h, "\t</dict>");
    }
    writeLine(h, "\t</array>");
}

static void writeCIDMap(ufwCtx h, abfTopDict *top, char *buffer, int buflen) {
    int i;
    writeLine(h, "\t<key>com.adobe.type.postscriptCIDMap</key>");
    writeLine(h, "\t<dict>");
    for (i = 0; i < h->glyphs.cnt; i++) {
        snprintf(buffer, buflen, "\t\t<key>%s</key>", h->glyphs.array[i].glyphName);
        writeLine(h, buffer);
        snprintf(buffer, buflen, "\t\t<integer>%d</integer>", h->glyphs.array[i].cid);
        writeLine(h, buffer);
    }
    writeLine(h, "\t</dict>");
}

static void writeLibPlist(ufwCtx h) {
    char buffer[FILENAME_MAX];
    int buflen = sizeof(buffer);
    int i;

    /* Set error handler */
    DURING_EX(h->err.env)

    h->state = 1; /* Indicates writing to dst stream */

    snprintf(buffer, buflen, "%s", "lib.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);

    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");

    if (h->top->sup.flags & ABF_CID_FONT) {
        if (h->top->cid.CIDFontName.ptr != NULL) {
            writeLine(h, "\t<key>com.adobe.type.CIDFontName</key>");
            snprintf(buffer, buflen, "\t<string>%s</string>", h->top->cid.CIDFontName.ptr);
            writeLine(h, buffer);
            writeLine(h, "\t<key>com.adobe.type.FSType</key>");
            snprintf(buffer, buflen, "\t<integer>%d</integer>", (int)h->top->FSType);
            writeLine(h, buffer);
            writeLine(h, "\t<key>com.adobe.type.ROS</key>");
            snprintf(buffer, buflen, "\t<string>%s-%s-%ld</string>", h->top->cid.Registry.ptr, h->top->cid.Ordering.ptr, h->top->cid.Supplement);
            writeLine(h, buffer);
        }
        // CIDMap needs the dict keys in alphabetical order,
        // but we want the font in CID order
        orderNameKeyedGlyphs(h);
        writeCIDMap(h, h->top, buffer, buflen);
        orderCIDKeyedGlyphs(h);
        removeUnusedFDicts(h);
        writeFDArray(h, h->top, buffer, buflen);
    }
    writeLine(h, "\t<key>public.glyphOrder</key>");
    writeLine(h, "\t<array>");
    for (i = 0; i < h->glyphs.cnt; i++) {
        Glyph *glyphRec;
        glyphRec = &h->glyphs.array[i];
        snprintf(buffer, buflen, "\t\t<string>%s</string>", glyphRec->glyphName);
        writeLine(h, buffer);
    }
    writeLine(h, "\t</array>");

    writeLine(h, "</dict>");
    writeLine(h, "</plist>");

    /* Close dst stream */
    flushBuf(h);
    h->cb.stm.close(&h->cb.stm, h->stm.dst);

    HANDLER
    if (h->stm.dst)
        h->cb.stm.close(&h->cb.stm, h->stm.dst);

    END_HANDLER
}

static void writeFDArraySelect(ufwCtx h, abfTopDict *top, char *buffer,
                               int buflen) {
    int fdIndex;
    int glyphArrIndex;

    for (fdIndex = 0; fdIndex < top->FDArray.cnt; fdIndex++) {
        abfFontDict *fd = &h->top->FDArray.array[fdIndex];
        if (fd->FontName.ptr != NULL)
            snprintf(buffer, buflen, "\t<key>FDArraySelect.%d.%s</key>", fdIndex, fd->FontName.ptr);
        else
            snprintf(buffer, buflen, "\t<key>FDArraySelect.%d</key>", fdIndex);
        writeLine(h, buffer);
        writeLine(h, "\t<array>");
        for (glyphArrIndex = 0; glyphArrIndex < h->glyphs.cnt; glyphArrIndex++) {
            if (h->glyphs.array[glyphArrIndex].iFD == fdIndex) {
                snprintf(buffer, buflen, "\t\t<string>%s</string>", h->glyphs.array[glyphArrIndex].glyphName);
                writeLine(h, buffer);
            }
        }
        writeLine(h, "\t</array>");
    }
}

static void writeGroups(ufwCtx h, abfTopDict *top) {
    char buffer[FILENAME_MAX];
    int buflen = sizeof(buffer);

    /* Set error handler */
    DURING_EX(h->err.env)

    h->state = 1; /* Indicates writing to dst stream */

    /* Open groups.plist file as dst stream */

    snprintf(buffer, buflen, "%s", "groups.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);

    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    writeFDArraySelect(h, top, buffer, sizeof(buffer));
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");

    /* Close dst stream */
    flushBuf(h);
    h->cb.stm.close(&h->cb.stm, h->stm.dst);

    HANDLER
    if (h->stm.dst)
        h->cb.stm.close(&h->cb.stm, h->stm.dst);
    END_HANDLER
}

static void writeMetaInfo(ufwCtx h) {
    char buffer[FILENAME_MAX];
    int buflen = sizeof(buffer);

    /* Set error handler */
    DURING_EX(h->err.env)

    h->state = 1; /* Indicates writing to dst stream */

    /* Open metainfo.plist file as dst stream */

    snprintf(buffer, buflen, "%s", "metainfo.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);

    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    writeLine(h, "\t<key>creator</key>");
    writeLine(h, "\t<string>com.adobe.type.tx</string>");
    writeLine(h, "\t<key>formatVersion</key>");
    writeLine(h, "\t<integer>2</integer>");
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");

    /* Close dst stream */
    flushBuf(h);
    h->cb.stm.close(&h->cb.stm, h->stm.dst);

    HANDLER
    if (h->stm.dst)
        h->cb.stm.close(&h->cb.stm, h->stm.dst);
    END_HANDLER
}

static void setStyleName(char *dst, int buflen, const char *postScriptName) {
    /* Copy text after '-'; if none, return empty string.*/
    const char *p = &postScriptName[0];
    while ((*p != '-') && (*p != 0x00))
        p++;

    if (*p == 0x00) {
        *dst = 0;
        return;
    }
    p++;
    STRCPY_S(dst, buflen, p);
}

static void setVersionMajor(char *dst, const char *version) {
    /* Copy text up to '.' Skip leading zeros.*/
    int seenNonZero = 0;
    const char *p = &version[0];
    while ((*p != '.') && (*p != 0x00)) {
        if ((!seenNonZero) && (*p == '0')) {
            p++;
        } else {
            *dst++ = *p++;
            seenNonZero = 1;
        }
    }
    *dst = 0x00;
}

static void setVersionMinor(char *dst, const char *version) {
    /* Copy text after '.'; if none, return '0'.*/
    int seenNonZero = 0;
    const char *p = &version[0];
    while ((*p != '.') && (*p != 0x00))
        p++;

    if (*p == 0x00) {
        dst[0] = '0';
        dst[1] = 0x00;
        return;
    }
    p++; /* skip the period */
    while (*p != 0x00) {
        if ((!seenNonZero) && (*p == '0')) {
            p++;
        } else {
            *dst++ = *p++;
            seenNonZero = 1;
        }
    }
    if (!seenNonZero)
        *dst++ = '0';
    *dst = 0x00;
}

static int writeFontInfo(ufwCtx h, abfTopDict *top) {
    char buffer[FILENAME_MAX];
    char buffer2[FILENAME_MAX];
    abfFontDict *fontDict0;
    abfPrivateDict *privateDict;

    if (h->lastiFD != ABF_UNSET_INT)
        fontDict0 = &(top->FDArray.array[h->lastiFD]);
    else
        fontDict0 = &(top->FDArray.array[0]);

    /* Set error handler */
    DURING_EX(h->err.env)

    h->state = 1; /* Indicates writing to dst stream */

    /* Open fontinfo.plist file as dst stream */

    snprintf(buffer, sizeof(buffer), "%s", "fontinfo.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);

    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");

    /* This is what I care about the most. Add the rest in the order of the
     UFO 3 spec. */
    if (top->sup.flags & ABF_CID_FONT) {
        if (top->cid.CIDFontName.ptr != NULL) {
            writeLine(h, "\t<key>postscriptFontName</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<string>%s</string>", top->cid.CIDFontName.ptr);
            writeLine(h, buffer);
            setStyleName(buffer2, sizeof(buffer2), top->cid.CIDFontName.ptr);
            writeLine(h, "\t<key>styleName</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<string>%s</string>", buffer2);
            writeLine(h, buffer);
        }
    } else if (fontDict0->FontName.ptr != NULL) {
        writeLine(h, "\t<key>postscriptFontName</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", fontDict0->FontName.ptr);
        writeLine(h, buffer);
        setStyleName(buffer2, sizeof(buffer2), fontDict0->FontName.ptr);
        writeLine(h, "\t<key>styleName</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", buffer2);
        writeLine(h, buffer);
    }

    if (top->FamilyName.ptr != NULL) {
        writeLine(h, "\t<key>familyName</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", top->FamilyName.ptr);
        writeLine(h, buffer);
    }

    if (top->sup.flags & ABF_CID_FONT) {
        if (top->cid.CIDFontVersion != 0) {
            char versionStr[32];
            snprintf(versionStr, sizeof(versionStr),
                     "%.3f", top->cid.CIDFontVersion);
            setVersionMajor(buffer2, versionStr);
            writeLine(h, "\t<key>versionMajor</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
            setVersionMinor(buffer2, versionStr);
            writeLine(h, "\t<key>versionMinor</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
        }
    } else {
        if (top->version.ptr != NULL) {
            setVersionMajor(buffer2, top->version.ptr);
            writeLine(h, "\t<key>versionMajor</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
            setVersionMinor(buffer2, top->version.ptr);
            writeLine(h, "\t<key>versionMinor</key>");
            snprintf(buffer, sizeof(buffer),
                     "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
        }
    }

    if (top->Copyright.ptr != NULL) {
        writeLine(h, "\t<key>copyright</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", top->Copyright.ptr);
        writeLine(h, buffer);
    }

    if (top->Notice.ptr != NULL) {
        writeLine(h, "\t<key>trademark</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", top->Notice.ptr);
        writeLine(h, buffer);
    }

    writeLine(h, "\t<key>unitsPerEm</key>");
    if (fontDict0->FontMatrix.cnt > 0) {
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>",
                (int)round(1.0 / fontDict0->FontMatrix.array[0]));
        writeLine(h, buffer);

    } else {
        writeLine(h, "\t<integer>1000</integer>");
    }

    if (top->ItalicAngle != cff_DFLT_ItalicAngle) {
        writeLine(h, "\t<key>italicAngle</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<real>%.8f</real>", top->ItalicAngle);
        writeLine(h, buffer);
    }

    /* Now for all the other PostScript-specific UFO data */
    if (top->FullName.ptr != NULL) {
        writeLine(h, "\t<key>postscriptFullName</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", top->FullName.ptr);
        writeLine(h, buffer);
    }

    if (top->Weight.ptr != NULL) {
        writeLine(h, "\t<key>postscriptWeightName</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<string>%s</string>", top->Weight.ptr);
        writeLine(h, buffer);
    }

    if (top->UnderlinePosition != cff_DFLT_UnderlinePosition) {
        writeLine(h, "\t<key>postscriptUnderlinePosition</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>", (int)floor(0.5 + top->UnderlinePosition));
        writeLine(h, buffer);
    }

    if (top->UnderlineThickness != cff_DFLT_UnderlineThickness) {
        writeLine(h, "\t<key>postscriptUnderlineThickness</key>");
        snprintf(buffer, sizeof(buffer),
                 "\t<integer>%d</integer>", (int)floor(0.5 + top->UnderlineThickness));
        writeLine(h, buffer);
    }

    if ((top->isFixedPitch != cff_DFLT_isFixedPitch) && (top->isFixedPitch > 0)) {
        writeLine(h, "\t<key>postscriptIsFixedPitch</key>");
        writeLine(h, "\t<true/>");
    }

    if (top->sup.flags != ABF_CID_FONT) {
        privateDict = &(fontDict0->Private);
        writeBlueValues(h, privateDict);
    }

    /* Finish the file */
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");
    flushBuf(h);

    /* Close dst stream */
    if (h->cb.stm.close(&h->cb.stm, h->stm.dst) == -1)
        return ufwErrDstStream;

    HANDLER
    if (h->stm.dst)
        h->cb.stm.close(&h->cb.stm, h->stm.dst);
    return Exception.Code;

    END_HANDLER

    return ufwSuccess;
}

/* Finish reading font. */
int ufwEndFont(ufwCtx h, abfTopDict *top) {
    int errCode = ufwSuccess;

    /* Check for errors when accumulating glyphs */
    if (h->err.code != 0)
        return h->err.code;

    h->top = top;
    errCode = writeFontInfo(h, top);
    if (errCode != ufwSuccess)
        return errCode;

    writeContents(h);
    writeLibPlist(h);
    if (h->top->sup.flags & ABF_CID_FONT || top->FDArray.cnt > 1)  // for now, only write if CID font. Revisit in next PR adding multi-fddict support to non-cidkeyed fonts.
        writeGroups(h, top);
    writeMetaInfo(h);
    h->state = 0; /* Indicates writing to temp stream */
    return ufwSuccess;
}

/* Get version numbers of libraries. */
void ufwGetVersion(ctlVersionCallbacks *cb) {
    if (cb->called & 1 << UFW_LIB_ID)
        return; /* Already enumerated */

    /* Support libraries */
    dnaGetVersion(cb);

    /* This library */
    cb->getversion(cb, UFW_VERSION, "ufowrite");

    /* Record this call */
    cb->called |= 1 << UFW_LIB_ID;
}

/* Map error code to error string. */
const char *ufwErrStr(int err_code) {
    static const char *errstrs[] = {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string) string,
#include "ufowerr.h"
    };
    return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs)) ? "unknown error" : errstrs[err_code];
}

/* ------------------------------ Glyph Path  ------------------------------ */

static void mapGlyphToGLIFName(char *glyphName, char *glifName) {
    char *p = &glyphName[0];
    char *q = &glifName[0];

    if (*p == '.') {
        *q++ = '_';
        p++;
    }

    while (*p != 0x00) {
        if ((q - &glifName[0]) >= MAX_UFO_GLYPH_NAME) {
            glifName[MAX_UFO_GLYPH_NAME - 6] = 0; /* allow for extension */
            printf("Warning! glif name '%s' is longer than the glyph name buffer size.\n", glifName);
            break;
        }

        if ((*p >= 'A') && (*p <= 'Z')) {
            *q++ = *p++;
            *q++ = '_';

        } else if ((*p >= 0x00) && (*p <= 0x1F)) {
            p++;
            *q++ = '_';

        } else {
            char code = *p;
            switch (code) {
                case '(':
                case ')':
                case '*':
                case '+':
                case '/':
                case ':':
                case '<':
                case '>':
                case '?':
                case '[':
                case '\\':
                case ']':
                case 0x7F:
                    p++;
                    *q++ = '_';
                    break;
                default:
                    *q++ = *p++;
                    break;
            }
        }
    }
    *q = 0x00;
    STRCAT_S(glifName, MAX_UFO_GLYPH_NAME, ".glif");
}

/* Begin new glyph definition. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
    ufwCtx h = (ufwCtx) cb->direct_ctx;
    char glyphName[MAX_UFO_GLYPH_NAME];
    char glifName[MAX_UFO_GLYPH_NAME];
    char glifRelPath[FILENAME_MAX];
    Glyph *glyphRec;

    cb->info = info;

    if (h->err.code != 0)
        return ABF_FAIL_RET; /* Pending error */
    else if (h->path.state != 0) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return ABF_FAIL_RET;
    } else if (info->flags & ABF_GLYPH_SEEN)
        return ABF_SKIP_RET; /* Ignore duplicate glyph */

    if (info->flags & ABF_GLYPH_CID) {
        h->lastiFD = info->iFD;
        if (info->gname.ptr == NULL) {
            snprintf(glyphName, sizeof(glyphName), "cid%05hu", info->cid);
        } else {
            snprintf(glyphName, sizeof(glyphName), "%s", info->gname.ptr);
        }
    } else {
        snprintf(glyphName, sizeof(glyphName), "%s", info->gname.ptr);
        snprintf(glifName, sizeof(glifName), "%s", info->gname.ptr);
    }

    mapGlyphToGLIFName(glyphName, glifName);

    /* Initialize */
    h->path.x = 0;
    h->path.y = 0;
    h->path.state = 1;

    DURING_EX(h->err.env)

    /* Open GLIF file as dst stream */
    snprintf(glifRelPath, sizeof(glifRelPath), "%s/%s",
             h->arg.glyphLayer, glifName);
    h->cb.stm.clientFileName = glifRelPath;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    // write start of GLIF file and open glyph element.
    writeLine(h, XML_HEADER);
    writeStr(h, "<glyph");

    // write glyph name
    writeStr(h, " name=\"");
    writeStr(h, glyphName);
    writeStr(h, "\"");

    // close glyph element
    writeLine(h, " format=\"1\" >");

    // write encoding, if present.

    if (info->flags & ABF_GLYPH_UNICODE) {
        char buf[9];
        writeStr(h, "\t<unicode hex=\"");
        snprintf(buf, sizeof(buf), "%06lX", info->encoding.code);
        writeStr(h, buf);
        writeLine(h, "\"/>");
    }
    glyphRec = dnaNEXT(h->glyphs);
    STRCPY_S(glyphRec->glyphName, sizeof(glyphRec->glyphName), glyphName);
    STRCPY_S(glyphRec->glifFileName, sizeof(glyphRec->glifFileName), glifName);
    glyphRec->cid = info->cid;
    glyphRec->iFD = info->iFD;

    HANDLER

    return Exception.Code;

    END_HANDLER

    return ABF_CONT_RET;
}

/* Add horizontal advance width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv) {
    ufwCtx h = (ufwCtx) cb->direct_ctx;

    if (h->err.code != 0)
        return; /* Pending error */
    else if (h->path.state != 1) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    writeStr(h, "\t<advance width=\"");
    writeInt(h, (long)roundf(hAdv));
    writeLine(h, "\"/>");

    h->path.state = 2;
}

static void writeGlyphMove(ufwCtx h, float x0, float y0) {
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, x0);
    writeStr(h, "\" y=\"");
    writeReal(h, y0);
    writeLine(h, "\" type=\"move\"/>");
}

static void writeGlyphLine(ufwCtx h, float x0, float y0) {
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, x0);
    writeStr(h, "\" y=\"");
    writeReal(h, y0);
    writeLine(h, "\" type=\"line\"/>");
}

static void writeGlyphCurve(ufwCtx h, float *coords) {
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" />");

    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[2]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[3]);
    writeLine(h, "\" />");

    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[4]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[5]);
    writeLine(h, "\" type=\"curve\"/>");
}

static void writeGlyphInitialCurve(ufwCtx h, float *coords) {
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" type=\"curve\"/>");
}

static void writeGlyphFinalCurve(ufwCtx h, float *coords) {
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" />");

    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[2]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[3]);
    writeLine(h, "\" />");
}

bool pointsAreNearlyIdentical(float x1, float y1, float x2, float y2) {
    float precision = 0.015;
    return (fabs(x1 - x2) <= precision) && (fabs(y1 - y2) <= precision);
}

static void writeContour(ufwCtx h) {
    OpRec *opRec;
    OpRec *opRec2;
    float *lastCoords;

    int i;

    if (h->path.opList.cnt < 2) {
        h->path.opList.cnt = 0;
        return; /* Don't write paths with only a single move-to. UFO fonts can make these. */
    }

    /* Fix up the start op. UFO fonts require a completely closed path, and no initial move-to.
    If the last op coords are not the same as the move-to:
      - change the initial move-to to a line-to.
    else if the last op is a curve-to:
       - change the initial move-to type to a "initial curve-to" type - just writes the final curve point
       - change the final curve-to type to a "final curve-to" type - just writes the first two points of the curve.
    else if the last op is a line-to:
        - Change the initial move-to op to a line-to
        - Note:
            - Previously, the last pt would also be removed if it was a duplicate point
            - However, this code was removed because it interferes with duplicate points needed by variable fonts.
            - The last point is not removed anymore, even if its a duplicate.
     */
    opRec = &h->path.opList.array[0];
    opRec2 = &h->path.opList.array[h->path.opList.cnt - 1];
    if (opRec2->opType == curvetoType) {
        lastCoords = &(opRec2->coords[4]);
    } else {
        lastCoords = opRec2->coords;
    }

    if (!pointsAreNearlyIdentical(lastCoords[0], lastCoords[1], opRec->coords[0], opRec->coords[1])) {
        opRec->opType = linetoType;
    } else if (opRec2->opType == linetoType) {
        opRec->opType = linetoType;
    } else if (opRec2->opType == curvetoType) {
        opRec->opType = initialCurvetoType;
        opRec2->opType = finalCurvetoType;
    }

    writeStr(h, "\t\t<contour>\n");
    for (i = 0; i < h->path.opList.cnt; i++) {
        opRec = &h->path.opList.array[i];
        switch (opRec->opType) {
            case movetoType:
                writeGlyphMove(h, opRec->coords[0], opRec->coords[1]);
                break;
            case linetoType:
                writeGlyphLine(h, opRec->coords[0], opRec->coords[1]);
                break;
            case curvetoType:
                writeGlyphCurve(h, opRec->coords);
                break;
            case initialCurvetoType:
                writeGlyphInitialCurve(h, opRec->coords);
                break;
            case finalCurvetoType:
                writeGlyphFinalCurve(h, opRec->coords);
                break;
            default:
                break;
        }
    }
    writeStr(h, "\t\t</contour>\n");
    h->path.opList.cnt = 0;
}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0) {
    OpRec *opRec;
    ufwCtx h = (ufwCtx) cb->direct_ctx;
    h->path.x = x0;
    h->path.y = y0;

    if (h->err.code != 0)
        return; /* Pending error */
    else if (h->path.state < 2) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    if (h->path.state == 3) {
        /* Write previous path. */
        writeContour(h);
    } else if (h->path.state < 3) {
        /* First path data seen in glyph */
        writeLine(h, "\t<outline>");
    } else if (h->path.state > 3) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }
    opRec = dnaNEXT(h->path.opList);
    opRec->opType = movetoType;
    opRec->coords[0] = x0;
    opRec->coords[1] = y0;

    h->path.state = 3;
}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1) {
    OpRec *opRec;
    ufwCtx h = (ufwCtx) cb->direct_ctx;
    h->path.x = x1;
    h->path.y = y1;

    if (h->err.code != 0)
        return; /* Pending error */
    else if (h->path.state != 3) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    opRec = dnaNEXT(h->path.opList);
    opRec->opType = linetoType;
    opRec->coords[0] = x1;
    opRec->coords[1] = y1;
}

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
                       float x1, float y1,
                       float x2, float y2,
                       float x3, float y3) {
    OpRec *opRec;
    ufwCtx h = (ufwCtx) cb->direct_ctx;
    h->path.x = x3;
    h->path.y = y3;

    if (h->err.code != 0)
        return; /* Pending error */
    else if (h->path.state != 3) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    opRec = dnaNEXT(h->path.opList);
    opRec->opType = curvetoType;
    opRec->coords[0] = x1;
    opRec->coords[1] = y1;
    opRec->coords[2] = x2;
    opRec->coords[3] = y2;
    opRec->coords[4] = x3;
    opRec->coords[5] = y3;
}

/* Generic operator callback; ignored. */
static void glyphGenop(abfGlyphCallbacks *cb, int cnt, float *args, int op) {
    ufwCtx h = (ufwCtx) cb->direct_ctx;

    if (h->err.code != 0)
        return; /* Pending error */
    else if (h->path.state < 2) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    /* Do nothing; ignore generic operator. */
}

/* Seac operator callback. It is an error to call this when writing ufo
 fonts. Instead, clients should flatten seac operators into compound
 glyphs. */
static void glyphSeac(abfGlyphCallbacks *cb,
                      float adx, float ady, int bchar, int achar) {
    ufwCtx h = (ufwCtx) cb->direct_ctx;

    if (h->err.code != 0)
        return; /* Pending error */

    /* Call sequence error; seac glyphs should be expanded by the client. */
    h->err.code = ufwErrBadCall;
}

/* End glyph definition. */
static void glyphEnd(abfGlyphCallbacks *cb) {
    ufwCtx h = (ufwCtx) cb->direct_ctx;

    if (h->err.code != 0)
        return;
    else if (h->path.state < 2) {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }

    if (h->path.state >= 3) /* have seen a move to. */
        writeContour(h);

    if (h->path.state < 3) /* have NOT seen a move to, hence have never emitted an <outline> tag. */
        writeLine(h, "\t<outline>");

    writeLine(h, "\t</outline>");
    writeLine(h, "</glyph>");
    h->path.state = 0;

    flushBuf(h);
    /* Close dst stream */
    h->cb.stm.close(&h->cb.stm, h->stm.dst);

    return;
}

/* Public callback set */
const abfGlyphCallbacks ufwGlyphCallbacks = {
        NULL,
        NULL,
        NULL,
        glyphBeg,
        glyphWidth,
        glyphMove,
        glyphLine,
        glyphCurve,
        NULL,
        NULL,
        glyphGenop,
        glyphSeac,
        glyphEnd,
        NULL,
        NULL,
        NULL,  // compose
        NULL,  // transform
};
