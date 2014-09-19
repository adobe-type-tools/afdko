/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hdmx.h	1.1
 * Changed:    7/19/95 13:26:37
 ***********************************************************************/

/*
 * hdmx table support.
 */

#ifndef HDMX_H
#define HDMX_H

#include "global.h"

extern void hdmxRead(LongN offset, Card32 length);
extern void hdmxDump(IntX level, LongN offset);
extern void hdmxFree(void);

#endif /* HDMX_H */
