/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * fdsc table support.
 */

#ifndef FDSC_H
#define FDSC_H

#include "spot_global.h"

extern void fdscRead(int32_t offset, uint32_t length);
extern void fdscDump(int level, int32_t offset);
extern void fdscFree(void);

#endif /* FDSC_H */
