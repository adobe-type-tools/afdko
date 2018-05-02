/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GPOS.h	1.1
 * Changed:    12/15/97 09:25:21
 ***********************************************************************/

/*
 * GPOS table support.
 */

#ifndef GPOS_H
#define GPOS_H

#include "global.h"

extern void GPOSRead(LongN offset, Card32 length);
extern void GPOSDump(IntX level, LongN offset);
extern void GPOSFree(void);
extern void GPOSUsage(void);
#endif /* GPOS_H */
