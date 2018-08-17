/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef MEMCHECK_H
#define MEMCHECK_H

void memAllocated(void *ptr);
void memDeallocated(void *ptr);
void memReport(void);

#endif /* MEMCHECK_H */
