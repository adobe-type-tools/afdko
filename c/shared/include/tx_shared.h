/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 Code shared by tx, rotatefont, and mergefonts.
 */

#ifndef TX_SHARED_H
#define TX_SHARED_H

#include "ctlshare.h"
#include "cfembed.h"
#include "cffread.h"
#include "cffwrite.h"
#include "ctutil.h"
#include "dynarr.h"
#include "pdfwrite.h"
#include "sfntread.h"
#include "t1read.h"
#include "ttread.h"
#include "t1write.h"
#include "svread.h"
#include "svgwrite.h"
#include "uforead.h"
#include "ufowrite.h"
#include "txops.h"
#include "dictops.h"
#include "abfdesc.h"
#include "sha1.h"

#undef global /* Remove conflicting definition from buildch */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if PLAT_MAC
#include <console.h>
#include <file_io.h>
#endif /* PLAT_MAC */

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <limits.h>

#if _WIN32
#include <fcntl.h>
#include <io.h>
#define stat _stat
#define S_IFDIR _S_IFDIR
#include <direct.h> /* to get _mkdir() */
#include <time.h>
#else
#include <sys/time.h>
#endif

/* ----------------------- Miscellaneous Definitions ----------------------- */

#define ARRAY_LEN(t) (sizeof(t) / sizeof((t)[0]))

/* Predefined tags */
#define CID__ CTL_TAG('C', 'I', 'D', ' ') /* sfnt-wrapped CID table */
#define POST_ CTL_TAG('P', 'O', 'S', 'T') /* Macintosh POST resource */
#define TYP1_ CTL_TAG('T', 'Y', 'P', '1') /* GX Type 1 sfnt table */
#define sfnt_ CTL_TAG('s', 'f', 'n', 't') /* sfnt resource */

/* File signatures */
#define sig_PostScript0 CTL_TAG('%', '!', 0x00, 0x00)
#define sig_PostScript1 CTL_TAG('%', 'A', 0x00, 0x00) /* %ADO... */
#define sig_PostScript2 CTL_TAG('%', '%', 0x00, 0x00) /* %%... */
#define sig_PFB ((ctlTag)0x80010000)
#define sig_CFF ((ctlTag)0x01000000)
#define sig_CFF2 ((ctlTag)0x02000000)
#define sig_OTF CTL_TAG('O', 'T', 'T', 'O')
#define sig_MacResource ((ctlTag)0x00000100)
#define sig_AppleSingle ((ctlTag)0x00051600)
#define sig_AppleDouble ((ctlTag)0x00051607)
#define sig_UFO CTL_TAG('<', '?', 'x', 'm')
/* Generate n-bit mask */
#define N_BIT_MASK(n) (~(~0UL << (n)))
enum { MAX_VERSION_SIZE = 100 };

typedef struct txCtx_ *txCtx; /* tx program context */

typedef struct /* Macintosh resource info */
{
    ctlTag type;
    unsigned short id;
    unsigned long name; /* Name offset then name index */
    unsigned char attrs;
    unsigned long offset; /* Offset to resource data (excludes length) */
    unsigned long length; /* Length of resource data */
} ResInfo;

typedef struct /* AppleSingle/Double entry descriptor */
{
    unsigned long id;
    long offset;
    unsigned long length;
} EntryDesc;

typedef struct /* Glyph name to Unicode value mapping */
{
    char *gname;
    unsigned short uv;
} Name2UV;

enum /* Stream types */
{
    stm_Src,    /* Source */
    stm_SrcUFO, /* Source */
    stm_Dst,    /* Destination */
    stm_Tmp,    /* Temporary */
    stm_Dbg     /* Debug */
};

#define TMPSIZE 50000 /* Temporary stream buffer size */

typedef struct /* Data stream */
{
    short type;
    short flags;
#define STM_TMP_ERR    (1 << 0) /* Temporary stream error occurred */
#define STM_DONT_CLOSE (1 << 1) /* Don't close stream */
    char *filename;
    FILE *fp;
    char *buf;
    size_t pos; /* Tmp stream position */
} Stream;

typedef struct /* Font record */
{
    int type;    /* Font technology type */
    int iTTC;    /* TrueType Collection index */
    long offset; /* File offset */
} FontRec;

typedef struct /* CFF subr data */
{
    unsigned long count;
    unsigned char offSize;
    unsigned long offset; /* Offset array base */
    unsigned long dataref;
    dnaDCL(unsigned char, stemcnt); /* Per-subr stem count */
    unsigned short bias;
} SubrInfo;

typedef struct /* cmap format 4 segment */
{
    unsigned short endCode;
    unsigned short startCode;
    short idDelta;
    unsigned long idRangeOffset; /* changed from unsigned short */
} cmapSegment4;

enum /* Charstring types */
{
    dcf_CharString,
    dcf_GlobalSubr,
    dcf_LocalSubr
};

typedef float FltMtx[6]; /* Float matrix */

/* Function types */
typedef size_t (*SegRefillFunc)(txCtx h, char **ptr);
typedef void (*abfGlyphWidthCallback)(abfGlyphCallbacks *cb, float hAdv);
typedef void (*DumpElementProc)(txCtx h, long index, const ctlRegion *region);

enum /* Source font technology types */
{
    src_Type1,    /* Type 1 */
    src_OTF,      /* OTF */
    src_CFF,      /* Naked CFF */
    src_TrueType, /* TrueType */
    src_SVG,      /* SVG data. */
    src_UFO,      /* UFO data. */
};
/* Note that for mergeFonts.c, SVG is supported only as a merge font,
   and not as the first in the set. */

enum /* Operation modes */
{
    mode_dump,
    mode_ps,
    mode_afm,
    mode_path,
    mode_cff,
    mode_cef,
    mode_pdf,
    mode_mtx,
    mode_t1,
    mode_bc, /* is deprecated, but still support the option in order to be able to print an error message */
    mode_svg,
    mode_ufow,
    mode_dcf
};

/* Memory allocation failure simulation control values */
enum {
    FAIL_REPORT = -1,  /* Report total calls to mem_manage() */
    FAIL_INACTIVE = -2 /* Inactivate memory allocation failure */
};

typedef struct
{
    unsigned long regionCount;
} RegionInfo;

enum WhichApp {
    APP_TX,
    APP_ROTATEFONT,
    APP_MERGEFONTS
};

struct txCtx_ {
    char *progname;                   /* This program's name (for diagnostics) */
    enum WhichApp app;                /* this application (used in shared code) */
    long flags;                       /* Control flags */
#define SEEN_MODE           (1 <<  0) /* Flags mode option seen */
#define DONE_FILE           (1 <<  1) /* Processed font file */
#define DUMP_RES            (1 <<  2) /* Print mac resource map */
#define DUMP_ASD            (1 <<  3) /* Print AppleSingle/Double data */
#define AUTO_FILE_FROM_FILE (1 <<  4) /* Gen. dst filename from src filename */
#define AUTO_FILE_FROM_FONT (1 <<  5) /* Gen. dst filename from src FontName */
#define SUBSET_OPT          (1 <<  6) /* Subsetting option specified */
#define EVERY_FONT          (1 <<  7) /* Read every font from multi-font file */
#define SHOW_NAMES          (1 <<  8) /* Show filename and FontName being processed */
#define PRESERVE_GID        (1 <<  9) /* Preserve gids when subsetting */
#define NO_UDV_CLAMPING     (1 << 10) /* Don't clamp UVD's */
#define SUBSET__EXCLUDE_OPT (1 << 11) /* use glyph list to exclude glyphs, instead of including them */
#define SUBSET_SKIP_NOTDEF  (1 << 12) /* While this is set, don't force the notdef into the current subset. */
#define SUBSET_HAS_NOTDEF   (1 << 13) /* Indicates that notdef has been added, no need to force it in.*/
#define PATH_REMOVE_OVERLAP (1 << 14) /* Do not remove path overlaps */
#define PATH_SUPRESS_HINTS  (1 << 15) /* Do not remove path overlaps */
    int mode;                         /* Current mode */
    char *modename;                   /* Name of current mode */
    void *appSpecificInfo;            /* different data for rotateFont.c & mergeFonts.c */
    void (*appSpecificFree)(txCtx h); /* free for app-specific info */
    abfTopDict *top;                  /* Top dictionary */
    struct                            /* Source data */
    {
        int type;                        /* Font technology type */
        Stream stm;                      /* Input stream */
        long offset;                     /* Buffer offset */
        long length;                     /* Buffer length */
        char buf[BUFSIZ];                /* Buffer data */
        char *end;                       /* One past end of buffer */
        char *next;                      /* Next byte available */
        int print_file;                  /* Flags when to print filename ahead of debug */
        dnaDCL(abfGlyphInfo *, glyphs);  /* Source glyph list when subsetting */
        dnaDCL(abfGlyphInfo *, exclude); /* Excluded glyph list when subsetting */
        dnaDCL(float, widths);           /* Source glyph width for -t1 -3 mode */
        dnaDCL(Stream, streamStack);     /* support recursive opening of source stream files ( for components) */
    } src;
    struct /* Destination data */
    {
        Stream stm;       /* Output stream */
        char buf[BUFSIZ]; /* Buffer data */
        void (*begset)(txCtx h);
        void (*begfont)(txCtx h, abfTopDict *top);
        void (*endfont)(txCtx h);
        void (*endset)(txCtx h);
    } dst;
    dnaDCL(FontRec, fonts); /* Source font records */
    struct                  /* Macintosh resources */
    {
        dnaDCL(ResInfo, map); /* Resource map */
        dnaDCL(char, names);  /* Resource names */
    } res;
    struct /* AppleSingle/Double data */
    {
        unsigned long magic;        /* Magic #, 00051600-single, 00051607-double */
        unsigned long version;      /* Format version */
        dnaDCL(EntryDesc, entries); /* Entry descriptors */
    } asd;
    struct /* Font data segment */
    {
        SegRefillFunc refill; /* Format-specific refill */
        size_t left;          /* Bytes remaining in segment */
    } seg;
    struct /* Script file data */
    {
        char *buf;            /* Whole file buffer */
        dnaDCL(char *, args); /* Arg list */
    } script;
    struct /* File processing */
    {
        char *sr; /* Source root path */
        char *sd; /* Source directory path */
        char *dd; /* Destination directory path */
        char src[FILENAME_MAX];
        char dst[FILENAME_MAX];
    } file;
    struct /* Random subset data */
    {
        dnaDCL(unsigned short, glyphs); /* Tag list */
        dnaDCL(char, args);             /* Simulated -g args */
    } subset;
    struct /* Option args */
    {
        char *U;
        char *i;
        char *p;
        char *P;
        struct /* Glyph list (g option) */
        {
            int cnt;       /* Substring count */
            char *substrs; /* Concatenated substrings */
        } g;
        struct
        {
            int level;
        } path;
        struct /* cef-specific */
        {
            unsigned short flags; /* cefEmbedSpec flags */
            char *F;
        } cef;
    } arg;
    struct /* t1read library */
    {
        t1rCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags;
    } t1r;
    struct /* cffread library */
    {
        cfrCtx ctx;
        Stream dbg;
        long flags;
    } cfr;
    struct /* ttread library */
    {
        ttrCtx ctx;
        Stream dbg;
        long flags;
    } ttr;
    struct /* svread library */
    {
        svrCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags;
    } svr;
    struct /* uforead library */
    {
        ufoCtx ctx;
        Stream src;
        Stream dbg;
        long flags;
        char *altLayerDir;
    } ufr;
    struct /* cffwrite library */
    {
        cfwCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags;
        unsigned long maxNumSubrs;
    } cfw;
    struct /* cfembed library */
    {
        cefCtx ctx;
        Stream src;
        Stream tmp0;
        Stream tmp1;
        dnaDCL(cefSubsetGlyph, subset);
        dnaDCL(char *, gnames);
        dnaDCL(unsigned short, lookup); /* Glyph lookup */
    } cef;
    struct /* abf facilities */
    {
        abfCtx ctx;
        struct abfDumpCtx_ dump;
        struct abfDrawCtx_ draw;
        struct abfMetricsCtx_ metrics;
        struct abfAFMCtx_ afm;
    } abf;
    struct /* pdfwrite library */
    {
        pdwCtx ctx;
        long flags;
        long level;
    } pdw;
    struct /* Metrics mode data */
    {
        int level; /* Output level */
        struct     /* Metric data */
        {
            struct abfMetricsCtx_ ctx;
            abfGlyphCallbacks cb;
        } metrics;
        struct /* Aggregate bbox */
        {
            float left;
            float bottom;
            float right;
            float top;
            struct /* Glyph that set value */
            {
                abfGlyphInfo *left;
                abfGlyphInfo *bottom;
                abfGlyphInfo *right;
                abfGlyphInfo *top;
            } setby;
        } bbox;
    } mtx;
    struct /* t1write library */
    {
        long options;             /* Control options */
#define T1W_NO_UID       (1 << 0) /* Remove UniqueID keys */
#define T1W_DECID        (1 << 1) /* -decid option */
#define T1W_USEFD        (1 << 2) /* -usefd option */
#define T1W_REFORMAT     (1 << 3) /* -pfb or -LWFN options */
#define T1W_WAS_EMBEDDED (1 << 4) /* +E option */
        t1wCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags; /* Library flags */
        int lenIV;
        long maxGlyphs;
        long fd;               /* -decid target fd */
        dnaDCL(char, gnames); /* -decid glyph names */
    } t1w;
    struct /* svgwrite library */
    {
        long options; /* Control options */
        svwCtx ctx;
        unsigned short unrec;
        Stream tmp;
        Stream dbg;
        long flags; /* Library flags */
    } svw;
    struct /* ufowrite library */
    {
        ufwCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags; /* Library flags */
    } ufow;
    struct /* Dump cff mode */
    {
        long flags; /* Control flags */
#define DCF_Header           (1 <<  0)
#define DCF_NameINDEX        (1 <<  1)
#define DCF_TopDICTINDEX     (1 <<  3)
#define DCF_StringINDEX      (1 <<  4)
#define DCF_GlobalSubrINDEX  (1 <<  5)
#define DCF_Encoding         (1 <<  6)
#define DCF_Charset          (1 <<  7)
#define DCF_FDSelect         (1 <<  8)
#define DCF_FDArrayINDEX     (1 <<  9)
#define DCF_CharStringsINDEX (1 << 10)
#define DCF_PrivateDICT      (1 << 11)
#define DCF_LocalSubrINDEX   (1 << 12)
#define DCF_AllTables   N_BIT_MASK(13)
#define DCF_BreakFlowed      (1 << 13) /* Break flowed objects */
#define DCF_TableSelected    (1 << 14) /* -T option used */
#define DCF_Flatten          (1 << 15) /* Flatten charstrings */
#define DCF_SaveStemCnt      (1 << 16) /* Save h/vstems counts */
#define DCF_IS_CFF2          (1 << 18) /* Font has CFF table is CFF 2 */
#define DCF_END_HINTS        (1 << 19) /* have seen moveto */

        int level;                    /* Dump level */
        char *sep;                    /* Flowed text separator */
        SubrInfo global;              /* Global subrs */
        dnaDCL(SubrInfo, local);      /* Local subrs */
        dnaDCL(unsigned char, glyph); /* Per-glyph stem count */
        dnaDCL(RegionInfo, varRegionInfo);
        long stemcnt;    /* Current stem count */
        long vsIndex;    /* needed to derive numRegions */
        long numRegions; /* needed to decode blend args */
        SubrInfo *fd;    /* Current local info */
        int subrDepth;   /* Subroutine call depth */
    } dcf;
    struct /* Operand stack */
    {
        long cnt;
        float array[CFF2_MAX_OP_STACK];
    } stack;
    struct /* Font Dict filter */
    {
        dnaDCL(int, fdIndices); /* Source glyph width for -t1 -3 mode */
    } fd;
    struct /* OTF cmap encoding */
    {
        dnaDCL(unsigned long, encoding);
        dnaDCL(cmapSegment4, segment);
    } cmap;
    struct /* Library contexts */
    {
        dnaCtx dna; /* dynarr library */
        sfrCtx sfr; /* sfntread library */
    } ctx;
    struct /* Callbacks */
    {
        ctlMemoryCallbacks mem;
        ctlStreamCallbacks stm;
        abfGlyphCallbacks glyph;
        abfGlyphBegCallback saveGlyphBeg;
        abfGlyphCallbacks save;
        int selected;
    } cb;
    struct /* Memory allocation failure simulation data */
    {
        long iCall; /* Index of next call to mem_manange */
        long iFail; /* Index of failing call or FAIL_REPORT or FAIL_INACTIVE */
    } failmem;
    long maxOpStack;
};

/* Check stack contains at least n elements. */
#define CHKUFLOW(n)                                                 \
    do {                                                            \
        if (h->stack.cnt < (n)) fatal(h, "Type 2 stack underflow"); \
    } while (0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n)                             \
    do {                                        \
        if (h->stack.cnt + (n) > h->maxOpStack) \
            fatal(h, "Type 2 stack overflow");  \
    } while (0)

/* Check that current charstring has enough bytes left. */
#define CHECK_CHARSTRING_BYTES_LEFT(n)        \
    do {                                      \
        if (n > left)                         \
            fatal(h, "charstring too short"); \
    } while (0)

/* Check that current dict has enough bytes left. */
#define CHECK_DICT_BYTES_LEFT(n)    \
    do {                            \
        if (n > left)                   \
            fatal(h, "dict too short"); \
    } while (0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++] = (float)(v))

/* SID to standard string length */
#define SID2STD_LEN 391 /* number of entries in stdstr1.h */

enum /* Glyph selector types */
{
    sel_by_tag,
    sel_by_cid,
    sel_by_name
};

void buildFontList(txCtx h);
void callbackGlyph(txCtx h, int type, unsigned short id, char *name);
void callbackSubset(txCtx h);
void dcf_ParseTableArg(txCtx h, char *arg);
void dstFileSetName(txCtx h, char *filename);
void CTL_CDECL fatal(txCtx h, char *fmt, ...);
void fileError(txCtx h, char *filename);
float *getUDV(txCtx h);
void memInit(txCtx h);
void memFree(txCtx h, void *ptr);
void *memNew(txCtx h, size_t size);
void parseFDSubset(txCtx h);
void prepOTF(txCtx h);
void prepSubset(txCtx h);
void printText(int cnt, char *text[]);
long randrange(long N);
void *safeManage(ctlMemoryCallbacks *cb, void *old, size_t size);
void seedtime(void);
void setMode(txCtx h, int mode);
void stmFree(txCtx h, Stream *s);
void stmInit(txCtx h);
void svrReadFont(txCtx h, long origin);
void t1rReadFont(txCtx h, long origin);
void ttrReadFont(txCtx h, long origin, int iTTC);
void ufoReadFont(txCtx h, long origin);

#endif /* TX_SHARED_H */
