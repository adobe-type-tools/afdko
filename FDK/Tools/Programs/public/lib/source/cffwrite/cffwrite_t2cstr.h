/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Charsting support.
 */

#ifndef CSTR_H
#define CSTR_H

#include "cffwrite_share.h"

void cfwCstrNew(cfwCtx g);
void cfwCstrReuse(cfwCtx g);
void cfwCstrFree(cfwCtx g);

void cfwCstrBegFont(cfwCtx g, int nFDs);
void printFinalWarn(cfwCtx g);

#endif /* CSTR_H */
