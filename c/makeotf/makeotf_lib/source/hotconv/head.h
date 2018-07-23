/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef HEAD_H
#define HEAD_H

#include "common.h"

#define head_   TAG('h', 'e', 'a', 'd')

/* Standard functions */
void headNew(hotCtx g);
int headFill(hotCtx g);
void headWrite(hotCtx g);
void headReuse(hotCtx g);
void headFree(hotCtx g);

#define HEAD_ADJUST_OFFSET  (2 * int32)   /* Checksum adjustment offset */

#endif /* HEAD_H */
