/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * vmtx table support.
 */

#ifndef VMTX_H
#define VMTX_H

#include "spot_global.h"

extern void vmtxRead(int32_t offset, uint32_t length);
extern void vmtxDump(int level, int32_t offset);
extern void vmtxFree_spot(void);

extern int vmtxGetMetrics(GlyphId glyphId, FWord *tsb, uFWord *vadv,
                           uint32_t client);

#endif /* VMTX_H */
