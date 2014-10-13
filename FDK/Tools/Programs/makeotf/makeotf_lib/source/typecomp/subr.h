/* @(#)CM_VerSion subr.h atm08 1.2 16245.eco sum= 63581 atm08.002 */
/* @(#)CM_VerSion subr.h atm07 1.2 16164.eco sum= 62866 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Subroutinization support.
 *
 * xxx get this comment up to date.
 * After initialization each font's charstring data is added with
 * subrFont(). When all charstring data in the font set has been added
 * the fonts are rescanned and overlap conflicts are resolved by a call to
 * subrRescanFont(). Partial subroutine selection is then performed by a call
 * to subrSelect(). Finally, each font is subroutinized by a call to
 * subrAddLocal().
 */

#ifndef SUBR_H
#define SUBR_H

#include "common.h"
#include "font.h"

#if TC_SUBR_SUPPORT
void subrNew(tcCtx g);
void subrFree(tcCtx g);
void subrReuse(tcCtx g);

void subrSubrize(tcCtx g, int nFonts, Font *fonts);

long subrSizeLocal(CSData *subrs);
void subrWriteLocal(tcCtx g, CSData *subrs);

#endif /* TC_SUBR_SUPPORT */

long subrSizeGlobal(tcCtx g);
void subrWriteGlobal(tcCtx g);

#endif /* SUBR_H */