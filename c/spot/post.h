/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * post table support.
 */

#ifndef POST_H
#define POST_H

#include "spot_global.h"

extern void postRead(int32_t offset, uint32_t length);
extern void postDump(int level, int32_t offset);
extern void postFree_spot(void);

extern int postInitName(void);
extern int8_t *postGetName(GlyphId glyphId, int *length);

#endif /* POST_H */
