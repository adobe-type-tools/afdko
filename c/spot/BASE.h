/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * BASE table support.
 */

#ifndef BASE_H
#define BASE_H

#include "spot_global.h"

extern void BASERead(int32_t offset, uint32_t length);
extern void BASEDump(int level, int32_t offset);
extern void BASEFree_spot(void);
extern void BASEUsage(void);
extern int BASEgetValue(uint32_t tag, char axis, int16_t *value);

#endif /* BASE_H */
