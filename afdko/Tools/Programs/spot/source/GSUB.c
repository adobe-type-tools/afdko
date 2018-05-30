/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GSUB.c	1.20
 * Changed:    8/4/99 11:28:28
 ***********************************************************************/
 
#include <string.h>
#include "tto.h"
#include "sfnt_GSUB.h"

#include "GSUB.h"
#include "head.h"
#include "sfnt.h"
#include "proof.h"


static GSUBTbl GSUB;
static IntX loaded = 0;
static Card32 GSUBStart;
static Byte8 contextPrefix[MAX_NAME_LEN];

static ProofContextPtr proofctx = NULL;
/*static Card32 featuretag = 0;*/
static Card32 prev_featuretag = 0;
static Card32 prev_scripttag = 0;
static Card32 prev_langtag = 0;
static Card32 prev_lookupListIndex = 0;
static Card16 unitsPerEm;

typedef struct {
	IntX seen;
	IntX parent;
	IntX cur_parent;
	} SeenChainLookup;

static IntX GSUBSubtableindex = 0;
static IntX GSUBSubtableCnt = 0;
static IntX GSUBLookupIndex = 0;
static IntX GSUBLookupCnt = 0;
static IntX GSUBContextRecursionCnt = 0;

static SeenChainLookup * seenChainLookup;

static void dumpSubtable(LOffset offset, Card16 type, void *subtable, 
						 IntX level, void *feattag, IntX lookupListIndex, IntX subTableIndex, IntX subtableCount, IntX recursion);
static void *readSubtable(Card32 offset, Card16 type);
static void evalSubtable(Card16 type, void *subtable,
			  IntX numinputglyphs, GlyphId *inputglyphs, 
			  IntX *numoutputglyphs, GlyphId *outputglyphs);



static void *readSingle1(Card32 offset)
	{
	SingleSubstFormat1 *fmt = memNew(sizeof(SingleSubstFormat1));

	fmt->SubstFormat = 1;	/* Already read */
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->DeltaGlyphId);

	return fmt;
	}

static void *readSingle2(Card32 offset)
	{
	IntX i;
	SingleSubstFormat2 *fmt = memNew(sizeof(SingleSubstFormat2));

	fmt->SubstFormat = 2;	/* Already read */
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->GlyphCount);

	fmt->Substitute = memNew(sizeof(GlyphId) * fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++)
		IN1(fmt->Substitute[i]);

	return fmt;
	}

static void *readSingle(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readSingle1(offset);
	case 2:
		return readSingle2(offset);
	default:
		warning(SPOT_MSG_GSUBUNKSINGL, format, offset);
		return NULL;
		}
	}

static void *readOverflow1(Card32 offset)
	{
	OverflowSubstFormat1 *fmt = memNew(sizeof(OverflowSubstFormat1));

	fmt->SubstFormat = 1;	/* Already read */
	IN1(fmt->OverflowLookupType);
	IN1(fmt->OverflowOffset);
	fmt->subtable=readSubtable(fmt->OverflowOffset+offset, fmt->OverflowLookupType);
	return fmt;
	}
	
static void *readOverflow(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readOverflow1(offset);
	default:
		warning(SPOT_MSG_GSUBUNKSINGL, format, offset);
		return NULL;
		}
	}

static void readSequence(Card32 offset, Sequence *sequence)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(sequence->GlyphCount);
	
	sequence->Substitute = memNew(sizeof(GlyphId) * sequence->GlyphCount);
	for (i = 0; i < sequence->GlyphCount; i++)
		IN1(sequence->Substitute[i]);

	SEEK_ABS(save);
	}

static void *readMultiple1(Card32 offset)
	{
	IntX i;
	MultipleSubstFormat1 *fmt = memNew(sizeof(MultipleSubstFormat1));

	fmt->SubstFormat = 1;	/* Already read */
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->SequenceCount);

	fmt->Sequence = memNew(sizeof(Offset) * fmt->SequenceCount);
	fmt->_Sequence = memNew(sizeof(Sequence) * fmt->SequenceCount);
	for (i = 0; i < fmt->SequenceCount; i++)
		{
		IN1(fmt->Sequence[i]);
		readSequence(offset + fmt->Sequence[i], &fmt->_Sequence[i]);
		}

	return fmt;
	}

static void *readMultiple(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readMultiple1(offset);
	default:
		warning(SPOT_MSG_GSUBUNKMULT, format, offset);
		return NULL;
		}
	}

static void readAlternateSet(Card32 offset, AlternateSet *altset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(altset->GlyphCount);
	
	altset->Alternate = memNew(sizeof(GlyphId) * altset->GlyphCount);
	for (i = 0; i < altset->GlyphCount; i++)
		IN1(altset->Alternate[i]);

	SEEK_ABS(save);
	}

static void *readAlternate1(Card32 offset)
	{
	IntX i;
	AlternateSubstFormat1 *fmt = memNew(sizeof(AlternateSubstFormat1));

	fmt->SubstFormat = 1;	/* Already read */
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->AlternateSetCnt);

	fmt->AlternateSet = memNew(sizeof(Offset) * fmt->AlternateSetCnt);
	fmt->_AlternateSet = memNew(sizeof(AlternateSet) * fmt->AlternateSetCnt);
	for (i = 0; i < fmt->AlternateSetCnt; i++)
		{
		IN1(fmt->AlternateSet[i]);
		readAlternateSet(offset + fmt->AlternateSet[i], &fmt->_AlternateSet[i]);
		}

	return fmt;
	}

static void *readAlternate(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readAlternate1(offset);
	default:
		warning(SPOT_MSG_GSUBUNKALT, format, offset);
		return NULL;
		}
	}

static void readLigatures(Card32 offset, Ligature *Ligature)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(Ligature->LigGlyph);
	IN1(Ligature->CompCount);

	Ligature->Component = memNew(sizeof(GlyphId) * (Ligature->CompCount - 1));
	for (i = 0; i < Ligature->CompCount - 1; i++)
		IN1(Ligature->Component[i]);

	SEEK_ABS(save);
	}

static void readLigatureSet(Card32 offset, LigatureSet *LigatureSet)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(LigatureSet->LigatureCount);
	
	LigatureSet->Ligature = 
		memNew(sizeof(Offset) * LigatureSet->LigatureCount);
	LigatureSet->_Ligature = 
		memNew(sizeof(Ligature) * LigatureSet->LigatureCount);
	for (i = 0; i < LigatureSet->LigatureCount; i++)
		{
		IN1(LigatureSet->Ligature[i]);
		readLigatures(offset + LigatureSet->Ligature[i], 
					 &LigatureSet->_Ligature[i]);
		}
	SEEK_ABS(save);
	}

static void *readLigature1(Card32 offset)
	{
	IntX i;
	LigatureSubstFormat1 *fmt = memNew(sizeof(LigatureSubstFormat1));

	fmt->SubstFormat = 1;	/* Already read */
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->LigSetCount);

	fmt->LigatureSet = memNew(sizeof(Offset) * fmt->LigSetCount);
	fmt->_LigatureSet = memNew(sizeof(LigatureSet) * fmt->LigSetCount);
	for (i = 0; i < fmt->LigSetCount; i++)
		{
		IN1(fmt->LigatureSet[i]);
		readLigatureSet(offset + fmt->LigatureSet[i], &fmt->_LigatureSet[i]);
		}
	return fmt;
	}

static void *readLigature(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readLigature1(offset);
	default:
		warning(SPOT_MSG_GSUBUNKLIG, format, offset);
		return NULL;
		}
	}

static void readSubRule(Card32 offset, SubRule *subrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subrule->GlyphCount);
	IN1(subrule->SubstCount);
	subrule->Input = memNew(sizeof(GlyphId) * subrule->GlyphCount);
	subrule->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * subrule->SubstCount);

	for (i = 1; i < subrule->GlyphCount; i++) 
	  {
		IN1(subrule->Input[i]);
	  }
	for (i = 0; i < subrule->SubstCount; i++) 
	  {
		IN1(subrule->SubstLookupRecord[i].SequenceIndex);
		IN1(subrule->SubstLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readSubRuleSet(Card32 offset, SubRuleSet *subruleset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subruleset->SubRuleCount);
	subruleset->SubRule = memNew(sizeof(Offset) * subruleset->SubRuleCount);
	subruleset->_SubRule = memNew(sizeof(SubRule) * subruleset->SubRuleCount);
	for (i = 0; i < subruleset->SubRuleCount; i++) 
	  {
		IN1(subruleset->SubRule[i]);
		readSubRule(offset + subruleset->SubRule[i], &subruleset->_SubRule[i]);
	  }
	SEEK_ABS(save);
	}

static void *readContext1(Card32 offset)
	{
	IntX i;
	ContextSubstFormat1 *fmt = memNew(sizeof(ContextSubstFormat1));

	fmt->SubstFormat = 1;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->SubRuleSetCount);
	fmt->SubRuleSet = memNew(sizeof(Offset) * fmt->SubRuleSetCount);
	fmt->_SubRuleSet = memNew(sizeof(SubRuleSet) * fmt->SubRuleSetCount);

	for (i = 0; i < fmt->SubRuleSetCount; i++) 
		{
		IN1(fmt->SubRuleSet[i]);
		readSubRuleSet(offset + fmt->SubRuleSet[i], &fmt->_SubRuleSet[i]);
		}
	return fmt;
	}

static void readSubClassRule(Card32 offset, SubClassRule *subclassrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subclassrule->GlyphCount);
	IN1(subclassrule->SubstCount);
	subclassrule->Class = memNew(sizeof(Card16) * subclassrule->GlyphCount);
	subclassrule->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * subclassrule->SubstCount);

	for (i = 1; i < subclassrule->GlyphCount; i++) 
	  {
		IN1(subclassrule->Class[i]);
	  }
	for (i = 0; i < subclassrule->SubstCount; i++) 
	  {
		IN1(subclassrule->SubstLookupRecord[i].SequenceIndex);
		IN1(subclassrule->SubstLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readSubClassSet(Card32 offset, SubClassSet *subclassset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subclassset->SubClassRuleCnt);
	subclassset->SubClassRule = memNew(sizeof(Offset) * subclassset->SubClassRuleCnt);
	subclassset->_SubClassRule = memNew(sizeof(SubRule) * subclassset->SubClassRuleCnt);
	for (i = 0; i < subclassset->SubClassRuleCnt; i++) 
		{
		IN1(subclassset->SubClassRule[i]);
		readSubClassRule(offset + subclassset->SubClassRule[i], &subclassset->_SubClassRule[i]);
		}
	SEEK_ABS(save);
	}

static void *readContext2(Card32 offset)
	{
	IntX i;
	ContextSubstFormat2 *fmt = memNew(sizeof(ContextSubstFormat2));

	fmt->SubstFormat = 2;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->ClassDef);
	fmt->_ClassDef = ttoReadClass(offset + fmt->ClassDef);
	IN1(fmt->SubClassSetCnt);
	fmt->SubClassSet = memNew(sizeof(Offset) * fmt->SubClassSetCnt);
	fmt->_SubClassSet = memNew(sizeof(SubRuleSet) * fmt->SubClassSetCnt);

	for (i = 0; i < fmt->SubClassSetCnt; i++) 
		{
		IN1(fmt->SubClassSet[i]);
		if (fmt->SubClassSet[i] != 0)
			{
			readSubClassSet(offset + fmt->SubClassSet[i], &fmt->_SubClassSet[i]);
			}
		}
	return fmt;
	}

static void *readContext3(Card32 offset)
	{
	IntX i;
	ContextSubstFormat3 *fmt = memNew(sizeof(ContextSubstFormat3));

	fmt->SubstFormat = 3;
	IN1(fmt->GlyphCount);
	IN1(fmt->SubstCount);
	fmt->CoverageArray = memNew(sizeof(Offset) * (fmt->GlyphCount + 1));
	fmt->_CoverageArray = memNew(sizeof(void *) * (fmt->GlyphCount + 1));
	fmt->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * fmt->SubstCount);
	for (i = 0; i < fmt->GlyphCount; i++)
	  {
		IN1(fmt->CoverageArray[i]);
		fmt->_CoverageArray[i] = ttoReadCoverage(offset + fmt->CoverageArray[i]);
	  }
	for (i = 0; i < fmt->SubstCount; i++) 
	  {
		IN1(fmt->SubstLookupRecord[i].SequenceIndex);
		IN1(fmt->SubstLookupRecord[i].LookupListIndex);
	  }
	return fmt;
	}

static void *readContext(Card32 offset)
	{
	  Card16 format;
	  
	  IN1(format);
	  switch (format)
	    {
	    case 1:
	      return readContext1(offset);
	    case 2:
	      return readContext2(offset);
	    case 3:
	      return readContext3(offset);
	    default:
		  warning(SPOT_MSG_GSUBUNKCTX, format, offset);
		return NULL;
		}
	}


static void readChainSubRule(Card32 offset, ChainSubRule *subrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);

	IN1(subrule->BacktrackGlyphCount);
	subrule->Backtrack = memNew(sizeof(GlyphId) * subrule->BacktrackGlyphCount);
	for (i = 0; i < subrule->BacktrackGlyphCount; i++) 
	  {
		IN1(subrule->Backtrack[i]);
	  }	

	IN1(subrule->InputGlyphCount);
	subrule->Input = memNew(sizeof(GlyphId) * subrule->InputGlyphCount);
	for (i = 1; i < subrule->InputGlyphCount; i++) 
	  {
		IN1(subrule->Input[i]);
	  }	

	IN1(subrule->LookaheadGlyphCount);
	subrule->Lookahead = memNew(sizeof(GlyphId) * subrule->LookaheadGlyphCount);
	for (i = 0; i < subrule->LookaheadGlyphCount; i++) 
	  {
		IN1(subrule->Lookahead[i]);
	  }	

	IN1(subrule->SubstCount);
	subrule->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * subrule->SubstCount);
	for (i = 0; i < subrule->SubstCount; i++) 
	  {
		IN1(subrule->SubstLookupRecord[i].SequenceIndex);
		IN1(subrule->SubstLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readChainSubRuleSet(Card32 offset, ChainSubRuleSet *subruleset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subruleset->ChainSubRuleCount);
	subruleset->ChainSubRule = memNew(sizeof(Offset) * subruleset->ChainSubRuleCount);
	subruleset->_ChainSubRule = memNew(sizeof(ChainSubRule) * subruleset->ChainSubRuleCount);
	for (i = 0; i < subruleset->ChainSubRuleCount; i++) 
	  {
		IN1(subruleset->ChainSubRule[i]);
		readChainSubRule(offset + subruleset->ChainSubRule[i], &subruleset->_ChainSubRule[i]);
	  }
	SEEK_ABS(save);
	}

static void *readChainContext1(Card32 offset)
	{
	IntX i;
	ChainContextSubstFormat1 *fmt = memNew(sizeof(ChainContextSubstFormat1));

	fmt->SubstFormat = 1;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->ChainSubRuleSetCount);
	fmt->ChainSubRuleSet = memNew(sizeof(Offset) * fmt->ChainSubRuleSetCount);
	fmt->_ChainSubRuleSet = memNew(sizeof(ChainSubRuleSet) * fmt->ChainSubRuleSetCount);

	for (i = 0; i < fmt->ChainSubRuleSetCount; i++) 
		{
		IN1(fmt->ChainSubRuleSet[i]);
		readChainSubRuleSet(offset + fmt->ChainSubRuleSet[i], &fmt->_ChainSubRuleSet[i]);
		}
	return fmt;
	}


static void readChainSubClassRule(Card32 offset, ChainSubClassRule *subclassrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subclassrule->BacktrackGlyphCount);
	subclassrule->Backtrack = memNew(sizeof(Card16) * subclassrule->BacktrackGlyphCount);
	for (i = 0; i < subclassrule->BacktrackGlyphCount; i++) 
	  {
		IN1(subclassrule->Backtrack[i]);
	  }

	IN1(subclassrule->InputGlyphCount);
	subclassrule->Input = memNew(sizeof(Card16) * subclassrule->InputGlyphCount);
	for (i = 1; i < subclassrule->InputGlyphCount; i++) 
	  {
		IN1(subclassrule->Input[i]);
	  }

	IN1(subclassrule->LookaheadGlyphCount);
	subclassrule->Lookahead = memNew(sizeof(Card16) * subclassrule->LookaheadGlyphCount);
	for (i = 0; i < subclassrule->LookaheadGlyphCount; i++) 
	  {
		IN1(subclassrule->Lookahead[i]);
	  }

	IN1(subclassrule->SubstCount);
	subclassrule->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * subclassrule->SubstCount);

	for (i = 0; i < subclassrule->SubstCount; i++) 
	  {
		IN1(subclassrule->SubstLookupRecord[i].SequenceIndex);
		IN1(subclassrule->SubstLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readChainSubClassSet(Card32 offset, ChainSubClassSet *subclassset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(subclassset->ChainSubClassRuleCnt);
	subclassset->ChainSubClassRule = memNew(sizeof(Offset) * subclassset->ChainSubClassRuleCnt);
	subclassset->_ChainSubClassRule = memNew(sizeof(ChainSubRule) * subclassset->ChainSubClassRuleCnt);
	for (i = 0; i < subclassset->ChainSubClassRuleCnt; i++) 
		{
		IN1(subclassset->ChainSubClassRule[i]);
		readChainSubClassRule(offset + subclassset->ChainSubClassRule[i], &subclassset->_ChainSubClassRule[i]);
		}
	SEEK_ABS(save);
	}

static void *readChainContext2(Card32 offset)
	{
	IntX i;
	ChainContextSubstFormat2 *fmt = memNew(sizeof(ChainContextSubstFormat2));

	fmt->SubstFormat = 2;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->BackTrackClassDef);
	if (fmt->BackTrackClassDef != 0)
		fmt->_BackTrackClassDef = ttoReadClass(offset + fmt->BackTrackClassDef);
	else
		fmt->_BackTrackClassDef = NULL;
	IN1(fmt->InputClassDef);
	fmt->_InputClassDef = ttoReadClass(offset + fmt->InputClassDef);
	IN1(fmt->LookAheadClassDef);
	if (fmt->LookAheadClassDef != 0)
		fmt->_LookAheadClassDef = ttoReadClass(offset + fmt->LookAheadClassDef);
	else
		fmt->_LookAheadClassDef = NULL;
	IN1(fmt->ChainSubClassSetCnt);
	fmt->ChainSubClassSet = memNew(sizeof(Offset) * fmt->ChainSubClassSetCnt);
	fmt->_ChainSubClassSet = memNew(sizeof(ChainSubRuleSet) * fmt->ChainSubClassSetCnt);

	for (i = 0; i < fmt->ChainSubClassSetCnt; i++) 
		{
		IN1(fmt->ChainSubClassSet[i]);
		if (fmt->ChainSubClassSet[i] == 0)
			{
			fmt->_ChainSubClassSet[i].ChainSubClassRuleCnt = 0;
			continue;
			}
		readChainSubClassSet(offset + fmt->ChainSubClassSet[i], &fmt->_ChainSubClassSet[i]);
		}
	return fmt;
	}

static void *readChainContext3(Card32 offset)
	{
	IntX i;
	ChainContextSubstFormat3 *fmt = memNew(sizeof(ChainContextSubstFormat3));

	fmt->SubstFormat = 3;

	IN1(fmt->BacktrackGlyphCount);
	if (fmt->BacktrackGlyphCount > 0)
		{
		fmt->Backtrack = memNew(sizeof(Offset) * (fmt->BacktrackGlyphCount + 1));
		fmt->_Backtrack = memNew(sizeof(void *) * (fmt->BacktrackGlyphCount + 1));
		}
	else
		{
		fmt->Backtrack = NULL;
		fmt->_Backtrack = NULL;
		}
	
	for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		{
		IN1(fmt->Backtrack[i]);
		fmt->_Backtrack[i] = ttoReadCoverage(offset + fmt->Backtrack[i]);
		}

	IN1(fmt->InputGlyphCount);
	fmt->Input = memNew(sizeof(Offset) * (fmt->InputGlyphCount + 1));
	fmt->_Input = memNew(sizeof(void *) * (fmt->InputGlyphCount + 1));
	for (i = 0; i < fmt->InputGlyphCount; i++)
	  {
		IN1(fmt->Input[i]);
		fmt->_Input[i] = ttoReadCoverage(offset + fmt->Input[i]);
	  }
	IN1(fmt->LookaheadGlyphCount);
	if (fmt->LookaheadGlyphCount > 0)
		{
		fmt->Lookahead = memNew(sizeof(Offset) * (fmt->LookaheadGlyphCount + 1));
		fmt->_Lookahead = memNew(sizeof(void *) * (fmt->LookaheadGlyphCount + 1));
		}
	else
		{
		fmt->_Lookahead = NULL;
		fmt->Lookahead = NULL;
		}
	for (i = 0; i < fmt->LookaheadGlyphCount; i++)
	  {
		IN1(fmt->Lookahead[i]);
		fmt->_Lookahead[i] = ttoReadCoverage(offset + fmt->Lookahead[i]);
	  }

	IN1(fmt->SubstCount);
	fmt->SubstLookupRecord = memNew(sizeof(SubstLookupRecord) * fmt->SubstCount);
	for (i = 0; i < fmt->SubstCount; i++) 
	  {
		IN1(fmt->SubstLookupRecord[i].SequenceIndex);
		IN1(fmt->SubstLookupRecord[i].LookupListIndex);
	  }

	return fmt;
	}

static void *readChainContext(Card32 offset)
	{
	  Card16 format;
	  
	  IN1(format);
	  switch (format)
	    {
	    case 1:
	      return readChainContext1(offset); 
	    case 2:
	      return readChainContext2(offset); 
	    case 3:
	      return readChainContext3(offset);
	    default:
		  warning(SPOT_MSG_GSUBUNKCCTX, format, offset);
		return NULL;
		}
	}

static void *readReverseChain1(Card32 offset)
	{
	IntX i;
	ReverseChainContextSubstFormat1 *fmt = memNew(sizeof(ReverseChainContextSubstFormat1));

	fmt->SubstFormat = 1;

	IN1(fmt->InputCoverage);
	fmt->_InputCoverage = ttoReadCoverage(offset + fmt->InputCoverage);

	IN1(fmt->BacktrackGlyphCount);
	if (fmt->BacktrackGlyphCount > 0)
		{
		fmt->Backtrack = memNew(sizeof(Offset) * (fmt->BacktrackGlyphCount + 1));
		fmt->_Backtrack = memNew(sizeof(void *) * (fmt->BacktrackGlyphCount + 1));
		}
	else
		{
		fmt->Backtrack = NULL;
		fmt->_Backtrack = NULL;
		}
	
	for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		{
		IN1(fmt->Backtrack[i]);
		fmt->_Backtrack[i] = ttoReadCoverage(offset + fmt->Backtrack[i]);
		}

	IN1(fmt->LookaheadGlyphCount);
	if (fmt->LookaheadGlyphCount > 0)
		{
		fmt->Lookahead = memNew(sizeof(Offset) * (fmt->LookaheadGlyphCount + 1));
		fmt->_Lookahead = memNew(sizeof(void *) * (fmt->LookaheadGlyphCount + 1));
		}
	else
		{
		fmt->_Lookahead = NULL;
		fmt->Lookahead = NULL;
		}
	for (i = 0; i < fmt->LookaheadGlyphCount; i++)
	  {
		IN1(fmt->Lookahead[i]);
		fmt->_Lookahead[i] = ttoReadCoverage(offset + fmt->Lookahead[i]);
	  }

	IN1(fmt->GlyphCount);
	fmt->Substitutions = memNew(sizeof(GlyphId) * fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++) 
	  {
		IN1(fmt->Substitutions[i]);
	  }

	return fmt;
	}

static void *readReverseChain(Card32 offset)
	{
	  Card16 format;
	  
	  IN1(format);
	  switch (format)
	    {
	    case 1:
	      return readReverseChain1(offset); 
	    default:
		  warning(SPOT_MSG_GSUBUNKCCTX, format, offset);
		return NULL;
		}
	}


static void *readSubtable(Card32 offset, Card16 type)
	{
	void *result;
	Card32 save = TELL();

	SEEK_ABS(offset);
	switch (type)
		{
	case SingleSubsType:
		result = readSingle(offset);
		break;
	case MultipleSubsType:
		result = readMultiple(offset);
		break;
	case AlternateSubsType:
		result = readAlternate(offset);
		break;
	case LigatureSubsType:
		result = readLigature(offset);
		break;
	case ContextSubsType:
		result = readContext(offset);
		break;
	case ChainingContextSubsType:
		result = readChainContext(offset);
		break;
	case OverflowSubsType:
		result = readOverflow(offset);
		break;
	case ReverseChainContextType:
		result = readReverseChain(offset);
		break;
	default:
		warning(SPOT_MSG_GSUBUNKRLOOK, type, type,offset);
		result = NULL;
		}

	SEEK_ABS(save);
	return result;
	}

void GSUBRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	SEEK_ABS(start);
	GSUBStart = start;
	IN1(GSUB.Version);
	IN1(GSUB.ScriptList);
	IN1(GSUB.FeatureList);
	IN1(GSUB.LookupList);

	if ( (GSUB.ScriptList == 0) || (GSUB.FeatureList == 0)) /* empty GSUB table */
		{
		return;
		}
		
	ttoReadScriptList(start + GSUB.ScriptList, &GSUB._ScriptList);
	ttoReadFeatureList(start + GSUB.FeatureList, &GSUB._FeatureList);
	ttoReadLookupList(start + GSUB.LookupList, &GSUB._LookupList,
					  readSubtable);
	loaded = 1;
	/*featuretag = 0;*/
	prev_featuretag = 0;
	}
	

static Byte8 *dumpTitle(Card32 tag, IntX flavor)
	{
	  static Byte8 othertag[80];

	  if (tag == 0) 
		{
		  return ("GSUB table features ");
		}
	  else if (tag == (STR2TAG("aalt"))) return ("aalt (Access all alternates)");
	  else if (tag == (STR2TAG("afrc"))) return ("afrc (Alternative fractions)");
	  else if (tag == (STR2TAG("c2sc"))) return ("c2sc (Small Caps substitution (Majuscules))");
	  else if (tag == (STR2TAG("calt"))) return ("calt (Connection alternates)");
	  else if (tag == (STR2TAG("case"))) return ("case (Case-sensitive substitution)");
	  else if (tag == (STR2TAG("clig"))) return ("clig (Contextual ligature substitution)");
	  else if (tag == (STR2TAG("crcy"))) return ("crcy (Currency substitution)");
	  else if (tag == (STR2TAG("cswh"))) return ("cswh (Contextual swash substitution)");
	  else if (tag == (STR2TAG("DCMP"))) return ("DCMP (Ligature decomposition)");
	  else if (tag == (STR2TAG("dlig"))) return ("dlig (Discretionary ligature substitution)");
	  else if (tag == (STR2TAG("dnom"))) return ("dnom (Denominators)");
	  else if (tag == (STR2TAG("dpng"))) return ("dpng (Dipthong substitution)");
	  else if (tag == (STR2TAG("expt"))) return ("expt (Expert-forms substitution)");
	  else if (tag == (STR2TAG("fina"))) return ("fina (Terminal alternates)");
	  else if (tag == (STR2TAG("frac"))) {
		if (flavor == 4)
		  return ("frac (Pre-built Fraction substitution)");
		else if (flavor == 5)
		  return ("frac (Arbitrary Fraction substitution)");
		else
		  return ("frac (Fraction substitution)");
	  }
	  else if (tag == (STR2TAG("fwid"))) return ("fwid (Full-width substitution)");
	  else if (tag == (STR2TAG("hist"))) return ("hist (Historical alternates)");
	  else if (tag == (STR2TAG("hkna"))) return ("hkna (Horizontal Kana Alternates)");
	  else if (tag == (STR2TAG("hlig"))) return ("hlig (Historical ligatures)");
	  else if (tag == (STR2TAG("hngl"))) return ("hngl (Hanja to Hangul substitution)");
	  else if (tag == (STR2TAG("hwid"))) return ("hwid (Half-width substitution)");
	  else if (tag == (STR2TAG("init"))) return ("init (Initial alternates)");
	  else if (tag == (STR2TAG("ital"))) return ("ital (Italics alternates)");
	  else if (tag == (STR2TAG("jajp"))) return ("jajp (Japanese forms)");
	  else if (tag == (STR2TAG("jp78"))) return ("jp78 (JIS78-forms substitution)");
	  else if (tag == (STR2TAG("jp83"))) return ("jp83 (JIS83-forms substitution)");
	  else if (tag == (STR2TAG("jp90"))) return ("jp90 (JIS90-forms substitution)");
	  else if (tag == (STR2TAG("kokr"))) return ("kokr (Korean forms)");
	  else if (tag == (STR2TAG("liga"))) return ("liga (Ligature substitution)");
	  else if (tag == (STR2TAG("lnum"))) return ("lnum (Lining figures)");
	  else if (tag == (STR2TAG("medi"))) return ("medi (Medial alternates)");
	  else if (tag == (STR2TAG("mgrk"))) return ("mgrk (Mathematical Greek)");
	  else if (tag == (STR2TAG("mset"))) return ("mset (Mark positioning substitution)");
	  else if (tag == (STR2TAG("nalt"))) return ("nalt (Alternate annotation forms)");
	  else if (tag == (STR2TAG("nlck"))) return ("nlck (National Language Committee kanji)");
	  else if (tag == (STR2TAG("numr"))) return ("numr (Numerators)");
	  else if (tag == (STR2TAG("nutf"))) return ("nutf ('Nut' fractions)");
	  else if (tag == (STR2TAG("onum"))) return ("onum (Oldstyle figures)");
	  else if (tag == (STR2TAG("ordn"))) return ("ordn (Ordinals substitution)");
	  else if (tag == (STR2TAG("ornm"))) return ("ornm (Ornaments)");
	  else if (tag == (STR2TAG("pnum"))) return ("pnum (Proportional figures)");
	  else if (tag == (STR2TAG("pwid"))) return ("pwid (Proportional-width substitution)");
	  else if (tag == (STR2TAG("qwid"))) return ("qwid (Quarter-width substitution)");
	  else if (tag == (STR2TAG("rand"))) return ("rand (Random-selection alternates)");
	  else if (tag == (STR2TAG("ruby"))) return ("ruby (Ruby Notation Forms)");
	  else if (tag == (STR2TAG("salt"))) return ("salt (Stylistic alternates)");
	  else if (tag == (STR2TAG("SALT"))) return ("SALT (Stylistic alternates)");
	  else if (tag == (STR2TAG("sinf"))) return ("sinf (Scientific subscript/inferiors)");
	  else if (tag == (STR2TAG("smcp"))) return ("smcp (Small Caps substitution (Minuscules))");
	  else if (tag == (STR2TAG("smpl"))) return ("smpl (Simplified-forms substitution)");
	  else if (tag == (STR2TAG("subs"))) return ("subs (Subscript substitution)");
	  else if (tag == (STR2TAG("sups"))) return ("sups (Superscript substitution)");
	  else if (tag == (STR2TAG("swsh"))) return ("swsh (Swash alternates)");
	  else if (tag == (STR2TAG("titl"))) return ("titl (Titling alternates)");
	  else if (tag == (STR2TAG("tnam"))) return ("tnam (Traditional name forms)");
	  else if (tag == (STR2TAG("tnum"))) return ("tnum (Tabular figures)");
	  else if (tag == (STR2TAG("trad"))) return ("trad (Traditional-forms substitution)");
	  else if (tag == (STR2TAG("twid"))) return ("twid (Third-width substitution)");
	  else if (tag == (STR2TAG("vert"))) return ("vert (Vertical-forms substitution)");
	  else if (tag == (STR2TAG("vivn"))) return ("vivn (Vietnamese Forms)");
	  else if (tag == (STR2TAG("vrt2"))) return ("vrt2 (Rotated vertical-forms substitution)");
	  else if (tag == (STR2TAG("zero"))) return ("zero (Slashed-Zero substitution)");
	  else if (tag == (STR2TAG("zhcn"))) return ("zhcn (Simplified Chinese Forms)");
	  else if (tag == (STR2TAG("zhtw"))) return ("zhtw (Traditional Chinese Forms)");
	  else if (tag == (STR2TAG("akhn"))) return ("akhn (Akhands)");
	else if (tag == (STR2TAG("blwf"))) return ("blwf (Below-base Forms)");
	else if (tag == (STR2TAG("half"))) return ("half ( Half Forms)");
	else if (tag == (STR2TAG("vatu"))) return ("vatu (Vattu Variants)");
	else if (tag == (STR2TAG("vkna"))) return ("vkna (Vertical Kana Alternates)");
	else if (tag == (STR2TAG("pstf"))) return ("pstf (Post-base Forms)");
	else if (tag == (STR2TAG("nukt"))) return ("nukt (Nukta Forms)");
	else if (tag == (STR2TAG("abvs"))) return ("abvs (Above-base Substitutions)");
	else if (tag == (STR2TAG("blws"))) return ("blws (Below-base Substitutions)");
	else if (tag == (STR2TAG("falt"))) return ("falt (Final Glyph on Line Alternates)");
	else if (tag == (STR2TAG("haln"))) return ("haln (Halant Forms)");
	else if (tag == (STR2TAG("isol"))) return ("isol (Isolated Forms)");
	else if (tag == (STR2TAG("jalt"))) return ("jalt (Justification Alternates)");
	else if (tag == (STR2TAG("locl"))) return ("locl (Localized Forms)");
	else if (tag == (STR2TAG("pres"))) return ("pres (Pre-base Substitutions)");
	else if (tag == (STR2TAG("psts"))) return ("psts (Post-base Substitutions)");
	else if (tag == (STR2TAG("rphf"))) return ("rphf (Reph Forms)");

	  else
		{
		  sprintf(othertag,"'%c%c%c%c' (Unknown/Unregistered tag)", TAG_ARG(tag));
		  return(othertag);
		}
	}

static IntX isVerticalFeature(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("vert"))) return 1;
	  else if (tag == (STR2TAG("vrt2"))) return 1;
	  return 0;
	}


#if 0
static IntX isAlternatesFormat(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("CALT"))) return 1;
	  else if (tag == (STR2TAG("fina"))) return 1;
	  else if (tag == (STR2TAG("HIST"))) return 1;
	  else if (tag == (STR2TAG("init"))) return 1;
	  else if (tag == (STR2TAG("medi"))) return 1;
	  else if (tag == (STR2TAG("NALT"))) return 1;
	  else if (tag == (STR2TAG("ORNM"))) return 1;
	  else if (tag == (STR2TAG("RAND"))) return 1;
	  else if (tag == (STR2TAG("SALT"))) return 1;
	  else if (tag == (STR2TAG("swsh"))) return 1;
	  else if (tag == (STR2TAG("TITL"))) return 1;
	  return 0;
	}

static IntX isOneToOneFormat(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("C2SC"))) return 1;
	  else if (tag == (STR2TAG("EXPT"))) return 1;
	  else if (tag == (STR2TAG("FWID"))) return 1;
	  else if (tag == (STR2TAG("HWID"))) return 1;
	  else if (tag == (STR2TAG("JP78"))) return 1;
	  else if (tag == (STR2TAG("JP83"))) return 1;
	  else if (tag == (STR2TAG("LNUM"))) return 1;
	  else if (tag == (STR2TAG("onum"))) return 1;
	  else if (tag == (STR2TAG("ORDN"))) return 1;
	  else if (tag == (STR2TAG("pnum"))) return 1;
	  else if (tag == (STR2TAG("PWID"))) return 1;
	  else if (tag == (STR2TAG("smcp"))) return 1;
	  else if (tag == (STR2TAG("SMPL"))) return 1;
	  else if (tag == (STR2TAG("sups"))) return 1;
	  else if (tag == (STR2TAG("subs"))) return 1;
	  else if (tag == (STR2TAG("TNUM"))) return 1;
	  else if (tag == (STR2TAG("TRAD"))) return 1;
	  else if (tag == (STR2TAG("vert"))) return 1;
	  else if (tag == (STR2TAG("ZERO"))) return 1;
	  return 0;
	}

static IntX isOnetoManyFormat(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("DCMP"))) return 1;
	  return 0;
	}

static IntX isManyToOneFormat(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("dlig"))) return 1;
	  else if (tag == (STR2TAG("DPNG"))) return 1;
	  else if (tag == (STR2TAG("frac"))) {
		if (flavor == 4)
		  return 1;
		else if (flavor == 5)
		  return 1;
		else
		  return 1;
	  }
	  else if (tag == (STR2TAG("liga"))) return 1;
	  return 0;
	}
#endif

static void dumpMessage(char * str, Tag feature)
{
	if(isVerticalFeature(feature, 0))
	{
		proofSetVerticalMode();
		proofMessage(proofctx, str);
		proofUnSetVerticalMode();
	}else
		proofMessage(proofctx, str);
}

static void proofSingle1(SingleSubstFormat1 *fmt)
   {
	ttoEnumRec CovList;
	Card32 nitems;
	Card32 i;
	IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
	IntX isVert = proofIsVerticalMode();

	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	for (i = 0; i < nitems; i++)
	  {
		char name[MAX_NAME_LEN];
		char name2[MAX_NAME_LEN];
		IntX glyphId2;
		GlyphId glyphId = *da_INDEX(CovList.glyphidlist, (Int32)i);
		strcpy(name, getGlyphName(glyphId, 1));
		getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		if (isVert)
		  proofCheckAdvance(proofctx, 1000 + 2*(abs(vwidth)));
		else
		  proofCheckAdvance(proofctx, 1000 + 2*width);

		if (isVert)
		  {
			if (vwidth != 0) W = vwidth;
			else  W = width;
		  }
		else
			W = width;

		proofDrawGlyph(proofctx, 
					   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
					   name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
					   NULL, 0, /* altlabel,altlabelflags */
					   0,0, /* originDx,originDy */
					   0,0, /* origin, originflags */
					   W, 0, /* width,widthflags */
					   NULL, yorig, ""
					   );
		
		proofSymbol(proofctx, PROOF_YIELDS);
		glyphId2 = glyphId + fmt->DeltaGlyphId;
		strcpy(name2, getGlyphName(glyphId2, 1));
		getMetrics(glyphId2, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		if (isVert)
		  {
			if (vwidth != 0) W = vwidth;
			else  W = width;
		  }
		else
			W = width;

		proofDrawGlyph(proofctx, 
					   glyphId2, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
					   name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
					   NULL,0,
					   0,0, /* originDx,originDy */
					   0,0, /* origin, originflags */
					   W, 0, /* width,widthflags */
					   NULL, yorig, ""
					   );
		  proofThinspace(proofctx, 4);
	  }
	if (CovList.glyphidlist.size > 0)
	  da_FREE(CovList.glyphidlist);
	}

static void dumpSingle1(SingleSubstFormat1 *fmt, IntX level)
	{
	  if (level == 8)
	  	proofSingle1(fmt);
	  else if (level==7){
	  	ttoEnumRec CovList;
	  	Card32 nitems;
	  	Int32 i;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	    for (i = 0; i < (Int32)nitems; i++) 
		{
		  char name[MAX_NAME_LEN];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

		  strcpy(name, getGlyphName(glyphId, 0));
		  fprintf(OUTPUTBUFF,  "%s sub %s ", contextPrefix, name);
		  
		  glyphId = *da_INDEX(CovList.glyphidlist, i)+fmt->DeltaGlyphId;
		  strcpy(name, getGlyphName(glyphId, 0));
		  fprintf(OUTPUTBUFF,  "by %s", name);

		  fprintf(OUTPUTBUFF,  ";\n");
		 }
		 
	   if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  }
	  else
		{
		  DLu(2, "SubstFormat =", fmt->SubstFormat);
		  DLx(2, "Coverage    =", fmt->Coverage);
		  DLs(2, "DeltaGlyphId=", fmt->DeltaGlyphId);
		  
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		}
	}
	
static void proofSingle2(SingleSubstFormat2 *fmt)
	{
		ttoEnumRec CovList;
		Card32 nitems;
		Int32 i;
		IntX isVert = proofIsVerticalMode();
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);

		for (i = 0; i < (Int32)nitems; i++)
		  {
			char name[MAX_NAME_LEN];
			char name2[MAX_NAME_LEN];
			IntX glyphId2;
			IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			strcpy(name, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			if (isVert)
			  proofCheckAdvance(proofctx, 1000 + 2*(abs(vwidth)));
			else
			  proofCheckAdvance(proofctx, 1000 + 2*width);
			if (isVert)
			  {
				if (vwidth != 0) W = vwidth;
				else  W = width;
			  }
			else
			  W = width;

			proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   W, 0, /* width,widthflags */
						   NULL, yorig, ""
						   );
			
			proofSymbol(proofctx, PROOF_YIELDS);
			glyphId2 = fmt->Substitute[i];
			strcpy(name2, getGlyphName(glyphId2, 1));
			getMetrics(glyphId2, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			if (isVert)
			  {
				if (vwidth != 0) W = vwidth;
				else  W = width;
			  }
			else
			  W = width;

			proofDrawGlyph(proofctx, 
						   glyphId2, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL,0,
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   W, 0, /* width,widthflags */
						   NULL, yorig, ""
						   );
			  proofThinspace(proofctx, 4);
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);
  	
	}

static void dumpSingle2(SingleSubstFormat2 *fmt, IntX level)
	{
	IntX i;

	if (level == 8)
		proofSingle2(fmt);
	else if (level==7){
	  	ttoEnumRec CovList;
	  	Card32 nitems;
	  	IntX i;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	    for (i = 0; i < (Int32)nitems; i++) 
		{
		  char name[MAX_NAME_LEN];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

		  strcpy(name, getGlyphName(glyphId, 0));
		  fprintf(OUTPUTBUFF,  "%s sub %s ", contextPrefix, name);
		  
		  glyphId = fmt->Substitute[i];
		  strcpy(name, getGlyphName(glyphId, 0));
		  fprintf(OUTPUTBUFF,  "by %s", name);

		  fprintf(OUTPUTBUFF,  ";\n");
		 }
		 
	   if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  }
	else
	  {
		DLu(2, "SubstFormat=", fmt->SubstFormat);
		DLx(2, "Coverage   =", fmt->Coverage);
		DLs(2, "GlyphCount =", fmt->GlyphCount);
		
		if (level < 4)
			DL(2, (OUTPUTBUFF, "--- Substitute[index]=glyphId\n"));
		else
			DL(3, (OUTPUTBUFF, "--- Substitute[index]=glyphId glyphName/CID\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
		  {
			if (level < 4) 
			  DL(2, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->Substitute[i]));
			else
			  DL(3, (OUTPUTBUFF, "[%d]=%hu (%s) ", i, fmt->Substitute[i], getGlyphName(fmt->Substitute[i], 0)));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
  }

static void dumpSingle(void *fmt, IntX level)
	{
	DL(2, (OUTPUTBUFF, "--- SingleSubst\n"));

	switch (((SingleSubstFormat1 *)fmt)->SubstFormat)
		{
	case 1:
		dumpSingle1(fmt, level);
		break;
	case 2:
		dumpSingle2(fmt, level);
		break;
		}
	}

static IntX SingleSubstFlavor(void *fmt)
	{
	  return (((SingleSubstFormat1 *)fmt)->SubstFormat);
	}

static void dumpOverflow1(OverflowSubstFormat1 *fmt, IntX level, void *feattag)
	{
	  DL(2, (OUTPUTBUFF, "SubstFormat = 1\n"));
	  DL(2, (OUTPUTBUFF, "LookupType  = %d\n", fmt->OverflowLookupType));
	  DL(2, (OUTPUTBUFF, "Offset      = %08x\n", fmt->OverflowOffset));
	  
	  dumpSubtable(fmt->OverflowOffset, fmt->OverflowLookupType, fmt->subtable, 
						 level, feattag, GSUBLookupIndex, GSUBSubtableindex, GSUBSubtableCnt, 1);
	}
	
static void dumpOverflow(void *fmt, IntX level, void *feattag)
	{
	DL(2, (OUTPUTBUFF, "--- ExtensionSubst\n"));

	switch (((OverflowSubstFormat1 *)fmt)->SubstFormat)
		{
	case 1:
		dumpOverflow1(fmt, level, feattag); 
		break;
		}
	}

static IntX OverflowSubstFlavor(void *fmt)
	{
	  return (((OverflowSubstFormat1 *)fmt)->SubstFormat);
	}

static void proofMultiple1(MultipleSubstFormat1 *fmt)
	{
		ttoEnumRec CovList;
		Card32 nitems;
		IntX i;
		IntX isVert = proofIsVerticalMode();
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);

		for (i = 0; i < (Int32)nitems; i++)
		  {
			char name[MAX_NAME_LEN];
			char name2[MAX_NAME_LEN];
			IntX glyphId2;
			IntX j;
			Sequence *sequence;
			IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			strcpy(name, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			if (isVert)
			  {
				if (vwidth != 0) W = vwidth;
				else  W = width;
			  }
			else
			  W = width;
			proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   W, 0, /* width,widthflags */
						   NULL, yorig, ""
						   );
			proofSymbol(proofctx, PROOF_YIELDS);
			sequence = &fmt->_Sequence[i];
			for (j = 0; j < sequence->GlyphCount; j++)
			  {
				glyphId2 = sequence->Substitute[j];
				strcpy(name2, getGlyphName(glyphId2, 1));
				getMetrics(glyphId2, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;

				proofDrawGlyph(proofctx, 
							   glyphId2, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );					
				if ((j+1) < sequence->GlyphCount)
				  proofSymbol(proofctx, PROOF_PLUS);
			  }
			proofNewline(proofctx);
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);  	
	}
	
static void dumpMultiple1(MultipleSubstFormat1 *fmt, IntX level)
	{
	IntX i;
	if (level == 8)
		proofMultiple1(fmt);
	else if (level ==7)
	  {
	  	ttoEnumRec CovList;
	  	Int32 nitems;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, (Card32*)&nitems);
		for (i = 0; i < nitems; i++) 
		{
		  	IntX j;
		  	Sequence *sequence = &fmt->_Sequence[i];
		  	GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			fprintf(OUTPUTBUFF, "sub %s by", getGlyphName(glyphId, 0));
			
		  	for (j = 0; j < sequence->GlyphCount; j++)
		 	 {
				fprintf(OUTPUTBUFF, " %s", getGlyphName(sequence->Substitute[j], 0));
			  }
		   	fprintf(OUTPUTBUFF, ";\n");
		  }
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
	  }
	else
	  {
		DLu(2, "SubstFormat  =", fmt->SubstFormat);
		DLx(2, "Coverage     =", fmt->Coverage);
		DLu(2, "SequenceCount=", fmt->SequenceCount);
		
		DL(2, (OUTPUTBUFF, "--- Sequence[index]=offset\n"));
		for (i = 0; i < fmt->SequenceCount; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Sequence[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->SequenceCount; i++)
		  {
			IntX j;
			Sequence *sequence = &fmt->_Sequence[i];
			
			DL(2, (OUTPUTBUFF, "--- Sequence (%04hx)\n", fmt->Sequence[i]));
			DLu(2, "GlyphCount=", sequence->GlyphCount);
			
			if (level < 4) 
			  DL(2, (OUTPUTBUFF, "--- Substitute[index]=glyphId\n"));
			else
			  DL(2, (OUTPUTBUFF, "--- Substitute[index]=glyphId glyphName/CID\n"));
			for (j = 0; j < sequence->GlyphCount; j++)
			  {
				if (level < 4) 
				  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, sequence->Substitute[j]));
				else 
				  DL(4, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, sequence->Substitute[j], getGlyphName(sequence->Substitute[j], 0)));
			  }
			DL(2, (OUTPUTBUFF, "\n"));
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
	}

static void dumpMultiple(void *fmt, IntX level)
	{
	DL(2, (OUTPUTBUFF, "--- MultipleSubst\n"));

	switch (((MultipleSubstFormat1 *)fmt)->SubstFormat)
		{
	case 1:
		dumpMultiple1(fmt, level);
		break;
		}
	}

static IntX MultipleSubstFlavor(void *fmt)
	{
	  return (((MultipleSubstFormat1 *)fmt)->SubstFormat);
	}

static void proofAlternate1(AlternateSubstFormat1 *fmt)
	{
		ttoEnumRec CovList;
		Card32 nitems;
		IntX i, j;
		IntX isVert = proofIsVerticalMode();
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);

		for (i = 0; i < (IntX)nitems; i++)
		  {
			char name[MAX_NAME_LEN];
			char name2[MAX_NAME_LEN];
			IntX glyphId2;
			AlternateSet *altset;
			IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			strcpy(name, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			if (isVert)
			  {
				if (vwidth != 0) W = vwidth;
				else  W = width;
			  }
			else
			  W = width;
			proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   W, 0, /* width,widthflags */
						   NULL, yorig, ""
						   );
			
			proofThinspace(proofctx, 2);
			proofSymbol(proofctx, PROOF_COLON);
			altset = &fmt->_AlternateSet[i];
			for (j = 0; j < altset->GlyphCount; j++) 
			  {
				glyphId2 = altset->Alternate[j];
				strcpy(name2, getGlyphName(glyphId2, 1));
				getMetrics(glyphId2, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				proofThinspace(proofctx, 1);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   glyphId2, ANNOT_SHOWIT|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name2, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								: 
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),   /* glyphname,glyphnameflags */
							   NULL,0,
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
			  }
			proofNewline(proofctx);
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);
	}
	
static void dumpAlternate1(AlternateSubstFormat1 *fmt, IntX level)
	{
	IntX i,j;
	if (level == 8)
		proofAlternate1(fmt);
	else if (level ==7)
	  {
	  	ttoEnumRec CovList;
	  	Int32 nitems;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, (Card32*)&nitems);
		for (i = 0; i < (Int32)nitems; i++) 
		{
		  	AlternateSet *altset = &fmt->_AlternateSet[i];
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			fprintf(OUTPUTBUFF, "sub %s from [", getGlyphName(glyphId, 0));

			for (j = 0; j < altset->GlyphCount; j++)
			  {
				 fprintf(OUTPUTBUFF, " %s", getGlyphName(altset->Alternate[j], 0));
			  }
			fprintf(OUTPUTBUFF, "];\n");
		  }
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
	  }
	  else
	  {
		DLu(2, "SubstFormat  =", fmt->SubstFormat);
		DLx(2, "Coverage     =", fmt->Coverage);
		DLu(2, "AlternateSetCount=", fmt->AlternateSetCnt);
		
		DL(2, (OUTPUTBUFF, "--- AlternateSet[index]=offset\n"));
		for (i = 0; i < fmt->AlternateSetCnt; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->AlternateSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->AlternateSetCnt; i++)
		  {
			AlternateSet *altset = &fmt->_AlternateSet[i];
			
			DL(2, (OUTPUTBUFF, "--- AlternateSet (%04hx)\n", fmt->AlternateSet[i]));
			DLu(2, "GlyphCount=", altset->GlyphCount);
			if (level < 4)
			  DL(2, (OUTPUTBUFF, "--- Alternate[index]=glyphId\n"));
			else
			  DL(4, (OUTPUTBUFF, "--- Alternate[index]=glyphId glyphName/CID\n"));
			for (j = 0; j < altset->GlyphCount; j++)
			  {
				if (level < 4) 
				  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, altset->Alternate[j]));
				else
				  DL(4, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, altset->Alternate[j], getGlyphName(altset->Alternate[j], 0)));
			  }
			DL(2, (OUTPUTBUFF, "\n"));
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
	}

static void dumpAlternate(void *fmt, IntX level)
	{
	DL(2, (OUTPUTBUFF, "--- AlternateSubst\n"));

	switch (((AlternateSubstFormat1 *)fmt)->SubstFormat)
		{
	case 1:
		dumpAlternate1(fmt, level);
		break;
		}
	}

static IntX AlternateSubstFlavor(void *fmt)
	{
	  return (((AlternateSubstFormat1 *)fmt)->SubstFormat);
	}

static void proofLigature1(LigatureSubstFormat1 *fmt)
	{
		ttoEnumRec CovList;
		Card32 nitems;
		IntX i;
		IntX isVert = proofIsVerticalMode();
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);

		for (i = 0; i < (IntX)nitems; i++)
		  {
			LigatureSet *ligatureSet;
			char name1[MAX_NAME_LEN];
			char name2[MAX_NAME_LEN];
			char ligname[MAX_NAME_LEN];
			IntX glyphId2, ligId;
			IntX j;
			IntX origShift, lsb, rsb, width1, width2, tsb, bsb, vwidth, W1, W2;
			IntX yorig1, yorig2, yorig3;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			
			strcpy(name1, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width1, &tsb, &bsb, &vwidth, &yorig1);
			if (isVert)
			  {
				if (vwidth != 0) W1 = vwidth;
				else  W1 = width1;
			  }
			else
			  W1 = width1;

			ligatureSet = &fmt->_LigatureSet[i];
			for (j = 0; j < ligatureSet->LigatureCount; j++)
			  {
				IntX k;
				Ligature *ligature = &ligatureSet->_Ligature[j];
				ligId = ligature->LigGlyph;
				strcpy(ligname, getGlyphName(ligId, 1));
				
				proofDrawGlyph(proofctx, 
							   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
				
				for (k = 0; k < ligature->CompCount - 1; k++) 
				  {
					glyphId2 = ligature->Component[k];
					strcpy(name2, getGlyphName(ligature->Component[k], 1));
					getMetrics(glyphId2, &origShift, &lsb, &rsb, &width2, &tsb, &bsb, &vwidth, &yorig2);
					if (isVert)
					  {
						if (vwidth != 0) W2 = vwidth;
						else  W2 = width2;
					  }
					else
					  W2 = width2;

					proofSymbol(proofctx, PROOF_PLUS);
					proofDrawGlyph(proofctx, 
								   glyphId2, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
								   name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
								   NULL, 0, /* altlabel,altlabelflags */
								   0,0, /* originDx,originDy */
								   0,0, /* origin, originflags */
								   W2, 0, /* width,widthflags */
								   NULL, yorig2, ""
								   );
				  }
				proofSymbol(proofctx, PROOF_YIELDS);
				getMetrics(ligId, &origShift, &lsb, &rsb, &width2, &tsb, &bsb, &vwidth, &yorig3);
				if (isVert)
				  {
					if (vwidth != 0) W2 = vwidth;
					else  W2 = width2;
				  }
				else
				  W2 = width2;
				proofDrawGlyph(proofctx, 
							   ligId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   ligname, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W2, 0, /* width,widthflags */
							   NULL, yorig3, ""
							   );
				proofNewline(proofctx);
			  }
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);	  	
	}
	
static void dumpLigature1(LigatureSubstFormat1 *fmt, IntX level)
	{
	IntX i;

	if (level == 8)
		proofLigature1(fmt);
	else if (level ==7)
	  {
	  	ttoEnumRec CovList;
	  	Int32 nitems;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, (Card32*)&nitems);
		for (i = 0; i < (Int32)nitems; i++) 
		{
		  char name[MAX_NAME_LEN];
		  IntX j;
		  LigatureSet *ligatureSet = &fmt->_LigatureSet[i];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

		  strcpy(name, getGlyphName(glyphId, 0));
			
			for (j = 0; j < ligatureSet->LigatureCount; j++)
			  {
				IntX k;
				Ligature *ligature = &ligatureSet->_Ligature[j];
				
				fprintf(OUTPUTBUFF, "%s sub %s ", contextPrefix, name);
				
				for (k = 0; k < ligature->CompCount - 1; k++) 
				  {
				  	fprintf(OUTPUTBUFF, "%s ", getGlyphName(ligature->Component[k], 0));
				  }
				fprintf(OUTPUTBUFF, "by %s;\n", getGlyphName(ligature->LigGlyph, 0));
			  }
		  }
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
	  }
	else
	  {
		DLu(2, "SubstFormat=", fmt->SubstFormat);
		DLx(2, "Coverage   =", fmt->Coverage);
		DLu(2, "LigSetCount=", fmt->LigSetCount);
		
		DL(2, (OUTPUTBUFF, "--- LigatureSet[index]=offset\n"));
		for (i = 0; i < fmt->LigSetCount; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->LigatureSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->LigSetCount; i++)
		  {
			IntX j;
			LigatureSet *ligatureSet = &fmt->_LigatureSet[i];
			
			DL(2, (OUTPUTBUFF, "--- LigatureSet (%04hx)\n", fmt->LigatureSet[i]));
			DLu(2, "LigatureCount=", ligatureSet->LigatureCount);
			
			DL(2, (OUTPUTBUFF, "--- Ligature[index]=offset\n"));
			for (j = 0; j < ligatureSet->LigatureCount; j++)
			  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", j, ligatureSet->Ligature[j]));
			DL(2, (OUTPUTBUFF, "\n"));
			
			for (j = 0; j < ligatureSet->LigatureCount; j++)
			  {
				IntX k;
				Ligature *ligature = &ligatureSet->_Ligature[j];
				DL(2, (OUTPUTBUFF, "--- Ligature (%04x)\n", ligatureSet->Ligature[j]));
				if (level < 4)
				  DL(2, (OUTPUTBUFF, "LigGlyphId =%hu\n", ligature->LigGlyph));
				else
				  DL(2, (OUTPUTBUFF, "LigGlyphId =%hu (%s)\n", ligature->LigGlyph, getGlyphName(ligature->LigGlyph, 0)));
				DLu(2, "CompCount=", ligature->CompCount);
				
				if (level < 4)
				  DL(2, (OUTPUTBUFF, "--- Component[index]=glyphId\n"));
				else
				  DL(3, (OUTPUTBUFF, "--- Component[index]=glyphId glyphName/CID\n"));
				for (k = 0; k < ligature->CompCount - 1; k++) 
				  {
					if (level < 4)
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", k, ligature->Component[k]));
					else
					  DL(3, (OUTPUTBUFF, "[%d]=%hu (%s) ", k, ligature->Component[k], getGlyphName(ligature->Component[k], 0)));
				  }
				DL(2, (OUTPUTBUFF, "\n"));
			  }
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
  }


static void dumpLigature(void *fmt, IntX level)
	{
	DL(2, (OUTPUTBUFF, "--- LigatureSubst\n"));

	switch (((LigatureSubstFormat1 *)fmt)->SubstFormat)
		{
	case 1:
		dumpLigature1(fmt, level);
		break;
		}
	}


static IntX LigatureSubstFlavor(void *fmt)
	{
	  return (((LigatureSubstFormat1 *)fmt)->SubstFormat);
	}

static void dumpSubRuleSet(SubRuleSet *subruleset, IntX level)
	{
	IntX i,j;

	DLu(2, "SubRuleCount=", subruleset->SubRuleCount);
	DL(2, (OUTPUTBUFF, "--- SubRule[index]=offset\n"));	
	for (i = 0; i < subruleset->SubRuleCount; i++) 
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, subruleset->SubRule[i]));
		}	
	DL(2, (OUTPUTBUFF, "\n"));
	for (i = 0; i < subruleset->SubRuleCount; i++) 
		{
		SubRule *subrule = &subruleset->_SubRule[i];
		DL(2, (OUTPUTBUFF, "--- SubRule (%04hx)\n", subruleset->SubRule[i]));
		DLu(2, "GlyphCount=", subrule->GlyphCount);
		DL(2, (OUTPUTBUFF, "--- Input[index]=glyphId\n"));
		for (j = 1; j < subrule->GlyphCount; j++) 
			{
			  if (level < 4)
				{
				  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subrule->Input[j]));
				}
			  else
				{
				  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, subrule->Input[j], getGlyphName(subrule->Input[j], 0)));
				}
			}
		DL(2, (OUTPUTBUFF, "\n"));
		DLu(2, "SubstCount=", subrule->SubstCount);
		DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
		for (j = 0; j < subrule->SubstCount; j++) 
		  {
			Card16 seq = subrule->SubstLookupRecord[j].SequenceIndex;
			Card16 lis = subrule->SubstLookupRecord[j].LookupListIndex;	
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}
	
typedef struct {
	GlyphId *gid;
	IntX numGids;
	IntX startPos;
	} GlyphRec;

static void dumpContext1(ContextSubstFormat1 *fmt, IntX level)
	{
	IntX i;
	if ((level == 8) || (level == 7))
		{
	 	 featureWarning(level, SPOT_MSG_GSUBUFMTCTX, fmt->SubstFormat);
		}
	else if (level < 7)
	  {
		DLu(2, "SubstFormat    =", fmt->SubstFormat);
		DLx(2, "Coverage       =", fmt->Coverage);
		DLu(2, "SubRuleSetCount=", fmt->SubRuleSetCount);
		
		DL(2, (OUTPUTBUFF, "--- SubRuleSet[index]=offset\n"));
		for (i = 0; i < fmt->SubRuleSetCount; i++) 
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->SubRuleSet[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->SubRuleSetCount; i++) 
		  {
			if (fmt->SubRuleSet != 0)
			  {
				DL(2, (OUTPUTBUFF, "--- SubRuleSet (%04hx)\n", fmt->SubRuleSet[i]));
				dumpSubRuleSet(&fmt->_SubRuleSet[i], level);
			  }
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
	  
	  
#if NOTYET
	else if (level == 8)
	  {
		ttoEnumRec CovList;
		Card32 nitems;
		IntX i;
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		
		for (i = 0; i < (IntX)nitems; i++)
		  {
			char name1[MAX_NAME_LEN];
			IntX srscnt;
			IntX origShift, lsb, rsb, width1, tsb, bsb, vwidth, yorig;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			
			strcpy(name1, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width1, &tsb, &bsb, &vwidth, &yorig);
			for (srscnt = 0; srscnt < fmt->SubRuleSetCount; srscnt++)
			  {
				if (fmt->SubRuleSet != 0)
				  {
#if __CENTERLINE__
					centerline_stop();
#endif
					IntX subr;
					SubRuleSet *subruleset = &(fmt->_SubRuleSet[srscnt]);
					/* ??? To complete */
				  }
			  }
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);
	  }
	  else if (level == 7)
		{
		/*
		ttoEnumRec CovList;
	  	Card32 nitems;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		for (i = 0; i < nitems; i++) 
		{
		  	char name[MAX_NAME_LEN];
		  	SubRuleSet *subruleset = &fmt->_SubRuleSet[i];
		  	GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			strcpy(name, getGlyphName(glyphId, 0));
			
		  	for (j = 0; j < subruleset->SubRuleCount; j++) 
			{
				SubRule *subrule = &subruleset->_SubRule[j];
				GlyphId *sequence;
				GlyphRec *dest;
				IntX offset=0;
				IntX size=subrule->GlyphCount;
				
				sequence = memNew(subrule->GlyphCount*sizeof(GlyphId));
				dest = memNew(subrule->GlyphCount*sizeof(GlyphRec));
				
				dest[0].gid= memNew(sizeof(GlyphId));
				sequence[0]=*(dest[0].gid)=glyphId;
				dest[0].startPos=0;
				dest[0].numGids=1;
				for (k = 0; k < subrule->GlyphCount-1; k++) 
				  {
				  	fprintf(OUTPUTBUFF, " %s", getGlyphName(subrule->Input[k], 0));
				  	dest[k+1].gid = memNew(sizeof(GlyphId));
				  	sequence[k+1]=*(dest[k+1].gid)=subrule->Input[k];
				  	dest[k+1].startPos=k+1;
				  	dest[k+1].numGids=1;
				  }
				for (k = 0; k < subrule->SubstCount; k++) 
				  {
					IntX outputglyphs;
					Card16 seq = subrule->SubstLookupRecord[k].SequenceIndex;
					Card16 lis = subrule->SubstLookupRecord[k].LookupListIndex;	
					if (!GSUBEvaliP(lis, seq, &size, dest))
					{
						fprintf(OUTPUTBUFF, "\nError: Could not evaluate GSUB lookup.\n");
						break;
					}	
				  }
				
				fprintf(OUTPUTBUFF, "sub", name);
				for (l=0, k = 0; k < size; k++){ 
				  if (dest[k].startPos>=0){
				  	for(;l<k; l++)
				  		fprintf(OUTPUTBUFF, "%s' ", getGlyphName(sequence[l], 0));
				  	fprintf(OUTPUTBUFF, "%s ", getGlyphName(sequence[k], 0));
				  }
				 }
				for(; l<subrule->GlyphCount	; l++)
					fprintf(OUTPUTBUFF, "%s ", getGlyphName(sequence[l], 0));
				
				fprintf(OUTPUTBUFF, " by");
				
				for (k = 0; k < size; k++) 
				  if (dest[k].startPos==-1){
				  	if(dest[k].numGids==1){
					  	if(*dest[i].gid!=0xffff)
					  		fprintf(OUTPUTBUFF, " %s", getGlyphName(*dest[i].gid, 0));
				  	}else
				  		fprintf(stderr, "\nError: Need to implement contextual one-to-many sub.\n");
				  }
				  		 
		   		fprintf(OUTPUTBUFF, ";\n");
		   		for (k=0; k<size; k++)
		   			memFree(dest[k].gid);
		   		memFree(dest);
		   		memFree(sequence);
		   	}
		  }
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
	*/ 
	}
#endif
	}

static void dumpSubClassSet(SubClassSet *subclassset, IntX level, void *feattag)
	{
	IntX i, j;

	DLu(2, "SubClassRuleCnt=", subclassset->SubClassRuleCnt);

	DL(2, (OUTPUTBUFF, "--- SubClassRule[index]=offset\n"));	
	for (i = 0; i < subclassset->SubClassRuleCnt; i++) 
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, subclassset->SubClassRule[i]));
		}	
	DL(2, (OUTPUTBUFF, "\n"));
	for (i = 0; i < subclassset->SubClassRuleCnt; i++) 
		{
		SubClassRule *subclassrule = &subclassset->_SubClassRule[i];
		DL(2, (OUTPUTBUFF, "--- SubClassRule (%04hx)\n", subclassset->SubClassRule[i]));
		DLu(2, "GlyphCount=", subclassrule->GlyphCount);
		DL(2, (OUTPUTBUFF, "--- Input[index]=classId\n"));
		for (j = 1; j < subclassrule->GlyphCount; j++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subclassrule->Class[j]));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		DLu(2, "SubstCount=", subclassrule->SubstCount);
		DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(GlyphSequenceIndex,LookupListIndex)\n"));
		for (j = 0; j < subclassrule->SubstCount; j++) 
			{
			Card16 seq = subclassrule->SubstLookupRecord[j].SequenceIndex;
			Card16 lis = subclassrule->SubstLookupRecord[j].LookupListIndex;
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
			}
		DL(2, (OUTPUTBUFF, "\n"));

		}
	}

static void showContext2(ContextSubstFormat2 *fmt, IntX level, void *feattag)
	{
		ttoEnumRec *classList;
		Card32 cnt, scscnt, psIndex;
		IntX i, forProofing, isVert;
		char proofString[kProofBufferLen];
		classList = memNew(sizeof(ttoEnumRec) * (fmt->SubClassSetCnt));
		ttoEnumerateClass(fmt->ClassDef, fmt->_ClassDef, 
						  fmt->SubClassSetCnt,
						  classList, &cnt);
		
		forProofing = isVert = 0;
		if (level == 8)
			{
			forProofing = 1;
		 	proofThinspace(proofctx, 1);

			}
			
		for (scscnt = 0; scscnt < fmt->SubClassSetCnt; scscnt++)
		  {
			if (fmt->SubClassSet[scscnt] != 0) 
			  {
				SubClassSet *subclassset = &fmt->_SubClassSet[scscnt];
				IntX scrcnt;
				
				for (scrcnt = 0; scrcnt < subclassset->SubClassRuleCnt; scrcnt++)
				  {
					SubClassRule *subclassrule = &subclassset->_SubClassRule[scrcnt];
					Card16 seqindex, lookupindex;
					IntX classi, classId, c;
					ttoEnumRec *CR;

					if (subclassrule->GlyphCount > fmt->SubClassSetCnt)
					 {
                        warning(SPOT_MSG_GSUBCTXDEF, subclassrule->GlyphCount, fmt->SubClassSetCnt);
					 }

					seqindex = subclassrule->SubstLookupRecord[0].SequenceIndex;
					lookupindex = subclassrule->SubstLookupRecord[0].LookupListIndex;

					sprintf(proofString, "context [");

					classId = scscnt;
					CR = &(classList[classId]);
					for (c = 0; c < CR->glyphidlist.cnt; c++)
					  {
						char name2[MAX_NAME_LEN];
						GlyphId glyphId2 = *da_INDEX(CR->glyphidlist, c);
						strcpy(name2, getGlyphName(glyphId2, forProofing));
						psIndex = strlen(proofString);
						sprintf(&proofString[psIndex]," %s", name2);
					  }
					  
					psIndex = strlen(proofString);
					sprintf(&proofString[psIndex],"]");
					if (level == 7)
						fprintf(OUTPUTBUFF, "%s", proofString);
					else if (level == 8)
						dumpMessage(proofString, ((Card32 *)feattag)[0]);
					proofString[0] = 0;
					
					for (classi=1; classi < subclassrule->GlyphCount; classi++)
						{
						classId = subclassrule->Class[classi];
						CR = &(classList[classId]);
						psIndex = strlen(proofString);
						sprintf(&proofString[psIndex],"[");
						for (c = 0; c < CR->glyphidlist.cnt; c++)
						  {
							char name2[MAX_NAME_LEN];
							GlyphId glyphId2 = *da_INDEX(CR->glyphidlist, c);
							strcpy(name2, getGlyphName(glyphId2, forProofing));
							psIndex = strlen(proofString);
                            if ((psIndex + strlen(name2) + 10) >= kProofBufferLen)
                            {
                                if (level == 7)
                                    fprintf(OUTPUTBUFF, "\n    %s", proofString);
                                else if (level == 8)
                                    dumpMessage(proofString, ((Card32 *)feattag)[0]);
                                proofString[0] = 0;
                                psIndex = 0;
                            }
							sprintf(&proofString[psIndex]," %s", name2);
						  }
						psIndex = strlen(proofString);
						sprintf(&proofString[psIndex]," ]");
						if (level == 7)
							fprintf(OUTPUTBUFF, "\n    %s", proofString);
						else if (level == 8)
							dumpMessage(proofString, ((Card32 *)feattag)[0]);
						proofString[0] = 0;
						}
					
					
					if (level == 7)
						fprintf(OUTPUTBUFF, "   {\n");
					sprintf(contextPrefix, "   at index %d ", seqindex);
					if (level == 8)
						dumpMessage(contextPrefix, ((Card32 *)feattag)[0]);
					
					ttoDumpLookupListItem(GSUB.LookupList, &GSUB._LookupList, lookupindex, level, dumpSubtable, feattag);
					
					contextPrefix[0] = 0;
					
					if (level == 7)
						fprintf(OUTPUTBUFF, "} context;\n\n");
					
				  }
			  }
		  }

		for (i = 0; i < fmt->SubClassSetCnt; i++)
		  {
			if (classList[i].glyphidlist.size > 0)
			  da_FREE(classList[i].glyphidlist);
		  }
		memFree(classList);  	
	}
	

#if 0
static void proofContext2(ContextSubstFormat2 *fmt)
	{
 		/* ??? Half-hearted attempt */
		ttoEnumRec CovList;
		Card32 nitems;
		ttoEnumRec *classList;
		Card32 cnt;
		IntX i;
		IntX isVert = proofIsVerticalMode();
		IntX origShift, lsb, rsb, width1, width2, tsb, bsb, vwidth, yorig1, yorig2, yorig3, W1, W2;

		classList = memNew(sizeof(ttoEnumRec) * (fmt->SubClassSetCnt));
		ttoEnumerateClass(fmt->ClassDef, fmt->_ClassDef, 
						  fmt->SubClassSetCnt,
						  classList, &cnt);
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		for (i = 0; i < nitems; i++)
		  {
			char name1[MAX_NAME_LEN];
			IntX scscnt;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			strcpy(name1, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width1, &tsb, &bsb, &vwidth, &yorig1);
			if (isVert)
			  {
				if (vwidth != 0) W1 = vwidth;
				else  W1 = width1;
			  }
			else
			  W1 = width1;

			for (scscnt = 0; scscnt < fmt->SubClassSetCnt; scscnt++)
			  {
				if (fmt->SubClassSet[scscnt] != 0) 
				  {
					SubClassSet *subclassset = &fmt->_SubClassSet[scscnt];
					IntX scrcnt;
					for (scrcnt = 0; scrcnt < subclassset->SubClassRuleCnt; scrcnt++)
					  {
						SubClassRule *subclassrule = &subclassset->_SubClassRule[scrcnt];
						Card16 seqindex, lookupindex;
						IntX classId, c;
						ttoEnumRec *CR;

						if (subclassrule->SubstCount < 1) 
						  continue;
#if __CENTERLINE__
					centerline_stop();
#endif
/* ??? to complete */
						if (subclassrule->GlyphCount > fmt->SubClassSetCnt)
							warning(SPOT_MSG_GSUBCTXDEF, subclassrule->GlyphCount, fmt->SubClassSetCnt);
						if (subclassrule->SubstCount > 1) 
						  warning(SPOT_MSG_GSUBCTXCNT);

						seqindex = subclassrule->SubstLookupRecord[0].SequenceIndex;
						lookupindex = subclassrule->SubstLookupRecord[0].LookupListIndex;
						if (seqindex > 1)
						  {
							warning(SPOT_MSG_GSUBCTXNYI); /* ??? */
							continue;
						  }
						classId = subclassrule->Class[seqindex];
						CR = &(classList[classId]);
						for (c = 0; c < CR->glyphidlist.cnt; c++)
						  {
							char name2[MAX_NAME_LEN];
							GlyphId inputgid[4];
							GlyphId outputgid[4];
							IntX inputcount, outputcount, o;
							GlyphId glyphId2 = *da_INDEX(CR->glyphidlist, c);

							strcpy(name2, getGlyphName(glyphId2, 1));
							getMetrics(glyphId2, &origShift, &lsb, &rsb, &width2, &tsb, &bsb, &vwidth, &yorig2);
							if (isVert)
							  {
								if (vwidth != 0) W2 = vwidth;
								else  W2 = width2;
							  }
							else
							  W2 = width2;

							inputcount = 1;
							inputgid[0] = glyphId2; 
							outputgid[0] = 0; /* for now */
							outputcount = 0;
							GSUBEval(lookupindex, inputcount, inputgid,
									 &outputcount, outputgid);
							if (outputcount == 0) 
							  continue;
							proofDrawGlyph(proofctx, 
							   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
							proofSymbol(proofctx, PROOF_PLUS);
							proofDrawGlyph(proofctx, 
							   glyphId2, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
						       0,0, /* origin, originflags */
							   W2, 0, /* width,widthflags */
							   NULL, yorig2, ""
							   );
							proofSymbol(proofctx, PROOF_YIELDS);
							proofDrawGlyph(proofctx, 
							   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							   name1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
 						       0,0, /* origin, originflags */
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
							proofSymbol(proofctx, PROOF_PLUS);					
							for (o = 0; o < outputcount; o++)
							  {
								IntX owid, OW;
								char oname[MAX_NAME_LEN];
								GlyphId ogid = outputgid[o];
								strcpy(oname, getGlyphName(ogid, 1));
								getMetrics(ogid, &origShift, &lsb, &rsb, &owid, &tsb, &bsb, &vwidth, &yorig3);
								if (isVert)
								  {
									if (vwidth != 0) OW = vwidth;
									else  OW = owid;
								  }
								else
								  OW = owid;

								proofDrawGlyph(proofctx, 
											   ogid, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
											   oname, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
											   NULL, 0, /* altlabel,altlabelflags */
											   0,0, /* originDx,originDy */
											   0,0, /* origin, originflags */
											   OW, 0, /* width,widthflags */
											   NULL, yorig3, ""
											   );
								if ((o+1) < outputcount)
								  proofSymbol(proofctx, PROOF_PLUS);
							  }
							proofNewline(proofctx);
						  }
					  }
				  }
			  }
		  }

		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);
		for (i = 0; i < fmt->SubClassSetCnt; i++)
		  {
			if (classList[i].glyphidlist.size > 0)
			  da_FREE(classList[i].glyphidlist);
		  }
		memFree(classList);  	
	}
#endif
	
static void dumpContext2(ContextSubstFormat2 *fmt, IntX level, void *feattag)
	{
	IntX i;

	if ((level == 8) || (level == 7))
		showContext2(fmt, level, feattag);
	else
	  {
		DLu(2, "SubstFormat   =", fmt->SubstFormat);
		DLx(2, "Coverage      =", fmt->Coverage);
		DLx(2, "ClassDef      =", fmt->ClassDef);
		DLu(2, "SubClassSetCnt=", fmt->SubClassSetCnt);
		
		DL(2, (OUTPUTBUFF, "--- SubClassSet[index]=offset\n"));
		for (i = 0; i < fmt->SubClassSetCnt; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->SubClassSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->SubClassSetCnt; i++) 
		  {
			if (fmt->SubClassSet[i] != 0) 
			  {
				DL(2, (OUTPUTBUFF, "--- SubClassSet (%04hx)\n", fmt->SubClassSet[i]));
				dumpSubClassSet(&fmt->_SubClassSet[i], level, feattag);
			  }
		  }

		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		ttoDumpClass(fmt->ClassDef, fmt->_ClassDef, level);
	  }
	}

static void dumpContext3(ContextSubstFormat3 *fmt, IntX level)
	{
	IntX i;

	if ((level == 8) || (level == 7))
		{
	 	 featureWarning(level, SPOT_MSG_GSUBUFMTCTX, fmt->SubstFormat);
		}
	else if (level < 7)
		{
		DLu(2, "SubstFormat   =", fmt->SubstFormat);
		DLu(2, "GlyphCount =", fmt->GlyphCount);
		DL(2, (OUTPUTBUFF, "--- CoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
			{
			  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->CoverageArray[i]));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
			{
			 ttoDumpCoverage(fmt->CoverageArray[i], fmt->_CoverageArray[i], level);
			}
		DLu(2, "SubstCount =", fmt->SubstCount);
		DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
		for (i = 0; i < fmt->SubstCount; i++) 
			{
			Card16 seq = fmt->SubstLookupRecord[i].SequenceIndex;
			Card16 lis = fmt->SubstLookupRecord[i].LookupListIndex;	
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", i, seq, lis));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		}

	}

static void dumpContext(void *fmt, IntX level, void *feattag)
	{
	  DL(2, (OUTPUTBUFF, "--- ContextSubst\n"));
	  GSUBContextRecursionCnt++;

	 if (GSUBContextRecursionCnt > 1)
		{
		featureWarning(level,  SPOT_MSG_CNTX_RECURSION, ((ContextSubstFormat1 *)fmt)->SubstFormat);
		return;
		}
	
	  switch (((ContextSubstFormat1 *)fmt)->SubstFormat)
		{
		case 1:
		  dumpContext1(fmt, level);
		  break;
		case 2:
		  dumpContext2(fmt, level, feattag);
		  break;
		case 3:
		  dumpContext3(fmt, level);
		  break;
		}
	  GSUBContextRecursionCnt--;
  	}

static IntX ContextSubstFlavor(void *fmt)
	{
	  return (((ContextSubstFormat1 *)fmt)->SubstFormat);
	}



static void dumpChainSubRuleSet(ChainSubRuleSet *subruleset, IntX level)
	{
	IntX i,j;

	if (level == 5)
		{
		for (i = 0; i < subruleset->ChainSubRuleCount; i++) 

			{
			ChainSubRule *subrule = &subruleset->_ChainSubRule[i];
			for (j = 0; j < subrule->SubstCount; j++) 
			  {
				Card16 lis = subrule->SubstLookupRecord[j].LookupListIndex;	
				if (seenChainLookup[lis].seen)
					{
					if (seenChainLookup[lis].cur_parent == GSUBLookupIndex)
						continue;
					seenChainLookup[lis].cur_parent = GSUBLookupIndex;
					if (seenChainLookup[lis].parent != GSUBLookupIndex)
						DL5((OUTPUTBUFF, " %hu*",lis));
					}
				else
					{
					seenChainLookup[lis].parent = GSUBLookupIndex;
					seenChainLookup[lis].cur_parent = GSUBLookupIndex;
					seenChainLookup[lis].seen =1;
					DL5((OUTPUTBUFF, " %hu",lis));
					}
			  }
			}
		}
	else
		{
		DLu(2, "ChainSubRuleCount=", subruleset->ChainSubRuleCount);
		DL(2, (OUTPUTBUFF, "--- ChainSubRule[index]=offset\n"));	
		for (i = 0; i < subruleset->ChainSubRuleCount; i++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, subruleset->ChainSubRule[i]));
			}	
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < subruleset->ChainSubRuleCount; i++) 
			{
			ChainSubRule *subrule = &subruleset->_ChainSubRule[i];
			DL(2, (OUTPUTBUFF, "--- ChainSubRule (%04hx)\n", subruleset->ChainSubRule[i]));

			DLu(2, "BacktrackGlyphCount=", subrule->BacktrackGlyphCount);
			for (j = 0; j < subrule->BacktrackGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subrule->Backtrack[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, subrule->Backtrack[j], getGlyphName(subrule->Backtrack[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "InputGlyphCount=", subrule->InputGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Input[index]=glyphId\n"));
			for (j = 1; j < subrule->InputGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subrule->Input[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, subrule->Input[j], getGlyphName(subrule->Input[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "LookaheadGlyphCount=", subrule->LookaheadGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Lookahead[index]=glyphId\n"));
			for (j = 0; j < subrule->LookaheadGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subrule->Lookahead[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, subrule->Lookahead[j], getGlyphName(subrule->Lookahead[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "SubstCount=", subrule->SubstCount);
			DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
			for (j = 0; j < subrule->SubstCount; j++) 
			  {
				Card16 seq = subrule->SubstLookupRecord[j].SequenceIndex;
				Card16 lis = subrule->SubstLookupRecord[j].LookupListIndex;	
				DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
			  }
			DL(2, (OUTPUTBUFF, "\n"));
			}
		} /* end if not level 5 */
	}

static void listSubLookups1(ChainContextSubstFormat1 *fmt, IntX level)
	{
	IntX i;
		for (i = 0; i < fmt->ChainSubRuleSetCount; i++) 
		  {
			if (fmt->ChainSubRuleSet[i] != 0) 
			  {
				dumpChainSubRuleSet(&fmt->_ChainSubRuleSet[i], level);
			  }
		  }
	}

static void dumpChainContext1(ChainContextSubstFormat1 *fmt, IntX level)
	{
	IntX i;
	if ((level == 8) || (level == 7))
		{
	 	 featureWarning(level,  SPOT_MSG_GSUBUFMTCCTX, fmt->SubstFormat);
		}
	else if (level == 5)
		listSubLookups1(fmt, level);
	else if (level < 7)
	  {
		DLu(2, "SubstFormat    =", fmt->SubstFormat);
		DLx(2, "Coverage       =", fmt->Coverage);
		DLu(2, "ChainSubRuleSetCount=", fmt->ChainSubRuleSetCount);
		
		DL(2, (OUTPUTBUFF, "--- ChainSubRuleSet[index]=offset\n"));
		for (i = 0; i < fmt->ChainSubRuleSetCount; i++) 
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->ChainSubRuleSet[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->ChainSubRuleSetCount; i++) 
		  {
			if (fmt->ChainSubRuleSet != 0)
			  {
				DL(2, (OUTPUTBUFF, "--- ChainSubRuleSet (%04hx)\n", fmt->ChainSubRuleSet[i]));
				dumpChainSubRuleSet(&fmt->_ChainSubRuleSet[i], level);
			  }
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
  }



static void dumpChainSubClassSet(ChainSubClassSet *subclassset, IntX level, void* feattag)
	{
	IntX i, j;

	if (level == 5)
		{
		for (i = 0; i < subclassset->ChainSubClassRuleCnt; i++) 
			{
			IntX j;
			ChainSubClassRule *subclassrule = &subclassset->_ChainSubClassRule[i];
			for (j = 0; j < subclassrule->SubstCount; j++) 
				{
				Card16 lis = subclassrule->SubstLookupRecord[j].LookupListIndex;
				if (seenChainLookup[lis].seen)
					{
					if (seenChainLookup[lis].cur_parent == GSUBLookupIndex)
						continue;
					seenChainLookup[lis].cur_parent = GSUBLookupIndex;
					if (seenChainLookup[lis].parent != GSUBLookupIndex)
						DL5((OUTPUTBUFF, " %hu*",lis));
					}
				else
					{
					seenChainLookup[lis].parent = GSUBLookupIndex;
					seenChainLookup[lis].cur_parent = GSUBLookupIndex;
					seenChainLookup[lis].seen =1;
					DL5((OUTPUTBUFF, " %hu",lis));
					}
				}
			}
		}
	else
		{
		DLu(2, "ChainSubClassRuleCnt=", subclassset->ChainSubClassRuleCnt);

		DL(2, (OUTPUTBUFF, "--- ChainSubClassRule[index]=offset\n"));	
		for (i = 0; i < subclassset->ChainSubClassRuleCnt; i++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, subclassset->ChainSubClassRule[i]));
			}	
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < subclassset->ChainSubClassRuleCnt; i++) 
			{
			ChainSubClassRule *subclassrule = &subclassset->_ChainSubClassRule[i];
			DL(2, (OUTPUTBUFF, "--- ChainSubClassRule (%04hx)\n", subclassset->ChainSubClassRule[i]));

			DLu(2, "BacktrackGlyphCount=", subclassrule->BacktrackGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Backtrack[index]=classId\n"));
			for (j = 0; j < subclassrule->BacktrackGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subclassrule->Backtrack[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "InputGlyphCount=", subclassrule->InputGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Input[index]=classId\n"));
			for (j = 1; j < subclassrule->InputGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subclassrule->Input[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "LookaheadGlyphCount=", subclassrule->LookaheadGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Lookahead[index]=classId\n"));
			for (j = 0; j < subclassrule->LookaheadGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, subclassrule->Lookahead[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "SubstCount=", subclassrule->SubstCount);
			DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(GlyphSequenceIndex,LookupListIndex)\n"));
			for (j = 0; j < subclassrule->SubstCount; j++) 
				{
				Card16 seq = subclassrule->SubstLookupRecord[j].SequenceIndex;
				Card16 lis = subclassrule->SubstLookupRecord[j].LookupListIndex;
				DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
				}
				
			DL(2, (OUTPUTBUFF, "\n"));
			}
		} /* end if not level 5 */
	}


static void listSubLookups2(ChainContextSubstFormat2 *fmt, IntX level, void* feattag)
	{
	IntX i;
		for (i = 0; i < fmt->ChainSubClassSetCnt; i++) 
		  {
			if (fmt->ChainSubClassSet[i] != 0) 
			  {
				dumpChainSubClassSet(&fmt->_ChainSubClassSet[i], level, feattag);
			  }
		  }
	}
	
static void dumpChainContext2(ChainContextSubstFormat2 *fmt, IntX level, void* feattag)
	{
	IntX i;
	if ((level == 8) || (level == 7))
	 	 featureWarning(level,  SPOT_MSG_GSUBUFMTCCTX, fmt->SubstFormat);
	else if (level == 5)
		listSubLookups2(fmt, level, feattag);
	else if (level < 7)
	  {
		DLu(2, "SubstFormat   =", fmt->SubstFormat);
		DLx(2, "Coverage      =", fmt->Coverage);
		DLx(2, "BackTrackClassDef      =", fmt->BackTrackClassDef);
		DLx(2, "InputClassDef      =", fmt->InputClassDef);
		DLx(2, "LookAheadClassDef      =", fmt->LookAheadClassDef);
		DLu(2, "ChainSubClassSetCnt=", fmt->ChainSubClassSetCnt);
		
		DL(2, (OUTPUTBUFF, "--- ChainSubClassSet[index]=offset\n"));
		for (i = 0; i < fmt->ChainSubClassSetCnt; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->ChainSubClassSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->ChainSubClassSetCnt; i++) 
		  {
			if (fmt->ChainSubClassSet[i] != 0) 
			  {
				DL(2, (OUTPUTBUFF, "--- ChainSubClassSet (%04hx)\n", fmt->ChainSubClassSet[i]));
				dumpChainSubClassSet(&fmt->_ChainSubClassSet[i], level, feattag);
			  }
		  }

		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		if (fmt->BackTrackClassDef != 0)
			ttoDumpClass(fmt->BackTrackClassDef, fmt->_BackTrackClassDef, level);
		ttoDumpClass(fmt->InputClassDef, fmt->_InputClassDef, level);
		if (fmt->LookAheadClassDef != 0)
			ttoDumpClass(fmt->LookAheadClassDef, fmt->_LookAheadClassDef, level);
	  }
  }

static void decompileChainContext3(ChainContextSubstFormat3 *fmt)
	{
		IntX i, isException = 0;
		unsigned short numSubRecs = fmt->SubstCount;
		SubstLookupRecord* slr = NULL;
		IntX needLookupRefs = (numSubRecs > 1);
		IntX haveAnyInLineRepl = 0; /* have any sub rules that will print the replacement string, and hence new-line */
		IntX decompileable = 0;
		Lookup *lookup;
		
		da_DCL(ttoEnumRec, InputGlyphCovListArray);
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);
		
		isException = (fmt->SubstCount == 0);

		da_INIT(InputGlyphCovListArray, fmt->InputGlyphCount, 1);
		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  fprintf(OUTPUTBUFF, "ignore sub");
		else
			fprintf(OUTPUTBUFF, "sub");
		
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* Backtrack is enumerated in the feaure syntax in reverse order form the offset order. */
			GlyphId gId;
			ttoEnumRec *ter = da_INDEX(BacktrackGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter, &nitems);
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, " [");
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
			  }
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, "]");
		  }

		/*
		 Determine what input nodes need a direct lookup reference
		*/
		{
			unsigned short *inputRuleList = NULL;
			int seqIndex;
			inputRuleList = memNew(sizeof(*inputRuleList) * (fmt->InputGlyphCount));

			for (i = 0; i < fmt->InputGlyphCount; i++)
				{
				inputRuleList[i] = MAX_CARD16;
				}
			for (i = 0; i < numSubRecs; i++)
				{
				slr =  &fmt->SubstLookupRecord[i];
				seqIndex = slr->SequenceIndex;
				lookup = &(GSUB._LookupList._Lookup[slr->LookupListIndex]);
				if (needLookupRefs)
					decompileable = 0;
				else if ( (lookup->LookupType == SingleSubsType) || (lookup->LookupType == LigatureSubsType) )
					decompileable = 1;
				else
					decompileable = 0;
				
				if (0) //8/31/2012: msousa request that in-line replacement be printed as a second commented-out line.
					{
					if (!decompileable)
						{
						addToReferencedList( slr->LookupListIndex);
						inputRuleList[i] = slr->LookupListIndex;
						}
					else
						haveAnyInLineRepl = 1;
					}
				addToReferencedList( slr->LookupListIndex);
				inputRuleList[i] = slr->LookupListIndex;
				}
			for (i = 0; i < fmt->InputGlyphCount; i++)
			  {
				Card32 nitems;
				IntX j;
				GlyphId gId;
				ttoEnumRec *ter = da_INDEX(InputGlyphCovListArray, i);
				ttoEnumerateCoverage(fmt->Input[i], fmt->_Input[i], ter, &nitems);
				if (nitems > 1)
				  fprintf(OUTPUTBUFF, " [");
				for (j = 0; j < (IntX)nitems; j++)
				  {
					gId = *da_INDEX(ter->glyphidlist, j);
					fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
				  }
				if (nitems > 1)
				  fprintf(OUTPUTBUFF, "]");
				fprintf(OUTPUTBUFF, "'");
				
				if ((inputRuleList != NULL) && (inputRuleList[i] != MAX_CARD16))
					{
					fprintf(OUTPUTBUFF, " lookup lkp_%d", inputRuleList[i]);
					}
			  }
		}
		
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			ttoEnumRec *ter = da_INDEX(LookaheadGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter, &nitems);
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, " [");
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
			  }
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, "]");
		  }

		if (isException) 
		  {
			fprintf(OUTPUTBUFF, ";\n");
			return;
		  }
		  
		// if (haveAnyInLineRepl == 0) 8/31/2012: msousa request that in-line replacement be printed as a second commented-out line.
			fprintf(OUTPUTBUFF, ";\n");
			

		/* Need to free these because the calls to ttoEnumerateCoverage re-initialize them*/
		
		
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			da_FREE(InputGlyphCovListArray.array[i].glyphidlist);
		  }

/* -------------------------------------------------------->>  */
	{ /*  start write -sub rule block  */
		int n;
		IntX inputcount;
		for (n = 0; n < numSubRecs; n++)
			{
			IntX outputcount = 0;

			slr = &fmt->SubstLookupRecord[n];
			lookup = &(GSUB._LookupList._Lookup[slr->LookupListIndex]);

			if ( (lookup->LookupType == SingleSubsType) || (lookup->LookupType == LigatureSubsType) )
				decompileable = 1;
			else
				decompileable = 0;
				
			if (!decompileable)
				continue; /* can't handle print-outs of these rules */

			if (needLookupRefs) /* If we used lookup refs because there was more than one sub rule, show the lookup */
				fprintf(OUTPUTBUFF, " # lookup %d: ", slr->LookupListIndex);
				
			if (lookup->LookupType == AlternateSubsType)
				{
				fprintf(OUTPUTBUFF, "# from");
				inputcount = 1;
				}
			else
				{
				fprintf(OUTPUTBUFF, "# by");
				if (lookup->LookupType == LigatureSubsType)
					{
					if (numSubRecs == n+1) /* if this is the last sub record */
						inputcount = fmt->InputGlyphCount - slr->SequenceIndex; 
					else
						{
						inputcount =  fmt->SubstLookupRecord[n+1].SequenceIndex -  slr->SequenceIndex;
						}
					/* inputcount is not exact; it may be an overestimate. */
					}
				else
					inputcount = 1;
				}

			
		
			  {
			  Card32 itemnumber=0, limit=1;
			  while(itemnumber<limit)
				{
				  GlyphId inputgid[kMaxMultipleSub];
				  GlyphId outputgid[kMaxMultipleSub];
				  Card32 nitems;
				  IntX i,o;
				  Card16 lookupindex, seqindex;

				  seqindex = slr->SequenceIndex;
				  lookupindex = slr->LookupListIndex;	


				  for (i = 0; i < inputcount; i++)
					{
					  int iSeq = i + seqindex;
					  ttoEnumRec *ter = da_INDEX(InputGlyphCovListArray, i);
					  ttoEnumerateCoverage(fmt->Input[iSeq], fmt->_Input[iSeq], ter, &nitems);

					  if (nitems>1 && limit==1){
						limit=nitems;
						fprintf(OUTPUTBUFF, " [");
					  }else if(nitems>1 && itemnumber==0)
                        warning(SPOT_MSG_GSUBMULTIPLEINPUTS);
					  
					  if(nitems>1)	
						  inputgid[i] = *da_INDEX(ter->glyphidlist, (Int32)itemnumber);
					  else
						  inputgid[i] = *da_INDEX(ter->glyphidlist, 0);
					}

				  outputgid[0] = 0;
				  outputcount = 0;
				  GSUBEval(lookupindex, inputcount, inputgid, &outputcount, outputgid);
					

				  for (o = 0; o < outputcount; o++)
					{
					  GlyphId ogid = outputgid[o];
					  fprintf(OUTPUTBUFF, " %s", getGlyphName(ogid, 0));
					}
				 itemnumber++;
				 }
				 if (limit>1)
					fprintf(OUTPUTBUFF, "]");
			  }
			
			fprintf(OUTPUTBUFF, ";\n");
			if (outputcount == 0)
				fprintf(OUTPUTBUFF, "# Error: GSUB: Did not find a subrule in lookup %d which matches the context.\n", slr->LookupListIndex);

			for (i = 0; i < inputcount; i++)
			  {
				da_FREE(InputGlyphCovListArray.array[i].glyphidlist);
			  }
			} /* end for each subRule rec */
		if (InputGlyphCovListArray.size > 0)
		  da_FREE(InputGlyphCovListArray);

		for (i = 0; i <  fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		if (BacktrackGlyphCovListArray.size > 0)
		  da_FREE(BacktrackGlyphCovListArray);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		if (LookaheadGlyphCovListArray.size > 0)
		  da_FREE(LookaheadGlyphCovListArray);
			  
		} /* end write -sub rule block */
	}

static void proofChainContext3(ChainContextSubstFormat3 *fmt)
	{
		IntX i, isException = 0;	
		IntX isVert = proofIsVerticalMode();
		IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
		da_DCL(ttoEnumRec, InputGlyphCovListArray);
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);
		
		isException = (fmt->SubstCount == 0);

		da_INIT(InputGlyphCovListArray, fmt->InputGlyphCount, 1);
		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  proofSymbol(proofctx, PROOF_NOTELEM);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* Backtack is enumerated in reverse order from the offset order */
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(BacktrackGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;

				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								: 
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),  
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);

			if (i < fmt->BacktrackGlyphCount-1)
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if ((fmt->BacktrackGlyphCount > 0) &&  ((fmt->InputGlyphCount > 0) ||  (fmt->LookaheadGlyphCount > 0)) )
		  proofSymbol(proofctx, PROOF_PLUS);

		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(InputGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Input[i], fmt->_Input[i], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, 
							   ((isException) ? 
							   (ANNOT_SHOWIT|
								(isVert ? ((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								 :
								 ((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)))
							   : 
							   (ANNOT_SHOWIT|
								(isVert ? ((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								 : 
								 ((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM))
								|ANNOT_EMPHASIS)),    
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->InputGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if ( (fmt->InputGlyphCount > 0) && (fmt->LookaheadGlyphCount > 0))
		  proofSymbol(proofctx, PROOF_PLUS);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(LookaheadGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								:
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->LookaheadGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if (isException) 
		  {
			proofThinspace(proofctx, 2);
			proofSymbol(proofctx, PROOF_COLON);
			proofNewline(proofctx);
			return;
		  }


		/* Need to free these because the calls to ttoEnumerateCoverage re-initialize them*/
		
		
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			da_FREE(InputGlyphCovListArray.array[i].glyphidlist);
		  }
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		
			
/* -------------------------------------------------------->>  */
		proofSymbol(proofctx, PROOF_YIELDS);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* Backtrack is enumerated in the feaure syntax in reverse order form the offset order. */
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(BacktrackGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ?
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								:
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),  
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);

			if (i < fmt->BacktrackGlyphCount-1)
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if ((fmt->InputGlyphCount > 0) && (fmt->BacktrackGlyphCount > 0))
		  proofSymbol(proofctx, PROOF_PLUS);
		
		if (fmt->InputGlyphCount == 1)
		  {
			GlyphId inputgid[1];
			GlyphId outputgid[1];
			IntX inputcount, outputcount;
			Card32 nitems;
			IntX j,o;
			Card16 lookupindex, seqindex;
			ttoEnumRec *ter = da_INDEX(InputGlyphCovListArray, 0);
			
			ttoEnumerateCoverage(fmt->Input[0], fmt->_Input[0], ter, &nitems);
			
			if (fmt->SubstCount > 1)
			  {
				warning(SPOT_MSG_GSUBCCTXCNT);
			  }
			
			seqindex = fmt->SubstLookupRecord[0].SequenceIndex;
			lookupindex = fmt->SubstLookupRecord[0].LookupListIndex;	
			
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			
			for (j = 0; j < (IntX)nitems; j++)
			  {
				GlyphId	gId = *da_INDEX(ter->glyphidlist, j);
				inputgid[0] = gId;
				inputcount = 1;
				outputgid[0] = 0;
				outputcount = 0;
				GSUBEval(lookupindex, inputcount, inputgid, &outputcount, outputgid);
				if (outputcount == 0)
				  continue;
				
				for (o = 0; o < outputcount; o++)
				  {
					IntX owid, OW;
					char oname[MAX_NAME_LEN];
					GlyphId ogid = outputgid[o];
					strcpy(oname, getGlyphName(ogid, 1));
					getMetrics(ogid, &origShift, &lsb, &rsb, &owid, &tsb, &bsb, &vwidth, &yorig);
					if (isVert)
					  {
						if (vwidth != 0) OW = vwidth;
						else  OW = owid;
					  }
					else
					  OW = owid;
					proofDrawGlyph(proofctx, 
								   ogid, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
								   oname, ANNOT_SHOWIT|
								   (isVert ?
									((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
									:
									((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM))
								   |ANNOT_EMPHASIS,    
								   NULL, 0, /* altlabel,altlabelflags */
								   0,0, /* originDx,originDy */
								   0,0, /* origin, originflags */
								   OW, 0, /* width,widthflags */
								   NULL, yorig, ""
								   );
					if (o < (outputcount -1))
					  proofThinspace(proofctx, 1);
				  }
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
		  }
		else  /* more than 1 input glyph */
		  { 
		  Card32 itemnumber=0, limit=1;
		  GlyphId prevoutputgid[10];
		  IntX prevoutputcount=-1, skip;
			  
		  while(itemnumber<limit)
		  	{
			  GlyphId inputgid[10];
			  GlyphId outputgid[10];
			  IntX inputcount, outputcount;
			  Card32 nitems;
			  IntX i,o;
			  Card16 lookupindex, seqindex;

			  skip=0;
			  
			  if (fmt->SubstCount > 1)
				{
				  warning(SPOT_MSG_GSUBCCTXCNT);
				}
			  seqindex = fmt->SubstLookupRecord[0].SequenceIndex;
			  lookupindex = fmt->SubstLookupRecord[0].LookupListIndex;	

			  for (i = 0; i < fmt->InputGlyphCount; i++)
				{
				  ttoEnumRec *ter = da_INDEX(InputGlyphCovListArray, i);
				  ttoEnumerateCoverage(fmt->Input[i], fmt->_Input[i], ter, &nitems);
				  /*if (nitems > 1)
					warning(SPOT_MSG_GSUBINPUTCNT);*/
				  if (nitems>1 && limit==1){
				  	limit=nitems;
				  	proofSymbol(proofctx, PROOF_LPAREN);
				  }else if(nitems>limit && itemnumber==0)
                  {
                    proofMessage(proofctx, (Byte8*)spotMsg(SPOT_MSG_GSUBMULTIPLEINPUTS));
 				  }
                 if(nitems>1 && itemnumber<nitems)
					  inputgid[i] = *da_INDEX(ter->glyphidlist, (Int32)itemnumber);
				  else if (nitems>1)
				  	  inputgid[i] = *da_INDEX(ter->glyphidlist, (Int32)nitems-1);
				  else
				  	  inputgid[i] = *da_INDEX(ter->glyphidlist, 0);
				}

			  inputcount = fmt->InputGlyphCount;
			  outputgid[0] = 0;
			  outputcount = 0;
			  GSUBEval(lookupindex, inputcount, inputgid, &outputcount, outputgid);
			  
			  if (prevoutputcount==outputcount)
			  {
			  		for (o = 0; o < outputcount; o++)
			  			if(outputgid[o]!=prevoutputgid[o])
			  				break;
			  		if (o==outputcount)
			  			skip=1;
			  }		
			  if (!skip)
			  {
			  	if(itemnumber>0)
			  		proofThinspace(proofctx, 2);
			  	for (o = 0; o < outputcount; o++)
					{
					  IntX owid, OW;
					  char oname[MAX_NAME_LEN];
					  GlyphId ogid = outputgid[o];
					  strcpy(oname, getGlyphName(ogid, 1));
					  getMetrics(ogid, &origShift, &lsb, &rsb, &owid, &tsb, &bsb, &vwidth, &yorig);
					  if (isVert)
						{
						  if (vwidth != 0) OW = vwidth;
						  else  OW = owid;
						}
					  else
						OW = owid;
					  proofDrawGlyph(proofctx, 
									 ogid, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
									 oname, ANNOT_SHOWIT|
									 (isVert ? 
									  ((o % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
									  :
									  ((o % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM))
									 |ANNOT_EMPHASIS,    
									 NULL, 0, /* altlabel,altlabelflags */
									 0,0, /* originDx,originDy */
									 0,0, /* origin, originflags */
									 OW, 0, /* width,widthflags */
									 NULL, yorig, ""
									 );
					  if (o < (outputcount - 1))
						proofThinspace(proofctx, 1);
					}
				prevoutputcount=outputcount;
				for (o = 0; o < outputcount; o++)
					prevoutputgid[o]=outputgid[o];
			   }
				itemnumber++;
			}
			if (limit>1)
				proofSymbol(proofctx, PROOF_RPAREN);
		  }

		if ((fmt->InputGlyphCount > 0) && (fmt->LookaheadGlyphCount > 0))
		  proofSymbol(proofctx, PROOF_PLUS);

		
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(LookaheadGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ?
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								:
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->LookaheadGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			da_FREE(InputGlyphCovListArray.array[i].glyphidlist);
		  }
		if (fmt->InputGlyphCount > 0)
		  da_FREE(InputGlyphCovListArray);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		if (fmt->BacktrackGlyphCount > 0)
		  da_FREE(BacktrackGlyphCovListArray);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		if (fmt->LookaheadGlyphCount > 0)
		  da_FREE(LookaheadGlyphCovListArray);	
		
		proofNewline(proofctx);
	}

static void listSubLookups3(ChainContextSubstFormat3 *fmt, IntX level)
	{
		IntX i;

		for (i = 0; i < fmt->SubstCount; i++) 
		  {
			Card16 lis = fmt->SubstLookupRecord[i].LookupListIndex;	
			if (seenChainLookup[lis].seen)
				{
				if (seenChainLookup[lis].cur_parent == GSUBLookupIndex)
					continue;
				seenChainLookup[lis].cur_parent = GSUBLookupIndex;
				if (seenChainLookup[lis].parent != GSUBLookupIndex)
					DL5((OUTPUTBUFF, " %hu*",lis));
				}
			else
				{
				seenChainLookup[lis].parent = GSUBLookupIndex;
				seenChainLookup[lis].cur_parent = GSUBLookupIndex;
				seenChainLookup[lis].seen =1;
				DL5((OUTPUTBUFF, " %hu",lis));
				}
		  }
	}

static void dumpChainContext3(ChainContextSubstFormat3 *fmt, IntX level)
	{
	IntX i;

	if (level == 8)
		proofChainContext3(fmt);
	else if (level==7){
		decompileChainContext3(fmt);
	}
	else if (level == 5)
		listSubLookups3(fmt, level);
	else
	  {
		DLu(2, "SubstFormat   =", fmt->SubstFormat);
		
		DLu(2, "BacktrackGlyphCount =", fmt->BacktrackGlyphCount);
		DL(2, (OUTPUTBUFF, "--- BacktrackCoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Backtrack[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			ttoDumpCoverage(fmt->Backtrack[i], fmt->_Backtrack[i], level);
		  }
		
		DLu(2, "InputGlyphCount =", fmt->InputGlyphCount);
		DL(2, (OUTPUTBUFF, "--- InputCoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Input[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			ttoDumpCoverage(fmt->Input[i], fmt->_Input[i], level);
		  }
		
		DLu(2, "LookaheadGlyphCount =", fmt->LookaheadGlyphCount);
		DL(2, (OUTPUTBUFF, "--- LookaheadCoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Lookahead[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			ttoDumpCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], level);
		  }
		
		DLu(2, "SubstCount =", fmt->SubstCount);
		DL(2, (OUTPUTBUFF, "--- SubstLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
		for (i = 0; i < fmt->SubstCount; i++) 
		  {
			Card16 seq = fmt->SubstLookupRecord[i].SequenceIndex;
			Card16 lis = fmt->SubstLookupRecord[i].LookupListIndex;	
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", i, seq, lis));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
	  }
	}

static void dumpChainContext(void *fmt, IntX level, void* feattag)
	{
	  DL(2, (OUTPUTBUFF, "--- ChainingContextSubst\n"));
	  GSUBContextRecursionCnt++;
	 if (GSUBContextRecursionCnt > 1)
		{
		featureWarning(level,  SPOT_MSG_CNTX_RECURSION, ((ChainContextSubstFormat1 *)fmt)->SubstFormat);
		return;
		}
	  
	  switch (((ChainContextSubstFormat1 *)fmt)->SubstFormat)
		{
		case 1:
		  dumpChainContext1(fmt, level);
		  break;
		case 2:
		  dumpChainContext2(fmt, level, feattag);
		  break;
		case 3:
		  dumpChainContext3(fmt, level);
		  break;
		}
	  GSUBContextRecursionCnt--;
	}



static void decompileReverseChainContext1(ReverseChainContextSubstFormat1 *fmt)
	{
		IntX i, isException = 0;
		Card32 nitems;
		GlyphId outputgid;

		ttoEnumRec InputGlyphCovList;
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);
		
		isException = (fmt->GlyphCount == 0);

		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  fprintf(OUTPUTBUFF, "ignore rsub");
		else
			fprintf(OUTPUTBUFF, "rsub");
		
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* Backtrack is enumerated in the feaure syntax in reverse order form the offset order. */
			GlyphId gId;
			ttoEnumRec *ter = da_INDEX(BacktrackGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter, &nitems);
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, " [");
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
			  }
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, "]");
		  }

		ttoEnumerateCoverage(fmt->InputCoverage, fmt->_InputCoverage, &InputGlyphCovList, &nitems);
		if (nitems > 1)
		  fprintf(OUTPUTBUFF, " [");
		for (i = 0; i < (IntX)nitems; i++)
		  {
			GlyphId gId;
			gId = *da_INDEX(InputGlyphCovList.glyphidlist, i);
			fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
		  }
		if (nitems > 1)
		  fprintf(OUTPUTBUFF, "]");
		fprintf(OUTPUTBUFF, "'");


		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			IntX j;
			GlyphId gId;
			ttoEnumRec *ter = da_INDEX(LookaheadGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter, &nitems);
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, " [");
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				fprintf(OUTPUTBUFF, " %s", getGlyphName(gId, 0));
			  }
			if (nitems > 1)
			  fprintf(OUTPUTBUFF, "]");
		  }

		if (isException) 
		  {
			fprintf(OUTPUTBUFF, "\n");
			return;
		  }

/* -------------------------------------------------------->>  */
		fprintf(OUTPUTBUFF, " by");
		
		
		if (fmt->GlyphCount > 1)
			fprintf(OUTPUTBUFF," [ ");
		
		for (i = 0; i < fmt->GlyphCount; i++)
			{
			outputgid = fmt->Substitutions[i];
			fprintf(OUTPUTBUFF, " %s", getGlyphName(outputgid, 0));
			}
		if (fmt->GlyphCount > 1)
			fprintf(OUTPUTBUFF," ]");

		fprintf(OUTPUTBUFF, ";\n");
		
		da_FREE(InputGlyphCovList.glyphidlist);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		if (BacktrackGlyphCovListArray.size > 0)
		  da_FREE(BacktrackGlyphCovListArray);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		if (LookaheadGlyphCovListArray.size > 0)
		  da_FREE(LookaheadGlyphCovListArray);	
	}

static void proofReverseChainContext1(ReverseChainContextSubstFormat1 *fmt)
	{
		IntX i, isException = 0;	
		IntX isVert = proofIsVerticalMode();
		IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig, W;
		ttoEnumRec InputGlyphCovList;
		
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);
		
		isException = (fmt->GlyphCount == 0);

		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  proofSymbol(proofctx, PROOF_NOTELEM);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* Backtrack is enumerated in the feaure syntax in reverse order form the offset order. */
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(BacktrackGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;

				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								: 
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),  
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);

			if (i < (fmt->BacktrackGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if (fmt->BacktrackGlyphCount > 0)
		  proofSymbol(proofctx, PROOF_PLUS); /* there is always an input glyph */

		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = &InputGlyphCovList;
			ttoEnumerateCoverage(fmt->InputCoverage, fmt->_InputCoverage, ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, 
							   ((isException) ? 
							   (ANNOT_SHOWIT|
								(isVert ? ((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								 :
								 ((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)))
							   : 
							   (ANNOT_SHOWIT|
								(isVert ? ((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								 : 
								 ((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM))
								|ANNOT_EMPHASIS)),    
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
		proofSymbol(proofctx, PROOF_PRIME);

		  }

		if (fmt->LookaheadGlyphCount > 0)
		  proofSymbol(proofctx, PROOF_PLUS);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			char name[MAX_NAME_LEN];
			ttoEnumRec *ter = da_INDEX(LookaheadGlyphCovListArray, i);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter, &nitems);
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems; j++)
			  {
				gId = *da_INDEX(ter->glyphidlist, j);
				strcpy(name, getGlyphName(gId, 1));
				getMetrics(gId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				if (isVert)
				  {
					if (vwidth != 0) W = vwidth;
					else  W = width;
				  }
				else
				  W = width;
				proofDrawGlyph(proofctx, 
							   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								:
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W, 0, /* width,widthflags */
							   NULL, yorig, ""
							   );
				if (j < (IntX)(nitems - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->LookaheadGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if (isException) 
		  {
			proofThinspace(proofctx, 2);
			proofSymbol(proofctx, PROOF_COLON);
			proofNewline(proofctx);
			return;
		  }
		
			
/* -------------------------------------------------------->>  */
		proofSymbol(proofctx, PROOF_YIELDS);

		if (fmt->GlyphCount > 1)
		  proofSymbol(proofctx, PROOF_LPAREN);

		for (i = 0; i < fmt->GlyphCount; i++)
			{
			GlyphId	gId = fmt->Substitutions[i];
			IntX owid, OW;
			char oname[MAX_NAME_LEN];
			strcpy(oname, getGlyphName(gId, 1));
			getMetrics(gId, &origShift, &lsb, &rsb, &owid, &tsb, &bsb, &vwidth, &yorig);
			if (isVert)
			  {
				if (vwidth != 0) OW = vwidth;
				else  OW = owid;
			  }
			else
			  OW = owid;
			proofDrawGlyph(proofctx, 
						   gId, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
						   oname, ANNOT_SHOWIT|
						   (isVert ?
							((i % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
							:
							((i % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM))
						   |ANNOT_EMPHASIS,    
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   OW, 0, /* width,widthflags */
						   NULL, yorig, ""
						   );
			if (i < fmt->GlyphCount -1)
			  proofThinspace(proofctx, 1);
			
			}
			
		if (fmt->GlyphCount > 1)
		  proofSymbol(proofctx, PROOF_RPAREN);
			

		da_FREE(InputGlyphCovList.glyphidlist);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		if (fmt->BacktrackGlyphCount > 0)
		  da_FREE(BacktrackGlyphCovListArray);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		if (fmt->LookaheadGlyphCount > 0)
		  da_FREE(LookaheadGlyphCovListArray);	
		
	}

static void dumpReverseChainContext1(ReverseChainContextSubstFormat1 *fmt, IntX level)
	{
	IntX i;

	if (level == 8)
		proofReverseChainContext1(fmt);
	else if (level==7){
		decompileReverseChainContext1(fmt);
	}
	else
	  {
		DLu(2, "SubstFormat   =", fmt->SubstFormat);
		DLx(2, "Coverage       =", fmt->InputCoverage);
		ttoDumpCoverage(fmt->InputCoverage, fmt->_InputCoverage, level);
		DLu(2, "BacktrackGlyphCount =", fmt->BacktrackGlyphCount);
		DL(2, (OUTPUTBUFF, "--- BacktrackCoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Backtrack[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			ttoDumpCoverage(fmt->Backtrack[i], fmt->_Backtrack[i], level);
		  }
		
		DLu(2, "LookaheadGlyphCount =", fmt->LookaheadGlyphCount);
		DL(2, (OUTPUTBUFF, "--- LookaheadCoverageArray[index]=offset\n"));
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->Lookahead[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			ttoDumpCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], level);
		  }
		
		DL(2, (OUTPUTBUFF, "\n"));
		DLu(2, "GlyphCount =", fmt->GlyphCount);
		DL(2, (OUTPUTBUFF, "--- Substitutions\n"));
		for (i = 0; i < fmt->GlyphCount; i++) 
		  {
				if (level < 4) 
				  DL(2, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->Substitutions[i]));
				else
				  DL(4, (OUTPUTBUFF, "[%d]=%hu (%s) ", i ,fmt->Substitutions[i], getGlyphName(fmt->Substitutions[i], 0)));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
	  }
	}

static void dumpReverseChainContext(void *fmt, IntX level, void* feattag)
	{
	  unsigned int format = ((ReverseChainContextSubstFormat1 *)fmt)->SubstFormat;
	  GSUBContextRecursionCnt++;
	 if (GSUBContextRecursionCnt > 1)
		{
		featureWarning(level,  SPOT_MSG_CNTX_RECURSION, format);
		return;
		}

	  switch (format)
		{
		case 1:
			dumpReverseChainContext1(fmt, level);
			break;
		default:
			DL(2, (OUTPUTBUFF, "Error. ReverseChainingContextSubst %u not supported.\n", format));
		
		}
	  GSUBContextRecursionCnt--;
	}



static IntX ChainContextSubstFlavor(void *fmt)
	{
	  return (((ChainContextSubstFormat1 *)fmt)->SubstFormat);
	}


/* To handle extended lookups correctly*/
static LOffset prev_offset=0;

static void dumpSubtable(LOffset offset, Card16 type, void *subtable, 
						 IntX level, void *feattag, IntX lookupListIndex, IntX subTableIndex, IntX subTableCount, IntX recursion)
	{
	 IntX i;
	  char tagstr[5]; 
	  Card32 scripttag=0, langtag=0, featuretag=0;

	  if (subtable == NULL)
		return;
#if __CENTERLINE__
centerline_stop();
#endif
	
	if (prev_offset!=0)
		offset=offset+prev_offset;
	else
		prev_offset=offset;
	
	DL(2, (OUTPUTBUFF, "--- Subtable [%d] (%08x)\n", subTableIndex, offset));
	  if (!recursion)
		{
		GSUBSubtableindex = subTableIndex; /* store for later use */
		GSUBSubtableCnt = subTableCount; /* store for later use */
		GSUBLookupIndex = lookupListIndex; /* store for later use */
		}
		
	  if ((Card32 *)feattag != NULL)
		{
		  featuretag = ((Card32 *)feattag)[0];
		  scripttag = ((Card32 *)feattag)[1];
		  langtag = ((Card32 *)feattag)[2];
		  
		  sprintf(tagstr,"%c%c%c%c", TAG_ARG(featuretag));
		}
	  else
		{
		  featuretag = 0;
		  scripttag=0;
		  langtag=0;
		  tagstr[0] = 0x20;
		  tagstr[1] = 0x20;
		  tagstr[2] = 0x20;
		  tagstr[3] = 0x20;
		  tagstr[4] = 0;
		}

	if ( (proofctx != NULL) && (featuretag != prev_featuretag) ) /* new feature */
		proofDestroyContext(&proofctx);

	switch (type)
		{
	case SingleSubsType:
	  if (level != 8)
		dumpSingle(subtable, level);
	  else
		{
		  if (isVerticalFeature(featuretag, SingleSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										STDPAGE_TOP, STDPAGE_BOTTOM,
										dumpTitle(featuretag, 
												  SingleSubstFlavor(subtable)),
										proofCurrentGlyphSize(), 
										0.0, unitsPerEm, 
										0, 1, tagstr);
		  if( (!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		
		  prev_featuretag = featuretag;
		  dumpSingle(subtable, level);

		  if (isVerticalFeature(featuretag, SingleSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;

	case OverflowSubsType:
	  if (level != 8)
		dumpOverflow(subtable, level, feattag);
	  else
		{
		  if (isVerticalFeature(featuretag, OverflowSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										STDPAGE_TOP, STDPAGE_BOTTOM,
										dumpTitle(featuretag, 
												  OverflowSubstFlavor(subtable)),
										proofCurrentGlyphSize(), 
										0.0, unitsPerEm, 
										0, 1, tagstr);
		  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		
		  prev_featuretag = featuretag;
		  dumpOverflow(subtable, level, feattag);

		  if (isVerticalFeature(featuretag, OverflowSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;


	case MultipleSubsType:
	  if (level != 8)
		dumpMultiple(subtable, level);
	  else
		{
		  if (isVerticalFeature(featuretag, MultipleSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												MultipleSubstFlavor(subtable)),
										proofCurrentGlyphSize(), 
										0.0, unitsPerEm, 
										0, 1, tagstr);
		  if ((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpMultiple(subtable, level);

		  if (isVerticalFeature(featuretag, MultipleSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;

	case AlternateSubsType:
	  if (level != 8)
		dumpAlternate(subtable, level);
	  else
		{
		  if (isVerticalFeature(featuretag, AlternateSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												AlternateSubstFlavor(subtable)),
									  proofCurrentGlyphSize(),
									  0.0, unitsPerEm, 
									  0, 1, tagstr);
		  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpAlternate(subtable, level);

		  if (isVerticalFeature(featuretag, AlternateSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;

	case LigatureSubsType:
	  if (level != 8)
		dumpLigature(subtable, level);
	  else
		{
		  if (isVerticalFeature(featuretag, LigatureSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												LigatureSubstFlavor(subtable)),
									  proofCurrentGlyphSize(),
									  0.0, unitsPerEm, 
									  0, 1, tagstr);
		  if((!recursion) &&  (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpLigature(subtable, level);

		  if (isVerticalFeature(featuretag, LigatureSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;

	case ContextSubsType:
	  if (level != 8)
		dumpContext(subtable, level, feattag);
	  else
		{
		  if (isVerticalFeature(featuretag, ContextSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												ContextSubstFlavor(subtable)),
									  proofCurrentGlyphSize(),
									  0.0, unitsPerEm, 
									  0, 1, tagstr);
		  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpContext(subtable, level, feattag);

		  if (isVerticalFeature(featuretag, ContextSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;

	case ChainingContextSubsType:
	  if (level == 5)
		{
		  if( (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag)){
				prev_featuretag = featuretag;
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
				for (i=0; i< GSUBLookupCnt; i++)
					{
					seenChainLookup[i].seen=0;
					seenChainLookup[i].parent=0;
					seenChainLookup[i].cur_parent=0;
					}
			  }
			if (subTableIndex == 0)		
				DL5((OUTPUTBUFF, "->("));
			dumpChainContext(subtable, level, feattag);
			if (subTableIndex == (subTableCount-1))		
				DL5((OUTPUTBUFF, " ) "));
		}
	  else if (level != 8)
		{
		dumpChainContext(subtable, level, feattag);
		}
	  else
		{
		  if (isVerticalFeature(featuretag, ChainContextSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												ChainContextSubstFlavor(subtable)),
									  proofCurrentGlyphSize(),
									  0.0, unitsPerEm, 
									  0, 1, tagstr);
		  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpChainContext(subtable, level, feattag); 

		  if (isVerticalFeature(featuretag, ChainContextSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
	  break;
	  
	case ReverseChainContextType:
		{
	  if (level == 5)
		{
		  if( (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag)){
				prev_featuretag = featuretag;
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
				for (i=0; i< GSUBLookupCnt; i++)
					{
					seenChainLookup[i].seen=0;
					seenChainLookup[i].parent=0;
					seenChainLookup[i].cur_parent=0;
					}
			  }
			if (subTableIndex == 0)		
				DL5((OUTPUTBUFF, "->("));
			dumpReverseChainContext(subtable, level, feattag);
			if (subTableIndex == (subTableCount-1))		
				DL5((OUTPUTBUFF, " ) "));
		}
	  else if (level != 8)
		{
		dumpReverseChainContext(subtable, level, feattag);
		}
	  else
		{
		  if (isVerticalFeature(featuretag, ChainContextSubstFlavor(subtable)))
			  proofSetVerticalMode();
		  if (proofctx == NULL) /* new feature */
			proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									  STDPAGE_TOP, STDPAGE_BOTTOM,
									  dumpTitle(featuretag, 
												ChainContextSubstFlavor(subtable)),
									  proofCurrentGlyphSize(),
									  0.0, unitsPerEm, 
									  0, 1, tagstr);
		  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || lookupListIndex!=(IntX)prev_lookupListIndex)){
		  		char message[100];
		  		
		  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
		  		proofMessage(proofctx, message);
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
		  		prev_lookupListIndex=lookupListIndex;
		  }
		  prev_featuretag = featuretag;
		  dumpReverseChainContext(subtable, level, feattag); 

		  if (isVerticalFeature(featuretag, ChainContextSubstFlavor(subtable)))
			  proofUnSetVerticalMode();
		  featuretag = 0;
		}
		break;
		}
	default:
	/* ReverseChainContextType */
		DL(2, (OUTPUTBUFF, "Error. LookupType %d not supported.\n", type));
	
		} /* switch */
	DL(2, (OUTPUTBUFF, " \n"));
	prev_offset=0;
	}

void GSUBDump(IntX level, LongN start)
	{
		IntX i;
		LookupList *lookuplist;
		
	   contextPrefix[0] = 0;
		GSUBLookupIndex = 0;
		GSUBContextRecursionCnt = 0;
		
	  if (!loaded) 
		{
		  if (sfntReadTable(GSUB_))
			return;
		}
		lookuplist =  &GSUB._LookupList;
		GSUBLookupCnt = lookuplist->LookupCount;
		seenChainLookup = (SeenChainLookup *)memNew(GSUBLookupCnt*sizeof(SeenChainLookup));
		for (i=0; i< GSUBLookupCnt; i++)
			{
			seenChainLookup[i].seen=0;
			seenChainLookup[i].parent=0;
			seenChainLookup[i].cur_parent=0;
			}
			
	  if (level == 5) /* dump features list */
		{
		  fprintf(OUTPUTBUFF,  "GSUB Features:\n");
		  ttoDumpFeaturesByScript(&GSUB._ScriptList, &GSUB._FeatureList, &GSUB._LookupList, dumpSubtable, level);
		  /*ttoDumpFeatureList(GSUB.FeatureList, &GSUB._FeatureList, level);*/
		  return;
		}

	  headGetUnitsPerEm(&unitsPerEm, GSUB_);

	  if ((opt_Present("-P") && level!=7) || (level == 8)) /* make a proof file */
		{
		  ttoDumpLookupifFeaturePresent(&GSUB._LookupList,
										&GSUB._FeatureList,
										&GSUB._ScriptList,
										8,
										dumpSubtable, dumpMessage, NULL);
		}
	  else if (level == 7) /* dump a features-format file */
	  	{
	  	  /* dump FeatureLookup-subtables according to Script */
	  	  ttoDecompileByScript(&GSUB._ScriptList, &GSUB._FeatureList, &GSUB._LookupList, dumpSubtable, 7);
	  	}
	  else /* text format dump */ 
		{
		  DL(1, (OUTPUTBUFF, "### [GSUB] (%08lx)\n", start));
		  
		  DLV(2, "Version    =", GSUB.Version);
		  DLx(2, "ScriptList =", GSUB.ScriptList);
		  DLx(2, "FeatureList=", GSUB.FeatureList);
		  DLx(2, "LookupList =", GSUB.LookupList);
		  
		  ttoDumpScriptList(GSUB.ScriptList, &GSUB._ScriptList, level);
		  ttoDumpFeatureList(GSUB.FeatureList, &GSUB._FeatureList, level);
		  ttoDumpLookupList(GSUB.LookupList, &GSUB._LookupList, level,
							dumpSubtable);
		}
		
	  memFree(seenChainLookup);

		if (proofctx)
		  proofDestroyContext(&proofctx);
	}

static void evalSingle(void *fmt,
					   IntX numinputglyphs, GlyphId *inputglyphs, 
					   IntX *numoutputglyphs, GlyphId *outputglyphs)
	{
	  SingleSubstFormat1 *fmt1;
	  SingleSubstFormat2 *fmt2;
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, at;
	  GlyphId inputglyphId;
	  IntX formattype = ((SingleSubstFormat1 *)fmt)->SubstFormat;
	  
	  if (numinputglyphs > 1)
		{
		  warning(SPOT_MSG_GSUBEVALCNT);
		  goto foo;
		}

	  switch (formattype)
		{
		case 1:
		  fmt1 = (SingleSubstFormat1 *)fmt;
		  ttoEnumerateCoverage(fmt1->Coverage, fmt1->_Coverage, &CovList, &nitems);
		  break;
		case 2:
		  fmt2 = (SingleSubstFormat2 *)fmt;
		  ttoEnumerateCoverage(fmt2->Coverage, fmt2->_Coverage, &CovList, &nitems);
		  break;
		}
	  /* find the position of the input glyph in the Coverage list */
	  at = (-1);
	  inputglyphId = inputglyphs[0];
	  for (i = 0; i < (IntX)nitems; i++)
		{
		  if (inputglyphId == *da_INDEX(CovList.glyphidlist, i))
			{
			  at = i; break;
			}
		}
	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  if (at < 0)
		{
		  warning(SPOT_MSG_GSUBNOCOVG, inputglyphId, getGlyphName(inputglyphId, 0));
		  goto foo;
		}

	  switch (formattype)
		{
		case 1:
		  outputglyphs[0] = inputglyphId + fmt1->DeltaGlyphId;
		  *numoutputglyphs = 1;
		  return;
		  break;
		case 2:
		  outputglyphs[0] = fmt2->Substitute[at];
		  *numoutputglyphs = 1;
		  return;
		  break;
		}
	  
foo:
	  *numoutputglyphs = 0;
	}
	
static void evalMultiple(MultipleSubstFormat1 *fmt,
					   IntX numinputglyphs, GlyphId *inputglyphs, 
					   IntX *numoutputglyphs, GlyphId *outputglyphs)
	{
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, at;
	  GlyphId inputglyphId;
	  
	  if (numinputglyphs > 1)
		{
		  warning(SPOT_MSG_GSUBEVALCNT);
		  goto foo;
		}

	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	  /* find the position of the input glyph in the Coverage list */
	  at = (-1);
	  inputglyphId = inputglyphs[0];
	  for (i = 0; i < (IntX)nitems; i++)
		{
		  if (inputglyphId == *da_INDEX(CovList.glyphidlist, i))
			{
			  at = i; break;
			}
		}
	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  if (at < 0)
		{
		  warning(SPOT_MSG_GSUBNOCOVG, inputglyphId, getGlyphName(inputglyphId, 0));
		  goto foo;
		}

		{
		 Sequence *sequence = &fmt->_Sequence[at];
		 *numoutputglyphs = sequence->GlyphCount;
		  	for (i = 0; i < sequence->GlyphCount; i++)
		 	 {
		 	 outputglyphs[i] = sequence->Substitute[i];
			  }
		}
	  return;
	  
foo:
	  *numoutputglyphs = 0;
	}
	
static void evalOverflow(void *fmt,
					   IntX numinputglyphs, GlyphId *inputglyphs, 
					   IntX *numoutputglyphs, GlyphId *outputglyphs)
	{
	  OverflowSubstFormat1 *fmt1;
	  IntX formattype = ((OverflowSubstFormat1 *)fmt)->SubstFormat;
	  switch (formattype)
		{
		case 1:
			fmt1=(OverflowSubstFormat1 *)fmt;
			evalSubtable(fmt1->OverflowLookupType, fmt1->subtable, numinputglyphs, 
						inputglyphs, numoutputglyphs, outputglyphs);
			break;
		}
	}
	
/*static int evalSingleiP(void *fmt,
					   IntX startglyph, IntX *numglyphs, GlyphRec *glyphs)
	{
	  SingleSubstFormat1 *fmt1;
	  SingleSubstFormat2 *fmt2;
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, at;
	  GlyphId inputglyphId;
	  IntX formattype = ((SingleSubstFormat1 *)fmt)->SubstFormat;
	  
	  switch (formattype)
		{
		case 1:
		  fmt1 = (SingleSubstFormat1 *)fmt;
		  ttoEnumerateCoverage(fmt1->Coverage, fmt1->_Coverage, &CovList, &nitems);
		  break;
		case 2:
		  fmt2 = (SingleSubstFormat2 *)fmt;
		  ttoEnumerateCoverage(fmt2->Coverage, fmt2->_Coverage, &CovList, &nitems);
		  break;
		}
	   at = (-1);
	  inputglyphId = *glyphs[startglyph].gid;
	  for (i = 0; i < nitems; i++)
		{
		  if (inputglyphId == *da_INDEX(CovList.glyphidlist, i))
			{
			  at = i; break;
			}
		}
	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  if (at < 0)
		{
		  warning(SPOT_MSG_GSUBNOCOVG, inputglyphId);
		  goto foo;
		}

	  switch (formattype)
		{
		case 1:
		  *glyphs[startglyph].gid = inputglyphId + fmt1->DeltaGlyphId;
		  glyphs[startglyph].startPos=-1;
		  return 1;
		  break;
		case 2:
		  *glyphs[startglyph].gid = fmt2->Substitute[at];
		  glyphs[startglyph].startPos=-1;
		  return 1;
		  break;
		}
	  
foo:
	  return 0;
	}
*/
static void evalLigature(void *fmt,
						 IntX numinputglyphs, GlyphId *inputglyphs, 
						 IntX *numoutputglyphs, GlyphId *outputglyphs)
	{
	  LigatureSubstFormat1 *fmt1;
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, at, inputcount;
	  GlyphId inputglyphId, glyphId2;
	  IntX ligId;
	  IntX formattype = ((LigatureSubstFormat1 *)fmt)->SubstFormat;
	  LigatureSet *ligatureSet;

	  switch (formattype)
		{
		case 1:
		  fmt1 = (LigatureSubstFormat1 *)fmt;		  
		  ttoEnumerateCoverage(fmt1->Coverage, fmt1->_Coverage, &CovList, &nitems);
		  break;
		default:
		  goto foo;
		  break;
		}
	  /* find the positions of the input glyphs in the Coverage list */
	  inputcount = 0;
	  at = (-1); 
	  inputglyphId = inputglyphs[inputcount];
	  for (i = 0; i < (IntX)nitems; i++)
		{
		  if (inputglyphId == *da_INDEX(CovList.glyphidlist, i))
			{
			  at = i; 
			  inputcount++; 
			  break;
			}
		}
	  if (at < 0)
		{
		  warning(SPOT_MSG_GSUBNOCOVG, inputglyphId, getGlyphName(inputglyphId, 0));
		  goto foo;
		}

	  ligatureSet = &fmt1->_LigatureSet[at];
	  ligId = (-1);
	  for (i = 0; i < ligatureSet->LigatureCount; i++)
		{
		  IntX k;
		  Ligature *ligature = &ligatureSet->_Ligature[i];

		  for (k = 0; k < ligature->CompCount - 1; k++)
			{
			  glyphId2 = ligature->Component[k];
			  if (inputglyphs[inputcount] != glyphId2) 
				break;
			  else 
				inputcount++;
			}

		  if (inputcount == numinputglyphs)
			{
			  ligId = ligature->LigGlyph;
			  break;
			}else{
				inputcount=1; /*Reset to 1 since we already matched first input glyph but didn't match whole ligature*/
			}
		}

	  if (ligId < 0)
		goto foo;

	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  outputglyphs[0] = ligId;
	  *numoutputglyphs = 1;
	  return;
	  
foo:
	  *numoutputglyphs = 0;
	}
	
/*static int evalLigatureiP(void *fmt,
						 IntX startglyph, IntX *numglyphs, GlyphRec *glyphs)
	{
	  LigatureSubstFormat1 *fmt1;
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, at, inputcount;
	  GlyphId inputglyphId, glyphId2;
	  IntX ligId;
	  IntX formattype = ((LigatureSubstFormat1 *)fmt)->SubstFormat;
	  LigatureSet *ligatureSet;

	  switch (formattype)
		{
		case 1:
		  fmt1 = (LigatureSubstFormat1 *)fmt;		  
		  ttoEnumerateCoverage(fmt1->Coverage, fmt1->_Coverage, &CovList, &nitems);
		  break;
		default:
		  goto foo;
		  break;
		}
	  
	  at = (-1); 
	  inputcount=0;
	  inputglyphId = *glyphs[startglyph].gid;
	  for (i = 0; i < nitems; i++)
		{
		  if (inputglyphId == *da_INDEX(CovList.glyphidlist, i))
			{
			  at = i; 
			  break;
			}
		}
	  if (at < 0)
		{
		  warning(SPOT_MSG_GSUBNOCOVG, inputglyphId);
		  goto foo;
		}

	  ligatureSet = &fmt1->_LigatureSet[at];
	  ligId = (-1);
	  for (i = 0; i < ligatureSet->LigatureCount; i++)
		{
		  IntX k;
		  Ligature *ligature = &ligatureSet->_Ligature[i];

		  for (k = 0; k < ligature->CompCount - 1; k++)
			{
			  glyphId2 = ligature->Component[k];
			  if (startglyph+inputcount>=*numglyphs || *glyphs[startglyph+inputcount].gid != glyphId2) 
				break;
			  else 
				inputcount++;
			}

		  if (inputcount == k-1)
			{
			  ligId = ligature->LigGlyph;
			  break;
			}
		}

	  if (ligId < 0)
		goto foo;

	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  *glyphs[startglyph].gid = ligId;
	  glyphs[startglyph].startPos = -1;
	  
	  memFree(glyphs[startglyph+1].gid);
	  
	  for (i=startglyph+1; i<*numglyphs-inputcount; i++)
	  	glyphs[i]=glyphs[i+inputcount];
	  for(; i<*numglyphs; i++)
	  {
	  	memFree(glyphs[i].gid);
	  	glyphs[i].startPos=-1;
	  	glyphs[i].numGids=0;
	  }	
	  *numglyphs-=inputcount;
	  return 1;
	  
foo:
	  return 0;
	}
*/

static void evalSubtable(Card16 type, void *subtable,
			  IntX numinputglyphs, GlyphId *inputglyphs, 
			  IntX *numoutputglyphs, GlyphId *outputglyphs)
{
  if (subtable == NULL)
	return;

  switch (type)
	{
	case SingleSubsType:
	  evalSingle(subtable, 
				 numinputglyphs, inputglyphs, 
				 numoutputglyphs, outputglyphs);
	  break;

	case LigatureSubsType:
	  evalLigature(subtable, 
				   numinputglyphs, inputglyphs, 
				   numoutputglyphs, outputglyphs);
	  break;
	case OverflowSubsType:
		evalOverflow(subtable, 
				   numinputglyphs, inputglyphs, 
				   numoutputglyphs, outputglyphs);
		break;
		
	case MultipleSubsType:
	  evalMultiple(subtable, 
				 numinputglyphs, inputglyphs, 
				 numoutputglyphs, outputglyphs);
		break;
	
	case AlternateSubsType:
	case ContextSubsType:
	default:
	  warning(SPOT_MSG_GSUBEUNKLOOK, type);
	  *numoutputglyphs = 0;
	  return;
	  break;
	}
}

void GSUBEval(IntX GSUBLookupListIndex, 
			  IntX numinputglyphs, GlyphId *inputglyphs, 
			  IntX *numoutputglyphs, GlyphId *outputglyphs)
{
  Lookup *lookup;

  if (numinputglyphs == 0) 
	{
	  *numoutputglyphs = 0;
	  return;
	}

  lookup = &(GSUB._LookupList._Lookup[GSUBLookupListIndex]);

  if (lookup->SubTableCount > 1)
	{
	  warning(SPOT_MSG_GSUBESUBCNT, GSUBLookupListIndex, lookup->LookupType, lookup->SubTableCount);
	  *numoutputglyphs = 0;
	  return;
	}

  evalSubtable(lookup->LookupType, lookup->_SubTable[0], 
				   numinputglyphs, inputglyphs, 
				   numoutputglyphs, outputglyphs);
}
/*
int GSUBEvaliP(IntX GSUBLookupListIndex, IntX startglyph,IntX *numglyphs, GlyphRec *glyphs)
{
  Lookup *lookup;

  if (startglyph>=*numglyphs) 
	{
	  return 0;
	}

  lookup = &(GSUB._LookupList._Lookup[GSUBLookupListIndex]);

  if (lookup->SubTableCount > 1)
	{
	  warning(SPOT_MSG_GSUBESUBCNT, GSUBLookupListIndex, lookup->LookupType, lookup->SubTableCount);
	  return 0;
	}

  return evalSubtableiP(lookup->LookupType, lookup->_SubTable[0], 
				   startglyph, numglyphs, glyphs);
}*/


static void freeSingleSubsType(void *subtable)
	{
	SingleSubstFormat1 *SingleSubstfmt1;
	SingleSubstFormat2 *SingleSubstfmt2;
	switch (((SingleSubstFormat1 *)(subtable))->SubstFormat)
		{
			case 1:
				SingleSubstfmt1 = (SingleSubstFormat1 *)subtable;
				ttoFreeCoverage(SingleSubstfmt1->_Coverage);					
				memFree(subtable);
				break;
			case 2:
				SingleSubstfmt2 = (SingleSubstFormat2 *)subtable;
				ttoFreeCoverage(SingleSubstfmt2->_Coverage);
				memFree(SingleSubstfmt2->Substitute);
				memFree(subtable);
				break;
		}
	}
	
static void freeMultipleSubsType (void *subtable)
	{
	IntX i;
	MultipleSubstFormat1 *MultipleSubstfmt1;
	switch (((MultipleSubstFormat1 *)(subtable))->SubstFormat)
		{
			case 1:
				MultipleSubstfmt1 = (MultipleSubstFormat1 *)subtable;
				ttoFreeCoverage(MultipleSubstfmt1->_Coverage);
				for (i = 0; i < (IntX)(MultipleSubstfmt1->SequenceCount); i++)
					{
					 memFree(MultipleSubstfmt1->_Sequence[i].Substitute);
					}
				memFree(MultipleSubstfmt1->Sequence);
				memFree(MultipleSubstfmt1->_Sequence);				
				memFree(subtable);
				break;					
		}
	}
	
static void freeAlternateSubsType (void *subtable)
	{
	IntX i;
	AlternateSubstFormat1 *AlternateSubstfmt1;
	switch (((AlternateSubstFormat1 *)(subtable))->SubstFormat)
		{
			case 1:
				AlternateSubstfmt1 = (AlternateSubstFormat1 *)subtable;
				ttoFreeCoverage(AlternateSubstfmt1->_Coverage);
				for (i = 0; i < (IntX)(AlternateSubstfmt1->AlternateSetCnt); i++)
					{
					 memFree(AlternateSubstfmt1->_AlternateSet[i].Alternate);
					}
				memFree(AlternateSubstfmt1->AlternateSet);				
				memFree(AlternateSubstfmt1->_AlternateSet);				
				memFree(subtable);
				break;
		}
	}
	
static void freeLigatureSubsType (void *subtable)
	{
	IntX i, j;
	LigatureSubstFormat1 *LigatureSubstfmt1;
  	switch (((LigatureSubstFormat1 *)(subtable))->SubstFormat)
		{
			case 1:
				LigatureSubstfmt1 = (LigatureSubstFormat1 *)subtable;
				ttoFreeCoverage(LigatureSubstfmt1->_Coverage);	
				for (i = 0; i < (IntX)(LigatureSubstfmt1->LigSetCount); i++)
					{
						LigatureSet *ligatureSet = &LigatureSubstfmt1->_LigatureSet[i];
						for (j = 0; j < (IntX)(ligatureSet->LigatureCount); j++)
							{
							 memFree(ligatureSet->_Ligature[j].Component);
							}
						memFree(ligatureSet->Ligature);
						memFree(ligatureSet->_Ligature);
					}			
				memFree(LigatureSubstfmt1->LigatureSet);				
				memFree(LigatureSubstfmt1->_LigatureSet);
				memFree(subtable);  	
			break;
		}
	}
	
static void freeContextSubsType (void *subtable)
	{
	IntX i, j;
	ContextSubstFormat1 *ContextSubstfmt1;
	ContextSubstFormat2 *ContextSubstfmt2;
	ContextSubstFormat3 *ContextSubstfmt3;
	SubRuleSet *subruleset;		
	SubRule *subrule;	
	SubClassSet *subclassset;
	SubClassRule *subclassrule;
	switch (((ContextSubstFormat1 *)(subtable))->SubstFormat)
	{
		case 1:
			ContextSubstfmt1 = (ContextSubstFormat1 *)subtable;
			ttoFreeCoverage(ContextSubstfmt1->_Coverage);	
			for (i = 0; i < (IntX)(ContextSubstfmt1->SubRuleSetCount); i++) 
				{	
				subruleset = &ContextSubstfmt1->_SubRuleSet[i];
				for (j = 0; j < (IntX)(subruleset->SubRuleCount); j++) 
					{
					 subrule = &subruleset->_SubRule[j];
					 memFree(subrule->Input);
					 memFree(subrule->SubstLookupRecord);
					}
				 memFree(subruleset->SubRule);
				 memFree(subruleset->_SubRule);
				}	
			memFree(ContextSubstfmt1->SubRuleSet);
			memFree(ContextSubstfmt1->_SubRuleSet);
			memFree(subtable);	
			break;
		case 2:
			ContextSubstfmt2 = (ContextSubstFormat2 *)subtable;
			ttoFreeCoverage(ContextSubstfmt2->_Coverage);	
			ttoFreeClass(ContextSubstfmt2->_ClassDef);
			for (i = 0; i < (IntX)(ContextSubstfmt2->SubClassSetCnt); i++) 
				{
				 subclassset = &ContextSubstfmt2->_SubClassSet[i];
				 for (j = 0; j < (IntX)(subclassset->SubClassRuleCnt); j++) 
					{
					 subclassrule = &subclassset->_SubClassRule[j];
					 memFree(subclassrule->Class);
					 memFree(subclassrule->SubstLookupRecord);
					}
				 memFree(subclassset->SubClassRule);
				 memFree(subclassset->_SubClassRule);
				}
			memFree(ContextSubstfmt2->SubClassSet);
			memFree(ContextSubstfmt2->_SubClassSet);
			memFree(subtable);	
			break;
		case 3:
			ContextSubstfmt3 = (ContextSubstFormat3 *)subtable;
			for (i = 0; i < (IntX)(ContextSubstfmt3->GlyphCount); i++)
				{
					ttoFreeCoverage(ContextSubstfmt3->_CoverageArray[i]);
				}
			memFree(ContextSubstfmt3->CoverageArray);
			memFree(ContextSubstfmt3->_CoverageArray);
			memFree(ContextSubstfmt3->SubstLookupRecord);
			memFree(subtable);	
			break;
	}
	}
	
static void freeChainingContextSubsType (void *subtable)
	{
	IntX i, j;
	ChainContextSubstFormat1 *ChainContextSubstfmt1;
	ChainContextSubstFormat2 *ChainContextSubstfmt2;
	ChainContextSubstFormat3 *ChainContextSubstfmt3;
	ChainSubRuleSet *chainsubruleset;
	ChainSubRule *chainsubrule;
	ChainSubClassSet *chainsubclassset;
	ChainSubClassRule *chainsubclassrule;
	switch (((ChainContextSubstFormat1 *)(subtable))->SubstFormat)
	{
		case 1:
			ChainContextSubstfmt1 = (ChainContextSubstFormat1 *)subtable;
			ttoFreeCoverage(ChainContextSubstfmt1->_Coverage);	
			for (i = 0; i < (IntX)(ChainContextSubstfmt1->ChainSubRuleSetCount); i++) 
				{	
				chainsubruleset = &ChainContextSubstfmt1->_ChainSubRuleSet[i];
				for (j = 0; j < (IntX)(chainsubruleset->ChainSubRuleCount); j++) 
					{
					 chainsubrule = &chainsubruleset->_ChainSubRule[j];
					 memFree(chainsubrule->Backtrack);
					 memFree(chainsubrule->Input);
					 memFree(chainsubrule->Lookahead);
					 memFree(chainsubrule->SubstLookupRecord);
					}
				 memFree(chainsubruleset->ChainSubRule);
				 memFree(chainsubruleset->_ChainSubRule);
				}	
			memFree(ChainContextSubstfmt1->ChainSubRuleSet);
			memFree(ChainContextSubstfmt1->_ChainSubRuleSet);
			memFree(subtable);	
			break;
		case 2:
			ChainContextSubstfmt2 = (ChainContextSubstFormat2 *)subtable;
			ttoFreeCoverage(ChainContextSubstfmt2->_Coverage);	
			ttoFreeClass(ChainContextSubstfmt2->_BackTrackClassDef);
			ttoFreeClass(ChainContextSubstfmt2->_InputClassDef);
			ttoFreeClass(ChainContextSubstfmt2->_LookAheadClassDef);
			for (i = 0; i < (IntX)(ChainContextSubstfmt2->ChainSubClassSetCnt); i++) 
				{
				 chainsubclassset = &ChainContextSubstfmt2->_ChainSubClassSet[i];
				 for (j = 0; j < (IntX)(chainsubclassset->ChainSubClassRuleCnt); j++) 
					{
					 chainsubclassrule = &chainsubclassset->_ChainSubClassRule[j];
					 memFree(chainsubclassrule->Backtrack);
					 memFree(chainsubclassrule->Input);
					 memFree(chainsubclassrule->Lookahead);
					 memFree(chainsubclassrule->SubstLookupRecord);
					}
				 memFree(chainsubclassset->ChainSubClassRule);
				 memFree(chainsubclassset->_ChainSubClassRule);
				}
			memFree(ChainContextSubstfmt2->ChainSubClassSet);
			memFree(ChainContextSubstfmt2->_ChainSubClassSet);
			memFree(subtable);	
			break;
		case 3:
			ChainContextSubstfmt3 = (ChainContextSubstFormat3 *)subtable;
			for (i = 0; i < (IntX)(ChainContextSubstfmt3->BacktrackGlyphCount); i++)
				{
					ttoFreeCoverage(ChainContextSubstfmt3->_Backtrack[i]);
				}
			memFree(ChainContextSubstfmt3->Backtrack);
			memFree(ChainContextSubstfmt3->_Backtrack);
			for (i = 0; i < (IntX)(ChainContextSubstfmt3->InputGlyphCount); i++)
				{
					ttoFreeCoverage(ChainContextSubstfmt3->_Input[i]);
				}
			memFree(ChainContextSubstfmt3->Input);
			memFree(ChainContextSubstfmt3->_Input);
			for (i = 0; i < (IntX)(ChainContextSubstfmt3->LookaheadGlyphCount); i++)
				{
					ttoFreeCoverage(ChainContextSubstfmt3->_Lookahead[i]);
				}
			memFree(ChainContextSubstfmt3->Lookahead);
			memFree(ChainContextSubstfmt3->_Lookahead);
			memFree(ChainContextSubstfmt3->SubstLookupRecord);		
			memFree(subtable);	
			break;
	}
	}
	

static void freeReverseChainingContextSubsType (void *subtable)
	{
	IntX i;
	ReverseChainContextSubstFormat1 *ReverseChainContextSubstfmt1;
	switch (((ReverseChainContextSubstFormat1 *)(subtable))->SubstFormat)
		{
		case 1:
			ReverseChainContextSubstfmt1 = (ReverseChainContextSubstFormat1 *)subtable;
			ttoFreeCoverage(ReverseChainContextSubstfmt1->_InputCoverage);	
			for (i = 0; i < (IntX)(ReverseChainContextSubstfmt1->BacktrackGlyphCount); i++)
				{
					ttoFreeCoverage(ReverseChainContextSubstfmt1->_Backtrack[i]);
				}
			memFree(ReverseChainContextSubstfmt1->Backtrack);
			memFree(ReverseChainContextSubstfmt1->_Backtrack);
			for (i = 0; i < (IntX)(ReverseChainContextSubstfmt1->LookaheadGlyphCount); i++)
				{
					ttoFreeCoverage(ReverseChainContextSubstfmt1->_Lookahead[i]);
				}
			memFree(ReverseChainContextSubstfmt1->Lookahead);
			memFree(ReverseChainContextSubstfmt1->_Lookahead);
			memFree(ReverseChainContextSubstfmt1->Substitutions);		
			memFree(subtable);	
			break;
		}
	}
	

static void freeSubtable(Card16 type, void *subtable)
	{
			
	if (subtable == NULL)
		return;
	switch (type)
		{
	case SingleSubsType:
		freeSingleSubsType(subtable);
		break;
	case MultipleSubsType:
		freeMultipleSubsType (subtable);
		break;
	case AlternateSubsType:
		freeAlternateSubsType (subtable);
		break;
	case LigatureSubsType:
		freeLigatureSubsType (subtable);
		break;
	case ContextSubsType:
		freeContextSubsType (subtable);
		break;
	case ChainingContextSubsType:
		freeChainingContextSubsType (subtable);
		break;
	case ReverseChainContextType:
		freeReverseChainingContextSubsType (subtable);
		break;
	default:
		break;
		}
	}

void GSUBFree(void)
	{
	  if (!loaded)
		return;

	ttoFreeScriptList(&GSUB._ScriptList);
	ttoFreeFeatureList(&GSUB._FeatureList);
	ttoFreeLookupList(&GSUB._LookupList, freeSubtable);
	loaded = 0;
	contextPrefix[0] = 0;
	/*featuretag = 0;*/
	prev_featuretag = 0;
	}

void GSUBUsage(void)
	{
	  fprintf(OUTPUTBUFF,  "--- GSUB\n"
			 "=4  output GlyphName/CID in dumps\n"
			 "=5  List GSUB Features\n"
		 	 "=7  De-compile GSUB feature(s) in features-file style\n"
			 "=8  Proof GSUB features\n");

	}

