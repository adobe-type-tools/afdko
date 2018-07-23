/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)name.h	1.3
 * Changed:    3/9/98 13:18:38
 ***********************************************************************/

/*
 * name table support.
 */

#ifndef NAME_H
#define NAME_H

#include "global.h"

extern void nameRead(LongN offset, Card32 length);
extern void nameDump(IntX level, LongN offset);
extern void nameFree(void);
extern void nameUsage(void);

extern Byte8 *nameFontName(void);
extern Byte8 *namePostScriptName(void);

#endif /* NAME_H */
