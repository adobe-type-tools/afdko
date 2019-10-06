/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_VHEA_H
#define HOTCONV_VHEA_H

#include "common.h"

#define vhea_ TAG('v', 'h', 'e', 'a')

/* Standard functions */
void vheaNew(hotCtx g);
int vheaFill(hotCtx g);
void vheaWrite(hotCtx g);
void vheaReuse(hotCtx g);
void vheaFree(hotCtx g);

#endif /* HOTCONV_VHEA_H */
