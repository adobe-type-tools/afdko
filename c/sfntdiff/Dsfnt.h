/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * sfnt reading/dumping.
 */

#ifndef SFNTDIFF_DSFNT_H_
#define SFNTDIFF_DSFNT_H_

#include "Dglobal.h"
#include "opt.h"

extern void sdSfntRead(int32_t start1, int id1, int32_t start2, int id2);
extern void sdSfntDump(void);
extern void sdSfntFree(void);
extern int sdSfntReadATable(uint8_t which, uint32_t tag);

extern opt_Scanner sdSfntTagScan;

void hexDump(uint8_t which, uint32_t tag, int32_t start, uint32_t length);
void hexDiff(uint32_t tag, int32_t start1, uint32_t length1, int32_t start2, uint32_t length2);

#endif  // SFNTDIFF_DSFNT_H_
