/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMVR.h	1.1
 * Changed:    7/19/95 13:26:34
 ***********************************************************************/

/*
 * MMVR table support.
 */

#ifndef MMVR_H
#define MMVR_H

#include "global.h"

extern void MMVRRead(LongN offset, Card32 length);
extern void MMVRDump(IntX level, LongN offset);
extern void MMVRFree(void);

#endif /* MMVR_H */
