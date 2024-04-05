/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_MVAR_H_
#define ADDFEATURES_HOTCONV_MVAR_H_

#include "common.h"

/* Standard functions */
void MVARNew(hotCtx g);
int MVARFill(hotCtx g);
void MVARWrite(hotCtx g);
void MVARReuse(hotCtx g);
void MVARFree(hotCtx g);

void MVARAddValue(hotCtx g, ctlTag tag, const VarValueRecord &vvr);

#endif  // ADDFEATURES_HOTCONV_MVAR_H_
