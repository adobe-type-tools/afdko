/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef FDSELECT_H
#define FDSELECT_H

#include "cffwrite_share.h"

void cfwFdselectNew(cfwCtx g);
void cfwFdselectReuse(cfwCtx g);
void cfwFdselectFree(cfwCtx g);

void cfwFdselectBeg(cfwCtx g);
void cfwFdselectAddIndex(cfwCtx g, unsigned char fd);
int cfwFdselectEnd(cfwCtx g);

long cfwFdselectFill(cfwCtx g);
void cfwFdselectWrite(cfwCtx g);
Offset cfwFdselectGetOffset(cfwCtx g, int iSelector, Offset base);

#endif /* FDSELECT_H */