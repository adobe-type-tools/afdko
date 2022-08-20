/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * SING table support.
 */

#ifndef SING_H
#define SING_H

#include "spot_global.h"

extern void SINGRead(int32_t offset, uint32_t length);
extern void SINGDump(int level, int32_t offset);
extern void SINGFree(void);

extern int SINGGetUnitsPerEm(uint16_t *unitsPerEm, uint32_t client);

#endif /* SING_H */
