/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)kern.h	1.2
 * Changed:    8/15/97 10:41:20
 ***********************************************************************/

/*
 * kern table support.
 */

#ifndef KERN_H
#define KERN_H

#include "global.h"

extern void kernRead(LongN offset, Card32 length);
extern void kernDump(IntX level, LongN offset);
extern void kernFree(void);
extern void kernUsage(void);

#endif /* KERN_H */
