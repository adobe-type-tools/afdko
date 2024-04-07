/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Common definitions.
 */

#ifndef ADDFEATURES_HOTCONV_COMMON_H_
#define ADDFEATURES_HOTCONV_COMMON_H_

#include <stdbool.h>

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "hotconv.h"
#include "dynarr.h"
#include "cffread_abs.h"
#include "ctutil.h"
#include "varsupport.h"

#if WIN32
#define SAFE_LOCALTIME(x, y) localtime_s(y, x)
#else
#define SAFE_LOCALTIME(x, y) localtime_r(x, y)
#endif

/* --------------------------------- Macros -------------------------------- */

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CTL_CDECL
#define CTL_CDECL
#endif

typedef int32_t cffFixed;                /* 16.16 fixed point */

/* Apple's TrueType definitions */
typedef short FWord;                     /* Signed metric in font (em) units */
typedef unsigned short uFWord;           /* Unsigned metric in font (em) units */
#define VERSION(a, b) (((int32_t)(a) << 16) | (b) << 12) /* Table version */

/* Size of types (bytes) */
#define int8 1   // XXX remove
#define uint8 1
#define int16 2
#define uint16 2
#define int32 4
#define uint24 3
#define uint32 4

/* Special types */

/* In some otf tables an Offset is a byte offset of data relative to some
   format component, normally the beginning of the record it belongs to. The
   OFFSET macros allow a simple declaration of both the byte offset field and a
   structure containing the data for that offset. (In header definitions the
   symbol |-> is used as a shorthand for "relative to".) */
typedef uint32_t LOffset;
typedef unsigned short Offset;
#define NULL_OFFSET 0
#define DCL_OFFSET(type, name) \
    Offset name;               \
    type name##_
#define DCL_OFFSET_ARRAY(type, name) \
    Offset *name;                    \
    type *name##_

/* Tag support */
typedef uint32_t Tag;
#define TAG(a, b, c, d) ((Tag)(a) << 24 | (Tag)(b) << 16 | (c) << 8 | (d))
#define TAG_ARG(t) (char)((t) >> 24 & 0xff), (char)((t) >> 16 & 0xff), \
                   (char)((t) >> 8 & 0xff), (char)((t)&0xff)

#define CFF__ TAG('C', 'F', 'F', ' ')
#define CFF2_ TAG('C', 'F', 'F', '2')
#define fvar_ TAG('f', 'v', 'a', 'r')
#define avar_ TAG('a', 'v', 'a', 'r')
#define HVAR_ TAG('H', 'V', 'A', 'R')
#define MVAR_ TAG('M', 'V', 'A', 'R')
#define VVAR_ TAG('V', 'V', 'A', 'R')

inline std::string TAG_STR(Tag t) {
    std::string s;
    for (int i = 3; i >= 0; i--) {
        char o = (char)(t >> (8*i) & 0xff);
        s += o;
    }
    return s;
}

extern void *hotMemNew(hotCtx g, size_t s);
extern void *hotMemResize(hotCtx g, void *old, size_t s);
extern void hotMemFree(hotCtx g, void *ptr);
/* Memory management. MEM_FREE() sets its argument to NULL after freeing */
#define MEM_NEW(g, s) hotMemNew((g), (s))
#define MEM_RESIZE(g, p, s) hotMemResize((g), (p), (s))
#define MEM_FREE(g, p)              \
    do {                            \
        hotMemFree((g), (p));       \
        (p) = NULL;                 \
    } while (0)

/* OTF I/O macros */
#define OUT1(v) hout1(h->g, (v))
#define OUT2(v) hotOut2(h->g, (v))
#define OUT3(v) hotOut3(h->g, (v))
#define OUT4(v) hotOut4(h->g, (v))
#define OUTN(c, v) g->cb.stm.write(&g->cb.stm, g->out_stream, (c), (v))
#define TELL() h->g->cb.stm.tell(&h->g->cb.stm, h->g->out_stream)
#define SEEK(o) h->g->cb.stm.seek(&h->g->cb.stm, h->g->out_stream, (o))

/* Specify scale normalized em units (1000/em) to font units */
#define EM_SCALE(u) (g->font.unitsPerEm * (int32_t)(u) / 1000)
#define ARRAY_LEN(t) (sizeof(t) / sizeof((t)[0]))
#define RAD_DEG 57.2958
#define INT2FIX(i) ((Fixed)(i) << 16)
#define FIX2DBL(f) ((f) / 65536.0)

#define RND(d) ((int32_t)(((d) > 0) ? (d) + 0.5 : (d)-0.5))

#define MOVE(d, s, n) memmove(d, s, sizeof((s)[0]) * (n))
#define COPY(d, s, n) memcpy(d, s, sizeof((s)[0]) * (n))
#define INSERT(da, inx) (dnaNEXT(da),               \
                         MOVE(&(da).array[inx] + 1, \
                              &(da).array[inx],     \
                              (da).cnt - (inx)-1),  \
                         &(da).array[inx])

/* Miscellaneous */
#define GET_GID(gi) ((gi) - &g->glyphs[0])

/* -------------------------------- Typedefs ------------------------------- */

typedef int32_t Fixed;

/* Glyph index */
typedef unsigned short GID;
#define GID_NOTDEF 0x0000 /* GID of .notdef glyph */
#define GID_UNDEF 0xffff  /* GID of undefined glyph */

/* String id */
typedef unsigned short SID;
#define SID_UNDEF 0xffff /* SID of undefined string */

/* CID */
typedef unsigned short CID;

/* Unicode value */
typedef uint32_t UV;                      /* Unicode scalar value */
typedef unsigned short UV_BMP;            /* Unicode BMP value */
#define UV_UNDEF ((unsigned)0xFFFF)       /* Indicates undefined UV */
#define UV_SURR_HI_BEG ((unsigned)0xD800) /* High surrogate range begin */
#define UV_SURR_HI_END ((unsigned)0xDBFF) /* High surrogate range end */
#define UV_SURR_LO_BEG ((unsigned)0xDC00) /* Low surrogate range begin */
#define UV_SURR_LO_END ((unsigned)0xDFFF) /* Low surrogate range end */
#define UV_SURR_BEG UV_SURR_HI_BEG        /* Surrogate range begin */
#define UV_SURR_END UV_SURR_LO_END        /* Surrogate range end */
#define UV_EUS_BEG ((unsigned)0xE000)     /* End user subarea begin */
#define UV_EUS_END ((unsigned)0xF5FF)     /* End user subarea end              */
                                          /* (start of Adobe''s Corporate Use) */
#define IN_RANGE(n, b, e) ((n) >= (b) && (n) <= (e))

#define IS_NONCHAR_UV(c) /* Is "c" a Unicode noncharacter? */ \
    (((((c) & ((unsigned)0xFFFF)) == ((unsigned)0xFFFE) || ((c) & ((unsigned)0xFFFF)) == ((unsigned)0xFFFF)) && ((c) >> 16) <= 0x10) || IN_RANGE((c), ((unsigned)0xFDD0), ((unsigned)0xFDEF)))

#define IS_SUPP_UV(c) ((c) > ((uint32_t)0xFFFF)) /* Is "c" a supplementary (ie non-BMP) UV?*/

/* Various heuristic glyphs */
#define UV_CAP_HEIGHT ((unsigned)0x0048)     /* "H" */
#define UV_PRO_CAP_HEIGHT ((unsigned)0x004F) /* "O" */
#define UV_ASCENT ((unsigned)0x0064)         /* "d" */
#define UV_DESCENT ((unsigned)0x0070)        /* "p" */
#define UV_X_HEIGHT_1 ((unsigned)0x0078)     /* "x" */
#define UV_X_HEIGHT_2 ((unsigned)0xF778)     /* "Xsmall" */
#define UV_PRO_X_HEIGHT_1 ((unsigned)0x006F) /* "o" */
#define UV_PRO_X_HEIGHT_2 ((unsigned)0xF76F) /* "Osmall" */
#define UV_SPACE ((unsigned)0x0020)          /* "space" */
#define UV_BULLET ((unsigned)0x2022)         /* "bullet" */
#define UV_VERT_BOUNDS ((unsigned)0x253C)    /* BOX DRAWINGS LIGHT VERTICAL & HORIZONTAL*/
#define UV_CARET_OFFSET_1 ((unsigned)0x004F) /* "O" */
#define UV_CARET_OFFSET_2 ((unsigned)0xF76F) /* "Osmall" */
#define UV_CARET_OFFSET_3 ((unsigned)0xF730) /* "zerooldstyle" */
#define UV_MATH_FONT ((unsigned)0x2118)      /* "weierstrass" */

#define UV_NBSPACE ((unsigned)0x00A0)

typedef struct { /* Bounding box */
    FWord left;
    FWord bottom;
    FWord right;
    FWord top;
} BBox;
#define BBOX_WIDTH(b) ((b).right - (b).left)
#define BBOX_HEIGHT(b) ((b).top - (b).bottom)

/* Kerning pair. Bit 15 in each field determines field interpretation:
   set- bits 14-0 are unencoded char index, clear- bits 7-0 are char code */
typedef struct {
    unsigned short first;
    unsigned short second;
} KernPair_;
/* The Mac pollutes my namespace with KernPair already, hence the underscore */
#define KERN_UNENC 0x8000 /* Flags unencoded kern character */

typedef struct { /* Horizontal and vertical metric */
    FWord h;
    FWord v;
} hvMetric;

typedef dnaDCL(char, CharName);

typedef struct AddlUV_ AddlUV;
struct AddlUV_ { /* Additional UV */
    AddlUV *next;
    UV uv;
};

struct hotGlyphInfo { /* Glyph information */
    /* --- Same as cffGlyphInfo: */
    unsigned short id; /* SID/CID */
    uint32_t code;        /* Encoding */
    FWord hAdv;     /* Horizontal advance width */
    FWord vAdv;     /* Vertical advance width */
    BBox bbox;      /* Bounding box */
    std::vector<uint32_t> sup;   /* Supplementary encodings */

    /* --- Additional fields: */
    std::string gname;
    std::string srcName; /* Actual pointer to development glyph name */
    short flags;
#define GNAME_UNI (1 << 0)            /* Has uni<CODE> or u<CODE> name e.g. "u102345" */
#define GNAME_UNREC (1 << 1)          /* Unrecognized name e.g. "A.swash" */
#define GNAME_UNREC_HAS_SUPP (1 << 2) /* Name contains a base supp UV e.g. "u102345.alt" */
#define GNAME_DBLMAP (1 << 3)         /* Glyph has been double-mapped */
#define GNAME_UNENC (1 << 4)          /* Unenc (uni<CODE> override or EUS overflow) */
#define GNAME_UNI_OVERRIDE (1 << 5)   /* Has user supplied (uni<CODE> */

    UV uv;          /* Primary UV */
    std::vector<UV> addlUV; /* Additional UVs */
    short vOrigY;   /* Y coordinate of the glyph's vertical origin */
};

struct FontInfo_ { /* Font information */
    struct {     /* --- Version information */
        Fixed otf;
        std::string PS;
        const char *client;
    } version;

    short fsSelectionMask_on;
    short fsSelectionMask_off;
    unsigned short os2Version;
#if HOT_DEBUG
    unsigned short debug; /* Flags from client via hotNew() */
#endif                    /* HOT_DEBUG */
    unsigned short flags;
#define FI_MISC_FLAGS_MASK 0x01ff /* Flags from client via hotAddMiscData()*/
#define FI_FIXED_PITCH (1 << 13)  /* Fixed pitch font */
#define FI_CID (1 << 15)          /* CID font */
    std::string FontName;
    std::string Notice;
    std::string FamilyName;
    std::string FullName;
    unsigned short unitsPerEm;
    BBox bbox;
    BBox minBearing;
    hvMetric maxAdv;
    hvMetric maxExtent;
    Fixed ItalicAngle;
    FWord UnderlinePosition;
    FWord UnderlineThickness;
    short hheaAscender;
    short hheaDescender;
    short hheaLineGap;
    short TypoAscender;        /* In OS/2 */
    short TypoDescender;       /* In OS/2 */
    short TypoLineGap;         /* In OS/2 */
    unsigned short winAscent;  /* In OS/2 */
    unsigned short winDescent; /* In OS/2 */
    short VertTypoAscender;    /* In vhea */
    short VertTypoDescender;   /* In vhea */
    short VertTypoLineGap;     /* In vhea */
    short Encoding;            /* Encoding id */
#define FI_STD_ENC 0           /* Standard */
#define FI_EXP_ENC 1           /* Expert */
#define FI_CUSTOM_ENC 2        /* Custom (defined by font) */
    struct {
        /* Windows-specific data */
        uint8_t Family;
        uint8_t CharSet;
        UV_BMP DefaultChar;
        UV_BMP BreakChar;
        FWord AvgWidth;
        unsigned short ascent;
        unsigned short descent;
        uFWord XHeight;
        uFWord CapHeight;
        FWord SubscriptXSize;
        FWord SubscriptYSize;
        FWord SubscriptXOffset;
        FWord SubscriptYOffset;
        FWord SuperscriptXSize;
        FWord SuperscriptYSize;
        FWord SuperscriptXOffset;
        FWord SuperscriptYOffset;
        FWord StrikeOutSize;
        FWord StrikeOutPosition;
    } win;
    struct {
        /* Mac-specific data */
        int32_t cmapScript;   /* Platform-specific field for Mac cmap */
        int32_t cmapLanguage; /* Language field for Mac cmap */
    } mac;
    struct { /* --- Kerning data */
        dnaDCL(KernPair_, pairs);
        dnaDCL(short, values); /* [nPairs] */
    } kern;
    dnaDCL(CharName, unenc);       /* Unencoded chars */
    struct { /* --- CID-specific data */
        std::string registry;
        std::string ordering;
        unsigned short supplement;
    } cid;
    std::string vendId;    /* Vendor id */
    std::string licenseID; /* Font licensor ID. */
};
/* The Mac pollutes my namespace with FontInfo already, hence the underscore */

/* Convenience macros */
#define IS_CID(g) ((g)->font.flags & FI_CID)

/* -------------------------------- Contexts ------------------------------- */
typedef struct mapCtx_ *mapCtx;
typedef struct cfrCtx_ *cfrCtx;
typedef struct BASECtx_ *BASECtx;
typedef struct OS_2Ctx_ *OS_2Ctx;
typedef struct STATCtx_ *STATCtx;
typedef struct anonCtx_ *anonCtx;
typedef struct cmapCtx_ *cmapCtx;
typedef struct headCtx_ *headCtx;
typedef struct maxpCtx_ *maxpCtx;
typedef struct postCtx_ *postCtx;
typedef struct sfntCtx_ *sfntCtx;

class FeatCtx;
class GDEF;
class GPOS;
class GSUB;
class nam_name;
class var_hmtx;
class var_vmtx;
class var_axes;
class var_MVAR;
class VarLocationMap;

#define ID_TEXT_SIZE 1024 /* Size of text buffer used to hold identifying info about the current feature for error messages. */

class hotVarWriter : public VarWriter {
 public:
    hotVarWriter() = delete;
    explicit hotVarWriter(hotCtx g) : g(g) {}
    void w1(char o) override;
    void w2(int16_t o) override;
    void w3(int32_t o) override;
    void w4(int32_t o) override;
    void w(size_t count, char *data) override;
    hotCtx g;
};

struct hotCtx_ {
    hotCtx_() : vw(this) {}
    const char *getNote() {
        if (note.size() > 1024) {
            note.resize(1024);
            note[1023] = '.';
            note[1022] = '.';
            note[1021] = '.';
        }
        return note.c_str();
    }
    long version;    /* Hot lib version number */
    struct tm time;  /* Local time */
    FontInfo_ font;  /* Font information */
    std::vector<hotGlyphInfo> glyphs;
    hotCallbacks cb; /* Client callbacks */
    hotVarWriter vw;
    ctlSharedStmCallbacks sscb;
    struct {         /* --- Package contexts */
        mapCtx map;
        FeatCtx *feat;
        cfrCtx cfr;
        BASECtx BASE;
        GDEF *GDEFp;
        GPOS *GPOSp;
        GSUB *GSUBp;
        OS_2Ctx OS_2;
        STATCtx STAT;
        anonCtx anon;
        cmapCtx cmap;
        headCtx head;
        var_hmtx *hmtx;
        maxpCtx maxp;
        nam_name *name;
        postCtx post;
        sfntCtx sfnt;
        var_vmtx *vmtx;
        var_axes *axes;
        var_MVAR *MVAR;
        VarLocationMap *locMap;
    } ctx;
    dnaCtx DnaCTX;
    std::string note;
    void *in_stream;
    void *out_stream;
    std::string error_id_text;
    bool hadError;         /* Flags if error occurred */
    uint32_t convertFlags; /* flags for building final OTF. */
    char *bufnext;         /* Next byte available in input buffer */
    long bufleft;          /* Number of bytes available in input buffer */

    std::shared_ptr<slogger> logger;
    std::shared_ptr<GOADB> goadb;
};

/* Functions */
void hotQuitOnError(hotCtx g);

inline void hout1(hotCtx g, char c) {
    g->cb.stm.write(&g->cb.stm, g->out_stream, 1, &c);
}

void hotOut2(hotCtx g, short value);
void hotOut3(hotCtx g, int32_t value);
void hotOut4(hotCtx g, int32_t value);

/* Read from OTF data */
char hotFillBuf(hotCtx g);
inline char hin1(hotCtx g) {
    return (g->bufleft-- == 0) ? hotFillBuf(g) : *g->bufnext++;
}
short hotIn2(hotCtx g);
int32_t hotIn4(hotCtx g);
void hotInN(hotCtx g, size_t count, char *ptr);

void hotCalcSearchParams(unsigned unitSize, long nUnits,
                         unsigned short *searchRange,
                         unsigned short *entrySelector,
                         unsigned short *rangeShift);
void hotWritePString(hotCtx g, const char *string);

void hotAddVertOriginY(hotCtx g, GID gid, short value);
void hotAddVertAdvanceY(hotCtx g, GID gid, short value);

void setVendId_str(hotCtx g, const char *vend);

#define OVERRIDE(field) ((field) != SHRT_MAX)

#endif  // ADDFEATURES_HOTCONV_COMMON_H_
