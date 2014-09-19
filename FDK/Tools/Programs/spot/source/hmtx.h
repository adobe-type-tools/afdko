/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hmtx.h	1.2
 * Changed:    9/3/97 23:12:16
 ***********************************************************************/

/*
 * hmtx table support.
 */

#ifndef HMTX_H
#define HMTX_H

#include "global.h"

extern void hmtxRead(LongN offset, Card32 length);
extern void hmtxDump(IntX level, LongN offset);
extern void hmtxFree(void);
extern void hmtxUsage(void);

extern IntX hmtxGetMetrics(GlyphId glyphId, FWord *lsb, uFWord *width,
						   Card32 client);

#endif /* HMTX_H */
