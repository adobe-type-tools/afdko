/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TYP1 table support.
 */

#ifndef TYP1_H
#define TYP1_H

#include "spot_global.h"

extern void TYP1Read(int32_t offset, uint32_t length);
extern void TYP1Dump(int level, int32_t offset);
extern void TYP1Free(void);

extern int TYP1GetNGlyphs(uint16_t *nGlyphs, uint32_t client);

#endif /* TYP1_H */
