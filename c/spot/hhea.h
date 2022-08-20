/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * hhea table support.
 */

#ifndef HHEA_H
#define HHEA_H

#include "spot_global.h"

extern void hheaRead(int32_t offset, uint32_t length);
extern void hheaDump(int level, int32_t offset);
extern void hheaFree_spot(void);

extern void hheaGetTypocenders(int32_t *ascender, int32_t *descender);
extern int hheaGetNLongHorMetrics(uint16_t *nLongHorMetrics, uint32_t client);

#endif /* HHEA_H */
