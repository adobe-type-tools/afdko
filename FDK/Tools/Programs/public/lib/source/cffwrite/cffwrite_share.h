/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFFWRITE_SHARE_H
#define CFFWRITE_SHARE_H

#include "cffwrite.h"
#include "dynarr.h"
#include "ctutil.h"

#include <setjmp.h>

/* --------------------------- Shared Definitions -------------------------- */

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

typedef unsigned char OffSize;  /* Offset size indicator */
typedef long Offset;            /* 1-4 byte offset */

typedef unsigned short SRI;     /* String record index */
#define SRI_UNDEF   0xffff      /* SRI of undefined string */

typedef unsigned short SID;     /* String id */
#define SID_SIZE    2           /* SID CFF size */
#define SID_UNDEF   0xffff      /* SID of undefined string */
#define SID_NOTDEF  0           /* SID for .notdef */

typedef unsigned short GID;     /* Glyph id */

#define OFF_SIZE(o) \
	((OffSize)(((o) > 0x00ffffff) ? 4 : (((o) > 0x0000ffff) ? 3 : (((o) > 0x000000ff) ? 2 : 1))))

typedef struct                  /* INDEX */
{
	unsigned short count;       /* Element count */
	OffSize offSize;            /* Offset size */
	long datasize;              /* Data size */
	unsigned short bias;        /* Subr number bias */
} INDEX;

/* INDEX macros */
#define INDEX_HDR_SIZE  (2 + 1)
#define INDEX_OFF_SIZE(size) OFF_SIZE((size) + 1)
#define INDEX_SIZE(items, size) \
	(((items) == 0) ? 2 : \
	 (INDEX_HDR_SIZE + ((items) + 1) * INDEX_OFF_SIZE(size) + (size)))

/* Library-wide utility functions */
void CTL_CDECL cfwFatal(cfwCtx g, int err_code, char *fmt, ...);
void CTL_CDECL cfwMessage(cfwCtx g, char *fmt, ...);

void *cfwMemNew(cfwCtx g, size_t size);
void cfwMemFree(cfwCtx g, void *ptr);

void cfwWrite1(cfwCtx g, unsigned char value);
void cfwWrite2(cfwCtx g, unsigned short value);
void cfwWriteN(cfwCtx g, int N, unsigned long value);
void cfwWrite(cfwCtx g, size_t count, char *buf);

int cfwEncInt(long i, unsigned char *t);
int cfwEncReal(float r, unsigned char *t);

long cfwSeenGlyph(cfwCtx g, abfGlyphInfo *info, int *result,
                  long startNew, long endNew);
void cfwAddGlyph(cfwCtx g, abfGlyphInfo *info, float hAdv, long length,
                 long offset, long seen_index);

/* -------------------------------- Contexts -------------------------------

   The cffwrite library implements a data hiding model based on contexts. A
   context is an instance of data a module or the library uses in order to
   perform its function and is private to the module or library, i.e. not
   visible externally.

   The library context (cfwCtx) is public to the modules that make up the
   libary (but not to the client) and is defined below. The library context
   contains a structure (called ctx) containing an opaque context pointer for
   each module.

   By convention the library context is passed around in a variable called g
   (mnemonic: global) and each module passes its context (<module>Ctx) around
   as h (mnemonic: handle and h follows g in the alphabet).

   The only way to manipulate the data in a module context is by calling a
   function (method) from the module. By convention each external function
   provided by a module takes a g pointer as its first argument and internal
   functions typically take an h pointer as their first argument.

   A g pointer can be converted to an h pointer using the code:

   <module>Ctx h = g->ctx.<module>;

   Each module context contains a field called g which is a pointer to the
   global context. Thus, an h pointer can be converted to a g pointer as
   follows: h->g, which permits cross-module calling.

   Module contexts are created by calling <module>New(g), prepared for reuse by
   calling <module>reuse(g), and are and destroyed by calling <module>Free(g).
   Similarly, a client creates the library context by calling cfwNew() and
   destroys it by calling cfwFree(). */

typedef struct controlCtx_ *controlCtx;
typedef struct charsetCtx_ *charsetCtx;
typedef struct encodingCtx_ *encodingCtx;
typedef struct fdselectCtx_ *fdselectCtx;
typedef struct sindexCtx_ *sindexCtx;
typedef struct dictCtx_ *dictCtx;
typedef struct cstrCtx_ *cstrCtx;
typedef struct subrCtx_ *subrCtx;

/* Library context (the one returned to client) */
struct cfwCtx_ {
	long flags;                 /* Control flags */
	struct                      /* Client callbacks */
	{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
	}
	cb;
	struct                      /* Streams */
	{
		void *dst;
		void *tmp;
		void *dbg;
	}
	stm;
	struct                      /* Temorary stream */
	{
		long offset;            /* Buffer offset */
		size_t length;          /* Buffer length */
		char *buf;              /* Buffer beginning */
		char *end;              /* Buffer end */
		char *next;             /* Next byte available (buf <= next < end) */
	}
	tmp;
	struct                      /* Service library and module contexts */
	{
		dnaCtx dnaSafe;         /* longjmp on error */
		dnaCtx dnaFail;         /* Return on error */
		controlCtx control;
		charsetCtx charset;
		encodingCtx encoding;
		fdselectCtx fdselect;
		sindexCtx sindex;
		dictCtx dict;
		cstrCtx cstr;
		subrCtx subr;
	}
	ctx;
	struct                      /* Error handling */
	{
		short code;
		jmp_buf env;
	}
	err;
};

#endif /* CFFWRITE_SHARE_H */