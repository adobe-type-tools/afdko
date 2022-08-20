/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * LTSH table support.
 */

#ifndef LTSH_H
#define LTSH_H

#include "spot_global.h"

extern void LTSHRead(int32_t offset, uint32_t length);
extern void LTSHDump(int level, int32_t offset);
extern void LTSHFree(void);

#endif /* LTSH_H */
