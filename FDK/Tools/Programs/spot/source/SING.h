/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)SING.h	1.2
 * Changed:    10/21/97 14:49:08
 ***********************************************************************/

/*
 * SING table support.
 */

#ifndef SING_H
#define SING_H

#include "global.h"

extern void SINGRead(LongN offset, Card32 length);
extern void SINGDump(IntX level, LongN offset);
extern void SINGFree(void);

extern IntX SINGGetUnitsPerEm(Card16 *unitsPerEm, Card32 client);

#endif /* SING_H */
