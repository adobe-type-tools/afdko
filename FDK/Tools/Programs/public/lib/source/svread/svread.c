/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Simple bez file parser.
 */

#include "absfont.h"
#include "dynarr.h"
#include "ctutil.h"
#include <setjmp.h>
#include "svread.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

enum
	{
	svrUnknown,
	svrNumeric,
	svrOperator,
	svrNotSet,
	};

#define kMaxToken 1024
#define kMaxName 64
typedef struct
	{
	int type;
	char val[kMaxToken];
	size_t length;
	size_t offset;	/* into buffer */
	} token;

typedef unsigned short STI;		/* String index */
#define STI_UNDEF	0xffff		/* Undefined string index */
#define ARRAY_LEN(a)	(sizeof(a)/sizeof(a[0]))

enum
	{
#define DCL_KEY(key,index) index,
#include "sv_ops.h"
#undef DCL_KEY
	kKeyCount
	};

static const char op_keys[] =
	{
#define DCL_KEY(key,index) key,
#include "sv_ops.h"
#undef DCL_KEY
	};

typedef struct _floatRect
	{
	float left;
	float bottom;
	float right;
	float top;
	} floatRect;

struct svrCtx_
	{
	abfTopDict top;
	abfFontDict fdict;
	int flags;
#define SEEN_END			1
	struct 
		{
		void *dbg;
		void *src;
		} stm;
	struct 
		{
		long offset;
		char *buf;
		size_t length;
		char *end;
		char *next;
		token tk;
		} src;
	struct
		{
		dnaDCL(char, gnames);
		} data;

	dnaDCL(char, tmp);		/* Temporary buffer */
	char *mark;				/* Buffer position marker */

	struct						
		{
		int cnt;
		int flags;
#define PARSE_INIT 			(1<<0)
#define PARSE_PATH			(1<<1)
#define PARSE_STARTHINT     (1<<2)
#define PARSE_HINT			(1<<3)
#define PARSE_ENDHINT		(1<<4)
#define PARSE_END			(1<<5)

		int hintflags;
#define SVG_MAX_OP_STACK 18

		float array[SVG_MAX_OP_STACK];
		float cx, cy;
		} stack;

	floatRect aggregatebounds;
	struct /* Metric data */
        {
        struct abfMetricsCtx_ ctx;
		abfGlyphInfo gi;
        abfGlyphCallbacks cb;
		long defaultWidth;
        } metrics;

	struct 
		{
		dnaDCL(abfGlyphInfo, index);
		dnaDCL(long, byName);	/* In glyph name order */
		dnaDCL(long, widths);	/* In index order; [SRI]->width */
		} chars;
	struct						/* String pool */
		{
		dnaDCL(long, index);	/* In index order; [SRI]->iBuf */
		dnaDCL(char, buf);		/* String buffer */
		} strings;
	struct 
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	dnaCtx dna;
	struct
		{
		jmp_buf env;
		int code;
		} err;
};

typedef abfGlyphInfo Char;		/* Character record */


static STI addString(svrCtx h, size_t length, const char *value);
static char *getString(svrCtx h, STI sti);
static void newStrings(svrCtx h);
static void freeStrings(svrCtx h);
static void addWidth(svrCtx h, STI sti, long value);
static void setWidth(svrCtx h, STI sti, long value);
static long getWidth(svrCtx h, STI sti);
static int addChar(svrCtx h, STI sti, Char **chr);

/* -------------------------- Error Support ------------------------ */

char *svrErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "svrerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		(char*)"unknown error": errstrs[err_code];
	}

/* Write message to debug stream from va_list. */
static void vmessage(svrCtx h, char *fmt, va_list ap)
	{
	char text[BUFSIZ];

	if (h->stm.dbg == NULL)
		return;	/* Debug stream not available */

	vsprintf(text, fmt, ap);
	(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
	}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(svrCtx h, char *fmt, ...)
	{
	va_list ap;
	va_start(ap, fmt);
	vmessage(h, fmt, ap);
	va_end(ap);
	}

static void CTL_CDECL fatal(svrCtx h, int err_code, char *fmt, ...)
	{
	if (fmt == NULL)
		/* Write standard error message */
		message(h, "%s", svrErrStr(err_code));
	else
		{
		/* Write font-specific error message */
		va_list ap;
		va_start(ap, fmt);
		vmessage(h, fmt, ap);
		va_end(ap);
		}
	h->err.code = err_code;
	longjmp(h->err.env, 1);
	}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(svrCtx h, size_t size)
	{
	void *ptr = h->cb.mem.manage(&h->cb.mem, NULL, size);
	if (ptr == NULL)
		fatal(h, svrErrNoMemory, NULL);
	return ptr;
	}
	
/* Free memory. */
static void memFree(svrCtx h, void *ptr)
	{
	(void)h->cb.mem.manage(&h->cb.mem, ptr, 0);
	}

/* -------------------------- Safe dynarr Callbacks ------------------------ */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	svrCtx h = (svrCtx)cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, svrErrNoMemory, NULL);
	return ptr;
	}

/* Initialize error handling dynarr context. */
static void dna_init(svrCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->dna		= dnaNew(&cb, DNA_CHECK_ARGS);
	}
/* ------------------------ Context handling ------------------------------ */
void svrFree(svrCtx h)
	{
	if (h == NULL)
		return;

	dnaFREE(h->chars.index);
	dnaFREE(h->chars.byName);
	dnaFREE(h->chars.widths);
	dnaFREE(h->tmp);
	dnaFREE(h->data.gnames);
	freeStrings(h);
	dnaFree(h->dna);

	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);

	/* Free library context */
	memFree(h, h);
	}

/* Validate client and create context */
svrCtx svrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL)
	{
	svrCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(SVR_VERSION))
		return NULL;

	/* Allocate context */
	h = (svrCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct svrCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	h->flags = 0;
	h->stm.dbg = NULL;
	h->stm.src = NULL;
	h->tmp.size = 0;
	h->mark = NULL;
	h->chars.index.size = 0;
	h->chars.byName.size = 0;
	h->chars.widths.size = 0;
	h->strings.index.cnt = 0;
	h->strings.buf.cnt = 0;
	h->dna = NULL;
	h->metrics.defaultWidth = 1000;


	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Set error handler */
	if (setjmp(h->err.env))
		goto cleanup;

	/* Initialize service library */
	dna_init(h);

	dnaINIT(h->dna, h->tmp, 100, 250);
	dnaINIT(h->dna, h->chars.index, 256, 1000);
	dnaINIT(h->dna, h->chars.byName, 256, 1000);
	dnaINIT(h->dna, h->chars.widths, 256, 1000);
	dnaINIT(h->dna, h->data.gnames, 14, 100);
	newStrings(h);
	
	/* Open debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, SVR_DBG_STREAM_ID, 0);

	return h;

 cleanup:
	/* Initialization failed */
	svrFree(h);
	return NULL;
	}

static void prepClientData(svrCtx h)
	{
	h->top.sup.nGlyphs = h->chars.index.cnt;
	if (h->stm.dbg == NULL)
		abfCheckAllDicts(NULL, &h->top);
	}

/* ---------------------- Buffer handling ---------------------------- */
static void fillbuf(svrCtx h, long offset)
	{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
	if (h->src.length == 0)
		h->flags |= SEEN_END;
	h->src.offset = offset;
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
	}

static int c(svrCtx h)
	{
	if ( h->flags & SEEN_END)
		return 0;

	/* buffer read must be able to contain a full token */
	if ( h->mark && h->mark != h->src.buf)
		{
		size_t new_offset = h->src.offset + ( h->mark - h->src.buf );
		h->cb.stm.seek(&h->cb.stm, h->stm.src, new_offset);
		fillbuf(h, new_offset);

		/* make sure we are still pointing at the beginning of token */
		h->mark = h->src.buf;
		}
	if ( h->src.next == h->src.end )
		{
		fillbuf(h, h->src.offset + h->src.length);
		}
		
	return ((h->flags & SEEN_END) == 0);
	}

static int nextbuf(svrCtx h)
{
	if ( h->flags & SEEN_END)
		return 0;
    
	/* buffer read must be able to contain a full token */
	if ( h->mark && h->mark != h->src.buf)
    {
		size_t new_offset = h->src.offset + ( h->mark - h->src.buf );
		h->cb.stm.seek(&h->cb.stm, h->stm.src, new_offset);
		fillbuf(h, new_offset);
        
		/* make sure we are still pointing at the beginning of token */
		h->mark = h->src.buf;
    }
	if ( h->src.next == h->src.end )
    {
		fillbuf(h, h->src.offset + h->src.length);
    }
    
	return ((h->flags & SEEN_END) == 0);
}

static int bufferReady(svrCtx h)
	{
	if ( h->src.next == h->src.end )
		return nextbuf(h);
	
	return 1;
	}

static token* setToken(svrCtx h)
	{
	size_t len;
	if ( h->src.buf == NULL || h->mark == NULL)
		return NULL;

	len = h->src.next - h->mark;
	if ((len+1) > kMaxToken)
		return NULL;
		
	memcpy(h->src.tk.val, h->mark, len);
	h->src.tk.val[len] = 0;
	h->src.tk.length = len;
	h->src.tk.offset = h->src.offset + (h->mark - h->src.buf);	
	h->src.tk.type = svrUnknown;

	return &h->src.tk;
	}

/* return actual tokens, ignores comments as well */
static token* getToken(svrCtx h)
	{
	char ch;
	h->mark = NULL;

	while (bufferReady(h))
		{
		ch = *h->src.next;
		if ( ch == 0 )
			{
			break;
			}
		if ( isspace(ch) || (ch == '"'))
			h->src.next++;
		else 
			break;
		}
		
	while (bufferReady(h))
		{
		if ( ch == 0 )
			{
			break;
			}
		else if (( ch == '%' ) || ( ch == '#' ))
			{
			h->src.next++;
			while ( (bufferReady(h)) && ( !(ch == '\n' || ch == '\r' || ch == '\f') ))
				{
				ch = *h->src.next;
				if ( ch == 0 )
					break;
				h->src.next++;
				}
			}
		else if ( h->mark == NULL )
			{
			h->mark = h->src.next++;
			if (!bufferReady(h))
				break;
			ch = *h->src.next;
			while ( (!isspace(ch)) &&  (!( ch == '"')))
				{
				h->src.next++;
				if (( ch == 0 ) || (!bufferReady(h)))
					break;
				ch = *h->src.next;
				}
			break;
			}
		else
			{
			break;
			}
		}
	return setToken(h);
	}

/* return actual tokens, ignores comments, preserves white space */
static token* getAttribute(svrCtx h)
	{
	char ch;
	h->mark = NULL;

	while (bufferReady(h))
		{
		ch = *h->src.next;
		if ( ch == 0 )
			{
			break;
			}
		if (ch == '"')
			h->src.next++;
		else 
			break;
		}
		
	while (bufferReady(h))
		{
		if ( ch == 0 )
			{
			break;
			}
		else if (( ch == '%' ) || ( ch == '#' ))
			{
			ch = *h->src.next++;
			if (!bufferReady(h))
				break;
			while ( !(ch == '\n' || ch == '\r' || ch == '\f') )
				{
				ch = *h->src.next++;
				if (( ch == 0 ) || (!bufferReady(h)))
					break;
				}
			}
		else if ( h->mark == NULL )
			{
			h->mark = h->src.next++;
			if (( ch == 0 ) || (!bufferReady(h)))
				break;
			ch = *h->src.next;
			while  (!( ch == '"'))
				{
				h->src.next++;
				if (( ch == 0 ) || (!bufferReady(h)))
					break;
				ch = *h->src.next;
				}
			break;
			}
		else
			{
			break;
			}
		}
	return setToken(h);
	}

static long getPathLength(svrCtx h)
	{
	char ch;
	int startOffset = h->src.offset + (h->src.next - h->src.buf);
	int endOffset;
	while (bufferReady(h))
		{
		ch = *h->src.next;
		if ( ch == 0 )
			{
			break;
			}
		if ( isspace(ch) || (ch == ',') || (ch == '"'))
			h->src.next++;
		else if (( ch == '%' ) || ( ch == '#' ))
			{
			ch = *h->src.next++;
			if (!bufferReady(h))
				break;
			while ( !(ch == '\n' || ch == '\r' || ch == '\f') )
				{
				ch = *h->src.next++;
				if (!bufferReady(h))
					break;
				}
			}
		else if (ch == '/' || ch == '"' )
			break;
		else
			ch = *h->src.next++;
		}
	endOffset = h->src.offset + (h->src.next - h->src.buf);
	return endOffset - startOffset;
	}
	

static token* setPathToken(svrCtx h, int tokenType)
	{
	size_t len;
	if ( h->src.buf == NULL || h->mark == NULL)
		return NULL;

	len = h->src.next - h->mark;
	if ((len+1) > kMaxToken)
		return NULL;
		
	memcpy(h->src.tk.val, h->mark, len);
	h->src.tk.val[len] = 0;
	h->src.tk.length = len;
	h->src.tk.offset = h->src.offset + (h->mark - h->src.buf);	
	h->src.tk.type = tokenType;
	return &h->src.tk;
	}

/* return actual tokens, ignores comments as well */
static token* getPathToken(svrCtx h, long endOffset)
{
	char ch;
	int tokenType = svrUnknown;
	h->mark = NULL;
	
	if (endOffset == 0) /* A non-marking glyph. */
		return NULL;
		
	while (bufferReady(h))
		{
		ch = *h->src.next;
		if ( ch == 0 )
			{
			return NULL;
			}
		if ( isspace(ch) || (ch == ',') || (ch == '"'))
			h->src.next++;
		else 
			break;
		}
		
	if ( (h->src.offset + (h->src.next - h->src.buf)) >=endOffset)
		return NULL;
		
	h->mark = h->src.next;
	/* If we reload the buffer while processing the token, h->mark is outdated. Make sure that we have
	enough text to be able to deal with the entire token. */
	if ((h->src.end - h->src.next) < kMaxToken)
	{
		nextbuf(h); /* refills the buffer with h->mark as the first glyph, and resets h->mark to point to the start of the buffer. */
	}

	ch = *h->src.next;
	if ( ch == 0 )
		{
		return NULL;
		}
	else
		{
		if (isdigit(ch) || (ch == '-') || (ch == '.')) /* Possible chars for start of numeric coordinate */
			{
			tokenType =  svrNumeric;
			h->src.next++; /* This is safe, since we have at least kMaxToken bytes at this point.*/
			while (1)
				{
					ch = *h->src.next;
					if ( ch == 0 )
						break;
					else if (isdigit(ch) || (ch == '.'))
						{
						h->src.next++;
						if (h->src.next >= h->src.end)
							return NULL;
						}
					else
						break;
				}
			}
		else if (isalpha(ch))
				{
				/* The non-digit, no whitespace values are assumed to be single-letter oparators */
				tokenType =  svrOperator;
				h->src.next++;
				}
		else /* Found something other than digit or alphabetic.*/
				{
				h->src.next++;
				}
		}
	return setPathToken(h, tokenType);
}
					 
					   
static void copyToken(token *src, token* dst)
	{
		*dst = *src;
		memcpy(dst->val, src->val, src->length);
		src->val[src->length] = 0;
	}

/* -------------------------- Parsing routines ------------------------ */

/* Check stack contains at least n elements. */
#define CHKUFLOW(n) \
	do if(h->stack.cnt<(n))fatal(h,svrErrStackUnderflow,"");while(0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n) \
do if(h->stack.cnt+(n)>SVG_MAX_OP_STACK)fatal(h,svrErrStackOverflow,"");while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++]=(float)(v))

static void EnsureState(svrCtx h, int flags)
	{
		if (!(h->stack.flags & flags))
			fatal(h, svrErrParse, "Invalid token");
	}
/* hints */

static void doOp_trans(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	/* There can be either 3 or 5 values on the stack. */
	
    /* Rotation is expressed in degrees */
    float rotate = (float)INDEX(0);
    float scaleX = (float)INDEX(1);
	float scaleY = (float)INDEX(2);
	float skewX;
	float skewY;
	
	switch (h->stack.cnt)
	{
		case 3:
			skewX = 0;
			skewY = 0;
			break;
		case 5:
			skewX = (float)INDEX(3);
			skewY = (float)INDEX(4);
			break;
		default:
			fatal(h, svrErrParse, "Rotation matrix needs 3 or 5 ops.");
	}
	h->stack.cnt = 0;
	glyph_cb->cubeTransform(glyph_cb, rotate, scaleX, scaleY, skewX, skewY);
	}
	
static void doOp_cmp(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	int le;
	float x0, y0;
	int numDV = h->stack.cnt -3;
	
	le = (int)INDEX(0);
	x0 = INDEX(1);
	y0 = INDEX(2);

	glyph_cb->cubeCompose(glyph_cb, le, x0, y0, numDV, &h->stack.array[3]);

	h->stack.cx = x0;
	h->stack.cy = y0;
	h->stack.cnt = 0;
	}


static void doOp_rmt(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float dy, dx;
	CHKUFLOW(2);

	dy = h->stack.cy + POP();
	dx = h->stack.cx + POP();

	glyph_cb->move(glyph_cb, dx, dy);
	h->metrics.cb.move(&h->metrics.cb, dx, dy);

	h->stack.cx = dx;
	h->stack.cy = dy;
	}
	
static void doOp_mt(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float dy, dx;
	CHKUFLOW(2);

	dy = POP();
	dx = POP();
	
	glyph_cb->move(glyph_cb, dx, dy);
	h->metrics.cb.move(&h->metrics.cb, dx, dy);

	h->stack.cx = dx;
	h->stack.cy = dy;
	}


static void doOp_rdt(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float dy, dx;
	CHKUFLOW(2);

	dy = h->stack.cy + POP();
	dx = h->stack.cx + POP();

	glyph_cb->line(glyph_cb, dx, dy);
	h->metrics.cb.line(&h->metrics.cb, dx, dy);

	h->stack.cx = dx;
	h->stack.cy = dy;
	}
static void doOp_dt(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float dy, dx;
	CHKUFLOW(2);

	dy = POP();
	dx = POP();

	glyph_cb->line(glyph_cb, dx, dy);
	h->metrics.cb.line(&h->metrics.cb, dx, dy);

	h->stack.cx = dx;
	h->stack.cy = dy;
	}

static void doOp_rct(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float y3, x3, y2, x2, y1, x1;
	float dy3, dx3, dy2, dx2, dy1, dx1;
	CHKUFLOW(6);

	y3 = POP();
	x3 = POP();
	y2 = POP();
	x2 = POP();
	y1 = POP();
	x1 = POP();

	dy1 = h->stack.cy + y1; 
	dx1 = h->stack.cx + x1;
	dy2 = dy1 + y2;
	dx2 = dx1 + x2;
	dy3 = dy2 + y3;
	dx3 = dx2 + x3;

	glyph_cb->curve(glyph_cb, 
			dx1, dy1,
			dx2, dy2,
			dx3, dy3);
	h->metrics.cb.curve(&h->metrics.cb,
			dx1, dy1,
			dx2, dy2,
			dx3, dy3);

	h->stack.cx = dx3;
	h->stack.cy = dy3;
	}
static void doOp_ct(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	float dy3, dx3, dy2, dx2, dy1, dx1;
	CHKUFLOW(6);

	dy3 = POP();
	dx3 = POP();
	dy2 = POP();
	dx2 = POP();
	dy1 = POP();
	dx1 = POP();

	glyph_cb->curve(glyph_cb, 
			dx1, dy1,
			dx2, dy2,
			dx3, dy3);
	h->metrics.cb.curve(&h->metrics.cb,
			dx1, dy1,
			dx2, dy2,
			dx3, dy3);

	h->stack.cx = dx3;
	h->stack.cy = dy3;
	}

static void doOp_ed(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	abfMetricsCtx g = &h->metrics.ctx;

	glyph_cb->end(glyph_cb);

	/* get the bounding box and compute the aggregate */
	if (h->aggregatebounds.left > g->real_mtx.left)
		h->aggregatebounds.left = g->real_mtx.left;
	if (h->aggregatebounds.bottom > g->real_mtx.bottom)
		h->aggregatebounds.bottom = g->real_mtx.bottom;
	if (h->aggregatebounds.right < g->real_mtx.right)
		h->aggregatebounds.right = g->real_mtx.right;
	if (h->aggregatebounds.top < g->real_mtx.top)
		h->aggregatebounds.top = g->real_mtx.top;
	}

static int tkncmp(token* tk, char* str)
	{
	return strncmp(tk->val, str, tk->length);
	}

static int tokenEqualStr(token* tk, char* str)
	{
	return tkncmp(tk, str) == 0;
	}

static int isUnknownAttribute(token* tk)
	{
	return tk->val[tk->length-1] == '=';
	}
	
static int matchOps(const void *l, const void *r)
	{
	int retval;
	token* tk = (token*)l;
	char * c = (char*)r;
		if ( tk->length != 1)
			return -1;
			
	retval =  strncmp( tk->val,c, 1);
	return retval;
	}

static void doOperator(svrCtx h, token* opToken, abfGlyphCallbacks *glyph_cb)
	{
	int op_cmd;
	const char *p; 

	p = (const char*)bsearch(opToken, op_keys, ARRAY_LEN(op_keys), sizeof(op_keys[0]), matchOps);

	if (p == NULL)
		return;			/* ignore unknown keys */

	op_cmd = p - op_keys;	 
	switch (op_cmd)
		{
		/* ------------ FontMatrix ------------*/
		case k_trans:
			{
			EnsureState(h, PARSE_PATH);
			doOp_trans(h, glyph_cb);
			}
			break;
		/* ------------ Compose ------------*/
		case k_cmp:
			{
			EnsureState(h, PARSE_PATH);
			doOp_cmp(h, glyph_cb);
			}
			break;
		/* ------------ Move ------------*/
		case k_rmt:
			{
			EnsureState(h, PARSE_PATH);
			doOp_rmt(h, glyph_cb);
			}
			break;
		case k_mt:
			{
			EnsureState(h, PARSE_PATH);
			doOp_mt(h, glyph_cb);
			}
			break;
			/* ------------ Line ------------*/
		case k_rdt:
			{
			EnsureState(h, PARSE_PATH);
			doOp_rdt(h, glyph_cb);
			}
			break;
		case k_dt:
			{
			EnsureState(h, PARSE_PATH);
			doOp_dt(h, glyph_cb);
			}
			break;
			/* ----------- Curve ------------- */
		case k_rct:
			{
			EnsureState(h, PARSE_PATH);
			doOp_rct(h, glyph_cb);
			}
			break;
		case k_ct:
			{
			EnsureState(h, PARSE_PATH);
			doOp_ct(h, glyph_cb);
			}
			break;
			/* ---------- Path ---------------- */
		case k_cp:
		case k_CP:
			break; /* abs font path ops don't support close-path - it is implicit in a new moveto or end glyph - so we don't call these. */
		}
	}

/* --------------------- Glyph Processing ----------------------- */
static void createNotdef(svrCtx h)
	{
	unsigned short tag = (unsigned short)h->chars.index.cnt;	
	abfGlyphInfo *chr = dnaNEXT(h->chars.index);
	chr->flags 		= 0;
	chr->tag		= tag;
	chr->gname.ptr	= ".notdef";
	chr->iFD 		= 0;
	chr->encoding.code =  ABF_GLYPH_UNENC;
	chr->encoding.next = 0;
	chr->sup.begin  = 0;
	chr->sup.end    = 0;
	}

static void updateGlyphNames(svrCtx h)
{
    int i = 0;
    while (i < h->chars.index.cnt)
    {
        abfGlyphInfo *chr = &h->chars.index.array[i++];
        chr->gname.ptr	= getString(h, (STI)chr->gname.impl);
    }
}

static int parseSVG(svrCtx h)
	{
	/* This does a first pass through the font, loading the glyph name/ID and the offset and length of the  start each glyph's path attribute
	*/
	int state = 0; /* 0 == start, 1= seen start of glyph, 2 = seen glyph name, 3 = in path, 4 in comment.  */
	int prevState = 0;
	long char_begin;
	long char_end;
	long defaultWidth = 1000;
	long glyphWidth;
	char *gname;
	unsigned long unicode = ABF_GLYPH_UNENC;
	char tempVal[kMaxName];
	char tempName[kMaxName];
	STI gnameIndex;
	token* tk;

	h->src.next = h->mark = NULL;
	h->stm.src = h->cb.stm.open(&h->cb.stm, SVR_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))
		return svrErrSrcStream;

	fillbuf(h, 0);

	//createNotdef(h);

	h->metrics.defaultWidth = defaultWidth;
	

	while (!(h->flags & SEEN_END)) 
		{
		tk = getToken(h);
		if (tk == NULL)
		{
		if (state != 0)
			{
			h->src.buf[64]=0; /* truncate buffer string */
			fatal(h,svrErrParse,  "Encountered end of buffer before end of glyph.%s.", h->src.buf);
			}
		else
            {
            updateGlyphNames(h);
			return svrSuccess;
            }
		}
		
		if (tokenEqualStr(tk, "<!--"))
		{
			prevState = state;
			state = 4;
		}
		else if (tokenEqualStr(tk, "-->")) 
			{
			if (state != 4)
				fatal(h,svrErrParse,  "Encountered end comment token while not in comment.");
			state = prevState;
			}
		else if (state == 4)
			{
			continue;
			}
		else if (tokenEqualStr(tk, "font-family="))
			{
			char *fnp;
			tk = getToken(h);
			fnp = memNew(h, tk->length+1);
			strncpy(fnp, tk->val, tk->length);
			fnp[tk->length] = 0;
			h->top.FDArray.array[0].FontName.ptr = fnp;
			h->top.FullName.ptr = fnp;
			}
		else if (tokenEqualStr(tk, "<glyph"))
			{
			if (state != 0)
				fatal(h,svrErrParse,  "Encountered start glyph token while processing another glyph.%s.", tk->val);
			state = 1;
			char_begin = 0;
			char_end = 0;
			/* Set default widht and name, to be used if these values are not supplied by the glyph attributes. */
			glyphWidth = defaultWidth;
			sprintf(tempName, "gid%05d", (unsigned short)h->chars.index.cnt);
			}
		else if (tokenEqualStr(tk, "<missing-glyph"))
			{	

			if (state != 0)
				fatal(h,svrErrParse,  "Encountered missing-glyph token while processing another glyph. %s.", tk->val);

			state = 2;
			gnameIndex = addString(h, 7, ".notdef");
			gname = getString(h, gnameIndex);
			addWidth(h, gnameIndex, defaultWidth); /* will set this to a real value later, if the value is supplied. */
			char_begin = 0;
			char_end = 0;
		}
		else if (tokenEqualStr(tk, "unicode="))
		{
			char* endPtr;
			tk = getAttribute(h);
			if (tk->length == 1)
			{
				unicode = (long)*(tk->val);
				if ((unicode >= 'A') && (unicode <= 'z'))
					sprintf(tempName, "%c", *(tk->val));
			}
			else if (tk->length < 6)
			{
				if (tokenEqualStr(tk, "&amp;"))
					{
					unicode = '&';
					sprintf(tempName, "ampersand");
					}
				else if (tokenEqualStr(tk, "&quot;"))
					{
					unicode = '"';
					sprintf(tempName, "quotedbl");
					}
				else if (tokenEqualStr(tk, "&lt;"))
					{
					unicode = '<';
					sprintf(tempName, "less");
					}
				else if (tokenEqualStr(tk, "&gt;"))
					{
					unicode = '>';
					sprintf(tempName, "greater");
					}
				else
				{
					int len = tk->length;
					if (len > kMaxName)
						len = kMaxName-1;
					strncpy(tempVal, tk->val, len);
					message(h,  "Encountered bad Unicode value: '%s'.", tempVal);
					continue;
				}
			}
			else
			{
			strncpy(tempVal, tk->val+1, tk->length-2); /* remove final ";" and initial '&' */
			tempVal[0] = '0';
			tempVal[tk->length-1] = 0;
			
			unicode = strtol(tempVal, &endPtr, 16);
			}

		}
		else if (tokenEqualStr(tk, "horiz-adv-x="))
		{
			long width;
			tk = getToken(h);
			strncpy(tempVal, tk->val, tk->length);
			tempVal[tk->length] = 0;
			width = atol(tempVal);
			
			if (state == 0) /* Thiis a font attribute; set default width. */
				defaultWidth = h->metrics.defaultWidth = width;
			else if (state == 1) /* in glyph, but have not seen name yet. Defer setting width until name has been added.*/
				glyphWidth = width;
			else if (state == 2) /* Have seen a glyph name. Set the width */
				setWidth(h, gnameIndex, width);
			else
			{
				h->src.buf[h->src.next - (h->src.buf)] = 0; /* terminate the buffer after the current token */
				fatal(h, svrErrParse, "Encountered horiz-adv-x attribute in unexpected context. state %d. Glyph GID: %d. '%s'.", state, (unsigned short)h->chars.index.cnt, h->src.buf);
			}
		}
		else if (tokenEqualStr(tk, "glyph-name="))
			{	
			if (state != 1)
				{
				h->src.buf[h->src.next - (h->src.buf)] = 0; /* terminate the buffer after the current token */
				fatal(h, svrErrParse, "Encountered start glyph-name attribute token while not in start of glyph. '%s'.", h->src.buf);
				}
			 tk = getToken(h);
			state = 2;
			gnameIndex = addString(h, tk->length, tk->val);
			gname = getString(h, gnameIndex);
			addWidth(h, gnameIndex, glyphWidth); /* will set this to a real value later, if the value is supplied. */
			}
		else if (tokenEqualStr(tk, "d="))
			{
			if (state == 1) /*No glyph name was supplied. We need to add it.*/
				{
				state = 2;
				gnameIndex = addString(h, strlen(tempName), tempName);
				gname = getString(h, gnameIndex);
				addWidth(h, gnameIndex, glyphWidth);
				}
			if (state != 2)
				{
				h->src.buf[h->src.next - (h->src.buf)] = 0; /* terminate the buffer after the current token */
				fatal(h, svrErrParse, "Encountered start path attribute token in unexpected context. '%s'.", h->src.buf);
				}
			char_begin=tk->offset+2; /* skip d= */
			char_end = char_begin + getPathLength(h);
			state = 3;
			}
		else if (isUnknownAttribute(tk))
			{
			 tk = getToken(h);
			 /* discard its value.*/
			}
		else if (tokenEqualStr(tk, "/>"))
			{
			unsigned short tag;
			abfGlyphInfo *chr;
			if (state == 0)
					continue;
					
				if (state != 3)
					{
					if (char_begin != 0) /* it is a marking glyph */
						{
						h->src.buf[h->src.next - (h->src.buf)] = 0; /* terminate the buffer after the current token */
						fatal(h, svrErrParse, "Encountered end entity token while not after path attribute. %s\n", h->src.buf);
						}
					else if (state == 1)
					{
					/* It is a non-marking glyph, and we haven't seen a name. Need to save the name. */
						state = 2;
						gnameIndex = addString(h, strlen(tempName), tempName);
						gname = getString(h, gnameIndex);
						addWidth(h, gnameIndex, glyphWidth);
					}

				}

				if (addChar(h, gnameIndex, &chr))
					{
					
					message(h, "duplicate charstring <%s> (discarded)",
							getString(h, gnameIndex));
					}
				else
					{
					tag = (unsigned short)h->chars.index.cnt;
					
					state=0;
					chr->flags 		= 0;
					chr->tag		= tag;
                    /* note that we do not store gname here; as it is not stable: it is a pointer into the h->string->buf array,
                     which moves when it gets resized.  I coudl set this when all the glyphs have been read, 
                     but it is easier kust to use the impl field.*/
					chr->gname.ptr	= NULL;
					chr->gname.impl	= gnameIndex;
					chr->iFD		= 0;
					if (unicode != ABF_GLYPH_UNENC)
						{
						chr->flags |= ABF_GLYPH_UNICODE;
						chr->encoding.code = unicode;
						chr->encoding.next = 0;
						}
					else
						{
                            // If it doesn't have a Unicode, it stays unencoded.
                            chr->encoding.code = ABF_GLYPH_UNENC;
                            chr->encoding.next = 0;
						}
					chr->sup.begin  = char_begin;
					chr->sup.end    = char_end;
					//chr->width  = width;
					state = 0;
			
					}
			} /* end if is end of glyph */
			
		} /* end while more tokens */

    updateGlyphNames(h);
	return svrSuccess;
	}


static int readGlyph(svrCtx h, unsigned short tag, abfGlyphCallbacks *glyph_cb)
	{
	int result;
	char* end = NULL;
	long val;
	float realVal;
	token op_tk;
	abfGlyphInfo* gi;
	long width;
	
	op_tk.type = svrNotSet;
	
	gi = &h->chars.index.array[tag];

	h->src.next = h->mark = NULL;
	if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src,gi->sup.begin))
		fatal(h, svrErrSrcStream, 0);
	
	h->flags &= ~SEEN_END;
	fillbuf(h, gi->sup.begin);

	result = glyph_cb->beg(glyph_cb, gi);
	gi->flags |= ABF_GLYPH_SEEN;


	/* Check result */
	switch (result)
		{
	case ABF_SKIP_RET:
		return svrSuccess;
	case ABF_QUIT_RET:
		fatal(h, svrErrParseQuit, NULL);
	case ABF_FAIL_RET:
		fatal(h, svrErrParseFail, NULL);
		}
        
    h->metrics.cb.beg(&h->metrics.cb, &h->metrics.gi);
        

	width = getWidth(h, (STI)gi->gname.impl);
	glyph_cb->width(glyph_cb, (float)width );
	if (result == ABF_WIDTH_RET)
		return svrSuccess;

	h->stack.cnt = 0;
	h->stack.cx = h->stack.cy = 0.0;
	h->stack.hintflags = 0;
	h->stack.flags = PARSE_PATH;

	/* For SVG path data, the numeric args follow the operator */
	do 
		{
		token* tk;
		if (gi->sup.end == 0) // Not a marking glyph.
			break;
			
		tk = getPathToken(h, gi->sup.end);
		
		if (tk == NULL) /* end of glyph */
		{
			break;
		}

		switch (tk->type)
			{
				case svrNumeric:
					{
					CHKOFLOW(1);
					end = tk->val+tk->length;
					val = strtol(tk->val, &end , 10); 
					realVal = (float)strtod(tk->val, &end);
					if (realVal == val)
						PUSH(val);
					else
						PUSH(realVal);
					}
					break;
				case svrOperator:
					{
					/* do the previous operator */
					if (op_tk.type == svrOperator)
						{
						/* op_tk is not set until the first path op is seen. Once the first path op is seen,
						the first non-path op marks the end of the glyph. */
						doOperator(h, &op_tk, glyph_cb);
						}
					copyToken(tk, &op_tk);
					}
					break;
				case svrUnknown:
					{
					end = tk->val+tk->length;
					val = strtol(tk->val, &end , 10); 
					fatal(h, svrErrParse, "Encountered unknown operator '%s' in path attribute of glyph '%s'.", gi->gname.ptr);
					break;
					}
			}
		}
	while (!(h->stack.flags & PARSE_END));

	h->stack.flags = PARSE_END;
	doOp_ed(h, glyph_cb);
	
	/* set the FontBBox field in the abfTopDict */

	h->top.FontBBox[0] = h->aggregatebounds.left;
	h->top.FontBBox[1] = h->aggregatebounds.bottom;
	h->top.FontBBox[2] = h->aggregatebounds.right;
	h->top.FontBBox[3] = h->aggregatebounds.top;
    h->top.sup.srcFontType = abfSrcFontTypeSVGName;

	return svrSuccess;
	}

/* --------------------------- String Management --------------------------- */

/* Initialize strings. */
static void newStrings(svrCtx h)
	{
	dnaINIT(h->dna, h->strings.index, 50, 200);
	dnaINIT(h->dna, h->strings.buf, 32000, 6000);
	}

/* Reinitilize strings for new font. */
static void initStrings(svrCtx h)
	{
	h->strings.index.cnt = 0;
	h->strings.buf.cnt = 0;
	}

/* Free strings. */
static void freeStrings(svrCtx h)
	{
	dnaFREE(h->strings.index);
	dnaFREE(h->strings.buf);
	}

/* Add string. */
/* 64-bit warning fixed by type change here HO */
/* static STI addString(svrCtx h, unsigned length, const char *value) */
static STI addString(svrCtx h, size_t length, const char *value)
	{
	STI sti = (STI)h->strings.index.cnt;

	if (length == 0)
		{
		/* A null name (/) is legal in PostScript but could lead to unexpected
		   behaviour elsewhere in the coretype libraries so it is substituted
		   for a name that is very likely to be unique in the font */
		const char subs_name[] = "_null_name_substitute_";
		value = subs_name;
		length = sizeof(subs_name) - 1;
		message(h, "null charstring name");
		}

	/* Add new string index */
	*dnaNEXT(h->strings.index) = h->strings.buf.cnt;

	/* Add null-terminated string to buffer */
	/* 64-bit warning fixed by cast here HO */
	memcpy(dnaEXTEND(h->strings.buf, (long)(length + 1)), value, length);
	h->strings.buf.array[h->strings.buf.cnt - 1] = '\0';

	return sti;
	}

/* Get string from STI. */
static char *getString(svrCtx h, STI sti)
	{
	return &h->strings.buf.array[h->strings.index.array[sti]];
	}

/* ----------------------Width management -----------------------*/
static void addWidth(svrCtx h, STI sti, long value)
{
	if (sti != h->chars.widths.cnt)
	{
		fatal(h, svrErrParse, "Width index does not match glyph name index. Glyph index %d.", sti);
	}
	*dnaNEXT(h->chars.widths) = value;

}

static long getWidth(svrCtx h, STI sti)
{
	return h->chars.widths.array[sti];
}

static void setWidth(svrCtx h, STI sti, long value)
{
	h->chars.widths.array[sti] = value;
}

/* ----------------------Char management -----------------------*/

/* Match glyph name. */
static int CTL_CDECL matchChar(const void *key, const void *value, void *ctx)
	{
    /* I use getString() rtaher than reference the glyphInfo->gname.ptr because
     gname.ptr  isn't set until after the entire font is read.
    */
	svrCtx h = ctx;
	return strcmp((char *)key, getString(h, (STI)h->chars.index.array
										 [*(long *)value].gname.impl) );
	}

/* Add char record. Return 1 if record exists else 0. Char record returned by
   "chr" parameter. */
static int addChar(svrCtx h, STI sti, Char **chr)
	{
	size_t index;
	int found = 
		ctuLookup(getString(h, sti), 
				  h->chars.byName.array, h->chars.byName.cnt, 
				  sizeof(h->chars.byName.array[0]), matchChar, &index, h);
	
	if (found)
		/* Match found; return existing record */
		*chr = &h->chars.index.array[h->chars.byName.array[index]];
	else
		{
		/* Not found; add to table and return new record */
		long *new = &dnaGROW(h->chars.byName, h->chars.byName.cnt)[index];
		
		/* Make and fill hole */	
		memmove(new + 1, new, (h->chars.byName.cnt++ - index) * 
				sizeof(h->chars.byName.array[0]));
		*new = h->chars.index.cnt;

		*chr = dnaNEXT(h->chars.index);
		}

	return found;
	}

/* Find char by name. NULL if not found else char record. */
static Char *findChar(svrCtx h, STI sti)
	{
	size_t index;
	if (ctuLookup(getString(h, sti),
				  h->chars.byName.array, h->chars.byName.cnt, 
				  sizeof(h->chars.byName.array[0]), matchChar, &index, h))
		return &h->chars.index.array[h->chars.byName.array[index]];
	else
		return NULL;
	}


/* ---------------------- Public API -----------------------*/

/* Parse files */
int svrBegFont(svrCtx h, long flags, abfTopDict **top)
	{
	int result;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;

	/* Initialize */
	abfInitTopDict(&h->top);
	abfInitFontDict(&h->fdict);

	h->top.FDArray.cnt = 1;
	h->top.FDArray.array = &h->fdict;

	/* init glyph data structures used */
	h->chars.index.cnt = 0;
	h->data.gnames.cnt = 0;	

	h->aggregatebounds.left = 0.0;
	h->aggregatebounds.bottom = 0.0;
	h->aggregatebounds.right = 0.0;
	h->aggregatebounds.top = 0.0;

	h->metrics.cb = abfGlyphMetricsCallbacks;
	h->metrics.cb.direct_ctx = &h->metrics.ctx;
	h->metrics.ctx.flags = 0x0;

	result = parseSVG(h);
	if (result)
		fatal(h,result, NULL);

	prepClientData(h);
	*top = &h->top;

	return result;
	}

int svrEndFont(svrCtx h)
	{
	if ( h->stm.src )
		h->cb.stm.close(&h->cb.stm, h->stm.src);
	memFree(h,  h->top.FullName.ptr);
	return svrSuccess;
	}

int svrIterateGlyphs(svrCtx h, abfGlyphCallbacks *glyph_cb)
	{
	unsigned short i;
	int res;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;

	for (i = 0; i < h->chars.index.cnt; i++)
		{
		res = readGlyph(h, i, glyph_cb);
		if (res != svrSuccess)
			return res;
		}

	return svrSuccess;
	}

int svrGetGlyphByTag(svrCtx h, unsigned short tag, abfGlyphCallbacks *glyph_cb)
	{
	int res =  svrSuccess;

	if (tag >= h->chars.index.cnt)
		return svrErrNoGlyph;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	res = readGlyph(h, tag, glyph_cb);

	return res;
	}	

/* Match glyph name after font fully parsed. */
static int CTL_CDECL postMatchChar(const void *key, const void *value, 
								   void *ctx)
	{
	svrCtx h = ctx;
	return strcmp((char *)key, (char*)h->chars.index.array[*(long *)value].gname.ptr);
	}

int svrGetGlyphByName(svrCtx h, char *gname, abfGlyphCallbacks *glyph_cb)
	{
	size_t index;
	int result;

	if (!ctuLookup(gname, h->chars.byName.array, h->chars.byName.cnt, 
				   sizeof(h->chars.byName.array[0]), postMatchChar, &index, h))
		return svrErrNoGlyph;
	
	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;

	result = readGlyph(h,  (unsigned short)h->chars.byName.array[index], glyph_cb);

	return result;
	}	

int svrResetGlyphs(svrCtx h)
	{ 
	long i;
	
	for (i = 0; i < h->chars.index.cnt; i++)
		h->chars.index.array[i].flags &= ~ABF_GLYPH_SEEN;

	return svrSuccess;
	}

void svrGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<SVR_LIB_ID)
		return;	/* Already enumerated */

	abfGetVersion(cb);
	dnaGetVersion(cb);

	cb->getversion(cb, SVR_VERSION, "svread");

	cb->called |= 1<<SVR_LIB_ID;
	}
