/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BBOX.h	1.1
 * Changed:    7/19/95 13:26:31
 ***********************************************************************/

/*
 * BBOX table support.
 */

#ifndef BBOX_H
#define BBOX_H

#include "global.h"

extern void BBOXRead(LongN offset, Card32 length);
extern void BBOXDump(IntX level, LongN offset);
extern void BBOXFree(void);

#endif /* BBOX_H */
