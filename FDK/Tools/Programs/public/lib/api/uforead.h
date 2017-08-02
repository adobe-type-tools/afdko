/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef UFOEAD_H
#define UFOEAD_H

#include "ctlshare.h"

#define UFO_VERSION CTL_MAKE_VERSION(1,0,14)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/*SVG Font Parser Library
   =======================================
   This library parses parses UFO fonts. 

   This library is initialized with a single call to ufoNew() that allocates
   an opaque context (ufoCtx) which is passed to subsequent functions and is
   destroyed by a single call to ufoFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Each font parse is begun by calling ufoBegFont() and completed by calling
   ufoEndFont(). Between these calls one or more glyphs may be parsed with the
   ufoIterateGlyphs(), ufoGetGlyphByTag(), and ufoGetGlyphByName() functions. 

   Memory management and source data functions are provided by two sets of
   client-supplied callbacks described in ctlshare.h. */

typedef struct ufoCtx_ *ufoCtx;
ufoCtx ufoNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define UFO_CHECK_ARGS CTL_CHECK_ARGS_CALL(UFO_VERSION)

/* ufoNew() initializes the library and returns an opaque context (ufoCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The UFO_CHECK_ARGS macro is passed as the last parameter to ufoNew() in
   order to perform a client/library compatibility check. 

   The temporary data stream (UFO_TMP_STREAM_ID) and the optional debug data
   stream (UFO_DBG_STREAM_ID) are opened by this call. The debug stream
   provides more detailed error and warning messages than is available via
   ufoErrStr(). If the client doesn't require the debug data stream NULL should
   be returned from its stream open call. */

int ufoBegFont(ufoCtx h, 
               long flags, abfTopDict **top, char * altLayerDir);

#define UFO_SEEN_GLYPH (1<<) /* have seen a glyph */

/* ufoBegFont() is called to initiate a new font parse. The source data stream
   (UFO_SRC_STREAM_ID) is opened, positioned at the offset specified by the
   "origin" parameter, the SVG source is parsed, and selected
   dictionary values are stored in the data structure pointed to by the "top"
   parameter (which will remain stable until ufoEndFont() is called).
   Charstring and subroutine data is then stored in the temporary stream for subsequent parsing.

  */     

int ufoIterateGlyphs(ufoCtx h, abfGlyphCallbacks *glyph_cb);

/* ufoIterateGlyphs() is called to iterate through all the glyph data in the
   font. (The number of glyphs in the font is passed back to the client via the
   "top" parameter to ufoBegFont() in the "sup.nGlyphs" field.) Glyph data is
   passed back to the client via the callbacks specified via the "glyph_cb"
   parameter (see absfont.h). 

   Each glyph is introduced by calling the beg() glyph callback. The "info"
   parameter passed by this call provides a means of identifying each glyph.

   The "tag" field is the order number of the charstring
   in the CharStrings dictionary, starting from 0 and the "gname" field
   specifies the glyph's name.

   The client can control whether path data is read or ignored by the value
   returned from the beg() callback and can thus use this interface to select a
   subset of glyphs or just enumerate the glyph set without reading any path
   data. */

int ufoGetGlyphByTag(ufoCtx h, 
                     unsigned short tag, abfGlyphCallbacks *glyph_cb); 
int ufoGetGlyphByName(ufoCtx h, 
                      char *gname, abfGlyphCallbacks *glyph_cb);

/* ufoGetGlyphByTag(), ufoGetGlyphByName() are called
   obtain glyph data from a glyph selected by its tag (described above) or its
   name respectively. The glyph name is specified by as a
   null-terminated string via the "gname" parameter. 

   These functions return ufoErrNoGlyph if the requested glyph is not present
   in the font or the access method is incompatible with the font type. */


int ufoResetGlyphs(ufoCtx h);

/* ufoResetGlyphs() may be called at any time after the ufoIterateGlyphs() or
   ufoGetGlyphBy*() functions have been called to reset the record of the
   glyphs seen (called back) by the client. This is achieved by clearing the
   ABF_GLYPH_SEEN bit in the abfGlyphInfo flags field of each glyph.

   A client wishing to have direct access to the charstring data may do so by
   utilizing the tmp stream (UFO_TMP_STREAM_ID) offsets for subroutines and
   charstrings. The per-Private-dictionary subroutine data is available via
   ufoGetSubrs() (see below). The per-glyph charstring information is available
   in the "abfGlyphInfo.sup" field. In order to obtain per-glyph region data
   the client must ask for glyph data via one of the 5 functions specified
   above and then issue an ABF_SKIP_RET from the glyphBeg() callback so that
   the charstring is not parsed and called back in the usual manner. */


int ufoEndFont(ufoCtx h);

/* ufoEndFont() is called to terminate a font parse initiated with
   ufoBegFont(). The source data stream is closed. */

void ufoFree(ufoCtx h);

/* ufoFree() destroys the library context and all the resources allocated to
   it. The temporary and debug data streams are closed. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "ufoerr.h"
    ufoErrCount
    };

/* Library functions return either zero (ufoSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from ufoerr.h. */

char *ufoErrStr(int err_code);

/* ufoErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void ufoGetVersion(ctlVersionCallbacks *cb);

/* ufoGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* UFOEAD_H */
