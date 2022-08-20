/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * name table support.
 */

#ifndef NAME_H
#define NAME_H

#include "spot_global.h"

extern void nameRead(int32_t offset, uint32_t length);
extern void nameDump(int level, int32_t offset);
extern void nameFree_spot(void);
extern void nameUsage(void);

extern int8_t *nameFontName(void);
extern int8_t *namePostScriptName(void);

#endif /* NAME_H */
