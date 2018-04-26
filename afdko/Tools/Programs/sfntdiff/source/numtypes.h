/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)numtypes.h	1.10
 * Changed:    7/21/95 14:08:20
 ***********************************************************************/

#ifndef _NUMTYPES_H
#define _NUMTYPES_H

#define TYPEDEF_boolean 1
/* import type definitions for Int16, Card16 etc from shared header file */
#include "supportpublictypes.h"

typedef char            Byte8;
typedef int             IntN;
typedef unsigned        CardN;
typedef short           ShortN;
typedef unsigned short  ShortCardN;
typedef long            LongN;
typedef unsigned long   LongCardN;
typedef float           FloatN;
typedef double          DoubleN;

#define MAX_INT16   MAXInt16
#define MAX_INT32   MAXInt32
#define MAX_CARD16  MAXCard16
#define MAX_CARD32  MAXCard32

#undef global   /* global.h uses this name for a different purpose */

#endif/*_NUMTYPES_H*/
