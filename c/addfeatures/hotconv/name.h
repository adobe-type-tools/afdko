/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_NAME_H_
#define ADDFEATURES_HOTCONV_NAME_H_

#include <string>

#include "common.h"
#include "namesupport.h"

#define name_ TAG('n', 'a', 'm', 'e')

#define MISSING_MAC_DEFAULT_NAME 0x0002
#define MISSING_WIN_DEFAULT_NAME 0x0001

/* Standard functions */
void nameNew(hotCtx g);
int nameFill(hotCtx g);
void nameWrite(hotCtx g);
void nameReuse(hotCtx g);
void nameFree(hotCtx g);

void nameAdd(hotCtx g, uint16_t platformId, uint16_t platspecId,
             uint16_t languageId, uint16_t nameId, const std::string &str);

int verifyDefaultNames(hotCtx g, uint16_t nameId);

#endif  // ADDFEATURES_HOTCONV_NAME_H_
