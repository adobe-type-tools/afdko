/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Simple top-level PostScript tokenizer. Discards comments and doesn't look
 * inside string, hex string, array, or procedure constructs.
 */

#include "pstoken.h"
#include "dynarr.h"
#include "ctutil.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ARRAY_LEN(a)	(sizeof(a)/sizeof(a[0]))

/* Selected PostScript lexical classes */
#define N_	(1<<0)	/* Newline (\n \r) */
#define W_	(1<<1)	/* Whitespace (\0 \t \n \f \r space) */ 
#define S_	(1<<2)	/* Special (delimeter: ( ) < > [ ] { } / %) */
#define D_	(1<<3)	/* Decimal digit (0-9)*/
#define P_	(1<<4)	/* Decimal point (period) */
#define G_	(1<<5)	/* Sign (+ -) */
#define E_	(1<<6)	/* Exponent (E e) */

/* Index by ascii character and return lexical class(es) */
static char class[256] =
	{
	   W_,	   0,	  0,	 0,		0,	   0,	  0,	 0,	 /* 00-07 */
		0,	  W_, W_|N_,	 0,	   W_, W_|N_,	  0,	 0,	 /* 08-0f */
		0,	   0,	  0,	 0,		0,	   0,	  0,	 0,	 /* 10-17 */
		0,	   0,	  0,	 0,		0,	   0,	  0,	 0,	 /* 18-1f */
	   W_,	   0,	  0,	 0,		0,	  S_,	  0,	 0,	 /* 20-27 */
	   S_,	  S_,	  0,	G_,		0,	  G_,	 P_,	S_,	 /* 28-2f */
	   D_,	  D_,	 D_,	D_,	   D_,	  D_,	 D_,	D_,	 /* 30-37 */
	   D_,	  D_,	  0,	 0,	   S_,	   0,	 S_,	 0,	 /* 38-3f */
		0,	   0,	  0,	 0,     0,	  E_,	  0,	 0,	 /* 40-47 */
	    0,	   0,	  0,	 0,	    0,	   0,	  0,	 0,	 /* 48-4f */
	    0,	   0,	  0,	 0,	    0,	   0,	  0,	 0,	 /* 50-57 */
	    0,	   0,	  0,	S_,		0,	  S_,	  0,	 0,	 /* 58-5f */
		0,	   0,	  0,	 0,     0,	  E_,	  0,	 0,	 /* 60-67 */
	    0,	   0,	  0,	 0,	    0,	   0,	  0,	 0,	 /* 68-6f */
	    0,	   0,	  0,	 0,	    0,	   0,	  0,	 0,	 /* 70-77 */
	    0,	   0,	  0,	S_,		0,	  S_,	  0,	 0,	 /* 78-7f */
	/* Rest are zero */
	};

#define IS_WHITE(c)		(class[(int)(c)]&W_)
#define IS_NEWLINE(c)	(class[(int)(c)]&N_)
#define IS_DELIMETER(c)	(class[(int)(c)]&(S_|W_))
#define IS_NUMBER(c)	(class[(int)(c)]&(D_|G_|P_))
#define IS_SIGN(c)		(class[(int)(c)]&G_)
#define IS_EXPONENT(c)	(class[(int)(c)]&E_)

/* Index by ascii char and return digit value (to radix 36) or error (99) */
static unsigned char digit[256] =
	{
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* 00-0f */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* 10-1f */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* 20-2f */
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,/* 30-3f */
     99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,/* 40-4f */
     25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 99, 99, 99, 99, 99,/* 50-5f */
     99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,/* 60-6f */
     25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 99, 99, 99, 99, 99,/* 70-7f */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* 80-8f */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* 90-9f */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* a0-af */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* b0-bf */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* c0-cf */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* d0-df */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* e0-ef */
     99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,/* f0-ff */
	};

#define IS_DIGIT(c)		(digit[(int)(c)]<10)
#define IS_HEX(c)		(digit[(int)(c)]<16)
#define IS_RADIX(c,b)	(digit[(int)(c)]<(b))

/* Parse context */
struct pstCtx_
	{
	long flags;				/* Control flags */
#define FIRST_COMMENT (1<<0)/* Flags waiting for first comment */
	int errcode;			/* Return code */
	struct					/* Source stream */
		{					
		int id;				/* Client's stream identifier */
		void *stm;			/* Client's stream object */
		char *next;			/* Remaining source buffer */
		size_t left;		/* Bytes left in source buffer */
		int (*refill)(pstCtx h);/* Buffer refill function */
		} src;
	struct					/* Decryption data */
		{
		short binary;		/* Binary eexec */
		char *buf;			/* Cipher buffer */
		size_t length;		/* Cipher buffer length */
		unsigned short r;	/* Decryption state */
		short hi_nib;		/* High nibble split across buffers (-1 if none) */
		} cipher;
	dnaDCL(char, plain);	/* Decrypted cipher buffer */
	dnaDCL(char, tmp);		/* Temporary buffer */
	char *mark;				/* Buffer position marker */
	struct					/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	dnaCtx dna;				/* NULL-returning dynarr lib context */
	int dump;				/* Dump level */
	};

/* --------------------------- Context Management -------------------------- */

/* Begin new file parse */
pstCtx pstNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  int src_stream_id, CTL_CHECK_ARGS_DCL)
	{
	pstCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(PST_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct pstCtx_));
	if (h == NULL)
		return NULL;

	/* Initialize context */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	h->plain.size = 0;
	h->tmp.size = 0;
	h->src.id = src_stream_id;
	h->src.stm = NULL;

	/* Initialize dynamic array library */
	h->dna = dnaNew(mem_cb, DNA_CHECK_ARGS);
	if (h->dna == NULL)
		goto cleanup;

	dnaINIT(h->dna, h->plain, 500, 1000);
	dnaINIT(h->dna, h->tmp, 500, 1000);

	h->errcode = pstSuccess;

	h->dump = 0;	/* Turn off dumping */

	return h;

 cleanup:
	pstFree(h);
	return NULL;
	}

/* Free library context. */
int pstFree(pstCtx h)
	{
	if (h == NULL)
		return pstSuccess;

	dnaFREE(h->plain);
	dnaFREE(h->tmp);

	dnaFree(h->dna);

	h->cb.mem.manage(&h->cb.mem, h, 0);

	return pstSuccess;
	}

/* ------------------------ Source Data Management ------------------------- */

/* Decrypt ASCII source buffer to plain buffer. Return 1 on error else 0. */
static int ascii_decrypt(pstCtx h, size_t length, char *buf)
	{
	int hi_nib = h->cipher.hi_nib;
	char *end = buf + length;
	char *src = buf;
	char *dst;

	/* Set plain text buffer size */
	/* 64-bit warning fixed by cast here */
	if (dnaSetCnt(&h->plain, 1, (long)((length + (hi_nib != -1)) / 2)))
		{
		h->errcode = pstErrNoMemory;
		return 1;
		}

	/* Decrypt and copy buffer */
	dst = h->plain.array;
	do
		{
		int nib = digit[(int)(*src++)];
		if (nib > 15)
			continue;
		else if (hi_nib == -1)
			hi_nib = nib;
		else
			{
			unsigned char cipher = hi_nib<<4 | nib;
			*dst++ = cipher ^ (h->cipher.r>>8);
			h->cipher.r = (cipher + h->cipher.r)*52845 + 22719;
			hi_nib = -1;
			}
		} 
	while (src < end);

	/* Save possible odd nibble that's split across buffers */
	h->cipher.hi_nib = hi_nib;

	/* 64-bit warning fixed by cast here */
	h->plain.cnt = (long)(dst - h->plain.array);
	return 0;
	}

/* Decrypt binary source buffer to plain buffer. Return 1 on error else 0. */
static int binary_decrypt(pstCtx h, size_t length, char *buf)
	{
	size_t i;
	char *src = buf;
	char *dst;

	/* Set plain text buffer size */
	/* 64-bit warning fixed by cast here */
	if (dnaSetCnt(&h->plain, 1, (long)length))
		{
		h->errcode = pstErrNoMemory;
		return 1;
		}

	/* Decrypt cipher buffer */
	dst = h->plain.array;
	for (i = 0; i < length; i++)
		{
		unsigned char cipher = *src++;
		*dst++ = cipher ^ (h->cipher.r>>8);
		h->cipher.r = (cipher + h->cipher.r)*52845 + 22719;
		}

	return 0;
	}

/* Save next part of buffer-spanning token. Return 1 on error else 0. */
static int saveTokenPart(pstCtx h)
	{
	/* 64-bit warning fixed by cast here */
	long length = (long)(h->src.next - h->mark);
	if (length != 0)
		{
		/* Accommodate length + null termination */
		long i = dnaExtend(&h->tmp, 1, length + 1);
		if (i == -1)
			{
			h->errcode = pstErrNoMemory;
			return 1;
			}

		/* Copy buffer and null terminate */
		memcpy(&h->tmp.array[i], h->mark, length);
		h->tmp.array[--h->tmp.cnt] = '\0';
		}
	return 0;
	}

/* Refill from plain text source. Return -1 on error else next char. */
static int plain_refill(pstCtx h)
	{
	if (saveTokenPart(h))
		return -1;
	h->src.left = h->cb.stm.read(&h->cb.stm, h->src.stm, &h->src.next);
	h->mark = h->src.next;
	if (h->src.left-- == 0)
		{
		h->errcode = pstErrSrcStream;
		return -1;
		}
	return (*h->src.next++)&0xff;
	}

/* Refill from binary eexec source. Return -1 on error else next char. */
static int eexec_refill(pstCtx h)
	{
	if (saveTokenPart(h))
		return -1;

	/* Refill cipher buffer */
	h->cipher.length = h->cb.stm.read(&h->cb.stm, h->src.stm, &h->cipher.buf);
	if (h->cipher.length == 0)
		{
		h->errcode = pstErrSrcStream;
		return -1;
		}
	
	/* Decrypt cipher buffer to plain buffer */
	if ((h->cipher.binary?
		 binary_decrypt(h, h->cipher.length, h->cipher.buf):
		 ascii_decrypt(h, h->cipher.length, h->cipher.buf)))
		return -1;
	h->mark = h->plain.array;

	/* Set source to plain text buffer */
	h->src.left = h->plain.cnt - 1;
	h->src.next = h->plain.array + 1;
	return h->plain.array[0]&0xff;
	}

/* Read one source character. Return -1 on error else next char. */
#define read1(h) ((h->src.left--==0)?h->src.refill(h):(*h->src.next++)&0xff)

/* Unread lookahead character. */
static void unread1(pstCtx h)
	{
	h->src.left++;
	h->src.next--;
	}

/* Set eexec decryption. */
int pstSetDecrypt(pstCtx h)
	{
	int i;
	int c;
	char rand[8];	/* Initial random bytes */

	if (h->src.refill == eexec_refill)
		return h->errcode = pstErrBadCall;	/* Already decrypting */

	/* Discard eexec delimeter */
	if (read1(h) == -1)
		return h->errcode;

	h->errcode = pstSuccess;
	h->cipher.binary = 0;

	/* Determine encryption type. (See rasterizer's parse.c for rationale.) */
	for (i = 0; i < 4; i++)
		{
		c = read1(h);
		if (c == -1)
			return h->errcode;
		else if (!IS_HEX(c) && c != ' ' && c != '\t' && c != '\n' && c != '\r')
			h->cipher.binary = 1;
		rand[i] = c;
		}

	/* Set initial encryption state */
	h->cipher.r = 55665;

	if (h->cipher.binary)
		{
		/* Binary eexec; initialize decryption state from random bytes */
		if (binary_decrypt(h, 4, rand))
			return h->errcode;
		}
	else
		{
		/* ASCII eexec; discard eexec delimiting whitespace (likely for DOS) */
		while (rand[0] == ' ' || 
			   rand[0] == '\t' || 
			   rand[0] == '\n' || 
			   rand[0] == '\r')
			{
			for (i = 0; i < 3; i++)
				rand[i] = rand[i + 1];
			c = read1(h);
			if (c == -1)
				return h->errcode;
			rand[3] = c;
			}

		/* Read remaining random nibbles */
		for (i = 4; i < 8; i++)
			{
			c = read1(h);
			if (c == -1)
				return h->errcode;
			rand[i] = c;
			}

		/* Initialize decryption state from random nibbles */
		h->cipher.hi_nib = -1;
		if (ascii_decrypt(h, 8, rand))
			return h->errcode;
		}

	/* Save source buffer in case of restore */
	h->cipher.length = h->src.left;
	h->cipher.buf = h->src.next;

	h->src.refill = eexec_refill;
	if (h->src.left > 0)
		{
		/* Decrypt remaining source buffer */
		if (h->cipher.binary? 
			binary_decrypt(h, h->src.left, h->src.next):
			ascii_decrypt(h, h->src.left, h->src.next))
			return h->errcode;
		
		/* Set new source buffer */
		h->src.left = h->plain.cnt;
		h->src.next = h->mark = h->plain.array;
		}

	return h->errcode;
	}

/* Remove eexec decryption. */
int pstSetPlain(pstCtx h)
	{
	if (h->src.refill == plain_refill)
		return h->errcode = pstErrBadCall;	/* Wasn't decrypting */

	h->src.next++;	/* Skip the last encrypted token delimiter */

	/* Restore partial cipher buffer */
	if (h->cipher.binary)
		h->src.next = h->cipher.buf + (h->cipher.length - h->src.left);
	else
		{
		/* Find corresponding position in cipher buffer */
		/* 64-bit warning fixed by cast here */
		long cnt = (long)((h->src.next - h->plain.array)*2);
		char *src = h->cipher.buf;
		while (cnt > 0)
			if (digit[(int)(*src++)] <= 15)
				cnt--;
		h->src.next = src;
		h->src.left = h->cipher.length - (src - h->cipher.buf);
		}

	h->src.refill = plain_refill;

	return h->errcode = pstSuccess;
	}

/* Begin parse by opening source stream. */
int pstBegParse(pstCtx h, long origin)
	{
	if (h->src.stm != NULL)
		return h->errcode = pstErrBadCall;	/* Already open */
	h->flags = FIRST_COMMENT;
	h->src.next = h->mark = NULL;
	h->src.left = 0;
	h->src.refill = plain_refill;
	h->src.stm = h->cb.stm.open(&h->cb.stm, h->src.id, 0);
	if (h->src.stm == NULL || h->cb.stm.seek(&h->cb.stm, h->src.stm, origin))
		return h->errcode = pstErrSrcStream;
	return h->errcode = pstSuccess;
	}

/* End parse by closing source stream. */
int pstEndParse(pstCtx h)
	{
	if (h->src.stm == NULL)
		return h->errcode = pstErrBadCall;	/* Not open */
	if (h->cb.stm.close(&h->cb.stm, h->src.stm) == -1)
		h->errcode = pstErrSrcStream;
	h->src.stm = NULL;
	return h->errcode = pstSuccess;
	}

/* --------------------------- PostScript Parser --------------------------- */

/* Skip to delimiter and push it. Return 1 on error else 0. */
static int skip2Delim(pstCtx h)
	{
	for (;;)
		{
		int c = read1(h);
		if (c == -1)
			return 1;
		else if (IS_DELIMETER(c))
			break;
		}
	unread1(h);
	return 0;
	}

/* Skip comment line and push newline. Return 1 on error else 0.*/
static int skipComment(pstCtx h)
	{
	for (;;)
		{
		int c = read1(h);
		if (c == -1)
			return 1;
		else if (IS_NEWLINE(c))
			break;
		}
	unread1(h);
	return 0;
	}

/* Skip over string token. Return 1 on error else 0. */
static int skipString(pstCtx h)
	{
	int cnt = 1;	/* Already seen '(' */
	do 
		switch (read1(h))
			{
		case '\\':
			 /* Skip escaped character */
			if (read1(h) == -1)
				return 1;
			break;
		case '(':
			cnt++;
			break;
		case ')':
			cnt--;
			break;
		case -1:
			return 1;
			}
	while (cnt > 0);
	return 0;
	}

/* Skip procedure type. Return 1 on error else 0. */
static int skipProcedure(pstCtx h)
	{
	int cnt = 1;	/* Already seen '{' */
	do
		switch (read1(h))
			{
		case '%':
			if (skipComment(h))
				return 1;
			break;
		case '(':
			if (skipString(h))
				return 1;
			break;
		case '{':
			cnt++;
			break;
		case '}':
			cnt--;
			break;
		case -1:
			return 1;
			}
	while (cnt > 0);
	return 0;
	}

/* Skip array type. Return 1 on error else 0. */
static int skipArray(pstCtx h)
	{
	int cnt = 1;	/* Already seen '[' */
	do
		switch (read1(h))
			{
		case '%':
			if (skipComment(h))
				return 1;
			break;
		case '(':
			if (skipString(h))
				return 1;
			break;
		case '[':
			cnt++;
			break;
		case ']':
			cnt--;
			break;
		case -1:
			return 1;
			}
	while (cnt > 0);
	return 0;
	}

static int skipAngle(pstCtx h);

/* Skip dictionary. Return 1 on error else 0. */
static int skipDictionary(pstCtx h)
	{
	for (;;)
		switch (read1(h))
			{
		case '>':
			switch (read1(h))
				{
			case '>':
				return 0;
			case -1:
				return 1;
				}
			unread1(h);
			break;
		case '%':
			if (skipComment(h))
				return 1;
			break;
		case '(':
			if (skipString(h))
				return 1;
			break;
		case '<':
			if (skipAngle(h))
				return 1;
			break;
			}
	}

/* Skip angle-delimited object. Already seen '<'. Return -1 on error else 
   token type. */
static int skipAngle(pstCtx h)
	{
	int c = read1(h);
	switch (c)
		{
	case '<':
		return skipDictionary(h)? -1: pstDictionary;
	case '~':
		/* Skip ASCII 85 string */
		for (;;)
			{
			c = read1(h);
			if (c == '~')
				switch (read1(h))
					{
				case '>':
					return pstASCII85;
				case -1:
					h->errcode = pstErrBadASCII85;
					return -1;
					}
			else if ((c < '!' || c > 'u') && !IS_WHITE(c) && c != 'z')
				{
				h->errcode = pstErrBadASCII85;
				return -1;
				}
			}
	default:
		{
		/* Skip hexadecimal string */
		do
			{
			if (!IS_HEX(c) && !IS_WHITE(c))
				{
				h->errcode = pstErrBadHexStr;
				return -1;
				}
			c = read1(h);
			}
		while (c != '>');
		return pstHexString;
		}
	case -1:
		break;
		}
	return -1;
	}

/* Check if token is real or integer number or something else. In the following
 * state table 'd' stands for single digit in the input column and one or more
 * digits in the regexp column. EOT stands for End Of Token, i.e. anything that
 * doesn't match in a particular state. State 0 is the initial state.
 *
 *	State	Input	Next	Regexp(s)							Type
 *	-----	-----	----	---------							----
 *	0		d		1
 *			+,-		2
 *			.		3
 *	1		d		1
 *			.		4
 *			#		5
 *			EOT		-		d									int
 *	2		.		3
 *			d		6
 *			EOT		-		[+-]								operator
 *	3		d		7
 *			EOT		-		[+-]?\.								operator
 *	4		d		7
 *			E,e		8
 *			EOT		-		d\.									real
 *	5		d		9
 *			EOT		-		d#									operator
 *	6		d		6
 *			.		7
 *			EOT		-		[+-]d								int
 *	7		d		7
 *			E,e		8
 *			EOT		-		\.d	 d\.d  [+-](\.d|d\.|d\.d)		real
 *	8		d		10		
 *			+,-		11
 *			EOT		-		(d\.|\.d|d\.d)[Ee]					operator
 *	9		d		9
 *			EOT		-		d#d									int
 *	10		d		10
 *			EOT		-		[+-]?(\.d|d\.|d\.d)[Ee][+-]?d		real
 *	11		d		10
 *			EOT		-		(d\.|\.d|d\.d)[Ee][+-]				operator
 *
 * Return -1 on error else token type. */
static int skipNumber(pstCtx h, int c)
	{
	int state;

	/* Determine initial state */
	if (IS_DIGIT(c))
		state = 1;
	else if (IS_SIGN(c))
		state = 2;
	else if (c == '.')
		state = 3;
	else
		goto operator;

	for (;;)
		{
		c = read1(h);
		if (c == -1)
			return -1;

		if (IS_DELIMETER(c))
			/* Determine token type by examining finish state (d = [0-9]+) */
			switch (state)
				{
			case 2:		/* [+-] */
			case 3:		/* [+-]?\. */
			case 5:		/* d# */
			case 8:		/* (d.|.d|d.d)[Ee] */
			case 11:	/* (d.|.d|d.d)[Ee][+-] */
				/* Nearly, but not quite, a number */
				unread1(h);
				return pstOperator;
			case 1:		/* d */
			case 6:		/* [+-]d */
			case 9:		/* d#d */
				unread1(h);
				return pstInteger;
			case 4:		/* d. */
			case 7:		/* .d  d.d	[+-](.d|d.|d.d) */
			case 10:	/* [+-]{0,1}(.d|d.|d.d)[Ee][+-]?d */
				unread1(h);
				return pstReal;
				}

		/* Determine next state */
		switch (state)
			{
		case 1:
			if (c == '.')
				state = 4;
			else if (c == '#')
				state = 5;
			else if (!IS_DIGIT(c))
				goto operator;
			break;
		case 2:
			if (IS_DIGIT(c))
				state = 6;
			else if (c == '.')
				state = 3;
			else
				goto operator;
			break;
		case 3:
			if (IS_DIGIT(c))
				state = 7;
			else
				goto operator;
			break;
		case 4:
			if (IS_DIGIT(c))
				state = 7;
			else if (IS_EXPONENT(c))
				state = 8;
			else
				goto operator;
			break;
		case 5:
			if (IS_RADIX(c, 36))
				state = 9;
			else
				goto operator;
			break;
		case 6:
			if (c == '.')
				state = 7;
			else if (!IS_DIGIT(c))
				goto operator;
			break;
		case 7:
			if (IS_EXPONENT(c))
				state = 8;
			else if (!IS_DIGIT(c))
				goto operator;
			break;
		case 8:
			if (IS_DIGIT(c))
				state = 10;
			else if (IS_SIGN(c))
				state = 11;
			else
				goto operator;
			break;
		case 9:
			if (!IS_RADIX(c, 36))
				goto operator;
			break;
		case 10:
			if (!IS_DIGIT(c))
				goto operator;
			break;
		case 11:
			if (IS_DIGIT(c))
				state = 10;
			else
				goto operator;
			break;
			}
		}

	/* Non-numeric character encountered, skip to delimeter */
 operator:
	return skip2Delim(h)? -1: pstOperator;
	}

/* Get next PostScript token. */
int pstGetToken(pstCtx h, pstToken *token)
	{
	int c;
	int type;

	h->errcode = pstSuccess;

	/* Skip comments and whitespace */
	for (;;)
		{
		c = read1(h);
		if (c == -1)
			return h->errcode;
		else if (IS_WHITE(c))
			continue;
		else if (c == '%')
			{
			int first = h->flags & FIRST_COMMENT;
			h->flags &= ~FIRST_COMMENT;

			/* Mark token start */
			h->tmp.cnt = 0;
			h->mark = h->src.next - 1;

			c = read1(h);
			if (c == -1)
				return h->errcode;
			else if (c == '!')
				{
				if (skipComment(h))
					return h->errcode;
				else if (first)
					{
					type = pstDocType;
					goto finish;
					}
				}
			else if (!IS_NEWLINE(c))
				{
				if (skipComment(h))
					return h->errcode;
				}
			}
		else
			break;
		}

	/* Mark token start */
	h->tmp.cnt = 0;
	h->mark = h->src.next - 1;

	switch (c)
		{
	case '/':
		c = read1(h);
		switch (c)
			{
		case '/':
			type = pstImmediate;
			c = read1(h);
			break;
		default:
			type = pstLiteral;
			break;
		case -1:
			return h->errcode;
			}

		/* skip2Delim() won't do here because might have null name object */
		for (;;)
			if (c == -1)
				return h->errcode;
			else if (IS_DELIMETER(c))
				break;
			else
				c = read1(h);
		unread1(h);
		break;
	case '{':
		if (skipProcedure(h))
			return h->errcode;
		type = pstProcedure;
		break;
	case '(':
		if (skipString(h))
			return h->errcode;
		type = pstString;
		break;
	case '[':
		if (skipArray(h))
			return h->errcode;
		type = pstArray;
		break;
	case '<':
		type = skipAngle(h);
		if (type == -1)
			return h->errcode;
		break;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9':
	case '.': case '+': case '-':
		type = skipNumber(h, c);
		if (type == -1)
			return h->errcode;
		break;
	default:
		/* Executable name */
		if (skip2Delim(h))
			return h->errcode;
		type = pstOperator;
		break;
		}

 finish:
	/* Make token */
	token->type = (pstType)type;
	if (h->tmp.cnt != 0)
		{
		/* Buffer-spanning token */
		if (saveTokenPart(h))
			return h->errcode;
		token->value = h->tmp.array;
		token->length = h->tmp.cnt;
		}
	else
		{
		token->value = h->mark;
		/* 64-bit warning fixed by cast here */
		token->length = (long)(h->src.next - h->mark);
		}

	if (h->dump)
		pstDumpToken(token);

	return h->errcode;
	}

/* Match token's value. Return 1 on match else 0. */
int pstMatch(pstCtx h, pstToken *token, char *value)
	{
	/* 64-bit warning fixed by cast here */
	long length = (long)strlen(value);
	return token->length == length && memcmp(token->value, value, length) == 0;
	}

/* Find token of type and value. Return 0 on success else error code. */
int pstFindToken(pstCtx h, pstToken *token, char *value)
	{
	h->errcode = pstSuccess;
	for (;;)
		if (pstGetToken(h, token) || pstMatch(h, token, value))
			break;
	return h->errcode;
	}

/* Convert integer token to integer value. */
long pstConvInteger(pstCtx h, pstToken *token)
	{
	int base = 10;
	long value = 0;
	char *p = token->value;
	char *end = token->value + token->length;
	int neg = *p == '-';

	if (token->type != pstInteger)
		return 0;

	if (IS_SIGN(*p))
		p++;	/* Skip leading sign */

	do
		if (*p == '#')
			{
			/* It's a radix number so change base */
			base = value;
			value = 0;
			}
		else
			value = value*base + digit[(int)(*p)];
	while (++p < end);
	
	return neg? -value: value;
	}

/* Convert real token to double value. */
double pstConvReal(pstCtx h, pstToken *token)
	{
	if (token->type != pstReal)
		return 0.0;

	if (token->value != h->tmp.array)
		{
		/* Make null-terminated copy of value */
		int i = dnaSetCnt(&h->tmp, 1, token->length + 1);
		if (i == -1)
			return 0.0;	/* Can't get buffer space */
		memcpy(h->tmp.array, token->value, token->length);
		h->tmp.array[token->length] = '\0';
		}
	return ctuStrtod(h->tmp.array, NULL);
	}

/* Convert string token to string value. */
char *pstConvString(pstCtx h, pstToken *token, long *length)
	{
	if (token->type != pstString)
		return NULL;

	*length = token->length - 2;
	return token->value + 1;
	}

/* Convert literal token to string value. */
char *pstConvLiteral(pstCtx h, pstToken *token, long *length)
	{
	if (token->type != pstLiteral)
		return NULL;

	*length = token->length - 1;
	return token->value + 1;
	}

/* Convert hexadecimal string to integer value. Only hexadecimal strings of
   eight digits or fewer may be converted by this function. Longer strings are
   likely to cause undiagnosed overflow on certain platforms. */
unsigned long pstConvHexString(pstCtx h, pstToken *token)
	{
	char *p = token->value + 1;
	int digits = 0;
	long value = 0;

	if (token->type != pstHexString)
		return 0;

	do
		if (IS_HEX(*p))
			{
			value = value * 16 + digit[(int)(*p)];
			digits++;
			}
	while (*++p != '>');

	if (digits & 1)
		value *= 16;	/* Odd digit count implies 0 (See Red Book) */
	
	return value;  
	}

/* Get hexadecimal string length in digits. */
int pstGetHexLength(pstCtx h, pstToken *token)
	{
	int length;	
	char *p = token->value + 1;
	int digits = 0;

	if (token->type != pstHexString)
		return 0;

	do
		if (IS_HEX(*p))
			digits++;
	while (*++p != '>');

	length = (digits + 1) / 2;
	return length;
	}

/* Read specified number of bytes from input. */
int pstRead(pstCtx h, size_t count, char **ptr)
	{
	if (count <= h->src.left)
		{
		*ptr = h->src.next;
		h->src.next += count;
		h->src.left -= count;
		}
	else
		{
		/* Buffer-spanning read */
		h->tmp.cnt = 0;
		do
			{
			h->mark = h->src.next;
			h->src.next += h->src.left;
			count -= h->src.left;
			if (h->src.refill(h) == -1)
				return h->errcode = pstErrNoMemory;
			/* Put back last char */
			h->src.next--;
			h->src.left++;
			}
		while (h->src.left < count);

		h->src.next += count;
		h->src.left -= count;
		if (saveTokenPart(h))
			return h->errcode;
		*ptr = h->tmp.array;
		}

	return pstSuccess;
	}

/* Return stream and offset. */
void *pstHijackStream(pstCtx h, long *offset)
	{
	long where = h->cb.stm.tell(&h->cb.stm, h->src.stm);
	if (where == -1)
		return NULL;
	/* 64-bit warning fixed by cast here */
	*offset = where - (long)h->src.left;
	return h->src.stm;
	}

/* Get version numbers of libraries. */
void pstGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<PST_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	dnaGetVersion(cb);

	/* This library */
	cb->getversion(cb, PST_VERSION, "pstoken");

	/* Record this call */
	cb->called |= 1<<PST_LIB_ID;
	}

/* Map error code to error string. */
char *pstErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "psterr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

/* ----------------------------- Debug Support ----------------------------- */

/* Set debug level. */
void pstSetDumpLevel(pstCtx h, int level)
	{
	h->dump = level;
	}

/* Dump token. */
void pstDumpToken(pstToken *token)
	{
	static char *type[] =
		{
		"integer",
		"real",
		"literal",
		"immediate",
		"string",
		"hexstring",
		"ASCII85",
		"dictionary",
		"array",
		"procedure",
		"operator",
		"doctype",
		};
	
	if (token->length > 52)
		printf("{%-10s,%4ld,%.26s ... %.26s}\n",
			   type[token->type], 
			   token->length,
			   token->value,
			   token->value + token->length - 26);
	else
		printf("{%-10s,%4ld,%.*s}\n",
			   type[token->type], 
			   token->length,
			   (int)token->length,
			   token->value);
	}
