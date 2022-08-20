/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * vhea table support.
 */

#ifndef VHEA_H
#define VHEA_H

#include "spot_global.h"

extern void vheaRead(int32_t offset, uint32_t length);
extern void vheaDump(int level, int32_t offset);
extern void vheaFree_spot(void);

extern int vheaGetNLongVerMetrics(uint16_t *nLongVerMetrics, uint32_t client);

#endif /* VHEA_H */
