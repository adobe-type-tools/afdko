/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)head.h	1.4
 * Changed:    6/10/98 10:51:16
 ***********************************************************************/

/*
 * head table support.
 */

#ifndef HEAD_H
#define HEAD_H

#include "global.h"

extern void headRead(LongN offset, Card32 length);
extern void headDump(IntX level, LongN offset);
extern void headFree(void);

extern IntX headGetLocFormat(Card16 *locFormat, Card32 client);
extern IntX headGetUnitsPerEm(Card16 *unitsPerEm, Card32 client);
extern IntX headGetSetLsb(Card16 *setLsb, Card32 client);
extern IntX headGetBBox(Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax,
						Card32 client);
extern Byte8 *headGetCreatedDate(Card32 client);
extern Byte8 *headGetModifiedDate(Card32 client);
extern IntX headGetFontRevision(float *rev, Card32 client);
extern IntX headGetVersion(float *ver, Card32 client);
#endif /* HEAD_H */
