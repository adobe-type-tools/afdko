/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_SFNT_H
#define HOTCONV_SFNT_H

#include "common.h"

/* Standard functions */
void sfntNew(hotCtx g);
void sfntFill(hotCtx g);
void sfntWrite(hotCtx g);
void sfntReuse(hotCtx g);
void sfntFree(hotCtx g);

/* Supplementary functions */
void sfntSetnGlyphs(void *ctx, unsigned nGlyphs);
void sfntSetMetrics(void *ctx, unsigned gid, FWord width,
                    FWord left, FWord bottom, FWord right, FWord top);
void sfntAddAnonTable(hotCtx g, uint32_t tag, hotAnonRefill refill);

#endif /* HOTCONV_SFNT_H */
