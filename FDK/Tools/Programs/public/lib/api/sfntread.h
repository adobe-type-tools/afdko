/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SFNTREAD_H
#define SFNTREAD_H

#include "ctlshare.h"

#define SFR_VERSION CTL_MAKE_VERSION(1,0,5)

#ifdef __cplusplus
extern "C" {
#endif

/* sfnt Format Table Handler
   =========================
   This library provides facilities for reading and returning table information
   found in the header of sfnt-formatted data. 

   This library is initialized with a single call to sfrNew() which allocates
   an opaque context (sfrCtx) which is passed to subsequent functions and is
   destroyed by a single call to sfrFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   A new font is opened and the header is parsed and cached by calling
   sfrBegFont(). Table data described by the sfrTable data structure may then
   be accessed by using the sfrGetNextTable() and the sfrGetTableByTag()
   functions. Finally, a font may be closed by calling sfrEndFont(). 

   The library also provides support for reading TrueType Collections via the
   sfrGetNextTTCOffset() function. */

typedef struct sfrCtx_ *sfrCtx;
sfrCtx sfrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define SFR_CHECK_ARGS CTL_CHECK_ARGS_CALL(SFR_VERSION)

/* sfrNew() initializes the library and returns an opaque context (sfrCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the "mem_cb" and "stm_cb" parameters. If the library
   couldn't be initialized, NULL is returned.

   The SFR_CHECK_ARGS macro is passed as the last parameter to sfrNew() in 
   order to perform a client/library compatibility check. */

int sfrBegFont(sfrCtx h, void *stm, long origin, ctlTag *sfnt_tag);

/* sfrBegFont() is called to initiate a new sfnt parse. The "stm" parameter
   controls source stream usage (described in ctlshare.h). If NULL, the
   function will open the default source stream identified by SFR_SRC_STEAM_ID
   in sfrBegFont() and close it in sfrEndFont(). If non-NULL, the client must
   open the source stream and pass the stream object to the function via the
   "stm" parameter, and must also close the stream after sfrEndFont() is
   called. The "origin" parameter specifies the stream offset of the first byte
   of the sfnt header. This offset is added to subsequent table offsets
   returned to the client. The "sfnt_tag" parameter gets the 4-byte version tag
   field from the sfnt header. If it doesn't match one of the known sfnt types,
   listed below, the function returns an error. */

#define sfr_v1_0_tag    CTL_TAG(  0,  1,  0,  0)    /* TrueType version 1.0 */
#define sfr_true_tag    CTL_TAG('t','r','u','e')    /* TrueType */
#define sfr_OTTO_tag    CTL_TAG('O','T','T','O')    /* OTF */
#define sfr_typ1_tag    CTL_TAG('t','y','p','1')    /* Type 1 GX */
#define sfr_ttcf_tag    CTL_TAG('t','t','c','f')    /* TrueType Collection */

/* A version tag of 'ttcf' indicates that the data represents a TrueType
   Collection rather than a single sfnt. In this case the client must call
   sfrGetNextTTCOffset() to obtain an offset of one of the sfnt members of the
   collection. */

long sfrGetNextTTCOffset(sfrCtx h);

/* sfrGetNextTTCOffset() is called to iterate through the offsets in TrueType
   Collection table directory. It must only be called if a previous call to
   sfrBegFont() returned a version tag value of 'ttcf'. Repeated calls to
   sfrGetNextTTCOffset() return succesive offsets from the table directory
   stating with the first. When all the offsets in the table directory have
   been returned, the function returns 0 indicating the end of the list. The
   next call will then wrap round to the beginning of the list and return the
   first offset again.

   A non-zero offset returned by sfrGetNextTTCOffset() can be passed as the
   origin parameter to sfrBegFont() in order to parse one of the members of a
   TrueType collection. Alternating calls to sfrGetNextTTCOffset() and
   sfrBegFont() allow each TrueType Collection member sfnt to be parsed in
   turn. 

   Note that the "stm" parameter is ignored in all but the first call to
   sfrBegFont() when navigating TTC streams.

   The offset returned by sfrGetNextTTCOffset() are adjusted by adding the
   "origin" parameter passed to sfrBegFont() so that it always specifies the
   absolute source data stream offset of the member sfnt. */

typedef struct
    {
    ctlTag tag;
    unsigned long checksum;
    unsigned long offset;
    unsigned long length;
    } sfrTable;

/* The sfrTable data structure represents the per-table data that is read from
   the sfnt table directory and returned to the client. The "offset" field is
   adjusted by adding the "origin" parameter passed to sfrBegFont() so that it
   always specifies the absolute source data stream offset of the table
   identified by the "tag" field. */

sfrTable *sfrGetNextTable(sfrCtx h);

/* sfrGetNextTable() is called to iterate through all the tables in the table
   directory of the sfnt. It must only be called if the last call to
   sfrBegFont() returned a valid tag whose value wasn't 'ttcf'. Repeated calls
   to sfrGetNextTable() return succesive table directory entries starting with
   the first. When all the entries in the table directory have been returned,
   the fuction returns NULL indicating the end of the list. The next call will
   then wrap round to the beginning of the list and return the first entry
   again. */

sfrTable *sfrGetTableByTag(sfrCtx h, ctlTag tag);

/* sfrGetTableByTag() may be called to obtain the table directory entry for a
   table specified by the "tag" parameter. NULL is returned if there is no
   matching table.

   When the client has obtained a non-NULL table directory entry by calling
   either sfrGetNextTable() or sfrGetTableByTag() the table data can be
   accessed by seeking to the offset given by the "offset" field. This offset
   is automatically converted from a relative offset to and absolute offset by
   adding in the "origin" offset supplied to the sfrBegFont() call. */

int sfrEndFont(sfrCtx h);

/* sfrEndFont() is called to terminate the current sfnt parse initiated by
   sfrBegFont(). */

void sfrFree(sfrCtx h);

/* sfrFree() destroys the library context and all the resources allocated to
   it. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "sfrerr.h"
    sfrErrCount
    };

/* Library functions return either zero (sfrSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from sfrerr.h. */

char *sfrErrStr(int err_code);

/* sfrErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void sfrGetVersion(ctlVersionCallbacks *cb);

/* sfrGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* SFNTREAD_H */
