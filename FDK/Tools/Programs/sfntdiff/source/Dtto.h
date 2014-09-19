/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)tto.h	1.3
 * Changed:    12/15/97 08:56:39
 ***********************************************************************/

/*
 * TT Open common table support.
 */

#ifndef TTO_H
#define TTO_H

#include "Dglobal.h"
#include "sfnt_tto.h"

#include "Dda.h"

typedef struct _ttoEnumRec
{
  IntX mingid, maxgid;
  da_DCL(GlyphId, glyphidlist);
} ttoEnumRec;

extern void ttoReadScriptList(Card8 which, Card32 offset, ScriptList *list);
extern void ttoDumpScriptList(Offset offset, ScriptList *list, IntX level);
extern void ttoFreeScriptList(ScriptList *list);

extern void ttoReadFeatureList(Card8 which, Card32 offset, FeatureList *list);
extern void ttoDumpFeatureList( Offset offset, FeatureList *list, IntX level);
extern void ttoFreeFeatureList(FeatureList *list);

typedef void *(*ttoReadCB)(Card8 which, Card32 offset, Card16 type);
typedef void (*ttoDumpCB)(Offset offset, Card16 type, 
						  void *subtable, IntX level, void *arg);
typedef void (*ttoFreeCB)(void *subtable);

extern void ttoReadLookupList(Card8 which, Card32 offset, LookupList *list, 
							  ttoReadCB readCB);
extern void ttoDumpLookupList( Offset offset, LookupList *list, IntX level,
							  ttoDumpCB dumpCB);
extern void ttoDumpLookupListItem(Offset offset, LookupList *list, IntX which,
								  IntX level, ttoDumpCB dumpCB);
extern void ttoFreeLookupList(LookupList *list, ttoFreeCB freeCB);

extern void *ttoReadCoverage(Card8 which, Card32 offset);
extern void ttoDumpCoverage(Card8 which, Offset offset, void *coverage, IntX level);
extern void ttoEnumerateCoverage(Offset offset, void *coverage,
								 ttoEnumRec *coverageenum, 
								 Card32 *numitems);
extern void ttoFreeCoverage(void *coverage);

extern void *ttoReadClass(Card8 which, Card32 offset);
extern void ttoDumpClass(Card8 which, Offset offset, void *class, IntX level);
extern void ttoEnumerateClass(Offset offset, void *class,
							  IntX numclasses,
							  ttoEnumRec *classenumarray,
							  Card32 *numitems);
extern void ttoFreeClass(void *class);

extern void ttoReadDeviceTable(Card8 which, Card32 offset, DeviceTable *table);
extern void ttoDumpDeviceTable(Offset offset, DeviceTable *table, IntX level);
extern void ttoFreeDeviceTable(DeviceTable *table);

#endif /* TTO_H */
