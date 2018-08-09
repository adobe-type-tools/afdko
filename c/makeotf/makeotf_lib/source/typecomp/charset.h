/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Character set support.
 */

#ifndef CHARSET_H
#define CHARSET_H

#include "common.h"

void charsetNew(tcCtx g);
void charsetFree(tcCtx g);
int charsetAdd(tcCtx g, unsigned nGlyphs, SID *glyph, int is_cid);
long charsetFill(tcCtx g);
void charsetWrite(tcCtx g);
Offset charsetGetOffset(tcCtx g, int iCharset, Offset base);

#endif /* CHARSET_H */
