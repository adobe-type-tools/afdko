/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)map.h	1.1
 * Changed:    11/20/98 10:31:33
 ***********************************************************************/

#include "global.h"

extern const Byte8 * map_nicename(Byte8 *oldnam, IntX *newlength);
void freemap(void);