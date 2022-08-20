/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * BBOX table support.
 */

#ifndef BBOX_H
#define BBOX_H

#include "spot_global.h"

extern void BBOXRead(int32_t offset, uint32_t length);
extern void BBOXDump(int level, int32_t offset);
extern void BBOXFree(void);

#endif /* BBOX_H */
