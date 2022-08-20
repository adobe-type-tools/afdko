/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * loca table support.
 */

#ifndef LOCA_H
#define LOCA_H

#include "spot_global.h"

extern void locaRead(int32_t offset, uint32_t length);
extern void locaDump(int level, int32_t offset);
extern void locaFree(void);

extern int locaGetOffset(GlyphId glyphId, int32_t *offset, uint32_t *length,
                          uint32_t client);

#endif /* LOCA_H */
