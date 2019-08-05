/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

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

/* In order to avoid matching repeats across charstring boundaries it is
   necessary to add a unique separator between charstrings. This is achieved by
   inserting the t2_separator operator after each endchar. The 1-byte operator
   value is followed by a 24-bit number that is guaranteed to be different for
   each charstring thereby removing any possibility of a match spanning an
   endchar operator. These operators are inserted by the recode module and
   removed by the subroutinizer */
#define t2_separator t2_reserved9

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
