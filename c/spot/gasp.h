/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * gasp table support.
 */

#ifndef GASP_H
#define GASP_H

#include "spot_global.h"

extern void gaspRead(int32_t offset, uint32_t length);
extern void gaspDump(int level, int32_t offset);
extern void gaspFree(void);

#endif /* GASP_H */
