/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef CB_H
#define CB_H

#include <stddef.h>
#include "dynarr.h"
typedef struct cbCtx_ *cbCtx;

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CDECL
#define CDECL
#endif

/* --- Basic functions --- */
cbCtx cbNew(char *progname, char *pfbdir, char *otfdir,
			char *cmapdir, char *featdir, dnaCtx mainDnaCtx);
void cbConvert(cbCtx h, int flags, char *clientVers,
			   char *pfbfile, char *otffile,
			   char *featurefile, char *hcmapfile, char *vcmapfile, char *mcmapfile,  char * uvsFile,
			   long otherflags, short macScript, short macLanguage,
			   long addGlyphWeight, unsigned long maxNumSubrs,  short fsSelectionMask_on, short fsSelectionMask_off, unsigned short os2Version, char * licenseID);
			   
void cbFCDBRead(cbCtx h, char *filename);
void cbAliasDBRead(cbCtx h, char *filename);
void cbAliasDBCancel(cbCtx h);
void cbFree(cbCtx h);

/* --- Utility functions --- */
void CDECL cbFatal(cbCtx h, char *fmt, ...);
void CDECL cbWarning(cbCtx h, char *fmt, ...);
void *cbMemNew(cbCtx h, size_t size);
void *cbMemResize(cbCtx h, void *old, size_t size);
void cbMemFree(cbCtx h, void *ptr);
void myfatal(void *ctx);
void message(void *ctx, int type, char *text);
#define OTHERFLAGS_ISWINDOWSBOLD (1<<0)
#define OTHERFLAGS_HASBULLET     (1<<1)

#define OTHERFLAGS_RELEASEMODE  (1<<2)
#define OTHERFLAGS_ISITALIC     (1<<4)
#define OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT (1<<5)
#define OTHERFLAGS_DOUBLE_MAP_GLYPHS (1<<6)  /* Provide 2 unicode values for a hard-coded list of glyphs - see agl2uv.h:

									{ "Delta",                0x2206 },
									{ "Delta%",               0x0394 },
									{ "Omega",                0x2126 },
									{ "Omega%",               0x03A9 },
									{ "Scedilla",             0x015E },
									{ "Scedilla%",            0xF6C1 },
									{ "Tcommaaccent",         0x0162 },
									{ "Tcommaaccent%",        0x021A },
									{ "fraction",             0x2044 },
									{ "fraction%",            0x2215 },
									{ "hyphen",               0x002D },
									{ "hyphen%",              0x00AD },
									{ "macron",               0x00AF },
									{ "macron%",              0x02C9 },
									{ "mu",                   0x00B5 },
									{ "mu%",                  0x03BC },
									{ "periodcentered",       0x00B7 },
									{ "periodcentered%",      0x2219 },
									{ "scedilla",             0x015F },
									{ "scedilla%",            0xF6C2 },
									{ "space%",               0x00A0 },
									{ "spade",                0x2660 },
									{ "tcommaaccent",         0x0163 },
									{ "tcommaaccent%",        0x021B },
										*/

#define OTHERFLAGS_ALLOW_STUB_GSUB (1<<7)
#define OTHERFLAGS_OLD_SPACE_DEFAULT_CHAR  (1<<8)
#define OTHERFLAGS_OLD_NAMEID4  (1<<9)
#define OTHERFLAGS_OMIT_MAC_NAMES (1<<10)
#define OTHERFLAGS_STUB_CMAP4 (1<< 11)
#define OTHERFLAGS_OVERRIDE_MENUNAMES (1<<12)
#define OTHERFLAGS_DO_NOT_OPTIMIZE_KERN (1<<13)
#define OTHERFLAGS_ADD_STUB_DSIG (1<<14)

#endif /* CB_H */
