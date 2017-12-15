/* @(#)CM_VerSion ASMath.h atm09 1.2 16563.eco sum= 55706 atm09.004 */
/* @(#)CM_VerSion ASMath.h atm08 1.2 16390.eco sum= 08986 atm08.007 */
/* @(#)CM_VerSion ASMath.h 3011 1.2 21403.eco sum= 64692 3011.002 */
/* @(#)CM_VerSion ASMath.h 3010 1.3 19979.eco sum= 63982 3010.102 */
/*
  ASMath.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	_H_ASMATH
#define	_H_ASMATH

#include "supportenvironment.h"
#include "supportasbasic.h"

#if !NOT_USE_NATIVE_MATH_LIBS
#include <math.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Math Library Interface
 */

#if !NOT_USE_NATIVE_MATH_LIBS

#define os_atan2 atan2
#define os_ceil  ceil
#define os_cos   cos
#define os_exp   exp
#define os_floor floor
#define os_frexp frexp
#define os_ldexp ldexp
#define os_log   log
#define os_log10 log10
#define os_modf  modf
#define os_pow   pow
#define os_sin   sin
#define os_sqrt  sqrt

#else /* NOT_USE_NATIVE_MATH_LIBS */

/* The following procedures correspond to standard C library procedures,
   but with an "os_" prefix. They can normally be implemented as a
   veneer over the standard C library procedures, which are usually
   accessed via math.h. The purpose of renaming them is to insulate
   the client from details of the C library interface on
   the target machine, and also to allow modified implementations to
   be substituted without causing name conflicts.

   The main difference between these procedures and the ones in the
   C library is in their error behavior. When an error is detected,
   all the procedures below raise an exception, using the facilities
   defined in except.h; specifically, they execute one of:

     RAISE(ecRangeCheck, NULL);
     RAISE(ecUndefResult, NULL);

   where ecRangeCheck and ecUndefResult are error codes defined in
   except.h. The error code ecRangeCheck indicates that the function is
   mathematically not defined for the arguments given (for example,
   os_sqrt(x) where x is negative). The error code ecUndefResult
   indicates that the result cannot be represented as a floating point
   number.

   In contrast, C library procedures usually indicate an error either
   by storing a code in the external variable "errno" or by generating
   a signal that can be caught by a SIGFPE signal handler (some
   libraries do both, depending on the cause of the error). In the
   former case, a veneer procedure must initialize errno before calling
   the C library procedure, test it afterward, and call "RAISE" if
   an error is indicated. In the latter case, "RAISE" may be called
   from within an implementation-provided SIGFPE signal handler;
   no checking is required in the veneer procedure.

   All floating point types are explicitly declared to be double
   so as to eliminate any uncertainty about whether or not the C
   compiler performs automatic float to double promotion.
 */

extern double os_atan2( double num, double den);
/* Returns the angle (in radians between -pi and pi) whose tangent is
   num/den. Either num or den may be zero, but not both. The signs
   of num and den determine the quadrant in which the result will
   lie: a positive num yields a result in the positive y plane;
   a positive den yields a result in the positive x plane.
 */

extern double os_ceil( double x);
/* Returns the least integer value greater than or equal to x. */

extern double os_cos( double x);
/* Returns the cosine of x, where x represents an angle in radians. */

extern double os_exp( double x);
/* Returns the exponential of x. */

extern double os_floor( double x);
/* Returns the greatest integer value less than or equal to x. */

extern double os_frexp( double x, ASInt32 * exp);
/* Stores an exponent in *exp and returns a value in the interval
   [0.5, 1.0) such that value * 2**exponent = x. If x is zero,
   both *exp and the returned value are set to zero.
 */

extern double os_ldexp( double x, ASInt32 exp);
/* Returns the value x * 2**exp. If the result is too large to represent,
   an exception is raised; if an underflow occurs, zero is substituted. */

extern double os_log( double x);
/* Returns the natural (base e) logarithm of x. */

extern double os_log10( double x);
/* Returns the common (base 10) logarithm of x. */

extern double os_modf( double x, double * ipart);
/* Breaks the value x into integer and fractional parts; stores the
   integer part in *ipart and returns the fractional part. Both
   results have the same sign as x.
 */

extern double os_pow( double base, double exp);
/* Returns base**exp, i.e., base raised to the exp power. If base is
   negative, exp must be an integer; otherwise an error is raised.
 */

extern double os_sin( double x);
/* Returns the sine of x, where x represents an angle in radians. */

extern double os_sqrt( double x);
/* Returns the square root of x. */

#endif /* NOT_USE_NATIVE_MATH_LIBS */

/*
   The following procedures correspond to standard C library procedures,
   but with an "os_" prefix. They can normally be implemented as a
   veneer over the standard C library procedures. However, some C
   libraries do not have such procedures. If that is the case, os_ecvt
   and os_fcvt can be implemented using the C library "sprintf";
   os_isinf and os_isnan can be dummies that always return false.

   No errors or exceptions can be generated by these procedures.
 */

/* Exported Procedures */

extern char *os_ecvt( double value, int ndigits, int * decpt, int * sign);
/* Converts value to a null-terminated string containing ndigits
   ASCII decimal digits and returns a pointer to the resulting string.
   The string contains no decimal point or other punctuation, only digits.
   The first digit in this string is the first significant digit of
   the number. The result is rounded to the specified number of digits.

   The position of the decimal point relative to the start of the string
   is returned in *decpt: zero means the decimal point immediately
   precedes the first digit; positive means *decpt places to the right;
   negative means -*decpt places to the left. The sign is returned in
   *sign: zero means positive; nonzero means negative.

   The string returned by this procedure is contained in static storage
   that is part of the implementation of the procedure itself. Thus,
   the caller must copy the string before calling os_ecvt (or os_fcvt)
   again.
 */

extern char *os_fcvt( double value, int ndigits, int * decpt, int * sign);
/* Similar to os_ecvt. However, ndigits specifies the number of digits
   to be produced to the right of the decimal point, rather than the
   total number of digits. Thus, the resulting string can have more
   or fewer than ndigits characters according to whether the first
   significant digit is to the left or to the right of the decimal
   point. (If there are no significant digits among the first ndigits
   digits to the right of the decimal point, the result string is empty.)
 */

extern ASBool os_isinf(double d);
/* Returns true if the argument d represents "infinity", either positive
   or negative. If the machine's floating point format does not have
   a representation for "infinity", returns false always.
 */

extern ASBool os_isnan(double d);
/* Returns true if the argument d is an invalid floating point number
   other than infinity.
 */

extern double os_atof( const char * str);
/* Converts a string *str to a floating point number and returns it.
   The string consists of zero or more tabs and spaces, an optional
   sign, a series of digits optionally containing a decimal point,
   and optionally an exponent consisting of "E" or "e", an optional
   sign, and one or more digits. Processing of the string terminates
   at the first character not conforming to the above description
   (the character may but need not be null). An error results if
   no number can be recognized or if it is unrepresentable.
 */

/* Inline procedures */


#define os_abs(x)	((x)<0?-(x):(x))
/* Returns the absolute value of whatever is passed in, no matter the
   type. Whatever is passed in may be evaluated twice, so it should never be
   used with an expression - those can be passed to  either os_labs for
   an integer expression or os_fabs for a float expression.
*/

#define os_labs(x)	((ASInt32)((x)<0?-(x):(x)))
/* Similar to os_abs, but restricted to integers */

/* See also os_fabs (below) */



/*
 * Floating Point Interface
 */

/* Data Structures */

typedef union {	/* Floating point representations */
  float native;	/* whatever the native format is */
#if SWAPBITS
  struct {unsigned fraction: 23, exponent: 8, sign: 1;} ieee;
    /* IEEE float, low-order byte first */
#else    /*  SWAPBITS  */
  struct {unsigned sign: 1, exponent: 8, fraction: 23;} ieee;
    /* IEEE float, high-order byte first */
#endif   /*  SWAPBITS  */
  unsigned char bytes[4];
    /* raw bytes, in whatever order they appear in memory */
  } ASFloatRep;

/* Exported Procedures */

/* The following procedures convert floating point numbers between
   this machine's "native" representation and IEEE standard
   representations with either byte order. They all pass arguments
   and results by pointer so that C does not perform inappropriate
   conversions; in any call, the "from" and "to" pointers must be
   different. They all raise the exception ecUndefResult if the
   source value cannot be represented in the destination format
   (e.g., dynamic range exceeded, NAN, etc.)

   The conversions are done in-line when the native representation
   is the same as IEEE (perhaps with bytes reordered); otherwise they
   are done by calling a procedure. The macros and the procedures
   have the same semantics otherwise; they operate according to
   the procedure declarations given below.

   The argument designated as "float *" is a native float; it must not
   be a formal parameter of a procedure, which may be a double even
   if declared to be a float. The argument designated as "FloatRep *"
   is an IEEE floating point number. Although its representation may
   be of little interest to the caller, it must be declared as (or cast
   to) type "FloatRep *" in order to satisfy alignment requirements.
*/

#define ASCopySwap4(from, to) \
  (((ASFloatRep *) to)->bytes[0] = ((ASFloatRep *) from)->bytes[3], \
   ((ASFloatRep *) to)->bytes[1] = ((ASFloatRep *) from)->bytes[2], \
   ((ASFloatRep *) to)->bytes[2] = ((ASFloatRep *) from)->bytes[1], \
   ((ASFloatRep *) to)->bytes[3] = ((ASFloatRep *) from)->bytes[0] )

#if AS_IEEEFLOAT

#if SWAPBITS
#define IEEELowToNative(from, to) *(to) = ((ASFloatRep *) from)->native
#define NativeToIEEELow(from, to) ((ASFloatRep *) to)->native = *(from)
#define IEEEHighToNative(from, to) ASCopySwap4((from), (to))
#define NativeToIEEEHigh(from, to) ASCopySwap4((from), (to))
#else   /*  SWAPBITS  */
#define IEEEHighToNative(from, to) *(to) = ((ASFloatRep *) from)->native
#define NativeToIEEEHigh(from, to) ((ASFloatRep *) to)->native = *(from)
#define IEEELowToNative(from, to) ASCopySwap4((from), (to))
#define NativeToIEEELow(from, to) ASCopySwap4((from), (to))
#endif  /*  SWAPBITS  */

#else   /*  AS_IEEEFLOAT  */

extern void IEEEHighToNative( ASFloatRep * from, float * to );
/* Converts from IEEE high-byte-first float to native float. */

extern void NativeToIEEEHigh( float * from, ASFloatRep * to);
/* Converts from native float to IEEE high-byte-first float. */

extern void IEEELowToNative( ASFloatRep * from, float * to);
/* Converts from IEEE low-byte-first float to native float. */

extern void NativeToIEEELow(float * from, ASFloatRep * to);
/* Converts from native float to IEEE low-byte-first float. */

#endif  /*  AS_IEEEFLOAT  */

/* Inline Procedures */

/* The following macros implement comparisons of float values
   with zero. If AS_IEEESOFT is true (defined in ASEnv.h), these
   comparisons are done more efficiently than the general floating
   point compare subroutines would typically do them.
   Note: the argument r must be a simple variable (not an expression)
   of type float; it must not be a formal parameter
   of a procedure, which is a double even if declared to be a float.
   The RealScheck(r) macro is there to protect against the mistaken use
   of the macro on procedure parameters, and converts the test into
   a normal (slow) comparison in that case.
 */

#if AS_IEEESOFT
#define ASRtoILOOPHOLE(r) (*(ASInt32 *)(&(r)))
#define ASRealScheck(r) (sizeof(r)==sizeof(float))
#define ASRealEq0(r) (ASRealScheck(r)?((ASRtoILOOPHOLE(r)<<1)==0):(r)==0)
#define ASRealNe0(r) (ASRealScheck(r)?((ASRtoILOOPHOLE(r)<<1)!=0):(r)!=0)
#define ASRealGt0(r) (ASRealScheck(r)?(ASRtoILOOPHOLE(r)>0):(r)>0)
#define ASRealGe0(r) (ASRealScheck(r)?((ASRtoILOOPHOLE(r)>=0)||ASRealEq0(r)):(r)>=0)
#define ASRealLt0(r) (ASRealScheck(r)?((ASRtoILOOPHOLE(r)<0)&&ASRealNe0(r)):(r)<0)
#define ASRealLe0(r) (ASRealScheck(r)?(ASRtoILOOPHOLE(r)<=0):(r)<=0)
#else   /*  AS_IEEESOFT  */
#define ASRealEq0(r) ((r)==((float)0.0))
#define ASRealNe0(r) ((r)!=((float)0.0))
#define ASRealGt0(r) ((r)>((float)0.0))
#define ASRealGe0(r) ((r)>=((float)0.0))
#define ASRealLt0(r) ((r)<((float)0.0))
#define ASRealLe0(r) ((r)<=((float)0.0))
#endif  /*  AS_IEEESOFT  */

#define AS_RAD(a) ((a)*1.745329251994329577e-2)
/* Converts the float or double value a from degrees to radians. */

#define AS_DEG(x) ((x)*57.29577951308232088)
/* Converts the float or double value x from radians to degrees. */

#if AS_IEEEFLOAT
#define ASIsValidReal(pReal) (((ASFloatRep *) pReal)->ieee.exponent != 255)
#else   /*  AS_IEEEFLOAT  */
extern ASBool ASIsValidReal(float * pReal);
#endif  /*  AS_IEEEFLOAT  */
/* Returns true iff the float at *pReal is a valid floating point number
   (i.e., not infinity, NaN, reserved operand, etc.) */

/*
 * Fixed Point interface
 */

/* Data Structures */

/* A fixed point number consists of a sign bit, i integer bits, and f
   fraction bits; the underlying representation is simply an integer.
   The integer n represents the fixed point value n/(2**f). The current
   definition is pretty heavily dependent on an integer being 32 bits.

   ASFixed	i = 15, f = 16, range [-32768, 32768)

   The type "ASFixed", defined in the ASBasic interface, is simply
   a typedef for integer and is type-equivalent to integer as far
   as the C compiler is concerned.

   All procedures, including the conversions from float, return a
   correctly signed "infinity" value ASFixedPosInf or ASFixedNegInf
   if an overflow occurs; they return zero if an underflow occurs.
   No overflow or underflow traps ever occur; it is the caller's
   responsibility either to detect an out-of-range result or to
   prevent it from occurring in the first place.

   All procedures compute exact intermediate results and then round
   according to some consistent rule such as IEEE "round to nearest".
 */

#define ASFixedPosInf ASMAXInt32
#define ASFixedNegInf ASMINInt32

/* Exported Procedures */

extern ASFixed ASFixedMul(ASFixed x, ASFixed y);
/* Multiples two ASFixed numbers and returns a ASFixed result. */

extern ASFixed ASFixedDiv(ASFixed x, ASFixed y);
/* Divides the ASFixed number x by the ASFixed number y and returns a
   ASFixed result. */

extern ASFixed ASTFixedDiv(ASFixed x, ASFixed y);
/* Same as fixdiv except that instead of rounding to the nearest
   representable value, it truncates toward zero. It thus guarantees
   that |q| * |y| <= |x|, where q is the returned quotient. */

extern ASFixed ASUFixedRatio( ASUns32 x, ASUns32 y);
/* Divides the ASUns32 x by the ASUns32 y and returns a ASFixed result. */


/* The following type conversions pass their float arguments and results
   by value; the C language forces them to be of type double.
 */

extern double ASFixedToDouble( Fixed x);
/* Converts the ASFixed number x to a double. */

extern Fixed ASDoubleToFixed( double  d);
/* Converts the double d to a ASFixed. */

/* The following type conversions pass their float arguments and results
   by pointer. This circumvents C's usual conversion of float arguments
   and results to and from double, which can have considerable
   performance consequences. Caution: do not call these procedures
   with pointers to float formal parameters, since those are
   actually of type double!
 */

extern void ASFixedToPFloat( ASFixed x, float * pf);
/* Converts the ASFixed number x to a float and stores the result at *pf. */

extern ASFixed ASPFloatToFixed(float * pf);
/* Converts the float number at *pf to a ASFixed and returns it. */


/* Inline Procedures */

/* ASInt32 <-> ASFixed */
#define ASInt32ToFixed(i)		((ASFixed)(i) << 16)
/* Converts the integer x to a ASFixed and returns it. */

#define ASFixedRoundToInt32(f)	((ASInt32) (((f) + (1<<15)) >> 16))
/* Converts the ASFixed number x to an integer, rounding it to the
   nearest integer value, and returns it. */

#define ASFixedTruncToInt32(f)	((ASInt32) ((f) >> 16))
/* Converts the ASFixed number x to an integer, truncating it to the
   next lower integer value, and returns it. */

#define ASFCeil(x) (((ASInt32)((x)+0xFFFFL))>>16)
/* Returns the smallest integer greater that or equal to the ASFixed number
   x. */

#define ASFRound4(x) (((((ASInt32)(x))+(1<<17))>>18)<<2)
/* Converts the ASFixed number x to an integer, rounding it to the
   nearest multiple of 4, and returns it. */

#define ASFTruncF(x) ((ASFixed)((ASInt32)(x) & 0xFFFF0000L))
#define ASFCeilF(x) ((ASFixed) (((x) + 0xFFFFL) & 0xFFFF0000L))
#define ASFRoundF(x) ((((ASInt32)(x))+0x8000L) & (ASInt32)0xFFFF0000L)
/* Like above but return ASFixed instead of integer. */

#define ASFFloorF4(x) ((ASFixed)((ASInt32)(x) & 0xFFFC0000L))
/* Takes ASFixed number x, rounding it down to the nearest 
   multiple of 4, and returns it. */

#define ASFCeilF4(x) (ASFFloorF4((x) + 0x3FFFF))
/* Takes ASFixed number x, rounding it up to the nearest
   multiple of 4, and returns it. */


/*
 * Double precision integer interface
 */

/* Data structures */

typedef struct _t_ASInt64struct {		/* Double precision integer */
#if SWAPBITS
  ASUns32 l; ASInt32 h;
#else   /*  SWAPBITS  */
  ASInt32 h; ASUns32 l;
#endif  /*  SWAPBITS  */
  } ASInt64struct;

/* Exported Procedures */

/* Note: for os_dpneg, os_dpadd, os_dpsub, and os_muldiv, the arguments
   and results declared as ASInt32 or Int64struct may equally well be signed fixed
   point numbers with their binary point in an arbitrary position.
   This does not hold for os_dpmul or os_dpdiv. */

extern void os_dpneg( ASInt64struct* pa, ASInt64struct* pResult );
/* Computes the negative of the double-precision integer in *pa
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as pa. */

extern void os_dpadd( ASInt64struct* pa, ASInt64struct* pb, ASInt64struct* pResult );
/* Computes the sum of the double-precision integers in *pa and *pb
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as either pa or pb. */

extern void os_dpsub( ASInt64struct* pa, ASInt64struct* pb, ASInt64struct* pResult );
/* Computes the difference of the double-precision integers in *pa and *pb
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as either pa or pb. */

extern void os_dpmul( ASInt32 a, ASInt32 b, ASInt64struct* pResult );
/* Computes the signed product of a and b and stores the double-precision
   result in *pResult. It is not possible for an overflow to occur. */

extern ASInt32 os_dpdiv( ASInt64struct* pa, ASInt32 b, ASBool round );
/* Divides the double-precision integer in *pa by the single-precision
   integer b and returns the quotient. If round is true, the quotient
   is rounded to the nearest integer; otherwise it is truncated
   toward zero. If an overflow occurs, MAXinteger or MINinteger is
   returned (no overflow or underflow traps ever occur). */

extern ASInt32 os_muldiv( ASInt32 a, ASInt32 b, ASInt32 c, ASBool round );
/* Computes the expression (a*b)/c, retaining full intermediate precision;
   the result is rounded or truncated to single precision only after the
   division. The descriptions of os_dpmul and os_dpdiv pertain here also. */

#ifdef __cplusplus
}
#endif
#endif	 /* _H_ASMATH */
