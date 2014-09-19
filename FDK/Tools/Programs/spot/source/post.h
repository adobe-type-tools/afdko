/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)post.h	1.3
 * Changed:    10/25/95 13:46:50
 ***********************************************************************/

/*
 * post table support.
 */

#ifndef POST_H
#define POST_H

#include "global.h"

extern void postRead(LongN offset, Card32 length);
extern void postDump(IntX level, LongN offset);
extern void postFree(void);

extern IntX postInitName(void);
extern Byte8 *postGetName(GlyphId glyphId, IntX *length);

#endif /* POST_H */
