/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)numtypes.h	1.10
 * Changed:    7/21/95 14:08:20
 ***********************************************************************/

#ifndef _NUMTYPES_H
#define _NUMTYPES_H

/***********************************************************************/
/* GUIDELINES FOR SELECTION OF TYPE                                    */
/*                                                                     */
/* The following types give best performance because implicit          */
/* conversion does not take place.  So, use them if at all possible.   */
/* Types are show in order of decreasing preference.  The dependable   */
/* domain for each type is show enclosed in [].                        */
/*                                                                     */
/* IntX                                                                */
/* CardX                                                               */
/* ByteX8      whenever there is NO mandated requirement for signed    */
/*             or unsigned values [0, 127].                            */
/***********************************************************************/

/***********************************************************************/
/* The present file assumes:                                           */
/*                                                                     */
/*             char:   8 bits                                          */
/*             short: 16 bits                                          */
/*             long:  32 bits                                          */
/*                                                                     */
/* This assumption is currently true for Sun, PC, and Mac environments.*/
/* When this assumption is no longer true, #ifdef the code accordingly */
/* and change this comment.                                            */
/***********************************************************************/

/***********************************************************************/
/* The following numeric types guarantee the number of bits: neither   */
/* more or less than specified.  Use these ONLY when you actually must */
/* have the number of bits specified.  If you need a minimum of a      */
/* certain number of bits, use types from the IntX.../CardX... group.  */
/***********************************************************************/

typedef long                            Fixed;


typedef long            Int32;
#define MAX_INT32       ((Int32)0x7FFFFFFF)
#define MIN_INT32       ((Int32)0x80000000)

typedef unsigned long   Card32;
#define MAX_CARD32      ((Card32)0xFFFFFFFF)

typedef short           Int16;
#define MAX_INT16       ((Int16)0x7FFF)
#define MIN_INT16       ((Int16)0x8000)

typedef unsigned short  Card16;
#define MAX_CARD16      ((Card16)0xFFFF)

typedef signed char     Int8;
#define MAX_INT8        ((Int8)0x7F)
#define MIN_INT8        ((Int8)0x80)

typedef unsigned char   Card8;
#define MAX_CARD8       ((Card8)0xFF)

typedef char            Byte8;
/* MAX_BYTE8 and MIN_BYTE8 are omitted because it is not known whether */
/* if a Byte8 (char) is signed or unsigned.                            */

/***********************************************************************/
/* The following numeric types guarantee at least the number of bits   */
/* indicated.  Use these when a specific size is not required, but     */
/* a minimum size is.  Note that IntX/CardX are guaranteed to be at    */
/* least 16 bits, but can be set (for testing purposes) to either      */
/* 16 or 32 bit sizes by defining FORCE_INT_BITS to the compiler.      */
/***********************************************************************/

#if defined(FORCE16BITS)
#  define FORCE_INT_BITS      16
#elif defined(FORCE32BITS)
#  define FORCE_INT_BITS      32
#endif

                                    /***********************************/
                                    /* Optimize for performance...     */
                                    /***********************************/
#if FORCE_INT_BITS == 32

   typedef Int32        IntX;
#  define MAX_INTX      MAX_INT32
#  define MIN_INTX      MIN_INT32
   typedef Card32       CardX;
#  define MAX_CARDX     MAX_CARD32

#elif FORCE_INT_BITS == 16

   typedef Int16        IntX;
#  define MAX_INTX      MAX_INT16
#  define MIN_INTX      MIN_INT16
   typedef Card16       CardX;
#  define MAX_CARDX     MAX_CARD16
#  define MAX_CARDX     MAX_CARD16

#else

   typedef int          IntX;
#  define MAX_INTX      ((IntX)((1<<(sizeof(IntX)*8-1))-1))
#  define MIN_INTX      ((IntX)(1<<(sizeof(IntX)*8-1)))
   typedef unsigned     CardX;
#  define MAX_CARDX     ((CardX)(MAX_INTX|MIN_INTX))
#endif

                                    /***********************************/
                                    /* Optimize for space...           */
                                    /***********************************/
typedef long            IntX32;
#define MAX_INTX32      MAX_INT32
#define MIN_INTX32      MIN_INT32

typedef unsigned long   CardX32;
#define MAX_CARDX32     MAX_CARD32

typedef short           IntX16;
#define MAX_INTX16      MAX_INT16
#define MIN_INTX16      MIN_INT16

typedef unsigned short  CardX16;
#define MAX_CARDX16     MAX_CARD16

typedef signed char     IntX8;
#define MAX_INTX8       MAX_INT8
#define MIN_INTX8       MIN_INT8

typedef unsigned char   CardX8;
#define MAX_CARDX8      MAX_CARD8

typedef char            ByteX8;
/* MAX_BYTEX8 and MIN_BYTEX8 are omitted because it is not known       */
/* whether if a ByteX8 (char) is signed or unsigned.                   */

/***********************************************************************/
/* Use this type for use with variables connected with system calls.   */
/***********************************************************************/
typedef int             IntN;
#define MAX_INTN        ((IntN)((1<<(sizeof(IntN)*8-1))-1))
#define MIN_INTN        ((IntN)(1<<(sizeof(IntN)*8-1)))

typedef unsigned        CardN;
#define MAX_CARDN       ((CardN)(MAX_INTN|MIN_INTN))

typedef short           ShortN;
typedef unsigned short  ShortCardN;
typedef long            LongN;
typedef unsigned long   LongCardN;
typedef float           FloatN;
typedef double          DoubleN;

/***********************************************************************/
/* Use these macros with scanf and printf.                             */
/***********************************************************************/
#define FMT_32          "l"
#define FMT_16          "h"

#if FORCE_INT_BITS == 32
#  define FMT_X         FMT_32
#elif FORCE_INT_BITS == 16
#  define FMT_X         FMT_16
#endif

#define FMT_N           ""

#endif/*_NUMTYPES_H*/
