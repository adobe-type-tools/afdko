/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef BEZREAD_H
#define BEZREAD_H

#include "ctlshare.h"

#define BZR_VERSION CTL_MAKE_VERSION(0,7,0)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bez Parse Library
   =================
   This library parses the bez format as described in the bez_parse.y file.  This file 
   contains the yacc grammer for this format.  The bez format is normally encrypted, 
   however this library only works on the unencrypted text form.

   This library is initialized with a single call to bzrNew() that allocates
   an opaque context (bzrCtx) which is passed to subsequent functions and is
   destroyed by a single call to bzrFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   A parse is begun by calling bzrBegFont() and completed by calling bzrEndFont().
   Between these calls one or more glyphs may be parsed with either
   bzrIterateGlyphs() or bzrGetGlyphByTag().

   Memory management and source data functions are provided by two sets of
   client-supplied callbacks described in ctlshare.h. */

typedef struct bzrCtx_ *bzrCtx;
bzrCtx bzrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define BZR_CHECK_ARGS CTL_CHECK_ARGS_CALL(BZR_VERSION)

/* bzrNew() initializes the library and returns an opaque context (bzrCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The BZR_CHECK_ARGS macro is passed as the last parameter to bzrNew() in
   order to perform a client/library compatibility check. 

   The optional debug data stream (BZR_DBG_STREAM_ID) are opened by this call. 
   The debug stream provides more detailed error and warning messages than is 
   available via bzrErrStr(). If the client doesn't require the debug data stream 
   NULL should be returned from its stream open call. */

int bzrBegFont(bzrCtx h, 
               long flags, abfTopDict **top);

#define BZR_NO_AC (1<<0)

/* bzrBegFont() is called to initiate a new parse.  The source data stream
   (BZR_SRC_STREAM_ID) is opened and each bez path is read. A bez path is defined
   as a beginning "sc" token and ends with an "ed" token.
 
   Since the bez format does not contain any width information this library will
   issue a width() callback immediately following the beg() if the return 
   value permits it.  A client should intercept this call and provide the 
   appropriate width to be used for the based on the preceeding glyphInfo 
   given in the beg() callback.

   There is additional information that is not available from the format.
   For example encoding and glyphname.  These two fields are automatically assigned
   a value.  

   For the encoding, the first glyph is assigned the standard encoding
   value of 'A' with all subsequent paths following in succession (e.g. 'B', 'C').

   The glyphname is constructed from the order of the paths seen.  The first
   path will be named "g1", the second "g2", and so forth.

   It is worth noting that the .notdef is handled specially and will have the appropriate
   glyphname and will be left unencoded.
   */
int bzrIterateGlyphs(bzrCtx h, abfGlyphCallbacks *glyph_cb);

/* bzrIterateGlyphs() is called to interate through all the glyph data in the
   font. (The number of glyphs in the font is passed back to the client via the
   "top" parameter to bzrBegFont() in the "sup.nGlyphs" field.) Glyph data is
   passed back to the client via the callbacks specified via the "glyph_cb"
   parameter (see absfont.h). 

   Each glyph is introduced by calling the beg() glyph callback. The "info"
   parameter passed by this call provides a means of identifying each glyph.

   The "tag" field is the order number of the bez paths seen in the source 
   stream starting with 0.

   The client can control whether path data is read or ignored by the value
   returned from the beg() callback and can thus use this interface to select a
   subset of glyphs or just enumerate the glyph set without reading any path
   data. */

int bzrGetGlyphByTag(bzrCtx h, 
                     unsigned short tag, abfGlyphCallbacks *glyph_cb); 

/* bzrGetGlyphByTag(), is called obtain glyph data from a glyph selected by 
   its tag (described above).
 
   These functions return bzrErrNoGlyph if the requested glyph is not present
   in the font or the access method is incompatible with the font type. */

int bzrResetGlyphs(bzrCtx h);

/* bzrResetGlyphs() may be called at any time after the bzrIterateGlyphs() or
   bzrGetGlyphBy*() functions have been called to reset the record of the
   glyphs seen (called back) by the client. This is achieved by clearing the
   ABF_GLYPH_SEEN bit in the abfGlyphInfo flags field of each glyph. */

int bzrEndFont(bzrCtx h);

/* bzrEndFont() is called to terminate a bez parse initiated with
   bzrBegFont(). The source data stream is closed. */

void bzrFree(bzrCtx h);

/* bzrFree() destroys the library context and all the resources allocated to
   it. The debug data stream is closed. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "bzrerr.h"
    bzrErrCount
    };

/* Library functions return either zero (bzrSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from bzrerr.h. */

char *bzrErrStr(int errcode);

/* bzrErrStr() maps the "errcode" parameter to a null-terminated error 
   string. */

void bzrGetVersion(ctlVersionCallbacks *cb);

/* bzrGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */


#ifdef __cplusplus
}
#endif

#endif /* BEZREAD_H */

