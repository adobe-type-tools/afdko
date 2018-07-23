/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef T1WRITE_H
#define T1WRITE_H

#include "ctlshare.h"

#define T1W_VERSION CTL_MAKE_VERSION(1,0,34)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type 1 Font Format Generation Library
   =====================================
   This library supports the generation of name-keyed and CID-keyed Type 1
   fonts from an abstract font. The generation of incremental fonts for
   downloading to PostScript printers is also supported.

   This library is initialized with a single call to t1wNew() which allocates a
   opaque context (t1wCtx) which is passed to subsequent functions and is
   destroyed by a single call to t1wFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may call other functions in the
   interface in order to generate the desired font.

   Memory management and stream I/O are implemented via two sets of
   client-supplied callback functions passed to t1wNew(). I/O is performed on
   two abstract data streams:
   
   o Type 1 data (w)
   o temporary data (rw)

   These streams are managed by a single set of client callback functions
   enabling the client to choose from a wide variety of implementation schemes
   ranging from disk files to memory buffers. The temporary stream is used to
   keep the run-time memory usage within resonable limits when handling large
   fonts.

   Glyph data is passed to the library via the set of glyph callback functions
   defined in t1wGlyphCallbacks. */

typedef struct t1wCtx_ *t1wCtx;
t1wCtx t1wNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define T1W_CHECK_ARGS  CTL_CHECK_ARGS_CALL(T1W_VERSION)

/* t1wNew() initializes the library and returns an opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes sets
   of memory and stream callback functions (described in ctlshare.h) to the
   library via the "mem_cb" and "stm_cb" parameters.

   The T1W_CHECK_ARGS macro is passed as the last parameter to t1wNew() in
   order to perform a client/library compatibility check. */

typedef struct t1wSINGCallback_ t1wSINGCallback;
struct t1wSINGCallback_
{
    void *ctx;
    void *stm;
    size_t length;
    int (*get_stream)(t1wSINGCallback *sing_cb);
};

int t1wSetSINGCallback(t1wCtx h, t1wSINGCallback *sing_cb);

/* t1wSetSINGCallback is called to copy the SING callback passed via the
   "sing_cb" parameter to the context passed via the "h" parameter. The
   t1wSINGCallback structure contains the following information:

   The "ctx" field can be used to specify a client context. Set to NULL if not
   used.

   The get_stream() callback is called from t1wEndFont() to initialize the
   "stm" and "length" fields with the SING glyphlet stream to be written into
   the sfnts literal array in the Private dictionary of an incremental download
   base font. The stream to be read is passed via the "stm" field and is
   expected to be open and positioned at the first byte of the glyphlet data.
   The "length" field specifies the total length of the glyphlet in bytes.
   Repeated reads will be made on the stream until "length" bytes have been
   read or read() returns a count of 0 indicating a stream error. A stream
   error will cause the t1wEndFont() to return an error. get_stream() returns 1
   on error and 0 on success. */

int t1wBegFont(t1wCtx h, long flags, int lenIV, long maxglyphs);

/* t1wBegFont() is called to begin a font definition. The "flags" parameter
   provides control over certain attributes of the generated font with the
   options enumerated below. */

enum
    {
    T1W_TYPE_HOST           = 1<<0, /* Host font */
    T1W_TYPE_BASE           = 1<<1, /* Incremental download base font */
    T1W_TYPE_ADDN           = 1<<2, /* Incremental download addition font */
    T1W_TYPE_MASK           = 7<<0,

    /* One bit from bits 0-2 must be set to encode the font type. The host type
       follows the specification in the "Adobe Type 1 Font Format" (commonly
       known as the Black Book) or the "Adobe CMap and CIDFont Files
       Specification" depending on whether the source font is name-keyed or
       cid-keyed, respectively. However, hint-substitution, if present, is
       always represented in-line rather than using subroutines and is
       therefore not compatible with very early PostScript interpreters.

       The incremental types support incremental downloading of the fonts to
       PostScript printers which allows memory usage in the printer to be more
       closely managed. An incremental font is downloaded in multiple parts: a
       single base part which is a complete font with a limited or empty glyph
       set, and one or more addition parts which just contain new glyph
       definitions to be added to the base part but aren't fonts in there own
       right. 

       The incremental cid-keyed fonts emitted by this library make use of the
       Adobe_CoolType_Utility ProcSet. It is the responsibility of the client
       to ensure that this ProcSet has been suitably defined before the font is
       downloaded.

       Note: use of subroutines is restricted to a small number of
       fixed-purpose subroutines. Hint or VM subroutines are never generated by
       any of the formats. */

    T1W_ENCODE_BINARY       = 1<<3,
    T1W_ENCODE_ASCII        = 1<<4,
    T1W_ENCODE_ASCII85      = 1<<5,
    T1W_ENCODE_MASK         = 7<<3,

    /* One of bits 3-5 must be set to control the encoding of the font data.
       Selecting the T1W_ENCODE_ASCII or T1W_ENCODE_ASCII85 options ensures the
       font can be safely transmitted over 7-bit ASCII communications channel,
       but it also increases the font data size. Selecting the
       T1W_ENCODE_BINARY option will keep the font data size smaller but must
       only be used if it is known that the font will be transmitted over an
       8-bit binary communications channel.
       
       ASCII encoding is achieved by using a hexadecimal string representation
       of the binary data or by wrapping the binary data using the hexadecimal
       form of the "eexec" operator. The form of ASCII encoding is dependent on
       the generated font type and the encoding scheme:

       Font type    T1W_ENCODE_BINARY   T1W_ENCODE_ASCII    T1W_ENCODE_ASCII85

       name-keyed
       host         binary eexec        ASCII eexec         ASCII eexec
       download     binary cstrs        ASCII-Hex cstrs     ASCII-85 cstrs

       cid-keyed
       host         binary StartData    binary StartData    binary StartData
       download     binary cstrs        ASCII-Hex cstrs     ASCII-85 cstrs

       Note: a host cid-keyed font is always generated using binary StartData
       since hex StartData isn't widely supported. ASCII85-encoding is a
       PostScript level-2 feature and therefore must only be used with
       compatible interpreters. */

    T1W_OTHERSUBRS_PRIVATE  = 1<<6, /* Private dictionary */
    T1W_OTHERSUBRS_PROCSET  = 1<<7, /* External ProcSet */
    T1W_OTHERSUBRS_MASK     = 3<<6,

    /* One bit from bits 6 and 7 must be set to control the location of the
       OtherSubrs. Selecting T1W_OTHERSUBR_PRIVATE defines the OtherSubrs in
       their conventional Private dictionary location. Selecting
       T1W_OTHERSUBR_PROCSET references the OtherSubrs in a ProcSet that is
       external to the font. It is the responsibility of the client to ensure
       that the ProcSet has been suitably defined before the font is
       downloaded. */

    T1W_NEWLINE_UNIX        = 1<<8, /* \n */
    T1W_NEWLINE_WIN         = 1<<9, /* \r\n */
    T1W_NEWLINE_MAC         = 1<<10, /* \r */
    T1W_NEWLINE_MASK        = 7<<8,

    /* One bit from bits 8-10 must be set to encode the newline type to match
       one of 3 common platform conventions. This is just a convenient styling
       and doesn't affect the the operation of the font, regardless of
       platform. */

    T1W_WIDTHS_ONLY         = 1<<11,

    /* When bit 11 is set a font that retains only the widths from the glyph
       data is created. Such a font is used by MPS for determining metrics when
       the original font is unavailable. */

    T1W_ENABLE_CMP_TEST     = 1<<12,

    /* When bit 12 is set several optimizations are disabled so as to enable
       rendering comparison tests to succeed. Do not use in production code. */

    T1W_DONT_LOOKUP_BASE    = 1<<13,

    /* When bit 13 is set findfont is not used to lookup the base font when
       incrementally downloading name-keyed fonts and when specifying an
       encoding vector. */     
 
	T1W_IS_CUBE    = 1<<14,
	
    /* When bit 14 is set, the data contains cube font operators. Stack depth
	and operator defs are different. Cube subr's are added at the end of gsubr's for CID fonts,
	subrs for non-CID fonts. */     
	
	T1W_FORCE_STD_ENCODING    = 1<<15,
    /* When bit 15 is set, TopDict encoding vector is set to StandardEncoding, no matter
	what is in the font. This is useful for working with font sources that will be used for
	OpenType/CFF fonts */     
	
   };

/* The "lenIV" parameter controls charstring encryption as follows:

   -1   No encryption
   0    Encrypt charstrings with no priming bytes
   1    Encrypt and prefix charstrings with 1 priming byte
   4    Encrypt and prefix charstrings with 4 priming bytes

   All other values are invalid. A value -1 is the most efficient but is isn't
   compatible with old PostScript devices. A value of 4 is the least efficient
   but is compatible with all PostScript devices. If the characteristics of the
   target device are unknown it is safest to choose a value of 4.

   The "maxglyphs" parameter is only used when T1W_TYPE_BASE is selected and
   must be set to 0 otherwise. It specifies the maximum number of glyphs that
   may be added to an incremental font (the sum of the glyphs specified in the
   base and addition fonts) and is normally set equal to the number of glyphs
   in the original font. This value is used to set the size of the dictionary
   in the base font that contains the glyph definitions. */

extern const abfGlyphCallbacks t1wGlyphCallbacks;

/* t1wGlyphCallbacks is a glyph callback set template that will add the data
   passed to these callbacks to the current font. Clients should make a copy of
   this data structure and set the "direct_ctx" field to the context returned
   by t1wNew(). */

int t1wEndFont(t1wCtx h, abfTopDict *top);

/* t1wEndFont() completes the definition of the font that commenced with
   t1wBegFont(). The "top" parameter specifies global font information via the
   data structures described in absfont.h. */ 

void t1wFree(t1wCtx h);

/* t1wFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "t1werr.h"
    t1wErrCount
    };

/* Library functions return either zero (t1wSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from t1werr.h. */

char *t1wErrStr(int err_code);

/* t1wErrStr() maps the "errcode" parameter to a null-terminated error 
   string. */

void t1wGetVersion(ctlVersionCallbacks *cb);

/* t1wGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

void t1wUpdateGlyphNames(t1wCtx h, char *glyphNames);
/* Used to update the array of glyph name pointers, when the source data array has changed location becuase it needed to grow. */
    
#ifdef __cplusplus
}
#endif

#endif /* T1WRITE_H */
