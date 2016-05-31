/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef HOT_H
#define HOT_H

#include <stddef.h>             /* For size_t */

#ifdef __cplusplus
extern "C" {
#endif

#define HOT_VERSION 0x0100633 /* Library version (1.0.93) */
/*Warning: this string is now part of heuristic used by CoolType to identify the
first round of CoolType fonts which had the backtrack sequence of a chaining 
contextual substitution ordered incorrectly.  Fonts with the old ordering MUST match
the regex:
	 "(Version|OTF) 1.+;Core 1\.0\..+;makeotflib1\."
inside the (1,0,0) nameID 5 "Version: string. */

/***********************************************************************/
/* Note: multiple master Type 1 is not supported by this version of    */
/* the library.                                                        */
/***********************************************************************/

/* Hatch OpenType (HOT) Font Conversion Library
   ============================================

   Overview
   --------
   This library converts platform independent font data (PostScript outline
   font, menu names, metric information, etc) into an OpenType Format (OTF)
   font. Outline font data must be stripped of its platform-specific formatting
   bytes which involves the removal of segment headers from Windows PFB fonts
   or unpacking of POST resources from Macintosh FFIL files. (The removal of
   eexec and charstring encryption is handled internally by the library.) The
   library supports single master, multiple master, synthetic, and CID-keyed
   fonts by auto-detection, obviating the need for the client to specify the
   font type prior to conversion.

   This library is initialized with a single call to hotNew() which allocates a
   opaque context (hotCtx) which is passed to subsequent functions and is
   destroyed by a single call to hotFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client must call hotReadFont() to read
   the PostScript outline font. This function returns information about the
   font including: FontName, font type: single master, multiple master, or CID,
   and font encoding. The FontName may be used for OTF filename generation. The
   font type may be used to determine which metric file should be subsequently
   read, e.g. choose .MMM instead of .PFM for multiple master fonts on the
   Windows platform. The encoding may be used to decide which encoding to use
   for decoding the kerning data.

   Next, the client must provide information that does not reside in the
   outline font file. This would normally be derived from a metrics file or
   resource. Menu name information is added first by calling hotAddName(), then
   miscellaneous data by hotAddMiscData(). Subsequently, optional kerning data
   may be added by calls to hotAddKernPair(), hotAddKernValue(), and
   hotAddUnencChar().

   If the client wishes to add tables to the OTF file which are not supported
   directly by the hot library, they may be added at this step by calling
   hotAddAnonTable() for each table to be added. If the specifications for the
   anonymous tables are in the feature file (which is read in by the library
   during hotConvert()), then calls to hotAddAnonTable() may also be made
   within the featAddAnonData() callbacks. See the documentation for the
   featAddAnonData() callback below for further details.

   When all additional information has been provided to the library the
   conversion is initiated by a single call to hotConvert(). Thus, a typical
   single master conversion follows the following call sequence.

   ctx = hotNew(...);
   hotSetConvertFlags(ctx, hotConvFlags) // set flags before any hot unctions are called.
   FontName = hotReadFont(ctx, ...);
   hotAddName(ctx, ...);       // repeat for each name
   hotAddMiscData(ctx, ...);
   hotAddKernPair(ctx, ...);   // repeat for each pair
   hotAddKernValue(ctx, ...);  // repeat for each value
   hotConvert(ctx);
   hotFree(ctx);

   Additional information required by multiple master fonts is added by calls
   to hotAddAxisData(), hotAddStyleData(), and hotAddInstance(). Thus, a
   multiple master conversion follows the following call sequence.

   ctx = hotNew(...);
   hotSetConvertFlags(ctx, hotConvFlags) // set flags before any hot unctions are called.
   FontName = hotReadFont(ctx, ...);
   hotAddName(ctx, ...);       // repeat for each name
   hotAddMiscData(ctx, ...);
   hotAddAxisData(ctx, ...);   // repeat for each axis
   hotAddStyleData(ctx, ...);  // repeat for each style
   hotAddInstance(ctx, ...);   // repeat for each instance
   hotAddKernPair(ctx, ...);   // repeat for each pair
   hotAddKernValue(ctx, ...);  // repeat for each value for each master
   hotConvert(ctx);
   hotFree(ctx);

   CJK fonts must specify horizontal and (optional) vertical Unicode
   information, as well as Macintosh encoding information. This is introduced
   by calls to hotAddCMap(). Thus, a CJK conversion has the following call
   sequence.

   ctx = hotNew(...);
   hotSetConvertFlags(ctx, hotConvFlags) // set flags before any hot unctions are called.
   FontName = hotReadFont(ctx, ...);
   hotAddName(ctx, ...);       // repeat for each name
   hotAddMiscData(ctx, ...);
   hotAddCMap(ctx, ...);
   hotAddCMap(ctx, ...);
   hotAddCMap(ctx, ...);
   hotConvert(ctx);
   hotFree(ctx);

   During conversion the font data is processed by a number of steps.

   1. PostScript font data is read from the client, parsed, partially converted
      to CFF, and stored in memory.
   2. During step 1 the client may optionally choose (by providing the
      appropriate callback functions) that a temporary file be created for
      storing the partially converted data in order to reduce memory
      requirements.
   3. The partially converted data from step 1 is assembled into a CFF font and
      written back to the client. The client has the option of receiving the
      CFF font data size before writing begins so that a memory buffer for it
      may be pre-allocated.
   4. The CFF data is randomly read back from the client in order to extract
      font-wide and glyph-specific data from the font.
   5. The OTF data is written to the client. Part of this process involves
      copying the CFF data into the 'CFF ' table in the OTF font.
   6. The OTF data is randomly read back, table checksums are calculated, and
      the sfnt directory is updated.

   The sequence of calls between hotReadFont() and hotConvert() inclusive, may
   be repeated to convert more than one font using the same context. The types
   of fonts (single master, multiple master, and CID) converted in this manner
   may be freely intermixed. This approach is more efficient than allocating
   and destroying a context for each converted font. */

typedef struct hotCtx_ *hotCtx; /* Opaque library context */
typedef struct hotCallbacks_ hotCallbacks;

hotCtx hotNew(hotCallbacks *cb);

/* hotNew() initializes the library and returns an opaque context that is
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called. The client passes a set of callback
   functions to the library via the cb argument which is a hotCallbacks data
   structure whose fields are described below. Optional fields should be passed
   with a NULL value if not required.

   Five sets of I/O callback functions are required during conversion.
   Functions within each set have the same name prefix, as follows:

   ps   PostScript font data
   cff  CFF font data
   otf  OTF font data
   feat Feature file data
   tmp  Optional temporary font data used during CFF conversion

   A client is free to choose any I/O implementation that supports the I/O
   callbacks, for example:

   o File-based I/O using standard functions: fopen(), fclose(), fread(),
     fwrite(), putc(), ftell(), and fseek()
   o Memory-based I/O using dynamically allocated memory
   o Memory-mapped I/O using virtual memory support provided by the system

   It is important (for performance reasons) for a client to choose an input
   buffer size that isn't too small. For file-based clients a buffer size of
   BUFSIZ would be a good choice because it will match the underlying low-level
   system input functions. */


/* Message types (for use with message callback) */
enum
    {
    hotNOTE,
    hotWARNING,
    hotERROR,
    hotFATAL
    };

struct hotCallbacks_
    {
    void *ctx;          /* Client context (optional) */

/* [Optional] This is a client context that is passed back to the client as the
   first parameter to the callback functions. It is intended to be used in
   multi-threaded environments.

   Exception handling: */

    void (*fatal)       (void *ctx);

/* [Required] fatal() is an exception handler that is called if an
   unrecoverable error is encountered during conversion. The client should
   immediately call hotFree() to release the current context. This function
   must NOT return and the client should use longjmp() to return control to a
   point prior to calling tcNew(). */

    void (*message)     (void *ctx, int type, char *text);  /* (optional) */

/* [Optional] message() simply passes a message back to the client as a
   null-terminated string. Four message types are supported: hotNOTE,
   hotWARNING, hotERROR, and hotFATAL. A client is free to handle messages in
   any manner they choose.

   Memory management: */

    void *(*malloc)     (void *ctx, size_t size);
    void *(*realloc)    (void *ctx, void *old, size_t size);
    void (*free)        (void *ctx, void *ptr);

/* [Required] These three memory management functions manage memory in the same
   manner as the Standard C Library functions of the same name. (This means
   that they must observe the alignment requirements imposed by the standard.)
   The client is required to handle any error conditions that may arise.
   Specifically, a client must ensure requested memory is available and must
   never return a NULL pointer from malloc() or realloc().

   PostScript data input: */

    char *(*psId)       (void *ctx);

/* [Optional] psId() should provide a way of identifying the source of the
   current PostScript input font data. This would typically be a filename and
   is used in conjunction with the message() callback. */

    char *(*psRefill)   (void *ctx, long *count);

/* [Required] psRefill() is called in order to refill the library's PostScript
   font data input buffer. It returns a pointer to the new data and the count
   of the number of bytes of new data available via the count parameter. The
   client must arrange that successive calls to psRefill() return consecutive
   blocks of data starting with the first byte in the font. (A file-based
   client might map this function to fread() and fill buffers of BUFSIZ bytes.)

   CFF data input/output: */

    char *(*cffId)      (void *ctx);

/* [Optional] cffId() should provide a way of identifying the destination of
   the CFF data that is produced during conversion. This would typically be a
   filename in a file-based implementation and is used in conjunction with the
   message() callback. */

    void (*cffWrite1)   (void *ctx, int c);
    void (*cffWriteN)   (void *ctx, long count, char *ptr);

/* [Required] These functions are called to handle the output of a single byte
   (cffWrite1()) or multiple bytes (cffWriteN()) of CFF data. (A file-based
   client might map these functions to putc() and fwrite().) */

    char *(*cffSeek)    (void *ctx, long offset, long *count);

/* [Required] cffSeek() is called in order to seek to an absolute position in
   the CFF data specified by the offset parameter. It returns a pointer to the
   new data (beginning at that offset) and the count of the number of bytes of
   new data available via the count parameter. A client should protect itself
   by checking that the offset lies within the data region. (A file-based
   client might map this function to fseek() followed by fread().)
   */

    char *(*cffRefill)  (void *ctx, long *count);

/* [Required] cffRefill() is called in order to refill the library's CFF input
   buffer. It returns a pointer to the new data and the count of the number of
   bytes of new data available via the count parameter. The client must arrange
   that successive calls to cffRefill() return consecutive blocks of CFF data.
   (A file-based client might map this function to fread().) */

    void (*cffSize)     (void *ctx, long size, int euroAdded);

/* [Optional] cffSize() is called just prior to calling output functions
   cffWrite1() and cffWriteN() with the size of the CFF data in bytes. It
   permits a memory-based client to allocate the exact amount of space for the
   CFF data in memory, for example. The euroAdded argument is set to 1 if the
   Euro glyph was automatically added to the font; otherwise, it is set to 0.

   OTF data input/output: */

    char *(*otfId)      (void *ctx);
    void (*otfWrite1)   (void *ctx, int c);
    void (*otfWriteN)   (void *ctx, long count, char *ptr);
    long (*otfTell)     (void *ctx);
    void (*otfSeek)     (void *ctx, long offset);
    char *(*otfRefill)  (void *ctx, long *count);

/* [Required, except for otfId()] These functions are identical to the
   similarly named cff functions except that the operate on OTF data and
   otfSeek just performs positioning and doesn't return data.

   otfSeek() returns the current offset in the OTF data (relative to its start
   at offset 0). (A file-based client would typically map this to function to
   ftell().)

   Feature file data input: */

    char *(*featOpen)       (void *ctx, char *name, long offset);
    char *(*featRefill)     (void *ctx, long *count);
    void (*featClose)       (void *ctx);
    void (*featAddAnonData) (void *ctx, char *data, long count,
                             unsigned long tag);

/* [Optional] These functions are called to handle feature file support.
   featOpen() is called to open either the main feature file for the font
   (indicated by name being NULL) or a feature include file (name will be the
   file name indicated in the "include" directive in a feature file).
   featOpen() should locate the file if name is not absolute (typically by
   searching first in the directory in which the main feature file resides, and
   then in a standard directory). If the file is not found or not needed (in
   the case of name being NULL), featOpen() should return NULL. If the file is
   found, featOpen() should open it, seek to offset, and return a descriptive
   identifier (typically the full file name), which hotconv will use to report
   any feature error messages within the file.

   featRefill() is identical to cffRefill() except that it operates on the
   feature file opened by featOpen().

   featClose() closes the feature file opened by featOpen().

   featAddAnonData() is called at the end of every anonymous block in the
   feature file. It supplies a pointer to the data seen within the block (from
   the beginning of the line after the opening brace and upto the end of the
   line, including the newline, before the closing brace) and a count of the
   number of bytes of data available. The data is guaranteed to be valid only
   for the duration of the function call. The tag argument indicates the tag
   associated with the anonymous block in the feature file. Anonymous blocks
   typically contain information needed to specify OTF tables that the hot
   library does not directly support. In these cases, the client must add the
   anonymous tables by calls to hotAddAnonTable() within the duration of the
   featAddAnonData() callbacks.

   Temporary file I/O (all optional) */

    void (*tmpOpen)     (void *ctx);
    void (*tmpWriteN)   (void *ctx, long count, char *ptr);
    void (*tmpRewind)   (void *ctx);
    char *(*tmpRefill)  (void *ctx, long *count);
    void (*tmpClose)    (void *ctx);

/* [Optional] These functions are called to handle large CID-keyed fonts when
   memory is at a premium. If tmpOpen() is non-NULL the library will use these
   functions to reduce memory requirements. The idea behind this mode is that
   rather than keep entire fonts in memory they are accumulated in a temporary
   file that is re-read during the output of the CFF data. (A client might map
   these functions onto tmpfile(), fwrite(), rewind() or fseek(), fread(), and
   fclose(), respectively.) */

    char *(*getFinalGlyphName)(void *ctx, char *gname);

/* [Optional] getFinalGlyphName() is called in order to convert an aliased
   glyph name (a user-friendly glyph name used within the feature file) to a
   final name that is used internally within the OpenType font. If no such
   mapping exists the gname argument is returned. */

    char *(*getUVOverrideName)(void *ctx, char *gname);

/* [Optional] getUVOverrideName() is called in order to get a user-supplied UV value for
	the glyph, in the form of a u<UV hex Code> glyph name. If there is no
	override value for the glyph, then the function returns NULL. isFinal specifies whether
	the gname passed in is a final name or an alias name */

   void (*getAliasAndOrder) (void *ctx, char* oldName, char** newName, long int *order);
/* Optional. If present,  parse.c wil call it to get a new name, and an ordering index. These are
	used to rename the glyphs in teh font, and establish a new glyph order.
*/

 /*  Unicode Variation Selection file data input: */

    char *(*uvsOpen)       (void *ctx, char *name);
    char *(*uvsGetLine)     (void *ctx, char *buffer, long *count);
    void (*uvsClose)       (void *ctx);

    };

typedef struct hotReadFontOverrides_ hotReadFontOverrides;

void hotSetConvertFlags(hotCtx g, unsigned long hotConvFlags); // set flags before any hot unctions are called.

char *hotReadFont(hotCtx g, int flags, int *psinfo, hotReadFontOverrides *fontOverride );

/* hotReadFont() is called to read the PostScript outline font. The flags
   argument debugging and font processing: */

#define HOT_DB_AFM          (1<<0)  /* Enable AFM debug */
#define HOT_DB_MAP          (1<<1)  /* Enable map debug */
#define HOT_DB_FEAT_1       (1<<2)  /* Enable feature debug level 1 */
#define HOT_DB_FEAT_2       (1<<3)  /* Enable feature debug level 2 */
#define HOT_DB_MASK         0x000F  /* Debug flags mask */
#define HOT_ADD_AUTH_AREA   (1<<4)  /* Add authentication area to CID font */
#define HOT_SUBRIZE         (1<<5)  /* Subroutinize charstrings */
#define HOT_ADD_EURO        (1<<6)  /* Add euro glyph to standard fonts */
#define HOT_NO_OLD_OPS      (1<<7)  /* Convert seac, remove dotsection */
#define HOT_IS_SERIF        (1<<8)  /* Target font is serif: used when synthesizing  glyphs */
#define HOT_SUPRESS_HINT_WARNINGS   (1<<9)  /* Emit warnings during target font processing */
#define HOT_FORCE_NOTDEF  (1<<10)  /* Force replacement of source font notdef by markign notdef. */
#define HOT_IS_SANSSERIF        (1<<11)  /* Target font is sans serif: used when synthesizing  glyphs */
										/* heuristic is used when neither HOT_IS_SANSSERIF nor HOT_IS_SERIF is used. */
#define HOT_RENAME  (1<<12)  /*Use client call-back to rename and reorder glyphs. */
#define HOT_SUBSET (1<<13)
 
#define HOT_SUPRESS__WIDTH_OPT (1<<14) /* supress width optimization in CFF: makes it easier to poek at charstrings with other tools */
    
struct hotReadFontOverrides_           /* Record for instructions to modify font as it is read in. */
    {
    long syntheticWeight;
    unsigned long maxNumSubrs;
    };
										
/* hotReadFont() returns the font type via the low order bits of the psinfo
   argument. Whether the font specified Standard Encoding is also returned via
   this argument. The structure hotReadFontOverrides contains data to modify the font
   as it is read in. This currently only carries an override for the weight coordinate
   of the built-in substitution MM font, for adding new glyphs. */

enum                            /* Font types */
    {
    hotSingleMaster,
    hotMultipleMaster,
    hotCID
    };
#define HOT_TYPE_MASK   0x0f    /* Type returned in low 4 bits */
#define HOT_STD_ENC     0x10    /* Bit flags if font is standard encoded */

/* hotReadFont() itself returns the FontName which is not guaranteed to remain
   stable after subsequent calls to the library so should be copied if needed
   for a longer period. */

typedef struct hotCommonData_ hotCommonData;
typedef struct hotWinData_ hotWinData;
typedef struct hotMacData_ hotMacData;

void hotAddMiscData(hotCtx g,
                    hotCommonData *common, hotWinData *win, hotMacData *mac);

/* hotAddMiscData() provides miscellaneous data for the OTF font. The win or
   mac arguments should be passed as NULL if no data can be supplied by the
   client. The data structure fields are described below. */

/* 8-bit encoding that maps code (index) to glyph name */
typedef char *hotEncoding[256];

struct hotCommonData_           /* Miscellaneous data record */
    {
    long flags;				/* This filed is masked with 0x1ff (bits 0-8), and the result is used in the
    							input font processing modules. this is copied into txCtx->fonts.flags, for which  additional
    							bits are defined starting at (1<<12)  - see FI_MISC_FLAGS_MASK in comman.h */
#define HOT_BOLD            (1<<0)  /* Bold font */
#define HOT_ITALIC          (1<<1)  /* Italic font */
#define HOT_USE_FOR_SUBST   (1<<2)  /* MM: May be used for substitution */
#define HOT_CANT_INSTANCE   (1<<3)  /* MM: Can't make instance */
#define HOT_DOUBLE_MAP_GLYPHS      (1<<4)    /* Provide 2 unicode values for a hard-coded list of glyphs - see agl2uv.h:

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
#define HOT_WIN             (1<<6)  /* Windows data */
#define HOT_MAC             (1<<7)  /* Macintosh data */
#define HOT_EURO_ADDED      (1<<8)  /* Flags Euro glyph added to CFF data */
    char *clientVers;
    long nKernPairs;
    short nStyles;
    short nInstances;
    hotEncoding *encoding;
    short fsSelectionMask_on;
    short fsSelectionMask_off;
    unsigned short os2Version;
    char *licenseID;
    };

/* The flags field passes style information and the data source (Windows or
   Macintosh).

   The clientVers field, if non-null, will be appended to the end of the
   name table's version string (id=5).

   The nKernPairs field specifies the number of kerning pairs to be
   subsequently added with hotAddKernPair().

   The nStyles field specifies the number of styles to be subsequently
   described with hotAddStyleData() and should be set to 0 for non-multiple
   master fonts.

   The nInstances field specifies the number of instances to be subsequently
   added with hotAddInstance() and should be set to 0 for non-multiple master
   fonts.

   The encoding field, along with hotMacData.encoding described below,
   specifies the encoding that the library should use to decode kern pairs
   added by hotAddKernPair(). If both fields are set to NULL, the library will
   use the PostScript encoding to decode kern pairs.

   For a Macintosh conversion:
   o If the "use standard Macintosh reencoding" bit in the FontClass field of
     the FOND resource is set, hotCommonData.encoding is typically set to the
     Macintosh Standard Roman encoding and hotMacData.encoding is set to NULL.
   o Macintosh fonts with unusual encoding require that the "use non-standard
     Macintosh reencoding" bit in the FontClass be set and a font reencoding be
     specified in the FOND. In these cases, hotCommonData.encoding should be
     set to NULL and the reencoding found in the FOND should be be passed via
     hotMacData.encoding. Unused elements of the encoding array should be set
     to NULL. The library will layer the reencoding onto the PostScript
     encoding to recreate the Macintosh encoding.
   o If neither of the FontClass bits mentioned above are set, then both
     encoding fields should be set to NULL.

   For a Windows conversion:
   o If the font needs to be reencoded to a standard Windows code page,
     typically indicated by a charset that has a value other than symbol
     charset, then that Windows code page should be passed in via
     hotCommonData.encoding. If the PostScript encoding of the font is to be
     used, hotCommonData.encoding should be set to NULL.
   o If the font uses a non-standard Macintosh reencoding when used on the
     Macintosh, the entire Macintosh encoding (not just the reencoding that
     would have been in the FOND) should be passed via the hotMacData.encoding
     field. The client would typically access this information from a font
     conversion database. */

struct hotWinData_              /* Windows-specific data */
    {
    short nUnencChars;
    unsigned char Family;
#define HOT_DONTCARE    0
#define HOT_ROMAN       1
#define HOT_SWISS       2
#define HOT_MODERN      3
#define HOT_SCRIPT      4
#define HOT_DECORATIVE  5
    unsigned char CharSet;
    unsigned char DefaultChar;
    unsigned char BreakChar;
    };

/* The nUnencChars field specifies the number of unencoded character names
   to be subsequently added with hotAddUnencChar(). (See kerning section below
   for further details.)

   The Family field takes the same values as bits 4-7 of iFamily field in a PFM
   file. (See Windows documentation for further details.) A value of 0xFF
   indicates that this value as well as the CharSet, DefaultChar, and BreakChar
   values are unknown to the client in which case the library will attempt to
   set the values from inspection of other data values.

   The CharSet field specifies the windows charset for this font. (See Windows
   documentation for further details.) This field should be set from
   pfm.CharSet. Also see description of the Family field.

   The DefaultChar should be set from pfm.FirstChar + pfm.DefaultChar. The
   BreakChar should be set from pfm.FirstChar + pfm.BreakChar. Also see
   description of the Family field. */

struct hotMacData_              /* Macintosh-specific data */
    {
    hotEncoding *encoding;
    long cmapScript;
    long cmapLanguage;
#define HOT_CMAP_UNKNOWN (-1)
    };

/* The hotMacData.encoding field, along with hotCommonData.encoding described
   earlier, specifies the encoding that the library should use to decode kern
   pairs passed in by hotAddKernPair(). Additionally, when hotMacData.encoding
   is non-NULL, the Macintosh cmap created by the library will represent the
   original Macintosh encoding. Refer to the hotCommonData.encoding description
   for information on how to set this field.

   The cmapScript and cmapLanguage fields can be optionally set to specify the
   platformSpecific and language fields to be used in the Macintosh cmap that
   is created. Note that the cmapLanguage field is one greater than the
   Macintosh language ID code; a value of 0 indicates unspecific language.

   The client may obtain these values in any ways that it chooses, including
   databases or analysis of the FOND ID's range (in the case of a Macintosh
   conversion). The client may set either or both fields to HOT_CMAP_UNKNOWN
   to indicate that no information is available for that field. In these cases,
   the library will use its own heuristics to determine appropriate values. */

void hotAddKernPair(hotCtx g, long iPair, unsigned first, unsigned second);

/* hotAddKernPair() adds a single kerning pair to the OTF font. The iPair
   argument is an index identifying the pair and must be in the range: 0 <=
   iPair < nKernPairs. The first and second arguments specify the first and
   second glyphs of each pair either as a character code in bits 0-7 if bit
   15=0 or as an unencoded character index in bits 0-14 if bit 15=1. Character
   codes are converted to glyph names using the encoding provided by
   hotCommonData. (Unencoded character names are provided by calls to
   hotAddUnencChar().) */

void hotAddKernValue(hotCtx g, long iPair, int iMaster, short value);

/* hotAddKernValue() adds a kerning value to the kerning pair specified by the
   iPair argument for the master specified by the iMaster argument which
   must have the value 0 for a single master font or in the range: 0 <= iMaster
   < nMasters for multiple master fonts. */

void hotAddUnencChar(hotCtx g, int iChar, char *name);

/* hotAddUnencChar() adds an unencoded glyph name to the OTF. The iChar
   argument is an index identifying the character that has a name specified by
   the length and name arguments. The iChar argument must be in the range: 0 <=
   iChar < nUnencChars. Unencoded characters are used to associate member
   glyphs of a kern pair with a name when one or both glyphs are unencoded.
   This name is subsequently converted into a glyph id by the library. */

typedef long hotFixed;          /* 16.16 fixed point */
void hotAddAxisData(hotCtx g, int iAxis,
                    char *type, char *longLabel, char *shortLabel,
                    hotFixed minRange, hotFixed maxRange);

/* hotAddAxisData() adds information to an OTF font for the axis selected by
   the iAxis argument which must be in the range: 0 <= iAxis < nAxes. The type
   argument specifies the kind of axis as one of: "Weight", "Width",
   "OpticalSize", or "Serif". The "longLabel" and "shortLabel" arguments
   specify long and short name tags to be used in construction menu and font
   names, e.g. "Weight" and " wt ". */

void hotAddStyleData(hotCtx g, int iStyle, int iAxis, char *type,
                     hotFixed point0, hotFixed delta0,
                     hotFixed point1, hotFixed delta1);

/* hotAddStyleData() adds style information to an OTF font for the axis
   selected by the iAxis argument. The type argument specifies the style type
   as one of: "Bold", "Italic", "Condensed", and "Extended". The point
   arguments specify a coordinate on the axis and the "delta" arguments specify
   the coordinate change from that point to a new coordinate that represents
   the applied style. */

void hotAddInstance(hotCtx g, int iInstance, char *suffix);

/* hotAddInstance() adds instance information to an OTF font for the instance
   selected by the "iInstance" argument which must be in the range: 0 <=
   iInstance < nInstances. iInstance 0 must specify the default instance; other
   instances may be added in any order. The "suffix" argument specifies an name
   that may be concatenated with the 5-3-3 FontName to create an instance
   FontName, e.g.  "367 RG 465 CN 11 OP". */

typedef char *(*hotCMapId)(void *ctx);
typedef char *(*hotCMapRefill)(void *ctx, long *count);
void hotAddCMap(hotCtx g, hotCMapId id, hotCMapRefill refill);

/* hotAddCMap() parses up to 3 Adobe CMaps passed in by the client for CID
   fonts: the horizontal (required) and vertical (optional) Unicode CMaps, and
   the Macintosh CMap (required). A Unicode 'cmap' is created from the
   horizontal Unicode CMap; a 'vert' 'GSUB' feature is created from the
   vertical Unicode CMap, when present; a Macintosh 'cmap' is created form the
   Macintosh CMap. The CMaps may be passed in in any order. The Unicode CMaps
   must be UCS-2 encoded; the Macintosh CMap must have at least one single-byte
   codespace range.

   The id callback argument identifies the source of the CMap data (typically a
   filename) and is used in conjunction with the message() callback. The id
   callback argument may be set to NULL if no message feedback is required by
   the client.

   The refill callback argument is called in order to refill the library's
   PostScript CMap data input buffer. It returns a pointer to the new data and
   the count of the number of bytes of new data available via the count
   parameter. The client must arrange that successive calls to refill() return
   consecutive blocks of data starting with the first byte in the CMap file.
   The client context specified in the hotCallbacks structure supplied to
   hotNew() is passed back to the client when the refill() function is called.
   (A file-based client might map this function to fread() and fill buffers of
   BUFSIZ bytes.) */

void hotAddUVSMap(hotCtx g, char* uvsFileName);

/* hotAddUVSMap() parses hte input file uvsFileName to build a cmap format 14 subtable.
The uvsFileName is the file path to a specifiaction for a set of Unicode variation Selectors.
Tis is a simple text file with one record per line. The record fields are:  
 - base Unicode value, specified as decimnal, or as hex with a preceeding "0x"
 - Unicode Variation Selectior value, s pecified as decimnal, or as hex with a preceeding "0x"
 - ROS name, whci is ignored
 - CID for CID-keuyed fonts, else glyph name.
 Example:
 3402 E0100; Adobe-Japan1; CID+13698

Blank lines, and characters following the comment char "#" on a line, are ignored.
*/

int hotAddName(hotCtx g,
               unsigned short platformId, unsigned short platspecId,
               unsigned short languageId, unsigned short nameId,
               char *str);

/* hotAddName() is called to add menu names. This name will be directly stored
   in the name table using the various id arguments specified. The actual name
   is specified via the str argument as a null-terminated string.

   The name string is composed of 1-byte ASCII characters. These are converted
   on the Windows platform to Unicode by adding a high byte of 0. 2-byte
   Unicode values for the Windows platform may be specified using a special
   character sequence of a backslash character (\) followed by exactly four
   hexadecimal numbers (of either case) which may not all be zero, e.g. \4e2d .
   The ASCII backslash character must be represented as the sequence \005c .

   There is no corresponding conversion to Unicode for the Macintosh platform
   but character codes in the range 128-255 may be specified using a special
   character sequence of a backslash character (\) followed by exactly two
   hexadecimal numbers (of either case) which may not both be zero, e.g. \83 .

   The special notations described above aren't needed for Latin fonts because
   their menu names have traditionally been restricted to ASCII characters
   only. However, the special notations are required for CJK fonts where menu
   names are typically composed of characters outside of the usual ASCII range.

   hotAddName() returns 1 if the name string doesn't conform to the above rules
   else returns 0.
   */

/* Default Microsoft name ids */
#define HOT_NAME_MS_PLATFORM    3       /* Platform: Microsoft */
#define HOT_NAME_MS_UGL         1       /* Platform-specific: UGL */
#define HOT_NAME_MS_ENGLISH     0x0409  /* Language: English (American) */

/* Default Macintosh name ids */
#define HOT_NAME_MAC_PLATFORM   1       /* Platform: Macintosh */
#define HOT_NAME_MAC_ROMAN      0       /* Platform-specific: Roman */
#define HOT_NAME_MAC_ENGLISH    0       /* Language: English */

/* Registered name ids */
#define HOT_NAME_COPYRIGHT      0
#define HOT_NAME_FAMILY         1
#define HOT_NAME_SUBFAMILY      2
#define HOT_NAME_UNIQUE         3
#define HOT_NAME_FULL           4
#define HOT_NAME_VERSION        5
#define HOT_NAME_FONTNAME       6
#define HOT_NAME_TRADEMARK      7
#define HOT_NAME_MANUFACTURER   8
#define HOT_NAME_DESIGNER       9
#define HOT_NAME_DESCRIPTION    10
#define HOT_NAME_VENDOR_URL     11
#define HOT_NAME_DESIGNER_URL   12
#define HOT_NAME_PREF_FAMILY    16
#define HOT_NAME_PREF_SUBFAMILY 17
#define HOT_NAME_COMP_FULL      18
#define HOT_NAME_WPF_FAMILY     21
#define HOT_NAME_WPF_STYLE      22
#define HOT_NAME_REG_LAST       HOT_NAME_WPF_STYLE

/* Maximum name lengths (including '\0') */
#define HOT_MAX_FONT_NAME      64
#define HOT_MAX_MENU_NAME      32
#define HOT_MAX_SHORT_STR       6

typedef char *(*hotAnonRefill)(void *ctx, long *count, unsigned long tag);
void hotAddAnonTable(hotCtx g, unsigned long tag, hotAnonRefill refill);

/* hotAddAnonTable() is an optional function that allows a client to add tables
   to the sfnt that aren't directly supported by the hot library. It is called
   once for each table to be added.

   The tag argument specifies the table tag which must be unique within an sfnt
   and therefore cannot match any of the supported tables or other anonymous
   client tables that were added using hotAddAnonTable().

   The refill() callback specifies a function to be called when it is time to
   add the client-supplied data for an anonymous table. It returns a pointer to
   the new data and the count of the number of bytes of new data available via
   the count parameter. The client must arrange that successive calls to
   refill() return consecutive blocks of data starting with the first byte of
   data in the table. When all table data has been supplied via the refill()
   callback the client must set the count argument to 0 to signal end of data.
   The tag argument of refill() may be used by the client in order to identify
   the table being added to the sfnt.

   Tables added using hotAddAnonTable() are added to the end of the sfnt in the
   order that they were added via hotAddAnonTable(). Anonymous table additions
   do not persist across multiple fonts and must be supplied via calls
   hotAddAnonTable() before hotConvert() is called for each font. */

unsigned short hotMapName2GID(hotCtx g, char *gname);
unsigned short hotMapPlatEnc2GID(hotCtx g, int code);
unsigned short hotMapCID2GID(hotCtx g, unsigned short cid);

/* hotMapName2GID(), hotMapPlatEnc2GID(), and hotMapCID2GID() allow the client
   to determine the glyph ordering that has been imposed by the hot library.
   The client can determine a glyph id from either a glyph name or platform
   encoding, for non-CID fonts, or from a CID, for CID fonts. These functions
   may be useful for ordering data in client-supplied anonymous tables. These
   functions may be used only after hotAddMiscData() is called. All these
   functions return a invalid GID with a value of 65535 if the mapping doesn't
   exist. */

void hotConvert(hotCtx g);

/* hotConvert() is used to initiate the final conversion to OTF after all the
   miscellaneous data has been provided via the other library functions. 
   
   	convertFlags is used to control the processing of the data. */

/* convertFlags values */
#define HOT_ID2_CHAIN_CONTXT3  (1 << 0)  /* Index the backup glyph node list backwards  (relative to the spec)
											in GSUB Lookup 6/GPOS Lookup 8,
											as required by InDesign 2 and other consumers of the CoolType
											OpenType libraries of Aug 2002 and earlier*/

#define HOT_ALLOW_STUB_GSUB  (1 << 1)  /* If no GSUB rules are specified, make a stub GSUB table */
#define HOT_OLD_SPACE_DEFAULT_CHAR  (1 << 2) /* Spec says use notdef, but QuarkXPress 6.5 needs space; CJK publishers still use this. */
#define HOT_USE_V1_MENU_NAMES (1 << 3) /* Build name table Mac menu names as Apple originally asked in 1999, and per FDK through 2.0, rather
										by the OpenType spec. This mode writes the Preferred Family and Style names in name ID's 1 and 2
										 rather than 16 and 17, when these differ from the Compatible Family/Style names */
#define HOT_SEEN_VERT_ORIGIN_OVERRIDE (1 << 4) /* IF an explicit glyph origin override, or a vrt2 feature, is seen, then write the vmtx table  */
#define HOT_USE_OLD_NAMEID4 (1 << 5) /* For fonts with previous shipped versions, build name ID 4 compatible with previously shipped versions:
									Windows name ID 4 == PS name, and Mac name ID 4 built from preferred family and style names.  Else,
									do both by OT spec: name id 1 + space + name ID 2, or "" if nme ID 2 is "Regular". */

#define HOT_OMIT_MAC_NAMES (1 << 6) /* Build name table without Mac platform names */
#define HOT_STUB_CMAP4 (1 << 7) /* Build only a stub cmap 4 table. Useful for AdobeBlank, and otehr cases where size is an issue. Font must contain cmap format 4 to work on Windows, but it doesn't have to be useful. */
#define HOT_OVERRIDE_MENUNAMES (1<<8)
#define HOT_DO_NOT_OPTIMIZE_KERN (1<<9) /* Do not use left side kern class 0 for non-zero kern values. Saves a a few hundred to thousand bytes, but confuses some developers. */
    
void hotFree(hotCtx g);

/* hotFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

/* Environment variables used to set default values */
#define kFSTypeEnviron "FDK_FSTYPE"

#ifdef __cplusplus
}
#endif

#endif /* HOT_H */
