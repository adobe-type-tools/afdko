/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sfnt.h	1.5
 * Changed:    3/16/98 10:59:31
 ***********************************************************************/

/*
 * sfnt reading/dumping.
 */

#ifndef SFNT_H
#define SFNT_H

#include "Dglobal.h"
#include "Dopt.h"

extern void sfntTTCRead(Card8 which, LongN start);
extern void sfntRead(LongN start1, IntX id1, LongN start2, IntX id2);
extern void sfntDump(void);
extern void sfntFree(void);
extern IntX sfntReadTable(Card32 tag);
extern IntX sfntReadATable(Card8 which, Card32 tag);

extern opt_Scanner sfntTagScan;
extern opt_Scanner sfntTTCScan;
extern void sfntUsage(void);
extern void sfntTableSpecificUsage(void);

extern void sfntReset();

void hexDump(Card8 which, Card32 tag, LongN start, Card32 length);
void hexDiff(Card32 tag, LongN start1, Card32 length1, LongN start2, Card32 length2);
#endif /* SFNT_H */
