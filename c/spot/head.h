/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * head table support.
 */

#ifndef HEAD_H
#define HEAD_H

#include "spot_global.h"

extern void headRead(int32_t offset, uint32_t length);
extern void headDump(int level, int32_t offset);
extern void headFree_spot(void);

extern int headGetLocFormat(uint16_t *locFormat, uint32_t client);
extern int headGetUnitsPerEm(uint16_t *unitsPerEm, uint32_t client);
extern int headGetSetLsb(uint16_t *setLsb, uint32_t client);
extern int headGetBBox(int16_t *xMin, int16_t *yMin, int16_t *xMax, int16_t *yMax,
                        uint32_t client);
extern char *headGetCreatedDate(uint32_t client);
extern char *headGetModifiedDate(uint32_t client);
extern int headGetFontRevision(float *rev, uint32_t client);
extern int headGetVersion(float *ver, uint32_t client);
#endif /* HEAD_H */
