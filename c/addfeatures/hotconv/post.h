/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_POST_H_
#define ADDFEATURES_HOTCONV_POST_H_

#include "common.h"

#define post_ TAG('p', 'o', 's', 't')

/* Standard functions */
void postNew(hotCtx g);
int postFill(hotCtx g);
void postWrite(hotCtx g);
void postReuse(hotCtx g);
void postFree(hotCtx g);

void postRead(hotCtx g, int offset, int length);

#endif  // ADDFEATURES_HOTCONV_POST_H_
