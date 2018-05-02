/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GSUB.h	1.3
 * Changed:    3/23/98 16:04:08
 ***********************************************************************/

/*
 * GSUB table support.
 */

#ifndef GSUB_H
#define GSUB_H

#include "global.h"

extern void GSUBRead(LongN offset, Card32 length);
extern void GSUBDump(IntX level, LongN offset);
extern void GSUBFree(void);
extern void GSUBEval(IntX GSUBLookupListIndex, 
			  IntX numinputglyphs, GlyphId *inputglyphs, 
			  IntX *numoutputglyphs, GlyphId *outputglyphs);
extern void GSUBUsage(void);
#endif /* GSUB_H */
