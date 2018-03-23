/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef CFFREAD_H
#define CFFREAD_H

#include <stddef.h>             /* For [u]int32_t */
#include <stdint.h>             /* For size_t */

#define CFF_VERSION 0x010005    /* Library version */

/* Compact Font Format (CFF) Parser Library
   ========================================
   This library parses information in CFF fonts and provides an interface for
   executing metric charstrings in conjunction with these fonts. Input data
   containing one single master, multiple master, or CID-keyed font is
   supported. (Multiple-font FontSets are not supported.)

   This library is initialized with a single call to cffNew() for each font to
   be parsed which allocates an opaque context (cffCtx) which is passed to
   subsequent functions and is destroyed by a single call to cffFree(). Thus,
   multiple contexts may be concurrently allocated permitting operation in a
   multi-threaded environment.

   Once a context has been allocated, a client may call cffGetFontInfo() to get
   global information about the font or make calls to cffGetGlyphInfo() or
   cffGetGlyphWidth() to get per-glyph information. These calls return
   information for the default instance of a multiple master font unless a
   prior call is made to either cffSetUDV() or cffSetWV() which override the
   default. The "set" and "get" functions may be freely intermixed to fetch
   values for a number of different instances.

   The cffExecMetric() and cffExecLocalMetric() functions provide a facility
   for executing metric charstrings and returning the array of values that
   remained on the stack when the endchar operator was encountered.

   Path information may be optionally returned by calling cffGetGlyphInfo()
   with a non-NULL cb argument.

   Exception handling, memory management, and data input are implemented via a
   number of client-supplied callback functions passed to the cffNew() function
   via the cffStdCallbacks data structure. These are described in detail below.

   Data input is provided by the seek() and refill() client functions. These
   provide a window onto the CFF data. The CFF data may be just a part of a
   larger aggregate object (e.g. an OpenType font). In this case the client may
   arrange for the seek() and refill() to traverse the entire object and pass
   the offset of the start of the CFF data to the cffNew() function rather than
   passing 0. This offers the advantage of using the same data input routines
   for parsing the CFF font and multiple master metric data. */

typedef struct cffCtx_ *cffCtx;
typedef struct cffStdCallbacks_ cffStdCallbacks;

cffCtx cffNew(cffStdCallbacks *cb, long offset);

/* cffNew() initializes the library and returns an opaque context that is
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes a set
   of callback functions to the library via the cb argument which is an
   cffStdCallbacks data structure whose fields are described below. Optional
   fields should be passed with a NULL value if not required. */

/* Message types (for use with message callback) */
enum
    {
    cffWARNING = 1,
    cffERROR,
    cffFATAL
    };

struct cffStdCallbacks_
    {
    void *ctx;

    /* [Optional] ctx is a client context that is passed back to the client
       as the first parameter to the callback functions. It is intended to be
       used in multi-threaded environments. */
                    
    void (*fatal)      (void *ctx);

    /* [Required] fatal() is an exception handler that is called if an
       irrecoverable error is encountered during parsing. The current context
       is destroyed and all allocated resources released before fatal() is
       called. This function must NOT return and the client should use
       longjmp() to return control to a point prior to calling cffNew(). */

    void (*message)    (void *ctx, int type, char *text);
                    
    /* [Optional] message() simply passes a message back to the client as a
       null-terminated string. Three message types are supported: cffWARNING,
       cffERROR, and cffFATAL. A client is free to handle messages in any
       manner they choose. */

    void *(*malloc)    (void *ctx, size_t size);
    void (*free)       (void *ctx, void *ptr);

    /* [Required] The malloc() and free() functions manage memory in the same
       manner as the Standard C Library functions of the same name. (This means
       that they must observe the alignment requirements imposed by the
       standard.) The client is required to handle any error conditions that
       may arise. Specifically, a client must ensure requested memory is
       available and must never return a NULL pointer from malloc(). */

    char *(*cffSeek)   (void *ctx, long offset, long *count);

    /* [Required] cffSeek() is called in order to seek to an absolute position
       in the input data specified by the offset parameter. It returns a
       pointer to the new data (beginning at that offset) and the count of the
       number of bytes of new data available via the count parameter. A client
       should protect itself by checking that the offset lies within the data
       region. (A file-based client might map this function to a call to
       fseek() followed by fread().) */
                    
    char *(*cffRefill) (void *ctx, long *count);

    /* [Required] cffRefill() is called in order to refill the library's input
       buffer. It returns a pointer to the new data and the count of the number
       of bytes of new data available via the count parameter. (A file-based
       client might map this function to fread().) 

       It is important (for performance reasons) to choose an input buffer size
       that isn't too small. For file-based clients a buffer size of BUFSIZ
       would be a good choice because it will match the underlying low-level
       input functions. */
    };

typedef int32_t cffFixed;          /* 16.16 fixed point */

void cffSetUDV(cffCtx h, int nAxes, cffFixed *UDV);
void cffSetWV(cffCtx h, int nMasters, cffFixed *WV);

/* cffSetUDV() and cffSetWV() allow the implicit default instance of a multiple
   master font to be overridden with an explicit instance specified either as a
   user design vector in the case of cffSetUDV(), or as a weight vector in the
   case of cffSetWV(). The "set" and "get" functions may be freely intermixed
   to fetch values for a number of different instances. */

typedef struct cffFontInfo_ cffFontInfo;

cffFontInfo *cffGetFontInfo(cffCtx h);

/* cffGetFontInfo retrieve font-wide information about a font via the pointer
   to cffFontInfo data structure the function returns. This data structure is
   described below. */

#include "txops.h"

typedef unsigned short cffSID;  /* String identifier */
typedef short cffFWord;         /* Font metric in em-relative units */

#define CFF_SID_UNDEF   0xffff  /* SID of undefined string */
#define CFF_UNENC       (-1)    /* Unencoded glyph code */

typedef struct              /* Bounding box */
    {
    cffFWord left;
    cffFWord bottom;
    cffFWord right;
    cffFWord top;
    } cffBBox;

struct cffFontInfo_
    {
    struct                  /* PostScript font name */
        {
        short length;
        long offset;
        } FontName;
    cffSID version;         /* Optional */
    cffSID Notice;          /* Optional */
    cffSID Copyright;       /* Optional */
    cffSID FamilyName;      /* Optional */
    cffSID FullName;        /* Optional */
    cffBBox FontBBox;
    unsigned short unitsPerEm;
    cffFWord isFixedPitch;
    cffFixed ItalicAngle;
    cffFWord UnderlinePosition;
    cffFWord UnderlineThickness;
    short Encoding;         /* Predefined or custom encoding (see below) */
    short charset;          /* Predefined or custom charset (see below) */
    struct
        {
        short nAxes;    
        short nMasters;
        short lenBuildCharArray;
        cffSID NDV;
        cffSID CDV;
        cffFixed UDV[TX_MAX_AXES]; /* Default User design vector */
        cffSID axisTypes[TX_MAX_AXES];
        } mm;
    struct                  /* CID data (optional) */
        {
        double version;
        cffSID registry;
        cffSID ordering;
        short supplement;
        struct              /* Vertical origin vector */
            {
            cffFWord x;
            cffFWord y;
            } vOrig;
        } cid;
    unsigned short nGlyphs; /* Glyph count */
    };

enum                        /* Encoding type */
    {
    CFF_STD_ENC,
    CFF_EXP_ENC,
    CFF_CUSTOM_ENC
    };
enum                        /* Charset types */
    {
    CFF_ISO_CS,
    CFF_EXP_CS,
    CFF_EXPSUB_CS,
    CFF_CUSTOM_CS
    };

/* Optional unset cffSID fields are indicated with an SID value of
   CFF_SID_UNDEF. A CID font is indicated by having a defined cid.registry SID
   value. A multiple master font is indicated by mm.nMasters being greater than
   1. */

typedef struct cffGlyphInfo_ cffGlyphInfo;
typedef struct cffPathCallbacks_ cffPathCallbacks;

cffGlyphInfo *cffGetGlyphInfo(cffCtx h, unsigned gid, cffPathCallbacks *cb);

/* cffGetGlyphInfo() returns information about the glyph specified by the gid
   argument. The information is returned via a pointer to a cffGlyphInfo data
   structure which is described below. The optional cb argument permits the
   client to obtain path information from the glyph as its charstring data is
   parsed. This is described in detail below. If this facility isn't required
   the cb argument should be set to NULL. */

typedef struct cffSupCode_ cffSupCode;
struct cffSupCode_          /* Supplementary encoding */
    {
    cffSupCode *next;
    unsigned char code;
    };

struct cffGlyphInfo_        /* Glyph information */
    {
    unsigned short id;      /* SID/CID */
    short code;             /* Encoding (unencoded=-1) */
    cffFWord hAdv;          /* Horizontal advance width */
    cffFWord vAdv;          /* Vertical advance width */
    cffBBox bbox;           /* Bounding box */
    cffSupCode *sup;        /* Supplementary encodings (linked list) */
    };

/* The id field is interpreted as a CID for CIDFonts and an SID otherwise. The
   sup field, when non-NULL, points to a chain of supplementary encodings for
   the glyph. This is a rare occurrence but some fonts have glyphs that have
   multiple encodings.

   The path callback functions are optionally supplied to cffGetGlyphInfo() via
   the cffPathCallbacks data structure which is passed to the function as a
   pointer (passing a NULL pointer will disable all path callbacks). All the
   path callback functions are optional and the corresponding field should be
   set to NULL if a particular callback is not required.

   Each disconnected subpath within a glyph is bracketed by calls to newpath()
   and closepath(). Each subpath begins with a single call to moveto()
   followed by calls to lineto(), curveto(), and hintmask() in any order. The
   end of path information for a particular glyph is signalled by a call to
   endchar().

   The first subpath may be preceded by calls to hintstem() which can be used
   to build up a hint stem list for a particular glyph. The stems in the stem
   list are numbered consecutively from 0 beginning with the first stem. This
   stem list may be used in conjunction with hintmask() to determine which
   hints are active at any point in the path. These stems in the stem list
   correspond to the hint mask bits passed via hintmask(). The most significant
   bit of the first byte of the hint mask corresponds to stem 0, the next
   highest bit to stem 1, and so on.

   All coordinate data passed via callback arguments represents absolute data
   in font units. */

#define CFF_MAX_MASK_BYTES (T2_MAX_STEMS / 8) /* Maximum hint mask bytes */

struct cffPathCallbacks_
    {
    void (*newpath)    (void *ctx);

/* [optional] newpath() is called immediately before the path construction
   callbacks: moveto(), lineto(), and curveto(), are called for a new subpath.
   */ 

    void (*moveto)     (void *ctx, cffFixed x1, cffFixed y1);

/* [optional] moveto() is called at the beginning of a new subpath in order to
   set the current point to (x1, y1). */

    void (*lineto)     (void *ctx, cffFixed x1, cffFixed y1);

/* [optional] lineto() is called to add a line segment to the current path from
   the current point to (x1, y1). The current point becomes (x1, y1). */

    void (*curveto)    (void *ctx, int flex,
                        cffFixed x1, cffFixed y1, 
                        cffFixed x2, cffFixed y2, 
                        cffFixed x3, cffFixed y3);

/* [optional] curveto() is called to add a cubic bezier curve segment to the
   current path from the current point guided by control points (x1, y1) and
   (x2, y2) and terminating at point (x3, y3). The current point becomes (x3,
   y3). The flex argument is set to 1 if the curve is part of a flex feature
   and set to 0 otherwise. A flex feature consists of two consecutive curveto()
   calls with their flex arguments set to 1. */

    void (*closepath)  (void *ctx);

/* [optional] closepath() is called to signal the end of a subpath. */

    void (*endchar)    (void *ctx);

/* [optional] endchar() is called to signal the end of the path definition of
   the current glyph. */

    void (*hintstem)   (void *ctx, int vert, cffFixed edge0, cffFixed edge1);

/* [optional] hintstem() is called before the first subpath to indicate a hint
   stem that will be activated by a subsequent hintmask in the current glyph.
   Typically, multiple hint stems are used to construct a stem list for the
   current glyph. The vert argument is set to 1 to indicate a vertical stem and
   set to 0 to indicate a horizontal stem. The horizontal stems precede the
   vertical stems in the stem list and within each direction group the stems
   are organized by increasing values of the edge0 argument. The edge0 argument
   generally represents the left or bottom edge of a stem and the edge1
   argument generally represents the right or top edge of a stem. The width of
   a stem is calculated as (egde1 - edge0). Edge (ghost) hints are represented
   by negative stem widths of -20 for a right or top edge and -21 for a left or
   bottom edge. */

void (*hintmask) (void *ctx, int cntr, int n, char mask[CFF_MAX_MASK_BYTES]);

/* [optional] hintmask() is called to establish a new set of stem hints. The
   active stem hints are specified by bits set to 1 and inactive stem hints are
   specified by bits set to 0 in the mask argument. The number of valid data
   bytes in the mask is exactly the number needed to represent the number of
   stems in the hint stem list. For convenience the number of valid bytes is
   passed in argument n and is fixed for all hintmask functions within a single
   glyph. The cntr argument is set to 1 to indicate that the stem hints are to
   be used for counter control and set to 0 to indicate stem control. */
 
   };

void cffGetGlyphWidth(cffCtx h, unsigned gid, cffFWord *hAdv, cffFWord *vAdv);

/* cffGetGlyphWidth() provides quick access to the horizontal and vertical
   advance widths of a glyph specified by the gid argument without the overhead
   of parsing the entire charstring that is implicit in cffGetGlyphInfo(). hAdv
   or vAdv may be set to NULL if not needed. */

void cffGetGlyphOrg(cffCtx h, unsigned gid, 
                    unsigned short *id, short *code, cffSupCode **sup);

/* cffGetGlyphOrg() provides quick access to the SID/CID and the encoding(s) of
   the specified glyph without the overhead of parsing the entire charstring
   that is implicit in cffGetGlyphInfo(). The value passed via the id argument
   is interpreted as a CID for CIDFonts and an SID otherwise. The value passed
   via the code argument is an encoding in the range 0-255 or -1 if there is no
   encoding for the specified glyph. The value passed by the sup argument, when
   non-NULL, points to a chain of supplementary encodings for the glyph. When
   the value passed via the code argument is -1, indicating an unencoded glyph,
   there can be no supplementary encoding and therefore the value passed via
   the sup argument will be NULL. */

int cffExecMetric(cffCtx h, long offset, cffFixed result[T2_MAX_OP_STACK]);

/* cffExecMetric() provides a facility for executing multiple master metric
   charstrings and returning the array of values that remained on the stack
   when the endchar operator was encountered. (The charstrings must not contain
   drawing operators or subroutine calls in order to ensure correct operation.)
   The client must initialize the library in the usual way with a call to
   cffNew() and is also required to set up the seek() and refill() callback
   functions before calling cffExecMetric() so that they provide the requested
   bytes from the metric charstring. If the metric charstrings are combined
   together into a single block of data the set up can be performed once and
   different metric charstrings can be executed by calling cffExecMetric() with
   different values for the offset parameter. Calls to cffSetUDV() or
   cffSetWV() may be freely intermixed with calls to cffExecMetric() or
   cffExecLocalMetric() permitting metrics to be executed for different
   instances. The results are returned via a client-supplied array parameter
   that is large enough to contain T2_MAX_OP_STACK values (from txops.h). The
   count of the number of values copied into the array is returned by
   cffExecMetric(). The first element of the array represents the bottommost
   stack value. */

int cffExecLocalMetric(cffCtx h, char *cstr, long length, cffFixed *result);

/* cffExecLocalMetric() is identical to cffExecMetric() except that the client
   specifies the multiple master metric charstring to be executed by a pointer
   to the start of the charstring (the cstr parameter) and the number of bytes
   of data available (the length parameter) instead of by an offset. The seek()
   and refill() callbacks will not be used when reading the charstring. This is
   useful in situations where the charstring is not within the CFF font
   itself. */

int cffGetString(cffCtx h, 
                 cffSID sid, unsigned *length, char **ptr, long *offset);

/* cffGetString() provides a facility for translating a string id (SID) into a
    a string pointer and length. This function returns 1 if SID corresponds to
    one of the standard CFF strings otherwise it returns 0. In the former case
    a pointer to the null-terminated standard string is returned via the ptr
    parameter. In the latter case the absolute CFF data offset of the string is
    returned via the offset argument which must be converted to string pointer
    by the client. In either case the string length is returned by the length
    argument. */

void cffFree(cffCtx h);

/* cffFree() destroys the library context and all the resources allocated to
   it. */

#endif /* CFFREAD_H */
