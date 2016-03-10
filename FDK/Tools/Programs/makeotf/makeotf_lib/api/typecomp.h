/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef TYPECOMP_H
#define TYPECOMP_H

#include <stddef.h>             /* For size_t */

#ifdef __cplusplus
extern "C" {
#endif

#define TC_VERSION 0x020025 /* Library version 2.0.37 */

/* CFF major and minor version numbers supported by library */
#define TC_MAJOR    1
#define TC_MINOR    0

/***********************************************************************/
/* Note: multiple master Type 1 is not supported by this version of    */
/* the library.                                                        */
/***********************************************************************/

typedef struct tcCtx_ *tcCtx;   
typedef struct tcCallbacks_ tcCallbacks;

tcCtx tcNew(tcCallbacks *cb);

/* tcNew() initializes the library and returns and opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes a set
   of callback functions to the library via the cb argument which is a
   tcCallbacks data structure whose fields are described below. Optional fields
   should be passed with a NULL value if not required. */

/* Message types (for use with message callback) */
enum
    {
    tcNOTE = 0,
    tcWARNING = 1,
    tcERROR,
    tcFATAL
    };

struct tcCallbacks_
    {
    void *ctx;

/* [Optional] ctx is a client context that is passed back to the client as the
   first parameter to the callback functions. It is intended to be used in
   multi-threaded environments.

   Exception handling: */

    void (*fatal)       (void *ctx);

/* [Required] fatal() is an exception handler that is called if an
   unrecoverable error is encountered during compaction. The client should
   imediately call tcFree() to release the current context. This function must
   NOT return and the client should use longjmp() to return control to a point
   prior to calling tcNew(). */

    void (*message)     (void *ctx, int type, char *text);

/* [Optional] message() simply passes a message back to the client as a
   null-terminated string. Four message types are supported: tcNOTE, tcWARNING,
   tcERROR, and tcFATAL. The client can handle messages in any manner they
   choose.

   Memory management: */

    void *(*malloc)     (void *ctx, size_t size);
    void *(*realloc)    (void *ctx, void *old, size_t size);
    void (*free)        (void *ctx, void *ptr);

/* [Required] The malloc(), realloc(), and free() fuctions manage memory in the
   same manner as the Standard C Library functions of the same name. (This
   means that they must observe the alignment requirements imposed by the
   standard.) The client is required to handle any error conditions that may
   arise. Specifically, a client must ensure requested memory is available and
   must never return a NULL pointer from malloc() or realloc().

   PostScript data input: */

    char *(*psId)       (void *ctx); 

/* [Optional] psId() should provide a way of identifying the source of the
   current input font data. This would typically be a filename and is used in
   conjunction with the message() callback. */

    char *(*psRefill)   (void *ctx, long *count);

/* [Required] psRefill() is called in order to refill the library's font data
   input buffer. It returns a pointer to the new data and the count of the
   number of bytes of new data available via the count parameter. The client
   must arrange for successive calls to psRefill() to return consecutive blocks
   of data starting with the first byte in the font. (A file-based client might
   map this function to fread() and fill buffers of BUFSIZ bytes.) */

    long (*psSize)      (void *ctx);

/* [Optional] psSize() is called at the end of input to get the size of the
   input data so that it may be reported with the statistics option. The code
   to do this is only included when the TC_STATISTICS macro is defined. (A
   file-based client might seek to the end of the input file with fseek() and
   report the offset, and therefore file size, with ftell()).

   CFF data output: */

    char *(*cffId)      (void *ctx);

/* [Optional] cffId() should provide a way of identifying the destination of
   the CFF data. This would typically be a filename and is used in conjunction
   with the message() callback. */

    void (*cffWrite1)   (void *ctx, int c);
    void (*cffWriteN)   (void *ctx, long count, char *ptr);

/* [Required] cffWrite1() and cffWriteN() are called to handle the output of a
   single byte (cffWrite1()) or multiple bytes (cffWriteN()) of CFF data. (A
   file-based client might map these functions onto putc() and fwrite().) */

    void (*cffSize)     (void *ctx, long size, int euroAdded);

/* [Optional] cffSize() is called just prior to calling output functions
   cffWrite1() and cffWriteN() with the size of the CFF data in bytes. It
   permits a memory-based client to allocate the exact amount of space for the
   CFF data in memory, for example. This function also returns an indication of
   whether the Euro glyph was automatically added to the font.

   Temporary file I/O: */

    void (*tmpOpen)     (void *ctx);                        
    void (*tmpWriteN)   (void *ctx, long count, char *ptr); 
    void (*tmpRewind)   (void *ctx);                        
    char *(*tmpRefill)  (void *ctx, long *count);                
    void (*tmpClose)    (void *ctx);

/* [Optional] The tmp functions are called to handle small-memory mode for
   CID-keyed fonts and are enabled when the TC_SMALLMEMORY bit is set in the
   flags argument to tcCompactFont() and tcAddFont(). The idea behind this mode
   is that rather than keep entire fonts in memory they are accumulated in a
   temporary file that is re-read during the output of the CFF data. (A client
   might map these functions onto tmpfile(), fwrite(), rewind() or fseek(),
   fread(), and fclose(), respectively.) */

    void (*glyphMap)    (void *ctx, unsigned short nGlyphs, unsigned short gid,
                         char *gname, unsigned short cid);

/* [Optional] glyphMap() passes the gid mapping for each glyph in the CFF font
   back to the client. This permits a client to determine the glyph layout
   within the CFF font which is normally opaque. The nGlyphs argument is
   specifies the total number of glyphs in the CFF font including the .notdef
   glyph. It is provided so that clients may allocate a suitably-sized data
   structure for all the glyph mapping data, presumably in the first call of
   glyphMap(). The gid argument specifies the glyph index. glyphMap() will be
   called nGlyphs times for each converted font from gid 0 through to gid
   (nGlyphs - 1). Either the cid or gname arguments specify the glyph "name"
   that was mapped to the specified gid, for CID and non-CID fonts,
   respectively. The gname argument is always set to NULL for CID fonts and the
   cid argument is always set to zero for non-CID fonts. The gname string
   is not persistent and should be copied if needed later. */
   
   void (*getAliasAndOrder) (void *ctx, char* oldName, char** newName, long int *order);
/* Optional. If present,  parse.c wil call it to get a new name, and an ordering index. These are
	used to rename the glyphs in the font, and establish a new glyph order.
*/
    };
	
	
/* [The file t1c/cb.c is an example of a client callback layer for the TC
   library which fully implements all of the available callbacks and includes
   auto-sensing support for PFA, PFB, and LWFN formats of font files.] */

typedef struct tcSubset_ tcSubset;

void tcAddFont(tcCtx g, long flags);

/* tcAddFont() is called to add a PostScript outline font to a FontSet.
   Multiple fonts may be accumulated in a FontSet by repeated calls to this
   function. Compaction features may be controlled by the flags argument
   (described below). */

void tcAddCopyright(tcCtx g, char *copyright);

void tcSetMaxNumSubrsOverride(tcCtx g, unsigned long maxNumSubrs);

/* tcSetMaxNumSubsOverride is used to set a maximum numbe of subroutines.*/

void tcSetWeightOverride(tcCtx g, long syntheticWeight);

/* tcSetWeightOverride() is called to set the synthetic weight to be used
   in the design vector, when adding synthetic glyphs */

long tcGetWeightOverride(tcCtx g);

/* tcGetWeightOverride() is called to set the synthetic weight to be used
   in the design vector, when adding synthetic glyphs */


void tcWrapSet(tcCtx g, char *name, char *version, char *master);

/* tcWrapSet() is called to wrap a FontSet with a PostScript wrapper that makes
   the FontSet usable as a PostScript interpreter core FontSet. The
   null-terminated name argument specifies the name of the FontSet. The
   null-terminated version argument specifies the real version of the FontSet.
   If non-NULL, the null-terminated master argument specifies the Chameleon
   master font name. This function should be called once per FontSet. */

void tcCompactSet(tcCtx g);

void tcCompactFont(tcCtx g, long flags);

/* tcCompactFont() is called to make a single-font FontSet. It is equivalent to
   calling tcAddFont() followed by tcCompactSet(). Compaction features may be
   controlled by the flags argument (described below).

   The representation of fonts compacted by tcAddFont() and tcCompactFont() may
   be controlled by the flags argument to these functions using the following
   flag bits: */

#define TC_SUBRIZE      (1<<0)  /* Subroutinize charstrings (enabled by 
                                   TC_SUBR_SUPPORT macro) */
#define TC_EMBED        (1<<1)  /* Space-optimize embeddable set */
#define TC_ROM          (1<<2)  /* Space optimize ROM set (no old printer
                                   support) */
#define TC_NOHINTS      (1<<3)  /* Discard hints in charstrings */
#define TC_NONOTICE     (1<<4)  /* Don't add copyright notice to font dict */
#define TC_SHOWNOTICE   (1<<5)  /* Show copyright notices (used in preparing
                                   ROM set) */
#define TC_SMALLMEMORY  (1<<6)  /* Reduce heap space during execution (CID
                                   fonts only) */
#define TC_ADDAUTHAREA  (1<<7)  /* Add font authentication area to CID font */
#define TC_ADDEURO      (1<<8)  /* Add euro glyph to suitable fonts (enabled 
                                   by TC_EURO_SUPPORT macro) */
#define TC_NOOLDOPS     (1<<9)  /* Convert/remove old charstring operators */
#define TC_IS_SERIF     (1<<10)  /* target font is serif */
#define TC_DO_WARNINGS  (1<<11)  /* emit parse warnings */
#define TC_FORCE_NOTDEF (1<<12)  /* Add marking notdf, even if font already has one */
#define TC_IS_SANSSERIF (1<<13)  /*  target font is sans-serif. If neither this nor TC_IS_SERIF
								is set, then a heuristic is used. Needed only for synthetic glyphs. */
#define TC_RENAME (1<<14)  /* Use glyph alais call-back to rename glyphs. */
#define TC_SUBSET (1<<15) /* Omit glyphs not named in the GOADB. */
#define TC_SUPPRESS_HINT_WARNINGS (1<<16) /* Used to supress hitn warngins when buiulding a temp font from a TTF source font. */

#define TC_SUPPRESS_WIDTH_OPT (1<<17) /* supress width optimization in CFF: makes it easier to poek at charstrings with other tools */


int tcSetStats(tcCtx g, int gather);

/* tcSetStats() permits compaction statistics to be gathered and reported if
   the gather argument is non-0 or suppressed if the gather argument is 0. This
   function is only effective if the library was compiled with the
   TC_STATISTICS macro enabled. */

void tcFree(tcCtx g);

/* tcFree() destroys the library context and all resources allocated to it. */ 

#ifdef __cplusplus
}
#endif

#endif /* TYPECOMP_H */
