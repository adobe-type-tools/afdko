/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)loca.h	1.2
 * Changed:    7/25/95 14:11:23
 ***********************************************************************/

/*
 * loca table support.
 */

#ifndef LOCA_H
#define LOCA_H

#include "global.h"

extern void locaRead(LongN offset, Card32 length);
extern void locaDump(IntX level, LongN offset);
extern void locaFree(void);

extern IntX locaGetOffset(GlyphId glyphId, LongN *offset, Card32 *length,
						  Card32 client);

#endif /* LOCA_H */
