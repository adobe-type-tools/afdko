/* @(#)CM_VerSion framepixel.h atm09 1.2 16563.eco sum= 43975 atm09.004 */
/* @(#)CM_VerSion framepixel.h atm06 1.2 13532.eco sum= 31298 */
/* @(#)CM_VerSion framepixel.h atm05 1.2 11580.eco sum= 64242 */
/* @(#)CM_VerSion framepixel.h atm04 1.4 07348.eco sum= 28005 */
/* $Header$ */
/*
  framepixel.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


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
