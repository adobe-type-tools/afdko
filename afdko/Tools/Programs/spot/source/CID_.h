/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CID_.h	1.3
 * Changed:    3/9/98 13:18:59
 ***********************************************************************/

/*
 * CID_ table support.
 */

#ifndef CID__H
#define CID__H

#include "global.h"

extern void CID_Read(LongN offset, Card32 length);
extern void CID_Dump(IntX level, LongN offset);
extern void CID_Free(void);

extern IntX CID_GetNGlyphs(Card16 *nGlyphs, Card32 client);
extern IntX CID_isCID(void);
#endif /* CID__H */



