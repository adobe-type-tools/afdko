/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CTLSHARE_H
#define CTLSHARE_H

#ifdef __EPOC32__
/* The Symbian platform does not define NULL in stddef.h (an is therefore
   non-standard) so we use an alternative definition that is available
   stdlib.h. */
#include <stdlib.h>
#else
#include <stddef.h>     /* For size_t and NULL */
#endif
#include <setjmp.h>

/* CoreType Library Shared Definitions
   ===================================
   This library supplies definitions that are shared between members of the
   CoreType library suite. */

/* ---------------------------- Version Numbers ---------------------------- */

/* A version number is a long value that encodes minor, branch, and major
   numbers into bytes of increasing significance, starting with the least
   significant byte, respectively. The most significant byte is unused and
   should be set to 0. */

#define CTL_MAKE_VERSION(major,branch,minor) \
    ((long)major<<16|branch<<8|minor)

/* CTL_MAKE_VERSION combines the "major", "branch", and "minor" parameters into
   a single value of type long. */

#define CTL_SPLIT_VERSION(version) \
    (int)((version)>>16&0xff), \
    (int)((version)>>8&0xff), \
    (int)((version)&0xff)

/* CTL_SPLIT_VERSION splits the "version" parameter into its comma-separated
   major, branch, and minor component values that may be passed to printf and
   printed using a %d.%d.%d format. */

/* --------------------------------- CDECL --------------------------------- */

#ifndef CTL_CDECL
#define CTL_CDECL
#endif /* CTL_CDECL */

/* C code in the Microsoft environment may be sped up if compiled using Pascal
   calling conventions (/Gr compiler option). However, standard library
   callback functions such as those used by qsort(), bsearch(), main(), and
   varargs functions must still use the C calling conventions. In these cases
   the affected functions can be indicated to the compiler by using the
   non-standard __cdecl keyword before function name.

   The CoreType libararies have been written so that the affected functions
   include the CTL_CDECL macro before their function names. Therefore, clients
   wishing to compile on the Microsoft platform using Pascal calling
   conventions can simply define CTL_CDECL as __cdecl to obtain the correct
   behavior when the code is compiled with the /Gr compiler option. This may be
   simply achieved with the /D CTL_CDECL=__cdecl compiler option. */

/* ---------------------------- Memory Callbacks --------------------------- */

typedef struct ctlMemoryCallbacks_ ctlMemoryCallbacks;
struct ctlMemoryCallbacks_
    {
    void *ctx;
    void *(*manage)(ctlMemoryCallbacks *cb, void *old, size_t size);
    };

/* The ctlMemoryCallbacks structure specifies a single memory management
   callback function and a context.

   [optional] The "ctx" field can be used to specify a client context. Set to
   NULL if not used.

   [required] The manage() callback manages memory in a similar manner to the
   realloc() ISO C Standard library function. Its operation is defined (in
   standard library function terms) as follows: 
   
   old      size    action      return
   ---      ----    ------      ------
   ==NULL   ==0     none        NULL
   ==NULL   !=0     malloc      allocation, or NULL if allocation failed
   !=NULL   ==0     free        NULL
   !=NULL   !=0     realloc     reallocation, or NULL if reallocation failed

   The "cb" parameter is the address of the ctlMemoryCallbacks structure
   containing the function being called. This parameter can be used by the
   client to access the ctx field.

   This function must observe the alignment requirements imposed by the C
   Standard for memory allocators. 

   If the client has access to a conformant ISO C realloc() function a possible
   implementation is as follows:

   void *manage(ctlMemoryCallbacks *cb, void *old, size_t size)
       {
       return (old == NULL && size == 0)? NULL: realloc(old, size);
       }

   The condition is required because the ISO C Standard doesn't define a single
   behavior for that case and the CoreType libraries rely on NULL being
   returned.

   If a client is unsure about compliance with the ISO C standard a safer
   implementation is as follows:

   void *manage(ctlMemoryCallbacks *cb, void *old, size_t size)
       {
       if (size > 0)
           {
           if (old == NULL)
               return malloc(size);
           else                         
               return realloc(old, size);
           }                                
       else                             
           {                                
           if (old == NULL)             
               return NULL;
           else                         
               {                            
               free(old);
               return NULL;
               }
           }
       }
   */

/* --------------------------- Stream Callbacks ---------------------------- */

typedef struct ctlStreamCallbacks_ ctlStreamCallbacks;
struct ctlStreamCallbacks_
    {
    void *direct_ctx;
    void *indirect_ctx;
    char *clientFileName; // set by clients which need to open private files.
    void *(*open)   (ctlStreamCallbacks *cb, int id, size_t size);
    int (*seek)     (ctlStreamCallbacks *cb, void *stream, long offset);
    long (*tell)    (ctlStreamCallbacks *cb, void *stream);
    size_t (*read)  (ctlStreamCallbacks *cb, void *stream, char **ptr);
    size_t (*write) (ctlStreamCallbacks *cb,
                     void *stream, size_t count, char *ptr); 
    int (*status)   (ctlStreamCallbacks *cb, void *stream);
    int (*close)    (ctlStreamCallbacks *cb, void *stream);
    };

/* The ctlStreamCallbacks structure specifies a standard set of functions that
   perform I/O on abstract data streams. They are modeled on the Standard C
   library functions of the same name but do not need to be implemented using
   files. A client, for example, could choose to keep the data in memory
   buffers. 

   A stream is identified by a unique "id" parameter that is provided by the
   open() call. The client turns this into a client-specific pointer that is
   returned by open(). This value is associated by the library with that stream
   and is passed as the "stream" parameter in subsequent calls. When a library
   has finished using a stream it calls the close() function.

   Each library API file will specify the purpose and id for each stream it
   uses. The direction of data movement is encoded in the stream id's
   enumeration constant: _SRC_ meaning that the library is reading on that
   stream, _DST_ meaning that the library is writing on that stream, and _TMP_
   meaning a write/read temporary stream.

   [optional] The "direct_ctx" field can be used to specify a client context.
   Set to NULL if not used.

   [not used by clients] The "indirect_ctx" field is be used internally by
   libraries that act as a bridge between the client and another library that
   is hidden from the client. Clients must set this field to NULL.

   [required] open() is called in order for the client to initialize for a
   stream. The stream being opened is identified by the "id" parameter using an
   enumeration constants defined in the API. The client performs the necessary
   initialization for the stream and returns a pointer to a client object that
   represents the stream, e.g. a file pointer, or NULL if the stream couldn't
   be opened. The "size" parameter when set to a non-0 value specifies the
   total size (in bytes) of output data that will be written to this stream
   (when known). It permits a memory-based client to allocate the exact amount
   of space for the embedding font in memory, for example.

   [required] seek() is called in order to seek to an absolute byte position in
   a stream specified by the "offset" parameter. It returns 0 on success and
   non-0 otherwise.

   [required] tell() returns the absolute byte position of a stream. It returns
   -1 if the operation couldn't be performed on the stream.

   [required] read() is called in order to read bytes from a stream. It returns
   the number of bytes read and a pointer to a buffer, via the "ptr" parameter,
   that contains those bytes. The client should return a count of 0 in the
   event of an end-of-stream condition or if the stream cannot be read. A
   library distinguishes these conditions by calling status(). The client must
   arrange that successive calls to read() return consecutive blocks of data
   from the source stream. (Note, this scheme allows a client to choose a
   buffer size for input that is optimized for the client's implementation.
   However, very small buffer sizes are likely to be inefficient and should be
   avoided.)

   [required] write() is called in order to write data to a stream. The "ptr"
   parameter points to a buffer of data to be written and the "count" parameter
   specifies the number of bytes available. The function returns the count
   parameter on success and 0 otherwise.

   [required] close() is called to close a previously opened stream. The
   function returns 0 on success and -1 on failure.

   [required] status() returns the status of a stream. The returns values are
   specified by the enumerations: */

enum                    /* Stream status */
    {
    CTL_STREAM_OK,      /* Functioning normally */
    CTL_STREAM_ERROR,   /* Error occurred */
    CTL_STREAM_END      /* At end-of-stream */
    };

/* status() is normally called after the read() callback returned 0 in order to
   distinguish end-of-stream from a read error.

   [required] close() is called to tear down the client's implementation of a
   stream. Typically, a client would release any resources associated with the
   stream. The function returns 0 on success and -1 otherwise. */

enum
    {
    T1R_SRC_STREAM_ID,  /* t1read */
    T1R_TMP_STREAM_ID,
    T1R_DBG_STREAM_ID,

    TTR_SRC_STREAM_ID,  /* ttread */
    TTR_DBG_STREAM_ID,

    CFR_SRC_STREAM_ID,  /* cffread */
    CFR_DBG_STREAM_ID,

    SFR_SRC_STREAM_ID,  /* sfntread (default) */

    T1W_DST_STREAM_ID,  /* t1write */
    T1W_TMP_STREAM_ID,
    T1W_DBG_STREAM_ID,

    CFW_DST_STREAM_ID,  /* cffwrite */
    CFW_TMP_STREAM_ID,
    CFW_DBG_STREAM_ID,

    PDW_DST_STREAM_ID,  /* pdfwrite */

    SFW_DST_STREAM_ID,  /* sfntwrite */

    SVW_DST_STREAM_ID,  /* svgwrite */
    SVW_TMP_STREAM_ID,
    SVW_DBG_STREAM_ID,

    CEF_SRC_STREAM_ID,  /* cfembed */
    CEF_DST_STREAM_ID,
    CEF_TMP0_STREAM_ID,
    CEF_TMP1_STREAM_ID,

    T1C_SRC_STREAM_ID,  /* t1cstr */

    T2C_SRC_STREAM_ID,  /* t2cstr */

	BZR_SRC_STREAM_ID,  /* bezread */
	BZR_DBG_STREAM_ID,

    SVR_SRC_STREAM_ID,  /* svgread */
    SVR_TMP_STREAM_ID,
    SVR_DBG_STREAM_ID,
        
    UFO_SRC_STREAM_ID, /* ufo read format */
    UFO_DBG_STREAM_ID,
        
    UFW_DST_STREAM_ID,  /* ufowrite */
    UFW_TMP_STREAM_ID,
    UFW_DBG_STREAM_ID,

    CTL_SRC_STREAM_ID,  /* ctlshare source stream */
    
    CTL_STREAM_CNT
    };

/* These enumeration constants are used by each library API file to establish a
   unique stream ids. A library client will use these to identify which stream
   is being opened by the stream open() callback. */

/* -------------------------- Stream Data Access -------------------------- */

typedef struct
    {
    long cnt;
    long *offset;   /* [cnt] */
    } ctlSubrs;

/* The t1cstr and t2cstr libraries obtain charstring data via the stream
   interface described above. Consequently, character and subroutine
   charstrings are specified by a stream offset. A set of subroutines is
   specified with the ctlSubrs data structure as an array of offsets via the
   "cnt" and "offset" fields. If there are no subroutines the "cnt" and
   "offset" field must be set to 0.

   The cffread library guarantees that the offsets in the "offset" array are in
   increasing order but no such guarantee
   is made by the t1read library. */

typedef struct
    {
    long begin;
    long end;
    } ctlRegion;

/* A ctlRegion struct specifies a set of consecutive bytes within a stream. The
   "begin" field is the offset of the first byte in the set and the "end" field
   is one past the last byte in the set. Several of the libraries return
   ctlRegion's so that clients may access objects in the stream data directly.
   An empty region is indicated by setting both fields to -1. */

typedef struct ctlSharedStmCallbacks_ ctlSharedStmCallbacks;
struct ctlSharedStmCallbacks_
    {
    void *direct_ctx;           /* host library context */

    struct dnaCtx_ *dna;        /* dynarr context */

    /* Allocate memory. */
    void* (*memNew)(ctlSharedStmCallbacks *h, size_t size);

    /* Free memory. */
    void (*memFree)(ctlSharedStmCallbacks *h, void *ptr);

    /* Seek to an offset in a source stream. */
    void (*seek)(ctlSharedStmCallbacks *h, long offset);

    /* Return absolute byte position in stream. */
    long (*tell)(ctlSharedStmCallbacks *h);

    /* Copy count bytes from source stream. */
    void (*read)(ctlSharedStmCallbacks *h, size_t count, char *ptr);

    /* Read 1-byte unsigned number. */
    unsigned char (*read1)(ctlSharedStmCallbacks *h);

    /* Read 2-byte number. */
    unsigned short (*read2)(ctlSharedStmCallbacks *h);

    /* Read 4-byte unsigned number. */
    unsigned long (*read4)(ctlSharedStmCallbacks *h);

    /* Error message. */
    void (*message)(ctlSharedStmCallbacks *h, char *msg, ...);
    };

/* ctlSharedStmCallbacks is used for reading SFNT tables shared by multiple font formats.
 * Unlike other font reading libraries, none of stream data and stream reading functions are
 * implemented in this library. Instead they are implmented by each host library as callbacks.
 */

/* ----------------------------- Tag Constants ----------------------------- */

/* Several Apple data structures use the concept of a tag which is simply a
   32-bit big endian integer constant that is expressed as four 8-bit
   charaters. Macintosh compilers normally provide a C language extension for
   creating tag constants, e.g. 'sfnt'. This is convienient but non-portable.
   The following definition, while a little more awkward to use, is portable
   and creates an integer constant so can be used anywhere a constant is
   required, e.g. as an enumeration value. Example usage: 
   CTL_TAG('s','f','n','t') . */

typedef unsigned long ctlTag;
#define CTL_TAG(a,b,c,d) \
    ((ctlTag)(a)<<24|(ctlTag)(b)<<16|(ctlTag)(c)<<8|(ctlTag)(d))

#define CTL_SPLIT_TAG(tag) \
    (int)((tag)>>24&0xff), \
    (int)((tag)>>16&0xff), \
    (int)((tag)>>8&0xff), \
    (int)((tag)&0xff)

/* CTL_SPLIT_TAG splits the "tag" parameter into its 4 comma-separated
   characters so that it may be passed to printf and printed using a %c%c%c%c
   format. */

/* ------------------------ Error Codes and Strings ------------------------ */

/* All the library functions return either 0 (afbSuccess) to indicate success
   or a positive non-zero error code that is defined by a set of enumeration
   constants. Each library also provides a function to map the error code to a
   null-terminated error string.

   In order to keep the error code and error string definitions in sync they
   are defined in a speparate include file by a macro called CTL_DCL_ERR that
   takes a name and a string parameter.

   By defining the CTL_DCL_ERR macro to select only one of its two parameters
   it is possible to build either an enumeration of the error codes or an
   initializer for an error strings array.

   If the macro is defined as:

   #define CTL_DCL_ERR(name,string) name,

   before the file is included an enumeration may be built. If the macro is
   defined as:

   #define CTL_DCL_ERR(name,string) string,

   before the file is included an initializer may be built. */

/* ---------------------- Client/Library Compatibiliy ---------------------- */

struct ctlPadStruct
    {
    char c;
    short s;
    long l;
    void *p;
    float f;
    double d;
    };

#define CTL_CHECK_ARGS_DCL \
    long version_arg, \
    size_t size_short, size_t size_long, size_t size_ptr, \
    size_t size_float, size_t size_double, size_t size_struct

#define CTL_CHECK_ARGS_CALL(version) \
    version, \
    sizeof(short),sizeof(long),sizeof(void*),sizeof(float),sizeof(double), \
    sizeof(struct ctlPadStruct)

#define CTL_CHECK_ARGS_TEST(version) \
    version != version_arg || \
    sizeof(short) != size_short || \
    sizeof(long) != size_long || \
    sizeof(void*) != size_ptr || \
    sizeof(float) != size_float || \
    sizeof(double) != size_double || \
    sizeof(struct ctlPadStruct) != size_struct

/* It is possible to compile library code and client code using different and
   incompatible options that will not be detected by the compiler or linker.
   This can lead to strange runtime errors that are hard to diagnose. The above
   structure and macros are used to implement a compatibility check between the
   client and the library when the library is initialized.

   CTL_CHECK_ARGS_DCL macro is used to declare the compatibility checking args
   in the library initialization function usually call <lib-prefix>New(). The
   CTL_CHECK_ARGS_CALL macro is used to pass the compatibility checking args to
   the called library initialization function. The CTL_CHECK_ARGS_TEST macro is
   used within the library to peform the validation test.

   The following incompatibilities are checked with this mechanism:

   o APIs have different versions
   o basic C types have different sizes
   o structure padding differs 
*/

/* ---------------------- Library Version Information ---------------------- */

/* There are complicated dependencies between the CoreType library components
   so it becomes important to track the component versions are linked into an
   application. The interface described below addresses this requirement by
   defining a recursive version callback function and an enumeration that
   uniquely identifies library components. */

typedef struct ctlVersionCallbacks_ ctlVersionCallbacks;
struct ctlVersionCallbacks_
    {
    void *ctx;
    unsigned long called;
    void (*getversion)(ctlVersionCallbacks *cb, long version, char *libname);
    };

enum
    {
    DNA_LIB_ID,
    PST_LIB_ID,
    CTU_LIB_ID,
    ABF_LIB_ID,
    T1R_LIB_ID,
    CFR_LIB_ID,
    TTR_LIB_ID,
    T1C_LIB_ID,
    T2C_LIB_ID,
    CFW_LIB_ID,
    CEF_LIB_ID,
    PDW_LIB_ID,
    T1W_LIB_ID,
    SFR_LIB_ID,
    SFW_LIB_ID,
	BZR_LIB_ID,
    SVR_LIB_ID,
    SVW_LIB_ID,
    UFR_LIB_ID,
    UFW_LIB_ID,
    CTL_LIB_ID,
    CTL_LIB_ID_CNT
    };

/* Each library component provides a function to obtain both the component's
   own version and also the versions of all the components it is dependent
   upon. The function is defined as follows (where xxx is replaced by the
   components unique prefix):

   void xxxGetVersion(ctlVersionCallbacks *cb);

   A client passes a callback set via the "cb" parameter. This structure
   contains an optional client "ctx" field. It also contains a
   "called"-component flags field. This must be initialized to 0 before making
   the first xxxGetVersion() call. As each component's version is called back a
   unique bit representing the component is set in the "called" field which is
   subsequently used to ensure that that only one callback is made per
   component.

   The "getversion" callback returns the library name and its version number to
   the client via the "libname" and "version" parameters. The callback set is
   also available via the "cb" parameter so that the client may access the
   "ctx" field if neccessary. */

#endif /* CTLSHARE_H */
