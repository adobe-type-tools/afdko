/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)META.h	1.2
 * Changed:    10/21/97 14:49:08
 ***********************************************************************/

/*
 * META table support.
 */

#ifndef META_H
#define META_H

#include "global.h"

extern void METARead(LongN offset, Card32 length);
extern void METADump(IntX level, LongN offset);
extern void METAFree(void);
#endif /* META_H */
