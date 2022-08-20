/*
 Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 This software is licensed as OpenSource, under the Apache License, Version 2.0.
 This license is available at: http://opensource.org/licenses/Apache-2.0.
 */
/***********************************************************************/

#include "cb.h"

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

extern jmp_buf motf_mark;

#include "hotconv.h"
#include "sfile.h"
#include "slogger.h"
#include "smem.h"
#include "fcdb.h"
#undef _DEBUG
#include "ctutil.h"

#define FEATUREDIR "features"

#define INT2FIX(i) ((int32_t)(i) << 16)

#define WIN_SPACE 32
#define WIN_BULLET 149
#define WIN_NONSYMBOLCHARSET 0

#define WINDOWS_DONT_CARE (0 << 4)
#define WINDOWS_ROMAN (1 << 4)
#define WINDOWS_SWISS (2 << 4)
#define WINDOWS_MODERN (3 << 4)
#define WINDOWS_SCRIPT (4 << 4)
#define WINDOWS_DECORATIVE (5 << 4)
#define MAX_CHAR_NAME_LEN 63       /* Max charname len (inc '\0') */
#define MAX_FINAL_CHAR_NAME_LEN 63 /* Max charname len (inc '\0') */
#define MAX_UV_CHAR_NAME_LEN 2047 /* length of entire alias string (column) */

#ifdef _MSC_VER /* defined by Microsoft Compiler */
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#endif

/*extern char *font_encoding;
  extern int font_serif;
 */
extern int KeepGoing;

typedef struct {
    unsigned long tag;  /* Anon table tag */
    short refillDone;   /* Done with refilling? */
    dnaDCL(char, data); /* Data */
} AnonInfo;

typedef struct { /* Alias name record */
    long iKey;   /* Alias name key index */
    long iFinal; /* Final name index */
    long iUV;    /* UV override name index */
    long iOrder; /* order index */
} AliasRec;

/* tc library client callback context */
struct cbCtx_ {
    const char *progname; /* Program name */
    dnaCtx dnactx;

    struct { /* Hot library */
        hotCtx ctx;
        hotCallbacks cb;
    } hot;

    struct { /* Postscript font file input */
        sFile file;
        char buf[BUFSIZ];
        char *(*refill)(cbCtx h, long *count); /* Buffer refill callback */
        long left;                             /* Bytes remaining in segment */
    } ps;

    struct {        /* CFF data input/output */
        char *next; /* Data fill pointer */
        dnaDCL(char, buf);
        short euroAdded;
    } cff;

    struct { /* OTF file input/output */
        sFile file;
        char buf[BUFSIZ];
    } otf;

    struct {                 /* Feature file input */
        const char *mainFile;      /* Main feature file name */
    } feat;

    struct { /* ucs file input/output */
        sFile file;
    } uvs;

    struct { /* Temporary file input/output */
        sFile file;
        char buf[BUFSIZ];
    } tmp;

    struct { /* Adobe CMap input */
        sFile file;
        char buf[BUFSIZ];
    } CMap;

    struct { /* Font conversion database */
        fcdbCtx ctx;
        dnaDCL(sFile, files); /* Database file list */
        char buf[BUFSIZ];
        hotEncoding macenc; /* Mac encoding accumulator */
        unsigned short syntaxVersion;
    } fcdb;

    struct {                    /* Glyph name aliasing database */
        dnaDCL(AliasRec, recs); /* Alias name records */
        dnaDCL(char, names);    /* Name string buffer */
        int useFinalNames;
    } alias;

    struct {                    /* Glyph name aliasing database */
        dnaDCL(AliasRec, recs); /* final name records */
    } final;

    const char *matchkey; /* Temporary lookup key for match functions */

    struct { /* Directory paths */
        const char *pfb;
        const char *otf;
        const char *cmap;
    } dir;

    dnaDCL(char, tmpbuf); /* Temporary buffer */
    hotMacData mac;       /* Mac-specific data from database */
    std::shared_ptr<slogger> logger;
};

/* ----------------------------- Error Handling ---------------------------- */

/* Fatal exception handler */
static void myfatal(void *ctx) {
    if (!KeepGoing) {
        cbCtx h = (cbCtx) ctx;
        /*This seems to cause all kinds of crashes on Windows and OSX*/
        /* hotFree(h->hot.ctx);*/ /* Free library context */

        free(h);
        exit(1); /* Could also longjmp back to caller from here */
    }
}

/* [hot callback] Print error message */
static void motf_message(void *ctx, int type, const char *text) {
    cbCtx h = (cbCtx) ctx;
    int slevel = sINFO;

    if (type == hotINDENT) {
        slevel = sINDENT;
    } else if (type == hotFLUSH) {
        slevel = sFLUSH;
    } else if (type == hotWARNING) {
        slevel = sWARNING;
    } else if (type == hotERROR) {
        slevel = sERROR;
    } else if (type == hotFATAL) {
        slevel = sFATAL;
    }
    h->logger->msg(slevel, text);
}

/* Print fatal error message */
void cbFatal(cbCtx h, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    h->logger->vlog(sFATAL, fmt, ap);
    va_end(ap);
    if (!KeepGoing) {
        myfatal(h);
    }
}

/* Print warning message */
void cbWarning(cbCtx h, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    h->logger->vlog(sWARNING, fmt, ap);
    va_end(ap);
}

/* ------------------------------- Font Input ------------------------------ */

/* Read 1-byte */
#define read1(h) sFileRead1(&h->ps.file)

/* [hot callback] Return source filename */
static const char *psId(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return h->ps.file.name;
}

/* Refill input buffer from CIDFont file */
static char *CIDRefill(cbCtx h, long *count) {
    *count = sFileReadN(&h->ps.file, BUFSIZ, h->ps.buf);
    return h->ps.buf;
}

/* Refill input buffer from PFB file */
static char *PFBRefill(cbCtx h, long *count) {
reload:
    if (h->ps.left >= BUFSIZ) {
        /* Full block */
        *count = sFileReadN(&h->ps.file, BUFSIZ, h->ps.buf);
        if (*count != BUFSIZ) {
            sFileError(&h->ps.file);
        }
        h->ps.left -= BUFSIZ;
    } else if (h->ps.left > 0) {
        /* Partial block */
        *count = sFileReadN(&h->ps.file, h->ps.left, h->ps.buf);
        if (*count != h->ps.left) {
            sFileError(&h->ps.file);
        }
        h->ps.left = 0;
    } else {
        /* New segment; read segment id */
        int escape = read1(h);
        int type = read1(h);

        /* Check segment id */
        if (escape != 128 || (type != 1 && type != 2 && type != 3)) {
            cbFatal(h, "invalid segment [%s]", h->ps.file.name);
        }

        if (type == 3) {
            *count = 0; /* EOF */
        } else {
            /* Read segment length (little endian) */
            h->ps.left = read1(h);
            h->ps.left |= (int32_t)read1(h) << 8;
            h->ps.left |= (int32_t)read1(h) << 16;
            h->ps.left |= (int32_t)read1(h) << 24;

            goto reload;
        }
    }
    return h->ps.buf;
}

/* Determine font type and convert font to CFF */
static char *psConvFont(cbCtx h, int flags, char *filename, int *psinfo, hotReadFontOverrides *fontOverrides) {
    int b0;
    int b1;
    char *FontName;

    sFileOpen(&h->ps.file, filename, "rb");

    /* Determine font file type */
    b0 = read1(h);
    b1 = read1(h);
    if (b0 == '%' && b1 == '!') {
        /* CIDFont */
        h->ps.refill = CIDRefill;
    } else if (b0 == 128 && b1 == 1) {
        /* PFB */
        h->ps.refill = PFBRefill;
    } else {
        cbFatal(h, "unknown file type [%s]\n", filename);
    }

    /* Seek back to beginning */
    sFileSeek(&h->ps.file, 0, SEEK_SET);
    h->ps.left = 0;
    FontName = hotReadFont(h->hot.ctx, flags, psinfo, fontOverrides);
    sFileClose(&h->ps.file);

    return FontName;
}

/* [TC callback] Refill input buffer */
static char *psRefill(void *ctx, long *count) {
    cbCtx h = (cbCtx)ctx;
    return h->ps.refill(h, count);
}

/* ------------------------- CFF data input/output ------------------------- */

/* [hot callback] Return CFF data id (for diagnostics) */
static const char *cffId(void *ctx) {
    return "internal CFF data buffer";
}

/* [hot callback] Write 1 byte to data buffer */
static void cffWrite1(void *ctx, int c) {
    cbCtx h = (cbCtx)ctx;
    *h->cff.next++ = c;
}

/* [hot callback] Write N bytes to data buffer */
static void cffWriteN(void *ctx, long count, char *ptr) {
    cbCtx h = (cbCtx)ctx;
    memcpy(h->cff.next, ptr, count);
    h->cff.next += count;
}

/* [hot callback] Seek to offset and return data */
static char *cffSeek(void *ctx, long offset, long *count) {
    cbCtx h = (cbCtx)ctx;
    if (offset >= h->cff.buf.cnt || offset < 0) {
        /* Seek outside data bounds */
        *count = 0;
        return NULL;
    } else {
        *count = h->cff.buf.cnt - offset;
        return &h->cff.buf.array[offset];
    }
}

/* [hot callback] Refill data buffer from current position */
static char *cffRefill(void *ctx, long *count) {
    /* Never called in this implementation since all data in memory */
    *count = 0;
    return NULL;
}

/* [hot callback] Receive CFF data size */
static void cffSize(void *ctx, long size, int euroAdded) {
    cbCtx h = (cbCtx)ctx;
    dnaSET_CNT(h->cff.buf, size);
    h->cff.next = h->cff.buf.array;
    h->cff.euroAdded = euroAdded;
}

/* -------------------------- OTF data input/output ------------------------- */

/* [hot callback] Return OTF filename */
static const char *otfId(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return h->otf.file.name;
}

/* [hot callback] Write single byte to output file (errors checked on close) */
static void otfWrite1(void *ctx, int c) {
    cbCtx h = (cbCtx)ctx;
    sFileWrite1(&h->otf.file, c);
}

/* [hot callback] Write multiple bytes to output file (errors checked on
   close) */
static void otfWriteN(void *ctx, long count, const char *ptr) {
    cbCtx h = (cbCtx)ctx;
    sFileWriteN(&h->otf.file, count, (void *)ptr);
}

/* [hot callback] Return current file position */
static long otfTell(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return sFileTell(&h->otf.file);
}

/* [hot callback] Seek to offset */
static void otfSeek(void *ctx, long offset) {
    cbCtx h = (cbCtx)ctx;
    sFileSeek(&h->otf.file, offset, SEEK_SET);
}

/* [hot callback] Refill data buffer from file */
static char *otfRefill(void *ctx, long *count) {
    cbCtx h = (cbCtx)ctx;
    *count = sFileReadN(&h->otf.file, BUFSIZ, h->otf.buf);
    return h->otf.buf;
}

/* -------------------------- Feature file input --------------------------- */

static const char *featTopLevelFile(void *ctx) {
    cbCtx h = (cbCtx)ctx;

    return h->feat.mainFile;
}

#if 0
#define TAG_ARG(t) (char)((t) >> 24 & 0xff), (char)((t) >> 16 & 0xff), \
                   (char)((t) >> 8 & 0xff), (char)((t)&0xff)

/* [hot callback] Refill anonymous table, selecting by tag */
static char *anonRefill(void *ctx, long *count, unsigned long tag) {
    cbCtx h = ctx;
    int i;

    for (i = 0; i < h->feat.anon.cnt; i++) {
        AnonInfo *ai = &h->feat.anon.array[i];

        if (ai->tag == tag) {
            *count = ai->refillDone++ ? 0 : ai->data.cnt;
            return (*count == 0) ? NULL : ai->data.array;
        }
    }

    cbFatal(h, "unrecognized anon table tag: %c%c%c%c\n", TAG_ARG(tag));
    return 0; /* Supress compiler warning */
}
#endif

/* [hot callback] Add anonymous data from feature file */
static void featAddAnonData(void *ctx, const char *data, long count,
                            unsigned long tag) {
#if 0
    /* Sample code for adding anonymous tables */

    cbCtx h = ctx;
    AnonInfo *ai = dnaNEXT(h->feat.anon);
    long destCnt = (count == 0) ? 0 : count - 1;

    /* Store feature file data except for the last newline. */
    ai->tag = tag;
    ai->refillDone = 0;
    dnaSET_CNT(ai->data, destCnt);
    memcpy(ai->data.array, data, destCnt);

    /* Simply add it intact as table data! */
    hotAddAnonTable(h->hot.ctx, tag, anonRefill);
#endif
}

/* [hot callback] Open Unicode Variation Selector file. (name == NULL) indicates not supplied. The full file name is returned. */
static const char *uvsOpen(void *ctx, const char *name) {
    cbCtx h = (cbCtx)ctx;

    if (name == NULL) {
        return NULL; /* No uvs file for this font */
    }

    sFileOpen(&h->uvs.file, name, "rb");
    return name;
}

/* [hot callback] Refill data buffer from file. ALways read just one line. */
static char *uvsGetLine(void *ctx, char *buffer, long *count) {
    cbCtx h = (cbCtx)ctx;
    char *buff;
    buff = fgets(buffer, 255, h->uvs.file.fp);
    if (buff == NULL) {
        *count = 0;
    } else {
        *count = strlen(buff);
    }
    if (*count >= 254) {
        char *p;
        /* before echoing this line, look to see where the next new-line in any form is, and terminate the string there.*/
        p = strchr(buffer, '\n');
        if (p == NULL) {
            p = strchr(buffer, '\r');
        }
        if (p != NULL) {
            *p = '\0';
        } else {
            buff[64] = 0;
        }
        cbFatal(h, "Line in Unicode Variation Sequence does not end in a new-line.\n\tPlease check if the file type is correct. Line content:\n\t%s\n", buffer);
    }

    return (buff == NULL) ? NULL : buffer;
}

/* [hot callback] Close feature file */
static void uvsClose(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    sFileClose(&h->uvs.file);
}

/* --------------------------- Temporary file I/O -------------------------- */

/* [hot callback] Open temporary file */
static void tmpOpen(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    sFileOpen(&h->tmp.file, "tmpfile", "w+b");
    if ((h->tmp.file.fp = tmpfile()) == NULL) {
        sFileError(&h->tmp.file);
    }
}

/* [hot callback] Write multiple bytes to temporary file */
static void tmpWriteN(void *ctx, long count, char *ptr) {
    cbCtx h = (cbCtx)ctx;
    sFileWriteN(&h->tmp.file, count, ptr);
}

/* [hot callback] Rewind temporary file */
static void tmpRewind(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    sFileSeek(&h->tmp.file, 0, SEEK_SET);
}

/* [hot callback] Refill temporary file input buffer */
static char *tmpRefill(void *ctx, long *count) {
    cbCtx h = (cbCtx)ctx;
    *count = sFileReadN(&h->tmp.file, BUFSIZ, h->tmp.buf);
    return h->tmp.buf;
}

/* [hot callback] Close temporary file */
static void tmpClose(void *ctx) {
    cbCtx h = (cbCtx)ctx;

    if (h->tmp.file.fp != NULL) {
        sFileClose(&h->tmp.file);
    }

#if CID_CFF_DEBUG /* turn this on to write CFF file, for debugging CID CFF creation */
    if (h->tmp.file.name != NULL) {
        FILE *tmp_fp;
        size_t n;
        char str[1024];

        h->logger->log(sINFO, "Saving temp CFF file to: %s%s",
                       h->tmp.file.name, ".cff");

        snprintf(str, sizeof(str), "%s%s", h->tmp.file.name, ".cff");
        tmp_fp = fopen(str, "wb");
        if (tmp_fp == NULL) {
            cbFatal(h, "file error <%s> [%s]", strerror(errno), str);
        }

        n = fwrite(h->cff.buf.array, 1, h->cff.buf.cnt, tmp_fp);
        if (n == 0 && ferror(tmp_fp)) {
            cbFatal(h, "file error <%s> [%s]", strerror(errno), str);
        }

        fclose(tmp_fp);
    }
#endif
}

/* ------------------------------- CMap input ------------------------------ */

/* [hot callback] Return CMap filename */
static const char *CMapId(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return h->CMap.file.name;
}

/* [hot callback] Refill CMap file input buffer */
static char *CMapRefill(void *ctx, long *count) {
    cbCtx h = (cbCtx)ctx;
    *count = sFileReadN(&h->CMap.file, BUFSIZ, h->CMap.buf);
    return h->CMap.buf;
}

/* Open CMap file */
static void CMapOpen(void *ctx, const char *filename) {
    cbCtx h = (cbCtx)ctx;
    sFileOpen(&h->CMap.file, filename, "rb");
}

/* Close CMap file */
static void CMapClose(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    sFileClose(&h->CMap.file);
}

// ------------------------ Font Conversion Database -----------------------

// [fcdb callback] Refill current database input file buffer.
static char *fcdbRefill(void *ctx, unsigned fileid, size_t *count) {
    cbCtx h = (cbCtx)ctx;
    sFile *file = &h->fcdb.files.array[fileid];
    *count = sFileReadN(file, BUFSIZ, h->fcdb.buf);
    if (*count == 0) {
        // Signal EOF
        h->fcdb.buf[0] = '\0';
        *count = 1;
    }
    return h->fcdb.buf;
}

// [fcdb callback] Get record from database file at given offset and length.
static void fcdbGetBuf(void *ctx,
                       unsigned fileid, long offset, long length, char *buf) {
    cbCtx h = (cbCtx)ctx;
    sFile *file = &h->fcdb.files.array[fileid];
    sFileSeek(file, offset, SEEK_SET);
    if (sFileReadN(file, length, buf) != length)
        sFileError(file);
}

// [fcdb callback] Report database parsing warning.
static void fcdbError(void *ctx, unsigned fileid, long line, int errid) {
    cbCtx h = (cbCtx)ctx;
    static const char *msgs[fcdbErrCnt] = {
            "syntax error",
            "duplicate record",
            "record key length too long",
            "bad name id range",
            "bad code range",
            "empty name",
            "Compatible Full name may be specified only for the Mac platform.",
            "Both version 1 and version 2 syntax is present in the Font Menu Name DB file: name table font menu names may be in error."};
    cbWarning(h, "%s [%s:%ld] (record skipped) (fcdbError)",
              msgs[errid], h->fcdb.files.array[fileid].name, line);
}

// [fcdb callback] Add name from requested font record.
static int fcdbAddName(void *ctx,
                       unsigned short platformId, unsigned short platspecId,
                       unsigned short languageId, unsigned short nameId,
                       const char *str) {
    cbCtx h = (cbCtx)ctx;
    return hotAddName(h->hot.ctx,
                      platformId, platspecId, languageId, nameId, str);
}

#undef TRY_LINKS
#ifdef TRY_LINKS
// [fcdb callback] Add style link from requested font record.
void fcdbAddLink(void *ctx, int style, const char *fontname) {
    printf("link: %s=%s\n", (style == fcdbStyleBold) ? "Bold" : (style == fcdbStyleItalic) ? "Italic" : "Bold Italic", fontname);
}
#endif

// [fcdb callback] Add encoding from requested font record.
static void fcdbAddEnc(void *ctx, int code, const char *gname) {
    cbCtx h = (cbCtx)ctx;
    if (h->mac.encoding == NULL) {
        // Initialize encoding accumulator
        int i;
        for (i = 0; i < 256; i++) {
            h->fcdb.macenc[i] = ".notdef";
        }
        h->mac.encoding = &h->fcdb.macenc;
    }
    h->fcdb.macenc[code] = gname;
}

static void fcdSetMenuVersion(void *ctx, unsigned fileid, unsigned short syntaxVersion) {
    cbCtx h = (cbCtx)ctx;
    if ((syntaxVersion != 1) && (syntaxVersion != 2)) {
        cbFatal(h, "Syntax version of the Font Menu Name Db file %s is not recognized.",
                h->fcdb.files.array[fileid].name);
    } else {
        h->fcdb.syntaxVersion = syntaxVersion;
    }
}

/* ----------------------- Glyph Name Alias Database  ---------------------- */

static void gnameError(cbCtx h, const char *message, const char *filename,
                       long line) {
    long messageLen = strlen(message) + strlen(filename);

    if (messageLen > (256 - 5)) {
        /* 6 is max probable length of decimal line number in Glyph Name Alias Database file */
        cbWarning(h, "Glyph Name Alias Database error message [%s:$d] + file name is too long. Please move Database file to shorter absolute path.", message, line);
    } else {
        cbWarning(h, "%s [%s:%ld] (record skipped)(gnameError)", message, filename, line);
    }
}

/* Validate glyph name and return a NULL pointer if the name failed to validate
   else return a pointer to character that stopped the scan. */

/* Next state table */
static const unsigned char nextFinal[3][4] = {
    /*  A-Za-z_   0-9      .       *    index  */
    /* -------- ------- ------- ------- ------ */
    {     1,       0,      2,      0},  /* [0] */
    {     1,       1,      1,      0},  /* [1] */
    {     1,       2,      2,      0},  /* [2] */
};

/* Action table */
#define Q_ (1 << 0) /* Quit scan on unrecognized character */
#define E_ (1 << 1) /* Report syntax error */

static const unsigned char actionFinal[3][4] = {
    /*  A-Za-z_   0-9      .       *    index  */
    /* -------- ------- ------- ------- ------ */
    {     0,       E_,     0,      Q_}, /* [0] */
    {     0,       0,      0,      Q_}, /* [1] */
    {     0,       0,      0,      E_}, /* [2] */
};

/* Allow glyph names to start with numbers. */
static const unsigned char actionDev[3][4] = {
    /*  A-Za-z_   0-9      .       *    index  */
    /* -------- ------- ------- ------- ------ */
    {     0,       0,      0,      Q_}, /* [0] */
    {     0,       0,      0,      Q_}, /* [1] */
    {     0,       0,      0,      E_}, /* [2] */
};

enum {
    finalName,
    sourceName,
    uvName
};

static char *gnameScan(cbCtx h, char *p, unsigned const char *action, unsigned const char *next, int nameType) {
    const char *start = p;
    int state = 0;
    int nameLimit = MAX_CHAR_NAME_LEN;

    if (nameType == uvName) {
        nameLimit = MAX_UV_CHAR_NAME_LEN;
    }

    for (;;) {
        int actn;
        int cls;
        int c = *p;

        /* Determine character class */
        if (isalpha(c) || c == '_') {
            cls = 0;
        } else if (isdigit(c)) {
            cls = 1;
        } else if (c == '.') {
            cls = 2;
        } else if ((nameType == sourceName) && (c == '.' || c == '+' || c == '*' || c == ':' || c == '~' || c == '^' || c == '!' || c == '-')) {
            cls = 2;
        } else if ((nameType == uvName) && (c == ',')) {
            cls = 2;
        } else {
            cls = 3;
        }

        /* Fetch action and change state */
        actn = (int)(action[state * 4 + cls]);
        state = (int)(next[state * 4 + cls]);

        /* Performs actions */
        if (actn == 0) {
            p++;
            continue;
        }
        if (actn & E_) {
            return NULL;
        }
        if (actn & Q_) {
            if (p - start > nameLimit) {
                return NULL; /* Maximum glyph name length exceeded */
            }
            return p;
        }
    }
}

static char *gnameDevScan(cbCtx h, char *p) {
    return gnameScan(h, p, (unsigned char *)actionDev, (unsigned char *)nextFinal, sourceName);
}

static char *gnameFinalScan(cbCtx h, char *p) {
    return gnameScan(h, p, (unsigned char *)actionFinal, (unsigned char *)nextFinal, finalName);
}

static char *gnameUVScan(cbCtx h, char *p) {
    return gnameScan(h, p, (unsigned char *)actionFinal, (unsigned char *)nextFinal, uvName);
}

/* Match alias name record. */
static int CTL_CDECL matchAliasRec(const void *key, const void *value) {
    AliasRec *alias = (AliasRec *)value;
    cbCtx h = (cbCtx)key;
    const char *aliasString;

    aliasString = &h->alias.names.array[alias->iKey];
    return strcmp(h->matchkey, (const char *)aliasString);
}

static int CTL_CDECL matchAliasRecByFinal(const void *key, const void *value) {
    AliasRec *finalAlias = (AliasRec *)value;
    cbCtx h = (cbCtx)key;
    const char *finalString;

    finalString = &h->alias.names.array[finalAlias->iFinal];
    return strcmp(h->matchkey, (const char *)finalString);
}

static int CTL_CDECL cmpAlias(const void *first, const void *second, void *ctx) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias1 = (AliasRec *)first;
    AliasRec *alias2 = (AliasRec *)second;

    return strcmp(&h->alias.names.array[alias1->iKey], &h->alias.names.array[alias2->iKey]);
}

static int CTL_CDECL cmpFinalAlias(const void *first, const void *second, void *ctx) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias1 = (AliasRec *)first;
    AliasRec *alias2 = (AliasRec *)second;

    return strcmp(&h->alias.names.array[alias1->iFinal], &h->alias.names.array[alias2->iFinal]);
}

/* Parse glyph name aliasing file.

   Glyph Name Aliasing Database File Format
   ========================================
   The glyph alias database uses a simple line-oriented ASCII format.

   Lines are terminated by either a carriage return (13), a line feed (10), or
   a carriage return followed by a line feed. Lines may not exceed 255
   characters in length (including the line terminating character(s)).

   Any of the tab (9), vertical tab (11), form feed (12), or space (32)
   characters are regarded as "blank" characters for the purposes of this
   description.

   Empty lines, or lines containing blank characters only, are ignored.

   Lines whose first non-blank character is hash (#) are treated as comment
   lines which extend to the line end and are ignored.

   All other lines must contain two glyph names separated by one or more blank
   characters. A working glyph name may be up to 63 characters in length,  a
   final character name may be up to 32 characters in length, and may only
   contain characters from the following set:

   A-Z  capital letters
   a-z  lowercase letters
   0-9  figures
    _   underscore
    .   period
    -   hyphen
    +   plus
    *   asterisk
    :   colon
    ~   tilde
    ^   asciicircum
    !   exclam

   A glyph name may not begin with a figure or consist of a period followed by
   a sequence of digits only.

   The first glyph name on a line is a final name that is stored within an
   OpenType font and second glyph name is an alias for the final name. The
   second name may be optionally followed a comment beginning with a hash (#)
   character and separated from the second name by one or more blank
   characters.

   If a final name has more that one alias it may be specified on a separate
   line beginning with the same final name.

   An optional third field may contain a glyph name in the form of 'u' + hexadecimal
   Unicode value. This will override Makeotf's default heuristics for assigning a
   UV, including any UV suggested by the form of the final name.

   All alias names must be
   unique. */

#define kMaxLineLen 1024

void cbAliasDBRead(cbCtx h, const char *filename) {
    sFile file;
    long lineno;
    long iOrder = -1;
    char buf[kMaxLineLen];

    h->alias.recs.cnt = 0;
    h->final.recs.cnt = 0;

    sFileOpen(&file, filename, "r");
    for (lineno = 1; sFileGetLine(&file, buf, kMaxLineLen) != NULL;
         lineno++) {
        int iNL = strlen(buf) - 1;
        char *final;
        char *alias;
        char *uvName;
        char *p = buf;

        /* Skip blanks */
        while (isspace(*p)) {
            p++;
        }

        if (*p == '\0' || *p == '#') {
            continue; /* Skip blank or comment line */
        }

        if (buf[iNL] != '\n') {
            cbFatal(h, "GlyphOrderAndAliasDB line is longer than limit of %d characters. [%s line number: %ld]\n", kMaxLineLen, filename, lineno);
        }

        iOrder++;
        /* Parse final name */
        final = p;
        p = gnameFinalScan(h, final);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p = '\0';
        if (strlen(final) > MAX_FINAL_CHAR_NAME_LEN) {
            cbWarning(h, "final name %s is longer (%lu) than limit %d, in %s line %ld.\n", final, strlen(final), MAX_FINAL_CHAR_NAME_LEN, filename, lineno);
        }

        /* Skip blanks */
        do {
            p++;
        } while (isspace(*p));

        /* Parse alias name */
        alias = p;
        p = gnameDevScan(h, alias);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p = '\0';
        if (strlen(alias) > MAX_CHAR_NAME_LEN) {
            cbWarning(h, "alias name %s is longer (%lu) than limit %d, in %s line %ld.\n", alias, strlen(alias), MAX_CHAR_NAME_LEN, filename, lineno);
        }

        /* Skip blanks. Since line is null terminated, will not go past end of line. */
        do {
            p++;
        } while (isspace(*p));

        /* Parse uv override name */
        /* *p is either '\0' or '#' or a uv-name.  */
        uvName = p;
        if (*p != '\0') {
            if (*p == '#') {
                *p = '\0';
            } else {
                uvName = p;
                p = gnameUVScan(h, uvName);
                if (p == NULL || !isspace(*p)) {
                    goto syntaxError;
                }
                *p = '\0';
            }
        }

        if (*p == '\0' || *p == '#') {
            size_t index, finalIndex;
            AliasRec *previous;

            h->matchkey = alias;

            /* build sorted list of alias names */
            if (bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                        sizeof(AliasRec), matchAliasRec)) {
                gnameError(h, "duplicate name", filename, lineno);
                continue;
            } else {
                index = h->alias.recs.cnt;
            }

            /* local block - NOT under closest if statement */
            {
                /* Add string */
                long length;
                AliasRec *nw = &dnaGROW(h->alias.recs, h->alias.recs.cnt)[index];

                /* Make hole */
                memmove(nw + 1, nw, sizeof(AliasRec) * (h->alias.recs.cnt++ - index));

                /* Fill record */
                nw->iKey = h->alias.names.cnt;
                length = strlen(alias) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), alias, length);
                nw->iFinal = h->alias.names.cnt;
                length = strlen(final) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), final, length);
                nw->iUV = h->alias.names.cnt;
                length = strlen(uvName) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), uvName, length);
                nw->iOrder = iOrder;
            }

            /* build sorted list of final names */
            h->matchkey = final;
            previous = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
                                           sizeof(AliasRec), matchAliasRecByFinal);
            if (previous) {
                char *previousUVName;
                previousUVName = &h->alias.names.array[previous->iUV];

                if (strcmp(previousUVName, uvName)) {
                    gnameError(h, "duplicate final name, with different uv ovveride", filename, lineno);
                }
                continue; /* it is not an error to have more than one final name entry, but we don;t want to entry duplicates in the search array */
            } else {
                finalIndex = h->final.recs.cnt;
            }

            /* local block - NOT under closest if statement. If we get here, both alias and final names are new. */
            {
                /* Add string */
                AliasRec *newFinal;
                AliasRec *newAlias = &h->alias.recs.array[index];
                newFinal = &dnaGROW(h->final.recs, h->final.recs.cnt)[finalIndex];

                /* Make hole */
                memmove(newFinal + 1, newFinal, sizeof(AliasRec) * (h->final.recs.cnt++ - finalIndex));

                /* Fill record */
                newFinal->iKey = newAlias->iKey;
                newFinal->iFinal = newAlias->iFinal;
                newFinal->iUV = newAlias->iUV;
                newFinal->iOrder = newAlias->iOrder;
            }
        } /* end if *p == \0 */

        continue; /* avoid final syntaxError */

    syntaxError:
        gnameError(h, "syntax error", filename, lineno);
    }
    sFileClose(&file);
    ctuQSort(h->alias.recs.array, h->alias.recs.cnt, sizeof(AliasRec),
             cmpAlias, h);
    ctuQSort(h->final.recs.array, h->final.recs.cnt, sizeof(AliasRec),
             cmpFinalAlias, h);

#if 0 /* xxx remove when fully tested */
    {
        int i;
        for (i = 0; i < h->alias.recs.cnt; i++) {
            AliasRec *alias = &h->alias.recs.array[i];
            printf("%s\t%s %d\n",
                   &h->alias.names.array[alias->iFinal],
                   &h->alias.names.array[alias->iKey], alias->iOrder);
        }
    }
#endif
}

/* used to override AliasDB when -q option is used: Usage scenario:
   default options are read in and processed from the project file, then
   the user overrides -r with -q. */
void cbAliasDBCancel(cbCtx h) {
    h->alias.names.cnt = 0;
    h->alias.recs.cnt = 0;
    h->final.recs.cnt = 0;
}

/* [hot callback] Convert alias name to final name. */
static const char *getFinalGlyphName(void *ctx, const char *gname) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias;
    h->matchkey = gname;
    alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                                sizeof(AliasRec), matchAliasRec);
    return (alias == NULL) ? gname : &(h->alias.names.array[alias->iFinal]);
}

/* [hot callback] Convert final name to src name. */
static const char *getSrcGlyphName(void *ctx, const char *gname) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias;
    h->matchkey = gname;
    alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
                                sizeof(AliasRec), matchAliasRecByFinal);
    return (alias == NULL) ? gname : &(h->alias.names.array[alias->iKey]);
}

/* [hot callback] Get UV override in form of u<UV Code> glyph name. */
static char *getUVOverrideName(void *ctx, const char *gname) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias;
    char *uvName = NULL;

    if (h->alias.recs.cnt == 0) {
        return NULL;
    }

    /* Assume that  gname is an alias name from the GAODB. */
    h->matchkey = gname;
    if (h->alias.useFinalNames) {
        /* Assume that gname is an final name from the GAODB. */
        alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
                                    sizeof(AliasRec), matchAliasRecByFinal);
        if (alias != NULL) {
            uvName = &h->alias.names.array[alias->iUV];
            if (*uvName == '\0') {
                uvName = NULL;
            }
        }
    } else {
        alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                                    sizeof(AliasRec), matchAliasRec);
        if (alias != NULL) {
            uvName = &h->alias.names.array[alias->iUV];
            if (*uvName == '\0') {
                uvName = NULL;
            }
        }
    }

    return uvName;
}

/* [hot callback] Get alias name and order. */
static void getAliasAndOrder(void *ctx, const char *oldName,
                             const char **newName, long int *order) {
    cbCtx h = (cbCtx)ctx;
    AliasRec *alias;
    h->matchkey = oldName;
    alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                                sizeof(AliasRec), matchAliasRec);
    if (alias != NULL) {
        *newName = &h->alias.names.array[alias->iFinal];
        *order = alias->iOrder;
    } else {
        /* if it wasn't a "friendly" name, maybe it was already a final name */
        alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
                                    sizeof(AliasRec), matchAliasRecByFinal);
        if (alias != NULL) {
            *newName = &h->alias.names.array[alias->iFinal];
            *order = alias->iOrder;
        } else {
            *newName = NULL;
            *order = -1;
        }
    }
}

/* ---------------------------- Callback Context --------------------------- */

static void anonInit(void *ctx, long count, AnonInfo *ai) {
    dnaCtx localDnaCtx = (dnaCtx)ctx;
    long i;
    for (i = 0; i < count; i++) {
        dnaINIT(localDnaCtx, ai->data, 1, 3); /* xxx */
        ai++;
    }
    return;
}

/* Create new callback context */
cbCtx cbNew(const char *progname, const char *pfbdir, const char *otfdir,
            const char *cmapdir, const char *featdir, dnaCtx mainDnaCtx) {
    static const hotCallbacks tmpl = {
        NULL, /* Callback context; set after creation */
        myfatal,
        motf_message,
        psId,
        psRefill,
        cffId,
        cffWrite1,
        cffWriteN,
        cffSeek,
        cffRefill,
        cffSize,
        otfId,
        otfWrite1,
        otfWriteN,
        otfTell,
        otfSeek,
        otfRefill,
        featTopLevelFile,
        featAddAnonData,
        tmpOpen,
        tmpWriteN,
        tmpRewind,
        tmpRefill,
        tmpClose,
        getFinalGlyphName,
        getSrcGlyphName,
        getUVOverrideName,
        getAliasAndOrder,
        uvsOpen,
        uvsGetLine,
        uvsClose,
    };
    fcdbCallbacks fcdbcb;
    cbCtx h = new cbCtx_;

    h->logger = slogger::getLogger("makeotfexe");

    /* Initialize context */
    h->progname = progname;
    h->dir.pfb = pfbdir;
    h->dir.otf = otfdir;
    h->dir.cmap = cmapdir;

    h->hot.cb = tmpl; /* Copy template */
    h->hot.cb.ctx = h;

    h->dnactx = mainDnaCtx;

    dnaINIT(mainDnaCtx, h->cff.buf, 50000, 150000);
    h->cff.euroAdded = 0;
    h->hot.ctx = hotNew(&h->hot.cb, h->logger);
    dnaINIT(mainDnaCtx, h->tmpbuf, 32, 32);
    h->mac.encoding = NULL;
    h->mac.cmapScript = HOT_CMAP_UNKNOWN;
    h->mac.cmapLanguage = HOT_CMAP_UNKNOWN;

    h->tmp.file.fp = NULL;   /*CFFDBG part of hack to print out temp cff file for CID fonts */
    h->tmp.file.name = NULL; /*CFFDBG part of hack to print out temp cff file for CID fonts */

    /* Initialize font conversion database */
    fcdbcb.ctx = h;
    fcdbcb.refill = fcdbRefill;
    fcdbcb.getbuf = fcdbGetBuf;
    fcdbcb.addname = fcdbAddName;
#ifdef TRY_LINKS
    fcdbcb.addlink = fcdbAddLink;
#else
    fcdbcb.addlink = NULL;
#endif
    fcdbcb.addenc = fcdbAddEnc;
    fcdbcb.error = fcdbError;
    fcdbcb.setMenuVersion = fcdSetMenuVersion;

    h->fcdb.ctx = fcdbNew(&fcdbcb, mainDnaCtx);
    h->fcdb.syntaxVersion = 0;

    dnaINIT(mainDnaCtx, h->fcdb.files, 5, 5);

    dnaINIT(mainDnaCtx, h->alias.recs, 700, 200);
    dnaINIT(mainDnaCtx, h->alias.names, 15000, 5000);
    dnaINIT(mainDnaCtx, h->final.recs, 700, 200);

    return h;
}

/* Add Adobe CMap to conversion */
static void addCMap(cbCtx h, const char *cmapfile) {
    if (cmapfile != NULL) {
        char cmappath[FILENAME_MAX + 1];

        snprintf(cmappath, sizeof(cmappath), "%s%s", h->dir.cmap, cmapfile);

        CMapOpen(h, cmappath);
        hotAddCMap(h->hot.ctx, CMapId, CMapRefill);
        CMapClose(h);
    }
}

/* Make OTF pathname */
static void makeOTFPath(cbCtx h, char *otfpath, const char *FontName) {
    int length = strlen(FontName);

    if (length > 27) {
        /* FontName too long for Mac if used as filename; shorten it */
        typedef struct {
            const char *style;
            const char *abrev;
        } Rule;
        static const Rule rules[] = {  // Taken from ADS TN#5088 (shortest first)
                {"Bold", "Bd"},
                {"Book", "Bk"},
                {"Demi", "Dm"},
                {"Nord", "Nd"},
                {"Semi", "Sm"},
                {"Thin", "Th"},
                {"Black", "Blk"},
                {"Extra", "X"},
                {"Heavy", "Hv"},
                {"Light", "Lt"},
                {"Roman", "Rm"},
                {"Super", "Su"},
                {"Swash", "Sw"},
                {"Ultra", "Ult"},
                {"Expert", "Exp"},
                {"Italic", "It"},
                {"Kursiv", "Ks"},
                {"Medium", "Md"},
                {"Narrow", "Nr"},
                {"Poster", "Po"},
                {"Script", "Scr"},
                {"Shaded", "Sh"},
                {"Sloped", "Sl"},
                {"Compact", "Ct"},
                {"Display", "DS"},
                {"Oblique", "Obl"},
                {"Outline", "Ou"},
                {"Regular", "Rg"},
                {"Rounded", "Rd"},
                {"Slanted", "Sl"},
                {"Titling", "Ti"},
                {"Upright", "Up"},
                {"Extended", "Ex"},
                {"Inclined", "Ic"},
                {"Alternate", "Alt"},
                {"Condensed", "Cn"},
                {"Oldestyle", "OS"},
                {"Ornaments", "Or"},
                {"Compressed", "Cm"},
            };

        cbWarning(h, "filename too long [%s] (editing)", FontName);

        /* Size buffer and make copy */
        STRCPY_S(dnaGROW(h->tmpbuf, length+1), length+1, FontName);

        do {
            unsigned int i;
            for (i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
                const Rule *rule = &rules[i];
                char *p = strstr(h->tmpbuf.array, rule->style);
                if (p != NULL) {
                    /* Found match */
                    int stylelen = strlen(rule->style);
                    int abrevlen = strlen(rule->abrev);
                    int shrinklen = stylelen - abrevlen;

                    /* Edit FontName */
                    memmove(p, rule->abrev, abrevlen);
                    memmove(p + abrevlen, p + stylelen,
                            length - (p - h->tmpbuf.array) - shrinklen);

                    length -= shrinklen;
                    goto matched;
                }
            }
            cbWarning(h, "filename too long [%s] (truncating)", FontName);
            length = 27;
        matched:
            {}
        } while (length > 27);

        h->tmpbuf.array[length] = '\0';
        snprintf(otfpath, FILENAME_MAX+1, "%s%s.otf", h->dir.otf, h->tmpbuf.array);
    } else {
        snprintf(otfpath, FILENAME_MAX+1, "%s%s.otf", h->dir.otf, FontName);
    }
}

/* 5 functions below were formerly part of sun.c*/

static char *parseInstance(char *rawstring, char *finalstring, int nAxes) {
    char *p;
    int i = 0;
    int a;

    p = rawstring;
    finalstring[0] = '\0';
    p = strchr(p++, '[');
    for (a = 0; a < nAxes; a++) {
        while (isdigit(*p)) { /* axis value */
            finalstring[i++] = *p++;
        }

        p = strchr(p++, '(');
        if (*p == ' ') {
            p++;
        }
        finalstring[i++] = ' ';  /* space */
        finalstring[i++] = *p++; /* letter */
        finalstring[i++] = *p++; /* letter */
        finalstring[i++] = ' ';  /* space */
        while ((*p == ')') || (*p == ' ')) {
            p++;
        }
    }
    p = strchr(p++, ']');

    if (finalstring[i - 1] == ' ') {
        finalstring[i - 1] = '\0';
    }
    finalstring[i] = '\0';
    return p;
}

/* instanceName is of the form:  "300 LT 440 NR"
   regCoords is of the form: "[399 110]"
 */
static int isRegularInstance(const char *instanceName,
                             const char *regCoords, int nAxes) {
    char inst[HOT_MAX_MENU_NAME];
    char reg[HOT_MAX_MENU_NAME];
    char *p;
    int reg1, reg2, reg3, reg4;
    int in1, in2, in3, in4;

    STRCPY_S(reg, sizeof(reg), regCoords);
    p = reg;
    while (*p) {
        if (!isdigit(*p)) {
            *p = ' ';
        }
        p++;
    }

    STRCPY_S(inst, sizeof(inst), instanceName);
    p = inst;
    while (*p) {
        if (!isdigit(*p)) {
            *p = ' ';
        }
        p++;
    }

    sscanf(reg, "%d %d %d %d", &reg1, &reg2, &reg3, &reg4);

    sscanf(inst, "%d %d %d %d", &in1, &in2, &in3, &in4);

    switch (nAxes) {
        case 4:
            if (reg4 != in4) {
                return 0;
            }
            /* nobreak */

        case 3:
            if (reg3 != in3) {
                return 0;
            }
            /* nobreak */

        case 2:
            if (reg2 != in2) {
                return 0;
            }
            /* nobreak */

        case 1:
            if (reg1 != in1) {
                return 0;
            }
            /* nobreak */
    }

    return 1;
}

static void parseStyles(const char *stylestring, int *point0, int *delta0, int *point1, int *delta1) {
    char *p;
    char str[HOT_MAX_MENU_NAME];

    STRCPY_S(str, sizeof(str), stylestring);
    p = str;
    while (*p) {
        if (!isdigit(*p)) {
            *p = ' ';
        }
        p++;
    }

    *point0 = *delta0 = *point1 = *delta1 = 0;
    sscanf(str, "%d %d %d %d", point0, delta0, point1, delta1);
}

/*This used to be handled in sun.c but is now platform independent in this Python version.
 */

static void ProcessFontInfo(hotCtx g, const char *version, const char *FontName,
                            int psinfo, hotMacData *mac, long otherflags,
                            short fsSelectionMask_on, short fsSelectionMask_off,
                            unsigned short os2Version, const char *licenseID) {
    hotWinData win;
    hotCommonData common;

    memset((void *)&win, 0, sizeof(hotWinData));
    win.Family = WINDOWS_ROMAN; /* This is not currently used by the hot lib; */
                                /* it currently always sets OS2.sFamily to "undefined". */
    win.CharSet = WIN_NONSYMBOLCHARSET;
    win.DefaultChar = WIN_SPACE; /* We don't have any fonts that use the bullet as the .notdef. */
    win.BreakChar = WIN_SPACE;

    /* Create flags */
    common.flags = HOT_WIN | (psinfo & HOT_EURO_ADDED);

    if (otherflags & OTHERFLAGS_ISWINDOWSBOLD) {
        common.flags |= HOT_BOLD;
    }
    if (otherflags & OTHERFLAGS_ISITALIC) {
        common.flags |= HOT_ITALIC;
    }
    if (otherflags & OTHERFLAGS_DOUBLE_MAP_GLYPHS) {
        common.flags |= HOT_DOUBLE_MAP_GLYPHS;
    }

    common.nKernPairs = 0;
    common.clientVers = version;
    common.licenseID = licenseID;
    common.encoding = NULL;

    common.fsSelectionMask_on = fsSelectionMask_on;
    common.fsSelectionMask_off = fsSelectionMask_off;
    common.os2Version = os2Version;
    hotAddMiscData(g, &common, &win, mac);
}

/* Convert font */
void cbConvert(cbCtx h, int flags, const char *clientVers, const char *pfbfile,
               const char *otffile, const char *featurefile, const char *hcmapfile,
               const char *vcmapfile, const char *mcmapfile, const char *uvsFile,
               long otherflags, short macScript, short macLanguage,
               long addGlyphWeight, unsigned long maxNumSubrs,
               short fsSelectionMask_on, short fsSelectionMask_off,
               unsigned short os2Version, const char *licenseID) {
    int psinfo;
    int type;
    const char *FontName;
    char pfbpath[FILENAME_MAX + 1];
    char otfpath[FILENAME_MAX + 1];
    int freeFeatName = 0;
    unsigned long hotConvertFlags = 0;

    if (otherflags & OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT) {
        hotConvertFlags |= HOT_ID2_CHAIN_CONTXT3;
    }

    if (otherflags & OTHERFLAGS_ALLOW_STUB_GSUB) {
        hotConvertFlags |= HOT_ALLOW_STUB_GSUB;
    }
    if (otherflags & OTHERFLAGS_OLD_SPACE_DEFAULT_CHAR) {
        hotConvertFlags |= HOT_OLD_SPACE_DEFAULT_CHAR;
    }

    if (h->fcdb.syntaxVersion == 1) {
        hotConvertFlags |= HOT_USE_V1_MENU_NAMES;
    }

    if (otherflags & OTHERFLAGS_OLD_NAMEID4)
        hotConvertFlags |= HOT_USE_OLD_NAMEID4;
    if (otherflags & OTHERFLAGS_OMIT_MAC_NAMES) {
        hotConvertFlags |= HOT_OMIT_MAC_NAMES;
    }

    if (otherflags & OTHERFLAGS_STUB_CMAP4) {
        hotConvertFlags |= HOT_STUB_CMAP4;
    }
    if (otherflags & OTHERFLAGS_OVERRIDE_MENUNAMES) {
        hotConvertFlags |= HOT_OVERRIDE_MENUNAMES;
    }
    if (otherflags & OTHERFLAGS_DO_NOT_OPTIMIZE_KERN) {
        hotConvertFlags |= HOT_DO_NOT_OPTIMIZE_KERN;
    }
    if (otherflags & OTHERFLAGS_ADD_STUB_DSIG) {
        hotConvertFlags |= HOT_ADD_STUB_DSIG;
    }

    if (otherflags & OTHERFLAGS_VERBOSE) {
        hotConvertFlags |= HOT_CONVERT_VERBOSE;
    }

    if (otherflags & OTHERFLAGS_FINAL_NAMES) {
        hotConvertFlags |= HOT_CONVERT_FINAL_NAMES;
    }

    hotSetConvertFlags(h->hot.ctx, hotConvertFlags);

    if (flags & HOT_RENAME) {
        h->alias.useFinalNames = 1;
    } else {
        h->alias.useFinalNames = 0;
    }
    /* Make PFB path */
    snprintf(pfbpath, sizeof(pfbpath), "%s%s", h->dir.pfb, pfbfile);

    /*CFFDBG part of hack to print out temp cff file for CID fonts */
    /*h->CMap.file.name set here is used only to trigger dbg behavior in tempClose() */
    if (hcmapfile != NULL) {
        h->tmp.file.name = pfbpath;
    }

    if (!sFileExists(pfbpath)) {
        cbFatal(h, "Source font file not found: %s \n", pfbpath);
        return;
    }
    if ((featurefile != NULL) && (!sFileExists(featurefile))) {
        cbFatal(h, "Feature file not found: %s \n", featurefile);
        return;
    }

    /* Convert font to CFF */
    {
        hotReadFontOverrides fontOverrides;
        fontOverrides.syntheticWeight = addGlyphWeight;
        fontOverrides.maxNumSubrs = maxNumSubrs;
        FontName = psConvFont(h, flags, pfbpath, &psinfo, &fontOverrides);
    }

    type = psinfo & HOT_TYPE_MASK;
    if (h->cff.euroAdded) {
        psinfo |= HOT_EURO_ADDED;
    }

    if (uvsFile != NULL) {
        hotAddUVSMap(h->hot.ctx, uvsFile);
    }

    /* Determine dir that feature file's in */
    h->feat.mainFile = featurefile;

    if (type == hotCID) {
        /* Add CMaps */
        if (hcmapfile == NULL) {
            cbFatal(h, "no CMaps specified [%s]\n", pfbpath);
        }

        addCMap(h, hcmapfile);
        addCMap(h, vcmapfile);
        addCMap(h, mcmapfile);
    }

    // Get database data via callbacks
    if (h->fcdb.files.cnt == 0) {
        {
            fcdbGetRec(h->fcdb.ctx, FontName);
            cbWarning(h, "FontMenuNameDB file was not specified or not found. [%s]", FontName);
        }
    } else if (fcdbGetRec(h->fcdb.ctx, FontName)) {
        {
            cbWarning(h, "not in FontMenuNameDB [%s]", FontName);
        }
    }

    // Make sure that GOADB file has been read in, if required
    if ((flags & HOT_RENAME) && (h->alias.recs.cnt < 1) && (type != hotCID)) {
        cbWarning(h, "Glyph renaming was requested, but the GlyphOrderAndAliasDB file was not specified.");
    }

    h->mac.cmapScript = macScript; /* Used in hotAddmiscData, in ProcessFontInfo */
    h->mac.cmapLanguage = macLanguage;

    ProcessFontInfo(h->hot.ctx, clientVers, FontName, psinfo,
                    &h->mac, otherflags, fsSelectionMask_on, fsSelectionMask_off, os2Version, licenseID);

    /* Make OTF path */
    if (otffile == NULL) {
        makeOTFPath(h, otfpath, FontName);
    } else {
        snprintf(otfpath, sizeof(otfpath), "%s%s", h->dir.otf, otffile);
    }

    /* Write OTF file */
    sFileOpen(&h->otf.file, otfpath, "w+b");
    hotConvert(h->hot.ctx);
    sFileClose(&h->otf.file);
}

// Read font conversion database
void cbFCDBRead(cbCtx h, const char *filename) {
    unsigned fileid = h->fcdb.files.cnt;
    sFileOpen(dnaNEXT(h->fcdb.files), filename, "rb");
    fcdbAddFile(h->fcdb.ctx, fileid, h);
}

// Free callback context
void cbFree(cbCtx h) {
    int i;

    hotFree(h->hot.ctx);
    dnaFREE(h->cff.buf);
    dnaFREE(h->tmpbuf);

    // Free database resources
    fcdbFree(h->fcdb.ctx);
    for (i = 0; i < h->fcdb.files.cnt; i++) {
        sFileClose(&h->fcdb.files.array[i]);
    }
    dnaFREE(h->fcdb.files);

    dnaFREE(h->alias.recs);
    dnaFREE(h->final.recs);
    dnaFREE(h->alias.names);

    delete h;
}
