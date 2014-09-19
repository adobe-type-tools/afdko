/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * MMSD table support.
 */

#ifndef MMSD_H
#define MMSD_H

#include "global.h"

extern void MMSDRead(LongN offset, Card32 length);
extern void MMSDDump(IntX level, LongN offset);
extern void MMSDFree(void);
#endif /* MMSD_H */
