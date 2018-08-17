/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

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
