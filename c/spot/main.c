/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <string.h>
#include <stdlib.h>

#include "spot.h"
#include "spot_global.h"
#include "sfnt.h"
#include "sfnt_sfnt.h"
#include "CFF_.h"
#include "res.h"
#include "glyf.h"
#include "proof.h"
#include "da.h"
#include "cmap.h"
#include <ctype.h>
#include "setjmp.h"
#include "map.h"
#include "sfile.h"

jmp_buf spot_mark;
#define MAX_ARGS 200

static const char *version = "3.5.65520";    /* Program version */
char *sourcepath;
char *outputfilebase = NULL;
char *infilename = NULL;

extern GlyphComplementReportT gcr; /*Used for Glyph Complement Reporter*/
extern char aliasfromfileinit;

char *glyphaliasfilename = NULL;

/*VORGfound: set in CFF_getMetrics to indicate whether a VORG entry was found for the glyph.*/
char VORGfound = 0;

#if AUTOSCRIPT
static int8_t scriptfilename[512];

typedef struct _cmdlinetype {
    da_DCL(char *, args); /* arg list */
} cmdlinetype;

static struct
{
    int8_t *buf; /* input buffer */
    da_DCL(cmdlinetype, cmdline);
} script;

#endif

static char *MakeFullPath(char *source) {
    char *dest;

    dest = (char *)sMemNew(_MAX_PATH);
    if (sourcepath[0] == '\0' || strchr(source, '\\') != NULL)
        sprintf(dest, "%s", source);
    else
        sprintf(dest, "%s\\%s", sourcepath, source);
    return dest;
}

/* File signatures */

typedef uint32_t ctlTag;
#define CTL_TAG(a, b, c, d) \
    ((ctlTag)(a) << 24 | (ctlTag)(b) << 16 | (ctlTag)(c) << 8 | (ctlTag)(d))

#define sig_PostScript0 CTL_TAG('%', '!', 0x00, 0x00)
#define sig_PostScript1 CTL_TAG('%', 'A', 0x00, 0x00) /* %ADO... */
#define sig_PostScript2 CTL_TAG('%', '%', 0x00, 0x00) /* %%... */
#define sig_PFB         ((ctlTag)0x80010000)
#define sig_CFF         ((ctlTag)0x01000000)
#define sig_MacResource ((ctlTag)0x00000100)
#define sig_AppleSingle ((ctlTag)0x00051600)
#define sig_AppleDouble ((ctlTag)0x00051607)
typedef struct /* AppleSingle/Double entry descriptor */
{
    uint32_t id;
    int32_t offset;
    uint32_t length;
} EntryDesc;

/* Process AppleSingle/Double format data. */
static void doASDFormats(ctlTag magic) {
    long junk;
    long i;
    uint16_t entryCount = 0;
    struct /* AppleSingle/Double data */
    {
        uint32_t magic;               /* Magic #, 00051600-single, 00051607-double */
        uint32_t version;             /* Format version */
        da_DCL(EntryDesc, entries); /* Entry descriptors */
    } asd;

    asd.magic = magic;
    IN1(asd.version);

    /* Skip filler of 16 bytes*/
    IN1(junk);
    IN1(junk);
    IN1(junk);
    IN1(junk);

    /* Read number of entries */
    IN1(entryCount);
    da_INIT(asd.entries, entryCount, 10);

    /* Read entry descriptors */
    for (i = 0; i < entryCount; i++) {
        EntryDesc *entry = da_INDEX(asd.entries, i);
        IN1(entry->id);
        IN1(entry->offset);
        IN1(entry->length);
    }

    for (i = 0; i < entryCount; i++) {
        EntryDesc *entry = da_INDEX(asd.entries, i);
        if (entry->length > 0)
            switch (entry->id) {
                case 1:
                    /* Data fork (AppleSingle); see if it's an sfnt */
                    sfntRead(entry->offset + 4, -1); /* Read plain sfnt file */
                    sfntDump();
                    sfntFree_spot(1);
                    break;
                case 2:
                    /* Resource fork (AppleSingle/Double) */
                    fileSeek(entry->offset, 0);
                    resRead(entry->offset); /* Read and dump Macintosh resource file */
                    break;
            }
    }
}

static int readFile(char *filename) {
    uint32_t value;

    /* See if we can recognize the file type */
    value = fileSniff();
    switch (value) {
        case bits_:
        case typ1_:
        case true_:
        case mor0_:
        case OTTO_:
        case VERSION(1, 0):
            sfntRead(0, -1); /* Read plain sfnt file */
            sfntDump();
            sfntFree_spot(1);
            break;
        case ttcf_:
            sfntTTCRead(0); /* Read TTC and dump file */
            break;
        case 256:
            SEEK_ABS(0); /* Raw resource file as a data fork */
            resRead(0);  /* Read and dump  Macintosh resource file */
            break;
        case sig_AppleSingle:
        case sig_AppleDouble:
            doASDFormats((ctlTag)value);
            break;
        default:
            spotWarning(SPOT_MSG_BADFILE, filename);
            return 1;
    }
    return 0;
}

/* Print usage information */
static void printUsage(void) {
    fprintf(OUTPUTBUFF,
            "Usage: %s [-u|-h|-ht|-r] [-n|-nc|-G|-T|-F] [-f] [-V] [-m] [-d]"
#if AUTOSPOOL
            "[-l|-O] "
#endif
            "[-i<ids>] [-o<offs>] [-t<tags>|-P<featuretags>] [-p<policy>] [-@ <ptsize>]  "
            "<fontfile>+\n\n"
#if AUTOSCRIPT
            "OR: %s  -X <scriptfile>\n\n"
#endif
            "Options:\n"
            "    -u  print usage information\n"
            "    -h  print usage and help information\n"
            "   -ht  print table-specific usage information\n"
            "    -r  dump Macintosh resource map\n"
            "    -n  dump glyph id/name mapping (also see '-m' below)\n"
            "   -nc  dump glyph id/name mapping, one per line (also see '-m' below)\n"
            " -ngid  Suppress terminal gid on glyph names from TTF fonts.\n"
            "    -T  list table-directory in sfnt table\n"
            "    -F  list features in GPOS,GSUB tables\n"
            "    -G  proof glyph synopsis\n"
            "    -f  proof GPOS features in font order instead of GID order\n"
            "    -V  proof glyphs in Vertical writing mode (same as -p6 below)\n"
            "    -m  map glyph names into Adobe 'friendly' names, not AGL/Unicode names\n"
            "    -d  suppress header info from proof\n"
            "   -br  proof glyph synopsis one per page\n"
#if AUTOSPOOL
            "    -l  leave proofing output files; do not spool them to printer\n"
            "    -O  proof data to 'stdout' (e.g., for sending to printer or saving to some file)\n"
#endif
            "    -i  sfnt resource id list (see help)\n"
            "    -o  TTC directory offsets list (see help)\n"
            "    -t  table dump list (see help)\n"
            "    -P  <list of feature tags from GSUB or GPOS>, e.g 'P cswh,frac,kern'  (use '-Proof' for all)\n"
            "    -p  set proofing policies: \n"
            "        1=No glyph name labels\n"
            "        2=No glyph numeric labels\n"
         /* "        3=Show glyph lines (lsb, rsb, etc.)\n" */
            "        4=Show KanjiStandardEMbox on glyph\n"
            "        5=Show GlyphBBox on glyph\n"
            "        6=Show Kanji in Vertical writing mode\n"
            "        7=Don't show Kanji 'kern','vkrn' with 'palt','vpal' values applied\n"
            "    -@  set proofing glyph point-size (does not apply to certain synopses)\n"
#if AUTOSCRIPT
            "    -X  execute a series of complete command-lines from <scriptfile> [default: OTFproof.scr ]\n"
#endif
            "\n"
            "Note: Proof options write a PostScript file to standard output, and must be redirected to a file.\n"
            "Example: 'spot -P kern test.otf > kern.ps'\n"
            "This file can then be converted to PDF with Distiller, or downloaded to a printer.\n"
            "\n"
            "Version:\n"
            "    %s\n",
#if AUTOSCRIPT
            spotGlobal.progname,
#endif
            spotGlobal.progname,
            version);
}

/* Show usage information */
static void showUsage(void) {
    printUsage();
    quit(0);
}

/* Show usage and help information */
static void showHelp(void) {
    printUsage();
    sfntUsage();
    fprintf(OUTPUTBUFF,
        "Notes:\n"
        "\n"
        "This program dumps sfnt data from plain files or Macintosh resource\n"
        "files.\n"
        "In the latter case, when there are 2 or more sfnt resources, or in\n"
        "the absence of the -i option, the resource map is dumped as a list of\n"
        "resource types and ids. The program may then be rerun with an argument\n"
        "to the -i option specifying a comma-separated sfnt id list of the sfnt\n"
        "resources to be dumped or the argument 'all' which dumps all sfnt\n"
        "resources.\n"
        "\n"
        "All the multiple sfnts within an OpenType Collection (TTC) will be\n"
        "dumped by default but may be selectively dumped via TTC directory\n"
        "offsets specified with the -o option, e.g. -o0x14,0x170. Use the\n"
        "-tttcf option to view all the TTC directory offsets available.\n"
        "\n"
        "The argument to the -t option specifies a list of tables to be dumped.\n"
        "Tables are selected by a comma-separated list of table tags, e.g.\n"
        "cmap,MMVR,fdsc. An optional dump level may be specified by appending\n"
        "'=' followed by the level to a tag, e.g. cmap=2. Higher levels print\n"
        "successively more information up to a maximum of 4 (the default)\n"
        "although all 4 levels are not supported by all tables. Level 1 always\n"
        "prints the table tag and file offset. When a level is specified its\n"
        "value becomes the default for the remaining tables in the list unless\n"
        "a new level is specified, e.g. cmap=2,MMVR,HFMX,fdsc=1 dumps the cmap,\n"
        "MMVR, and HFMX tables at level 2 and the fdsc table at level 1. If a\n"
        "level is given as 'x', e.g. cmap=x the table is dumped in hexadecimal\n"
        "format. If an unsupported table tag is specified the table is dumped\n"
        "in hexadecimal format regardless of whether a level of 'x' is\n"
        "specified.\n\n");
    fprintf(OUTPUTBUFF,
        "The -n option dumps glyph id to glyph name mapping for all glyphs in\n"
        "the font. Naming information is extracted from post format 1.0, 2.0,\n"
        "or 2.5 tables, or if they are not available, the MS UGL cmap. In the\n"
        "event that none of these is available the glyph name is simply printed\n"
        "as @ followed by the decimal glyph id in decimal.\n"
        "\n"
        "The special tag 'sfnt' dumps the sfnt directory which lists all the\n"
        "tables present in the sfnt. The special tag 'uset' sets the dump level\n"
        "on all unset tables in the font allowing tables to be dumped without\n"
        "explicitly naming them, e.g. -tuset will dump all tables at the\n"
        "default level 4. If this produces too much information you can disable\n"
        "dumping of selected tables by setting their dump level to 0, e.g.\n"
        "-tuset,glyf=0,hdmx.\n"
        "\n"
        "The -ht option provides additional table-specific help.\n");

    quit(0);
}

#if AUTOSCRIPT
static void makeArgs(char *filename) {
    int state;
    long i;
    long length;
    sFile file;
    char *start = NULL; /* Suppress optimizer warning */
    cmdlinetype *cmdl;

    /* Read whole file into buffer */
    sFileOpen(&file, filename, "r");
    if (file.fp == NULL)
        spotFatal(SPOT_MSG_BADSCRIPTFILE, filename);
    length = sFileLen(&file);
    if (length < 1)
        spotFatal(SPOT_MSG_BADSCRIPTFILE, filename);
    script.buf = sMemNew(length + 2);

    spotMessage(SPOT_MSG_ECHOSCRIPTFILE, filename);
    sFileSeek(&file, 0, 0);
    sFileReadN(&file, length, (uint8_t *)script.buf);
    sFileClose(&file);

    script.buf[length] = '\n';     /* Ensure termination */
    script.buf[length + 1] = '\0'; /* Ensure termination */
    /* Parse buffer into args */
    state = 0;
    da_INIT(script.cmdline, 10, 10);
    cmdl = da_NEXT(script.cmdline);
    da_INIT(cmdl->args, 10, 10);
    *da_NEXT(cmdl->args) = spotGlobal.progname;
    for (i = 0; i < length + 1; i++) {
        char c = script.buf[i];
        switch (state) {
            case 0:
                switch ((int)c) {
                    case '\n':
                    case '\r':
                        cmdl = da_NEXT(script.cmdline);
                        da_INIT(cmdl->args, 10, 10);
                        *da_NEXT(cmdl->args) = spotGlobal.progname;
                        break;
                    case '\f':
                    case '\t':
                        break;
                    case ' ':
                        break;
                    case '#':
                        state = 1;
                        break;
                    case '"':
                        start = &script.buf[i + 1];
                        state = 2;
                        break;
                    default:
                        start = &script.buf[i];
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
                    script.buf[i] = '\0'; /* Terminate string */
                    *da_NEXT(cmdl->args) = start;
                    state = 0;
                }
                break;
            case 3: /* Space-delimited string */
                if (isspace((int)c)) {
                    script.buf[i] = '\0'; /* Terminate string */
                    *da_NEXT(cmdl->args) = start;
                    state = 0;
                    if ((c == '\n') || (c == '\r')) {
                        cmdl = da_NEXT(script.cmdline);
                        da_INIT(cmdl->args, 10, 10);
                        *da_NEXT(cmdl->args) = spotGlobal.progname;
                    }
                }
                break;
        }
    }
}
#endif

/* Main program */
int main__spot(int argc, char *argv[]) {
    int value = 0;
    static double glyphptsize = STDPAGE_GLYPH_PTSIZE;
    static opt_Option opt[] =
    {
        {"-u", opt_Call, (void *)showUsage},
        {"-h", opt_Call, (void *)showHelp},
        {"-ht", opt_Call, (void *)sfntTableSpecificUsage},
#if AUTOSPOOL
        {"-l", opt_Flag},
        {"-O", opt_Flag},
#endif
        {"-r", opt_Flag},
        {"-n", opt_Flag},
        {"-nc", opt_Flag},
        {"-ngid", opt_Flag},
        {"-T", opt_Flag},
        {"-F", opt_Flag},
        {"-f", opt_Flag},
        {"-G", opt_Flag},
        {"-V", opt_Flag},
        {"-m", opt_Flag},
        {"-d", opt_Flag},
        {"-br", opt_Flag},
        {"-i", resIdScan},
        {"-o", sfntTTCScan},
        {"-t", sfntTagScan},
        {"-P", sfntFeatScan},
        {"-A", sfntFeatScan},
        {"-p", proofPolicyScan},
        {"-a", opt_Flag},
        {"-R", opt_Flag},
        {"-c", opt_Flag},
        {"-g", glyfGlyphScan},
        {"-b", glyfBBoxScan},
        {"-s", glyfScaleScan},
        {"-@", opt_Double, &glyphptsize},
        {"-C", opt_Int, &cmapSelected},
#if AUTOSCRIPT
        {"-X", opt_String, scriptfilename},
#endif
        {"-ag", opt_String, &glyphaliasfilename},
        {"-of", opt_String, &outputfilebase},
    };

    int files, goodFileCount = 0;
    int argi;
    char *filename = NULL;
    volatile int i = 0;
#if AUTOSCRIPT
    cmdlinetype *cmdl;
    int8_t foundXswitch = 0;
#endif
    int status = 0; /* = setjmp(spotGlobal.env); only used when compiled as lib */

    if (status) {
#if AUTOSCRIPT
        if (spotGlobal.doingScripting) {
            goto scriptAbEnd;
        } else
#endif
            exit(status - 1); /* Finish processing */
    }
    gcr.reportNumber = 0;
    /*  value = setjmp(spot_mark); only used when compiled as lib */

    if (value == -1)
        exit(1);

    da_SetMemFuncs(sMemNew, sMemResize, sMemFree);
    spotGlobal.progname = "spot";

#if AUTOSCRIPT
    scriptfilename[0] = '\0'; /* init */

    if (!foundXswitch && (argc < 2)) /* if no -X on cmdline, and no OTHER switches */
        strcpy(scriptfilename, "spot.scr");

    /* see if scriptfile exists to Auto-execute */
    if ((scriptfilename[0] != '\0') && sFileExists(scriptfilename)) {
        spotGlobal.doingScripting = 1;
        makeArgs(scriptfilename);
    }
#endif /* AUTOSCRIPT */

    if (
#if AUTOSCRIPT
        !spotGlobal.doingScripting
#else
        1
#endif
    ) {
        argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
        if (opt_hasError()) {
            exit(1);
        }

        if (argi == 0)
            showUsage();

#if AUTOSCRIPT
        if (!spotGlobal.doingScripting && opt_Present("-X")) {
            if (scriptfilename[0] != '\0') {
                spotGlobal.doingScripting = 1;
                makeArgs(scriptfilename);
                goto execscript;
            }
        }
#endif

        if (opt_Present("-@"))
            proofSetGlyphSize(glyphptsize);

        if (opt_Present("-V")) /* equivalent to "-p6" */
            proofSetPolicy(6, 1);

        if (opt_Present("-ngid"))
            spotGlobal.flags |= SUPPRESS_GID_IN_NAME;

        files = argc - argi;

        for (; argi < argc; argi++) {
            filename = argv[argi];

            if (files > 1) {
                spotInform(SPOT_MSG_PROOFING, filename);
            }

            if (outputfilebase == NULL)
                outputfilebase = filename;
            fileOpen(filename);

            if (!fileIsOpened()) {
                spotWarning(SPOT_MSG_BADFILE, filename);
                fileClose();
                continue;
            }
            if (readFile(filename)) {
                fileClose();
                continue;
            }

            goodFileCount++;
            fileClose();
        }
    }
#if AUTOSCRIPT
    else /* executing cmdlines from a script file */
    {
    execscript : {
        char *end;

        end = strrchr(scriptfilename, '\\');
        if (end == NULL)
            sourcepath = "";
        else {
            char *scurr = scriptfilename;
            char *dcurr;

            sourcepath = (char *)sMemNew(strlen(scriptfilename));
            dcurr = sourcepath;
            while (scurr != end) {
                *dcurr++ = *scurr++;
            }
            *dcurr = 0;
        }
    }

        for (i = 0; i < script.cmdline.cnt; i++) {
            char *tempfilename;

            cmdl = da_INDEX(script.cmdline, i);
            if (cmdl->args.cnt < 2) continue;

            proofResetPolicies();

            {
                int a;

                spotInform(SPOT_MSG_EOLN);
                spotMessage(SPOT_MSG_ECHOSCRIPTCMD);
                for (a = 1; a < cmdl->args.cnt; a++) {
                    spotInform(SPOT_MSG_RAWSTRING, cmdl->args.array[a]);
                }
                spotInform(SPOT_MSG_EOLN);
            }

            argi = opt_Scan(cmdl->args.cnt, cmdl->args.array, opt_NOPTS(opt), opt, NULL, NULL);
            if (opt_hasError()) {
                exit(1);
            }

            if (opt_Present("-@"))
                proofSetGlyphSize(glyphptsize);
            if (opt_Present("-V")) /* equivalent to "-p6" */
                proofSetPolicy(6, 1);

            tempfilename = MakeFullPath(cmdl->args.array[cmdl->args.cnt - 1]);

            if (sdFileExists(tempfilename)) { /* (new) font filename on cmdline */
                sMemFree(tempfilename);
                if (filename != NULL) /* not first time */
                {
                    fileClose(); /* previous font file */
                    sfntFree_spot(1);
                }
                if (sourcepath[0] != '\0')
                    filename = MakeFullPath(cmdl->args.array[cmdl->args.cnt - 1]);
                else
                    filename = cmdl->args.array[cmdl->args.cnt - 1];
                fileOpen(filename);
                if (outputfilebase == NULL)
                    outputfilebase = filename;
                spotInform(SPOT_MSG_PROOFING, filename);
                goodFileCount++;

                if (readFile(filename)) {
                    goodFileCount--;
                    fileClose();
                    continue;
                }

            } else {
                /* none specified */
                spotFatal(SPOT_MSG_MISSINGFILENAME);
                sMemFree(tempfilename);
                continue;
            }

            sfntDump();

        scriptAbEnd:
            sfntFree_spot(1);
            fileClose();
        }
        spotGlobal.doingScripting = 0;
    }
#endif /* AUTOSCRIPT */

    /* spotInform("Done."); */
    if (goodFileCount <= 0)
        exit(1);

    quit(0);
    return 0;
}
