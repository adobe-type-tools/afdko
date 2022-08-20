/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * CID_ table support.
 */

#ifndef CID__H
#define CID__H

#include "spot_global.h"

extern void CID_Read(int32_t offset, uint32_t length);
extern void CID_Dump(int level, int32_t offset);
extern void CID_Free(void);

extern int CID_GetNGlyphs(uint16_t *nGlyphs, uint32_t client);
extern int CID_isCID(void);
#endif /* CID__H */
