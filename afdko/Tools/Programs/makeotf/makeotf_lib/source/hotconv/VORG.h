/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef VORG_H
#define VORG_H

#include "common.h"

#define VORG_   TAG('V', 'O', 'R', 'G')

/* Standard functions */
void VORGNew(hotCtx g);
int VORGFill(hotCtx g);
void VORGWrite(hotCtx g);
void VORGReuse(hotCtx g);
void VORGFree(hotCtx g);

/* Supplementary functions */

#endif /* VORG_H */
