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

#include "ctlshare.h"
#include "goadb.h"
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

#ifdef _MSC_VER /* defined by Microsoft Compiler */
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#endif

struct Stream {
    sFile file;
    char buf[BUFSIZ];
};

/* tc library client callback context */
struct cbCtx_ {
    const char *progname; /* Program name */
    dnaCtx dnactx;

    struct { /* Hot library */
        hotCtx ctx;
        hotCallbacks cb;
    } hot;

    Stream in;
    Stream out;
    Stream uvs;
    Stream tmp;
    Stream CMap;

    struct {                 /* Feature file input */
        const char *mainFile;      /* Main feature file name */
    } feat;

    struct { /* Font conversion database */
        fcdbCtx ctx;
        dnaDCL(sFile, files); /* Database file list */
        char buf[BUFSIZ];
        hotEncoding macenc; /* Mac encoding accumulator */
        unsigned short syntaxVersion;
    } fcdb;

    struct { /* Directory paths */
        const char *in;
        const char *out;
        const char *cmap;
    } dir;

    dnaDCL(char, tmpbuf); /* Temporary buffer */
    hotMacData mac;       /* Mac-specific data from database */

    std::shared_ptr<GOADB> goadb;
    std::shared_ptr<slogger> logger;
};

/* ----------------------------- Error Handling ---------------------------- */

/* Fatal exception handler */
static void myfatal(void *ctx) {
    cbCtx h = (cbCtx) ctx;
    /*This seems to cause all kinds of crashes on Windows and OSX*/
    /* hotFree(h->hot.ctx);*/ /* Free library context */

    free(h);
    exit(1); /* Could also longjmp back to caller from here */
}

/* [hot callback] Print error message */
static void motf_message(void *ctx, int type, const char *text) {
    cbCtx h = (cbCtx) ctx;
    h->logger->msg(type, text);
}

/* Print fatal error message */
void cbFatal(cbCtx h, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    h->logger->vlog(sFATAL, fmt, ap);
    va_end(ap);
    myfatal(h);
}

/* Print warning message */
void cbWarning(cbCtx h, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    h->logger->vlog(sWARNING, fmt, ap);
    va_end(ap);
}

/* [hot callback] Return source filename */
static const char *inId(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return h->in.file.name;
}

/* [hot callback] Return OTF filename */
static const char *outId(void *ctx) {
    cbCtx h = (cbCtx)ctx;
    return h->out.file.name;
}

/* ------------------------------- Tmp Stream ------------------------------ */

/* [hot callback] Open temporary file */
static void tmpOpen(cbCtx h) {
}

/* [hot callback] Write multiple bytes to temporary file */
static void tmpWriteN(cbCtx h, long count, char *ptr) {
    sFileWriteN(&h->tmp.file, count, ptr);
}

/* [hot callback] Rewind temporary file */
static void tmpRewind(cbCtx h) {
    sFileSeek(&h->tmp.file, 0, SEEK_SET);
}

/* [hot callback] Refill temporary file input buffer */
static char *tmpRefill(cbCtx h, long *count) {
    *count = sFileReadN(&h->tmp.file, BUFSIZ, h->tmp.buf);
    return h->tmp.buf;
}

/* [hot callback] Close temporary file */
static void tmpClose(cbCtx h) {
    if (h->tmp.file.fp != NULL)
        sFileClose(&h->tmp.file);
}

/* ---------------------------- Stream Callbacks --------------------------- */

/* Open stream. */
static void *stm_open(ctlStreamCallbacks *cb, int id, size_t size) {
    cbCtx h = (cbCtx) cb->direct_ctx;
    Stream *s = NULL;
    switch (id) {
        case CFR_SRC_STREAM_ID:
            /* Source stream already open; just return it */
            s = &h->in;
            break;
        case OTF_DST_STREAM_ID:
            /* Open destination stream */
            s = &h->out;
            if (s->file.fp == NULL)
                return NULL;
            break;
        case OTF_TMP_STREAM_ID:
            s = &h->tmp;
            if (s->file.fp == NULL)
                sFileOpen(&s->file, "tmpfile", "w+b");
            break;
        default:
            cbFatal(h, "invalid stream open");
    }
    return s;
}

/* Seek to stream position. */
static int stm_seek(ctlStreamCallbacks *cb, void *stream, long offset) {
    if (offset >= 0) {
        Stream *s = (Stream *)stream;
        sFileSeek(&s->file, offset, SEEK_SET);
        return 0;  // XXX what if not successful?
    }
    return -1; /* Bad seek */
}

/* Return stream position. */
static long stm_tell(ctlStreamCallbacks *cb, void *stream) {
    Stream *s = (Stream *)stream;
    return sFileTell(&s->file);
}

/* Read from stream. */
static size_t stm_read(ctlStreamCallbacks *cb, void *stream, char **ptr) {
    Stream *s = (Stream *)stream;
    size_t l = sFileReadN(&s->file, BUFSIZ, s->buf);
    if (l > 0)
        *ptr = s->buf;
    return l;
}

/* Write to stream. */
static size_t stm_write(ctlStreamCallbacks *cb, void *stream,
                        size_t count, const char *ptr) {
    Stream *s = (Stream *)stream;
    return sFileWriteN(&s->file, count, (void *)ptr);
}

/* Return stream status. */
static int stm_status(ctlStreamCallbacks *cb, void *stream) {
    Stream *s = (Stream *)stream;
    if (feof(s->file.fp))
        return CTL_STREAM_END;
    else if (ferror(s->file.fp))
        return CTL_STREAM_ERROR;
    else
        return CTL_STREAM_OK;
}

/* Close stream. */
static int stm_close(ctlStreamCallbacks *cb, void *stream) {
    cbCtx h = (cbCtx) cb->direct_ctx;
    Stream *s = (Stream *)stream;
    if (s != &h->in && s != &h->out) {
        sFileClose(&s->file);
    }
    return 0;
}

/* Initialize stream record. */
static void stmClear(Stream *s) {
    memset((void *)s, 0, sizeof(Stream));
}

/* Close steam at exit if still open. */
static void stmFree(Stream *s) {
    if (s->file.fp != NULL)
        sFileClose(&s->file);
}

/* -------------------------- Feature file input --------------------------- */

static const char *featTopLevelFile(void *ctx) {
    cbCtx h = (cbCtx)ctx;

    return h->feat.mainFile;
}

static void featAddAnonData(void *ctx, const char *data, long count,
                            unsigned long tag) {
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

/* Initialize stream callbacks and stream records. */
void hotcbInit(cbCtx h) {
    h->hot.cb.stm.direct_ctx = h;
    h->hot.cb.stm.indirect_ctx = NULL;
    h->hot.cb.stm.clientFileName = NULL;
    h->hot.cb.stm.open = stm_open;
    h->hot.cb.stm.seek = stm_seek;
    h->hot.cb.stm.tell = stm_tell;
    h->hot.cb.stm.read = stm_read;
    h->hot.cb.stm.xml_read = NULL;
    h->hot.cb.stm.write = stm_write;
    h->hot.cb.stm.status = stm_status;
    h->hot.cb.stm.close = stm_close;

    h->hot.cb.ctx = h;
    h->hot.cb.fatal = myfatal;
    h->hot.cb.message = motf_message;
    h->hot.cb.inId = inId;
    h->hot.cb.outId = outId;
    h->hot.cb.featTopLevelFile = featTopLevelFile;
    h->hot.cb.featAddAnonData = featAddAnonData;
    h->hot.cb.uvsOpen = uvsOpen;
    h->hot.cb.uvsGetLine = uvsGetLine;
    h->hot.cb.uvsClose = uvsClose;
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

void cbAliasDBRead(cbCtx h, const char *filename) {
    if (!h->goadb->read(filename))
        cbFatal(h, "Problem reading GlyphOrderAndAliasDB file");
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

/* ---------------------------- Callback Context --------------------------- */

/* Create new callback context */
cbCtx cbNew(const char *progname, const char *indir, const char *outdir,
            const char *cmapdir, const char *featdir, dnaCtx mainDnaCtx) {
    cbCtx h = new cbCtx_;

    hotcbInit(h);

    stmClear(&h->in);
    stmClear(&h->out);
    stmClear(&h->tmp);

    fcdbCallbacks fcdbcb;

    h->logger = slogger::getLogger("addfeatures");
    h->goadb = std::make_shared<GOADB>(h->logger);  // XXX use final names?

    /* Initialize context */
    h->progname = progname;
    h->dir.in = indir;
    h->dir.out = outdir;
    h->dir.cmap = cmapdir;

    h->dnactx = mainDnaCtx;

    h->hot.ctx = hotNew(&h->hot.cb, h->goadb, h->logger);
    dnaINIT(mainDnaCtx, h->tmpbuf, 32, 32);
    h->mac.encoding = NULL;
    h->mac.cmapScript = HOT_CMAP_UNKNOWN;
    h->mac.cmapLanguage = HOT_CMAP_UNKNOWN;

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
        snprintf(otfpath, FILENAME_MAX+1, "%s%s.otf", h->dir.out, h->tmpbuf.array);
    } else {
        snprintf(otfpath, FILENAME_MAX+1, "%s%s.otf", h->dir.out, FontName);
    }
}

static void ProcessFontInfo(hotCtx g, const char *version,
                            const char *FontName, hotMacData *mac,
                            long otherflags,
                            short fsSelectionMask_on,
                            short fsSelectionMask_off,
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
    common.flags = HOT_WIN;

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
void cbConvert(cbCtx h, int flags, const char *clientVers, const char *infile,
               const char *outfile,
               const char *featurefile, const char *hcmapfile,
               const char *vcmapfile, const char *mcmapfile, const char *uvsFile,
               long otherflags, short macScript, short macLanguage,
               long addGlyphWeight,
               short fsSelectionMask_on, short fsSelectionMask_off,
               unsigned short os2Version, const char *licenseID) {
    int type;
    const char *FontName;
    char inpath[FILENAME_MAX + 1];
    char outpath[FILENAME_MAX + 1];
    int freeFeatName = 0;
    unsigned long hotConvertFlags = 0;
    bool isCID;

    if (otherflags & OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT) {
        hotConvertFlags |= HOT_ID2_CHAIN_CONTXT3;
    }

    if (otherflags & OTHERFLAGS_ALLOW_STUB_GSUB) {
        hotConvertFlags |= HOT_ALLOW_STUB_GSUB;
    }

    if (h->fcdb.syntaxVersion == 1) {
        hotConvertFlags |= HOT_USE_V1_MENU_NAMES;
    }

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
        hotConvertFlags |= HOT_VERBOSE;
    }

    if (otherflags & OTHERFLAGS_FINAL_NAMES) {
        hotConvertFlags |= HOT_CONVERT_FINAL_NAMES;
    }
    if (otherflags & OTHERFLAGS_LOOKUP_FINAL_NAMES) {
        hotConvertFlags |= HOT_LOOKUP_FINAL_NAMES;
    }

    hotSetConvertFlags(h->hot.ctx, hotConvertFlags);

    /* Make IN path */
    snprintf(inpath, sizeof(inpath), "%s%s", h->dir.in, infile);

    if (!sFileExists(inpath)) {
        cbFatal(h, "Source font file not found: %s \n", inpath);
        return;
    }
    if ((featurefile != NULL) && (!sFileExists(featurefile))) {
        cbFatal(h, "Feature file not found: %s \n", featurefile);
        return;
    }

    /* Read in font file */
    sFileOpen(&h->in.file, inpath, "rb");
    FontName = hotReadFont(h->hot.ctx, flags, isCID);

    if (uvsFile != NULL) {
        hotAddUVSMap(h->hot.ctx, uvsFile);
    }

    /* Determine dir that feature file's in */
    h->feat.mainFile = featurefile;

    if (isCID) {
        if (hcmapfile == NULL) {
            cbFatal(h, "no CMaps specified [%s]\n", inpath);
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

    h->mac.cmapScript = macScript; /* Used in hotAddmiscData, in ProcessFontInfo */
    h->mac.cmapLanguage = macLanguage;

    ProcessFontInfo(h->hot.ctx, clientVers, FontName, &h->mac, otherflags,
                    fsSelectionMask_on, fsSelectionMask_off,
                    os2Version, licenseID);

    /* Make OTF path */
    if (outfile == NULL) {
        makeOTFPath(h, outpath, FontName);
    } else {
        snprintf(outpath, sizeof(outpath), "%s%s", h->dir.out, outfile);
    }

    /* Write OUT file */
    sFileOpen(&h->out.file, outpath, "w+b");
    hotConvert(h->hot.ctx);
    sFileClose(&h->in.file);  // Close here so hotConvert can copy CFF, etc.
    sFileClose(&h->out.file);
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
    dnaFREE(h->tmpbuf);

    // Free database resources
    fcdbFree(h->fcdb.ctx);
    for (i = 0; i < h->fcdb.files.cnt; i++) {
        sFileClose(&h->fcdb.files.array[i]);
    }
    dnaFREE(h->fcdb.files);

    delete h;
}
