/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type 2 charstring services.
 */

#ifndef T2CSTR_H
#define T2CSTR_H

#include "ctlshare.h"

#define T2C_VERSION CTL_MAKE_VERSION(1,0,20)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
    {
    long flags;
#define T2C_WIDTH_ONLY  (1<<0)
#define T2C_USE_MATRIX  (1<<1)
#define T2C_UPDATE_OPS  (1<<2)
#define T2C_IS_CUBE  (1<<3)
#define T2C_FLATTEN_CUBE  (1<<4)
#define T2C_CUBE_GSUBR (1<<5) /* current charstring is a subr. */
#define T2C_CUBE_RND (1<<6) /* start (x,y) for a Cube element is rounded to a multiple of 4: used when building real Cube host fonts. Required to be rasterized by PFR. */
#define T2C_IS_CFF2 (1<<7)
#define T2C_FLATTEN_BLEND (1<<8)
        
    void *src;
    ctlStreamCallbacks *stm;
    ctlSubrs subrs;
    ctlSubrs gsubrs;
	long gsubrsEnd;
	long subrsEnd;
    float defaultWidthX;
    float nominalWidthX;
    void *ctx;
    long (*getStdEncGlyphOffset)(void *ctx, int stdcode);
    unsigned char bchar;
    unsigned char achar;
    float matrix[6];
    float WV[4];
    void *dbg;
    unsigned short default_vsIndex; /* this is the vsindex set in the private dict. */
    struct var_itemVariationStore_ *varStore;
    float *scalars;
    } t2cAuxData;

typedef struct cff2GlyphCallbacks_ cff2GlyphCallbacks;

struct cff2GlyphCallbacks_
    {
    void *direct_ctx;
    float (*getWidth)   (cff2GlyphCallbacks *cb, unsigned short gid);
    };

int t2cParse(long offset, long endOffset, t2cAuxData *aux, unsigned short gid, cff2GlyphCallbacks *cff2, abfGlyphCallbacks *glyph, ctlMemoryCallbacks *mem);

/* t2cParse() is called to parse a Type 2 charstring into its constituent
   parts.

   The "offset" prameter specifies the stream offset of the charstring to be
   parsed.

   The "endOffset" prameter specifies the stream offset of the end of charstring to be
   parsed; it points one byte past the end end of the charstring.

   The "aux" parameter specifies charstring parse data that is normally stable
   across charstrings. A client may therefore set this up once before parsing
   all the charstrings in a font. The "aux" parameter fields are described
   below.

   The "gid" prameter specifies the glyph ID.

   The "cff2" parameter specifies the context for reading SFNT tables
   for metrics and variation font data needed if this is a CFF2 font.

   The "flags" field allows client control over the parse by setting bits as
   follows:

   T2C_WIDTH_ONLY - terminate parse after calling back the width

   T2C_USE_MATRIX - transform path coordinates using the "matrix" field before
   they are called back to the client. This facility would not normally be used
   during font format conversion since it is generally better to leave the path
   coordinates unmodified and specify a non-default matrix in the font

   T2C_UPDATE_OPS - charstrings using the deprecated seac operator are
   converted to a regular path by combining the component glyphs specified in
   the charstring and the deprecated dotsection operator is removed

   T2C_UPDATE_OPS - charstrings using the deprecated seac operator are
   converted to a regular path by combining the component glyphs specified in
   the charstring and the deprecated dotsection operator is removed

   T2C_IS_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. Cube subr's are found at the end of gsubr's
   for CID fonts, subrs for non-CID fonts.
	   
   T2C_FLATTEN_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. The Cube compose ops must be flattened.

   The "getStdEncGlyphOffset" field holds the address of a function that will
   return the stream offset of a standard encoded glyph. It is only called when
   the T2C_UPDATE_OPS bit is set and a seac operator is encountered. The glyph
   is specified as a character code in StandardEncoding via the "stdcode"
   parameter which is merely acts a synonym for the glyph's name. A client
   should return the stream offset of a glyph whose name is at that standard
   code point or -1 if an error is encountered. The function passes a copy of
   the "ctx" field via the function's "ctx" parameter.

   The "bchar" and "achar" fields specify the base and accent standard
   character codes for seac charstrings, respectively. Non-zero values for
   either of these fields after return from t1cParse() indicates that the
   charstring contained a seac operator. A client building a subset font may
   use this information to ensure that the glyphs with the standard encodings
   of "achar" and "bchar" are added to the subset.

   Charstring data is obtained via a stream interface specified by the
   "src" stream field and the stream callbacks specified via the "stm"
   field. The stream is assumed to have been opened by the client and will be
   closed by the client. t2cParse() only make seek() and read() calls on the
   source stream.

   Type 2 charstrings are typically subroutinized with both local and global
   subroutines whose offsets are specified by the "subrs" and "gsubrs"
   fields, respectively (see ctlshare.h).

   The "defaultWidthX" and "nominalWidthX" fields are copied from the relevant
   CFF Private dictionary data and are used to compute correct widths from
   the charstring data.

   The "WV" field provides support for a private implementation of multiple
   master substitution fonts which are assumed to have 4 master designs. This
   field may be left uninitialized by clients who don't need to support
   substitution.

   The glyph callback functions are specified via the "glyph" parameter (see
   absfont.h). The glyph callbacks pass path construction data back to the
   client. However, the glyph.new() and glyph.end() callbacks are not called
   from t2cParse() since the client is directly initiating the callbacks.

   The parse proceeds until one of the following conditions:

   o an endchar operator is encountered in the charstring or a subroutine
     (t2cSuccess is returned)
   o the end of the charstring is encountered.
   o the width has be called back to the client and the T2C_WIDTH_ONLY bit 
     is set (t1cSuccess is returned)
   o an error is encountered and returned

   t2cParse() returns zero (t2cSuccess) if successful, but otherwise returns a
   positive non-zero error code described below. 

   The gsubr array must have one more entry than there actually are gsubrs, in order
   to allow getting the end of the last gubsr.

   A charstring should terminate with one of the "endchar", "callsubr", or
   "callgsubr" operators. A subroutine should terminate with one of the
   preceding operators or "return". */ 

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "t2cerr.h"
    t2cErrCount
    };

/* Library functions return either zero (t2cSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from t2cerr.h. */

char *t2cErrStr(int err_code);

/* t2cErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void t2cGetVersion(ctlVersionCallbacks *cb);

/* t2cGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* T2CSTR_H */
