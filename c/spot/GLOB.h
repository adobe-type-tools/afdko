/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * GLOB table support.
 */

#ifndef GLOB_H
#define GLOB_H

#include "spot_global.h"

extern void GLOBRead(int32_t offset, uint32_t length);
extern void GLOBDump(int level, int32_t offset);
extern void GLOBFree(void);
extern void GLOBUniBBox(FWord *bbLeft, FWord *bbBottom, FWord *bbRight, FWord *bbTop);
#endif /* GLOB_H */
