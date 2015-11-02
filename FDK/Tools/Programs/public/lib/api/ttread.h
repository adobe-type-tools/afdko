/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef TTREAD_H
#define TTREAD_H

#include "ctlshare.h"

#define TTR_VERSION CTL_MAKE_VERSION(1,0,21)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TrueType Font Parsing Library
   =============================
   This library parses outline font information in TrueType fonts. Multiple
   TrueType fonts packaged as a TrueType Collection are also supported.

   The library is initialized with a single call to ttrNew() which allocates an
   opaque context (ttrCtx) which is passed to subsequent functions and is
   destroyed by a single call to ttrFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   A new font is opened and parsed and top level font data is returned by
   calling ttrBegFont(). Glyph data may then be accessed using the
   ttrIterateGlyphs(), ttrGetGlyphByTag(), and ttrGetGlyphByName(). Finally, a
   font may be closed by calling ttrEndFont(). */

typedef struct ttrCtx_ *ttrCtx;
ttrCtx ttrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb, 
              CTL_CHECK_ARGS_DCL);

#define TTR_CHECK_ARGS  CTL_CHECK_ARGS_CALL(TTR_VERSION)

/* ttrNew() initializes the library and returns an opaque context (ttrCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The TTR_CHECK_ARGS macro is passed as the last parameter to ttrNew() in
   order to perform a client/library compatibility check. */

int ttrBegFont(ttrCtx h, long flags, long origin, int iTTC, abfTopDict **top);

/* ttrBegFont() is called to initiate a new font parse. The source data stream
   (CFR_SRC_STREAM_ID) is opened, positioned and the offset specified by the
   "origin" parameter, parsed, and global font data is stored in the data
   structure pointed to by the "top" parameter. The "flags" parameter specifies
   parse options: */

enum
    {
    TTR_EXACT_PATH  = 1<<0, /* Return mathematically exact path conversion */
    TTR_BOTH_PATHS  = 1<<1  /* Return combined approx and exact conversions */
    };

/* TrueType curve segments are represented as quadratic Beziers but the path
   returned by the glyph callbacks described below represent curve segments
   using cubic Beziers. By default this conversion is approximated by combining
   ajoining pairs of quadratic segments into a single cubic segment thus
   significantly reducing the number of segments required to represent the
   glyph outline. If, for some reason, a client requires an exact conversion
   the TTR_EXACT_PATH may be specified via the "flags" parameter.
   TTR_BOTH_PATHS returns the combined approximate and exact path conversions.
   This is really only useful for comparing the quality of the approximate
   conversion compared to the exact conversion, by overprinting, for example.
   
   When parsing a TrueType Collection (TTC) the "iTTC" parameter may be used to
   index a specific font within the TTC TableDirectory. The "iTTC" must be set
   to zero when parsing regular TrueType fonts. */

int ttrIterateGlyphs(ttrCtx h, abfGlyphCallbacks *glyph_cb);

/* ttrIterateGlyphs() is called to interate through all the glyph data in the
   font. (The number of glyphs in the font is passed back to the client via the
   "top" parameter to ttrBegFont() in the sup.nGlyphs field.) Glyph data is
   passed back to the client via the callbacks specified via the "glyph_cb"
   parameter (see absfont.h). 

   Glyphs are presented to the client in glyph index order. Each glyph may
   be identified from the "info" parameter that is passed back to the client
   via the beg() callback (the current glyph index is specified via the "tag"
   field). 

   The client can control whether path data is read or ignored by the value
   returned from beg() and can thus use this interface to select a subset of
   glyphs or just enumerate the glyph set without reading any path data. */

int ttrGetGlyphByTag(ttrCtx h, 
                     unsigned short tag, abfGlyphCallbacks *glyph_cb);
int ttrGetGlyphByName(ttrCtx h, 
                      char *gname, abfGlyphCallbacks *glyph_cb);

/* ttrGetGlyphByTag() and ttrGetGlyphByName() are called obtain glyph data from
   a glyph selected by its tag value (glyph index) or its name, respectively.
   The glyph name is specified by as a null-terminated string via the "gname"
   parameter.

   These functions return ttrErrNoGlyph if the requested glyph is not present
   in the font. */

int ttrResetGlyphs(ttrCtx h);

/* ttrResetGlyphs() may be called at any time after the ttrIterateGlyphs() or
   ttrGetGlyphBy*() functions have been called to reset the record of the
   glyphs seen (called back) by the client. This is achieved by clearing the
   ABF_GLYPH_SEEN bit in the abfGlyphInfo flags field of each glyph. */

int ttrEndFont(ttrCtx h);

/* ttrEndFont() is called to terminate a font parse initiated with
   ttrBegFont(). The input stream is closed as a result of calling this
   function. */

void ttrFree(ttrCtx h);

/* ttrFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "ttrerr.h"
    ttrErrCount
    };

/* Library functions return either 0 to indicate success or a positive non-zero
   error code that is defined in the above enumeration that is built from
   ttrerr.h. */

char *ttrErrStr(int err_code);

/* ttrErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void ttrGetVersion(ctlVersionCallbacks *cb);

/* ttrGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* TTREAD_H */
