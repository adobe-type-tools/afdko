/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * WDTH table support.
 */

#ifndef WDTH_H
#define WDTH_H

#include "spot_global.h"

extern void WDTHRead(int32_t offset, uint32_t length);
extern void WDTHDump(int level, int32_t offset);
extern void WDTHFree(void);

#endif /* WDTH_H */
