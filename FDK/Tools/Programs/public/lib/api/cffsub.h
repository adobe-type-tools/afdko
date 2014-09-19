/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFFSUB_H
#define CFFSUB_H

#include <stddef.h>             /* For size_t */

#ifdef __cplusplus
extern "C" {
#endif

#define CFFSUB_VERSION 0x01000a /* Library version */

/* CFF Subsetting Library
   ======================
   This library reduces the size of CFF (Compact Font Format) fonts by creating
   a new font that contains a subset of glyphs in the original font. The input
   font must be a CFF FontSet containing exactly one single master, multiple
   master, or CID-keyed font. Multiple master fonts are snapshot at a
   user-specified instance before subsetting resulting in a single master CFF
   font. 

   A single master or multiple master font may be converted to a CID-keyed font
   containing just one dictionary and a special registry-ordering-supplement.
   The CIDs of this special registry-ordering-supplement match the input font's
   glyph ids (GIDs) and thus the output font's charset provides a way of
   "encoding" the font using these GIDs.

   This library is initialized with a single call to cffSubNew() which
   allocates a opaque context (cffSubCtx) which is passed to subsequent
   functions and is destroyed by a single call to cffSubFree(). Thus, multiple
   contexts may be concurrently allocated permitting operation in a
   multi-threaded environment.

   Once a context has been allocated a client may call cffSubFont() for each
   font to be subset.

   Memory management and data movement are implemented via a number of
   client-supplied callback functions passed to cffSubNew() via the
   cffSubCallbacks data structure. These are described in detail below.

   A number of callback functions that handle temporary intermediate data
   (presumably by the use of temporary files) are used to keep run-time memory
   usage within reasonable limits. */

typedef struct cffSubCtx_ *cffSubCtx;
typedef struct cffSubCallbacks_ cffSubCallbacks;

cffSubCtx cffSubNew(cffSubCallbacks *cb);

/* cffSubNew() initializes the library and returns and opaque context that
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes a set
   of callback functions to the library via the cb argument which is a
   cffSubCallbacks data structure whose fields are described below. Optional
   fields should be passed with a NULL value if not required. This function
   returns NULL if the initialization failed. */

struct cffSubCallbacks_
    {
    void *ctx;

/* [Optional] ctx is a client context that is passed back to the client as the
   first parameter to the callback functions. It is intended to be used in
   multi-threaded environments.

   Memory management: */

    void *(*malloc)     (void *ctx, size_t size);
    void *(*realloc)    (void *ctx, void *old, size_t size);
    void (*free)        (void *ctx, void *ptr);

/* [Required] The malloc(), realloc(), and free() functions manage memory in
   the same manner as the Standard C Library functions of the same name. (This
   means that they must observe the alignment requirements imposed by the
   standard.)

   CFF data input: */

    char *(*srcSeek)    (void *ctx, long offset, long *count);

/* [Required] srcSeek() is called in order to seek to an absolute position in
   the CFF data specified by the offset parameter. It returns a pointer to the
   new data (beginning at that offset) and the count of the number of bytes of
   new data available via the count parameter. A client should protect itself
   by checking that the offset lies within the data region and returning a
   count of 0 in the event of an error. (A file-based client might map this
   function to fseek() followed by fread() using buffers of BUFSIZ bytes.) */

    char *(*srcRefill)  (void *ctx, long *count);

/* [Required] srcRefill() is called in order to refill the library's CFF input
   buffer. It returns a pointer to the new data and the count of the number of
   bytes of new data available via the count parameter. The client must arrange
   that successive calls to srcRefill() return consecutive blocks of CFF data.
   The client should return a count of 0 in the event of more data being
   requested than is available (buffer overrun). (A file-based client might map
   this function to fread() and fill buffers of BUFSIZ bytes.)

   CFF data output: */

    void (*dstSize)     (void *ctx, long size);

/* [Optional] dstSize() is called just prior to calling the output function
   dstWriteN() with the total size of the subset CFF data in bytes. It permits
   a memory-based client to allocate the exact amount of space for the subset
   CFF data in memory, for example. */

    void (*dstWriteN)   (void *ctx, long count, char *ptr);

/* [Required] dstWriteN() is called to handle the output of one or more bytes
   of CFF data. (A file-based client might map this function fwrite().)

   Temporary file I/O: */

    void *(*tmpOpen)    (void *ctx);                        
    void (*tmpWriteN)   (void *ctx, void *file, long count, char *ptr); 
    long (*tmpTell)     (void *ctx, void *file);
    void (*tmpSeek)     (void *ctx, void *file, long offset);
    char *(*tmpRefill)  (void *ctx, void *file, long *count);                
    void (*tmpClose)    (void *ctx, void *file);

/* [Required] The tmp functions are called to handle temporary intermediate
   data so that rather than keep entire fonts in memory they can be accumulated
   in a temporary file that is re-read during the output of the CFF data.
   tmpOpen() returns an identifier to the temporary file object that is
   subsequently passed back to the client in the file argument (which might be
   a file descriptor or file pointer, for example). If the temporary file
   cannot be opened, NULL should be returned. tmpWriteN() and tmpRefill()
   behave identically to dstWriteN() and srcRefill(), respectively, except that
   they operate on the temporary file. tmpTell() returns the current absolute
   byte position in temporary file. tmpSeek() seeks to the absolute byte
   position given by the offset argument (which is guaranteed to be valid). (A
   client might map these functions onto tmpfile(), fwrite(), rewind() or
   fseek(), fread(), and fclose(), respectively.) */
    };

typedef long cffSubFixed;   /* 16.16 fixed point */

int cffSubFont(cffSubCtx h, int flags, char *newFontName, long cffDataSize,
               long nSubsetGlyphs, unsigned short *subsetGlyphList,
               char **subsetGlyphNames, cffSubFixed *UDV);

/* cffSubFont() actually does the work of subsetting the font. It reads the
   input CFF font using the srcSeek() and srcRefill() callbacks and subsets it
   according to the supplied arguments producing an output CFF font using the
   dstWriteN() callback. The temporary file callbacks may also be used to
   reduce run-time memory requirements.

   The flags argument specifies the subsetting options below: */

#define CFFSUB_ENCODE_BY_GID (1<<0) /* Create CIDFont with GID "encoding".
                                       Implies CFFSUB_EMBED. */
#define CFFSUB_REMOVE_UIDS   (1<<1) /* Remove UniqueID and XUID. */
#define CFFSUB_EMBED         (1<<2) /* Perform embedding optimizations. */

/* The newFontName, if non-NULL, specifies a new FontName as a null-terminated
   string. This name will be stored in the Name INDEX of the subsetted font.

   The cffDataSize argument specifies the total size of the source CFF data in
   bytes.

   The nSubsetGlyphs argument specifies the number of subset glyphs stored in
   the subsetGlyphList array argument. This list is used to determine the
   glyphs that are to be retained in the resulting subsetted font. All other
   glyphs in the font are discarded except the .notdef glyph at GID 0, which is
   retained regardless of whether or not it was specified by the client.

   The optional subsetGlyphNames argument permits a client to specify the
   subset by glyph name rather than GID. The argument, when non-NULL, specifies
   an array of pointers to C strings terminated by a NULL element. In this case
   the GIDs in the subsetGlyphList are interpreted as indexes into the
   subsetGlyphName array for the purpose of identifying the glyph to be
   retained in the subset. The glyph-name-based lookup may be disabled by
   passing a NULL subsetGlyphNames argument. (As a convenience to CoolType
   clients the subsetGlyphNames argument exactly matches the array returned by
   CTGetVal(... "charstringnames" ...)).

   The UDV argument is used with multiple master fonts and, if non-NULL,
   specifies a multiple master instance as a user design vector of up to 15
   elements corresponding to the axes of the font. The resulting subsetted CFF
   font will be a single master version of the multiple master input font
   representing the specified instance in the design space. If a multiple
   master font is subset with a NULL UDV argument the font is converted to a
   single master font at the default instance which is read from the CFF font.

   cffSubFont() returns 0 on success and a positive non-zero error code in the
   event of an error. The list of possible error codes is specified below. */

enum
    {
    cffSubErrOK,            /* Subsetting completed successfully */
    cffSubErrOutOfMemory,   /* Out of memory */
    cffSubErrInvalidFont,   /* Invalid CFF font data */
    cffSubErrGIDBounds,     /* Subset GID(s) out of valid range for font */
    cffSubErrBadSubset,     /* subsetGlyphNames arg specified with CID font */
    cffSubErrCantOpenTmp    /* Temporary file open failed */
    };

void cffSubFree(cffSubCtx h);

/* cffSubFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

#ifdef __cplusplus
}
#endif

#endif /* CFFSUB_H */
