/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * MMFX table support.
 */

#ifndef MMFX_H
#define MMFX_H

#include "spot_global.h"

extern void MMFXRead(int32_t offset, uint32_t length);
extern void MMFXDump(int level, int32_t offset);
extern void MMFXDumpMetric(uint32_t MetricID);
extern void MMFXFree(void);

#endif /* MMFX_H */
