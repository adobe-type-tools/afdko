/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef BASE_H
#define BASE_H

#include "common.h"

#define BASE_   TAG('B', 'A', 'S', 'E')

/* Standard functions */
void BASENew(hotCtx g);
int BASEFill(hotCtx g);
void BASEWrite(hotCtx g);
void BASEReuse(hotCtx g);
void BASEFree(hotCtx g);

/* Supplementary functions */
void BASESetBaselineTags(hotCtx g, int vert, int nTag,
                         Tag *baselineTag); /* [nTag]. Must be sorted */

void BASEAddScript(hotCtx g, int vert, Tag script, Tag dfltBaseline,
                   short *coord);   /* [nTag] */

#endif /* BASE_H */
