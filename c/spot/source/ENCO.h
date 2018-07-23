/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)ENCO.h	1.1
 * Changed:    7/19/95 13:26:32
 ***********************************************************************/

/*
 * ENCO table support.
 */

#ifndef ENCO_H
#define ENCO_H

#include "global.h"

extern void ENCORead(LongN offset, Card32 length);
extern void ENCODump(IntX level, LongN offset);
extern void ENCOFree(void);

#endif /* ENCO_H */
