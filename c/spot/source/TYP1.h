/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TYP1 table support.
 */

#ifndef TYP1_H
#define TYP1_H

#include "global.h"

extern void TYP1Read(LongN offset, Card32 length);
extern void TYP1Dump(IntX level, LongN offset);
extern void TYP1Free(void);

extern IntX TYP1GetNGlyphs(Card16 *nGlyphs, Card32 client);

#endif /* TYP1_H */
