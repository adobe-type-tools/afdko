/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFFWRITE_H
#define CFFWRITE_H

#include "ctlshare.h"

#define CFW_VERSION CTL_MAKE_VERSION(1,0,50)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compact Font Format (CFF) Generation Library
   ============================================
   This library allows the generation of CFF FontSets. Regular and synthetic
   name-based (Type 1) and registry-based (CID-keyed) fonts are supported as
   members of single- or multiple-font FontSets.

   This library is initialized with a single call to cfwNew() which allocates a
   opaque context (cfwCtx) which is passed to subsequent functions and is
   destroyed by a single call to cfwFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may call other functions in the
   interface in order to construct the desired FontSet.

   Memory management and data I/O are implemented via a number of
   client-supplied callback functions passed to cfwNew() via the cfwCallbacks
   data structure.

   Library I/O is performed on two abstract data streams: 
   
   o cff FontSet data output
   o temporary data input and output

   These streams are managed by a single set of client callback functions
   enabling the client to choose from a wide variety of implementation schemes
   ranging from disk files to memory buffers.

   The temporary stream is used to keep the run-time memory usage within
   resonable limits when handling large fonts. */

typedef struct cfwCtx_ *cfwCtx;
cfwCtx cfwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define CFW_CHECK_ARGS  CTL_CHECK_ARGS_CALL(CFW_VERSION)

/* cfwNew() initializes the library and returns an opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes sets
   memory and stream callback functions (described in ctlshare.h) to the
   library via the mem_cb and stm_cb parameters.

   The CFW_CHECK_ARGS macro is passed as the last parameter to cfwNew() in
   order to perform a client/library compatibility check. */

int cfwBegSet(cfwCtx h, long flags);

/* cfwBegSet() is called to begin a new FontSet specification. The flags
   parameter specifies FontSet creation options below: */

enum
    {
    CFW_SUBRIZE         = 1<<0, /* Subroutinize member fonts' charstrings */
    CFW_EMBED_OPT       = 1<<1, /* Perform embedding optimization */
    CFW_ROM_OPT         = 1<<2, /* Perform ROM-resident FontSet optimization */
    CFW_NO_DEP_OPS      = 1<<3, /* Remove/convert deprecated ops */
    CFW_NO_FAMILY_OPT   = 1<<4, /* Don't apply family value optimizations */
    CFW_WARN_DUP_HINTSUBS = 1<<5, /* Report duplicate hintsubs warnings */
    CFW_PRESERVE_GLYPH_ORDER = 1<<6, /* Preserve glyph addition order */
    CFW_CHECK_IF_GLYPHS_DIFFER = 1<<7, /* When adding a charstring, check if
                                        it is already present and same or different. */
    CFW_ENABLE_CMP_TEST = 1<<8,  /* Enable testing by disabling optimizations */
	CFW_IS_CUBE    = 1<<9,
	
    /* When bit 9 is set, the data contains cube font operators. Stack depth
	and operator defs are different. Cube subr's are added at the end of gsubr's for CID fonts,
	subrs for non-CID fonts. */
	
	CFW_FORCE_STD_ENCODING    = 1<<10,
   /* When bit 10 is set, TopDict encoding vector is set to StandardEncoding, no matter
	what is in the font. This is useful for working with font sources that will be used for
	OpenType/CFF fonts */     

    CFW_CUBE_RND    = 1<<11,
    CFW_NO_OPTIMIZATION   = 1<<12,
    CFW_WRITE_CFF2   = 1<<13,
    CFW_NO_FUTILE_SUBRS = 1<<14  /* Remove futile subroutines during subroutinization */
    };

/* If the CFW_PRESERVE_GLYPH_ORDER bit is not set, glyphs are accumulated and
   then sorted during cfwEndFont(), into an order that reduces the internal CFF
   charset and encoding data structure sizes.

   Conversely, if the CFW_PRESERVE_GLYPH_ORDER bit is set, the order in which
   glyphs are added to the font via the glyph callbacks is preserved. However,
   in either of these cases the .notdef glyph is special and is always assigned
   a glyph index of 0. 

   The CFW_CHECK_IF_GLYPHS_DIFFER allows clients to merge two or more fonts
   together. Note that the client is responsible for ensuring that the fonts
   are sufficiently compatible that the merged glyphs play well together.

   If the CFW_ENABLE_CMP_TEST bit is set, several necessary optimization are
   disabled so as to enable rendering tests to succeed. This must never be
   used by production code.
   
   If the CFW_SUBRIZE bit is set, repeated patterns in charstrings are extracted
   as subroutines in order to minimize the total font size. Since this process
   is both memory and CPU intensive, this option should be used cautiously. */

typedef struct cfwMapCallback_ cfwMapCallback;
struct cfwMapCallback_
    {
    void *ctx;
    void (*glyphmap)(cfwMapCallback *cb, 
                     unsigned short gid, abfGlyphInfo *info);
    };

int cfwBegFont(cfwCtx h, cfwMapCallback *map, unsigned long maxNumSubrs);

/* cfwBegFont() is called to begin a new member font definition.

   The "map" parameter specifies an optional glyph mapping callback that
   provides information to the client about how the glyphs have been allocated
   in the CFF CharStrings INDEX. This facility may be disabled by setting the
   "map" parameter to NULL.

   If "map" is non-NULL and the CFW_PRESERVE_GLYPH_ORDER bit is set the
   glyphmap() callback function is called during the glyphEnd() callback thus
   enabling clients to get information about the glyph just added (like its
   glyph index).

   If "map" is non-NULL and the CFW_PRESERVE_GLYPH_ORDER bit is NOT set the
   glyphmap() callback function is called during cfwEndFont() after the glyphs
   have been sorted into order. Each glyph is called back, in order, beginning
   with the first at GID 0.

   The glyphmap() "gid" parameter specifies the glyph's index and the
   "info" parameter identifies the glyph. */

extern const abfGlyphCallbacks cfwGlyphCallbacks;

/* cfwGlyphCallbacks is a glyph callback set template that will add the data
   passed to these callbacks to the current font. Clients should make a copy of
   this data structure and set the "direct_ctx" field to the context returned
   by cfwNew(). 

   The client must keep the glyph information, passed via the "info" parameter
   to the glyphBeg() callback, stable until after cfwEndFont() returns. */

int cfwCompareFDArrays(cfwCtx h, abfTopDict *srcTop);

/* cfwCompareFDArrays() compares the FDArray of the source font with the the
   FDArray of the destination font. If a source font dict has a FontName
   different than that in the font dict in the destination font, then it is
   skipped. If there is a font dict in the destination font which has the same
   name, then the two font dicts are compared. If any source font dict is not
   compatible with a destination font dict of the same FontName, the function
   returns 1 else it returns 0. */

int cfwMergeFDArray(cfwCtx h, abfTopDict *top, int *newFDIndex);

/* cfwMergeFDArray() merges all the fdArray font dicts of the passed-in source
   font top dict into the destination font. It also fills in the array pointed
   to by newFDIndex such that newFDIndex[source font dict index] == destination
   font dict index. The newFDIndex array must be allocated by the client, and
   must be large enough to hold an array of length top->FDArray.cnt. If the
   destination font has a font dict with the same name as a source font dict,
   then source font dict is assumed to be compatible to the destination font
   dict, and is not merged into the destination font; otherwise, the source
   font dict as added to the destination font as a new font dict.

   if the cffwrite module error code is set before or during the merging
   process, then cfwMergeFDArray() will set the destination font iFD to
   ABF_UNSET_INT for all remaining font dicts, and will return the error code.

   It is assumed that source and destination font dicts with the same name are
   compatible; the function cfwCompareFDArrays can be used to verify this.

   The client must then set the abfGlyphInfo->iFD field to the destination font
   FDArray index, as returned by cfwMergeDict, after reading from the source
   font, and before calling the destination font glyphBeg function.

   Note! Once cfwMergeTopDict() has ben called, the cffwrite module will NOT
   merge in the original source font top dicts when fontEnd is called. If
   cfwMergeTopDict() is called for any font dict of any source font, then it
   must be called for every font dict of every source font, and the
   abfGlyphInfo->iFD value of each source glyph must be set to the destination
   font iFD value for the font dict before the glyph callbacks are used.

   Note! cfwMergeTopDict() should not be called for fonts which do not meet the
   test: font is not CID and both font have only one dict in the FDArray This
   is becuase the cfwMergeTopDict() function assumes that fontDicts with
   different names must be treated as different font dicts. For non_CID fonts
   with one font dict, this is not the case; subset fonts will have fontDicts
   with different prefixes, but which should be considered equivalent. If
   cfwMergeTopDict() is used with two subset fonts, the destination font will
   incorrectly get two font dicts. */

int cfwGetErrCode(cfwCtx h);

/* cfwGetErrCode() returns any error flags currently set in the cfwCtx. It can
   be called at any time, and does not change the state of the cfwCtx */

int cfwEndFont(cfwCtx h, abfTopDict *top);

/* cfwEndFont() completes the definition of the font that commenced with
   cfwBegFont(). The "top" pararmeter specifies global font information via the
   data structures described in absfont.h. 

   A client can abandon a font part way through its definition by calling
   cfwEndFont() with  a "top" parameter value of NULL. 

   When cfwEndFont() returns the client no longer needs to keep the glyph
   information, passed via the "info" parameter to the glyphBeg() callback,
   stable. */

int cfwEndSet(cfwCtx h);

/* cfwEndSet() completes the definition of the FontSet that was initiated with
   cfwBegSet(); */

void cfwFree(cfwCtx h);

/* cfwFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "cfwerr.h"
    cfwErrCount
    };

/* Library functions return either zero (cfwSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from cfwerr.h. */

char *cfwErrStr(int err_code);

/* cfwErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void cfwGetVersion(ctlVersionCallbacks *cb);

/* cfwGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* CFFWRITE_H */
