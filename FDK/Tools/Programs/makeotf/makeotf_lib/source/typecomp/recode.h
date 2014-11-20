/* @(#)CM_VerSion recode.h atm08 1.2 16245.eco sum= 45198 atm08.002 */
/* @(#)CM_VerSion recode.h atm07 1.2 16164.eco sum= 45550 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Type 1 charstring re-encoding support.
 */

#ifndef RECODE_H
#define RECODE_H

#include "common.h"
#include "cs.h"

void recodeNew(tcCtx g);
void recodeFree(tcCtx g);

void recodeDecrypt(unsigned length, unsigned char *cstr);
csConvProcs recodeGetConvProcs(tcCtx g);
void recodeSaveConvSubrs(tcCtx g, Font *font, int nAxes, double *BDM,
                         int *order, int lenIV, ConvSubr *NDV, ConvSubr *CDV);

#if TC_EURO_SUPPORT
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
#define kEuroMMIndex 0
#define kFillinMMIndex 1
#define kMaxAlternateNames 5 /* max number of alternate names considered as a match for a
	                            synthetic glyph. */
void InitStaticFontData(tcCtx g, int font__serif_selector, double *StdVW, double *FontMatrix, unsigned isFixedPitch);
void recodeAddNewGlyph(tcCtx g, unsigned id, unsigned fill_in_font_id, unsigned gl_id, unsigned noslant,  unsigned adjust_overshoot, char *char_name);

unsigned getNextStdGlyph(unsigned gl_id,  char ***charListptr);

#endif /* TC_EURO_SUPPORT */

#endif /* RECODE_H */