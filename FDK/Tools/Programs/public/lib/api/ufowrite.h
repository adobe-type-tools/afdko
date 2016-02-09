/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef UFOWRITE_H
#define UFOWRITE_H

#include "ctlshare.h"

#define UFW_VERSION CTL_MAKE_VERSION(1,0,4)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UFO Font Format Generation Library
   =====================================

   This library supports the generation of UFO fonts from an abstract font.

   This library is initialized with a single call to ufwNew() which allocates a
   opaque context (ufwCtx) which is passed to subsequent functions and is
   destroyed by a single call to ufwFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may call other functions in the
   interface in order to generate the desired font.

   Memory management and stream I/O are implemented via two sets of
   client-supplied callback functions passed to ufwNew(). I/O is performed on
   two abstract data streams:
   
   o UFO font data (w)
   o debug diagnostics (w)

   These streams are managed by a single set of client callback functions
   enabling the client to choose from a wide variety of implementation schemes
   ranging from disk files to memory buffers. The temporary stream is used to
   keep the run-time memory usage within resonable limits when handling large
   fonts.

   Glyph data is passed to the library via the set of glyph callback functions
   defined in ufwGlyphCallbacks. */

typedef struct ufwCtx_ *ufwCtx;
ufwCtx ufwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define UFW_CHECK_ARGS  CTL_CHECK_ARGS_CALL(UFW_VERSION)

/* ufwNew() initializes the library and returns an opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes sets
   of memory and stream callback functions (described in ctlshare.h) to the
   library via the "mem_cb" and "stm_cb" parameters.

   The UFW_CHECK_ARGS macro is passed as the last parameter to ufwNew() in
   order to perform a client/library compatibility check. */

int ufwBegFont(ufwCtx h, long flags, char *glyphLayerDir);

/* ufwBegFont() is called to begin a font definition. The "flags" parameter
   provides control over certain attributes of the generated font with the
   options enumerated below. Returns ufwSuccess on success. */

enum /* enum too provide values for flags field. */
    {
    UFW_GLYPHNAMES_NONE     =    0, /* Include no glyph names (the default) */
    };

extern const abfGlyphCallbacks ufwGlyphCallbacks;

/* ufwGlyphCallbacks is a glyph callback set template that will add the data
   passed to these callbacks to the current font. Clients should make a copy of
   this data structure and set the "direct_ctx" field to the context returned
   by ufwNew().
  
   Clients need to be aware that certain values are not valid in an XML 1.0 file.
   Because of this the library will skip glyphs that have a unicode value 
   that is equal to an invalid character.  These include all control chars (values
   less than 0x20) except 0x9, 0xa, 0xd which represent whitespace. */

int ufwEndFont(ufwCtx h, abfTopDict *top);

/* ufwEndFont() completes the definition of the font that commenced with
   ufwBegFont(). The "top" parameter specifies global font information via the
   data structures described in absfont.h. */ 

void ufwFree(ufwCtx h);

/* ufwFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "ufowerr.h"
    ufwErrCount
    };

/* Library functions return either zero (ufwSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from ufwerr.h. */

char *ufwErrStr(int err_code);

/* ufwErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void ufwGetVersion(ctlVersionCallbacks *cb);

/* ufwGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* UFOWRITE_H */
