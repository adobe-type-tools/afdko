/*
  framepixeltypes.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef FRAMEPIXELTYPES_H
#define FRAMEPIXELTYPES_H

#include "supportpublictypes.h"
#include "supportenvironment.h"

/*
  Scan unit definitions.
  Raster memory (frame buffer, masks, etc.) is always accessed in units
  of this size. Any compile-time dependence on the following definitions
  requires building separate package configurations for each distinct
  scan unit size.
 */
#ifndef SCANUNIT
#define SCANUNIT 32
#endif

#if (SCANUNIT!=8) && (SCANUNIT!=16) && (SCANUNIT!=32) && (SCANUNIT!=64)
#define SCANUNIT 32
#endif   /*  (SCANUNIT!=8) && (SCANUNIT!=16) && (SCANUNIT!=32) && (SCANUNIT!=64) */

#if SCANUNIT==8
#define SCANSHIFT 3
#define SCANMASK 07
#define SCANTYPE Card8
#define LASTSCANVAL 0xFF
#endif   /*  SCANUNIT==8  */

#if SCANUNIT==16
#define SCANSHIFT 4
#define SCANMASK 017
#define SCANTYPE Card16
#define LASTSCANVAL 0xFFFF
#endif   /*  SCANUNIT==16  */

#if SCANUNIT==32
#define SCANSHIFT 5
#define SCANMASK 037
#define LASTSCANVAL 0xFFFFFFFF
#define SCANTYPE Card32
#endif   /*  SCANUNIT==32  */

#if SCANUNIT==64
#define SCANSHIFT 6
#define SCANMASK 077
#define LASTSCANVAL 0xFFFFFFFFFFFFFFFF
#define SCANTYPE Card64
#endif   /*  SCANUNIT==64  */

typedef SCANTYPE *PSCANTYPE;

/* SWAPUNIT Definitions.  Swapunit is the unit that bits should
   be swapped when written to a mask
 */
#ifndef SWAPUNIT  
/* Default to SCANUNIT values */

#define SWAPUNIT SCANUNIT
#define SWAPSHIFT SCANSHIFT
#define SWAPMASK SCANMASK
#define SWAPTYPE SCANTYPE
#define LASTSWAPVAL LASTSCANVAL
#define PSWAPTYPE PSCANTYPE

#else /* SWAPUNIT */
#if (SWAPUNIT > SCANUNIT)
/* SWAPUNIT must be less than or equal to SCANUNIT */

#define SWAPUNIT SCANUNIT
#define SWAPSHIFT SCANSHIFT
#define SWAPMASK SCANMASK
#define SWAPTYPE SCANTYPE
#define LASTSWAPVAL LASTSCANVAL
#define PSWAPTYPE PSCANTYPE
#endif /* (SWAPUNIT > SCANUNIT) */

#if (SWAPUNIT!=8) && (SWAPUNIT!=16) && (SWAPUNIT!=32)
#define SWAPUNIT 32
#endif /* (SWAPUNIT!=8) && (SWAPUNIT!=16) && (SWAPUNIT!=32) */

#if SWAPUNIT==8
#define SWAPSHIFT 3
#define SWAPMASK 07
#define SWAPTYPE Card8
#define LASTSWAPVAL 0xFF
#endif /* SWAPUNIT==8 */

#if SWAPUNIT==16
#define SWAPSHIFT 4
#define SWAPMASK 017
#define SWAPTYPE Card16
#define LASTSWAPVAL 0xFFFF
#endif /* SWAPUNIT==16 */

#if SWAPUNIT==32
#define SWAPSHIFT 5
#define SWAPMASK 037
#define LASTSWAPVAL 0xFFFFFFFF
#define SWAPTYPE Card32
#endif /* SWAPUNIT==32 */

typedef SWAPTYPE *PSWAPTYPE;

#endif /* SWAPUNIT */

/*
  DEVICE_CONSISTENT indicates whether the bit order of raster memory is
  consistent with the native byte order of the CPU. For configurations in
  which the orders are inconsistent, DEVICE_CONSISTENT must be defined in
  CFLAGS as 0 because the default value is 1.

  A DEVICE_CONSISTENT value of 0 (inconsistent order) imposes special
  requirements on how the contents of raster memory are to be treated
  when sending the raster data as video to a printer or display.
  Either the SCANUNIT size must be 8 or the data must be sent as SCANUNIT
  size chunks whose byte order is opposite the CPU native byte order.

  Any compile-time dependence on the following definitions requires building
  separate package configurations for consistent and inconsistent orders.
 */


#ifndef DEVICE_CONSISTENT
#if ISP==isp_i80486 || ISP==isp_ppc || ISP==isp_x64 || ISP==isp_ia64
#define	DEVICE_CONSISTENT 0
#else
#define DEVICE_CONSISTENT 1
#endif
#endif    /*  DEVICE_CONSISTENT  */

#if (SWAPBITS == DEVICE_CONSISTENT)
#define LSHIFT >>
#define RSHIFT <<
#define LSHIFTEQ >>=
#define RSHIFTEQ <<=
#else   /*  (SWAPBITS == DEVICE_CONSISTENT)  */
#define LSHIFT <<
#define RSHIFT >>
#define LSHIFTEQ <<=
#define RSHIFTEQ >>=
#endif  /*  (SWAPBITS == DEVICE_CONSISTENT)  */


#endif  /* FRAMEPIXELTYPES_H */
