/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef CFF__H
#define CFF__H

#include "common.h"

#define CFF__   TAG('C', 'F', 'F', ' ')

/* Standard functions */
void CFF_New(hotCtx g);
int CFF_Fill(hotCtx g);
void CFF_Write(hotCtx g);
void CFF_Reuse(hotCtx g);
void CFF_Free(hotCtx g);

#endif /* CFF__H */
