/* @(#)CM_VerSion bcutil.h atm09 1.2 16563.eco sum= 63210 atm09.004 */
/* @(#)CM_VerSion bcutil.h atm08 1.2 16248.eco sum= 61220 atm08.003 */
/* @(#)CM_VerSion bcutil.h atm05 1.2 11580.eco sum= 18110 */
/* @(#)CM_VerSion bcutil.h atm04 1.5 08720.eco sum= 51998 */
/* $Header$ */
/*
  bcutil.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	BCUTIL_H
#define	BCUTIL_H

#include "buildch.h"

/* The following routines provide access to information that is contained
   in structures private to the buildch pacakge. These routines should only be
   used by Adobe Internal clients of the renderer. As currently 
   implemented the routines depend on the FontInst structure used by the 
   software and Type 1 Coprocessor renderer (defined
   in the bcpriv.h file in the buildch package).
*/

extern procedure BCGetForceBold 
(
  PFontInst  pFInst,
  boolean *  pForceBold
);

/* BCGetForceBold assigns true to *pForceBold if bolding should be applied
  to the resulting bitmap to make it appear bold at small pt sizes. False is    
  assigned if the renderer has done the bolding or if bolding does not need 
  to be applied to the bitmap.

  NOTE: currently the renderer does not perform this bolding operation.
  However in a future release this feature will be supported and this routine
  will always assign a false value to pForceBold.
*/

extern procedure BCGetFontBBox 
(
  PFontInst  pFInst, 
  FCdBBox *  pFontBBox
);
/*
  BCGetFontBBox returns the fontBBox stored in the FontInst structure. This 
  bounding box is in device space coordinates and has been blended according 
  to the weightVector if the font is a Multiple Master font. Also if the font
  is a LanguageGroup 1 font, the expansion factor will be applied to the 
  bounding box. 

*/
extern Fixed BCGetFlatness 
(
  PFontInst  pFInst 
);
/*
  BCGetFlatness returns the flatness stored in the FontInst structure.  This 
  routine may only be used with the Type 1 software renderer.   
*/

extern Fixed BCSetFlatness 
(
  PFontInst  pFInst,
  Fixed  flatness 
);
/*
  BCSetFlatness sets the flatness stored in the FontInst structure. This 
  routine may only be used with the Type 1 software renderer.
  
  Flatness is specified as a Fixed number that is aproximately four times
  the allowed error in pixels between the actual curve and the approximation 
  of the curve by line segments. A large flatness value will increase the
  error but decrease the number of line segments needed to represent the
  curve. For example a value of 0x00020000 ( decimal 2) will cause the
  renderer to generate an aproximation with an error of 0.5 pixels.
  
  The routine will return the flatness value actually set by the renderer.
  Since internal limits are imposed on the flatness value, this value may
  differ from the value specified by the caller of BCSetFlatness.
  The client should inspect the return value to see when internal limits have
  been reached.
   
*/

extern Fixed BCSetQRedFlatnessOverride
(
  PFontInst  pFInst,
  Fixed  len1000
);
/*
  BCSetQRedFlatnessOverride resets the flatness stored in the FontInst 
  structure, for large values of lines per em X or Y. This routine may 
  only be used with the Type 1 software renderer.

*/

#endif /* BCUTIL_H */
