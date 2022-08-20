/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * name table support.
 */

#ifndef SFNTDIFF_DNAME_H_
#define SFNTDIFF_DNAME_H_

#include "Dglobal.h"

extern void sdNameReset(void);

extern void sdNameRead(uint8_t which, int32_t offset, uint32_t length);
extern void sdNameDiff(int32_t offset1, int32_t offset2);
extern void sdNameFree(uint8_t which);

#endif  // SFNTDIFF_DNAME_H_
