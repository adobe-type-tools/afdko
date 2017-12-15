/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef VMTX_H
#define VMTX_H

#include "common.h"

#define vmtx_   TAG('v', 'm', 't', 'x')

/* Standard functions */
void vmtxNew(hotCtx g);
int vmtxFill(hotCtx g);
void vmtxWrite(hotCtx g);
void vmtxReuse(hotCtx g);
void vmtxFree(hotCtx g);

/* Supplementary Functions */
int vmtxGetNLongVerMetrics(hotCtx g);

#endif /* VMTX_H */
