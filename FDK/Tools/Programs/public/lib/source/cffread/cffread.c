/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Compact Font Format (CFF) parser.
 */

#include "cffread.h"
#include "dynarr.h"
#include "t2cstr.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"
#include "sfntread.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

/* Predefined encodings */
static const unsigned char stdenc[] =
{
#include "stdenc0.h"
};
static const unsigned char exenc[] =
{
#include "exenc0.h"
};

#define ARRAY_LEN(t) 	(sizeof(t)/sizeof((t)[0]))

typedef unsigned char OffSize;  /* Offset size indicator */
typedef long Offset;			/* 1, 2, 3, or 4-byte offset */
typedef unsigned short SID;		/* String identifier */
#define SID_SIZE	2			/* SID byte size */

typedef struct          		/* INDEX */
{
	unsigned short count;		/* Element count */
	OffSize offSize;			/* Offset size */
	Offset offset;				/* Offset start */
	Offset data;				/* Data start - 1 */
	unsigned bias;				/* Subr number bias */
} INDEX;

typedef dnaDCL(long, SubrOffsets);/* Subr charstring offsets */

typedef struct					/* FDArray element */
{
	unsigned long flags;
#define SEEN_BLUE_VALUES (1<<0)	/* Flags BlueValues operator seen */
	cfrRepeatRegions region;
	SubrOffsets Subrs;			/* Local subr offsets */
	t2cAuxData aux;				/* Auxiliary charstring parse data */
	abfFontDict *fdict;			/* Abstract font data */
} FDInfo;

typedef struct					/* Operand Stack element */
{
    int is_int;
    union
    {
        long int_val;		/* allow at least 32-bit int */
        float real_val;
    } u;
} stack_elem;

struct cfrCtx_					/* Context */
{
	long flags;					/* Status flags */
#define CID_FONT		(1UL<<31)	/* CID Font */
#define FREE_ENCODINGS	(1UL<<30)	/* Return encoding nodes to free list */
	cfrSingleRegions region;
	struct						/* CFF header */
    {
		unsigned char major;
		unsigned char minor;
		unsigned char hdrSize;
		OffSize offSize;
    } header;
	struct						/* INDEXes */
    {
		INDEX name;
		INDEX top;
		INDEX string;
		INDEX FDArray;
    } index;
	SubrOffsets gsubrs;			/* Global subr offsets */
	abfTopDict top;				/* Top dict */
	dnaDCL(FDInfo, FDArray);	/* FDArray */
	dnaDCL(abfFontDict, fdicts);/* Font Dicts */
	FDInfo *fd;					/* Active Font Dict */
	struct						/* DICT operand stack */
    {
		int cnt;
		stack_elem array[T2_MAX_OP_STACK];
    } stack;
	struct						/* String data */
    {
		dnaDCL(Offset, offsets);
		dnaDCL(char *, ptrs);
		dnaDCL(char, buf);
    } string;
	dnaDCL(abfGlyphInfo, glyphs);/* Glyph data */
	dnaDCL(unsigned short, glyphsByName);/* Glyphs sorted by name */
	dnaDCL(unsigned short, glyphsByCID); /* Glyphs sorted by cid */
	struct						/* Streams */
    {
		void *src;
		void *dbg;
    } stm;
	struct						/* Source stream */
    {
		Offset origin;			/* Origin offset of font */
		Offset offset;			/* Buffer offset */
		size_t length;			/* Buffer length */
		char *buf;				/* Buffer beginning */
		char *end;				/* Buffer end */
		char *next;				/* Next byte available (buf <= next < end) */
    } src;
	unsigned short stdEnc2GID[256];/* Map standard encoding to GID */
	abfEncoding *encfree;		/* Supplementary encoding free list */
	struct						/* Client callbacks */
    {
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
    } cb;
	struct						/* Contexts */
    {
		dnaCtx dna;				/* dynarr */
		sfrCtx sfr;				/* sfntread */
    } ctx;
	struct						/* Error handling */
    {
		jmp_buf env;
		int code;
    } err;
};

static void encListFree(cfrCtx h, abfEncoding *node);

/* ----------------------------- Error Handling ---------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(cfrCtx h, char *fmt, va_list ap)
{
	char text[500];
    
	if (h->stm.dbg == NULL)
		return;	/* Debug stream not available */
    
	vsprintf(text, fmt, ap);
	(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(cfrCtx h, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vmessage(h, fmt, ap);
	va_end(ap);
}

/* Handle fatal error. */
static void fatal(cfrCtx h, int err_code)
{
	message(h, "%s", cfrErrStr(err_code));
	h->err.code = err_code;
	longjmp(h->err.env, 1);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(cfrCtx h, size_t size)
{
	void *ptr = h->cb.mem.manage(&h->cb.mem, NULL, size);
	if (ptr == NULL)
		fatal(h, cfrErrNoMemory);
	return ptr;
}

/* Free memory. */
static void memFree(cfrCtx h, void *ptr)
{
	h->cb.mem.manage(&h->cb.mem, ptr, 0);
}

/* -------------------------- Safe dynarr Context -------------------------- */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
{
	cfrCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, cfrErrNoMemory);
	return ptr;
}

/* Initialize error handling dynarr context. */
static void dna_init(cfrCtx h)
{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->ctx.dna	= dnaNew(&cb, DNA_CHECK_ARGS);
}

/* --------------------------- Context Management -------------------------- */

/* Initialize FDArray element. */
static void initFDArray(void *ctx, long cnt, FDInfo *info)
{
	cfrCtx h = ctx;
	while (cnt--)
    {
		dnaINIT(h->ctx.dna, info->Subrs, 200, 100);
		info++;
    }
}

/* Validate client and create context. */
cfrCtx cfrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
{
	cfrCtx h;
    
	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(CFR_VERSION))
		return NULL;
    
	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct cfrCtx_));
	if (h == NULL)
		return NULL;
    
	/* Safety initialization */
	h->gsubrs.size = 0;
	h->FDArray.size = 0;
	h->fdicts.size = 0;
	h->string.offsets.size = 0;
	h->string.ptrs.size = 0;
	h->string.buf.size = 0;
	h->glyphs.size = 0;
	h->glyphsByName.size = 0;
	h->glyphsByCID.size = 0;
	h->stm.src = NULL;
	h->stm.dbg = NULL;
	h->encfree = NULL;
	h->ctx.dna = NULL;
    
	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;
    
	/* Set error handler */
	if (setjmp(h->err.env))
		goto cleanup;
    
	/* Initialize service libraries */
	dna_init(h);
	h->ctx.sfr = sfrNew(mem_cb, stm_cb, SFR_CHECK_ARGS);
	if (h->ctx.sfr == NULL)
		fatal(h, cfrErrSfntread);
    
	/* Initialize */
	dnaINIT(h->ctx.dna, h->gsubrs, 200, 100);
	dnaINIT(h->ctx.dna, h->FDArray, 1, 13);
	h->FDArray.func = initFDArray;
	dnaINIT(h->ctx.dna, h->fdicts, 1, 13);
	dnaINIT(h->ctx.dna, h->glyphs, 256, 768);
	dnaINIT(h->ctx.dna, h->glyphsByName, 256, 768);
	dnaINIT(h->ctx.dna, h->glyphsByCID, 256, 768);
	dnaINIT(h->ctx.dna, h->string.offsets, 16, 256);
	dnaINIT(h->ctx.dna, h->string.ptrs, 16, 256);
	dnaINIT(h->ctx.dna, h->string.buf, 200, 2000);
	
	/* Open optional debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, CFR_DBG_STREAM_ID, 0);
    
	return h;
    
cleanup:
	/* Initialization failed */
	cfrFree(h);
	return NULL;
}

/* Free library context. */
void cfrFree(cfrCtx h)
{
	int i;
    
	if (h == NULL)
		return;
    
	/* Free dynamic arrays */
	for (i = 0; i < h->FDArray.size; i++)
		dnaFREE(h->FDArray.array[i].Subrs);
    
	dnaFREE(h->gsubrs);
	dnaFREE(h->FDArray);
	dnaFREE(h->fdicts);
	dnaFREE(h->glyphs);
	dnaFREE(h->glyphsByName);
	dnaFREE(h->glyphsByCID);
	dnaFREE(h->string.offsets);
	dnaFREE(h->string.ptrs);
	dnaFREE(h->string.buf);
    
	encListFree(h, h->encfree);
    
	dnaFree(h->ctx.dna);
	sfrFree(h->ctx.sfr);
    
	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);
    
	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);
}

/* ----------------------------- Source Stream ----------------------------- */

/* Fill source buffer. */
static void fillbuf(cfrCtx h, long offset)
{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
	if (h->src.length == 0)
		fatal(h, cfrErrSrcStream);
	h->src.offset = offset;
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
}

/* Perform intial seek and buffer fill. */
static void seekbuf(cfrCtx h, long offset)
{
	if (h->cb.stm.seek(&h->cb.stm, h->stm.src, offset))
		fatal(h, cfrErrSrcStream);
	fillbuf(h, offset);
}

/* Read next sequential source buffer; update offset and return first byte. */
static char nextbuf(cfrCtx h)
{
	/* 64-bit warning fixed by cast here */
	fillbuf(h, (long)(h->src.offset + h->src.length));
	return *h->src.next++;
}

/* Seek to specified offset. */
static void srcSeek(cfrCtx h, long offset)
{
	long delta = offset - h->src.offset;
	if (delta >= 0 && (size_t)delta < h->src.length)
    /* Offset within current buffer; reposition next byte */
		h->src.next = h->src.buf + delta;
	else
    /* Offset outside current buffer; seek to offset and fill buffer */
		seekbuf(h, offset);
}

/* Return absolute byte position in stream. */
#define srcTell(h)	(h->src.offset + (h->src.next - h->src.buf))

/* Copy count bytes from source stream. */
static void srcRead(cfrCtx h, size_t count, char *ptr)
{
	size_t left = h->src.end - h->src.next;
    
	while (left < count)
    {
		/* Copy buffer */
		memcpy(ptr, h->src.next, left);
		ptr += left;
		count -= left;
        
		/* Refill buffer */
		/* 64-bit warning fixed by cast here */
		fillbuf(h, (long)(h->src.offset + h->src.length));
		left = h->src.length;
    }
    
	memcpy(ptr, h->src.next, count);
	h->src.next += count;
}

/* Read 1-byte unsigned number. */
#define read1(h) \
((unsigned char)((h->src.next==h->src.end)?nextbuf(h):*h->src.next++))

/* Read 2-byte number. */
static unsigned short read2(cfrCtx h)
{
	unsigned short value = (unsigned short)read1(h)<<8;
	return value | (unsigned short)read1(h);
}

/* Read 1-, 2-, 3-, or 4-byte number. */
static unsigned long readN(cfrCtx h, int N)
{
	unsigned long value = 0;
	switch (N)
    {
        case 4:
            value = read1(h);
        case 3:
            value = value<<8 | read1(h);
        case 2:
            value = value<<8 | read1(h);
        case 1:
            value = value<<8 | read1(h);
    }
	return value;
}

/* Open source stream. */
static void srcOpen(cfrCtx h, long origin, int ttcIndex)
{
    int i;
    int result;
    ctlTag sfnt_tag;
    long offset = origin;
    
	h->stm.src = h->cb.stm.open(&h->cb.stm, CFR_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL)
		fatal(h, cfrErrSrcStream);
    /* Parse stream as sfnt */
readhdr:
    /* Read sfnt header */
    result = sfrBegFont(h->ctx.sfr, h->stm.src, offset, &sfnt_tag);
    /* If it looks like an SFNT, use the logic for OTF and TTC.*/
    if (result == sfrSuccess)
    {
        
        switch (sfnt_tag)
		{
            case sfr_ttcf_tag:
                /* Get i'th TrueType Collection offset */
                for (i = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr)); i++)
                    if (i == ttcIndex)
                    {
                        ttcIndex = 0;
                        goto readhdr;
                    }
                fatal(h, cfrErrSfntread);
            case sfr_OTTO_tag:
                if (ttcIndex != 0)
                    fatal(h, cfrErrSfntread);
                break;
            case sfr_true_tag:
                if (ttcIndex != 0)
                    fatal(h, cfrErrSfntread);
                break;
            default:
                fatal(h, cfrErrSfntread);
		}
        
        switch (result)
		{
            case sfrSuccess:
                if (sfnt_tag == sfr_OTTO_tag)
                {
                    /* OTF; use CFF table offset */
                    sfrTable *table =
                    sfrGetTableByTag(h->ctx.sfr, CTL_TAG('C','F','F',' '));
                    if (table == NULL)
                        fatal(h, cfrErrNoCFF);
                    origin = table->offset;
                    
                    /* Try to read OS/2.fsType */
                    table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('O','S','/','2'));
                    if (table != NULL)
                    {
                        seekbuf(h, table->offset + 4*2);
                        h->top.FSType = read2(h);
                    }
                    
                    /* See if its a SING glyphlet */
                    table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('n','a','m','e'));
                    if (table == NULL)
                    {
                        table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('S','I','N','G'));
                        if (table != NULL)
                        {
                            /* Read "permissions" field as FSType */
                            seekbuf(h, table->offset + 3*2);
                            h->top.FSType = read2(h);
                            h->top.sup.flags |= ABF_SING_FONT;
                        }
                    }
                }
                else
                    fatal(h, cfrErrBadFont);
                break;
            case sfrErrBadSfnt:
                break;	/* Not a recognized sfnt; assume naked CFF */
            default:
                fatal(h, cfrErrSfntread);
		}
    }
	/* Read first buffer */
	seekbuf(h, origin);
	h->src.origin = origin;
}

/* ------------------------------ DICT Parsing ----------------------------- */

/* Check stack contains at least n elements. */
#define CHKUFLOW(n) \
do if(h->stack.cnt<(n))fatal(h,cfrErrStackUnderflow);while(0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n) \
do if(h->stack.cnt+(n)>T2_MAX_OP_STACK)fatal(h,cfrErrStackOverflow);while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define INDEX_INT(i) (INDEX(i).is_int? INDEX(i).u.int_val: (long)INDEX(i).u.real_val)
#define PUSH_INT(v) { stack_elem*	elem = &h->stack.array[h->stack.cnt++];	\
elem->is_int = 1; elem->u.int_val = (v); }
#define INDEX_REAL(i) (INDEX(i).is_int? (float)INDEX(i).u.int_val: INDEX(i).u.real_val)
#define PUSH_REAL(v) { stack_elem*	elem = &h->stack.array[h->stack.cnt++];	\
elem->is_int = 0; elem->u.real_val = (v); }

/* Save font transformation matrix. */
static void saveMatrix(cfrCtx h, int topdict)
{
	float	array[6];
    
	CHKUFLOW(6);
    
	array[0] = INDEX_REAL(0);
	array[1] = INDEX_REAL(1);
	array[2] = INDEX_REAL(2);
	array[3] = INDEX_REAL(3);
	array[4] = INDEX_REAL(4);
	array[5] = INDEX_REAL(5);
    
	if (topdict && h->flags & CID_FONT)
    {
		/* Top-level CIDFont matrix */
		if (array[0] != 1.0 ||
			array[1] != 0.0 ||
			array[2] != 0.0 ||
			array[3] != 1.0 ||
			array[4] != 0.0 ||
			array[5] != 0.0)
        {
			/* Save matrix if not identity */
			memcpy(h->top.cid.FontMatrix.array, array, 6*sizeof(float));
			h->top.cid.FontMatrix.cnt = 6;
        }
    }
	else
    {
		/* Font dict matrix */
		float *result = h->fd->fdict->FontMatrix.array;
        
		if (h->top.cid.FontMatrix.cnt != ABF_EMPTY_ARRAY)
        {
			/* Form product of top and FDArray matrices */
			float *a = h->top.cid.FontMatrix.array;
			float *b = array;
            
			result[0] = a[0]*b[0] + a[1]*b[2];
			result[1] = a[0]*b[1] + a[1]*b[3];
			result[2] = a[2]*b[0] + a[3]*b[2];
			result[3] = a[2]*b[1] + a[3]*b[3];
			result[4] = a[4]*b[0] + a[5]*b[2] + b[4];
			result[5] = a[4]*b[1] + a[5]*b[3] + b[5];
        }
		else
        /* Copy matrix */
			memcpy(result, array, 6*sizeof(float));
        
		if (result[0] != 0.001f ||
			result[1] != 0.0 ||
			result[2] != 0.0 ||
			result[3] != 0.001f ||
			result[4] != 0.0 ||
			result[5] != 0.0)
        {
			/* Non-default matrix */
			float max;
			int i;
            
			/* Make matrix available */
			h->fd->fdict->FontMatrix.cnt = 6;
            
			/* Find the largest of a, b, c, and d */
			max = 0.0;
			for (i = 0; i < 4; i++)
            {
				float value = result[i];
				if (value < 0.0)
					value = -value;
				if (value > max)
					max = value;
            }
			if (max == 0.0)
				fatal(h, cfrErrFontMatrix);
            
			/* Calculate units-per-em */
			h->top.sup.UnitsPerEm = (unsigned short)(1.0/max + 0.5);
            
			if (h->flags & CFR_USE_MATRIX)
            {
				/* Prepare charstring transformation matrix */
				for (i = 0; i < 6; i++)
					h->fd->aux.matrix[i] = result[i]*h->top.sup.UnitsPerEm;
				h->fd->aux.flags |= T2C_USE_MATRIX;
            }
        }
    }
}

/* Convert BCD number to double. */
static double convBCD(cfrCtx h)
{
	double result;
	char *p;
	char buf[64];
	int byte = 0;	/* Suppress optimizer warning */
	int i = 0;
	int count = 0;
    
	/* Construct string representation of number */
	for (;;)
    {
		int nibble;
        
		if (count++ & 1)
			nibble = byte & 0xf;
		else
        {
			byte = read1(h);
			nibble = byte>>4;
        }
        
		if (nibble == 0xf)
			break;
		else if (nibble == 0xd || i >= (int)sizeof(buf) - 2)
			fatal(h, cfrErrBCDArg);
		else
        {
			buf[i++] = "0123456789.EE?-?"[nibble];
			if (nibble == 0xc)
				buf[i++] = '-';	/* Negative exponent */
        }
    }
	buf[i] = '\0';
    
	result = ctuStrtod(buf, &p);
	if (*p != '\0')
		fatal(h, cfrErrBCDArg);
    
	return result;
}

/* Get string from its sid. */
static char *sid2str(cfrCtx h, long sid)
{
	static char *stdstrs[] =
    {
#include "stdstr1.h"
    };
    
	if (sid < 0)
		;
	else if (sid < (int)ARRAY_LEN(stdstrs))
		return stdstrs[sid];	/* Standard string */
	else
    {
		sid -= ARRAY_LEN(stdstrs);
		if (sid < h->string.ptrs.cnt)
			return h->string.ptrs.array[sid];	/* Custom string */
    }
    
	fatal(h, cfrErrSID);
	return NULL;	/* Suppress compiler warning */
}

/* Save real delta array operands. */
static void saveDeltaArray(cfrCtx h, size_t max, long *cnt, float *array)
{
	int i;
	if (h->stack.cnt == 0 || h->stack.cnt > (long)max)
		fatal(h, cfrErrDICTArray);
	array[0] = INDEX_REAL(0);
	for (i = 1; i < h->stack.cnt; i++)
		array[i] = array[i - 1] + INDEX_REAL(i);
	*cnt = h->stack.cnt;
}

/* Save integer array operands. */
static void saveIntArray(cfrCtx h, size_t max, long *cnt, long *array,
						 int delta)
{
	int i;
	if (h->stack.cnt == 0 || h->stack.cnt > (long)max)
		fatal(h, cfrErrDICTArray);
	array[0] = INDEX_INT(0);
	for (i = 1; i < h->stack.cnt; i++)
		array[i] = (delta? array[i - 1] + INDEX_INT(i): INDEX_INT(i));
	*cnt = h->stack.cnt;
}

/* Save PostScript operator string. */
static char *savePostScript(cfrCtx h, char *str)
{
	long value;
	int n;
	char *p;
	char *q;
    
	p = strstr(str, "/FSType");
	if (p != NULL)
    {
		/* Extract FSType from string */
		n = -1;
		q = p + sizeof("/FSType") - 1;
		if (sscanf(q, " %ld def%n", &value, &n) == 1 && n != -1 &&
			0 <= value && value < 65536)
        {
			/* Sucessfully parsed value; remove definition from string */
			memmove(p, q + n, strlen(p) + 1 - (q - p + n));
            
			if (h->top.FSType != ABF_UNSET_INT)
				message(h, "two FSTypes (OS/2 value retained, "
						"CFF value removed)");
			else
				h->top.FSType = value;
        }
		else
			fatal(h, cfrErrFSType);
    }
    
	p = strstr(str, "/OrigFontType");
	if (p != NULL)
    {
		/* Extract OrigFontType from string */
		n = -1;
		q = p + sizeof("/OrigFontType") - 1;
		if (sscanf(q, " /Type1 def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeType1;
		else if (sscanf(q, " /CID def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeCID;
		else if (sscanf(q, " /TrueType def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeTrueType;
		else if (sscanf(q, " /OCF def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeOCF;
		else
			fatal(h, cfrErrOrigFontType);
        
		/* Sucessfully parsed value; remove definition from string */
		memmove(p, q + n, strlen(p) + 1 - (q - p + n));
    }
    
	/* Check for empty string */
	for (p = str; isspace(*p); p++)
		;
	if (*p == '\0')
		return NULL;
    
	return str;
}

/* Read DICT data */
static void readDICT(cfrCtx h, ctlRegion *region, int topdict)
{
	abfTopDict *top = &h->top;
	abfFontDict *font = h->fd->fdict;
	abfPrivateDict *private = &font->Private;
    
	if (region->begin == region->end)
    {
		/* Empty dictionary (assuming Private); valid but set BlueValues to
         avoid warning */
		h->fd->flags |= SEEN_BLUE_VALUES;
		return;
    }
    
	srcSeek(h, region->begin);
    
	h->stack.cnt = 0;
	while (srcTell(h) < region->end)
    {
		int byte0 = read1(h);
		switch (byte0)
        {
            case cff_version:
                CHKUFLOW(1);
                top->version.ptr = sid2str(h, (SID)INDEX_INT(0));
                break;
            case cff_Notice:
                CHKUFLOW(1);
                top->Notice.ptr = sid2str(h, (SID)INDEX_INT(0));
                break;
            case cff_FullName:
                CHKUFLOW(1);
                top->FullName.ptr = sid2str(h, (SID)INDEX_INT(0));
                break;
            case cff_FamilyName:
                CHKUFLOW(1);
                top->FamilyName.ptr	= sid2str(h, (SID)INDEX_INT(0));
                break;
            case cff_Weight:
                CHKUFLOW(1);
                top->Weight.ptr = sid2str(h, (SID)INDEX_INT(0));
                break;
            case cff_FontBBox:
                CHKUFLOW(4);
                top->FontBBox[0] = INDEX_REAL(0);
                top->FontBBox[1] = INDEX_REAL(1);
                top->FontBBox[2] = INDEX_REAL(2);
                top->FontBBox[3] = INDEX_REAL(3);
                break;
            case cff_BlueValues:
                saveDeltaArray(h, ARRAY_LEN(private->BlueValues.array),
                               &private->BlueValues.cnt,
                               private->BlueValues.array);
                h->fd->flags |= SEEN_BLUE_VALUES;
                break;
            case cff_OtherBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(private->OtherBlues.array),
                                   &private->OtherBlues.cnt,
                                   private->OtherBlues.array);
                else
                    message(h, "OtherBlues empty array (ignored)");
                break;
            case cff_FamilyBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(private->FamilyBlues.array),
                                   &private->FamilyBlues.cnt,
                                   private->FamilyBlues.array);
                else
                    message(h, "FamilyBlues empty array (ignored)");
                break;
            case cff_FamilyOtherBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(private->FamilyOtherBlues.array),
                                   &private->FamilyOtherBlues.cnt,
                                   private->FamilyOtherBlues.array);
                else
                    message(h, "FamilyBlues empty array (ignored)");
                break;
            case cff_StdHW:
                CHKUFLOW(1);
                private->StdHW = INDEX_REAL(0);
                break;
            case cff_StdVW:
                CHKUFLOW(1);
                private->StdVW = INDEX_REAL(0);
                break;
            case cff_escape:
                /* Process escaped operator */
                byte0 = read1(h);
                switch (cff_ESC(byte0))
            {
                case cff_Copyright:
                    CHKUFLOW(1);
                    top->Copyright.ptr = sid2str(h, (SID)INDEX_INT(0));
                    break;
                case cff_isFixedPitch:
                    CHKUFLOW(1);
                    top->isFixedPitch = INDEX_INT(0);
                    break;
                case cff_ItalicAngle:
                    CHKUFLOW(1);
                    top->ItalicAngle = INDEX_REAL(0);
                    break;
                case cff_UnderlinePosition:
                    CHKUFLOW(1);
                    top->UnderlinePosition = INDEX_REAL(0);
                    break;
                case cff_UnderlineThickness:
                    CHKUFLOW(1);
                    top->UnderlineThickness = INDEX_REAL(0);
                    break;
                case cff_PaintType:
                    CHKUFLOW(1);
                    font->PaintType = INDEX_INT(0);
                    break;
                case cff_CharstringType:
                    CHKUFLOW(1);
                    switch (INDEX_INT(0))
                {
                    case 2:
                        break;
                    default:
                        fatal(h, cfrErrCstrType);
                }
                    break;
                case cff_FontMatrix:
                    saveMatrix(h, topdict);
                    break;
                case cff_StrokeWidth:
                    CHKUFLOW(1);
                    top->StrokeWidth = INDEX_REAL(0);
                    break;
                case cff_BlueScale:
                    CHKUFLOW(1);
                    private->BlueScale = INDEX_REAL(0);
                    break;
                case cff_BlueShift:
                    CHKUFLOW(1);
                    private->BlueShift = INDEX_REAL(0);
                    break;
                case cff_BlueFuzz:
                    CHKUFLOW(1);
                    private->BlueFuzz = INDEX_REAL(0);
                    break;
                case cff_StemSnapH:
                    saveDeltaArray(h, ARRAY_LEN(private->StemSnapH.array),
                                   &private->StemSnapH.cnt,
                                   private->StemSnapH.array);
                    break;
                case cff_StemSnapV:
                    saveDeltaArray(h, ARRAY_LEN(private->StemSnapV.array),
                                   &private->StemSnapV.cnt,
                                   private->StemSnapV.array);
                    break;
                case cff_ForceBold:
                    CHKUFLOW(1);
                    private->ForceBold = INDEX_INT(0);
                    break;
                case cff_LanguageGroup:
                    CHKUFLOW(1);
                    private->LanguageGroup = INDEX_INT(0);
                    break;
                case cff_ExpansionFactor:
                    CHKUFLOW(1);
                    private->ExpansionFactor = INDEX_REAL(0);
                    break;
                case cff_initialRandomSeed:
                    CHKUFLOW(1);
                    private->initialRandomSeed = INDEX_REAL(0);
                    break;
                case cff_PostScript:
                    CHKUFLOW(1);
                    top->PostScript.ptr =
					savePostScript(h, sid2str(h, (SID)INDEX_INT(0)));
                    break;
                case cff_BaseFontName:
                    CHKUFLOW(1);
                    top->BaseFontName.ptr = sid2str(h, (SID)INDEX_INT(0));
                    break;
                case cff_BaseFontBlend:
                    saveIntArray(h, ARRAY_LEN(top->BaseFontBlend.array),
                                 &top->BaseFontBlend.cnt,
                                 top->BaseFontBlend.array, 1);
                    break;
                case cff_ROS:
                    CHKUFLOW(3);
                    top->cid.Registry.ptr = sid2str(h, (SID)INDEX_INT(0));
                    top->cid.Ordering.ptr = sid2str(h, (SID)INDEX_INT(1));
                    top->cid.Supplement = INDEX_INT(2);
                    h->flags |= CID_FONT;
                    break;
                case cff_CIDFontVersion:
                    CHKUFLOW(1);
                    top->cid.CIDFontVersion = INDEX_REAL(0);
                    break;
                case cff_CIDFontRevision:
                    CHKUFLOW(1);
                    top->cid.CIDFontRevision = INDEX_INT(0);
                    break;
                case cff_CIDFontType:
                    /* Validate and discard */
                    CHKUFLOW(1);
                    if (INDEX_INT(0) != 0)
                        fatal(h, cfrErrCIDType);
                    break;
                case cff_CIDCount:
                    CHKUFLOW(1);
                    top->cid.CIDCount = INDEX_INT(0);
                    break;
                case cff_UIDBase:
                    CHKUFLOW(1);
                    top->cid.UIDBase = INDEX_INT(0);
                    break;
                case cff_FDArray:
                    CHKUFLOW(1);
                    h->region.FDArrayINDEX.begin = h->src.origin + INDEX_INT(0);
                    break;
                case cff_FDSelect:
                    CHKUFLOW(1);
                    h->region.FDSelect.begin = h->src.origin + INDEX_INT(0);
                    break;
                case cff_FontName:
                    CHKUFLOW(1);
                    font->FontName.ptr = sid2str(h, (SID)INDEX_INT(0));
                    break;
                case cff_reservedESC15:
                    /* Was cff_ForceBoldThreshold but was erroneously retained in
                     snapshot MMs by cffsub. Report, but otherwise ignore. */
                    message(h, "defunct /ForceBoldThreshold operator (ignored)");
                    break;
                case cff_SyntheticBase:
                case cff_Chameleon:
                    fatal(h, cfrErrChameleon);
                case cff_reservedESC16:
                    /* Was cff_lenIV but later removed from specification. Some
                     Dainippon Screen fonts are using this operator with a value
                     of -1 which is a no-op so can be safely ignored. */
                    if (h->stack.cnt != 1 || INDEX_INT(0) != -1)
                        fatal(h, cfrErrDICTOp);
                    message(h, "defunct /lenIV operator (ignored)");
                    break;
                case cff_numMasters:
                    /* Was cff_MultipleMaster so this must be one of the old test
                     MMs that are no longer supported. There are too many things
                     that will break if an attempt is made to continue so just
                     print a helpful message and quit. */
                    message(h, "defunct /MultipleMaster operator");
                    break;
                case cff_reservedESC40:
                    /* Was an experimental cff_VToHOrigin but the experiment was
                     never conducted. However, it was implemented in cff_parse.c
                     and Apple apparently saw it in that code and decided to use
                     in their implementation of "Save As PDF" when subsetting
                     fonts. Since it has never done anything it can be safely
                     ignored. */
                    message(h, "defunct /VToHOrigin operator (ignored)");
                    break;
                case cff_reservedESC41:
                    /* Was an experimental cff_defaultWidthY but the experiment was
                     never conducted. However, it was implemented in cff_parse.c
                     and Apple apparently saw it in that code and decided to use
                     in their implementation of "Save As PDF" when subsetting
                     fonts. Since it has never done anything it can be safely
                     ignored. */
                    message(h, "defunct /defaultWidthY operator (ignored)");
                    break;
                case cff_reservedESC25:
                case cff_reservedESC26:
                case cff_reservedESC27:
                case cff_reservedESC28:
                case cff_reservedESC29:
                default:
                    if (h->stm.dbg == NULL)
                        fatal(h, cfrErrDICTOp);
                    else
                        message(h, "invalid DICT op ESC%hu (ignored)", byte0);
            }	/* End: switch (escop) */
                break;
            case cff_UniqueID:
                CHKUFLOW(1);
                top->UniqueID = INDEX_INT(0);
                break;
            case cff_XUID:
                saveIntArray(h, ARRAY_LEN(top->XUID.array),
                             &top->XUID.cnt,
                             top->XUID.array, 0);
                break;
            case cff_charset:
                CHKUFLOW(1);
                h->region.Charset.begin = INDEX_INT(0);
                if (h->region.Charset.begin > cff_ExpertSubsetCharset)
				/* Custom charset; adjust offset */
                    h->region.Charset.begin += h->src.origin;
                break;
            case cff_Encoding:
                CHKUFLOW(1);
                h->region.Encoding.begin = INDEX_INT(0);
                if (h->region.Encoding.begin > cff_ExpertEncoding)
				/* Custom encoding; adjust offset */
                    h->region.Encoding.begin += h->src.origin;
                break;
            case cff_CharStrings:
                CHKUFLOW(1);
                h->region.CharStringsINDEX.begin = h->src.origin + INDEX_INT(0);
                break;
            case cff_Private:
                CHKUFLOW(2);
                h->fd->region.PrivateDICT.begin = h->src.origin + INDEX_INT(1);
                h->fd->region.PrivateDICT.end =
				h->fd->region.PrivateDICT.begin + INDEX_INT(0);
                break;
            case cff_Subrs:
                CHKUFLOW(1);
                h->fd->region.LocalSubrINDEX.begin =
				region->begin + INDEX_INT(0);
                break;
            case cff_defaultWidthX:
                CHKUFLOW(1);
                h->fd->aux.defaultWidthX = INDEX_REAL(0);
                break;
            case cff_nominalWidthX:
                CHKUFLOW(1);
                h->fd->aux.nominalWidthX = INDEX_REAL(0);
                break;
            case cff_shortint:
                /* 3 byte number */
                CHKOFLOW(1);
			{
                short value = read1(h);
                value = value<<8 | read1(h);
#if SHRT_MAX > 32767
                /* short greater that 2 bytes; handle negative range */
                if (value > 32767)
                    value -= 65536;
#endif
                PUSH_INT(value);
			}
                continue;
            case cff_longint:
                /* 5 byte number */
                CHKOFLOW(1);
			{
                long value = read1(h);
                value = value<<8 | read1(h);
                value = value<<8 | read1(h);
                value = value<<8 | read1(h);
#if LONG_MAX > 2147483647
                /* long greater that 4 bytes; handle negative range */
                if (value > 2417483647)
                    value -= 4294967296;
#endif
                PUSH_INT(value);
			}
                continue;
            case cff_BCD:
                CHKOFLOW(1);
                PUSH_REAL((float)convBCD(h));
                continue;
            case cff_Blend:
                h->flags |= CFR_SEEN_BLEND;
                PUSH_INT(byte0);
                continue;
            default:
                /* 1 byte number */
                CHKOFLOW(1);
                PUSH_INT(byte0 - 139);
                continue;
            case 247: case 248: case 249: case 250:
                /* Positive 2 byte number */
                CHKOFLOW(1);
                PUSH_INT(108 + 256*(byte0 - 247) + read1(h));
                continue;
            case 251: case 252: case 253: case 254:
                /* Negative 2 byte number */
                CHKOFLOW(1);
                PUSH_INT(-108 - 256*(byte0 - 251) - read1(h));
                continue;
            case cff_reserved22:
            case cff_reserved23:
            case cff_reserved24:
            case cff_reserved25:
            case cff_reserved26:
            case cff_reserved27:
            case cff_reserved255:
                if (h->stm.dbg == NULL)
                    fatal(h, cfrErrDICTOp);
                else
                    message(h, "Invalid DICT op %hu (ignored)", byte0);
        }				/* End: switch (byte0) */
		h->stack.cnt = 0;	/* Clear stack */
    }					/* End: while (...) */
}

/* -------------------------- Component Processing ------------------------- */

/* Read subroutine index. */
static void readSubrINDEX(cfrCtx h, ctlRegion *region, SubrOffsets *offsets)
{
	long i;
	long cnt;
	Offset dataoff;
	OffSize offSize;
    
	/* Read INDEX count */
	srcSeek(h, region->begin);
	cnt = read2(h);
    
	if (cnt == 0)
    {
		/* Empty INDEX */
		region->end = region->begin + 2;
		return;
    }
    
	offSize = read1(h);		/* Get offset size */
    
	/* Compute data offset */
	dataoff = region->begin + 2 + 1 + (cnt + 1)*offSize - 1;
    
	/* Allocate offset array */
	dnaSET_CNT(*offsets, cnt);
    
	/* Read charstring offsets */
	for (i = 0; i < cnt; i++)
		offsets->array[i] = dataoff + readN(h, offSize);
    
	/* Save INDEX length */
	region->end = dataoff + readN(h, offSize);
}

/* Read INDEX structure parameters and return offset of byte following INDEX.*/
static void readINDEX(cfrCtx h, ctlRegion *region, INDEX *index)
{
	/* Read INDEX count */
	srcSeek(h, region->begin);
	index->count = read2(h);
    
	if (index->count == 0)
    {
		/* Empty INDEX */
		region->end = region->begin + 2;
		return;
    }
    
	/* Read and validate offset size */
	index->offSize = read1(h);
	if (index->offSize < 1 || index->offSize > 4)
		fatal(h, cfrErrINDEXHeader);
    
	index->offset = region->begin + 2 + 1;	/* Get offset array base */
    
	/* Read and validate first offset */
	if (readN(h, index->offSize) != 1)
		fatal(h, cfrErrINDEXOffset);
    
	/* Set data reference */
	index->data = index->offset + (index->count + 1)*index->offSize - 1;
    
	/* Read last offset and compute INDEX length */
	srcSeek(h, index->offset + index->count*index->offSize);
	region->end = index->data + readN(h, index->offSize);
}

/* Get an element (offset and length) from INDEX structure. */
static void INDEXGet(cfrCtx h, INDEX *index,
					 unsigned element, ctlRegion *region)
{
	long length;
    
	if (index->count < element)
    /* Requested element out-of-bounds */
		fatal(h, cfrErrINDEXBounds);
    
	/* Get offsets */
	srcSeek(h, index->offset + element*index->offSize);
	region->begin = index->data + readN(h, index->offSize);
	region->end = index->data + readN(h, index->offSize);
    
	/* Validate length */
	length = region->end - region->begin;
	if (length < 0 || length > 65535)
		fatal(h, cfrErrINDEXOffset);
}

/* Read Private DICT. */
static void readPrivate(cfrCtx h, int iFD)
{
	FDInfo *fd = &h->FDArray.array[iFD];
    
	if (fd->region.PrivateDICT.begin == -1)
		fatal(h, cfrErrNoPrivate);
    
	readDICT(h, &fd->region.PrivateDICT, 0);
    
	if (fd->region.LocalSubrINDEX.begin != -1)
    {
		readSubrINDEX(h, &fd->region.LocalSubrINDEX, &fd->Subrs);
		fd->aux.subrs.cnt = fd->Subrs.cnt;
		fd->aux.subrs.offset = fd->Subrs.array;
		fd->aux.subrsEnd = fd->region.LocalSubrINDEX.end;
    }
    
	fd->aux.gsubrs.cnt = h->gsubrs.cnt;
	fd->aux.gsubrs.offset = h->gsubrs.array;
	fd->aux.gsubrsEnd = h->region.GlobalSubrINDEX.end;
}

/* Return the offset of a standard encoded glyph of -1 if none. */
static long getStdEncGlyphOffset(void *ctx, int stdcode)
{
	cfrCtx h = ctx;
	unsigned short gid = h->stdEnc2GID[stdcode];
	return (gid == 0)? -1: h->glyphs.array[gid].sup.begin;
}

/* Initialize region. */
static void initRegion(ctlRegion *region)
{
	region->begin = -1;
	region->end = -1;
}

/* Initialize FDArray element. */
static void initFDInfo(cfrCtx h, int iFD)
{
	FDInfo *fd = h->fd = &h->FDArray.array[iFD];
	abfFontDict *fdict = &h->fdicts.array[iFD];
	fd->flags					= 0;
	initRegion(&fd->region.PrivateDICT);
	initRegion(&fd->region.LocalSubrINDEX);
	fd->aux.flags 				= 0;
	if (h->flags & CFR_UPDATE_OPS)
		fd->aux.flags 			|= T2C_UPDATE_OPS;
	fd->aux.src 				= h->stm.src;
	fd->aux.dbg					= h->stm.dbg;
	fd->aux.stm 				= &h->cb.stm;
	fd->aux.subrs.cnt 			= 0;
	fd->aux.gsubrs.cnt 			= 0;
	fd->aux.defaultWidthX 		= cff_DFLT_defaultWidthX;
	fd->aux.nominalWidthX 		= cff_DFLT_nominalWidthX;
	fd->aux.ctx					= h;
	fd->aux.getStdEncGlyphOffset= getStdEncGlyphOffset;
	fd->fdict 					= fdict;
	abfInitFontDict(fdict);
}

/* Read FDArray. */
static void readFDArray(cfrCtx h)
{
	int i;
    
	/* Read FDArray INDEX */
	if (h->region.FDArrayINDEX.begin == -1)
		fatal(h, cfrErrNoFDArray);
	readINDEX(h, &h->region.FDArrayINDEX, &h->index.FDArray);
	if (h->index.FDArray.count > 256)
		fatal(h, cfrErrBadFDArray);
    
	/* Read FDArray */
	dnaSET_CNT(h->FDArray, h->index.FDArray.count);
	dnaSET_CNT(h->fdicts, h->index.FDArray.count);
	for (i = 0; i < h->FDArray.cnt; i++)
    {
		ctlRegion region;
		INDEXGet(h, &h->index.FDArray, i, &region);
		initFDInfo(h, i);
		readDICT(h, &region, 0);
		readPrivate(h, i);
    }
}

/* Read FontName string and string INDEX strings. */
static void readStrings(cfrCtx h)
{
	Offset offset;
	int i;
	char *p;
	ctlRegion FontName;
	long lenFontName;
	long lenStrings;
    
	if (h->index.name.count != 1)
		fatal(h, cfrErrMultipleFont);
    
	/* Get FontName data and compute its size */
	INDEXGet(h, &h->index.name, 0, &FontName);
	lenFontName = FontName.end - FontName.begin;
    
	/* Compute string data size */
	lenStrings = (h->index.string.count == 0)? 0:
    (h->region.GlobalSubrINDEX.begin -
     h->index.string.data + 1 + 	/* String data bytes */
     h->index.string.count);		/* Null termination */
    
	/* Allocate buffers */
	dnaSET_CNT(h->string.offsets, h->index.string.count + 1);
	dnaSET_CNT(h->string.ptrs, h->index.string.count);
	dnaSET_CNT(h->string.buf, lenFontName + 1 +	lenStrings);
    
	/* Copy FontName into buffer */
	p = h->string.buf.array;
	srcSeek(h, FontName.begin);
	srcRead(h, lenFontName, p);
	p += lenFontName;
	*p++ = '\0';
    
	if (h->index.string.count == 0)
		return;	/* Empty string INDEX */
    
	/* Read string offsets */
	srcSeek(h, h->index.string.offset);
	offset = readN(h, h->index.string.offSize);
	for (i = 0; i < h->index.string.count; i++)
    {
		long length;
		h->string.offsets.array[i] = offset;
		offset = readN(h, h->index.string.offSize);
        
		/* Compute and validate object length */
		length = offset - h->string.offsets.array[i];
		if (length < 0 || length > 65535)
			fatal(h, cfrErrINDEXOffset);
    }
	h->string.offsets.array[i] = offset;
    
	/* Read string data */
	for (i = 0; i < h->string.ptrs.cnt; i++)
    {
		long length =
        h->string.offsets.array[i + 1] - h->string.offsets.array[i];
		srcRead(h, length, p);
		h->string.ptrs.array[i] = p;
		p += length;
		*p++ = '\0';
    }
}

/* Read CharStrings INDEX. */
static void readCharStringsINDEX(cfrCtx h, short flags)
{
	long i;
	INDEX index;
	Offset offset;
    
	/* Read INDEX */
	if (h->region.CharStringsINDEX.begin == -1)
		fatal(h, cfrErrNoCharStrings);
	readINDEX(h, &h->region.CharStringsINDEX, &index);
    
	/* Allocate and initialize glyphs array */
	dnaSET_CNT(h->glyphs, index.count);
	srcSeek(h, index.offset);
	offset = index.data + readN(h, index.offSize);
	for (i = 0; i < index.count; i++)
    {
		long length;
		abfGlyphInfo *info = &h->glyphs.array[i];
        
		abfInitGlyphInfo(info);
		info->flags = flags;
		info->tag = (unsigned short)i;
		info->sup.begin = offset;
		info->sup.end = index.data + readN(h, index.offSize);
        
		/* Validate object length */
		length = info->sup.end - info->sup.begin;
		if (length < 1 || length > 65535)
			fatal(h, cfrErrINDEXOffset);
        
		offset = info->sup.end;
    }
}

/* -------------------------------- Charset -------------------------------- */

/* Add SID/CID to charset */
static void addID(cfrCtx h, long gid, unsigned short id)
{
	abfGlyphInfo *info = &h->glyphs.array[gid];
	if (h->flags & CID_FONT)
    /* Save CID */
		info->cid = id;
	else
    {
		/* Save SID */
		info->gname.impl = id;
		info->gname.ptr = sid2str(h, id);
        
		/* Non-CID font so select FD[0] */
		info->iFD = 0;
        
		if (id < ARRAY_LEN(stdenc))
        {
			/* Add GID of standard encoded info to table */
			int code = stdenc[id];
			if (code != 0)
			{
				h->stdEnc2GID[code] = (unsigned short)gid;
			}
        }
    }
}

/* Initialize from predefined charset */
static void predefCharset(cfrCtx h, int cnt, const SID *charset)
{
	long gid;
    
	if (cnt > h->glyphs.cnt)
    /* Font contains fewer glyphs than predef charset; adjust count */
		cnt = h->glyphs.cnt;
    
	for (gid = 0; gid < cnt; gid++)
		addID(h, (unsigned short)gid, charset[gid]);
}

/* Read charset */
static void readCharset(cfrCtx h)
{
	/* Predefined charsets */
	static const SID isocs[] =
    {
		0,	/* .notdef */
#include "isocs0.h"
    };
	static const SID excs[] =
    {
		0,	/* .notdef */
#include "excs0.h"
    };
	static const SID exsubcs[] =
    {
		0,	/* .notdef */
#include "exsubcs0.h"
    };
    
	switch (h->region.Charset.begin)
    {
        case cff_ISOAdobeCharset:
            predefCharset(h, ARRAY_LEN(isocs), isocs);
            break;
        case cff_ExpertCharset:
            predefCharset(h, ARRAY_LEN(excs), excs);
            break;
        case cff_ExpertSubsetCharset:
            predefCharset(h, ARRAY_LEN(exsubcs), exsubcs);
            break;
        default:
		{
            /* Custom charset */
            long gid;
            int size = 2;
            
            srcSeek(h, h->region.Charset.begin);
            
            gid = 0;
            addID(h, gid++, 0);	/* .notdef */
            
            switch (read1(h))
			{
                case 0:
                    for (; gid < h->glyphs.cnt; gid++)
                        addID(h, gid, read2(h));
                    break;
                case 1:
                    size = 1;
                    /* Fall through */
                case 2:
                    while (gid < h->glyphs.cnt)
                    {
                        unsigned short id = read2(h);
                        long nLeft = readN(h, size);
                        while (nLeft-- >= 0)
                            addID(h, gid++, id++);
                    }
                    break;
                default:
                    fatal(h, cfrErrCharsetFmt);
			}
            h->region.Charset.end = srcTell(h);
		}
            break;
    }
}

/* -------------------------------- Encoding ------------------------------- */

/* Add encoding to glyph. */
static void encAdd(cfrCtx h, abfGlyphInfo *info, unsigned short code)
{
	abfEncoding *enc = &info->encoding;
	if (enc->code == ABF_GLYPH_UNENC)
    {
		/* First encoding; initialize */
		enc->code = code;
		enc->next = NULL;
    }
	else
    {
		/* Second or subsequent encoding; link new encoding */
		abfEncoding *new = h->encfree;
		if (new == NULL)
        /* Get node from heap */
			new = memNew(h, sizeof(abfEncoding));
		else
        /* Remove head node from free list */
			h->encfree = new->next;
        
		new->code = code;
		new->next = enc->next;
		enc->next = new;
		h->flags |= FREE_ENCODINGS;
    }
}

/* Put encoding node list on free list. */
static void encReuse(cfrCtx h, abfEncoding *node)
{
	if (node == NULL)
		return;
	encReuse(h, node->next);
	node->next = h->encfree;
	h->encfree = node;
}

/* Reuse encoding node list. */
static void encListReuse(cfrCtx h)
{
	long i;
    
	if (!(h->flags & FREE_ENCODINGS))
		return;	/* Font has no multiple encodings */
    
	/* Put multiple encodings back on free list */
	for (i = 0; i < h->glyphs.cnt; i++)
    {
		abfEncoding *enc = &h->glyphs.array[i].encoding;
		if (enc->code != ABF_GLYPH_UNENC)
			encReuse(h, enc->next);
    }
	h->flags &= ~FREE_ENCODINGS;
}

/* Free encoding node list. */
static void encListFree(cfrCtx h, abfEncoding *node)
{
	if (node == NULL)
		return;
	encListFree(h, node->next);
	memFree(h, node);
}

/* Initialize from predefined encoding */
static void predefEncoding(cfrCtx h, int cnt, const unsigned char *encoding)
{
	long gid;
	for (gid = 0; gid < h->glyphs.cnt; gid++)
    {
		abfGlyphInfo *info = &h->glyphs.array[gid];
		SID sid = (SID)info->gname.impl;
		if (sid >= cnt)
			break;
		if (encoding[sid] != 0)
			encAdd(h, info, encoding[sid]);
    }
}

/* Read encoding */
static void readEncoding(cfrCtx h)
{
	switch (h->region.Encoding.begin)
    {
        case cff_StandardEncoding:
            predefEncoding(h, ARRAY_LEN(stdenc), stdenc);
            break;
        case cff_ExpertEncoding:
            predefEncoding(h, ARRAY_LEN(exenc), exenc);
            break;
        default:
		{
            /* Custom encoding */
            int i;
            int cnt;
            int fmt;
            long gid = 1;	/* Skip glyph 0 (.notdef) */
            
            /* Read format byte */
            srcSeek(h, h->region.Encoding.begin);
            fmt = read1(h);
            
            switch (fmt & 0x7f)
			{
                case 0:
                    cnt = read1(h);
                    while (gid <= cnt)
                        encAdd(h, &h->glyphs.array[gid++], read1(h));
                    break;
                case 1:
                    cnt = read1(h);
                    while (cnt--)
                    {
                        short code = read1(h);
                        int nLeft = read1(h);
                        while (nLeft-- >= 0)
                            encAdd(h, &h->glyphs.array[gid++], code++);
                    }
                    break;
                default:
                    fatal(h, cfrErrEncodingFmt);
			}
            if (fmt & 0x80)
			{
                /* Read supplementary encoding */
                cnt = read1(h);
                
                for (i = 0; i < cnt; i++)
				{
                    unsigned short code = read1(h);
                    SID sid = read2(h);
                    
                    /* Search for glyph with this SID. Glyphs with multiple
                     encodings are rare so linear search is acceptable */
                    for (gid = 0; gid < h->glyphs.cnt; gid++)
					{
                        abfGlyphInfo *info = &h->glyphs.array[gid];
                        if (info->gname.impl == sid)
						{
                            /* Found match: add encoding to list */
                            encAdd(h, info, code);
                            goto next;
						}
					}
                    
                    fatal(h, cfrErrEncodingSupp);
                    
                next: ;
				}
			}
            h->region.Encoding.end = srcTell(h);
		}
            break;
    }
}

/* -------------------------------- FDSelect ------------------------------- */

/* Read FDSelect */
static void readFDSelect(cfrCtx h)
{
	long gid;
    
	if (h->region.FDSelect.begin == -1)
		fatal(h, cfrErrNoFDSelect);
	
	srcSeek(h, h->region.FDSelect.begin);
	switch (read1(h))
    {
        case 0:
            for (gid = 0; gid < h->glyphs.cnt; gid++)
                h->glyphs.array[gid].iFD = read1(h);
            break;
        case 3:
		{
            int nRanges = read2(h);
            
            gid = read2(h);
            while (nRanges--)
			{
                int fd = read1(h);
                long next = read2(h);
                
                while (gid < next)
                    h->glyphs.array[gid++].iFD = fd;
			}
		}
            break;
        default:
            fatal(h, cfrErrFDSelectFmt);
    }
	h->region.FDSelect.end = srcTell(h);
}

/* ------------------------------- Interface ------------------------------- */

/* Report absfont error message to debug stream. */
static void report_error(abfErrCallbacks *cb, int err_code, int iFD)
{
	cfrCtx h = cb->ctx;
	if (iFD == -1)
		message(h, "%s (ignored)", abfErrStr(err_code));
	else
		message(h, "%s FD[%d] (ignored)", abfErrStr(err_code), iFD);
}

/* Begin reading new font. */
int cfrBegFont(cfrCtx h, long flags, long origin, int ttcIndex, abfTopDict **top)
{
	long i;
	ctlRegion region;
	short gi_flags;	/* abfGlyphInfo flags */
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	/* Initialize top DICT */
	abfInitTopDict(&h->top);
    
	srcOpen(h, origin, ttcIndex);
    
	/* Initialize */
	h->flags = flags & 0xffff;
	h->fd = NULL;
	h->glyphsByName.cnt = 0;
	h->glyphsByCID.cnt = 0;
	memset(h->stdEnc2GID, 0, sizeof(h->stdEnc2GID));
	dnaSET_CNT(h->FDArray, 1);
	dnaSET_CNT(h->fdicts, 1);
	h->region.Encoding.begin = cff_ISOAdobeCharset;
	h->region.Encoding.end = -1;
	h->region.Charset.begin = cff_StandardEncoding;
	h->region.Charset.end = -1;
    initRegion(&h->region.FDSelect);
    initRegion(&h->region.CharStringsINDEX);
    initRegion(&h->region.FDArrayINDEX);
	initFDInfo(h, 0);
    
	/* Read header */
	h->region.Header.begin = h->src.origin;
	h->header.major = read1(h);
	h->header.minor = read1(h);
	h->header.hdrSize = read1(h);
	h->header.offSize = read1(h);
	if ((h->header.major != 1) && (h->header.major != 2))
		fatal(h, cfrErrBadFont);
	h->region.Header.end = h->src.origin + h->header.hdrSize;
    
	/* Read name INDEX */
	h->region.NameINDEX.begin = h->region.Header.end;
	readINDEX(h, &h->region.NameINDEX, &h->index.name);
    
	/* Read top INDEX */
	h->region.TopDICTINDEX.begin = h->region.NameINDEX.end;
	readINDEX(h, &h->region.TopDICTINDEX, &h->index.top);
    
	/* Read string INDEX  */
	h->region.StringINDEX.begin = h->region.TopDICTINDEX.end;
	readINDEX(h, &h->region.StringINDEX, &h->index.string);
    
	/* Read strings */
	h->region.GlobalSubrINDEX.begin = h->region.StringINDEX.end;
	readStrings(h);
    
	/* Read gsubrs INDEX */
	readSubrINDEX(h, &h->region.GlobalSubrINDEX, &h->gsubrs);
	
	/* Read top DICT */
	INDEXGet(h, &h->index.top, 0, &region);
	readDICT(h, &region, 1);
    
	gi_flags = 0;
	if (h->flags & CID_FONT)
    {
		if (!(flags & CFR_SHALLOW_READ))
			readFDArray(h);
		h->top.cid.CIDFontName.ptr = h->string.buf.array;
		h->top.sup.srcFontType = abfSrcFontTypeCFFCID;
		h->top.sup.flags |= ABF_CID_FONT;
		gi_flags |= ABF_GLYPH_CID;
    }
	else
    {
		readPrivate(h, 0);
		h->fd->fdict->FontName.ptr = h->string.buf.array;
		h->top.sup.srcFontType = abfSrcFontTypeCFFName;
    }
    
	if (!(flags & CFR_SHALLOW_READ))
		readCharStringsINDEX(h, gi_flags);
    
	/* Prepare client data */
	h->top.FDArray.cnt = h->fdicts.cnt;
	h->top.FDArray.array = h->fdicts.array;
	h->top.sup.nGlyphs = h->glyphs.cnt;
	*top = &h->top;
    
    if (!(h->flags & CFR_SEEN_BLEND))
    {
        /* Validate dictionaries */
        if (h->stm.dbg == NULL)
            abfCheckAllDicts(NULL, &h->top);
        else
        {
            abfErrCallbacks cb;
            cb.ctx = h;
            cb.report_error = report_error;
            abfCheckAllDicts(&cb, &h->top);
        }
    }
    
	/* Fill glyphs array */
	if (!(flags & CFR_SHALLOW_READ))
    {
		readCharset(h);
		if (h->flags & CID_FONT)
        {
			readFDSelect(h);
            
			/* Mark LanguageGroup 1 glyphs */
			for (i = 0; i < h->glyphs.cnt; i++)
            {
				abfGlyphInfo *info = &h->glyphs.array[i];
				if (h->fdicts.array[info->iFD].Private.LanguageGroup == 1)
					info->flags |= ABF_GLYPH_LANG_1;
            }
            
			/* We pre-multiplied the FontMatrix's in the FDArray with the one in
             the top dict FontMatrix so we can discard it */
			h->top.cid.FontMatrix.cnt = ABF_EMPTY_ARRAY;
        }
		else
        {
			if (!(flags & CFR_NO_ENCODING))
				readEncoding(h);
            
			if (h->fdicts.array[0].Private.LanguageGroup == 1)
            /* Mark all glyphs LanguageGroup 1 */
				for (i = 0; i < h->glyphs.cnt; i++)
					h->glyphs.array[i].flags |= ABF_GLYPH_LANG_1;
        }
    }
    
	/* Check each Private dict contained a BlueValues operator */
	for (i = 0; i < h->fdicts.cnt; i++)
		if (!(h->FDArray.array[i].flags & SEEN_BLUE_VALUES))
			message(h, "/BlueValues missing: FD[%ld]", i);
    
	return cfrSuccess;
}

/* Read GSubr as if it were a glyph. */
static void readGSubr(cfrCtx h, abfGlyphCallbacks *glyph_cb, Offset gsubrStartOffset, Offset gsubrEndOffset, abfGlyphInfo *glyph_info, t2cAuxData *glyph_aux )
{
	int result;
	
	/* Copy template info from .notdef glyph. gname or cid is not used when writing GSUBr's,
     o we don't need to change this. */
	abfGlyphInfo info = *glyph_info;
	t2cAuxData aux = *glyph_aux;
    
	info.sup.begin = gsubrStartOffset;
	info.sup.end = gsubrEndOffset;
	info.flags |= ABF_GLYPH_CUBE_GSUBR;
	aux.flags |= T2C_CUBE_GSUBR;
	info.flags &= ~ABF_GLYPH_SEEN;
	/* Begin glyph and mark it as seen */
	result = glyph_cb->beg(glyph_cb, &info);
    
	/* Check result */
	switch (result)
    {
        case ABF_CONT_RET:
            aux.flags &= ~T2C_WIDTH_ONLY;
            break;
        case ABF_WIDTH_RET:
            aux.flags |= T2C_WIDTH_ONLY;
            break;
        case ABF_SKIP_RET:
            return;
        case ABF_QUIT_RET:
            fatal(h, cfrErrCstrQuit);
        case ABF_FAIL_RET:
            fatal(h, cfrErrCstrFail);
    }
    
	if (h->flags & CFR_IS_CUBE)
		aux.flags |= T2C_IS_CUBE;
    
	if (h->flags & CFR_CUBE_RND)
		aux.flags |= T2C_CUBE_RND;
    
	/* Parse charstring */
	result = t2cParse(info.sup.begin, info.sup.end, &aux, glyph_cb);
	if (result)
    {
		if (info.flags & ABF_GLYPH_CID)
			message(h, "(t2c) %s <cid-%hu>", t2cErrStr(result), info.cid);
		else
			message(h, "(t2c) %s <%s>", t2cErrStr(result), info.gname.ptr);
		fatal(h, cfrErrCstrParse);
    }
    
	/* End glyph */
	glyph_cb->end(glyph_cb);
}



/* Read charstring. */
static void readGlyph(cfrCtx h,
					  unsigned short gid, abfGlyphCallbacks *glyph_cb)
{
	int result;
	long flags = h->flags;
	abfGlyphInfo *info = &h->glyphs.array[gid];
	t2cAuxData *aux = &h->FDArray.array[info->iFD].aux;
    
	/* Begin glyph and mark it as seen */
	result = glyph_cb->beg(glyph_cb, info);
	info->flags |= ABF_GLYPH_SEEN;
    
	/* Check result */
	switch (result)
    {
        case ABF_CONT_RET:
            aux->flags &= ~T2C_WIDTH_ONLY;
            break;
        case ABF_WIDTH_RET:
            aux->flags |= T2C_WIDTH_ONLY;
            break;
        case ABF_SKIP_RET:
            return;
        case ABF_QUIT_RET:
            fatal(h, cfrErrCstrQuit);
        case ABF_FAIL_RET:
            fatal(h, cfrErrCstrFail);
    }
    
	if (h->flags & CFR_IS_CUBE)
		aux->flags |= T2C_IS_CUBE;
	if (h->flags & CFR_FLATTEN_CUBE)
		aux->flags |= T2C_FLATTEN_CUBE;
	if (h->flags & CFR_CUBE_RND)
		aux->flags |= T2C_CUBE_RND;
    
	/* Parse charstring */
	result = t2cParse(info->sup.begin, info->sup.end, aux, glyph_cb);
	if (result)
    {
		if (info->flags & ABF_GLYPH_CID)
			message(h, "(t2c) %s <cid-%hu>", t2cErrStr(result), info->cid);
		else
			message(h, "(t2c) %s <%s>", t2cErrStr(result), info->gname.ptr);
		fatal(h, cfrErrCstrParse);
    }
    
	/* End glyph */
	glyph_cb->end(glyph_cb);
	
    
	/* if it is a CUBE font, play the GSUBR's through to the client */
	if ( (flags & CFR_IS_CUBE) && (glyph_cb->cubeSetwv != NULL) && !(flags & CFR_FLATTEN_CUBE) && !(flags & CFR_SEEN_GLYPH) && (h->gsubrs.cnt > 10))
    {
		int i;
        Offset gsubrStartOffset = 0;
        Offset gsubrEndOffset;
        h->flags |= CFR_SEEN_GLYPH;
		for (i = 0; i < h->gsubrs.cnt; i++)
        {
            
			gsubrStartOffset = aux->gsubrs.offset[i];
			if ((i +1) < aux->gsubrs.cnt)
				gsubrEndOffset = aux->gsubrs.offset[i+1];
			else
				gsubrEndOffset = aux->gsubrsEnd;
			readGSubr(h, glyph_cb, gsubrStartOffset, gsubrEndOffset, info, aux);
        }
    }
    
}

/* Iterate through all glyphs in font. */
int cfrIterateGlyphs(cfrCtx h, abfGlyphCallbacks *glyph_cb)

{
	long i;
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	for (i = 0; i < h->glyphs.cnt; i++)
		readGlyph(h, (unsigned short)i, glyph_cb);
    
	return cfrSuccess;
}

/* Get glyph from font by its tag. */
int cfrGetGlyphByTag(cfrCtx h, 
					 unsigned short tag, abfGlyphCallbacks *glyph_cb)
{
	if (tag >= h->glyphs.cnt)
		return cfrErrNoGlyph;
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, tag, glyph_cb);
    
	return cfrSuccess;
}

/* Compare glyph names. */
static int CTL_CDECL cmpNames(const void *first, const void *second, void *ctx)
{
	cfrCtx h = ctx;
	return strcmp(h->glyphs.array[*(unsigned short *)first].gname.ptr,
				  h->glyphs.array[*(unsigned short *)second].gname.ptr);
}

/* Match glyph name. */
static int CTL_CDECL matchName(const void *key, const void *value, void *ctx)
{
	cfrCtx h = ctx;
	return strcmp((char *)key,
				  h->glyphs.array[*(unsigned short *)value].gname.ptr);
}

/* Get glyph from font by its name. */
int cfrGetGlyphByName(cfrCtx h, char *gname, abfGlyphCallbacks *glyph_cb)
{
	size_t index;
    
	if (h->flags & CID_FONT)
		return cfrErrNoGlyph;
    
	if (h->glyphsByName.cnt == 0)
    {
		/* Make lookup array */
		long i;
		dnaSET_CNT(h->glyphsByName, h->glyphs.cnt);
		for (i = 0; i < h->glyphsByName.cnt; i++)
			h->glyphsByName.array[i] = (unsigned short)i;
		ctuQSort(h->glyphsByName.array, h->glyphsByName.cnt, 
				 sizeof(h->glyphsByName.array[0]), cmpNames, h);
    }
    
	if (!ctuLookup(gname, h->glyphsByName.array, h->glyphsByName.cnt, 
				   sizeof(h->glyphsByName.array[0]), matchName, &index, h))
		return cfrErrNoGlyph;
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, (unsigned short)h->glyphsByName.array[index], glyph_cb);
    
	return cfrSuccess;
}

/* Compare cids. */
static int CTL_CDECL cmpCIDs(const void *first, const void *second, void *ctx)
{
	cfrCtx h = ctx;
	unsigned short a = h->glyphs.array[*(unsigned short *)first].cid;
	unsigned short b = h->glyphs.array[*(unsigned short *)second].cid;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
}

/* Match CID value. */
static int CTL_CDECL matchCID(const void *key, const void *value, void *ctx)
{
	cfrCtx h= ctx;
	unsigned short a = *(unsigned short *)key;
	unsigned short b = h->glyphs.array[*(unsigned short *)value].cid;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
}

/* Get glyph from font by its CID. */
int cfrGetGlyphByCID(cfrCtx h, unsigned short cid, abfGlyphCallbacks *glyph_cb)
{
	volatile unsigned short gid;	/* volatile suppresses optimizer warning */
	size_t index;
    
	if (!(h->flags & CID_FONT))
		return cfrErrNoGlyph;
    
	if (h->glyphs.array[h->glyphs.cnt - 1].cid == h->glyphs.cnt - 1)
    {
		/* Non-subset font; index by CID */
		if (cid >= h->glyphs.cnt)
			return cfrErrNoGlyph;
		else
			gid = cid;
    }
	else
    { 		/* Subset font; search for matching CID */
        
		if (h->glyphsByCID.cnt == 0)
        {
			/* Make lookup array */
			long i;
			dnaSET_CNT(h->glyphsByCID, h->glyphs.cnt);
			for (i = 0; i < h->glyphsByCID.cnt; i++)
				h->glyphsByCID.array[i] = (unsigned short)i;
			/* Sort glyphs array by cid before doing search. */
			ctuQSort(h->glyphsByCID.array, h->glyphsByCID.cnt,
                     sizeof(h->glyphsByCID.array[0]), cmpCIDs, h);
        }
        
		if (!ctuLookup(&cid, h->glyphsByCID.array, h->glyphsByCID.cnt, 
                       sizeof(h->glyphsByCID.array[0]), matchCID, &index, h))
			return cfrErrNoGlyph;
        
		gid = h->glyphsByCID.array[index];
    }
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, gid, glyph_cb);
    
	return cfrSuccess;
}

/* Get standard-encoded glyph from font. */
int cfrGetGlyphByStdEnc(cfrCtx h, int stdcode, abfGlyphCallbacks *glyph_cb)
{
	unsigned short gid;
    
	if (stdcode < 0 || stdcode >= 256 || (gid = h->stdEnc2GID[stdcode]) == 0)
		return cfrErrNoGlyph;
    
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, gid, glyph_cb);
    
	return cfrSuccess;
}

/* Clear record of glyphs seen by client. */
int cfrResetGlyphs(cfrCtx h)
{
	long i;
	
	for (i = 0; i < h->glyphs.cnt; i++)
		h->glyphs.array[i].flags &= ~ABF_GLYPH_SEEN;
    
	return cfrSuccess;
}

/* Return single regions. */
const cfrSingleRegions *cfrGetSingleRegions(cfrCtx h)
{
	return &h->region;
}

/* Return repeat regions. */
const cfrRepeatRegions *cfrGetRepeatRegions(cfrCtx h, int iFD)
{
	return (iFD < 0 || iFD >= h->FDArray.cnt)?
    NULL: &h->FDArray.array[iFD].region;
}

/* Return font DICT widths. */
int cfrGetWidths(cfrCtx h, int iFD, 
				 float *defaultWidthX, float *nominalWidthX)
{	
	t2cAuxData *aux;
	if (iFD < 0 || iFD >= h->FDArray.cnt)
		return 1;
	aux = &h->FDArray.array[iFD].aux;
	*defaultWidthX = aux->defaultWidthX;
	*nominalWidthX = aux->nominalWidthX;
	return 0;
}

/* Return table mapping standard encoding and glyph index. */
const unsigned char *cfrGetStdEnc2GIDMap(cfrCtx h)
{
	return (unsigned char *)h->stdEnc2GID;
}

/* Finish reading font. */
int cfrEndFont(cfrCtx h)
{
	/* Close source stream */
	if (h->cb.stm.close(&h->cb.stm, h->stm.src) == -1)
		return cfrErrSrcStream;
    
	encListReuse(h);
    
	return cfrSuccess;
}

/* Get version numbers of libraries. */
void cfrGetVersion(ctlVersionCallbacks *cb)
{
	if (cb->called & 1<<CFR_LIB_ID)
		return;	/* Already enumerated */
    
	/* Support libraries */
	abfGetVersion(cb);
	ctuGetVersion(cb);
	dnaGetVersion(cb);
	sfrGetVersion(cb);
	t2cGetVersion(cb);
    
	/* This library */
	cb->getversion(cb, CFR_VERSION, "cffread");
    
	/* Record this call */
	cb->called |= 1<<CFR_LIB_ID;
}

/* Convert error code to error string. */
char *cfrErrStr(int err_code)
{
	static char *errstrs[] =
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "cfrerr.h"
    };
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
    "unknown error": errstrs[err_code];
}

/* ----------------------------- Debug Support ----------------------------- */

#if CFR_DEBUG

#include <stdio.h>
static void dbt2cstack(cfrCtx h)
{
	printf("--- stack\n");
	if (h->stack.cnt == 0)
		printf("empty\n");
	else
    {
		int i;
		for (i = 0; i < h->stack.cnt; i++)
			printf("[%d]=%g ", i, h->stack.array[i]);
		printf("\n");
    }
}

/* This function just serves to suppress annoying "defined but not used"
 compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...)
{
	dbuse(0, dbt2cstack);
}

#endif /* CFR_DEBUG */

