/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fvar.h	1.1
 * Changed:    7/19/95 13:26:36
 ***********************************************************************/

/*
 * fvar table support.
 */

#ifndef FVAR_H
#define FVAR_H

#include "global.h"

extern void fvarRead(LongN offset, Card32 length);
extern void fvarDump(IntX level, LongN offset);
extern void fvarFree(void);

#endif /* FVAR_H */
