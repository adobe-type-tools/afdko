/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * GDEF table support.
 */

#ifndef GDEF_H
#define GDEF_H

#include "spot_global.h"

extern void GDEFRead(int32_t offset, uint32_t length);
extern void GDEFDump(int level, int32_t offset);
extern void GDEFFree_spot(void);
extern void GDEFUsage(void);
#endif /* GDEF_H */
