/* @(#)CM_VerSion ASBasic.h atm09 1.2 16563.eco sum= 49471 atm09.004 */
/* @(#)CM_VerSion ASBasic.h atm08 1.3 16255.eco sum= 59023 atm08.003 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 * ASBasic.h -- Adobe Standard Definitions of "Basic" Types
 *
 * BEWARE: Compilation parameter settings must be the same for all
 *         code that #includes this header file and will get linked
 *         into the same application, e.g. product code as well as
 *         all separately-built libraries.
 *
 * REQUIRED COMPILATION PARAMETERS:
 *	It is assumed that the following are defined when this is compiled:
 *		AS_ISP
 *		AS_OS
 *
 * OPTIONAL COMPILATION PARAMETERS:
 *  AS_FORCE_X_16BITS (See below)
 *  AS_FORCE_X_32BITS (See below)
 *  AS_SIZE_T (See below)
 *
 */


#ifndef _H_ASBASIC
#define _H_ASBASIC

#include <stddef.h>	/* Include standard ANSI C stuff: size_t, NULL etc */

/* Signed and unsigned integer types, and pointers
   to these.

   The ASInt64 and ASUns64 types are defined only
   if ARCH_64BIT is defined to be 1.

   |  Typedef  | Bits | Max Value  | Min Value  |
   +-----------+------+------------+------------+
   |  ASInt8   |   8  | ASMAXInt8  | ASMINInt8  |
   | *ASInt8P  |      |            |            |
   +-----------+------+------------+------------+
   |  ASUns8   |   8  | ASMAXUns8  | ASMINUns8  |
   | *ASUns8P  |      |            |            |
   +-----------+------+------------+------------+
   |  ASInt16  |   16 | ASMAXInt16 | ASMINInt16 |
   | *ASInt16P |      |            |            |
   +-----------+------+------------+------------+
   |  ASUns16  |   16 | ASMAXUns16 | ASMINUns16 | 
   | *ASUns16P |      |            |            |
   +-----------+------+------------+------------+
   |  ASInt32  |   32 | ASMAXInt32 | ASMINInt32 |
   | *ASInt32P |      |            |            |
   +-----------+------+------------+------------+
   |  ASUns32  |   32 | ASMAXUns32 | ASMINUns32 |
   | *ASUns32P |      |            |            |
   +-----------+------+------------+------------+
   |  ASInt64  |   64 | ASMAXInt64 | ASMINInt64 |
   | *ASInt64P |      |            |            |
   +-----------+------+------------+------------+
   |  ASUns64  |   64 | ASMAXUns64 | ASMINUns64 |
   | *ASUns64P |      |            |            |
   +-----------+------+------------+------------+
*/

#if ARCH_64BIT

typedef int                 ASInt32, *ASInt32P;
typedef unsigned int        ASUns32, *ASUns32P;
typedef long int            ASInt64, *ASInt64P;
#define ASMAXInt64          ((ASInt64)0x7FFFFFFFFFFFFFFF)
#define ASMINInt64          ((ASInt64)0x8000000000000000)

typedef unsigned long int   ASUns64, *ASUns64P;
#define ASMAXUns64          ((ASUns64)0xFFFFFFFFFFFFFFFF)

#else /* ARCH_64BIT */

typedef long int            ASInt32, *ASInt32P;
typedef unsigned long int   ASUns32, *ASUns32P;

#endif /* ARCH_64BIT */

typedef signed char         ASInt8, *ASInt8P;
typedef short int           ASInt16, *ASInt16P;

#define ASMAXInt8           ((ASInt8)0x7F)
#define ASMINInt8           ((ASInt8)0x80)
#define ASMAXInt16          ((ASInt16)0x7FFF)
#define ASMINInt16          ((ASInt16)0x8000)
#define ASMAXInt32          ((ASInt32)0x7FFFFFFF)
#define ASMINInt32          ((ASInt32)0x80000000)

/* other unsigned integer types */
typedef unsigned char       ASUns8, *ASUns8P;
typedef unsigned short int  ASUns16, *ASUns16P;

#define ASMAXUns8           ((ASUns8)0xFF)
#define ASMINUns8           ((ASUns8)0x00)
#define ASMAXUns16          ((ASUns16)0xFFFF)
#define ASMINUns16          ((ASUns16)0x0000)
#define ASMAXUns32          ((ASUns32)0xFFFFFFFF)
#define ASMINUns32          ((ASUns32)0x00000000)

/* ASIntX, ASUnsX, ASSize_t

   The definitions of ASIntX, ASUnsX and ASSize_t differ
   from "int", "unsigned int" and "size_t" because code using
   them indicates thereby that the implementation is free to
   use whatever number of bits it chooses, but at least 16.
   
   The natural definitions for these are "int", "unsigned
   int" and "size_t", of course, but it is possible to define
   them to be (say) 16 or 32 bits when necessary, for example
   when an ASIntX type is used to specify an interface between
   modules compiled by different compilers making different
   choices of the size of an int. Generally one has no such
   flexibility in specifying the size of an int.
   
   To force X to be 16, define AS_FORCE_X_16BITS==1
   
   To force X to be 32, define AS_FORCE_X_32BITS==1
   
   Don't define both of the above to be 1 !!
   
   To specify the definition of ASSize_t, define AS_SIZE_T
   to be the desired type.

 */
#ifndef	AS_FORCE_X_16BITS
#define	AS_FORCE_X_16BITS	0
#endif	/* AS_FORCE_X_16BITS */

#ifndef AS_FORCE_X_32BITS
#define AS_FORCE_X_32BITS   0
#endif  /* AS_FORCE_X_32BITS */

#if AS_FORCE_X_16BITS
typedef short int           ASIntX;
typedef unsigned short int  ASUnsX;
#elif AS_FORCE_X_32BITS && !ARCH_64BIT
typedef long int            ASIntX;
typedef unsigned long int   ASUnsX;
#else
typedef int                 ASIntX;
typedef unsigned int        ASUnsX;
#endif

/* ASSize_t - canonical type for sizes of things in bytes. */
#ifndef AS_SIZE_T
#define AS_SIZE_T size_t
#endif
typedef AS_SIZE_T ASSize_t;


/* boolean */
typedef ASUnsX              ASBool;

/* "true" and "false" are usually in stddef.h, but not always ;) ... */
#ifndef true
#define true	1
#endif
#ifndef false
#define false	0
#endif

/* Fixed-point numbers

   The ASFixed type is a 32-bit quantity representing a rational number
   with 16-bits for the whole number part and 16 bits for the fractional part.

   Addition, subtraction and negation with ASFixed values can be done
   with + and - unless you care about overflow, in which case you should
   use library routines like the ones defined in ASFixed.h.
*/

typedef ASInt32             ASFixed;

/* typedef of enums

   These types have been introduced to permit explicit control over
   the sizes of enums, e.g for exporting them through plug-in APIs.
   Sadly, enums are not sized the same way on all platforms.
*/
#if OS == os_windows95 || OS == os_windowsNT || OS == os_windows3

typedef ASInt16 ASEnum8;
typedef ASInt16 ASEnum16;

#else

typedef ASInt8 ASEnum8;
typedef ASInt16 ASEnum16;

#endif

#endif /* _H_ASBASIC */
