/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFFREAD_H
#define CFFREAD_H

#include "ctlshare.h"

#define CFR_VERSION CTL_MAKE_VERSION(2,0,52)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compact Font Format (CFF) Parser Library
   ========================================
   This library parses information in CFF FontSets containing only one single
   master or CID-keyed font.

   This library is initialized with a single call to cfrNew() which allocates
   an opaque context (cfrCtx) which is passed to subsequent functions and is
   destroyed by a single call to cfrFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   A new font is opened and parsed and top level font data is returned by
   calling cfrBegFont(). Glyph data may then be accessed using the
   cfrIterateGlyphs(), cfrGetGlyphByTag(), cfrGetGlyphByName(),
   cfrGetGlyphByCID() or t1cGetByStdEnc() functions. 

   The regions of the source stream occupied by various CFF data structs may be
   obtained by calling cfrGetSingleRegions() and cfrGetRepeatRegions(). The
   values associated with the "defaultWidthX" and "nominalWidthX" DICT
   operators may be obtained by calling cfrGetWidths().

   Finally, a font may be closed by calling cfrEndFont(). */

typedef struct cfrCtx_ *cfrCtx;
cfrCtx cfrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define CFR_CHECK_ARGS CTL_CHECK_ARGS_CALL(CFR_VERSION)

/* cfrNew() initializes the library and returns an opaque context (cfrCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The CFR_CHECK_ARGS macro is passed as the last parameter to cfrNew() in 
   order to perform a client/library compatibility check. 

   The optional debug data stream (CFR_DBG_STREAM_ID) is opened by this call.
   The debug stream provides more detailed error and warning messages than is
   available via cfrErrStr(). If the client doesn't require the debug data
   stream, NULL should be returned from its stream open call. */

int cfrBegFont(cfrCtx h, long flags, long origin, int ttcIndex, abfTopDict **top, float *UDV);

#define CFR_UPDATE_OPS  (1<<0)
#define CFR_USE_MATRIX  (1<<1)
#define CFR_NO_ENCODING (1<<2)
#define CFR_SHALLOW_READ   (1<<3)
#define CFR_IS_CUBE   (1<<4)
#define CFR_FLATTEN_CUBE   (1<<5)
#define CFR_SEEN_GLYPH (1<<6) /* have seen a glyph */
#define CFR_CUBE_RND (1<<7)
#define CFR_FLATTEN_VF   (1<<8)
#define CFR_SHORT_VF_NAME   (1<<9)
#define CFR_UNUSE_VF_NAMED_INSTANCE   (1<<10)

/* cfrBegFont() is called to initiate a new font parse. The source data stream
   (CFR_SRC_STREAM_ID) is opened, positioned at the offset specified by the
   "origin" parameter, parsed, and dictionary values are stored in the data
   structure pointed to by the "top" parameter (which will remain stable until
   cfrEndFont() is called).

    The "origin" parameter can either be the stream offset of the first
    byte of a CFF header or the first byte of an OpenType font (OTF), or
    the first byte of an TTC font which contains OTF fonts. In the latter
    two cases the library will automatically locate the CFF table and
    seek to it before attempting to parse the CFF data.
 
   The ttcIndex parameter selects whcih font to use in a TTC. Thsi has effect only
   when the font file is a TTC font.
 
   The "flags" parameter allows client control over the parse by setting bits
   as follows:

   CFR_UPDATE_OPS - charstrings using the deprecated seac operator are
   converted to a regular path by combining the component glyphs and the
   deprecated dotsection operator is removed

   CFR_USE_MATRIX - transform charstring path coordinates using the font's own
   FontMatrix. This facility would not normally be used during font format
   conversion since it is generally better to leave the path coordinates
   unmodified and specify a non-default matrix in the font. However, it is
   useful for drawing or manipulating glyph outline data.

   CFR_NO_ENCODING - don't read the encoding and set all glyphs unencoded. This
   options should be used when reading the CFF table from an OTF font since
   the font is encoded using the data in the cmap table and not CFF table.

   CFR_SHALLOW_READ - don't read FDArray or glyph related data like the charstring,
   the charset, or the encoding data. This flag should be used when the client
   wants to read only the following information quickly: name INDEX, string INDEX,
   gsubrs INDEX, and top dict. Glyph access functions like cfrIterateGlyphs,
   cfrGetGlyphByTag, cfrGetGlyphByName, cfrGetGlyphByCID may not be called
   with a font parsed with this flag set.

   CFR_IS_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. Cube subr's are found at the end of gsubr's
   for CID fonts, subrs for non-CID fonts.
	   
   CFR_FLATTEN_CUBE - the data contains cube font operators. Stack depth
   and operator defs are different. The Cube compose ops must be flattened.

   CFR_FLATTEN_VF - flatten CFF2 variable font data for a given user design vector.

   CFR_SHORT_VF_NAME - when flattening a CFF2 variable font, a generated instance
   PS name will be limited to 63 characters, as opposed to 127 characters as
   documented in Adobe Tech Notes 5902.

   CFR_UNUSE_VF_NAMED_INSTANCE - when flattening a CFF2 variable font, the instance PS name
   is generated from the design vector as documented in Tech Notes 5902 regarding named
   instances in the fvar table. If the flag is unset, a PS name of a named instance is used
   if there is a match.

   The "UDV" parameter specifies the User Design Vector to be used in
   flattening (snapshotting) a CFF2 variable font. If NULL, the font is flattened at the
   default instance. The parameter may be set to NULL for non-variable fonts.
*/


int cfrIterateGlyphs(cfrCtx h, abfGlyphCallbacks *glyph_cb);

/* cfrIterateGlyphs() is called to interate through all the glyph data in the
   font. (The number of glyphs in the font is passed back to the client via the
   "top" parameter to cfrBegFont() in the "sup.nGlyphs" field.) Glyph data is
   passed back to the client via the callbacks specified via the "glyph_cb"
   parameter (see absfont.h). 

   Glyphs are presented to the client in glyph index order. Each glyph may be
   identified from the "info" parameter that is passed back to the client via
   the beg() callback (the glyph index is specified via the "tag" field).

   The client can control whether path data is read or ignored by the value
   returned from beg() and can thus use this interface to select a subset of
   glyphs or just enumerate the glyph set without reading any path data. */

int cfrGetGlyphByTag(cfrCtx h, 
                     unsigned short tag, abfGlyphCallbacks *glyph_cb); 
int cfrGetGlyphByName(cfrCtx h, 
                      char *gname, abfGlyphCallbacks *glyph_cb);
int cfrGetGlyphByCID(cfrCtx h, 
                     unsigned short cid, abfGlyphCallbacks *glyph_cb);

/* cfrGetGlyphByTag(), cfrGetGlyphByName(), cfrGetGlyphByCID() are called
   obtain glyph data from a glyph selected by its tag (described above), its
   name, or its CID, respectively. The glyph name is specified by as a
   null-terminated string via the "gname" parameter. 

   These functions return cfrErrNoGlyph if the requested glyph is not present
   in the font or the access method is incompatible with the font type. */

int cfrGetGlyphByStdEnc(cfrCtx h, int stdcode, abfGlyphCallbacks *glyph_cb);

/* cfrGetGlyphByStdEnc() is called to obtain glyph data from a glyph selected
   by its standard encoding specified by the "stdcode" parameter (which is
   really an alias for a standard glyph name). It is primarily intended to be
   used when the client is subsetting a font that contains glyphs composed
   using the seac operator. When a client selects such a glyph they must ensure
   that the base and accent glyphs are also present in the subset. This may be
   achieved by calling cfrGetStdEncGlyph() with the "bchar" and "achar"
   parameters from the seac() glyph callback taking special note of the
   ABF_GLYPH_SEEN flags parameter so that repeated glyph definitions are
   avoided. */

int cfrResetGlyphs(cfrCtx h);

/* cfrResetGlyphs() may be called at any time after the cfrIterateGlyphs() or
   cfrGetGlyphBy*() functions have been called to reset the record of the
   glyphs seen (called back) by the client. This is achieved by clearing the
   ABF_GLYPH_SEEN bit in the abfGlyphInfo flags field of each glyph.

   A client wishing to have direct access to the charstring data may do so by
   utilizing the source stream (CFR_SRC_STREAM_ID) offsets for subroutines and
   charstrings. The regions of the source stream occupied by the global subr
   INDEX and the local subr INDEX are available via cfrGetSingleRegions() and
   cfrGetRepeatRegions(). The per-glyph charstring information is available in
   the "abfGlyphInfo.sup" field. In order to obtain per-glyph offset data the
   client must ask for glyph data via one of the 5 functions specified above
   and then issue an ABF_SKIP_RET from the glyphBeg() callback so that the
   charstring is not parsed and called back in the usual manner. */

typedef struct
    {
    ctlRegion Header;
    ctlRegion NameINDEX;
    ctlRegion TopDICTINDEX;
    ctlRegion StringINDEX;
    ctlRegion GlobalSubrINDEX;
    ctlRegion Encoding;
    ctlRegion Charset;
    ctlRegion FDSelect;
    ctlRegion VarStore;
    ctlRegion CharStringsINDEX;
    ctlRegion FDArrayINDEX;
    } cfrSingleRegions;

const cfrSingleRegions *cfrGetSingleRegions(cfrCtx h);

/* cfrGetSingleRegions() is called to get the src stream regions occupied by
   the CFF data struct indentified by its field name. This functions returns a
   pointer to a structure containing regions that appear only once per-font.

   Note: The "Encoding" and "Charset" regions need special processing because
   the "begin" field of the region either contains a predefined encoding or
   charset value or it contains the stream offset of a custom encoding. In the
   former case the "end" field of the region contains -1 and in the later case
   it contains the byte following the custom encoding. 

   Predefined encoding and charset values are enumerated in dictops.h. */

typedef struct
    {
    ctlRegion PrivateDICT;
    ctlRegion LocalSubrINDEX;
    } cfrRepeatRegions;

const cfrRepeatRegions *cfrGetRepeatRegions(cfrCtx h, int iFD);

/* cfrGetRepeatRegions() is called to get the src stream regions occupied by
   the CFF data struct indentified by its field name. This function returns a
   pointer to a structure containing regions in the FDArray that may appear
   more than once per-font. The "iFD" parameter selects the font dictionary
   containing the regions to be returned. If the "iFD" parameter is invalid a
   NULL pointer is returned.

   Note: a Private DICT is required but it may be empty (0-length). */

int cfrGetWidths(cfrCtx h, int iFD, 
                 float *defaultWidthX, float *nominalWidthX);

/* cfrGetWidths() is called to get the values of the default and nominal widths
   assciated with a font dictionary selected by the "iFD" parameter. The
   function returns 0 on success and 1 if the "iFD" parameter is invalid. */

const unsigned char *cfrGetStdEnc2GIDMap(cfrCtx h);

/* cfrGetStdEnc2GIDMap() returns an array of 256 elements which map character
   codes in standard encoding to glyph indexes for the current font. It is
   intended to be used for rasterizing glyphs that use the deprecated Type 2
   seac specification which is encoded as a 4-operand endchar operator. Glyphs
   in fonts using seac are expected to be ordered by their SID and therefore
   the maximum glyph index of a glyph in standard encoding is 149 (germandbls)
   which can easily be stored in a byte. */

int cfrEndFont(cfrCtx h);

/* cfrEndFont() is called to terminate a font parse initiated with
   cfrBegFont(). The input stream is closed as a result of calling this
   function. */

void cfrFree(cfrCtx h);

/* cfrFree() destroys the library context and all the resources allocated to
   it. */

typedef long (*cfrlastResortInstanceNameCallback)(void *clientCtx, float *udv, long numAxes,
                                                    char *prefixStr, long prefixLen,
                                                    char *nameBuffer, long nameBufferLen);
/* CFF2 variable font last resort naming callback
   If successful, the function should return a postive length of the string returned in nameBuffer.
   If unsuccessful, the function returns a non-positive number.

   clientCtx - context pointer passed to cfrSetLastResortInstanceNameCallback

   udv - user design vector specifying the variable font instance

   numAxes - the number of axes, or the length of udv array

   prefixStr - a null-terminated string of the variable font family name,
        to be used as a prefix of the instance name

   prefixLen - length of the prefix stirng

   nameBuffer - string buffer where a null-terminated Postscript name of the variable font
        instance should be returned by the client

   nameBufferLen - the length of the nameBuffer (including a null-byte) */

void cfrSetLastResortInstanceNameCallback(cfrCtx h, cfrlastResortInstanceNameCallback cb, void *clientCtx);

/* cfrSetLastResortInstanceNameCallback() allows a client to provide a callback function to be called
   when a flattened CFF2 variable font cannot be determined with a named instance in the fvar table
   or using the algorithm described in Adobe TechNotes 5902 "Generating PostScript Names for Fonts
   Using OpenType Font Variations". The callback function is called to give the client a chance to
   name a font instance adequately.
 
   If no callback is provided, a last-resort instance is named as a font family name prefix followed
   by "-<hex>" where <hex> is a hex string generated by hashing the design vector of the instance. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "cfrerr.h"
    cfrErrCount
    };

/* Library functions return either zero (cfrSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from cfrerr.h. */

char *cfrErrStr(int err_code);

/* cfrErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void cfrGetVersion(ctlVersionCallbacks *cb);

/* cfrGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* CFFREAD_H */
