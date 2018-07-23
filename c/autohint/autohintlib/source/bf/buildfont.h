/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "basic.h"

#ifndef BUILDFONT_H
#define BUILDFONT_H

#ifdef EXECUTABLE
#define OUTPUTBUFF stdout
#else
#define OUTPUTBUFF stderr
#endif

#define ASCIIFONTNAME            "font.ps"
#define CHARSTRINGSFILENAME      ".bfees.bin"
#define COMPAFMFILE              ".bfcomposites.afm"
#define COMPFILE                 "composites.ps"
#define COMPOSITESFILENAME       ".bfees2.bin"
#define DEFAULTWIDTH             500
#define ENCFILENAME              ".bfencodingfile"
#define FONTBBOXFILENAME         ".bffontbbox"
#define FONTSTKLIMIT             22
                                    /*****************************************/
                                    /* font interpreter stack limit - Note   */
                                    /* that the actual limit is 24, but      */
                                    /* because the # of parameters and       */
                                    /* callothersubr # are also pushed on    */
                                    /* the stack, the effective value is 22. */
                                    /*****************************************/
#define INTEGER 0
#define KERNAFMFILE              "kerning.afm"
#define MAXBYTES                 13000
                                    /*****************************************/
                                    /* max number of bytes in a decrypted    */
                                    /* bez file or encoded charstring        */
                                    /*****************************************/
#define MAXCHARNAME              37
                                    /*****************************************/
                                    /* max character name length (including  */
                                    /* byte for null terminator) - a bug in  */
                                    /* early LW ROMs limits PS names to      */
                                    /* 36 bytes                              */
                                    /*****************************************/
#define MAXCHARS                 400
                                    /*****************************************/
                                    /* max characters in a font (including   */
                                    /* composites)                           */
                                    /*****************************************/
#define MAXENCLINE               65000L
                                    /*****************************************/
                                    /* linelength used when encrypting       */
                                    /*****************************************/
#define MAXFILENAME              32
                                    /*****************************************/
                                    /* max relative file name length         */
                                    /* (including byte for null terminator)  */
                                    /* - limit imposed by max Mac filename   */
                                    /* length - note that max Mac foldername */
                                    /* would be 34 chars because of colons   */
                                    /* and that for root is 29 chars         */
                                    /*****************************************/
#ifndef MAXINT
#define MAXINT                   32767L
#endif
#define MAXLINE                  1000
                                    /*****************************************/
                                    /* maximum length of a line              */
                                    /*****************************************/
#ifndef MININT
#define MININT                   -32768L
#endif
#define MULTIFONTBBOXFILENAME    ".bfmultifontbbox"
#define SCALEDHINTSINFO          "scaledhintsinfo"
#define SUBRSFILENAME            ".bfsubrs.bin"
#define TEMPFILE                 ".bftempfile"
#define UNINITWIDTH              -1          
                                    /*****************************************/
                                    /* width value if no width seen yet      */
                                    /*****************************************/
#define UNSCALEDASCIIFONT        "font.unscaled"
#define WIDTHSFILENAME           "widths.ps"
#define diskfontmax              65000L


/*****************************************************************************/
/* the following are for scanning "PostScript format" input file lines       */
/*****************************************************************************/

#define COMMENT(s)               s[strspn(s, " \n\r\t\v\f")] == '%'
#define BLANK(s)                 sscanf(s, "%*s") == EOF
#define STREQ(a,b)               (((a)[0] == (b)[0]) && (strcmp((a),(b)) == 0))
#define STRNEQ(a,b)              (((a)[0] != (b)[0]) || (strcmp((a),(b)) != 0))

#define NL                       '\012'
                                    /*****************************************/
                                    /* Since the Mac treats both '\n' and    */
                                    /* '\r' as ASCII 13 (i.e. carr. return)  */
                                    /* need to define this.                  */
                                    /*****************************************/

/*****************************************************************************/
/* Defines character bounding box.                                           */
/*****************************************************************************/
typedef struct Bbox
   {
   long int llx, lly, urx, ury;
   } Bbox, *BboxPtr;

/*****************************************************************************/
/* Defines character point coordinates.                                      */
/*****************************************************************************/
typedef struct
   {
   long int x, y;
   } Cd, *CdPtr;



/*****************************************************************************/
/* Defines parsing method based on fontinfo key CharacterSetFileType         */
/*****************************************************************************/
typedef enum
   {
   bf_CHARSET_STANDARD,
   bf_CHARSET_CID
   } CharsetParser;

#define BF_LAYOUTS               "layouts"
#define BF_SUBSETS               "subsets"
#define BF_STD_LAYOUT            "standard"

#define OPENERROR                2
#define OPENWARN                 1
#define OPENOK                   0

extern char bezdir[MAXPATHLEN];
extern boolean scalinghints;

/*****************************************************************************/
/* Tries to open the given file with the access attribute specified.  If it  */
/* fails an error message is printed and the program exits.                  */
/*****************************************************************************/
extern FILE *
ACOpenFile(char *, char *, short);

/*****************************************************************************/
/* Deletes the specified file from the bez directory.                        */
/*****************************************************************************/
extern void
DeleteBezFile(char *, char *);

/*****************************************************************************/
/* Reads successive character and file names from character set file.        */
/*****************************************************************************/
extern char *
ReadNames(char *, char *, long *, long *, FILE *);

/*****************************************************************************/
/* Reads a token from the data buffer.                                       */
/*****************************************************************************/
extern char *
ReadToken(char *, char *, char *, char *);

/*****************************************************************************/
/* Returns whether file has .BAK suffix.                                     */
/*****************************************************************************/
extern boolean
BAKFile(char *);

/*****************************************************************************/
/* Reads the file composites.ps, if it exists, and attempts to create        */
/* composite chars. for those listed.                                        */
/*****************************************************************************/
extern long
make_composites(boolean, boolean, long *, char *, boolean, char **);

/*****************************************************************************/
/* Returns the max number of composites for the font.                        */
/*****************************************************************************/
extern int
readcomposite(boolean, indx);

/*****************************************************************************/
/* Computes the character bounding box.                                      */
/*****************************************************************************/
extern int
computesbandbbx(char *, char *, long, BboxPtr, boolean, long, long);

extern long
GetMaxBytes(void);

extern void
SetMaxBytes(long);

extern CharsetParser
GetCharsetParser();

extern void
SetCharsetLayout(char *fontinfoString);

extern char *
GetCharsetLayout();

/*****************************************************************************/
/* Sets the global variable charsetDir.                                      */
/*****************************************************************************/
extern void
set_charsetdir(char *);

/*****************************************************************************/
/* Sets the global variable matrix.                                          */
/*****************************************************************************/
extern int
set_matrix(char *);

/*****************************************************************************/
/* Sets the global variable uniqueIDFile.                                    */
/*****************************************************************************/
extern void
set_uniqueIDFile(char *);

/*****************************************************************************/
/* Transforms a point using the global variable, matrix.                     */
/*****************************************************************************/
extern int
TransformPoint(long *, long*, boolean);

/*****************************************************************************/
/* Returns the name of the character set file.                               */
/*****************************************************************************/
extern void
getcharsetname(char *);

/*****************************************************************************/
/* Returns the name of the character set file.                               */
/*****************************************************************************/
extern void
setcharsetname(boolean , char *, char *);

/*****************************************************************************/
/* Returns the name of the encoding file.                                    */
/*****************************************************************************/
extern void
get_encodingfilename(char *, boolean, int);

/*****************************************************************************/
/* Make an Adobe PS font or a release font.                                  */
/*****************************************************************************/
extern int
makePSfont(char *, boolean, boolean, boolean, long, long, long);

/*****************************************************************************/
/* Make a Macintosh printer font.                                            */
/*****************************************************************************/
extern int
makelwfn(char *, char *, char);

/*****************************************************************************/
/* Make an IBM printer font.                                                 */
/*****************************************************************************/
extern int
makeibmfont(char *, char *, char);

extern void
GetFontMatrix(char *, char *);

/*****************************************************************************/
/* Returns the total number of input directories.                            */
/*****************************************************************************/
extern short
GetTotalInputDirs(void);

extern void
SetTotalInputDirs(short);

/*****************************************************************************/
/* Parse the private dict portion of an ASCII format font.                   */
/*****************************************************************************/
extern void
ParseFont(FILE *, FILE *, long, long *, long *, long *, long *, boolean);

/*****************************************************************************/
/* Program to derive printer font file name for the mac from the PostScript  */
/* printer font name.                                                        */
/*****************************************************************************/
extern char *
printer_filename(char *);

extern int
convert_PScharfile(const char *, const char *);

/*****************************************************************************/
/* Converts raw PS files to relativized bez format.                          */
/*****************************************************************************/
extern int
convert_rawPSfiles(boolean);

extern void
convert_illcharfile(const char *, const char *);

extern void
convert_illfiles(boolean);

extern long
process_chars(boolean, boolean, boolean, long *, long *, boolean, boolean);

extern void
set_scale(float *);

extern short
strindex(char *, char *);

extern boolean ConvertCharFiles(char *inputDir,	
				boolean release,				
				float scale,				
				void (*convertFunc)(const char *, const char *)	
				);

/*****************************************************************************/
/* Creates an Adobe Font Metrics (AFM) file.                                 */
/*****************************************************************************/
extern int
make_afm(boolean);

extern int
ReadWriteFile(FILE *, char *);

/*****************************************************************************/
/* Looks up a keyword in the fontinfo file and writes the value to the       */
/* given output file.                                                        */
/*****************************************************************************/
extern int
WriteEntry(FILE *, char *, char *, boolean, boolean);

/*****************************************************************************/
/* Returns whether given character is in StandardEncoding.                   */
/*****************************************************************************/
extern boolean
InStandardEncoding(char *, long *);

/*****************************************************************************/
/* Writes copyright notice and trademark information.                        */
/*****************************************************************************/
extern int
WriteCopyrightsAndTrademark(FILE *, char);

/*****************************************************************************/
/* Frees composite table.                                                    */
/*****************************************************************************/
extern void
FreeCompTab(void);

/*****************************************************************************/
/* Deallocates memory and deletes temporary files.                           */
/*****************************************************************************/
extern int
cleanup(short);

extern char *
GetBaseFontPath(boolean);

extern void
FileNameLenOK(char *);

extern void
CharNameLenOK(char *);

extern void
PathNameLenOK(char *);

extern char *
AllocateMem(unsigned int, unsigned int, const char *);

extern char *
ReallocateMem(char *, unsigned int, const char *);

extern void
UnallocateMem(void *ptr);

extern unsigned long
CheckFileBufferLen(char **, char *);

extern void
WriteBlendEntry(FILE *, char *, char *, boolean);

extern int
WriteStart(FILE *, const char *);

/*****************************************************************************/
/* Gets the name of the file containing the current uniqueID.                */
/*****************************************************************************/
extern void
getidfilename(char *);

extern long
ACReadFile(char *, FILE *, char *, long);

extern long
getmaxcomps(void);

extern long
getrealcomps(void);

extern void
get_working_dir(char *);

extern void
init_working_dir(void);

extern void
set_working_dir(void);

extern void
LoadSubsetData(void);

extern void
SetSubsetName(char *name);

extern char *
GetSubsetName(void);

extern char *
GetSubsetPath(void);

extern boolean
UsesSubset(void);

#endif /*BUILDFONT_H*/
