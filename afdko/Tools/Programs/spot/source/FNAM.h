/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)FNAM.h	1.1
 * Changed:    7/19/95 13:26:33
 ***********************************************************************/

/*
 * FNAM table support.
 */

#ifndef FNAM_H
#define FNAM_H

#include "global.h"

extern void FNAMRead(LongN offset, Card32 length);
extern void FNAMDump(IntX level, LongN offset);
extern void FNAMFree(void);

extern IntX FNAMGetNEncodings(Card16 *nEncoings, Card32 client);

#endif /* FNAM_H */
