/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)OS_2.h	1.2
 * Changed:    10/12/95 14:50:54
 ***********************************************************************/

/*
 * OS_2 table support.
 */

#ifndef OS_2_H
#define OS_2_H

#include "global.h"

extern void OS_2Read(LongN offset, Card32 length);
extern void OS_2Dump(IntX level, LongN offset);
extern void OS_2GetTypocenders(Int32 *ascender, Int32*descender);
extern void OS_2Free(void);

#endif /* OS_2_H */
