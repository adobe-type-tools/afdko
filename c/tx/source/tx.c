/* Copyright 2014-2018 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type eXchange.
 */

#include "tx_shared.h"

#define TX_VERSION CTL_MAKE_VERSION(1, 2, 3)

#include "varread.h"

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

static void cff2_Help(txCtx h) {
    static char *text[] = {
#include "cff2.h"
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
    float *uv;
    if (h->cfr.ctx == NULL) {
        h->cfr.ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
        if (h->cfr.ctx == NULL)
            fatal(h, "(cfr) can't init lib");
    }

    if (h->flags & SUBSET_OPT && h->mode != mode_dump)
        h->cfr.flags |= CFR_UPDATE_OPS; /* Convert seac for subsets */

    uv = getUDV(h);
    if (uv)
        h->cfr.flags |= CFR_FLATTEN_VF;
    if (cfrBegFont(h->cfr.ctx, h->cfr.flags, origin, ttcIndex, &h->top, uv))
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
                if (h->cfw.flags & CFW_WRITE_CFF2) {
                    cff2_Help(h);
                } else {
                    cff_Help(h);
                }
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
        "    tx        %s\n",
        CTL_SPLIT_VERSION(version_buf, TX_VERSION));

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
    varreadGetVersion(&cb);

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

/* Process file. */
static void doFile(txCtx h, char *srcname) {
    long i;
    char *p;
    struct stat fileStat;
    int statErrNo;

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

    h->t1r.flags = 0; /* I initialize these here,as I need to set the std Encoding flags before calling setMode. */
    h->cfr.flags = 0;
    h->cfw.flags = 0;
    h->dcf.flags = DCF_AllTables | DCF_BreakFlowed;
    h->dcf.level = 5;
    h->svr.flags = 0;
    h->ufr.flags = 0;
    h->ufow.flags = 0;
    h->t1w.options = 0;

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
            case opt_cff2:
                h->cfw.flags |= CFW_WRITE_CFF2;
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
                h->flags |= PATH_SUPRESS_HINTS;
                /* Setting the hint callbacks to NULL works for the most common
                   case where the callbacks have already been assigned by
                   setMode. However, in a number of cases, the call backs are
                   assigned later, within beginFont. This, we need the flag, so
                   we can do the right thing there.*/
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
            case opt_no_futile:
                switch (h->mode) {
                    case mode_cff:
                        fprintf(stderr, "%s: option -no_futile deprecated (ignored)\n", h->progname);
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt_no_opt:
                switch (h->mode) {
                    case mode_cff:
                        h->cfw.flags |= CFW_NO_OPTIMIZATION;
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
            case opt_V:
                switch (h->mode) {
                    case mode_cff:
                    case mode_t1:
                        h->flags &= ~PATH_REMOVE_OVERLAP;
                        break;
                    default:
                        goto wrongmode;
                }
                break;
            case opt__V:
                switch (h->mode) {
                    case mode_cff:
                    case mode_t1:
                        h->flags |= PATH_REMOVE_OVERLAP;
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
            case opt_maxs: /* set max number subrs. */
                if (!argsleft)
                    goto noarg;
                else {
                    char *p;
                    char *q;
                    p = argv[++i];
                    h->cfw.maxNumSubrs = strtol(p, &q, 0);
                    if (*q != '\0')
                        goto badarg;
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

    h->progname = progname;
    h->flags = 0;
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
    h->cfw.maxNumSubrs = 0; /* 0 is translated to the MAX_NUMBER_SUBRS defined in the cffWrite module. */
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
    dnaINIT(h->ctx.dna, h->dcf.varRegionInfo, 1, 15);
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

    memFree(h, h->script.buf);
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
    dnaFREE(h->dcf.varRegionInfo);
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

    h->app = APP_TX;
    h->appSpecificInfo = NULL; /* unused in tx.c, used in rotateFont.c & mergeFonts.c */
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
