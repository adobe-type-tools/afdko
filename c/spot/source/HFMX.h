/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * HFMX table support.
 */

#ifndef HFMX_H
#define HFMX_H

#include "global.h"

extern void HFMXRead(LongN offset, Card32 length);
extern void HFMXDump(IntX level, LongN offset);
extern void HFMXFree(void);

#endif /* HFMX_H */
