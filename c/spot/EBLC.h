/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * EBLC table support.
 */

#ifndef EBLC_H
#define EBLC_H

#include "spot_global.h"

extern void EBLCRead(int32_t offset, uint32_t length);
extern void EBLCDump(int level, int32_t offset);
extern void EBLCFree(void);
#endif /* EBLC_H */
