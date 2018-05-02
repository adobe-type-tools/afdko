/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BLND.h	1.1
 * Changed:    7/19/95 13:26:32
 ***********************************************************************/

/*
 * BLND table support.
 */

#ifndef BLND_H
#define BLND_H

#include "global.h"

extern void BLNDRead(LongN offset, Card32 length);
extern void BLNDDump(IntX level, LongN offset);
extern void BLNDFree(void);

extern IntX BLNDGetNMasters(void);

#endif /* BLND_H */
