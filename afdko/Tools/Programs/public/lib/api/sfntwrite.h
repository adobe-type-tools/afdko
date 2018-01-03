/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SFNTWRITE_H
#define SFNTWRITE_H

#include "ctlshare.h"

#define SFW_VERSION CTL_MAKE_VERSION(1,0,6)

#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* sfnt Writing Library
   ====================
   This library supports writing of sfnt-formatted streams.

   The library is initialized with a single call to sfwNew() which allocates an
   opaque context (sfwCtx) which is passed to subsequent functions and is
   destroyed by a single call to sfwFree(). Thus, multiple contexts may be
   concurrently allocated permitting operation in a multi-threaded environment.

   Member tables in the sfnt are registered by calling sfwRegisterTable() for
   each table. The new_table() table callbacks are called via sfwNewTables().
   Repeated sfnts can then be written by repeating calls to sfwFillTables(),
   sfwWriteTables() and the sfwReuseTables() functions which call the
   corresponding client callbacks. Finally, the free_table() table callbacks
   are called via the sfwFreeTables() function. */

typedef struct sfwCtx_ *sfwCtx;
sfwCtx sfwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb, 
              CTL_CHECK_ARGS_DCL);

#define SFW_CHECK_ARGS  CTL_CHECK_ARGS_CALL(SFW_VERSION)

/* sfwNew() initializes the library and returns an opaque context (sfwCtx) that
   is subsequently passed to all the other library functions. It must be the
   first function in the library to be called by the client. The client passes
   a set of memory and stream callback functions (described in ctlshare.h) to
   the library via the mem_cb and stm_cb parameters.

   The SFW_CHECK_ARGS macro is passed as the last parameter to sfwNew() in
   order to perform a client/library compatibility check. */

typedef struct sfwTableCallbacks_ sfwTableCallbacks;
struct sfwTableCallbacks_
    {
    void *ctx;
    ctlTag table_tag;
    int (*new_table)(sfwTableCallbacks *cb);
    int (*fill_table)(sfwTableCallbacks *cb, int *dont_write);
    int (*write_table)(sfwTableCallbacks *cb, 
					   ctlStreamCallbacks *stm_cb, void *stm,
					   int *use_checksum, unsigned long *checksum);
    int (*reuse_table)(sfwTableCallbacks *cb);
    int (*free_table)(sfwTableCallbacks *cb);
    int fill_seq;
    int write_seq;
    };

int sfwRegisterTable(sfwCtx h, sfwTableCallbacks *cb);

/* sfwRegisterTable() registers a callback set that manages an sfnt table. It
   must be the first library function to be called after calling sfwNew(). The
   "cb" parameter points to a sfwTableCallbacks structure, a copy of which is 
   saved by the library. This structure contains the following information:

   The "ctx" field can be used to specify a client context. Set to NULL if not
   used.

   The "table_tag" field specifies the 4-byte tag that is saved within the sfnt
   directory to identify the table.

   A set of five callback functions is provided to sequence the client code for
   a specific table. Only the fill_table() and write_table() callbacks are
   required, and the remaining callbacks may be disabled by setting their
   fields to NULL. A callback must return 0 to continue a callback sequence and
   1 to abort the sequence.
   
   The intended usage of the various callbacks is as follows:

   The optional new_table() callback is called to initialize the code for
   generating this table. For example, a client module and associated data
   structures could be initialized by this call, making it ready to accumulate
   table data. The new_table() callbacks are called as a result of the client
   calling the sfwNewTables() function described below.

   The required fill_table() callback is called to fill the data structures for
   this table. For example, the client could select the format and layouts of
   the data will subsequently be written by the write_table() callback.
   Typically, not all fonts will include all tables, e.g. 'vmtx' is only
   required for fonts supporting vertical layout. The client can determine if
   this table is needed by the font and communicate this to the library via the
   boolean "dont_write" parameter. If its value is 1 the table won't be
   included in the font and the write_table() callback will not be called. The
   fill_table() callback might do as little as just setting this parameter or
   as much as performing complicated calculations on the table data -- the
   choice is completely up to the client.

   The "fill_seq" field is associated with the fill_table() callback and is
   used to establish the order in which all the registered fill_table()
   callbacks are called, smallest first. If two or more callbacks share the
   same "fill_seq" value they are ordered by "table_tag", smallest first. The
   order in which the fill_table() callbacks are called would matter to a
   client if one fill_table() callback is dependent on another having been
   previously called to calculate some value that was needed. An example of
   such a dependency is the "numberOfHMetrics" field in the 'hhea' table which
   is dependent on how the 'hmtx' table is going to be formatted. If the client
   has no such dependencies all "fill_seq" fields can be set to the same value,
   e.g. 0, which will force the "table_tag" to be used to establish the order
   instead. The fill_table() callbacks are called as a result of the client
   calling the sfwFillTables() function described below.

   The required write_table() callback is called to write the table data to the
   output stream specified by the "stm" parameter using the callbacks specified
   via the "stm_cb" parameter. Per-table checksum calculations may be overriden
   by setting the value pointed to by the "checksum" parameters to the desired
   checksum for the table and setting the value pointed to by the
   "use_checksum" parameter to 1. The library initializes the value pointed to
   by the "use_checksum" parameter to 0 before the write_table() function is
   called so the library performs the checksum calculation by default. The
   write_table() callback will only be called if the the corresponding
   fill_table() callback returned a "dont_write" value of 0.

   The "write_seq" field is associated with the write_table() callback and is
   used to establish the order in which all the registered write_table()
   callbacks are called, smallest first. If two or more callbacks have the same
   "write_seq" value they are ordered by "table_tag", smallest first. An sfnt
   may be optimized by placing tables that are needed when the font is first
   loaded, early in the order. If a client doesn't wish to optimize the table
   order all "write_seq" fields can be set to the same value, e.g. 0, which
   will force the "table_tag" to be used to establish the order instead. The
   write_table() callbacks are called as a result of the client calling the
   sfwWriteTables() function described below.

   The optional reuse_table() callback is called to reinitialize a client table
   for another font. For example, data counts can be reset so as to be ready to
   accumulate new data. The reuse_table() callbacks are called as a result of
   the client calling the sfwReuseTables() function described below.
   
   The optional free_table() callback is called to free client resources
   associated with a table. For example, dynamic memory used by the table could
   be freed. The free_table() callbacks are called as a result of the client
   calling the sfwFreeTables() function described below. */

int sfwNewTables(sfwCtx h);

/* sfwNewTables() is called to call the new_table() callbacks that were
   previously registered. There is no need to call this function if no non-NULL
   new_table() callbacks were registered. */

int sfwFillTables(sfwCtx h);

/* sfwFillTables() is called to call the fill_table() callbacks that were
   previously registered. It must be called before the sfwWriteTables()
   function. */

int sfwWriteTables(sfwCtx h, void *stm, ctlTag sfnt_tag);

/* sfwWriteTables() is called to write the sfnt data by calling the
   write_table() callbacks that were previously registered. It must be called
   after the sfwFillTables() function is called.

   The "stm" parameter controls the destination stream usage (described in
   ctlshare.h). If NULL, the function will open the default destination stream
   identified by SFW_DST_STREAM_ID. If non-NULL, the client must open the
   source stream prior to calling sfwWriteTables() and pass the stream object
   to the function via the "stm" parameter, and must also close the stream
   after the call to sfwWriteTables() returns.

   The "sfnt_tag" parameter provides the value of the "version" field in the
   sfnt header. Example values are:

   'OTTO'     - OTF
   'true'     - TrueType (used by Apple)
   0x00010000 - TrueType (used by Microsoft)

   The destination stream is opened, the tables are written, the table
   checksums are computed, the sfnt header is written, the head table's
   "checkSumAdjustment" field is computed and written into the head table (if
   present), and finally the destination stream is closed. 

   The library will automatically pad the table data to a 4-byte boundary by
   writing up to 3 zero-bytes to the stream after the per-table data is
   written. Note that both read and write operations are applied to the stream
   which must therefore use a "w+b" mode parameter to fopen() if standard
   C library stream I/O is used. */

int sfwReuseTables(sfwCtx h);

/* sfwReuseTables() is called to call the reuse_table() callbacks that were
   previously registered. There is no need to call this function if no non-NULL
   reuse_table() callbacks were registered. */

int sfwFreeTables(sfwCtx h);

/* sfwFreeTables() is called to call the free_table() callbacks that were
   previously registered. There is no need to call this function if no non-NULL
   free_table() callbacks were registered. */

void sfwFree(sfwCtx h);

/* sfwFree() destroys the library context and all the resources allocated to
   it. It must be the last function called by a client of the library. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "sfwerr.h"
    sfwErrCount
    };

/* Library functions return either 0 to indicate success or a positive non-zero
   error code that is defined in the above enumeration that is built from
   sfwerr.h. */

char *sfwErrStr(int err_code);

/* sfwErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void sfwGetVersion(ctlVersionCallbacks *cb);

/* sfwGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* SFNTWRITE_H */
