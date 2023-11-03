/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * hmtx table support.
 */

#ifndef HMTX_H
#define HMTX_H

#include "spot_global.h"

extern void hmtxRead(int32_t offset, uint32_t length);
extern void hmtxDump(int level, int32_t offset);
extern void hmtxFree_spot(void);
extern void hmtxUsage(void);

extern int hmtxGetMetrics(GlyphId glyphId, FWord *lsb, uFWord *width,
                           uint32_t client);

#endif /* HMTX_H */