/* 
  fixed.c
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
Fixed point multiply, divide, and conversions -- machine independent.
See fp.h for the specification of what these procedures do.

The double-to-int conversion is assumed to truncate rather than round;
this is specified by the C language manual. The direction of truncation
is machine-dependent, but is toward zero rather than toward minus
infinity on the Vax and Sun. This explains the peculiar way in which
fixmul and fixdiv do rounding.
*/

#include "supportpublictypes.h"
#include "supportfp.h"
#include "supportasbasic.h"

/*
 * ABORT() is a primitive substitute for ASSERT().
 */
#if STAGE==DEVELOP && 0
#define ABORT() abort()
#else
#define ABORT()
#endif

/* These switches define what code will come from this "C" module,
   and what code is in an assembler module */

/* if ASMARITH==1, arithmetic routines are in assembler by default */
#ifndef ASMARITH
#define ASMARITH        0
#endif  /* ASMARITH */

/* assembler source files as well should refer to INTARITH */
#ifndef INTARITH
#define INTARITH        0
#endif /* INTARITH */

/* Conversion routines are in assembler by default */
#ifndef ASMCONV
#define ASMCONV         0
#endif  /* ASMCONV */

#define fixedScale 65536.0      /* i=15, f=16, range [-32768, 32768) */
#define fracScale 1073741824.0  /* i=1, f=30 , range [-2, 2) */

/* Macros for software conversion between unsigned long and double
   (some compilers don't implement these conversions correctly) */

#ifndef SOFTUDCONV
#define SOFTUDCONV (ISP==isp_vax && OS==os_bsd)
#endif

#if SOFTUDCONV
#define UnsignedToDouble(u) \
  ((u > (Card32) MAXinteger)? \
    (double) (u - (Card32) MAXinteger - 1) + (double) MAXinteger + 1.0 : \
    (double) u)

#define DoubleToUnsigned(d) \
  ((d >= (double) MAXinteger + 1.0)? \
    (Card32) (d - (double) MAXinteger - 1.0) + (Card32) MAXinteger + 1 : \
    (Card32) d)

#else /* SOFTUDCONV */
#define UnsignedToDouble(u) ((double) u)
#define DoubleToUnsigned(d) ((Card32) d)
#endif /* SOFTUDCONV */

/*
 * Certain calls in bcsetup.c rely on the +/- infinity clamping
 * implemented here.
 */
#ifndef ASM_fixmul
#define ASM_fixmul ASMARITH
#endif
#if !ASM_fixmul
#if (OS==os_windowsNT) && ((ISP==isp_i80386) || (ISP==isp_i80486))
#define POS_INF 0x7fffffff
#define NEG_INF 0x80000000
PUBLIC Fixed FIXEDFUNC __declspec(naked) fixmul(Fixed a, Fixed b)
{
	__asm {
		// uses only eax, edx  no need to save any registers
		// res = a * b <-- res is edx:eax
		
		mov		eax,[esp+4]			;a
		imul	DWORD PTR [esp+8]	;b
		
		// check for negaitive -- rounding is done differently
		cmp		edx, 0
		js		neg_res

		// res += .5 <-- add in rounding bit

		add		eax, 0x8000		/* rounding bit, in fixed-point terms "0.5" */
		adc		edx, 0			/* it's a 64-bit add here... */

		// now check for overflow

		cmp		edx, 0x8000
		jae		pos_oflo

		// fix up result for return

		shrd	eax,edx,16
		ret

pos_oflo:		
		mov		eax,POS_INF
		ret

neg_res:
		// res -= .5 <-- add in negative rounding bit

		add		eax, 0x7fff		/* rounding bit, in fixed-point terms "-0.5" */
		adc		edx, 0			/* it's a 64-bit add here... */
		jns		done			/* underflow to 0 is OK */

		// now check for overflow

		cmp		edx, 0xffff8000
		jb		neg_oflo

		// fix up result for return
done:
		shrd	eax,edx,16
		ret

neg_oflo:
		mov		eax,NEG_INF
		ret
	}
}

#else /* was */
PUBLIC Fixed FIXEDFUNC fixmul (Fixed  x, Fixed  y)       /* returns x*y */
  {
  double d = (double) x * (double) y / fixedScale;
  if(d >= (double)0.0){
    d += (double)0.5;
    if(d < (double)FixedPosInf) 
		return (Fixed) d;
    else 
		return FixedPosInf;
  } else {
    d -= (double)0.5;
    if(d > (double)FixedNegInf)
		return (Fixed) d;
    else 
		return FixedNegInf;
  }
  }
#endif 
#endif /* !ASM_fixmul */

#ifndef ASM_fracmul
#define ASM_fracmul ASMARITH
#endif
#if !ASM_fracmul
PUBLIC Frac FIXEDFUNC fracmul (Frac  x, Frac  y)       /* returns x*y */
  {
  double d = (double) x * (double) y / fracScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Frac) d;
  }
#endif

#ifndef ASM_fixdiv
#define ASM_fixdiv ASMARITH
#endif
#if !ASM_fixdiv
#if (OS==os_windowsNT) && ((ISP==isp_i80386) || (ISP==isp_i80486))

 /* These two routines must change if the default calling sequence is 
 *	changed from "__cdecl"
 */
#ifndef POS_INF
#define POS_INF     0x7fffffff
#endif
#ifndef NEG_INF
#define NEG_INF     0x80000000
#endif
PUBLIC Fixed FIXEDFUNC __declspec(naked) fixdiv(Fixed a, Fixed b)
{
/*
	This routine is derived from the version in PostScript done for the 
	PharLap compiler on the Pentium (see 3016.xxx.packages/fp/sources/I80386).
	Basically, we do the divide here with only *positive* values, so that
	we can have 31 bits of value and a rounding bit.  If we did this signed,
	we would have to keep the top bit.  So we compute the sign of the result
	up front with an xor.  ebx holds the sign of the result as it's top bit.
	We then convert the operands to positive values, if need be, and multiply 
	the divisor by 128k.  We then do the divide, and shift the result down one,
	dropping the rounding "guard bit" into the carry flag.  The adc (add with
	carry) of zero will add in the bottom bit (we could have simply added one
	and then shifted).  We check for overflows and (possibly) negate the 
	result and exit.
  
*/
	__asm {
		/* uses eax, ebx, ecx, edx -- div itself uses eax, ecx and edx.        */
		/* we'll be safe and save ebx, but the others should be safe to        */
		/* trash.  ebx is assumed to be preserved across calls (also esi, edi) */
		mov     eax, [esp+4]
		mov     ecx, [esp+8]
		push	ebx
		mov		ebx, ecx
		// the xor will hold the sign bit as the top bit of ebx
		xor     ebx, eax

		// check for a "divide by zero", and return the properly signed infinity
		cmp		ecx, 0
		jz		fxd_oflow

		// check for a negative operand and "fix it"
		jns     fxd_next_op
		neg     ecx

		/* now let's do the same for the next operand */
fxd_next_op:
		cmp     eax, 0
		jns     fxd_do_div
		neg     eax

		/* here we're going to make the 32-bit eax value into a 64-bit */
		// one spread across edx:eax and multiplied by 128k
fxd_do_div:
		mov     edx, eax
		shr     edx, 15
		shl     eax, 17
		
		// check for a possible overflow first
		cmp     edx, ecx
		jae     fxd_oflow

		// now do the divide.  The result is 2x too big, but has the
		// rounding bit at the bottom
		div     ecx

		// divide by 2, and get the rounding bit into the carry flag
		shr     eax, 1

		// add in the rounding bit so that we round up.  Check for
		// overflow when we round up...
		adc     eax, 0
		jo      fxd_oflow

		// now correct the result if it was supposed to be negative
		cmp     ebx, 0
		jns     fxd_out
		neg     eax

		// if the the top bit is not set after this neg, then we overflowed
		// in the divide.  exit if the sign bit is set
		js		fxd_out

		// special case -- zero result does not show sign after negation
		// but is not an overflow, either. (zero divided by negative number)
		jz		fxd_out

		// deal with choosing the right version of infinity for overflows
		// top bit of ebx is sign bit for result.  Adding it to 7fffffff
		// will yield 80000000 if it was negative
fxd_oflow:
		mov     eax, POS_INF
		shl     ebx, 1
		adc		eax, 0

fxd_out:     
		pop		ebx
		ret
	}
}
#else
PUBLIC Fixed FIXEDFUNC fixdiv (Fixed  x, Fixed  y)       /* returns x/y */
  {
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
  }
#endif 
#endif /* !ASM_fixdiv */

#ifndef ASM_fixratio
#define ASM_fixratio ASMARITH
#endif
#if !ASM_fixratio
PUBLIC Frac FIXEDFUNC fixratio (Fixed  x, Fixed  y)      /* returns x/y */
  {
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fracScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Frac) d;
  }
#endif

#ifndef ASM_ufixratio
#define ASM_ufixratio ASMARITH
#endif
#if !ASM_ufixratio
PUBLIC Fixed FIXEDFUNC ufixratio (Card32  x, Card32  y)	/* returns x/y */
  {
  double d, d1, d2;
  if (y == 0) return FixedPosInf;
  d1 = UnsignedToDouble(x);
  d2 = UnsignedToDouble(y);
  d = (double) d1 / (double) d2 * fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
  }
#endif


/* converts x to float and stores at *pf */
#ifndef ASM_fixtopflt
#define ASM_fixtopflt ASMCONV
#endif
#if !ASM_fixtopflt
PUBLIC procedure fixtopflt (Fixed  x, float *  pf)
  {*pf = (float) x / (float)fixedScale;}
#endif

#ifndef ASM_pflttofix
#define ASM_pflttofix ASMCONV
#endif
#if !ASM_pflttofix
PUBLIC Fixed pflttofix(float *pf)      /* converts float *pf to fixed */
  {
  float f = *pf;
  if (f >= (float)(FixedPosInf / fixedScale)) return FixedPosInf;
  if (f <= (float)(FixedNegInf / fixedScale)) return FixedNegInf;
  return (Fixed) (f * fixedScale);
  }
#endif

#ifndef ASM_fracsqrt
#define ASM_fracsqrt ASMARITH
#endif
#if !ASM_fracsqrt
PUBLIC UFrac FIXEDFUNC fracsqrt (UFrac  x)        /* returns square root of x */
  {
  double d = UnsignedToDouble(x);
  d = sqrt(d / fracScale) * fracScale + 0.5;
  return DoubleToUnsigned(d);
  }
#endif

#ifndef ASM_fracdiv
#define ASM_fracdiv ASMARITH
#endif
#if !ASM_fracdiv
PUBLIC Frac FIXEDFUNC fracdiv (Frac  x, Frac  y) {
  double d;
  if (y == 0)
    return (x < 0)? FixedNegInf : FixedPosInf;
  d = ((double) x) / ((double) y);
  d += (d < 0) ? (-0.5 / fracScale) : (0.5 / fracScale);
  if (d < -2.0)
    return FixedNegInf;
  if (d >= 2.0)
    return FixedPosInf;
  return (Frac) (d * fracScale);
}
#endif

PUBLIC Fixed FFloorFModN(Fixed x, Card16 n){
   Fixed a;
   if(x > 0){
      a = x;
      /* fixdiv rounds */
      a -= (Int32)n/2;
      a = fixdiv(a,(Fixed)(n<<16));
      a &= 0xffff0000;
      a = fixmul(a,(Fixed)(n<<16));
      return a;
   } else {
      a = -x;
      a += (Fixed)(n<<16) -1;
      a -= (Int32)n/2;
      a = fixdiv(a,(Fixed)(n<<16));
      a &= 0xffff0000;
      a = fixmul(a,(Fixed)(n<<16));
      return -a;
   }
}

PUBLIC Fixed FCeilFModN(Fixed x, Card16 n){
   Fixed a;
   a = x + (Fixed)(n<<16) -1;
   a = FFloorFModN(a,n);
   return a;
}

PUBLIC Fixed FRoundFModN(Fixed x, Card16 n){
  Fixed a;
  a = x + (Fixed)(n<<15);
  a = FFloorFModN(a,n);
  return a;
}

PUBLIC Int32 IFloorModN(Int32 x, Card16 n){
  Int32 a;
  if(x >= 0){
     a = x/n;
     a = a*n;
     return a;
  } else {
     a = -x;
     a = (a + n - 1)/n;
     a = a*n;
     return -a;
  }
}

PUBLIC Int32 ICeilModN(Int32 x, Card16 n){
  Int32 a;
  if(x >= 0){
     a = x + n - 1;
     a = a/n;
     a = a*n;
     return a;
  } else {
     a = (x/n)*n;
     return a;
  }
}

PUBLIC Fixed FFloorFModNR(Fixed v, Card16 n, Int16 r){
    Fixed w;
    w = FFloorFModN(v,n);
    w += (Fixed)(r<<16);
    if(w <= v)
        return w;
    w -= (Fixed)(n<<16);
    return w;
}

PUBLIC Fixed FCeilFModNR(Fixed v, Card16 n, Int16 r){
    Fixed w;
    w = FCeilFModN(v,n);
    w -= (Fixed)(r<<16);
    if(w >= v)
        return w;
    w += (Fixed)(n<<16);
    return w;
}

PUBLIC Fixed FRoundFModNR(Fixed v, Card16 n, Int16 r){
    Fixed w, w1, w2;
    w = FFloorFModNR(v + (((Fixed)n)<<15),n,r);
    if(w > v){
        w1 = w - v;
        w2 = (((Fixed)n)<<16) - w1;
        if (w2 < w1)
            w = w - (((Fixed)n)<<16);
    } else {
        w1 = v - w;
        w2 = (((Fixed)n)<<16) - w1;
        if (w2 < w1)
            w = w + (((Fixed)n)<<16);
    }
    return w;
}



/* taken from the postscript world... */
#if USE_SWFP
PUBLIC double ASFixedToDouble (Fixed  x)
{
        double d;
        unsigned int *ip;

        ip = (unsigned int*)&d;

        if (x) { /* otherwise return 0 */
            /* exponent bias = 1023;
               fixed point is 16 - (sign bit) - (hidden bit) = 14 */
            unsigned int exp = (1023+14);
            unsigned int sign = x & (1<<31);

            if (sign) x = -x;   /* absolute value */

            /* basically, this is an unrolled loop --
               while (x < 0x80000000) {
                   x = x << 1;
                   exp -= 1;
               }
               with some compaction done.  This is easier in
               assembly, as you can use shift/jump carry pairs
               until the hidden bit falls off the end, or use
               a bit scan or (29k) count-leading-zeros. */

            if (!(x >> 15)) { exp -= 16; x = x << 16; }
            if (!(x >> 23)) { exp -= 8; x = x << 8; }
            if (!(x >> 27)) { exp -= 4; x = x << 4; }
            if (!(x >> 29)) { exp -= 2; x = x << 2; }
            if (!(x >> 30)) { exp -= 1; x = x << 1; }

            /* 15+7+3+3+1+1=30.  We never shift more than 30. */

            x = x << 2; /* drop the hidden bit (normalize this) */

#if AS_LITTLEENDIAN
            *(ip+1) = ((unsigned int)x>>12) | exp << 20 | sign;
            *ip = x << 20;
#else
            *ip = ((unsigned int)x>>12) | exp << 20 | sign;
            *(ip+1) = x << 20;
#endif
            return(d);
        }
        *ip = *(ip+1) = 0;
        return(d);
}


PUBLIC Fixed ASDoubleToFixed (double  d)
{ 
        double temp;
        unsigned int *ptr, i, lsw;
        int sign, exp;

        ptr = (unsigned int *)&temp;
        temp = d;
#if AS_LITTLEENDIAN
        lsw = *ptr; i = *(ptr + 1);
#else
        i = *ptr; lsw = *(ptr+1);
#endif

        sign = (i >> 31);               /* get sign bit */
        exp = (i >> 20);                /* biased exponent */

        /* make exponent into shift count */
        /* 1023 (exponent bias) - 16 (offset of fixed point)
           + 20 (offset of fp point) + 11 (shift from above)
           - 1 (the other shift from above -- makes room for carry
           from rounding) = 1037 */

        exp = 1037 - (exp & 0x7ff);

        /* check for overflow and return +- infinity */
        if (exp >= 0) {

        /* check for underflow and return 0 */
            if (exp <= 31) {

        /* denormalize, get near top so all later shifts are down */
                i = ((i << 12) >> 2) | (1 << 30) | (lsw >> 22);

        /* add in the rounding factor */
                if (exp) {
                    i = i + (( 1 << (exp - 1)));

        /* and shift result into proper place */
                    i = i >> exp;
                    return sign ? -(int)i :i;
                }

        /* no other shift -- round and check for overflow */
                i += (lsw >> 21) & 1;
                if (!( i & (1 << 31)))
                    return (sign?(- (int) i):i);

            } return (0);
        }
        return (ASFixedPosInf + sign);
}


PUBLIC Frac dbltofrac(double d)
  {
        double temp;
        unsigned int *ptr, i, lsw;
        int sign, exp;

        ptr = (unsigned int *)&temp;
        temp = d;
#if AS_LITTLEENDIAN
        lsw = *ptr; i = *(ptr + 1);
#else
        i = *ptr; lsw = *(ptr+1);
#endif

        sign = (i >> 31);               /* get sign bit */
        exp = ((i << 1) >> 21);         /* biased exponent */

        /* make exponent into shift count */
        /* 1023 (exponent bias) - 30 (offset of fract point)
           + 20 (offset of fp point) + 11 (shift from above)
           - 1 (the other shift from above -- makes room for carry
           from rounding) = 1023 */

        exp = 1023 - exp;

        /* check for overflow and return +- infinity */
        if (exp >= 0) {

        /* check for underflow and return 0 */
            if (exp <= 31) {

        /* denormalize, get near top so all later shifts are down */
                i = ((i << 12) >> 2) | (1 << 30) | (lsw >> 22);

        /* add in the rounding factor */
                if (exp) {
                    i = i + (( 1 << (exp - 1)));

        /* and shift result into proper place */
                    i = i >> exp;
                    return (sign?(- (int) i):i);
                }

        /* no other shift -- round and check for overflow */
                i += (lsw >> 21) & 1;
                if (!( i & (1 << 31)))
                    return (sign?(- (int) i):i);

            } else  return (0);
        }
        return (~(0 - sign) + ( 1 << 31 ));
  }

#else /* USE_SWFP */

PUBLIC double ASFixedToDouble (Fixed  x)
{
    return (double) x / fixedScale;
}

PUBLIC Fixed ASDoubleToFixed (double  d)
{
  d *= fixedScale;
  if (d < 0)
    {if ((d -= 0.5) < ASMINInt32) return ASMINInt32;}
  else
    {if ((d += 0.5) > ASMAXInt32) return ASMAXInt32;}
  return (ASFixed) d;
}


PUBLIC Frac dbltofrac(double d)
  {
  d *= fracScale;
  if (d < 0)
    {if ((d -= 0.5) < ASMINInt32) return ASMINInt32;}
  else
    {if ((d += 0.5) > ASMAXInt32) return ASMAXInt32;}
  return (Frac) d;
  }

#endif /* USE_SWFP */
