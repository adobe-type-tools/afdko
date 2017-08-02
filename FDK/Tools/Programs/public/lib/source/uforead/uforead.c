/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Simple ufo file parser.
 
 If a glif file exists in the processed glyph layer, I trust that the data is up to date, and use it, 
 else I use the glif from the default layer. I read both the hint and drawing operator list before
 calling any call backs, and I need to have both in hand to do so.
 
  - parse points into a list of operators
  - parse ADobe t1 hint data, if any, into a list of hint mask operators
  - play back the list of operators, calling the abf call backs.
 
 Hint processing. 
 The hint data is stored as an separate XML element from the outline data, This is so that outline editors do not need
 to understand this info, and it can be private data.
 
 Read the hint data into list of hint mask pointers. The hint mask pointer list is expanded to include a pointer for each point index,
 up to the highest point referenced in any hintmask. Hint mask pointers not referenced by a hint mask are set to null.
 The list of hint mask pointers is passed into the point processing function, which checks to see if there is a non-null
 hintmask pointer for the current point. If so, the function  plays back the current hint list through the call backs.
 
 The flex operator is also kept in the hint data.
 
 */

#include "absfont.h"
#include "dynarr.h"
#include "ctutil.h"
#include "supportexcept.h"
#include "txops.h"
#include "uforead.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

enum
{
	ufoUnknown,
	ufoNumeric,
	ufoOperator,
	ufoNotSet,
};

#define kMaxToken 1024
#define kMaxName 64
const char * t1HintKey = "com.adobe.type.autohint";
const char * t1HintKeyV1 = "com.adobe.type.autohint";
const char * t1HintKeyV2 = "com.adobe.type.autohint.v2";

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
#ifdef _WIN32
#define round round_int
#endif


enum
{
#define DCL_KEY(key,index) index,
#include "ufo_ops.h"
#undef DCL_KEY
	kKeyCount
};

static const char op_keys[] =
{
#define DCL_KEY(key,index) key,
#include "ufo_ops.h"
#undef DCL_KEY
};

typedef struct
{
    float mtx[6];	/* Float matrix */
    int isDefault;
    int isOffsetOnly;
} Transform;

typedef struct _floatRect
{
	float left;
	float bottom;
	float right;
	float top;
} floatRect;

typedef struct
{
    char* glyphName;
    char* glifFileName;
    char* glifFilePath;
    long int glyphOrder; // used to sort the glypfRecs by glyph order from lib.plist.
} GLIF_Rec;

typedef struct 
{
    int order;
    char* glyphName;
} GlIFOrderRec;

typedef struct
{
    int opType;
    float coords[6];	/* Float matrix */
    char* pointName;
} OpRec;

enum
{
	movetoType,
	linetoType,
	curvetoType,
	closepathType,
} OpType;

enum
{
	outlineStart,
	outlineInContour,
	outlineInPoint,
	outlineInComponent,
	outlineInComment,
	outlineInLib,
	outlineInLibDict,
	outlineInID,
	outlineInHintData,
	outlineInHintSetList,
    outlineInFlexList,
	outlineInHintSet,
	outlineInHintSetName,
	outlineInHintSetStemList,
	outlineInStemHint,
    otherDictKey,
    otherInElement,
    otherInTag,
} outlineState;

typedef struct {
    float edge;
    float width;
    int flags;
} StemHint;
#define IS_VERT_STEM 1

typedef struct {
    dnaDCL(StemHint, maskStems);
    char* pointName;
} HintMask;

struct ufoCtx_
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
		dnaDCL(GLIF_Rec, glifRecs);
		dnaDCL(GlIFOrderRec, glifOrder);
		dnaDCL(OpRec, opList);
    } data;
	struct
    {
        StemHint stems[T2_MAX_STEMS];
		dnaDCL(HintMask, hintMasks);
		dnaDCL(char*, flexOpList);
        char * pointName; /* used save the hint refernce from the first point of a curve, since it has to be used in trh last point of the curve. */
    } hints;
    
    dnaDCL(char*, valueArray);	/* used when parsing <array> elements */
    char *parseKeyName; /* used to keep track of current top element name. */
 	dnaDCL(char, tmp);		/* Temporary buffer */
	char *mark;				/* Buffer position marker */
    char *altLayerDir;
    char *defaultLayerDir;
	struct
    {
		int cnt;
		unsigned long flags;
#define PARSE_INIT 			(1<<0)
#define PARSE_PATH			(1<<1)
#define PARSE_STARTHINT     (1<<2)
#define PARSE_HINT			(1<<3)
#define PARSE_ENDHINT		(1<<4)
#define PARSE_SEEN_MOVETO   (1<<6) // have seen an explicit a "move". IF there is no subsequent marking operator, we skip the operation.
#define PARSE_END			(1<<7)
        
		int hintflags;
#define UFO_MAX_OP_STACK 18
        
		float array[UFO_MAX_OP_STACK];
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
		_Exc_Buf env;
		int code;
    } err;
};

typedef struct
{
    long cnt;               /* ABF_EMPTY_ARRAY */
    float array[14];
} BluesArray;

typedef abfGlyphInfo Char;		/* Character record */


static STI addString(ufoCtx h, size_t length, const char *value);
static char *getString(ufoCtx h, STI sti);
static void newStrings(ufoCtx h);
static void freeStrings(ufoCtx h);
static void addWidth(ufoCtx h, STI sti, long value);
static void setWidth(ufoCtx h, STI sti, long value);
static long getWidth(ufoCtx h, STI sti);
static int addChar(ufoCtx h, STI sti, Char **chr);
static int CTL_CDECL postMatchChar(const void *key, const void *value,
								   void *ctx);

/* -------------------------- Error Support ------------------------ */

char *ufoErrStr(int err_code)
{
	static char *errstrs[] =
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "ufoerr.h"
    };
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
    (char*)"unknown error": errstrs[err_code];
}

/* Write message to debug stream from va_list. */
static void vmessage(ufoCtx h, char *fmt, va_list ap)
{
	char text[BUFSIZ];
    
	if (h->stm.dbg == NULL)
		return;	/* Debug stream not available */
    
	vsprintf(text, fmt, ap);
	(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(ufoCtx h, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vmessage(h, fmt, ap);
	va_end(ap);
}

static void CTL_CDECL fatal(ufoCtx h, int err_code, char *fmt, ...)
{
	if (fmt == NULL)
    /* Write standard error message */
		message(h, "%s", ufoErrStr(err_code));
	else
    {
		/* Write font-specific error message */
		va_list ap;
		va_start(ap, fmt);
		vmessage(h, fmt, ap);
		va_end(ap);
    }
	h->err.code = err_code;
	RAISE(&h->err.env, err_code, NULL);
}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(ufoCtx h, size_t size)
{
	void *ptr = h->cb.mem.manage(&h->cb.mem, NULL, size);
	if (ptr == NULL)
		fatal(h, ufoErrNoMemory, NULL);
	return ptr;
}

/* Free memory. */
static void memFree(ufoCtx h, void *ptr)
{
	(void)h->cb.mem.manage(&h->cb.mem, ptr, 0);
}

/* -------------------------- Safe dynarr Callbacks ------------------------ */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
{
	ufoCtx h = (ufoCtx)cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, ufoErrNoMemory, NULL);
	return ptr;
}

/* Initialize error handling dynarr context. */
static void dna_init(ufoCtx h)
{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->dna		= dnaNew(&cb, DNA_CHECK_ARGS);
}
/* ------------------------ Context handling ------------------------------ */

static void initHintMask(void *ctx, long cnt, HintMask *base)
{
    // cnt is number of new elements allocated
    // base is ptr to the first new element.
    ufoCtx h = (ufoCtx)ctx;
    int i = 0;
    while (i < cnt)
    {
        HintMask* curHintMask = base + i++;
        dnaINIT(h->dna, curHintMask->maskStems, 20, 20);
        curHintMask->pointName = NULL;
    }
}


void ufoFree(ufoCtx h)
{
	if (h == NULL)
		return;
    
    dnaFREE(h->valueArray);
     {
        int i = 0;
        while (i < h->hints.hintMasks.size)
        {
            HintMask* hintMask =  &h->hints.hintMasks.array[i++];
            if (hintMask->maskStems.cnt > 0)
                dnaFREE(hintMask->maskStems);
        }
    }
    dnaFREE(h->hints.hintMasks);
    dnaFREE(h->hints.flexOpList);
    dnaFREE(h->chars.index);
	dnaFREE(h->chars.byName);
	dnaFREE(h->chars.widths);
	dnaFREE(h->tmp);
	dnaFREE(h->data.glifRecs);
	dnaFREE(h->data.glifOrder);
	dnaFREE(h->data.opList);
	freeStrings(h);
	dnaFree(h->dna);
    
	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);
    
	/* Free library context */
	memFree(h, h);
}

/* Validate client and create context */
ufoCtx ufoNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL)
{
	ufoCtx h;
    
	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(UFO_VERSION))
		return NULL;
    
	/* Allocate context */
	h = (ufoCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct ufoCtx_));
	if (h == NULL)
		return NULL;

    /* Safety initialization */
    memset(h, 0, sizeof(*h));
    
	h->metrics.defaultWidth = 1000;
 	h->altLayerDir = "glyphs.com.adobe.type.processedGlyphs";
 	h->defaultLayerDir = "glyphs";
    
    
	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;
    
	/* Set error handler */
  DURING_EX(h->err.env)
  
    /* Initialize service library */
    dna_init(h);
      
      dnaINIT(h->dna, h->valueArray, 256, 50);
    dnaINIT(h->dna, h->tmp, 100, 250);
    dnaINIT(h->dna, h->chars.index, 256, 1000);
    dnaINIT(h->dna, h->chars.byName, 256, 1000);
    dnaINIT(h->dna, h->chars.widths, 256, 1000);
    dnaINIT(h->dna, h->data.glifRecs, 14, 100);
    dnaINIT(h->dna, h->data.glifOrder, 14, 100);
    dnaINIT(h->dna, h->data.opList, 50, 50);
    dnaINIT(h->dna, h->hints.hintMasks, 10, 10);
    dnaINIT(h->dna, h->hints.flexOpList, 10, 10);
      h->hints.hintMasks.func = initHintMask;
      
    newStrings(h);
    
    /* Open debug stream */
    h->stm.dbg = h->cb.stm.open(&h->cb.stm, UFO_DBG_STREAM_ID, 0);

	HANDLER
    /* Initialization failed */
    ufoFree(h);
    h = NULL;
  END_HANDLER

	return h;
}

static void prepClientData(ufoCtx h)
{
	h->top.sup.nGlyphs = h->chars.index.cnt;
	if (h->stm.dbg == NULL)
		abfCheckAllDicts(NULL, &h->top);
}

/* ---------------------- Buffer handling ---------------------------- */
static void fillbuf(ufoCtx h, long offset)
{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
	if (h->src.length == 0)
		h->flags |= SEEN_END;
	h->src.offset = offset;
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
}

static int nextbuf(ufoCtx h)
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

static int bufferReady(ufoCtx h)
{
	if ( h->src.next == h->src.end )
		return nextbuf(h);
	
	return 1;
}

static char* getBufferContextPtr(ufoCtx h)
{
    // Get pointer into buffer showing current processing context. Used for error messages
    char* p = h->src.next - 20;
    if (p < h->src.buf)
        p =  h->src.buf;
    if ((p +64) >= h->src.end)
        *(h->src.end-1) = 0;
    else
        *(p+64) = 0;
    if (strlen(p) > 128)
        p[128] = 0;
    return p;
    
}

static token* setToken(ufoCtx h)
{
	size_t len;
	if ( h->src.buf == NULL || h->mark == NULL)
		return NULL;
    
	len = h->src.next - h->mark;
	if ((len+1) > kMaxToken)
		return NULL;
    
	if (len >0)
        memcpy(h->src.tk.val, h->mark, len);
	h->src.tk.val[len] = 0;
	h->src.tk.length = len;
	h->src.tk.offset = h->src.offset + (h->mark - h->src.buf);
	h->src.tk.type = ufoUnknown;
    
	return &h->src.tk;
}

/* Tokens are separated by whitespace and '"'
 '<' and '>' also serve to mark beginning and end of tokens.
 */
static token* getToken(ufoCtx h, int state)
{
	char ch;
    token* tk = NULL;
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
		else if ( h->mark == NULL )
        {
			h->mark = h->src.next++;
			if (( ch == 0 ) || (!bufferReady(h)))
				break;
			ch = *h->src.next;
            if ((ch == '<') && (h->mark != h->src.next))
                break;
			while ( (!isspace(ch)) &&  ( ch != '"'))
            {
				h->src.next++;
				if (( ch == 0 )  || ( ch == '>' )|| (!bufferReady(h)))
					break;
				ch = *h->src.next;
                if ((ch == '<') && (h->mark != h->src.next))
                    break;
            }
			break;
        }
		else
        {
			break;
        }
    }
    
     // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark ) && (isspace(*(h->src.next-1))))
        h->src.next--;
    
    tk = setToken(h);
    if (tk == NULL)
    {
        if (state != 0)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.%s.", getBufferContextPtr(h));
        }
    }
    
    return tk;
}

/* return actual tokens, ignores comments, preserves white space */
static token* getAttribute(ufoCtx h, int state)
{
	char ch;
    token* tk = NULL;
    int lastWasQuote = 0;
	h->mark = NULL;
    
	while (bufferReady(h))
    {
		ch = *h->src.next;
		if ( ch == 0 )
        {
			break;
        }
		if ( isspace(ch))
		{
            lastWasQuote = 0;
            h->src.next++;
        }
		else if (ch == '"')
		{
            if (lastWasQuote)
            {
                ch = 0; /* we have an empty attribute */
                h->mark = h->src.next;
                break;
            }
            else
            {
                h->src.next++;
                lastWasQuote = 1;
            }
        }
		else
			break;
    }
    
	while (bufferReady(h))
    {
		if ( ch == 0 )
        {
			break;
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
    // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark ) && (isspace(*(h->src.next-1))))
        h->src.next--;
    
    tk = setToken(h);
    if (tk == NULL)
    {
        if (state != 0)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.%s.", getBufferContextPtr(h));
        }
    }
    
    return tk;
}

/* return value between begin/end element, skipping white space before and after. */
static token* getElementValue(ufoCtx h, int state)
{
	char ch;
    token* tk = NULL;
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
		else if ( h->mark == NULL )
        {
			h->mark = h->src.next++;
			if (( ch == 0 ) || (!bufferReady(h)))
				break;
			ch = *h->src.next;
			while (ch != '<')
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
    // Back up and remove any final whitespace.
    while ((h->mark != NULL) && (h->src.next != h->mark ) && (isspace(*(h->src.next-1))))
        h->src.next--;
    
    tk = setToken(h);
    if (tk == NULL)
    {
        if (state != 0)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.%s.",getBufferContextPtr(h));
        }
    }
    
    return tk;
}

static long getPathLength(ufoCtx h)
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


static token* setPathToken(ufoCtx h, int tokenType)
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
static token* getPathToken(ufoCtx h, long endOffset)
{
	char ch;
	int tokenType = ufoUnknown;
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
			tokenType =  ufoNumeric;
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
				tokenType =  ufoOperator;
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

#if _WIN32
static int round_int( double r )
{
    return (int)((r > 0.0) ? (r + 0.5) : (r - 0.5));
}
#endif

/* Construct matrix from args. */
static void setTransformMtx(Transform* transform,
					float a, float b, float c, float d, float tx, float ty, int isDefault, int isOffsetOnly)
{
    float* val = transform->mtx;
	val[0] = a;
	val[1] = b;
	val[2] = c;
	val[3] = d;
	val[4] = tx;
	val[5] = ty;
    transform->isDefault = isDefault;
    transform->isOffsetOnly = isOffsetOnly;
}

/* Multiple matices a and b giving result. */
static void matMult(float *result, float * a, float * b)
{
	result[0] = a[0]*b[0] + a[1]*b[2];
	result[1] = a[0]*b[1] + a[1]*b[3];
	result[2] = a[2]*b[0] + a[3]*b[2];
	result[3] = a[2]*b[1] + a[3]*b[3];
	result[4] = a[4]*b[0] + a[5]*b[2] + b[4];
	result[5] = a[4]*b[1] + a[5]*b[3] + b[5];
}

static void setTransformValue(Transform* transform, float val, int valIndex)
{
    transform->mtx[valIndex] = val;
    if ((valIndex == 0) || (valIndex == 3))
    {
        if (valIndex != 1.0)
        {
            transform->isDefault = 0;
            transform->isOffsetOnly = 0;
        }
    }
    else if (val != 0)
    {
        transform->isDefault = 0;
        if ((valIndex == 1) || (valIndex == 2))
            transform->isOffsetOnly = 0;
    }
    
}

/* -------------------------- Parsing routines ------------------------ */

/* Check stack contains at least n elements. */
#define CHKUFLOW(n) \
do if(h->stack.cnt<(n))fatal(h,ufoErrStackUnderflow,"");while(0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n) \
do if(h->stack.cnt+(n)>UFO_MAX_OP_STACK)fatal(h,ufoErrStackOverflow,"");while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++]=(float)(v))

static void EnsureState(ufoCtx h, int flags)
{
    if (!(h->stack.flags & flags))
        fatal(h, ufoErrParse, "Invalid token");
}
/* hints */

static void doOp_mt(ufoCtx h, abfGlyphCallbacks *glyph_cb, char * pointName)
{
	float dy, dx;
    OpRec* opRec;
	CHKUFLOW(2);
	dy = POP();
	dx = POP();
	
    opRec = dnaNEXT(h->data.opList);
    opRec->opType = movetoType;
    opRec->coords[0] = dx;
    opRec->coords[1] = dy;
    opRec->pointName = pointName; /* pointName may or may not be NULL */
    h->stack.flags |= PARSE_SEEN_MOVETO;
    //printf("moveto %f %f. opIndex: %ld \n", dx, dy, h->data.opList.cnt-1);
	h->metrics.cb.move(&h->metrics.cb, dx, dy);
    
}


static void doOp_dt(ufoCtx h, abfGlyphCallbacks *glyph_cb, char * pointName)
{
	float dy, dx;
    OpRec* opRec;
	CHKUFLOW(2);
    
	dy = POP();
	dx = POP();
    
    opRec = dnaNEXT(h->data.opList);
    opRec->opType = linetoType;
    opRec->coords[0] = dx;
    opRec->coords[1] = dy;
    opRec->pointName = pointName; /* pointName may or may not be NULL */
    //printf("lineto %f %f. opIndex: %ld \n", dx, dy, h->data.opList.cnt-1);
    if (!(h->stack.flags & PARSE_SEEN_MOVETO))
    {
        h->metrics.cb.move(&h->metrics.cb, dx, dy);
        h->stack.flags |= PARSE_SEEN_MOVETO;
    }
    else
    {
        h->metrics.cb.line(&h->metrics.cb, dx, dy);
    }
    
}

static void doOp_ct(ufoCtx h, abfGlyphCallbacks *glyph_cb, char * pointName)
{
	float dy3, dx3, dy2, dx2, dy1, dx1;
    OpRec* opRec;

    opRec = dnaNEXT(h->data.opList);
    opRec->opType = curvetoType;
    opRec->pointName = pointName; /* pointName may or may not be NULL */

    if (!(h->stack.flags & PARSE_SEEN_MOVETO))
    {
        /* If this is the first operator, then only the final curve point has been seen; the first two points are at the end of the contour. */
        CHKUFLOW(2);
        dy3 = POP();
        dx3 = POP();

        opRec->coords[0] = dx3;
        opRec->coords[1] = dy3;

        h->metrics.cb.move(&h->metrics.cb, dx3, dy3);
        h->stack.flags |= PARSE_SEEN_MOVETO;
    }
    else
    {
        CHKUFLOW(6);
        
        dy3 = POP();
        dx3 = POP();
        dy2 = POP();
        dx2 = POP();
        dy1 = POP();
        dx1 = POP();
        
        opRec->coords[0] = dx1;
        opRec->coords[1] = dy1;
        opRec->coords[2] = dx2;
        opRec->coords[3] = dy2;
        opRec->coords[4] = dx3;
        opRec->coords[5] = dy3;
        //printf("curveo %f %f. opIndex: %ld \n", dx3, dy3, h->data.opList.cnt-1);
        
        h->metrics.cb.curve(&h->metrics.cb,
                            dx1, dy1,
                            dx2, dy2,
                            dx3, dy3);
    }
    
}

static void doOp_ed(ufoCtx h, abfGlyphCallbacks *glyph_cb)
{
	abfMetricsCtx g = &h->metrics.ctx;
    
	/* get the bounding box and compute the aggregate */
	if (h->aggregatebounds.left > g->real_mtx.left)
		h->aggregatebounds.left = g->real_mtx.left;
	if (h->aggregatebounds.bottom > g->real_mtx.bottom)
		h->aggregatebounds.bottom = g->real_mtx.bottom;
	if (h->aggregatebounds.right < g->real_mtx.right)
		h->aggregatebounds.right = g->real_mtx.right;
	if (h->aggregatebounds.top < g->real_mtx.top)
		h->aggregatebounds.top = g->real_mtx.top;
    
    glyph_cb->end(glyph_cb); /* I call glyph_cb->end directly, unlike the path operators, as the opList has already been played back. */
}

static int tkncmp(token* tk, char* str)
{
    size_t len = strlen(str);
    int retVal;
    if (len != tk->length)
        retVal =  1;
    else
        retVal = strncmp(tk->val, str, tk->length);   
	return retVal;
}

static int tokenEqualStr(token* tk, char* str)
{
	return tkncmp(tk, str) == 0;
}

static int tokenEqualStrN(token* tk, char* str, int n)
{
	return 0 == strncmp(tk->val, str, n);
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


/* --------------------- Glyph Processing ----------------------- */
static char* copyStr(ufoCtx h, char* oldString)
{
    char *value = NULL;
    value = memNew(h, strlen(oldString)+1);
    strcpy(value, oldString);
    return value;
}

static char* getKeyValue(ufoCtx h, char* endName, int state)
{
    char *value = NULL;
    token* tk;
    
    tk =  getElementValue(h, state);
    if (!tokenEqualStr(tk, endName))
    {
        value = memNew(h, tk->length+1);
        strncpy(value, tk->val, tk->length);
        value[tk->length] = 0;
        // get end element to clear it
        tk = getToken(h, state);
        if (!tokenEqualStr(tk, endName))
            fatal(h,ufoErrParse,  "Encountered element '%s' when reading value for element '%s'.", tk->val, endName);
    }
    return value;
    
}

static void setBluesArrayValue(ufoCtx h, BluesArray* bluesArray, int numElements)
{
    int i = 0;
    bluesArray->cnt = h->valueArray.cnt;
    while ((i < h->valueArray.cnt) && (i < numElements))
    {
        bluesArray->array[i] = (float)atof(h->valueArray.array[i]);
        memFree(h, h->valueArray.array[i]);
        i++;
    }
}


static void setFontDictKey(ufoCtx h, char * keyValue)
{
    char* keyName = h->parseKeyName;
    abfTopDict* top = &h->top;
    abfFontDict* fd0 = &h->top.FDArray.array[0];
    abfPrivateDict* pd = &fd0->Private;
    BluesArray* bluesArray;
    
    /* If we do not use a keyValue, we need to dispose of it
     */
 	if (!strcmp(keyName, "copyright"))
    {
        top->Copyright.ptr = keyValue;
    }
    else if (!strcmp(keyName, "trademark"))
    {
        char * copySymbol;
		top->Notice.ptr = keyValue;
        copySymbol = strstr(keyValue, "\u00A9");
        if (copySymbol != NULL)
        {
            char* cpy = "Copyright";
            char* newString = memNew(h, strlen(cpy) + strlen(keyValue) + 2);
            copySymbol = '\0';
            sprintf(newString, "%s%s%s", keyValue, "Copyright", copySymbol + 5);
            top->Notice.ptr = newString;
        }
    }
    else if (!strcmp(keyName, "versionMajor"))
    {
        if (top->version.ptr == NULL)
            top->version.ptr = keyValue;
        else
        {
            char* newString = memNew(h, strlen(top->version.ptr) + strlen(keyValue) + 2);
            sprintf(newString, "%s.%s", keyValue, top->version.ptr);
            memFree(h, top->version.ptr);
            top->version.ptr = newString;
            memFree(h, keyValue);
        }
    }
    else if (!strcmp(keyName, "versionMinor"))
    {
        if (top->version.ptr == NULL)
            top->version.ptr = keyValue;
        else
        {
            char* newString = memNew(h, strlen(top->version.ptr) + strlen(keyValue) + 2);
            sprintf(newString,  "%s.%s", top->version.ptr, keyValue);
            memFree(h, top->version.ptr);
            top->version.ptr = newString;
            memFree(h, keyValue);
        }
    }
    else if (!strcmp(keyName, "postscriptFontName"))
    {
        fd0->FontName.ptr = keyValue;
    }
    else if (!strcmp(keyName, "openTypeNamePreferredFamilyName"))
    {
        top->FamilyName.ptr = keyValue;
    }
    else if (!strcmp(keyName, "familyName"))
    {
        if ( top->FamilyName.ptr  == NULL) // we don't re-set this if it was set by "openTypeNamePreferredFamilyName"
            top->FamilyName.ptr = keyValue;
        else
            memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptFullName"))
    {
        top->FullName.ptr = keyValue;
    }
    else if (!strcmp(keyName, "postscriptWeightName"))
    {
        top->Weight.ptr = keyValue;
    }
    else if (!strcmp(keyName, "postscriptIsFixedPitch"))
    {
        top->isFixedPitch = atol(keyValue);
    }
    else if (!strcmp(keyName, "italicAngle"))
    {
        top->ItalicAngle = (float)strtod(keyValue, NULL);
    }
    else if (!strcmp(keyName, "postscriptUnderlinePosition"))
    {
        top->UnderlinePosition = (float)strtod(keyValue, NULL);
    }
    else if (!strcmp(keyName, "postscriptUnderlineThickness"))
    {
        top->UnderlineThickness = (float)strtod(keyValue, NULL);
    }
    else if (!strcmp(keyName, "unitsPerEm"))
    {
        double ppem =  strtod(keyValue, NULL);
        top->sup.UnitsPerEm = (int)ppem;
        fd0->FontMatrix.cnt = 6;
        fd0->FontMatrix.array[0] = (float)(1.0 / ppem);
        fd0->FontMatrix.array[1] = 0;
        fd0->FontMatrix.array[2] = 0;
        fd0->FontMatrix.array[3] = (float)(1.0 / ppem);
        fd0->FontMatrix.array[4] = 0;
        fd0->FontMatrix.array[5] = 0;
        memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptBlueFuzz"))
    {
        pd->BlueFuzz = (float)strtod(keyValue, NULL);
        memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptBlueShift"))
    {
        pd->BlueShift = (float)strtod(keyValue, NULL);
        memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptBlueScale"))
    {
        pd->BlueScale  = (float)strtod(keyValue, NULL);
        memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptForceBold"))
    {
        pd->ForceBold = atol(keyValue);
    }
    
    else if (!strcmp(keyName, "postscriptBlueValues"))
    {
        bluesArray = (BluesArray*)&pd->BlueValues;
        setBluesArrayValue(h, bluesArray, 14);
    }
    else if  (!strcmp(keyName, "postscriptOtherBlues"))
    {
        bluesArray = (BluesArray*)&pd->OtherBlues;
        setBluesArrayValue(h, bluesArray, 10);
    }
    else if  (!strcmp(keyName, "postscriptFamilyBlues"))
    {
        bluesArray = (BluesArray*)&pd->FamilyBlues;
        setBluesArrayValue(h, bluesArray, 14);
    }
    else if  (!strcmp(keyName, "postscriptFamilyOtherBlues"))
    {
        bluesArray = (BluesArray*)&pd->FamilyOtherBlues;
        setBluesArrayValue(h, bluesArray, 10);
    }
    else if (!strcmp(keyName, "postscriptStdHW"))
    {
        pd->StdHW  = (float)strtod(keyValue, NULL);
        memFree(h, keyValue);
    }
    else if (!strcmp(keyName, "postscriptStdVW"))
    {
        pd->StdVW  = (float)strtod(keyValue, NULL);
        memFree(h, keyValue);
    }
    else if  (!strcmp(keyName, "postscriptStemSnapH"))
    {
        bluesArray = (BluesArray*)&pd->StemSnapH;
        setBluesArrayValue(h, bluesArray, 12);
    }
    else if  (!strcmp(keyName, "postscriptStemSnapV"))
    {
        bluesArray = (BluesArray*)&pd->StemSnapV;
        setBluesArrayValue(h, bluesArray, 12);
    }
    
    else
    {
        // if it isn't used, free the string.
        memFree(h, keyValue);
    }
    
}



static int doFontDictValue(ufoCtx h, char* keyName, char* endKeyName, int state)
{
    char* valueString  = getKeyValue(h,  endKeyName, state);
    if (valueString == NULL)
    {
        if (strcmp(h->parseKeyName, "styleName"))
        {
        message(h, "Warning: Encountered empty %s for fontinfo key %s. Skipping", keyName, h->parseKeyName);
        /* It is valid and common for styleName can be present and empty, when the style is "Regular".*/
        }
    }
    
    if (state == 2) // we are processing a key value.
    {

        if (valueString != NULL)
            setFontDictKey(h, valueString);
        state = 1;
        memFree(h, h->parseKeyName);
    }
    else if (state == 3) // we are within an <array>, which is a key value
    {
        if (valueString != NULL)
            *dnaNEXT(h->valueArray) = valueString;
    }
    else
    {
        memFree(h, h->parseKeyName);
        fatal(h,ufoErrParse, "Encountered key value when not after <key> or <array> element. Text: '%s'.", getBufferContextPtr(h));
    }
    return state;
}

static int CTL_CDECL cmpOrderRecs(const void *first, const void *second, void *ctx)
{
    ufoCtx h = ctx;
    GlIFOrderRec* orderRec1= (GlIFOrderRec *)first;
    GlIFOrderRec* orderRec2= (GlIFOrderRec *)second;
    int retVal = 0;
    retVal =  strcmp(orderRec1->glyphName, orderRec2->glyphName);
    return retVal;
}

static int CTL_CDECL cmpGlifRecs(const void *first, const void *second, void *ctx)
{
    ufoCtx h = ctx;
    GLIF_Rec* glifRec1= (GLIF_Rec *)first;
    GLIF_Rec* glifRec2= (GLIF_Rec *)second;
    int retVal = 0;
    if (glifRec1->glyphOrder == glifRec2->glyphOrder)
    {
        retVal =  strcmp(glifRec1->glyphName, glifRec2->glyphName);
    }
    else if (glifRec1->glyphOrder == ABF_UNSET_INT)
    {
        retVal =  1;
    }
    else if (glifRec2->glyphOrder == ABF_UNSET_INT)
    {
        retVal =  -1;
    }
    else if (glifRec2->glyphOrder == ABF_UNSET_INT)
    {
        
    }
    else {
        if (glifRec1->glyphOrder > glifRec2->glyphOrder)
            retVal =  1;
        else
            retVal =  -1;
    }
    return retVal;
}


static int matchGLIFOrderRec(const void *key, const void *value, void *ctx)
{
    ufoCtx h = ctx;
    GlIFOrderRec* orderRec= (GlIFOrderRec *)value;
    return strcmp((char *)key, orderRec->glyphName);
}

static int getGlyphOrderIndex(ufoCtx h, char * glyphName)
{
    int orderIndex = ABF_UNSET_INT;
    int recIndex = 0;
    
 	if (ctuLookup(glyphName, h->data.glifOrder.array, h->data.glifOrder.cnt,
				   sizeof(h->data.glifOrder.array[0]), matchGLIFOrderRec, (size_t*)&recIndex, h))
	{
        orderIndex = h->data.glifOrder.array[recIndex].order;
    }
    else
    {
        message(h, "Warning: glyph order does not contain glyph name '%s'.", glyphName);
    }
   
    return orderIndex;
}


static void  addGLIFRec(ufoCtx h, int state)
{
	char* fileName;
    GLIF_Rec* newGLIFRec;
    long int glyphOrder;
    int k;

    glyphOrder = getGlyphOrderIndex(h,  h->parseKeyName);
    newGLIFRec = dnaNEXT(h->data.glifRecs);
    k = h->data.glifRecs.cnt-1;
    newGLIFRec->glyphName = h->parseKeyName;
    newGLIFRec->glyphOrder = glyphOrder;
    fileName = getKeyValue(h,  "</string>", state);
    if (fileName == NULL)
    {
        fatal(h,ufoErrParse, "Encounted glyph reference in contents.plist with an empty file path. Text: '%s'.", getBufferContextPtr(h));
    }
    newGLIFRec->glifFileName = memNew(h, 1+ strlen(fileName));
    sprintf(newGLIFRec->glifFileName, "%s", fileName);
}

#define START_PUBLIC_ORDER 1256
#define IN_PUBLIC_ORDER 1257
#define IN_COMMENT 1258

static int parseGlyphOrder(ufoCtx h)
{
    int state = 0; /* 0 == start, 1= seen start of glyph, 2 = seen glyph name, 3 = in path, 4 in comment.  */
    int seenGO = 0;
    int prevState = 0;
    token* tk;
    h->src.next = h->mark = NULL;
    
    h->cb.stm.clientFileName = "lib.plist";
	h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))
    {
        fprintf(stderr, "Failed to read lib.plist\n");
		return ufoErrSrcStream;
    }
    
    dnaSET_CNT(h->valueArray,0);
	fillbuf(h, 0);
    /* Read in file, then sort by glyph name */

	while (!(seenGO || (h->flags & SEEN_END)))
    {
		tk = getToken(h, state);
        
        if (tk == NULL)
            break;
		
		if (tokenEqualStr(tk, "<!--"))
		{
			prevState = state;
			state = IN_COMMENT;
		}
		else if (tokenEqualStr(tk, "-->"))
        {
			if (state != IN_COMMENT)
				fatal(h,ufoErrParse,  "Encountered end comment token while not in comment.");
			state = prevState;
        }
		else if (state == IN_COMMENT)
        {
			continue;
        }
		else if (tokenEqualStrN(tk, "<dict", 5))
        {
            state++;
        }
		else if (tokenEqualStr(tk, "</dict>"))
        {
            state--;
        }
		else if (tokenEqualStrN(tk, "<key", 4) && (state == 1)) /* if we are at the top level of the lib dict, check the key values */
        {
            if ((tk->val[tk->length-2] == '/') && (tk->val[tk->length-1] == '>'))
            {
                message(h, "Warning: Encountered empty <key></key>. Text: '%s'.", getBufferContextPtr(h));
            }
            else
            {
                // get key name
                tk = getElementValue(h, state);
                if (tokenEqualStr(tk, "</key>"))
                {
                    message(h, "Warning: Encountered empty <key></key>. Text: '%s'.", getBufferContextPtr(h));
                }
                else
                {
                    if (tokenEqualStr(tk, "public.glyphOrder"))
                    {
                        prevState = state;
                        state = START_PUBLIC_ORDER;
                    }
                    // get end-key.
                    tk = getToken(h, state);
                    if (!tokenEqualStr(tk, "</key>"))
                    {
                        tk->val[tk->length -1] = 0;
                        fatal(h,ufoErrParse,  "Encountered element other than </key> when reading <key> name: %s. Text: '%s'.", tk->val, getBufferContextPtr(h));
                    }
                }
            }
        }
        else if ((tokenEqualStrN(tk, "<array",6)) && (state == START_PUBLIC_ORDER))
        {
            state = IN_PUBLIC_ORDER;
            if ((tk->val[tk->length-2] == '/') && (tk->val[tk->length-1] == '>'))
            {
                state = prevState;
            }
        }
        else if ((tokenEqualStr(tk, "</array>")) && (state == IN_PUBLIC_ORDER))
        {
            state = prevState;
            seenGO = 1;
            break;
        }
        else if ((tokenEqualStrN(tk, "<string", 7)) && (state == IN_PUBLIC_ORDER))
        {
            GlIFOrderRec* newGLIFOrderRec;
            
            if ((tk->val[tk->length-2] == '/') && (tk->val[tk->length-1] == '>'))
            {
                message(h, "Warning: Encountered empty <string/> in public.glyphOrder. Text: '%s'.", getBufferContextPtr(h));
            }
            else
            {
                 
                tk = getElementValue(h, state);
                if (tokenEqualStr(tk, "</string>"))
                {
                    message(h, "Warning: Encountered empty <string></string>. Text: '%s'.", getBufferContextPtr(h));
                }
                else
                {
                    newGLIFOrderRec = dnaNEXT(h->data.glifOrder);
                    newGLIFOrderRec->glyphName =  copyStr(h, tk->val); // get a copy in memory
                    newGLIFOrderRec->order = h->data.glifOrder.cnt -1;
                    tk = getToken(h, state);
                    if (!tokenEqualStr(tk, "</string>"))
                    {
                        tk->val[tk->length -1] = 0;
                        fatal(h,ufoErrParse,  "Encountered element other than </string> when reading <string> name: %s. Text: '%s'.", tk->val, getBufferContextPtr(h));
                    }
                }
            }
        }
        
    } /* end while more tokens */
    
    if (h->data.glifOrder.cnt > 0)
    {
        /* Sort the array by glyph name. */
        
		ctuQSort(h->data.glifOrder.array, h->data.glifOrder.cnt,
				 sizeof(h->data.glifOrder.array[0]), cmpOrderRecs, h);
        
        /* weed out duplicates - these casue sorting to work differently depending on whether
         we approach the pair from the top or bottom. */
        {
            int i = 1;
            while (i <  h->data.glifOrder.cnt)
            {
                if (0 == strcmp(h->data.glifOrder.array[i].glyphName, h->data.glifOrder.array[i-1].glyphName))
                {
                    /* set the glyoh orders to be the same. First wins */
                    h->data.glifOrder.array[i].order = h->data.glifOrder.array[i-1].order;
                    message(h, "Warning: glyph order contains duplicate entries for glyphs '%s'.", h->data.glifOrder.array[i].glyphName);

                }
                i++;
             }
        }
    }
    
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    
	return ufoSuccess;
}

static int parseGlyphList(ufoCtx h)
{
    int state = 0; /* 0 == start, 1= seen start of glyph, 2 = seen glyph name, 3 = in path, 4 in comment.  */
    int prevState = 0;
    token* tk;
    h->src.next = h->mark = NULL;
    
    h->cb.stm.clientFileName = "glyphs/contents.plist";
	h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))
    {
        fprintf(stderr, "Failed to read glyphs/contents.plist\n");
		return ufoErrSrcStream;
    }
    
    dnaSET_CNT(h->valueArray,0);
	fillbuf(h, 0);
    
	h->metrics.defaultWidth = 0;
	h->flags &= ~SEEN_END;
    
	while (!(h->flags & SEEN_END))
    {
		tk = getToken(h, state);
		
		if (tokenEqualStr(tk, "<!--"))
		{
			prevState = state;
			state = 4;
		}
		else if (tokenEqualStr(tk, "-->"))
        {
			if (state != 4)
				fatal(h,ufoErrParse,  "Encountered end comment token while not in comment.");
			state = prevState;
        }
		else if (state == 4)
        {
			continue;
        }
		else if (tokenEqualStr(tk, "<dict>"))
        {
			if (state > 0)
				fatal(h,ufoErrParse,  "Encountered second <dict while in first");
            state = 1;
        }
		else if (tokenEqualStr(tk, "</dict>"))
        {
            state = 0;
            h->flags |=  SEEN_END;
            break;  // end processing at end of first dict.
        }
		else if (tokenEqualStr(tk, "<key>"))
        {
			if (state != 1)
				fatal(h,ufoErrParse,  "Encountered <key> while not in top level of first <dict>");
            // get key name
            tk = getElementValue(h, state);
            if (tokenEqualStr(tk, "</key>"))
            {
                message(h, "Warning: Encountered empty <key></key>. Text: '%s'.", getBufferContextPtr(h));
            }
            else
            {
                h->parseKeyName = copyStr(h, tk->val); // get a copy in memory
                // get end-key.
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "</key>"))
                {
                    tk->val[tk->length -1] = 0;
                    fatal(h,ufoErrParse,  "Encountered element other than </key> when reading <key> name: %s. Text: '%s'.", tk->val, getBufferContextPtr(h));
                }
            }
            state = 2;
        }
        else if ((tokenEqualStr(tk, "<string>")) && (state == 2))
        {
            addGLIFRec(h, state);
            state = 1;
        }
 		else if (state != 0)
        {
            message(h, "Warning: discarding token '%s", tk->val);
        }
        else
        {
            if (h->data.glifRecs.cnt > 0) // If we have seen the glifs, then we are done with any additional parsing
                break;
        }
        
    } /* end while more tokens */
    
    if (h->data.glifOrder.cnt > 0)
    {
		ctuQSort(h->data.glifRecs.array, h->data.glifRecs.cnt,
				 sizeof(h->data.glifRecs.array[0]), cmpGlifRecs, h);
        
    }
    if (h->data.glifRecs.cnt > 0)
    {
        GLIF_Rec* glifRec;
        int i = 0;
        while (i < h->data.glifRecs.cnt)
        {
            glifRec = &h->data.glifRecs.array[i++];
            addString(h, strlen(glifRec->glyphName), glifRec->glyphName);
         }
    }
   
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    
	return ufoSuccess;
}

static int preParseGLIF(ufoCtx h, GLIF_Rec* newGLIFRec, int tag);

static int preParseGLIFS(ufoCtx h)
{
    int tag = 0;
    int retVal = ufoSuccess;
    
    h->metrics.defaultWidth = 0;
    while (tag < h->data.glifRecs.cnt)
    {
        int glyphRetVal;
        GLIF_Rec* glifRec = &h->data.glifRecs.array[tag];
        addWidth(h, tag, 0); /* will set this to a real value later, if the value is supplied. */
        glyphRetVal = preParseGLIF(h, glifRec, tag);  // set width, stores offset to outline data
        if (glyphRetVal != ufoSuccess)
            retVal = glyphRetVal;
        tag++;
    }
    return retVal;
}

static void addCharFromGLIF(ufoCtx h, int tag, char* glyphName, long char_begin, long char_end, unsigned long unicode)
{
    abfGlyphInfo *chr;

    if (addChar(h, tag, &chr))
    {
        
        message(h, "Warning: duplicate charstring <%s> (discarded)",
                getString(h, tag));
    }
    else
    {
        chr->flags 		= 0;
        chr->tag		= tag;
        chr->gname.ptr	= glyphName;
        chr->gname.impl	= tag;
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
        
    }
   
}

static int preParseGLIF(ufoCtx h, GLIF_Rec* glifRec, int tag)
{
    // set width, stores offset to outline data, Does not parse it.
    int state = 0; /* 0 == start, 1= seen start of glyph,  4 in comment.  */
    int prevState = 0;
    long i;
    long char_begin;
    long char_end;
    long defaultWidth = 1000;
    long glyphWidth;
    unsigned long unicode = ABF_GLYPH_UNENC;
    char tempVal[kMaxName];
    char tempName[kMaxName];
    token* tk;
    h->src.next = h->mark = NULL;
    
    h->flags &= ~((unsigned long)SEEN_END);

    /* First, try the alt layer directory */
    glifRec->glifFilePath = memNew(h, 2 + strlen(h->altLayerDir) + strlen(glifRec->glifFileName));
    sprintf(glifRec->glifFilePath, "%s/%s", h->altLayerDir, glifRec->glifFileName);

    h->cb.stm.clientFileName = glifRec->glifFilePath;
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    if (h->stm.src == NULL)
    {
        /* Didn't work. Try the glyph layer directory */
        memFree(h, glifRec->glifFilePath);
        glifRec->glifFilePath = memNew(h, 2 + strlen(h->defaultLayerDir) + strlen(glifRec->glifFileName));
        sprintf(glifRec->glifFilePath, "%s/%s", h->defaultLayerDir, glifRec->glifFileName);
        
        h->cb.stm.clientFileName = glifRec->glifFilePath;
        h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    }
    if (h->stm.src == NULL)
    {
        fprintf(stderr, "Failed to open glif file in parseGLIF. h->stm.src == NULL. %s.\n", glifRec->glifFilePath);
        return ufoErrSrcStream;
    }
    if (h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))
    {
        fprintf(stderr, "Failed to open glif file in parseGLIF. seek failed. %s.\n", glifRec->glifFilePath);
        return ufoErrSrcStream;
    }

    fillbuf(h, 0);
    
	h->flags &= ~SEEN_END;
    while (!(h->flags & SEEN_END))
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            if (state != 0)
            {
                *(h->src.end-1)=0; /* truncate buffer string */
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.%s.", h->src.buf);
            }
            else
                break;
        }
        
        if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = 4;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != 4)
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment.");
            state = prevState;
        }
        else if (state == 4)
        {
            continue;
        }
        else if ( tokenEqualStr(tk, "<glyph"))
        {
            if (state != 0)
            {
                fatal(h,ufoErrParse,  "Encountered start glyph token while processing another glyph.%s.", getBufferContextPtr(h));
            }
            state = 1;
            char_begin = 0;
            char_end = 0;
            /* Set default width and name, to be used if these values are not supplied by the glyph attributes. */
            glyphWidth = defaultWidth;
            sprintf(tempName, "gid%05d", (unsigned short)h->chars.index.cnt);
        }
        else if ( tokenEqualStr(tk, "<glyph>"))
        {
            if (state != 0)
            {
                 fatal(h,ufoErrParse,  "Encountered start glyph token while processing another glyph.%s.", getBufferContextPtr(h));
            }
            state = 1;
            char_begin = 0;
            char_end = 0;
            /* Set default width and name, to be used if these values are not supplied by the glyph attributes. */
            glyphWidth = defaultWidth;
            sprintf(tempName, "gid%05d", (unsigned short)h->chars.index.cnt);
        }
        else if (tokenEqualStr(tk, "</glyph>"))
        {
            if (char_begin == 0) // happens when there is no <outline> element, which can happen with spaces.
                addCharFromGLIF(h, tag, glifRec->glyphName, char_begin, char_end, unicode);
            state = 0;
            h->flags |= SEEN_END;
            break;
        }
        else if (tokenEqualStr(tk, "<unicode"))
        {
            if (state != 1)
                continue;
            tk = getToken(h, state);
            if (!tokenEqualStr(tk, "hex="))
                fatal(h,ufoErrParse,  "Encountered unknown attribute of <unicode>.%s.", tk->val);
            
            tk = getAttribute(h, state);
            {
                strncpy(tempVal, tk->val+1, tk->length-2); /* remove final ";" and initial '&' */
                tempVal[0] = '0';
                tempVal[tk->length-1] = 0;
                
                unicode = strtol(tk->val, NULL, 16);
            }
            
        }
        else if (tokenEqualStr(tk, "<advance"))
        {
            if (state != 1)
                continue;
            i = 0;
            while (i++ < 2)
            {
                tk = getToken(h, state);
                if (tokenEqualStr(tk, "width="))
                {
                    getAttribute(h, state);
                    strncpy(tempVal, tk->val, tk->length);
                    tempVal[tk->length] = 0;
                    glyphWidth = atol(tempVal);
                    setWidth(h, tag, glyphWidth);
                }
                if (tokenEqualStr(tk, "advance="))
                {
                    getAttribute(h, state);
                    strncpy(tempVal, tk->val, tk->length);
                    tempVal[tk->length] = 0;
                    glyphWidth = atol(tempVal);
                    setWidth(h, tag, glyphWidth);
                }
            }
        }
        else if (tokenEqualStr(tk, "<outline>"))
        {
            if (state != 1)
            {
                continue;
            }
            state = 2;
            char_begin=tk->offset+9; /* skip <outline> tag */
        }
        else if (tokenEqualStr(tk, "</outline>"))
        {
             char_end=tk->offset;
            
            if (state != 2)
            {
                continue;
            }
            state = 1;
            char_end = tk->offset;
            addCharFromGLIF(h, tag, glifRec->glyphName, char_begin, char_end, unicode);
            state = 0;
            break;
        }
        else if (tokenEqualStr(tk, "lib"))
        {
            /* since any lib element follows the outline element, if we get here there is no outline element.
             end we need to add the current glyph to the char list with no outline content. */
            char_begin= 0;
            char_end = 0;
            
            if (state != 1)
            {
                continue;
            }
            addCharFromGLIF(h, tag, glifRec->glyphName, char_begin, char_end, unicode);
            state = 0;
            break;
        }
        else if (isUnknownAttribute(tk))
        {
            tk = getToken(h, state);
            /* discard its value.*/
        }
        else if (tokenEqualStr(tk, "/>"))
        {
            continue;
        }
        
    } /* end while more tokens */
    
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
    return ufoSuccess;
}

static int cmpNumeric(const void *first, const void *second, void *ctx)
{
    int retVal;
    if ((*(float*)first) == (*(float*)second))
        retVal  = 0;
    else if ((*(float*)first) > (*(float*)second))
        retVal  = 1;
    else
        retVal = -1;
    return retVal;
}

static void fixUnsetDictValues(ufoCtx h)
{
    abfTopDict* top = &h->top;
    abfFontDict* fd0 = &h->top.FDArray.array[0];
    abfPrivateDict* pd = &fd0->Private;
  
    if ( fd0->FontName.ptr == NULL)
    {
        fd0->FontName.ptr = "PSNameNotSpecified";
        message(h, "Warning: No PS name specified in source UFO font. Using 'PSNameNotSpecified'.");
    }
    
    if ( top->version.ptr == NULL)
    {
        top->version.ptr  = "1.000";
        message(h, "Warning: No version specified in source UFO font. Using '1.000'.");
    }
    
    if (pd->StemSnapH.cnt > ABF_EMPTY_ARRAY)
    {
        if (pd->StdHW == ABF_UNSET_REAL)
        {
        pd->StdHW  = pd->StemSnapH.array[0];
        }
  		ctuQSort(pd->StemSnapH.array, pd->StemSnapH.cnt,
				 sizeof(pd->StemSnapH.array[0]), cmpNumeric, h);
  }
    if (pd->StemSnapV.cnt > ABF_EMPTY_ARRAY)
    {
        if (pd->StdVW == ABF_UNSET_REAL)
        {
            pd->StdVW = pd->StemSnapV.array[0];
        }
  		ctuQSort(pd->StemSnapV.array, pd->StemSnapV.cnt,
				 sizeof(pd->StemSnapV.array[0]), cmpNumeric, h);
    }
    top->sup.srcFontType = abfSrcFontTypeUFOName;
    
}

static void skipToDictEnd(ufoCtx h)
{
    int state = 0; /* 0 == start, 1=in first dict, 2 in key, 3= in value, 4=in array 4 in comment, 5 in child dict.*/
    int prevState = 0; /* used to save prev state while in comment. */
    token* tk;

    while (!(h->flags & SEEN_END))
    {
        tk = getToken(h, state);
        
        if (tokenEqualStr(tk, "<dict>"))
        {
            skipToDictEnd(h);
        }
        else if (tokenEqualStr(tk, "</dict>"))
        {
            break;
        }
        
    } /* end while more tokens */

}

static int parseFontInfo(ufoCtx h)
{
    int state = 0; /* 0 == start, 1=in first dict, 2 in key, 3= in value, 4=in array 4 in comment, 5 in child dict.*/
    int prevState = 0; /* used to save prev state while in comment. */
    token* tk;
    int seenDict = 0;
	h->src.next = h->mark = NULL;
    h->cb.stm.clientFileName = "fontinfo.plist";
	h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src, 0))
    {
        message(h, "Warning: Failed to open fontinfo.plist in source UFO font. No PostScript FontDict values are specified. \n");
        fixUnsetDictValues(h);
		return ufoSuccess;
    }
   
    dnaSET_CNT(h->valueArray, 0);
	fillbuf(h, 0);
    
    h->flags &= ~((unsigned long)SEEN_END);
    
	while (!(h->flags & SEEN_END))
    {
		tk = getToken(h, state);
		
		if (tokenEqualStr(tk, "<!--"))
		{
			prevState = state;
			state = 4;
		}
		else if (tokenEqualStr(tk, "-->"))
        {
            if (state != 4)
            {
                 fatal(h,ufoErrParse,  "Encountered end comment token while not in comment, in fontinfo.plist file. Context: '%s'.\n", getBufferContextPtr(h));
            }
            state = prevState;
        }
		else if (state == 4)
        {
			continue;
        }
		else if (tokenEqualStr(tk, "<dict>"))
        {
			if (state > 0)
            {
                skipToDictEnd(h);
            }
            else
                state = 1;
        }
		else if (tokenEqualStr(tk, "</dict>"))
        {
                seenDict = 1;
                break;
        }
		else if (state > 4)
            continue;
		else if (tokenEqualStr(tk, "<key>"))
        {
			if (state != 1)
            {
                fatal(h,ufoErrParse,  "Encountered '<key>' while not in top level of first <dict>, in fontinfo.plist file. Context: '%s'.\n", getBufferContextPtr(h));
            }
            // get key name
            tk = getElementValue(h, state);
            if (tokenEqualStr(tk, "</key>"))
            {
                 message(h, "Warning: Encountered empty <key></key>. Text: '%s'.", getBufferContextPtr(h));
            }
            else
            {
                 h->parseKeyName = copyStr(h, tk->val); // get a copy in memory. This is freed when the value is assigned.
                // get end-key.
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "</key>"))
                {
                    tk->val[tk->length -1] = 0;
                     fatal(h,ufoErrParse,  "Encountered element other than </key> when reading <key> name: %s, in fontinfo.plist file. Context: '%s'.\n", tk->val, getBufferContextPtr(h));
                }
            }
            state = 2;
        }
        else if (tokenEqualStr(tk, "<array>"))
        {
            if (state != 2)
            {
                fatal(h,ufoErrParse,  "Encountered <array> when not after <key> element, in fontinfo.plist file. Context: '%s'.\n", getBufferContextPtr(h));
            }
            else
                state = 3;
            dnaSET_CNT(h->valueArray, 0);
        }
		else if (tokenEqualStr(tk, "</array>"))
        {
            if (state != 3)
            {
                fatal(h,ufoErrParse,  "Encountered </array> when not after <array> element, in fontinfo.plist file. Context: '%s'.\n", getBufferContextPtr(h));
            }
            else
            {
                setFontDictKey(h, NULL);
                state = 1;
            }
        }
        else if (tokenEqualStr(tk, "<string>"))
        {
            state = doFontDictValue(h, "<string>", "</string>", state);
        }
        else if (tokenEqualStr(tk, "<real>"))
        {
            state = doFontDictValue(h, "<real>", "</real>", state);
        }
        else if (tokenEqualStr(tk, "<integer>"))
        {
            state = doFontDictValue(h, "<integer>", "</integer>", state);
        }
        else if (tokenEqualStr(tk, "<date>"))
        {
            state = doFontDictValue(h, "<date>", "</date>", state);
        }
        else if (tokenEqualStr(tk, "<data>"))
        {
            message(h, "Warning: encountered <data> element: skipping.");
        }
        else if (tokenEqualStr(tk, "<false/>"))
        {
            setFontDictKey(h, "0");
            state--;
        }
        else if (tokenEqualStr(tk, "<true/>"))
        {
            setFontDictKey(h, "1");
            state--;
        }
 		else if (state != 0)
        {
            tk->val[tk->length -1] = 0;
             message(h, "Warning: discarding token '%s', in fontinfo.plist file. Context: '%s'.", tk->val, getBufferContextPtr(h));
            if (state == 2)
                state = 1;
            else if (state == 3)
                state = 2;
            
        }
        else
        {
            if (seenDict) // If we have seen the <dict>, then we are done with any additional parsing
                break;
        }
        
    } /* end while more tokens */
    
    /* Currently, proscriptStdHW and postscriptStdVW have to be hand-edited in. If they haven't been provided,
     use the first value of the stemsnap entries.
    */
    fixUnsetDictValues(h);
    
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    h->stm.src = NULL;
	return ufoSuccess;
}

static int parseUFO(ufoCtx h)
{
	/* This does a first pass through the font, loading the glyph name/ID and the path to each glyph file.
     Open the UFO fontinfo.plist, and extract the PS infop
     Open the glyphs/contents.plist file, and extract any glyphs that are not in the glyphs.ac/contents.plist file
     */
    int dummy;
    int retVal = parseFontInfo(h);
    if (retVal == ufoSuccess)
        dummy = parseGlyphOrder(h);
    if (retVal == ufoSuccess)
        retVal = parseGlyphList(h);
    if (retVal == ufoSuccess)
        retVal = preParseGLIFS(h);
    return retVal;
}

static int parsePoint(ufoCtx h, abfGlyphCallbacks *glyph_cb, GLIF_Rec* glifRec, int state, Transform* transform)
{
    // we've seen the point element name, Parse until we see the end.
    token* tk;
    float x = 0;
    float y = 0;
    int type = 0;
    char * end;
    char * pointName = NULL;
    int prevState;
    int result = ufoSuccess;
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
           fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.%s. Context: %s\n", glifRec->glifFilePath, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph %s. Context: %s\n", glifRec->glifFilePath, getBufferContextPtr(h));
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        else if (tokenEqualStr(tk, "x="))
        {
			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            x = (float)strtod(tk->val, &end);
        }
        else if (tokenEqualStr(tk, "y="))
        {
			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            y =  (float)strtod(tk->val, &end);
        }
        else if (tokenEqualStr(tk, "name="))
        {
			tk = getAttribute(h, state);
            if (tk->length > 0)
            {
                pointName = memNew(h, tk->length+1);
                end = tk->val+tk->length;
                strncpy(pointName, tk->val, tk->length);
                pointName[tk->length] = 0;
            }
         }
        
        
        else if (tokenEqualStr(tk, "type="))
        {
			tk = getAttribute(h, state);
            if (tokenEqualStr(tk, "move"))
                type = 1;
            else if (tokenEqualStr(tk, "line"))
                type = 2;
            else if (tokenEqualStr(tk, "curve"))
                type = 3;
            else if (tokenEqualStr(tk, "offcurve"))
                continue; // type is already set to 0. x and y will get pushed on the stack, and no other operation will happen.
            else
            {
                tk->val[tk->length -1] = 0;
                fatal(h,ufoErrParse, "Encountered unsupported point type '%s' in glyph '%s'. Context: %s.\n", tk->val, glifRec->glyphName, getBufferContextPtr(h));
                result = ufoErrParse;
                break;
            }
        }
        else if (tokenEqualStr(tk, "/>"))
        {
            if ((transform != NULL) && (!transform->isDefault))
            {
                float *mtx = (float*)transform->mtx;
                float xnew = mtx[0]*x + mtx[2]*y + mtx[4];
                y = mtx[1]*x + mtx[3]*y + mtx[5];
                x = xnew;
           }
            CHKOFLOW(2);
            PUSH(x);
            PUSH(y);
            switch(type)
            {
                case 0:
					if (pointName != NULL)
						h->hints.pointName = pointName; /* we need to save the point name for later reference only for off-curve points, to be passed into the curve OpRec */
                   break;
                case 1:
                    {
                    doOp_mt(h, glyph_cb, pointName);
                    h->hints.pointName = NULL;
                   break;
                    }
                case 2:
                {
                    doOp_dt(h, glyph_cb, pointName);
                    h->hints.pointName = NULL;
                   break;
                }
                case 3:
                {
                    /* The hint set which precedes a curve is referenced in the point name for the first point of the curve.
                    This is currently stored in h->hints.pointName. Save, it, then set h->hints.pointName to NULL.
                     
                     A curve has a point name in teh final point only when the point is the first point of the contour,
                     and the last two points are at the end of the contour. Im this case,  h->hints.pointName is always NULL,
                     and the hint reference is applied before the move-to of the contour.*/
                    
                    if (pointName != NULL)
                        doOp_ct(h, glyph_cb, pointName);
                    else if (h->hints.pointName != NULL)
                        doOp_ct(h, glyph_cb, h->hints.pointName);
                    else
                        doOp_ct(h, glyph_cb, NULL);
                   h->hints.pointName = NULL; /* don't free h->hints.pointName; it is now references from the opRec. */
                     break;
                }
            }
           break;
        }
        else
        {
            continue;
        }
    }
    return result;
}

static int parseGLIF(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks *glyph_cb, Transform* transform);


static int parseComponent(ufoCtx h, GLIF_Rec* glifRec, abfGlyphCallbacks *glyph_cb, int state, Transform* transform)
{
    int result = ufoSuccess;
    abfGlyphInfo* gi;
    
    token* tk;
    char * end;
    int prevState;
    Transform localTransform; // transform specified in component
    Transform* newTransform = NULL; // result of multiplication with existing transform.
    float val;

    setTransformMtx(&localTransform, 1.0 ,0 ,0 ,1.0 ,0 ,0, 1, 1);
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
             fatal(h,ufoErrParse,  "Encountered end of buffer before end of component. Glyph: %s, Context: %s.\n", glifRec->glifFilePath,  getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment in component. Glyph: %s, Context: %s.\n", glifRec->glifFilePath,  getBufferContextPtr(h));
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        else if (tokenEqualStr(tk, "base="))
        {
            size_t index;
            char* glyphName;
			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
            glyphName = tk->val;
            if (!ctuLookup(glyphName, h->chars.byName.array, h->chars.byName.cnt,
                           sizeof(h->chars.byName.array[0]), postMatchChar, &index, h))
            {
                fprintf(stderr, "Could not find component base glyph %s. parent Glyph: %s\n", glyphName, glifRec->glifFilePath);
                result = ufoErrNoGlyph;
                break;
            }
            
            gi = &h->chars.index.array[h->chars.byName.array[index]];
        }
        else if (tokenEqualStr(tk, "xScale="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
           val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 0);
        }
        else if (tokenEqualStr(tk, "yScale="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
           val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 3);
        }
        else if (tokenEqualStr(tk, "xyScale="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
            val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 1);
        }
        else if (tokenEqualStr(tk, "yxScale="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
             val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 2);
        }
        else if (tokenEqualStr(tk, "xOffset="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
            val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 4);
        }
        else if (tokenEqualStr(tk, "yOffset="))
        {
 			tk = getAttribute(h, state);
            end = tk->val+tk->length;
            *end = 0;
            val =  (float)strtod(tk->val, &end);
            setTransformValue(&localTransform, val, 5);
        }
        else if (tokenEqualStr(tk, "/>"))
        {
            int filePos;
            Transform concatTransform;
            if (gi == NULL)
            {
                fprintf(stderr, "Missing component base attribute. Glyph: %s, Context: %s.\n", glifRec->glifFilePath,  getBufferContextPtr(h));
                result = ufoErrNoGlyph;
                break;
            }
            /* save the current stream */
            filePos = h->src.offset + (h->src.next - h->src.buf);
            
            /* Figure out transforms */
            if (transform == NULL)
            {
                if (localTransform.isDefault)
                    newTransform = NULL;
                else
                    newTransform = &localTransform;
            }
            else
            {
                if (localTransform.isDefault)
                    newTransform = transform;
                else if (transform->isOffsetOnly == 1)
                {
                    localTransform.mtx[4] += transform->mtx[4];
                    localTransform.mtx[5] += transform->mtx[5];
                    newTransform = &localTransform;
                    //localTransform.isOffsetOnly remains unchanged
                }
                else
                {
                    newTransform = &concatTransform;
                    matMult(concatTransform.mtx, localTransform.mtx, transform->mtx);
                    concatTransform.isOffsetOnly = 0;
                    concatTransform.isDefault = 0;
                }
            }
            result =  parseGLIF(h, gi, glyph_cb, newTransform);
            h->cb.stm.seek(&h->cb.stm, h->stm.src, filePos);
            fillbuf(h, filePos);
            h->stack.flags &= ~PARSE_END;
            break;
        }
        else
        {
            continue;
        }
    }
     return result;
}

static void doOpList(ufoCtx h, abfGlyphInfo* gi,  abfGlyphCallbacks *glyph_cb)
{
    int i = 0;
    
    /* At this point, we have parsed the outline data, and build a list of pointNameRecs, mapping point names to point index.
    We have also parsed the hint data, and have a a list of hint masks and flexCurves by point name.
    I then step through all the points. We first play back the hintset, if there is one for that pointName.
    */
    
    if (h->data.opList.cnt < 2) // if there is only 1 op, it is a move to. We skip - these are GLIF format 1 anchors.
        return;
    
    while ( i < h->data.opList.cnt)
    {
        int isFlex = 0;
       OpRec* opRec= &h->data.opList.array[i];
        
        if (opRec->pointName != NULL)
        {
            /* We might have a hint set.  play back hints, if any, and if we are being called from a glyph_cb that supports hints ( the metrics cb doesn't) */
            if ((glyph_cb->stem != NULL) && (h->hints.hintMasks.cnt > 0))
            {
                int m;
                
                for (m=0; m < h->hints.hintMasks.cnt; m++)
                {
                    HintMask* hintMask = &h->hints.hintMasks.array[m];
                    if ((0 == strcmp(hintMask->pointName, opRec->pointName)) && (hintMask->maskStems.cnt > 0))
                    {
                        int j = 0;
                        while (j < hintMask->maskStems.cnt)
                        {
                            StemHint * stemHint  = &hintMask->maskStems.array[j++];
                            glyph_cb->stem(glyph_cb, stemHint->flags, stemHint->edge, stemHint->edge + stemHint->width);
                        }
                        break;
                    }
                }
             }
            
            if (opRec->opType == curvetoType)
            {
                /* check for flex */
                
                if (h->hints.flexOpList.cnt > 0)
                {
                    int m;
                    
                    for (m=0; m < h->hints.flexOpList.cnt; m++)
                    {
                        char* flexPointName = h->hints.flexOpList.array[m];
                        if (0 == strcmp(flexPointName, opRec->pointName))
                        {
                            isFlex = 1;
                            break;
                        }
                    }
                }
            }
        }
        
        /* if the opIndex matches an opIndexFlex value, send a flex op instead. */
        switch (opRec->opType)
        {
            case movetoType:
            {
                OpRec* nextOpRec;
                
                /* UFO format fonts can have paths with a single move-to, used as anchor points during design.Moti these. */
                if ((i+1) == h->data.opList.cnt)
                    break;
                nextOpRec = &h->data.opList.array[i+1];
                if (nextOpRec->opType == movetoType)
                     break;
                glyph_cb->move(glyph_cb, opRec->coords[0], opRec->coords[1]);
                break;
            }
            case linetoType:
            {
                glyph_cb->line(glyph_cb, opRec->coords[0], opRec->coords[1]);
                break;
            }
            case curvetoType:
            {
                int callCurve = 1;
                if ((glyph_cb->stem != NULL) && isFlex)
                {
                    
                    OpRec* opRec1= &h->data.opList.array[i+1];
                    /* despite the T1 reference guide, autohint has always emitted a fixed flex depth of 50. */
                    callCurve = 0;
                    glyph_cb->flex(glyph_cb, 50,
                                   opRec->coords[0], opRec->coords[1],
                                   opRec->coords[2], opRec->coords[3],
                                   opRec->coords[4], opRec->coords[5],
                                   opRec1->coords[0], opRec1->coords[1],
                                   opRec1->coords[2], opRec1->coords[3],
                                   opRec1->coords[4], opRec1->coords[5]);
                    i++; /* we skip the next op, since we just cappsed throught the flex callback. */
                    break;
                }
                
                
                if (callCurve)
                {
                    glyph_cb->curve(glyph_cb,
                                opRec->coords[0], opRec->coords[1],
                                opRec->coords[2], opRec->coords[3],
                                opRec->coords[4], opRec->coords[5]);
                }
                break;
            }
                
        }
        i++;
    }
}

static int parseStem(ufoCtx h, GLIF_Rec* glifRec, HintMask * curHintMask, int stemFlags, Transform* transform)
{
    int result = ufoSuccess;
    token* tk;
    int state = outlineInStemHint;
    char* end;
    float pos = 0;
    float width = 0;
    StemHint* stem;
    int isH = !(stemFlags & ABF_VERT_STEM);
    
    if ((transform != NULL) && (!transform->isOffsetOnly))
    {
        /* We omit stems if the stems are being skewed */
        if (isH && (transform->mtx[2] != 0.0))
            return result;
        if ( (!isH) && (transform->mtx[1] != 0.0))
            return result;
    }
    
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing stem.%s.", glifRec->glyphName);
        }
        
        else if (tokenEqualStr(tk, "pos="))
        {
            tk = getAttribute(h, state);
            end = tk->val+tk->length;
            pos = (float)strtod(tk->val, &end);
        }
        
        else if (tokenEqualStr(tk, "width="))
        {
            tk = getAttribute(h, state);
            end = tk->val+tk->length;
            width = (float)strtod(tk->val, &end);
        }
        
        
        else if (tokenEqualStr(tk, "/>"))
        {
            state = outlineInHintSet;
            break;
        }
        else
        {
            break;
        }
        
    }
    
    if (state != outlineInHintSet)
    {
        fatal(h,ufoErrParse,  "Encountered unexpected token when parsing stem hint. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
    }
    
    if ((transform != NULL) && (!transform->isDefault))
    {
        float *mtx = (float*)transform->mtx;
        { // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH)
            {
                pos = mtx[3]*pos + mtx[5];
                width =  mtx[3]*width;
            }
            else
            {
                pos = mtx[0]*pos + mtx[4];
                width = mtx[0]*width;
            }
        }
    }
    
    stem = dnaNEXT(curHintMask->maskStems);
    stem->edge = pos;
    stem->width = width;
    stem->flags = stemFlags;
    return result;
    
}



static int parseStem3(ufoCtx h, GLIF_Rec* glifRec, HintMask * curHintMask, int stemFlags, Transform* transform)
{
    int result = ufoSuccess;
    token* tk;
    int state = outlineInStemHint;
    float coords[] = {0,0,0,0,0,0};
    int i;
    StemHint* stem;
    int isH = !(stemFlags & ABF_VERT_STEM);
   
    if ((transform != NULL) && (!transform->isOffsetOnly))
    {
        /* We omit stems if the stems are being skewed */
        if (isH && (transform->mtx[2] != 0.0))
            return result;
        if ( (!isH) && (transform->mtx[1] != 0.0))
            return result;
    }
    
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing stem3.%s.", glifRec->glyphName);
        }
        
        else if (tokenEqualStr(tk, "stem3List="))
        {
            char* start;
            char* end;
           tk = getAttribute(h, state);
            start = tk->val;
            end = start + tk->length;
            *end = ',';
            /* This is 6 float numbers, separated by periods. */
            i = 0;
            while (i < 6)
            {
                end = strchr(start, ',');
                if (end == NULL)
                    break;
                coords[i++] = (float)strtod(start, &end);
                start = end +1;
            }
            if (i != 6)
            {
                fatal(h,ufoErrParse,  "When parsing stem3, did not find 6 coordinates. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
        }
        
        else if (tokenEqualStr(tk, "/>"))
        {
            state = outlineInHintSet;
            break;
        }
        else
        {
            break;
        }
        
    }
    
    if (state != outlineInHintSet)
    {
        fatal(h,ufoErrParse,  "Encountered unexpected token when parsing stem3 hint. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
    }
    
    if ((transform != NULL) && (!transform->isDefault))
    {
        float *mtx = (float*)transform->mtx;
        i = 0;
        while (i < 6)
        { // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH)
            {
                coords[i] = mtx[3]*coords[i] + mtx[5];
                i++;
                coords[i] =  mtx[3]*coords[i];
                i++;
           }
            else
            {
                coords[i] = mtx[0]*coords[i] + mtx[4];
                i++;
               coords[i] = mtx[0]*coords[i];
                i++;
            }
        }
    }
    
    // Add the stems
    i = 0;
    while (i < 6)
    { 
        stem = dnaNEXT(curHintMask->maskStems);
        stem->edge = coords[i++];
        stem->width = coords[i++];
        stem->flags = stemFlags;
        if ((i ==0) && (stemFlags & ABF_NEW_HINTS))
            stemFlags &= ~ABF_NEW_HINTS;
    }
    return result;
    
}



static int parseType1HintSet(ufoCtx h, GLIF_Rec* glifRec, char* pointName, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintSet;
    token* tk;
    int prevState;
    int stemFlags = 0;
    HintMask * curHintMask;


    curHintMask = dnaNEXT(h->hints.hintMasks);
    curHintMask->pointName = pointName;

    //printf("Parsing Type1 hint set.\n");
    if (h->hints.hintMasks.cnt > 1)
        stemFlags |= ABF_NEW_HINTS;
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));;
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "<hstem"))
        {
            parseStem(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        else if (tokenEqualStr(tk, "<hstem3"))
        {
            stemFlags |= ABF_STEM3_STEM;
            parseStem3(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        
        else if (tokenEqualStr(tk, "<vstem"))
        {
            stemFlags |= ABF_VERT_STEM;
            parseStem(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        else if (tokenEqualStr(tk, "<vstem3"))
        {
            stemFlags |= ABF_VERT_STEM;
            stemFlags |= ABF_STEM3_STEM;
            parseStem3(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
       }
        
        else if (tokenEqualStr(tk, "</hintset>"))
        {
            if (state != outlineInHintSet)
            {
                fatal(h,ufoErrParse,  "Encountered </hintset> when not at same level as <hintset>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            return result;
        }
        else
        {
            printf("parseT1HintSet: unhandled token: %s\n", tk->val);
            continue;
        }
        
        
    }
    
    
    return result;
}

static int parseType1HintDataV1(ufoCtx h, GLIF_Rec* glifRec, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintData;
    char * pointName = NULL;
    
    token* tk;
    int prevState;
    //printf("Parsing Type1 hint data.\n");
    /* This is Adobe private T1 hint data, so we report a fatal error if the struture is not as expected */
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }

        else if (tokenEqualStrN(tk, "<hintSetList", 12))
        {
            if (state != outlineInHintData)
            {
                fatal(h,ufoErrParse,  "Encountered <hintSetList> when not under <stemhints>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInHintSetList;
            if (tk->length == 12)
            {
                tk = getToken(h, state);
                if (tk == NULL)
                {
                    fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                
                if (tokenEqualStr(tk, "id="))
                {
                    tk = getAttribute(h, state);
                    /* We discard the ID - tx assumes the file is up to date */
                    tk = getToken(h, state);
                }
                if (!tokenEqualStr(tk, ">"))
                {
                    fatal(h,ufoErrParse, "Failed to find end of element for <hintSetList id='xxx'> element. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
            }
        }
        else if (tokenEqualStr(tk, "</hintSetList>"))
        {
            if (state != outlineInHintSetList)
            {
                fatal(h,ufoErrParse,  "Encountered </hintSetList> when not after <hintSetList>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInHintData;
        }
 
        else if (tokenEqualStr(tk, "<hintset"))
        {
            char* end;
            if (state != outlineInHintSetList)
            {
                fatal(h,ufoErrParse,  "Encountered <hintset> when not under <hintSetList>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if (!tokenEqualStr(tk, "pointTag="))
            {
                fatal(h,ufoErrParse,  "Failed to find pointIndex attribute in <hintset pointIndex='n'>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            tk = getAttribute(h, state);
            end = tk->val+tk->length;
            pointName = memNew(h, tk->length+1);
            strcpy(pointName, tk->val);
            /* We discard the ID - tx assumes the file is up to date */
            
            tk = getToken(h, state);
            if (!tokenEqualStr(tk, ">"))
            {
                fatal(h,ufoErrParse, "Failed to find end of element for <hintset pointIndex='n'> element. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            parseType1HintSet(h, glifRec, pointName, transform);
            pointName = NULL;
        }
        
        else if (tokenEqualStr(tk, "<flexList>"))
        {
            if (state != outlineInHintSetList)
            {
                fatal(h,ufoErrParse,  "Encountered <flexList> when not under <hintSetList>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInFlexList;
        }
        else if (tokenEqualStr(tk, "</flexList>"))
        {
            if (state != outlineInFlexList)
            {
                fatal(h,ufoErrParse,  "Encountered </flexList> when not under <flexList>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInHintSetList;
        }
        else if (tokenEqualStr(tk, "<flex"))
        {
            char* end;
            
            if (state != outlineInFlexList)
            {
                fatal(h,ufoErrParse,  "Encountered <flex> when not under <outlineInFlexList>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if (!tokenEqualStr(tk, "pointTag="))
            {
                fatal(h,ufoErrParse,  "Failed to find pointTag attribute in <flex pointTag='n'>. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }

            
            if ((transform == NULL) || ( (transform->mtx[1] == 0) && (transform->mtx[2] == 0)))
            { /* Omit flex if the transform exists and has any skew. */
                tk = getAttribute(h, state);
                end = tk->val+tk->length;
                pointName = memNew(h, tk->length+1);
                strncpy(pointName, tk->val, tk->length);
                pointName[tk->length] = 0;
                
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "/>"))
                {
                    fatal(h,ufoErrParse, "Failed to find end of element for <hintset pointIndex='n'> element. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                
                *dnaNEXT(h->hints.flexOpList) = pointName;
                pointName = NULL;
            }
        }
        
        else if (tokenEqualStr(tk, "</data>"))
        {
            if (state != outlineInHintData)
            {
                fatal(h,ufoErrParse,  "Encountered </data> when not at same level as <data>.. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            return result;
        }
        else
        {
            printf("parseT1HintDataV1: unhandled token: %s\n", tk->val);
            continue;
        }
    }
    return result;
}


static int parseStemV2(ufoCtx h, GLIF_Rec* glifRec, HintMask * curHintMask, int stemFlags, Transform* transform)
{
    int result = ufoSuccess;
    token* tk;
    int state = outlineInStemHint;
    char* end;
    float pos = 0;
    float width = 0;
    StemHint* stem;
    int isH = !(stemFlags & ABF_VERT_STEM);
    int count = 0;
    if ((transform != NULL) && (!transform->isOffsetOnly))
    {
        /* We omit stems if the stems are being skewed */
        if (isH && (transform->mtx[2] != 0.0))
            return result;
        if ( (!isH) && (transform->mtx[1] != 0.0))
            return result;
    }
    
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing stem.%s.", glifRec->glyphName);
        }
        
        else if (tokenEqualStr(tk, "</string>"))
        {
            if (count != 2)
            {
                fatal(h,ufoErrParse,  "Encountered end of stem hint before seeing both pos and width. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInHintSet;
            break;
        }
        
        else if (tk->val[0] == '<')
        {
            fatal(h,ufoErrParse,  "Encountered new XML element before seeing both pos and width. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            return result;
        }
        
        else if (count == 0)
        {
            end = tk->val+tk->length;
            pos = (float)strtod(tk->val, &end);
            count++;
        }
        
        else if (count == 1)
        {
            end = tk->val+tk->length;
            width = (float)strtod(tk->val, &end);
            count++;
        }
        
        else
        {
            break;
        }
        
    }
    
    if (state != outlineInHintSet)
    {
        fatal(h,ufoErrParse,  "Encountered unexpected token when parsing stem hint. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
    }
    
    if ((transform != NULL) && (!transform->isDefault))
    {
        float *mtx = (float*)transform->mtx;
        { // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH)
            {
                pos = mtx[3]*pos + mtx[5];
                width =  mtx[3]*width;
            }
            else
            {
                pos = mtx[0]*pos + mtx[4];
                width = mtx[0]*width;
            }
        }
    }
    
    stem = dnaNEXT(curHintMask->maskStems);
    stem->edge = pos;
    stem->width = width;
    stem->flags = stemFlags;
    return result;
    
}



static int parseStem3V2(ufoCtx h, GLIF_Rec* glifRec, HintMask * curHintMask, int stemFlags, Transform* transform)
{
    int result = ufoSuccess;
    token* tk;
    int state = outlineInStemHint;
    float coords[] = {0,0,0,0,0,0};
    StemHint* stem;
    int isH = !(stemFlags & ABF_VERT_STEM);
    int count = 0;
    
    if ((transform != NULL) && (!transform->isOffsetOnly))
    {
        /* We omit stems if the stems are being skewed */
        if (isH && (transform->mtx[2] != 0.0))
            return result;
        if ( (!isH) && (transform->mtx[1] != 0.0))
            return result;
    }
    
    
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing stem3.%s.", glifRec->glyphName);
        }
        else if (tokenEqualStr(tk, "</string>"))
        {
            if (count != 6)
            {
                fatal(h,ufoErrParse,  "Encountered end of stem3 hint before seeing both pos and width. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = outlineInStemHint;
            break;
        }
        else if (tk->val[0] == '<')
        {
            fatal(h,ufoErrParse,  "Encountered new XML element before seeing both pos and width in stem3 hint. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            return result;
        }
        
        else
        {
            char* start;
            char* end;
            start = tk->val;
            end = start + tk->length;
            coords[count++] = (float)strtod(start, &end);
            /* This is 6 float numbers, separated by spaces. */
            while (count < 6)
            {
                tk = getToken(h, state);
                start = tk->val;
                end = start + tk->length;
                coords[count] = (float)strtod(start, &end);
                start = end +1;
                count++;
            }
            if (count != 6)
            {
                fatal(h,ufoErrParse,  "When parsing stem3, did not find 6 coordinates. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
        }
         
    }
    
    if (state != outlineInStemHint)
    {
        fatal(h,ufoErrParse,  "Encountered unexpected token when parsing stem3 hint. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
    }
    
    if ((transform != NULL) && (!transform->isDefault))
    {
        float *mtx = (float*)transform->mtx;
        count = 0;
        while (count < 6)
        { // width is relative, and needs only to be scaled. WE quite at the beginning if the scale factors were not 0.
            if (isH)
            {
                coords[count] = mtx[3]*coords[count] + mtx[5];
                count++;
                coords[count] =  mtx[3]*coords[count];
                count++;
            }
            else
            {
                coords[count] = mtx[0]*coords[count] + mtx[4];
                count++;
                coords[count] = mtx[0]*coords[count];
                count++;
            }
        }
    }
    
    // Add the stems
    count = 0;
    while (count < 6)
    {
        stem = dnaNEXT(curHintMask->maskStems);
        stem->edge = coords[count++];
        stem->width = coords[count++];
        stem->flags = stemFlags;
        if ((count ==0) && (stemFlags & ABF_NEW_HINTS))
            stemFlags &= ~ABF_NEW_HINTS;
    }
    return result;
    
}



static int parseHintSetV2(ufoCtx h, GLIF_Rec* glifRec, char* pointName, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintSet;
    token* tk;
    int prevState;
    int stemFlags = 0;
    HintMask * curHintMask;
    
    curHintMask = dnaNEXT(h->hints.hintMasks);
    curHintMask->pointName = pointName;
    
    //printf("Parsing Type1 hint set.\n");
    if (h->hints.hintMasks.cnt > 1)
        stemFlags |= ABF_NEW_HINTS;
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));;
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "</array>"))
        {
            return result;
        }
        
        else if (tokenEqualStr(tk, "<string>"))
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "</string>"))
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "hstem"))
        {
            parseStemV2(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        else if (tokenEqualStr(tk, "hstem3"))
        {
            stemFlags |= ABF_STEM3_STEM;
            parseStem3V2(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        
        else if (tokenEqualStr(tk, "vstem"))
        {
            stemFlags |= ABF_VERT_STEM;
            parseStemV2(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        else if (tokenEqualStr(tk, "vstem3"))
        {
            stemFlags |= ABF_VERT_STEM;
            stemFlags |= ABF_STEM3_STEM;
            parseStem3V2(h, glifRec, curHintMask, stemFlags, transform);
            stemFlags = 0;
        }
        
        else
        {
            printf("parseT1HintSet: unhandled token: %s\n", tk->val);
            continue;
        }
        
        
    }
    
    
    return result;
}

static int parseHintSetListV2(ufoCtx h, GLIF_Rec* glifRec, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintData;
    token* tk;
    int prevState;
    int stemFlags = 0;
    char* pointName = NULL;
    
    
    //printf("Parsing Type1 hint set.\n");
    if (h->hints.hintMasks.cnt > 1)
        stemFlags |= ABF_NEW_HINTS;
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));;
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "<array>") && (state == outlineInHintData))
        {
            state = outlineInHintSetList;
            continue;
        }
        
        else if (tokenEqualStr(tk, "</array>") && (state == outlineInHintSetList))
        {
            state = outlineInHintData;
            return result;
        }
        
        else if (tokenEqualStr(tk, "<key>") && (state == outlineInHintSet))
        {
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            if (tk->val[0] == '<')
            {
                fatal(h,ufoErrParse,  "hintSet entry was empty. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if (!strcmp(tk->val, "pointTag"))
            {
                state = outlineInHintSetName;
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "</key>"))
                {
                    fatal(h,ufoErrParse, "Failed to find end of element for <key>pointTag Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
            }
            continue;
       }
        else if (tokenEqualStr(tk, "<string>") && (state == outlineInHintSetName))
        {
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            if (tk->val[0] == '<')
            {
                fatal(h,ufoErrParse,  "hintSet entry was empty. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }

            pointName = memNew(h, tk->length+1);
            strcpy(pointName, tk->val);
            //pointName[tk->length+1] = 0;
            
            tk = getToken(h, state);
            if (!tokenEqualStr(tk, "</string>"))
            {
                fatal(h,ufoErrParse, "Failed to find end of element for <string>hintSet name. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            state = outlineInHintSetStemList;
            continue;
        }
        else if (tokenEqualStr(tk, "<key>") && (state == outlineInHintSetStemList))
        {
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            if (tk->val[0] == '<')
            {
                fatal(h,ufoErrParse,  "hintSet entry was empty. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if (!strcmp(tk->val, "stems"))
            {
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "</key>"))
                {
                    fatal(h,ufoErrParse, "Failed to find end of element for <key>stems Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
            }
            continue;
        }
        else if (tokenEqualStr(tk, "<array>") && (state == outlineInHintSetStemList))
        {
            if (pointName == NULL)
            {
                fatal(h,ufoErrParse, "Encountered hintset stems array before seing point name. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            result = parseHintSetV2(h, glifRec, pointName, transform);
            state = outlineInHintSet;
            pointName = NULL;
            continue;
        }

        
        else if (tokenEqualStr(tk, "</dict>") && (state == outlineInHintSet))
        {
            state = outlineInHintSetList;
            continue;
        }
        else if (tokenEqualStr(tk, "<dict>") && (state == outlineInHintSetList))
        {
            state = outlineInHintSet;
            continue;
        }
        
        else
        {
            printf("parseHintSetListV2: unhandled token: %s. Glyph: %s. Context: %s\n.", tk->val, glifRec->glyphName, getBufferContextPtr(h));
            continue;
        }
        
        
    }
    
    
    return result;
}

static int parseFlexListV2(ufoCtx h, GLIF_Rec* glifRec, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintData;
    token* tk;
    int prevState;
    char* pointName;
     
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "<array>") && (state == outlineInHintData))
        {
            state = outlineInFlexList;
            continue;
        }
        
        else if (tokenEqualStr(tk, "</array>") && (state == outlineInFlexList))
        {
            state = outlineInFlexList;
            return result;
        }
        
        
        else if (tokenEqualStr(tk, "<string>") && (state == outlineInFlexList))
        {
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph.. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if (tk->val[0] == '<')
            {
                fatal(h,ufoErrParse,  "Entry in flexList was empty. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            
            if ((transform == NULL) || ( (transform->mtx[1] == 0) && (transform->mtx[2] == 0)))
            { /* Omit flex if the transform exists and has any skew. */
                pointName = memNew(h, tk->length+1);
                strcpy(pointName, tk->val);
                //pointName[tk->length+1] = 0;
                
                tk = getToken(h, state);
                if (!tokenEqualStr(tk, "</string>"))
                {
                    fatal(h,ufoErrParse, "Failed to find end of element for <string>flexPointName. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                
                *dnaNEXT(h->hints.flexOpList) = pointName;
                pointName = NULL;
            }
        }
 
        else
        {
            fatal(h,ufoErrParse, "parseFlexListV2: unhandled token: %s. Glyph: %s. Context: %s\n.", tk->val, glifRec->glyphName, getBufferContextPtr(h));
            continue;
        }
        
        
    }
    
    
    return result;
}


static int parseType1HintDataV2(ufoCtx h, GLIF_Rec* glifRec, Transform* transform)
{
    int result = ufoSuccess;
    int state = outlineInHintData;
    char * pointName = NULL;
    
    token* tk;
    int prevState;
    //printf("Parsing Type1 hint data.\n");
    /* This is Adobe private T1 hint data, so we report a fatal error if the struture is not as expected */
    while (1)
    {
        tk = getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        
        else if (tokenEqualStr(tk, "<!--"))
        {
            prevState = state;
            state = outlineInComment;
        }
        else if (tokenEqualStr(tk, "-->"))
        {
            if (state != outlineInComment)
            {
                fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            state = prevState;
        }
        else if (state == outlineInComment)
        {
            continue;
        }
        
        else if (tokenEqualStr(tk, "</dict>"))
        {
            return result;
        }
        else if (tokenEqualStr(tk, "<key>"))
        {
            tk = getToken(h, state);
            if (tk == NULL)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            if (!strcmp(tk->val, "id"))
            {
                tk = getToken(h, state); /* skip the close key */
                state = outlineInID;
                continue;
            }
            else if (!strcmp(tk->val, "hintSetList"))
            {
                tk = getToken(h, state); /* skip the close key */
                parseHintSetListV2(h, glifRec, transform);
                continue;
            }
            else if (!strcmp(tk->val, "flexList"))
            {
                tk = getToken(h, state); /* skip the close key */
                parseFlexListV2(h, glifRec, transform);
                continue;
            }
            else
            {
                fatal(h,ufoErrParse, "parseType1HintDataV2: unhandled key value: %s. Glyph: %s. Context: %s\n.", tk->val, glifRec->glyphName, getBufferContextPtr(h));
            }
           
           
       }
        else if ((tokenEqualStr(tk, "<string>")) && (state == outlineInID))
        {
            tk = getToken(h, state); /* skip the value */
            if (tokenEqualStr(tk, "</string>"))
            {
                state = outlineInHintData; // cover case of empty element.
            }
            continue;
        }
        else if ((tokenEqualStr(tk, "</string>")) && (state == outlineInID))
        {
            state = outlineInHintData;
            continue;
        }
        else
        {
            fatal(h,ufoErrParse, "parseType1HintDataV2: unhandled token: %s. Glyph: %s. Context: %s\n.", tk->val, glifRec->glyphName, getBufferContextPtr(h));
        }
    }
    return result;
}

static void skipData(ufoCtx h, GLIF_Rec* glifRec)
{
    int state = otherDictKey;
    int prevState = otherDictKey;
    token* tk;
    token* lastToken;
    dnaDCL(token, tokenList);
    dnaINIT(h->dna, tokenList, 10,10);
    
    /* We just saw a third party <lib>/<dict>/<key>xx</key>. Now we want to skip until the following item is consumed. */
    do
    {
        
        tk =  getToken(h, state);
        if (tk == NULL)
        {
            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        }
        else
        {
            if (tokenEqualStr(tk, "<!--"))
            {
                prevState = state;
                state = outlineInComment;
            }
            else if (tokenEqualStr(tk, "-->"))
            {
                if (state != outlineInComment)
                {
                    fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = prevState;
            }
             else
            {
                /* If the token is in the form "<NNN", skip until see  "/>" or ">". If the latter, increment depth, push the token on the stack.
                 If the token is in the form <NNN>, increment depth, push the token on the stack.
                 if the token is in the form </NNN>, then check that this matches the last token on the stack, and decrement depth.
                 when depth is 0, return.
                 */
                if (tk->val[0] == '<')
                {
                    if (tk->val[1] == '/')
                    {
                        /* An end token! this should match lastToken, which is the last item pushed on the stack.*/
                        if (0 == strncmp(&tk->val[2], &lastToken->val[1], lastToken->length-1))
                        {
                            tokenList.cnt--;
                            if (tokenList.cnt == 0)
                                return;
                            lastToken = &tokenList.array[tokenList.cnt-1];
                        }
                        else
                        {
                            fatal(h,ufoErrParse,  "Encountered end element that does not match previous start element. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                        }
                    }
                    else
                    {
                       
                        if (tk->val[tk->length-1] == '>')
                        { /* A start element */
                            if (tk->val[tk->length-2] == '/')
                            {
                                /* a token in the form <NNN/>. We consumed the whole thing.*/
                                if (tokenList.cnt == 0)
                                    return;
                                continue;                       }
                            else
                            {
                                /* Looks like a token in the form <NNN>. Push it on the stack, */
                                lastToken = dnaNEXT(tokenList);
                                *lastToken = *tk;
                                state = otherInElement;
                            }
                            
                        }
                        else
                        {
                            /* An open start element in the form "<NNN". Consume tokens until we see "/>" or ">". */
                            lastToken = dnaNEXT(tokenList);
                            *lastToken = *tk;
                            state = otherInTag;
                       }
                   }                   
                }
                else if ((tk->val[0] == '>') && (tk->length == 1))
                {
                    if (state != otherInTag)
                    {
                        fatal(h,ufoErrParse,  "Encountered '>' while not parsing an element tag. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                    }
                    /* The closing brace for the current element. */
                    state = otherInElement;
                }
                else if ((tk->val[0] == '/') &&(tk->val[1] == '>') && (tk->length == 2))
                {
                    /* The end of element */
                    if (state != otherInTag)
                    {
                        fatal(h,ufoErrParse,  "Encountered '/>' while not parsing an element tag. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                    }
                    /* The closing brace for the current element. */
                    tokenList.cnt--;
                    if (tokenList.cnt == 0)
                        return;
                    lastToken = &tokenList.array[tokenList.cnt-1];
                    state = otherInElement;
                }
            }
        }
    }
    while (!(h->stack.flags & PARSE_END));

}


static int parseGLIF(ufoCtx h, abfGlyphInfo* gi, abfGlyphCallbacks *glyph_cb, Transform* transform)
{
    
    /* The first point in a GLIF outlne serves two purposes: it is the start point, but also the end-point.
     We need to to convert it to the initial move-to, but we also need to
     re-issue it as the original point type at the end of the path.
     */
    
    int result = ufoSuccess;
    int state = outlineStart;
    int prevState = outlineStart;
    int contourStartOpIndex = 0;
    token* tk;
    
    STI sti =  (STI)gi->tag;
    GLIF_Rec* glifRec = &h->data.glifRecs.array[sti];
    
    /* open the file */
    h->src.next = h->mark = NULL;
    h->flags &= ~((unsigned long)SEEN_END);
    
    h->cb.stm.clientFileName = glifRec->glifFilePath;
    h->stm.src = h->cb.stm.open(&h->cb.stm, UFO_SRC_STREAM_ID, 0);
    
    /* Note that in preParseGlyph, we found the start of the <outline> element, and set gi->sup.begin
    to that, so we can seek to this and skip all the preceding stuff, if any.
    */
    if (h->stm.src == NULL || h->cb.stm.seek(&h->cb.stm, h->stm.src,gi->sup.begin))
    {
        fprintf(stderr, "Failed to open glif file in parseGLIFOutline. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
        fatal(h, ufoErrSrcStream, 0);
    }
    
    if (gi->sup.begin == 0)
    {
        /* This is a non-marking glyph. Just return. */

        h->cb.stm.close(&h->cb.stm, h->stm.src);
        return result;
    }
    
    fillbuf(h, gi->sup.begin);
    
    h->stack.cnt = 0;
    h->stack.hintflags = 0;
    h->stack.flags = 0;
 	h->hints.pointName = NULL;
   
    do
    {
        
        tk = getToken(h, state);
        if (tk == NULL)
        {
            if (state != outlineStart)
            {
                fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
            }
            h->stack.flags = PARSE_END;
        }
        else
        {
            if (tokenEqualStr(tk, "<!--"))
            {
                prevState = state;
                state = outlineInComment;
            }
            else if (tokenEqualStr(tk, "-->"))
			{
                if (state != outlineInComment)
                {
                    fatal(h,ufoErrParse,  "Encountered end comment token while not in comment. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = prevState;
			}
            else if (state == outlineInComment)
			{
                continue;
			}
            else if (tokenEqualStrN(tk, "<contour", 8))
			{
                if (state != outlineStart)
                {
                    fatal(h,ufoErrParse,  "Encountered </contour> token unexpectedly. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = outlineInContour;
                if (tk->length == 8)
                {
                    tk = getToken(h, state);
                    while (tk->val[tk->length-1] != '>')
                        tk = getToken(h, state);
                    
                    // advance to end token
                }
                contourStartOpIndex = h->data.opList.cnt; // This will be the index of the first op in the new contour.
                h->stack.flags |= PARSE_PATH;
                h->stack.flags &= ~((unsigned long)PARSE_SEEN_MOVETO);
			}
            else if (tokenEqualStr(tk, "</contour>"))
			{
                 if (state != outlineInContour)
                {
                    fatal(h,ufoErrParse,  "Encountered </contour> token unexpectedly. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = outlineStart;

                if (h->data.opList.cnt > 1)
                {
                    OpRec* firstOpRec = &h->data.opList.array[contourStartOpIndex];
                   /* Now we need to fix up the OpList. In GLIF, there is usually no explicit start point, as the format expresses
                     the path segments as a complete closed path, with no explicit start point.
                     
                     I use the first path operator end point as the start point, and convert this first operator to move-to.
                     If the first path operator was a line-to, then I do not add it to the end of the op-list, as it should become an implicit rather than explicit close path.
                     If it is a curve, then I need to add it to the oplist as the final path segment. */
                    if (firstOpRec->opType == linetoType)
                    {
                        /* I just need to convert this to a move-to. The final line-to will become implicit */
                        firstOpRec->opType = movetoType;
                    }
                    else if (firstOpRec->opType == curvetoType)
                    {
                      /* The first two points for the curve should be on the stack.
                       If there is a hint set for this last curve, it was part of the first point element for the curve
                       and is currently stored in  h->hints.pointName.
                       If there is a pointName in the firstOpRec, it belongs in the first move-to.
                        */
                        /* Add the final curve-to to the opList.*/
                        CHKOFLOW(2);
                        PUSH(firstOpRec->coords[0]);
                        PUSH(firstOpRec->coords[1]);
                        doOp_ct(h, glyph_cb, h->hints.pointName); /* adds a new curve opRec to the op list, using the point name (if any) of the first point of the curve.  */
                        
                        /* doOp_ct can resize the opList array, invalidating the firstOpRec pointer */
                        firstOpRec = &h->data.opList.array[contourStartOpIndex];
                        firstOpRec->opType = movetoType;
                        
                        h->hints.pointName = NULL;

                     }
                    
                 }
                 h->stack.flags &= ~((unsigned long)(PARSE_PATH | PARSE_SEEN_MOVETO));
			}
            else if (tokenEqualStr(tk, "<point"))
			{
                if (state != outlineInContour)
                {
                    fatal(h,ufoErrParse,  "Encountered <point token unexpectedly. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = outlineInPoint;
                result =  parsePoint(h, glyph_cb, glifRec, state, transform);
                state = outlineInContour;
                if (ufoSuccess != result)
                    break;
 			}
            else if (tokenEqualStr(tk, "<component"))
			{
                if (state != outlineStart)
                {
                    fatal(h,ufoErrParse,  "Encountered <component token unexpectedly. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
                state = outlineInComponent;
                result =  parseComponent(h, glifRec, glyph_cb, state, transform);
                state = outlineStart;
                if (ufoSuccess != result)
                    break;
 			}
            else if (tokenEqualStr(tk, "<lib>"))
            {
                if (state == outlineStart)
                {
                    state = outlineInLib;
                }
           }
            else if (tokenEqualStr(tk, "</lib>"))
            {
                if (state == outlineInLib)
                {
                    state = outlineStart;
                }
            }
            else if (tokenEqualStr(tk, "<dict>"))
            {
                if (state == outlineInLib)
                {
                    state = outlineInLibDict;
                }
            }
            else if (tokenEqualStr(tk, "</dict>"))
            {
                if (state == outlineInLibDict)
                {
                    state = outlineInLib;
                }
            }
            else if (tokenEqualStr(tk, "<key>"))
            {
                if (state == outlineInLibDict) // This consumes the "<\key>" token, whether or not it is an Adobe key.
                {
                    char* valueString  = getKeyValue(h,  "</key>", state);
                    if (valueString == NULL)
                    {
                        fatal(h,ufoErrParse,  "Encountered empty <key> for GLIF lib/dict. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                        continue;
                    }

                    if (tk == NULL)
                    {
                        fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                        h->stack.flags = PARSE_END;
                    }
                    
                    if (0 == strcmp(valueString, t1HintKeyV1))
                    {
                        tk = getToken(h, state);
                        if (tk == NULL)
                        {
                            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing t1 hint key. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                            h->stack.flags = PARSE_END;
                        }
                        
                        if (tokenEqualStr(tk, "<data>"))
                        {
                            parseType1HintDataV1(h, glifRec, transform);
                        }
                    }
                    else if (0 == strcmp(valueString, t1HintKeyV2))
                    {
                        tk = getToken(h, state);
                        if (tk == NULL)
                        {
                            fatal(h,ufoErrParse,  "Encountered end of buffer before end of glyph when parsing t1 hint key. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                            h->stack.flags = PARSE_END;
                        }
                        
                        if (tokenEqualStr(tk, "<dict>"))
                        {
                            parseType1HintDataV2(h, glifRec, transform);
                        }
                    }
                    else
                    {
                        /* We just saw a <key>data<\key> for a  third party <lib><dict>. We want to skip tokens until we see the end of the data. */
                        skipData(h, glifRec);
                    }
                    
                }
            }
           else if (tokenEqualStr(tk, "</outline>"))
            {
                if (state != outlineStart)
                {
                    fatal(h,ufoErrParse,  "Encountered </outline> token unexpectedly. Glyph: %s. Context: %s\n.", glifRec->glyphName, getBufferContextPtr(h));
                }
            }
            else if (tokenEqualStr(tk, "</glyph>"))
            {
                /* The normal exit */
                h->cb.stm.close(&h->cb.stm, h->stm.src);
                return result;
            }
           else
            {
                if ((state != outlineInLib) && (state != outlineInLibDict))
                    printf("parseGlyphOutline: unhandled token: %s. Glyph: %s. Context: %s.\n", tk->val, glifRec->glyphName, getBufferContextPtr(h));
            }
            
            
            
        }
    }
    while (!(h->stack.flags & PARSE_END));
    
    /* An odd exit - didn't see  "</glyph>"  */
    h->cb.stm.close(&h->cb.stm, h->stm.src);
    return result;
}


static int readGlyph(ufoCtx h, unsigned short tag, abfGlyphCallbacks *glyph_cb)
{
	int result;
	token op_tk;
	abfGlyphInfo* gi;
	long width;
    int i;
	
	op_tk.type = ufoNotSet;
	
	gi = &h->chars.index.array[tag];
    
	/* note that gname.ptr is not stable: it is a pointer into the h->string->buf array, which moves when it gets resized. */
	gi->gname.ptr = getString(h, (STI)gi->tag);
	result = glyph_cb->beg(glyph_cb, gi);
	gi->flags |= ABF_GLYPH_SEEN;
    
    
	/* Check result */
	switch (result)
    {
        case ABF_SKIP_RET:
            return ufoSuccess;
        case ABF_QUIT_RET:
            fatal(h, ufoErrParseQuit, NULL);
        case ABF_FAIL_RET:
            fatal(h, ufoErrParseFail, NULL);
    }
	result = h->metrics.cb.beg(&h->metrics.cb, &h->metrics.gi);
    width = getWidth(h, (STI)gi->tag);
    glyph_cb->width(glyph_cb, (float)width );
    if (result == ABF_WIDTH_RET)
        return ufoSuccess;
    
    parseGLIF(h, gi, glyph_cb, NULL);
    doOpList(h, gi, glyph_cb);
    h->stm.src = NULL;
    doOp_ed(h, glyph_cb); //  we do this outside of parseGLIFOutline so that we can use parseGLIFOutline to process component glyphs.

    /* set the FontBBox field in the abfTopDict */
	h->top.FontBBox[0] = h->aggregatebounds.left;
	h->top.FontBBox[1] = h->aggregatebounds.bottom;
	h->top.FontBBox[2] = h->aggregatebounds.right;
	h->top.FontBBox[3] = h->aggregatebounds.top;
    
    /* clear out any point names from the last run */
    if (h->data.opList.cnt > 0)
    {
        for (i=0; i < h->data.opList.cnt; i++)
        {
            OpRec* opRec =  &h->data.opList.array[i++];
			if (opRec->pointName != NULL)
			{
			memFree(h,  opRec->pointName);
            opRec->pointName = NULL;
			}
        }
		h->data.opList.cnt = 0; /* zero the list of ops in the glyph */
    }
    /* reset all the hint masks */
    if (h->hints.hintMasks.cnt > 0)
    {
        for (i=0; i < h->hints.hintMasks.cnt; i++)
        {
             HintMask* curMask = dnaINDEX(h->hints.hintMasks, i );
            curMask->maskStems.cnt = 0;
			if (curMask->pointName != NULL)
            {
                memFree(h, curMask->pointName);
                curMask->pointName = NULL;
            }
 
        }
		h->hints.hintMasks.cnt = 0;
    }

	/* clear the list of flex names */
	 if (h->hints.flexOpList.cnt > 0)
	 {
       for (i=0; i < h->hints.flexOpList.cnt; i++)
       {
		   char* flexPointName = h->hints.flexOpList.array[i++];
		   memFree(h, flexPointName);
	   }
	   h->hints.flexOpList.cnt = 0;
	 }
    
	return ufoSuccess;
}

/* --------------------------- String Management --------------------------- */

/* Initialize strings. */
static void newStrings(ufoCtx h)
{
	dnaINIT(h->dna, h->strings.index, 50, 200);
	dnaINIT(h->dna, h->strings.buf, 32000, 6000);
}

/* Reinitilize strings for new font. */
static void initStrings(ufoCtx h)
{
	h->strings.index.cnt = 0;
	h->strings.buf.cnt = 0;
}

/* Free strings. */
static void freeStrings(ufoCtx h)
{
	dnaFREE(h->strings.index);
	dnaFREE(h->strings.buf);
}

/* Add string. */
/* 64-bit warning fixed by type change here HO */
/* static STI addString(ufoCtx h, unsigned length, const char *value) */
static STI addString(ufoCtx h, size_t length, const char *value)
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
static char *getString(ufoCtx h, STI sti)
{
	return &h->strings.buf.array[h->strings.index.array[sti]];
}

/* ----------------------Width management -----------------------*/
static void addWidth(ufoCtx h, STI sti, long value)
{
	if (sti != h->chars.widths.cnt)
	{
		fatal(h, ufoErrParse, "Width index does not match glyph name index. Glyph index %d.", sti);
	}
	*dnaNEXT(h->chars.widths) = value;
    
}

static long getWidth(ufoCtx h, STI sti)
{
	return h->chars.widths.array[sti];
}

static void setWidth(ufoCtx h, STI sti, long value)
{
	h->chars.widths.array[sti] = value;
}

/* ----------------------Char management -----------------------*/

/* Match glyph name. */
static int CTL_CDECL matchChar(const void *key, const void *value, void *ctx)
{
	ufoCtx h = ctx;
	return strcmp((char *)key, getString(h, (STI)h->chars.index.array
										 [*(long *)value].tag) );
}

/* Add char record. Return 1 if record exists else 0. Char record returned by
 "chr" parameter. */
static int addChar(ufoCtx h, STI sti, Char **chr)
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
static Char *findChar(ufoCtx h, STI sti)
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
int ufoBegFont(ufoCtx h, long flags, abfTopDict **top,  char *altLayerDir)
{
	int result;
    char *dummy;
    
	/* Set error handler */
  DURING_EX(h->err.env)
  
    /* Initialize */
    abfInitTopDict(&h->top);
    abfInitFontDict(&h->fdict);
      
    h->top.FDArray.cnt = 1;
    h->top.FDArray.array = &h->fdict;
      
    /* init glyph data structures used */
    h->valueArray.cnt = 0;
    h->chars.index.cnt = 0;
    h->data.glifRecs.cnt = 0;
    h->data.opList.cnt = 0;
    h->hints.hintMasks.cnt = 0;
      
    h->aggregatebounds.left = 0.0;
    h->aggregatebounds.bottom = 0.0;
    h->aggregatebounds.right = 0.0;
    h->aggregatebounds.top = 0.0;
      
    h->metrics.cb = abfGlyphMetricsCallbacks;
    h->metrics.cb.direct_ctx = &h->metrics.ctx;
    h->metrics.ctx.flags = 0x0;
      
      if (altLayerDir != NULL)
          h->altLayerDir = altLayerDir;
      
      dummy = *dnaGROW(h->valueArray,14);
      
    result = parseUFO(h);
    if (result)
      fatal(h,result, NULL);
      
    prepClientData(h);
    *top = &h->top;

	HANDLER
  	result = Exception.Code;
  END_HANDLER
  
	return result;
}

int ufoEndFont(ufoCtx h)
{
	if ( h->stm.src )
		h->cb.stm.close(&h->cb.stm, h->stm.src);
	memFree(h,  h->top.FullName.ptr);
	return ufoSuccess;
}

int ufoIterateGlyphs(ufoCtx h, abfGlyphCallbacks *glyph_cb)
{
	unsigned short i;
	int res;
    
	/* Set error handler */
  DURING_EX(h->err.env)
  
    for (i = 0; i < h->chars.index.cnt; i++)
      {
      res = readGlyph(h, i, glyph_cb);
      if (res != ufoSuccess)
        return res;
      }
  
  HANDLER
  	return Exception.Code;
  END_HANDLER
  
	return ufoSuccess;
}

int ufoGetGlyphByTag(ufoCtx h, unsigned short tag, abfGlyphCallbacks *glyph_cb)
{
	int res =  ufoSuccess;
    
	if (tag >= h->chars.index.cnt)
		return ufoErrNoGlyph;
    
	/* Set error handler */
  DURING_EX(h->err.env)
  
		res = readGlyph(h, tag, glyph_cb);
  
  HANDLER
  	res = Exception.Code;
  END_HANDLER
  
	return res;
}

/* Match glyph name after font fully parsed. */
static int CTL_CDECL postMatchChar(const void *key, const void *value,
								   void *ctx)
{
	ufoCtx h = ctx;
    char * testKey = getString(h, (STI)h->chars.index.array
                               [*(long *)value].tag);
	return strcmp((char *)key, testKey );
}

int ufoGetGlyphByName(ufoCtx h, char *gname, abfGlyphCallbacks *glyph_cb)
{
	size_t index;
	int result;
    
	if (!ctuLookup(gname, h->chars.byName.array, h->chars.byName.cnt,
				   sizeof(h->chars.byName.array[0]), postMatchChar, &index, h))
		return ufoErrNoGlyph;
	
	/* Set error handler */
  DURING_EX(h->err.env)
  
	result = readGlyph(h,  (unsigned short)h->chars.byName.array[index], glyph_cb);

	HANDLER
  	result = Exception.Code;
  END_HANDLER

	return result;
}	

int ufoResetGlyphs(ufoCtx h)
{ 
	long i;
	
	for (i = 0; i < h->chars.index.cnt; i++)
		h->chars.index.array[i].flags &= ~((unsigned long)ABF_GLYPH_SEEN);
    
	return ufoSuccess;
}

void ufoGetVersion(ctlVersionCallbacks *cb)
{
	if (cb->called & 1<<UFR_LIB_ID)
		return;	/* Already enumerated */
    
	abfGetVersion(cb);
	dnaGetVersion(cb);
    
	cb->getversion(cb, UFO_VERSION, "uforead");
    
	cb->called |= 1<<UFR_LIB_ID;
}
