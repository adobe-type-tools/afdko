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
#include "nameread.h"
#include "varread.h"
#include "supportfp.h"
#include "supportexcept.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <stdlib.h>
#include <limits.h>
#include <math.h>
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

static char *stdstrs[] =
{
#include "stdstr1.h"
};


/* Glyph names for Standard Apple Glyph Ordering */
static char *applestd[258] =
{
#include "applestd.h"
};

#define SHORT_PS_NAME_LIMIT   63      /* according to the OpenType name table spec */
#define LONG_PS_NAME_LIMIT    127     /* according to Adobe tech notes #5902 */
#define STRING_BUFFER_LIMIT   1024    /* used to load family name/full name/copyright/trademark strings */

#define ARRAY_LEN(t) 	(sizeof(t)/sizeof((t)[0]))

typedef unsigned char OffSize;  /* Offset size indicator */
typedef long Offset;			/* 1, 2, 3, or 4-byte offset */
typedef unsigned short SID;		/* String identifier */
#define SID_SIZE	2			/* SID byte size */

typedef struct          		/* INDEX */
{
	unsigned long count;		/* Element count */
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
} cfrFDInfo;

typedef struct					/* Operand Stack element */
{
    int is_int;
    union
    {
        long int_val;		/* allow at least 32-bit int */
        float real_val;
    } u;
        unsigned short numBlends;    /* the last operand before the blend operator.
                            The number of items in blend_val is numBlends*(h->numRegions)
                            */
        float* blend_val;  /* NULL unless the blend operator has been encountered. */
} stack_elem;

typedef struct					/* Format 2.0 data */
{
    dnaDCL(unsigned short, glyphNameIndex);
    dnaDCL(char *, strings);
    dnaDCL(char, buf);
} postFmt2;

typedef short FWord;

typedef struct					/* post table */
{
    Fixed format;				/* =1.0, 2.0, 2.5, 3.0, 4.0 */
    Fixed italicAngle;
    FWord underlinePosition;
    FWord underlineThickness;
    unsigned long isFixedPitch;
    unsigned long minMemType42;
    unsigned long maxMemType42;
    unsigned long minMemType1;
    unsigned long maxMemType1;
    postFmt2 fmt2;				
} postTbl;

struct cfrCtx_					/* Context */
{
	long flags;					/* Status flags */
#define CID_FONT		(1UL<<31)	/* CID Font */
#define FREE_ENCODINGS	(1UL<<30)	/* Return encoding nodes to free list */
#define CFR_SEEN_BLEND  (1<<29)
#define CFR_IS_CFF2     (1<<28)
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
	dnaDCL(cfrFDInfo, FDArray);	/* FDArray */
	dnaDCL(abfFontDict, fdicts);/* Font Dicts */
	cfrFDInfo *fd;				/* Active Font Dict */
	struct						/* DICT operand stack */
    {
        /* Operands are pushed on the stack as they are encountered. However,
         when the 'blend' operator is called, the stack is edited. See the comment in copyToBlendStackElement()
         for what is done.
         */
        int cnt;
        int numRegions; /* current number of regions ( 1 less than the numer of source master designs), defined by
                         the varData structure which is selected by  the current vsindex into the VarStore. */
		stack_elem array[CFF2_MAX_OP_STACK];
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
    postTbl post;			/* post table */
    struct                      /* CFF2 font tables */
    {
		float           *UDV;	/* From client */
        var_F2dot14     ndv[CFF2_MAX_AXES];     /* normalized weight vector */
        float           scalars[CFF2_MAX_MASTERS];               /* scalar values for regions */
        unsigned short  regionCount;          /* number of all regions */
        unsigned short  regionIndices[CFF2_MAX_MASTERS];  /* region indices for the current vsindex */

        unsigned short  axisCount;
        var_axes        axes;
        var_hmtx        hmtx;
        var_MVAR        mvar;
        nam_name        name;
        var_itemVariationStore  varStore;
		void            *lastResortInstanceNameClientCtx;
        cfrlastResortInstanceNameCallback    lastResortInstanceNameCallback;
    } cff2;
	struct						/* Client callbacks */
    {
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
        cff2GlyphCallbacks cff2;
        ctlSharedStmCallbacks shstm;
    } cb;
	struct						/* Contexts */
    {
		dnaCtx dna;				/* dynarr */
		sfrCtx sfr;				/* sfntread */
    } ctx;
	struct					/* Error handling */
    {
		_Exc_Buf env;
    } err;
};


static void encListFree(cfrCtx h, abfEncoding *node);
static void setupSharedStream(cfrCtx h);

/* ----------------------------- Error Handling ---------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(cfrCtx h, char *fmt, va_list ap)
{
	char text[500];
    const size_t textLen = sizeof(text);
	if (h->stm.dbg == NULL)
		return;	/* Debug stream not available */
    
	VSPRINTF_S(text, textLen, fmt, ap);
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
	RAISE(&h->err.env, err_code, NULL);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(cfrCtx h, size_t size)
{
	void *ptr = h->cb.mem.manage(&h->cb.mem, NULL, size);
	if (ptr == NULL)
		fatal(h, cfrErrNoMemory);

	/* Safety initialization */
	memset(ptr, 0, size);

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
	cfrCtx h = (cfrCtx)cb->ctx;
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
static void initFDArray(void *ctx, long cnt, cfrFDInfo *info)
{
	cfrCtx h = (cfrCtx)ctx;
	while (cnt--)
    {
		memset(info, 0, sizeof(*info));
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
	h = (cfrCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct cfrCtx_));
	if (h == NULL)
		return NULL;
    
	/* Safety initialization */
	memset(h, 0, sizeof(*h));
    
	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Set error handler */
    DURING_EX(h->err.env)
    
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
        dnaINIT(h->ctx.dna, h->post.fmt2.glyphNameIndex, 256, 768);
        dnaINIT(h->ctx.dna, h->post.fmt2.strings, 50, 200);
        dnaINIT(h->ctx.dna, h->post.fmt2.buf, 300, 1200);
        dnaINIT(h->ctx.dna, h->string.offsets, 16, 256);
        dnaINIT(h->ctx.dna, h->string.ptrs, 16, 256);
        dnaINIT(h->ctx.dna, h->string.buf, 200, 2000);

        /* Open optional debug stream */
        h->stm.dbg = h->cb.stm.open(&h->cb.stm, CFR_DBG_STREAM_ID, 0);
        
        /* Set up shared source stream callbacks */
        setupSharedStream(h);
    
    HANDLER
        /* Initialization failed */
        cfrFree(h);
        h = NULL;
    END_HANDLER

    return h;
}

static void freeBlend(cfrCtx h, abfOpEntry *blend)
{
    if (blend->blendValues != NULL)
    {
        memFree(h, blend->blendValues);
        blend->blendValues = NULL;
}
}

static void freeBlendArray(cfrCtx h, abfOpEntryArray *blendArray)
{
    long    i;
    for (i = 0; i < blendArray->cnt; i++)
        freeBlend(h, &blendArray->array[i]);
}

static void freePrivateBlends(cfrCtx h, abfPrivateDict *priv)
{
	freeBlend(h, &priv->blendValues.StdHW);
	freeBlend(h, &priv->blendValues.StdVW);
	freeBlend(h, &priv->blendValues.BlueScale);
	freeBlend(h, &priv->blendValues.BlueShift);
	freeBlend(h, &priv->blendValues.BlueFuzz);
	freeBlendArray(h, &priv->blendValues.BlueValues);
    freeBlendArray(h, &priv->blendValues.OtherBlues);
	freeBlendArray(h, &priv->blendValues.FamilyBlues);
	freeBlendArray(h, &priv->blendValues.FamilyOtherBlues);
	freeBlendArray(h, &priv->blendValues.StemSnapH);
	freeBlendArray(h, &priv->blendValues.StemSnapV);
}

/* Free library context. */
void cfrFree(cfrCtx h)
{
	int i;
    
	if (h == NULL)
		return;
    
	/* Free dynamic arrays */
	for (i = 0; i < h->FDArray.size; i++)
	{
		dnaFREE(h->FDArray.array[i].Subrs);
	}
 
    /* Free dynamic arrays */
    for (i = 0; i < h->FDArray.cnt; i++)
    {
		if (h->FDArray.array[i].fdict)
			freePrivateBlends(h, &h->FDArray.array[i].fdict->Private);
    }

	dnaFREE(h->gsubrs);
	dnaFREE(h->FDArray);
	dnaFREE(h->fdicts);
	dnaFREE(h->glyphs);
	dnaFREE(h->glyphsByName);
	dnaFREE(h->glyphsByCID);
    dnaFREE(h->post.fmt2.glyphNameIndex);
    dnaFREE(h->post.fmt2.strings);
    dnaFREE(h->post.fmt2.buf);
	dnaFREE(h->string.offsets);
	dnaFREE(h->string.ptrs);
	dnaFREE(h->string.buf);
    
    if (h->cff2.varStore != NULL) {
        /* Call this here rather than end cfrEndFont, so that
        the allocated memory will be available to aby client modules
         */
        var_freeItemVariationStore(&h->cb.shstm, h->cff2.varStore);
    }
    
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
                for (i = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr))!=0; i++)
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
                    {
                        table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('C','F','F','2'));
                    }
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
do if(h->stack.cnt+(n)>h->top.maxstack)fatal(h,cfrErrStackOverflow);while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define INDEX_INT(i) (INDEX(i).is_int? INDEX(i).u.int_val: (long)INDEX(i).u.real_val)
#define PUSH_INT(v) { stack_elem*	elem = &h->stack.array[h->stack.cnt++];	\
elem->is_int = 1; elem->u.int_val = (v); elem->numBlends = 0; }
#define INDEX_REAL(i) (INDEX(i).is_int? (float)INDEX(i).u.int_val: INDEX(i).u.real_val)
#define PUSH_REAL(v) { stack_elem*	elem = &h->stack.array[h->stack.cnt++];	\
elem->is_int = 0; elem->u.real_val = (v); elem->numBlends = 0;}

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


static void handleBlend(cfrCtx h)
{
    /* When the CFF2 blend operator is encountered, the last items on the stack are
     the blend operands. These are in order:
     numBlends operands from the default font
     (numMaster-1)* numBlends blend delta operands
     numBlends value
     
     What we do is to find the first blend operand item, which is the first value from the
     default font. We assign numBlends to the field of the same name in this struct_elem, and pop
     it off the stack. We then allocate memory to hold the (numMaster-1)* numBlends blend delta operands,
     and copy them to the blendValues field. We then pop the (numMaster-1)* numBlends blend delta operands,
     leaving the numBlends operands.
     When an operator is encountered, a handler that does not know about the blend values will
     simply see the usual stack values from the default fonts. A handler which does know about
     the blend values can use the blend values to fill the blendable fields. 
     
     Alternatively, the blend handler can use the blend operands to actually produce blended values,
     and replace the  numBlends operands from the defaultfont with the blended values.
     */
    
    int numBlends = INDEX_INT(h->stack.cnt -1);
    stack_elem* firstItem;
    int i = 0;
    int numDeltaBlends = numBlends*h->stack.numRegions;
    int firstItemIndex;
    h->flags |= CFR_SEEN_BLEND;

    h->stack.cnt--;

	if (numBlends < 0 || numDeltaBlends < 0)
		fatal(h, cfrErrStackUnderflow);
    CHKUFLOW(numBlends + numDeltaBlends);
    firstItemIndex = (h->stack.cnt - (numBlends + numDeltaBlends));
    firstItem = &(h->stack.array[firstItemIndex]);

    if (h->flags & CFR_FLATTEN_VF)
    {
        /* Blend values on the stack and replace the default values with the results.
         */
        for (i = 0; i < numBlends; i++) {
            float   val = INDEX_REAL(firstItemIndex + i);
            int r;

            for (r = 0; r < h->stack.numRegions; r++) {
                int index = (i * h->stack.numRegions) + r + (h->stack.cnt - numDeltaBlends);
                val += INDEX_REAL(index) * h->cff2.scalars[h->cff2.regionIndices[r]];
            }

            firstItem[i].is_int = 0;
            firstItem[i].u.real_val = val;
            firstItem[i].numBlends = 0;
        }
        
        h->stack.cnt -= numDeltaBlends;
    }
    else
    {
    
        firstItem->numBlends = (unsigned short)numBlends;
        firstItem->blend_val = (float *)memNew(h, sizeof(float)*numDeltaBlends);
        while (i < numDeltaBlends)
        {
            int index = i + (h->stack.cnt - numDeltaBlends);
            float val= INDEX_REAL(index);
            firstItem->blend_val[i] = val;
            i++;
        }
        h->stack.cnt -= numDeltaBlends;
    }
}


/* Get string from its sid. */
static char *sid2str(cfrCtx h, long sid)
{
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

static int hasBlend(cfrCtx h)
{
    int i;
    int hasBlend = 0;
    for (i = 0; i < h->stack.cnt; i++)
    {
        if (INDEX(i).numBlends > 0)
        {
            hasBlend = 1;
        }
    }
    return hasBlend;
}

/* Save real delta array operands. */
static void saveDeltaArray(cfrCtx h, size_t max, long *cnt, float *array, long *blendCnt, abfOpEntry* blendArray)
{
    /* The regular values on the stack just need to be copied to the regular value array, and this happens
     whether or not there are blend operators.
     
     However, some of the values are the default font values from the operands for a blend operation.
     If any of these exist in the stack, then we fill out the blendArray. The regular values in the stack are just
     copied over as is. However, when we encounter a stack entry with numBlends > 0, we know this is the start of
     numBlends operands from a blend operation. In this case, we first copy the numBlends operands
     the current blendArray entry, and then copy the numBlends * numMaster delta values
     from that first stack entry to the current blendArray entry. The current blend entry then contains all the
     operands for the blend operator.
     We then skip over the numBlends stack entries, as these have already been copied to the current blendArray entry.
     
     Note that the blendArray will have fewer elements than the regular stack, as each set of numBlends default font
     values from a blend operation will be copied into a single blendArray entry.
     */
    
	int i,j, k, bi;
    int numBlends;

    if (h->stack.cnt == 0 || h->stack.cnt > (long)max)
        fatal(h, cfrErrDICTArray);

    array[0] = INDEX_REAL(0);
    for (i = 1; i < h->stack.cnt; i++)
        array[i] = array[i - 1] + INDEX_REAL(i);
    
    *cnt = h->stack.cnt;

    if (!hasBlend(h))
    {
        *blendCnt = 0; /* show that there are no blended values - the regular array should be used. */
    }
    else
    {
        float lastValue = 0;
        
        i = j = k = bi = 0;
        while (i < h->stack.cnt)
        {
            abfOpEntry *blendEntry = &blendArray[bi];
            
            stack_elem *stackEntry = &(INDEX(i));
            numBlends = stackEntry->numBlends;

            if (numBlends == 0)
            {
                blendEntry->value = lastValue + (stackEntry->is_int) ? (float)stackEntry->u.int_val : stackEntry->u.real_val;
                lastValue = blendEntry->value;
                blendEntry->numBlends = 0;
                blendEntry->blendValues = NULL;
                i++;
                bi++;
            }
            else
            {
                int l;
                float val;
                signed int numRegions = h->stack.numRegions;
                unsigned short numBlendValues = (unsigned short)(numBlends +  numBlends*numRegions);
                float *blendValues = (float *)memNew(h, sizeof(abfOpEntry)*numBlendValues);
                
                blendEntry->numBlends = (unsigned short)numBlends;
                blendEntry->blendValues = blendValues;
                
                /* first, copy the numBlend default values to blendArray[i], summing the deltas to give absolute values. */
                for (j = 0; j < numBlends; j++)
                {
                    val = INDEX_REAL(i);
                    blendValues[j] = lastValue + val;
                    lastValue = blendValues[j];
                    i++;
                }
                /* now, copy the blend deltas default values to blendArray[i]. 
                We write all the absolute values for region 0, then for region 1, ...,r egion n.*/
                l = numBlends;
                for (j = 0; j < numRegions; j++)
                {
                    float diff = 0;
                    for (k = 0; k < numBlends; k++)
                    {
                        val = stackEntry->blend_val[j + numRegions*k];
                        diff += val;
                        blendValues[l++] = blendValues[k] + diff;
                    }
                }
                
               bi++;
           }
        }
        *blendCnt = bi;
    }
    
}

static void saveBlend(cfrCtx h, float *realValue, abfOpEntry *blendEntry)
{
    /* Save a value that may be blended. Stack depth is 1 for a non-blended value, or numBlends for a blended value. */
    int i;
    int numBlends;
    
    stack_elem *stackEntry = &(INDEX(0));
    numBlends = stackEntry->numBlends;
    
    *realValue = (stackEntry->is_int) ? (float)stackEntry->u.int_val : stackEntry->u.real_val;

    if (numBlends == 0)
    {
        blendEntry->value = *realValue;
        blendEntry->numBlends = 0; /* shows there is no blend value, and the regular value shoud be used instead. */
        blendEntry->blendValues = NULL;
    }
    else
    {
        float defaultValue;
        signed int numRegions = h->stack.numRegions;
        unsigned short numBlendValues = (unsigned short)(numBlends + numRegions);
        float *blendValues = (float *)memNew(h, sizeof(abfOpEntry)*numBlendValues);

        blendEntry->numBlends = (unsigned short)numBlends;
        blendEntry->blendValues = blendValues;
        
        // copy the defaul region value
        blendValues[0] = defaultValue = INDEX_REAL(0);

        /* now, copy the blend abusolate values to blendArray[i].
        The default region value is in stackEntry->int_val or real_val
        The region delta values are in the stackEntry->blend_val array.
        */
        
       for (i = 0; i < numRegions; i++)
        {
            blendValues[i+1] = stackEntry->blend_val[i] + defaultValue;
        }
        
    }

   
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

static int setNumMasters(cfrCtx h, unsigned short vsindex)
{
    int numRegions = h->stack.numRegions = var_getIVSRegionCountForIndex(h->cff2.varStore, vsindex);
    
	if (numRegions > CFF2_MAX_MASTERS) {
		message(h, "too many regions %d for vsindex %d", numRegions, vsindex);
		numRegions = 0;
	}
    if (!var_getIVSRegionIndices(h->cff2.varStore, vsindex, h->cff2.regionIndices, (long)numRegions)) {
        message(h, "inconsistent region indices detected in item variation store subtable %d", vsindex);
		numRegions = 0;
    }
    return numRegions;
}

/* -------------------------------- VarStore ------------------------------- */

static float AxisCoord2Real(short val)
{
    float newVal = (float)((val >> 14) + ((val & 0x3fff) / 16384.0));
    return newVal;
}

static void readVarStore(cfrCtx h)
{
    unsigned short length;
    unsigned int vstoreStart = h->region.VarStore.begin + 2;

    if (h->region.VarStore.begin == -1)
        fatal(h, cfrErrNoFDSelect);
    
    if (h->cff2.varStore) {
        var_freeItemVariationStore(&h->cb.shstm, h->cff2.varStore);
        h->cff2.varStore = 0;
    }
    
    srcSeek(h, h->region.VarStore.begin);
    length = (unsigned long)read2(h);
    h->region.VarStore.end = vstoreStart + length;
    h->cff2.varStore = var_loadItemVariationStore(&h->cb.shstm, (unsigned long)vstoreStart, length, 0);
	if (!h->cff2.varStore)
		return;
    h->cff2.regionCount = var_getIVSRegionCount(h->cff2.varStore);
	if (h->cff2.regionCount > CFF2_MAX_MASTERS)
		fatal(h, cfrErrGeometry);
    h->top.varStore = h->cff2.varStore;
    /* pre-calculate scalars for all regions for the current weight vector */
    var_calcRegionScalars(&h->cb.shstm, h->cff2.varStore, h->cff2.axisCount, h->cff2.ndv, h->cff2.scalars);
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
		if (SSCANF_S(q, " %ld def%n", &value, &n) == 1 && n != -1 &&
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
		if (SSCANF_S(q, " /Type1 def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeType1;
		else if (SSCANF_S(q, " /CID def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeCID;
		else if (SSCANF_S(q, " /TrueType def%n", &n) == 0 && n != -1)
			h->top.OrigFontType = abfOrigFontTypeTrueType;
        else if (SSCANF_S(q, " /OCF def%n", &n) == 0 && n != -1)
            h->top.OrigFontType = abfOrigFontTypeOCF;
        else if (SSCANF_S(q, " /UFO def%n", &n) == 0 && n != -1)
            h->top.OrigFontType = abfOrigFontTypeUFO;
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
	abfPrivateDict *priv = &font->Private;
    
	if (region->begin == region->end)
    {
		/* Empty dictionary (assuming Private); valid but set BlueValues to
         avoid warning */
		h->fd->flags |= SEEN_BLUE_VALUES;
		return;
    }
    
	srcSeek(h, region->begin);
    
	h->stack.cnt = 0;
    h->stack.numRegions = priv->numRegions = 0;
    priv->vsindex = 0;
    
    if (h->cff2.varStore != NULL)
    {
        h->stack.numRegions = priv->numRegions = setNumMasters(h, priv->vsindex);
    }
	while (srcTell(h) < region->end)
    {
		int byte0 = read1(h);
		switch (byte0)
        {
            case cff_vsindex:
                priv->vsindex = (unsigned short)INDEX_INT(0);
                h->stack.numRegions = priv->numRegions = setNumMasters(h, priv->vsindex);
                break;

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
                saveDeltaArray(h, ARRAY_LEN(priv->BlueValues.array),
                               &priv->BlueValues.cnt,
                               priv->BlueValues.array,
                               &priv->blendValues.BlueValues.cnt,
                               priv->blendValues.BlueValues.array);
                h->fd->flags |= SEEN_BLUE_VALUES;
                break;
            case cff_OtherBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(priv->OtherBlues.array),
                                   &priv->OtherBlues.cnt,
                                   priv->OtherBlues.array,
                                   &priv->blendValues.OtherBlues.cnt,
                                priv->blendValues.OtherBlues.array);
                else
                    message(h, "OtherBlues empty array (ignored)");
                break;
            case cff_FamilyBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(priv->FamilyBlues.array),
                                   &priv->FamilyBlues.cnt,
                                   priv->FamilyBlues.array,
                                   &priv->blendValues.FamilyBlues.cnt,
                                   priv->blendValues.FamilyBlues.array);

                else
                    message(h, "FamilyBlues empty array (ignored)");
                break;
            case cff_FamilyOtherBlues:
                if (h->stack.cnt > 0)
                    saveDeltaArray(h, ARRAY_LEN(priv->FamilyOtherBlues.array),
                                   &priv->FamilyOtherBlues.cnt,
                                   priv->FamilyOtherBlues.array,
                                   &priv->blendValues.FamilyOtherBlues.cnt,
                                   priv->blendValues.FamilyOtherBlues.array);

                else
                    message(h, "FamilyBlues empty array (ignored)");
                break;
            case cff_StdHW:
                 saveBlend(h, &priv->StdHW, &priv->blendValues.StdHW);
                break;
            case cff_StdVW:
                saveBlend(h, &priv->StdVW, &priv->blendValues.StdVW);
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
                    saveBlend(h, &priv->BlueScale, &priv->blendValues.BlueScale);
                    break;
                case cff_BlueShift:
                    saveBlend(h, &priv->BlueShift, &priv->blendValues.BlueShift);
                    break;
                case cff_BlueFuzz:
                    saveBlend(h, &priv->BlueFuzz, &priv->blendValues.BlueFuzz);
                    break;
                case cff_StemSnapH:
                    saveDeltaArray(h, ARRAY_LEN(priv->StemSnapH.array),
                                   &priv->StemSnapH.cnt,
                                   priv->StemSnapH.array,
                                   &priv->blendValues.StemSnapH.cnt,
                                   priv->blendValues.StemSnapH.array);

                    break;
                case cff_StemSnapV:
                    saveDeltaArray(h, ARRAY_LEN(priv->StemSnapV.array),
                                   &priv->StemSnapV.cnt,
                                   priv->StemSnapV.array,
                                   &priv->blendValues.StemSnapV.cnt,
                                   priv->blendValues.StemSnapV.array);

                    break;
                case cff_ForceBold:
                    CHKUFLOW(1);
                    priv->ForceBold = INDEX_INT(0);
                    break;
                case cff_LanguageGroup:
                    CHKUFLOW(1);
                    priv->LanguageGroup = INDEX_INT(0);
                    break;
                case cff_ExpansionFactor:
                    CHKUFLOW(1);
                    priv->ExpansionFactor = INDEX_REAL(0);
                    break;
                case cff_initialRandomSeed:
                    CHKUFLOW(1);
                    priv->initialRandomSeed = INDEX_REAL(0);
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
                    h->flags |= CID_FONT;
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
            case cff_VarStore:
                CHKUFLOW(1);
                h->region.VarStore.begin = h->src.origin + INDEX_INT(0);
                break;
            case cff_maxstack:
                CHKUFLOW(1);
                /* deprecated April 2017.
                   top->maxstack = (unsigned short)INDEX_INT(0); */
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
            case cff_blend:
                if (h->stack.numRegions == 0)
                {
                    /* priv->vsindex is set to 0 by default; it is otherwise only if the vsindex operator is used */
                    setNumMasters(h, priv->vsindex);
                }
                handleBlend(h);
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
            case cff_reserved26:
            case cff_reserved27:
            case cff_reserved255:
                if (h->stm.dbg == NULL)
                    fatal(h, cfrErrDICTOp);
                else
                    message(h, "Invalid DICT op %hu (ignored)", byte0);
        }				/* End: switch (byte0) */
        if (h->flags & CFR_SEEN_BLEND)
        {
            int j = 0;
            while (j < h->stack.cnt)
            {
                if (h->stack.array[j].numBlends > 0)
                {
                    memFree(h, h->stack.array[j].blend_val);
                    h->stack.array[j].numBlends = 0;
                }
                j++;
            }
            h->flags &= ~CFR_SEEN_BLEND;
        }

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
    OffSize cntSize;
    
	/* Read INDEX count */
	srcSeek(h, region->begin);
    if (h->flags & CFR_IS_CFF2)
    {
        cntSize = 4;
       cnt = readN(h, cntSize);
    }
    else
    {
        cntSize = 2;
        cnt = read2(h);
    }
    
	if (cnt == 0)
    {
		/* Empty INDEX */
		region->end = region->begin + cntSize;
		return;
    }
    
	offSize = read1(h);		/* Get offset size */
    
	/* Compute data offset */
	dataoff = region->begin + cntSize + 1 + (cnt + 1)*offSize - 1;
    
	/* Allocate offset array */
	if (dnaSetCnt(offsets, DNA_ELEM_SIZE_(*offsets), cnt) < 0)
		fatal(h, cfrErrNoMemory);
    
	/* Read charstring offsets */
	for (i = 0; i < cnt; i++)
		offsets->array[i] = dataoff + readN(h, offSize);
    
	/* Save INDEX length */
	region->end = dataoff + readN(h, offSize);
}

/* Read INDEX structure parameters and return offset of byte following INDEX.*/
static void readINDEX(cfrCtx h, ctlRegion *region, INDEX *index)
{
    OffSize cntSize;
    
	/* Read INDEX count */
	srcSeek(h, region->begin);

    if (h->flags & CFR_IS_CFF2)
    {
        cntSize = 4;
        index->count = readN(h, 4);
    }
    else
    {
        cntSize = 2;
        index->count = read2(h);
    }

    
	if (index->count == 0)
    {
		/* Empty INDEX */
		region->end = region->begin + cntSize;
		return;
    }
    
	/* Read and validate offset size */
	index->offSize = read1(h);
	if (index->offSize < 1 || index->offSize > 4)
		fatal(h, cfrErrINDEXHeader);
    
	index->offset = region->begin + cntSize + 1;	/* Get offset array base */
    
	/* Read and validate first offset */
	if (readN(h, index->offSize) != 1)
		fatal(h, cfrErrINDEXOffset);
    
	/* Set data reference */
	index->data = index->offset + (index->count + 1)*index->offSize - 1;
    
	/* Read last offset and compute INDEX length */
	srcSeek(h, index->offset + index->count*index->offSize);
	region->end = index->data + readN(h, index->offSize);
}

static void readTopDataAsIndex(cfrCtx h, ctlRegion *region, INDEX *index)
{
    srcSeek(h, region->begin);
    index->count = 1;
    index->offset = region->begin;	/* Get offset array base */
    index->offSize = 0;
    index->data = region->end;
    srcSeek(h, region->end);
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
	cfrFDInfo *fd = &h->FDArray.array[iFD];
    
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
    fd->aux.default_vsIndex = fd->fdict->Private.vsindex;
    fd->aux.varStore = h->cff2.varStore;
    fd->aux.scalars = h->cff2.scalars;
}

/* Return the offset of a standard encoded glyph of -1 if none. */
static long getStdEncGlyphOffset(void *ctx, int stdcode)
{
	cfrCtx h = (cfrCtx)ctx;
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
	cfrFDInfo *fd = h->fd = &h->FDArray.array[iFD];
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
    fd->aux.varStore            = 0;
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
	memset(h->fdicts.array, 0, sizeof(h->fdicts.array[0]) * h->index.FDArray.count);
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
	unsigned long i;
	char *p;
	ctlRegion FontName;
	long lenFontName = 0;
	long lenStrings;

    if (h->index.name.count != 1)
        fatal(h, cfrErrMultipleFont);
    
    /* Get FontName data and compute its size */
    INDEXGet(h, &h->index.name, 0, &FontName);
    lenFontName = FontName.end - FontName.begin;
    
	/* Compute string data size */
	lenStrings = (h->index.string.count == 0)? 0:
    (h->region.StringINDEX.end -
     h->index.string.data + 1 + 	/* String data bytes */
     h->index.string.count);		/* Null termination */
    
	/* Allocate buffers */
	dnaSET_CNT(h->string.offsets, h->index.string.count + 1);
	dnaSET_CNT(h->string.ptrs, h->index.string.count);
	dnaSET_CNT(h->string.buf, lenFontName + 1 +	lenStrings);
    
    p = h->string.buf.array;
    *p = '\0';
    if (h->header.major == 1)
    {
        /* Copy FontName into buffer */
        srcSeek(h, FontName.begin);
        srcRead(h, lenFontName, p);
        p += lenFontName;
        *p++ = '\0';
    }
    
    
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
	for (i = 0; i < (unsigned long)h->string.ptrs.cnt; i++)
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
	unsigned long i;
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
        if (h->flags & CFR_IS_CFF2)
        {
            if (length > 65535)
                fatal(h, cfrErrINDEXOffset);
        }
        else
        {
            if (length < 1 || length > 65535)
                fatal(h, cfrErrINDEXOffset);
        }
        
		offset = info->sup.end;
    }
}

/* -------------------------------- Charset -------------------------------- */

/* Get glyph name from format 2.0 post table. */
static char *post2GetName(cfrCtx h, SID gid)
{
    if (gid >= h->post.fmt2.glyphNameIndex.cnt)
        return NULL;	/* Out of bounds; .notdef */
    else if (h->post.format != 0x00020000)
        return h->post.fmt2.strings.array[gid];
    else
    {
        long nid = h->post.fmt2.glyphNameIndex.array[gid];
        if (nid == 0)
            return stdstrs[nid];	/* .notdef */
        else if (nid < 258)
            return applestd[nid];
        else if (nid - 258 >= h->post.fmt2.strings.cnt)
        {
            return NULL;	/* Out of bounds; .notdef */
 
        }
        else
            return h->post.fmt2.strings.array[nid - 258];
    }
}

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

/* Read 2-byte signed number. */
static short sread2(cfrCtx h)
{
    unsigned short value = (unsigned short)read1(h)<<8;
    value |= (unsigned short)read1(h);
#if SHRT_MAX == 32767
    return (short)value;
#else
    return (short)((value > 32767)? value - 65536: value);
#endif
}

/* Read 4-byte unsigned number. */
static unsigned long read4(cfrCtx h)
{
    unsigned long value = (unsigned long)read1(h)<<24;
    value |= (unsigned long)read1(h)<<16;
    value |= (unsigned long)read1(h)<<8;
    return value | (unsigned long)read1(h);
}

/* Read 4-byte signed number. */
static long sread4(cfrCtx h)
{
    unsigned long value = (unsigned long)read1(h)<<24;
    value |= (unsigned long)read1(h)<<16;
    value |= (unsigned long)read1(h)<<8;
    value |= (unsigned long)read1(h);
#if LONG_MAX == 2147483647
    return (long)value;
#else
    return (long)((value > 2417483647)? value - 4294967296: value);
#endif
}

static int invalidStreamOffset(cfrCtx h, unsigned long offset)
{
    h->src.length = 0;	/* To force re-read after this test */
    h->cb.stm.seek(&h->cb.stm, h->stm.src, offset);
    return h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf) == 0;
}

static void buildGIDNames(cfrCtx h)
{
    char *p;
    long length;
    long numGlyphs = h->glyphs.cnt;
    unsigned short i;

    dnaSET_CNT(h->post.fmt2.glyphNameIndex, numGlyphs);
    for (i = 0; i < numGlyphs; i++)
    {
        h->post.fmt2.glyphNameIndex.array[i] = i;
     }
    /* Read string data */
    length = numGlyphs*9; /* 3 for 'gid', 5 for GID, 1 for null termination. */
    dnaSET_CNT(h->post.fmt2.buf, length + 1);
    /* Build C strings array */
    dnaSET_CNT(h->post.fmt2.strings, numGlyphs);
    p = h->post.fmt2.buf.array;
    sprintf(p, ".notdef");
    length = strlen(p);
    h->post.fmt2.strings.array[0] = p;
    p += length+1;
    for (i = 1; i < numGlyphs; i++)
    {
        h->post.fmt2.strings.array[i] = p;
        sprintf(p, "gid%05d", i);
        length = strlen(p);
        p += length+1;
    }
   
    return;	/* Success */
   
}

/* Read post table. */
static void postRead(cfrCtx h)
{
    enum { POST_HEADER_SIZE = 4*7 + 2*2 };
    char *p;
    char *end;
    long i;
    long length;
    long numGlyphs;
	long strCount;
    sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('p','o','s','t'));
    if (table == NULL)
        return;	/* Optional table missing */
    
    if (invalidStreamOffset(h, table->offset + POST_HEADER_SIZE - 1))
    {
        message(h, "post: header outside stream bounds");
        return;
    }
    
    srcSeek(h, table->offset);
    
    h->post.format				= read4(h);
    h->post.italicAngle			= sread4(h);
    h->post.underlinePosition	= sread2(h);
    h->post.underlineThickness	= sread2(h);
    h->post.isFixedPitch		= read4(h);
    h->post.minMemType42		= read4(h);
    h->post.maxMemType42		= read4(h);
    h->post.minMemType1			= read4(h);
    h->post.maxMemType1			= read4(h);

    if (h->flags & CID_FONT)
        return; /* Don't read glyph names for CID fonts */
    
    if (h->post.format != 0x00020000)
    {
        buildGIDNames(h);
        return;
    }
    
    if (invalidStreamOffset(h, table->offset + table->length - 1))
    {
        message(h, "post: table truncated");
        goto parseError;
    }
    
    srcSeek(h, table->offset + POST_HEADER_SIZE);
    
    /* Parse format 2.0 data */
    numGlyphs = read2(h);
    if (numGlyphs != h->glyphs.cnt)
        message(h, "post 2.0: name index size doesn't match numGlyphs");
    
    /* Validate table length against size of glyphNameIndex */
    length = (long)(table->length - (srcTell(h) - table->offset));
    if (length < numGlyphs * 2)
    {
        message(h, "post 2.0: table truncated (table ignored)");
        goto parseError;
    }
    
    /* Read name index */
    dnaSET_CNT(h->post.fmt2.glyphNameIndex, numGlyphs);
    strCount = 0;
    for (i = 0; i < numGlyphs; i++)
    {
        unsigned short nid = read2(h);
        h->post.fmt2.glyphNameIndex.array[i] = nid;
        if (nid > 32767)
        {
            message(h, "post 2.0: invalid name id (table ignored)");
            goto parseError;
        }
        else if (nid > 257)
            strCount++;
    }
    
    /* Read string data */
    length = (long)(table->length - (srcTell(h) -  table->offset));
    dnaSET_CNT(h->post.fmt2.buf, length + 1);
    srcRead(h, length, h->post.fmt2.buf.array);
    
    /* Build C strings array */
    dnaSET_CNT(h->post.fmt2.strings, strCount);
    p = h->post.fmt2.buf.array;
    end = p + length;
    i = 0;
    for (i = 0; i < h->post.fmt2.strings.cnt; i++)
    {
        length = *(unsigned char *)p;
        *p++ = '\0';
        h->post.fmt2.strings.array[i] = p;
        p += length;
        if (p > end)
        {
            message(h, "post 2.0: invalid strings");
            goto parseError;
        }
    }
    *p = '\0';
    if (p != end)
        message(h, "post 2.0: string data didn't reach end of table");
    
    return;	/* Success */
    
parseError:
    /* We managed to read the header but the rest of the table had an error that
     prevented reading some (or all) glyph names. We set the the post format
     to a value that will allow us to use the header values but will prevent
     us from using any glyph name data which is likely missing or invalid */
    h->post.format = 0x00000001;
}

/* Read MVAR table for font wide variable metrics. */
static void MVARread(cfrCtx h)
{
	abfTopDict *top = &h->top;
    float   thickness, position;

    if (!var_lookupMVAR(&h->cb.shstm, h->cff2.mvar, h->cff2.axisCount, h->cff2.scalars, MVAR_unds_tag, &thickness)) {
        top->UnderlineThickness += thickness;
        top->UnderlinePosition -= thickness/2;
    }
    if (!var_lookupMVAR(&h->cb.shstm, h->cff2.mvar, h->cff2.axisCount, h->cff2.scalars, MVAR_undo_tag, &position)) {
        top->UnderlinePosition += position;
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

static void readCharSetFromPost(cfrCtx h)
{
    Offset offset;
    unsigned long i;
	long gid;
    char *p;
    long lenStrings = ARRAY_LEN(stdstrs);

    if (!(h->flags & CFR_IS_CFF2))
    {
        /* init string.offsets and h->string.ptrs */
        h->index.string.count = (short)h->glyphs.cnt;
        dnaSET_CNT(h->string.offsets, h->index.string.count + 1);
        dnaSET_CNT(h->string.ptrs, h->index.string.count);
        
         /* Compute string data size */
        lenStrings = (h->index.string.count == 0)? 0:
        (h->region.StringINDEX.end -
         h->index.string.data + 1 + 	/* String data bytes */
         h->index.string.count);		/* Null termination */
        
        /* Allocate buffers */
        dnaSET_CNT(h->string.offsets, h->index.string.count + 1);
        dnaSET_CNT(h->string.ptrs, h->index.string.count);
        /* Append char names after the font name already in the string buffer */
        p = &h->string.buf.array[h->string.buf.cnt];
        dnaSET_CNT(h->string.buf,  h->string.buf.cnt + lenStrings);
        
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
        for (i = 0; i < (unsigned long)h->string.ptrs.cnt; i++)
        {
            long length =
            h->string.offsets.array[i + 1] - h->string.offsets.array[i];
            srcRead(h, length, p);
            h->string.ptrs.array[i] = p;
            p += length;
            *p++ = '\0';
        }
    }

	for (gid = 0; gid < h->glyphs.cnt; gid++)
    {
        abfGlyphInfo *info = &h->glyphs.array[gid];
        info->gname.ptr = post2GetName(h, (SID)gid);
    }
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
    
    if (h->header.major == 2)
    {
        postRead(h);
        if (h->cff2.mvar)
            MVARread(h);
        if (!(h->flags & CID_FONT))
            readCharSetFromPost(h);
        return;
    }
    
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
            h->region.Charset.end = (long)srcTell(h);
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
		abfEncoding *_new = h->encfree;
		if (_new == NULL)
        /* Get node from heap */
			_new = (abfEncoding *)memNew(h, sizeof(abfEncoding));
		else
        /* Remove head node from free list */
			h->encfree = _new->next;
        
		_new->code = code;
		_new->next = enc->next;
		enc->next = _new;
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
            h->region.Encoding.end = (long)srcTell(h);
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
                    h->glyphs.array[gid++].iFD = (unsigned char)fd;
			}
		}
            break;
        default:
            fatal(h, cfrErrFDSelectFmt);
    }
	h->region.FDSelect.end = (long)srcTell(h);
}

/* ------------------------------- CFF2 glyph callbacks ------------------------------- */

static float cff2GetWidth(cff2GlyphCallbacks *cb, unsigned short gid)
{
    cfrCtx  h = (cfrCtx)cb->direct_ctx;
    var_glyphMetrics    metrics;

    if (!var_lookuphmtx(&h->cb.shstm, h->cff2.hmtx, h->cff2.axisCount, h->cff2.scalars, gid, &metrics)) {
        return metrics.width;
    }

    return .0f;
}

/* ------------------------------- make up info missing from CFF2 ------------------------------- */
typedef dnaDCL(abfString *, strPtrArray);

static void addString(cfrCtx h, strPtrArray *ptrs, abfString *abfStr, char *str)
{
    long    len = (long)strlen(str);
    long    offset = h->string.buf.cnt;

	*dnaNEXT(*ptrs) = abfStr;
	abfStr->impl = offset;
    dnaSET_CNT(h->string.buf, offset + len + 1);
    STRCPY_S(&h->string.buf.array[offset], len+1, str);

    abfStr->ptr = &h->string.buf.array[offset];	/* needs to be updated after adding all strings */
}

/* Update string pointers that might have become stale due to buffer reallocation */
static void resetStringPtrs(cfrCtx h, strPtrArray *ptrs)
{
    long	i;

    for (i = 0; i < ptrs->cnt; i++)
    {
        abfString	*abfStr = ptrs->array[i];
        abfStr->ptr = &h->string.buf.array[abfStr->impl];
    }
}

static long generateInstancePSName(cfrCtx h, char *nameBuffer, long nameBufferLen)
{
    long    nameLen;

    /* Get a named-instance name from the fvar table, if it is preferred and one is available */
    if (!(h->flags & CFR_UNUSE_VF_NAMED_INSTANCE))
        {
        nameLen = nam_getNamedInstancePSName(h->cff2.name, h->cff2.axes, &h->cb.shstm,
                                                h->cff2.UDV, h->cff2.axisCount, -1,
                                                nameBuffer, nameBufferLen);
        if (nameLen > 0)
            return nameLen;
        }

    /* Generate an instance name from the design vector */
    nameLen = nam_generateArbitraryInstancePSName(h->cff2.name, h->cff2.axes, &h->cb.shstm,
                                                h->cff2.UDV, h->cff2.axisCount,
                                                nameBuffer, nameBufferLen);
    if (nameLen > 0)
        return nameLen;

    /* If above attempts failed, generate a last-resort name. If a naming callback
       is provided, try it first. */
    if (h->cff2.lastResortInstanceNameCallback)
        {
        char    prefixBuffer[LONG_PS_NAME_LIMIT+1];
        long    prefixLen;

        prefixLen = nam_getFamilyNamePrefix(h->cff2.name, &h->cb.shstm, prefixBuffer, LONG_PS_NAME_LIMIT+1);
        if (prefixLen <= 0)
            return prefixLen;

        nameLen = (*h->cff2.lastResortInstanceNameCallback)(h->cff2.lastResortInstanceNameClientCtx, h->cff2.UDV, h->cff2.axisCount, prefixBuffer, prefixLen, nameBuffer, nameBufferLen);

        if (nameLen > 0)
            return nameLen;
        }

    /* Use our own last resort name */
    nameLen = nam_generateLastResortInstancePSName(h->cff2.name, h->cff2.axes, &h->cb.shstm,
                                                h->cff2.UDV, h->cff2.axisCount,
                                                nameBuffer, nameBufferLen);

    return nameLen;
}

static void makeupCFF2Info(cfrCtx h)
{
    abfTopDict *top = &h->top;
    sfrTable *table;
    char   buf[64];
    const size_t bufLen = sizeof(buf);
    long   lenFontName;
    char    postscriptName[LONG_PS_NAME_LIMIT+1];
    char    stringBuffer1[STRING_BUFFER_LIMIT+1];
    char    stringBuffer2[STRING_BUFFER_LIMIT+1];
    unsigned long   nameLengthLimit;
    long   lenTrademark;
    strPtrArray	strPtrs;

    dnaINIT(h->ctx.dna, strPtrs, 10, 1);

	DURING_EX(h->err.env)

		/* read font version and fontBBox from head table */
		table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('h','e','a','d'));
		if ((table != NULL) && !invalidStreamOffset(h, table->offset + 54 - 1))
		{
			unsigned long   version;
            unsigned short	unitsPerEm;

			srcSeek(h, table->offset + 4);
			version = read4(h);
			SPRINTF_S(buf, bufLen, "%ld.%ld", version>>16, (version>>12)&0xf);    /* Apple style "fixed point" version number */
			addString(h, &strPtrs, &top->version, buf);

			srcSeek(h, table->offset + 18);
            unitsPerEm = read2(h);
            if (!unitsPerEm) {
                message(h, "head: zero unitsPerEm (1000 assumed)");
                unitsPerEm = 1000;
            }

			srcSeek(h, table->offset + 36);
			top->FontBBox[0] = (float)sread2(h);
			top->FontBBox[1] = (float)sread2(h);
			top->FontBBox[2] = (float)sread2(h);
			top->FontBBox[3] = (float)sread2(h);

			/* FontMatrix has been deprecated in CFF2 font dicts
             * if unitsPerEm in the head table is non-standard and FontMatrix is unspecified,
             * synthesize one from unitsPerEm */
            if (unitsPerEm != 1000) {
                float	invUPM = 1.0f / (float)unitsPerEm;
            	long	iFD;

				for (iFD = 0; iFD < h->fdicts.cnt; iFD++) {
                	abfFontDict *fdict = &h->fdicts.array[iFD];
                   
                    if (fdict->FontMatrix.cnt == ABF_EMPTY_ARRAY) {
                    	fdict->FontMatrix.cnt = 6;

                    	fdict->FontMatrix.array[0] = invUPM;
                    	fdict->FontMatrix.array[1] = 0.0f;
                    	fdict->FontMatrix.array[2] = 0.0f;
                    	fdict->FontMatrix.array[3] = invUPM;
                    	fdict->FontMatrix.array[4] = 0.0f;
                    	fdict->FontMatrix.array[5] = 0.0f;
                    
                        h->top.sup.UnitsPerEm = unitsPerEm;
                    
                        if (h->flags & CFR_USE_MATRIX)
                        {
                            cfrFDInfo *fd = &h->FDArray.array[iFD];
                        
                            /* Prepare charstring transformation matrix */
                        	fd->aux.matrix[0] = 1.0f;
                        	fd->aux.matrix[1] = 0.0f;
                        	fd->aux.matrix[2] = 0.0f;
                        	fd->aux.matrix[3] = 1.0f;
                        	fd->aux.matrix[4] = 0.0f;
                        	fd->aux.matrix[5] = 0.0f;
                            h->fd->aux.flags |= T2C_USE_MATRIX;
                        }
                    }
                }
            }
		}

		/* read isFixedPitch, ItalicAngle, UnderlinePosition, UnderlineThickness from post table */
		table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('p','o','s','t'));
		if ((table != NULL) && !invalidStreamOffset(h, table->offset + 32 - 1))
		{
			srcSeek(h, table->offset + 4);
			top->ItalicAngle = (float)(sread4(h) / 65536.0);
			top->UnderlinePosition = (float)sread2(h);
			top->UnderlineThickness = (float)sread2(h);
			top->UnderlinePosition -= top->UnderlineThickness/2;
			top->isFixedPitch = read4(h) != 0;
		}

		/* read Notice, FullName, FamilyName, Copyright from name table */
		lenFontName = nam_getASCIIName(h->cff2.name, &h->cb.shstm, stringBuffer1, sizeof(stringBuffer1), NAME_ID_FULL, 0);
		if (lenFontName > 0)
			addString(h, &strPtrs, &h->top.FullName, stringBuffer1);
		lenFontName = nam_getASCIIName(h->cff2.name, &h->cb.shstm, stringBuffer1, sizeof(stringBuffer1), NAME_ID_FAMILY, 0);
		if (lenFontName > 0)
			addString(h, &strPtrs, &h->top.FamilyName, stringBuffer1);
		lenFontName = nam_getASCIIName(h->cff2.name, &h->cb.shstm, stringBuffer1, sizeof(stringBuffer1), NAME_ID_COPYRIGHT, 0);
		if (lenFontName > 0)
			addString(h, &strPtrs, &h->top.Copyright, stringBuffer1);
		lenTrademark = nam_getASCIIName(h->cff2.name, &h->cb.shstm, stringBuffer2, sizeof(stringBuffer2), NAME_ID_TRADEMARK, 0);
		if (lenFontName > 0 && lenTrademark > 0 && ((unsigned long)(lenFontName + lenTrademark) + 2 < sizeof(stringBuffer1)))
		{   /* concatename copyright and trademark strings */
			STRCAT_S(stringBuffer1, STRING_BUFFER_LIMIT, " ");
			STRCAT_S(stringBuffer1, STRING_BUFFER_LIMIT, stringBuffer2);
		}
		if (lenFontName > 0)
			addString(h, &strPtrs, &h->top.Notice, stringBuffer1);

		/* generate PostScript name for the instance */
		if (h->flags & CFR_SHORT_VF_NAME)
			nameLengthLimit = SHORT_PS_NAME_LIMIT;
		else
			nameLengthLimit = LONG_PS_NAME_LIMIT;

		/* generate an instance PostScript name */
		lenFontName = generateInstancePSName(h, postscriptName, nameLengthLimit + 1);

		if (lenFontName > 0) {
			addString(h, &strPtrs, &h->fdicts.array[0].FontName, postscriptName);
		}

		/* Load CIDFontName from name table and make up CID font ROS */
		if (h->flags & CID_FONT) {

			lenFontName = nam_getASCIIName(h->cff2.name, &h->cb.shstm, postscriptName, sizeof(postscriptName), NAME_ID_CIDFONTNAME, 1);
			if (lenFontName <= 0)    /* if no CID font name in the name table, put PS name instead */
				lenFontName = nam_getASCIIName(h->cff2.name, &h->cb.shstm, postscriptName, sizeof(postscriptName), NAME_ID_POSTSCRIPT, 1);

			if (lenFontName > 0)
				addString(h, &strPtrs, &h->top.cid.CIDFontName, postscriptName);
			addString(h, &strPtrs, &h->top.cid.Registry, "Adobe");
			addString(h, &strPtrs, &h->top.cid.Ordering, "Identity");
			h->top.cid.Supplement = 1;
		}

		resetStringPtrs(h, &strPtrs);
		dnaFREE(strPtrs);
	HANDLER
		dnaFREE(strPtrs);
		RERAISE;
	END_HANDLER
}

void cfrSetLastResortInstanceNameCallback(cfrCtx h, cfrlastResortInstanceNameCallback cb, void *clientCtx)
{
	h->cff2.lastResortInstanceNameClientCtx = clientCtx;
    h->cff2.lastResortInstanceNameCallback = cb;
}

/* ------------------------------- Interface ------------------------------- */

/* Report absfont error message to debug stream. */
static void report_error(abfErrCallbacks *cb, int err_code, int iFD)
{
	cfrCtx h = (cfrCtx)cb->ctx;
	if (iFD == -1)
		message(h, "%s (ignored)", abfErrStr(err_code));
	else
		message(h, "%s FD[%d] (ignored)", abfErrStr(err_code), iFD);
}

/* Begin reading new font. */
int cfrBegFont(cfrCtx h, long flags, long origin, int ttcIndex, abfTopDict **top, float *UDV)
{
	long i;
	ctlRegion region;
	short gi_flags;	/* abfGlyphInfo flags */
    
	/* Set error handler */
    DURING_EX(h->err.env)
    
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
		memset(h->fdicts.array, 0, sizeof(h->fdicts.array[0]));
		initRegion(&h->region.NameINDEX);
        initRegion(&h->region.StringINDEX);
        initRegion(&h->region.Charset);
        initRegion(&h->region.Encoding);
        initRegion(&h->region.FDSelect);
        initRegion(&h->region.VarStore);
        initRegion(&h->region.CharStringsINDEX);
        initRegion(&h->region.FDArrayINDEX);
        initFDInfo(h, 0);
        h->region.Encoding.begin = cff_ISOAdobeCharset;
        h->region.Encoding.end = -1;
        h->region.Charset.begin = cff_StandardEncoding;
        h->region.Charset.end = -1;

        /* Read header */
        h->region.Header.begin = h->src.origin;
        h->header.major = read1(h);
        h->header.minor = read1(h);
        h->header.hdrSize = read1(h);
        
        if ((h->header.major != 1) && (h->header.major != 2))
            fatal(h, cfrErrBadFont);
        h->region.Header.end = h->src.origin + h->header.hdrSize;
    
        if (h->header.major == 1)
        {
            h->header.offSize = read1(h);
            /* Read name INDEX */
            h->region.NameINDEX.begin = h->region.Header.end;

            readINDEX(h, &h->region.NameINDEX, &h->index.name);
            /* Read top INDEX */
            
            h->region.TopDICTINDEX.begin = h->region.NameINDEX.end;
            readINDEX(h, &h->region.TopDICTINDEX, &h->index.top);
        }
        else
        {
            /* We use the TopDICTINDEX region to just hold the TopDict data block. */
            h->header.offSize = (OffSize)read2(h); /* In CFF2, this is the size of the Top Dict. */
            h->region.TopDICTINDEX.begin = h->region.Header.end;
            h->region.TopDICTINDEX.end = h->region.TopDICTINDEX.begin + h->header.offSize;
            h->flags |= CFR_IS_CFF2;

            h->cb.cff2.direct_ctx = h;
            h->cb.cff2.getWidth = cff2GetWidth;

            /* Load CFF2 font tables */
            if (!(flags & CFR_SHALLOW_READ))
            {
                unsigned short  axis;

                h->cff2.axes = var_loadaxes(h->ctx.sfr, &h->cb.shstm);
                h->cff2.hmtx = var_loadhmtx(h->ctx.sfr, &h->cb.shstm);
                h->cff2.mvar = var_loadMVAR(h->ctx.sfr, &h->cb.shstm);
                h->cff2.axisCount = var_getAxisCount(h->cff2.axes);
                if (h->cff2.axisCount > CFF2_MAX_AXES)
                    fatal(h, cfrErrGeometry);

                h->cff2.UDV = UDV;

                /* normalize the variable font design vector */
                for (axis = 0; axis < h->cff2.axisCount; axis++) {
                    h->cff2.ndv[axis] = 0;
                }
                if (h->cff2.UDV != NULL) {
                    Fixed   userCoords[CFF2_MAX_AXES];

                    for (axis = 0; axis < h->cff2.axisCount; axis++) {
                        userCoords[axis] = pflttofix(&h->cff2.UDV[axis]);
                    }

                    if (var_normalizeCoords(&h->cb.shstm, h->cff2.axes, userCoords, h->cff2.ndv)) {
                        fatal(h, cfrErrGeometry);
                    }
                }

                /* name table */
                h->cff2.name = nam_loadname(h->ctx.sfr, &h->cb.shstm);
            }
         }
        
        /* Read string INDEX  */
        if (h->header.major == 1)
        {
            h->region.StringINDEX.begin = h->region.TopDICTINDEX.end;
            if (h->region.StringINDEX.begin > 0)
            {
                readINDEX(h, &h->region.StringINDEX, &h->index.string);
                /* Read strings */
                readStrings(h);
            }
            /* Read gsubrs INDEX */
            h->region.GlobalSubrINDEX.begin = h->region.StringINDEX.end;
            readSubrINDEX(h, &h->region.GlobalSubrINDEX, &h->gsubrs);
       }
        else
        {
            h->index.string.count = 0;

            /* Read gsubrs INDEX */
            h->region.GlobalSubrINDEX.begin = h->region.TopDICTINDEX.end;
            readSubrINDEX(h, &h->region.GlobalSubrINDEX, &h->gsubrs);
        }
        
        
        
        /* Read top DICT */
        if (h->header.major == 1)
        {
            h->top.maxstack = T2_MAX_OP_STACK;
            INDEXGet(h, &h->index.top, 0, &region);
            readDICT(h, &region, 1);
        }
        else
        {
            h->top.maxstack = CFF2_MAX_OP_STACK;
            /* h->index.top.begin/end already marks the span of the TopDict data. */
            readDICT(h, &h->region.TopDICTINDEX, 1);
            if (h->region.VarStore.begin > 0) {
                readVarStore(h);
        }
        
        }
        
        
        gi_flags = 0;
        if (h->flags & CID_FONT)
        {
            if (!(flags & CFR_SHALLOW_READ))
                readFDArray(h);
            if (h->header.major == 1)
                h->top.cid.CIDFontName.ptr = h->string.buf.array;
            else
                makeupCFF2Info(h);
            h->top.sup.srcFontType = abfSrcFontTypeCFFCID;
            h->top.sup.flags |= ABF_CID_FONT;
            gi_flags |= ABF_GLYPH_CID;
        }
        else
        {
            if (h->header.major == 1)
            {
                readPrivate(h, 0);
                h->fd->fdict->FontName.ptr = h->string.buf.array;
            }
            else
            {
                if (!(flags & CFR_SHALLOW_READ)) {
                    readFDArray(h);
                }
                makeupCFF2Info(h);
            }
            h->top.sup.srcFontType = abfSrcFontTypeCFFName;
        }
        
        if (!(flags & CFR_SHALLOW_READ))
            readCharStringsINDEX(h, gi_flags);
        
        /* Prepare client data */
        h->top.FDArray.cnt = h->fdicts.cnt;
        h->top.FDArray.array = h->fdicts.array;
        h->top.sup.nGlyphs = h->glyphs.cnt;
        *top = &h->top;
        
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
                if ((!(flags & CFR_NO_ENCODING)) && (h->header.major == 1))
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

    HANDLER
        return Exception.Code;
    END_HANDLER
    
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
    info.blendInfo.vsindex = aux.default_vsIndex;
    
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
	result = t2cParse(info.sup.begin, info.sup.end, &aux, 0, NULL, glyph_cb, &h->cb.mem);
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
    cff2GlyphCallbacks *cff2_cb = NULL;
    
	/* Begin glyph and mark it as seen */
	result = glyph_cb->beg(glyph_cb, info);
	info->flags |= ABF_GLYPH_SEEN;
    info->blendInfo.vsindex = aux->default_vsIndex;
    
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
    if (h->flags & CFR_IS_CFF2)
        {
        aux->flags |= T2C_IS_CFF2;
        cff2_cb = &h->cb.cff2;
        }
    if (h->flags & CFR_FLATTEN_VF)
        aux->flags |= T2C_FLATTEN_BLEND;
    
	/* Parse charstring */
    info->blendInfo.vsindex = aux->default_vsIndex;
    info->blendInfo.maxstack = CFF2_MAX_OP_STACK;
	result = t2cParse(info->sup.begin, info->sup.end, aux, gid, cff2_cb, glyph_cb, &h->cb.mem);
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
    DURING_EX(h->err.env)
	
        for (i = 0; i < h->glyphs.cnt; i++)
            readGlyph(h, (unsigned short)i, glyph_cb);
    
    HANDLER
        return Exception.Code;
    END_HANDLER
    
	return cfrSuccess;
}

/* Get glyph from font by its tag. */
int cfrGetGlyphByTag(cfrCtx h, 
					 unsigned short tag, abfGlyphCallbacks *glyph_cb)
{
	if (tag >= h->glyphs.cnt)
		return cfrErrNoGlyph;
    
	/* Set error handler */
    DURING_EX(h->err.env)
	
        readGlyph(h, tag, glyph_cb);

    HANDLER
        return Exception.Code;
    END_HANDLER
    
	return cfrSuccess;
}

/* Compare glyph names. */
static int CTL_CDECL cmpNames(const void *first, const void *second, void *ctx)
{
	cfrCtx h = (cfrCtx)ctx;
	return strcmp(h->glyphs.array[*(unsigned short *)first].gname.ptr,
				  h->glyphs.array[*(unsigned short *)second].gname.ptr);
}

/* Match glyph name. */
static int CTL_CDECL matchName(const void *key, const void *value, void *ctx)
{
	cfrCtx h = (cfrCtx)ctx;
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
	DURING_EX(h->err.env)
	
        readGlyph(h, (unsigned short)h->glyphsByName.array[index], glyph_cb);

    HANDLER
        return Exception.Code;
    END_HANDLER
    
	return cfrSuccess;
}

/* Compare cids. */
static int CTL_CDECL cmpCIDs(const void *first, const void *second, void *ctx)
{
	cfrCtx h = (cfrCtx)ctx;
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
	cfrCtx h = (cfrCtx)ctx;
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
	DURING_EX(h->err.env)
	
        readGlyph(h, gid, glyph_cb);
    
    HANDLER
        return Exception.Code;
    END_HANDLER

	return cfrSuccess;
}

/* Get standard-encoded glyph from font. */
int cfrGetGlyphByStdEnc(cfrCtx h, int stdcode, abfGlyphCallbacks *glyph_cb)
{
	unsigned short gid;
    
	if (stdcode < 0 || stdcode >= 256 || (gid = h->stdEnc2GID[stdcode]) == 0)
		return cfrErrNoGlyph;
    
	/* Set error handler */
	DURING_EX(h->err.env)
	
        readGlyph(h, gid, glyph_cb);
    
    HANDLER
        return Exception.Code;
    END_HANDLER

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

    if (h->header.major != 1) {
        var_freeaxes(&h->cb.shstm, h->cff2.axes);
        var_freehmtx(&h->cb.shstm, h->cff2.hmtx);
        var_freeMVAR(&h->cb.shstm, h->cff2.mvar);
        nam_freename(&h->cb.shstm, h->cff2.name);
    }

	return cfrSuccess;
}

/* --------------------------- Shared source stream  -------------------------- */

static void* sharedSrcMemNew(ctlSharedStmCallbacks *h, size_t size)
{
    return memNew((cfrCtx)h->direct_ctx, size);
}

static void sharedSrcMemFree(ctlSharedStmCallbacks *h, void *ptr)
{
    memFree((cfrCtx)h->direct_ctx, ptr);
}

static void sharedSrcSeek(ctlSharedStmCallbacks *h, long offset)
{
    srcSeek((cfrCtx)h->direct_ctx, offset);
}

static long sharedSrcTell(ctlSharedStmCallbacks *h)
{
    return (long)srcTell(((cfrCtx)h->direct_ctx));
}

static void sharedSrcRead(ctlSharedStmCallbacks *h, size_t count, char *ptr)
{
    srcRead((cfrCtx)h->direct_ctx, count, ptr);
}

static unsigned char sharedSrcRead1(ctlSharedStmCallbacks *h)
{
    return read1(((cfrCtx)h->direct_ctx));
}

static unsigned short sharedSrcRead2(ctlSharedStmCallbacks *h)
{
    return read2(((cfrCtx)h->direct_ctx));
}

static unsigned long sharedSrcRead4(ctlSharedStmCallbacks *h)
{
    return read4(((cfrCtx)h->direct_ctx));
}

void sharedSrcMessage(ctlSharedStmCallbacks *h, char *fmt, ...)
{
 	va_list ap;
	va_start(ap, fmt);
    vmessage((cfrCtx)h->direct_ctx, fmt, ap);
	va_end(ap);
}

static void setupSharedStream(cfrCtx h)
{
    h->cb.shstm.direct_ctx = h;
    h->cb.shstm.dna = h->ctx.dna;
    h->cb.shstm.memNew = sharedSrcMemNew;
    h->cb.shstm.memFree = sharedSrcMemFree;
    h->cb.shstm.seek = sharedSrcSeek;
    h->cb.shstm.tell = sharedSrcTell;
    h->cb.shstm.read = sharedSrcRead;
    h->cb.shstm.read1 = sharedSrcRead1;
    h->cb.shstm.read2 = sharedSrcRead2;
    h->cb.shstm.read4 = sharedSrcRead4;
    h->cb.shstm.message = sharedSrcMessage;
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
			printf("[%d]=%g ", i, INDEX_REAL(i));
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

