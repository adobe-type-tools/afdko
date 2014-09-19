/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMFX.h	1.2
 * Changed:    8/13/98 16:18:12
 ***********************************************************************/

/*
 * MMFX table support.
 */

#ifndef MMFX_H
#define MMFX_H

#include "global.h"

extern void MMFXRead(LongN offset, Card32 length);
extern void MMFXDump(IntX level, LongN offset);
extern void MMFXDumpMetric(Card32 MetricID);
extern void MMFXFree(void);

#endif /* MMFX_H */

