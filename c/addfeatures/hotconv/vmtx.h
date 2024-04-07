/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_VMTX_H_
#define ADDFEATURES_HOTCONV_VMTX_H_

#include <vector>

#include "common.h"

#define vhea_ TAG('v', 'h', 'e', 'a')
#define vmtx_ TAG('v', 'm', 't', 'x')
#define VORG_ TAG('V', 'O', 'R', 'G')
#define VVAR_ TAG('V', 'V', 'A', 'R')

/* Standard functions */
void vmtxNew(hotCtx g);
int vmtxFill(hotCtx g);
void vmtxWrite(hotCtx g);
void vmtxReuse(hotCtx g);
void vmtxFree(hotCtx g);

void vheaNew(hotCtx g);
int vheaFill(hotCtx g);
void vheaWrite(hotCtx g);
void vheaReuse(hotCtx g);
void vheaFree(hotCtx g);

void VORGNew(hotCtx g);
int VORGFill(hotCtx g);
void VORGWrite(hotCtx g);
void VORGReuse(hotCtx g);
void VORGFree(hotCtx g);

void VVARNew(hotCtx g);
int VVARFill(hotCtx g);
void VVARWrite(hotCtx g);
void VVARReuse(hotCtx g);
void VVARFree(hotCtx g);

#endif  // ADDFEATURES_HOTCONV_VMTX_H_
