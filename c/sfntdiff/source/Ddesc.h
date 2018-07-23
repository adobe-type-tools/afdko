/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)desc.h	1.1
 * Changed:    10/12/95 14:56:52
 ***********************************************************************/

/*
 * Platform/script/language/name description.
 */

#ifndef DESC_H
#define DESC_H

#include "Dglobal.h"

extern Byte8 *descPlat(Card16 platformId);
extern Byte8 *descScript(Card16 platformId, Card16 scriptId);
extern Byte8 *descLang(Card16 cmap, Card16 platformId, Card16 languageId);
extern Byte8 *descName(Card16 nameId);

#endif /* DESC_H */
