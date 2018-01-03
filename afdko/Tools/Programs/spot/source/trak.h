/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)trak.h	1.1
 * Changed:    7/19/95 13:26:41
 ***********************************************************************/

/*
 * trak table support.
 */

#ifndef TRAK_H
#define TRAK_H

#include "global.h"

extern void trakRead(LongN offset, Card32 length);
extern void trakDump(IntX level, LongN offset);
extern void trakFree(void);

#endif /* TRAK_H */
