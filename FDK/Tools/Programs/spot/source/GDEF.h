/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GDEF.h	1.3
 * Changed:    3/23/98 16:04:08
 ***********************************************************************/

/*
 * GDEF table support.
 */

#ifndef GDEF_H
#define GDEF_H

#include "global.h"

extern void GDEFRead(LongN offset, Card32 length);
extern void GDEFDump(IntX level, LongN offset);
extern void GDEFFree(void);
extern void GDEFUsage(void);
#endif /* GDEF_H */
