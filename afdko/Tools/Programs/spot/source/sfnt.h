/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sfnt.h	1.7
 * Changed:    7/14/99 10:02:04
 ***********************************************************************/

/*
 * sfnt reading/dumping.
 */

#ifndef SFNT_H
#define SFNT_H

#include "global.h"
#include "opt.h"

extern void sfntTTCRead(LongN start);
extern void sfntRead(LongN start, IntX id);
extern IntX sfntReadTable(Card32 tag);
extern void sfntDump(void);
extern void sfntFree(IntX freeTTC);

extern opt_Scanner sfntTagScan;
extern opt_Scanner sfntTTCScan;
extern void sfntUsage(void);
extern void sfntTableSpecificUsage(void);
extern opt_Scanner sfntFeatScan;
extern IntX sfntIsInFeatProofList(Card32 feat_tag); /* 0=>not in list, 
													   >0 is in list:
													   return dump level,
													  -1=>empty list */
extern void sfntAllProcessedProofList();
extern void resetReferencedList(void);
extern void addToReferencedList(Card32 n);
extern Card32 numReferencedList(void);
extern Int32 getReferencedListLookup( Card32 n);

#endif /* SFNT_H */
