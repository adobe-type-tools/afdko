/* @(#)CM_VerSion fp.h atm10 1.2 16593.eco sum= 24684 atm10.002 */
/* @(#)CM_VerSion fp.h atm09 1.2 16563.eco sum= 21005 atm09.004 */
/* @(#)CM_VerSion fp.h atm08 1.4 16380.eco sum= 29629 atm08.007 */
/* @(#)CM_VerSion fp.h atm07 1.3 16010.eco sum= 31035 atm07.005 */
/* @(#)CM_VerSion fp.h atm06 1.5 14053.eco sum= 33392 */
/* @(#)CM_VerSion fp.h atm05 1.4 11769.eco sum= 17673 */
/* @(#)CM_VerSion fp.h atm04 1.6 07436.eco sum= 64835 */
/* $Header$ */
/* 
  fp.h	-- Interface to Floating/Fixed package.
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef	FP_H
#define	FP_H

#include "supportpublictypes.h"
/*
 * Math Library Interface
 */

/* This interface incorporates, by reference, the following procedures
   from the standard C math library, as documented in most C manuals;
   it may include others as well, but is not required to.

   acos, asin, atan, atan2, atod, atof, ceil, cos, ecvt, exp, fcvt,
   floor, frexp, ldexp, log, log10, modf, pow, sin, sqrt
 */

#include <math.h>


#ifndef WINATM
#define WINATM 0
#endif

#if WINATM

#include "atmtypes.h"

#include "fpadjust.h"

#else  /* WINATM */

#ifndef FIXEDFUNC
#define FIXEDFUNC
#endif /* FIXEDFUNC */

#endif  /* WINATM */


/*
 * Floating Point Interface
 */

/* Exported Procedures */

#define os_ceil ceil
#define os_floor floor
#define os_fabs fabs
#define os_log10 log10
#define os_sqrt sqrt
#define os_atof atof

/* Inline Procedures */

/* The following macros implement comparisons of real (float) values
   with zero. If IEEESOFT is true (defined in environment.h), these
   comparisons are done more efficiently than the general floating
   point compare subroutines would typically do them.
   Note: the argument r must be a simple variable (not an expression)
   of type real (float); it must not be a formal parameter
   of a procedure, which is a double even if declared to be a float.
 */

#if IEEESOFT
#define RtoILOOPHOLE(r) (*(integer *)(&(r)))
#define RealEq0(r) ((RtoILOOPHOLE(r)<<1)==0)
#define RealNe0(r) ((RtoILOOPHOLE(r)<<1)!=0)
#define RealGt0(r) (RtoILOOPHOLE(r)>0)
#define RealGe0(r) ((RtoILOOPHOLE(r)>=0)||RealEq0(r))
#define RealLt0(r) ((RtoILOOPHOLE(r)<0)&&RealNe0(r))
#define RealLe0(r) (RtoILOOPHOLE(r)<=0)
#else /* IEEESOFT */
#define RealEq0(r) ((r)==0.0)
#define RealNe0(r) ((r)!=0.0)
#define RealGt0(r) ((r)>0.0)
#define RealGe0(r) ((r)>=0.0)
#define RealLt0(r) ((r)<0.0)
#define RealLe0(r) ((r)<=0.0)
#endif /* IEEESOFT */

#ifndef fabs
#define fabs(x) (RealLt0(x)?-(x):(x))
/* Returns the absolute value of x, which must be a simple variable
   of type real (float), as described above. */
#endif /* fabs */

#define RAD(a) ((a)*1.745329251994329577e-2)
/* Converts the float or double value a from degrees to radians. */


/*
 * Fixed Point interface
 */

/* Data Structures */

/* A fixed point number consists of a sign bit, i integer bits, and f
   fraction bits; the underlying representation is simply an integer.
   The integer n represents the fixed point value n/(2**f). The current
   definition is pretty heavily dependent on an integer being 32 bits.

   There are three kinds of fixed point numbers:
	Fixed	i = 15, f = 16, range [-32768, 32768)
	Frac	i = 1, f = 30, range [-2, 2)
	UFrac	i = 2, f = 30, unsigned, range [0, 4)
   The type "Fixed", defined in the basictypes interface, is simply
   a typedef for integer and is type-equivalent to integer as far
   as the C compiler is concerned.

   Within the fp interface, the types Fixed, Frac, and UFrac are used to
   document the kinds of fixed point numbers that are the arguments
   and results of procedures. However, most clients of this interface
   declare variables to be of type Fixed, or occasionally of type integer,
   without specifying which kind of fixed point number is actually
   intended. Most Fixed numbers are of the first kind; the other two
   are provided as a convenience to certain specialized algorithms in
   the graphics machinery.

   All procedures, including the conversions from real, return a
   correctly signed "infinity" value FixedPosInf or FixedNegInf
   if an overflow occurs; they return zero if an underflow occurs.
   No overflow or underflow traps ever occur; it is the caller's
   responsibility either to detect an out-of-range result or to
   prevent it from occurring in the first place.

   All procedures compute exact intermediate results and then round
   according to some consistent rule such as IEEE "round to nearest".
 */

#if (ARCH_64BIT == 1)
typedef      int /* Fixed, */ Frac, UFrac, *PFrac;
#else
typedef long int /* Fixed, */ Frac, UFrac, *PFrac;
#endif

#define FixedPosInf MAXinteger
#define FixedNegInf MINinteger

/* Pass arguments in registers, if using a Metrowerks compiler and target is a 68K */

/* Exported Procedures */

#if REGISTER_ARGS && ASM_fixmul
extern Fixed FIXEDFUNC fixmul_reg (Fixed  x MW_D0, Fixed  y MW_D1);
#define fixmul( a, b)		fixmul_reg( a, b)
#else
extern Fixed FIXEDFUNC fixmul (Fixed  x, Fixed  y);
#endif
/* Multiples two Fixed numbers and returns a Fixed result. */

extern Frac FIXEDFUNC fracmul (Frac  x, Frac  y);
/* Multiples two Frac numbers and returns a Frac result. */

#define fxfrmul(x,y) ((Fixed)fracmul((Frac) (x), y))
/* extern Fixed fxfrmul ( Fixed  x, Frac  y);
   Multiplies a Fixed number with a Frac number and returns a Fixed
   result. The order of the Fixed and Frac arguments does not matter. */

#if REGISTER_ARGS && ASM_fixdiv
extern Fixed FIXEDFUNC fixdiv_reg (Fixed  x MW_D0, Fixed  y MW_D1);
#define fixdiv( a, b)		fixdiv_reg( a, b)
#else
extern Fixed FIXEDFUNC fixdiv (Fixed  x, Fixed  y);
#endif
/* Divides the Fixed number x by the Fixed number y and returns a
   Fixed result. */

extern Frac FIXEDFUNC fracdiv (Frac  x, Frac  y);
/* Divides two Frac numbers and returns a Frac result. */

#define fracratio(x,y) (fixdiv((Fixed) (x), (Fixed) (y)))
/* extern Fixed fracratio (Frac  x, Frac  y);
   Divides the Frac number x by the Frac number y and returns a
   Fixed result. */

extern Fixed FIXEDFUNC ufixratio (Card32  x, Card32  y);
/* Divides the unsigned integer x by the unsigned integer y and
   returns a Fixed result. */

extern Frac FIXEDFUNC fixratio (Fixed  x, Fixed  y);
/* Divides the Fixed number x by the Fixed number y and returns a
   Frac result. */

extern UFrac FIXEDFUNC fracsqrt (UFrac  x);
/* Computes the square root of the UFrac number x and returns a
   UFrac result. */

/* The following type conversions pass their float arguments and results
   by value; the C language forces them to be of type double.
 */

extern double ASFixedToDouble (Fixed  x);
#define fixtodbl ASFixedToDouble
/* Converts the Fixed number x to a double. */

extern Fixed ASDoubleToFixed (double  d);
#define dbltofix ASDoubleToFixed
/* Converts the double d to a Fixed. */

extern double fractodbl (Frac  x);
/* Converts the Frac number x to a double. */

extern Frac dbltofrac (double  d);
/* Converts the double d to a Frac. */

/* The following type conversions pass their float arguments and results
   by pointer. This circumvents C's usual conversion of float arguments
   and results to and from double, which can have considerable
   performance consequences. Caution: do not call these procedures
   with pointers to float ("real") formal parameters, since those are
   actually of type double!
 */

extern procedure fixtopflt (Fixed  x, float *  pf);
/* Converts the Fixed number x to a float and stores the result at *pf. */

extern Fixed pflttofix (float *  pf);
/* Converts the float number at *pf to a Fixed and returns it. */

extern procedure fractopflt (Frac  x, float *  pf);
/* Converts the Frac number x to a float and stores the result at *pf. */

extern Frac pflttofrac (float *  pf);
/* Converts the float number at *pf to a Frac and returns it. */

/* new code for DDR and anti-alias 
   probably make some/all of this macros later
*/

extern Fixed FCeilFModN(Fixed x, Card16 n);
extern Fixed FFloorFModN(Fixed x, Card16 n);
extern Fixed FRoundFModN(Fixed x, Card16 n);
extern Int32 ICeilModN(Int32 x, Card16 n);
extern Int32 IFloorModN(Int32 x, Card16 n);
extern Fixed FFloorFModNR(Fixed v, Card16 n, Int16 r);
extern Fixed FCeilFModNR(Fixed v, Card16 n, Int16 r);
extern Fixed FRoundFModNR(Fixed v, Card16 n, Int16 r);


/* Inline Procedures */

#define FixInt(x) ((Fixed)((Card32)(x) << 16))
/* Converts the integer x to a Fixed and returns it. */

#define FTrunc(x) ((integer)((x)>>16))
/* Converts the Fixed number x to an integer, truncating it to the
   next lower integer value, and returns it. */

#define FTruncF(x) ((integer)((x) & 0xFFFF0000))
/* Returns the Fixed value that represents the next lower integer value. */

#define FRound(x) ((integer)(((x)+(FIXEDHALF))>>16))
/* Converts the Fixed number x to an integer, rounding it to the
   nearest integer value, and returns it. */

#define FRoundF(x) ((integer)(((x)+(FIXEDHALF)) & 0xFFFF0000))
/* Returns x as a Fixed number, with its integer part rounded to the
   nearest integer value, and its fraction part 0. */

#define FCeil(x) (FTrunc((x) + 0xFFFF))
#define FCeilF(x) (FTruncF((x) + 0xFFFF)) 

#define FracToFixed(f) ((f) >> 14)
#define FixedToFrac(f) ((f) << 14)

#endif	/* FP_H */
/* v004 taft Tue Nov 22 14:09:28 PST 1988 */
/* v005 brotz Fri Feb 17 11:16:46 PST 1989 */
