/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef HMTX_H
#define HMTX_H

#include "common.h"

#define hmtx_   TAG('h', 'm', 't', 'x')

/* Standard functions */
void hmtxNew(hotCtx g);
int hmtxFill(hotCtx g);
void hmtxWrite(hotCtx g);
void hmtxReuse(hotCtx g);
void hmtxFree(hotCtx g);

/* Supplementary Functions */
int hmtxGetNLongHorMetrics(hotCtx g);

#endif /* HMTX_H */
