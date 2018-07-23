/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type 1 charstring services.
 */

#ifndef T1CSTR_H
#define T1CSTR_H

#include "ctlshare.h"

#define T1C_VERSION CTL_MAKE_VERSION(1,0,19)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*t1cDecryptFunc)(int, long *, char *, char *);

int t1cDecrypt(int lenIV, long *length, char *cipher, char *plain);

/* t1cDecrypt() decrypts the Type 1 charstring specified by the "cipher" string
   and "length" parameters and copies the result to the string specified by the
   "plain" string parameter.

   The "lenIV" parameter controls the decryption according to the "Adobe Type 1
   Font Format" specification. A value of -1 indicates no encryption and should
   thus not be passed to this function (no decryption will be performed and
   t1cErrBadLenIV will be returned in this case). Positive values (including 0)
   specify both encryption and the number of initial random bytes in the
   charstring. The initial random bytes are discarded before the remaining
   decrypted bytes are copied to the plain string and the length parameter is
   reduced accordingly. (The value of lenIV for all charstrings in a font is
   typically specified with the /lenIV key in a font's Private dictionary or by
   its default value of 4 if this key is absent.) 

   The buffer pointed to by the "plain" parameter must be at least "length" -
   "lenIV" bytes long. The "cipher" and "plain" parameters may point to the
   same buffer. */

int t1cUnprotect(int lenIV, long *length, char *cipher, char *plain);

/* t1cUnprotect() removes protection from charstrings contained in protected
   CJK fonts (identified by the /RunInt key value of /eCCRun) but otherwise
   operates identically to t1cDecrypt(). */

typedef struct
    {
    long flags;
#define T1C_WIDTH_ONLY  (1<<0)
#define T1C_USE_MATRIX  (1<<1) /* Matrix applied to entire charstring; used by 'rotateFont'.*/
#define T1C_UPDATE_OPS  (1<<2)
#define T1C_IS_CUBE		(1<<3)
#define T1C_FLATTEN_CUBE  (1<<4)
#define T1C_CUBE_GSUBR (1<<5) /* current charstring is a subr. */
    void *src;
    ctlStreamCallbacks *stm;
    ctlSubrs subrs;
	long subrsEnd;
    void *ctx;
    long (*getStdEncGlyphOffset)(void *ctx, int stdcode);
    unsigned char bchar;
    unsigned char achar;
    float matrix[6];
    short nMasters;
    float UDV[15];
    float NDV[15];
    float WV[64]; /* cube fonts can have up to 64 master designs*/
    } t1cAuxData;
    
int t1cParse(long offset, t1cAuxData *aux, abfGlyphCallbacks *glyph);

/* t1cParse() is called to parse a plain text Type 1 charstring into its
   constituent parts. 

   The "offset" parameter specifies the stream offset of the plain text
   charstring to be parsed.

   The "aux" parameter specifies charstring parse data that is normally stable
   across charstrings. A client may therefore set this up once before parsing
   all the charstrings in a font. The "aux" parameter fields are described
   below.

   The "flags" field allows client control over the parse by setting bits
   as follows:

   T1C_WIDTH_ONLY - terminate parse after calling back the width

   T1C_USE_MATRIX - transform path coordinates using the "matrix" field before
   they are called back to the client. This facility would not normally be used
   during font format conversion since it is generally better to leave the path
   coordinates unmodified and specify a non-default matrix in the font

   T1C_UPDATE_OPS - charstrings using the deprecated seac operator are
   converted to a regular path by combining the component glyphs specified in
   the charstring and the deprecated dotsection operator is removed

   The "getStdEncGlyphOffset" field holds the address of a function that
   will return the stream offset of a standard encoded glyph. It is only called
   when the T1C_UPDATE_OPS bit is set and a seac operator is encountered. The
   glyph is specified as a character code in StandardEncoding via the "stdcode"
   parameter which is merely acts a synonym for the glyph's name. A client
   should return the stream offset of a glyph whose name is at that standard
   code point or -1 if an error is encountered. The function passes a copy of
   the "ctx" field via the "ctx" parameter.

   The "bchar" and "achar" fields specify the base and accent standard
   character codes for seac charstrings, respectively. Non-zero values for
   either of these fields after return from t1cParse() indicates that the
   charstring contained a seac operator. A client building a subset font may
   use this information to ensure that the glyphs with the standard encodings
   of "achar" and "bchar" are added to the subset.

   Charstring data is obtained via a stream interface specified by the
   "src" stream field and the stream callbacks specified via the "stm"
   field. The stream is assumed to have been opened by the client and will be
   closed by the client. t1cParse() only makes seek() and read() calls this
   stream.

   Subroutine stream offsets are specified by the "subrs" field (see
   ctlshare.h). 

   The glyph callback functions are specified via the "glyph" parameter (see
   absfont.h). The glyph callbacks pass path construction data back to the
   client. However, the glyph.new() and glyph.end() callbacks are not called
   from t1cParse() since the client is directly initiating the callbacks.

   The parse proceeds until one of the following conditions:

   o an "endchar" or "seac" operator is encountered in the charstring or a
     subroutine (t1cSuccess is returned)
   o the width has be called back to the client and the T1C_WIDTH_ONLY bit 
     is set (t1cSuccess is returned)
   o an error is encountered and returned

   Multiple master charstrings (those containing blend operators) are snapshot
   at the instance specified by a weight vector supplied via the "WV" field.
   The number of master designs is specified by the "nMasters" field which
   should be set to 1 when parsing charstrings from a single master font.

   A User Design Vector (UDV) for a specific font may be converted to a Weight
   Vector (WV) if the Normalize Design Vector (NDV) and Convert Design Vector
   (CDV) subroutines are available in the font. This process is carried out by
   initializing the "UDV" field to the desired UDV instance and executing the
   NDV subroutine followed by the CDV subroutine by calling t1cParse() on each
   of these subroutines. The result will appear in the "WV" field which can be
   subsequently used to blend charstring coordinates. (Note that the NDV
   subroutine leaves an intermediate result in the "NDV" field that must be
   preserved for the subsequent CDV subroutine to operate correctly.)

   t1cParse() returns zero (t1cSuccess) if successful, but otherwise returns a
   positive non-zero error code described below.

   Clients obtaining encrypted charstrings directly from font files must call
   one of the above decryption functions in order to obtain a plain text
   version of the charstring before calling t1cParse(). 

   This library doesn't have knowledge of the lengths of charstrings that it's
   parsing. Consequently, it is possible to read beyond the end of a charstring
   that isn't correctly terminated. Thus, attempts to read beyond the end of
   the source stream should be detected and an error returned. The client may
   also check for correct operator termination when preparing the charstrings.

   A charstring should terminate with one of the "endchar", "seac", or
   "callsubr" operators. A subroutine should terminate with one of the
   preceding operators or "return" operator. */ 

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "t1cerr.h"
    t1cErrCount
    };

/* Library functions return either zero (t1cSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from t1cerr.h. */

char *t1cErrStr(int err_code);

/* t1cErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void t1cGetVersion(ctlVersionCallbacks *cb);

/* t1cGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* T1CSTR_H */
