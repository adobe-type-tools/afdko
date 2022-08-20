/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * trak table support.
 */

#ifndef TRAK_H
#define TRAK_H

#include "spot_global.h"

extern void trakRead(int32_t offset, uint32_t length);
extern void trakDump(int level, int32_t offset);
extern void trakFree(void);

#endif /* TRAK_H */
