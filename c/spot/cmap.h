/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * cmap table support.
 */

#ifndef CMAP_H
#define CMAP_H

#include "spot_global.h"

extern void cmapRead(int32_t offset, uint32_t length);
extern void cmapDump(int level, int32_t offset);
extern void cmapFree_spot(void);

extern int cmapInitName(void);
extern int8_t *cmapGetName(GlyphId glyphId, int *length);

extern void cmapUsage(void);

extern int cmapSelected;
#endif /* CMAP_H */
