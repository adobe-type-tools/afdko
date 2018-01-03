/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
#include <stdio.h>
#include "sha1.h"

#include <limits.h>   /* to get info about range unsigned int */


#if ULONG_MAX == 4294967295U
typedef unsigned long int Card32;
#elif UINT_MAX == 4294967295U
typedef unsigned int Card32;
#else
#error "There must be a way to get an unsigned 32 bit integer"
#endif

typedef struct sha1_ctx {
        Card32 H[5];
        Card32 count_hi;
        Card32 count_lo;
        unsigned char c[64];
        Card32 count_in;
        } sha1_ctx;

void sha1_process(sha1_ctx *ctx){
Card32 A, B, C, D, E, W[80], TEMP, t;

    for(t=0; t<16; t++)
	W[t] = (((Card32)(ctx->c[t*4]))<<24) +
	       (((Card32)(ctx->c[t*4+1]))<<16) +
	       (((Card32)(ctx->c[t*4+2]))<<8) +
	       ((Card32)(ctx->c[t*4+3]));
    for(t=16; t<80; t++){
	W[t] = W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
        W[t] = (W[t]<<1) | (W[t]>>31);
    }
    A = ctx->H[0];
    B = ctx->H[1];
    C = ctx->H[2];
    D = ctx->H[3];
    E = ctx->H[4];
    for(t=0; t<20; t++){
	TEMP = ((A<<5) | (A>>27)) +
	       ((B & C) | ((~B) & D)) +
	       E + W[t] + 0x5a827999;
        E = D;
        D = C;
        C = ((B<<30) | (B>>2));
        B = A;
        A = TEMP;
    }
    for(t=20; t<40; t++){
        TEMP = ((A<<5) | (A>>27)) +
               (B ^ C ^ D) +
               E + W[t] + 0x6ed9eba1;
        E = D;
        D = C;
        C = ((B<<30) | (B>>2));
        B = A;
        A = TEMP;
    }
    for(t=40; t<60; t++){
        TEMP = ((A<<5) | (A>>27)) +
               ((B & C) | (B & D) | (C & D)) +
               E + W[t] + 0x8f1bbcdc;
        E = D;
        D = C;
        C = ((B<<30) | (B>>2));
        B = A;
        A = TEMP;
    }
    for(t=60; t<80; t++){
        TEMP = ((A<<5) | (A>>27)) +
               (B ^ C ^ D) +
               E + W[t] + 0xca62c1d6;
        E = D;
        D = C;
        C = ((B<<30) | (B>>2));
        B = A;
        A = TEMP;
    }

    ctx->H[0] += A;
    ctx->H[1] += B;
    ctx->H[2] += C;
    ctx->H[3] += D;
    ctx->H[4] += E;

}

sha1_ctx * sha1_init(sha1_malloc p,void *hook){
    sha1_ctx *c;
    int i;

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning(disable: 4127)
#endif
    if(sizeof(Card32) != 4)
	return NULL;
#ifdef _MSC_VER
#pragma warning (pop)
#endif

    c = (sha1_ctx*)p(sizeof(sha1_ctx),hook);
    if(c == NULL)
	return NULL;
    c->H[0] = 0x67452301;
    c->H[1] = 0xefcdab89;
    c->H[2] = 0x98badcfe;
    c->H[3] = 0x10325476;
    c->H[4] = 0xc3d2e1f0;
    c->count_hi = 0;
    c->count_lo = 0;
    c->count_in = 0;
    for(i=0;i<64;i++)
	c->c[i] = 0;
    return c;
}

int sha1_update(sha1_pctx ctx,unsigned char *b, size_t len){
    Card32 old_lo;
    if(ctx == NULL)
	return 1;
    if(len == 0)
	return 0;
    if(b == NULL)
	return 1;
    while(len--){
        old_lo = ctx->count_lo;
	ctx->count_lo += 8; /* count is bits, not bytes */
	if(old_lo > ctx->count_lo) /* overflow */
	    ctx->count_hi++; /* once in 4 billion bits */
	ctx->c[ctx->count_in++] = *b++;
        if(ctx->count_in == 64){
	    sha1_process(ctx);
	    ctx->count_in = 0;
	}
    }

    return 0;
}

int sha1_finalize(sha1_pctx ctx,sha1_free p,sha1_hash hash,void *hook){
    int i;
    if(ctx == NULL)
	return 1;
    if(p == NULL)
	return 1;
    if(hash == NULL)
	return 1;
    ctx->c[ctx->count_in++] = 0x80; /* pad bit */
    if(ctx->count_in < 57){ /* count will fit in this buffer */
	for(i=ctx->count_in; i < 56; i++)
	    ctx->c[i] = 0; /* zero fill remainder of buffer */
    } else { /* count doesn't fit. process this buffer and put count in next */
	for(i=ctx->count_in; i < 64; i++)
            ctx->c[i] = 0; /* zero fill remainder of buffer */
        sha1_process(ctx);
        for(i=0; i < 56; i++)
            ctx->c[i] = 0; /* zero fill the buffer (less count */
    }
    ctx->c[56] = (unsigned char)(ctx->count_hi>>24);
    ctx->c[57] = (unsigned char)(ctx->count_hi>>16);
    ctx->c[58] = (unsigned char)(ctx->count_hi>>8);
    ctx->c[59] = (unsigned char)(ctx->count_hi);
    ctx->c[60] = (unsigned char)(ctx->count_lo>>24);
    ctx->c[61] = (unsigned char)(ctx->count_lo>>16);
    ctx->c[62] = (unsigned char)(ctx->count_lo>>8);
    ctx->c[63] = (unsigned char)(ctx->count_lo);
    sha1_process(ctx);
    for(i=0; i< 5; i++){
	hash[i*4] = (unsigned char)(ctx->H[i]>>24);
	hash[i*4+1] = (unsigned char)(ctx->H[i]>>16);
	hash[i*4+2] = (unsigned char)(ctx->H[i]>>8);
	hash[i*4+3] = (unsigned char)(ctx->H[i]);
    }
    p(ctx,hook);
    return 0;
}
