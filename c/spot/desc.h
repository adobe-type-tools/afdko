/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Platform/script/language/name description.
 */

#include <stdint.h>

#ifndef DESC_H
#define DESC_H

extern char *descPlat(uint16_t platformId);
extern char *descScript(uint16_t platformId, uint16_t scriptId);
extern char *descLang(uint16_t cmap, uint16_t platformId, uint16_t languageId);
extern char *descName(uint16_t nameId);

#endif /* DESC_H */
