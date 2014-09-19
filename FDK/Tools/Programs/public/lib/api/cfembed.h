/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFEMBED_H
#define CFEMBED_H

#include "ctlshare.h"
#include "sfntwrite.h"

#define CEF_VERSION CTL_MAKE_VERSION(2,0,25)

#ifdef __cplusplus
extern "C" {
#endif

/* Compact Font Embedding Library
   ==============================
   This library prepares PostScript, OpenType, and TrueType fonts for embedding
   by subsetting and converting them to a compact sfnt-based representation
   called the Compact Embedded Font (CEF) format that contains CFF, cmap, and
   some other optional OpenType tables.

   Multiple master fonts are snapshot at a user-specified instance before
   subsetting resulting in a much smaller single master CFF font.

   The input font type and format is automatically detected from the following
   list.

   Type             Platform
   ----             --------
   PFB              Type 1 Win
   LWFN res.        Type 1 Mac
   sfnt.typ1        Type 1 GX Mac
   PFA              Type 1 Unix
   Naked CID        CID Win
   sfnt.CID         CID Mac
   CFF              Naked CFF
   OTF              OpenType Mac/Win
   TTF              TrueType Win
   sfnt.true        TrueType Mac

   Note: there is no support for TrueType Collections or FFIL resource files
   containing sfnt resources.

   This library is initialized with a single call to cefNew() which allocates a
   opaque context (cefCtx) which is passed to subsequent functions and is
   destroyed by a single call to cefFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may call cefMakeEmbeddingFont()
   for each font to be subset. */

typedef struct cefCtx_ *cefCtx;
cefCtx cefNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb, 
              CTL_CHECK_ARGS_DCL);

#define CEF_CHECK_ARGS CTL_CHECK_ARGS_CALL(CEF_VERSION)

/* cefNew() initializes the library and returns and opaque context (cefCtx)
   that subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The CEF_CHECK_ARGS macro is passed as the last parameter to cefNew() in
   order to perform a client/library compatibility check. If this check fails,
   or the library can't be initialized for some other reason, NULL is returned.
   */

enum                    
    {
    cef_CFF__FILL_SEQ = 10,     /* Predefined table fill sequence numbers */
    cef_cmap_FILL_SEQ = 20,
    cef_GPOS_FILL_SEQ = 30,

    cef_cmap_WRITE_SEQ = 10,    /* Predefined table write sequence numbers */
    cef_CFF__WRITE_SEQ = 20,
    cef_GPOS_WRITE_SEQ = 30
    };

int cefRegisterTable(cefCtx h, sfwTableCallbacks *tbl_cb);

/* cefRegisterTable() is an optional function that adds a client table to the
   tables processed for inclusion in the generated CEF font. If used, it must
   be called once for each table before the first call to
   cefMakeEmbeddingFont(). The "tbl_cb" specifies a callback set that is used
   to manage the table. A full description of the sfwTableCallbacks structure
   and the table registration process can be found in sfntwrite.h. The cfembed
   library pre-registers the cmap, CFF, and GPOS tables, therefore clients must
   not register these tables for themselves.

   The order in which the table fill and write callbacks are called is
   established by setting the "fill_seq" and "write_seq" fields of the "tbl_cb"
   parameter. Clients must coordinate their table fill and write sequence
   numbers with the predefined values above in order to establish the desired
   table filling and writing order. */

enum 
    {
    CEF_MAX_AXES = 4,       /* Maximum multiple master axes */
    CEF_VID_BEGIN = 64000   /* Beginning of virtual id (VID) range */
    };

typedef struct          /* Glyph specification */
    {
	unsigned short clientflags;
    unsigned short id;  /* NID/GID/CID/VID */
    unsigned long uv;  /* was: short */
    } cefSubsetGlyph;

typedef struct          /* Kerning pair */
    {
    unsigned short first;
    unsigned short second;
    short value;
    } cefKernPair;

typedef struct          /* Embedding specification */
    {
    unsigned short flags;
#define CEF_FORCE_LANG_1  (1<<0)  /* Force LanguageGroup 1 */
#define CEF_WRITE_SVG     (1<<1)  /* Write SVG font instead of CEF font */
#define CEF_FORCE_IDENTITY_ROS (1<<2) /* Force Registry-Ordering-Supplement in CFF to be "Adobe-Identity-0" */
    char *newFontName;
    float *UDV;
    char *URL;
    struct              /* Subset glyphs */
        {
        long cnt;
        cefSubsetGlyph *array;
        char **names;
        } subset;
    struct              /* Kern pairs */
        {
        long cnt;
        cefKernPair *array;
        } kern;
    } cefEmbedSpec;

typedef struct cefMapCallback_ cefMapCallback;
struct cefMapCallback_
    {
    void *ctx;
    void (*glyphmap)(cefMapCallback *cb, 
                     unsigned short gid, abfGlyphInfo *info);
    };

int cefMakeEmbeddingFont(cefCtx h, cefEmbedSpec *spec, cefMapCallback *map);

/* cefMakeEmbeddingFont() actually performs the work of creating the embedding
   font. It reads the input font data on the source stream, subsets the data
   according to the supplied parameters, and writes the embedding font to the
   destination stream.

   If the CEF_WRITE_SVG flag is set in the cefEmbedSpec flags field, then this
   routine will write out an SVG font instead of a CEF font by using the
   svgwrite library. This option and CEF_FORCE_LANG_1 are incompatible. See
   cefSetSvwFlags() comments for details on setting svgwrite library flags in
   the cfembed context.

   The process of making an embedding font requires that a number of different
   streams are managed by the client's stream callback functions which are
   fully described in ctlshare.h.

   The source stream data types are identified within the client's open()
   stream callback via the following constants.

   CEF_SRC_STREAM_ID    source font data (ro - "rb")
   CEF_DST_STREAM_ID    destination CEF or SVG font data (rw - "w+b)
   CEF_TMP0_STREAM_ID   temporary stream 0 (rw - "w+b")
   CEF_TMP1_STREAM_ID   temporary stream 1 (rw - "w+b")

   If the streams are implemented using ANSI files they must be opened in
   binary mode, possibly using the fopen type strings indicated above.

   The "h" parameter is the context that was returned from a successfull call
   to cefNew(). The "spec" parameter is a pointer to a structure containing an
   embedding specification via the following fields.

   The "flags" field specifies subsetting options. If CEF_FORCE_LANG_1 is
   specified, all the Private DICTs in the generated CFF font will set a
   LanguageGroup value of 1. WARNING: This option is unlikely to be useful
   outside of SING glyphlet generation.

   The "newFontName" field, if non-NULL, specifies a new PostScript FontName as
   a null-terminated string. This name will be stored in the Name INDEX of the
   CFF table in the embedding font.

   The "UDV" field is used with multiple master fonts and, if non-NULL,
   specifies a multiple master instance as a user design vector of up to
   CEF_MAX_AXES elements corresponding to the axes of the font. The resulting
   embedding font will be a single master version of the multiple master input
   font representing the specified instance in the design space. If a multiple
   master font is passed to the library with a NULL UDV parameter the font is
   converted to a single master font at the default instance specified in the
   source font.

   The "URL" field, if non-NULL, specifies a URL as a null-terminated string.
   The string will be stored as an unreference entry in the String INDEX of the
   CFF table in the embedding font.

   The subset structure specifies the glyphs to be included in the embedded
   font. Since several different font types and formats are supported by the
   library, which differ in the method of glyph access, several different
   methods of subset glyph selection are provided as shown below.

   Font Type/Format     Glyph Access
   ----------------     ------------
   Type 1               glyph name
   Naked CID            CID (character identifier)
   OTF (CID)            CID
   OTF (non-CID)        GID (glyph index) or glyph name
   TTF                  GID

   The "subset.cnt" field specifies the number of subset glyph elements in the
   "subset.array". Each element of the array specifies an index and a Unicode
   value via the "id" and "uv" fields, respectively. The "subset.names" field
   specifies the glyph names to be used for selection when appropriate (see
   above table). This field, when non-NULL, specifies an array of pointers to C
   strings terminated by the NULL character. This array must end with a NULL 
   entry. (As a convenience to CoolType clients the "subset.names" field exactly
   matches the array returned by CTGetVal(... "charstringnames" ...)).

   The "id" field and glyph names array are used in the following manner for
   the different font types and formats.

   Font Type/Format     subset.array[].id       subset.names
   ----------------     -----------------       ------------
   Type 1               NID (name index)        yes
   Naked CID            CID                     -
   OTF (CID)            CID                     -
   OTF (non-CID)        GID/NID                 optional
   TTF                  GID                     -
   Any                  VID                     -

   For Type 1 fonts the "id" field represents a name index that is used to
   index the "subset.names" array in order to access the name of the glyph to
   be selected for the subset.

   All other glyphs not specified by the "subset.array" are discarded except
   the notdef glyph (a glyph named ".notdef" and/or GID 0) which is retained
   regardless of whether it was specified by the client.

   A client may specify virtual ids (VIDs) in the subset. These are ids in the
   range CEF_VID_BEGIN to 65535, inclusive. Ids in this range do not correspond
   to real glyphs in the font and therefore do not specify glyphs for inclusion
   in the subset. They are, however, included in the cmap table and the GPOS
   table where they represent the virtual glyph ids (VGIDs) used by SING
   glyphlet fonts.

   The "uv"s in the "subset.array" are used to generate the cmap table in the
   embedding font. Some glyphs may legitimately not have a Unicode encoding if
   they are to be accessed directly. In such cases the "uv" field should be set
   to the value 0xFFFF.  If duplicate "id"s are encountered the pair with the 
   lowest "uv" value will be chosen.

   The "kern.cnt" field specifies the number of kerning pairs stored in the
   "kern.list" array parameter. Each element of the array specifies a pair of
   glyphs as IDs in the first and second fields and their kerning amount via
   the value field. The kerning IDs must be consistent with those in the
   "subset.array" field.

   The "map" parameter specifies an optional glyph mapping callback that
   provides information to the client about how the glyphs have been allocated
   in the CFF CharStrings INDEX. The glyphmap() callback function is called for
   each glyph in the INDEX, in order, beginning with the first at GID 0. The
   "gid" parameter specifies the glyph's index and the "info" parameter
   identifies the glyph. This facility may be disabled by setting the "map"
   parameter to NULL.

   cefMakeEmbeddingFont() returns 0 on success and a positive non-0 error code
   in the event of an error. The list of possible error codes is specified
   below. */

void cefFree(cefCtx h);

/* cefFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

int cefSetSvwFlags(cefCtx h, long flags);

/* cefSetSvwFlags() sets svgwrite library flags in the cfembed context. These
   flags are passed through to the svgwrite library when the CEF_WRITE_SVG
   flag is set in the cefEmbedSpec flags field in the call to the function
   cefMakeEmbeddingFont().

   cefSetSvwFlags() returns 0 on success and a positive non-0 error code
   in the event of an error. The list of possible error codes is specified
   below. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "ceferr.h"
    cefErrCount
    };

/* Library functions either return zero (cefSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from ceferr.h. */

char *cefErrStr(int err_code);

/* cefErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void cefGetVersion(ctlVersionCallbacks *cb);

/* cefGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

/* ------------------------ Example Callback Functions ------------------------

   This section illustrates a simple memory callback and simple file-based
   implemetation of the stream client callback functions. */

#if 0   /* Example */

#include <stdio.h>
#include <stdlib.h>
#include "cfembed.h"

typedef struct
    {
    FILE *fp;
    char buf[BUFSIZ];
    } Stream;
   
typedef struct
    {
    /* ... */
    char *srcfile;
    char *dstfile;
    struct
        {
        Stream src;
        Stream dst;
        Stream tmp0;
        Stream tmp1;
        } stm;
    } ClientCtx;

void *mem_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
    {
    if (size > 0)
        {
        if (old == NULL)
            return malloc(size);        /* size != 0, old == NULL */
        else                            
            return realloc(old, size);  /* size != 0, old != NULL */
        }                               
    else                                
        {                               
        if (old == NULL)                
            return NULL;                /* size == 0, old == NULL */
        else                            
            {                           
            free(old);                  /* size == 0, old != NULL */
            return NULL;
            }
        }
    }
    
void *stm_open(ctlStreamCallbacks *cb, int id, size_t size)
    {
    ClientCtx *h = cb->direct_ctx;
    Stream *s;
    switch (id)
        {
    case CEF_SRC_STREAM_ID:
        s = &h->stm.src;
        s->fp = fopen(h->srcfile, "rb");
        break;
    case CEF_DST_STREAM_ID:
        s = &h->stm.dst;
        s->fp = fopen(h->dstfile, "w+b");
        break;
    case CEF_TMP0_STREAM_ID:
        s = &h->stm.tmp0;
        s->fp = tmpfile();
        break;
    case CEF_TMP1_STREAM_ID:
        s = &h->stm.tmp1;
        s->fp = tmpfile();
        break;
        }
    return (s->fp == NULL)? NULL: s;
    }
   
int stm_seek(ctlStreamCallbacks *cb, void *stream, long offset)
    {
    Stream *s = stream;
    return fseek(s->fp, offset, SEEK_SET);
    }
   
long stm_tell(ctlStreamCallbacks *cb, void *stream)
    {
    Stream *s = stream;
    return ftell(s->fp);
    }
   
size_t stm_read(ctlStreamCallbacks *cb, void *stream, char **ptr)
    {
    Stream *s = stream;
    *ptr = s->buf;
    return fread(s->buf, 1, sizeof(s->buf), s->fp);
    }
   
size_t stm_write(ctlStreamCallbacks *cb, void *stream, size_t count, char *ptr)
    {
    Stream *s = stream;
    return fwrite(ptr, 1, count, s->fp) != count;
    }

int stm_status(ctlStreamCallbacks *cb, void *stream)
    {
    Stream *s = stream;
    if (feof(s->fp))
        return CTL_STREAM_END;
    else if (ferror(s->fp))
        return CTL_STREAM_ERROR;
    else 
        return CTL_STREAM_OK;
    }
   
int stm_close(ctlStreamCallbacks *cb, void *stream)
    {
    Stream *s = stream;
    return fclose(s->fp) == EOF;
    }

int main(void)
    {
    ClientCtx h;
    ctlMemoryCallbacks mem;
    ctlStreamCallbacks stm;
    cefCtx cef;

    mem.ctx = &h;
    mem.manage = mem_manage;
    
    stm.direct_ctx = &h;
    stm.indirect_ctx = NULL;
    stm.open = stm_open;
    stm.seek = stm_seek;
    stm.tell = stm_tell;
    stm.read = stm_read;
    stm.write = stm_write;
    stm.status = stm_status;
    stm.close = stm_close;

    /* Initilize h.srcfile and h.dstfile */

    cef = cefNew(&mem, &stm, CEF_CHECK_ARGS);
    if (cef == NULL)
        exit(1);    /* Can't initialize library */

    /* Use library */

    cefFree(cef);

    return 0;
    }
#endif /* Example */

#ifdef __cplusplus
}
#endif

#endif /* CFEMBED_H */
