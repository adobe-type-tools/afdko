/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)cmap.h	1.4
 * Changed:    8/3/99 10:12:08
 ***********************************************************************/

/*
 * cmap table support.
 */

#ifndef CMAP_H
#define CMAP_H

#include "global.h"

extern void cmapRead(LongN offset, Card32 length);
extern void cmapDump(IntX level, LongN offset);
extern void cmapFree(void);

extern IntX cmapInitName(void);
extern Byte8 *cmapGetName(GlyphId glyphId, IntX *length);

extern void cmapUsage(void);

extern IntX cmapSelected;
#endif /* CMAP_H */
