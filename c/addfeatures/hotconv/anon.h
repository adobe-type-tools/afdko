/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_ANON_H_
#define ADDFEATURES_HOTCONV_ANON_H_

#include "common.h"

/* Standard functions */
void anonNew(hotCtx g);
int anonFill(hotCtx g);
void anonWrite(hotCtx g);
void anonReuse(hotCtx g);
void anonFree(hotCtx g);

/* Supplementary functions */
void anonAddTable(hotCtx g, unsigned long tag, hotAnonRefill refill);

#endif  // ADDFEATURES_HOTCONV_ANON_H_
