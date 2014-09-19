/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vmtx.h	1.2
 * Changed:    6/26/98 11:21:57
 ***********************************************************************/

/*
 * vmtx table support.
 */

#ifndef VMTX_H
#define VMTX_H

#include "global.h"

extern void vmtxRead(LongN offset, Card32 length);
extern void vmtxDump(IntX level, LongN offset);
extern void vmtxFree(void);


extern IntX vmtxGetMetrics(GlyphId glyphId, FWord *tsb, uFWord *vadv,
						   Card32 client);

#endif /* VMTX_H */
