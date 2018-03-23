/* @(#)CM_VerSion global.h atm08 1.2 16245.eco sum= 47732 atm08.002 */
/* @(#)CM_VerSion global.h atm07 1.2 16164.eco sum= 30445 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef  COMMON_H
#define  COMMON_H

#include "dynarr.h"
#include "typecomp.h"
#include "stdint.h"

/* Types */
typedef int32_t Fixed;			/* 16.16 fixed point */
typedef unsigned char OffSize;  /* Offset size indicator */
typedef uint32_t Offset;   /* 1, 2, 3, or 4-byte offset */
typedef unsigned short SID;     /* String id */
typedef struct {                /* INDEX table header */
	unsigned short count;
	OffSize offSize;
} INDEXHdr;

/* Size of standard types (bytes) */
#define sizeCard8   1
#define sizeCard16  2

/* --- Macros --- */
#ifndef ABS
#define ABS(v) (((v) < 0) ? -(v) : (v))
#endif
#define TABLE_LEN(t) (sizeof(t) / sizeof((t)[0]))
#define COPY(d, s, n) memmove(d, s, sizeof((s)[0]) * (n))
#define OFF_SIZE(o) \
	(((o) > 0x00ffffff) ? 4 : (((o) > 0x0000ffff) ? 3 : (((o) > 0x000000ff) ? 2 : 1)))
#define INDEX_OFF_SIZE(size) OFF_SIZE((size) + 1)
#define INDEX_SIZE(items, size) \
	((items) == 0 ? sizeCard16 : \
	 (sizeCard16 + sizeCard8 + ((items) + 1) * INDEX_OFF_SIZE(size) + (size)))
#define FixedHalf   ((Fixed)0x00008000)
#define INT2FIX(i)  ((Fixed)((uint32_t)i) << 16)
#define DBL2FIX(d)  ((Fixed)((double)(d) * 65536.0 + ((d) < 0 ? -0.5 : 0.5)))
#define FIX2DBL(f)  ((double)(f) / 65536.0)
#define RNDFIX(f)   (((f) + FixedHalf) & 0xffff0000)

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CDECL
#define CDECL
#endif

/* --- Error Handling --- */
void CDECL tcFatal(tcCtx g, char *fmt, ...);
void CDECL tcWarning(tcCtx g, char *fmt, ...);
void CDECL tcNote(tcCtx g, char *fmt, ...);

/* --- Memory management --- */
#define MEM_NEW(g, s)        g->cb.malloc(g->cb.ctx, (s))
#define MEM_RESIZE(g, p, s)   g->cb.realloc(g->cb.ctx, (p), (s))
#define MEM_FREE(g, p)       g->cb.free(g->cb.ctx, (p))

/* --- Output interface --- */
void tcOut2(tcCtx g, short value);
void tcOutOff(tcCtx g, int size, Offset offset);

#define OUT1(v)     g->cb.cffWrite1(g->cb.ctx, (v))
#define OUT2(v)     tcOut2(g, (short)(v))
#define OUTN(c, v)   g->cb.cffWriteN(g->cb.ctx, (c), (v))
#define OUTOFF(c, v) tcOutOff(g, (c), (v))

/* --- Miscellaneous --- */
int tc_strncmp(const char *s1, int length1, const char *s2);
char *tc_dupstr(tcCtx g, char *src);
char *tc_dupstrn(tcCtx g, char *src, int length);

/* -------------------------------- Contexts ------------------------------- */

/* The TC library implements a data hiding model based on contexts. A context
   is an instance of the data a module or the library uses in order to perform
   its function and is private to the module or library, i.e. not visible
   externally.

   The library context is public to the modules that make up the libary (but
   not to the client) and is defined below. The library context contains a
   structure (called ctx) containing an opaque context pointer for each module.

   By convention the library context (type tcCtx) is passed around in a
   variable called g (mnemonic: global) and each module passes its context
   (type <module>Ctx) around as h (mnemonic: handle and h follows g).

   The only way to manipulate the data in a module context is by calling a
   function (method) from the module. By convention each external function
   provided by a module takes a g pointer as its first argument and internal
   functions typically take an h pointer as their first argument.

   A g pointer can be converted to an h pointer using the code:

   <module>Ctx h = g->ctx.<module>;

   Each module context contains a field called g which is a pointer to the
   global context. Thus, an h pointer can be converted to a g pointer as
   follows: h->g.

   Module contexts are created by a call to <module>New(g) and re destroyed by
   a call to <module>Free(g). Similarly, a client creates a library context by
   a call to tcNew() and destroys it by a call to tcFree().

 */

typedef struct charsetCtx_ *charsetCtx;
typedef struct csCtx_ *csCtx;
typedef struct dictCtx_ *dictCtx;
typedef struct encodingCtx_ *encodingCtx;
typedef struct fdselectCtx_ *fdselectCtx;
typedef struct parseCtx_ *parseCtx;
typedef struct recodeCtx_ *recodeCtx;
typedef struct sindexCtx_ *sindexCtx;
typedef struct subrCtx_ *subrCtx;
typedef struct t13Ctx_ *t13Ctx;
typedef struct tcprivCtx_ *tcprivCtx;

/* Subroutinizer parse data */
typedef struct {
	int (*oplen)(unsigned char *);
	unsigned char *(*cstrcpy)(unsigned char *, unsigned char *, unsigned);
	int (*encInteger)(int32_t, char *);
	short maxCallStack;
	short hintmask;
	short cntrmask;
	short callsubr;
	short callgsubr;
	short return_;
	short endchar;
	short separator;
} SubrParseData;

/* Package context */
struct tcCtx_ {
	tcCallbacks cb;     /* Client callbacks */
	int32_t flags;         /* Compression specification flags */
	int32_t status;        /* Program status flags */
#define TC_MESSAGE      (1 << 0)  /* Flags message for font already posted */
#define TC_EURO_ADDED   (1 << 1)  /* Flags Euro glyph added to font */
	short nMasters;     /* Number of masters */
	uint32_t maxNumSubrs;
	SubrParseData *spd; /* Subroutinizer parse data */
	struct {            /* --- Module contexts */
		dnaCtx dnaCtx;
		charsetCtx charset;
		csCtx cs;
		encodingCtx encoding;
		fdselectCtx fdselect;
		parseCtx parse;
		recodeCtx recode;
		sindexCtx sindex;
		subrCtx subr;
		t13Ctx t13;
		tcprivCtx tcpriv;
	}
	ctx;
#if TC_STATISTICS
	struct {            /* --- Source font statistics (totals) */
		short gather;
		long nSubrs;
		long subrSize;
		long nChars;
		long charSize;
		long flatSize;
		long fontSize;
	}
	stats;
#endif /* TC_STATISTICS */
};

#endif /* COMMON_H */
