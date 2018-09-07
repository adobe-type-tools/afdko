/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * LTSH table support.
 */

#ifndef LTSH_H
#define LTSH_H

#include "global.h"

extern void LTSHRead(LongN offset, Card32 length);
extern void LTSHDump(IntX level, LongN offset);
extern void LTSHFree(void);

#endif /* LTSH_H */
