/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * name table support.
 */

#ifndef DNAME_H
#define DNAME_H

#include "Dglobal.h"

extern void nameReset(void);

extern void nameRead(Card8 which, LongN offset, Card32 length);
extern void nameDiff(LongN offset1, LongN offset2);
extern void nameFree(Card8 which);
extern Byte8 *nameFontName(Card8 which);
extern Byte8 *namePostScriptName(Card8 which);

#endif /* DNAME_H */
