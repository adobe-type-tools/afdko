/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * VORG table support.
 */

#ifndef VORG_H
#define VORG_H

#include "spot_global.h"

extern void VORGRead(int32_t offset, uint32_t length);
extern void VORGDump(int level, int32_t offset);
extern void VORGFree_spot(void);
extern void VORGUsage(void);

extern int VORGGetVertOriginY(GlyphId glyphId, int16_t *vertOriginY, uint32_t client);

#endif /* VMTX_H */
