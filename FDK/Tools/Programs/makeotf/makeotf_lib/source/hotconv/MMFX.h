/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef MMFX_H
#define MMFX_H

#include "common.h"

#define MMFX_   TAG('M', 'M', 'F', 'X')

/* Standard functions */
void MMFXNew(hotCtx g);
int MMFXFill(hotCtx g);
void MMFXWrite(hotCtx g);
void MMFXReuse(hotCtx g);
void MMFXFree(hotCtx g);

/* Supplementary functions */
enum {                  /* Named metric ids */
	MMFXZero,
	MMFXAscender,
	MMFXDescender,
	MMFXLineGap,
	MMFXAdvanceWidthMax,
	MMFXAvgCharWidth,
	MMFXXHeight,
	MMFXCapHeight,
	MMFXNamedMetricCnt
};

void MMFXAddNamedMetric(hotCtx g, unsigned id, FWord *metric);
unsigned MMFXAddMetric(hotCtx g, FWord *metric);
Fixed MMFXExecMetric(hotCtx g, unsigned id);

#endif /* MMFX_H */
