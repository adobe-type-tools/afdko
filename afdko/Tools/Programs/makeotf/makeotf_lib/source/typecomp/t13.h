/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Type 13 charstring re-encoding support.
 */

#ifndef T13_H
#define T13_H

#include "common.h"
#include "cs.h"

#include "pstoken.h"

void t13New(tcCtx g);
void t13Free(tcCtx g);

int t13CheckConv(tcCtx g, psCtx ps, csDecrypt *decrypt);
int t13CheckAuth(tcCtx g, Font *font);

#endif /* T13_H */