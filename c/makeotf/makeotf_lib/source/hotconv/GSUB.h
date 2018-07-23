/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/


#ifndef GSUB_H
#define GSUB_H

#include "common.h"
#include "feat.h"

#define GSUB_   TAG('G', 'S', 'U', 'B')

/* Standard functions */

void GSUBNew(hotCtx g);
int GSUBFill(hotCtx g);
void GSUBWrite(hotCtx g);
void GSUBReuse(hotCtx g);
void GSUBFree(hotCtx g);

/* Supplementary functions (See otl.h for script, language, feature,
   and lookup flag definitions) */
void GSUBFeatureBegin(hotCtx g, Tag script, Tag language, Tag feature);
void GSUBFeatureEnd(hotCtx g);

/* Lookup types */
enum {
	GSUBSingle = 1,
	GSUBMultiple,
	GSUBAlternate,
	GSUBLigature,
	GSUBContext,
	GSUBChain,
	GSUBExtension,  /* Handled specially: it points to any of the above */
	GSUBReverse,
	GSUBFeatureNameParam,
    GSUBCVParam,
};

void GSUBLookupBegin(hotCtx g, unsigned lkpType, unsigned lkpFlag,
                     Label label, short useExtension, unsigned short useMarkSetIndex);
void GSUBLookupEnd(hotCtx g, Tag feature);
void GSUBRuleAdd(hotCtx g, GNode *targ, GNode *repl);
int GSUBSubtableBreak(hotCtx g);
void GSUBSetFeatureNameID(hotCtx g, Tag feat, unsigned short nameID);
void GSUBAddFeatureMenuParam(hotCtx g, void *param);
void GSUBAddCVParam(hotCtx g, void *param);

/*
   Each feature definition is bracketed by a GSUBFeatureBegin() and
   GSUBFeatureEnd(). Each lookup definition is bracketed by GSUBLookupBegin()
   and GSUBLookupEnd(). Each substitution rule is specified by GSUBRuleAdd().
   Substitution occurs when "target" glyphs are substituted by "replace"
   glyphs.
                    target                replacement
                    ------                -----------
   Single:			Single|Class          Single|Class
   Ligature:        Sequence              Single
   Alternate:       Single                Class
   Contextual:      Sequence              Single

   For example, fi and fl ligature formation may be specified thus:

   GSUBFeatureBegin(g, latn_, dflt_, liga_);

   GSUBLookupBegin(g, GSUBLigature, 0, label, 0, 0);

   GSUBRuleAdd(g, targ, repl);	// targ->("f")->("i"), repl->("fi")
   GSUBRuleAdd(g, targ, repl);	// targ->("f")->("l"), repl->("fl")

   GSUBLookupEnd(g);

   GSUBFeatureEnd(g); */

#endif /* GSUB_H */
