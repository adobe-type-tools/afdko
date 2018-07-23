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

#define fixedScale 65536.0      /* i=15, f=16, range [-32768, 32768) */
#define fracScale 1073741824.0  /* i=1, f=30 , range [-2, 2) */

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

PUBLIC procedure fixtopflt (Fixed  x, float *  pf)
  {*pf = (float) x / (float)fixedScale;}
PUBLIC Fixed pflttofix(float *pf)      /* converts float *pf to fixed */
  {
  float f = *pf;
  if (f >= (float)(FixedPosInf / fixedScale)) return FixedPosInf;
  if (f <= (float)(FixedNegInf / fixedScale)) return FixedNegInf;
  return (Fixed) (f * fixedScale);
  }
