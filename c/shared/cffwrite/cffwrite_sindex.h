/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * String index support.
 */

#ifndef SHARED_CFFWRITE_CFFWRITE_SINDEX_H_
#define SHARED_CFFWRITE_CFFWRITE_SINDEX_H_

#include "cffwrite_share.h"

void cfwSindexNew(cfwCtx g);
void cfwSindexReuse(cfwCtx g);
void cfwSindexFree(cfwCtx g);

int cfwSindexInvalidString(const char *str);
SRI cfwSindexAddString(cfwCtx g, const char *str);
const char *cfwSindexGetString(cfwCtx g, SRI index);
SID cfwSindexAssignSID(cfwCtx g, SRI index);

long cfwSindexSize(cfwCtx g);
void cfwSindexWrite(cfwCtx g);

#endif  // SHARED_CFFWRITE_CFFWRITE_SINDEX_H_
