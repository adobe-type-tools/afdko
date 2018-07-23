/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef PSTOKEN_H
#define PSTOKEN_H

#include "ctlshare.h"

#define PST_VERSION CTL_MAKE_VERSION(2,0,9)

#ifdef __cplusplus
extern "C" {
#endif

/* PostScript Tokenizer Library
   ============================
   The PostScript tokenizer library parses a PostScript input stream into
   top-level PostScript tokens. Top-level means that the parser doesn't descend
   into compound objects but rather treats them as single tokens. The compound
   objects are listed below:

   Object                   Delimiters
   ------                   ----------
   string                   (    )
   hex-encoded string       <    >
   ASCII85-encoded string   <~  ~>
   dictionary               <<  >>
   array                    [    ]
   procedure                {    }

   Whitespace and comments (except the first one if it begins with %!) are
   skipped by the tokenizer.

   This library is initialized with a single call to pstNew() which allocates a
   opaque context (pstCtx) which is passed to subsequent functions and is
   destroyed by a single call to pstFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Once a context has been allocated a client may begin the parse by calling
   pstBegParse() which will open the source stream. The client may then call
   the the pstGetToken() or pstFindToken() functions to tokenize the source
   stream. Tokens are returned to the client as a type and a value
   (length-delimited string). The value is not persistent and should be copied
   by the client before the next token is read if it is needed later. The
   client may call the pstConv*() functions to convert a token to a more usable
   form when needed. Finally, the pstEndParse() function should be called to
   close the input stream.

   Memory management and data input are implemented by two sets of
   client-supplied callback functions passed to pstNew().

   Multiple PostScript streams may be parsed serially by opening and closing
   new streams with calls pstBegParse() and pstEndParse(). */

typedef struct pstCtx_ *pstCtx;
pstCtx pstNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              int src_stream_id, CTL_CHECK_ARGS_DCL);

#define PST_CHECK_ARGS CTL_CHECK_ARGS_CALL(PST_VERSION)

/* pstNew() initializes the library and returns an opaque context (pstCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   sets of memory and stream callback functions (described in ctlshare.h) to
   the library via the "mem_cb" and "stm_cb" parameters.

   The PostScript source stream is identified by the "src_stream_id" parameter
   which is passed back to the client when the source stream is opened.

   The PST_CHECK_ARGS macro is passed as the last parameter to pstNew() in
   order to perform a client/library compatibility check. pstNew() returns NULL
   if it could not be initialized. */

int pstBegParse(pstCtx h, long origin);

/* pstBegParse() begins a new parse by opening the PostScript source stream
   with the "src_stream_id" parameter that was passed to pstNew() and
   positioning the stream at the offset specified by the "origin" parameter.
   This stream is subsequently closed by calling pstBegParse(). This function
   returns 0 if successful or an error code otherwise. */

int pstEndParse(pstCtx h);

/* pstEndParse() ends a parse begun with pstBegParse() by closing the
   PostScript source stream. The client may serially parse multiple streams
   with a sequence of pstBegParse() and pstEndParse() calls. This function
   returns 0 if successful or an error code otherwise. */

int pstSetDecrypt(pstCtx h);

/* pstSetDecrypt() inserts an eexec decryption filter into the input stream so
   that eexec encrypted PostScript tokens may be parsed. This would normally be
   called after reading the operator token sequence "currentfile eexec". The
   decryption filter may be removed by calling pstSetPlain(). This function
   returns 0 if successful or an error code otherwise.
   
   Note: calling this function inappropriately will result in garbage input
   data being presented to the parser. */

int pstSetPlain(pstCtx h);

/* pstSetPlain() removes the decryption filter inserted by pstSetDecrypt(). It
   is normally called after reading the operator token sequence "currentfile
   closefile". The decryption sequence may be reinstalled by calling
   pstSetDecrypt(). This function returns 0 if successful or an error code
   otherwise. */

int pstRead(pstCtx h, size_t count, char **ptr);

/* pstRead() reads a number of bytes specified by the "count" parameter from
   the source stream without interpreting stream as PostScript tokens. Bytes
   are counted AFTER the decryption filter, if any, has been applied to the
   source stream. This function is normally used to read binary data sections
   in the input stream, charstrings, for example. This function returns 0 if
   successful or an error code otherwise. */

typedef enum
    {
    pstInteger,     /* Integer number */                            
    pstReal,        /* Real number */                               
    pstLiteral,     /* /... */                                      
    pstImmediate,   /* //... */                                     
    pstString,      /* ( ... ) */                                   
    pstHexString,   /* < ... > */                                   
    pstASCII85,     /* <~ ... ~> */                                 
    pstDictionary,  /* << ... >> */                                 
    pstArray,       /* [ ... ] */                                   
    pstProcedure,   /* { ... } */                                   
    pstOperator,    /* Executable name, i.e. not one of the above */
    pstDocType,     /* %!... (first line only) */                   
    pstTokenCnt
    } pstType;

/* Input token */
typedef struct
    {
    pstType type;
    long length;
    char *value;
    } pstToken;

/* Several functions use the pstToken data type to describe a single PostScript
   token. A token has a type, specified by the type field, which holds one of
   the pstType enumerations, and a value, specified by the value and length
   fields, including self-delimiting characters, if any.

   The pstDocType token can only be returned if the first comment encountered
   in the stream begins with "%!". The client can parse the token value in
   order to determine the PostScript document type. */

int pstGetToken(pstCtx h, pstToken *token);

/* pstGetToken() reads the next token from the input stream. The token's value
   field isn't guaranteed to be persistent and should be copied by the client
   if its value is needed beyond the next pstGetToken() call. This function
   returns 0 if successful or an error code otherwise. */

int pstFindToken(pstCtx h, pstToken *token, char *value);

/* pstFindToken() returns the next token in the input stream that matches the
   token specified by the "value" parameter. If the matching token is found 0
   is returned otherwise an error code is returned (most likely pstEndStream).
   The value parameter is specified as a null-terminated string. */

int pstMatch(pstCtx h, pstToken *token, char *value);

/* pstMatchValue() returns 1 if the "token" parameter's value field matches the
   null-terminated string specified by the "value" parameter, otherwise it
   returns 0. The value parameter is specified as a null-terminated string. */

int32_t pstConvInteger(pstCtx h, pstToken *token);

/* pstConvInteger() converts the integer "token" parameter to an integer
   number. If the token isn't an integer, 0 is returned. Integers requiring
   more than 32 bits of precision may overflow on certain platforms. */

double pstConvReal(pstCtx h, pstToken *token);

/* pstConvReal() converts the real "token" parameter to a real number. If the
   token isn't a real 0.0 is returned. See the strtod() standard C library
   function for a description of the values returned on overflow or 
   underflow conditions. */

char *pstConvString(pstCtx h, pstToken *token, long *length);

/* pstConvString() forms a new string by removing the delimiters from the
   string "token" parameter and returns a new pointer and length accordingly.
   If the token isn't a string, NULL is returned. */

char *pstConvLiteral(pstCtx h, pstToken *token, long *length);

/* pstConvLiteral() forms a new string by removing the leading / from the
   literal "token" parameter and returns a new pointer and length accordingly.
   If the token isn't a literal, NULL is returned. */
 
uint32_t pstConvHexString(pstCtx h, pstToken *token);

/* pstConvHexString() converts the hexadecimal string "token" parameter to an
   integer number. If the token isn't a hexadecimal string 0 is returned.
   Converted hexadecimal strings requiring more than 32 bits of precision may
   overflow on certain platforms. */

int pstGetHexLength(pstCtx h, pstToken *token);

/* pstGetHexLength() counts the number of hexadecimal digits in the hexadecimal
   string "token" parameter. If the token isn't a hexadecimal string 0 is
   returned. */

void *pstHijackStream(pstCtx h, long *offset);

/* pstHijackStream() returns the source stream and the current stream offset
   via the "offset" parameter to the client. This permits the client to regain
   control of the parsing process by directly reading bytes from the stream,
   abandoning the tokenizing services offered by the library. Tokeninzing of
   the stream cannot be resumed after calling this function and therefore the
   client can only call pstEndParse() when finished. NULL is returned if a
   stream error is encountered.

   This function is useful for handling CID-keyed fonts which begin with a
   PostScript section followed by a large binary section. */

void pstSetDumpLevel(pstCtx h, int level);

/* pstSetDumpLevel() permits the client to control the dumping of PostScript
   tokens to stdout as they are parsed via the "level" parameter. Setting a
   "level" of 1 enables dumping and 0 disables dumping. */

void pstDumpToken(pstToken *token);

/* pstDumpToken() permits the client to print token type and value from the
   "token" parameter on stdout. */

int pstFree(pstCtx h);

/* pstFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "psterr.h"
    pstErrCount
    };

/* Library functions return either 0 to indicate success or a positive non-zero
   error code that is defined in the above enumeration that is built from
   psterr.h. */

char *pstErrStr(int err_code);

/* pstErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void pstGetVersion(ctlVersionCallbacks *cb);

/* pstGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* PSTOKEN_H */
