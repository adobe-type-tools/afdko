/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)gasp.h	1.1
 * Changed:    10/12/95 14:56:53
 ***********************************************************************/

/*
 * gasp table support.
 */

#ifndef GASP_H
#define GASP_H

#include "global.h"

extern void gaspRead(LongN offset, Card32 length);
extern void gaspDump(IntX level, LongN offset);
extern void gaspFree(void);

#endif /* GASP_H */
