/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * GPOS table support.
 */

#ifndef GPOS_H
#define GPOS_H

#include "spot.h"
#include "spot_global.h"

extern void GPOSRead(int32_t offset, uint32_t length);
extern void GPOSDump(int level, int32_t offset);
extern void GPOSFree_spot(void);
extern void GPOSUsage(void);
#endif /* GPOS_H */
