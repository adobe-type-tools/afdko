/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * BLND table support.
 */

#ifndef BLND_H
#define BLND_H

#include "spot_global.h"

extern void BLNDRead(int32_t offset, uint32_t length);
extern void BLNDDump(int level, int32_t offset);
extern void BLNDFree(void);

extern int BLNDGetNMasters(void);

#endif /* BLND_H */
