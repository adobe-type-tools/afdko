/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * MMSD table support.
 */

#ifndef MMSD_H
#define MMSD_H

#include "spot_global.h"

extern void MMSDRead(int32_t offset, uint32_t length);
extern void MMSDDump(int level, int32_t offset);
extern void MMSDFree(void);
#endif /* MMSD_H */
