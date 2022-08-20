/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * GSUB table support.
 */

#ifndef GSUB_H
#define GSUB_H

#include "spot_global.h"

extern void GSUBRead(int32_t offset, uint32_t length);
extern void GSUBDump(int level, int32_t offset);
extern void GSUBFree_spot(void);
extern void GSUBEval(int GSUBLookupListIndex,
                     int numinputglyphs, GlyphId *inputglyphs,
                     int *numoutputglyphs, GlyphId *outputglyphs);
extern void GSUBUsage(void);
#endif /* GSUB_H */
