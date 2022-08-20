/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef MAKEOTF_LIB_HOTCONV_HHEA_H_
#define MAKEOTF_LIB_HOTCONV_HHEA_H_

#include "common.h"

#define hhea_ TAG('h', 'h', 'e', 'a')

/* Standard functions */
void hheaNew(hotCtx g);
int hheaFill(hotCtx g);
void hheaWrite(hotCtx g);
void hheaReuse(hotCtx g);
void hheaFree(hotCtx g);

/* Supplementary functions */
void hheaSetCaretOffset(hotCtx g, short caretOffset);

#endif  // MAKEOTF_LIB_HOTCONV_HHEA_H_
