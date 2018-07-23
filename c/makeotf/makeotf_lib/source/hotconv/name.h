/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef  NAME_H
#define  NAME_H

#include "common.h"

#define name_   TAG('n', 'a', 'm', 'e')

#define MISSING_MAC_DEFAULT_NAME 0x0002
#define MISSING_WIN_DEFAULT_NAME 0x0001

/* Standard functions */
void nameNew(hotCtx g);
int nameFill(hotCtx g);
void nameWrite(hotCtx g);
void nameReuse(hotCtx g);
void nameFree(hotCtx g);

/* Supplementary functions */
void nameAddReg(hotCtx g,
                unsigned short platformId,
                unsigned short platspecId,
                unsigned short languageId,
                unsigned short nameId, char *str);
unsigned short nameAddUser(hotCtx g, char *str);

unsigned short nameReserveUserID(hotCtx g);

int nameVerifyDefaultNames(hotCtx g, unsigned short nameId);


#endif /* NAME_H */
