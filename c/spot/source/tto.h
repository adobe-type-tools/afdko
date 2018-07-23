/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * TT Open common table support.
 */

#ifndef TTO_H
#define TTO_H

#include "global.h"
#include "sfnt_tto.h"
#include "da.h"

typedef struct _ttoEnumRec
{
  IntX mingid, maxgid;
  da_DCL(GlyphId, glyphidlist);
} ttoEnumRec;

extern void ttoReadScriptList(Card32 offset, ScriptList *list);
extern void ttoDumpScriptList(Offset offset, ScriptList *list, IntX level);
extern void ttoFreeScriptList(ScriptList *list);

extern void ttoReadFeatureList(Card32 offset, FeatureList *list);
extern void ttoDumpFeatureList(Offset offset, FeatureList *list, IntX level);
extern void ttoFreeFeatureList(FeatureList *list);

typedef void *(*ttoReadCB)(Card32 offset, Card16 type);
typedef void (*ttoDumpCB)(LOffset offset, Card16 type, 
						  void *subtable, IntX level, void *arg, 
						  IntX lookupListIndex, IntX subTableIndex, IntX subTableCount, IntX recursion);
typedef void (ttoPostDumpCB)(Tag feature);
typedef void (*ttoMessageCB)(char *str, Tag feature);
typedef void (*ttoFreeCB)(Card16 type, void *subtable);

extern void ttoDumpFeaturesByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist, 
							ttoDumpCB dumpCB, IntX level);
extern void ttoReadLookupList(Card32 offset, LookupList *list, 
							  ttoReadCB readCB);
extern void ttoDumpLookupList(Offset offset, LookupList *list, IntX level,
							  ttoDumpCB dumpCB);
extern void ttoDumpLookupListItem(Offset offset, LookupList *list, IntX which,
								  IntX level, ttoDumpCB dumpCB, void *feattag);
extern void ttoDumpLookupifFeaturePresent(LookupList *lookuplist, 
										  FeatureList *featurelist,
										  ScriptList * scriptlist,
										  IntX level,
										  ttoDumpCB dumpCB, ttoMessageCB messageCB, ttoPostDumpCB postDumpCB);
extern void ttoFreeLookupList(LookupList *list, ttoFreeCB freeCB);

extern void *ttoReadCoverage(Card32 offset);
extern IntX ttoGlyphIsInCoverage(Offset offset, void *coverage, GlyphId gid, IntX *where);
extern void ttoDumpCoverage(Offset offset, void *coverage, IntX level);
extern void ttoEnumerateCoverage(Offset offset, void *coverage,
								 ttoEnumRec *coverageenum, 
								 Card32 *numitems);
extern void ttoFreeCoverage(void *coverage);

extern void *ttoReadClass(Card32 offset);
extern IntX ttoGlyphIsInClass(Offset offset, void *class, GlyphId gid, IntX *whichclassnum);
extern void ttoDumpClass(Offset offset, void *class, IntX level);
extern void ttoEnumerateClass(Offset offset, void *class,
							  IntX numclasses,
							  ttoEnumRec *classenumarray,
							  Card32 *numitems);
extern void ttoFreeClass(void *class);

extern void ttoReadDeviceTable(Card32 offset, DeviceTable *table);
extern void ttoDumpDeviceTable(Offset offset, DeviceTable *table, IntX level);
extern void ttoFreeDeviceTable(DeviceTable *table);


extern void ttoDecompileByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist, ttoDumpCB dumpCB, IntX level);

#endif /* TTO_H */
