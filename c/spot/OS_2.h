/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * OS_2 table support.
 */

#ifndef OS_2_H
#define OS_2_H

#include "spot_global.h"

extern void OS_2Read(int32_t offset, uint32_t length);
extern void OS_2Dump(int level, int32_t offset);
extern void OS_2GetTypocenders(int32_t *ascender, int32_t *descender);
extern void OS_2Free_spot(void);

#endif /* OS_2_H */
