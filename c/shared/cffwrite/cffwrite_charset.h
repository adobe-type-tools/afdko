/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Character set support.
 */

#ifndef SHARED_CFFWRITE_CFFWRITE_CHARSET_H_
#define SHARED_CFFWRITE_CFFWRITE_CHARSET_H_

#include "cffwrite_share.h"

void cfwCharsetNew(cfwCtx g);
void cfwCharsetReuse(cfwCtx g);
void cfwCharsetFree(cfwCtx g);

void cfwCharsetBeg(cfwCtx g, int is_cid);
void cfwCharsetAddGlyph(cfwCtx g, unsigned short nameid);
int cfwCharsetEnd(cfwCtx g);

long cfwCharsetFill(cfwCtx g);
void cfwCharsetWrite(cfwCtx g);
Offset cfwCharsetGetOffset(cfwCtx g, int iCharset, Offset base);

#endif  // SHARED_CFFWRITE_CFFWRITE_CHARSET_H_
