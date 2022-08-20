/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * FNAM table support.
 */

#ifndef FNAM_H
#define FNAM_H

#include "spot_global.h"

extern void FNAMRead(int32_t offset, uint32_t length);
extern void FNAMDump(int level, int32_t offset);
extern void FNAMFree(void);

extern int FNAMGetNEncodings(uint16_t *nEncoings, uint32_t client);

#endif /* FNAM_H */
