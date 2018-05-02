/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hhea.h	1.1
 * Changed:    7/19/95 13:26:38
 ***********************************************************************/

/*
 * hhea table support.
 */

#ifndef HHEA_H
#define HHEA_H

#include "global.h"

extern void hheaRead(LongN offset, Card32 length);
extern void hheaDump(IntX level, LongN offset);
extern void hheaFree(void);

extern void hheaGetTypocenders(Int32 *ascender, Int32*descender);
extern IntX hheaGetNLongHorMetrics(Card16 *nLongHorMetrics, Card32 client);

#endif /* HHEA_H */
