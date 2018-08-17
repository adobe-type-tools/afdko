/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
  framepixel.h
*/

#ifndef FRAMEPIXEL_H
#define FRAMEPIXEL_H

#include "supportpublictypes.h"
#include "supportenvironment.h"
#include "supportframepixeltypes.h"

/* Bit Array Stuff: defined in bitarray.c */

extern readonly SWAPTYPE leftBitArray[SWAPUNIT];
/* For i in [0, SCANUNIT), i'th element is a mask consisting of i zero
     bits followed by SCANUNIT-i one bits (in device order).
   */
extern readonly SWAPTYPE rightBitArray[SWAPUNIT];
/* For i in [0, SCANUNIT), i'th element is a mask consisting of i one
     bits followed by SCANUNIT-i zero bits (in device order).
   */

#endif /* FRAMEPIXEL_H */
