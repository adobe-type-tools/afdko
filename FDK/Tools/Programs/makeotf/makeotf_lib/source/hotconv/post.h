/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef POST_H
#define POST_H

#include "common.h"

#define post_   TAG('p', 'o', 's', 't')

/* Standard functions */
void postNew(hotCtx g);
int postFill(hotCtx g);
void postWrite(hotCtx g);
void postReuse(hotCtx g);
void postFree(hotCtx g);

#endif /* POST_H */
