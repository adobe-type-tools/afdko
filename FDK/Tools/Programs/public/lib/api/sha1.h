/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHA1_H
#define SHA1_H
#include <stdlib.h> /* to get size_t */

/**********************************************

Expected use...

    client calls sha1_init. sha1_init will use the memory callback
           to request memory for sha1_ctx. sha1_ctx will be initialized
           and a pointer to it will be returned to signal success.

    client calls sha1_update (with sha1_ctx pointer returned by sha1_init)
           one or more times with data to be included in the hash.
           Clients are expected to do reasonable buffering of input
           data. Calls may be made with 1 byte, but better performance
           will result with larger buffers (say 64 or more bytes).

    client calls sha1_finalize when all of the data to be hashed
           has been accumulated. sha1_finalize returns the hash
           via the client supplied sha1_hash and calls the client
           supplied sha1_free to return the work context.

Note: 
    client is responsible for the memory callbacks. In particular,
    the sha1_malloc and sha1_free procedures are expected to honor
    all of the ANSI C rules with respect to buffer alignment.

    The implementation is completely thread safe and reentrant.
    Separate client threads may safely interleave calls to the
    relevant sha1 procedures. A client, from one or more threads
    may have more than one hash accumulation in progress concurrently.
    (Of course, each hash accumulation must have its own context
    returned by sha1_init etc.)

    Data to be hashed and the resulting hash are treated as unsigned
    char data. This code will produce identical results on
    any system, regardless of byte order or word size. The client
    must understand and accomodate byte order issues if multibyte 
    binary data is hashed and compared on different platforms.

*******************************************************/


typedef struct sha1_ctx *sha1_pctx;   /* pointer to sha1 private work area */
                       /* the contents of this work area are private
                                  to the implementation of sha1 and must
                                  be preserved without modification between
                                  calls to sha1_update and sha1_finalize.
                                  The size of this work area is about 100 
                                  bytes. */

typedef void * sha1_malloc(size_t size, void *hook); 
                               /* client supplied callback procedure */
                               /* this client supplied malloc procedure
                                  has the same requirements as the standard C
                                  library malloc. The sha1 library depends on
                                  the client malloc procedure to honor alignment
                                  requirements. hook is a client context
                                  pointer passed through to the client
                                  callback proceedure unmodified by 
                                  sha1_init. */
typedef void   sha1_free(sha1_pctx ctx, void *hook);
                               /* client supplied callback procedure */
                               /* this client supplied free procedure is used
                                  to return the memory allocated during sha1_init
                                  via its callback to the client procedure.
                                  hook is a client contextpointer passed
                          through to the client callback proceedure
                          unmodified by sha1_free. */

typedef unsigned char sha1_hash[20];
                               /* hash is returned into this client provided
                                  space */

#ifdef __cplusplus
extern "C" {
#endif

sha1_pctx  sha1_init(sha1_malloc client_malloc, void *hook);
                               /* called once per hash generation */
                               /* returned context must be passed with 
                                  unmodified contents to subsequent calls */
                               /* sha1_malloc is client provided malloc 
                                  procedure */

                               /* return valid pointer for success, 
                                  NULL for failure.
                                  failure will occur 
                                     if client_malloc callback is NULL or
                                     if client_malloc callback returns NULL or
                                     if sizeof(Card32) != 4 (the implementation
                                        requires that a 4 byte unsigned integer
                                        type exists)
                                */


int sha1_update(sha1_pctx ctx, unsigned char *buffer, size_t len);
                               /* called 1 or more times to accumulate hash 
                                  value.  */

                               /* return 0 for success, 1 for failure.
                                  failure will occur if ctx is NULL or
                                  buffer is NULL and len > 0. */

int sha1_finalize(sha1_pctx ctx, sha1_free client_free, 
                  sha1_hash hash, void *hook);
                               /* called once after all data has been accumulated
                                  via calls to sha1 update. Note that hash value
                                  is returned to the client in hash and that the
                                  context (ctx) is freed via a call to 
                                  client_free */

                               /* return 0 for success, 1 for failure.
                                  failure will occur if callback is NULL or
                                  hash is NULL, or ctx is NULL */

#ifdef __cplusplus
}
#endif

/* Testing:

   This code has been tested on sparc sunos gcc
                                sparc solaris (sun cc product compiler)
                                Pentium G6/200 linux gcc
                                win32/ppro/vc6

   Test cases include all the samples in FIPS 180-1.

*/

/* Performance:

   On Pentium G6/200 linux gcc with 512 byte buffers passed to sha1_update
   hash rate is about 4,000,000 bytes/sec.

*/
#endif /* SHA1_H */
