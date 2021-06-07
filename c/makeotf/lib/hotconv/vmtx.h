/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_VMTX_H
#define HOTCONV_VMTX_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define vmtx_ TAG('v', 'm', 't', 'x')

/* Standard functions */
void vmtxNew(hotCtx g);
int vmtxFill(hotCtx g);
void vmtxWrite(hotCtx g);
void vmtxReuse(hotCtx g);
void vmtxFree(hotCtx g);

/* Supplementary Functions */
int vmtxGetNLongVerMetrics(hotCtx g);

#ifdef __cplusplus
}
#endif

#endif /* HOTCONV_VMTX_H */
