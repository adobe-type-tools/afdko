/* Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_CFFWRITE_CFFWRITE_VARSTORE_H_
#define SHARED_CFFWRITE_CFFWRITE_VARSTORE_H_

#include "cffwrite.h"
#include "cffwrite_dict.h"
#include "varsupport.h"

void cfwDictFillVarStore(cfwCtx g, DICT *dst, abfTopDict *top);

#endif  // SHARED_CFFWRITE_CFFWRITE_VARSTORE_H_
