/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GLOB.h	1.2
 * Changed:    10/21/97 14:49:08
 ***********************************************************************/

/*
 * GLOB table support.
 */

#ifndef GLOB_H
#define GLOB_H

#include "global.h"

extern void GLOBRead(LongN offset, Card32 length);
extern void GLOBDump(IntX level, LongN offset);
extern void GLOBFree(void);
extern void GLOBUniBBox(FWord *bbLeft, FWord *bbBottom, FWord *bbRight, FWord *bbTop);
#endif /* GLOB_H */
