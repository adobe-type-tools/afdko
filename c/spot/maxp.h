/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * maxp table support.
 */

#ifndef MAXP_H
#define MAXP_H

#include "spot_global.h"

extern void maxpRead(int32_t offset, uint32_t length);
extern void maxpDump(int level, int32_t offset);
extern void maxpFree_spot(void);

extern int maxpGetNGlyphs(uint16_t *nGlyphs, uint32_t client);
extern int maxpGetMaxComponents(uint16_t *nComponents, uint32_t client);

#endif /* MAXP_H */
