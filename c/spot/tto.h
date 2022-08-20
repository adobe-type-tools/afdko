/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TT Open common table support.
 */

#ifndef TTO_H
#define TTO_H

#include "spot_global.h"
#include "sfnt_tto.h"
#include "da.h"

typedef struct _ttoEnumRec {
    int mingid, maxgid;
    da_DCL(GlyphId, glyphidlist);
} ttoEnumRec;

extern void ttoReadScriptList(uint32_t offset, ScriptList *list);
extern void ttoDumpScriptList(Offset offset, ScriptList *list, int level);
extern void ttoFreeScriptList(ScriptList *list);

extern void ttoReadFeatureList(uint32_t offset, FeatureList *list);
extern void ttoDumpFeatureList(Offset offset, FeatureList *list, int level);
extern void ttoFreeFeatureList(FeatureList *list);

typedef void *(*ttoReadCB)(uint32_t offset, uint16_t type);
typedef void (*ttoDumpCB)(LOffset offset, uint16_t type,
                          void *subtable, int level, void *arg,
                          int lookupListIndex, int subTableIndex, int subTableCount, int recursion);
typedef void(ttoPostDumpCB)(Tag feature);
typedef void (*ttoMessageCB)(char *str, Tag feature);
typedef void (*ttoFreeCB)(uint16_t type, void *subtable);

extern void ttoDumpFeaturesByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist,
                                    ttoDumpCB dumpCB, int level);
extern void ttoReadLookupList(uint32_t offset, LookupList *list,
                              ttoReadCB readCB);
extern void ttoDumpLookupList(Offset offset, LookupList *list, int level,
                              ttoDumpCB dumpCB);
extern void ttoDumpLookupListItem(Offset offset, LookupList *list, int which,
                                  int level, ttoDumpCB dumpCB, void *feattag);
extern void ttoDumpLookupifFeaturePresent(LookupList *lookuplist,
                                          FeatureList *featurelist,
                                          ScriptList *scriptlist,
                                          int level,
                                          ttoDumpCB dumpCB, ttoMessageCB messageCB, ttoPostDumpCB postDumpCB);
extern void ttoFreeLookupList(LookupList *list, ttoFreeCB freeCB);

extern void *ttoReadCoverage(uint32_t offset);
extern int ttoGlyphIsInCoverage(Offset offset, void *coverage, GlyphId gid, int *where);
extern void ttoDumpCoverage(Offset offset, void *coverage, int level);
extern void ttoEnumerateCoverage(Offset offset, void *coverage,
                                 ttoEnumRec *coverageenum,
                                 uint32_t *numitems);
extern void ttoFreeCoverage(void *coverage);

extern void *ttoReadClass(uint32_t offset);
extern int ttoGlyphIsInClass(Offset offset, void *class, GlyphId gid, int *whichclassnum);
extern void ttoDumpClass(Offset offset, void *class, int level);
extern void ttoEnumerateClass(Offset offset, void *class,
                              int numclasses,
                              ttoEnumRec *classenumarray,
                              uint32_t *numitems);
extern void ttoFreeClass(void *class);

extern void ttoReadDeviceTable(uint32_t offset, DeviceTable *table);
extern void ttoDumpDeviceTable(Offset offset, DeviceTable *table, int level);
extern void ttoFreeDeviceTable(DeviceTable *table);

extern void ttoDecompileByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist, ttoDumpCB dumpCB, int level);

#endif /* TTO_H */
