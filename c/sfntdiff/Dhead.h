/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * head table support.
 */

#include "Dglobal.h"

#ifndef SFNTDIFF_DHEAD_H_
#define SFNTDIFF_DHEAD_H_

extern void sdHeadRead(uint8_t which, int32_t offset, uint32_t length);
extern void sdHeadDiff(int32_t offset1, int32_t offset2);
extern void sdHeadFree(uint8_t which);

extern char *sdHeadGetCreatedDate(uint8_t which, uint32_t client);
extern char *sdHeadGetModifiedDate(uint8_t which, uint32_t client);

#endif  // SFNTDIFF_DHEAD_H_
