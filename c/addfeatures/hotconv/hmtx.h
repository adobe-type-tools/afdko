/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_HMTX_H_
#define ADDFEATURES_HOTCONV_HMTX_H_

#include <vector>

#include "common.h"

#define hmtx_ TAG('h', 'm', 't', 'x')
#define hhea_ TAG('h', 'h', 'e', 'a')
#define HVAR_ TAG('H', 'V', 'A', 'R')

/* Standard functions */
void hmtxNew(hotCtx g);
int hmtxFill(hotCtx g);
void hmtxWrite(hotCtx g);
void hmtxReuse(hotCtx g);
void hmtxFree(hotCtx g);

void hheaNew(hotCtx g);
int hheaFill(hotCtx g);
void hheaWrite(hotCtx g);
void hheaReuse(hotCtx g);
void hheaFree(hotCtx g);

void HVARNew(hotCtx g);
int HVARFill(hotCtx g);
void HVARWrite(hotCtx g);
void HVARReuse(hotCtx g);
void HVARFree(hotCtx g);

#endif  // ADDFEATURES_HOTCONV_HMTX_H_
