/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * ENCO table support.
 */

#ifndef ENCO_H
#define ENCO_H

#include "spot_global.h"

extern void ENCORead(int32_t offset, uint32_t length);
extern void ENCODump(int level, int32_t offset);
extern void ENCOFree(void);

#endif /* ENCO_H */
