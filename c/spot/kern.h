/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * kern table support.
 */

#ifndef KERN_H
#define KERN_H

#include "spot_global.h"

extern void kernRead(int32_t offset, uint32_t length);
extern void kernDump(int level, int32_t offset);
extern void kernFree(void);
extern void kernUsage(void);

#endif /* KERN_H */
