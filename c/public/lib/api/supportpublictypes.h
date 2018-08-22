/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* publictypes.h */

#ifndef PUBLICTYPES_H
#define PUBLICTYPES_H

#include <stdio.h>
#include <stdint.h>

#include "supportenvironment.h"

#define readonly const

/* Inline Functions */

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Declarative Sugar */

#define PUBLIC
#define procedure void

/* ToDo: change this typedef to use bool instead of uint8_t, but note that
         "bool" is currently used for a variable name in some parts of the code */
typedef uint8_t boolean;

#define true 1
#define false 0

typedef uint8_t Card8, *PCard8;
typedef int8_t Int8, *PInt8;
typedef int8_t Char8;

#define MAXCard8 ((Card8)0xFF)

typedef int16_t Int16, *PInt16;
#define MAXInt16 ((Int16)0x7FFF)
#define MINInt16 ((Int16)0x8000)

typedef uint16_t Card16, *PCard16;
#define MAXCard16 ((Card16)0xFFFF)

/* Under 16-bit x86 compilers, the type of a pointer difference is a
   signed 16 bits; such an expression may be immediately cast to CardPtrDiff
   or IntPtrDiff without undesired side-effects. */
typedef unsigned long int CardPtrDiff; /* unsigned pointer difference */
typedef long int IntPtrDiff;           /* signed pointer difference */

typedef int32_t Int32, *PInt32;

typedef uint32_t Card32, *PCard32;

typedef int64_t Int64, *PInt64;
#define MAXInt64 ((Int64)0x7FFFFFFFFFFFFFFF)
#define MINInt64 ((Int64)0x8000000000000000)

typedef uint64_t Card64, *PCard64;
#define MAXCard64 ((Card64)0xFFFFFFFFFFFFFFFF)

typedef uint64_t longcardinal;
typedef int32_t integer;
typedef uint32_t unsignedinteger;

#define MAXInt32 ((Int32)0x7FFFFFFF)
#define MINInt32 ((Int32)0x80000000)
#define MAXCard32 ((Card32)0xFFFFFFFF)

#define MAXinteger ((integer)0x7FFFFFFF)
#define MINinteger ((integer)0x80000000)

/* The following definitions of CardX and IntX differ from "int" and
   "unsigned int" in that they mean  "use whatever width you like".
   It is assumed to be at least 16 bits. */
typedef unsigned int CardX;
typedef int IntX;

/* Reals and Fixed point */
typedef Int32 Fixed; /*  16 bits of integer, 16 bits of fraction */
typedef Fixed *PFixed;

#endif /* PUBLICTYPES_H */
