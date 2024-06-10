/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/* This module provides a test client for the hot library. hot library clients
   do not need to include this module; it is merely provided for reference */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "setjmp.h"

#define PROFILE 0
#if PROFILE
#include <profiler.h>
#endif

#include "dynarr.h"
#include "hotconv.h"
#include "cb.h"
#include "sfile.h"
#include "smem.h"

#define ADDFEATURES_VERSION "2.6.0"
/* Warning: this string is now part of heuristic used by CoolType to identify the
   first round of CoolType fonts which had the backtrack sequence of a chaining
   contextual substitution ordered incorrectly.  Fonts with the old ordering MUST match
   the regex:
     "(Version|OTF) 1.+;Core 1\.0\..+;addfeatureslib1\."
   inside the (1,0,0) nameID 5 "Version: string. */

static char *progname; /* Program name */
static cbCtx cbctx;    /* Client callback context */

/* Conversion data */
static struct {
    struct { /* Directory paths */
        char in[FILENAME_MAX + 1];
        char out[FILENAME_MAX + 1];
        char cmap[FILENAME_MAX + 1];
        char feat[FILENAME_MAX + 1];
    } dir;
    int fontDone;
    char *features;
    char *hCMap;
    char *vCMap;
    char *mCMap;
    char *uvsFile;
    short macScript; /* Script and language for Mac cmap. Usu set to HOT_CMAP_UNKNOWN, to use heuristics. */
    short macLanguage;
    short flags; /* Library flags */
    long otherflags;
    long addGlyphWeight;        /* used when adding synthetic glyphs,            */
                                /* to override heuristics for determining        */
                                /* target weight for use with the FillinMM font. */
    unsigned long maxNumSubrs;  /* if not 0, caps the number of subrs used. */
    unsigned short os2_version; /* OS/2 table version override. Ignore if 0. */
    short fsSelectionMask_on;   /* mask for OR-ing with OS/2 table fsSelection.  */
    short fsSelectionMask_off;  /* mask for NAND-ing with OS/2 table fsSelection. */
    char *licenseID;
} convert;

/* Print usage information */
static void printUsage(void) {
    printf(
        "Usage:\n"
        "    %s [options]\n"
        "Options:\n"
        "-h   : Help\n"
        "-u   : Help\n"

        "-f <path> : Specify path to font file. Default for roman font is 'font.otf'\n"
        "-o <path> : Specify output file name for OTF file. Default is <PostScript\n"
        "    name> + '.otf'\n"
        "-b : Specify that font has bold style.\n"
        "-i : Specify that font has italic style.\n"
        "-ff <path> : Specify path for feature file\n"
        "-fs : If there are no GSUB rules, make a stub GSUB table.\n"
        "-mf <path> : Specify path for the FontMenuNameDB file.\n"
        "-gf <path> : Specify path for the GlyphOrderAndAliasDB file. This is only\n"
        "    used as a source for unicode mappings\n"
        "-ds <path> : Specify path to a Designspace file. This is only used as a\n"
        "    source for mappings of user to design units\n"
        "-lookupFinal : Look for final GlyphOrderAndAliasDB names when determining\n"
        "    Unicode values\n"
        "-showFinal : Display final GlyphOrderAndAliasDB names even when not\n"
        "    substituting\n"
        "-osbOn n : Set bit 'n' to on in the OS/2 table field 'fsSelection'. Can be\n"
        "    used more than once to set different bits, for example '-osbOn 7 -osbOn\n"
        "    8' to set the bits 7 and 8 to true.\n"
        "-osbOff n : set bit 'n' to off in the OS/2 table field 'fsSelection'. Can be\n"
        "    used more than once to set different bits, for example '-osbOff 7\n"
        "    -osbOff 8' to set the bits 7 and 8 to false. Option osbOff is always \n"
        "    processed after osbOn.\n"
        "-osv n : Set the OS/2 table version to integer value 'n'.\n"
        "-cs <integer> : Integer value for Mac cmap script ID. -1 means undefined.\n"
        "-cl <integer> : Integer value for Mac cmap language ID. -1 means undefined.\n"
        "-cm <path> : Path to Mac encoding CMAP file. Used only for CID fonts.\n"
        "-ch <path> : Path to Unicode horizontal CMAP file. Used only for CID fonts.\n"
        "-cv <path> : Path to Unicode vertical CMAP file. Used only for CID fonts.\n"
        "-ci <path> : Path to Unicode Variation Sequences specification file.\n"
        "-dbl : Double map a predefined set of glyphs to two Unicode values if\n"
        "    there is no other glyph mapped to the same Unicode value. This used to\n"
        "    be the default behavior. The glyph list is: Delta:(0x2206, 0x0394),\n"
        "    Omega:(0x2126, 0x03A9), Scedilla:(0x015E, 0xF6C1), Tcommaaccent:(0x0162,\n"
        "    0x021A), fraction:(0x2044, 0x2215), hyphen:(0x002D, 0x00AD),\n"
        "    macron:(0x00AF, 0x02C9), mu:(0x00B5, 0x03BC), periodcentered:(0x00B7,\n"
        "    0x2219), scedilla:(0x015F, 0xF6C2), space:(0x0020, 0x00A00),\n"
        "    tcommaaccent:(0x0163, 0x021B).\n"
        "-lic <string> Adds arbitrary string to the end of name ID 3. Adobe uses this\n"
        "    to add a code indicating from which foundry a font is licensed.\n"
        "-stubCmap4 : This causes addfeatures to build only a stub cmap 4 subtable, with\n"
        "    just two segments. Needed only for special cases like AdobeBlank, where\n"
        "    every byte is an issue. Windows requires a cmap format 4 subtable, but\n"
        "    not that it be useful.\n"
        "-omitMacNames : Omit all Mac platform names from the name table. Note: You\n"
        "    cannot specify this and -oldNameID4, otherwise the font would contain\n"
        "    only the PS name as a name ID 4.\n"
        "-overrideMenuNames : Allow feature file name-table entries to override name\n"
        "    IDs that are set via default values or via the FontMenuNameDB file.\n"
        "    Name IDs 2 and 6 cannot be overridden. Use this with caution, and make\n"
        "    sure you have provided feature file name table entries for all platforms.\n"
        "-skco : Suppress kern class optimization by not using the default class 0 for\n"
        "    non-zero left side kern classes. Using the optimization saves hundreds\n"
        "    to thousands of bytes and is the default behavior, but causes kerning to\n"
        "    not be seen by some applications.\n"
        "-mvar : Keep values (not otherwise overwritten) in the MVAR table of the\n"
        "    source font.\n"
        "-V : Show warnings about common, but usually not problematic, issues such as\n"
        "    a glyph having conflicting GDEF classes because it is used in more than\n"
        "    one class type in a layout table. Example: a glyph used as a base in one\n"
        "    replacement rule, and as a mark in another.\n"
        "Build:\n"
        "    addfeatures.lib version: %s \n"
        "    OTF Library Version: %u\n",
        progname,
        ADDFEATURES_VERSION,
        HOT_VERSION);
}

/* Show usage information */
static void showUsage(void) {
    printUsage();
    exit(0);
}

/* Show usage and help information */
static void showHelp(void) {
    printUsage();
    exit(0);
}

extern "C" const char *curdir() {
    return ".";
}

extern "C" const char *sep() {
    return "/";
}

/* Copy directory path with overflow checking */
static void dircpy(char *dst, char *src) {
    if (strlen(src) >= FILENAME_MAX) {
        cbFatal(cbctx, "directory path too long");
    }
    snprintf(dst, FILENAME_MAX, "%s%s", src, sep());
}

/* Convert font */
static void convFont(const char *infile, const char *outfile) {
    cbConvert(cbctx, convert.flags, "addfeatures " ADDFEATURES_VERSION,
              infile, outfile,
              convert.features, convert.hCMap, convert.vCMap, convert.mCMap, convert.uvsFile,
              convert.otherflags, convert.macScript, convert.macLanguage,
              convert.addGlyphWeight, convert.fsSelectionMask_on, convert.fsSelectionMask_off,
              convert.os2_version, convert.licenseID);

    /* Feature file applies only to one font */
    if (convert.features != NULL) {
        convert.features = NULL;
    }
}

/* Parse argument list */
static void parseArgs(int argc, char *argv[], int inScript) {
    int i;
    const char *outputOTFfilename = NULL;
    const char *infile = "font.otf";
    convert.features = NULL;
    convert.maxNumSubrs = 0;
    convert.licenseID = NULL;
    for (i = 0; i < argc; i++) {
        int argsleft = argc - i - 1;
        char *arg = argv[i];
        switch (arg[0]) {
            case '-':
                /* Process regular and disabling options */
                switch (arg[1]) {
                    case 'D': {
                        /* Process list of debug args */
                        int j = 2;
                        do {
                            switch (arg[j]) {
                                case 'f': /* [-Df] Features debug level 1 */
                                    convert.flags |= HOT_DB_FEAT_1;
                                    break;

                                case 'F': /* [-DF] Features debug level 2 */
                                    convert.flags |= HOT_DB_FEAT_2;
                                    break;

                                case 'm': /* [-Dm] Map debug */
                                    convert.flags |= HOT_DB_MAP;
                                    break;

                                default:
                                    cbFatal(cbctx, "unrecognized debug option (%s)", arg);
                            }
                        } while (arg[++j] != '\0');
                    } break;

                    case 'a': /* adding glyphs */
                        if (!strcmp(arg, "-addDSIG")) {
                            convert.otherflags |= OTHERFLAGS_ADD_STUB_DSIG;
                            break;
                        } else {
                            cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }

                        break;

                    case 'b':
                        convert.otherflags |= OTHERFLAGS_ISWINDOWSBOLD;
                        break;

                    case 'c': /* Adobe CMap directory */
                        switch (arg[2]) {
                            case '\0': /* [-c] CMap directory */
                                if (argsleft == 0) {
                                    showUsage();
                                }
                                dircpy(convert.dir.cmap, argv[++i]);
                                break;

                            case 'h': /* [-ch] Horizontal CMap */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.hCMap = argv[++i];
                                break;

                            case 'i': /* [-ci] UVS map */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.uvsFile = argv[++i];
                                break;

                            case 'v': /* [-cv] Vertical CMap */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.vCMap = argv[++i];
                                break;

                            case 'm': /* [-cm] Mac Adobe CMap */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.mCMap = argv[++i];
                                break;

                            case 's': /* [-cs] Mac Adobe CMap script id */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.macScript = atoi(argv[++i]);
                                break;

                            case 'l': /* [-cl] Mac Adobe CMap script id */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.macLanguage = atoi(argv[++i]);
                                break;

                            default:
                                cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'd':
                        switch (arg[2]) {
                            case 'b':
                                if (arg[3] != 'l' || arg[4] != '\0') {
                                    cbFatal(cbctx, "unrecognized option (%s)", arg);
                                } else {
                                    convert.otherflags |= OTHERFLAGS_DOUBLE_MAP_GLYPHS;
                                }
                                break;
                            case 's':
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                cbDesignspaceRead(cbctx, argv[++i]);
                                break;


                            default:
                                cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'f':
                        switch (arg[2]) {
                            case '\0': /* [-f] Process file list */

                                if (argsleft == 0) {
                                    showUsage();
                                } else {
                                    infile = argv[++i];
                                }
                                break;

                            case 'c':
                                /* [-fc] Force ID2 (incorrect) reading of backtrack string in changing context substitution. */
                                convert.otherflags |= OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT;
                                break;

                            case 'd': /* [-fd] Standard feature directory */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                dircpy(convert.dir.feat, argv[++i]);
                                break;

                            case 'f': /* [-ff] Feature file */
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                convert.features = argv[++i];
                                break;

                            case 's':
                                /* [-fs] If there are no GSUB rules, make a stub GSUB table */
                                convert.otherflags |= OTHERFLAGS_ALLOW_STUB_GSUB;
                                break;

                            default:
                                cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'g': /* Glyph name alias database */
                        switch (arg[2]) {
                            case 'f':
                                if (arg[3] != '\0' || argsleft == 0) {
                                    showUsage();
                                }
                                cbAliasDBRead(cbctx, argv[++i]);
                                break;

                            default:
                                cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'i':
                        convert.otherflags |= OTHERFLAGS_ISITALIC;
                        break;

                    case 'l':
                        if (!strcmp(arg, "-lic")) {
                            convert.licenseID = argv[++i];
                        } else if (!strcmp(arg, "-lookupFinal")) {
                            convert.otherflags |= OTHERFLAGS_LOOKUP_FINAL_NAMES;
                        } else {
                            cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'm': /* Font conversion database */
                        if (!strcmp(arg, "-mf")) {
                            if (argsleft == 0)
                                showUsage();
                            cbFCDBRead(cbctx, argv[++i]);
                        } else if (!strcmp(arg, "-mvar")) {
                            convert.otherflags |= OTHERFLAGS_KEEP_MVAR;
                        } else {
                            cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'o': /* OTF filename*/
                        if (arg[2] == '\0') {
                            if (argsleft == 0) {
                                showUsage();
                            }
                            outputOTFfilename = argv[++i];
                            break;
                        } else if (0 == strcmp(arg, "-omitMacNames")) {
                            convert.otherflags |= OTHERFLAGS_OMIT_MAC_NAMES;
                            break;
                        } else if (0 == strcmp(arg, "-overrideMenuNames")) {
                            convert.otherflags |= OTHERFLAGS_OVERRIDE_MENUNAMES;
                            break;
                        } else if (!strcmp(arg, "-omitDSIG")) {
                            convert.otherflags &= ~OTHERFLAGS_ADD_STUB_DSIG;
                            break;
                        }

                        switch (arg[2]) {
                            case 's': {
                                short val;
                                if (0 == strcmp(arg, "-osbOn")) {
                                    val = atoi(argv[++i]);
                                    if ((val < 0) || (val > 15)) {
                                        cbFatal(cbctx, "The bit index value for option (-%s) must be an integer number between 0 and 15 (%d)", arg, val);
                                    }
                                    if (convert.fsSelectionMask_on >= 0) {
                                        convert.fsSelectionMask_on |= 1 << val;
                                    } else {
                                        convert.fsSelectionMask_on = 1 << val;
                                    }
                                } else if (0 == strcmp(arg, "-osbOff")) {
                                    val = atoi(argv[++i]);
                                    if (val < 1) {
                                        cbFatal(cbctx, "The OS/2 table version value for option (-%s) must be an integer number greater than 0 (%d)", arg, val);
                                    }
                                    if (convert.fsSelectionMask_off >= 0) {
                                        convert.fsSelectionMask_off |= 1 << val;
                                    } else {
                                        convert.fsSelectionMask_off = 1 << val;
                                    }
                                } else if (0 == strcmp(arg, "-osv")) {
                                    val = atoi(argv[++i]);
                                    if (val < 1) {
                                        cbFatal(cbctx, "The OS/2 table version value for option (-%s) must be an integer number greater than 0 (%d)", arg, val);
                                    }
                                    convert.os2_version = val;
                                } else {
                                    cbFatal(cbctx, "unrecognized option (%s)", arg);
                                }
                                break;
                            }

                            default:
                                cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 's': /* Process script file */
                        if (!strcmp(arg, "-stubCmap4")) {
                            convert.otherflags |= OTHERFLAGS_STUB_CMAP4;
                        } else if (!strcmp(arg, "-skco")) {
                            convert.otherflags |= OTHERFLAGS_DO_NOT_OPTIMIZE_KERN;
                            break;
                        } else if (!strcmp(arg, "-showFinal")) {
                            convert.otherflags |= OTHERFLAGS_FINAL_NAMES;
                            break;
                        } else {
                            cbFatal(cbctx, "unrecognized option (%s)", arg);
                        }
                        break;

                    case 'u':
                        showUsage();
                        break;

                    case 'V':
                        convert.otherflags |= OTHERFLAGS_VERBOSE;  // controls warnings during feature file processing
                        break;

                    case 't':
                    case 'h':
                        showHelp();
                        break;

                    default:
                        cbFatal(cbctx, "unrecognized option (%s)", arg);
                }
                break;

            default: /* Non-option arg is taken to be filename */
                cbFatal(cbctx, "unrecognized option (%s)", arg);
                break;
        }
    }
    if (!convert.fontDone) {
        convFont(infile, outputOTFfilename);
    }

    convert.fontDone = 1;
}

static ctlMemoryCallbacks cb_dna_memcb;

static void *cb_manage(ctlMemoryCallbacks *cb, void *old, size_t size) {
    void *p = NULL;
    if (size > 0) {
        if (old == NULL) {
            p = sMemNew(size);
            return (p);
        } else {
            p = sMemResize(old, size);
            return (p);
        }
    } else {
        if (old == NULL) {
            return NULL;
        } else {
            free(old);
            return NULL;
        }
    }
}

/* Main program */
extern "C" int main__addfeatures(int argc, char *argv[]) {
    dnaCtx mainDnaCtx = NULL;
    long value;

    /* Extract program name */
    progname = strrchr(argv[0], '/');
    if (progname == NULL) {
        progname = strrchr(argv[0], '\\');
    }
    progname = (progname == NULL) ? argv[0] : progname + 1;
    argc--;
    argv++;

    /* Initialize */
    cb_dna_memcb.ctx = mainDnaCtx;
    cb_dna_memcb.manage = cb_manage;
    mainDnaCtx = dnaNew(&cb_dna_memcb, DNA_CHECK_ARGS);

    cbctx = cbNew(progname, convert.dir.in,
                  convert.dir.out, convert.dir.cmap, convert.dir.feat, mainDnaCtx);

    convert.dir.in[0] = '\0';
    convert.dir.out[0] = '\0';
    convert.dir.cmap[0] = '\0';
    convert.dir.feat[0] = '\0';
    convert.features = NULL;
    convert.hCMap = NULL;
    convert.vCMap = NULL;
    convert.flags = 0;
    convert.otherflags = 0;
    convert.os2_version = 0;
    convert.fsSelectionMask_on = -1;
    convert.fsSelectionMask_off = -1;

    /* Process args. Call convFont at end. */
    parseArgs(argc, argv, 0);

    /* Clean up */
    cbFree(cbctx);

    return 0;
}
