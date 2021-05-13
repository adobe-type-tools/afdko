/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * String index support.
 */

#ifndef CFFWRITE_SINDEX_H
#define CFFWRITE_SINDEX_H

#include "cffwrite_share.h"

void cfwSindexNew(cfwCtx g);
void cfwSindexReuse(cfwCtx g);
void cfwSindexFree(cfwCtx g);

int cfwSindexInvalidString(char *string);
SRI cfwSindexAddString(cfwCtx g, char *string);
char *cfwSindexGetString(cfwCtx g, SRI index);
SID cfwSindexAssignSID(cfwCtx g, SRI index);

long cfwSindexSize(cfwCtx g);
void cfwSindexWrite(cfwCtx g);

#endif /* CFFWRITE_SINDEX_H */
