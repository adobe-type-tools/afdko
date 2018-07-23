/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)maxp.h	1.1
 * Changed:    7/19/95 13:26:39
 ***********************************************************************/

/*
 * maxp table support.
 */

#ifndef MAXP_H
#define MAXP_H

#include "global.h"

extern void maxpRead(LongN offset, Card32 length);
extern void maxpDump(IntX level, LongN offset);
extern void maxpFree(void);

extern IntX maxpGetNGlyphs(Card16 *nGlyphs, Card32 client);
extern IntX maxpGetMaxComponents(Card16 *nComponents, Card32 client);

#endif /* MAXP_H */
