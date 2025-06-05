/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "Dglobal.h"
#include "Dsfnt.h"
#include "sfnt_sfnt.h"
#include "opt.h"  // informal sharing with spot
#include "da.h"
#include "sfile.h"
#include "smem.h"
#include "slogger.h"

extern char *FDK_VERSION;

static const char *progname = "sfntdiff";

#define MAX_ARGS 200

/* Print usage information */
static void printUsage(void) {
    printf(
        "Usage: %s [-u|-h] [-T] [-d <level>] [-x<tags>|-i<tags>] "
        "<FONTS|DIRS>\n"
        "OR: %s  -X <scriptfile>\n\n"
        "where: <FONTS|DIRS> is:\n"
        "\t    <fontfile1> <fontfile2>\n"
        "\tOR: <fontfile> <otherfontdir>\n"
        "\tOR: <fontdir1> <fontdir2>\n "
        "\nOptions:\n"
        "    -u  print usage information\n"
        "    -h  print usage and help information\n"
        "    -T  show time-stamp of font files\n"
        "    -d  set diff level of detail\n"
        "    -x  exclude table(s)   _OR_\n"
        "    -i  include table(s) e.g., -iname,head\n"
        "Version:\n"
        "    %s\n",
        progname,
        progname,
        FDK_VERSION);
}

/* Show usage information */
static void showUsage(void) {
    printUsage();
}

/* Show usage and help information */
static void showHelp(void) {
    printUsage();
    printf(
        "Notes:\n"
        "    -d <level>  : level of detail in indicating differences\n"
        "        0: (default) indicate if there is a difference\n"
        "            in a particular table\n"
        "        1: indicate the byte-position within the table where\n"
        "            a difference begins\n"
        "        2: dump the tables in a Hex format and produce\n"
        "            a line-by-line diff\n"
        "        3: if the contents of the table is amenable, compare\n"
        "            the data-structures specific to that table, and\n"
        "            produce the differences in the fields of the\n"
        "            data-structures of the two files\n"
        "        4: a possibly more-verbose version of level 3\n");
    printf(
        "    -x <tag>[,<tag>]*\n"
        "        exclude one or more tables from comparisons.\n"
        "        e.g., -x cmap,name\n"
        "        will skip inspecting/comparing the 'cmap' and 'name' tables\n");
    printf(
        "    -i <tag>[,<tag>]*\n"
        "        include one or more tables for comparisons.\n"
        "        e.g., -i cmap,name\n"
        "        will inspect/compare ONLY the 'cmap' and 'name' tables\n");
    printf("    -i and -x switches are exclusive of each other.\n");
}

/* Main program */
int main__sfntdiff(int argc, char *argv[]) {
    static opt_Option opt[] = {
            {"-u", opt_Call, showUsage},
            {"-h", opt_Call, showHelp},
            {"-T", opt_Flag},
            {"-x", sdSfntTagScan},
            {"-i", sdSfntTagScan},
            {"-d", opt_Int, &level, "0", 0, 4},
    };

    int argi;
    bool supported, supported2;
    char *filename1;
    char *filename2;
    int name1isDir, name2isDir;
    char **SimpleNameList;
    int NumSimpleNames = 0;

    da_SetMemFuncs(sMemNew, sMemResize, sMemFree);

    argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);

    if (opt_Present("-u") || opt_Present("-h")) return 0;

    if (opt_Present("-x") && opt_Present("-i")) {
        printf("ERROR: '-x' switch and '-i' switch are exclusive of each other.\n");
        showUsage();
        return 1;
    }

    if (level > 4) level = 4;

    if ((argc - argi) < 2) {
        printf("ERROR: not enough files/directories specified.\n");
        showUsage();
        return 1;
    }

    filename1 = argv[argi];
    filename2 = argv[argi + 1];

    name1isDir = sFileIsDir(filename1);
    name2isDir = sFileIsDir(filename2);

    printf("%s\n", ourtime());
    printf("%s (%s) (-d %d)  files:\n", progname, FDK_VERSION, level);

    if (!name1isDir && !name2isDir) {
        if (!sdFileIsOpened(1)) sdFileOpen(1, filename1);
        if (!sdFileIsOpened(2)) sdFileOpen(2, filename2);

        if (opt_Present("-T")) {
            printf("< %s\t%s\n", filename1, fileModTimeString(1, filename1));
            printf("> %s\t%s\n", filename2, fileModTimeString(2, filename2));
        } else {
            printf("< %s\n", filename1);
            printf("> %s\n", filename2);
        }

        /* See if we can recognize the file type */
        supported = isSupportedFontFormat(sdFileSniff(1), filename1);
        supported2 = isSupportedFontFormat(sdFileSniff(2), filename2);

        if (!supported || !supported2) {
            sdFileClose(1);
            sdFileClose(2);
            exit(1);
        }

        sdSfntRead(0, -1, 0, -1); /* Read plain sfnt file */
        sdSfntDump();
        sdSfntFree();
        sdFileClose(1);
        sdFileClose(2);
    } else if (name1isDir && name2isDir) {
        int8_t fil1[MAX_PATH];
        int8_t fil2[MAX_PATH];
        int nn;

        NumSimpleNames = sFileReadInputDir(filename1, &SimpleNameList);
        for (nn = 0; nn < NumSimpleNames; nn++) {
            STRCPY_S(fil1, sizeof(fil1), filename1);
            STRCAT_S(fil1, sizeof(fil1), "/");
            STRCAT_S(fil1, sizeof(fil1), SimpleNameList[nn]);

            STRCPY_S(fil2, sizeof(fil2), filename2);
            STRCAT_S(fil2, sizeof(fil2), "/");
            STRCAT_S(fil2, sizeof(fil2), SimpleNameList[nn]);

            sdFileOpen(1, fil1);
            sdFileOpen(2, fil2);

            printf("\n---------------------------------------------\n");
            if (opt_Present("-T")) {
                printf("< %s\t%s\n", fil1, fileModTimeString(1, fil1));
                printf("> %s\t%s\n", fil2, fileModTimeString(2, fil2));
            } else {
                printf("< %s\n", fil1);
                printf("> %s\n", fil2);
            }

            /* See if we can recognize the file type */
            supported = isSupportedFontFormat(sdFileSniff(1), fil1);
            supported2 = isSupportedFontFormat(sdFileSniff(2), fil2);

            if (!supported || !supported2) {
                sdFileClose(1);
                sdFileClose(2);
                continue;
            }

            sdSfntRead(0, -1, 0, -1); /* Read plain sfnt file */
            sdSfntDump();
            sdSfntFree();
            sdFileClose(1);
            sdFileClose(2);
        }
    } else if (!name1isDir && name2isDir) {
        int8_t fil2[MAX_PATH];
        int8_t *c;

        STRCPY_S(fil2, sizeof(fil2), filename2);
        STRCAT_S(fil2, sizeof(fil2), "/");
        c = strrchr(filename1, '/');
        if (c == NULL)
            STRCAT_S(fil2, sizeof(fil2), filename1);
        else
            STRCAT_S(fil2, sizeof(fil2), ++c);

        sdFileOpen(1, filename1);
        sdFileOpen(2, fil2);

        if (opt_Present("-T")) {
            printf("< %s\t%s\n", filename1, fileModTimeString(1, filename1));
            printf("> %s\t%s\n", fil2, fileModTimeString(2, fil2));
        } else {
            printf("< %s\n", filename1);
            printf("> %s\n", fil2);
        }

        /* See if we can recognize the file type */
        supported = isSupportedFontFormat(sdFileSniff(1), filename1);
        supported2 = isSupportedFontFormat(sdFileSniff(2), fil2);

        if (!supported || !supported2) {
            sdFileClose(1);
            sdFileClose(2);
            exit(1);
        }

        sdSfntRead(0, -1, 0, -1); /* Read plain sfnt file */
        sdSfntDump();
        sdSfntFree();
        sdFileClose(1);
        sdFileClose(2);
    } else {
        printf("ERROR: Incorrect/insufficient files/directories specified.\n");
        showUsage();
        return 1;
    }
    return 0;
}
