/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * EBLC table support.
 */

#ifndef EBLC_H
#define EBLC_H

#include "global.h"

extern void EBLCRead(LongN offset, Card32 length);
extern void EBLCDump(IntX level, LongN offset);
extern void EBLCFree(void);
#endif /* EBLC_H */
