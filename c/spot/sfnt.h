/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * sfnt reading/dumping.
 */

#ifndef SFNT_H
#define SFNT_H

#include "spot_global.h"
#include "opt.h"

extern void sfntTTCRead(int32_t start);
extern void sfntRead(int32_t start, int id);
extern int sfntReadTable(uint32_t tag);
extern void sfntDump(void);
extern void sfntFree_spot(int freeTTC);

extern opt_Scanner sfntTagScan;
extern opt_Scanner sfntTTCScan;
extern void sfntUsage(void);
extern void sfntTableSpecificUsage(void);
extern opt_Scanner sfntFeatScan;
extern int sfntIsInFeatProofList(uint32_t feat_tag); /* 0=>not in list,      */
                                                    /* >0 is in list:       */
                                                    /*   return dump level, */
                                                    /* -1=>empty list       */
extern void sfntAllProcessedProofList(void);
extern void resetReferencedList(void);
extern void addToReferencedList(uint32_t n);
extern uint32_t numReferencedList(void);
extern int32_t getReferencedListLookup(uint32_t n);

#endif /* SFNT_H */
