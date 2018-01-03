/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef T1READ_H
#define T1READ_H

#include "ctlshare.h"

#define T1R_VERSION CTL_MAKE_VERSION(1,0,41)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type 1 (PostScript) Font Parser Library
   =======================================
   This library parses parses single master, multiple master, synthetic, and
   CID-keyed Type 1 fonts. 

   This library is initialized with a single call to t1rNew() that allocates
   an opaque context (t1rCtx) which is passed to subsequent functions and is
   destroyed by a single call to t1rFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Each font parse is begun by calling t1rBegFont() and completed by calling
   t1rEndFont(). Between these calls one or more glyphs may be parsed with the
   t1rIterateGlyphs(), t1rGetGlyphByTag(), t1rGetGlyphByName(),
   t1rGetGlyphByCID() or t1cGetByStdEnc() functions. Weight vector and
   subroutine information may be obtained by calling t1rGetWV() and
   t1rGetSubrs(), respectively.

   Multiple master fonts are always snapshot to a client-supplied instance that
   is set via t1rBegFont(). 

   Memory management and source data functions are provided by two sets of
   client-supplied callbacks described in ctlshare.h. */

typedef struct t1rCtx_ *t1rCtx;
t1rCtx t1rNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define T1R_CHECK_ARGS CTL_CHECK_ARGS_CALL(T1R_VERSION)

/* t1rNew() initializes the library and returns an opaque context (t1rCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The T1R_CHECK_ARGS macro is passed as the last parameter to t1rNew() in
   order to perform a client/library compatibility check. 

   The temporary data stream (T1R_TMP_STREAM_ID) and the optional debug data
   stream (T1R_DBG_STREAM_ID) are opened by this call. The debug stream
   provides more detailed error and warning messages than is available via
   t1rErrStr(). If the client doesn't require the debug data stream NULL should
   be returned from its stream open call. */

int t1rBegFont(t1rCtx h, 
               long flags, long origin, abfTopDict **top, float *UDV);

#define T1R_UPDATE_OPS  (1<<0)
#define T1R_USE_MATRIX  (1<<1)
#define T1R_DUMP_TOKENS (1<<2)
#define T1R_KEEP_CID_CSTRS (1<<3)
#define T1R_NO_UDV_CLAMPING (1<<4)
#define T1R_IS_CUBE   (1<<5)
#define T1R_FLATTEN_CUBE   (1<<6)
#define T1R_SEEN_GLYPH (1<<7) /* have seen a glyph */

/* t1rBegFont() is called to initiate a new font parse. The source data stream
   (T1R_SRC_STREAM_ID) is opened, positioned at the offset specified by the
   "origin" parameter, the PostScript source is parsed, and selected
   dictionary values are stored in the data structure pointed to by the "top"
   parameter (which will remain stable until t1rEndFont() is called). If eexec
   or charstring encryption is present it is removed. Charstring and subroutine
   data is then stored in the temporary stream for subsequent parsing.

   The "flags" parameter allows client control over the parse by setting bits
   as follows:

   T1C_UPDATE_OPS - charstrings using the deprecated seac operator are
   converted to a regular path by combining the component glyphs and the
   deprecated dotsection operator is removed

   T1C_USE_MATRIX - transform charstring path coordinates using the font's own
   FontMatrix. This facility would not normally be used during font format
   conversion since it is generally better to leave the path coordinates
   unmodified and specify a non-default matrix in the font. However, it is
   useful for drawing and manipulating glyph outline data

   T1R_DUMP_TOKENS - dump PostScript tokens to stdout as they are parsed.
   Useful for debugging bad PostScript font files.

   T1R_KEEP_CID_CSTRS - In normal operation CID charstrings are decrypted and
   copied to a fixed position in the tmp stream for the purpose of parsing the
   charstring data. Thus charstring data in the tmp stream is normally only
   valid while the glyph is called back. This is an optimization that limits
   the total size of the tmp stream for potentially large CID fonts. However,
   this may be undesirable behavior for some specialized clients, e.g. a
   rasterizer, and therefore it may be overridden by setting this flag.

   The "UDV" parameter specifies the User Design Vector to be used in
   snapshotting a multiple master font. If NULL, the font is snapshot at the
   default instance. The parameter may be set to NULL for non-multiple master
   fonts.
   
   T1R_IS_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. Cube subr's are found at the end of gsubr's
   for CID fonts, subrs for non-CID fonts.
	   
   T1R_FLATTEN_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. The Cube compose ops must be flattened.
*/     

int t1rIterateGlyphs(t1rCtx h, abfGlyphCallbacks *glyph_cb);

/* t1rIterateGlyphs() is called to interate through all the glyph data in the
   font. (The number of glyphs in the font is passed back to the client via the
   "top" parameter to t1rBegFont() in the "sup.nGlyphs" field.) Glyph data is
   passed back to the client via the callbacks specified via the "glyph_cb"
   parameter (see absfont.h). 

   Each glyph is introduced by calling the beg() glyph callback. The "info"
   parameter passed by this call provides a means of identifying each glyph.

   For CID-keyed fonts the "tag" field is the order number of the charstring in
   the data section starting with 0 and the "cid" field specifies the glyph's
   CID. (Thus, the tag and cid fields will generally differ for subset
   CID-keyed fonts.)

   For name-keyed fonts the "tag" field is the order number of the charstring
   in the CharStrings dictionary, starting from 0 and the "gname" field
   specifies the glyph's name.

   The client can control whether path data is read or ignored by the value
   returned from the beg() callback and can thus use this interface to select a
   subset of glyphs or just enumerate the glyph set without reading any path
   data. */

int t1rGetGlyphByTag(t1rCtx h, 
                     unsigned short tag, abfGlyphCallbacks *glyph_cb); 
int t1rGetGlyphByName(t1rCtx h, 
                      char *gname, abfGlyphCallbacks *glyph_cb);
int t1rGetGlyphByCID(t1rCtx h, 
                     unsigned short cid, abfGlyphCallbacks *glyph_cb);

/* t1rGetGlyphByTag(), t1rGetGlyphByName(), t1rGetGlyphByCID() are called
   obtain glyph data from a glyph selected by its tag (described above), its
   name, or its CID, respectively. The glyph name is specified by as a
   null-terminated string via the "gname" parameter. 

   These functions return t1rErrNoGlyph if the requested glyph is not present
   in the font or the access method is incompatible with the font type. */

int t1rGetGlyphByStdEnc(t1rCtx h, int stdcode, abfGlyphCallbacks *glyph_cb);

/* t1rGetGlyphByStdEnc() is called to obtain glyph data from a glyph selected
   by its standard encoding specified by the "stdcode" parameter (which is
   really an alias for a standard glyph name). It is primarily intended to be
   used when the client is subsetting a font that contains glyphs composed
   using the seac operator. When a client selects such a glyph they must ensure
   that the base and accent glyphs are also present in the subset. This may be
   achieved by calling t1rGetGlyphByStdEnc() with the "bchar" and "achar"
   parameters from the seac() glyph callback taking special note of the
   ABF_GLYPH_SEEN flags parameter so that repeated glyph definitions are
   avoided. */

int t1rResetGlyphs(t1rCtx h);

/* t1rResetGlyphs() may be called at any time after the t1rIterateGlyphs() or
   t1rGetGlyphBy*() functions have been called to reset the record of the
   glyphs seen (called back) by the client. This is achieved by clearing the
   ABF_GLYPH_SEEN bit in the abfGlyphInfo flags field of each glyph.

   A client wishing to have direct access to the charstring data may do so by
   utilizing the tmp stream (T1R_TMP_STREAM_ID) offsets for subroutines and
   charstrings. The per-Private-dictionary subroutine data is available via
   t1rGetSubrs() (see below). The per-glyph charstring information is available
   in the "abfGlyphInfo.sup" field. In order to obtain per-glyph region data
   the client must ask for glyph data via one of the 5 functions specified
   above and then issue an ABF_SKIP_RET from the glyphBeg() callback so that
   the charstring is not parsed and called back in the usual manner. */

const float *t1rGetWV(t1rCtx h, long *nMasters);

/* t1rGetWV() returns the weight vector that was computed from the user design
   vector supplied via the "UDV" parameter passed to t1rBegFont(). The weight
   vector is returned as a pointer to an array of "nMasters" elements. The
   "nMasters" parameter is set to 0 and the function returns NULL for single
   master fonts. */

const ctlSubrs *t1rGetSubrs(t1rCtx h, int iFD, const ctlRegion **region);

/* t1rGetSubrs() returns per-Private-dictionary subroutine offset data for the
   font dictionary specified by the "iFD" parameter. Temporary stream
   (T1R_TMP_STREAM_ID) offsets are returned by this functions via the ctlSubrs
   and ctlRegion structures. If the "iFD" is invalid the "region" parameter is
   set to NULL and the function returns NULL. */

int t1rEndFont(t1rCtx h);

/* t1rEndFont() is called to terminate a font parse initiated with
   t1rBegFont(). The source data stream is closed. */

void t1rFree(t1rCtx h);

/* t1rFree() destroys the library context and all the resources allocated to
   it. The temporary and debug data streams are closed. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "t1rerr.h"
    t1rErrCount
    };

/* Library functions return either zero (t1rSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from t1rerr.h. */

char *t1rErrStr(int err_code);

/* t1rErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void t1rGetVersion(ctlVersionCallbacks *cb);

/* t1rGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* T1READ_H */
