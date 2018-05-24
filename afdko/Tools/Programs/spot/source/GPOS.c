/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GPOS.c	1.26
 * Changed:    7/14/99 09:24:32
 ***********************************************************************/

#include <string.h>
#include "tto.h"
#include "sfnt_GPOS.h"

#include "GPOS.h"
#include "sfnt.h"
#include "head.h"
#include "proof.h"
#include "MMFX.h"
#include "bitstr.h"

#define MAXAFMLINESIZE 80
#define kScriptLabel "Script"
#define kScriptLabelLen 6

typedef struct {
		char kernPair[MAXAFMLINESIZE];
		int kernValue;
		int kernValue2;
		int	isClass;
		int	isDuplicate;
		} kernPairEntry;
	
extern char VORGfound;

static GPOSTbl GPOS;
static IntX loaded = 0;
static Card32 GPOSStart;

static ProofContextPtr proofctx = NULL;
static Card32 featuretag = 0;
static Card32 prev_featuretag = 0;
static Card32 prev_scripttag = 0;
static Card32 prev_langtag = 0;
static Card32 prev_lookupListIndex=0;
static Card16 unitsPerEm;
static IntX GPOStableindex = 0;
static IntX GPOStableCount = 0;
static IntX GPOSLookupIndex = 0;
static IntX GPOSLookupCnt = 0;
static IntX GPOSContextRecursionCnt = 0;

static Byte8 contextPrefix[MAX_NAME_LEN]; /* when dumping the context sub rules, this cntains the context string for the rule, if any. */

static FILE * AFMout;
static char* tempFileName;

static void *readSubtable(Card32 offset, Card16 type);

static IntX searchSubtable(const Card16 lookuptype, void *subtable, 
						   const GlyphId glyph, const GlyphId glyph2_or0, 
						   Card16 *vfmt, ValueRecord *vr, 
						   Card16 *vfmt2, ValueRecord *vr2_or0);
static void dumpSubtable(LOffset offset, Card16 type, void *subtable, 
			 IntX level, void *feattag, IntX lookupListIndex, IntX subTableIndex, IntX subtableCount, IntX recursion);

typedef struct {
		Card16 type;
		IntX i; /*Index of glyph in Coverage*/
		IntX j; /*Index of PVR with pairset (if pair)*/
		GlyphId g1, g2;
		IntX c2; /*Index of classe 2 for PosPairFormat2*/
		IntX vert;
		void *fmt;
} ProofRec;				/*To be used to store proof attempts for sorting*/

static da_DCL( ProofRec, proofrecords);
static IntX curproofrec=-1;

typedef struct {
	IntX seen;
	IntX parent;
	IntX cur_parent;
	} SeenChainLookup;

static SeenChainLookup * seenChainLookup;

static IntX GPOSlookupFeatureGlyph(Tag feattag, const GlyphId glyph, Card16 *vfmt, ValueRecord *vr)
	{
	 /* if there is a feature applicable to this glyph, return its valuerecord */
	  IntX i, j;
	  FeatureList *featurelist = &(GPOS._FeatureList);
	  
	  for (i = 0; i < featurelist->FeatureCount; i++)
		{
		  FeatureRecord *record = &featurelist->FeatureRecord[i];
		  Feature *feature = &record->_Feature;

		  if (record->FeatureTag == feattag)
			{
			  for (j = 0; j < feature->LookupCount; j++)
				{
				  Lookup *lookup = NULL;
				  IntX sindex, index;

				  index = feature->LookupListIndex[j];
				  lookup = &GPOS._LookupList._Lookup[index];
				  if (!lookup) 
				  	continue;
				  for (sindex = 0; sindex < lookup->SubTableCount; sindex++)
					{
					  if (searchSubtable(lookup->LookupType, (void *)(lookup->_SubTable[sindex]), glyph, 0, vfmt, vr, 0, NULL))
						return 1;
					}
				}
			}
		}
	  return 0; /* not found */
	}

static IntX GPOSlookupFeatureGlyphPair(Tag feattag, const GlyphId glyph1, const GlyphId glyph2, 
										Card16 *vfmt1, ValueRecord *vr1, Card16 *vfmt2, ValueRecord *vr2)
	{
	 /* if there is a feature applicable to this glyph, return its valuerecord */
	  IntX i, j, index;
	  FeatureList *featurelist = &(GPOS._FeatureList);
	  
	  for (i = 0; i < featurelist->FeatureCount; i++)
		{
		  FeatureRecord *record = &featurelist->FeatureRecord[i];
		  Feature *feature = &record->_Feature;

		  if (record->FeatureTag == feattag)
			{
			  for (j = 0; j < feature->LookupCount; j++)
				{
				  Lookup *lookup;
				  IntX sindex;

				  index = feature->LookupListIndex[j];
				  lookup = &GPOS._LookupList._Lookup[index];
				  for (sindex = 0; sindex < lookup->SubTableCount; sindex++)
					{
					  if (searchSubtable(lookup->LookupType, (void *)(lookup->_SubTable[sindex]), glyph1, glyph2, vfmt1, vr1, vfmt2, vr2))
						return 1;
					}
				}
			}
		}
	  return 0; /* not found */
	}

static void initProofRecs()
{
	if (curproofrec==-1)
	{
  		da_INIT(proofrecords, 1000, 100);
	}
	curproofrec=0;
	proofrecords.cnt = 0;
}



static void freeProofRecs()
{
	if (proofrecords.size > 0)
		{
	  	da_FREE(proofrecords);
	  	proofrecords.size = proofrecords.cnt = 0;
	  	}
}


static void clearValueRecord(ValueRecord *vr)
	{	
	  vr->XPlacement =
		vr->YPlacement =
		  vr->XAdvance =
			vr->YAdvance = 0;
      vr->XPlaDevice =
        vr->YPlaDevice =
		  vr->XAdvDevice =
			vr->YAdvDevice = 0;
	}

static void copyValueRecord(ValueRecord *dest, ValueRecord *src)
	{
	  dest->XPlacement = src->XPlacement;
	  dest->YPlacement = src->YPlacement;
	  dest->XAdvance =   src->XAdvance;
	  dest->YAdvance =   src->YAdvance;
      dest->XPlaDevice = src->XPlaDevice;
	  dest->YPlaDevice = src->YPlaDevice;
	  dest->XAdvDevice = src->XAdvDevice;
	  dest->YAdvDevice = src->YAdvDevice;
	}

static void readValueRecord(Card16 fmt, ValueRecord *vr)
	{
	  clearValueRecord(vr);	  /* INIT */

	  if (fmt == 0) return;
	  /* order is important here: */
	  if ((fmt & ValueXPlacement) || (fmt & ValueXIdPlacement))
		IN1(vr->XPlacement);
	  if ((fmt & ValueYPlacement) || (fmt & ValueYIdPlacement))
		IN1(vr->YPlacement);
	  if ((fmt & ValueXAdvance) || (fmt & ValueXIdAdvance))
		IN1(vr->XAdvance);
	  if ((fmt & ValueYAdvance) || (fmt & ValueYIdAdvance))
		IN1(vr->YAdvance);
	  if (fmt & ValueXPlaDevice)
		IN1(vr->XPlaDevice);
	  if (fmt & ValueYPlaDevice)
		IN1(vr->YPlaDevice);
	  if (fmt & ValueXAdvDevice)
		IN1(vr->XAdvDevice);
	  if (fmt & ValueYAdvDevice)
		IN1(vr->YAdvDevice);
	}

static void *readSinglePos1(Card32 offset)
	{
	  SinglePosFormat1 *fmt = memNew(sizeof(SinglePosFormat1));

	  fmt->PosFormat = 1; /* already read */
	  IN1(fmt->Coverage);
	  fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	  IN1(fmt->ValueFormat);
	  readValueRecord(fmt->ValueFormat, &(fmt->Value));
	  return fmt;
	}

static void *readSinglePos2(Card32 offset)
	{
	  IntX i;
	  SinglePosFormat2 *fmt = memNew(sizeof(SinglePosFormat2));

	  fmt->PosFormat = 2; /* already read */
	  IN1(fmt->Coverage);
	  fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	  IN1(fmt->ValueFormat);
	  IN1(fmt->ValueCount);
	  fmt->Value = memNew(sizeof(ValueRecord) * fmt->ValueCount);
	  for (i = 0; i < fmt->ValueCount; i++)
		{
		  readValueRecord(fmt->ValueFormat, &(fmt->Value[i]));
		}
	  return fmt;
	}

static void *readSinglePos(Card32 offset)
	{	
	  Card16 format;

	  IN1(format);
	  switch(format)
		{
		case 1:
		  return readSinglePos1(offset);
		case 2:
		  return readSinglePos2(offset);
		default:
		  warning(SPOT_MSG_GPOSUNKSINGL, format,offset - GPOSStart);
		  return NULL;
		}
	}

static void readPosRule(Card32 offset, PosRule *posrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(posrule->GlyphCount);
	IN1(posrule->PosCount);
	posrule->Input = memNew(sizeof(GlyphId) * posrule->GlyphCount);
	posrule->PosLookupRecord = memNew(sizeof(PosLookupRecord) * posrule->PosCount);

	for (i = 1; i < posrule->GlyphCount; i++) 
	  {
		IN1(posrule->Input[i]);
	  }
	for (i = 0; i < posrule->PosCount; i++) 
	  {
		IN1(posrule->PosLookupRecord[i].SequenceIndex);
		IN1(posrule->PosLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readPosRuleSet(Card32 offset, PosRuleSet *posruleset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(posruleset->PosRuleCount);
	posruleset->PosRule = memNew(sizeof(Offset) * posruleset->PosRuleCount);
	posruleset->_PosRule = memNew(sizeof(PosRule) * posruleset->PosRuleCount);
	for (i = 0; i < posruleset->PosRuleCount; i++) 
	  {
		IN1(posruleset->PosRule[i]);
		readPosRule(offset + posruleset->PosRule[i], &posruleset->_PosRule[i]);
	  }
	SEEK_ABS(save);
	}

static void *readPosContext1(Card32 offset)
	{
	IntX i;
	ContextPosFormat1 *fmt = memNew(sizeof(ContextPosFormat1));

	fmt->PosFormat = 1;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->PosRuleSetCount);
	fmt->PosRuleSet = memNew(sizeof(Offset) * fmt->PosRuleSetCount);
	fmt->_PosRuleSet = memNew(sizeof(PosRuleSet) * fmt->PosRuleSetCount);

	for (i = 0; i < fmt->PosRuleSetCount; i++) 
		{
		IN1(fmt->PosRuleSet[i]);
		readPosRuleSet(offset + fmt->PosRuleSet[i], &fmt->_PosRuleSet[i]);
		}
	return fmt;
	}

static void readPosClassRule(Card32 offset, PosClassRule *PosClassrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(PosClassrule->GlyphCount);
	IN1(PosClassrule->PosCount);
	PosClassrule->Class = memNew(sizeof(Card16) * PosClassrule->GlyphCount);
	PosClassrule->PosLookupRecord = memNew(sizeof(PosLookupRecord) * PosClassrule->PosCount);

	for (i = 1; i < PosClassrule->GlyphCount; i++) 
	  {
		IN1(PosClassrule->Class[i]);
	  }
	for (i = 0; i < PosClassrule->PosCount; i++) 
	  {
		IN1(PosClassrule->PosLookupRecord[i].SequenceIndex);
		IN1(PosClassrule->PosLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readPosClassSet(Card32 offset, PosClassSet *PosClassset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(PosClassset->PosClassRuleCnt);
	PosClassset->PosClassRule = memNew(sizeof(Offset) * PosClassset->PosClassRuleCnt);
	PosClassset->_PosClassRule = memNew(sizeof(PosRule) * PosClassset->PosClassRuleCnt);
	for (i = 0; i < PosClassset->PosClassRuleCnt; i++) 
		{
		IN1(PosClassset->PosClassRule[i]);
		readPosClassRule(offset + PosClassset->PosClassRule[i], &PosClassset->_PosClassRule[i]);
		}
	SEEK_ABS(save);
	}

static void *readPosContext2(Card32 offset)
	{
	IntX i;
	ContextPosFormat2 *fmt = memNew(sizeof(ContextPosFormat2));

	fmt->PosFormat = 2;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->ClassDef);
	fmt->_ClassDef = ttoReadClass(offset + fmt->ClassDef);
	IN1(fmt->PosClassSetCnt);
	fmt->PosClassSet = memNew(sizeof(Offset) * fmt->PosClassSetCnt);
	fmt->_PosClassSet = memNew(sizeof(PosRuleSet) * fmt->PosClassSetCnt);

	for (i = 0; i < fmt->PosClassSetCnt; i++) 
		{
			IN1(fmt->PosClassSet[i]);
		if (fmt->PosClassSet[i] != 0)
			{
			readPosClassSet(offset + fmt->PosClassSet[i], &fmt->_PosClassSet[i]);
			}
		}
	return fmt;
	}

static void *readPosContext3(Card32 offset)
	{
	IntX i;
	ContextPosFormat3 *fmt = memNew(sizeof(ContextPosFormat3));

	fmt->PosFormat = 3;
	IN1(fmt->GlyphCount);
	IN1(fmt->PosCount);
	fmt->CoverageArray = memNew(sizeof(Offset) * (fmt->GlyphCount + 1));
	fmt->_CoverageArray = memNew(sizeof(void *) * (fmt->GlyphCount + 1));
	fmt->PosLookupRecord = memNew(sizeof(PosLookupRecord) * fmt->PosCount);
	for (i = 0; i < fmt->GlyphCount; i++)
	  {
		IN1(fmt->CoverageArray[i]);
		fmt->_CoverageArray[i] = ttoReadCoverage(offset + fmt->CoverageArray[i]);
	  }
	for (i = 0; i < fmt->PosCount; i++) 
	  {
		IN1(fmt->PosLookupRecord[i].SequenceIndex);
		IN1(fmt->PosLookupRecord[i].LookupListIndex);
	  }
	return fmt;
	}

static void *readPosContext(Card32 offset)
	{
	  Card16 format;
	  
	  IN1(format);
	  switch (format)
	    {
	    case 1:
	      return readPosContext1(offset);
	    case 2:
	      return readPosContext2(offset);
	    case 3:
	      return readPosContext3(offset);
	    default:
		  warning(SPOT_MSG_GPOSUNKCTX, format, offset);
		return NULL;
		}
	}

static void readChainPosRule(Card32 offset, ChainPosRule *posrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);

	IN1(posrule->BacktrackGlyphCount);
	posrule->Backtrack = memNew(sizeof(GlyphId) * posrule->BacktrackGlyphCount);
	for (i = 0; i < posrule->BacktrackGlyphCount; i++) 
	  {
		IN1(posrule->Backtrack[i]);
	  }	

	IN1(posrule->InputGlyphCount);
	posrule->Input = memNew(sizeof(GlyphId) * posrule->InputGlyphCount);
	for (i = 1; i < posrule->InputGlyphCount; i++) 
	  {
		IN1(posrule->Input[i]);
	  }	

	IN1(posrule->LookaheadGlyphCount);
	posrule->Lookahead = memNew(sizeof(GlyphId) * posrule->LookaheadGlyphCount);
	for (i = 0; i < posrule->LookaheadGlyphCount; i++) 
	  {
		IN1(posrule->Lookahead[i]);
	  }	

	IN1(posrule->PosCount);
	posrule->PosLookupRecord = memNew(sizeof(PosLookupRecord) * posrule->PosCount);
	for (i = 0; i < posrule->PosCount; i++) 
	  {
		IN1(posrule->PosLookupRecord[i].SequenceIndex);
		IN1(posrule->PosLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readChainPosRuleSet(Card32 offset, ChainPosRuleSet *posruleset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(posruleset->ChainPosRuleCount);
	posruleset->ChainPosRule = memNew(sizeof(Offset) * posruleset->ChainPosRuleCount);
	posruleset->_ChainPosRule = memNew(sizeof(ChainPosRule) * posruleset->ChainPosRuleCount);
	for (i = 0; i < posruleset->ChainPosRuleCount; i++) 
	  {
		IN1(posruleset->ChainPosRule[i]);
		readChainPosRule(offset + posruleset->ChainPosRule[i], &posruleset->_ChainPosRule[i]);
	  }
	SEEK_ABS(save);
	}

static void *readPosChainContext1(Card32 offset)
	{
	IntX i;
	ChainContextPosFormat1 *fmt = memNew(sizeof(ChainContextPosFormat1));

	fmt->PosFormat = 1;
	IN1(fmt->Coverage);
	fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	IN1(fmt->ChainPosRuleSetCount);
	fmt->ChainPosRuleSet = memNew(sizeof(Offset) * fmt->ChainPosRuleSetCount);
	fmt->_ChainPosRuleSet = memNew(sizeof(ChainPosRuleSet) * fmt->ChainPosRuleSetCount);

	for (i = 0; i < fmt->ChainPosRuleSetCount; i++) 
		{
		IN1(fmt->ChainPosRuleSet[i]);
		readChainPosRuleSet(offset + fmt->ChainPosRuleSet[i], &fmt->_ChainPosRuleSet[i]);
		}
	return fmt;
	}


static void readChainPosClassRule(Card32 offset, ChainPosClassRule *PosClassrule)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(PosClassrule->BacktrackGlyphCount);
	PosClassrule->Backtrack = memNew(sizeof(Card16) * PosClassrule->BacktrackGlyphCount);
	for (i = 0; i < PosClassrule->BacktrackGlyphCount; i++) 
	  {
		IN1(PosClassrule->Backtrack[i]);
	  }

	IN1(PosClassrule->InputGlyphCount);
	PosClassrule->Input = memNew(sizeof(Card16) * PosClassrule->InputGlyphCount);
	for (i = 1; i < PosClassrule->InputGlyphCount; i++) 
	  {
		IN1(PosClassrule->Input[i]);
	  }

	IN1(PosClassrule->LookaheadGlyphCount);
	PosClassrule->Lookahead = memNew(sizeof(Card16) * PosClassrule->LookaheadGlyphCount);
	for (i = 0; i < PosClassrule->LookaheadGlyphCount; i++) 
	  {
		IN1(PosClassrule->Lookahead[i]);
	  }

	IN1(PosClassrule->PosCount);
	PosClassrule->PosLookupRecord = memNew(sizeof(PosLookupRecord) * PosClassrule->PosCount);

	for (i = 0; i < PosClassrule->PosCount; i++) 
	  {
		IN1(PosClassrule->PosLookupRecord[i].SequenceIndex);
		IN1(PosClassrule->PosLookupRecord[i].LookupListIndex);
	  }
	SEEK_ABS(save);
	}

static void readChainPosClassSet(Card32 offset, ChainPosClassSet *PosClassset)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	IN1(PosClassset->ChainPosClassRuleCnt);
	PosClassset->ChainPosClassRule = memNew(sizeof(Offset) * PosClassset->ChainPosClassRuleCnt);
	PosClassset->_ChainPosClassRule = memNew(sizeof(ChainPosRule) * PosClassset->ChainPosClassRuleCnt);
	for (i = 0; i < PosClassset->ChainPosClassRuleCnt; i++) 
		{
		IN1(PosClassset->ChainPosClassRule[i]);
		readChainPosClassRule(offset + PosClassset->ChainPosClassRule[i], &PosClassset->_ChainPosClassRule[i]);
		}
	SEEK_ABS(save);
	}

static void *readPosChainContext2(Card32 offset)
	{
	IntX i;
	ChainContextPosFormat2 *fmt = memNew(sizeof(ChainContextPosFormat2));

	fmt->PosFormat = 2;
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
	IN1(fmt->ChainPosClassSetCnt);
	fmt->ChainPosClassSet = memNew(sizeof(Offset) * fmt->ChainPosClassSetCnt);
	fmt->_ChainPosClassSet = memNew(sizeof(ChainPosRuleSet) * fmt->ChainPosClassSetCnt);

	for (i = 0; i < fmt->ChainPosClassSetCnt; i++) 
		{
		IN1(fmt->ChainPosClassSet[i]);
		if (fmt->ChainPosClassSet[i] == 0)
			{
			fmt->_ChainPosClassSet[i].ChainPosClassRuleCnt = 0;
			continue;
			}
		readChainPosClassSet(offset + fmt->ChainPosClassSet[i], &fmt->_ChainPosClassSet[i]);
		}
	return fmt;
	}

static void *readPosChainContext3(Card32 offset)
	{
	IntX i;
	ChainContextPosFormat3 *fmt = memNew(sizeof(ChainContextPosFormat3));

	fmt->PosFormat = 3;

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

	IN1(fmt->PosCount);
	fmt->PosLookupRecord = memNew(sizeof(PosLookupRecord) * fmt->PosCount);
	for (i = 0; i < fmt->PosCount; i++) 
	  {
		IN1(fmt->PosLookupRecord[i].SequenceIndex);
		IN1(fmt->PosLookupRecord[i].LookupListIndex);
	  }

	return fmt;
	}

static void *readPosChainContext(Card32 offset)
	{
	  Card16 format;
	  
	  IN1(format);
	  switch (format)
	    {
	    case 1:
	      return readPosChainContext1(offset); 
	    case 2:
	      return readPosChainContext2(offset); 
	    case 3:
	      return readPosChainContext3(offset);
	    default:
		  warning(SPOT_MSG_GPOSUNKCCTX, format, offset);
		return NULL;
		}
	}

static void *readExtension1(Card32 offset)
	{
	ExtensionPosFormat1 *fmt = memNew(sizeof(ExtensionPosFormat1));

	fmt->PosFormat = 1;	/* Already read */
	IN1(fmt->ExtensionLookupType);
	IN1(fmt->ExtensionOffset);
	fmt->subtable=readSubtable(fmt->ExtensionOffset+offset, fmt->ExtensionLookupType);
	return fmt;
	}
	

static void *readExtension(Card32 offset)
	{
	Card16 format;

	IN1(format);
	switch (format)
		{
	case 1:
		return readExtension1(offset);
	default:
		warning(SPOT_MSG_GPOSUNKSINGL, format, offset);
		return NULL;
		}
	}

static void readPairSet(Card32 offset, PairSet *pairset, Card16 val1, Card16 val2)
	{
	  IntX i;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(pairset->PairValueCount);
	  pairset->PairValueRecord = memNew(sizeof(PairValueRecord) *
										pairset->PairValueCount);
	  for (i = 0; i < pairset->PairValueCount; i++) 
		{
		  IN1(pairset->PairValueRecord[i].SecondGlyph);
		  readValueRecord(val1, &(pairset->PairValueRecord[i].Value1));
		  readValueRecord(val2, &(pairset->PairValueRecord[i].Value2));
		}

	  SEEK_ABS(save);
	}

static void *readPosPair1(Card32 offset)
	{
	  IntX i;
	  PosPairFormat1 *fmt = memNew(sizeof(PosPairFormat1));

	  fmt->PosFormat = 1; /* already read */
	  IN1(fmt->Coverage);
	  fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	  IN1(fmt->ValueFormat1);
	  IN1(fmt->ValueFormat2);
	  IN1(fmt->PairSetCount);
	  fmt->PairSet = memNew(sizeof(Offset) * fmt->PairSetCount);
	  fmt->_PairSet = memNew(sizeof(PairSet) * fmt->PairSetCount);
	  for (i = 0; i < fmt->PairSetCount; i++) 
		{
		  IN1(fmt->PairSet[i]);
		  readPairSet(offset + fmt->PairSet[i], &fmt->_PairSet[i],
					  fmt->ValueFormat1, fmt->ValueFormat2);
		}
	  return fmt;
	}


static void *readPosPair2(Card32 offset)
	{
	  IntX i;
	  PosPairFormat2 *fmt = memNew(sizeof(PosPairFormat2));

	  fmt->PosFormat = 2; /* already read */
	  IN1(fmt->Coverage);
	  fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);
	  IN1(fmt->ValueFormat1);
	  IN1(fmt->ValueFormat2);
	  IN1(fmt->ClassDef1);
	  fmt->_ClassDef1 = ttoReadClass(offset + fmt->ClassDef1);
	  IN1(fmt->ClassDef2);
	  fmt->_ClassDef2 = ttoReadClass(offset + fmt->ClassDef2);
	  IN1(fmt->Class1Count);
	  IN1(fmt->Class2Count);
	  
	  fmt->Class1Record = memNew(sizeof(Class1Record) * fmt->Class1Count);
	  for (i = 0; i < fmt->Class1Count; i++) 
		{
		  IntX j;
		  Class1Record *record1 = &fmt->Class1Record[i];
		  record1->Class2Record = memNew(sizeof(Class2Record) * fmt->Class2Count);
		  for (j = 0; j < fmt->Class2Count; j++) 
			{
			  Class2Record *record2 = &fmt->Class1Record[i].Class2Record[j];
			  readValueRecord(fmt->ValueFormat1, &(record2->Value1));
			  readValueRecord(fmt->ValueFormat2, &(record2->Value2));
			}
		}
	  return fmt;
	}

static void *readPosPair(Card32 offset)
	{
	  Card16 format;

	  IN1(format);
	  switch(format)
		{
		case 1:
		  return readPosPair1(offset);
		case 2:
		  return readPosPair2(offset);
		default:
		  warning(SPOT_MSG_GPOSUNKPAIR, format,offset - GPOSStart);
		  return NULL;
		}
	}

static void readAnchorFormat1(AnchorFormat1 *anchor)
	{
	  anchor->AnchorFormat = 1; /* already read */
	  IN1(anchor->XCoordinate);
	  IN1(anchor->YCoordinate);
	}

static void readAnchorFormat2(AnchorFormat2 *anchor)
	{
	  anchor->AnchorFormat = 2; /* already read */
	  IN1(anchor->XCoordinate);
	  IN1(anchor->YCoordinate);
	  IN1(anchor->AnchorPoint);
	}

static void readAnchorFormat3(Card32 offset, AnchorFormat3 *anchor)
	{
	  anchor->AnchorFormat = 3; /* already read */
	  IN1(anchor->XCoordinate);
	  IN1(anchor->YCoordinate);
	  IN1(anchor->XDeviceTable);
	  IN1(anchor->YDeviceTable);
	  
		anchor->_XDeviceTable = NULL;
		anchor->_YDeviceTable = NULL;
		
	  if (anchor->XDeviceTable != 0)
	  	{
	  	anchor->_XDeviceTable = memNew(sizeof(DeviceTable));
	  	ttoReadDeviceTable(offset + anchor->XDeviceTable, anchor->_XDeviceTable);
	  	}
	  	
	  if (anchor->YDeviceTable != 0)
	  	{
	  	anchor->_YDeviceTable = memNew(sizeof(DeviceTable));
	  	ttoReadDeviceTable(offset + anchor->YDeviceTable, anchor->_YDeviceTable);
	  	}
	}

static void readAnchor(Card32 offset, void **anchor)
	{
	  Card16 fmt;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(fmt);
	  switch(fmt)
		{
		case 1:
		   *anchor = memNew(sizeof(AnchorFormat1));
		   readAnchorFormat1(*anchor);
		   break;
		case 2:
		   *anchor = memNew(sizeof(AnchorFormat2));
		   readAnchorFormat2(*anchor);
		   break;
		case 3:
		   *anchor = memNew(sizeof(AnchorFormat3));
		   readAnchorFormat3(offset, *anchor);
		   break;

		 default:
		   warning(SPOT_MSG_GPOSUNKANCH, fmt, offset - GPOSStart);
		   break;
		}

	  SEEK_ABS(save);
	}

static void *readCursiveAttachPos1(Card32 offset)
	{
	IntX i;
	  CursivePosFormat1 *fmt = memNew(sizeof(CursivePosFormat1));

	  fmt->PosFormat = 1; /* already read */
	  IN1(fmt->Coverage);
	  fmt->_Coverage = ttoReadCoverage(offset + fmt->Coverage);  
	  IN1(fmt->EntryExitCount);

	  fmt->EntryExitRecord = memNew(sizeof(EntryExitRecord) * fmt->EntryExitCount);
	  for (i = 0; i <  fmt->EntryExitCount; i++)
		{
		  EntryExitRecord *record; 
		  record = &(fmt->EntryExitRecord[i]);
	 	 IN1(record->EntryAnchor);
	 	 IN1(record->ExitAnchor);
		 if (record->EntryAnchor)
			readAnchor(offset + record->EntryAnchor, &(record->_EntryAnchor));
		 if (record->ExitAnchor)
			readAnchor(offset + record->ExitAnchor, &(record->_ExitAnchor));
		}

	  return fmt;
	}

static void *readCursiveAttachPos(Card32 offset)
	{
	  Card16 format;
	  IN1(format);
	  switch(format)
		{
		case 1:
		  return readCursiveAttachPos1(offset);
		default:
		  warning(SPOT_MSG_GPOSUNKMARKB, format,offset - GPOSStart);
		  return NULL;
		}
	}


static void readMarkArray(Card32 offset, MarkArray *markarray)
	{
	  IntX i;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(markarray->MarkCount);

	  markarray->MarkRecord = memNew(sizeof(MarkRecord) * markarray->MarkCount);
	  for (i = 0; i < markarray->MarkCount; i++)
		{
		  MarkRecord *record = &markarray->MarkRecord[i];
		  IN1(record->Class);
		  IN1(record->MarkAnchor);
		  if (record->MarkAnchor > 0)
			readAnchor(offset + record->MarkAnchor, &(record->_MarkAnchor));
		}
	  SEEK_ABS(save);
	}

static void readBaseArray(Card32 offset, BaseArray *basearray, IntX classcount)
	{
	  IntX i, c;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(basearray->BaseCount);

	  basearray->BaseRecord = memNew(sizeof(BaseRecord) * basearray->BaseCount);
	  for (i = 0; i < basearray->BaseCount; i++)
		{
		  BaseRecord *record; 
		  record = &(basearray->BaseRecord[i]);
		  record->BaseAnchor = memNew(sizeof(Offset) * classcount);
		  record->_BaseAnchor = memNew(sizeof(record->_BaseAnchor[0]) * classcount);
		  for (c = 0; c < classcount; c++) 
			{
			  IN1(record->BaseAnchor[c]);
			  if (record->BaseAnchor[c] > 0)
				readAnchor(offset + record->BaseAnchor[c], &(record->_BaseAnchor[c]));
			}
		}
	  SEEK_ABS(save);
	}


static void *readMarkBasePos1(Card32 offset)
	{
	  MarkBasePosFormat1 *fmt = memNew(sizeof(MarkBasePosFormat1));

	  fmt->PosFormat = 1; /* already read */
	  IN1(fmt->MarkCoverage);
	  fmt->_MarkCoverage = ttoReadCoverage(offset + fmt->MarkCoverage);  
	  IN1(fmt->BaseCoverage);
	  fmt->_BaseCoverage = ttoReadCoverage(offset + fmt->BaseCoverage);  
	  IN1(fmt->ClassCount);
	  IN1(fmt->MarkArray);
	  IN1(fmt->BaseArray);
	  readMarkArray(offset + fmt->MarkArray, &(fmt->_MarkArray));
	  readBaseArray(offset + fmt->BaseArray, &(fmt->_BaseArray), fmt->ClassCount);
	  return fmt;
	}


static void *readMarkBasePos(Card32 offset)
	{
	  Card16 format;
	  IN1(format);
	  switch(format)
		{
		case 1:
		  return readMarkBasePos1(offset);
		default:
		  warning(SPOT_MSG_GPOSUNKMARKB, format,offset - GPOSStart);
		  return NULL;
		}
	}

static void readLigatureAttach(Card32 offset, LigatureAttach *ligatureAttach, IntX classcount)
	{
	  IntX i, c;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(ligatureAttach->ComponentCount);

	  ligatureAttach->ComponentRecord = memNew(sizeof(ComponentRecord) * ligatureAttach->ComponentCount);
	  for (i = 0; i <ligatureAttach->ComponentCount; i++)
		{
		  ComponentRecord *record; 
		  record = &(ligatureAttach->ComponentRecord[i]);
		  record->LigatureAnchor = memNew(sizeof(Offset) * classcount);
		  record->_LigatureAnchor = memNew(sizeof(record->LigatureAnchor[0]) * classcount); /* this makes an array of void* */
		  for (c = 0; c < classcount; c++) 
			{
			  IN1(record->LigatureAnchor[c]);
			  if (record->LigatureAnchor[c] != 0)
			 	 readAnchor(offset + record->LigatureAnchor[c], &(record->_LigatureAnchor[c]));
			  else
			   record->_LigatureAnchor[c] = 0;
			}
		}
	  SEEK_ABS(save);
	}


static void readLigatureArray(Card32 offset, LigatureArray *ligatureArray, IntX classcount)
	{
	  IntX i;
	  Card32 save = TELL();

	  SEEK_ABS(offset);
	  IN1(ligatureArray->LigatureCount);

	  ligatureArray->LigatureAttach = memNew(sizeof(Offset) * ligatureArray->LigatureCount);
	  ligatureArray->_LigatureAttach = memNew(sizeof(LigatureAttach) * ligatureArray->LigatureCount);
	  for (i = 0; i <ligatureArray->LigatureCount; i++)
		{
		 IN1(ligatureArray->LigatureAttach[i]);
		 readLigatureAttach(offset + ligatureArray->LigatureAttach[i], &(ligatureArray->_LigatureAttach[i]), classcount);
		 }
	  SEEK_ABS(save);
	}


static void *readMarkToLigatureAttach1(Card32 offset)
	{
	  MarkToLigPosFormat1 *fmt = memNew(sizeof(MarkToLigPosFormat1));

	  fmt->PosFormat = 1; /* already read */
	  IN1(fmt->MarkCoverage);
	  fmt->_MarkCoverage = ttoReadCoverage(offset + fmt->MarkCoverage);  
	  IN1(fmt->LigatureCoverage);
	  fmt->_LigatureCoverage = ttoReadCoverage(offset + fmt->LigatureCoverage);  
	  IN1(fmt->ClassCount);
	  IN1(fmt->MarkArray);
	  IN1(fmt->LigatureArray);
	  readMarkArray(offset + fmt->MarkArray, &(fmt->_MarkArray));
	  readLigatureArray(offset + fmt->LigatureArray, &(fmt->_LigatureArray), fmt->ClassCount);
	  return fmt;
	}

static void *readMarkToLigatureAttach(Card32 offset)
	{
	  Card16 format;
	  IN1(format);
	  switch(format)
		{
		case 1:
		  return readMarkToLigatureAttach1(offset);
		default:
		  warning(SPOT_MSG_GPOSUNKMARKB, format,offset - GPOSStart);
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
	case SingleAdjustType:
		result = readSinglePos(offset);
		break;

	case PairAdjustType: /* 2*/
		result = readPosPair(offset);
		break;

	case CursiveAttachType:/* 3*/
		result = readCursiveAttachPos(offset);
    	break;

    case MarkToBaseAttachType: /* 4*/
    case MarkToMarkAttachType: /* 6*/
		result = readMarkBasePos(offset);
		break;

	case MarkToLigatureAttachType: /* 5*/
		result = readMarkToLigatureAttach(offset);
		break;

    case ContextPositionType:
		result = readPosContext(offset);
		break;
	case ChainingContextPositionType:
		result = readPosChainContext(offset);
		break;
	case ExtensionPositionType:
		result = readExtension(offset);
		break;
		
	default:
		warning(SPOT_MSG_GPOSUNKRLOOK, type, type, offset - GPOSStart);
		result = NULL;
		}

	SEEK_ABS(save);
	return result;
	}

void GPOSRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	SEEK_ABS(start);

	GPOSStart = start;
	IN1(GPOS.Version);
	IN1(GPOS.ScriptList);
	IN1(GPOS.FeatureList);
	IN1(GPOS.LookupList);

	ttoReadScriptList(start + GPOS.ScriptList, &GPOS._ScriptList);
	ttoReadFeatureList(start + GPOS.FeatureList, &GPOS._FeatureList);
	ttoReadLookupList(start + GPOS.LookupList, &GPOS._LookupList,
					  readSubtable);
	loaded = 1;
	featuretag = 0;
	prev_featuretag = 0;
	}
	

static Byte8 *dumpTitle(Card32 tag, IntX flavor)
	{
	  static Byte8 othertag[80];

	  if (tag == 0) 
		{
		  return ("GPOS table features ");
		}
	  else if (tag == (STR2TAG("altv"))) return ("altv (Alternate vertical metrics)");
	  else if (tag == (STR2TAG("case"))) return ("case (Case-sensitive positioning)");
	  else if (tag == (STR2TAG("cpct"))) return ("cpct (Centered punctuation)");
	  else if (tag == (STR2TAG("cpsp"))) return ("cpsp (Capitals spacing)");
	  else if (tag == (STR2TAG("halt"))) return ("halt (Alternate Half-width metrics)");
	  else if (tag == (STR2TAG("hwid"))) return ("hwid (Half-width metrics)");
	  else if (tag == (STR2TAG("lfbd"))) return ("lfbd (Left-bounds positioning)");
	  else if (tag == (STR2TAG("kern")))
		{
		  if (flavor == 1)
			{
			  if (proofIsAltKanjiKern())
				return ("kern (Pair-wise kerning) + AltMetrics");				
			  else
				return ("kern (Pair-wise kerning)");
			}
		  else if (flavor == 2)
			{
			  if (proofIsAltKanjiKern())
				return ("kern (Class kerning) + AltMetrics");
			  else
				return ("kern (Class kerning)");
			}
		  else
			{
			  if (proofIsAltKanjiKern())
				return ("kern + AltMetrics");
			  else
				return ("kern");
			}
		}
	  else if (tag == (STR2TAG("mark"))) return ("mark (Mark positioning)");
	  else if (tag == (STR2TAG("opbd"))) return ("opbd (Optical bounds positioning)");
	  else if (tag == (STR2TAG("palt"))) return ("palt (Alternate Proportional-width metrics)");
	  else if (tag == (STR2TAG("pwid"))) return ("pwid (Proportional-width metrics)");
	  else if (tag == (STR2TAG("qwid"))) return ("qwid (Quarter-width metrics)");
	  else if (tag == (STR2TAG("rtbd"))) return ("rtbd (Right-bounds positioning)");
	  else if (tag == (STR2TAG("subs"))) return ("subs (Subscript positioning)");
	  else if (tag == (STR2TAG("twid"))) return ("twid (Third-width metrics)");
	  else if (tag == (STR2TAG("valt"))) return ("valt (Alternate vertical metrics)");
	  else if (tag == (STR2TAG("vhal"))) return ("vhal (Vertical alternate Half-width metrics)");
	  else if (tag == (STR2TAG("mkmk"))) return ("mkmk (Mark to Mark Positioning)");
	  else if (tag == (STR2TAG("dist"))) return ("dist (Distances )");
	  else if (tag == (STR2TAG("abvm"))) return ("abvm (Above-base Mark Positioning)");
	  else if (tag == (STR2TAG("blwm"))) return ("blwm ( Below-base Mark Positioning)");
	  else if (tag == (STR2TAG("curs"))) return ("curs (Cursive Positioning)");

	  else if (tag == (STR2TAG("vkrn")))
		{
		  if (flavor == 2)
			{
			  if (proofIsAltKanjiKern())
				return ("vkrn (Vertical class kerning) + AltMetrics");
			  else
				return ("vkrn (Vertical class kerning)");
			}
		  else
			{
			  if (proofIsAltKanjiKern())
				return ("vkrn (Vertical pair kerning) + AltMetrics");
			  else
				return ("vkrn (Vertical pair kerning)");
			}

		}
	  else if (tag == (STR2TAG("vpal"))) return ("vpal (Vertical alt. Proportional-width metrics)");
	  
	  
	  else
		{
		  sprintf(othertag,"'%c%c%c%c' (Unknown/Unregistered tag)", TAG_ARG(tag));
		  return(othertag);
		}
	}

static IntX isVerticalFeature(Card32 tag, IntX flavor)
	{
	  if (tag == 0) return 0;
	  else if (tag == (STR2TAG("altv"))) return 1;
	  else if (tag == (STR2TAG("vkrn"))) return 1;
	  else if (tag == (STR2TAG("vhal"))) return 1;
	  else if (tag == (STR2TAG("vpal"))) return 1;
	  else if (tag == (STR2TAG("valt"))) return 1;
	  return 0;
	}

static void dumpValueRecord(Card16 fmt, ValueRecord *vr, IntX level)
	{
	  if (fmt == 0) return;
	  if (fmt & ValueXPlacement)
		DL(2, (OUTPUTBUFF, " XPlacement= %d", vr->XPlacement));
	  if (fmt & ValueYPlacement)
		DL(2, (OUTPUTBUFF, " YPlacement= %d", vr->YPlacement));
	  if (fmt & ValueXAdvance)
		DL(2, (OUTPUTBUFF, " XAdvance= %d", vr->XAdvance));
	  if (fmt & ValueYAdvance)
		DL(2, (OUTPUTBUFF, " YAdvance= %d", vr->YAdvance));
	  if (fmt & ValueXPlaDevice)
		DL(2, (OUTPUTBUFF, " XPlaDevice= %d", vr->XPlaDevice));
	  if (fmt & ValueYPlaDevice)
		DL(2, (OUTPUTBUFF, " YPlaDevice= %d", vr->YPlaDevice));
	  if (fmt & ValueXAdvDevice)
		DL(2, (OUTPUTBUFF, " XAdvDevice= %d", vr->XAdvDevice));
	  if (fmt & ValueYAdvDevice)
		DL(2, (OUTPUTBUFF, " YAdvDevice= %d", vr->YAdvDevice));

	  /* MM extensions */
	  if (fmt & ValueXIdPlacement) {
		DL(2, (OUTPUTBUFF, " XPlacement_MMFXId= %d < ", vr->XPlacement));
		MMFXDumpMetric(vr->XPlacement);
		DL(2, (OUTPUTBUFF, ">"));
	  }
	  if (fmt & ValueYIdPlacement) {
		DL(2, (OUTPUTBUFF, " YPlacement_MMFXId= %d < ", vr->YPlacement));
		MMFXDumpMetric(vr->YPlacement);
		DL(2, (OUTPUTBUFF, ">"));
	  }
	  if (fmt & ValueXIdAdvance) {
		DL(2, (OUTPUTBUFF, " XAdvance_MMFXid= %d < ", vr->XAdvance));
		MMFXDumpMetric(vr->XAdvance);
		DL(2, (OUTPUTBUFF, ">"));
	  }
	  if (fmt & ValueYIdAdvance) {
		DL(2, (OUTPUTBUFF, " YAdvance_MMFXid= %d < ", vr->YAdvance));
		MMFXDumpMetric(vr->YAdvance);
		DL(2, (OUTPUTBUFF, ">"));
	  }
	}

static int isMMValueFmt(Card16 fmt)
	{
	  if (fmt == 0) return 0;
	  if (fmt & ValueXIdPlacement)  return 1;
	  if (fmt & ValueYIdPlacement)  return 1;
	  if (fmt & ValueXIdAdvance)  return 1;
	  if (fmt & ValueYIdAdvance)  return 1;
	  return 0;
	}

static int numValues(Card16 valFmt)
	{
	int i = 0;
	while (valFmt)
		{
		i++;
		valFmt &= valFmt - 1;		/* Remove least significant set bit */
		}
	return i;
	}

static void printValueRecord(Card16 fmt, ValueRecord *vr, int level)
	{
	  FILE * out;
	  
	  if (level==6)
	  	out=AFMout;
	  else
	  	out=OUTPUTBUFF;
	  if (fmt == 0) return;
	  
	  if (numValues(fmt) == 1)
		{
		  if (fmt & ValueXPlacement)
			fprintf(out,  " %d", vr->XPlacement);
		  if (fmt & ValueYPlacement)
			fprintf(out,  " %d", vr->YPlacement);
		  if (fmt & ValueXAdvance)
			fprintf(out,  " %d", vr->XAdvance);
		  if (fmt & ValueYAdvance)
			fprintf(out,  " %d", vr->YAdvance);
		  if (fmt & ValueXPlaDevice)
			fprintf(out,  " %d", vr->XPlaDevice);
		  if (fmt & ValueYPlaDevice)
			fprintf(out,  " %d", vr->YPlaDevice);
		  if (fmt & ValueXAdvDevice)
			fprintf(out,  " %d", vr->XAdvDevice);
		  if (fmt & ValueYAdvDevice)
			fprintf(out,  " %d", vr->YAdvDevice);
		}
	else
		{
		if (level == 7)
			fprintf(out,   " <");
		fprintf(out,   "%d %d %d %d", vr->XPlacement, vr->YPlacement, vr->XAdvance, vr->YAdvance);
		
		if (vr->XPlaDevice || vr->XAdvDevice)
			fprintf(out,  "< device  %d %d %d %d >", vr->XPlaDevice, vr->YPlaDevice, vr->XAdvDevice, vr->YAdvDevice);
		if (level == 7)
			fprintf(out,   ">");
		}
		
	  /* MM extensions */
	  if (fmt & ValueXIdPlacement) {
		fprintf(out,  " < ");
		MMFXDumpMetric(vr->XPlacement);
		fprintf(out,  ">");
	  }
	  if (fmt & ValueYIdPlacement) {
		fprintf(out,  " < ");
		MMFXDumpMetric(vr->YPlacement);
		fprintf(out,  ">");
	  }
	  if (fmt & ValueXIdAdvance) {
		fprintf(out,  " < ");
		MMFXDumpMetric(vr->XAdvance);
		fprintf(out,  ">");
	  }
	  if (fmt & ValueYIdAdvance) {
		fprintf(out,  " < ");
		MMFXDumpMetric(vr->YAdvance);
		fprintf (out, ">");
	  }
	}

static void proofSinglePos1(SinglePosFormat1 *fmt, IntX glyphtoproof)
	{
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i;
	  Int16 xpla, ypla, xadv, yadv;
	  IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
	  proofOptions options;
	  IntX isVert = proofIsVerticalMode();

	  xpla = fmt->Value.XPlacement;
	  ypla = fmt->Value.YPlacement;
	  xadv = fmt->Value.XAdvance;
	  yadv = fmt->Value.YAdvance;

	  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	  for ((glyphtoproof!=-1)?(i=glyphtoproof):(i = 0); (glyphtoproof!=-1)?(i<=glyphtoproof):(i < (IntX)nitems); i++) 
		{
		  char label[80];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
		  if (glyphtoproof==-1 && !opt_Present("-f")) {
		  		ProofRec * proofRec = da_INDEX(proofrecords, curproofrec);
				proofRec->g1=glyphId;
				proofRec->g2=0;
				proofRec->i=i;
				proofRec->fmt=fmt;
				proofRec->type=SingleAdjustType;
				proofRec->vert=proofIsVerticalMode();
				curproofrec++;
		  }else{
			  char name[MAX_NAME_LEN];
			  strcpy(name, getGlyphName(glyphId, 1));
			  getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			  if (isVert)
				proofCheckAdvance(proofctx, 1000 + 2*(abs(vwidth)));
			  else
				proofCheckAdvance(proofctx, 1000 + 2*width);
			  proofDrawGlyph(proofctx, 
							 glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							 name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							 NULL, 0, /* altlabel,altlabelflags */
							 0,0, /* originDx,originDy */
							 0,0, /* origin,originflags */
							 (isVert)? -vwidth : width, ANNOT_NOLINE|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1)|ANNOT_BOLD, /* width,widthflags */
							 NULL, (isVert)?yorig:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
							 );
			  
			  proofSymbol(proofctx, PROOF_YIELDS);
			  label[0] = '\0';
			  proofClearOptions(&options);
			  if (xpla || ypla)
				{
				  if (xpla)
					{
					  options.neworigin = xpla;
					  options.neworiginflags = ANNOT_DASHEDLINE|ANNOT_ATBOTTOM;
					}
				  if (ypla)
					{
					  options.newvorigin = ypla;
					  options.newvoriginflags = ANNOT_DASHEDLINE|ANNOT_ATRIGHT;
					}
				}

			  proofDrawGlyph(proofctx, 
							 glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							 name, 0,   /* glyphname,glyphnameflags */
							 NULL, 0, /* altlabel,altlabelflags */
							 xpla,((isVert)?(-1):(1))*ypla, /* originDx,originDy */
							 0,0, /* origin,originflags */
							 (isVert)?(- (abs(vwidth)+yadv)):(width+xadv), ANNOT_NOLINE|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1)|ANNOT_EMPHASIS, /* width,widthflags */
							 &options, (isVert)?yorig:DEFAULT_YORIG_KANJI, ""
							 );

			  proofThinspace(proofctx, 2);
			}
	  }

	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
		
	}
	
static void dumpSinglePos1(SinglePosFormat1 *fmt, IntX level, IntX glyphtoproof)
	{
	 if (level == 8)
		proofSinglePos1(fmt, glyphtoproof);
	 else if ((level == 6) || (level == 7))
	  {
	  	ttoEnumRec CovList;
	  	Card32 nitems;
	  	IntX i;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	    for (i = 0; i < (IntX)nitems; i++) 
		{
		  char name[MAX_NAME_LEN];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

		  strcpy(name, getGlyphName(glyphId, 0));

		if (level == 7) 
			{
			fprintf(OUTPUTBUFF,  "%s pos %s ", contextPrefix, name);
			printValueRecord(ValueXPlacement|ValueYPlacement|ValueXAdvance|ValueYAdvance, &(fmt->Value), level);
			fprintf(OUTPUTBUFF,  ";\n");
			}
		else
			{
			if (featuretag == STR2TAG("vkrn"))
				{
				DDL(2, (AFMout, "KPY %s GC_ALL_GLYPHS ", name));
				printValueRecord(ValueYAdvance, &(fmt->Value), level);
				}
			else
				{
				DDL(2, (AFMout, "KPX %s GC_ALL_GLYPHS ", name));
				printValueRecord(ValueXAdvance, &(fmt->Value), level);
				}
			DDL(2, (AFMout, " 1\n")); /* terminal 0 means is not a class pair. See GPOSdump() */
			}	 
		 }
		 
	   if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	  }
	  else
	    {
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "Coverage  =", fmt->Coverage);
		  DLu(2, "ValueFmt  =", fmt->ValueFormat);
		  if (level < 5)
			dumpValueRecord(fmt->ValueFormat, &(fmt->Value), level);
		  DL(2, (OUTPUTBUFF, "\n"));
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		 }
	}

static void proofSinglePos2(SinglePosFormat2 *fmt, IntX glyphtoproof)
	{
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i;
	  Int16 xpla, ypla, xadv, yadv;
	  IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
	  proofOptions options;
	  IntX isVert = proofIsVerticalMode();

	  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	  for ((glyphtoproof!=-1)?(i=glyphtoproof):(i = 0); (glyphtoproof!=-1)?(i<=glyphtoproof):(i < (IntX)nitems); i++) 
		{
		  char label[80];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
		  if (glyphtoproof ==-1 && !opt_Present("-f")) {
		  		ProofRec * proofRec = da_INDEX(proofrecords, curproofrec);
				proofRec->g1=glyphId;
				proofRec->g2=0;
				proofRec->i=i;
				proofRec->fmt=fmt;
				proofRec->type=SingleAdjustType;
				proofRec->vert=proofIsVerticalMode();
				curproofrec++;
		  }else{
			  char name[MAX_NAME_LEN];
			  strcpy(name, getGlyphName(glyphId, 1));
			  xpla = fmt->Value[i].XPlacement;
			  ypla = fmt->Value[i].YPlacement;
			  xadv = fmt->Value[i].XAdvance;
			  yadv = fmt->Value[i].YAdvance;
			  getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
			  if (isVert)
				proofCheckAdvance(proofctx, 1000 + 2*(abs(vwidth)));
			  else
				proofCheckAdvance(proofctx, 1000 + 2*width);
			  proofDrawGlyph(proofctx, 
							 glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							 name, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
							 NULL, 0, /* altlabel,altlabelflags */
							 0,0, /* originDx,originDy */
							 0,0, /* origin,originflags */
							 (isVert)? -vwidth : width,ANNOT_NOLINE|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1)|ANNOT_BOLD, /* width,widthflags */
							 NULL, (isVert)?yorig:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
							 );
			  
			  proofSymbol(proofctx, PROOF_YIELDS);
			  label[0] = '\0';
			  proofClearOptions(&options);
				if (xpla || ypla)
				{
				  if (xpla)
					{
					  options.neworigin = xpla;
					  options.neworiginflags = ANNOT_DASHEDLINE|ANNOT_ATBOTTOM;
					}
				  if (ypla)
					{
					  options.newvorigin = ypla;
					  options.newvoriginflags = ANNOT_DASHEDLINE|ANNOT_ATRIGHT;
					}
				}
			  proofDrawGlyph(proofctx, 
							 glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
							 name, 0,   /* glyphname,glyphnameflags */
							 NULL, 0, /* altlabel,altlabelflags */
							 xpla,((isVert)?(-1):(1))*ypla, /* originDx,originDy */
							 0,0, /* origin,originflags */
							 (isVert)?(- (abs(vwidth)+yadv)):(width+xadv), ANNOT_NOLINE|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1)|ANNOT_EMPHASIS, /* width,widthflags */
							 &options, (isVert)?yorig:DEFAULT_YORIG_KANJI, ""
							 );
		#if 0
			if (proofIsVerticalMode())
			  {
				if ((i % 5) == 4)
				  proofNewline(proofctx);
			  }
			else if ((i % 3) == 2)
				proofNewline(proofctx);
			  else
		#endif
				proofThinspace(proofctx, 4);
		  }
	  }

	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);		
	}
	
static void dumpSinglePos2(SinglePosFormat2 *fmt, IntX level , IntX glyphtoproof)
	{
	  if (level == 8) 
	  	proofSinglePos2(fmt, glyphtoproof);
	  else if ((level == 6) || (level ==7))
		{
		  	ttoEnumRec CovList;
		  	Card32 nitems;
		  	IntX i;
		  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		  	for (i = 0; i < (IntX)nitems; i++) 
			{
			  char name[MAX_NAME_LEN];
			  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			  strcpy(name, getGlyphName(glyphId, 0));
			  
			  if (level ==7)
				{
				  fprintf(OUTPUTBUFF,  "%s pos %s ", contextPrefix, name);
				  printValueRecord(ValueXPlacement|ValueYPlacement|ValueXAdvance|ValueYAdvance, &(fmt->Value[i]), level);
				  fprintf(OUTPUTBUFF,  ";\n");
				}
			else
				{
				if (featuretag == STR2TAG("vkrn"))
					{
					DDL(2, (AFMout, "KPY %s GC_ALL_GLYPHS ", name));
					printValueRecord(ValueYAdvance, &(fmt->Value[i]), level);
					}
				else
					{
					DDL(2, (AFMout, "KPX %s GC_ALL_GLYPHS ", name));
					printValueRecord(ValueXAdvance, &(fmt->Value[i]), level);
					}
				DDL(2, (AFMout, " 1\n")); /* terminal 0 means is not a class pair. See GPOSdump() */
				}
			}
			if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
		  }	 
	  else
	    {
	      IntX i;
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "Coverage  =", fmt->Coverage);
		  DLu(2, "ValueFmt  =", fmt->ValueFormat);
		  DLu(2, "ValueCnt  =", fmt->ValueCount);
		  for (i = 0; i < fmt->ValueCount; i++)
			{
			  DL(2, (OUTPUTBUFF, "Value[%d]= ", i));
			  dumpValueRecord(fmt->ValueFormat, &(fmt->Value[i]), level);
			  DL(2, (OUTPUTBUFF, "\n"));
			}
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		}
	}


static void dumpSinglePos(void *fmt, IntX level, IntX glyphtoproof)
	{
	switch (((SinglePosFormat1 *)fmt)->PosFormat)
		{
	case 1:
		dumpSinglePos1(fmt, level, glyphtoproof);
		break;
	case 2:
		dumpSinglePos2(fmt, level, glyphtoproof);
		break;
		}
	}

static IntX SinglePosFlavor(void *fmt)
	{
	  return (((SinglePosFormat1 *)fmt)->PosFormat);
	}


static void dumpExtension1(ExtensionPosFormat1 *fmt, IntX level, void *feattag)
	{
	  DL(2, (OUTPUTBUFF, "PosFormat = 1\n"));
	  DL(2, (OUTPUTBUFF, "LookupType  = %d\n", fmt->ExtensionLookupType));
	  DL(2, (OUTPUTBUFF, "Offset      = %08x\n", fmt->ExtensionOffset));
	  
	  dumpSubtable(fmt->ExtensionOffset, fmt->ExtensionLookupType, fmt->subtable, 
						 level, feattag, GPOSLookupIndex, GPOStableindex, GPOStableCount, 1);
	}
	
static void dumpExtension(void *fmt, IntX level, void *feattag)
	{
	DL(2, (OUTPUTBUFF, "--- ExtensionPos\n"));

	switch (((ExtensionPosFormat1 *)fmt)->PosFormat)
		{
	case 1:
		dumpExtension1(fmt, level, feattag); 
		break;
		}
	}

static IntX ExtensionPosFlavor(void *fmt)
	{
	  return (((ExtensionPosFormat1 *)fmt)->PosFormat);
	}


static IntX searchSinglePos1(SinglePosFormat1 *fmt, const GlyphId glyph, Card16 *vfmt, ValueRecord *vr)
	{
	  IntX where;
	  if (ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, glyph, &where))
		{
		  copyValueRecord(vr, &fmt->Value);
		  *vfmt = fmt->ValueFormat;
		  return 1;
		}	  
	  return 0;
	}

static IntX searchSinglePos2(SinglePosFormat2 *fmt, const GlyphId glyph, Card16 *vfmt, ValueRecord *vr)
	{
	  IntX where;
	  if (ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, glyph, &where))
		{
		  copyValueRecord(vr, &(fmt->Value[where]));
		  *vfmt = fmt->ValueFormat;
		  return 1;
		}
	  return 0;
	}

static IntX searchSinglePos(void *fmt, const GlyphId glyph, Card16 *vfmt, ValueRecord *vr)
	{
	switch (((SinglePosFormat1 *)fmt)->PosFormat)
		{
	case 1:
		return searchSinglePos1((SinglePosFormat1 *)fmt, glyph, vfmt, vr);
		break;
	case 2:
		return searchSinglePos2((SinglePosFormat2 *)fmt, glyph, vfmt, vr);
		break;
		}
	return 0;
	}



static void dumpPairSet(PairSet *pairset, Card16 val1, Card16 val2, IntX level)
	{
	  IntX i;

	  DLu(2, "PairValueCount=", pairset->PairValueCount);
	  if (level < 4) {
		DL(2, (OUTPUTBUFF, "--- PairValueRecord[index]=glyph2 ; glyph1Value , glyph2Value\n"));
		for (i = 0; i < pairset->PairValueCount; i++) 
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%hu ; ", i, pairset->PairValueRecord[i].SecondGlyph));
			dumpValueRecord(val1, &(pairset->PairValueRecord[i].Value1), level);
			DL(2, (OUTPUTBUFF, " , "));
			dumpValueRecord(val2, &(pairset->PairValueRecord[i].Value2), level);
			DL(2, (OUTPUTBUFF, "\n"));
		  }
	  }
	  else { 
		DL(3, (OUTPUTBUFF, "--- PairValueRecord[index]=glyph2 (glyphName/CID) ; glyph1Value , glyph2Value\n"));
		for (i = 0; i < pairset->PairValueCount; i++) 
		  {
			DL(3, (OUTPUTBUFF, "[%d]=%hu (%s) ; ", i, pairset->PairValueRecord[i].SecondGlyph, getGlyphName(pairset->PairValueRecord[i].SecondGlyph, 0)));
			dumpValueRecord(val1, &(pairset->PairValueRecord[i].Value1), level);
			DL(2, (OUTPUTBUFF, " , "));
			dumpValueRecord(val2, &(pairset->PairValueRecord[i].Value2), level);
			DL(2, (OUTPUTBUFF, "\n"));
		  }

	  }
	}

static void proofPosPair1(PosPairFormat1 *fmt, IntX glyphtoproof1, IntX glyphtoproof2)
	{
	  ttoEnumRec CovList;
	  Card32 nitems;
	  IntX i, j;
	  IntX xpla, xadv, yadv, ypla;
	  IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
	  proofOptions options;
	  IntX isVert = proofIsVerticalMode();

	  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	  /* generate the pairs list */
	  for ((glyphtoproof1!=-1)?(i=glyphtoproof1):(i = 0); (glyphtoproof1!=-1)?(i<=glyphtoproof1):(i < (IntX)nitems); i++) 
		{
		  char nam1[MAX_NAME_LEN];
		  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
		  PairSet *pairset = &(fmt->_PairSet[i]);
		  
		  strcpy(nam1, getGlyphName(glyphId, 1));
		  
		  for ((glyphtoproof2!=-1)?(j=glyphtoproof2):(j = 0); (glyphtoproof2!=-1)?(j<=glyphtoproof2):(j < pairset->PairValueCount); j++) 
			{
			  PairValueRecord *pv = &(pairset->PairValueRecord[j]);
			  char nam2[MAX_NAME_LEN];	
			  char label[80];
			  IntX width2, vwidth2, yorig2;
			  strcpy(nam2, getGlyphName(pv->SecondGlyph, 1));

			  label[0] = '\0';

			  if (glyphtoproof1==-1 && !opt_Present("-f")) {
		  			ProofRec * proofRec = da_INDEX(proofrecords, curproofrec);
					proofRec->g1=glyphId;
					proofRec->g2=pv->SecondGlyph;
					proofRec->i=i;
					proofRec->j=j;
					proofRec->fmt=fmt;
					proofRec->type=PairAdjustType;
					proofRec->vert=proofIsVerticalMode();
					curproofrec++;
			  }else{


				  if (!isMMValueFmt(fmt->ValueFormat1))
					{
					  xadv = pv->Value1.XAdvance;
					  xpla = pv->Value2.XPlacement;
					  yadv = pv->Value1.YAdvance;
					  ypla = pv->Value2.YPlacement;
					  if (xadv != 0)
						  sprintf(label,"%d", xadv);
					  else if ((xpla != 0) && (ypla != 0)) 
						  sprintf(label,"%d,%d", xpla, ypla);
					  else if (xpla != 0)
						  sprintf(label,"%d", xpla);
					  else if (yadv != 0)
						  sprintf(label,"%d", yadv);
					  else if (ypla != 0)
						  sprintf(label,"%d", ypla);
					}
					
				  /* get metrics here, because values can be changed below by Alt metrics */
		  		  getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig); 

				  if (label[0] != '\0')
					{
					  getMetrics(pv->SecondGlyph, &origShift, &lsb, &rsb, &width2, &tsb, &bsb, &vwidth2, &yorig2);
					  
					  if (((featuretag == STR2TAG("kern")) ||
						   (featuretag == STR2TAG("vkrn"))) 
						  && proofIsAltKanjiKern())
						{ 
						  ValueRecord leftvr, rightvr;
						  Card16 lvfmt, rvfmt;
						  IntX doLeft, doRight;

						  clearValueRecord(&leftvr);
						  clearValueRecord(&rightvr);
						  doLeft = GPOSlookupFeatureGlyph((featuretag == STR2TAG("vkrn")) ? 
														  (STR2TAG("vpal")) : 
														  (STR2TAG("palt")),
														  glyphId, &lvfmt, &leftvr);
						  doRight = GPOSlookupFeatureGlyph((featuretag == STR2TAG("vkrn")) ?
														   (STR2TAG("vpal")) : 
														   (STR2TAG("palt")),
														  pv->SecondGlyph, &rvfmt, &rightvr);
						  if (doLeft)
							{
					  			width += leftvr.XAdvance;
					  			vwidth = -( abs(vwidth) + leftvr.YAdvance);
					  		}
						  
						  if (doRight)
					  		{
					  			width2 += rightvr.XAdvance;
					  			vwidth2 = -( abs(vwidth2) + rightvr.YAdvance);
					  		}

						  if (isVert)
							proofCheckAdvance(proofctx, abs(vwidth) + abs(vwidth2));
						  else
							proofCheckAdvance(proofctx, width + width2);
						  proofClearOptions(&options);
						  if (isVert)
					  		{
					  		  if (yadv)
					  	  		options.newvwidth = -(abs(vwidth) + yadv);
					  		  else
						  		options.newvwidth = -(abs(vwidth) + xadv);					  	  
					  		}
						  else
					  		{
					  			options.newwidth = width + xadv;
					  		}
						  proofDrawGlyph(proofctx, 
										 glyphId, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 nam1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
										 NULL, 0, /* altlabel,altlabelflags */
										 (doLeft) ? (leftvr.XPlacement) : 0, ((isVert)?(-1):(1))*((doLeft) ? (leftvr.YPlacement) : 0), /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth : width, 0, /* width,widthflags */
										 &options, (isVert)?yorig:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
										 );
						  
						  proofDrawGlyph(proofctx, 
										 pv->SecondGlyph, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 nam2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1),   /* glyphname,glyphnameflags */
										 label, ANNOT_SHOWIT|ANNOT_EMPHASIS|(isVert ? ANNOT_ATRIGHTDOWN2 : ANNOT_ATBOTTOMDOWN2), /* altlabel,altlabelflags */
										 (doRight) ? (rightvr.XPlacement + xpla) : xpla, ((isVert)?(-1):(1))*((doRight) ? (rightvr.YPlacement + ypla) : ypla), /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth2 : width2, 0, /* width,widthflags */
										 NULL, (isVert)?yorig2:DEFAULT_YORIG_KANJI, ""
										 );
						}
					  else /* don't use alt metrics */
						{
						  if (isVert)
							proofCheckAdvance(proofctx, abs(vwidth) + abs(vwidth2));
						  else
							proofCheckAdvance(proofctx, width + width2);
						  proofClearOptions(&options);
						  if (isVert)
					  		{
					  		  if (yadv)
					  	  		options.newwidth = -(abs(vwidth) + yadv);
					  		  else
						  		options.newwidth = -(abs(vwidth) + xadv);					  	  
					  		}
						  else
					  		options.newwidth = width + xadv;
						  proofDrawGlyph(proofctx, 
										 glyphId, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 nam1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
										 NULL, 0, /* altlabel,altlabelflags */
										 0,0, /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth : width,0, /* width,widthflags */
										 &options, (isVert)?yorig:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
										 );
						  
						  proofDrawGlyph(proofctx, 
										 pv->SecondGlyph, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 nam2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1),   /* glyphname,glyphnameflags */
										 label, ANNOT_SHOWIT|ANNOT_BOLD|(isVert ? ANNOT_ATRIGHTDOWN2 : ANNOT_ATBOTTOMDOWN2), /* altlabel,altlabelflags */
										 xpla,((isVert)?(-1):(1))*ypla, /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth2 : width2,0, /* width,widthflags */
										 NULL, (isVert)?yorig2:DEFAULT_YORIG_KANJI, ""
										 );
						}
						proofThinspace(proofctx, 4);
					}
				} /*if printing*/
			} /* forall rights */
		} /* forall lefts */

	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);		
	}
	
	
static void dumpPosPair1(PosPairFormat1 *fmt, IntX level, IntX glyphtoproof1, IntX glyphtoproof2)
	{
	  if (level == 8) 
	  	proofPosPair1(fmt, glyphtoproof1, glyphtoproof2);
	  else if ((level == 6) || (level == 7))
	    {
		  ttoEnumRec CovList;
		  Card32 nitems;
		  IntX i, j;

		  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		  /* generate the pairs list */

		  for (i = 0; i < (IntX)nitems; i++) 
			{
			  char nam1[MAX_NAME_LEN];
			  GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			  PairSet *pairset = &(fmt->_PairSet[i]);
			  strcpy(nam1, getGlyphName(glyphId, 0));			  
			  
			  for (j = 0; j < pairset->PairValueCount; j++) 
				{
				  PairValueRecord *pv = &(pairset->PairValueRecord[j]);
				  char nam2[MAX_NAME_LEN];	
				  strcpy(nam2, getGlyphName(pv->SecondGlyph, 0));
				  if (level == 7)
				  {
					DDL(2, (OUTPUTBUFF, "%s pos %s %s ", contextPrefix, nam1, nam2));
					printValueRecord(fmt->ValueFormat1, &pv->Value1, level);
					DDL(2, (OUTPUTBUFF, ";\n"));
				  }
				  else  /* level 6 */
				  {
				    if (featuretag == STR2TAG("vkrn"))
					    DDL(2, (AFMout, "KPY %s %s ", nam1, nam2));
					 else
					    DDL(2, (AFMout, "KPX %s %s ", nam1, nam2));
					printValueRecord(fmt->ValueFormat1, &pv->Value1, level);
					DDL(2, (AFMout, " 0\n")); /* terminal 0 means is not a class pair. See GPOSdump() */
				  }	 
				}
			}
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
	    }
	   else
		{
		  IntX i;

		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "Coverage  =", fmt->Coverage);
		  DLu(2, "ValueFmt1 =", fmt->ValueFormat1);
		  DLu(2, "ValueFmt2 =", fmt->ValueFormat2);
		  DLs(2, "PairSetCnt=", fmt->PairSetCount);
		  
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		  
		  for (i = 0; i < fmt->PairSetCount; i++) 
			{
			  DL(2, (OUTPUTBUFF, "--- PairSet[%d] (%04hx)\n", i, fmt->PairSet[i]));
			  dumpPairSet(&(fmt->_PairSet[i]), fmt->ValueFormat1, fmt->ValueFormat2, level);
			}
		}
	}

static void proofPosPair2(PosPairFormat2 *fmt, IntX glyphtoproof1, IntX glyphtoproof2, IntX c2toproof)
	{
	  /* Note that this function sis called twice when option -f is NOT used. One th first pass, all the non-zero kern class kern
	  pairs are expanded ino thte prrofrecord list. these are then sorted, and this function is called again
	  with the glyphtoproof1, IntX glyphtoproof2, IntX c2toproof set, proofing one pair with each call.
	  */
	  ttoEnumRec *class1;
	  Card32 class1count;
	  ttoEnumRec *class2;
	  Card32 class2count;
	  IntX c1, c2, c1g, c2g;
	  IntX xpla, xadv, ypla, yadv;
	  IntX origShift, lsb, rsb, width, width2, tsb, bsb, vwidth, vwidth2, yorig , yorig2;
	  proofOptions options;
	  IntX isVert = proofIsVerticalMode();
	  char nam1[MAX_NAME_LEN];
	  char nam2[MAX_NAME_LEN];
	  char label[80];
	  ttoEnumRec CovList;
	  Card32 nitems, i;

	  class1 = memNew(sizeof(ttoEnumRec) * fmt->Class1Count);
	  ttoEnumerateClass(fmt->ClassDef1, fmt->_ClassDef1,
						fmt->Class1Count,
						class1, &class1count);
	  class2 = memNew(sizeof(ttoEnumRec) * fmt->Class2Count);
	  ttoEnumerateClass(fmt->ClassDef2, fmt->_ClassDef2, 
						fmt->Class2Count,
						class2, &class2count);

	
	  /* generate the pairs list */
	  /* Step through each glyph in the coverage table. For each glyph:
	 search the class definitions in the Class1 Def for the glyph. When find it:
	  	get the matching Class1 record
	  	for each class2 record in the Class 1 record:
	  		for each glyph in the matching Class 2 class definition:
	  			if re-ordering, or proofing specific glyphs, then append the kern pair and value to the proofrecords, else
	  			write it out to the proof file.
	  	if not re-ordering the font or adding specific glyphs, then write  a new line after processing
	  	each glyph in the coverage table.
	  */
	  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	  for ((glyphtoproof1!=-1)?(i=glyphtoproof1):(i = 0); (glyphtoproof1!=-1)?(((IntX)i)<=glyphtoproof1):(i < nitems); i++) 
		{
		   GlyphId g1;
		   Class1Record *rec1;
		   GlyphId glyphId = *da_INDEX(CovList.glyphidlist, (Int32)i);
		   int found=0;
		  	/* find the class definition that contains the glyphId */
		    for (c1 = 0; ((!found) && (c1 < fmt->Class1Count)); c1++) 
			{ 
			  ttoEnumRec *CER1 = &(class1[c1]);
			  rec1 = &(fmt->Class1Record[c1]);

			  for (c1g = 0; ((!found) && (c1g < CER1->glyphidlist.cnt)); c1g++) 
				{ /* for each glyph in that class */
				  g1 = *da_INDEX(CER1->glyphidlist, c1g);
				  if (g1!=glyphId)
					  continue;
				  found=1;
				} /* for each glyph in that class */
			}
			
			if (!found) /* this happens with Class definition format 2, where class 0 is not explicitly defined. */
				{
				rec1 = &(fmt->Class1Record[0]);
				g1 = glyphId;
				}
				
			strcpy(nam1, getGlyphName(g1, 1));
			getMetrics(g1, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
				
			/*  for (c2 = 0; c2 < fmt->Class2Count; c2++)*/
			  for ((c2toproof!=-1)?(c2=c2toproof):(c2 = 0); (c2toproof!=-1)?(c2<=c2toproof):(c2 < fmt->Class2Count); c2++) 
				{ /* for each other class */
				  ttoEnumRec *CER2 = &(class2[c2]);
				  Class2Record *rec2 = &(rec1->Class2Record[c2]);
				  
				  /*for (c2g = 0; c2g < CER2->glyphidlist.cnt; c2g++)*/
				  for ((glyphtoproof2!=-1)?(c2g=glyphtoproof2):(c2g = 0); (glyphtoproof2!=-1)?(c2g<=glyphtoproof2):(c2g < CER2->glyphidlist.cnt); c2g++) 
					{
					  GlyphId g2 = *da_INDEX(CER2->glyphidlist, c2g);
					  strcpy(nam2, getGlyphName(g2, 1));
					  getMetrics(g2, &origShift, &lsb, &rsb, &width2, &tsb, &bsb, &vwidth2, &yorig2);
					  label[0] = '\0';
						if (glyphtoproof1==-1 && !opt_Present("-f")) {
		  						ProofRec * proofRec = da_INDEX(proofrecords, curproofrec);
								proofRec->g1=glyphId;
								proofRec->g2=g2;
								proofRec->i=i;
								proofRec->j=c2g;
								proofRec->c2=c2;
								proofRec->fmt=fmt;
								proofRec->type=PairAdjustType;
								proofRec->vert=proofIsVerticalMode();
								curproofrec++;
						}else{


						  if (!isMMValueFmt(fmt->ValueFormat1))
							{
							  xadv = rec2->Value1.XAdvance;
							  xpla = rec2->Value2.XPlacement;
							  yadv = rec2->Value1.YAdvance;
							  ypla = rec2->Value2.YPlacement;
							  if (xadv != 0)
								  sprintf(label,"%d", xadv);
							  else if (yadv != 0)
								  sprintf(label,"%d", yadv);
							  else if ((xpla != 0) && (ypla != 0)) 
								  sprintf(label,"%d, %d", xpla, ypla);
							  else if (xpla != 0)
								  sprintf(label,"%d", xpla);
							  else if (ypla != 0)
								  sprintf(label,"%d", ypla);
							}
						  /* skip proofing glyph if all pos values are 0. */
						  if (label[0] != '\0')
							{
							  if (isVert)
								proofCheckAdvance(proofctx, abs(vwidth) + abs(vwidth2));
							  else
								proofCheckAdvance(proofctx, width + width2);
							  proofClearOptions(&options);
                                                          if (isVert)
                                                                  options.newvwidth = vwidth + yadv;
                                                          else
                                                                  options.newwidth = width + xadv;
							  proofDrawGlyph(proofctx, 
											 g1, ANNOT_SHOWIT, /* glyphId,glyphflags */
											 nam1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
											 NULL, 0, /* altlabel,altlabelflags */
											 0,0, /* originDx,originDy */
											 0,0, /* origin,originflags */
											 (isVert) ? vwidth : width,0, /* width,widthflags */
											 &options, (isVert)?yorig:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
											 );
							  proofDrawGlyph(proofctx, 
											 g2, ANNOT_SHOWIT, /* glyphId,glyphflags */
											 nam2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1),   /* glyphname,glyphnameflags */
											 label, ANNOT_SHOWIT|ANNOT_BOLD|(isVert ? ANNOT_ATRIGHTDOWN2 : ANNOT_ATBOTTOMDOWN2), /* altlabel,altlabelflags */
											 xpla,((isVert)?(-1):(1))*ypla, /* originDx,originDy */
											 0,0, /* origin,originflags */
											 (isVert) ? vwidth2 : width2,0, /* width,widthflags */
											 NULL, (isVert)?yorig2:DEFAULT_YORIG_KANJI, ""
											 );
							  proofThinspace(proofctx, 2);
							} /* end if kern pair has a non-zero value. */
						  } /* end if sorting kenr pairs for output - else */
					} /* end for each glyph in class2 class definition */
				} /* end for all class2 records in current Class1record */
			} /* end for all nitems in coverage table */
		
	  for (c1 = 0; c1 < fmt->Class1Count; c1++) 
		{
		  if (class1[c1].glyphidlist.size > 0)
			da_FREE(class1[c1].glyphidlist);
		}
	  for (c2 = 0; c2 < fmt->Class2Count; c2++)
		{
		  if (class2[c2].glyphidlist.size > 0)
			da_FREE(class2[c2].glyphidlist);
		}
	  memFree(class1);
	  memFree(class2);
	  if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	}

static void dumpPosPair2(PosPairFormat2 *fmt, IntX level, IntX glyphtoproof1, IntX glyphtoproof2, IntX c2toproof)
	{

	  if (level == 8) 
	    proofPosPair2(fmt, glyphtoproof1, glyphtoproof2, c2toproof);
	  else if (level == 7)
		{
		  ttoEnumRec CovList;
		  Card32 nitems;
		  ttoEnumRec *class1;
		  Card32 class1count;
		  ttoEnumRec *class2;
		  Card32 class2count;
		  IntX c1, c2, c1g, c2g;
		  char nam1[MAX_NAME_LEN];
		  char nam2[MAX_NAME_LEN];
		  bit_declp(CoverageSet);

		  class1 = memNew(sizeof(ttoEnumRec) * fmt->Class1Count);
		  ttoEnumerateClass(fmt->ClassDef1, fmt->_ClassDef1,
							fmt->Class1Count,
							class1, &class1count);
		  class2 = memNew(sizeof(ttoEnumRec) * fmt->Class2Count);
		  ttoEnumerateClass(fmt->ClassDef2, fmt->_ClassDef2, 
							fmt->Class2Count,
							class2, &class2count);

		  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		  /* create the set of glyphs in the Coverage */
		  bit_calloc(CoverageSet, (CovList.maxgid + 1));
		  for (c1 = 0; c1 < (IntX)nitems; c1++)
			{
			  GlyphId g = *da_INDEX(CovList.glyphidlist, c1);
			  bit_set(CoverageSet, g);
			}
		  /* now remove from the CoverageSet all elements that 
			 are in the other "left" classes */
		  for (c1 = 1; c1 < fmt->Class1Count; c1++) /* Note! skip class 0 */
			{ 
			  ttoEnumRec *CER1 = &(class1[c1]);
			  if (CER1->glyphidlist.cnt > 0)
				  for (c1g = 0; c1g < CER1->glyphidlist.cnt; c1g++) 
					{
					  GlyphId g1 = *da_INDEX(CER1->glyphidlist, c1g);
					  if (bit_test(CoverageSet, g1))
						bit_clear(CoverageSet, g1);
					}
			}

		  /* the remaining elements are in LEFT_CLASS_0 */

		  DDL(7, (OUTPUTBUFF, "# -------- Glyph Classes\n"));

		  /* output LEFT_CLASS_0, if it is not empty by this point. */
		  {
			IntX seenGlyph = 0;
			  for (c1 = CovList.mingid; c1 <= CovList.maxgid; c1++)
				{
				  if (bit_test(CoverageSet, c1))
					{
					 if (!seenGlyph)
						{
						DDL(7, (OUTPUTBUFF, "@LEFT_CLASS_c%d_s%d_l%d = [\n\t", 0, GPOStableindex, GPOSLookupIndex));
						seenGlyph = 1;
						}
					  strcpy(nam1, getGlyphName(c1, 0));
					  DDL(7, (OUTPUTBUFF, "%s ",nam1));
					  if ((c1 % 5) == 4) DDL(7,(OUTPUTBUFF, "\n\t"));
					}
				}
			
			if (seenGlyph)
			  DDL(7, (OUTPUTBUFF, "\n];\n"));
		}
		  /* generate the pairs list */
		  for (c1 = 1; c1 < fmt->Class1Count; c1++) 
			{ 
			  ttoEnumRec *CER1 = &(class1[c1]);
			  IntX where; 

			  if (CER1->glyphidlist.cnt > 0)
				{
				  DDL(7, (OUTPUTBUFF, "@LEFT_CLASS_c%d_s%d_l%d = [\n\t", c1, GPOStableindex, GPOSLookupIndex));
				  
				  for (c1g = 0; c1g < CER1->glyphidlist.cnt; c1g++) 
					{ /* for each glyph in that class */
					  GlyphId g1 = *da_INDEX(CER1->glyphidlist, c1g);
					  if (! ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, g1, &where))
						  continue;						  
					  strcpy(nam1, getGlyphName(g1, 0));
					  DDL(7, (OUTPUTBUFF, "%s ",nam1));
					  if ((c1g % 5) == 4) DDL(7,(OUTPUTBUFF, "\n\t"));
					}
				  DDL(2, (OUTPUTBUFF, "\n];\n"));
				}

			}
		  for (c2 = 0; c2 < fmt->Class2Count; c2++)
			{ /* for each other class */
			  ttoEnumRec *CER2 = &(class2[c2]);
			  if (CER2->glyphidlist.cnt > 0)
				{
				  DDL(7, (OUTPUTBUFF, "@RIGHT_CLASS_c%d_s%d_l%d = [\n\t", c2, GPOStableindex, GPOSLookupIndex));
				  for (c2g = 0; c2g < CER2->glyphidlist.cnt; c2g++)
					{
					  GlyphId g2 = *da_INDEX(CER2->glyphidlist, c2g);
					  strcpy(nam2, getGlyphName(g2, 0));
					  DDL(7, (OUTPUTBUFF, "%s ",nam2));
					  if ((c2g % 5) == 4) DDL(7,(OUTPUTBUFF, "\n\t"));
					}
				  DDL(7, (OUTPUTBUFF, "\n];\n"));
				}
			}

		  for (c1 = 0; c1 < fmt->Class1Count; c1++) 
			{ 
			  Class1Record *rec1 = &(fmt->Class1Record[c1]);
			  
			  for (c2 = 0; c2 < fmt->Class2Count; c2++)
				{ /* for each other class */
				  Class2Record *rec2 = &(rec1->Class2Record[c2]);
				  ttoEnumRec *CER2 = &(class2[c2]);
				  if (CER2->glyphidlist.cnt > 0)
					{
					  DDL(7, (OUTPUTBUFF, "%s pos @LEFT_CLASS_c%d_s%d_l%d @RIGHT_CLASS_c%d_s%d_l%d  ", contextPrefix, c1, GPOStableindex, GPOSLookupIndex, c2, GPOStableindex, GPOSLookupIndex));
					  if (fmt->ValueFormat1 != 0x0)
						printValueRecord(fmt->ValueFormat1, &rec2->Value1, level);
					  else
						printValueRecord(fmt->ValueFormat2, &rec2->Value2, level);
					  DDL(7, (OUTPUTBUFF, ";\n"));		
					}
				}
			}
	   	
		  for (c1 = 0; c1 < fmt->Class1Count; c1++) 
			{
			  if (class1[c1].glyphidlist.size > 0)
				da_FREE(class1[c1].glyphidlist);
			}
		  for (c2 = 0; c2 < fmt->Class2Count; c2++)
			{
			  if (class2[c2].glyphidlist.size > 0)
				da_FREE(class2[c2].glyphidlist);
			}
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);

		  bit_free(CoverageSet);

		  memFree(class1);
		  memFree(class2);
		}
	  else if (level == 6)
		{
		  ttoEnumRec CovList;
		  Card32 nitems;
		  ttoEnumRec *class1;
		  Card32 class1count;
		  ttoEnumRec *class2;
		  Card32 class2count;
		  IntX c1, c2, c1g, c2g;
		  IntX xpla, xadv, ypla, yadv;
		  char nam1[MAX_NAME_LEN];
		  char nam2[MAX_NAME_LEN];
		  bit_declp(CoverageSet);

		  class1 = memNew(sizeof(ttoEnumRec) * fmt->Class1Count);
		  ttoEnumerateClass(fmt->ClassDef1, fmt->_ClassDef1,
							fmt->Class1Count,
							class1, &class1count);
		  class2 = memNew(sizeof(ttoEnumRec) * fmt->Class2Count);
		  ttoEnumerateClass(fmt->ClassDef2, fmt->_ClassDef2, 
							fmt->Class2Count,
							class2, &class2count);

		  ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		  /* create the set of glyphs in the Coverage */
		  bit_calloc(CoverageSet, (CovList.maxgid + 1));
		  for (c1 = 0; c1 < (IntX)nitems; c1++)
			{
			  GlyphId g = *da_INDEX(CovList.glyphidlist, c1);
			  bit_set(CoverageSet, g);
			}
		  /* now remove from the CoverageSet all elements that 
			 are in the other "left" classes */
		  for (c1 = 1; c1 < fmt->Class1Count; c1++) 
			{ 
			  ttoEnumRec *CER1 = &(class1[c1]);
			  if (CER1->glyphidlist.cnt > 0)
				  for (c1g = 0; c1g < CER1->glyphidlist.cnt; c1g++) 
					{
					  GlyphId g1 = *da_INDEX(CER1->glyphidlist, c1g);
					  if (bit_test(CoverageSet, g1))
						bit_clear(CoverageSet, g1);
					}
			}

		  /* generate the pairs list for Left Class == 0 */
		  for (c1 = CovList.mingid; c1 <= CovList.maxgid; c1++)
			{
			  if (bit_test(CoverageSet, c1))
				{
				  Class1Record *rec1 = &(fmt->Class1Record[0]);
				  strcpy(nam1, getGlyphName(c1, 0));

				  for (c2 = 0; c2 < fmt->Class2Count; c2++)
					{ /* for each other class */
					  ttoEnumRec *CER2 = &(class2[c2]);
					  Class2Record *rec2 = &(rec1->Class2Record[c2]);
					  for (c2g = 0; c2g < CER2->glyphidlist.cnt; c2g++)
						{
						  int isVal;
						  GlyphId g2 = *da_INDEX(CER2->glyphidlist, c2g);
						  isVal = 0;
						  strcpy(nam2, getGlyphName(g2, 0));
						  if (isMMValueFmt(fmt->ValueFormat1))
							{
							  DDL(2, (AFMout, "KPX %s %s ", nam1, nam2));
							  printValueRecord(fmt->ValueFormat1, &rec2->Value1, level);
							  isVal = 1;
							}
						  else if (isMMValueFmt(fmt->ValueFormat2))
							{
							  DDL(2, (AFMout, "KPX %s %s ", nam1, nam2));
							  printValueRecord(fmt->ValueFormat2, &rec2->Value2, level);
							  isVal = 1;
							}
						  else
							{
							  xadv = rec2->Value1.XAdvance;
							  xpla = rec2->Value2.XPlacement;
							  yadv = rec2->Value1.YAdvance;
							  ypla = rec2->Value2.YPlacement;
							  /* We can afford to filter out zero-valued class pairs, becuase there
							  is no use for them. We cannot filter out zerp valued single pairs, as they
							  might be exceptions to class pairs. */
							  if (xadv != 0)
								{ DDL(2, (AFMout, "KPX %s %s %d", nam1, nam2, xadv)); isVal = 1;}
							  else if (yadv != 0)
								{ DDL(2, (AFMout, "KPY %s %s %d", nam1, nam2, yadv)); isVal = 1;}
							  else if ((xpla != 0) && (ypla != 0)) 
								{ DDL(2, (AFMout, "KPXY %s %s %d %d", nam1, nam2, xpla, ypla));  isVal = 1;}
							  else if (xpla != 0)
								{ DDL(2, (AFMout, "KPX2 %s %s %d", nam1, nam2, xpla)); isVal = 1;}
							  else if (ypla != 0)
								{ DDL(2, (AFMout, "KPY2 %s %s %d", nam1, nam2, ypla));  isVal = 1;}
							
							}
						if (isVal)
							DDL(2, (AFMout, " 1\n"));/* the final 1 means is class pair. See GPOSdump() */
						}
					}
				}
			}

		  bit_free(CoverageSet);
		  
		  /* generate the pairs list for Class > 0 */
		  for (c1 = 1; c1 < fmt->Class1Count; c1++) 
			{ 
			  ttoEnumRec *CER1 = &(class1[c1]);
			  Class1Record *rec1 = &(fmt->Class1Record[c1]);
			  IntX where;
			  
			  for (c1g = 0; c1g < CER1->glyphidlist.cnt; c1g++) 
				{ /* for each glyph in that class */
				  GlyphId g1 = *da_INDEX(CER1->glyphidlist, c1g);

				  if (! ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, g1, &where))
					  continue;

				  strcpy(nam1, getGlyphName(g1, 0));

				  for (c2 = 0; c2 < fmt->Class2Count; c2++)
					{ /* for each other class */
					  ttoEnumRec *CER2 = &(class2[c2]);
					  Class2Record *rec2 = &(rec1->Class2Record[c2]);

					  for (c2g = 0; c2g < CER2->glyphidlist.cnt; c2g++)
						{
						  int isVal;
						  GlyphId g2 = *da_INDEX(CER2->glyphidlist, c2g);
						  isVal = 0;
						  strcpy(nam2, getGlyphName(g2, 0));
						  if (isMMValueFmt(fmt->ValueFormat1))
							{
							  DDL(2, (AFMout, "KPX %s %s", nam1, nam2));
							  printValueRecord(fmt->ValueFormat1, &rec2->Value1, level);
							  isVal = 1;
							}
						  else if (isMMValueFmt(fmt->ValueFormat2))
							{
							  DDL(2, (AFMout, "KPX %s %s", nam1, nam2));
							  printValueRecord(fmt->ValueFormat2, &rec2->Value2, level);
							  isVal = 1;
							}
						  else
							{
							  xadv = rec2->Value1.XAdvance;
							  xpla = rec2->Value2.XPlacement;
							  yadv = rec2->Value1.YAdvance;
							  ypla = rec2->Value2.YPlacement;
							  
							  if (xadv != 0)
								{ DDL(2, (AFMout, "KPX %s %s %d", nam1, nam2, xadv)); isVal = 1;}
							  else if (yadv != 0)
								{ DDL(2, (AFMout, "KPY %s %s %d", nam1, nam2, yadv)); isVal = 1;}
							  else if ((xpla != 0) && (ypla != 0)) 
								{ DDL(2, (AFMout, "KPXY %s %s %d %d", nam1, nam2, xpla, ypla)); isVal = 1;}
							  else if (xpla != 0)
								{ DDL(2, (AFMout, "KPX2 %s %s %d", nam1, nam2, xpla)); isVal = 1;}
							  else if (ypla != 0)
								{ DDL(2, (AFMout, "KPY2 %s %s %d", nam1, nam2, ypla));isVal = 1; }
							}
						if (isVal)
							DDL(2, (AFMout, " 1\n"));/* the final 1 means is class pair. See GPOSdump() */
						}
					}
				}
			}
		  for (c1 = 0; c1 < fmt->Class1Count; c1++) 
			{
			  if (class1[c1].glyphidlist.size > 0)
				da_FREE(class1[c1].glyphidlist);
			}
		  for (c2 = 0; c2 < fmt->Class2Count; c2++)
			{
			  if (class2[c2].glyphidlist.size > 0)
				da_FREE(class2[c2].glyphidlist);
			}
		  memFree(class1);
		  memFree(class2);
		  if (CovList.glyphidlist.size > 0)
			da_FREE(CovList.glyphidlist);
		}
	  else
		{
		  IntX i, j;
		  
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "Coverage  =", fmt->Coverage);
		  DLu(2, "ValueFmt1 =", fmt->ValueFormat1);
		  DLu(2, "ValueFmt2 =", fmt->ValueFormat2);
		  DLx(2, "ClassDef1 =", fmt->ClassDef1);
		  DLx(2, "ClassDef2 =", fmt->ClassDef2);
		  DLu(2, "Class1Cnt =", fmt->Class1Count);
		  DLu(2, "Class2Cnt =", fmt->Class2Count);
		  DL(2, (OUTPUTBUFF, "Coverage:\n"));
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		  DL(2, (OUTPUTBUFF, "Class 1:\n"));
		  ttoDumpClass(fmt->ClassDef1, fmt->_ClassDef1, level);
		  DL(2, (OUTPUTBUFF, "Class 2:\n"));
		  ttoDumpClass(fmt->ClassDef2, fmt->_ClassDef2, level);
		  
		  DL(2, (OUTPUTBUFF, "--- Class1Record[class1value][class2value] = Value1 , Value2"));
		  for (i = 0; i < fmt->Class1Count; i++) 
			{
			  Class1Record *record1 = &fmt->Class1Record[i];
			  for (j = 0; j < fmt->Class2Count; j++) 
				{
				  Class2Record *record2 = &(record1->Class2Record[j]);
				  DL(2, (OUTPUTBUFF, "\n[%d][%d] = ", i, j));
				  dumpValueRecord(fmt->ValueFormat1, &(record2->Value1), level);
				  DL(2, (OUTPUTBUFF, " , "));
				  dumpValueRecord(fmt->ValueFormat2, &(record2->Value2), level);
				}
			}
		  DL(2, (OUTPUTBUFF, "\n"));
		}
	}


static void dumpPosPair(void *fmt, IntX level, IntX glyphtoproof1, IntX glyphtoproof2, IntX c2toproof)
	{
	switch (((PosPairFormat1 *)fmt)->PosFormat)
		{
	case 1:
		dumpPosPair1(fmt, level, glyphtoproof1, glyphtoproof2);
		break;
	case 2:
		dumpPosPair2(fmt, level, glyphtoproof1, glyphtoproof2, c2toproof);
		break;
		}
	}

static IntX searchPosPair1(PosPairFormat1 *fmt, const GlyphId glyph1, const GlyphId glyph2, Card16 *vfmt1, ValueRecord *vr1, Card16 *vfmt2, ValueRecord *vr2)
	{
	  IntX i, j, where;

	  if (ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, glyph1, &where))
		{
		  for (i = 0; i < fmt->PairSetCount; i++) 
			{
			  PairSet *pairset = &(fmt->_PairSet[i]);
			  for (j=0; j < pairset->PairValueCount; j++)
				{
				  if (pairset->PairValueRecord[j].SecondGlyph == glyph2)
					{
				  /* ??? Should (j == where) here ??? */

					  copyValueRecord(vr1, &(pairset->PairValueRecord[j].Value1));
					  copyValueRecord(vr2, &(pairset->PairValueRecord[j].Value2));
					  *vfmt1 = fmt->ValueFormat1;
					  *vfmt2 = fmt->ValueFormat2;
					  return 1;
					}
				}
			}
		}
	  return 0;
	}

static IntX searchPosPair2(PosPairFormat2 *fmt, const GlyphId glyph1, const GlyphId glyph2, Card16 *vfmt1, ValueRecord *vr1, Card16 *vfmt2, ValueRecord *vr2)
	{
	  IntX where1, where2, covwhere;

	  if (ttoGlyphIsInCoverage(fmt->Coverage, fmt->_Coverage, glyph1, &covwhere))
		{
		  if (ttoGlyphIsInClass(fmt->ClassDef1, fmt->_ClassDef1, glyph1, &where1) &&
			  ttoGlyphIsInClass(fmt->ClassDef2, fmt->_ClassDef2, glyph2, &where2))
			{
			  Class1Record *record1;
			  Class2Record *record2;
			  if ((where1 < fmt->Class1Count) && (where2 < fmt->Class2Count))
				{
				  record1 = &fmt->Class1Record[where1];
				  record2 = &(record1->Class2Record[where2]);
				  copyValueRecord(vr1, &(record2->Value1));
				  copyValueRecord(vr2, &(record2->Value2));
				  *vfmt1 = fmt->ValueFormat1;
				  *vfmt2 = fmt->ValueFormat2;
				  return 1;
				}
			}
		}
	  return 0;
	}

static IntX searchPosPair(void *fmt, const GlyphId glyph1, const GlyphId glyph2, Card16 *vfmt1, ValueRecord *vr1, Card16 *vfmt2, ValueRecord *vr2)
	{
	switch (((PosPairFormat1 *)fmt)->PosFormat)
		{
	case 1:
		return searchPosPair1((PosPairFormat1 *)fmt, glyph1, glyph2, vfmt1, vr1, vfmt2, vr2);
		break;
	case 2:
		return searchPosPair2((PosPairFormat2 *)fmt, glyph1, glyph2, vfmt1, vr1, vfmt2, vr2);
		break;
		}
	return 0;
	}

static IntX PosPairFlavor(void *fmt)
	{
	  return (((PosPairFormat1 *)fmt)->PosFormat);
	}

static void dumpAnchorFormat1(AnchorFormat1 *fmt, IntX level)
	{
	if (level == 7)
		{
		fprintf(OUTPUTBUFF,  "<anchor %d %d>", fmt->XCoordinate, fmt->YCoordinate);
		}
	else
		{
		DL(2, (OUTPUTBUFF, " AnchorFormat = %d,", fmt->AnchorFormat));
		DL(2, (OUTPUTBUFF, " XCoordinate= %d,", fmt->XCoordinate));
		DL(2, (OUTPUTBUFF, " YCoordinate= %d\n", fmt->YCoordinate));
		}
	}

static void dumpAnchorFormat2(AnchorFormat2 *fmt, IntX level)
	{
	if (level == 7)
		{
		fprintf(OUTPUTBUFF,  "<anchor %d %d contourpoint %d>", fmt->XCoordinate, fmt->YCoordinate, fmt->AnchorPoint);
		}
	else
		{
		DL(2, (OUTPUTBUFF, " AnchorFormat = %d,", fmt->AnchorFormat));
		DL(2, (OUTPUTBUFF, " XCoordinate= %d,", fmt->XCoordinate));
		DL(2, (OUTPUTBUFF, " YCoordinate= %d,", fmt->YCoordinate));
		DL(2, (OUTPUTBUFF, " AnchorPoint= %d\n", fmt->AnchorPoint));
		}
	}

static void dumpAnchorFormat3(AnchorFormat3 *fmt, IntX level)
	{
	if (level == 7)
		{
		fprintf(OUTPUTBUFF,  "<anchor %d %d ", fmt->XCoordinate, fmt->YCoordinate);
		if (fmt->XDeviceTable != 0)
			{
			ttoDumpDeviceTable(fmt->XDeviceTable, fmt->_XDeviceTable, level);
			fprintf(OUTPUTBUFF,  " ");
			}
		else
			fprintf(OUTPUTBUFF,  "<device NULL> ");
		
		if (fmt->YDeviceTable != 0)
			{
			ttoDumpDeviceTable(fmt->YDeviceTable, fmt->_YDeviceTable, level);
			}
		else
			fprintf(OUTPUTBUFF,  "<device NULL>");
		fprintf(OUTPUTBUFF,  ">");
		}
	else
		{
		DL(2, (OUTPUTBUFF, " AnchorFormat = %d,", fmt->AnchorFormat));
		DL(2, (OUTPUTBUFF, " XCoordinate= %d,", fmt->XCoordinate));
		DL(2, (OUTPUTBUFF, " YCoordinate= %d,", fmt->YCoordinate));
		DL(2, (OUTPUTBUFF, " XDeviceTable= (%04hx)\n,", fmt->XDeviceTable));
		DL(2, (OUTPUTBUFF, " YDeviceTable= (%04hx)\n", fmt->YDeviceTable));

		  if (fmt->XDeviceTable != 0)
		  	{
		  	ttoDumpDeviceTable(fmt->XDeviceTable, fmt->_XDeviceTable, level);
		  	}
		  if (fmt->YDeviceTable != 0)
		  	{
		  	ttoDumpDeviceTable(fmt->YDeviceTable, fmt->_YDeviceTable, level);
		  	}
		}

	}

static void dumpAnchorRecord(Card32 offset, void *fmt, IntX level)
	{
	 DL(2, (OUTPUTBUFF, "--- AnchorRecord (%0x)\n", offset));
	if (offset == 0)
		return;
		
	  switch (((AnchorFormat1 *)fmt)->AnchorFormat)
		{
		case 1:
		   dumpAnchorFormat1( (AnchorFormat1*)fmt, level);
		   break;
		case 2:
		   dumpAnchorFormat2( (AnchorFormat2*)fmt, level);
		   break;
		case 3:
		   dumpAnchorFormat3( (AnchorFormat3*)fmt, level);
		   break;
		}
	}

static void dumpCursiveAttachPos(CursivePosFormat1 *fmt, IntX level)
	{
	IntX i;
	  if (level == 8) 
	  	 warning( SPOT_MSG_GPOSUFMTCTX, fmt->PosFormat);
	  else if (level == 6)
	  /* skip - this if for kerning only. */;
	  else if (level == 7)
	  	{
	  	ttoEnumRec CovList;
	  	Card32 nitems;
	  	IntX i;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
	    for (i = 0; i < (IntX)nitems; i++) 
			{
			char name[MAX_NAME_LEN];
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			IntX offsetAnchor;
			void * anchorTable;
			EntryExitRecord *record = &(fmt->EntryExitRecord[i]);

			strcpy(name, getGlyphName(glyphId, 0));
			fprintf(OUTPUTBUFF,  "pos cursive %s ", name);

			offsetAnchor = record->EntryAnchor;
			if (offsetAnchor != 0)
				{
				anchorTable = record->_EntryAnchor;
				dumpAnchorRecord(offsetAnchor, anchorTable, level);
				}
			else
				fprintf(OUTPUTBUFF, "<anchor NULL>");
			fprintf(OUTPUTBUFF,  " ");
			
			offsetAnchor = record->ExitAnchor;
			if (offsetAnchor != 0)
				{
				anchorTable = record->_ExitAnchor;
				dumpAnchorRecord(offsetAnchor, anchorTable, level);
				}
			else
				fprintf(OUTPUTBUFF, "<anchor NULL>");
			fprintf(OUTPUTBUFF,  ";\n");
			 }
		 
	   if (CovList.glyphidlist.size > 0)
		da_FREE(CovList.glyphidlist);
	    }
	   else
		{
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "Coverage  =", fmt->Coverage);
		  DLu(2, "EntryExitCount  =", fmt->EntryExitCount);
		  
		  ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		  
		 for (i = 0; i <fmt->EntryExitCount; i++)
			{
				IntX offsetAnchor;
				void * anchorTable;
 		 		EntryExitRecord *record = &(fmt->EntryExitRecord[i]);
 		 		
			 	DL(2, (OUTPUTBUFF, "--- EntryExitRecord index %d\n", i));
				DL(2, (OUTPUTBUFF, " EntryAnchor offset (%04hx)\n", record->EntryAnchor));
				DL(2, (OUTPUTBUFF, " ExitAnchor offset (%04hx)\n", record->ExitAnchor));
				offsetAnchor = record->EntryAnchor;
				if (offsetAnchor != 0)
					{
					anchorTable = record->_EntryAnchor;
	    			dumpAnchorRecord(offsetAnchor, anchorTable, level);
					}
				else
			 		DL(2, (OUTPUTBUFF, "EntryAnchor Table offset is NULL\n"));
			 		
				offsetAnchor = record->ExitAnchor;
				if (offsetAnchor != 0)
					{
					anchorTable = record->_ExitAnchor;
	    			dumpAnchorRecord(offsetAnchor, anchorTable, level);
					}
				else
			 		DL(2, (OUTPUTBUFF, "ExitAnchor Table offset %d is NULL\n", i));

			}

		}
	}

static void dumpMarkArray(Card32 offset, MarkArray *markarray, IntX level)
	{
	 IntX i;
	 IntX j;
	 da_DCL(MarkRecord*, uniqueAnchorTables);
	 DL(2, (OUTPUTBUFF, "--- MarkArray (%04x)\n", offset));
	 DLu(2, "MarkCount =", markarray->MarkCount);
	 DL(2, (OUTPUTBUFF, "--- MarkRecord[index] = Class, AnchorRecord\n"));
	
	 da_INIT(uniqueAnchorTables, markarray->MarkCount, 10);
	 for (i = 0; i < markarray->MarkCount; i++)
		{
		IntX seenRecord = 0;
		 MarkRecord *record = &markarray->MarkRecord[i];
		 DL(2, (OUTPUTBUFF, " [%d]= Class %d Offset %04hx\n", i, record->Class, record->MarkAnchor));
		 for (j = 0; j < uniqueAnchorTables.cnt; j++)
			{
			 MarkRecord **tempRecord = da_INDEX(uniqueAnchorTables, j);
			if (record->MarkAnchor == (*tempRecord)->MarkAnchor)
				{
				seenRecord = 1;
				break;
				}
			}
		if (seenRecord == 0)
			{
			MarkRecord **tempRecord = da_NEXT(uniqueAnchorTables);
			*tempRecord = record;
			}
		}
	 DL(2, (OUTPUTBUFF, "\n"));
	
	 for (i = 0; i < uniqueAnchorTables.cnt; i++)
		{
		MarkRecord **record = da_INDEX(uniqueAnchorTables, i);
	    dumpAnchorRecord((*record)->MarkAnchor, (*record)->_MarkAnchor, level);
		}
		
	 da_FREE(uniqueAnchorTables);
	}

static void dumpBaseArray(Card32 offset, BaseArray *basearray, Card32 classCount, IntX level)
	{
	 IntX i;
	 IntX j;
	 IntX m;
	 da_DCL(AnchorRecord, uniqueAnchorTables);
	 DL(2, (OUTPUTBUFF, "--- BaseArray (%04x)\n", offset));
	 DLu(2, "BaseCount =", basearray->BaseCount);
	
	 da_INIT(uniqueAnchorTables, classCount, classCount);
	 
	 for (i = 0; i < basearray->BaseCount; i++)
		{
 		 BaseRecord *record = &basearray->BaseRecord[i];
		 DL(2, (OUTPUTBUFF, " --- BaseRecord [%d]\n", i));
		 for (j = 0; j < (IntX)classCount; j++)
			{
			IntX seenRecord = 0;
			IntX offset = record->BaseAnchor[j];
			void * fmt = record->_BaseAnchor[j];
		 	DL(2, (OUTPUTBUFF, " Class= %d Anchor Table offset= %04hx\n", j, (Card16)offset));

			 for (m = 0; m < uniqueAnchorTables.cnt; m++)
				{
				 AnchorRecord *tempRecord = da_INDEX(uniqueAnchorTables, m);
				if (tempRecord->BaseAnchor == offset)
					{
					seenRecord = 1;
					break;
					}
				}
			if (seenRecord == 0)
				{
				AnchorRecord *tempRecord = da_NEXT(uniqueAnchorTables);
				tempRecord->BaseAnchor = offset;
				tempRecord->_BaseAnchor = fmt;
				}

		    }
		}
	 DL(2, (OUTPUTBUFF, "\n"));
	for (m = 0; m < uniqueAnchorTables.cnt; m++)
		{
		AnchorRecord *tempRecord = da_INDEX(uniqueAnchorTables, m);
	    dumpAnchorRecord(tempRecord->BaseAnchor, tempRecord->_BaseAnchor, level);
	    }
	
	}


static void dumpMarkToBase(MarkBasePosFormat1 *fmt, IntX level, int isMarkToMark)
	{
	  char * opString;
	  
	  if (isMarkToMark)
		opString = "mark ";
	 else
		opString = "base ";
		
	  if (level == 8)
	  	 warning( SPOT_MSG_GPOSUFMTCTX, fmt->PosFormat);
	  else if (level == 6)
	  /* skip - this if for kerning only. */;
	  else if (level == 7)
	  	{
	  	ttoEnumRec MarkCovList;
	  	ttoEnumRec BaseCovList;
	  	Card32 nitems;
	  	Card32 mitems;
		IntX i;
		IntX j;
		MarkArray *markarray = &fmt->_MarkArray;
		BaseArray *basearray = &fmt->_BaseArray;
		
		ttoEnumerateCoverage(fmt->MarkCoverage, fmt->_MarkCoverage, &MarkCovList, &nitems);
		for (i = 0; i < (IntX)nitems; i++) 
			{
			char name[MAX_NAME_LEN];
			GlyphId glyphId = *da_INDEX(MarkCovList.glyphidlist, i);
			IntX offsetAnchor;
			void * anchorTable;
			MarkRecord *record = &markarray->MarkRecord[i];

			strcpy(name, getGlyphName(glyphId, 0));
			fprintf(OUTPUTBUFF,  "markClass %s ", name);

			offsetAnchor = record->MarkAnchor;
			if (offsetAnchor != 0)
				{
				anchorTable = record->_MarkAnchor;
				dumpAnchorRecord(offsetAnchor, anchorTable, level);
				}
			else
				fprintf(OUTPUTBUFF, "<anchor NULL>");
			fprintf(OUTPUTBUFF,  " ");
				
			fprintf(OUTPUTBUFF,  "@MARK_CLASS_%d;\n", record->Class);
			 }
		 


	  	ttoEnumerateCoverage(fmt->BaseCoverage, fmt->_BaseCoverage, &BaseCovList, &mitems);
	    for (i = 0; i < (IntX)mitems; i++) 
			{
			char name[MAX_NAME_LEN];
			GlyphId glyphId = *da_INDEX(BaseCovList.glyphidlist, i);
			
 		 	BaseRecord *record = &basearray->BaseRecord[i];

			strcpy(name, getGlyphName(glyphId, 0));

			fprintf(OUTPUTBUFF,  "pos %s%s ",  opString, name);
			
			 for (j = 0; j < fmt->ClassCount; j++)
				{
				IntX offsetAnchor;
				void * anchorTable;
				if (j != 0)
					fprintf(OUTPUTBUFF,  "      ");

				offsetAnchor = record->BaseAnchor[j];
				if (offsetAnchor != 0)
					{
					anchorTable = record->_BaseAnchor[j];
					dumpAnchorRecord(offsetAnchor, anchorTable, level);
					}
				else
					fprintf(OUTPUTBUFF, "<anchor NULL>");
				fprintf(OUTPUTBUFF,  " ");
					
				fprintf(OUTPUTBUFF,  "mark @MARK_CLASS_%d", j);
				if ((j+1) != fmt->ClassCount)
					fprintf(OUTPUTBUFF,  "\n");
				 }
				fprintf(OUTPUTBUFF,  ";\n");
		 
		    }

		   if (MarkCovList.glyphidlist.size > 0)
			   da_FREE(MarkCovList.glyphidlist);

		   if (BaseCovList.glyphidlist.size > 0)
			   da_FREE(BaseCovList.glyphidlist);

	    }
	   else
		{
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "MarkCoverage  =", fmt->MarkCoverage);
		  DLx(2, "BaseCoverage  =", fmt->BaseCoverage);
		  DLu(2, "ClassCount  =", fmt->ClassCount);
		  DLx(2, "MarkArray =", fmt->MarkArray);
		  DLx(2, "BaseArray =", fmt->BaseArray);
		  
		  ttoDumpCoverage(fmt->MarkCoverage, fmt->_MarkCoverage, level);
		  
		  ttoDumpCoverage(fmt->BaseCoverage, fmt->_BaseCoverage, level);
		  
		  DL(2, (OUTPUTBUFF, "\n"));
		  dumpMarkArray(fmt->MarkArray, &fmt->_MarkArray, level);

		  DL(2, (OUTPUTBUFF, "\n"));
		  dumpBaseArray(fmt->BaseArray, &fmt->_BaseArray, fmt->ClassCount, level);
		}
	}

static void dumpLigatureAttach(Card32 offset, LigatureAttach *ligatureAttach, Card32 classCount, IntX level)
	{
	 IntX i;
	 IntX j;
	 IntX m;
	 da_DCL(AnchorRecord, uniqueAnchorTables);
	 DL(2, (OUTPUTBUFF, "--- LigatureAttach (%04x)\n", offset));
	 DLu(2, "  ComponentCount =", ligatureAttach->ComponentCount);
	
	 da_INIT(uniqueAnchorTables, classCount, classCount);
	 
	 for (i = 0; i < ligatureAttach->ComponentCount; i++)
		{
 		 ComponentRecord *record = &ligatureAttach->ComponentRecord[i];
		 DL(2, (OUTPUTBUFF, " --- ComponentRecord [%d]\n", i));
		 for (j = 0; j < (IntX)classCount; j++)
			{
			IntX seenRecord = 0;
			IntX offset = record->LigatureAnchor[j];
			void * fmt;
			
		 	DL(2, (OUTPUTBUFF, " Class= %d Anchor Table offset= %04hx\n", j, (Card16)offset));
			if (offset == 0)
				{
		 		DL(2, (OUTPUTBUFF, " NULL offset for Anchor Table %d.\n", j));
				continue;
				}
				
			 fmt = record->_LigatureAnchor[j]; /* Note that record->_LigatureAnchor is NULL if offset == 0 */
			 
			 for (m = 0; m < uniqueAnchorTables.cnt; m++)
				{
				 AnchorRecord *tempRecord = da_INDEX(uniqueAnchorTables, m);
				if (tempRecord->BaseAnchor == offset)
					{
					seenRecord = 1;
					break;
					}
				}
			if (seenRecord == 0)
				{
				AnchorRecord *tempRecord = da_NEXT(uniqueAnchorTables);
				tempRecord->BaseAnchor = offset;
				tempRecord->_BaseAnchor = fmt;
				}

		    }
		}
	 DL(2, (OUTPUTBUFF, "\n"));
	for (m = 0; m < uniqueAnchorTables.cnt; m++)
		{
		AnchorRecord *tempRecord = da_INDEX(uniqueAnchorTables, m);
	    dumpAnchorRecord(tempRecord->BaseAnchor, tempRecord->_BaseAnchor, level);
	    }
	
	}



static void dumpLigatureArray(Card32 offset, LigatureArray *ligatureArray, Card32 classCount, IntX level)
	{
	 IntX i;
	 da_DCL(AnchorRecord, uniqueAnchorTables);
	 DL(2, (OUTPUTBUFF, "--- LigatureArray (%04x)\n", offset));
	 DLu(2, "LigatureCount =", ligatureArray->LigatureCount);
	
	 da_INIT(uniqueAnchorTables, classCount, classCount);
	 
	 for (i = 0; i <ligatureArray->LigatureCount; i++)
		{
			IntX offset = ligatureArray->LigatureAttach[i];
			void * fmt = &ligatureArray->_LigatureAttach[i];
		 	DL(2, (OUTPUTBUFF, " LigatureAttach index=%d\n", i));
			dumpLigatureAttach(offset, fmt, classCount, level);
		}
	}

	
static void dumpMarkToLigatureAttach(MarkToLigPosFormat1 *fmt, IntX level)
	{
	  if (level == 8)
	  	 warning( SPOT_MSG_GPOSUFMTCTX, fmt->PosFormat);
	  else if (level == 6)
	  /* skip - this if for kerning only. */;
	  else if (level == 7)
	  	{
	  	ttoEnumRec MarkCovList;
	  	ttoEnumRec LigatureCoverage;
	  	Card32 nitems;
	  	Card32 mitems;
		IntX i;
		IntX j;
		IntX c;
		MarkArray *markarray = &fmt->_MarkArray;
		LigatureArray* ligArray = &fmt->_LigatureArray;
	  	ttoEnumerateCoverage(fmt->MarkCoverage, fmt->_MarkCoverage, &MarkCovList, &nitems);
	    for (i = 0; i < (IntX)nitems; i++) 
			{
			char name[MAX_NAME_LEN];
			GlyphId glyphId = *da_INDEX(MarkCovList.glyphidlist, i);
			IntX offsetAnchor;
			void * anchorTable;
		 	MarkRecord *record = &markarray->MarkRecord[i];

			strcpy(name, getGlyphName(glyphId, 0));
			fprintf(OUTPUTBUFF,  "markClass %s ", name);

			offsetAnchor = record->MarkAnchor;
			if (offsetAnchor != 0)
				{
				anchorTable = record->_MarkAnchor;
				dumpAnchorRecord(offsetAnchor, anchorTable, level);
				}
			else
				fprintf(OUTPUTBUFF, "<anchor NULL>");
			fprintf(OUTPUTBUFF,  " ");
				
			fprintf(OUTPUTBUFF,  "@MARK_CLASS_%d;\n", record->Class);
			 }
		 


	  	ttoEnumerateCoverage(fmt->LigatureCoverage, fmt->_LigatureCoverage, &LigatureCoverage, &mitems);
	    for (i = 0; i < (IntX)mitems; i++) 
			{
			char name[MAX_NAME_LEN];
			GlyphId glyphId = *da_INDEX(LigatureCoverage.glyphidlist, i);
			
 		 	LigatureAttach *ligatureAttachRec = &ligArray->_LigatureAttach[i];

			strcpy(name, getGlyphName(glyphId, 0));
			
			fprintf(OUTPUTBUFF,  "pos ligature %s\n", name);



			 for (j = 0; j < ligatureAttachRec->ComponentCount; j++)
				{
				ComponentRecord *componentRec = &ligatureAttachRec->ComponentRecord[j];
				IntX offsetAnchor;
				void * anchorTable;
				if (j != 0)
					fprintf(OUTPUTBUFF,  "      ligComponent\n");

				for (c = 0; c < fmt->ClassCount; c++) 
					{
					offsetAnchor = componentRec->LigatureAnchor[c];
					fprintf(OUTPUTBUFF,  "      ");
					if (offsetAnchor != 0)
						{
						anchorTable = componentRec->_LigatureAnchor[c];
						dumpAnchorRecord(offsetAnchor, anchorTable, level);
						}
					else
						fprintf(OUTPUTBUFF, "<anchor NULL>");
						
					fprintf(OUTPUTBUFF,  " ");
					fprintf(OUTPUTBUFF,  "mark @MARK_CLASS_%d", c);
					if (( (j+1) != ligatureAttachRec->ComponentCount) || ((c+1) != fmt->ClassCount ) )
						fprintf(OUTPUTBUFF,  "\n");
						
					 }
				}
				fprintf(OUTPUTBUFF,  ";\n");

					
					
		 
			}

	   if (MarkCovList.glyphidlist.size > 0)
		   da_FREE(MarkCovList.glyphidlist);

	   if (LigatureCoverage.glyphidlist.size > 0)
		   da_FREE(LigatureCoverage.glyphidlist);

	    }
	   else
		{
		  DLu(2, "PosFormat =", fmt->PosFormat);
		  DLx(2, "MarkCoverage  =", fmt->MarkCoverage);
		  DLx(2, "LigatureCoverage  =", fmt->LigatureCoverage);
		  DLu(2, "ClassCount  =", fmt->ClassCount);
		  DLx(2, "MarkArray =", fmt->MarkArray);
		  DLx(2, "LigatureArray =", fmt->LigatureArray);
		  
		  ttoDumpCoverage(fmt->MarkCoverage, fmt->_MarkCoverage, level);
		  
		  ttoDumpCoverage(fmt->LigatureCoverage, fmt->_LigatureCoverage, level);
		  
		  DL(2, (OUTPUTBUFF, "\n"));
		  dumpMarkArray(fmt->MarkArray, &fmt->_MarkArray, level);

		  DL(2, (OUTPUTBUFF, "\n"));
		  dumpLigatureArray(fmt->LigatureArray, &fmt->_LigatureArray, fmt->ClassCount, level);
		}
	}


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


static void dumpPosRuleSet(PosRuleSet *posruleset, IntX level)
	{
	IntX i,j;
	
	if (level == 5)
		{
		for (i = 0; i < posruleset->PosRuleCount; i++) 
			{
			PosRule *posrule = &posruleset->_PosRule[i];
			for (j = 0; j < posrule->PosCount; j++) 
			  {
				Card16 lis = posrule->PosLookupRecord[j].LookupListIndex;	
				DL5((OUTPUTBUFF, " %hu",lis));
			  }
			}
		}
	else
		{

		DLu(2, "PosRuleCount=", posruleset->PosRuleCount);
		DL(2, (OUTPUTBUFF, "--- PosRule[index]=offset\n"));	
		for (i = 0; i < posruleset->PosRuleCount; i++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, posruleset->PosRule[i]));
			}	
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < posruleset->PosRuleCount; i++) 
			{
			PosRule *posrule = &posruleset->_PosRule[i];
			DL(2, (OUTPUTBUFF, "--- PosRule (%04hx)\n", posruleset->PosRule[i]));
			DLu(2, "GlyphCount=", posrule->GlyphCount);
			DL(2, (OUTPUTBUFF, "--- Input[index]=glyphId\n"));
			for (j = 1; j < posrule->GlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, posrule->Input[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, posrule->Input[j], getGlyphName(posrule->Input[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));
			DLu(2, "PosCount=", posrule->PosCount);
			DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
			for (j = 0; j < posrule->PosCount; j++) 
			  {
				Card16 seq = posrule->PosLookupRecord[j].SequenceIndex;
				Card16 lis = posrule->PosLookupRecord[j].LookupListIndex;	
				DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
			  }
			DL(2, (OUTPUTBUFF, "\n"));
			}
		} /* end if level not 5 */
	}
	
typedef struct {
	GlyphId *gid;
	IntX numGids;
	IntX startPos;
	} GlyphRec;

static int GPOSEval(IntX GPOSLookupListIndex, 
			  GlyphId gid1,  Card16* vfmt1, ValueRecord *vr1, GlyphId gid2, 
			  Card16* vfmt2, ValueRecord *vr2)
{
  Lookup *lookup;
  int i;
	*vfmt1 = 0;
	*vfmt2 = 0; /* caller can check this to see if this value record was set. */
	
  lookup = &GPOS._LookupList._Lookup[GPOSLookupListIndex];

	for (i=0; i < lookup->SubTableCount; i++)
		{
		if (searchSubtable(lookup->LookupType, (void *)(lookup->_SubTable[i]), gid1, gid2, vfmt1, vr1, vfmt2, vr2))
			return 1;
		}
	return 0;
}

static void dumpContext1(ContextPosFormat1 *fmt, IntX level)
	{
	IntX i;
	/*if (level ==7)
	{
		ttoEnumRec CovList;
	  	Card32 nitems;
	  	ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		for (i = 0; i < nitems; i++) 
		{
		  	char name[MAX_NAME_LEN];
		  	PosRuleSet *posruleset = &fmt->_PosRuleSet[i];
		  	GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);

			strcpy(name, getGlyphName(glyphId, 0));
			
		  	for (j = 0; j < posruleset->PosRuleCount; j++) 
			{
				PosRule *posrule = &posruleset->_PosRule[j];
				GlyphId *sequence;
				GlyphRec *dest;
				IntX offset=0;
				IntX size=posrule->GlyphCount;
				
				sequence = memNew(posrule->GlyphCount*sizeof(GlyphId));
				dest = memNew(posrule->GlyphCount*sizeof(GlyphRec));
				
				dest[0].gid= memNew(sizeof(GlyphId));
				sequence[0]=*(dest[0].gid)=glyphId;
				dest[0].startPos=0;
				dest[0].numGids=1;
				for (k = 0; k < posrule->GlyphCount-1; k++) 
				  {
				  	fprintf(OUTPUTBUFF, " %s", getGlyphName(posrule->Input[k], 0));
				  	dest[k+1].gid = memNew(sizeof(GlyphId));
				  	sequence[k+1]=*(dest[k+1].gid)=posrule->Input[k];
				  	dest[k+1].startPos=k+1;
				  	dest[k+1].numGids=1;
				  }
				for (k = 0; k < posrule->PosCount; k++) 
				  {
					IntX outputglyphs;
					Card16 seq = posrule->PosLookupRecord[k].SequenceIndex;
					Card16 lis = posrule->PosLookupRecord[k].LookupListIndex;	
					if (!GPOSEvaliP(lis, seq, &size, dest))
					{
						fprintf(OUTPUTBUFF, "\nError: Could not evaluate GPOS lookup.\n");
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
				for(; l<posrule->GlyphCount	; l++)
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
	}
*/
	if ((level == 8) || (level == 7) || (level == 6))
	 	 featureWarning(level, SPOT_MSG_GPOSUFMTCTX,  fmt->PosFormat);
	else
	  {
		DLu(2, "PosFormat    =", fmt->PosFormat);
		DLx(2, "Coverage       =", fmt->Coverage);
		DLu(2, "PosRuleSetCount=", fmt->PosRuleSetCount);
		
		DL(2, (OUTPUTBUFF, "--- PosRuleSet[index]=offset\n"));
		for (i = 0; i < fmt->PosRuleSetCount; i++) 
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->PosRuleSet[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->PosRuleSetCount; i++) 
		  {
			if (fmt->PosRuleSet != 0)
			  {
				DL(2, (OUTPUTBUFF, "--- PosRuleSet (%04hx)\n", fmt->PosRuleSet[i]));
				dumpPosRuleSet(&fmt->_PosRuleSet[i], level);
			  }
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
#if 0
	else
	  {
		ttoEnumRec CovList;
		Card32 nitems;
		IntX i;
		ttoEnumerateCoverage(fmt->Coverage, fmt->_Coverage, &CovList, &nitems);
		
		for (i = 0; i < nitems; i++)
		  {
			char name1[MAX_NAME_LEN];
			IntX srscnt;
			IntX origShift, lsb, rsb, width1, tsb, bsb, vwidth, yorig;
			GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
			
			strcpy(name1, getGlyphName(glyphId, 1));
			getMetrics(glyphId, &origShift, &lsb, &rsb, &width1, &tsb, &bsb, &vwidth, &yorig);
			for (srscnt = 0; srscnt < fmt->PosRuleSetCount; srscnt++)
			  {
				if (fmt->PosRuleSet != 0)
				  {
#if __CENTERLINE__
					centerline_stop();
#endif
					IntX subr;
					PosRuleSet *posruleset = &(fmt->_PosRuleSet[srscnt]);
					/* ??? To complete */
				  }
			  }
		  }
		if (CovList.glyphidlist.size > 0)
		  da_FREE(CovList.glyphidlist);
	  }
#endif
	}

static void dumpPosClassSet(PosClassSet *PosClassset, IntX level, void *feattag)
	{
	IntX i, j;

	DLu(2, "PosClassRuleCnt=", PosClassset->PosClassRuleCnt);

	DL(2, (OUTPUTBUFF, "--- PosClassRule[index]=offset\n"));	
	for (i = 0; i < PosClassset->PosClassRuleCnt; i++) 
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, PosClassset->PosClassRule[i]));
		}	
	DL(2, (OUTPUTBUFF, "\n"));
	for (i = 0; i < PosClassset->PosClassRuleCnt; i++) 
		{
		PosClassRule *PosClassrule = &PosClassset->_PosClassRule[i];
		DL(2, (OUTPUTBUFF, "--- PosClassRule (%04hx)\n", PosClassset->PosClassRule[i]));
		DLu(2, "GlyphCount=", PosClassrule->GlyphCount);
		DL(2, (OUTPUTBUFF, "--- Input[index]=classId\n"));
		for (j = 1; j < PosClassrule->GlyphCount; j++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, PosClassrule->Class[j]));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		DLu(2, "PosCount=", PosClassrule->PosCount);
		DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(GlyphSequenceIndex,LookupListIndex)\n"));
		for (j = 0; j < PosClassrule->PosCount; j++) 
			{
			Card16 seq = PosClassrule->PosLookupRecord[j].SequenceIndex;
			Card16 lis = PosClassrule->PosLookupRecord[j].LookupListIndex;
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
			}
		DL(2, (OUTPUTBUFF, "\n"));

		}
	}

static void showContext2(ContextPosFormat2 *fmt, IntX level, void *feattag)
	{
		ttoEnumRec *classList;
		Card32 cnt, scscnt, psIndex;
		IntX i, forProofing;
		char proofString[kProofBufferLen];
		classList = memNew(sizeof(ttoEnumRec) * (fmt->PosClassSetCnt));
		ttoEnumerateClass(fmt->ClassDef, fmt->_ClassDef, 
						  fmt->PosClassSetCnt,
						  classList, &cnt);
		
		forProofing = 0;
		if (level == 8)
			{
			forProofing = 1;
		 	proofThinspace(proofctx, 1);

			}
			
		for (scscnt = 0; scscnt < fmt->PosClassSetCnt; scscnt++)
		  {
			if (fmt->PosClassSet[scscnt] != 0) 
			  {
				PosClassSet *PosClassset = &fmt->_PosClassSet[scscnt];
				IntX scrcnt;
				
				for (scrcnt = 0; scrcnt < PosClassset->PosClassRuleCnt; scrcnt++)
				  {
					PosClassRule *PosClassrule = &PosClassset->_PosClassRule[scrcnt];
					Card16 seqindex, lookupindex;
					IntX classi, classId, c;
					ttoEnumRec *CR;

					if (PosClassrule->GlyphCount > fmt->PosClassSetCnt)
					 {
					  if (level == 7)
						  fprintf(OUTPUTBUFF,"\n# ");
					  warning(SPOT_MSG_GPOSCTXDEF, PosClassrule->GlyphCount, fmt->PosClassSetCnt);
					 }

					seqindex = PosClassrule->PosLookupRecord[0].SequenceIndex;
					lookupindex = PosClassrule->PosLookupRecord[0].LookupListIndex;

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
					
					for (classi=1; classi < PosClassrule->GlyphCount; classi++)
						{
						classId = PosClassrule->Class[classi];
						CR = &(classList[classId]);
						psIndex = strlen(proofString);
						sprintf(&proofString[psIndex],"[");
						for (c = 0; c < CR->glyphidlist.cnt; c++)
						  {
							char name2[MAX_NAME_LEN];
							GlyphId glyphId2 = *da_INDEX(CR->glyphidlist, c);
							strcpy(name2, getGlyphName(glyphId2, forProofing));
                              if ((psIndex + strlen(name2) + 10) >= kProofBufferLen)
                              {
                                  if (level == 7)
                                      fprintf(OUTPUTBUFF, "\n    %s", proofString);
                                  else if (level == 8)
                                      dumpMessage(proofString, ((Card32 *)feattag)[0]);
                                  proofString[0] = 0;
                                  psIndex = 0;
                              }
							psIndex = strlen(proofString);
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
					
					ttoDumpLookupListItem(GPOS.LookupList, &GPOS._LookupList, lookupindex, level, dumpSubtable, feattag);
					
					contextPrefix[0] = 0;
					
					if (level == 7)
						fprintf(OUTPUTBUFF, "} context;\n\n");
					
				  }
			  }
		  }

		for (i = 0; i < fmt->PosClassSetCnt; i++)
		  {
			if (classList[i].glyphidlist.size > 0)
			  da_FREE(classList[i].glyphidlist);
		  }
		memFree(classList);  	
	}
	

#if 0
static void proofContext2(ContextPosFormat2 *fmt)
	{
 		/* ??? Half-hearted attempt */
		ttoEnumRec CovList;
		Card32 nitems;
		ttoEnumRec *classList;
		Card32 cnt;
		IntX i;
		IntX isVert = proofIsVerticalMode();
		IntX origShift, lsb, rsb, width1, width2, tsb, bsb, vwidth, yorig1, yorig2, yorig3, W1, W2;
		Int16 metrics[4];
		
		classList = memNew(sizeof(ttoEnumRec) * (fmt->PosClassSetCnt));
		ttoEnumerateClass(fmt->ClassDef, fmt->_ClassDef, 
						  fmt->PosClassSetCnt,
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

			for (scscnt = 0; scscnt < fmt->PosClassSetCnt; scscnt++)
			  {
				if (fmt->PosClassSet[scscnt] != 0) 
				  {
					PosClassSet *PosClassset = &fmt->_PosClassSet[scscnt];
					IntX scrcnt;
					for (scrcnt = 0; scrcnt < PosClassset->PosClassRuleCnt; scrcnt++)
					  {
						PosClassRule *PosClassrule = &PosClassset->_PosClassRule[scrcnt];
						Card16 seqindex, lookupindex;
						IntX classId, c;
						ttoEnumRec *CR;

						if (PosClassrule->PosCount < 1) 
						  continue;
#if __CENTERLINE__
					centerline_stop();
#endif
/* ??? to complete */
						if (PosClassrule->GlyphCount > fmt->PosClassSetCnt)
							warning(SPOT_MSG_GPOSCTXDEF, PosClassrule->GlyphCount, fmt->PosClassSetCnt);
						if (PosClassrule->PosCount > 1) 
						  warning(SPOT_MSG_GPOSCTXCNT);

						seqindex = PosClassrule->PosLookupRecord[0].SequenceIndex;
						lookupindex = PosClassrule->PosLookupRecord[0].LookupListIndex;
						if (seqindex > 1)
						  {
							warning(SPOT_MSG_GPOSCTXNYI); /* ??? */
							continue;
						  }
						classId = PosClassrule->Class[seqindex];
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
							/*GPOSEval(lookupindex, inputcount, inputgid, metrics);*/
							if ((metrics[0] == 0) 
								&& (metrics[1] == 0)
								&& (metrics[2] == 0)
								&& (metrics[3] == 0))
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
		for (i = 0; i < fmt->PosClassSetCnt; i++)
		  {
			if (classList[i].glyphidlist.size > 0)
			  da_FREE(classList[i].glyphidlist);
		  }
		memFree(classList);  	
	}
#endif
	
static void dumpContext2(ContextPosFormat2 *fmt, IntX level, void *feattag)
	{
	IntX i;

	if ((level == 8) || (level == 7) || (level == 6))
		showContext2(fmt, level, feattag);
	else
	  {
		DLu(2, "PosFormat   =", fmt->PosFormat);
		DLx(2, "Coverage      =", fmt->Coverage);
		DLx(2, "ClassDef      =", fmt->ClassDef);
		DLu(2, "PosClassSetCnt=", fmt->PosClassSetCnt);
		
		DL(2, (OUTPUTBUFF, "--- PosClassSet[index]=offset\n"));
		for (i = 0; i < fmt->PosClassSetCnt; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->PosClassSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->PosClassSetCnt; i++) 
		  {
			if (fmt->PosClassSet[i] != 0) 
			  {
				DL(2, (OUTPUTBUFF, "--- PosClassSet (%04hx)\n", fmt->PosClassSet[i]));
				dumpPosClassSet(&fmt->_PosClassSet[i], level, feattag);
			  }
		  }

		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
		ttoDumpClass(fmt->ClassDef, fmt->_ClassDef, level);
	  }
	}

static void dumpContext3(ContextPosFormat3 *fmt, IntX level)
	{
	IntX i;

	if ((level == 8) || (level == 7) || (level == 6))
	 	 featureWarning(level, SPOT_MSG_GPOSUFMTCTX,  fmt->PosFormat);
	else
	  {
		DLu(2, "PosFormat   =", fmt->PosFormat);
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
		DLu(2, "PosCount =", fmt->PosCount);
		DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
		for (i = 0; i < fmt->PosCount; i++) 
			{
			Card16 seq = fmt->PosLookupRecord[i].SequenceIndex;
			Card16 lis = fmt->PosLookupRecord[i].LookupListIndex;	
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", i, seq, lis));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}
	
static void dumpPosContext(void *fmt, IntX level, void *feattag)
	{
	  DL(2, (OUTPUTBUFF, "--- ContextPos\n"));
	 GPOSContextRecursionCnt++;  

	 if (GPOSContextRecursionCnt > 1)
		{
		featureWarning(level,  SPOT_MSG_CNTX_RECURSION, ((ContextPosFormat1 *)fmt)->PosFormat);
		return;
		}

	  switch (((ContextPosFormat1 *)fmt)->PosFormat)
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
	 GPOSContextRecursionCnt--;  
	  
	}

static IntX ContextPosFlavor(void *fmt)
	{
	  return (((ContextPosFormat1 *)fmt)->PosFormat);
	}



static void dumpChainPosRuleSet(ChainPosRuleSet *posruleset, IntX level)
	{
	IntX i,j;
	if (level == 5)
		{
		for (i = 0; i < posruleset->ChainPosRuleCount; i++) 

			{
			ChainPosRule *posrule = &posruleset->_ChainPosRule[i];
			for (j = 0; j < posrule->PosCount; j++) 
			  {
				Card16 lis = posrule->PosLookupRecord[j].LookupListIndex;
				if (seenChainLookup[lis].seen)
					{
					if (seenChainLookup[lis].cur_parent == GPOSLookupIndex)
						continue;
					seenChainLookup[lis].cur_parent = GPOSLookupIndex;
					if (seenChainLookup[lis].parent != GPOSLookupIndex)
						DL5((OUTPUTBUFF, " %hu*",lis));
					}
				else
					{
					seenChainLookup[lis].parent = GPOSLookupIndex;
					seenChainLookup[lis].cur_parent = GPOSLookupIndex;
					seenChainLookup[lis].seen =1;
					DL5((OUTPUTBUFF, " %hu",lis));
					}
			  }
			}
		}
	else
		{

		DLu(2, "ChainPosRuleCount=", posruleset->ChainPosRuleCount);
		DL(2, (OUTPUTBUFF, "--- ChainPosRule[index]=offset\n"));	
		for (i = 0; i < posruleset->ChainPosRuleCount; i++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, posruleset->ChainPosRule[i]));
			}	
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < posruleset->ChainPosRuleCount; i++) 
			{
			ChainPosRule *posrule = &posruleset->_ChainPosRule[i];
			DL(2, (OUTPUTBUFF, "--- ChainPosRule (%04hx)\n", posruleset->ChainPosRule[i]));

			DLu(2, "BacktrackGlyphCount=", posrule->BacktrackGlyphCount);
			for (j = 0; j < posrule->BacktrackGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, posrule->Backtrack[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, posrule->Backtrack[j], getGlyphName(posrule->Backtrack[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "InputGlyphCount=", posrule->InputGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Input[index]=glyphId\n"));
			for (j = 1; j < posrule->InputGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, posrule->Input[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, posrule->Input[j], getGlyphName(posrule->Input[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "LookaheadGlyphCount=", posrule->LookaheadGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Lookahead[index]=glyphId\n"));
			for (j = 0; j < posrule->LookaheadGlyphCount; j++) 
				{
				  if (level < 4)
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, posrule->Lookahead[j]));
					}
				  else
					{
					  DL(2, (OUTPUTBUFF, "[%d]=%hu (%s) ", j, posrule->Lookahead[j], getGlyphName(posrule->Lookahead[j], 0)));
					}
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "PosCount=", posrule->PosCount);
			DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
			for (j = 0; j < posrule->PosCount; j++) 
			  {
				Card16 seq = posrule->PosLookupRecord[j].SequenceIndex;
				Card16 lis = posrule->PosLookupRecord[j].LookupListIndex;	
				DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
			  }
			DL(2, (OUTPUTBUFF, "\n"));
			}
		}
	}

static void listPosLookups1(ChainContextPosFormat1 *fmt, IntX level)
	{
	IntX i;
		for (i = 0; i < fmt->ChainPosRuleSetCount; i++) 
		  {
			if (fmt->ChainPosRuleSet != 0)
			  {
				dumpChainPosRuleSet(&fmt->_ChainPosRuleSet[i], level);
			  }
		  }

	}

static void dumpChainContext1(ChainContextPosFormat1 *fmt, IntX level)
	{
	IntX i;
	if ((level == 8) || (level == 7) || (level == 6))
	 	 featureWarning(level, SPOT_MSG_GPOSUFMTCCTX,  fmt->PosFormat);
	else if (level == 5)
		listPosLookups1(fmt, level);
	  else
	  {
		DLu(2, "PosFormat    =", fmt->PosFormat);
		DLx(2, "Coverage       =", fmt->Coverage);
		DLu(2, "ChainPosRuleSetCount=", fmt->ChainPosRuleSetCount);
		
		DL(2, (OUTPUTBUFF, "--- ChainPosRuleSet[index]=offset\n"));
		for (i = 0; i < fmt->ChainPosRuleSetCount; i++) 
		  {
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->ChainPosRuleSet[i]));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->ChainPosRuleSetCount; i++) 
		  {
			if (fmt->ChainPosRuleSet != 0)
			  {
				DL(2, (OUTPUTBUFF, "--- ChainPosRuleSet (%04hx)\n", fmt->ChainPosRuleSet[i]));
				dumpChainPosRuleSet(&fmt->_ChainPosRuleSet[i], level);
			  }
		  }
		ttoDumpCoverage(fmt->Coverage, fmt->_Coverage, level);
	  }
  }



static void dumpChainPosClassSet(ChainPosClassSet *PosClassset, IntX level, void* feattag)
	{
	IntX i, j;
	if (level == 5)
		{
		for (i = 0; i < PosClassset->ChainPosClassRuleCnt; i++) 
			{
			IntX j;
			ChainPosClassRule *PosClassrule = &PosClassset->_ChainPosClassRule[i];
			for (j = 0; j < PosClassrule->PosCount; j++) 
				{
				Card16 lis = PosClassrule->PosLookupRecord[j].LookupListIndex;
				if (seenChainLookup[lis].seen)
					{
					if (seenChainLookup[lis].cur_parent == GPOSLookupIndex)
						continue;
					seenChainLookup[lis].cur_parent = GPOSLookupIndex;
					if (seenChainLookup[lis].parent != GPOSLookupIndex)
						DL5((OUTPUTBUFF, " %hu*",lis));
					}
				else
					{
					seenChainLookup[lis].parent = GPOSLookupIndex;
					seenChainLookup[lis].cur_parent = GPOSLookupIndex;
					seenChainLookup[lis].seen =1;
					DL5((OUTPUTBUFF, " %hu",lis));
					}
				}
			}
		}
	else
		{
		DLu(2, "ChainPosClassRuleCnt=", PosClassset->ChainPosClassRuleCnt);

		DL(2, (OUTPUTBUFF, "--- ChainPosClassRule[index]=offset\n"));	
		for (i = 0; i < PosClassset->ChainPosClassRuleCnt; i++) 
			{
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, PosClassset->ChainPosClassRule[i]));
			}	
		DL(2, (OUTPUTBUFF, "\n"));
		for (i = 0; i < PosClassset->ChainPosClassRuleCnt; i++) 
			{
			ChainPosClassRule *PosClassrule = &PosClassset->_ChainPosClassRule[i];
			DL(2, (OUTPUTBUFF, "--- ChainPosClassRule (%04hx)\n", PosClassset->ChainPosClassRule[i]));

			DLu(2, "BacktrackGlyphCount=", PosClassrule->BacktrackGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Backtrack[index]=classId\n"));
			for (j = 0; j < PosClassrule->BacktrackGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, PosClassrule->Backtrack[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "InputGlyphCount=", PosClassrule->InputGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Input[index]=classId\n"));
			for (j = 1; j < PosClassrule->InputGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, PosClassrule->Input[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "LookaheadGlyphCount=", PosClassrule->LookaheadGlyphCount);
			DL(2, (OUTPUTBUFF, "--- Lookahead[index]=classId\n"));
			for (j = 0; j < PosClassrule->LookaheadGlyphCount; j++) 
				{
				DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, PosClassrule->Lookahead[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			DLu(2, "PosCount=", PosClassrule->PosCount);
			DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(GlyphSequenceIndex,LookupListIndex)\n"));
			for (j = 0; j < PosClassrule->PosCount; j++) 
				{
				Card16 seq = PosClassrule->PosLookupRecord[j].SequenceIndex;
				Card16 lis = PosClassrule->PosLookupRecord[j].LookupListIndex;
				DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", j, seq, lis));
				}
			DL(2, (OUTPUTBUFF, "\n"));

			}
		} /* end if not level 5 */
	}

static void listPosLookups2(ChainContextPosFormat2 *fmt, IntX level, void* feattag)
	{
	IntX i;
		for (i = 0; i < fmt->ChainPosClassSetCnt; i++) 
		  {
			if (fmt->ChainPosClassSet[i] != 0) 
			  {
				dumpChainPosClassSet(&fmt->_ChainPosClassSet[i], level, feattag);
			  }
		  }
	}
	
static void dumpChainContext2(ChainContextPosFormat2 *fmt, IntX level, void* feattag)
	{
	IntX i;

	if ((level == 8) || (level == 7) || (level == 6))
	 	 featureWarning(level, SPOT_MSG_GPOSUFMTCCTX,  fmt->PosFormat);
	else if (level == 5)
		listPosLookups2(fmt, level, feattag);
	  else
	  {
		DLu(2, "PosFormat   =", fmt->PosFormat);
		DLx(2, "Coverage      =", fmt->Coverage);
		DLx(2, "BackTrackClassDef      =", fmt->BackTrackClassDef);
		DLx(2, "InputClassDef      =", fmt->InputClassDef);
		DLx(2, "LookAheadClassDef      =", fmt->LookAheadClassDef);
		DLu(2, "ChainPosClassSetCnt=", fmt->ChainPosClassSetCnt);
		
		DL(2, (OUTPUTBUFF, "--- ChainPosClassSet[index]=offset\n"));
		for (i = 0; i < fmt->ChainPosClassSetCnt; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->ChainPosClassSet[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < fmt->ChainPosClassSetCnt; i++) 
		  {
			if (fmt->ChainPosClassSet[i] != 0) 
			  {
				DL(2, (OUTPUTBUFF, "--- ChainPosClassSet (%04hx)\n", fmt->ChainPosClassSet[i]));
				dumpChainPosClassSet(&fmt->_ChainPosClassSet[i], level, feattag);
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


static void decompileChainContext3(ChainContextPosFormat3 *fmt, int level)
	{
		IntX i, isException = 0;
		ValueRecord vr1, vr2;
		Card16 vfmt1, vfmt2;
		da_DCL(ttoEnumRec, InputGlyphCovListArray);
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);

		IntX doLookupRef = 0;
		PosLookupRecord *plr;
		Lookup *lookup;
		
		isException = (fmt->PosCount == 0);

		da_INIT(InputGlyphCovListArray, fmt->InputGlyphCount, 1);
		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  fprintf(OUTPUTBUFF, " ignore pos");
		else
			fprintf(OUTPUTBUFF, " pos");
		
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j = fmt->BacktrackGlyphCount - (i+1); /* backtrack enumerates in reverse order; entry 0 is the last glyph in the sequence. */
			GlyphId gId;
			ttoEnumRec *ter = da_NEXT(BacktrackGlyphCovListArray);
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

		vfmt1 = 0;
		vfmt2 = 0;
				
			for (i = 0; i < fmt->InputGlyphCount; i++)
			  {
				/* I need to get the current glyph and the next, in order to match
				any possible pair pos records in subsidiary lookups */
				Card32 nitems, nitems2;
				IntX j;
				GlyphId gid;
				ttoEnumRec *ter, *ter2;
				if (i == 0)
					{
					ter = da_NEXT(InputGlyphCovListArray);
					ttoEnumerateCoverage(fmt->Input[i], fmt->_Input[i], ter, &nitems);
					}
				else
					{
					ter = da_INDEX(InputGlyphCovListArray, i);
					nitems = ter->glyphidlist.cnt;
					}
				
				if (i+1 < fmt->InputGlyphCount)
					{
					ter2 = da_NEXT(InputGlyphCovListArray);
					ttoEnumerateCoverage(fmt->Input[i+1], fmt->_Input[i+1], ter2, &nitems2);
					}
				else
					ter2 = NULL;
				
				if (nitems > 1)
				  fprintf(OUTPUTBUFF, " [");
				for (j = 0; j < (IntX)nitems; j++)
				  {
					gid = *da_INDEX(ter->glyphidlist, j);
					fprintf(OUTPUTBUFF, " %s", getGlyphName(gid, 0));
				  }
				if (nitems > 1)
				  fprintf(OUTPUTBUFF, "]");
				fprintf(OUTPUTBUFF, "'");
				j = 0;
				
				if (vfmt2 != 0)
					{
					printValueRecord(ValueXPlacement|ValueYPlacement|ValueXAdvance|ValueYAdvance, &vr2, level);
					}
				vfmt1 = 0;
				vfmt2 = 0;
				
				while (j < fmt->PosCount)
					{
					Card16 seqindex, lookupindex;
					Card16 gid1, gid2;
					gid1 = 0;
					gid2 = 0;
					gid1 = *da_INDEX(ter->glyphidlist, 0);
					if (ter2 != NULL)
						gid2 = *da_INDEX(ter2->glyphidlist, 0);
					plr =  &fmt->PosLookupRecord[j];
					seqindex = plr->SequenceIndex;
					lookupindex = plr->LookupListIndex;
					lookup = &GPOS._LookupList._Lookup[lookupindex];

					if (seqindex == i)
						{
						if (lookup->LookupType >= CursiveAttachType)
							{
							fprintf(OUTPUTBUFF, " lookup lkp_%d ", lookupindex);
							addToReferencedList(lookupindex);
							doLookupRef = 1;
							}
						else if ((lookup->LookupType == SingleAdjustType)|| (lookup->LookupType  == PairAdjustType))
							{
							if (GPOSEval(lookupindex, gid1, &vfmt1,  &vr1, gid2, &vfmt2, &vr2))
								{
								if (vfmt1 != 0)
									{
									printValueRecord(ValueXPlacement|ValueYPlacement|ValueXAdvance|ValueYAdvance, &vr1, level);
									}
								}
							}
						break;
						}
					j++;
					}
			  }
			  
		
		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems;
			IntX j;
			GlyphId gId;
			ttoEnumRec *ter = da_NEXT(LookaheadGlyphCovListArray);
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


		fprintf(OUTPUTBUFF, ";\n");
				
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			da_FREE(InputGlyphCovListArray.array[i].glyphidlist);
		  }
		if (InputGlyphCovListArray.cnt > 0)
		  da_FREE(InputGlyphCovListArray);

		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			da_FREE(BacktrackGlyphCovListArray.array[i].glyphidlist);
		  }
		if (BacktrackGlyphCovListArray.cnt > 0)
		  da_FREE(BacktrackGlyphCovListArray);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			da_FREE(LookaheadGlyphCovListArray.array[i].glyphidlist);
		  }
		if (LookaheadGlyphCovListArray.cnt > 0)
		  da_FREE(LookaheadGlyphCovListArray);	
	}

static void  proofPosChainContext3(ChainContextPosFormat3 *fmt, int level, void* feattag)
	{
		IntX i, isException = 0;	
		IntX isVert = proofIsVerticalMode();
		IntX origShift1, lsb1, rsb1, width1, tsb1, bsb1, vwidth1, yorig1, W1;
		IntX origShift2, lsb2, rsb2, width2, tsb2, bsb2, vwidth2, yorig2;
		ValueRecord vr1, vr2;
		Card16 vfmt1, vfmt2;

		da_DCL(ttoEnumRec, InputGlyphCovListArray);
		da_DCL(ttoEnumRec, BacktrackGlyphCovListArray);
		da_DCL(ttoEnumRec, LookaheadGlyphCovListArray);
		
		isException = (fmt->PosCount == 0);

		da_INIT(InputGlyphCovListArray, fmt->InputGlyphCount, 1);
		da_INIT(BacktrackGlyphCovListArray, fmt->BacktrackGlyphCount, 1);
		da_INIT(LookaheadGlyphCovListArray, fmt->LookaheadGlyphCount, 1);

		if (isException)
		  proofSymbol(proofctx, PROOF_NOTELEM);
		/* first we write the the entire context line, showing all members of all classes */
		for (i = 0; i < fmt->BacktrackGlyphCount; i++)
		  {
			Card32 nitems1;
			IntX j = fmt->BacktrackGlyphCount - (i+1);
			GlyphId gId1;
			char name1[MAX_NAME_LEN];
			ttoEnumRec *ter1 = da_NEXT(BacktrackGlyphCovListArray);
			ttoEnumerateCoverage(fmt->Backtrack[j], fmt->_Backtrack[j], ter1, &nitems1);
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems1; j++)
			  {
				gId1 = *da_INDEX(ter1->glyphidlist, j);
				strcpy(name1, getGlyphName(gId1, 1));
				getMetrics(gId1, &origShift1, &lsb1, &rsb1, &width1, &tsb1, &bsb1, &vwidth1, &yorig1);
				if (isVert)
				  {
					if (vwidth1 != 0) W1 = vwidth1;
					else  W1 = width1;
				  }
				else
				  W1 = width1;

				proofDrawGlyph(proofctx, 
							   gId1, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name1, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								: 
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),  
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
				if (j < (IntX)(nitems1 - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);

			if (i < (fmt->BacktrackGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if ((!isException) && (fmt->BacktrackGlyphCount > 0) && (fmt->InputGlyphCount > 0))
		  proofSymbol(proofctx, PROOF_PLUS);

		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			Card32 nitems1;
			IntX j;
			GlyphId gId1;
			char name1[MAX_NAME_LEN];
			ttoEnumRec *ter1 = da_NEXT(InputGlyphCovListArray);
			ttoEnumerateCoverage(fmt->Input[i], fmt->_Input[i], ter1, &nitems1);
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems1; j++)
			  {
				gId1 = *da_INDEX(ter1->glyphidlist, j);
				strcpy(name1, getGlyphName(gId1, 1));
				getMetrics(gId1, &origShift1, &lsb1, &rsb1, &width1, &tsb1, &bsb1, &vwidth1, &yorig1);
				if (isVert)
				  {
					if (vwidth1 != 0) W1 = vwidth1;
					else  W1 = width1;
				  }
				else
				  W1 = width1;
				proofDrawGlyph(proofctx, 
							   gId1, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name1, 
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
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
				if (j < (IntX)(nitems1 - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->InputGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if ((!isException) && (fmt->InputGlyphCount > 0) && (fmt->LookaheadGlyphCount > 0))
		  proofSymbol(proofctx, PROOF_PLUS);

		for (i = 0; i < fmt->LookaheadGlyphCount; i++)
		  {
			Card32 nitems1;
			IntX j;
			GlyphId gId1;
			char name1[MAX_NAME_LEN];
			ttoEnumRec *ter1 = da_NEXT(LookaheadGlyphCovListArray);
			ttoEnumerateCoverage(fmt->Lookahead[i], fmt->_Lookahead[i], ter1, &nitems1);
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_LPAREN);
			for (j = 0; j < (IntX)nitems1; j++)
			  {
				gId1 = *da_INDEX(ter1->glyphidlist, j);
				strcpy(name1, getGlyphName(gId1, 1));
				getMetrics(gId1, &origShift1, &lsb1, &rsb1, &width1, &tsb1, &bsb1, &vwidth1, &yorig1);
				if (isVert)
				  {
					if (vwidth1 != 0) W1 = vwidth1;
					else  W1 = width1;
				  }
				else
				  W1 = width1;
				proofDrawGlyph(proofctx, 
							   gId1, ANNOT_SHOWIT|ADORN_WIDTHMARKS,
							   name1, ANNOT_SHOWIT|
							   (isVert ? 
								((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
								:
								((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),
							   NULL, 0, /* altlabel,altlabelflags */
							   0,0, /* originDx,originDy */
							   0,0, /* origin, originflags */
							   W1, 0, /* width,widthflags */
							   NULL, yorig1, ""
							   );
				if (j < (IntX)(nitems1 - 1))
				  proofThinspace(proofctx, 1);
			  }
			if (nitems1 > 1)
			  proofSymbol(proofctx, PROOF_RPAREN);
			if (i < (fmt->LookaheadGlyphCount - 1))
			  proofSymbol(proofctx, PROOF_PLUS);
		  }

		if (isException) 
		  {
			proofThinspace(proofctx, 2);
			proofSymbol(proofctx, PROOF_COLON);
			return;
		  }
		proofNewline(proofctx);
		proofSymbol(proofctx, PROOF_YIELDS);


		/* If feature = kern, write the cross product of the first item in the input list with the second item in the input list.*/
		vfmt1 = 0;
		vfmt2 = 0;
		for (i = 0; i < fmt->InputGlyphCount; i++)
		  {
			Card32 nitems1, nitems2 ;
			IntX j, k;
			GlyphId gId1, gId2;
			char name1[MAX_NAME_LEN], name2[MAX_NAME_LEN];
			ttoEnumRec *ter1, *ter2;
			char label[100];
			
			ter1 = da_INDEX(InputGlyphCovListArray, i);
			nitems1 = ter1->glyphidlist.cnt;
			if (i+1 <fmt->InputGlyphCount)
				ter2 = da_INDEX(InputGlyphCovListArray, i+1);
			else
				{
				if (fmt->LookaheadGlyphCount > 0)
					ter2 = da_INDEX(LookaheadGlyphCovListArray, 0);
				else
					ter2 = NULL;
				}
				
			vfmt1 = 0;
			vfmt2 = 0;
			for (j = 0; j < fmt->PosCount; j++)
				{
				Card16 seqindex, lookupindex;
				Card16 gid1, gid2;
				gid1 = 0;
				gid2 = 0;
				gid1 = *da_INDEX(ter1->glyphidlist, 0);
				if (ter2 != NULL)
					gid2 = *da_INDEX(ter2->glyphidlist, 0);
				
				seqindex = fmt->PosLookupRecord[j].SequenceIndex;
				lookupindex = fmt->PosLookupRecord[j].LookupListIndex;
				if (seqindex == i)
					{
					clearValueRecord(&vr1);
					clearValueRecord(&vr2);
					if (i+1 <fmt->InputGlyphCount)
						GPOSEval(lookupindex, gid1, &vfmt1,  &vr1, 0, &vfmt2, &vr2); /* gid2 is the first item in the look-ahead sequence, not in the input sequence. */
					else
						GPOSEval(lookupindex, gid1, &vfmt1,  &vr1, gid2, &vfmt2, &vr2);
					break;
					}
				}
			if ((vfmt1 == 0) && (vfmt2 == 0))
				break; /* there is no positioning info at this input pos. */


			for (j = 0; j < (IntX)nitems1; j++)
			  {
				gId1 = *da_INDEX(ter1->glyphidlist, j);
				strcpy(name1, getGlyphName(gId1, 1));
				getMetrics(gId1, &origShift1, &lsb1, &rsb1, &width1, &tsb1, &bsb1, &vwidth1, &yorig1);
				
				
				if (kern_ == *(Card32*)feattag)
					{
					if (ter2 == NULL)
						break;
					
					nitems2 = ter2->glyphidlist.cnt;
					for (k = 0; k < (IntX)nitems2; k++)
					  {
						gId2 = *da_INDEX(ter2->glyphidlist, k);
						strcpy(name2, getGlyphName(gId2, 1));
						getMetrics(gId2, &origShift2, &lsb2, &rsb2, &width2, &tsb2, &bsb2, &vwidth2, &yorig2);


				  	  label[0] = '\0';
				  	  if (vfmt1 != 0)
					 	 sprintf(label,"%d,%d,%d,%d", vr1.XPlacement, vr1.YPlacement, vr1.XAdvance, vr1.YAdvance);
						proofDrawGlyph(proofctx, 
										 gId1, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 name1, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHT : ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
										 label, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN2 : ANNOT_ATBOTTOMDOWN2), /* altlabel,altlabelflags */
										 vr1.XPlacement, vr1.YPlacement, /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth1+ vr1.YAdvance : width1 + vr1.XAdvance, 0, /* width,widthflags */
										  NULL,
										 (isVert)?yorig1:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
										 );

				  	 label[0] = '\0';
	  	 			 if (vfmt2 != 0)
				  	 	sprintf(label,"%d,%d,%d,%d", vr2.XPlacement, vr2.YPlacement, vr2.YAdvance, vr2.YAdvance);
						  
						proofDrawGlyph(proofctx, 
										 gId2, ANNOT_SHOWIT, /* glyphId,glyphflags */
										 name2, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN1 : ANNOT_ATBOTTOMDOWN1),   /* glyphname,glyphnameflags */
										 label, ANNOT_SHOWIT|(isVert ? ANNOT_ATRIGHTDOWN2 : ANNOT_ATBOTTOMDOWN2), /* altlabel,altlabelflags */
										  vr2.XPlacement,vr2.YPlacement, /* originDx,originDy */
										 0,0, /* origin,originflags */
										 (isVert) ? vwidth2 + vr2.YAdvance : width2 + vr2.XAdvance, 0, /* width,widthflags */
										 NULL, 
										 (isVert)?yorig2:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
										 );
						proofThinspace(proofctx, 1);
					  }
					} /* end if kern feat */
				else
					{ /* just show pos if each glyph.*/
					if (vfmt1 == 0)
						break; /* there is no positioning info at this input pos. */
				  	  label[0] = '\0';
				  	  if ((vfmt1 != 0) && (j == 0))
					 	 sprintf(label,"%d,%d,%d,%d", vr1.XPlacement, vr1.YPlacement, vr1.XAdvance, vr1.YAdvance);
					proofDrawGlyph(proofctx, 
									 gId1, ANNOT_SHOWIT, /* glyphId,glyphflags */
									 name1,  ANNOT_SHOWIT|(isVert ? 
										((j % 2)?ANNOT_ATRIGHTDOWN1:ANNOT_ATRIGHT)
										: 
										((j % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM)),/* glyphname,glyphnameflags */
									 label,  ANNOT_SHOWIT|(isVert ? 
										((j % 2)?ANNOT_ATRIGHTDOWN2:ANNOT_ATRIGHTDOWN1)
										: 
										((j % 2)?ANNOT_ATRIGHTDOWN2:ANNOT_ATBOTTOMDOWN2)),/* glyphname,glyphnameflags */
									 vr1.XPlacement, vr1.YPlacement, /* originDx,originDy */
									 0,0, /* origin,originflags */
									 (isVert) ? vwidth1+ vr1.YAdvance : width1 + vr1.XAdvance, 0, /* width,widthflags */
									 NULL,
									 (isVert)?yorig1:DEFAULT_YORIG_KANJI, (VORGfound!=0)?"*":""
									 );
					proofThinspace(proofctx, 1);
						  
					
					
					
					}
			  } /* end of each glyph in coverage for current inpuyt pos */
		  } /* end for each input position */


		/* free all data */
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

static void listPosLookups3(ChainContextPosFormat3 *fmt, IntX level)
	{
		IntX i;

		for (i = 0; i < fmt->PosCount; i++) 
		  {
			Card16 lis = fmt->PosLookupRecord[i].LookupListIndex;
			
			if (seenChainLookup[lis].seen)
				{
				if (seenChainLookup[lis].cur_parent == GPOSLookupIndex)
					continue;
				seenChainLookup[lis].cur_parent = GPOSLookupIndex;
				if (seenChainLookup[lis].parent != GPOSLookupIndex)
					DL5((OUTPUTBUFF, " %hu*",lis));
				}
			else
				{
				seenChainLookup[lis].parent = GPOSLookupIndex;
				seenChainLookup[lis].cur_parent = GPOSLookupIndex;
				seenChainLookup[lis].seen =1;
				DL5((OUTPUTBUFF, " %hu",lis));
				}
		  }
	}
	
static void dumpChainContext3(ChainContextPosFormat3 *fmt, IntX level, void* feattag)
	{
	IntX i;

	if (level == 8)
		proofPosChainContext3(fmt, level, feattag);
	else if (level==7)
		decompileChainContext3(fmt, level);
	else if (level == 5)
		listPosLookups3(fmt, level);
	else if (level == 6)
	 	 featureWarning(level, SPOT_MSG_GPOSUFMTCCTX3,  fmt->PosFormat);
	else
	  {
		DLu(2, "PosFormat   =", fmt->PosFormat);
		
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
		
		DLu(2, "PosCount =", fmt->PosCount);
		DL(2, (OUTPUTBUFF, "--- PosLookupRecord[index]=(SequenceIndex,LookupListIndex)\n"));
		for (i = 0; i < fmt->PosCount; i++) 
		  {
			Card16 seq = fmt->PosLookupRecord[i].SequenceIndex;
			Card16 lis = fmt->PosLookupRecord[i].LookupListIndex;	
			DL(2, (OUTPUTBUFF, "[%d]=(%hu,%hu) ", i, seq, lis));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
	  }
	}

static void dumpPosChainContext(void *fmt, IntX level, void* feattag)
	{
	  DL(2, (OUTPUTBUFF, "--- ChainingContextPos\n"));
	 GPOSContextRecursionCnt++;  

	 if (GPOSContextRecursionCnt > 1)
		{
		featureWarning(level,  SPOT_MSG_CNTX_RECURSION, ((ChainContextPosFormat1 *)fmt)->PosFormat);
		return;
		}
	  
	  switch (((ChainContextPosFormat1 *)fmt)->PosFormat)
		{
		case 1:
		  dumpChainContext1(fmt, level);
		  break;
		case 2:
		  dumpChainContext2(fmt, level, feattag);
		  break;
		case 3:
		  dumpChainContext3(fmt, level, feattag);
		  break;
		}
	 GPOSContextRecursionCnt--;  
	}

static IntX ChainContextPosFlavor(void *fmt)
	{
	  return (((ChainContextPosFormat1 *)fmt)->PosFormat);
	}

/*To handle extension offsets properly*/
static LOffset prev_offset=0;

static void dumpSubtable(LOffset offset, Card16 type, void *subtable, 
			 IntX level, void *feattag, IntX lookupListIndex, IntX subTableIndex, IntX subTableCount, IntX recursion)
	{
	  char tagstr[5];
	  Card32 scripttag=0, langtag=0;

	  if (subtable == NULL)
		return;

	  if (prev_offset==0)
	  	prev_offset=offset;
	  else
	  	offset=offset+prev_offset;
	  	
	  DL(2, (OUTPUTBUFF, "--- Subtable [%d] (%04x)\n", subTableIndex, offset));
	  if (!recursion)
			{
			GPOStableCount = subTableCount; /* store for later use in text dump only */
			GPOStableindex = subTableIndex; /* store for later use in text dump only*/
			GPOSLookupIndex = lookupListIndex; /* store for later use in text dump only*/
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
	      tagstr[0] = '\0';
	      if (level == 8)
			{
			  warning(SPOT_MSG_GPOSNULFEAT, offset - GPOSStart);
			  return;
			}
	    }

	  if (level == 8 && (!recursion)) /*Don't destroy context if called recursively*/
	    {
	      if ((prev_featuretag != 0) && /* not first time */
			  (featuretag != prev_featuretag)) /* new feature */
			proofDestroyContext(&proofctx);
	    }
	    
	if (level == 6) /* dump kern feature as AFM file. All features but 'kern' are filtered out in ttoDecompileByScript. */
		{			/* Flag changes in script/language, so we can later sort by section. */

		if ((prev_scripttag != scripttag) ||
			(langtag != prev_langtag)) 
			{
			fprintf(AFMout, kScriptLabel" %c%c%c%c Language %c%c%c%c\n", TAG_ARG(scripttag), TAG_ARG(langtag) );
		  		prev_scripttag=scripttag;
		  		prev_langtag=langtag;
			}

		switch (type)
			{
			case SingleAdjustType:
				dumpSinglePos(subtable, level, -1);
				break;
			case PairAdjustType:
				dumpPosPair(subtable, level, -1, -1, -1);
				break;
		case ExtensionPositionType:
		  if (level != 8)
			dumpExtension(subtable, level, feattag);
			break;
		default:
			;
			}
		}
	else
		{
		switch (type)
			{
		case SingleAdjustType:
		  if (level == 5)
			break;
		  else if (level != 8)
			dumpSinglePos(subtable, level, -1);
		  else
			{
			  if (isVerticalFeature(featuretag, SinglePosFlavor(subtable)))
				  proofSetVerticalMode();
			  if ((!recursion) && (featuretag != prev_featuretag)) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
							    STDPAGE_TOP, STDPAGE_BOTTOM,
							    dumpTitle(featuretag, 
								      SinglePosFlavor(subtable)),
							    proofCurrentGlyphSize(), 
							    0.0, unitsPerEm, 
							    0, 1, tagstr);
			 if(featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || (IntX)prev_lookupListIndex!=lookupListIndex){
			  		char message[100];
			  		
			  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
			  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
			  		proofMessage(proofctx, message);
			  		prev_scripttag=scripttag;
			  		prev_langtag=langtag;
			  		prev_lookupListIndex=lookupListIndex;
			  }
			  
			  dumpSinglePos(subtable, level, -1);

			  if (isVerticalFeature(featuretag, SinglePosFlavor(subtable)))
				  proofUnSetVerticalMode();
			  prev_featuretag = featuretag;
			  featuretag = 0;
			}
		  break;

		case PairAdjustType:
			if (level == 5)
			break;
		  else if (level != 8)
			 dumpPosPair(subtable, level, -1, -1, -1);
		  else
			{
			  if (isVerticalFeature(featuretag, PosPairFlavor(subtable)))
				  proofSetVerticalMode();
			  if ((!recursion)&& (featuretag != prev_featuretag)) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
							    STDPAGE_TOP, STDPAGE_BOTTOM,
							    dumpTitle(featuretag,
								      PosPairFlavor(subtable)),
							    proofCurrentGlyphSize(),
							    0.0, unitsPerEm, 
							    0, 1, tagstr);
			  if(featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || (IntX)prev_lookupListIndex!=lookupListIndex){
			  		char message[100];
			  		
			  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
			  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
			  		proofMessage(proofctx, message);
			  		prev_scripttag=scripttag;
			  		prev_langtag=langtag;
			  		prev_lookupListIndex=lookupListIndex;
			  }
			  dumpPosPair(subtable, level, -1, -1, -1);
			  
			  if (isVerticalFeature(featuretag, PosPairFlavor(subtable)))
				  proofUnSetVerticalMode();
			  prev_featuretag = featuretag;
			  featuretag = 0;
			}
		  break;

		case CursiveAttachType:
			if (level == 5)
			break;
		  else if (level != 8)
			 	dumpCursiveAttachPos(subtable, level);
			  else
		    	{
		  		char message[100];
		  		
			  if (proofctx == NULL) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										  STDPAGE_TOP, STDPAGE_BOTTOM,
										  dumpTitle(featuretag, 
													ContextPosFlavor(subtable)),
										  proofCurrentGlyphSize(),
										  0.0, unitsPerEm, 
										  0, 1, tagstr);
		  		sprintf(message, "Error: Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d LookupType %d is not suppoprted.",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex, type);
		  		proofMessage(proofctx, message);
		    	}
	    	break;

	    case MarkToBaseAttachType:
	    case MarkToMarkAttachType:
		  if (level == 5)
			break;
		  else if (level != 8)
			 dumpMarkToBase(subtable, level, type == MarkToMarkAttachType);
			  else
		    	{
		  		char message[100];
		  		
			  if (proofctx == NULL) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										  STDPAGE_TOP, STDPAGE_BOTTOM,
										  dumpTitle(featuretag, 
													ContextPosFlavor(subtable)),
										  proofCurrentGlyphSize(),
										  0.0, unitsPerEm, 
										  0, 1, tagstr);
		  		sprintf(message, "Error: Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d LookupType %d is not supported.",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex, type);
		  		proofMessage(proofctx, message);
		    	}
	    	break;

		case MarkToLigatureAttachType:
		  if (level == 5)
			break;
		  else if (level != 8)
			 dumpMarkToLigatureAttach(subtable, level);
			  else
		    	{
		  		char message[100];
		  		
			  if (proofctx == NULL) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
										  STDPAGE_TOP, STDPAGE_BOTTOM,
										  dumpTitle(featuretag, 
													ContextPosFlavor(subtable)),
										  proofCurrentGlyphSize(),
										  0.0, unitsPerEm, 
										  0, 1, tagstr);
		  		sprintf(message, "Error: Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d LookupType %d is not supported.",
		  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex, type);
		  		proofMessage(proofctx, message);
		    	}
	    	break;

	    case ContextPositionType:
		 	if (level == 5)
			break;
		  else if (level != 8)
	    		dumpPosContext(subtable, level, feattag);
			  else
				{
				  if (isVerticalFeature(featuretag, ContextPosFlavor(subtable)))
					  proofSetVerticalMode();
				  if (proofctx == NULL) /* new feature */
					proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
											  STDPAGE_TOP, STDPAGE_BOTTOM,
											  dumpTitle(featuretag, 
														ContextPosFlavor(subtable)),
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
				  dumpPosContext(subtable, level, feattag);

				  if (isVerticalFeature(featuretag, ContextPosFlavor(subtable)))
					  proofUnSetVerticalMode();
				  featuretag = 0;
				}
			break;
	    case ChainingContextPositionType:
			  if (level == 5)
				{
					IntX i;
				  if( (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag)){
						prev_featuretag = featuretag;
						prev_scripttag=scripttag;
						prev_langtag=langtag;
						prev_lookupListIndex=lookupListIndex;
						for (i=0; i< GPOSLookupCnt; i++)
							{
							seenChainLookup[i].seen=0;
							seenChainLookup[i].parent=0;
							seenChainLookup[i].cur_parent=0;
							}
					  }
					if (subTableIndex == 0)		
						DL5((OUTPUTBUFF, "->("));
					dumpPosChainContext(subtable, level, feattag);
					if (subTableIndex == (subTableCount-1))		
						DL5((OUTPUTBUFF, " ) "));
				}
	 		else if (level != 8)
				{
	    		dumpPosChainContext(subtable, level, feattag);
				}
			  else
				{
				  if (isVerticalFeature(featuretag, ChainContextPosFlavor(subtable)))
					  proofSetVerticalMode();
				  if (proofctx == NULL) /* new feature */
					proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
											  STDPAGE_TOP, STDPAGE_BOTTOM,
											  dumpTitle(featuretag, 
														ChainContextPosFlavor(subtable)),
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

				  dumpPosChainContext(subtable, level, feattag);

				  if (isVerticalFeature(featuretag, ChainContextPosFlavor(subtable)))
					  proofUnSetVerticalMode();
				  featuretag = 0;
				}
			break;
		case ExtensionPositionType:
		  if (level != 8)
			dumpExtension(subtable, level, feattag);
		  else
			{
			  if (isVerticalFeature(featuretag, ExtensionPosFlavor(subtable)))
				  proofSetVerticalMode();
			  if ((!recursion) && featuretag != prev_featuretag) /* new feature */
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
											STDPAGE_TOP, STDPAGE_BOTTOM,
											dumpTitle(featuretag, 
													  ExtensionPosFlavor(subtable)),
											proofCurrentGlyphSize(), 
											0.0, unitsPerEm, 
											0, 1, tagstr);
			  if((!recursion) && (featuretag != prev_featuretag || scripttag != prev_scripttag ||langtag != prev_langtag || (IntX)prev_lookupListIndex!=lookupListIndex)){
			  		char message[100];
			  		
			  		sprintf(message, "Script: '%c%c%c%c' Language: '%c%c%c%c' LookupIndex: %d",
			  				TAG_ARG(scripttag), TAG_ARG(langtag),lookupListIndex);
			  		proofMessage(proofctx, message);
			  		prev_scripttag=scripttag;
			  		prev_langtag=langtag;
			  		prev_lookupListIndex=lookupListIndex;
			  }
			
			  prev_featuretag = featuretag;
			  dumpExtension(subtable, level, feattag);

			  if (isVerticalFeature(featuretag, ExtensionPosFlavor(subtable)))
				  proofUnSetVerticalMode();
			  /*prev_featuretag = featuretag;     ASP: Let the recursive call take care of this*/
			  featuretag = 0;
			}
			break;
		default:
			warning(SPOT_MSG_GPOSUNKDLOOK, type, type, offset - GPOSStart);
			}
		} /* end if level == 6 - else -- */
	  DL(2, (OUTPUTBUFF, " \n"));
	  prev_offset=0;
	}


static IntX searchSubtable(const Card16 lookuptype, void *subtable, 
						   const GlyphId glyph, const GlyphId glyph2_or0, 
						   Card16 *vfmt, ValueRecord *vr, 
						   Card16 *vfmt2, ValueRecord *vr2_or0)
	{
	  if (subtable == NULL)
		return 0;

	  switch (lookuptype)
		{
		case SingleAdjustType:
		  if (searchSinglePos(subtable, glyph, vfmt, vr))
			return 1;
		  break;
		  
		case PairAdjustType:
		  if ((glyph2_or0 == 0) || (vr2_or0 == NULL))
			return 0;
		  if (searchPosPair(subtable, glyph, glyph2_or0, vfmt, vr, vfmt2, vr2_or0))
			return 1;
		  break;

		case ExtensionPositionType:
			{
			ExtensionPosFormat1 *fmt = subtable;
			return searchSubtable(fmt->ExtensionLookupType, fmt->subtable, glyph, glyph2_or0, vfmt, vr, vfmt2, vr2_or0);
			}
			break;

		  /* not yet: */
		case CursiveAttachType:
		case MarkToBaseAttachType:
		case MarkToLigatureAttachType:
		case MarkToMarkAttachType:
		case ContextPositionType:
		default:
		  return 0;
		}
	  return 0;
	}

static int proofreccomp(const void *r1, const void *r2)
{
	ProofRec *rec1=(ProofRec *)r1;
	ProofRec *rec2=(ProofRec *)r2;

	if (rec1->g1!=rec2->g1)
		return rec1->g1-rec2->g1;
	else
		return rec1->g2-rec2->g2;
}

static void GPOSdumpProofRecs(Tag feature)
{
	IntX i;
    featuretag = feature;
	if ( (curproofrec > 0) && (proofrecords.size >= curproofrec) ) 
		{
		qsort(proofrecords.array, curproofrec, sizeof(ProofRec), proofreccomp);
	
		for(i=0; i<curproofrec; i++)
		{
			ProofRec* proofRec = da_INDEX(proofrecords, i);
			if (proofRec->vert)
				proofSetVerticalMode();
			switch (proofRec->type)
			{
				case SingleAdjustType:
					dumpSinglePos(proofRec->fmt, 8, proofRec->i);
					break;
				case PairAdjustType:
					dumpPosPair(proofRec->fmt, 8, proofRec->i, proofRec->j, proofRec->c2);
					break;
				default:
					warning(SPOT_MSG_GPOSUNKDLOOK, proofRec->type, proofRec->type, 0);
			}
			if (proofRec->vert)
				proofUnSetVerticalMode();
		}
		curproofrec=0;
		proofrecords.cnt = 0;
		}
    featuretag = 0;
}



static int strcmpAFM(const void * p1, const void *p2)
{
	/* First compare only the two glyph names, not the value.
	If they are the same, then compare by whether one is a clas value and the other not.
	Then do a straight compare. */
	
	/* find the end of the second glyph name, aka the second space */
	int cmpValue;
	kernPairEntry * s1 = (kernPairEntry *)p1;
	kernPairEntry * s2 = (kernPairEntry *)p2;
	
	cmpValue = strcmp(s1->kernPair, s2->kernPair);
	if (cmpValue != 0)
		return cmpValue;

	/* pair glyph names are identical; either could be  probably class pair, 
	but both could also be a duplicate exceptions.  */
	
	if ((s1->isClass) && (!s2->isClass))
		return 1;
	if ((!s1->isClass) && (s2->isClass))
		return -1;
		
	if (s1->kernValue > s2->kernValue)
		return 1;
	if (s1->kernValue < s2->kernValue)
		return -1;
	return 0;
}
	  
void GPOSDump(IntX level, LongN start)
	{
	IntX i;
	LookupList *lookuplist;
	char tempFileName[MAXAFMLINESIZE];
		
	contextPrefix[0] = 0;
	  if (!loaded) 
		{
		  if (sfntReadTable(GPOS_))
			return;
		}

		GPOSContextRecursionCnt = 0;
		lookuplist = &GPOS._LookupList;
		GPOSLookupCnt = lookuplist->LookupCount;
		seenChainLookup = (SeenChainLookup *)memNew(GPOSLookupCnt*sizeof(SeenChainLookup));
		for (i=0; i< GPOSLookupCnt; i++)
			{
			seenChainLookup[i].seen=0;
			seenChainLookup[i].parent=0;
			seenChainLookup[i].cur_parent=0;
			}
			
	  if (level == 5) /* just list feature tags */
		{
		  fprintf(OUTPUTBUFF,  "GPOS Features:\n");
		  ttoDumpFeaturesByScript(&GPOS._ScriptList, &GPOS._FeatureList, &GPOS._LookupList, dumpSubtable, level);
		  /*ttoDumpFeatureList(GPOS.FeatureList, &GPOS._FeatureList, level);*/
		  return;
		}


	  headGetUnitsPerEm(&unitsPerEm, GPOS_);

	  if ((opt_Present("-P") && level!=7) || (level == 8)) /* make a proof file */
		{
			initProofRecs();
		  ttoDumpLookupifFeaturePresent(&GPOS._LookupList, &GPOS._FeatureList, &GPOS._ScriptList,
						8, 
						dumpSubtable, dumpMessage, GPOSdumpProofRecs);
		  if (proofctx) /* dumpSubtable will create proofctx with -P*/
		  	proofDestroyContext(&proofctx);
		  freeProofRecs();
		}
	  else if ( (level == 6) || (level == 7) ) /* AFM or features-file text syntax dump */
	  {
	  	char afmFilePath[128];
		afmFilePath[0] = 0;
	  	/*Write to temporary buffer in order to be able to sort AFM data before it is printed*/
	  	if (level==6)
	  		{
			strcpy(tempFileName, afmFilePath);
			strcat(tempFileName, "temp.txt");
	  		AFMout=fopen(tempFileName, "w");
	  		}
	    /* dump FeatureLookup-subtables according to Script */
	  	ttoDecompileByScript(&GPOS._ScriptList, &GPOS._FeatureList, &GPOS._LookupList, dumpSubtable, level);
	  /* dumpSubtable only creates proofctx with level 8 */
	  	
	  	/* Process and dump AFM data*/
	  	if (level==6)
	  	{
	  		long lineCount;
	  		int scanNum;
	  		char* inChar; /* anything except EOF */
	  		char	inLine[MAXAFMLINESIZE];
	  		kernPairEntry *kernEntry;
	  		da_DCL( kernPairEntry, afmLines);
	  		da_INIT(afmLines, 1000, 100);
	  		fclose(AFMout);
	  		AFMout=fopen(afmFilePath, "r");
	  		
	  		/* first data line is expected to be a script/language comment line. */
	  		kernEntry = da_INDEX(afmLines, 0);
	  		lineCount = 0;
	  		inChar = fgets(inLine, MAXAFMLINESIZE, AFMout);
	  		while( (inChar != NULL) && (strncmp((char*)inLine, kScriptLabel, 5) != 0))
	  			{
	  			inChar = fgets(inLine, MAXAFMLINESIZE, AFMout);
	  			lineCount++;
	  			}
	  			
	  		if (inChar != NULL)
	  			fprintf(OUTPUTBUFF, "Comment Begin %s", inLine);

	  		lineCount = 0;
	  		while(inChar != NULL)
	  			{
	  			inChar = fgets(inLine, MAXAFMLINESIZE, AFMout);
	  			if (inChar != NULL)
	  				{
		  			if (inLine[0] == ' ')/* skip blank lines */
		  				continue;
		  			}
	  				
	  			if ((inChar != NULL) && (0 != strncmp(inLine, kScriptLabel, kScriptLabelLen) ) )
	  				{
	  				/* collect data from temp AFM file line */
	  				char prefix[6];
	  				char name1[MAX_NAME_LEN];
	  				char name2[MAX_NAME_LEN];
	  				int val1 = 0, val2 = 0, val3 = 0;
	  				kernEntry = da_INDEX(afmLines, lineCount);
	  				scanNum = sscanf(inLine,"%5s %64s %64s %d %d %d", prefix, name1, name2, &val1, &val2, &val3);
	  				sprintf(kernEntry->kernPair, "%s %s %s", prefix, name1, name2);
	  				kernEntry->kernValue = val1;
	  				if (scanNum == 5)
	  					{
	  					kernEntry->kernValue2 = 0;
	  					kernEntry->isClass = val2;
	  					}
	  				else if (scanNum == 6)
	  					{
	  					kernEntry->kernValue2 = val2 ;
	  					kernEntry->isClass = val3;
	  					}
	  				else
	  					{
	  					fprintf(stderr, " Bad scan line! < %s >",inLine );
		  					} 
	  				lineCount++;
	  				}
	  			else 
	  				{
	  				long i;
	  				long uniquePairCount =0;
	  				kernPairEntry* prevKernEntry = NULL;
	  				/* New script/lang section, or EOF. Sort lines between last script line and new one. */
			  		qsort(afmLines.array, lineCount, sizeof(kernPairEntry), strcmpAFM);
	  				
	  				/* now check which are duplicates*/
			  		for( i=0; i < lineCount; i++)
			  		{
			  			kernEntry = &afmLines.array[i];
			  			if ((prevKernEntry != NULL) && (0 == strcmp(kernEntry->kernPair, prevKernEntry->kernPair)))
			  				kernEntry->isDuplicate = 1;
			  			else
			  				{
			  				prevKernEntry = kernEntry;
			  				kernEntry->isDuplicate = 0;
			  				uniquePairCount++;
			  				}
			  		}
			  		
	  				fprintf(OUTPUTBUFF, "StartKernPairs %ld\n", uniquePairCount);
	  				
	  				/* now write out the unique pairs*/
			  		for( i=0; i < lineCount; i++)
			  		{
			  			kernEntry = &afmLines.array[i];
			  			if (kernEntry->isDuplicate)
			  				continue; /* skip additional kern pairs for same two glyphs */
			  			fprintf(OUTPUTBUFF, "%s %d", kernEntry->kernPair, kernEntry->kernValue);
			  			if (kernEntry->kernValue2)
			  				fprintf(OUTPUTBUFF, "%d", kernEntry->kernValue2);
			  			/*
			  			if (kernEntry->isClass)
			  				fprintf(OUTPUTBUFF, " # class");
			  			*/
			  			fprintf(OUTPUTBUFF, "\n");
			  		}
			  		fprintf(OUTPUTBUFF, "EndKernPairs\n");
			  		
			  		if (inChar != NULL) /* Was new script line. set up for next script block. */
			  			{
	  					fputs("Comment End Script\n", OUTPUTBUFF);
	  	  				fprintf(OUTPUTBUFF, "Comment Begin %s", inLine);
						afmLines.cnt = 1;
	  					lineCount = 0;
	  					}
	  				else
	  					{
	  					fputs("Comment End Script\n", OUTPUTBUFF);
	  					if (!feof(AFMout))
	  						fprintf(stderr, "Error - %s  failed to read from AFM temporary file for reason other than end of file.", global.progname);
	  					}
	  				}
	  			}
	  		da_FREE(afmLines);
	  		fclose(AFMout);
			remove(tempFileName);
	  		remove(afmFilePath);
	  	} /* end if level 6 */
	  }
	  else { /* straight text dump */
		DL(1, (OUTPUTBUFF, "### [GPOS] (%08lx)\n", start));
		
		DLV(2, "Version    =", GPOS.Version);
		DLx(2, "ScriptList =", GPOS.ScriptList);
		DLx(2, "FeatureList=", GPOS.FeatureList);
		DLx(2, "LookupList =", GPOS.LookupList);
		
		ttoDumpScriptList(GPOS.ScriptList, &GPOS._ScriptList, level);
		ttoDumpFeatureList(GPOS.FeatureList, &GPOS._FeatureList, level);
		ttoDumpLookupList(GPOS.LookupList, &GPOS._LookupList, level,
						  dumpSubtable);
	  }
	  
	  memFree(seenChainLookup);

	if (proofctx)
		proofDestroyContext(&proofctx); /* finish final feature */
	}
	
static void freeSingleAdjustType (void *subtable)
	{
	SinglePosFormat1 *fmt1;
	SinglePosFormat2 *fmt2;
	switch (SinglePosFlavor(subtable))
		{
			case 1:
				fmt1 = (SinglePosFormat1 *)subtable;
				ttoFreeCoverage(fmt1->_Coverage);					
				memFree(subtable);	
				break;
			case 2:
				fmt2 = (SinglePosFormat2 *)subtable;
				ttoFreeCoverage(fmt2->_Coverage);
				memFree(fmt2->Value);
				memFree(subtable);	
				break;
		}
	}

static void freePairAdjustType (void *subtable)
	{
	PosPairFormat1 *fmt1;
	PosPairFormat2 *fmt2;
	IntX i;
	switch (PosPairFlavor(subtable))
		{
			case 1:
				fmt1 = (PosPairFormat1 *)subtable;
				ttoFreeCoverage(fmt1->_Coverage);
				for (i=0; i < fmt1->PairSetCount; i++)
				  {
					memFree(fmt1->_PairSet[i].PairValueRecord);
				  }
				memFree(fmt1->_PairSet);
				memFree(fmt1->PairSet);
				memFree(subtable);
				break;
			case 2:
				fmt2 = (PosPairFormat2 *)subtable;
				ttoFreeCoverage(fmt2->_Coverage);
				ttoFreeClass(fmt2->_ClassDef1);
				ttoFreeClass(fmt2->_ClassDef2);
				for (i = 0; i < fmt2->Class1Count; i++)
					{
					 Class1Record *record1 = &fmt2->Class1Record[i];
					 memFree(record1->Class2Record);
					}
				memFree(fmt2->Class1Record);
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
	case SingleAdjustType:	
		freeSingleAdjustType (subtable);
		break;

	case PairAdjustType:
		freePairAdjustType (subtable);
		break;
		  
    case MarkToBaseAttachType:
	case CursiveAttachType:
	case MarkToLigatureAttachType:
    case MarkToMarkAttachType:
    case ContextPositionType:
    			/* not yet supported */
	default:
		break;
		}
	}

void GPOSFree(void)
	{
	  if (!loaded)
		return;

	ttoFreeScriptList(&GPOS._ScriptList);
	ttoFreeFeatureList(&GPOS._FeatureList);
	ttoFreeLookupList(&GPOS._LookupList, freeSubtable);
	loaded = 0;
	contextPrefix[0] = 0;
	/*featuretag = 0;*/
  	prev_featuretag = 0;
	}

void GPOSUsage(void)
	{
	  fprintf(OUTPUTBUFF,  "--- GPOS\n"
		 "=5  List GPOS features\n"
		 "=6  Dump kerning data in AFM-file style\n"
		 "=7  De-compile GPOS feature(s) in features-file style\n" 
		 "=8  Proof GPOS features\n");

	}
