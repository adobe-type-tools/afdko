/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BASE.h	1.1
 * Changed:    10/12/95 14:56:51
 ***********************************************************************/

/*
 * BASE table support.
 */

#ifndef BASE_H
#define BASE_H

#include "global.h"

extern void BASERead(LongN offset, Card32 length);
extern void BASEDump(IntX level, LongN offset);
extern void BASEFree(void);
extern void BASEUsage(void);
extern int  BASEgetValue(Card32 tag, char axis, Int16 *value);

#endif /* BASE_H */
