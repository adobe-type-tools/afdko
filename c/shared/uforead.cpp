/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Simple ufo file parser.

 If a glif file exists in the processed glyph layer, I trust that the data is up to date, and use it,
 else I use the glif from the default layer. I read both the hint and drawing operator list before
 calling any call backs, and I need to have both in hand to do so.

  - parse points into a list of operators
  - parse ADobe t1 hint data, if any, into a list of hint mask operators
  - play back the list of operators, calling the abf call backs.

 Hint processing.
 The hint data is stored as an separate XML element from the outline data, This is so that outline editors do not need
 to understand this info, and it can be private data.

 Read the hint data into list of hint mask pointers. The hint mask pointer list is expanded to include a pointer for each point index,
 up to the highest point referenced in any hintmask. Hint mask pointers not referenced by a hint mask are set to null.
 The list of hint mask pointers is passed into the point processing function, which checks to see if there is a non-null
 hintmask pointer for the current point. If so, the function  plays back the current hint list through the call backs.

 The flex operator is also kept in the hint data.

 */

#include "uforead.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <errno.h>

#if defined(WIN32) || defined(WIN64)
# define strtok_r strtok_s
#endif
#include "absfont.h"
#include "dynarr.h"
#include "ctutil.h"
#include "supportexcept.h"
#include "txops.h"

enum {
    ufoUnknown,
    ufoNumeric,
    ufoOperator,
    ufoNotSet,
};

enum ufoFileState {
    None,
    parsingFontInfo,
    parsingLib,
    parsingGroups,
    parsingContentsDflt,
    parsingContentsAlt,
    preParsingGLIF,
    parsingGLIF
};

#define kMaxToken 1024
#define kMaxName 64
const char* t1HintKey = "com.adobe.type.autohint";
const char* t1HintKeyV1 = "com.adobe.type.autohint";
const char* t1HintKeyV2 = "com.adobe.type.autohint.v2";
int FDArrayInitSize = 50;

typedef struct {
    int type;
    char val[kMaxToken];
    size_t length;
    size_t offset; /* into buffer */
} token;

typedef unsigned short STI; /* String index */
#define STI_UNDEF 0xffff    /* Undefined string index */
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

enum {
#define DCL_KEY(key, index) index,
#include "ufo_ops.h"
#undef DCL_KEY
    kKeyCount
};

typedef struct {
    float mtx[6]; /* Float matrix */
    int isDefault;
    int isOffsetOnly;
} Transform;

typedef struct _floatRect {
    float left;
    float bottom;
    float right;
    float top;
} floatRect;

typedef struct {
    float x;
    float y;
    char* name;
    char* color;
    char* identifier;
} anchorRec;

typedef struct {
    float x;
    float y;
    float angle;
    char* name;
    char* color;
    char* identifier;
} guidelineRec;

typedef struct {
    const char* glyphName;
    char* glifFileName;
    char* glifFilePath;
    char* altLayerGlifFileName;
    long int glyphOrder;  // used to sort the glifRecs by glyph order from lib.plist.
    int cid;
    int iFD;
} GLIF_Rec;

typedef struct {
    long order;
    const char* glyphName;
} GlIFOrderRec;

typedef enum {
    movetoType,
    linetoType,
    curvetoType,
    closepathType,
} OpType;

typedef struct {
    OpType opType;
    float coords[6]; /* Float matrix */
    char* pointName;
} OpRec;

enum {
    outlineStart,
    outlineInContour,
    outlineInPoint,
    outlineInComponent,
    outlineInComment,
    outlineInLib,
    outlineInLibDict,
    outlineInID,
    outlineInHintData,
    outlineInHintSetList,
    outlineInFlexList,
    outlineInHintSet,
    outlineInHintSetName,
    outlineInHintSetStemList,
    outlineInStemHint,
    otherDictKey,
    otherInElement,
    otherInTag,
    otherInAnchor,
    otherInNote,
    otherInGuideline,
    inCustomLib,
    inCIDNumber,
    inFDNumber,
} outlineState;

typedef struct {
    float edge;
    float width;
    int flags;
} StemHint;
#define IS_VERT_STEM 1

typedef struct {
    dnaDCL(StemHint, maskStems);
    char* pointName;
} HintMask;

typedef struct {
    int currentCID;
    long CIDCount;
    int  currentiFD;
    int tag;
    GLIF_Rec* glifRec;
    Transform* transform;
    abfGlyphCallbacks* glyph_cb;
} glifInfo;

struct ufoCtx_ {
    abfTopDict top;
    abfFontDict fdict;
    int flags;
#define SEEN_END 1
    struct {
        void* dbg;
        void* src;
    } stm;
    struct {
        long offset;
        char* buf;
        size_t length;
        char* end;
        char* next;
        token tk;
    } src;
    struct {
        dnaDCL(GLIF_Rec, glifRecs);
        dnaDCL(GlIFOrderRec, glifOrder);
        dnaDCL(OpRec, opList);
    } data;
    struct {
        StemHint stems[T2_MAX_STEMS];
        dnaDCL(HintMask, hintMasks);
        dnaDCL(char*, flexOpList);
        char* pointName; /* used save the hint reference from the first point of a curve, since it has to be used in the last point of the curve. */
    } hints;

    dnaDCL(char*, valueArray); /* used when parsing <array> elements */
    const char* parseKeyName;  /* used to keep track of current top element name. */
    dnaDCL(char, tmp);         /* Temporary buffer */
    char* mark;                /* Buffer position marker */
    const char* altLayerDir;
    const char* defaultLayerDir;
    bool hasAltLayer;
    struct {
        int cnt;
        unsigned long flags;
#define PARSE_INIT          (1<<0)
#define PARSE_PATH          (1<<1)
#define PARSE_STARTHINT     (1<<2)
#define PARSE_HINT          (1<<3)
#define PARSE_ENDHINT       (1<<4)
#define PARSE_SEEN_MOVETO   (1<<6)  // have seen an explicit a "move". IF there is no subsequent marking operator, we skip the operation.
#define PARSE_END           (1<<7)

        int hintflags;
#define UFO_MAX_OP_STACK 18

        float array[UFO_MAX_OP_STACK];
    } stack;

    floatRect aggregatebounds;
    struct {  // Metric data
        struct abfMetricsCtx_ ctx;
        abfGlyphInfo gi;
        abfGlyphCallbacks cb;
        long defaultWidth;
    } metrics;

    struct {
        dnaDCL(abfGlyphInfo, index);
        dnaDCL(long, byName); /* In glyph name order */
        dnaDCL(long, widths); /* In index order; [SRI]->width */
    } chars;
    struct {  // String pool
        dnaDCL(long, index); /* In index order; [SRI]->iBuf */
        dnaDCL(char, buf);   /* String buffer */
    } strings;
    struct {
        ctlMemoryCallbacks mem;
        ctlStreamCallbacks stm;
    } cb;
    dnaCtx dna;
    struct {
       bool FDArray;
       bool CIDMap;
       bool hintSetListArray;
       bool type1HintDataV2;
       bool valueArray;
       enum ufoFileState UFOFile;
       glifInfo GLIFInfo;
    } parseState;

    uint8_t requiredCIDKeyFlag;
#define CIDKEY_CIDMAP      (1 << 1)
#define CIDKEY_FDARRAY     (1 << 2)
#define CIDKEY_SUPPLEMENT  (1 << 3)
#define CIDKEY_ORDERING    (1 << 4)
#define CIDKEY_REGISTRY    (1 << 5)

    struct
    {
        _Exc_Buf env;
        int code;
    } err;
};

typedef struct {
    long cnt; /* ABF_EMPTY_ARRAY */
    float array[14];
} BluesArray;

typedef abfGlyphInfo Char; /* Character record */

static STI addString(ufoCtx h, size_t length, const char* value);
static char* getString(ufoCtx h, STI sti);
static void newStrings(ufoCtx h);
static void freeStrings(ufoCtx h);
static void addWidth(ufoCtx h, STI sti, long value);
static void setWidth(ufoCtx h, STI sti, long value);
static long getWidth(ufoCtx h, STI sti);
static int addChar(ufoCtx h, STI sti, Char** chr);
static int CTL_CDECL postMatchChar(const void* key, const void* value,
                                   void* ctx);
static bool strEqual(const char* string1, const char* string2);
static long strtolCheck(ufoCtx h, char* keyValue, bool fail, char* msg, int base);
static double strtodCheck(ufoCtx h, char* keyValue, bool fail, char* msg);
static unsigned long strtoulCheck(ufoCtx h, char* keyValue, bool fail, char* msg, int base);

/* XML File Parsing Functions */
static xmlNodePtr parseXMLFile(ufoCtx h, const char* filename, const char* filetype);
static int parseXMLPlistFile(ufoCtx h, xmlNodePtr cur);
static int parseXMLGlifFile(ufoCtx h, xmlNodePtr cur, int tag, unsigned long *unicode, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec);
static const char* parseXMLKeyName(ufoCtx h, xmlNodePtr cur);
static const char* parseXMLKeyValue(ufoCtx h, xmlNodePtr cur);
static void parseXMLGLIFKey(ufoCtx h, xmlNodePtr cur, unsigned long *unicode, int tag, abfGlyphCallbacks* glyph_cb);
static int parseXMLPoint(ufoCtx h, xmlNodePtr cur, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec, int state);
static int parseXMLComponent(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec, abfGlyphCallbacks* glyph_cb);
static int parseXMLAnchor(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec);
static int parseXMLContour(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec, abfGlyphCallbacks* glyph_cb);
static int parseXMLGuideline(ufoCtx h, xmlNodePtr cur, int tag, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec);
static int parseType1HintDataV2(ufoCtx h, xmlNodePtr cur);
static bool parseFontInfoFDArray(ufoCtx h, xmlNodePtr cur);
static bool setFontInfoFD(ufoCtx h, char* keyName, char* keyValue);
static bool setFontInfoPD(ufoCtx h, char* keyName, char* keyValue);
static bool setFontInfoTopKey(ufoCtx h, char* keyName, char* keyValue);
static bool parseFontInfoPrivateDict(ufoCtx h, xmlNodePtr cur);
static bool parseCIDMap(ufoCtx h, xmlNodePtr cur);
static int parseGroups(ufoCtx h);
static bool parseFDArraySelectGroup(ufoCtx h, char* FDArraySelectKeyName, xmlNodePtr cur);
static bool validCIDKeyedFont(ufoCtx h, xmlNodePtr cur);

/* XML File Setting Functions */
static bool setXMLFileKey(ufoCtx h, const char* keyName, xmlNodePtr cur);
static bool setFontInfoKey(ufoCtx h, const char* keyName, xmlNodePtr cur);
static bool setLibKey(ufoCtx h, const char* keyName, xmlNodePtr cur);
static bool setGroupsKey(ufoCtx h, const char* keyName, xmlNodePtr cur);
static void setROSKeyValue(ufoCtx h, abfTopDict* top, const char* keyValue);

/* GLIF Rec Handling */
static void addGLIFRec(ufoCtx h, const char* glyphName, xmlNodePtr cur);  /* checks for existing rec*/
static GLIF_Rec* addNewGLIFRec(ufoCtx h, const char* glyphName, xmlNodePtr cur);  /* directly adds new GLIF rec */
static void updateGLIFRec(ufoCtx h, const char* glyphName, xmlNodePtr cur);
static GLIF_Rec* findGLIFRecByName(ufoCtx h, const char *glyphName);

/* -------------------------- Error Support ------------------------ */

const char* ufoErrStr(int err_code) {
    static const char* errstrs[] = {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string) string,
#include "ufoerr.h"
    };
    return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs)) ? (char*)"unknown error" : errstrs[err_code];
}

/* Write message to debug stream from va_list. */
static void vmessage(ufoCtx h, const char* fmt, va_list ap) {
    char text[BUFSIZ];

    if (h->stm.dbg == NULL)
        return; /* Debug stream not available */

    VSPRINTF_S(text, sizeof(text), fmt, ap);
    (void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(ufoCtx h, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vmessage(h, fmt, ap);
    va_end(ap);
}

static void CTL_CDECL fatal(ufoCtx h, int err_code, const char* fmt, ...) {
    if (fmt == NULL)
        /* Write standard error message */
        message(h, "%s", ufoErrStr(err_code));
    else {
        /* Write font-specific error message */
        va_list ap;
        va_start(ap, fmt);
        vmessage(h, fmt, ap);
        va_end(ap);
    }
    h->err.code = err_code;
    RAISE(&h->err.env, err_code, NULL);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void* memNew(ufoCtx h, size_t size) {
    void* ptr = h->cb.mem.manage(&h->cb.mem, NULL, size);
    if (ptr == NULL)
        fatal(h, ufoErrNoMemory, NULL);
    return ptr;
}

/* Free memory. */
static void memFree(ufoCtx h, void* ptr) {
    (void)h->cb.mem.manage(&h->cb.mem, ptr, 0);
}

/* -------------------------- Safe dynarr Callbacks ------------------------ */

/* Manage memory. */
static void* dna_manage(ctlMemoryCallbacks* cb, void* old, size_t size) {
    ufoCtx h = (ufoCtx)cb->ctx;
    void* ptr = h->cb.mem.manage(&h->cb.mem, old, size);
    if (size > 0 && ptr == NULL)
        fatal(h, ufoErrNoMemory, NULL);
    return ptr;
}

/* Initialize error handling dynarr context. */
static void dna_init(ufoCtx h) {
    ctlMemoryCallbacks cb;
    cb.ctx = h;
    cb.manage = dna_manage;
    h->dna = dnaNew(&cb, DNA_CHECK_ARGS);
}
/* ------------------------ Context handling ------------------------------ */

static void initHintMask(void* ctx, long cnt, HintMask* base) {
    // cnt is number of new elements allocated
    // base is ptr to the first new element.
    ufoCtx h = (ufoCtx)ctx;
    int i = 0;
    while (i < cnt) {
        HintMask* curHintMask = base + i++;
        dnaINIT(h->dna, curHintMask->maskStems, 20, 20);
        curHintMask->pointName = NULL;
    }
}

void ufoFree(ufoCtx h) {
    if (h == NULL)
        return;

    dnaFREE(h->valueArray);
    {
        int i = 0;
        while (i < h->hints.hintMasks.size) {
            HintMask* hintMask = &h->hints.hintMasks.array[i++];
            if (hintMask->maskStems.cnt > 0)
                dnaFREE(hintMask->maskStems);
        }
    }
    dnaFREE(h->hints.hintMasks);
    dnaFREE(h->hints.flexOpList);
    dnaFREE(h->chars.index);
    dnaFREE(h->chars.byName);
    dnaFREE(h->chars.widths);
    dnaFREE(h->tmp);
    dnaFREE(h->data.glifRecs);
    dnaFREE(h->data.glifOrder);
    dnaFREE(h->data.opList);
    freeStrings(h);
    dnaFree(h->dna);

    if (h->top.FDArray.array != &h->fdict) {  // if more memory was allocated for FDArray
        memFree(h, h->top.FDArray.array);
    }

    /* Close debug stream */
    if (h->stm.dbg != NULL)
        (void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);

    /* Free library context */
    memFree(h, h);
}

/* Validate client and create context */
ufoCtx ufoNew(ctlMemoryCallbacks* mem_cb, ctlStreamCallbacks* stm_cb,
              CTL_CHECK_ARGS_DCL) {
    ufoCtx h;

    /* Check client/library compatibility */
    if (CTL_CHECK_ARGS_TEST(UFO_VERSION))
        return NULL;

    /* Allocate context */
    h = (ufoCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct ufoCtx_));
    if (h == NULL)
        return NULL;

    /* Safety initialization */
    memset(h, 0, sizeof(*h));

    h->metrics.defaultWidth = 1000;
    h->altLayerDir = "glyphs.com.adobe.type.processedglyphs";
    h->defaultLayerDir = "glyphs";

    /* Copy callbacks */
    h->cb.mem = *mem_cb;
    h->cb.stm = *stm_cb;

    /* Set error handler */
    DURING_EX(h->err.env)

    /* Initialize service library */
    dna_init(h);

    dnaINIT(h->dna, h->valueArray, 256, 50);
    dnaINIT(h->dna, h->tmp, 100, 250);
    dnaINIT(h->dna, h->chars.index, 256, 1000);
    dnaINIT(h->dna, h->chars.byName, 256, 1000);
    dnaINIT(h->dna, h->chars.widths, 256, 1000);
    dnaINIT(h->dna, h->data.glifRecs, 14, 100);
    dnaINIT(h->dna, h->data.glifOrder, 14, 100);
    dnaINIT(h->dna, h->data.opList, 50, 50);
    dnaINIT(h->dna, h->hints.hintMasks, 10, 10);
    dnaINIT(h->dna, h->hints.flexOpList, 10, 10);
    h->hints.hintMasks.func = initHintMask;

    newStrings(h);

    /* Open debug stream */
    h->stm.dbg = h->cb.stm.open(&h->cb.stm, UFO_DBG_STREAM_ID, 0);

    HANDLER
    /* Initialization failed */
    ufoFree(h);
    h = NULL;
    END_HANDLER

    return h;
}

static void prepClientData(ufoCtx h) {
    h->top.sup.nGlyphs = h->chars.index.cnt;
    if (h->stm.dbg == NULL)
        abfCheckAllDicts(NULL, &h->top);
}

/* ---------------------- Buffer handling ---------------------------- */
static void fillbuf(ufoCtx h, long offset) {
    h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
    if (h->src.length == 0)
        h->flags |= SEEN_END;
    h->src.offset = offset;
    h->src.next = h->src.buf;
    h->src.end = h->src.buf + h->src.length;
}

static int nextbuf(ufoCtx h) {
    if (h->flags & SEEN_END)
        return 0;

    /* buffer read must be able to contain a full token */
    if (h->mark && h->mark != h->src.buf) {
        size_t new_offset = h->src.offset + (h->mark - h->src.buf);
        h->cb.stm.seek(&h->cb.stm, h->stm.src, new_offset);
        fillbuf(h, new_offset);

        /* make sure we are still pointing at the beginning of token */
        h->mark = h->src.buf;
    }
    if (h->src.next == h->src.end) {
        fillbuf(h, h->src.offset + h->src.length);
    }

    return ((h->flags & SEEN_END) == 0);
}

static int bufferReady(ufoCtx h) {
    if (h->src.next == h->src.end)
        return nextbuf(h);

    return 1;
}

static char* getBufferContextPtr(ufoCtx h) {
    // Get pointer into buffer showing current processing context. Used for error messages
    char* p = h->src.next - 20;
    if (p < h->src.buf)
        p = h->src.buf;
    if ((p + 64) >= h->src.end)
        *(h->src.end - 1) = 0;
    else
        *(p + 64) = 0;
    if (strlen(p) > 128)
        p[128] = 0;
    return p;
}

static token* setToken(ufoCtx h) {
    size_t len;
    if (h->src.buf == NULL || h->mark == NULL)
        return NULL;

    len = h->src.next - h->mark;
    if ((len + 1) > kMaxToken)
        return NULL;

    if (len > 0)
        memcpy(h->src.tk.val, h->mark, len);
    h->src.tk.val[len] = 0;
    h->src.tk.length = len;
    h->src.tk.offset = h->src.offset + (h->mark - h->src.buf);
    h->src.tk.type = ufoUnknown;

    return &h->src.tk;
}

/* Tokens are separated by whitespace and '"'
 '<' and '>' also serve to mark beginning and end of tokens.
 */
static token* getToken(ufoCtx h, int state) {
    char ch = 0;
    token* tk = NULL;
    h->mark = NULL;

    while (bufferReady(h)) {
        ch = *h->src.next;
        if (ch == 0) {
            break;
        }
        if (isspace((unsigned char)ch) || (ch == '"'))
            h->src.next++;
        else
            break;
    }

    while (bufferReady(h)) {
        if (ch == 0) {
            break;
        } else if (h->mark == NULL) {
            h->mark = h->src.next++;
            if (!bufferReady(h))
                break;
            ch = *h->src.next;
            if ((ch == '<') && (h->mark != h->src.next))
                break;
            while ((!isspace((unsigned char)ch)) && (ch != '"')) {
                h->src.next++;
                if ((ch == 0) || (ch == '>') || (!bufferReady(h)))
                    break;
                ch = *h->src.next;
                if ((ch == '<') && (h->mark != h->src.next))
                    break;
            }
            break;
        } else {
            break;
        }
    }

    // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark) && (isspace((unsigned char)*(h->src.next - 1))))
        h->src.next--;

    tk = setToken(h);
    if (tk == NULL) {
        if (state != 0) {
            fatal(h, ufoErrParse, "Encountered end of buffer before end of glyph.%s.", getBufferContextPtr(h));
        }
    }

    return tk;
}

/* return actual tokens, ignores comments, preserves white space */
static token* getAttribute(ufoCtx h, int state) {
    char ch = 0;
    token* tk = NULL;
    int lastWasQuote = 0;
    h->mark = NULL;

    while (bufferReady(h)) {
        ch = *h->src.next;
        if (ch == 0) {
            break;
        }
        if (isspace((unsigned char)ch)) {
            lastWasQuote = 0;
            h->src.next++;
        } else if (ch == '"') {
            if (lastWasQuote) {
                ch = 0; /* we have an empty attribute */
                h->mark = h->src.next;
                break;
            } else {
                h->src.next++;
                lastWasQuote = 1;
            }
        } else
            break;
    }

    while (bufferReady(h)) {
        if (ch == 0) {
            break;
        } else if (h->mark == NULL) {
            h->mark = h->src.next++;
            if (!bufferReady(h))
                break;
            ch = *h->src.next;
            while (!(ch == '"')) {
                h->src.next++;
                if ((ch == 0) || (!bufferReady(h)))
                    break;
                ch = *h->src.next;
            }
            break;
        } else {
            break;
        }
    }
    // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark) && (isspace((unsigned char)*(h->src.next - 1))))
        h->src.next--;

    tk = setToken(h);
    if (tk == NULL) {
        if (state != 0) {
            fatal(h, ufoErrParse, "Encountered end of buffer before end of glyph.%s.", getBufferContextPtr(h));
        }
    }

    return tk;
}

/* return value between begin/end element, skipping white space before and after. */
static token* getElementValue(ufoCtx h, int state) {
    char ch = 0;
    token* tk = NULL;
    h->mark = NULL;

    while (bufferReady(h)) {
        ch = *h->src.next;
        if (ch == 0) {
            break;
        }
        if (ch == '"')
            h->src.next++;
        else
            break;
    }

    while (bufferReady(h)) {
        if (ch == 0) {
            break;
        } else if (h->mark == NULL) {
            h->mark = h->src.next++;
            if (!bufferReady(h))
                break;
            ch = *h->src.next;
            while (ch != '<') {
                h->src.next++;
                if ((ch == 0) || (!bufferReady(h)))
                    break;
                ch = *h->src.next;
            }
            break;
        } else {
            break;
        }
    }
    // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark) && (isspace((unsigned char)*(h->src.next - 1))))
        h->src.next--;

    tk = setToken(h);
    if (tk == NULL) {
        if (state != 0) {
            fatal(h, ufoErrParse, "Encountered end of buffer before end of glyph.%s.", getBufferContextPtr(h));
        }
    }

    return tk;
}

/* Construct matrix from args. */
static void setTransformMtx(Transform* transform,
                            float a, float b, float c, float d, float tx, float ty, int isDefault, int isOffsetOnly) {
    float* val = transform->mtx;
    val[0] = a;
    val[1] = b;
    val[2] = c;
    val[3] = d;
    val[4] = tx;
    val[5] = ty;
    transform->isDefault = isDefault;
    transform->isOffsetOnly = isOffsetOnly;
}

/* Multiple matrices a and b giving result. */
static void matMult(float* result, float* a, float* b) {
    result[0] = a[0] * b[0] + a[1] * b[2];
    result[1] = a[0] * b[1] + a[1] * b[3];
    result[2] = a[2] * b[0] + a[3] * b[2];
    result[3] = a[2] * b[1] + a[3] * b[3];
    result[4] = a[4] * b[0] + a[5] * b[2] + b[4];
    result[5] = a[4] * b[1] + a[5] * b[3] + b[5];
}

static void setTransformValue(Transform* transform, float val, int valIndex) {
    transform->mtx[valIndex] = val;
    if ((valIndex == 0) || (valIndex == 3)) {
        if (val != 1.0) {
            transform->isDefault = 0;
            transform->isOffsetOnly = 0;
        }
    } else if (val != 0) {
        transform->isDefault = 0;
        if ((valIndex == 1) || (valIndex == 2))
            transform->isOffsetOnly = 0;
    }
}

/* -------------------------- Parsing routines ------------------------ */

/* Check stack contains at least n elements. */
#define CHKUFLOW(n)                                                 \
    do                                                              \
        if (h->stack.cnt < (n)) fatal(h, ufoErrStackUnderflow, ""); \
    while (0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n)                                                                   \
    do                                                                                \
        if (h->stack.cnt + (n) > UFO_MAX_OP_STACK) fatal(h, ufoErrStackOverflow, ""); \
    while (0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++] = (float)(v))

/* hints */

static void doOp_mt(ufoCtx h, abfGlyphCallbacks* glyph_cb, char* pointName) {
    float dy, dx;
    OpRec* opRec;
    CHKUFLOW(2);
    dy = POP();
    dx = POP();

    opRec = dnaNEXT(h->data.opList);
    opRec->opType = movetoType;
    opRec->coords[0] = dx;
    opRec->coords[1] = dy;
    opRec->pointName = pointName; /* pointName may or may not be NULL */
    h->stack.flags |= PARSE_SEEN_MOVETO;
    // printf("moveto %f %f. opIndex: %ld \n", dx, dy, h->data.opList.cnt-1);
    h->metrics.cb.move(&h->metrics.cb, dx, dy);
}

static void doOp_dt(ufoCtx h, abfGlyphCallbacks* glyph_cb, char* pointName) {
    float dy, dx;
    OpRec* opRec;
    CHKUFLOW(2);

    dy = POP();
    dx = POP();

    opRec = dnaNEXT(h->data.opList);
    opRec->opType = linetoType;
    opRec->coords[0] = dx;
    opRec->coords[1] = dy;
    opRec->pointName = pointName; /* pointName may or may not be NULL */
    // printf("lineto %f %f. opIndex: %ld \n", dx, dy, h->data.opList.cnt-1);
    if (!(h->stack.flags & PARSE_SEEN_MOVETO)) {
        h->metrics.cb.move(&h->metrics.cb, dx, dy);
        h->stack.flags |= PARSE_SEEN_MOVETO;
    } else {
        h->metrics.cb.line(&h->metrics.cb, dx, dy);
    }
}

static void doOp_ct(ufoCtx h, abfGlyphCallbacks* glyph_cb, char* pointName) {
    OpRec* opRec;

    opRec = dnaNEXT(h->data.opList);
    opRec->opType = curvetoType;
    opRec->pointName = pointName; /* pointName may or may not be NULL */

    if (!(h->stack.flags & PARSE_SEEN_MOVETO)) {
        float dy3, dx3;
        /* If this is the first operator, then only the final curve point has been seen; the first two points are at the end of the contour. */
        CHKUFLOW(2);
        dy3 = POP();
        dx3 = POP();

        opRec->coords[0] = dx3;
        opRec->coords[1] = dy3;

        h->metrics.cb.move(&h->metrics.cb, dx3, dy3);
        h->stack.flags |= PARSE_SEEN_MOVETO;
    } else {
        float dy3, dx3, dy2, dx2, dy1, dx1;
        CHKUFLOW(6);

        dy3 = POP();
        dx3 = POP();
        dy2 = POP();
        dx2 = POP();
        dy1 = POP();
        dx1 = POP();

        opRec->coords[0] = dx1;
        opRec->coords[1] = dy1;
        opRec->coords[2] = dx2;
        opRec->coords[3] = dy2;
        opRec->coords[4] = dx3;
        opRec->coords[5] = dy3;
        // printf("curveto %f %f. opIndex: %ld \n", dx3, dy3, h->data.opList.cnt-1);

        h->metrics.cb.curve(&h->metrics.cb,
                            dx1, dy1,
                            dx2, dy2,
                            dx3, dy3);
    }
}

static void doOp_ed(ufoCtx h, abfGlyphCallbacks* glyph_cb) {
    abfMetricsCtx g = &h->metrics.ctx;

    /* get the bounding box and compute the aggregate */
    if (h->aggregatebounds.left > g->real_mtx.left)
        h->aggregatebounds.left = g->real_mtx.left;
    if (h->aggregatebounds.bottom > g->real_mtx.bottom)
        h->aggregatebounds.bottom = g->real_mtx.bottom;
    if (h->aggregatebounds.right < g->real_mtx.right)
        h->aggregatebounds.right = g->real_mtx.right;
    if (h->aggregatebounds.top < g->real_mtx.top)
        h->aggregatebounds.top = g->real_mtx.top;

    glyph_cb->end(glyph_cb); /* I call glyph_cb->end directly, unlike the path operators, as the opList has already been played back. */
}

static int tkncmp(token* tk, const char* str) {
    size_t len = strlen(str);
    int retVal = 1;
    if ((tk != NULL) && (len == tk->length))
        retVal = strncmp(tk->val, str, tk->length);
    return retVal;
}

static int tokenEqualStr(token* tk, const char* str) {
    return tkncmp(tk, str) == 0;
}

static int tokenEqualStrN(token* tk, const char* str, int n) {
    return 0 == strncmp(tk->val, str, n);
}

static int isUnknownAttribute(token* tk) {
    if (tk == NULL)
        return true;
    else
        return tk->val[tk->length - 1] == '=';
}

/* --------------------- Glyph Processing ----------------------- */
static char* copyStr(ufoCtx h, const char* oldString) {
    char* value;
    int l = strlen(oldString);
    value = (char *)memNew(h, l + 1);
    memcpy(value, oldString, l);
    value[l] = '\0';
    return value;
}

static char* concatStrs(ufoCtx h, const char* string1, const char* string2) {
    char* catStr;
    catStr = memNew(h, strlen(string1) + strlen(string2) + 1);
    strcpy(catStr, string1);
    strcat(catStr, string2);
    return catStr;
}

static char* getKeyValue(ufoCtx h, const char* endName, int state) {
    char* value = NULL;
    token* tk;

    tk = getElementValue(h, state);
    if (!tokenEqualStr(tk, endName)) {
        value = (char *)memNew(h, tk->length + 1);
        STRCPY_S(value, tk->length+1, tk->val);
        value[tk->length] = 0;
        // get end element to clear it
        tk = getToken(h, state);
        if (!tokenEqualStr(tk, endName))
            fatal(h, ufoErrParse, "Encountered element '%s' when reading value for element '%s'.", tk->val, endName);
    }
    return value;
}

/* free dynamic valueArray and set cnt to 0 */
static void freeValueArray(ufoCtx h) {
    if (h->valueArray.cnt != 0) {
        int i = 0;
        while ((i < h->valueArray.cnt)) {
            memFree(h, h->valueArray.array[i]);
            i++;
        }
        dnaSET_CNT(h->valueArray, 0);
    }
    h->parseState.valueArray = false;
}

/* ToDo: add extra warnings for verbose-output */
static void setGlifOrderArray(ufoCtx h, const char* arrayKeyName) {
    int i = 0;
    if (h->valueArray.cnt == 0) {
//        message(h, "Warning: Encountered empty or invalid array for %s. Skipping", arrayKeyName);
        return;
    }
    while ((i < h->valueArray.cnt)) {
        GlIFOrderRec* newGLIFOrderRec;
        newGLIFOrderRec = dnaNEXT(h->data.glifOrder);
        newGLIFOrderRec->glyphName = copyStr(h, h->valueArray.array[i]);
        newGLIFOrderRec->order = h->data.glifOrder.cnt - 1;
        i++;
    }
    freeValueArray(h);
}

/* ToDo: add extra warnings for verbose-output */
static void setBluesArrayValue(ufoCtx h, BluesArray* bluesArray, int numElements,
                               const char* arrayKeyName) {
    int i = 0;
    if (h->valueArray.cnt == 0) {
//        message(h, "Warning: Encountered empty or invalid array for %s. Skipping", arrayKeyName);
        return;
    }
    bluesArray->cnt = h->valueArray.cnt;
    while ((i < h->valueArray.cnt) && (i < numElements)) {
        bluesArray->array[i] = (float)atof(h->valueArray.array[i]);
        i++;
    }
    freeValueArray(h);
}

/* ToDo: add extra warnings for verbose-output */
static void setFontMatrix(ufoCtx h, abfFontMatrix* fontMatrix, int numElements) {
    int i = 0;
    if (h->valueArray.cnt == 0) {
//        message(h, "Warning: Encountered empty or invalid array for FontMatrix. Skipping");
        return;
    }
    fontMatrix->cnt = h->valueArray.cnt;
    while ((i < h->valueArray.cnt) && (i < numElements)) {
        fontMatrix->array[i] = (float)atof(h->valueArray.array[i]);
        i++;
    }
    freeValueArray(h);
}

/* ToDo: add extra warnings for verbose-output
 Tests if the input key value is parseable. Handles array parsing and empty value parsing.
 If invalid array state, it attempts to free valueArray for any leftover values.
 */
static bool keyValueParseable(ufoCtx h, xmlNodePtr cur, const char* keyValue,
                              const char* keyName) {
    bool valid = true;
    if (keyValue == NULL) {
        if (!h->parseState.valueArray && !h->parseState.CIDMap) {
            valid = false;
//            message(h, "Warning: Encountered missing value for key %s. Skipping", keyName);
        } else if (h->parseState.valueArray && h->valueArray.cnt == 0) {
            valid = false;
//            message(h, "Warning: Encountered empty <%s> for key %s. Skipping", cur->name, keyName);
        }
    } else {
        if (strEqual(keyValue, "")){
//        message(h, "Warning: Encountered empty <%s> for key %s. Skipping", cur->name, keyName);
        valid = false;
        }
    }
    if (!valid)
        freeValueArray(h);  /* we free h->valueArray after parsing every key to clean up any leftover/invalid data */

    return valid;
}

static bool setXMLFileKey(ufoCtx h, const char* keyName, xmlNodePtr cur) {
    /* returns false when current key is NULL, not parseable, or an unknown key/value pair.
       otherwise returns true*/
    abfTopDict* top = &h->top;
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
    abfPrivateDict* pd = &fd->Private;
    BluesArray* bluesArray;
    abfFontMatrix* fontMatrix;
    const char* keyValue;
    bool result = true;

    if (keyName == NULL)
        return false;
    if (h->parseState.UFOFile == parsingFontInfo)
        result = setFontInfoKey(h, keyName, cur);
    else if (h->parseState.UFOFile == parsingLib)
        result = setLibKey(h, keyName, cur);
    else if (h->parseState.UFOFile == parsingGroups)
        result = setGroupsKey(h, keyName, cur);
    else if (h->parseState.UFOFile == parsingContentsDflt)
        addGLIFRec(h, keyName, cur);
    else if (h->parseState.UFOFile == parsingContentsAlt)
        updateGLIFRec(h, keyName, cur);
    freeValueArray(h);
    return result;
}

static int CTL_CDECL cmpOrderRecs(const void* first, const void* second, void* ctx) {
    GlIFOrderRec* orderRec1 = (GlIFOrderRec*)first;
    GlIFOrderRec* orderRec2 = (GlIFOrderRec*)second;
    int retVal = 0;
    retVal = strcmp(orderRec1->glyphName, orderRec2->glyphName);
    return retVal;
}

static int CTL_CDECL cmpGlifRecs(const void* first, const void* second, void* ctx) {
    GLIF_Rec* glifRec1 = (GLIF_Rec*)first;
    GLIF_Rec* glifRec2 = (GLIF_Rec*)second;
    int retVal = 0;
    if (glifRec1->glyphOrder == glifRec2->glyphOrder) {
        retVal = strcmp(glifRec1->glyphName, glifRec2->glyphName);
    } else if (glifRec1->glyphOrder == ABF_UNSET_INT) {
        retVal = 1;
    } else if (glifRec2->glyphOrder == ABF_UNSET_INT) {
        retVal = -1;
    } else {
        if (glifRec1->glyphOrder > glifRec2->glyphOrder)
            retVal = 1;
        else
            retVal = -1;
    }
    return retVal;
}

static int matchGLIFOrderRec(const void* key, const void* value, void* ctx) {
    GlIFOrderRec* orderRec = (GlIFOrderRec*)value;
    return strcmp((char*)key, orderRec->glyphName);
}

static long getGlyphOrderIndex(ufoCtx h, const char* glyphName) {
    long orderIndex = ABF_UNSET_INT;
    size_t recIndex = 0;

    if (ctuLookup(glyphName, h->data.glifOrder.array, h->data.glifOrder.cnt,
                  sizeof(h->data.glifOrder.array[0]), matchGLIFOrderRec, &recIndex, h)) {
        orderIndex = h->data.glifOrder.array[recIndex].order;
    } else {
        message(h, "Warning: glyph order does not contain glyph name '%s'.", glyphName);
    }

    return orderIndex;
}

static GLIF_Rec* addNewGLIFRec(ufoCtx h, const char* glyphName, xmlNodePtr cur) {
    GLIF_Rec* newGLIFRec;
    long int glyphOrder = ABF_UNSET_INT;
    const char* fileName = parseXMLKeyValue(h, cur);
    if (!keyValueParseable(h, cur, fileName, glyphName))
        return NULL;

    if (h->data.glifOrder.cnt != 0)  /* only try to get glyphOrderIndex if cnt > 0 */
        glyphOrder = getGlyphOrderIndex(h, glyphName);
    newGLIFRec = dnaNEXT(h->data.glifRecs);
    newGLIFRec->glyphName = glyphName;
    newGLIFRec->glyphOrder = glyphOrder;
    if (!h->parseState.CIDMap) {  /* do not proceed if currently parsing CIDMap in lib.plist. This information will be filled in later. */
        newGLIFRec->glifFileName = memNew(h, 1 + strlen(fileName));
        sprintf(newGLIFRec->glifFileName, "%s", fileName);
        newGLIFRec->altLayerGlifFileName = NULL;
    }
    newGLIFRec->cid = -1;
    newGLIFRec->iFD = -1;
    return newGLIFRec;
}

static void addGLIFRec(ufoCtx h, const char* glyphName, xmlNodePtr cur) {
    GLIF_Rec* foundGlyph = NULL;
    char* fileName = parseXMLKeyValue(h, cur);
    if (fileName == NULL) {
        fatal(h, ufoErrParse, "Encountered glyph reference '%s' in contents.plist with an empty file path.", glyphName);
    }
    foundGlyph = findGLIFRecByName(h, glyphName);
    if (foundGlyph == NULL){
        addNewGLIFRec(h, glyphName, cur);
    } else {
        foundGlyph->glifFileName = fileName;
        if (foundGlyph->glyphOrder == -1)
            foundGlyph->glyphOrder = getGlyphOrderIndex(h, glyphName);
    }
}

int glifRecNameComparator(const void *a, const void *b) {
    const GLIF_Rec *p1 = (GLIF_Rec *) a;
    const GLIF_Rec *p2 = (GLIF_Rec *) b;
    return strcmp((*p1).glyphName, (*p2).glyphName);
}

static GLIF_Rec* findGLIFRecByName(ufoCtx h, const char *glyphName)
{
    int i = 0;
    while (i < h->data.glifRecs.cnt) {
        GLIF_Rec* glifRec;
        glifRec = &h->data.glifRecs.array[i];
        if (strEqual(glifRec->glyphName, glyphName)) {
            return glifRec;
        }
        i++;
    }
    return NULL;
}

static void updateGLIFRec(ufoCtx h, char* glyphName, xmlNodePtr cur) {
    GLIF_Rec* glifRec;
    char* fileName = parseXMLKeyValue(h, cur);
    if (!keyValueParseable(h, cur, fileName, glyphName))
        return;
    if (fileName == NULL) {
        /* this is basically muted for now, as the previous check will return and skip if not parseable.
           We'll add this back once we add verbosity flag */
        message(h, "Encountered glyph reference %s in alternate layer's contents.plist with an empty file path. ", glyphName);
        return;
    }

    glifRec = findGLIFRecByName(h, glyphName);
    if (glifRec == NULL) {
        message(h, "Warning: glyph '%s' is in the processed layer but not in the default layer.", glyphName);
    } else {
        glifRec->altLayerGlifFileName = memNew(h, 1 + strlen(fileName));
        sprintf(glifRec->altLayerGlifFileName, "%s", fileName);
    }
}


#define START_PUBLIC_ORDER 1256
#define IN_PUBLIC_ORDER 1257
#define IN_COMMENT 1258
#define REGISTRY 1259
#define ORDERING 1260
#define SUPPLEMENT 1261
#define CIDNAME 1255

static int parseLibPlist(ufoCtx h) {
    const char* filetype = "plist";
    h->parseState.UFOFile = parsingLib;

    h->src.next = h->mark = NULL;
    h->cb.stm.clientFileName = "lib.plist";
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0)) {
        message(h, "Warning: Unable to open lib.plist in source UFO font.");
        return ufoSuccess;
    }

    dnaSET_CNT(h->valueArray, 0);

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLPlistFile(h, cur);

    if (h->data.glifOrder.cnt > 0) {
        /* Sort the array by glyph name. */

        ctuQSort(h->data.glifOrder.array, h->data.glifOrder.cnt,
                 sizeof(h->data.glifOrder.array[0]), cmpOrderRecs, h);

        /* weed out duplicates - these cause sorting to work differently depending on whether
         we approach the pair from the top or bottom. */
        {
            int i = 1;
            while (i < h->data.glifOrder.cnt) {
                if (0 == strcmp(h->data.glifOrder.array[i].glyphName, h->data.glifOrder.array[i - 1].glyphName)) {
                    /* set the glyph orders to be the same. First wins */
                    h->data.glifOrder.array[i].order = h->data.glifOrder.array[i - 1].order;
                    message(h, "Warning: glyph order contains duplicate entries for glyphs '%s'.", h->data.glifOrder.array[i].glyphName);
                }
                i++;
            }
        }
    }

    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    return parsingSuccess;
}

static int parseGroups(ufoCtx h) {
    const char* filetype = "plist";
    h->parseState.UFOFile = parsingGroups;

    h->src.next = h->mark = NULL;
    h->cb.stm.clientFileName = "groups.plist";
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0)) {
        if (h->top.sup.flags == ABF_CID_FONT)
            message(h, "Warning: FDArraySelect not defined for cid-keyed font");
        return ufoSuccess;
    }

    dnaSET_CNT(h->valueArray, 0);

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLPlistFile(h, cur);

    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    h->parseState.UFOFile = None;
    return parsingSuccess;
}


static int parseGlyphList(ufoCtx h, bool altLayer) {
    const char* filetype = "plist";
    char *clientFilePath;
    const char *plistFileName = "/contents.plist";
    int l;

    if (altLayer) {
        l = strlen(h->altLayerDir) + strlen(plistFileName) + 2;
        clientFilePath = (char *)memNew(h, l);
        STRCPY_S(clientFilePath, l, h->altLayerDir);
        h->parseState.UFOFile = parsingContentsAlt;
    } else {
        h->parseState.UFOFile = parsingContentsDflt;
        l = strlen(h->defaultLayerDir) + strlen(plistFileName) + 2;
        clientFilePath = (char *)memNew(h, l);
        STRCPY_S(clientFilePath, l, h->defaultLayerDir);
    }
    STRCAT_S(clientFilePath, l, plistFileName);
    h->cb.stm.clientFileName = clientFilePath;

    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0)) {
        if (altLayer) {
            h->hasAltLayer = false;
            return ufoSuccess;
        } else {
            fprintf(stderr, "Failed to read %s\n", h->cb.stm.clientFileName);
            return ufoErrSrcStream;
        }
    }

    dnaSET_CNT(h->valueArray, 0);

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLPlistFile(h, cur);

    /* 'glyph order does not contain glyph name' warnings in getGlyphOrderIndex are suppressed
        if glyphOrder count is 0 to reduce amount of warnings.
        Instead, add one warning here.*/
    if (h->data.glifOrder.cnt == 0)
        message(h, "Warning: public.glyphOrder key is empty and does not contain glyph name for all %ld glyphs. Consider defining this in lib.plist.", h->data.glifRecs.cnt);

    /* process the glyph order layers */
    if (!altLayer) {
        if (h->data.glifOrder.cnt > 0) {
            /* sort glifRecs by glyphOrder */
            ctuQSort(h->data.glifRecs.array, h->data.glifRecs.cnt,
                     sizeof(h->data.glifRecs.array[0]), cmpGlifRecs, h);
        }
        if (h->data.glifRecs.cnt > 0) {
            int i = 0;
            while (i < h->data.glifRecs.cnt) {
                GLIF_Rec* glifRec;
                glifRec = &h->data.glifRecs.array[i++];
                addString(h, strlen(glifRec->glyphName), glifRec->glyphName);
            }
        }
    }

    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;

    memFree(h, clientFilePath);
    return parsingSuccess;
}

static int preParseGLIF(ufoCtx h, GLIF_Rec* newGLIFRec, int tag);

static int preParseGLIFS(ufoCtx h) {
    int tag = 0;
    int retVal = ufoSuccess;

    h->metrics.defaultWidth = 0;
    while (tag < h->data.glifRecs.cnt) {
        int glyphRetVal;
        GLIF_Rec* glifRec = &h->data.glifRecs.array[tag];
        addWidth(h, tag, 0);                          /* will set this to a real value later, if the value is supplied. */
        glyphRetVal = preParseGLIF(h, glifRec, tag);  // set width, stores offset to outline data
        if (glyphRetVal != ufoSuccess)
            retVal = glyphRetVal;
        tag++;
    }
    return retVal;
}

static void addCharFromGLIF(ufoCtx h, int tag, GLIF_Rec* glifRec,
                            const char* glyphName, long char_begin,
                            long char_end, unsigned long unicode) {
    abfGlyphInfo* chr;

    if (addChar(h, tag, &chr)) {
        message(h, "Warning: duplicate charstring <%s> (discarded)",
                getString(h, tag));
    } else {
        chr->flags = 0;
        chr->tag = tag;
        chr->iFD = 0;
        if (glifRec->cid >= 0) {
            chr->cid = glifRec->cid;
            if (glifRec->iFD < 0){
                    fatal(h, ufoErrParse, "glyph '%s' missing FDArray index in <lib> dict", glyphName);
            }
            chr->iFD = glifRec->iFD;
        } else if (h->top.sup.flags & ABF_CID_FONT) {
            fatal(h, ufoErrParse, "glyph '%s' missing CID number in <lib> dict", glyphName);
        }
        chr->gname.ptr = glyphName;
        chr->gname.impl = tag;
        if (unicode != ABF_GLYPH_UNENC) {
            chr->flags |= ABF_GLYPH_UNICODE;
            chr->encoding.code = unicode;
            chr->encoding.next = 0;
        } else {
            // If it doesn't have a Unicode, it stays unencoded.
            chr->encoding.code = ABF_GLYPH_UNENC;
            chr->encoding.next = 0;
        }
        chr->sup.begin = char_begin;
        chr->sup.end = char_end;
        // chr->width  = width;
    }
}

static int preParseGLIF(ufoCtx h, GLIF_Rec* glifRec, int tag) {
    h->parseState.UFOFile = preParsingGLIF;
    const char* filetype = "glyph";
    int char_begin = 0;
    int char_end = 0;
    unsigned long unicode = ABF_GLYPH_UNENC;
    char tempVal[kMaxName];
    char tempName[kMaxName];
    token* tk;
    h->src.next = h->mark = NULL;

    h->flags &= ~((unsigned long)SEEN_END);

    h->stm.src = NULL;
    glifRec->glifFilePath = NULL;

    if ((h->hasAltLayer) && (glifRec->altLayerGlifFileName != NULL)) {
        /* First, try the alt layer directory */
        int l = strlen(h->altLayerDir) +
                strlen(glifRec->altLayerGlifFileName) + 1;
        glifRec->glifFilePath = (char *)memNew(h, l+1);
        snprintf(glifRec->glifFilePath, l+1, "%s/%s",
                 h->altLayerDir, glifRec->altLayerGlifFileName);

        h->cb.stm.clientFileName = glifRec->glifFilePath;
        h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    }

    if (h->stm.src == NULL) {
        /* Didn't work. Try the glyph layer directory */
        if (glifRec->glifFilePath) {
            memFree(h, glifRec->glifFilePath);
        }
        if (glifRec->glifFileName == NULL) {
            fatal(h, ufoErrParse, "Warning: glyph '%s' missing filename.", glifRec->glyphName);
        }
        int l = strlen(h->defaultLayerDir) + strlen(glifRec->glifFileName) + 1;
        glifRec->glifFilePath = (char *)memNew(h, l+1);
        snprintf(glifRec->glifFilePath, l+1, "%s/%s",
                 h->defaultLayerDir, glifRec->glifFileName);

        h->cb.stm.clientFileName = glifRec->glifFilePath;
        h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    }
    if ((h->stm.src == NULL) || (h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))) {
        fatal(h, ufoErrSrcStream, "Failed to open the %s glif file.\n", glifRec->glifFilePath);
    }

    dnaSET_CNT(h->valueArray, 0);
    h->parseState.GLIFInfo.glifRec = glifRec;

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLGlifFile(h, cur, tag, &unicode, NULL, glifRec);

    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    return parsingSuccess;
}

static int cmpNumeric(const void* first, const void* second, void* ctx) {
    int retVal;
    if ((*(float*)first) == (*(float*)second))
        retVal = 0;
    else if ((*(float*)first) > (*(float*)second))
        retVal = 1;
    else
        retVal = -1;
    return retVal;
}

static void fixUnsetDictValues(ufoCtx h) {
    abfTopDict* top = &h->top;
    abfFontDict* fd0 = &h->top.FDArray.array[0];
    abfPrivateDict* pd = &fd0->Private;

    if (fd0->FontName.ptr == NULL) {
        message(h, "Warning: No PS name specified in source UFO font.");
    }

    if (top->version.ptr == NULL) {
        message(h, "Warning: No version specified in source UFO font.");
    }

    if (pd->StemSnapH.cnt > ABF_EMPTY_ARRAY) {
        if (pd->StdHW == ABF_UNSET_REAL) {
            pd->StdHW = pd->StemSnapH.array[0];
        }
        ctuQSort(pd->StemSnapH.array, pd->StemSnapH.cnt,
                 sizeof(pd->StemSnapH.array[0]), cmpNumeric, h);
    }
    if (pd->StemSnapV.cnt > ABF_EMPTY_ARRAY) {
        if (pd->StdVW == ABF_UNSET_REAL) {
            pd->StdVW = pd->StemSnapV.array[0];
        }
        ctuQSort(pd->StemSnapV.array, pd->StemSnapV.cnt,
                 sizeof(pd->StemSnapV.array[0]), cmpNumeric, h);
    }
    top->sup.srcFontType = abfSrcFontTypeUFOName;
}

static void skipToDictEnd(ufoCtx h) {
    int state = 0; /* 0 == start, 1=in first dict, 2 in key, 3= in value, 4=in array 4 in comment, 5 in child dict.*/

    while (!(h->flags & SEEN_END)) {
        token* tk;
        tk = getToken(h, state);

        if (tokenEqualStr(tk, "<dict>")) {
            skipToDictEnd(h);
        } else if (tokenEqualStr(tk, "</dict>")) {
            break;
        }
    } /* end while more tokens */
}

static void reallocFDArray(ufoCtx h) {
    float newFDArraySize = h->top.FDArray.cnt * 1.5;  /* x1.5 for efficiency */
    abfFontDict* newFDArray = (abfFontDict *)memNew(h, newFDArraySize * sizeof(abfFontDict));
    memcpy(newFDArray, h->top.FDArray.array, (h->top.FDArray.cnt - 1) * sizeof(abfFontDict));
    memFree(h, h->top.FDArray.array);
    h->top.FDArray.array = newFDArray;
}

static xmlNodePtr parseXMLFile(ufoCtx h, const char* filename, const char* filetype){
    xmlDocPtr doc;
    xmlNodePtr cur;
    int result;

    xmlKeepBlanksDefault(0);

    result = h->cb.stm.xml_read(&h->cb.stm, h->stm.src, &doc);

    if (result == 0) {
        fatal(h, ufoErrParse, "The %s file is empty.\n", filename);
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xmlFreeDoc(doc);
        fatal(h, ufoErrSrcStream, "Unable to read '%s'.\n", filename);
    }

    if (!xmlStrEqual((cur)->name, (const xmlChar *) filetype)) {
        xmlFreeDoc(doc);
        fatal(h, ufoErrSrcStream, "File %s is of the wrong type, root node != %s.\n", filename, filetype);
    }

    cur = (cur)->xmlChildrenNode;
    while (cur && xmlIsBlankNode(cur)) {
        cur = (cur) -> next;
    }

    if ( cur == NULL ) {
        xmlFreeDoc(doc);
        return NULL;
    }

    if (strEqual("plist", filetype)) {
        if ((!xmlStrEqual((cur)->name, (const xmlChar *) "dict"))) {
            xmlFreeDoc(doc);
            fatal(h, ufoErrSrcStream, "Error reading outermost <dict> in %s.\n", filename);
        }
    }
    return cur;
}

static bool strEqual(const char* string1, const char* string2) {
    if ((string1 == NULL) || (string2 == NULL)) {
        return false;
    }
    return !(strcmp(string1, string2));
}

static bool xmlKeyEqual(xmlNodePtr cur, const char* name){
    if (cur != NULL)
        return xmlStrEqual(cur->name, (const xmlChar *) name);
    else
        return false;
}

static bool xmlAttrEqual(xmlAttr *attr, const char* name){
    if (attr != NULL)
        return xmlStrEqual(attr->name, (const xmlChar *) name);
    else
        return false;
}

static char* getXmlAttrValue(xmlAttr *attr){
    return (char*) attr->children->content;
}

static bool setXMLLib(ufoCtx h, xmlNodePtr cur, const char* keyName) {
    const char* keyValue;
    bool result = false;
    if (h->parseState.UFOFile == parsingGLIF) {  /* when called from parseGLIF */
        if (strEqual(keyName, "com.adobe.type.autohint.v2")) {
            if (cur != NULL) {
                h->parseState.type1HintDataV2 = true;
                result = parseType1HintDataV2(h, cur->children);
                h->parseState.type1HintDataV2 = false;
            }
        }
    }
    return result;
}

static void parseXMLLib(ufoCtx h, xmlNodePtr cur) {
    const char* keyName;
    cur = cur->xmlChildrenNode;
    if (xmlKeyEqual(cur, "dict") && xmlKeyEqual(cur->children, "key")) {
        cur = cur->children;
        while (cur != NULL) {
            keyName = parseXMLKeyName(h, cur);
            cur = cur->next;
            if (setXMLLib(h, cur, keyName) && cur != NULL)
                cur = cur->next;
        }
    }
}

static void parseXMLOutline(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec, abfGlyphCallbacks* glyph_cb ) {
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (xmlKeyEqual(cur, "contour")) {
            parseXMLContour(h, cur, glifRec, glyph_cb);
        } else if (xmlKeyEqual(cur, "component")) {
            parseXMLComponent(h, cur, glifRec, glyph_cb);
        }
        cur = cur->next;
    }
}

static void parseXMLGLIFKey(ufoCtx h, xmlNodePtr cur, unsigned long *unicode, int tag, abfGlyphCallbacks* glyph_cb) {
    GLIF_Rec* glifRec = h->parseState.GLIFInfo.glifRec;
    const char* keyValue;
    xmlAttr *attr = cur->properties;
    if (h->parseState.UFOFile == preParsingGLIF) {      /* called from preParseGLIF */
        if (xmlKeyEqual(cur, "advance")) {
            while (attr != NULL) {
                if (xmlAttrEqual(attr, "width") || xmlAttrEqual(attr, "advance"))
                    setWidth(h, tag, strtolCheck(h, getXmlAttrValue(attr), false, NULL, 10));
                attr = attr->next;
            }
        } else if (xmlKeyEqual(cur, "unicode")) {
            if (xmlAttrEqual(attr, "hex"))
                *unicode = strtoulCheck(h, getXmlAttrValue(attr), false, NULL, 16);
        }
    } else if (h->parseState.UFOFile == parsingGLIF) {  /* called from parseGLIF */
        if (xmlKeyEqual(cur, "outline"))
            parseXMLOutline(h, cur, glifRec, glyph_cb);
        else if (xmlKeyEqual(cur, "anchor"))
            parseXMLAnchor(h, cur, glifRec);
        else if (xmlKeyEqual(cur, "guideline"))
            parseXMLGuideline(h, cur, tag, glyph_cb, glifRec);
    }
    if (xmlKeyEqual(cur, "lib")) {
        /* so nice it's parsed twice.
         (parsed in both preParseGLIF and parseGLIF until these two are merged in the future.) */
        parseXMLLib(h, cur);
    }
}

/* ToDo: add extra warnings for verbose-output*/
static const char* parseXMLKeyName(ufoCtx h, xmlNodePtr cur) {
    if ((xmlStrEqual(cur->name, (const xmlChar *) "key"))) {
        cur = cur->xmlChildrenNode;
        if (cur != NULL) {
            if (xmlStrEqual(cur->name, (const xmlChar *) "text")) {
                return (char*) xmlNodeGetContent(cur);
            } else {
//                message(h, "Warning: Encountered non-text value %s within <key>.", cur->name);
                return NULL;
            }
        } else {
//            message(h, "Warning: Encountered empty <key></key>.");
            return NULL;
        }
    } else {
        if ((xmlStrEqual(cur->name, (const xmlChar *) "advance"))) {
            return "advance";
        }
    }
    return NULL;
}

static void parseXMLArray(ufoCtx h, xmlNodePtr cur) {
    dnaSET_CNT(h->valueArray, 0);
    h->parseState.valueArray = true;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        const char* valueString = parseXMLKeyValue(h, cur);
        if (valueString != NULL)
            *dnaNEXT(h->valueArray) = (char *)valueString;  // XXX dubious cast
        cur = cur->next;
    }
}

static void parseXMLDict(ufoCtx h, xmlNodePtr cur) {
    // go through an XML dict
    const char* keyName;
    cur = cur->xmlChildrenNode;

    if (h->parseState.FDArray) {
        h->parseState.GLIFInfo.currentiFD = h->parseState.GLIFInfo.currentiFD + 1;
        h->top.FDArray.cnt = h->top.FDArray.cnt + 1;
        if (h->top.FDArray.cnt > FDArrayInitSize) {  // Memory needs reallocation
            reallocFDArray(h);
        }
        abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
        abfInitFontDict(fd);
    } else if (h->parseState.hintSetListArray)
        dnaNEXT(h->hints.hintMasks);
    if (h->parseState.type1HintDataV2) {
        parseType1HintDataV2(h, cur);
        cur = NULL;
    }

    while (cur != NULL) {
        keyName = parseXMLKeyName(h, cur);
        cur = cur->next;
        if (setXMLFileKey(h, keyName, cur) && cur != NULL)
            cur = cur->next;
    }
}

static bool isSimpleKey(xmlNodePtr cur) {
    if (xmlStrEqual(cur->name, (const xmlChar *) "string" ) ||
        xmlStrEqual(cur->name, (const xmlChar *) "integer") ||
        xmlStrEqual(cur->name, (const xmlChar *) "real")    ||
        xmlStrEqual(cur->name, (const xmlChar *) "date")) {
        return true;
    } else {
        return false;
    }
}

static const char* parseXMLKeyValue(ufoCtx h, xmlNodePtr cur) {
    if (cur == NULL) {
        return NULL;
    }
    if (isSimpleKey(cur)) {  /* if string, integer, or real */
        return (char*) xmlNodeGetContent(cur);
    } else if (h->parseState.UFOFile == parsingContentsDflt || h->parseState.UFOFile == parsingContentsAlt) {
        return NULL; /* Only simple keys allowed when parsing contents.plist*/
    } else if (xmlStrEqual(cur->name, (const xmlChar *) "dict")) {
        parseXMLDict(h, cur);
        return NULL;
    } else if (xmlStrEqual(cur->name, (const xmlChar *) "array")) {
        parseXMLArray(h, cur);
        return NULL;  // returning NULL because value is in h->valueArray
    }  else if (xmlStrEqual(cur->name, (const xmlChar *) "true")) {
        return "1";
    }  else if (xmlStrEqual(cur->name, (const xmlChar *) "false")) {
        return "0";
    } else {
        return NULL;
    }
}

static int parseXMLPlistFile(ufoCtx h, xmlNodePtr cur) {
    const char* keyName;
    cur = cur->xmlChildrenNode;

    /* lib.plist: preparse set CID Font flags if has CID-keyed UFO required keys */
    if (h->parseState.UFOFile == parsingLib) {
        if (validCIDKeyedFont(h, cur)) {
            h->top.sup.flags |= ABF_CID_FONT;
            h->top.sup.srcFontType = abfSrcFontTypeUFOCID;
        }
    }

    while (cur != NULL) {
        keyName = parseXMLKeyName(h, cur);
        cur = cur->next;
        if (setXMLFileKey(h, keyName, cur) && cur != NULL)
           cur = cur->next;
    }
    return ufoSuccess;
}

static int parseXMLGlifFile(ufoCtx h, xmlNodePtr cur, int tag, unsigned long *unicode, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec) {
    const char* keyName;
    while (cur != NULL) {
        parseXMLGLIFKey(h, cur, unicode, tag, glyph_cb);
        cur = cur->next;
    }
    if (h->parseState.UFOFile == preParsingGLIF) {
        addCharFromGLIF(h, tag, glifRec, glifRec->glyphName, 0, 0, *unicode);
        h->parseState.GLIFInfo.currentCID = -1;
        h->parseState.GLIFInfo.currentiFD = -1;
    }
    h->flags |= SEEN_END;
    return ufoSuccess;
}

static bool validCIDKeyedFont(ufoCtx h, xmlNodePtr cur){
    /* if required key found, set as true*/
    while (cur != NULL) {
        char* keyName = parseXMLKeyName(h, cur);
        if (strEqual(keyName, "com.adobe.type.ROS")) {
            h->requiredCIDKeyFlag |= CIDKEY_REGISTRY;
            h->requiredCIDKeyFlag |= CIDKEY_ORDERING;
            h->requiredCIDKeyFlag |= CIDKEY_SUPPLEMENT;
        } else if (strEqual(keyName, "com.adobe.type.postscriptFDArray"))
            h->requiredCIDKeyFlag |= CIDKEY_FDARRAY;
        else if (strEqual(keyName, "postscriptFDArray"))
            h->requiredCIDKeyFlag |= CIDKEY_FDARRAY;
        else if (strEqual(keyName, "com.adobe.type.postscriptCIDMap"))
            h->requiredCIDKeyFlag |= CIDKEY_CIDMAP;
        cur = cur->next;
    }

    /* now check all required keys */
    if (h->requiredCIDKeyFlag == 62)
        return true;
    else
        return false;
}

static bool setLibKey(ufoCtx h, const char* keyName, xmlNodePtr cur){
    abfTopDict* top = &h->top;

    if (strEqual(keyName, "com.adobe.type.postscriptFDArray") || strEqual(keyName, "postscriptFDArray"))
     return parseFontInfoFDArray(h, cur);
    else if (strEqual(keyName, "PrivateDict"))
     return parseFontInfoPrivateDict(h, cur);
    else if (strEqual(keyName, "com.adobe.type.postscriptCIDMap"))
     return parseCIDMap(h, cur);

    char* keyValue = parseXMLKeyValue(h, cur);
    if (!keyValueParseable(h, cur, keyValue, keyName))
        return false;

    if (strEqual(keyName, "public.glyphOrder"))
       setGlifOrderArray(h, keyName);
    else if (strEqual(keyName, "com.adobe.type.CIDFontName"))
        top->cid.CIDFontName.ptr = copyStr(h, keyValue);
    else if (strEqual(keyName, "com.adobe.type.ROS")) {
        setROSKeyValue(h, top, keyValue);
    } else if (setFontInfoFD(h, keyName, keyValue))
        return true;
    else if (setFontInfoPD(h, keyName, keyValue))
        return true;
    else if (setFontInfoTopKey(h, keyName, keyValue))
        return true;
    else
        return false;

    if ((h->top.sup.flags & ABF_CID_FONT) &&
        top->cid.CIDFontName.ptr == NULL &&
        h->top.FDArray.array[0].FontName.ptr != NULL) {
        /* derive name from fontinfo.plist familyName-styleName */
        top->cid.CIDFontName.ptr = copyStr(h, h->top.FDArray.array[0].FontName.ptr);
    }
    return true;
}

static bool setGroupsKey(ufoCtx h, char* keyName, xmlNodePtr cur) {
    abfTopDict* top = &h->top;

    char* keyValue = parseXMLKeyValue(h, cur);
    if (!keyValueParseable(h, cur, keyValue, keyName))
        return false;

    char* substr = memNew(h, sizeof(char) * 15);
    memcpy(substr, keyName, 14);
    substr[14] = '\0';
    if (strEqual(substr, "FDArraySelect."))
        parseFDArraySelectGroup(h, keyName, cur);
    memFree(h, substr);
    return true;
}

static int parseFontInfo(ufoCtx h) {
    const char* filetype = "plist";
    h->parseState.GLIFInfo.currentiFD = 0;
    h->parseState.UFOFile = parsingFontInfo;

    h->src.next = h->mark = NULL;
    h->cb.stm.clientFileName = "fontinfo.plist";
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0)) {
        message(h, "Warning: Unable to open fontinfo.plist in source UFO font. No PostScript FontDict values are specified. \n");
        fixUnsetDictValues(h);
        return ufoSuccess;
    }

    dnaSET_CNT(h->valueArray, 0);

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLPlistFile(h, cur);

    fixUnsetDictValues(h);
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    return parsingSuccess;
}

static bool parseFontInfoFDArray(ufoCtx h, xmlNodePtr cur) {
    int version;
    float subVersion;

    char* warnMsg = "In lib.plist: Could not find parseable CIDFontVersion number.";
    h->top.FDArray.array = memNew(h, FDArrayInitSize *sizeof(abfFontDict));
    if (h->top.version.ptr != NULL) {
        version = strtodCheck(h, h->top.version.ptr, true, warnMsg);
        subVersion = strtodCheck(h, &h->top.version.ptr[2], true, warnMsg);
        h->top.cid.CIDFontVersion = (version % 10) + ((float) subVersion/1000);
    }
    abfInitFontDict(h->top.FDArray.array);
    h->parseState.FDArray = true;
    h->parseState.GLIFInfo.currentiFD = -1;
    parseXMLKeyValue(h, cur);
    h->parseState.FDArray = false;
    return true;
}

/* set each GLIF_REC's iFD based on the FDArraySelect groups*/
static bool parseFDArraySelectGroup(ufoCtx h, char* FDArraySelectKeyName, xmlNodePtr cur) {
    GLIF_Rec *g;
    char* errMsg;

    if (!xmlStrEqual((cur)->name, (const xmlChar *) "array"))
        return false;

    errMsg = concatStrs(h, "In groups.plist: expected FDArray index number but could not find parseable number in FDArraySelect group: ", FDArraySelectKeyName);
    int FDIndexInt = strtolCheck(h, FDArraySelectKeyName + 14, true, errMsg, 10);
    memFree(h, errMsg);

    /* check if FDArray is defined */
    if (h->top.FDArray.cnt - 1 <= FDIndexInt)
        fatal(h, ufoErrParseFail, "In groups.plist: FDict referenced in FDSelect Group %s is not defined at expected FDArray index %i.", FDArraySelectKeyName, FDIndexInt);

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        char* glyphName = parseXMLKeyValue(h, cur);
        g = findGLIFRecByName(h, glyphName);
        if (g != NULL) {
            if (FDIndexInt >= h->top.FDArray.cnt - 1) {  /* one cnt reserved for default FDArray at this point */
//                message(h, "Glyph %s is assigned to font dict index %d which is not defined. Will be assigned to default font dict at index 0 instead.\n", glyphName, FDIndexInt);
                FDIndexInt  = 0;
            }
            g->iFD = FDIndexInt;
        }
        cur = cur->next;
    }
    return true;
}

static long strtolCheck(ufoCtx h, char* keyValue, bool fail, char* msg, int base) {
    char* checkPtr;
    errno = 0;
    long val = strtol(keyValue, &checkPtr, base);
    if (keyValue != checkPtr && errno == 0)
        return val;
    else {
        if (fail)
            fatal(h, ufoErrParse, msg);
        else if (msg)
            message(h, msg);
        return 0;
    }
}

static unsigned long strtoulCheck(ufoCtx h, char* keyValue, bool fail, char* msg, int base) {
    char* checkPtr;
    errno = 0;
    unsigned long val = strtoul(keyValue, &checkPtr, base);
    if (keyValue != checkPtr && errno == 0)
        return val;
    else {
        if (fail)
            fatal(h, ufoErrParse, msg);
        else if (msg)
            message(h, msg);
        return 0;
    }
}

static double strtodCheck(ufoCtx h, char* keyValue, bool fail, char* msg) {
    char* checkPtr;
    errno = 0;
    double val = strtod(keyValue, &checkPtr);
    if (keyValue != checkPtr && errno == 0)
        return val;
    else {
        if (fail)
            fatal(h, ufoErrParse, msg);
        else if (msg)
            message(h, msg);
        return 0;
    }
}

static bool parseCIDMap(ufoCtx h, xmlNodePtr cur) {
    h->parseState.CIDMap = true;
    GLIF_Rec *g;
    char* ptr;
    int cidInt;
    char* errMsg;

    if (!xmlStrEqual((cur)->name, (const xmlChar *) "dict"))
        return false;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        char* keyName = parseXMLKeyName(h, cur);
        if (keyName != NULL) {
            g = addNewGLIFRec(h, keyName, NULL);
            if (g == NULL)
                continue;
            cur = cur->next;
            if (cur == NULL)
                continue;
            char* cid = parseXMLKeyValue(h, cur);
            if (cid != NULL) {
                errMsg = concatStrs(h, "In lib.plist postscriptCIDMap, expected cid number but could not find parseable number for glyph: ", keyName);
                g->cid = strtolCheck(h, cid, true, errMsg, 10);
                if (g->cid > h->parseState.GLIFInfo.CIDCount) {
                    h->parseState.GLIFInfo.CIDCount = (long)g->cid + 1;
                }
                memFree(h, errMsg);
            }
        }
        cur = cur->next;
    }
    h->parseState.CIDMap = false;
    h->top.cid.CIDCount = h->parseState.GLIFInfo.CIDCount;
    return true;
}

static bool parseFontInfoPrivateDict(ufoCtx h, xmlNodePtr cur) {
    bool prevFDArrayState = h->parseState.FDArray;
    h->parseState.FDArray = false;  // this is only set when parsing root of FDArray, not sub-dicts within a dict
    parseXMLKeyValue(h, cur);
    h->parseState.FDArray = prevFDArrayState;
    return true;
}

static void setFontInfoTrademark(ufoCtx h, const char* keyValue) {
    abfTopDict* top = &h->top;
    char* copySymbol;
    top->Notice.ptr = keyValue;
    /* look for the (c) symbol U+00A9, which is 0xC2, 0xA9 in UTF-8 */
    // XXX suspicous cast
    copySymbol = (char *) strstr(keyValue, "\xC2\xA9");
    if (copySymbol != NULL) {
        /* if there is a copyright symbol (U+00A9),
           replace it with the word "Copyright" */
        char* cpy = "Copyright";
        char* newString = (char *) memNew(h, strlen(cpy) + strlen(keyValue) + 2);
        /* set the 0xC2 to NULL to terminate the left side of the string */
        *copySymbol = '\0';
        /* use copySymbol + 2 to skip the NULL and the 0xA9
           to get the right side of the string */
        sprintf(newString, "%s%s%s", keyValue, "Copyright", copySymbol + 2);
        top->Notice.ptr = newString;
    }
}

static void setFontInfoVersionMajor(ufoCtx h, const char* keyValue) {
    abfTopDict* top = &h->top;

    if (top->version.ptr == NULL)
        top->version.ptr = keyValue;
    else {
        char* newString = (char *) memNew(h, strlen(top->version.ptr) + strlen(keyValue) + 2);
        sprintf(newString, "%s.%s", keyValue, top->version.ptr);
        // XXX might leak here
        // memFree(h, top->version.ptr);
        top->version.ptr = newString;
    }
}

static void setFontIndoVersionMinor(ufoCtx h, const char* keyValue) {
    abfTopDict* top = &h->top;

    if (top->version.ptr == NULL)
        top->version.ptr = keyValue;
    else {
        char* newString = (char *)memNew(h, strlen(top->version.ptr) + strlen(keyValue) + 2);
        sprintf(newString, "%s.%s", top->version.ptr, keyValue);
        // XXX might leak here
        // memFree(h, top->version.ptr);
        top->version.ptr = newString;
    }
}

static void setFontInfoUnitsPerEm(ufoCtx h, const char* keyValue) {
    char* errMsg;
    errMsg = concatStrs(h, "In fontinfo.plist: encountered unparseable number for UnitsPerEm", keyValue);
    double ppem = strtodCheck(h, keyValue, true, errMsg);
    abfTopDict* top = &h->top;
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;

    top->sup.UnitsPerEm = (int)ppem;
    if (fd->FontMatrix.cnt == 0) {
        fd->FontMatrix.cnt = 6;
        fd->FontMatrix.array[0] = (float)(1.0 / ppem);
        fd->FontMatrix.array[1] = 0;
        fd->FontMatrix.array[2] = 0;
        fd->FontMatrix.array[3] = (float)(1.0 / ppem);
        fd->FontMatrix.array[4] = 0;
        fd->FontMatrix.array[5] = 0;
    }
}

static void setFontInfoStdHW(ufoCtx h, const char* keyName,
                             const char* keyValue) {
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
    abfPrivateDict* pd = &fd->Private;

    if (keyValue != NULL) {
       pd->StdHW = (float)strtodCheck(h, keyValue, false, NULL);
    } else {
       pd->StdHW = (float)strtodCheck(h, h->valueArray.array[0], false, NULL);
       freeValueArray(h);
    }
}

static void setFontInfoStdVW(ufoCtx h, const char* keyName,
                             const char* keyValue) {
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
    abfPrivateDict* pd = &fd->Private;

    if (keyValue != NULL) {
        pd->StdVW = (float)strtodCheck(h, keyValue, false, NULL);
    } else {
        pd->StdVW = (float)strtodCheck(h, h->valueArray.array[0], false, NULL);
        freeValueArray(h);
    }
}

static bool setFontInfoFD(ufoCtx h, const char* keyName,
                          const char* keyValue) {
    abfTopDict* top = &h->top;
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
    abfFontMatrix* fontMatrix;

    if (strEqual(keyName, "FontName")) {
       fd->FontName.ptr = keyValue;
    } else if (strEqual(keyName, "unitsPerEm")) {
       setFontInfoUnitsPerEm(h, keyValue);
    } else if (strEqual(keyName, "postscriptFontName")) {
       fd->FontName.ptr = keyValue;
    } else if (strEqual(keyName, "PaintType")) {
       fd->PaintType = strtolCheck(h, keyValue, false, NULL, 10);
    } else if (strEqual(keyName, "FontMatrix")) {
       fontMatrix = &fd->FontMatrix;
       setFontMatrix(h, fontMatrix, 6);
    } else
       return false;
    return true;
}

static bool setFontInfoPD(ufoCtx h, const char* keyName,
                          const char* keyValue) {
    abfFontDict* fd = h->top.FDArray.array + h->parseState.GLIFInfo.currentiFD;
    abfPrivateDict* pd = &fd->Private;
    BluesArray* bluesArray;

    if (strEqual(keyName, "postscriptBlueFuzz")) {
        pd->BlueFuzz = (float)strtodCheck(h, keyValue, false, NULL);
    } else if (strEqual(keyName, "postscriptBlueShift")) {
        pd->BlueShift = (float)strtodCheck(h, keyValue, false, NULL);
    } else if (strEqual(keyName, "postscriptBlueScale")) {
        pd->BlueScale = (float)strtodCheck(h, keyValue, false, NULL);
    } else if (strEqual(keyName, "postscriptForceBold")) {
        pd->ForceBold = strtolCheck(h, keyValue, false, NULL, 10);
    } else if (strEqual(keyName, "postscriptBlueValues")) {
        bluesArray = (BluesArray*)&pd->BlueValues;
        setBluesArrayValue(h, bluesArray, 14, keyName);
    } else if (strEqual(keyName, "postscriptOtherBlues")) {
        bluesArray = (BluesArray*)&pd->OtherBlues;
        setBluesArrayValue(h, bluesArray, 10, keyName);
    } else if (strEqual(keyName, "postscriptFamilyBlues")) {
        bluesArray = (BluesArray*)&pd->FamilyBlues;
        setBluesArrayValue(h, bluesArray, 14, keyName);
    } else if (strEqual(keyName, "postscriptFamilyOtherBlues")) {
        bluesArray = (BluesArray*)&pd->FamilyOtherBlues;
        setBluesArrayValue(h, bluesArray, 10, keyName);
    } else if (strEqual(keyName, "postscriptStdHW")) {
        setFontInfoStdHW(h, keyName, keyValue);
    } else if (strEqual(keyName, "postscriptStdVW")) {
        setFontInfoStdVW(h, keyName, keyValue);
    } else if (strEqual(keyName, "postscriptStemSnapH")) {
        bluesArray = (BluesArray*)&pd->StemSnapH;
        setBluesArrayValue(h, bluesArray, 12, keyName);
    } else if (strEqual(keyName, "postscriptStemSnapV")) {
        bluesArray = (BluesArray*)&pd->StemSnapV;
        setBluesArrayValue(h, bluesArray, 12, keyName);
    } else if (strEqual(keyName, "LanguageGroup")) {
        pd->LanguageGroup = (float)strtodCheck(h, keyValue, false, NULL);
        h->parseKeyName = NULL;
    } else if (strEqual(keyName, "ExpansionFactor")) {
        pd->ExpansionFactor = (float)strtodCheck(h, keyValue, false, NULL);
        h->parseKeyName = NULL;
    } else
        return false;
    return true;
}

static bool setFontInfoTopKey(ufoCtx h, const char* keyName,
                              const char* keyValue) {
    abfTopDict* top = &h->top;

    if (strEqual(keyName, "familyName")) {
        if (top->FamilyName.ptr == NULL)  // we don't re-set this if it was set by "openTypeNamePreferredFamilyName"
            top->FamilyName.ptr = keyValue;
    } else if (strEqual(keyName, "versionMajor"))
        setFontInfoVersionMajor(h, keyValue);
     else if (strEqual(keyName, "versionMinor"))
        setFontIndoVersionMinor(h, keyValue);
     else if (strEqual(keyName, "copyright"))
         top->Copyright.ptr = keyValue;
     else if (strEqual(keyName, "trademark"))
         setFontInfoTrademark(h, keyValue);
     else if (strEqual(keyName, "italicAngle"))
         top->ItalicAngle = (float)strtodCheck(h, keyValue, false, NULL);
     else if (strEqual(keyName, "openTypeNamePreferredFamilyName"))
         top->FamilyName.ptr = keyValue;
     else if (strEqual(keyName, "postscriptFullName"))
         top->FullName.ptr = keyValue;
     else if (strEqual(keyName, "postscriptWeightName"))
         top->Weight.ptr = keyValue;
     else if (strEqual(keyName, "postscriptIsFixedPitch"))
         top->isFixedPitch = strtolCheck(h, keyValue, false, NULL, 10);
     else if (strEqual(keyName, "com.adobe.type.FSType") || strEqual(keyName, "FSType"))
         top->FSType = strtolCheck(h, keyValue, false, NULL, 10);
     else if (strEqual(keyName, "postscriptUnderlinePosition"))
         top->UnderlinePosition = (float)strtodCheck(h, keyValue, false, NULL);
     else if (strEqual(keyName, "postscriptUnderlineThickness"))
         top->UnderlineThickness = (float)strtodCheck(h, keyValue, false, NULL);
     else
         return false;
    return true;
}

static bool setFontInfoKey(ufoCtx h, const char* keyName, xmlNodePtr cur) {
    /* returns false when current key is NULL/ not parseable,
    otherwise returns true */
    bool result;
    if (keyName == NULL){
     return false;
    }
    /* parse dictionaries */
    if (strEqual(keyName, "PrivateDict"))
     return parseFontInfoPrivateDict(h, cur);

    const char* keyValue = parseXMLKeyValue(h, cur);
    if (!keyValueParseable(h, cur, keyValue, keyName))
        result = false;
    else if (setFontInfoFD(h, keyName, keyValue))
        result = true;
    else if (setFontInfoPD(h, keyName, keyValue))
        result = true;
    else if (setFontInfoTopKey(h, keyName, keyValue))
        result = true;
    else
        result = false;

    freeValueArray(h);
    return result;
}

static void setROSKeyValue(ufoCtx h, abfTopDict* top, char* keyValue) {
    char* ROSValues;  /* Registry, Ordering, Supplement */
    char* parsedValue;

    if (keyValue[0] == '-')
        fatal(h, ufoErrKeyValueNULL, "Empty Registry value for com.adobe.type.ROS key. ROS value format should be: registry-ordering-supplement");
    else {
        parsedValue = strtok_r(keyValue, "-", &ROSValues);
        top->cid.Registry.ptr = copyStr(h, parsedValue);
    }
    if (parsedValue == NULL || ROSValues[0] == '-')
        fatal(h, ufoErrKeyValueNULL, "Empty Ordering value for com.adobe.type.ROS key. ROS value format should be: registry-ordering-supplement");
    else {
        parsedValue = strtok_r(ROSValues, "-", &ROSValues);
        top->cid.Ordering.ptr = copyStr(h, parsedValue);
    }
    parsedValue = strtok_r(ROSValues, "-", &ROSValues);
    char* errSupplementMsg = "Empty Supplement value for com.adobe.type.ROS key. ROS value format should be: registry-ordering-supplement";
    if (parsedValue != NULL)
        top->cid.Supplement = strtolCheck(h, parsedValue, true, errSupplementMsg, 10);
    else
        fatal(h, ufoErrKeyValueNULL, errSupplementMsg);
}

static int parseUFO(ufoCtx h) {
    /* This does a first pass through the font, loading the glyph name/ID and the path to each glyph file.
     Open the UFO fontinfo.plist, and extract the PS info
     Open the glyphs/contents.plist file, and extract any glyphs that are not in the glyphs.ac/contents.plist file
     */
    h->hasAltLayer = true; /* assume we have one until we find we don't */

    int retVal = parseFontInfo(h);
    if (retVal == ufoSuccess)
        retVal = parseLibPlist(h); /* return value was being ignored prior to 15 June 2018, not sure why -- CJC */
    if (retVal == ufoSuccess)
        retVal = parseGroups(h);
    if (retVal == ufoSuccess)
        retVal = parseGlyphList(h, false); /* parse default layer */
    if (retVal == ufoSuccess)
        retVal = parseGlyphList(h, true); /* parse alternate layer */
    if (retVal == ufoSuccess)
        retVal = preParseGLIFS(h);
    return retVal;
}

static int parseXMLGuideline(ufoCtx h, xmlNodePtr cur, int tag, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec) {
    token* tk;
    int prevState = outlineStart;
    int result = ufoSuccess;
    guidelineRec* guideline;

    guideline = (guidelineRec *)memNew(h, sizeof(guidelineRec));
    memset(guideline, 0, sizeof(guidelineRec));

    xmlAttr *attr = cur->properties;
    while (attr != NULL) {
        if (xmlAttrEqual(attr, "x"))
            guideline->x = (float)strtodCheck(h, getXmlAttrValue(attr), false, NULL);
        else if (xmlAttrEqual(attr, "y"))
            guideline->y = (float)strtodCheck(h, getXmlAttrValue(attr), false, NULL);
        else if (xmlAttrEqual(attr, "angle"))
            guideline->angle = (float)strtodCheck(h, getXmlAttrValue(attr), false, NULL);
        else if (xmlAttrEqual(attr, "name")) {
            char* temp = getXmlAttrValue(attr);
            guideline->name = copyStr(h, temp);
        } else if (xmlAttrEqual(attr, "color")) {
            char* temp = getXmlAttrValue(attr);
            guideline->color = copyStr(h, temp);
        } else if (xmlAttrEqual(attr, "identifier")) {
            char* temp = getXmlAttrValue(attr);
            guideline->identifier = copyStr(h, temp);
        }
        attr = attr->next;
    }
    memFree(h, guideline);
    /* ToDo: instead of freeing it, append the guideline record to a
             dynamic array in, or associated with, the glyph record */
    return result;
}

static int parseXMLAnchor(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec) {
    int result = ufoSuccess;
    anchorRec* anchor;

    anchor = (anchorRec *)memNew(h, sizeof(anchorRec));
    memset(anchor, 0, sizeof(anchorRec));

    xmlAttr *attr = cur->properties;
    while (attr != NULL) {
        if (xmlAttrEqual(attr, "x"))
            anchor->x = (float)atof(getXmlAttrValue(attr));
        else if (xmlAttrEqual(attr, "y"))
            anchor->y = (float)atof(getXmlAttrValue(attr));
        else if (xmlAttrEqual(attr, "name")) {
            char* temp = getXmlAttrValue(attr);
            anchor->name = copyStr(h, temp);
        } else if (xmlAttrEqual(attr, "color")) {
            char* temp = getXmlAttrValue(attr);
            anchor->color = copyStr(h, temp);
        } else if (xmlAttrEqual(attr, "identifier")) {
            char* temp = getXmlAttrValue(attr);
            anchor->identifier = copyStr(h, temp);
        }
        attr = attr->next;
    }
    memFree(h, anchor);
    /* ToDo: instead of freeing it, append the anchor record to a
             dynamic array in, or associated with, the glyph record */
    return result;
}

static int parseNote(ufoCtx h, GLIF_Rec* glifRec, int state) {
    int result = ufoSuccess;

    while (1) {
        token* tk;
        tk = getToken(h, state);
        if (tk == NULL) {
            fatal(h, ufoErrParse, "Encountered end of buffer before end of note. Glyph %s. Context: %s\n", glifRec->glifFilePath, getBufferContextPtr(h));
        } else if (tokenEqualStr(tk, "</note>")) {
            break;
        }
    }
    return result;
}

static int setPointKeyValue(ufoCtx h, abfGlyphCallbacks* glyph_cb, float x, float y, int type, char* pointName){
    Transform* transform = h->parseState.GLIFInfo.transform;
    int result = ufoSuccess;
    if ((transform != NULL) && (!transform->isDefault)) {
        float* mtx = (float*)transform->mtx;
        float xnew = mtx[0] * x + mtx[2] * y + mtx[4];
        y = mtx[1] * x + mtx[3] * y + mtx[5];
        x = xnew;
    }
    CHKOFLOW(2);
    PUSH(x);
    PUSH(y);
    switch (type) {
        case 0:
            if (pointName != NULL)
                h->hints.pointName = pointName; /* we need to save the point name for later reference only for off-curve points, to be passed into the curve OpRec */
            break;
        case 1: {
            doOp_mt(h, glyph_cb, pointName);
            h->hints.pointName = NULL;
            break;
        }
        case 2: {
            doOp_dt(h, glyph_cb, pointName);
            h->hints.pointName = NULL;
            break;
        }
        case 3: {
            /* The hint set which precedes a curve is referenced in the point name for the first point of the curve.
            This is currently stored in h->hints.pointName. Save, it, then set h->hints.pointName to NULL.

             A curve has a point name in the final point only when the point is the first point of the contour,
             and the last two points are at the end of the contour. In this case,  h->hints.pointName is always NULL,
             and the hint reference is applied before the move-to of the contour.*/

            if (pointName != NULL)
                doOp_ct(h, glyph_cb, pointName);
            else if (h->hints.pointName != NULL)
                doOp_ct(h, glyph_cb, h->hints.pointName);
            else
                doOp_ct(h, glyph_cb, NULL);
            h->hints.pointName = NULL; /* don't free h->hints.pointName; it is now references from the opRec. */
            break;
        }
    }
    return result;
}

static int parseXMLPoint(ufoCtx h, xmlNodePtr cur, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec, int state) {
    float x = 0;
    float y = 0;
    int type = 0;
    char* pointName = NULL;
    int result = ufoSuccess;

    xmlAttr *attr = cur->properties;
    while (attr != NULL) {
        if (xmlAttrEqual(attr, "x"))
            x = (float)strtodCheck(h, getXmlAttrValue(attr), false, NULL);
        else if (xmlAttrEqual(attr, "y"))
            y = (float)strtodCheck(h, getXmlAttrValue(attr), false, NULL);
        else if (xmlAttrEqual(attr, "name")) {  // needs testing
            char* temp = getXmlAttrValue(attr);
            pointName = copyStr(h, temp);
        } else if (xmlAttrEqual(attr, "type")) {
            char* strType = getXmlAttrValue(attr);
            if (strEqual(strType, "move"))
                type = 1;
            else if (strEqual(strType, "line"))
                type = 2;
            else if (strEqual(strType, "curve"))
                type = 3;
            else if (strEqual(strType, "offcurve"))
                continue;  // type is already set to 0. x and y will get pushed on the stack, and no other operation will happen.
            else {
                fatal(h, ufoErrParse, "Encountered unsupported point type '%s' in glyph '%s'.\n", strType, glifRec->glyphName);
                result = ufoErrParse;
                break;
            }
        }
        attr = attr->next;
    }
    result = setPointKeyValue(h, glyph_cb, x, y, type, pointName);
    return result;
}

static int parseGLIF(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks* glyph_cb, Transform* transform);


static int setParseXMLComponentValue(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks* glyph_cb, GLIF_Rec* glifRec, Transform* transform, Transform localTransform, Transform *newTransform, int result) {
    Transform concatTransform;

    /* Figure out transforms */
    if (transform == NULL) {
        if (localTransform.isDefault)
            newTransform = NULL;
        else
            newTransform = &localTransform;
    } else {
        if (localTransform.isDefault)
            newTransform = transform;
        else if (transform->isOffsetOnly == 1) {
            localTransform.mtx[4] += transform->mtx[4];
            localTransform.mtx[5] += transform->mtx[5];
            newTransform = &localTransform;
            // localTransform.isOffsetOnly remains unchanged
        } else {
            newTransform = &concatTransform;
            matMult(concatTransform.mtx, localTransform.mtx, transform->mtx);
            concatTransform.isOffsetOnly = 0;
            concatTransform.isDefault = 0;
        }
    }
    result = parseGLIF(h, gi, glyph_cb, newTransform);
    h->cb.stm.seek(&h->cb.stm, h->stm.src, 0);
    fillbuf(h, 0);
    h->stack.flags &= ~PARSE_END;
    return result;
}

static int parseXMLContour(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec, abfGlyphCallbacks* glyph_cb){
    long contourStartOpIndex = h->data.opList.cnt;
    h->stack.flags |= PARSE_PATH;
    h->stack.flags &= ~((unsigned long)PARSE_SEEN_MOVETO);
    xmlNodePtr curChild = cur->xmlChildrenNode;
    int result = ufoSuccess;
    while (curChild != NULL){
        if (xmlKeyEqual(curChild, "point")){
            result = parseXMLPoint(h, curChild, glyph_cb, glifRec, 2);
        }
        curChild = curChild->next;
    }
    if (h->data.opList.cnt > 1) {
        OpRec* firstOpRec = &h->data.opList.array[contourStartOpIndex];
        /* Now we need to fix up the OpList. In GLIF, there is usually no explicit start point, as the format expresses
         the path segments as a complete closed path, with no explicit start point.

         I use the first path operator end point as the start point, and convert this first operator to move-to.
         If the first path operator was a line-to, then I do not add it to the end of the op-list, as it should become an implicit rather than explicit close path.
         If it is a curve, then I need to add it to the oplist as the final path segment. */
        if (firstOpRec->opType == linetoType) {
            /* I just need to convert this to a move-to. The final line-to will become implicit */
            firstOpRec->opType = movetoType;
        } else if (firstOpRec->opType == curvetoType) {
            /* The first two points for the curve should be on the stack.
           If there is a hint set for this last curve, it was part of the first point element for the curve
           and is currently stored in  h->hints.pointName.
           If there is a pointName in the firstOpRec, it belongs in the first move-to.
            */
            /* Add the final curve-to to the opList.*/
            CHKOFLOW(2);
            PUSH(firstOpRec->coords[0]);
            PUSH(firstOpRec->coords[1]);
            doOp_ct(h, glyph_cb, h->hints.pointName); /* adds a new curve opRec to the op list, using the point name (if any) of the first point of the curve.  */

            /* doOp_ct can resize the opList array, invalidating the firstOpRec pointer */
            firstOpRec = &h->data.opList.array[contourStartOpIndex];
            firstOpRec->opType = movetoType;

            h->hints.pointName = NULL;
        }
    }
    return result;
}


static int parseXMLComponent(ufoCtx h, xmlNodePtr cur, GLIF_Rec* glifRec, abfGlyphCallbacks* glyph_cb) {
    Transform* transform = h->parseState.GLIFInfo.transform;
    int result = ufoSuccess;
    abfGlyphInfo* gi = NULL;
    Transform localTransform;        // transform specified in component
    Transform* newTransform = NULL;  // result of multiplication with existing transform.
    float val;

    setTransformMtx(&localTransform, 1.0, 0, 0, 1.0, 0, 0, 1, 1);

    xmlAttr *attr = cur->properties;
    while (attr != NULL) {
        if (xmlAttrEqual(attr, "base")){
            size_t index;
            char* glyphName = getXmlAttrValue(attr);
            if (!ctuLookup(glyphName, h->chars.byName.array, h->chars.byName.cnt,
                           sizeof(h->chars.byName.array[0]), postMatchChar, &index, h)) {
                fatal(h,  ufoErrNoGlyph, "Encountered glyph component reference '%s' with an empty file path.", glifRec->glifFilePath);
            }
            gi = &h->chars.index.array[h->chars.byName.array[index]];
        } else {
            val = (float)atof(getXmlAttrValue(attr));
            if (xmlAttrEqual(attr, "xScale"))
                setTransformValue(&localTransform, val, 0);
            else if (xmlAttrEqual(attr, "yScale"))
                setTransformValue(&localTransform, val, 3);
            else if (xmlAttrEqual(attr, "xyScale"))
                setTransformValue(&localTransform, val, 1);
            else if (xmlAttrEqual(attr, "yxScale"))
                setTransformValue(&localTransform, val, 2);
            else if (xmlAttrEqual(attr, "xOffset"))
                setTransformValue(&localTransform, val, 4);
            else if (xmlAttrEqual(attr, "yOffset"))
                setTransformValue(&localTransform, val, 5);
        }
        attr = attr->next;
    }
    result = setParseXMLComponentValue(h, gi, glyph_cb, glifRec, transform, localTransform, newTransform, result);
    return result;
}

static void doOpList(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks* glyph_cb) {
    int i = 0;

    /* At this point, we have parsed the outline data, and build a list of pointNameRecs, mapping point names to point index.
    We have also parsed the hint data, and have a a list of hint masks and flexCurves by point name.
    I then step through all the points. We first play back the hintset, if there is one for that pointName.
    */

    if (h->data.opList.cnt < 2)  // if there is only 1 op, it is a move to. We skip - these are GLIF format 1 anchors.
        return;

    while (i < h->data.opList.cnt) {
        int isFlex = 0;
        OpRec* opRec = &h->data.opList.array[i];

        if (opRec->pointName != NULL) {
            /* We might have a hint set.  play back hints, if any, and if we are being called from a glyph_cb that supports hints ( the metrics cb doesn't) */
            if ((glyph_cb->stem != NULL) && (h->hints.hintMasks.cnt > 0)) {
                int m;

                for (m = 0; m < h->hints.hintMasks.cnt; m++) {
                    HintMask* hintMask = &h->hints.hintMasks.array[m];
                    if ((strEqual(hintMask->pointName, opRec->pointName)) && (hintMask->maskStems.cnt > 0)) {
                        int j = 0;
                        while (j < hintMask->maskStems.cnt) {
                            StemHint* stemHint = &hintMask->maskStems.array[j++];
                            glyph_cb->stem(glyph_cb, stemHint->flags, stemHint->edge, stemHint->edge + stemHint->width);
                        }
                        break;
                    }
                }
            }

            if (opRec->opType == curvetoType) {
                /* check for flex */

                if (h->hints.flexOpList.cnt > 0) {
                    int m;

                    for (m = 0; m < h->hints.flexOpList.cnt; m++) {
                        char* flexPointName = h->hints.flexOpList.array[m];
                        if (0 == strcmp(flexPointName, opRec->pointName)) {
                            isFlex = 1;
                            break;
                        }
                    }
                }
            }
        }

        /* if the opIndex matches an opIndexFlex value, send a flex op instead. */
        switch (opRec->opType) {
            case movetoType: {
                OpRec* nextOpRec;

                /* UFO format fonts can have paths with a single move-to, used as anchor points during design. Omit these. */
                if ((i + 1) == h->data.opList.cnt)
                    break;
                nextOpRec = &h->data.opList.array[i + 1];
                if (nextOpRec->opType == movetoType)
                    break;
                glyph_cb->move(glyph_cb, opRec->coords[0], opRec->coords[1]);
                break;
            }
            case linetoType: {
                glyph_cb->line(glyph_cb, opRec->coords[0], opRec->coords[1]);
                break;
            }
            case curvetoType: {
                if ((glyph_cb->stem != NULL) && isFlex) {
                    OpRec* opRec1 = &h->data.opList.array[i + 1];
                    /* despite the T1 reference guide, autohint has always emitted a fixed flex depth of 50. */
                    glyph_cb->flex(glyph_cb, 50,
                                   opRec->coords[0], opRec->coords[1],
                                   opRec->coords[2], opRec->coords[3],
                                   opRec->coords[4], opRec->coords[5],
                                   opRec1->coords[0], opRec1->coords[1],
                                   opRec1->coords[2], opRec1->coords[3],
                                   opRec1->coords[4], opRec1->coords[5]);
                    i++; /* we skip the next op, since we just passed through the flex callback. */
                    break;
                }

                glyph_cb->curve(glyph_cb,
                                opRec->coords[0], opRec->coords[1],
                                opRec->coords[2], opRec->coords[3],
                                opRec->coords[4], opRec->coords[5]);
                break;
            }
            case closepathType: {
                /* do nothing */
                /* prior to 15 June 2018 there was no explicit case for closepathType -- CJC */
                break;
            }
        }
        i++;
    }
}

static int parseStemV2(ufoCtx h, HintMask* curHintMask, int stemFlags, char* stemValues) {
    int result = ufoSuccess;
    float pos = 0;
    float width = 0;
    StemHint* stem;
    Transform* transform = h->parseState.GLIFInfo.transform;
    int isH = !(stemFlags & ABF_VERT_STEM);

    if ((transform != NULL) && (!transform->isOffsetOnly)) {
        /* We omit stems if the stems are being skewed */
        if (isH && (transform->mtx[2] != 0.0))
            return result;
        if ((!isH) && (transform->mtx[1] != 0.0))
            return result;
    }

    // ToDo: Add warning in verbose mode if its wrong hint type (stem instead of stem3)
    pos = (float) strtod(stemValues, &stemValues);
    width = (float) strtod(stemValues, &stemValues);

    if ((transform != NULL) && (!transform->isDefault)) {
        float* mtx = (float*)transform->mtx;
        {  // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH) {
                pos = mtx[3] * pos + mtx[5];
                width = mtx[3] * width;
            } else {
                pos = mtx[0] * pos + mtx[4];
                width = mtx[0] * width;
            }
        }
    }

    stem = dnaNEXT(curHintMask->maskStems);
    stem->edge = pos;
    stem->width = width;
    stem->flags = stemFlags;
    return result;
}

static int parseStem3V2(ufoCtx h, HintMask* curHintMask, int stemFlags, char* stemValues) {
    int result = ufoSuccess;
    float coords[] = {0, 0, 0, 0, 0, 0};
    int isH = !(stemFlags & ABF_VERT_STEM);
    int count = 0;
    Transform* transform = h->parseState.GLIFInfo.transform;

    while (count < 6 && stemValues != NULL) {
        float stem = (float) strtod(stemValues, &stemValues);
        coords[count] = stem;
        count++;
    }

    // ToDo: Add warning message if amount of stems not correct
    if (count != 6)
        return -1;

    if ((transform != NULL) && (!transform->isDefault)) {
        float* mtx = (float*)transform->mtx;
        count = 0;
        while (count < 6) {  // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH) {
                coords[count] = mtx[3] * coords[count] + mtx[5];
                count++;
                coords[count] = mtx[3] * coords[count];
                count++;
            } else {
                coords[count] = mtx[0] * coords[count] + mtx[4];
                count++;
                coords[count] = mtx[0] * coords[count];
                count++;
            }
        }
    }

    // Add the stems
    count = 0;
    while (count < 6) {
        StemHint* stem;
        stem = dnaNEXT(curHintMask->maskStems);
        stem->flags = stemFlags;
        if ((count == 0) && (stemFlags & ABF_NEW_HINTS))
            stemFlags &= ~ABF_NEW_HINTS;
        stem->edge = coords[count++];
        stem->width = coords[count++];
    }
    return result;
}

static bool setStemsArrayValue(ufoCtx h, HintMask* curHintMask) {
    StemHint* stem;
    float pos = 0;
    float width = 0;
    int stemFlags = 0;
    int count = 0;

    if (h->hints.hintMasks.cnt > 1)
        stemFlags |= ABF_NEW_HINTS;
    int i = 0;
    if (h->valueArray.cnt == 0)
        return false;
    while ((i < h->valueArray.cnt)) {
        char* stemValues;
        char* stemType = strtok_r(h->valueArray.array[i], " ", &stemValues);
        if (strEqual(stemType, "hstem")) {
            parseStemV2(h, curHintMask, stemFlags, stemValues);
        } else if (strEqual(stemType, "hstem3")) {
            stemFlags |= ABF_STEM3_STEM;
            parseStem3V2(h, curHintMask, stemFlags, stemValues);
        } else if (strEqual(stemType, "vstem")) {
            stemFlags |= ABF_VERT_STEM;
            parseStemV2(h, curHintMask, stemFlags, stemValues);
        } else if (strEqual(stemType, "vstem3")) {
            stemFlags |= ABF_VERT_STEM;
            stemFlags |= ABF_STEM3_STEM;
            parseStem3V2(h, curHintMask, stemFlags, stemValues);
        }
        stemFlags = 0;
        i++;
    }
    freeValueArray(h);
    return true;
}

static bool parseHintSetListV2(ufoCtx h, xmlNodePtr cur) {
    int result = false;

    h->parseState.hintSetListArray = true;
    if (parseXMLKeyValue(h, cur) == NULL && !h->parseState.valueArray)
        result = false;
    else
        result = true;
    h->parseState.hintSetListArray = false;

    return result;
}

static int parseFlexListV2(ufoCtx h, xmlNodePtr cur) {
    int result = ufoSuccess;

    xmlAttr *attr = cur->properties;
    while (attr != NULL) {
        if (xmlAttrEqual(attr, "hintSetList")) {
            parseHintSetListV2(h, cur);
            continue;
        }
    }

    return result;
}

static bool setFlexListArrayValue(ufoCtx h) {
    int i = 0;
    if (h->valueArray.cnt == 0)
        return false;
    while ((i < h->valueArray.cnt)) {
        char* pointName = copyStr(h, h->valueArray.array[i]);
        *dnaNEXT(h->hints.flexOpList) = pointName;
        i++;
    }
    freeValueArray(h);
    return true;
}

static bool setType1HintDataV2(ufoCtx h, const char* keyName, HintMask* curHintMask, xmlNodePtr cur) {
    if (cur == NULL)
        return false;
    const char* keyValue;
    if (strEqual(keyName, "hintSetList")) {
        return parseHintSetListV2(h, cur);
    } else if (strEqual(keyName, "flexList")) {
        parseXMLKeyValue(h, cur);
        return setFlexListArrayValue(h);
    } else if (strEqual(keyName, "pointTag")) {
        keyValue = parseXMLKeyValue(h, cur);
        if (keyValueParseable(h, cur, keyValue, keyName))
            curHintMask->pointName = copyStr(h, keyValue);
        else
            return false;
    } else if (strEqual(keyName, "stems")) {
        parseXMLKeyValue(h, cur);
        return setStemsArrayValue(h, curHintMask);
    } else
        return false;
    return true;
}

static int parseType1HintDataV2(ufoCtx h, xmlNodePtr cur) {
    const char* keyName;
    int result = ufoSuccess;
    GLIF_Rec* glifRec = h->parseState.GLIFInfo.glifRec;
    HintMask* curHintMask;

    if (h->hints.hintMasks.cnt > 0)
        curHintMask = h->hints.hintMasks.array + (h->hints.hintMasks.cnt - 1);
        /*  get the last hintMask in hintMasks array. The hintMasks array is augmented in parseXMLDict, and then parsed here afterwards. */
    else
        curHintMask = h->hints.hintMasks.array;

    while (cur != NULL) {
        keyName = parseXMLKeyName(h, cur);
        cur = cur->next;
        if (setType1HintDataV2(h, keyName, curHintMask, cur) && cur != NULL)
            cur = cur->next;
    }
    return result;
}

static void skipData(ufoCtx h, GLIF_Rec* glifRec) {
    int state = otherDictKey;
    int prevState = otherDictKey;
    token* tk;
    token* lastToken = NULL;
    dnaDCL(token, tokenList);
    dnaINIT(h->dna, tokenList, 10, 10);

    /* We just saw a third party <lib>/<dict>/<key>xx</key>. Now we want to skip until the following item is consumed. */
    do {
        tk = getToken(h, state);
        if (tk == NULL) {
            fatal(h, ufoErrParse, "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        } else {
            if (tokenEqualStr(tk, "<!--")) {
                prevState = state;
                state = outlineInComment;
            } else if (tokenEqualStr(tk, "-->")) {
                if (state != outlineInComment) {
                    fatal(h, ufoErrParse, "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = prevState;
            } else {
                /* If the token is in the form "<NNN", skip until see  "/>" or ">". If the latter, increment depth, push the token on the stack.
                 If the token is in the form <NNN>, increment depth, push the token on the stack.
                 if the token is in the form </NNN>, then check that this matches the last token on the stack, and decrement depth.
                 when depth is 0, return.
                 */
                if (tk->val[0] == '<') {
                    if (tk->val[1] == '/') {
                        /* An end token! this should match lastToken, which is the last item pushed on the stack.*/
                        if ((lastToken != NULL) && (0 == strncmp(&tk->val[2], &lastToken->val[1], lastToken->length - 1))) {
                            tokenList.cnt--;
                            if (tokenList.cnt == 0)
                                return;
                            lastToken = &tokenList.array[tokenList.cnt - 1];
                        } else {
                            fatal(h, ufoErrParse, "Encountered end element that does not match previous start element. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                        }
                    } else {
                        if (tk->val[tk->length - 1] == '>') { /* A start element */
                            if (tk->val[tk->length - 2] == '/') {
                                /* a token in the form <NNN/>. We consumed the whole thing.*/
                                if (tokenList.cnt == 0)
                                    return;
                                continue;
                            } else {
                                /* Looks like a token in the form <NNN>. Push it on the stack, */
                                lastToken = dnaNEXT(tokenList);
                                *lastToken = *tk;
                                state = otherInElement;
                            }

                        } else {
                            /* An open start element in the form "<NNN". Consume tokens until we see "/>" or ">". */
                            lastToken = dnaNEXT(tokenList);
                            *lastToken = *tk;
                            state = otherInTag;
                        }
                    }
                } else if ((tk->val[0] == '>') && (tk->length == 1)) {
                    if (state != otherInTag) {
                        fatal(h, ufoErrParse, "Encountered '>' while not parsing an element tag. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                    }
                    /* The closing brace for the current element. */
                    state = otherInElement;
                } else if ((tk->val[0] == '/') && (tk->val[1] == '>') && (tk->length == 2)) {
                    /* The end of element */
                    if (state != otherInTag) {
                        fatal(h, ufoErrParse, "Encountered '/>' while not parsing an element tag. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                    }
                    /* The closing brace for the current element. */
                    tokenList.cnt--;
                    if (tokenList.cnt == 0)
                        return;
                    lastToken = &tokenList.array[tokenList.cnt - 1];
                    state = otherInElement;
                }
            }
        }
    } while (!(h->stack.flags & PARSE_END));
}

static int parseGLIF(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks* glyph_cb, Transform* transform) {
    /* The first point in a GLIF outline serves two purposes: it is the start point, but also the end-point.
     We need to convert it to the initial move-to, but we also need to
     re-issue it as the original point type at the end of the path.
     */
    Transform *currentTransform;

    const char* filetype = "glyph";
    h->parseState.UFOFile = parsingGLIF;
    currentTransform = h->parseState.GLIFInfo.transform;
    h->parseState.GLIFInfo.transform = transform;
    int result = ufoSuccess;
    int state = outlineStart;
    int prevState = outlineStart;
    long contourStartOpIndex = 0;

    STI sti = (STI)gi->tag;
    GLIF_Rec* glifRec = &h->data.glifRecs.array[sti];
    h->parseState.GLIFInfo.glifRec = glifRec;

    /* open the file */
    h->src.next = h->mark = NULL;
    h->flags &= ~((unsigned long)SEEN_END);

    h->cb.stm.clientFileName = glifRec->glifFilePath;
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);

    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, gi->sup.begin)) {
        fatal(h, ufoErrSrcStream, "Failed to open the %s glif file.\n", glifRec->glyphName);
    }

    h->stack.cnt = 0;
    h->stack.hintflags = 0;
    h->stack.flags = 0;
    h->hints.pointName = NULL;

    xmlNodePtr cur = parseXMLFile(h, h->cb.stm.clientFileName, filetype);
    int parsingSuccess = parseXMLGlifFile(h, cur, gi->tag, NULL, glyph_cb, glifRec);

    /* An odd exit - didn't see  "</glyph>"  */
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->parseState.GLIFInfo.transform = currentTransform;
    return result;
}

static int readGlyph(ufoCtx h, unsigned short tag, abfGlyphCallbacks* glyph_cb) {
    int result;
    token op_tk;
    abfGlyphInfo* gi;
    long width;
    int i;

    op_tk.type = ufoNotSet;

    gi = &h->chars.index.array[tag];

    /* note that gname.ptr is not stable: it is a pointer into the h->string->buf array, which moves when it gets resized. */
    gi->gname.ptr = getString(h, (STI)gi->tag);
    if (h->top.sup.flags & ABF_CID_FONT) {
        gi->flags |= ABF_GLYPH_CID;
        if (h->top.FDArray.array[gi->iFD].Private.LanguageGroup == 1)
            gi->flags |= ABF_GLYPH_LANG_1;
    }
    result = glyph_cb->beg(glyph_cb, gi);
    gi->flags |= ABF_GLYPH_SEEN;

    /* Check result */
    switch (result) {
        case ABF_SKIP_RET:
            return ufoSuccess;
        case ABF_QUIT_RET:
            fatal(h, ufoErrParseQuit, NULL);
        case ABF_FAIL_RET:
            fatal(h, ufoErrParseFail, NULL);
    }
    result = h->metrics.cb.beg(&h->metrics.cb, &h->metrics.gi);
    width = getWidth(h, (STI)gi->tag);
    glyph_cb->width(glyph_cb, (float)width);
    if (result == ABF_WIDTH_RET)
        return ufoSuccess;

    parseGLIF(h, gi, glyph_cb, NULL);
    doOpList(h, gi, glyph_cb);
    h->stm.src = NULL;
    doOp_ed(h, glyph_cb);  //  we do this outside of parseGLIFOutline so that we can use parseGLIFOutline to process component glyphs.

    /* set the FontBBox field in the abfTopDict */
    h->top.FontBBox[0] = h->aggregatebounds.left;
    h->top.FontBBox[1] = h->aggregatebounds.bottom;
    h->top.FontBBox[2] = h->aggregatebounds.right;
    h->top.FontBBox[3] = h->aggregatebounds.top;

    /* clear out any point names from the last run */
    if (h->data.opList.cnt > 0) {
        for (i = 0; i < h->data.opList.cnt; i++) {
            OpRec* opRec = &h->data.opList.array[i++];
            if (opRec->pointName != NULL) {
                memFree(h, opRec->pointName);
                opRec->pointName = NULL;
            }
        }
        h->data.opList.cnt = 0; /* zero the list of ops in the glyph */
    }
    /* reset all the hint masks */
    if (h->hints.hintMasks.cnt > 0) {
        for (i = 0; i < h->hints.hintMasks.cnt; i++) {
            HintMask* curMask = dnaINDEX(h->hints.hintMasks, i);
            curMask->maskStems.cnt = 0;
            if (curMask->pointName != NULL) {
                memFree(h, curMask->pointName);
                curMask->pointName = NULL;
            }
        }
        h->hints.hintMasks.cnt = 0;
    }

    /* clear the list of flex names */
    if (h->hints.flexOpList.cnt > 0) {
        for (i = 0; i < h->hints.flexOpList.cnt; i++) {
            char* flexPointName = h->hints.flexOpList.array[i++];
            memFree(h, flexPointName);
        }
        h->hints.flexOpList.cnt = 0;
    }

    return ufoSuccess;
}

/* --------------------------- String Management --------------------------- */

/* Initialize strings. */
static void newStrings(ufoCtx h) {
    dnaINIT(h->dna, h->strings.index, 50, 200);
    dnaINIT(h->dna, h->strings.buf, 32000, 6000);
}

/* Free strings. */
static void freeStrings(ufoCtx h) {
    dnaFREE(h->strings.index);
    dnaFREE(h->strings.buf);
}

/* Add string. */
/* 64-bit warning fixed by type change here HO */
/* static STI addString(ufoCtx h, unsigned length, const char *value) */
static STI addString(ufoCtx h, size_t length, const char* value) {
    STI sti = (STI)h->strings.index.cnt;

    if (length == 0) {
        /* A null name (/) is legal in PostScript but could lead to unexpected
         behavior elsewhere in the coretype libraries so it is substituted
         for a name that is very likely to be unique in the font */
        static const char subs_name[] = "_null_name_substitute_";
        value = subs_name;
        length = sizeof(subs_name) - 1;
        message(h, "null charstring name");
    }

    /* Add new string index */
    *dnaNEXT(h->strings.index) = h->strings.buf.cnt;

    /* Add null-terminated string to buffer */
    /* 64-bit warning fixed by cast here HO */
    memcpy(dnaEXTEND(h->strings.buf, (long)(length + 1)), value, length);
    h->strings.buf.array[h->strings.buf.cnt - 1] = '\0';

    return sti;
}

/* Get string from STI. */
static char* getString(ufoCtx h, STI sti) {
    return &h->strings.buf.array[h->strings.index.array[sti]];
}

/* ----------------------Width management -----------------------*/
static void addWidth(ufoCtx h, STI sti, long value) {
    if (sti != h->chars.widths.cnt) {
        fatal(h, ufoErrParse, "Width index does not match glyph name index. Glyph index %d.", sti);
    }
    *dnaNEXT(h->chars.widths) = value;
}

static long getWidth(ufoCtx h, STI sti) {
    return h->chars.widths.array[sti];
}

static void setWidth(ufoCtx h, STI sti, long value) {
    h->chars.widths.array[sti] = value;
}

/* ----------------------Char management -----------------------*/

/* Match glyph name. */
static int CTL_CDECL matchChar(const void* key, const void* value, void* ctx) {
    ufoCtx h = (ufoCtx) ctx;
    return strcmp((char*)key, getString(h, (STI)h->chars.index.array
                                               [*(long*)value]
                                                   .tag));
}

/* Add char record. Return 1 if record exists else 0. Char record returned by
 "chr" parameter. */
static int addChar(ufoCtx h, STI sti, Char** chr) {
    size_t index;
    int found =
        ctuLookup(getString(h, sti),
                  h->chars.byName.array, h->chars.byName.cnt,
                  sizeof(h->chars.byName.array[0]), matchChar, &index, h);

    if (found)
        /* Match found; return existing record */
        *chr = &h->chars.index.array[h->chars.byName.array[index]];
    else {
        /* Not found; add to table and return new record */
        long* nw = &dnaGROW(h->chars.byName, h->chars.byName.cnt)[index];

        /* Make and fill hole */
        memmove(nw + 1, nw, (h->chars.byName.cnt++ - index) * sizeof(h->chars.byName.array[0]));
        *nw = h->chars.index.cnt;

        *chr = dnaNEXT(h->chars.index);
    }

    return found;
}

/* ---------------------- Public API -----------------------*/

/* Parse files */
int ufoBegFont(ufoCtx h, long flags, abfTopDict** top, char* altLayerDir) {
    int result;

    /* Set error handler */
    DURING_EX(h->err.env)

    /* Initialize */
    abfInitTopDict(&h->top);
    abfInitFontDict(&h->fdict);

    h->top.FDArray.cnt = 1;
    h->top.FDArray.array = &h->fdict;

    h->parseState.FDArray = false;
    h->parseState.hintSetListArray = false;
    h->parseState.type1HintDataV2 = false;
    h->parseState.valueArray = false;
    h->parseState.CIDMap = false;
    h->parseState.UFOFile = None;
    h->parseState.GLIFInfo.currentCID = -1;
    h->parseState.GLIFInfo.CIDCount = 0;
    h->parseState.GLIFInfo.currentiFD = 0;

    h->requiredCIDKeyFlag = 0;

    /* init glyph data structures used */
    h->valueArray.cnt = 0;
    h->chars.index.cnt = 0;
    h->data.glifRecs.cnt = 0;
    h->data.opList.cnt = 0;
    h->hints.hintMasks.cnt = 0;

    h->aggregatebounds.left = FLT_MAX;
    h->aggregatebounds.bottom = FLT_MAX;
    h->aggregatebounds.right = -FLT_MAX;
    h->aggregatebounds.top = -FLT_MAX;

    h->metrics.cb = abfGlyphMetricsCallbacks;
    h->metrics.cb.direct_ctx = &h->metrics.ctx;
    h->metrics.ctx.flags = 0x0;

    if (altLayerDir != NULL)
        h->altLayerDir = altLayerDir;

    dnaGROW(h->valueArray, 14);

    result = parseUFO(h);
    if (result)
        fatal(h, result, NULL);

    prepClientData(h);
    *top = &h->top;

    if ((h->top.cid.CIDFontName.ptr != NULL) &&     /* should've been set at this point */
        (h->top.cid.Registry.ptr != NULL) &&        /* required com.adobe.type.ROS key */
        (h->top.cid.Ordering.ptr != NULL) &&        /* required com.adobe.type.ROS key */
        (h->top.cid.Supplement >= 0) &&             /* required com.adobe.type.ROS key */
        (h->requiredCIDKeyFlag & CIDKEY_CIDMAP)) {  /* required com.adobe.type.postscriptCIDMap key */
        if (!(h->top.sup.flags & ABF_CID_FONT)) {
            h->top.sup.flags |= ABF_CID_FONT;
            h->top.sup.srcFontType = abfSrcFontTypeUFOCID;
        }
    }

    HANDLER
    result = Exception.Code;
    END_HANDLER

    return result;
}

int ufoEndFont(ufoCtx h) {
    if (h->stm.src)
        h->cb.stm.close(&h->cb.stm, h->stm.src);
    return ufoSuccess;
}

int ufoIterateGlyphs(ufoCtx h, abfGlyphCallbacks* glyph_cb) {
    long i;

    /* Set error handler */
    DURING_EX(h->err.env)

    for (i = 0; i < h->chars.index.cnt; i++) {
        int res;
        res = readGlyph(h, i, glyph_cb);
        if (res != ufoSuccess)
            return res;
    }
    HANDLER
    return Exception.Code;
    END_HANDLER

    return ufoSuccess;
}

int ufoGetGlyphByTag(ufoCtx h, unsigned short tag, abfGlyphCallbacks* glyph_cb) {
    int res = ufoSuccess;

    if (tag >= h->chars.index.cnt)
        return ufoErrNoGlyph;

    /* Set error handler */
    DURING_EX(h->err.env)

    res = readGlyph(h, tag, glyph_cb);

    HANDLER
    res = Exception.Code;
    END_HANDLER

    return res;
}

/* Match glyph name after font fully parsed. */
static int CTL_CDECL postMatchChar(const void* key, const void* value,
                                   void* ctx) {
    ufoCtx h = (ufoCtx) ctx;
    const char* testKey = getString(h, (STI)h->chars.index.array
                                         [*(long*)value]
                                             .tag);
    return strcmp((char*)key, testKey);
}

int ufoGetGlyphByName(ufoCtx h, const char* gname, abfGlyphCallbacks* glyph_cb) {
    size_t index;
    int result;

    if (!ctuLookup(gname, h->chars.byName.array, h->chars.byName.cnt,
                   sizeof(h->chars.byName.array[0]), postMatchChar, &index, h))
        return ufoErrNoGlyph;

    /* Set error handler */
    DURING_EX(h->err.env)

    result = readGlyph(h, (unsigned short)h->chars.byName.array[index], glyph_cb);

    HANDLER
    result = Exception.Code;
    END_HANDLER

    return result;
}

/* Match CID value. */
static int CTL_CDECL matchCID(const void *key, const void *value) {
    unsigned short a = *(unsigned short *)key;
    unsigned short b = ((Char *)value)->cid;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

/* Get glyph from font by its CID. */
int ufoGetGlyphByCID(ufoCtx h, unsigned short cid, abfGlyphCallbacks *glyph_cb) {
    volatile unsigned short tag; /* volatile suppresses optimizer warning */

    if (!(h->flags & ABF_CID_FONT))
        return ufoErrNoGlyph;

    if (h->chars.index.array[h->chars.index.cnt - 1].cid ==
        h->chars.index.cnt - 1) {
        /* Non-subset font; index by CID */
        if (cid >= h->chars.index.cnt)
            return ufoErrNoGlyph;
        else
            tag = cid;
    } else {
        /* Subset font; search for matching CID */
        Char *chr =
            (Char *)bsearch(&cid, h->chars.index.array,
                            h->chars.index.cnt, sizeof(Char), matchCID);
        if (chr == NULL)
            return ufoErrNoGlyph;
        else
            /* 64-bit warning fixed by cast here HO */
            tag = (unsigned short)(chr - h->chars.index.array);
    }

    /* Set error handler */
    DURING_EX(h->err.env)

    readGlyph(h, tag, glyph_cb);

    HANDLER
    return Exception.Code;
    END_HANDLER

    return ufoSuccess;
}

int ufoResetGlyphs(ufoCtx h) {
    long i;

    for (i = 0; i < h->chars.index.cnt; i++)
        h->chars.index.array[i].flags &= ~((unsigned long)ABF_GLYPH_SEEN);

    return ufoSuccess;
}

void ufoGetVersion(ctlVersionCallbacks* cb) {
    if (cb->called & 1 << UFR_LIB_ID)
        return; /* Already enumerated */

    abfGetVersion(cb);
    dnaGetVersion(cb);

    cb->getversion(cb, UFO_VERSION, "uforead");

    cb->called |= 1 << UFR_LIB_ID;
}
