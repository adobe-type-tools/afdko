/* Copyright 2020 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_STAT_H
#define HOTCONV_STAT_H

#include "common.h"

#define STAT_ TAG('S', 'T', 'A', 'T')

/* Standard functions */
void STATNew(hotCtx g);
int STATFill(hotCtx g);
void STATWrite(hotCtx g);
void STATReuse(hotCtx g);
void STATFree(hotCtx g);

/* Supplementary functions */

void STATAddDesignAxis(hotCtx g, Tag tag, uint16_t nameID, uint16_t ordering);
void STATAddAxisValueTable(hotCtx g, uint16_t format, Tag *axisTags,
                           Fixed *values, long count, uint16_t flags,
                           uint16_t nameID, Fixed minValue, Fixed maxValue);
bool STATSetElidedFallbackNameID(hotCtx g, uint16_t nameID);

#endif /* HOTCONV_STAT_H */
