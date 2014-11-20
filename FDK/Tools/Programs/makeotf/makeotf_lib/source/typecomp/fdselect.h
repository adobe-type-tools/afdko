/* @(#)CM_VerSion fdselect.h atm08 1.2 16245.eco sum= 22904 atm08.002 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef FDSELECT_H
#define FDSELECT_H

#include "common.h"

typedef unsigned char FDIndex;  /* FDArray index */

void fdselectNew(tcCtx g);
void fdselectFree(tcCtx g);

int fdselectAdd(tcCtx g, unsigned nGlyphs, FDIndex *fd);
long fdselectFill(tcCtx g);
void fdselectWrite(tcCtx g);
Offset fdselectGetOffset(tcCtx g, int iFDSelect, Offset base);

#endif /* FDSELECT_H */