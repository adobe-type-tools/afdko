/* @(#)CM_VerSion ps.h atm08 1.2 1.2 16245.eco sum= 45006 atm08.002 */
/* @(#)CM_VerSion ps.h atm07 1.2 1.2 16164.eco sum= 03481 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Simple top-level PostScript tokenizer.
 */

#ifndef PSTOKEN_H
#define PSTOKEN_H

#include <stddef.h>

#include "dynarr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define to supply Microsoft-specific function calling info, e.g. __cdecl */
#ifndef CDECL
#define CDECL
#endif

#define PSTOKEN_VERSION 0x010004    /* Library version */

/* PostScript token types. Comments/whitespace skipped inside tokenizer */
enum {
	PS_EOI,         /*  0: End of input */
	PS_INTEGER,     /*  1: Integer number */
	PS_REAL,        /*  2: Real number */
	PS_LITERAL,     /*  3: /... */
	PS_IMMEDIATE,   /*  4: //... */
	PS_STRING,      /*  5: ( ... ) */
	PS_HEXSTRING,   /*  6: < ... > */
	PS_ASCII85,     /*  7: <~ ... ~> */
	PS_DICTIONARY,  /*  8: << ... >> */
	PS_ARRAY,       /*  9: [ ... ] */
	PS_PROCEDURE,   /* 10: { ... } */
	PS_DOCTYPE,     /* 11: %!... (first line only) */
	PS_OPERATOR     /* 12: Executable name, i.e. not one of the above */
};

/* Input token */
typedef struct {
	short type;
	long index;
	long length;
} psToken;

typedef struct psCtx_ *psCtx;   /* Parse context */
typedef dnaDCL (char, psBuf);    /* Client's grow buffer */

/* Message types (for use with message callback) */
enum {
	psWARNING = 1,
	psERROR,
	psFATAL
};

/* Callbacks */
typedef struct {
	void *ctx;          /* Client's callback context (optional) */

	/* Exception handling */
	void (*fatal)(void *ctx);
	void (*message)(void *ctx, int type, char *text);       /* (optional) */

	/* Memory management */
	void *(*malloc)(void *ctx, size_t size);
	void (*free)(void *ctx, void *ptr);

	/* PostScript data input */
	char *(*psId)(void *ctx);           /* Data id (optional) */
	char *(*psRefill)(void *ctx, long *count);
	psBuf *buf;         /* Input grow buffer */
} psCallbacks;

/* The following functions control the tokenizer:
 *
 * psNew            Create new parse context
 * psFree           Free parse context
 *
 * psSetDecrypt     Install eexec decryption
 * psSetPlain       Remove eexec decryption
 * psReadBinary     Read specified number of bytes without tokenization
 */
psCtx psNew(psCallbacks *cb);
void psFree(psCtx h);

void psSetDecrypt(psCtx h);
void psSetPlain(psCtx h);
void psReadBinary(psCtx h, long nBytes);
void psSkipBinary(psCtx h, long nBytes);

/* The following functions retrieve parsing information:
 *
 * psGetToken       Return next PostScript token
 * psMatchToken     Match token against specified args
 * psFindToken      Read input until token with specified args found
 *
 * psMatchValue     Match token's value against specified string
 * psGetValue       Get pointer to token's value string
 *
 * psGetInteger     Read and convert next token as integer
 * psGetReal        Read and convert next token as real
 * psGetString      Read and convert next token as string
 * psGetHexLength   Return hexadecimal string length in digits
 * psGetHexString   Read and convert next token as hexadecimal string
 *
 * psConvInteger    Convert integer token to integer value
 * psConvReal       Convert real token to double value
 * psConvString     Convert string token to string value
 * psConvLiteral    Convert literal token to string value
 * psConvHexString  Convert hexadecimal string token to integer value
 */
psToken *psGetToken(psCtx h);
int psMatchToken(psCtx h, psToken *token, int type, char *value);
psToken *psFindToken(psCtx h, int type, char *value);

int psMatchValue(psCtx h, psToken *token, char *strng);
char *psGetValue(psCtx h, psToken *token);

long psGetInteger(psCtx h);
double psGetReal(psCtx h);
char *psGetString(psCtx h, unsigned *length);
int psGetHexLength(psCtx h, psToken *token);
unsigned long psGetHexString(psCtx h, int *length);

long psConvInteger(psCtx h, psToken *token);
double psConvReal(psCtx h, psToken *token);
char *psConvString(psCtx h, psToken *token, unsigned *length);
char *psConvLiteral(psCtx h, psToken *token, unsigned *length);
unsigned long psConvHexString(psCtx h, psToken *token);

/* Exception handling */
void CDECL psWarning(psCtx h, char *fmt, ...);
void CDECL psFatal(psCtx h, char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PS_H */
