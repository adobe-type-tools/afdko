/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef GDEF_H
#define GDEF_H

#include "common.h"
#include "feat.h"

#define GDEF_   TAG('G', 'D', 'E', 'F')
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

int addLigCaretEntryGDEF(hotCtx g, GNode *glyphNode, unsigned short caretValue, unsigned short format);

unsigned short addGlyphMarkClassGDEF(hotCtx g, GNode *markClass);

unsigned short addMarkSetClassGDEF(hotCtx g, GNode *markClass);


#endif /* GDEF_H */
