/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Assigns UVs, feeds cmap module
 */

#include "map.h"
#include "OS_2.h"
#include "cmap.h"
#include "GPOS.h"
#include "GSUB.h"
#include "otl.h"
#include "feat.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>

#include "pstoken.h"

#define SET_BIT_ARR(a, b) (a[(b) / 32] |= 1UL << (b) % 32)
#define TEST_BIT_ARR(a, b) (a[(b) / 32] & 1UL << (b) % 32)

#define MAX_GNAME_LEN 63

#define SUPP_UV_BITNUM 57 /* OS/2.ulUnicodeRange bitnum flagging supp UVs */

#if HOT_DEBUG
#define DB_MAP(g) (g->font.debug & HOT_DB_MAP)
#define DM(p)            \
    do {                 \
        if (DB_MAP(g)) { \
            printf p;    \
        }                \
    } while (0)
static void dbgPrintUV(UV uv);

#endif /* HOT_DEBUG */

typedef int32_t SInx;   /* Index into char da */
#define SINX_UNDEF (-1) /* Indicates invalid SInx */
#define STR(i) (&h->str.array[i])

#define MAKE_UNI_GLYPH(uv, gname) sprintf(gname, "uni%04X", uv)

/* To distinguish a Greek font from a Math font such as Symbol or
   LucidaMath-Italic: */
#define IS_MATH_FONT(s) ((s) == grek_ && mapUV2Glyph(g, UV_MATH_FONT) != NULL)

#define LATIN_BIT 0 /* latin (1252) code page bit number */

#define SYMBOL_CHARSET 2 /* Symbol WinCharSet */
#define OEM_BIT 30       /* OEM charset code page bit number */
#define SYMBOL_BIT 31    /* Symbol code page bit number */
#define MAC_BIT 29       /* Symbol code page bit number */

/* NOTE: the *_INX values below refer to the *INDEX* of the corresponding
         Unicode range in uniblock.h, and *NOT* the bit number of that range
         in the OS/2 Unicode ranges bits. For example UnicodeBlock[97] is the
         Bopomofo block, which corresponds to OS/2 Unicode range bit 51. */
#define HIRAGANA_INX    95 /* Unicode blocks used in determining... */
#define KATAKANA_INX    96
#define BOPOMOFO_INX    97
#define CJK_IDEO_INX   107
#define HANGUL_SYL_INX 120

#define JIS_CP 17 /* ... code page support for CJK */
#define CHN_SIMP_CP 18
#define KOR_WANSUNG_CP 19
#define CHN_TRAD_CP 20
#define KOR_JOHAB_CP 21

#define MAC_SCR_ROMAN 0 /* Macintosh script codes */
#define MAC_SCR_JAPANESE 1
#define MAC_SCR_CHN_TRAD 2
#define MAC_SCR_CHN_SIMP 25
#define MAC_SCR_KOR 3

#define MAC_LANG_UNSPEC 0

/* AFM printing */
#define SID_WRITE_FIELD(g, key, sid) \
    (printf("%s ", key), SIDWrite(g, sid), printf("\n"))
#define AFM_UNENC (-1)
typedef struct {
    short code;
    hotGlyphInfo *gi;
} AFMChar;
typedef struct {
    char *first;
    char *second;
    short value;
} KernNamePair;

/* ------------------------- Pre-generated Tables -------------------------- */

/* --- Unicode glyph name tables */

typedef struct {
    char *glyphName;
    UV_BMP uv; /* BMP Unicode value */
} UnicodeChar;

static UnicodeChar agl2uv[] = {
#define AGL2UV_DBLMAP_CHAR '%'
#include "agl2uv.h"
};

static UnicodeChar zapfDingbatsList[] = {
#include "zding2uv.h"
};

static unsigned short schinese[] = {
#include "schinese.h"
};

/* --- Unicode block table */

typedef struct {
    UV first;              /* First of range */
    UV last;               /* Last of range */
    unsigned numDefined;   /* num chars defined by Unicode for this block    */
                           /* if numDefined is 1, it means set the block bit */
                           /* if there are *any* characters in this block    */
    unsigned numFound;     /* Number of entries found for this block */
    unsigned bitNum;       /* in OS_2 */
    unsigned isSupported;  /* Is this block supported? */
    char *name;            /* Descriptive name (debugging purposes only) */
} UnicodeBlock;

static UnicodeBlock unicodeBlock[] = {
#include "uniblock.h"
};

/* --- Code page tables */

typedef struct {
    short bitNum;         /* in OS_2.CodePageRange1+2;                        */
                          /* -1 means not a Windows code page.                */
    UV_BMP uv1;           /* Heuristic char 1 (required)                      */
    char *gn1;            /* ..and its AGL glyph name (NULL if not in AGL).   */
                          /* It is needed here since there is no reverse AGL  */
                          /* lookup; i.e. UV->gname.                          */
    UV_BMP uv2;           /* Heuristic char 2 (optional; UV_UNDEF if none)    */
    char *gn2;            /* ..and its AGL glyph name (NULL if not in AGL)    */
    short isSupported;    /* Are either of the heuristic glyphs present in    */
                          /* the font? Valid: 1 or 0. Init to -1.             */
                          /* For Mac Greek encoding, note that the additional */
                          /* test of IS_MATH_FONT() must be false.            */
    Tag script;           /* OT script associated with this codepage/enc.     */
    long maccmapScript;   /* Platform-specific field for Mac cmap.            */
                          /* HOT_CMAP_UNKNOWN means not a mac encoding        */
    long maccmapLanguage; /* Language field for Mac cmap (this is Mac         */
                          /* language ID + 1, or 0 if unspecific language)    */
} CodePage;

static CodePage codePage[] = {
#include "codepage.h"
};

/* Predefined Mac encodings. (Same order as in valid Mac encodings in
   codepage.h): */
typedef UV_BMP MacEnc[256];
static MacEnc macEnc[] = {
    {
#include "macromn0.h" /* 0. Mac Roman */
    },
    {
#include "macce.h" /* 1. Mac CE */
    },
    {
#include "maccyril.h" /* 2. Mac 9.0 Cyrillic */
    },
    {
#include "macgreek.h" /* 3. Mac Greek */
    },
    {
#include "macturk.h" /* 4. Mac Turkish */
    },
    {
#include "machebrw.h" /* 5. Mac Hebrew */
    },
    {
#include "macarab.h" /* 6. Mac Arabic */
    },
    {
#include "macthai.h" /* 7. Mac Thai */
    },
    {
#include "maccroat.h" /* 8. Mac Croatian */
    },
    {
#include "maciceln.h" /* 9. Mac Icelandic */
    },
    {
#include "macrmian.h" /* 10. Mac Romanian */
    },
    {
#include "macfarsi.h" /* 11. Mac Farsi */
    },
    {
#include "macdevan.h" /* 12. Mac Devanagari */
    },
    {
#include "macgujar.h" /* 13. Mac Gujarati */
    },
    {
#include "macgurmk.h" /* 14. Mac Gurmukhi */
    },
};

typedef struct { /* Vertical variant map */
    UV uvTarg;   /* Target UV */
    GID gidRepl; /* Replacement GID */
} VVMap;

typedef struct { /* Weighting by GID */
    GID gid;
    short weight;
} GIDWeight;

typedef struct {
    short flags;
#define CODE_1BYTE (1 << 0)  /* Single byte range */
#define CODE_NOTDEF (1 << 1) /* Notdef range */
#define CODE_4BYTE (1 << 2)  /* 4 byte range byte range */
    UV lo;
    UV hi;
    CID cid;
} Range;

typedef struct {
    short flags;
    UV uv;
    UV uvs;
    CID cid;
    char gName[64];
} UVSEntry;

/* --------------------------- Context Definition -------------------------- */

struct mapCtx_ {
    /* ---- CID ---- flagged by IS_CID(g) */
    struct {
        psCtx ctx;      /* PostScript parser context */
        psBuf buf;      /* PostScript input buffer */
        psCallbacks cb; /* PostScript parser callbacks */
    } ps;
    struct {
        struct {
            SInx name;            /* /CMapName */
            dnaDCL(Range, range); /* Ranges from CMap */
        } hor;
        struct {
            SInx name;          /* /CMapName */
            SInx usecmap;       /* usecmap value */
            dnaDCL(VVMap, map); /* Vertical variant maps */
        } ver;
        struct {
            SInx name;                /* /CMapName */
            dnaDCL(Range, codespace); /* Codespace ranges from CMap */
            dnaDCL(Range, range);     /* CID and notdef ranges from CMap */
        } mac;
    } cid;
    struct {
        dnaDCL(UVSEntry, entries); /*UVS ecords from UVS file */
    } uvs;

    /* ---- non-CID ---- */
    struct {
        unsigned unrec; /* Num unrecognized glyph names */
        unsigned unenc; /* Num glyphs unencoded in Uni cmap */
    } num;

    /* ---- Common ---- */
    struct {
        dnaDCL(hotGlyphInfo *, gname); /* --- Sorted by glyph name/CID */

        dnaDCL(hotGlyphInfo *, uv); /* --- Sorted by primary UV */
        dnaDCL(GID, glyphAddlUV);   /* GIDs that have additional UVs */
        UV firstAddlUV;             /* For search optimization */
        UV lastAddlUV;              /*  "    "      "          */
        unsigned short nAddlUV;     /* Debug only */

        hotGlyphInfo *platEnc[256];
    } sort;

    unsigned short nSuppUV; /* num supplementary (i.e. non-BMP) UVs */
    long minBmpUV;          /* Minimum BMP UV */
    long maxBmpUV;          /* Maximum BMP UV */

    dnaDCL(char, str); /* Stores strings used */

    hotCtx g;
};

/* --------------------------- Standard Functions -------------------------- */

static void mapInit(hotCtx g);

void mapNew(hotCtx g) {
    mapCtx h = MEM_NEW(g, sizeof(struct mapCtx_));

    dnaINIT(g->dnaCtx, h->ps.buf, 35000, 50000); /* ? xxx */
    h->ps.cb.ctx = g->cb.ctx;
    h->ps.cb.fatal = g->cb.fatal;
    h->ps.cb.malloc = g->cb.malloc;
    h->ps.cb.free = g->cb.free;
    h->ps.cb.buf = &h->ps.buf;
    h->ps.cb.message = g->cb.message;

    dnaINIT(g->dnaCtx, h->cid.hor.range, 6970, 2000); /* Optimized for UniJIS-UCS2-[HV] */
    dnaINIT(g->dnaCtx, h->cid.ver.map, 180, 180);
    dnaINIT(g->dnaCtx, h->cid.mac.codespace, 5, 10);
    dnaINIT(g->dnaCtx, h->cid.mac.range, 225, 225); /* Optimized for 83pv-RKSJ-H */

    h->cid.hor.name = SINX_UNDEF;
    h->cid.ver.name = SINX_UNDEF;
    h->cid.mac.name = SINX_UNDEF;

    dnaINIT(g->dnaCtx, h->uvs.entries, 15000, 500); /* Optimized for 83pv-RKSJ-H */

    dnaINIT(g->dnaCtx, h->sort.gname, 400, 7000);
    dnaINIT(g->dnaCtx, h->sort.uv, 400, 6000);
    dnaINIT(g->dnaCtx, h->sort.glyphAddlUV, 10, 60);
    h->sort.firstAddlUV = UV_UNDEF;
    h->sort.lastAddlUV = 0;
    h->sort.nAddlUV = 0;

    h->nSuppUV = 0;
    h->minBmpUV = LONG_MAX;
    h->maxBmpUV = LONG_MIN;

    dnaINIT(g->dnaCtx, h->str, 2400, 3600);

    /* Link contexts */
    h->g = g;
    g->ctx.map = h;
}

static int CDECL cmpCID(const void *first, const void *second) {
    CID a = (*(hotGlyphInfo **)first)->id;
    CID b = (*(hotGlyphInfo **)second)->id;
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

static int CDECL matchCID(const void *key, const void *value) {
    CID a = *(CID *)key;
    CID b = (*((hotGlyphInfo **)value))->id;
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

hotGlyphInfo *mapCID2Glyph(hotCtx g, CID cid) {
    mapCtx h = g->ctx.map;
    hotGlyphInfo **found;

    if (!IS_CID(g)) {
        hotMsg(g, hotFATAL, "Not a CID font");
    }
    found =
        (hotGlyphInfo **)bsearch(&cid, h->sort.gname.array, h->sort.gname.cnt,
                                 sizeof(hotGlyphInfo *), matchCID);
    return (found != NULL) ? *found : NULL;
}

GID mapCID2GID(hotCtx g, CID cid) {
    hotGlyphInfo *gi = mapCID2Glyph(g, cid);
    return (gi != NULL) ? GET_GID(gi) : GID_UNDEF;
}

/* Sort GNAME_UNREC glyphs before others, and among the GNAME_UNREC glyphs,
   sort GNAME_UNREC_HAS_SUPP after the others. */
static int CDECL cmpUnrecGlyphName(const void *first, const void *second) {
    hotGlyphInfo *a = *(hotGlyphInfo **)first;
    hotGlyphInfo *b = *(hotGlyphInfo **)second;

    if ((a->flags & GNAME_UNREC) && (b->flags & GNAME_UNREC)) {
        if ((a->flags & GNAME_UNREC_HAS_SUPP) &&
            (b->flags & GNAME_UNREC_HAS_SUPP)) {
            return strcmp(a->gname.str, b->gname.str);
        } else if ((a->flags & GNAME_UNREC_HAS_SUPP)) {
            return 1;
        } else if ((b->flags & GNAME_UNREC_HAS_SUPP)) {
            return -1;
        } else {
            return strcmp(a->gname.str, b->gname.str);
        }
    } else if ((a->flags & GNAME_UNREC)) {
        return -1;
    } else if ((b->flags & GNAME_UNREC)) {
        return 1;
    } else {
        return 0;
    }
}

static int CDECL cmpGlyphName(const void *a, const void *b) {
    return strcmp((*(hotGlyphInfo **)a)->gname.str,
                  (*(hotGlyphInfo **)b)->gname.str);
}

static int CDECL matchGlyphName(const void *key, const void *value) {
    return strcmp((char *)key, (*(hotGlyphInfo **)value)->gname.str);
}

/* Map glyph name to glyph info. If useAliasDB is non-NULL and glyph alias db
   is present, it considers gname as an alias and uses the "real" name from
   the glyph alias db and sets *useAliasDB to that name. If useAliasDB is
   non-NULL and glyph alias db is not present, sets *useAliasDB to NULL. */
hotGlyphInfo *mapName2Glyph(hotCtx g, char *gname, char **useAliasDB) {
    mapCtx h = g->ctx.map;
    char *realName = gname;
    hotGlyphInfo **found;

    if (IS_CID(g) && useAliasDB == NULL) {
        hotMsg(g, hotFATAL, "Not a non-CID font");
    }

    if (useAliasDB != NULL) {
        if (g->cb.getFinalGlyphName != NULL) {
            *useAliasDB = g->cb.getFinalGlyphName(g->cb.ctx, gname);
            if (strcmp(*useAliasDB, gname) == 0)
                *useAliasDB = NULL;
            else
                realName = *useAliasDB;
        } else {
            *useAliasDB = NULL;
        }
    }

    if (IS_CID(g)) {
        CID cid = 0;
        sscanf(realName, "cid%hd", &cid);
        if (cid == 0)
            return NULL;
        return mapCID2Glyph(g, cid);
    }
    found = (hotGlyphInfo **)bsearch(realName, h->sort.gname.array,
                                     h->sort.gname.cnt, sizeof(hotGlyphInfo *),
                                     matchGlyphName);
    if (found != NULL) {
        return *found;
    } else {
        return NULL;
    }
}

void mapGID2Name(hotCtx g, GID gid, char *msg) {
    int len;
    hotGlyphInfo *hGID = &g->font.glyphs.array[gid];

    if (hGID->srcName == NULL) {
        sprintf(msg, "%s", hGID->gname.str);
        len = strlen(msg);
    } else {
        if (g->convertFlags & HOT_CONVERT_FINAL_NAMES)
            sprintf(msg, "%s", hGID->gname.str);
        else
            sprintf(msg, "%s", hGID->srcName);
        len = strlen(msg);
    }
}

/* Map glyph name to GID; using alias db if present (see mapName2Glyph comments
   for useAliasDB details) */
GID mapName2GID(hotCtx g, char *gname, char **useAliasDB) {
    hotGlyphInfo *gi = mapName2Glyph(g, gname, useAliasDB);
    return (gi != NULL) ? GET_GID(gi) : GID_UNDEF;
}

static int mapName2UVOverrideName(hotCtx g, char *gname, char **uvOverrideName) {
    char *uvName = NULL;
    if (IS_CID(g)) {
        hotMsg(g, hotFATAL, "Not a non-CID font");
    }

    uvName = (uvOverrideName != NULL && g->cb.getUVOverrideName != NULL)
                 ? g->cb.getUVOverrideName(g->cb.ctx, gname)
                 : NULL;
    if (uvOverrideName != NULL) {
        *uvOverrideName = g->cb.getUVOverrideName != NULL ? uvName : NULL;
    }

    return (uvName != NULL);
}

static int CDECL cmpUV(const void *first, const void *second) {
    UV a = (*(hotGlyphInfo **)first)->uv;
    UV b = (*(hotGlyphInfo **)second)->uv;
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

static int CDECL matchUV(const void *key, const void *value) {
    UV a = *(UV *)key;
    UV b = (*((hotGlyphInfo **)value))->uv;
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

/* Caution: The uv searched for isn't necessarily in the returned struct's uv
   field, since it could be one of the AddlUV values */
hotGlyphInfo *mapUV2Glyph(hotCtx g, UV uv) {
    mapCtx h = g->ctx.map;
    hotGlyphInfo **found;

    if (uv == UV_UNDEF) {
        return NULL;
    }

    found = (hotGlyphInfo **)bsearch(&uv, h->sort.uv.array, h->sort.uv.cnt,
                                     sizeof(hotGlyphInfo *), matchUV);
    if (found != NULL) {
        return *found;
    } else if (uv >= h->sort.firstAddlUV && uv <= h->sort.lastAddlUV) {
        /* Check additional UVs */
        long i;
        for (i = 0; i < h->sort.glyphAddlUV.cnt; i++) {
            AddlUV *sup;
            hotGlyphInfo *gi =
                &g->font.glyphs.array[h->sort.glyphAddlUV.array[i]];

            for (sup = gi->addlUV; sup != NULL; sup = sup->next) {
                if (sup->uv == uv) {
                    return gi;
                }
            }
        }
    }
    return NULL;
}

GID mapUV2GID(hotCtx g, UV uv) {
    hotGlyphInfo *gi = mapUV2Glyph(g, uv);
    return (gi != NULL) ? GET_GID(gi) : GID_UNDEF;
}

/* Map Windows ANSI code to Unicode */
UV mapWinANSI2UV(hotCtx g, int code) {
    static UV_BMP winANSI2UV[256] = {
#include "winansi.h"
    };
    return winANSI2UV[code & 0xff];
}

/* Map Windows ANSI code to glyph. Return NULL if undefined */
hotGlyphInfo *mapWinANSI2Glyph(hotCtx g, int code) {
    UV uv = mapWinANSI2UV(g, code);
    return (uv == UV_UNDEF) ? NULL : mapUV2Glyph(g, uv);
}

/* Map Windows ANSI code to GID */
GID mapWinANSI2GID(hotCtx g, int code) {
    return mapUV2GID(g, mapWinANSI2UV(g, code));
}

/* Map platform encoding to glyph */
hotGlyphInfo *mapPlatEnc2Glyph(hotCtx g, int code) {
    mapCtx h = g->ctx.map;

    if (IS_CID(g)) {
        hotMsg(g, hotFATAL, "Not a non-CID font");
    }
    return h->sort.platEnc[code];
}

GID mapPlatEnc2GID(hotCtx g, int code) {
    hotGlyphInfo *gi = mapPlatEnc2Glyph(g, code);
    return (gi != NULL) ? GET_GID(gi) : GID_UNDEF;
}

static int CDECL matchBlock(const void *key, const void *value) {
    UV uv = *(UV *)key;
    UnicodeBlock *bl = (UnicodeBlock *)value;
    if (uv < bl->first) {
        return -1;
    }
    if (uv > bl->last) {
        return 1;
    }
    return 0;
}

/* Add uv to uni block cnt (called only for additional UVs) */
static void addToBlock(mapCtx h, UV uv) {
    UnicodeBlock *found =
        (UnicodeBlock *)bsearch(&uv, unicodeBlock, ARRAY_LEN(unicodeBlock),
                                sizeof(UnicodeBlock), matchBlock);
    if (found != NULL) {
        found->numFound++;
    }
}

/* Add a UV to the hotGlyphInfo in its primary uv field, or, if that's already
   set, to its list of additional UVs. */
static void addUVToGlyph(hotCtx g, hotGlyphInfo *gi, UV uv) {
    mapCtx h = g->ctx.map;

    if (gi->uv == UV_UNDEF) {
        *dnaNEXT(h->sort.uv) = gi; /* Add gi to h->sort.uv array */
        gi->uv = uv;
    } else {
        AddlUV **new;

        /* Record GIDs that have additional UVs */
        if (gi->addlUV == NULL) {
            *dnaNEXT(h->sort.glyphAddlUV) = GET_GID(gi);
        }

        for (new = &gi->addlUV; *new != NULL; new = &(*new)->next) {
        }
        *new = MEM_NEW(g, sizeof(AddlUV));
        (*new)->next = NULL;
        (*new)->uv = uv;

        if (uv < h->sort.firstAddlUV) {
            h->sort.firstAddlUV = uv;
        }
        if (uv > h->sort.lastAddlUV) {
            h->sort.lastAddlUV = uv;
        }
        h->sort.nAddlUV++;

        addToBlock(h, uv);
    }

    if (IS_SUPP_UV(uv)) {
        h->nSuppUV++;
    } else if (!(gi->flags & GNAME_UNREC_HAS_SUPP)) {
        /* Since we don't put GNAME_UNREC_HAS_SUPP glyphs in the BMP-16 cmaps, we don't count them in min/maxBmpUV */
        if ((long)uv < h->minBmpUV) {
            h->minBmpUV = uv;
        }
        if ((long)uv > h->maxBmpUV) {
            h->maxBmpUV = uv;
        }
    }
}

/* --- begin CMap functions --- */

/* [CMap] Prints CMap message. Message should not have newline */
static void CDECL CMapMsg(mapCtx h, int msgType, char *fmt, ...) {
    va_list ap;
    char msgVar[1024];
    char msg[2048];

    va_start(ap, fmt);
    VSPRINTF_S(msgVar, sizeof(msgVar), fmt, ap);
    va_end(ap);
    sprintf(msg, "%s [%s]", msgVar, h->ps.cb.psId ? h->ps.cb.psId(h->ps.cb.ctx) : "");

    hotMsg(h->g, msgType, msg);
}

/* [CMap] Validate first line. */
static void checkCMapFirstLine(hotCtx g, psToken *token) {
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    char p[512];

    if (token->type != PS_DOCTYPE) {
        CMapMsg(h, hotFATAL, "Invalid first line in CMap");
    }
    memcpy(p, psGetValue(ps, token), token->length);
    p[token->length] = '\0';
    if (strstr(p, "Resource-CMap") == NULL) {
        CMapMsg(h, hotFATAL, "Invalid first line in CMap");
    }
}

/* [CMap] Registry and Ordering must match the CID font */
static void checkCMapCompatibility(hotCtx g) {
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    char *Reg, *reg, *Ord, *ord;
    unsigned Reglen, reglen, Ordlen, ordlen;

    reg = psGetString(ps, &reglen);
    Reg = hotGetString(g, g->font.cid.registry, &Reglen);
    if (reglen != Reglen || memcmp(reg, Reg, reglen) != 0) {
        CMapMsg(h, hotFATAL, "CMap /Registry incompatible with CID font");
    }
    if (!psMatchToken(ps, psGetToken(ps), PS_OPERATOR, "def")) {
        CMapMsg(h, hotFATAL, "expecting def after /Registry value");
    }

    if (!psMatchToken(ps, psGetToken(ps), PS_LITERAL, "/Ordering")) {
        CMapMsg(h, hotFATAL, "expecting /Ordering after /Registry def");
    }
    ord = psGetString(ps, &ordlen);
    Ord = hotGetString(g, g->font.cid.ordering, &Ordlen);
    if (ordlen != Ordlen || memcmp(ord, Ord, ordlen) != 0) {
        CMapMsg(h, hotFATAL, "CMap /Ordering incompatible with CID font");
    }
    if (!psMatchToken(ps, psGetToken(ps), PS_OPERATOR, "def")) {
        CMapMsg(h, hotFATAL, "expecting def after /Ordering value");
    }
}

/* Add string to mapCtx str da; return da index */
static SInx addString(mapCtx h, char *src, unsigned length) {
    char *dst = dnaEXTEND(h->str, (long)(length + 1));
    memcpy(dst, src, length);
    h->str.array[h->str.cnt - 1] = '\0';
    return h->str.cnt - length - 1;
}

/* [CMap] */
static SInx getUseCMap(hotCtx g, psToken *last) {
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    char *usecmap;
    unsigned length;

    if (last->type != PS_LITERAL) {
        CMapMsg(h, hotFATAL, "expecting literal before usecmap");
    }
    usecmap = psConvLiteral(ps, last, &length);
    return addString(h, usecmap, length);
}

/* [CMap] */
static SInx getCMapName(hotCtx g) {
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    char *name;
    unsigned length;
    psToken *token = psGetToken(ps);

    if (token->type != PS_LITERAL) {
        CMapMsg(h, hotFATAL, "expecting literal after /CMapName");
    }
    name = psConvLiteral(ps, token, &length);
    return addString(h, name, length);
}

/* [CMap] */
static int getCMapWMode(hotCtx g) {
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    long wMode = psGetInteger(ps);

    if (wMode != 0 && wMode != 1) {
        CMapMsg(h, hotFATAL, "invalid /WMode value");
    }
    return wMode;
}

/* [CMap] Read range of type code[s]pace, [c]id, or [n]otdef. vert is WMode.
   Single-byte codespace range sets isMac for horiz mode; vert mode means
   Unicode cmap; codespace ranges only stored for Mac CMap. */
static void readRange(hotCtx g, psToken *last, char rangeType, int *isMac,
                      int vert) {
#define IS_UNSET(var) ((var) != 0 && (var) != 1)
    mapCtx h = g->ctx.map;
    psCtx ps = h->ps.ctx;
    int i;
    int nRanges;
    dnaDCL(Range, tmpCodespace); /* Stores code ranges temporarily */
    int codespace = rangeType == 's';
    int charrange = rangeType == 'd';
    int notdef = rangeType == 'n';
    char *rangeName = codespace ? "codespace" : notdef ? "notdef" : charrange ? "cidchar" : "cid";

    if (IS_UNSET(vert)) {
        /* xxx Actually, this ordering is not required: */
        CMapMsg(h, hotFATAL, "expecting /WMode before CMap range operations");
    }

    if (!vert) {
        if (!codespace && IS_UNSET(*isMac)) {
            CMapMsg(h, hotFATAL, "codespacerange not seen in CMap");
        }
    } else {
        if (*isMac == -1) {
            *isMac = 0;
        }
        if (codespace) {
            CMapMsg(h, hotFATAL, "codespace range not supported in V CMap");
        }
        if (notdef) {
            /* [xxx Actually, it is legal:] */
            CMapMsg(h, hotFATAL, "notdef range not supported in V CMap");
        }
    }

    if (last->type != PS_INTEGER) {
        CMapMsg(h, hotFATAL, "bad begin%srange count", rangeName);
    }

    nRanges = psConvInteger(ps, last);

    for (i = 0; i < nRanges; i++) {
        int loLength;
        int hiLength;
        unsigned long lo = psGetHexString(ps, &loLength);
        unsigned long hi = lo; /* takes care of charrange case */
        long cid = 0;          /* Suppress optimizer warning */
        if (!charrange) {
            hi = psGetHexString(ps, &hiLength);

            if (loLength != hiLength || lo > hi) {
                CMapMsg(h, hotFATAL, "bad range: <%0*lx> <%0*lx>", loLength, lo,
                        hiLength, hi);
            }
        }

        if (codespace) {
            if (i == 0) {
                dnaINIT(g->dnaCtx, tmpCodespace, 5, 10);
            }
            if (*isMac != 1) {
                *isMac = (loLength == 1);
            }
        } else {
            cid = psGetInteger(ps);
            if (cid < 0 || cid > 0xffff) {
                CMapMsg(h, hotFATAL, "invalid CID <%ld>", cid);
            }
            if (!(*isMac) && loLength == 1) {
                CMapMsg(h, hotFATAL,
                        " 1-byte encoded Unicode CMaps are not supported for other than the Mac encoding.");
            }
        }

        if (!vert) {
            Range *range = codespace ? dnaNEXT(tmpCodespace) : *isMac ? dnaNEXT(h->cid.mac.range) : dnaNEXT(h->cid.hor.range);
            range->flags = (loLength == 1 ? CODE_1BYTE : 0) |
                           (loLength == 4 ? CODE_4BYTE : 0) |
                           (notdef ? CODE_NOTDEF : 0);
            range->lo = (UV)lo;
            range->hi = (UV)hi;
            range->cid = codespace ? 0xffff : (CID)cid;
        } else {
            /* Store if replacement cid of vert map is present */
            /* (target cid is checked for when adding to GSUB) */
            unsigned long curruv;
            for (curruv = lo; curruv <= hi; curruv++) {
                hotGlyphInfo *giRepl = mapCID2Glyph(g, (CID)cid++);
                if (giRepl != NULL) {
                    VVMap *vv = dnaNEXT(h->cid.ver.map);
                    vv->uvTarg = (UV)curruv;
                    vv->gidRepl = GET_GID(giRepl);
                }
            }
        }
    }

    if (!psMatchToken(ps, psGetToken(ps), PS_OPERATOR,
                      codespace ? "endcodespacerange" : notdef ? "endnotdefrange" : charrange ? "endcidchar" : "endcidrange")) {
        CMapMsg(h, hotFATAL, "expecting end%srange; range count too small?",
                rangeName);
    }

    /* Store codespace for Mac only */
    if (codespace) {
        if (*isMac) {
            dnaSET_CNT(h->cid.mac.codespace, tmpCodespace.cnt);
            COPY(h->cid.mac.codespace.array, tmpCodespace.array,
                 tmpCodespace.cnt);
        }
        dnaFREE(tmpCodespace);
    }
}

/* [CMap] Single-byte before double-byte, low code, notdef before regular */
static int CDECL cmpRanges(const void *first, const void *second) {
    Range *a = (Range *)first;
    Range *b = (Range *)second;

    if ((a->flags & CODE_1BYTE) && !(b->flags & CODE_1BYTE)) {
        return -1;
    } else if (!(a->flags & CODE_1BYTE) && (b->flags & CODE_1BYTE)) {
        return 1;
    } else if (a->lo < b->lo) {
        return -1;
    } else if (a->lo > b->lo) {
        return 1;
    } else if ((a->flags & CODE_NOTDEF) && !(b->flags & CODE_NOTDEF)) {
        return -1;
    } else if (!(a->flags & CODE_NOTDEF) && (b->flags & CODE_NOTDEF)) {
        return 1;
    } else {
        return 0;
    }
}

/* [CMap] Add mapping if CID present */
static void addHorMapping(hotCtx g, unsigned long code, CID cid, short flags,
                          int isMac) {
    hotGlyphInfo *gi = mapCID2Glyph(g, cid);
    if (gi != NULL) {
        if (isMac) {
            cmapAddMapping(g, code, GET_GID(gi), (flags & CODE_1BYTE) ? 1 : 2);
        } else {
            addUVToGlyph(g, gi, code);
        }
    }
}

/* [CMap] Unravel ranges and properly layer regular over notdef. Add only if
   CID present in font.
   [xxx: Doesn't work in this situation: notdef1: 0-10, notdef2: 11-20, cid:
   5-15. Must break cid range after 10.] */
static void addRanges(hotCtx g, int isMac) {
    mapCtx h = g->ctx.map;
    long i;
    CID cid;
    unsigned long code = 0; /* Suppress optimizer warning */
    Range *notdef = NULL;
    Range *rangeArr = isMac ? h->cid.mac.range.array : h->cid.hor.range.array;
    long rangeCnt = isMac ? h->cid.mac.range.cnt : h->cid.hor.range.cnt;
    long itemCount = 0;

    /* Sort single-byte before double-byte and notdef before regular range */
    qsort(rangeArr, rangeCnt, sizeof(Range), cmpRanges);

    for (i = 0; i < rangeCnt; i++) {
        Range *range = &rangeArr[i];

        if (notdef != NULL) {
            /* Add notdef mapping before next range */
            for (;;) {
                if (code > notdef->hi) {
                    /* End of notdef range */
                    notdef = NULL;
                    break;
                } else if (code >= range->lo) {
                    /* Reached next range */
                    break;
                } else {
                    /* Add notdef mapping */
                    addHorMapping(g, code++, notdef->cid, notdef->flags,
                                  isMac);
                }
            }
        }

        if (range->flags & CODE_NOTDEF) {
            /* Save notdef range */
            notdef = range;
            code = (unsigned long)notdef->lo;
        } else {
            /* Add range mapping */
            code = (unsigned long)range->lo;
            cid = range->cid;
#if 0
            /* used for debugging parsing of ranges.*/
            {
                char tempString[256];
                sprintf((char *)&tempString, " lo <%d> hi <%d> cid <%d> item count <%d>", code, range->hi, cid, itemCount);
                hotMsg(g, hotWARNING, "adding cid range <%s>", tempString);
            }
#endif
            while (code <= range->hi) {
                itemCount++;
                addHorMapping(g, code++, cid++, range->flags, isMac);
            }
        }
    }

    if (notdef != NULL) {
        /* Add remaining notdef range */
        while (code <= notdef->hi) {
            addHorMapping(g, code++, notdef->cid, notdef->flags, isMac);
        }
    }
}

void mapAddCMap(hotCtx g, hotCMapId id, hotCMapRefill refill) {
    mapCtx h = g->ctx.map;
    psCtx ps;
    psToken *token;
    psToken last;
    SInx name = SINX_UNDEF;    /* CMap name */
    SInx usecmap = SINX_UNDEF; /* (Optional) usecmap */
    int wMode = -1;
    int isMac = -1; /* identifies Macintosh CMap */

    h->ps.cb.psId = id;
    h->ps.cb.psRefill = refill;

    if (h->cid.hor.name != SINX_UNDEF && h->cid.ver.name != SINX_UNDEF &&
        h->cid.mac.name != SINX_UNDEF) {
        CMapMsg(h, hotFATAL, "can only read 3 CMaps");
    }

    if (h->cid.hor.name == SINX_UNDEF && h->cid.ver.name == SINX_UNDEF &&
        h->cid.mac.name == SINX_UNDEF) {
        /* First CMap */
        mapInit(g);
    } else {
        h->ps.buf.cnt = 0;
    }

    /* Read in CMap */
    ps = h->ps.ctx = psNew(&h->ps.cb);
    checkCMapFirstLine(g, psGetToken(ps));
    do {
        token = psGetToken(ps);

        switch (token->type) {
            case PS_LITERAL:
                if (psMatchValue(ps, token, "/Registry")) {
                    checkCMapCompatibility(g);
                } else if (psMatchValue(ps, token, "/CMapName")) {
                    name = getCMapName(g);
                } else if (psMatchValue(ps, token, "/WMode")) {
                    wMode = getCMapWMode(g);
                } else {
                    last = *token;
                }
                break;

            case PS_OPERATOR:
                if (psMatchValue(ps, token, "usecmap")) {
                    usecmap = getUseCMap(g, &last);
                } else if (psMatchValue(ps, token, "begincodespacerange")) {
                    readRange(g, &last, 's', &isMac, wMode);
                } else if (psMatchValue(ps, token, "beginnotdefrange")) {
                    readRange(g, &last, 'n', &isMac, wMode);
                } else if (psMatchValue(ps, token, "begincidrange")) {
                    readRange(g, &last, 'c', &isMac, wMode);
                } else if (psMatchValue(ps, token, "begincidchar")) {
                    readRange(g, &last, 'd', &isMac, wMode);
                } else {
                    last = *token;
                }
                break;

            default:
                last = *token;
        }
    } while (token->type != PS_EOI);

    psFree(h->ps.ctx);
    h->ps.ctx = NULL;

    /* Check and store data */
    if (name == SINX_UNDEF) {
        CMapMsg(h, hotFATAL, "/CMapName not found");
    }
    if (wMode == 0) {
        if (isMac) {
            if (h->cid.mac.name != SINX_UNDEF) {
                CMapMsg(h, hotFATAL, "Macintosh CMap already seen");
            }
            h->cid.mac.name = name; /* Used only to flag Mac CMap seen */
        } else {
            if (h->cid.hor.name != SINX_UNDEF) {
                CMapMsg(h, hotFATAL, "H Unicode CMap already seen");
            }
            h->cid.hor.name = name;
            if (usecmap != SINX_UNDEF) {
                CMapMsg(h, hotFATAL, "usecmap not supported in H CMap");
            }

            /* Add UVs to sort.uv da */
            addRanges(g, 0);
            if (h->sort.uv.cnt == 0) {
                CMapMsg(h, hotFATAL, "No cid maps in H Unicode CMap");
            }
            qsort(h->sort.uv.array, h->sort.uv.cnt, sizeof(hotGlyphInfo *),
                  cmpUV);
        }
    } else {
        if (isMac == SINX_UNDEF) {
            isMac = 0;
        }
        if (isMac) {
            CMapMsg(h, hotFATAL, "Macintosh CMap must have /WMode 0");
        }
        if (h->cid.ver.name != SINX_UNDEF) {
            CMapMsg(h, hotFATAL, "V Unicode CMap already seen");
        }
        h->cid.ver.name = name; /* Used only to flag V CMap seen */
        if (usecmap == SINX_UNDEF) {
            CMapMsg(h, hotFATAL, "usecmap not seen in V CMap");
        }
        h->cid.ver.usecmap = usecmap;
    }

    if (h->cid.hor.name != SINX_UNDEF && h->cid.ver.name != SINX_UNDEF) {
        /* Both Unicode CMaps read in */
        if (strcmp(STR(h->cid.ver.usecmap), STR(h->cid.hor.name)) != 0) {
            CMapMsg(h, hotFATAL,
                    "usecmap resource <%s> in V Unicode CMap not "
                    "found",
                    STR(h->cid.ver.usecmap));
        }
    }
}

/* --- end CMap functions --- */
/* --- start IVS functions --- */
/* Parse  UVS file

   Unicode Variation File format.
   ========================================
   The uvs file uses a simple line-oriented ASCII format.

   Lines are terminated by either a carriage return (13), a line feed (10), or
   a carriage return followed by a line feed. Lines may not exceed 255
   characters in length (including the line terminating character(s)).

   Any of the tab (9), vertical tab (11), form feed (12), or space (32)
   characters are regarded as "blank" characters for the purposes of this
   description.

   Empty lines, or lines containing blank characters only, are ignored.

   Lines whose first non-blank character is hash (#) are treated as comment
   lines which extend to the line end and are ignored.

   All other lines depressant a single UVS record, and must contain the following fields:

   - base Unicode value as a decimal value, or hex with a preceding "0x".
   - variation selector value as a hex value
   - the supplement-order string, which may be empty
   - the CID or glyph name for the glyph selected by the record.

   Fields are separated by semi-colons.

   Valid char codes are

   A-Z  capital letters
   a-z  lowercase letters
   0-9  figures
   _    underscore
   .    period

   A glyph name may not begin with a figure or consist of a period followed by
   a sequence of digits only.

 */

/* sort function  */
static int CDECL cmpUVSEntry(const void *first, const void *second, void *cts) {
    UVSEntry *uvs_a = (UVSEntry *)first;
    UVSEntry *uvs_b = (UVSEntry *)second;

    {
        UV a = uvs_a->uvs;
        UV b = uvs_b->uvs;
        if (a < b) {
            return -1;
        } else if (a > b) {
            return 1;
        }
    }

    {
        UV a = uvs_a->uv;
        UV b = uvs_b->uv;
        if (a < b) {
            return -1;
        } else if (a > b) {
            return 1;
        }
    }

    return 0;
}

static void gnameError(hotCtx g, char *message, char *token, char *filename, long line) {
    long messageLen = strlen(message) + strlen(filename);

    if (messageLen > (256 - 6)) {
        /* 6 is max probable max length of decimal line number in Glyph Name Alias Database file */
        hotMsg(g, hotWARNING, "%s  at token %s [%s: line %ld] (record skipped)", message, token, "UVS file", line);
        hotMsg(g, hotWARNING, "UVS  file path name  is too long to include in error message. Please move Unicode Variation Selector  file to shorter absolute path.\n");
    } else {
        hotMsg(g, hotWARNING, "%s  at token %s [%s: line %ld] (record skipped)", message, token, filename, line);
    }
}

/* Validate glyph name and return a NULL pointer if the name failed to validate
   else return a pointer to character that stopped the scan. */
static char *gnameScan(hotCtx g, char *p) {
    /* Next state table */
    static unsigned char next[3][4] = {
        /*  A-Za-z_   0-9      .       *    index  */
        /* -------- ------- ------- ------- ------ */
        {      1,      0,      2,      0},  /* [0] */
        {      1,      1,      1,      0},  /* [1] */
        {      1,      2,      2,      0},  /* [2] */
    };

    /* Action table */
#define Q_ (1 << 0) /* Quit scan on unrecognized character */
#define E_ (1 << 1) /* Report syntax error */

    static unsigned char action[3][4] = {
        /*  A-Za-z_    0-9     .       *    index  */
        /* -------- ------- ------- ------- ------ */
        {     0,        0,     0,      Q_}, /* [0] */
        {     0,        0,     0,      Q_}, /* [1] */
        {     0,        0,     0,      E_}, /* [2] */
    };

    int state = 0;

    for (;;) {
        int actn;
        int class;
        int c = *p;

        /* Determine character class */
        if (c == '-') {
            class = 0;
        } else if (c == ';') {
            class = 3;
            *p = ' ';
        } else if (isalpha(c) || c == '_') {
            class = 0;
        } else if (isdigit(c)) {
            class = 1;
        } else if (c == '.') {
            class = 2;
        } else if (c == '+') {
            class = 0;
        } else {
            class = 3;
        }

        /* Fetch action and change state */
        actn = action[state][class];
        state = next[state][class];

        /* Performs actions */
        if (actn == 0) {
            p++;
            continue;
        }
        if (actn & E_) {
            return NULL;
        }
        if (actn & Q_) {
            return p;
        }
    }
}

void mapAddUVS(hotCtx g, char *uvsName) {
    long lineno;
    char buf[256];
    char *p = NULL;
    char *p2 = NULL;
    long lineLen = 0;
    UVSEntry *uvsEntry = NULL;
    mapCtx h = g->ctx.map;
    int isCID = IS_CID(g);

    dnaSET_CNT(h->uvs.entries, 0);
    g->cb.uvsOpen(g->cb.ctx, uvsName); /*jumps to fatal error if not opened successfully */

    /* uvsGetLine gets 1 line, of up to a max of 255 chars */
    for (lineno = 1; g->cb.uvsGetLine(g->cb.ctx, buf, &lineLen) != NULL; lineno++) {
        char *uv;
        char *uvs;
        char *rosName;
        char *glyphTag;
        p = buf;

        /* Skip blanks and semi-colons */
        while (isspace(*p) || (*p == ';')) {
            p++;
        }

        if (*p == '\0' || *p == '#') {
            continue; /* Skip blank or comment line */
        }
        /* Parse uv name */
        uv = p2 = p;
        p = gnameScan(g, p);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p++ = '\0';

        /* Skip blanks and semi-colons */
        while (isspace(*p) || (*p == ';')) {
            p++;
        }

        /* Parse UVS string */
        uvs = p2 = p;
        p = gnameScan(g, uvs);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p++ = '\0';

        /* Skip blanks and semi-colons */
        while (isspace(*p) || (*p == ';')) {
            p++;
        }

        /* Parse R-O-S name */
        rosName = p2 = p;
        p = gnameScan(g, rosName);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p++ = '\0';

        /* Skip blanks and semi-colons */
        while (isspace(*p) || (*p == ';')) {
            p++;
        }

        /* Parse glyphTag string */
        glyphTag = p2 = p;
        p = gnameScan(g, glyphTag);
        if (p == NULL || !isspace(*p)) {
            if (isCID) {
                gnameError(g, "For CID fonts, UVS file line must contain 'UV IVS; R-O-S; CID+<cid number>'.", buf, "", lineno);
                continue;
            } else {
                glyphTag = p2 = rosName;
            }
        }
        *p++ = '\0';

        /* We now have the entire record. */
        /* Add it to uvs array            */
        uvsEntry = dnaNEXT(h->uvs.entries);
        uvsEntry->uv = strtoul(uv, NULL, 16);
        uvsEntry->uvs = strtoul(uvs, NULL, 16);
        if (0 == strncmp(glyphTag, "CID+", 4)) {
            uvsEntry->cid = atoi(&glyphTag[4]);
            uvsEntry->gName[0] = '\0';
        } else {
            uvsEntry->cid = -1;
            strcpy(uvsEntry->gName, glyphTag);
        }

        continue;

    syntaxError:
        gnameError(g, "syntax error ", p2, uvsName, lineno);
    } /* end for all lines */

    g->cb.uvsClose(g->cb.ctx);

    /* Sort lines by UVS, then by uv */
    ctuQSort(h->uvs.entries.array, h->uvs.entries.cnt, sizeof(UVSEntry),
             cmpUVSEntry, h);

    /* Weed out duplicate entires */

#if 0 /* xxx remove when fully tested */
    {
        int i;
        printf("UVEntry Count %d\n", h->uvs.entries.cnt);
        for (i = 0; i < h->uvs.entries.cnt; i++) {
            UVSEntry *uve = &h->uvs.entries.array[i];
            printf("%X    %X %d  <%s>\n",
                   uve->uv,
                   uve->uvs,
                   uve->cid,
                   uve->gName);
        }
    }
#endif
}

/* --- end IVS functions --- */

static int CDECL cmpGlyphNameUC(const void *key, const void *value) {
    return strcmp((char *)key, ((UnicodeChar *)value)->glyphName);
}

static UnicodeChar *getUVFromAGL(hotCtx g, char *glyphName, int fatalErr) {
    UnicodeChar *found =
        (UnicodeChar *)bsearch(glyphName, agl2uv, ARRAY_LEN(agl2uv),
                               sizeof(UnicodeChar), cmpGlyphNameUC);

    if (found == NULL && fatalErr) {
        hotMsg(g, hotFATAL, "glyphName <%s> not found in internal tables",
               glyphName);
    }

    return found;
}

/* Checks if gname is of the form uni<CODE> or u<CODE>. If it is, returns 1 and
   sets *usv to the <CODE> */
static int checkUniGName(hotCtx g, char *gn, uint32_t *usv) {
#define IS_HEX(h) (isxdigit(h) && !islower(h))
    uint32_t code;
    char *pHexStart;
    char *p;
    long numHexDig;
    int uniName = 0;

    /* Check for a valid prefix */
    if (strstr(gn, "uni") == gn) {
        uniName = 1;
        pHexStart = &gn[3];
    } else if (gn[0] == 'u') {
        pHexStart = &gn[1];
    } else {
        return 0;
    }

    /* Check that rest of glyph is uppercase hex digits */
    for (p = pHexStart; *p != '\0'; p++) {
        if (!IS_HEX(*p)) {
            break;
        }
    }
    if (*p != '\0') {
        return 0;
    }

    /* Check number of hex digits and zero padding */
    numHexDig = p - pHexStart;
    if (uniName) {
        if (numHexDig != 4) {
            return 0;
        }
    } else {
        if (numHexDig < 4 || numHexDig > 6) {
            return 0;
        }
        /* Zero-padding for UV > 4 digits not allowed in glyph name */
        if (numHexDig > 4 && *pHexStart == '0') {
            return 0;
        }
    }

    /* Check code */
    sscanf(pHexStart, "%x", &code);

#if 0
    if (IN_RANGE(code, UV_SURR_BEG, UV_SURR_END)) {
        hotMsg(g, hotWARNING, "surrogate Unicode value in glyph name <%s>",
               gn);
    }
#endif
    if (IS_NONCHAR_UV(code)) {
        hotMsg(g, hotWARNING, "Unicode noncharacter value in glyph name <%s>",
               gn);
    }

    *usv = code;
    return 1;
}

/* Does any base component of gname represent a supplementary UV? e.g.
   "u10000.alt", "a_u10000" will return 1. "A.swash" or "u1000.alt" will return
   0. */
static int gnameHasSuppUV(hotCtx g, char *gname) {
    char *p;
    char gn[MAX_GNAME_LEN + 1];
    UV uv;
    int i;
    int nComp;

    if (strlen(gname) > MAX_GNAME_LEN) {
        return 0;
    }
    strcpy(gn, gname); /* gname too long */

    /* Strip off the first period and everything after it (variant) */
    p = strchr(gn, '.');
    if (p) {
        if (p == gn) {
            return 0; /* gname starts with period */
        }
        *p = '\0';
    }

    /* Change all underscores to nulls and count components: */
    nComp = 1;
    for (p = gn; *p != '\0'; p++) {
        if (*p == '_') {
            if (p == gn ||
                (p > gn && *(p - 1) == '\0') ||
                *(p + 1) == '\0') {
                return 0; /* 0-width component not valid e.g. "u10000_" */
            }
            *p = '\0';
            nComp++;
        }
    }

    /* Check whether any component has a supp UV name: */
    p = gn;
    for (i = 0; i < nComp; i++) {
        if (checkUniGName(g, p, &uv) && IS_SUPP_UV(uv)) {
            return 1;
        }
        if (i < nComp - 1) {
            p += strlen(p) + 1;
        }
    }

    return 0;
}

/* Separate lookup table for ZapfDingbats */
static void assignUVsZapfDingbats(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;
    hotGlyphInfo *gi;

    for (i = 1 /* Skip .notdef */; i < g->font.glyphs.cnt; i++) {
        UnicodeChar *found;

        gi = &g->font.glyphs.array[i];
        found = (UnicodeChar *)bsearch(gi->gname.str, zapfDingbatsList,
                                       ARRAY_LEN(zapfDingbatsList),
                                       sizeof(UnicodeChar), cmpGlyphNameUC);
        if (found != NULL) {
            addUVToGlyph(g, gi, found->uv);
        }
    }

    /* Double-map the space to no-break space */
    if ((gi = mapName2Glyph(g, "space", NULL)) != NULL) {
        addUVToGlyph(g, gi, UV_NBSPACE);
        gi->flags |= GNAME_DBLMAP;
    }

    /* Sort by uv */
    qsort(h->sort.uv.array, h->sort.uv.cnt, sizeof(hotGlyphInfo *), cmpUV);
}

/* Set code pages if not already set */
static void setCodePages(hotCtx g) {
    unsigned int i;

    if (codePage[0].isSupported != -1) {
        return;
    }
    for (i = 0; i < ARRAY_LEN(codePage); i++) {
        CodePage *cp = &codePage[i];
        cp->isSupported = mapUV2Glyph(g, cp->uv1) != NULL ||
                          (cp->uv2 != UV_UNDEF && mapUV2Glyph(g, cp->uv2) != NULL);
    }
}

static char *getNextUVName(char **uvOverrideName) {
    /* return next name in list like 'uni002D,uni00AD,uni2010,uni2011' */
    char *uvName = *uvOverrideName;
    char *nextName;
    if (uvName == NULL)
        return uvName;

    nextName = strchr(*uvOverrideName, ',');
    if (nextName == NULL) {
        *uvOverrideName = NULL;
        return uvName;
    }

    nextName[0] = '\0';
    *uvOverrideName = nextName + 1; /* set *uvOverrideName to the start of the next name. */
    return uvName;
}
/* For non-CID fonts. h->sort.gnames has been sorted by glyph name. */
static void assignUVs(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;
    h->num.unrec = h->num.unenc = 0;

    /* Special case for ZapfDingbats */
    if (strcmp(g->font.FontName.array, "ZapfDingbats") == 0) {
        assignUVsZapfDingbats(g);
        return;
    }

    /* --- Assign recognized UVs. --- */

    for (i = 1 /* Skip .notdef */; i < g->font.glyphs.cnt; i++) {
        hotGlyphInfo *gi = &g->font.glyphs.array[i];
        UV uv;
        char *uvOverrideName;

        if (checkUniGName(g, gi->gname.str, &uv)) {
            /* --- Valid uni<CODE> or u<CODE> glyph name */
            addUVToGlyph(g, gi, uv);
            gi->flags |= GNAME_UNI;
        } else if (mapName2UVOverrideName(g, gi->gname.str, &uvOverrideName) != 0) {
            char *uvName = getNextUVName(&uvOverrideName);
            while (uvName != NULL) {
                if (checkUniGName(g, uvName, &uv)) {
                    /* --- Valid uni<CODE> or u<CODE> glyph name */
                    addUVToGlyph(g, gi, uv);
                    gi->flags |= GNAME_UNI_OVERRIDE;
                }
                uvName = getNextUVName(&uvOverrideName);
            }
        } else {
            UnicodeChar *uc = getUVFromAGL(g, gi->gname.str, 0);

            if (uc != NULL &&
                gi->gname.str[strlen(gi->gname.str) - 1] != AGL2UV_DBLMAP_CHAR) {
                /* --- present in AGL */
                char dblMap[128];
                char uni[8];
                int assignedUVs = 0;

                MAKE_UNI_GLYPH(uc->uv, uni);
                if (mapName2Glyph(g, uni, NULL) == NULL) {
                    /* No uni<CODE> override is present */
                    /* xxx Check also for a u<CODE> name */
                    addUVToGlyph(g, gi, uc->uv);
                    assignedUVs++;
                }

                if (g->font.flags & HOT_DOUBLE_MAP_GLYPHS) {
                    sprintf(dblMap, "%s%c", gi->gname.str, AGL2UV_DBLMAP_CHAR);
                    if ((uc = getUVFromAGL(g, dblMap, 0)) != NULL) {
                        /* It is a double-mapped glyph name e.g. "hyphen" */
                        MAKE_UNI_GLYPH(uc->uv, uni);
                        if (mapName2Glyph(g, uni, NULL) == NULL) {
                            /* Map glyph at secondary UV also.  */
                            /* No uni<CODE> override is present */
                            addUVToGlyph(g, gi, uc->uv);
                            assignedUVs++;
                        }
                    }
                }
                if (assignedUVs == 0) {
                    h->num.unenc++;
                    gi->flags |= GNAME_UNENC;
                    hotMsg(g, hotWARNING,
                           "glyph <%s> not encoded in Unicode "
                           "cmap: overridden by uni<CODE> glyph(s)",
                           gi->gname.str);
                } else if (assignedUVs == 2) {
                    gi->flags |= GNAME_DBLMAP;
                }
            } else {
                /* --- Neither uni<CODE> nor in AGL */
                h->num.unrec++;
                gi->flags |= GNAME_UNREC;

                /* Need to identify whether it's a base or ligature of a */
                /* supp UV, for proper EUS sorting                       */

                if (gnameHasSuppUV(g, gi->gname.str)) {
                    gi->flags |= GNAME_UNREC_HAS_SUPP;
                }
            }
        }
    }

    /* Sort by uv */
    qsort(h->sort.uv.array, h->sort.uv.cnt, sizeof(hotGlyphInfo *), cmpUV);

    hotQuitOnError(g);
}

/* Sets some fields for OS/2 table; also sets mac.cmapScript and
   mac.cmapLanguage, if needed, for CID fonts. */
static void setOS_2Fields(hotCtx g) {
    mapCtx h = g->ctx.map;
    unsigned long unicodeRange[4];
    unsigned long codePageRange[2];
    unsigned i;
    hotGlyphInfo **p;
    hotGlyphInfo **q;
    hotGlyphInfo **uvarray = h->sort.uv.array;
    unsigned long uvCnt = h->sort.uv.cnt;

    /* --- firstChar, lastChar (BMP) */
    unsigned short minBMP = UV_UNDEF;
    unsigned short maxBMP = UV_UNDEF;

    if (uvCnt != 0) {
        minBMP = (unsigned short)h->minBmpUV;
        if (!(h->nSuppUV > 0)) {
            maxBMP = (unsigned short)h->maxBmpUV;
        }
    }
    OS_2SetCharIndexRange(g, minBMP, maxBMP);


    /* Xcode compiler kept complaining about signed/unsigned mismatch - can't figure out why. */
    /* OS_2SetCharIndexRange(g, (unsigned short)( (uvCnt == (unsigned)0) ? UV_UNDEF : h->minBmpUV),
                            (unsigned short)( ((uvCnt == (unsigned)0) || (h->nSuppUV > (unsigned)0)) ? UV_UNDEF : h->maxBmpUV)); */
    /* --- unicodeRange */

    unicodeRange[0] = unicodeRange[1] = unicodeRange[2] = unicodeRange[3] = 0;

    if (uvCnt != 0) {
        /* Loop thru sorted UV array (saves bsearches for primary UVs). Addl */
        /* UVs have already been recorded by calls to addToBlock, which does */
        /* use bsearch                                                       */
        p = uvarray;
        for (i = 0; i < ARRAY_LEN(unicodeBlock); i++) {
            UnicodeBlock *block = &unicodeBlock[i];
            unsigned long numRequired;

            for (; p <= &uvarray[uvCnt - 1] && (*p)->uv < block->first; p++) {
            }
            if (p <= &uvarray[uvCnt - 1] && (*p)->uv <= block->last) {
                /* p now points at the first UV candidate for this block */
                for (q = p; (q + 1) <= &uvarray[uvCnt - 1] &&
                            (*(q + 1))->uv <= block->last;
                     q++) {
                }
                block->numFound += q - p + 1;
                p = q + 1;
            }
            numRequired = 1 + block->numDefined / 3;

            /* The numDefined field is set for each block in uniblock.h.  */
            /* If at least one third of the characters for the block are  */
            /* included in the font, the designer was making a reasonable */
            /* effort to support this block. Note that if numDefined==1,  */
            /* it means we want to set the corresponding bit if we see    */
            /* *any* characters in this block, e.g. for PUA & CJK blocks. */

            if (block->numFound >= numRequired) {
                block->isSupported = 1;
                SET_BIT_ARR(unicodeRange, block->bitNum);
            }
            /*
            else if (block->numFound > 0)
            {
                printf(" %4d %4d, name %s.\n", block->numFound,  block->numDefined,  block->name);
            }
            */
        }
    }
    if (h->nSuppUV > 0) {
        SET_BIT_ARR(unicodeRange, SUPP_UV_BITNUM);
    }

    OS_2SetUnicodeRanges(g, unicodeRange[0], unicodeRange[1],
                         unicodeRange[2], unicodeRange[3]);

    /* --- code pages */

    codePageRange[0] = codePageRange[1] = 0;

    /* Set single-byte code pages according to whether UV present */
    setCodePages(g);

    for (i = 0; i < ARRAY_LEN(codePage); i++) {
        if (codePage[i].bitNum >= 0 && codePage[i].isSupported) {
            SET_BIT_ARR(codePageRange, codePage[i].bitNum);
        }
    }

    /* Set symbol code page iff WinCharSet == 2 */
    if (g->font.win.CharSet == SYMBOL_CHARSET) {
        SET_BIT_ARR(codePageRange, SYMBOL_BIT); /* Symbol bit */
    }
    /* Set Latin (Windows 1252) bit if OEM  code page is set. */

    /* rroberts Nov 21 2002. I am re-enabling this, as it turns out that a  */
    /* font with only the OEM code page set is not well supported by        */
    /* Windows 2000 CharMap. This happens to GamePiLTStd-ChessDraughts.otf, */
    /* as it has the glyph name "shade", and gets the OEM_BIT set.          */
    if (TEST_BIT_ARR(codePageRange, OEM_BIT)) {
        SET_BIT_ARR(codePageRange, LATIN_BIT);
    }

    /* If the mac bit is set make sure that the LATIN_BIT is set as well. */
    /* Otherwise, for a few fonts with just the basic latin set, like     */
    /* LucidaMathStd-italic, only the Mac bit gets set, and this seems to */
    /* be ignored by Windows, so you can't type any test. with it.        */
    if (TEST_BIT_ARR(codePageRange, MAC_BIT)) {
        SET_BIT_ARR(codePageRange, LATIN_BIT);
    }

    /* Set CJK code pages according to other heuristics */
    if (unicodeBlock[CJK_IDEO_INX].isSupported &&
        unicodeBlock[BOPOMOFO_INX].isSupported) {
        if (g->font.mac.cmapScript == MAC_SCR_CHN_SIMP) {
            SET_BIT_ARR(codePageRange, CHN_SIMP_CP);
        } else if (g->font.mac.cmapScript == MAC_SCR_CHN_TRAD) {
            SET_BIT_ARR(codePageRange, CHN_TRAD_CP);
        } else {
            unsigned int accPresent = 0;

            /* Presence of at least half accented characters denotes Simpl. */
            for (i = 0; i < ARRAY_LEN(schinese); i++) {
                if (mapUV2Glyph(g, schinese[i]) != NULL) {
                    if (++accPresent >= ARRAY_LEN(schinese) / 2) {
                        break;
                    }
                }
            }
            if (accPresent >= ARRAY_LEN(schinese) / 2) {
                SET_BIT_ARR(codePageRange, CHN_SIMP_CP);
                if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
                    g->font.mac.cmapScript = MAC_SCR_CHN_SIMP;
                }
            } else {
                SET_BIT_ARR(codePageRange, CHN_TRAD_CP);
                if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
                    g->font.mac.cmapScript = MAC_SCR_CHN_TRAD;
                }
            }
        }
    }

    if (unicodeBlock[HANGUL_SYL_INX].isSupported) {
        SET_BIT_ARR(codePageRange, KOR_WANSUNG_CP);
        SET_BIT_ARR(codePageRange, KOR_JOHAB_CP); /* obsolete? */
        if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
            g->font.mac.cmapScript = MAC_SCR_KOR;
        }
    }

    if (unicodeBlock[HIRAGANA_INX].isSupported ||
        unicodeBlock[KATAKANA_INX].isSupported) {
        SET_BIT_ARR(codePageRange, JIS_CP);
        if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
            g->font.mac.cmapScript = MAC_SCR_JAPANESE;
        }
    }

    /* if no code page range bits are set, then set the Latin (Windows 1252) code page. */
    {
        int somePage = 0;
        for (i = 0; i < ARRAY_LEN(codePage); i++) {
            if (codePage[i].isSupported) {
                somePage = 1;
                break;
            }
        }

        if (!somePage) {
            SET_BIT_ARR(codePageRange, LATIN_BIT); /* Latin (Windows 1252) bit */
        }
    }

    OS_2SetCodePageRanges(g, codePageRange[0], codePageRange[1]);
}

/* Create a custom cmap which stores the custom PS encoding. */
static void makeCustomcmap(hotCtx g) {
    long i;
    uint8_t charset = g->font.win.CharSet;
    unsigned platSpec = 0;

    /* Set lower byte of platSpec to charset */
    platSpec = charset;

    cmapBeginEncoding(g, cmap_CUSTOM, platSpec, 0);

    for (i = 1; i < g->font.glyphs.cnt; i++) {
        /* Skip .notdef */
        hotGlyphInfo *gi = &g->font.glyphs.array[i];

        if (gi->code != CFF_UNENC) {
            cffSupCode *sup;

            /* Add first encoding */
            cmapAddMapping(g, gi->code, i, 1);

            /* Add supplementary encodings */
            for (sup = gi->sup; sup != NULL; sup = sup->next) {
                cmapAddMapping(g, sup->code, i, 1);
            }
        }
    }
    cmapEndEncoding(g); /* Custom */
}

/* Check if Mac heuristic glyphs are present in the given encoding (don't check
   unencoded glyphs). Return codePage index if detected, -1 otherwise. (This
   will only be called in situations 1 and 2 of makeNonCIDMaccmap(), which are
   the rarer cases, so the linear searching is OK.) */
static int doMacHeuristics(hotCtx g, hotEncoding *hotEnc, int usePlatEnc) {
    mapCtx h = g->ctx.map;
    unsigned int i;
    int j;

    for (i = 0; i < ARRAY_LEN(codePage); i++) {
        char uniGN1[8];
        char uniGN2[8];
        CodePage *cp = &codePage[i];

        if (cp->maccmapScript == HOT_CMAP_UNKNOWN || !cp->isSupported ||
            IS_MATH_FONT(cp->script)) {
            continue;
        }

        /* One of the heuristic glyphs is in the font (and it's not a Math */
        /* font). Now check whether the heuristic glyph's in the actual    */
        /* encoding:                                                       */
        MAKE_UNI_GLYPH(cp->uv1, uniGN1);
        if (cp->uv2 != UV_UNDEF) {
            MAKE_UNI_GLYPH(cp->uv2, uniGN2);
        }

        for (j = 255; j >= 0; j--) {
            char *gname;

            if (hotEnc) {
                gname = (*hotEnc)[j];
            } else if (usePlatEnc) {
                if (h->sort.platEnc[j] != NULL &&
                    h->sort.platEnc[j] != g->font.glyphs.array) {
                    gname = h->sort.platEnc[j]->gname.str;
                } else {
                    continue;
                }
            } else {
                hotMsg(g, hotWARNING, "can't do Mac heuristics");
                return -1;
            }

            if (gname != NULL && strcmp(gname, ".notdef") != 0 &&
                ((cp->gn1 != NULL && strcmp(gname, cp->gn1) == 0) ||
                 strcmp(gname, uniGN1) == 0 ||
                 (cp->uv2 != UV_UNDEF &&
                  ((cp->gn2 != NULL && strcmp(gname, cp->gn2) == 0) ||
                   strcmp(gname, uniGN2) == 0)))) {
                return i;
            }
        }
    }

    return -1;
}

/* Assign script and language if needed; create Mac cmap from one of hotEnc,
   uvEnc or platEnc. (Note: we can't ever simply point to the custom cmap
   because the Mac cmap C0 control codes will always make it different. */
static void createMaccmap(hotCtx g, long script, long language,
                          hotEncoding *hotEnc, UV_BMP *uvEnc, int usePlatEnc) {
    int i;
    GID gid;
    int macCPInx = -2;

    /* --- Assign script and language, if needed. --- */

    if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
        if (script != HOT_CMAP_UNKNOWN) {
            g->font.mac.cmapScript = script;
        } else {
            macCPInx = doMacHeuristics(g, hotEnc, usePlatEnc);
            g->font.mac.cmapScript = (macCPInx >= 0) ? codePage[macCPInx].maccmapScript : MAC_SCR_ROMAN;
        }
    }

    if (g->font.mac.cmapLanguage == HOT_CMAP_UNKNOWN) {
        if (language != HOT_CMAP_UNKNOWN) {
            g->font.mac.cmapLanguage = language;
        } else {
            if (macCPInx == -2) {
                macCPInx = doMacHeuristics(g, hotEnc, usePlatEnc);
            }
            g->font.mac.cmapLanguage = (macCPInx >= 0) ? codePage[macCPInx].maccmapLanguage : MAC_LANG_UNSPEC;
        }
    }

    /* --- Create cmap from hotEnc, uvEnc or platEnc --- */

    cmapBeginEncoding(g, cmap_MAC, g->font.mac.cmapScript,
                      g->font.mac.cmapLanguage);

    if (hotEnc != NULL) {
        for (i = 0; i < 256; i++) {
            char *gname = (*hotEnc)[i];
            if (gname != NULL && strcmp(gname, ".notdef") != 0) {
                gid = mapName2GID(g, gname, NULL);
                if (gid != GID_UNDEF) {
                    cmapAddMapping(g, i, gid, 1);
                }
            }
        }
    } else if (uvEnc != NULL) {
        for (i = 0; i < 256; i++) {
            if (uvEnc[i] != UV_UNDEF) {
                gid = mapUV2GID(g, uvEnc[i]);
                if (gid != GID_UNDEF) {
                    cmapAddMapping(g, i, gid, 1);
                }
            }
        }
    } else if (usePlatEnc) {
        for (i = 0; i < 256; i++) {
            gid = mapPlatEnc2GID(g, i);
            if (gid != GID_UNDEF) {
                cmapAddMapping(g, i, gid, 1);
            }
        }
    }

    cmapEndEncoding(g); /* nonCID Mac */
}

/* Create a Mac cmap for non-CID fonts */
static void makeNonCIDMaccmap(hotCtx g, hotEncoding *comEncoding,
                              hotEncoding *macEncoding) {
/* #if HOT_DEBUG && 1 */
#if 0
#define MAC_MSG(msg) hotMsg msg
#else
#define MAC_MSG(msg)
#endif
    unsigned i;
    int macEncInx;

    if (macEncoding != NULL) {
        MAC_MSG((g, hotNOTE, "[1. FONT.NSMR or fcdb entry]"));
        if (g->font.flags & HOT_MAC) {
            createMaccmap(g, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN,
                          NULL, NULL, 1 /* usePlatEnc */);
        } else {
            createMaccmap(g, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN,
                          macEncoding, NULL, 0);
        }
        return;
    }

    if (g->font.flags & HOT_MAC && comEncoding == NULL) {
        MAC_MSG((g, hotNOTE, "[2. Create cmap from PS encoding (link)]"));
        createMaccmap(g, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN,
                      NULL, NULL, 1 /* usePlatEnc */);
        return;
    }

    if ((g->font.flags & HOT_MAC && comEncoding != NULL) ||
        g->font.Encoding == FI_STD_ENC) {
        MAC_MSG((g, hotNOTE, "[3. Create Mac Std Roman]"));
        createMaccmap(g, MAC_SCR_ROMAN, MAC_LANG_UNSPEC,
                      NULL, macEnc[0] /* Mac Roman */, 0);
        return;
    }

    if (g->font.Encoding == FI_EXP_ENC) {
        static hotEncoding macExpert = {
#include "macexprt.h"
        };
        MAC_MSG((g, hotNOTE, "[4. Create Mac Expert]"));
        createMaccmap(g, MAC_SCR_ROMAN, MAC_LANG_UNSPEC,
                      &macExpert, NULL, 0);
        return;
    }

    /* 5. Check heuristic UVs: */
    macEncInx = -1;
    for (i = 0; i < ARRAY_LEN(codePage); i++) {
        CodePage *cp = &codePage[i];
        if (cp->maccmapScript != HOT_CMAP_UNKNOWN) {
            macEncInx++;
            if (cp->isSupported && !IS_MATH_FONT(cp->script)) {
                MAC_MSG((g, hotNOTE, "[5. heuristic UVs: macEncInx %d]",
                         macEncInx));
                if (macEncInx >= (long)ARRAY_LEN(macEnc)) {
                    hotMsg(g, hotFATAL, "macEnc index %d not supported",
                           macEncInx);
                }
                createMaccmap(g, cp->maccmapScript, cp->maccmapLanguage,
                              NULL, macEnc[macEncInx], 0);
                return;
            }
        }
    }

#if 0
    MAC_MSG((g, hotNOTE, "[6. Record PS encoding (link)]"));
    createMaccmap(g, MAC_SCR_ROMAN, MAC_LANG_UNSPEC,
                  NULL, NULL, 1 /* usePlatEnc */);
#endif
    MAC_MSG((g, hotNOTE, "[6. Failed to identify lang encoding, Force MacRoman]"));
    createMaccmap(g, MAC_SCR_ROMAN, MAC_LANG_UNSPEC,
                  NULL, macEnc[0] /* Mac Roman */, 0);
}

/* Create 16-bit and 32-bit (if needed) Unicode cmaps */
static void makeUnicodecmaps(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;

    /* Create the BMP UTF-16 cmaps */
    cmapBeginEncoding(g, cmap_MS, cmap_MS_UGL, 0);
    for (i = 0; i < h->sort.uv.cnt; i++) {
        AddlUV *adUV;
        hotGlyphInfo *gi = h->sort.uv.array[i];

        if (gi->flags & GNAME_UNREC_HAS_SUPP) {
            continue;
        }

        if (!IS_SUPP_UV(gi->uv)) {
            cmapAddMapping(g, gi->uv, GET_GID(gi), 2);
        }
        for (adUV = gi->addlUV; adUV != NULL; adUV = adUV->next) {
            if (!IS_SUPP_UV(adUV->uv)) {
                cmapAddMapping(g, adUV->uv, GET_GID(gi), 2);
            }
        }
    }
    if (cmapEndEncoding(g)) {
        cmapPointToPreviousEncoding(g, cmap_UNI, cmap_UNI_UTF16_BMP);
    } else {
        return;
    }

    /* Create UTF-32 cmaps, if needed, as supersets of the BMP UTF-16 cmaps: */
    if (h->nSuppUV == 0) {
        return;
    }

    cmapBeginEncoding(g, cmap_MS, cmap_MS_UCS4, 0);
    for (i = 0; i < h->sort.uv.cnt; i++) {
        AddlUV *adUV;
        hotGlyphInfo *gi = h->sort.uv.array[i];

        cmapAddMapping(g, gi->uv, GET_GID(gi), 4);
        for (adUV = gi->addlUV; adUV != NULL; adUV = adUV->next) {
            cmapAddMapping(g, adUV->uv, GET_GID(gi), 4);
        }
    }
    if (cmapEndEncoding(g)) {
        cmapPointToPreviousEncoding(g, cmap_UNI, cmap_UNI_UTF32);
    }
}

/* Make Mac cmap for CID fonts */
static void makeCIDMaccmap(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;

    if (g->font.mac.cmapScript == HOT_CMAP_UNKNOWN) {
        hotMsg(g, hotWARNING,
               "can't autodetect Macintosh cmap script for CID "
               "font <%s>; defaulting to Roman script",
               g->font.FontName.array);
        g->font.mac.cmapScript = MAC_SCR_ROMAN;
    }
    if (g->font.mac.cmapLanguage == HOT_CMAP_UNKNOWN) {
        g->font.mac.cmapLanguage = MAC_LANG_UNSPEC;
    }

    cmapBeginEncoding(g, cmap_MAC, g->font.mac.cmapScript,
                      g->font.mac.cmapLanguage);
    for (i = 0; i < h->cid.mac.codespace.cnt; i++) {
        Range *range = &h->cid.mac.codespace.array[i];
        cmapAddCodeSpaceRange(g, range->lo, range->hi,
                              (range->flags & CODE_1BYTE) ? 1 : 2);
    }
    addRanges(g, 1);
    cmapEndEncoding(g); /* CID Mac  */
}

/* will make an Unicode Variation Selector cmap 14. */
static void makeUVScmap(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;

    cmapBeginEncoding(g, cmap_UNI, cmap_UNI_IVS,
                      0);
    for (i = 0; i < h->uvs.entries.cnt; i++) {
        unsigned int uvsFlags = 0;
        UVSEntry *uve = &h->uvs.entries.array[i];
        hotGlyphInfo *gi_uv = NULL;
        hotGlyphInfo *gi_id = NULL;
        GID uveGID;

        if (IS_SUPP_UV(uve->uv)) {
            uvsFlags |= UVS_IS_SUPPLEMENT;
        }

        if (IS_CID(g)) {
            gi_id = mapCID2Glyph(g, uve->cid);
            if (gi_id == NULL) {
                hotMsg(g, hotWARNING, "Skipping UVS entry for CID '%d': not found in source font.", uve->cid);
                continue;
            }
        } else {
            char *finalName;
            gi_id = mapName2Glyph(g, uve->gName, &finalName);
            if (gi_id == NULL) {
                hotMsg(g, hotWARNING, "Skipping  UVS entry for glyph name '%s': not found in source font.", uve->gName);
                continue;
            }
            if (finalName != NULL) {
                strcpy(uve->gName, finalName);
            }
        }

        uveGID = GET_GID(gi_id);

        gi_uv = mapUV2Glyph(g, uve->uv);
        if ((gi_uv != NULL) && (uveGID == GET_GID(gi_uv))) {
            uvsFlags |= UVS_IS_DEFAULT;
        }

        cmapAddUVSEntry(g, uvsFlags, uve->uv, uve->uvs, uveGID);
    }
    cmapEndUVSEncoding(g);
}

/* ---------------------------- AFM printing ------------------------------- */

/* Get GID for kerning glyph */
static GID kernGetGID(hotCtx g, unsigned index) {
    if (index & KERN_UNENC) {
        /* Index is unencoded char index */
        int iChar = index & ~KERN_UNENC;

        return (iChar >= g->font.unenc.cnt) ? GID_UNDEF : mapName2GID(g, g->font.unenc.array[iChar].array, NULL);
    } else {
        /* Index is char code */
        return (index > 255) ? GID_UNDEF : mapPlatEnc2GID(g, index);
    }

    return 0; /* Suppress compiler warning */
}

static int CDECL cmpAFMOrder(const void *first, const void *second) {
    short a = ((AFMChar *)first)->code;
    short b = ((AFMChar *)second)->code;

    if (a != AFM_UNENC && b == AFM_UNENC) {
        return -1;
    } else if (a == AFM_UNENC && b != AFM_UNENC) {
        return 1;
    } else if (a != AFM_UNENC && b != AFM_UNENC) {
        if (a < b) {
            return -1;
        } else {
            return (a > b) ? 1 : 0;
        }
    } else {
        return strcmp(((AFMChar *)first)->gi->gname.str,
                      ((AFMChar *)second)->gi->gname.str);
    }
}

static void AFMPrintCharMetrics(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;

    if (IS_CID(g)) {
        /* Print CID glyphs in CID order */

        printf("StartCharMetrics %ld\n", h->sort.gname.cnt);
        for (i = 0; i < h->sort.gname.cnt; i++) {
            hotGlyphInfo *gi = h->sort.gname.array[i];
            printf("C %d ; W0X %hd ; N %hu ; B %hd %hd %hd %hd ;\n",
                   gi->code, gi->hAdv, gi->id,
                   gi->bbox.left, gi->bbox.bottom,
                   gi->bbox.right, gi->bbox.top);
        }
        printf("EndCharMetrics\n");
    } else {
        /* Store and sort AFM chars in canonical order. */
        /* Caution: multiply encoded glyphs             */
        long nGlyphs = g->font.glyphs.cnt;
        dnaDCL(AFMChar, chars);
        dnaINIT(g->dnaCtx, chars, 400, 7000);

        for (i = 1; i < nGlyphs; i++) {
            cffSupCode *sup;
            hotGlyphInfo *gi = &g->font.glyphs.array[i];
            AFMChar *next = dnaNEXT(chars);

            next->code = (gi->code == CFF_UNENC) ? AFM_UNENC : gi->code;
            next->gi = gi;
            for (sup = gi->sup; sup != NULL; sup = sup->next) {
                next = dnaNEXT(chars);
                next->code = sup->code;
                next->gi = gi;
            }
        }
        qsort(chars.array, chars.cnt, sizeof(AFMChar), cmpAFMOrder);

        printf("StartCharMetrics %ld\n", chars.cnt);
        for (i = 0; i < chars.cnt; i++) {
            AFMChar *afmchar = &chars.array[i];
            hotGlyphInfo *gi = afmchar->gi;

            printf("C %d ; WX %hd ; ", afmchar->code, gi->hAdv);
            if (IS_CID(g)) {
                printf("N %hu ; ", gi->id);
            } else {
                printf("N %s ; ", gi->gname.str);
            }
            printf("B %hd %hd %hd %hd ;",
                   gi->bbox.left, gi->bbox.bottom,
                   gi->bbox.right, gi->bbox.top);
            if (g->font.Encoding == FI_STD_ENC &&
                strcmp(gi->gname.str, "f") == 0) {
                if (mapName2Glyph(g, "fi", NULL) != NULL) {
                    printf(" L i fi ;");
                }
                if (mapName2Glyph(g, "fl", NULL) != NULL) {
                    printf(" L l fl ;");
                }
            }
            printf("\n");
        }
        printf("EndCharMetrics\n");

        dnaFREE(chars);
    }
}

static int CDECL cmpKernNamePair(const void *first, const void *second) {
    KernNamePair *a = (KernNamePair *)first;
    KernNamePair *b = (KernNamePair *)second;
    int cmp = strcmp(a->first, b->first);

    if (cmp != 0) {
        return cmp;
    } else {
        return strcmp(a->second, b->second);
    }
}

static void AFMPrintKernData(hotCtx g) {
    int i;
    int nPairs = g->font.kern.pairs.cnt;
    dnaDCL(KernNamePair, pairs);
    dnaINIT(g->dnaCtx, pairs, 1500, 1000);

    if (nPairs == 0) {
        return;
    } else if (IS_CID(g)) {
        hotMsg(g, hotWARNING, "can't print AFM KernData for CID");
        return;
    }

    /* Store and sort kern data by gname */
    for (i = 0; i < nPairs; i++) {
        KernNamePair *kern;
        KernPair_ *pair = &g->font.kern.pairs.array[i];
        GID first = kernGetGID(g, pair->first);
        GID second = kernGetGID(g, pair->second);

        if (first == GID_UNDEF || second == GID_UNDEF) {
            continue;
        }

        kern = dnaNEXT(pairs);
        kern->first = g->font.glyphs.array[first].gname.str;
        kern->second = g->font.glyphs.array[second].gname.str;
        kern->value = g->font.kern.values.array[i];
    }
    qsort(pairs.array, pairs.cnt, sizeof(KernNamePair), cmpKernNamePair);

    /* Print kern data */
    printf(
        "StartKernData\n"
        "StartKernPairs %ld\n",
        pairs.cnt);
    for (i = 0; i < pairs.cnt; i++) {
        printf("KPX %s %s %d\n",
               pairs.array[i].first,
               pairs.array[i].second,
               pairs.array[i].value);
    }
    printf(
        "EndKernPairs\n"
        "EndKernData\n");

    dnaFREE(pairs);
}

/* Write string corresponding to sid */
static void SIDWrite(hotCtx g, SID sid) {
    if (sid != SID_UNDEF) {
        unsigned length;
        char *value = hotGetString(g, sid, &length);
        printf("%.*s", (int)length, value);
    }
}

/* Write AFM data */
void mapPrintAFM(hotCtx g) {
    printf("StartFontMetrics 2.0\n");

    printf("FontName %s\n", g->font.FontName.array);
    SID_WRITE_FIELD(g, "FullName", g->font.FullName);
    SID_WRITE_FIELD(g, "FamilyName", g->font.FamilyName);
    printf("ItalicAngle %g\n", FIX2DBL(g->font.ItalicAngle));
    printf("IsFixedPitch %s\n",
           (g->font.flags & FI_FIXED_PITCH) ? "true" : "false");
    printf("FontBBox %hd %hd %hd %hd\n",
           g->font.bbox.left, g->font.bbox.bottom,
           g->font.bbox.right, g->font.bbox.top);
    if (IS_CID(g)) {
        printf("CharacterSet ");
        SIDWrite(g, g->font.cid.registry);
        printf("-");
        SIDWrite(g, g->font.cid.ordering);
        printf("-%hu\n", g->font.cid.supplement);
    }
    printf("UnderlinePosition %hd\n", g->font.UnderlinePosition);
    printf("UnderlineThickness %hd\n", g->font.UnderlineThickness);
    SID_WRITE_FIELD(g, "Notice", g->font.Notice);
    printf("EncodingScheme %s\n", (g->font.Encoding == FI_STD_ENC) ? "AdobeStandardEncoding" : "FontSpecific");

    AFMPrintCharMetrics(g);

    AFMPrintKernData(g);

    printf("EndFontMetrics\n");
}

#if HOT_DEBUG

static void dbgUniBlock(hotCtx g) {
    unsigned int i;

    for (i = 0; i < ARRAY_LEN(unicodeBlock); i++) {
        UnicodeBlock *block = &unicodeBlock[i];
        printf("%2d  (%3d)  %s\n", i, block->numFound, block->name);
    }
}

static void dbgPrintUV(UV uv) {
    if (IS_SUPP_UV(uv)) {
        fprintf(stderr, "%uX", uv);
    } else {
        fprintf(stderr, "%04uX", uv);
    }
}

static void dbgPrintInfo(hotCtx g) {
    mapCtx h = g->ctx.map;
    uint32_t i;
    uint32_t nGlyphs = g->font.glyphs.cnt;
#define SUBSET_MAX 1000 /* Heuristic for printing */

    if (!IS_CID(g)) {
        fprintf(stderr, "dfCharSet     = %02x (%d)\n", g->font.win.CharSet,
                g->font.win.CharSet);
    }

    fprintf(stderr,
            "sort (gname.cnt, uv.cnt, glyphAddlUV.cnt) = (%ld, %ld, %ld)\n",
            h->sort.gname.cnt, h->sort.uv.cnt, h->sort.glyphAddlUV.cnt);
    fprintf(stderr, "      (firstAddlUV, lastAddlUV, nAddlUV, nSupplUV) = (");
    dbgPrintUV(h->sort.firstAddlUV);
    fprintf(stderr, ", ");
    dbgPrintUV(h->sort.lastAddlUV);
    fprintf(stderr, ", %u, %u)\n", h->sort.nAddlUV, h->nSuppUV);

    fprintf(stderr, "num (unrec,unenc) = (%d,%d)\n",
            h->num.unrec, h->num.unenc);

    /* dbgUniBlock(g); */

    if (IS_CID(h->g)) {
        fprintf(stderr,
                "------------------------------\n"
                "gid        cid        uv   ... %s"
                "------------------------------\n",
                (nGlyphs > SUBSET_MAX) ? " [only multiple uv maps listed]\n"
                                       : "");
        for (i = 0; i < nGlyphs; i++) {
            AddlUV *addlUV;
            hotGlyphInfo *gi = &g->font.glyphs.array[i];

            if (nGlyphs <= SUBSET_MAX || gi->addlUV != NULL) {
                fprintf(stderr, "%4X(%4ud) ", i, i);
                if (i != gi->id) {
                    fprintf(stderr, "%4hX(%4d) ", gi->id, gi->id);
                } else {
                    fprintf(stderr, "<         ");
                }
                if (gi->uv != UV_UNDEF) {
                    dbgPrintUV(gi->uv);
                }
                for (addlUV = gi->addlUV; addlUV != NULL; addlUV = addlUV->next) {
                    fprintf(stderr, ",");
                    dbgPrintUV(addlUV->uv);
                }
                fprintf(stderr, "\n");
            }
        }
        return;
    }

    fprintf(stderr,
            "--------------------------------------------------\n"
            "uv...      enc...  gid    sid      flg gname\n"
            "--------------------------------------------------\n");

    for (i = 0; i < (uint32_t)h->sort.gname.cnt; i++) {
        cffSupCode *sup;
        AddlUV *addlUV;
        hotGlyphInfo *gi = h->sort.gname.array[i];

        if (gi->uv != UV_UNDEF) {
            dbgPrintUV(gi->uv);
        } else {
            fprintf(stderr, "-?- ");
        }
        if (gi->addlUV == NULL) {
            fprintf(stderr, "      ");
        } else {
            for (addlUV = gi->addlUV; addlUV != NULL; addlUV = addlUV->next) {
                fprintf(stderr, ",");
                dbgPrintUV(addlUV->uv);
            }
        }

        fprintf(stderr, " %3d", gi->code);
        if (gi->sup == NULL) {
            fprintf(stderr, "     ");
        } else {
            for (sup = gi->sup; sup != NULL; sup = sup->next) {
                fprintf(stderr, ",%3d", sup->code);
            }
        }
        fprintf(stderr, " %4td ", GET_GID(gi));
        if (GET_GID(gi) != gi->id) {
            fprintf(stderr, "%4d", gi->id);
        } else {
            fprintf(stderr, " <  ");
        }

        fprintf(stderr, " %02x", gi->flags);

        fprintf(stderr, " %-20s", gi->gname.str);

        if (gi->flags & GNAME_UNI) {
            fprintf(stderr, " UNI");
        }
        if (gi->flags & GNAME_UNREC) {
            fprintf(stderr, " UNREC");
        }
        if (gi->flags & GNAME_UNREC_HAS_SUPP) {
            fprintf(stderr, "_HAS_SUPP");
        }
        if (gi->flags & GNAME_DBLMAP) {
            fprintf(stderr, " DBLMAP");
        }
        if (gi->flags & GNAME_UNENC) {
            fprintf(stderr, " UNENC");
        }

        fprintf(stderr, "\n");
    }
}

/* This function just serves to suppress "defined but not used" compiler
   messages when debugging */
static void CDECL dbgUse(int arg, ...) {
    dbgUse(0, dbgUniBlock);
}

#endif /* HOT_DEBUG */

/* Any CMaps have already been read in by mapAddCmap */
int mapFill(hotCtx g) {
    mapCtx h = g->ctx.map;

    if (IS_CID(g) && h->cid.hor.name == SINX_UNDEF) {
        hotMsg(g, hotFATAL, "H Unicode CMap not seen");
    }

    /* Roman Custom cmap made in mapInit(), Roman Mac cmap made in mapApplyReencoding(). */
    makeUnicodecmaps(g);

    setOS_2Fields(g);
    if (IS_CID(g)) {
        makeCIDMaccmap(g); /* will make a Mac cmap with only a not def, if Macintosh Adobe CMap is not provided.*/
        if (h->cid.mac.name == SINX_UNDEF) {
            hotMsg(g, hotWARNING, "Macintosh Adobe CMap not seen");
        }

        if (h->uvs.entries.cnt == 0) {
            hotMsg(g, hotWARNING, "Unicode variation Selector File not seen");
        }
    }

    if (h->uvs.entries.cnt > 0) {
        makeUVScmap(g); /* will make a Unicode Variation Selector cmap 14  when UVS data file was seen..*/
    }
#if HOT_DEBUG
    if (DB_MAP(g)) {
        dbgPrintInfo(g);
    }
#endif

    return 1;
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Initialize both CID and non-CID fonts */
static void initGlyphs(hotCtx g) {
    mapCtx h = g->ctx.map;
    int32_t i;
    int32_t nGlyphs = g->font.glyphs.cnt;

    dnaSET_CNT(h->sort.gname, nGlyphs); /* Includes .notdef/CID 0 */

    if (!IS_CID(g)) {
        for (i = 0; i < 256; i++) {
            h->sort.platEnc[i] = NULL;
        }
    }

    for (i = 0; i < nGlyphs; i++) {
        hotGlyphInfo *gi = &g->font.glyphs.array[i];

        gi->flags = 0;
        gi->uv = UV_UNDEF;
        gi->addlUV = NULL;
        h->sort.gname.array[i] = gi; /* Init sort ptrs to array elements */

        if (!IS_CID(g)) {
            unsigned length;
            char *str = hotGetString(g, gi->id, &length);

            gi->gname.inx = addString(h, str, length);
            if (gi->code != CFF_UNENC) {
                cffSupCode *p;
                h->sort.platEnc[gi->code] = gi; /* PS encoding */
                for (p = gi->sup; p != NULL; p = p->next) {
                    h->sort.platEnc[p->code] = gi; /* Supp PS encodings */
                }
            }
        }
    }

    if (!IS_CID(g)) {
        /* Store gnames as char * instead of SInx */
        for (i = 0; i < nGlyphs; i++) {
            char *srcName;
            hotGlyphInfo *gi = &g->font.glyphs.array[i];
            gi->gname.str = STR(gi->gname.inx);
            // gi->gname.str have all been renamed to the final strings.
            srcName = g->cb.getSrcGlyphName(g->cb.ctx, gi->gname.str);
            if (strcmp(srcName, gi->gname.str) != 0) {
                gi->srcName = srcName;
            }
        }
    }

    /* Sort by glyph name/CID */
    qsort(h->sort.gname.array, h->sort.gname.cnt, sizeof(hotGlyphInfo *),
          IS_CID(g) ? cmpCID : cmpGlyphName);

    /* Make custom cmap, if applicable */
    /*
    if (!IS_CID(g) && g->font.Encoding != FI_STD_ENC)
        makeCustomcmap(g);
    */
}

/* Initialize both CID and non-CID fonts */
static void mapInit(hotCtx g) {
    initGlyphs(g);
    if (!IS_CID(g)) {
        assignUVs(g);
        setCodePages(g);
    }

    /* non-CID: gname and uv lookup utilities ready to be used.             */
    /*          platEnc lookups ready after hotAddMiscData is done.         */
    /* CID:     CID lookup utility ready to be used. uv will be ready after */
    /*          Unicode CMaps have been read in (mapAddCMap).               */
}

/* Prepare platEnc[] to resolve kern pairs */
void mapApplyReencoding(hotCtx g, hotEncoding *comEncoding,
                        hotEncoding *macEncoding) {
    mapCtx h = g->ctx.map;
    hotEncoding *reenc = NULL;
    int i;

    if (IS_CID(g)) {
        return;
    }

    mapInit(g); /* This assigns UVs */

    if (g->font.flags & HOT_WIN) {
        reenc = comEncoding;
    } else if (g->font.flags & HOT_MAC) {
        if (comEncoding != NULL && macEncoding != NULL) {
            hotMsg(g, hotFATAL,
                   "can't apply standard and non-standard Mac reencoding");
        }
        reenc = (comEncoding != NULL) ? comEncoding : macEncoding;
    }

    /* platEnc has been initialized to PS encoding. Now apply any reencoding */
    if (reenc != NULL) {
        for (i = 0; i < 256; i++) {
            char *gname = (*reenc)[i];
            if (gname != NULL) {
                h->sort.platEnc[i] = (strcmp(gname, ".notdef") == 0)
                                         ? NULL
                                         : mapName2Glyph(g, gname, NULL);
            }
        }
    }

    makeNonCIDMaccmap(g, comEncoding, macEncoding);
}

void mapReuse(hotCtx g) {
    mapCtx h = g->ctx.map;
    long i;

    for (i = 0; i < (long)ARRAY_LEN(codePage); i++) {
        codePage[i].isSupported = -1;
    }
    for (i = 0; i < (long)ARRAY_LEN(unicodeBlock); i++) {
        unicodeBlock[i].numFound = unicodeBlock[i].isSupported = 0;
    }

    if (IS_CID(g)) {
        h->ps.buf.cnt = 0;
        h->cid.hor.range.cnt = 0;
        h->cid.ver.map.cnt = 0;
        h->cid.mac.codespace.cnt = 0;
        h->cid.mac.range.cnt = 0;

        h->cid.hor.name = SINX_UNDEF;
        h->cid.ver.name = SINX_UNDEF;
        h->cid.mac.name = SINX_UNDEF;
    }
    dnaSET_CNT(h->uvs.entries, 0);

    /* Free sup UVs */
    for (i = 0; i < h->sort.glyphAddlUV.cnt; i++) {
        AddlUV *sup =
            g->font.glyphs.array[h->sort.glyphAddlUV.array[i]].addlUV;
        while (sup != NULL) {
            AddlUV *next = sup->next;
            MEM_FREE(g, sup);
            sup = next;
        }
    }

    h->sort.gname.cnt = 0;
    h->sort.uv.cnt = 0;
    h->sort.glyphAddlUV.cnt = 0;
    h->sort.firstAddlUV = UV_UNDEF;
    h->sort.lastAddlUV = 0;
    h->sort.nAddlUV = 0;

    h->nSuppUV = 0;
    h->minBmpUV = LONG_MAX;
    h->maxBmpUV = LONG_MIN;

    h->str.cnt = 0;
}

void mapFree(hotCtx g) {
    mapCtx h = g->ctx.map;

    /* Free da's, context */
    if (IS_CID(g)) {
        dnaFREE(h->ps.buf);
        dnaFREE(h->cid.hor.range);
        dnaFREE(h->cid.ver.map);
        dnaFREE(h->cid.mac.codespace);
        dnaFREE(h->cid.mac.range);
    }
    if (h->uvs.entries.cnt > 0) {
        dnaFREE(h->uvs.entries);
    }

    dnaFREE(h->sort.gname);
    dnaFREE(h->sort.uv);
    dnaFREE(h->sort.glyphAddlUV);

    dnaFREE(h->str);

    MEM_FREE(g, h);
}
