/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * hdmx table support.
 */

#ifndef HDMX_H
#define HDMX_H

#include "spot_global.h"

extern void hdmxRead(int32_t offset, uint32_t length);
extern void hdmxDump(int level, int32_t offset);
extern void hdmxFree(void);

#endif /* HDMX_H */
