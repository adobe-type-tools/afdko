/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * feat table support.
 */

#ifndef FEAT_H
#define FEAT_H

#include "spot_global.h"

extern void featRead(int32_t offset, uint32_t length);
extern void featDump(int level, int32_t offset);
extern void featFree_spot(void);

#endif /* FEAT_H */
