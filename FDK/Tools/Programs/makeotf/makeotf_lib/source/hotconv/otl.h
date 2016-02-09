/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef OTL_H
#define OTL_H

#include "common.h"
#include "feat.h"

/* Module-wide functions */
void otlNew(hotCtx g);
void otlFree(hotCtx g);

/* Client table functions */
typedef struct otlTbl_ *otlTbl;

otlTbl otlTableNew(hotCtx g);
void otlTableFill(hotCtx g, otlTbl t);
void otlTableFillStub(hotCtx g, otlTbl t);
void otlTableWrite(hotCtx g, otlTbl t);
void otlTableReuse(hotCtx g, otlTbl t);
void otlTableFree(hotCtx g, otlTbl t);

void sortLabelList(hotCtx g, otlTbl t);
int otlLabel2LookupIndex(hotCtx g, otlTbl t, int baselabel);
void checkStandAloneTablRefs(hotCtx g, otlTbl t);

#if HOT_DEBUG
void otlDumpSubtables(hotCtx g, otlTbl t);
void otlDumpSizes(hotCtx g, otlTbl t, LOffset subtableSize,
                  LOffset extensionSectionSize);

#endif /* HOT_DEBUG */

/* Script tags used by hotconv */
#define arab_   TAG('a', 'r', 'a', 'b')    /* Arabic */
#define cyrl_   TAG('c', 'y', 'r', 'l')    /* Cyrillic */
#define deva_   TAG('d', 'e', 'v', 'a')    /* Devanagari */
#define grek_   TAG('g', 'r', 'e', 'k')    /* Greek */
#define gujr_   TAG('g', 'u', 'j', 'r')    /* Gujarati */
#define hebr_   TAG('h', 'e', 'b', 'r')    /* Hebrew */
#define kana_   TAG('k', 'a', 'n', 'a')    /* Kana (Hiragana & Katakana) */
#define latn_   TAG('l', 'a', 't', 'n')    /* Latin */
#define punj_   TAG('p', 'u', 'n', 'j')    /* Gurmukhi */
#define thai_   TAG('t', 'h', 'a', 'i')    /* Thai */
#define DFLT_   TAG('D', 'F', 'L', 'T')    /* Default */

/* Language tags used by hotconv */
#define dflt_   TAG(' ', ' ', ' ', ' ')    /* Default (sorts first) */

/* Feature tags used by hotconv */
#define aalt_   TAG('a', 'a', 'l', 't')    /* All alternates */
#define kern_   TAG('k', 'e', 'r', 'n')    /* Kerning */
#define vkrn_   TAG('v', 'k', 'r', 'n')    /* Vertical kerning */
#define vpal_   TAG('v', 'p', 'a', 'l')    /* Vertical kerning */
#define vhal_   TAG('v', 'h', 'a', 'l')    /* Vertical kerning */
#define valt_   TAG('v', 'a', 'l', 't')    /* Vertical kerning */
#define vert_   TAG('v', 'e', 'r', 't')    /* Vertical writing */
#define vrt2_   TAG('v', 'r', 't', '2')    /* Vertical rotation */
#define size_   TAG('s', 'i', 'z', 'e')    /* Vertical rotation */

/* Subtable lookup flags */
#define otlRightToLeft          (1 << 0)
#define otlIgnoreBaseGlyphs     (1 << 1)
#define otlIgnoreLigatures      (1 << 2)
#define otlIgnoreMarks          (1 << 3)
#define otlUseMarkFilteringSet (1 << 4)
#define otlMarkAttachmentType   0xFF00 /* Mask */

void otlSubtableAdd(hotCtx g, otlTbl t, Tag script, Tag language, Tag feature,
                    int lkpType, int lkpFlag, unsigned short markSetIndex, unsigned extensionLookupType,
                    unsigned offset, Label label, unsigned short fmt,
                    int isFeatParam);

/* --- Coverage table --- */

void otlCoverageBegin(hotCtx g, otlTbl t);
void otlCoverageAddGlyph(hotCtx g, otlTbl t, GID glyph);
Offset otlCoverageEnd(hotCtx g, otlTbl t);
void otlCoverageWrite(hotCtx g, otlTbl t);

/* --- Class table --- */

void otlClassBegin(hotCtx g, otlTbl t);
void otlClassAddMapping(hotCtx g, otlTbl t, GID glyph, unsigned class);
Offset otlClassEnd(hotCtx g, otlTbl t);
void otlClassWrite(hotCtx g, otlTbl t);

/* --- Device table */
void otlDeviceBeg(hotCtx g, otlTbl t);
void otlDeviceAddEntry(hotCtx g, otlTbl t, unsigned short ppem, short delta);
Offset otlDeviceEnd(hotCtx g, otlTbl t);
void otlDeviceWrite(hotCtx g, otlTbl t);

/* --- Size functions */

LOffset otlGetCoverageSize(otlTbl t);
LOffset otlGetClassSize(otlTbl t);

#endif /* OTL_H */
