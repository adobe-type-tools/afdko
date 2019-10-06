/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_FVAR_H
#define HOTCONV_FVAR_H

#include "common.h"

#define fvar_ TAG('f', 'v', 'a', 'r')

/* Standard functions */
void fvarNew(hotCtx g);
int fvarFill(hotCtx g);
void fvarWrite(hotCtx g);
void fvarReuse(hotCtx g);
void fvarFree(hotCtx g);

#endif /* HOTCONV_FVAR_H */
