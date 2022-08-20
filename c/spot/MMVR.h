/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * MMVR table support.
 */

#ifndef MMVR_H
#define MMVR_H

#include "spot_global.h"

extern void MMVRRead(int32_t offset, uint32_t length);
extern void MMVRDump(int level, int32_t offset);
extern void MMVRFree(void);

#endif /* MMVR_H */
