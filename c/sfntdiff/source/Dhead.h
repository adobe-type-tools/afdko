/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)head.h	1.3
 * Changed:    10/12/95 14:52:41
 ***********************************************************************/

/*
 * head table support.
 */

#ifndef HEAD_H
#define HEAD_H

#include "Dglobal.h"

extern void headRead(Card8 which, LongN offset, Card32 length);
extern void headDiff(LongN offset1, LongN offset2);
extern void headFree(Card8 which);

extern IntX headGetLocFormat(Card8 which, Card16 *locFormat, Card32 client);
extern IntX headGetUnitsPerEm(Card8 which, Card16 *unitsPerEm, Card32 client);
extern IntX headGetSetLsb(Card8 which, Card16 *setLsb, Card32 client);
extern IntX headGetBBox(Card8 which, Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax,
						Card32 client);
extern Byte8 *headGetCreatedDate(Card8 which, Card32 client);
extern Byte8 *headGetModifiedDate(Card8 which, Card32 client);

#endif /* HEAD_H */
