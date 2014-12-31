/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Encoding support.
 */

#ifndef ENCODING_H
#define ENCODING_H

#include "cffwrite_share.h"

void cfwEncodingNew(cfwCtx g);
void cfwEncodingReuse(cfwCtx g);
void cfwEncodingFree(cfwCtx g);

const unsigned char *cfwEncodingGetPredef(int id, int *cnt);

void cfwEncodingBeg(cfwCtx g);
void cfwEncodingAddCode(cfwCtx g, unsigned char code);
void cfwEncodingAddSupCode(cfwCtx g, unsigned char code, SID sid);
int cfwEncodingEnd(cfwCtx g);

long cfwEncodingFill(cfwCtx g);
void cfwEncodingWrite(cfwCtx g);
Offset cfwEncodingGetOffset(cfwCtx g, int iEncoding, Offset base);

#endif /* ENCODING_H */