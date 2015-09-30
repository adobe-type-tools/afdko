/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* This module provides a test client for the hot library. hot library clients
   do not need to include this module; it is mearly provided for reference */

#if BUILDLIB
#include "makeotflib.h"
#endif
#include <stdio.h>
#if __CENTERLINE__
#include "dynarr.h"
#include "hotconv.h"
#else

#ifndef PACKAGE_SPECS
#include "package.h"
#else
#include PACKAGE_SPECS
#endif

#include DYNARR
#include HOTCONV

#endif

#include "cb.h"
#include "file.h"

#include "lstdlib.h"
#include "lstdio.h"
#include "lctype.h"
#include "lstring.h"
#define PROFILE 0
#if PROFILE
#include <profiler.h>
#endif

#include "setjmp.h"

jmp_buf mark;

#define MAKEOTF_VERSION "2.5.64775"
/*Warning: this string is now part of heuristic used by CoolType to identify the
   first round of CoolType fonts which had the backtrack sequence of a chaining
   contextual substitution ordered incorrectly.  Fonts with the old ordering MUST match
   the regex:
     "(Version|OTF) 1.+;Core 1\.0\..+;makeotflib1\."
   inside the (1,0,0) nameID 5 "Version: string. */

int KeepGoing = 0;

typedef char bool;

static char *progname;		/* Program name */
static cbCtx cbctx;			/* Client callback context */


/* Conversion data */
static struct {
	struct {					/* Directory paths */
		char pfb[FILENAME_MAX + 1];
		char otf[FILENAME_MAX + 1];
		char cmap[FILENAME_MAX + 1];
		char feat[FILENAME_MAX + 1];
	}
	dir;
	int	fontDone;
	char *features;
	char *hCMap;
	char *vCMap;
	char *mCMap;
	char *uvsFile;
	short macScript;	/* Script and language for Mac cmap. Usu set to HOT_CMAP_UNKNOWN, to use heuristics. */
	short macLanguage;
	short flags;			/* Library flags */
	long otherflags;
	long addGlyphWeight;/* used when adding synthetic glyphs, to override heuristics for determining
						   target weight for use with the FillinMM font.*/
	unsigned long maxNumSubrs;	/* if nto 0, caps the number of subrs used. */
	unsigned short os2_version;	/* OS/2 table version override. Ignore if 0. */
	short fsSelectionMask_on;	/* mask for OR-ing with OS/2 table fsSelection.  */
	short fsSelectionMask_off;	/* mask for NAND-ing with OS/2 table fsSelection. */
	char *licenseID;
} convert;

/* Script data */
static struct {
	char *buf;				/* Input buffer */
	dnaDCL(char *, args);	/* Argument list */
} script;

/* Split font set file into arg list */
static void makeArgs(char *filename) {
	int state;
	long i;
	long length;
	File file;
	char *start = NULL;	/* Suppress optimizer warning */

	/* Read whole file into buffer */
	fileOpen(&file, cbctx, filename, "r");
	fileSeek(&file, 0, SEEK_END);

	length = fileTell(&file);
	script.buf = cbMemNew(cbctx, length + 1);

	fileSeek(&file, 0, SEEK_SET);
	fileReadN(&file, length, script.buf);
	fileClose(&file);

	script.buf[length] = '\n';	/* Ensure termination */

	/* Parse buffer into args */
	state = 0;
	for (i = 0; i < length + 1; i++) {
		int c = script.buf[i];
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
						start = &script.buf[i + 1];
						state = 2;
						break;

					default:
						start = &script.buf[i];
						state = 3;
						break;
				}
				break;

			case 1:	/* Comment */
				if (c == '\n' || c == '\r') {
					state = 0;
				}
				break;

			case 2:	/* Quoted string */
				if (c == '"') {
					script.buf[i] = '\0';	/* Terminate string */
					*dnaNEXT(script.args) = start;
					state = 0;
				}
				break;

			case 3:	/* Space-delimited string */
				if (isspace(c)) {
					script.buf[i] = '\0';	/* Terminate string */
					*dnaNEXT(script.args) = start;
					state = 0;
				}
				break;
		}
	}
}

/* Print usage information */
static void printUsage(void) {
	printf(
		"Usage:\n"
		"    %s [options]\n"
		"Options:\n"
		"-h   : Help\n"
		"-u   : Help\n"
		"-f  <path>  : Specify path to font  file. Default for roman font is 'font.ps' \n"
		"-o  <path>   : Specify output file name for OTF file. Default is <PostScript name> + '.otf'\n"
		"-b : Specify that font has bold style.\n"
		"-i : Specify that font has italic style.\n"
		"-ff  <path>  : Specify path other than default 'features' for feature file\n"
		"-fs  : If there are no GSUB rules, amke a stub GSUB table.\n"
		"-mf  <path>  : Specify path for the Font Menu Name DB file. Required if the '-r' option is used.\n"
		"-gf  <path>  : Specify path for the Glyph Order and Alias DB file Required if the '-r' or '-ga' options are used.\n"
		"               When used without the -ga or -r options, the Unicode assignments will be used, but not the final names.\n"
		"-r   : Release mode:  do NOT put the string 'Development' in the name ID 5 \n"
		"      'Version' string.\n"
		"       Also turns on subroutinization,\n"
		"       and use of the GOADB file to re-order and rename glyphs.\n"
		"-osbOn n   : set bit 'n' to on in the OS/2 table field 'fsSelection'. Can be used more than once to set different bits,\n"
		"			for example '-osbOn 7 -osbOn 8' to set the bits 7 and 8 to true.\n"
		"-osbOff n  : set bit 'n' to off in the OS/2 table field 'fsSelection'. Can be used more than once to set different bits,\n"
		"			for example '-osbOff 7 -osbOff 8' to set the bits 7 and 8 to false.osbOff is always processed after osbOn.\n"
		"-osv n    : Set the OS/2 table version to integer value n.\n"
		"-ga/-nga : Quick mode.  Use/do not use the Glyph Order and Alias DB file to rename and re-order\n"
		"       glyphs. Default is to not do the renaming and re-ordering. If used after -r,\n"
		"       -nga overrides the default -r setting.\n"
		" -gs omit from the font any glyphs that are not in the GOADB file\n"
		"-S/-nS : Turn subroutinzation on/off. If '-ns' is used after -r, it overrides the default -r\n"
		"       setting. Default is no subroutinization, except in release mode.\n"
		"-cm  <path>  : Path to Mac encoding CMAP file : used only for CID fonts.\n"
		"-cs  <integer>  : Integer value for MAc cmap script ID: -1 means undedfined. : used only for CID fonts.\n"
		"-cl  <integer>  : Integer value for MAc cmap language ID: -1 means undedfined.: used only for CID fonts.\n"
		"-ch  <path>  : Path to Unicode horizontal CMAP file : used only for CID fonts\n"
		"-cv  <path>  : Path to Unicode vertical CMAP file : used only for CID fonts\n"
		"-ci  <path>  : Path to Unicode Variation Sequences specification file : used only for CID fonts\n"
		"-addn  : Override any .notdef in the font with a  synthetic marking notdef.\n"
		"-adds [integer]  : If the source font is missing any glyphs from a std set, add them,\n"
		"               using an MM font to match weight and width.If an integer value follows,\n"
		"               it is osed as the design weight for the  the synthetic glyphs.\n"
		"-serif/-sans   : Force using serif/sans synthetic glyphs, rather than choice derived by heuristics.\n"
		"-dbl          : for a short list of glyphs, double map them to two Unicode values if there is no other glyph\n"
		"              mapped to the same Unicode value. This used to be default behaviour. The glyph list is:\n"
		"              Delta:(0x2206, 0x0394), Omega:(0x2126, 0x03A9), Scedilla:(0x015E, 0xF6C1), Tcommaaccent:(0x0162, 0x021A),\n"
		"              fraction:(0x2044, 0x2215), hyphen:(0x002D, 0x00AD), macron:(0x00AF, 0x02C9), mu:(0x00B5, 0x03BC),\n"
		"              periodcentered:(0x00B7, 0x2219), scedilla:(0x015F, 0xF6C2), space:(0x0020, 0x00A00),\n"
		"              tcommaaccent:(0x0163, 0x021B).\n"
		"-dcs         : Set  OS/2.DefaultChar to the Unicode value for 'space', rather than '.notdef.\n"
		"               The latter is correct by the OT spec, but QuarkXPress 6.5 requires the former in order to print OTF/CFF fonts.\n"
		"-oldNameID4     : Set Windows name ID 4 to the PS name, and Mac name ID 4 to Preferred Family + space + Preferred Style name.\n"
		"              Required because  this is how all FDK fonts were done before 11/2010, and changing the name ID 4 from previously\n"
		"              shipped versions causes major installation problems under Windows. New fonts should be built without this option.\n"
		"-lic <string>  Adds arbitrary string to the end of name ID 3. Adobe uses this to add a code indicating from which foundry a font is licensed.\n"
       "-shw       : suppress warnings about missing or problematic hints. \n"
       "               This is useful when processsing a temporary font made from a TTF or other no PS font.\n"
       "-stubCmap4 : This causes makeotf to build only a stub cmap 4 subtable, with just two segments. Needed only for special cases like\n"
       "AdobeBlank, where every byte is an issue. Windows requires a cmap format 4 subtable, but not that it be useful.\n"
       "-swo       : suppress width optimization in CFF (use of defaultWidthX and nominalWidthX). Makes it easier to poke at charstrings with other tools.\n"
       "-omitMacNames   : Omit all Mac platform names from the name table. Note: You cannot specify this and -oldNameID4; else\n"
       "              font would contain only teh PS name as a name ID 4.\n"
       "-overrideMenuNames   : Allow feature file name table entries to override default values and the values from the font manu name DB for name IDs. \n"
       "  Name ID's 2 and 6 cannot be overridden. Use this with caution, and make sure you have provided feature file name table entries for all platforms.\n"
       "-skco  :  suppress kern class optimization. Do not use the default class 0 for non-zero left side kern classes. Using the optimization saves hundreds to thousands of\n"
       "bytes and is the default behavior, but causes kerning to be not seen by some programs.\n"
/* Always do +z now. "+z   : Remove the deprecated Type 1 operators seac and dot section from the output front\n" */
		""
		"Build:\n"
		"    makeotf.lib version: %s \n"
		"    OTF Library Version: %u\n",
		progname,
		MAKEOTF_VERSION,
		HOT_VERSION);
	/*
	 * The intent here is that the user should be able to find out
	 * "Which hotconv am I using?".
	 * In order for the message to reflect the last *link*, the makefile must
	 * arrange to recompile main.c on every link, whether main.c is out of
	 * date or not.
	 *
	 * Since support for the required ANSI-standard macros from any
	 * given compiler seems unpredictable, we test that each is defined.	*/
#ifdef BUILDDIR
/* Stringizing macros (C Programming FAQs, section 11.17) */
#define STR(x)	#x
#define XSTR(x)	STR(x)
	printf("    Source:  %s\n", XSTR(BUILDDIR));
#undef STR
#undef XSTR
#endif	/* BUILDDIR */
#ifdef __DATE__
	printf("    Date:    %s\n", __DATE__);
#endif
#ifdef __TIME__
	printf("    Time:    %s\n", __TIME__);
#endif
}

/* Show usage information */
static void showUsage(void) {
	printUsage();
	exit(1);
}

/* Show usage and help information */
static void showHelp(void) {
	printUsage();
	exit(1);
}

extern char *sep();

/* Copy directory path with overflow checking */
static void dircpy(char *dst, char *src) {
	if (strlen(src) >= FILENAME_MAX) {
		cbFatal(cbctx, "directory path too long");
	}
	sprintf(dst, "%s%s", src, sep());
}

/* Convert font */
static void convFont(char *pfbfile,  char *otffile) {
	cbConvert(cbctx, convert.flags,
			  (convert.otherflags & OTHERFLAGS_RELEASEMODE) ?
			  "makeotf.lib"MAKEOTF_VERSION :
			  "makeotf.lib"MAKEOTF_VERSION" DEVELOPMENT" ,
			  pfbfile, otffile,
			  convert.features, convert.hCMap, convert.vCMap, convert.mCMap, convert.uvsFile,
			  convert.otherflags, convert.macScript, convert.macLanguage,
			  convert.addGlyphWeight, convert.maxNumSubrs, convert.fsSelectionMask_on, convert.fsSelectionMask_off,
			  convert.os2_version, convert.licenseID);

	/* Feature file applies only to one font */
	if (convert.features != NULL) {
		convert.features = NULL;
	}
}

/* Parse argument list */
static void parseArgs(int argc, char *argv[], int inScript) {
	int i;
	char *outputOTFfilename = NULL;
	char *pfbfile = "font.ps";
	convert.features = NULL;
	convert.maxNumSubrs = 0;
	convert.flags |= HOT_NO_OLD_OPS;	/* always remove old ops */
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
								case 'a':	/* [-Da] AFM debug */
									convert.flags |= HOT_DB_AFM;
									break;

								case 'f':	/* [-Df] Features debug level 1 */
									convert.flags |= HOT_DB_FEAT_1;
									break;

								case 'F':	/* [-DF] Features debug level 2 */
									convert.flags |= HOT_DB_FEAT_2;
									break;

								case 'm':	/* [-Dm] Map debug */
									convert.flags |= HOT_DB_MAP;
									break;

								default:
									cbFatal(cbctx, "unrecognized debug option (%s)", arg);
							}
						} while (arg[++j] != '\0');
					}
					break;

					case 'a':	/* adding glyphs */
						if (!strcmp(arg, "-addn")) {
							convert.flags |= HOT_FORCE_NOTDEF;
						}
						else if (!strcmp(arg, "-adds")) {
							convert.flags |= HOT_ADD_EURO;
							convert.addGlyphWeight = 0;
							if (argsleft > 0) {
								int value =  atoi(argv[i]);
								if ((value != 0) || (!strcmp(arg, "0"))) {
									convert.addGlyphWeight = value;
									i++;
								}
							}
						}
						else {
							cbFatal(cbctx, "unrecognized option (%s)", arg);
						}

						break;

					case 'b':
						convert.otherflags |= OTHERFLAGS_ISWINDOWSBOLD;
						break;

					case 'c':	/* Adobe CMap directory */
						switch (arg[2]) {
							case '\0':	/* [-c] CMap directory */
								if (argsleft == 0) {
									showUsage();
								}
								dircpy(convert.dir.cmap, argv[++i]);
								break;

							case 'h':	/* [-ch] Horizontal CMap */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								convert.hCMap = argv[++i];
								break;

							case 'i':	/* [-ci] UVS map */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								convert.uvsFile = argv[++i];
								break;

							case 'v':	/* [-cv] Vertical CMap */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								convert.vCMap = argv[++i];
								break;

							case 'm':	/* [-cm] Mac Adobe CMap */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								convert.mCMap = argv[++i];
								break;

							case 's':	/* [-cs] Mac Adobe CMap script id */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								convert.macScript = atoi(argv[++i]);
								break;

							case 'l':	/* [-cl] Mac Adobe CMap script id */
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
								if (arg[3] != 'l' ||  arg[4] != '\0') {
									cbFatal(cbctx, "unrecognized option (%s)", arg);
								}
								else {
									convert.otherflags |= OTHERFLAGS_DOUBLE_MAP_GLYPHS;
								}
								break;

							case 'c':
								if (arg[3] != 's' ||  arg[4] != '\0') {
									cbFatal(cbctx, "unrecognized option (%s)", arg);
								}
								else {
									convert.otherflags |= OTHERFLAGS_OLD_SPACE_DEFAULT_CHAR;
								}
								break;

							default:
								cbFatal(cbctx, "unrecognized option (%s)", arg);
						}
						break;

					case 'f':
						switch (arg[2]) {
							case '\0':	/* [-f] Process file list */

								if (argsleft == 0) {
									showUsage();
								}
								else {
									pfbfile = argv[++i];
								}
								break;

							case 'c':
								/* [-fc] Force ID2 (incorrect) reading of backtrack string in changing context substitution. */
								convert.otherflags |= OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT;
								break;

							case 'd':	/* [-fd] Standard feature directory */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								dircpy(convert.dir.feat, argv[++i]);
								break;

							case 'f':	/* [-ff] Feature file */
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

					case 'g':				/* Glyph name alias database */
						switch (arg[2]) {
							case 'f':	/* [-c] CMap directory */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								cbAliasDBRead(cbctx, argv[++i]);
								break;

							case 'a':
								convert.flags |= HOT_RENAME;
								break;

							case 's':
								convert.flags |= HOT_SUBSET;
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
						}
						else {
							cbFatal(cbctx, "unrecognized option (%s)", arg);
						}
						break;

					case 'm':	/* Font conversion database */
						switch (arg[2]) {
							case 'f':	/* [-c] CMap directory */
								if (arg[3] != '\0' || argsleft == 0) {
									showUsage();
								}
								cbFCDBRead(cbctx, argv[++i]);
								break;

							case 'a':
								if ((arg[3] != 'x') || (arg[4] != 's') || (arg[5] != '\0') || argsleft == 0) {
									showUsage();
								}
								convert.maxNumSubrs = atoi(argv[++i]);
								break;

							default:
								cbFatal(cbctx, "unrecognized option (%s)", arg);
						}
						break;

					case 'o':		/* OTF filename*/
						if (arg[2] == '\0') {
							if (argsleft == 0) {
								showUsage();
							}
							outputOTFfilename = argv[++i];
							break;
						}

						else if (0 == strcmp(arg, "-oldNameID4")) {
							convert.otherflags |= OTHERFLAGS_OLD_NAMEID4;
                            if (convert.otherflags & OTHERFLAGS_OMIT_MAC_NAMES)
                                cbFatal(cbctx, "You cannot specify both -omitMacNames and -oldNameID4.");
							break;
						}


						else if (0 == strcmp(arg, "-omitMacNames")) {
							convert.otherflags |= OTHERFLAGS_OMIT_MAC_NAMES;
                            if (convert.otherflags & OTHERFLAGS_OLD_NAMEID4)
                                cbFatal(cbctx, "You cannot specify both -omitMacNames and -oldNameID4.");
							break;
						}
                        
						else if (0 == strcmp(arg, "-overrideMenuNames")) {
							convert.otherflags |= OTHERFLAGS_OVERRIDE_MENUNAMES;
							break;
						}
                        
                        
						switch (arg[2]) {
							case 's': {
								short val;
								if (0 == strcmp(arg, "-osbOn")) {
									val = atoi(argv[++i]);
									if ((val < 0) || (val > 15)) {
										cbFatal(cbctx, "The bit index value for option (-%s) must be an integer number between 0 and 15 (%s)", arg);
									}
									if (convert.fsSelectionMask_on >= 0) {
										convert.fsSelectionMask_on |= 1 << val;
									}
									else {
										convert.fsSelectionMask_on = 1 << val;
									}
								}
								else if (0 == strcmp(arg, "-osbOff")) {
									val = atoi(argv[++i]);
									if (val < 1) {
										cbFatal(cbctx, "The OS/2 table version value for option (-%s) must be an integer number greater than 0 (%s)", arg);
									}
									if (convert.fsSelectionMask_off >= 0) {
										convert.fsSelectionMask_off |= 1 << val;
									}
									else {
										convert.fsSelectionMask_off = 1 << val;
									}
								}
								else if (0 == strcmp(arg, "-osv")) {
									val = atoi(argv[++i]);
									if (val < 1) {
										cbFatal(cbctx, "The OS/2 table version value for option (-%s) must be an integer number greater than 0 (%s)", arg);
									}
									convert.os2_version = val;
								}
								else {
									cbFatal(cbctx, "unrecognized option (%s)", arg);
								}
								break;
							}

							default:
								cbFatal(cbctx, "unrecognized option (%s)", arg);
						}
						break;

					case 'n':	/* all the 'off' settings */
						switch (arg[2]) {
							case 'g': {
								if (arg[4] != '\0') {
									showUsage();
								}
								if (arg[3] == 'a') {
									convert.flags &= ~HOT_RENAME;
									/* just in case the GOADB has already been read in */
									cbAliasDBCancel(cbctx);
								}
								if (arg[3] == 's') {
									convert.flags &= ~HOT_SUBSET;
									/* just in case the GOADB has already been read in */
								}

								break;
							}

							case 'S':
								if (arg[3] != '\0') {
									showUsage();
								}
								convert.flags &= ~HOT_SUBRIZE;
								break;

							default:
								cbFatal(cbctx, "unrecognized option (%s)", arg);
						}
						break;

					case 's':	/* Process script file */
						if (!strcmp(arg, "-serif")) {
							convert.flags &= ~HOT_IS_SANSSERIF;
							convert.flags |= HOT_IS_SERIF;
						}
						else if (!strcmp(arg, "-sans")) {
							convert.flags |= HOT_IS_SANSSERIF;
							convert.flags &= ~HOT_IS_SERIF;
						}
						else if (!strcmp(arg, "-shw")) {
							convert.flags |= HOT_SUPRESS_HINT_WARNINGS;
						}
						else if (!strcmp(arg, "-swo")) {
							convert.flags |= HOT_SUPRESS__WIDTH_OPT;
						}
                        
						else if (!strcmp(arg, "-stubCmap4")) {
							convert.otherflags |= OTHERFLAGS_STUB_CMAP4;
						}
						else if (!strcmp(arg, "-skco")) {
							convert.otherflags |= OTHERFLAGS_DO_NOT_OPTIMIZE_KERN;
							break;
						}
						else {
							/* Process script file */
							if (arg[2] != '\0' || argsleft == 0) {
								showUsage();
							}
							if (inScript) {
								cbFatal(cbctx, "can't nest scripts");
							}
							if (script.buf != NULL) {
								cbFatal(cbctx, "can't have multiple scripts");
							}
							makeArgs(argv[++i]);
							parseArgs(script.args.cnt, script.args.array, 1);
						}
						break;

/*	This is left over from when Morisawa was demanding font protection mechanims.
            case 'A':
                convert.flags |= HOT_ADD_AUTH_AREA;
                break;
            case 'a':
                convert.flags &= ~HOT_ADD_AUTH_AREA;
                break;
 */
					case 'r':
						convert.otherflags |= OTHERFLAGS_RELEASEMODE;
						convert.flags |= HOT_RENAME;
						convert.flags |= HOT_SUBRIZE;
						break;

/* No longer supported - not used enough
            case 'K':
                KeepGoing = 1;
                break;
 */
					case 'S':
						convert.flags |= HOT_SUBRIZE;
						break;

/* I now always set this flag; see start of this function.
            case 'z':
                convert.flags &= ~HOT_NO_OLD_OPS;
                break;
 */
					case 'u':
						showUsage();
						break;

					case 't':
					case 'h':
						showHelp();
						break;

					default:
						cbFatal(cbctx, "unrecognized option (%s)", arg);
				}
				break;

			case '+':
				/* Process enabling options */
				switch (arg[1]) {
/*	This is left over from when Morisawa was demanding font protection mechanims.
        case 'a':
                convert.flags |= HOT_ADD_AUTH_AREA;
                break;
 */
/* I now always remove old ops, and don't bother with warning.
            case 'z':
                convert.flags |= HOT_NO_OLD_OPS;
                break;
 */
					default:
						cbFatal(cbctx, "unrecognized option (%s)", arg);
				}
				break;

			default:			/* Non-option arg is taken to be filename */
				cbFatal(cbctx, "unrecognized option (%s)", arg);
				break;
		}
	}
	if (!convert.fontDone) {
		convFont(pfbfile, outputOTFfilename);
	}

	convert.fontDone = 1;
}

/*Used to parse the parameter string passed in by python
 * start is the start index and is updated to the beginning of the next substring
 */

static char *NextToken(char *str, int *start) {
	char *token;
	int end, current;

	if (str[*start] == '\0') {
		return NULL;
	}

	if (str[*start] == '\"') {
		for (end = *start + 1; str[end] != '\"' && str[end] != '\0'; end++);

		token = (char *)malloc((end - *start + 1) * sizeof(char));
		for (current = *start + 1; current < end; current++) {
			token[current - *start - 1] = str[current];
		}
		token[current - *start - 1] = '\0';

		if (str[end] == '\"') {
			end++;
		}
		while (str[end] == ' ') {
			end++;
		}
		*start = end;
	}
	else {
		for (end = *start; str[end] != ' ' && str[end] != '\0'; end++);

		token = (char *)malloc((end - *start + 1) * sizeof(char));
		for (current = *start; current < end; current++) {
			token[current - *start] = str[current];
		}
		token[current - *start] = '\0';

		while (str[end] == ' ') {
			end++;
		}
		*start = end;
	}
	return token;
}

/* The framework for the interface between python and hot_unix
 * See makeotfutil.py for content of each field.
 */

#define MAX_ARGS 100


#define MDEBUG 0

static ctlMemoryCallbacks cb_dna_memcb;

static void *cb_manage(ctlMemoryCallbacks *cb, void *old, size_t size) {
	void *p = NULL;
	if (size > 0) {
		if (old == NULL) {
			p = malloc(size);
			return (p);
		}
		else {
			p = realloc(old, size);
			return (p);
		}
	}
	else {
		if (old == NULL) {
			return NULL;
		}
		else {
			free(old);
			return NULL;
		}
	}
}

/* Main program */
int main(int argc, char *argv[]) {
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
	value = setjmp(mark);
	if (value == -1) {
		return 1;
	}


	/* Initialize */
	cb_dna_memcb.ctx = mainDnaCtx;
	cb_dna_memcb.manage = cb_manage;
	mainDnaCtx = dnaNew(&cb_dna_memcb, DNA_CHECK_ARGS);

	cbctx = cbNew(progname, convert.dir.pfb,
				  convert.dir.otf, convert.dir.cmap, convert.dir.feat, mainDnaCtx);

	script.buf = NULL;
	dnaINIT(mainDnaCtx, script.args, 100, 500);

	convert.dir.pfb[0] = '\0';
	convert.dir.otf[0] = '\0';
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

	fprintf(stderr, "\n");			/* Terminate progress line */

	/* Clean up */
	cbMemFree(cbctx, script.buf);
	dnaFREE(script.args);
	cbFree(cbctx);

	return 0;
}
