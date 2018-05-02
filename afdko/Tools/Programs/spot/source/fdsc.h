/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fdsc.h	1.1
 * Changed:    7/19/95 13:26:36
 ***********************************************************************/

/*
 * fdsc table support.
 */

#ifndef FDSC_H
#define FDSC_H

#include "global.h"

extern void fdscRead(LongN offset, Card32 length);
extern void fdscDump(IntX level, LongN offset);
extern void fdscFree(void);

#endif /* FDSC_H */
