/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)VORG.h	
 * Changed:    6/02/00
 ***********************************************************************/

/*
 * VORG table support.
 */

#ifndef VORG_H
#define VORG_H

#include "global.h"

extern void VORGRead(LongN offset, Card32 length);
extern void VORGDump(IntX level, LongN offset);
extern void VORGFree(void);
extern void VORGUsage(void);

extern IntX VORGGetVertOriginY(GlyphId glyphId, Int16 *vertOriginY, Card32 client);

#endif /* VMTX_H */
