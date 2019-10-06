/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Platform/script/language/name description.
 */

#ifndef DDESC_H
#define DDESC_H

#include "Dglobal.h"

extern Byte8 *descPlat(Card16 platformId);
extern Byte8 *descScript(Card16 platformId, Card16 scriptId);
extern Byte8 *descLang(Card16 cmap, Card16 platformId, Card16 languageId);
extern Byte8 *descName(Card16 nameId);

#endif /* DDESC_H */
