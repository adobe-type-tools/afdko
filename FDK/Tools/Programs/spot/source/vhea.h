/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vhea.h	1.1
 * Changed:    10/24/95 18:30:32
 ***********************************************************************/

/*
 * vhea table support.
 */

#ifndef VHEA_H
#define VHEA_H

#include "global.h"

extern void vheaRead(LongN offset, Card32 length);
extern void vheaDump(IntX level, LongN offset);
extern void vheaFree(void);

extern IntX vheaGetNLongVerMetrics(Card16 *nLongVerMetrics, Card32 client);

#endif /* VHEA_H */
