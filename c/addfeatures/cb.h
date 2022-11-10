/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_CB_H_
#define ADDFEATURES_CB_H_

#include <cstddef>

#include "dynarr.h"

typedef struct cbCtx_ *cbCtx;

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CTL_CDECL
#define CTL_CDECL
#endif

/* --- Basic functions --- */
cbCtx cbNew(const char *progname, const char *indir, const char *outdir,
            const char *cmapdir, const char *featdir, dnaCtx mainDnaCtx);
void cbConvert(cbCtx h, int flags, const char *clientVers,
               const char *infile, const char *outfile,
               const char *featurefile, const char *hcmapfile,
               const char *vcmapfile, const char *mcmapfile,
               const char *uvsFile,
               long otherflags, short macScript, short macLanguage,
               long addGlyphWeight,
               short fsSelectionMask_on, short fsSelectionMask_off,
               unsigned short os2Version, const char *licenseID);

void cbFCDBRead(cbCtx h, const char *filename);
void cbAliasDBRead(cbCtx h, const char *filename);
void cbFree(cbCtx h);

/* --- Utility functions --- */
void CTL_CDECL cbFatal(cbCtx h, const char *fmt, ...);
void CTL_CDECL cbWarning(cbCtx h, const char *fmt, ...);
#define OTHERFLAGS_ISWINDOWSBOLD (1 << 0)
#define OTHERFLAGS_ISITALIC (1 << 4)
#define OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT (1 << 5)
#define OTHERFLAGS_DOUBLE_MAP_GLYPHS (1 << 6) /* Provide 2 unicode values for a hard-coded list of glyphs - see agl2uv.h: */
    /* { "Delta",                0x2206 }, */
    /* { "Delta%",               0x0394 }, */
    /* { "Omega",                0x2126 }, */
    /* { "Omega%",               0x03A9 }, */
    /* { "Scedilla",             0x015E }, */
    /* { "Scedilla%",            0xF6C1 }, */
    /* { "Tcommaaccent",         0x0162 }, */
    /* { "Tcommaaccent%",        0x021A }, */
    /* { "fraction",             0x2044 }, */
    /* { "fraction%",            0x2215 }, */
    /* { "hyphen",               0x002D }, */
    /* { "hyphen%",              0x00AD }, */
    /* { "macron",               0x00AF }, */
    /* { "macron%",              0x02C9 }, */
    /* { "mu",                   0x00B5 }, */
    /* { "mu%",                  0x03BC }, */
    /* { "periodcentered",       0x00B7 }, */
    /* { "periodcentered%",      0x2219 }, */
    /* { "scedilla",             0x015F }, */
    /* { "scedilla%",            0xF6C2 }, */
    /* { "space%",               0x00A0 }, */
    /* { "spade",                0x2660 }, */
    /* { "tcommaaccent",         0x0163 }, */
    /* { "tcommaaccent%",        0x021B }, */

#define OTHERFLAGS_ALLOW_STUB_GSUB (1 << 7)
#define OTHERFLAGS_OMIT_MAC_NAMES (1 << 10)
#define OTHERFLAGS_STUB_CMAP4 (1 << 11)
#define OTHERFLAGS_OVERRIDE_MENUNAMES (1 << 12)
#define OTHERFLAGS_DO_NOT_OPTIMIZE_KERN (1 << 13)
#define OTHERFLAGS_ADD_STUB_DSIG (1 << 14)
#define OTHERFLAGS_VERBOSE (1 << 15)
#define OTHERFLAGS_FINAL_NAMES (1 << 16)
#define OTHERFLAGS_LOOKUP_FINAL_NAMES (1 << 17)

#endif  // ADDFEATURES_CB_H_
