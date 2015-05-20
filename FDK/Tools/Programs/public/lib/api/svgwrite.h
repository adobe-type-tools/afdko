/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SVGWRITE_H
#define SVGWRITE_H

#include "ctlshare.h"

#define SVW_VERSION CTL_MAKE_VERSION(1,1,10)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SVG Font Format Generation Library
   =====================================

   This library supports the generation of SVG fonts from an abstract font.

   This library is initialized with a single call to svwNew() which allocates a
   opaque context (svwCtx) which is passed to subsequent functions and is
   destroyed by a single call to svwFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may call other functions in the
   interface in order to generate the desired font.

   Memory management and stream I/O are implemented via two sets of
   client-supplied callback functions passed to svwNew(). I/O is performed on
   two abstract data streams:
   
   o SVG font data (w)
   o debug diagnostics (w)

   These streams are managed by a single set of client callback functions
   enabling the client to choose from a wide variety of implementation schemes
   ranging from disk files to memory buffers. The temporary stream is used to
   keep the run-time memory usage within resonable limits when handling large
   fonts.

   Glyph data is passed to the library via the set of glyph callback functions
   defined in svwGlyphCallbacks. */

typedef struct svwCtx_ *svwCtx;
svwCtx svwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define SVW_CHECK_ARGS  CTL_CHECK_ARGS_CALL(SVW_VERSION)

/* svwNew() initializes the library and returns an opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes sets
   of memory and stream callback functions (described in ctlshare.h) to the
   library via the "mem_cb" and "stm_cb" parameters.

   The SVW_CHECK_ARGS macro is passed as the last parameter to svwNew() in
   order to perform a client/library compatibility check. */

int svwBegFont(svwCtx h, long flags);

/* svwBegFont() is called to begin a font definition. The "flags" parameter
   provides control over certain attributes of the generated font with the
   options enumerated below. Returns svwSuccess on success. */

enum
    {
    SVW_GLYPHNAMES_NONE     =    0, /* Include no glyph names (the default) */
    SVW_GLYPHNAMES_ALL      = 1<<0, /* Include glyph names with all glyphs */
    SVW_GLYPHNAMES_NONASCII = 1<<1, /* Include glyph names with non-ASCII chars only */
    SVW_GLYPHNAMES_MASK     = 3<<0,

    /* If bit 0 is set, then glyph name attributes will be written out for all
	   glyphs. If bit 1 is set, then glyph names will only be written out for
	   non-ASCII characters only. */

    SVW_NEWLINE_UNIX        = 1<<2, /* \n */
    SVW_NEWLINE_WIN         = 1<<3, /* \r\n */
    SVW_NEWLINE_MAC         = 1<<4, /* \r */
    SVW_NEWLINE_MASK        = 7<<2,

    /* One bit from bits 2-4 must be set to encode the newline type to match
       one of 3 common platform conventions. This is just a styling convenience
       and doesn't affect the the operation of the font, regardless of
       platform. */

    SVW_STANDALONE          = 1<<5,

	/* Indicates that the SVG font should be written as a standalone SVG file
	   rather than as a font that will be embedded within another SVG file. */
	   
	SVW_ABSOLUTE			= 1<<6,
	/* Indicates that coordinates should be written as absolute rather than relative. */
	   
    };

extern const abfGlyphCallbacks svwGlyphCallbacks;

/* svwGlyphCallbacks is a glyph callback set template that will add the data
   passed to these callbacks to the current font. Clients should make a copy of
   this data structure and set the "direct_ctx" field to the context returned
   by svwNew().
  
   Clients need to be aware that certain values are not valid in an XML 1.0 file.
   Because of this the library will skip glyphs that have a unicode value 
   that is equal to an invalid character.  These include all control chars (values
   less than 0x20) except 0x9, 0xa, 0xd which represent whitespace. */

int svwEndFont(svwCtx h, abfTopDict *top);

/* svwEndFont() completes the definition of the font that commenced with
   svwBegFont(). The "top" parameter specifies global font information via the
   data structures described in absfont.h. */ 

void svwFree(svwCtx h);

/* svwFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "svwerr.h"
    svwErrCount
    };

/* Library functions return either zero (svwSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from svwerr.h. */

char *svwErrStr(int err_code);

/* svwErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void svwGetVersion(ctlVersionCallbacks *cb);

/* svwGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* SVGWRITE_H */
