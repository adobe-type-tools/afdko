/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef MAKEOTF_LIB_HOTCONV_MAXP_H_
#define MAKEOTF_LIB_HOTCONV_MAXP_H_

#include "common.h"

#define maxp_ TAG('m', 'a', 'x', 'p')

/* Standard functions */
void maxpNew(hotCtx g);
int maxpFill(hotCtx g);
void maxpWrite(hotCtx g);
void maxpReuse(hotCtx g);
void maxpFree(hotCtx g);

/* Supplementary functions */

#endif  // MAKEOTF_LIB_HOTCONV_MAXP_H_
