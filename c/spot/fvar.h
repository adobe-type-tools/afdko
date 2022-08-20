/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * fvar table support.
 */

#ifndef FVAR_H
#define FVAR_H

#include "spot_global.h"

extern void fvarRead(int32_t offset, uint32_t length);
extern void fvarDump(int level, int32_t offset);
extern void fvarFree(void);

#endif /* FVAR_H */
