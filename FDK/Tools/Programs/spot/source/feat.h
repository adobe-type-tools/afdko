/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)feat.h	1.1
 * Changed:    7/19/95 13:26:36
 ***********************************************************************/

/*
 * feat table support.
 */

#ifndef FEAT_H
#define FEAT_H

#include "global.h"

extern void featRead(LongN offset, Card32 length);
extern void featDump(IntX level, LongN offset);
extern void featFree(void);

#endif /* FEAT_H */
