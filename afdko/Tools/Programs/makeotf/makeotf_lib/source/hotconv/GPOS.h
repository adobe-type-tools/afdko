/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/


#ifndef GPOS_H
#define GPOS_H

#include "common.h"
#include "feat.h"

#define GPOS_   TAG('G', 'P', 'O', 'S')

/* Standard functions */

void GPOSNew(hotCtx g);
int GPOSFill(hotCtx g);
void GPOSWrite(hotCtx g);
void GPOSReuse(hotCtx g);
void GPOSFree(hotCtx g);

/* Supplementary functions (See otl.h for script, language, feature,
   and lookup flag definitions) */
void GPOSFeatureBegin(hotCtx g, Tag script, Tag language, Tag feature);
void GPOSFeatureEnd(hotCtx g);

/* Lookup types */
enum {
	GPOSSingle = 1,
	GPOSPair,
	GPOSCursive,
	GPOSMarkToBase,
	GPOSMarkToLigature,
	GPOSMarkToMark,
	GPOSContext,
	GPOSChain,
	GPOSExtension,      /* Handled specially: it points to any of the above */
	GPOSFeatureParam,   /* Treated like a lookup in the code */
};

void GPOSRuleAdd(hotCtx g, int lkpType, GNode *targ, char *fileName, long lineNum, int anchorCount, AnchorMarkInfo *anchorMarkInfo);

void GPOSLookupBegin(hotCtx g, unsigned lkpType, unsigned lkpFlag,
                     Label label, short useExtension, unsigned short useMarkSetIndex);
void GPOSLookupEnd(hotCtx g, Tag feature);

void GPOSAddSize(hotCtx g, short *params, unsigned short numParams);

void GPOSSetSizeMenuNameID(hotCtx g, unsigned short nameID);

int GPOSSubtableBreak(hotCtx g);

void GPOSAddPair(hotCtx g, void *si, GNode *first, GNode *second, char *filename, int lineNum);

void GPOSPrintAFMKernData(hotCtx g);

#endif /* GPOS_H */
