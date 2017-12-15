/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef MMSD_H
#define MMSD_H

#include "common.h"

#define MMSD_   TAG('M', 'M', 'S', 'D')

/* Standard functions */
void MMSDNew(hotCtx g);
int MMSDFill(hotCtx g);
void MMSDWrite(hotCtx g);
void MMSDReuse(hotCtx g);
void MMSDFree(hotCtx g);

/* Supplementary functions */

#endif /* MMSD_H */
