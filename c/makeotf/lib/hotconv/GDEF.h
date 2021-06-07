/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_GDEF_H
#define HOTCONV_GDEF_H

#include "common.h"
#include "feat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GDEF_ TAG('G', 'D', 'E', 'F')
#define kMaxMarkAttachClasses 15

/* Standard functions */
void GDEFNew(hotCtx g);
int GDEFFill(hotCtx g);
void GDEFWrite(hotCtx g);
void GDEFReuse(hotCtx g);
void GDEFFree(hotCtx g);

void setGlyphClassGDef(hotCtx g, GNode *simpl, GNode *ligature, GNode *mark,
                       GNode *component);

int addAttachEntryGDEF(hotCtx g, GNode *glyphNode, unsigned short contour);

void addLigCaretEntryGDEF(hotCtx g, GNode *glyphNode, unsigned short* caretValue, int caretCount, unsigned short format);

unsigned short addGlyphMarkClassGDEF(hotCtx g, GNode *markClass);

unsigned short addMarkSetClassGDEF(hotCtx g, GNode *markClass);

#ifdef __cplusplus
}
#endif

#endif /* HOTCONV_GDEF_H */
