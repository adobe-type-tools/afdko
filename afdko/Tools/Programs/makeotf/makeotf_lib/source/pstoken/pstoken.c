/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Simple top-level PostScript tokenizer. Discards comments and doesn't look
 * inside string, hex string, array, or procedure constructs.
 */

#include <stdint.h>
#include "pstoken.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Selected PostScript lexical classes */
#define N_  (1 << 0)  /* Newline (\n \r) */
#define W_  (1 << 1)  /* Whitespace (\0 \t \n \f \r space) */
#define S_  (1 << 2)  /* Special (delimeter: ( ) < > [ ] { } / %) */
#define D_  (1 << 3)  /* Decimal digit (0-9)*/
#define P_  (1 << 4)  /* Decimal point (period) */
#define G_  (1 << 5)  /* Sign (+ -) */
#define E_  (1 << 6)  /* Exponent (E e) */

/* Index by ascii character and return lexical class(es) */
static char class[256] = {
	W_,     0,     0,     0,     0,     0,     0,     0,     /* 00-07 */
	0,    W_, W_ | N_,     0,    W_, W_ | N_,     0,     0,  /* 08-0f */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 10-17 */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 18-1f */
	W_,     0,     0,     0,     0,    S_,     0,     0,     /* 20-27 */
	S_,    S_,     0,    G_,     0,    G_,    P_,    S_,     /* 28-2f */
	D_,    D_,    D_,    D_,    D_,    D_,    D_,    D_,     /* 30-37 */
	D_,    D_,     0,     0,    S_,     0,    S_,     0,     /* 38-3f */
	0,     0,     0,     0,     0,    E_,     0,     0,      /* 40-47 */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 48-4f */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 50-57 */
	0,     0,     0,    S_,     0,    S_,     0,     0,      /* 58-5f */
	0,     0,     0,     0,     0,    E_,     0,     0,      /* 60-67 */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 68-6f */
	0,     0,     0,     0,     0,     0,     0,     0,      /* 70-77 */
	0,     0,     0,    S_,     0,    S_,     0,     0,      /* 78-7f */
	/* -- Rest are zero -- */
};

#define ISWHITE(c)      (class[(int)(c)] & W_)
#define ISNEWLINE(c)    (class[(int)(c)] & N_)
#define ISSPECIAL(c)    (class[(int)(c)] & S_)
#define ISDELIMETER(c)  (class[(int)(c)] & (S_ | W_))
#define ISNUMBER(c)     (class[(int)(c)] & (D_ | G_ | P_))
#define ISSIGN(c)       (class[(int)(c)] & G_)
#define ISEXPONENT(c)   (class[(int)(c)] & E_)

/* Index by ascii character and return digit value or error (255) */
static unsigned char digit[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 00-0f */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 10-1f */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 20-2f */
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 255, 255, 255, 255, 255, 255, /* 30-3f */
	255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 40-4f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 255, 255, 255, 255, 255, /* 50-5f */
	255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 60-6f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 255, 255, 255, 255, 255, /* 70-7f */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 80-8f */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 90-9f */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* a0-af */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* b0-bf */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* c0-cf */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* d0-df */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* e0-ef */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* f0-ff */
};

#define ISDIGIT(c)      (digit[(int)(c)] < 10)
#define ISHEX(c)        (digit[(int)(c)] < 16)
#define ISRADIX(c, b)    (digit[(int)(c)] < (b))

/* Parse context */
struct psCtx_ {
	psCallbacks cb;     /* Client callbacks */
	psToken token;      /* Last input token */
	int (*getch)(psCtx h, int check); /* Get next input char */
	int lookahead;      /* Lookahead occured on last token */
	int binary;         /* Flags binary input */
	int first;          /* Flags first line */
	unsigned short r;   /* Decryption state */
	int c;              /* Last character read */
	char *next;         /* Next byte available in input buffer */
	long left;          /* Number of bytes available in input buffer */
#if PSTOKEN_DEBUG
	int debug;          /* Dump PostScript tokens */
#endif  /* PSTOKEN_DEBUG */
};

/* Replenish input buffer using refill function and handle end of input */
static char fillbuf(psCtx h, int check) {
	h->next = h->cb.psRefill(h->cb.ctx, &h->left);
	if (h->left-- == 0) {
		if (check) {
			psFatal(h, "premature end of input");
		}
		else {
			return -1;  /* Signal end of input */
		}
	}
	return *h->next++;
}

/* Get character from PostScript input */
#define GETRAW(check) \
	((unsigned char)((h->left-- == 0) ? fillbuf(h, check) : *h->next++))

/* Get decrypted character from hexadecimal eexec encrypted font file */
static int hexdecrypt(psCtx h, int check) {
	int nibble;
	int cipher;
	int plain;

	/* Form cipher byte from 2 hexadecimal characters */
	while ((nibble = digit[(int)GETRAW(1)]) > 15) {
	}
	cipher = nibble << 4;

	while ((nibble = digit[(int)GETRAW(1)]) > 15) {
	}
	cipher |= nibble;

	/* Decrypt cipher byte */
	plain = *dnaNEXT(*h->cb.buf) = cipher ^ (h->r >> 8);
	h->r = (cipher + h->r) * 52845 + 22719;
	return plain;
}

/* Get decrypted character from binary eexec encrypted font file */
static int bindecrypt(psCtx h, int check) {
	int cipher = GETRAW(1);
	int plain = *dnaNEXT(*h->cb.buf) = cipher ^ (h->r >> 8);
	h->r = (cipher + h->r) * 52845 + 22719;
	return plain;
}

/* Set eexec decryption */
void psSetDecrypt(psCtx h) {
	int i;
	unsigned char c[4];
	int hex = 1;

	/* Determine encryption type (see rasterizer's parse.c for rationale) */
	for (i = 0; i < 4; i++) {
		c[i] = GETRAW(1);
		if (!ISHEX(c[i]) &&
		    c[i] != ' ' && c[i] != '\t' && c[i] != '\n' && c[i] != '\r') {
			hex = 0;
		}
	}
	h->r = 55665;
	if (hex) {
		/* Hex eexec: discard eexec delimiting whitespace (likely for DOS) */
		while (c[0] == ' ' || c[0] == '\t' || c[0] == '\n' || c[0] == '\r') {
			for (i = 0; i < 3; i++) {
				c[i] = c[i + 1];
			}
			c[3] = GETRAW(1);
		}
		/* Initialize decryption state */
		h->r = ((digit[c[0]] << 4 | digit[c[1]]) + h->r) * 52845 + 22719;
		h->r = ((digit[c[2]] << 4 | digit[c[3]]) + h->r) * 52845 + 22719;
		(void)hexdecrypt(h, 0);
		(void)hexdecrypt(h, 0);
		h->getch = hexdecrypt;
	}
	else {
		/* Binary eexec: initialize decryption state */
		for (i = 0; i < 4; i++) {
			h->r = (c[i] + h->r) * 52845 + 22719;
		}
		h->getch = bindecrypt;
	}
}

/* Get unencrypted character from font file */
static int getplain(psCtx h, int check) {
	return *dnaNEXT(*h->cb.buf) = GETRAW(check);
}

#define SKIPTODELIM do { c = h->getch(h, 1); } \
	while (!ISDELIMETER(c))
#define SKIPCOMMENT do { c = h->getch(h, 1); } \
	while (!ISNEWLINE(c))

/* Remove eexec decryption */
void psSetPlain(psCtx h) {
	h->getch = getplain;
}

/* Begin new file parse */
psCtx psNew(psCallbacks *cb) {
	/* Allocate context */
	psCtx h = cb->malloc(cb->ctx, sizeof(struct psCtx_));

	h->cb = *cb;
	h->left = 0;
	psSetPlain(h);
	h->lookahead = 0;
	h->binary = 0;
	h->first = 1;
#if PSTOKEN_DEBUG
	h->debug = 0;
#endif /* PSTOKEN_DEBUG */

	return h;
}

/* End file parse */
void psFree(psCtx h) {
	h->cb.free(h->cb.ctx, h);   /* Free context */
}

/* Skip over string token */
static void skipString(psCtx h) {
	int cnt = 1;    /* Already seen '(' */
	do {
		int c = h->getch(h, 1);
		if (c == '\\') {
			h->getch(h, 1); /* Read escape character */
		}
		else if (c == '(') {
			cnt++;
		}
		else if (c == ')') {
			cnt--;
		}
	}
	while (cnt > 0);
}

/* Skip array type */
static void skipArray(psCtx h, char begin, char end) {
	int cnt = 1;    /* Already seen begin character */
	do {
		int c = h->getch(h, 1);
		if (c == '%') {
			SKIPCOMMENT;
		}
		else if (c == '(') {
			skipString(h);
		}
		else if (c == begin) {
			cnt++;
		}
		else if (c == end) {
			cnt--;
		}
	}
	while (cnt > 0);
}

static int skipAngle(psCtx h);

/* Scipt dictionary */
static void skipDictionary(psCtx h) {
	for (;; ) {
		int c = h->getch(h, 1);
		if (c == '>' && (c = h->getch(h, 1)) == '>') {
			return;
		}
		if (c == '%') {
			SKIPCOMMENT;
		}
		else if (c == '(') {
			skipString(h);
		}
		else if (c == '<') {
			(void)skipAngle(h);
		}
	}
}

/* Skip angle-delimited object. Already seen '<' */
static int skipAngle(psCtx h) {
	int c = h->getch(h, 1);
	if (c == '<') {
		skipDictionary(h);
		return PS_DICTIONARY;
	}
	else if (c == '~') {
		/* Skip ASCII 85 string */
		for (;; ) {
			c = h->getch(h, 1);
			if (c == '~') {
				if (h->getch(h, 1) == '>') {
					return PS_ASCII85;
				}
				else {
					psFatal(h, "bad ASCII85 termination");
				}
			}
			else if ((c < '!' || c > 'u') && c != 'z') {
				psFatal(h, "bad ASCII85 character");
			}
		}
	}
	else {
		/* Skip hexadecimal string */
		do {
			if (!ISHEX(c) && !ISWHITE(c)) {
				psFatal(h, "bad hex string");
			}
			c = h->getch(h, 1);
		}
		while (c != '>');
		return PS_HEXSTRING;
	}
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
 */
static int checkForNumber(psCtx h, int c, short *type) {
	int state;

	if (ISDIGIT(c)) {
		state = 1;
	}
	else if (ISSIGN(c)) {
		state = 2;
	}
	else if (c == '.') {
		state = 3;
	}
	else {
		goto operator;
	}

	for (;; ) {
		c = h->getch(h, 0);

		if (c == EOF || ISDELIMETER(c)) {
			/* Determine token type by examining finish state (d = [0-9]+) */
			switch (state) {
				case 2: /* [+-] */
				case 3: /* [+-]?\. */
				case 5: /* d# */
				case 8: /* (d.|.d|d.d)[Ee] */
				case 11: /* (d.|.d|d.d)[Ee][+-] */
					/* Nearly, but not quite, a number */
					*type = PS_OPERATOR;
					return c;

				case 1: /* d */
				case 6: /* [+-]d */
				case 9: /* d#d */
					*type = PS_INTEGER;
					return c;

				case 4: /* d. */
				case 7: /* .d  d.d	[+-](.d|d.|d.d) */
				case 10: /* [+-]{0,1}(.d|d.|d.d)[Ee][+-]?d */
					*type = PS_REAL;
					return c;
			}
		}

		/* Determine next state */
		switch (state) {
			case 1:
				if (c == '.') {
					state = 4;
				}
				else if (c == '#') {
					state = 5;
				}
				else if (!ISDIGIT(c)) {
					goto operator;
				}
				break;

			case 2:
				if (ISDIGIT(c)) {
					state = 6;
				}
				else if (c == '.') {
					state = 3;
				}
				else {
					goto operator;
				}
				break;

			case 3:
				if (ISDIGIT(c)) {
					state = 7;
				}
				else {
					goto operator;
				}
				break;

			case 4:
				if (ISDIGIT(c)) {
					state = 7;
				}
				else if (ISEXPONENT(c)) {
					state = 8;
				}
				else {
					goto operator;
				}
				break;

			case 5:
				if (ISRADIX(c, 36)) {
					state = 9;
				}
				else {
					goto operator;
				}
				break;

			case 6:
				if (c == '.') {
					state = 7;
				}
				else if (!ISDIGIT(c)) {
					goto operator;
				}
				break;

			case 7:
				if (ISEXPONENT(c)) {
					state = 8;
				}
				else if (!ISDIGIT(c)) {
					goto operator;
				}
				break;

			case 8:
				if (ISDIGIT(c)) {
					state = 10;
				}
				else if (ISSIGN(c)) {
					state = 11;
				}
				else {
					goto operator;
				}
				break;

			case 9:
				if (!ISRADIX(c, 36)) {
					goto operator;
				}
				break;

			case 10:
				if (!ISDIGIT(c)) {
					goto operator;
				}
				break;

			case 11:
				if (ISDIGIT(c)) {
					state = 10;
				}
				else {
					goto operator;
				}
				break;
		}
	}

	/* Non-numeric character encountered, skip to delimeter */
operator:
	SKIPTODELIM;
	*type = PS_OPERATOR;
	return c;
}

#if PSTOKEN_DEBUG
static int DBToken(psCtx h) {
	char *name[] = {
		"EOI",
		"INTEGER",
		"REAL",
		"LITERAL",
		"IMMEDIATE",
		"STRING",
		"HEXSTRING",
		"ASCII85",
		"DICTIONARY",
		"ARRAY",
		"PROCEDURE",
		"DOCTYPE",
		"OPERATOR",
	};

	if (h->token.length > 40) {
		printf("--- token={%9s,<%.20s ... %.20s>,%ld,%ld}\n",
		       name[h->token.type], &h->cb.buf->array[h->token.index],
		       &h->cb.buf->array[h->token.index + h->token.length - 20],
		       h->token.index, h->token.length);
	}
	else {
		printf("--- token={%9s,<%.*s>,%ld,%ld}\n",
		       name[h->token.type], (int)h->token.length,
		       &h->cb.buf->array[h->token.index], h->token.index,
		       h->token.length);
	}

	return 0;   /* Returning a value satisfies the calling statement */
}

#endif /* PSTOKEN_DEBUG */

/* Get next token */
psToken *psGetToken(psCtx h) {
	int c = h->lookahead ? h->c : h->getch(h, 0);

	/* Skip comments and whitespace */
	for (;; ) {
		if (c == -1) {
			h->token.type = PS_EOI;
			return &h->token;
		}
		else if (ISWHITE(c)) {
			c = h->getch(h, 0);
		}
		else if (c == '%') {
			SKIPCOMMENT;
			if (h->first) {
				h->first = 0;
				if (h->cb.buf->array[0] != '%' || h->cb.buf->array[1] != '!') {
					break;  /* %!... Must begin line */
				}
				h->lookahead = 1;
				h->token.index = 0;
				h->token.type = PS_DOCTYPE;
				goto finish;
			}
		}
		else {
			break;
		}
	}

	h->token.index = h->cb.buf->cnt - 1;    /* Save token start */
	switch (c) {
		case '/':
			c = h->getch(h, 1);
			h->token.type = (c == '/') ? PS_IMMEDIATE : PS_LITERAL;
			/* SKIPTODELIM won't do here because might have null name object */
			while (!ISDELIMETER(c)) {
				c = h->getch(h, 1);
			}
			h->lookahead = 1;
			break;

		case '{':
			h->token.type = PS_PROCEDURE;
			skipArray(h, '{', '}');
			h->lookahead = 0;
			break;

		case '(':
			h->token.type = PS_STRING;
			skipString(h);
			h->lookahead = 0;
			break;

		case '[':
			h->token.type = PS_ARRAY;
			skipArray(h, '[', ']');
			h->lookahead = 0;
			break;

		case '<':
			h->token.type = skipAngle(h);
			h->lookahead = 0;
			break;

		default:
			/* We have a number or an executable name */
			if (ISNUMBER(c)) {
				c = checkForNumber(h, c, &h->token.type);
			}
			else {
				SKIPTODELIM;
				h->token.type = PS_OPERATOR;
			}
			h->lookahead = 1;
			break;
	}

	/* Compute token length */
finish:
	h->token.length = h->cb.buf->cnt - h->token.index - h->lookahead;

#if PSTOKEN_DEBUG
	if (h->debug) {
		DBToken(h);
	}
#endif /* PSTOKEN_DEBUG */

	h->c = c;
	return &h->token;
}

/* Match token */
int psMatchToken(psCtx h, psToken *token, int type, char *value) {
	size_t length = strlen(value);
	return token->type == type && token->length == (long)length &&
	       memcmp(&h->cb.buf->array[token->index], value, length) == 0;
}

/* Find token */
psToken *psFindToken(psCtx h, int type, char *value) {
	for (;; ) {
		psToken *token = psGetToken(h);
		if (psMatchToken(h, token, type, value)) {
			return token;
		}
		else if (token->type == PS_EOI) {
			psFatal(h, "premature EOF");
		}
	}
}

/* Match token's value */
int psMatchValue(psCtx h, psToken *token, char *value) {
	size_t length = strlen(value);
	return token->length == (long)length &&
	       memcmp(&h->cb.buf->array[token->index], value, length) == 0;
}

/* Get pointer to token value */
char *psGetValue(psCtx h, psToken *token) {
	return &h->cb.buf->array[token->index];
}

/* Convert integer token to integer value. Already validated */
int32_t psConvInteger(psCtx h, psToken *token) {
	int32_t base = 10;
	int32_t value = 0;
	char *p = &h->cb.buf->array[token->index];
	char *end = p + token->length;
	int neg = *p == '-';

	if (ISSIGN(*p)) {
		p++;
	}

	do {
		if (*p == '#') {
			/* It's a radix number so change base */
			base = value;
			value = 0;
		}
		else {
			value = value * base + digit[(int)(*p)];
		}
	}
	while (++p < end);

	return neg ? -value : value;
}

/* Get next token as an integer and convert its value */
int32_t psGetInteger(psCtx h) {
	psToken *value = psGetToken(h);

	if (value->type != PS_INTEGER) {
		psFatal(h, "expecting integer");
	}

	return psConvInteger(h, value);
}

/* Convert real token to double value. Already validated */
double psConvReal(psCtx h, psToken *token) {
	return strtod(&h->cb.buf->array[token->index], NULL);
}

/* Get next token as a real and convert its value */
double psGetReal(psCtx h) {
	psToken *value = psGetToken(h);

	if (value->type != PS_REAL) {
		psFatal(h, "expecting real");
	}

	return psConvReal(h, value);
}

/* Convert string token to string value. Already validated */
char *psConvString(psCtx h, psToken *token, unsigned *length) {
	*length = token->length - 2;
	return &h->cb.buf->array[token->index + 1];
}

/* Get next token as a string and convert its value */
char *psGetString(psCtx h, unsigned *length) {
	psToken *value = psGetToken(h);

	if (value->type != PS_STRING) {
		psFatal(h, "expecting string");
	}

	return psConvString(h, value, length);
}

/* Convert literal token to string value. Already validated */
char *psConvLiteral(psCtx h, psToken *token, unsigned *length) {
	*length = token->length - 1;
	return &h->cb.buf->array[token->index + 1];
}

/* Convert hexadecimal string to integer value. Already validated */
uint32_t psConvHexString(psCtx h, psToken *token) {
	char *p = &h->cb.buf->array[token->index + 1];
	int digits = 0;
	uint32_t value = 0;

	do {
		if (ISHEX(*p)) {
			value = value * 16 + digit[(int)(*p)];
			digits++;
		}
	}
	while (*++p != '>');

	if (digits & 1) {
		value *= 16;    /* Odd digit count implies 0 (See Red Book) */
	}
	return value;
}

/* Get hexadecimal string length in digits */
int psGetHexLength(psCtx h, psToken *token) {
	int length;
	char *p = &h->cb.buf->array[token->index + 1];
	int digits = 0;

	do {
		if (ISHEX(*p)) {
			digits++;
		}
	}
	while (*++p != '>');

	length = (digits + 1) / 2;
	return length;
}

/* Get next token as a hexadecimal string and convert its value */
uint32_t psGetHexString(psCtx h, int *length) {
	psToken *value = psGetToken(h);

	if (value->type != PS_HEXSTRING) {
		psFatal(h, "expecting hex string");
	}

	*length = psGetHexLength(h, value);
	return psConvHexString(h, value);
}

/* Read specified number of binary bytes from input */
void psReadBinary(psCtx h, long nBytes) {
	h->binary = 1;
	while (nBytes--) {
		h->getch(h, 1);
	}
	h->binary = 0;
}

/* Skip binary bytes of input without storing in input buffer. */
void psSkipBinary(psCtx h, long nBytes) {
	while (nBytes > h->left) {
		/* Reduce count by bytes left in the buffer and refill buffer */
		nBytes -= h->left;
		h->next = h->cb.psRefill(h->cb.ctx, &h->left);
		if (h->left == 0) {
			psFatal(h, "premature end of input");
		}
	}
	h->next += nBytes;
	h->left -= nBytes;
}

/* Print warning error message with input id appended */
void CDECL psWarning(psCtx h, char *fmt, ...) {
	if (h->cb.message != NULL) {
		va_list ap;
		char text[513];

		/* Format message */
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);

		if (h->cb.psId != NULL) {
			/* Append source data id */
			sprintf(&text[strlen(text)], " [%s]", h->cb.psId(h->cb.ctx));
		}

		h->cb.message(h->cb.ctx, psWARNING, text);
		va_end(ap);
	}
}

/* Print fatal error message with input id appended and quit */
void CDECL psFatal(psCtx h, char *fmt, ...) {
	void *ctx = h->cb.ctx;  /* Client context */

	if (h->cb.message != NULL) {
		/* Format message */
		va_list ap;
		char text[513];

		va_start(ap, fmt);
		vsprintf(text, fmt, ap);

		if (h->cb.psId != NULL) {
			/* Append source data id */
			sprintf(&text[strlen(text)], " [%s]", h->cb.psId(h->cb.ctx));
		}

		h->cb.message(ctx, psFATAL, text);
		va_end(ap);
	}
	h->cb.fatal(h->cb.ctx);
}
