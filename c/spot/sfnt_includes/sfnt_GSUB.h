/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GSUB.h	1.3
 * Changed:    9/4/98 16:35:00
 ***********************************************************************/

/*
 * TrueType Open glyph substution table format definition.
 */

#ifndef FORMAT_GSUB_H
#define FORMAT_GSUB_H

#include "tto.h"

/* --- Lookup types --- */
enum
	{
	SingleSubsType = 1,
	MultipleSubsType,
	AlternateSubsType,
	LigatureSubsType,
	ContextSubsType,
	ChainingContextSubsType,
	OverflowSubsType,
	ReverseChainContextType
	};

/* --- Single Substitution (type 1) --- */
typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Int16 DeltaGlyphId;
	} SingleSubstFormat1;

typedef struct
	{
	Card16 SubstFormat;					/* =2 */
	OFFSET(void *, Coverage);
	Card16 GlyphCount;
	GlyphId *Substitute;				/* [GlyphCount] */	
	} SingleSubstFormat2;

/* --- Multiple Substitution (type 2) --- */
typedef struct
	{
	Card16 GlyphCount;
	GlyphId *Substitute;				/* [GlyphCount] */
	} Sequence;

typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 SequenceCount;
	OFFSET_ARRAY(Sequence, Sequence);	/* [SequenceCount] */
	} MultipleSubstFormat1;

#define kMaxMultipleSub 100 /* the max glyphs we'll report when the MultipleSubstFormat1 is being referenced from a Context Sub.*/

/* --- Alternate Substitution (type 3) --- */
typedef struct
	{
	Card16 GlyphCount;
	GlyphId *Alternate;					/* [GlyphCount] */
	} AlternateSet;

typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 AlternateSetCnt;
	OFFSET_ARRAY(AlternateSet, AlternateSet);	/* [AlternateSetCnt] */
	} AlternateSubstFormat1;

/* --- Ligature Substitution (type 4) --- */
typedef struct
	{
	GlyphId LigGlyph;
	Card16 CompCount;
	GlyphId *Component;					/* [CompCount - 1] */
	} Ligature;

typedef struct
	{
	Card16 LigatureCount;
	OFFSET_ARRAY(Ligature, Ligature);	/* [LigatureCount] */
	} LigatureSet;

typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 LigSetCount;
	OFFSET_ARRAY(LigatureSet, LigatureSet);		/* [LigSetCount] */
	} LigatureSubstFormat1;

/* --- Contextual Substitution (type 5) --- */
typedef struct
	{
	Card16 SequenceIndex;
	Card16 LookupListIndex;
	} SubstLookupRecord;

typedef struct
	{
	Card16 GlyphCount;
	Card16 SubstCount;
	GlyphId *Input;						/* [GlyphCount - 1] */
	SubstLookupRecord *SubstLookupRecord;	/* [SubstCount] */
	} SubRule;

typedef struct
	{
	Card16 SubRuleCount;
	OFFSET_ARRAY(SubRule, SubRule);		/* [SubRuleCount] */
	} SubRuleSet;

typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 SubRuleSetCount;
	OFFSET_ARRAY(SubRuleSet, SubRuleSet); /* [SubRuleSetCount] */
	} ContextSubstFormat1;

typedef struct
	{
	Card16 GlyphCount;
	Card16 SubstCount;
	Card16 *Class;						/* [GlyphCount - 1] */
	SubstLookupRecord *SubstLookupRecord;	/* [SubstCount] */
	} SubClassRule;

typedef struct
	{
	Card16 SubClassRuleCnt;
	OFFSET_ARRAY(SubClassRule, SubClassRule);	/* [SubClassRuleCnt] */
	} SubClassSet;

typedef struct
	{
	Card16 SubstFormat;					/* =2 */
	OFFSET(void *, Coverage);
	OFFSET(void *, ClassDef);
	Card16 SubClassSetCnt;
	OFFSET_ARRAY(SubClassSet, SubClassSet);	/* [SubClassSetCnt] */
	} ContextSubstFormat2;

typedef struct
	{
	Card16 SubstFormat;					/* =3 */
	Card16 GlyphCount;
	Card16 SubstCount;
	OFFSET_ARRAY(void *, CoverageArray);		/* [GlyphCount] */
	SubstLookupRecord *SubstLookupRecord;	/* [SubstCount] */
	} ContextSubstFormat3;

/* --- Chaining Contextual Substitution (type 6) --- */
typedef struct
    {
    Card16 BacktrackGlyphCount;
    GlyphId *Backtrack;                             /* [BacktrackGlyphCount] */
    Card16 InputGlyphCount;
    GlyphId *Input;                                 /* [InputGlyphCount -1] */
    Card16 LookaheadGlyphCount;
    GlyphId *Lookahead;                             /* [LookaheadGlyphCount] */
    Card16 SubstCount;
    SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
    } ChainSubRule;

typedef struct
    {
    Card16 ChainSubRuleCount;
    OFFSET_ARRAY(ChainSubRule, ChainSubRule);   /* [ChainSubRuleCount] */
    } ChainSubRuleSet;


typedef struct
    {
    Card16 SubstFormat;                     /* =1 */
    OFFSET(void *, Coverage);
    Card16 ChainSubRuleSetCount;
    OFFSET_ARRAY(ChainSubRuleSet, ChainSubRuleSet); /* [ChainSubRuleSetCount]*/
    } ChainContextSubstFormat1;


typedef struct
    {
    Card16 BacktrackGlyphCount;
    Card16 *Backtrack;                      /* [BacktrackGlyphCount] */
    Card16 InputGlyphCount;
    Card16 *Input;                          /* [InputGlyphCount - 1] */
    Card16 LookaheadGlyphCount;
    Card16 *Lookahead;                      /* [LookaheadGlyphCount] */
    Card16 SubstCount;
    SubstLookupRecord *SubstLookupRecord;           /* [SubstCount] */
    } ChainSubClassRule;

typedef struct
    {
    Card16 ChainSubClassRuleCnt;
    OFFSET_ARRAY(ChainSubClassRule, ChainSubClassRule); /* [ChainSubClassRuleCnt]*/
    } ChainSubClassSet;

typedef struct
    {
    Card16 SubstFormat;                     /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, BackTrackClassDef);
    OFFSET(void *, InputClassDef);
    OFFSET(void *, LookAheadClassDef);
    Card16 ChainSubClassSetCnt;
    OFFSET_ARRAY(ChainSubClassSet, ChainSubClassSet); /* [ChainSubClassSetCnt] */
    } ChainContextSubstFormat2;

typedef struct
    {
    Card16 SubstFormat;                     /* =3 */
    Card16 BacktrackGlyphCount;
    OFFSET_ARRAY(void *, Backtrack);        /* [BacktrackGlyphCount] */
    Card16 InputGlyphCount;
    OFFSET_ARRAY(void *, Input);            /* [InputGlyphCount] */
    Card16 LookaheadGlyphCount;
    OFFSET_ARRAY(void *, Lookahead);        /* [LookaheadGlyphCount] */
    Card16 SubstCount;
    SubstLookupRecord *SubstLookupRecord;   /* [SubstCount] */
    } ChainContextSubstFormat3;

typedef struct
    {
    Card16 SubstFormat;                     /* =3 */
     OFFSET(void *, InputCoverage);
     Card16 BacktrackGlyphCount;
     OFFSET_ARRAY(void *, Backtrack);        /* [BacktrackGlyphCount] */
     Card16 LookaheadGlyphCount;
     OFFSET_ARRAY(void *, Lookahead);        /* [LookaheadGlyphCount] */
     Card16 GlyphCount;
    GlyphId *Substitutions;   /* [SubstCount] */
    } ReverseChainContextSubstFormat1;

/* --- Extended Substitution (type 7) --- */
typedef struct
	{
	Card16 SubstFormat;					/* =1 */
	Card16 OverflowLookupType;
	Card32 OverflowOffset;
	void *subtable;					/*pointer to the table in the overflow subtable*/
	} OverflowSubstFormat1;

/* --- Header --- */
typedef struct
	{
	Fixed Version;
	OFFSET(ScriptList, ScriptList);
	OFFSET(FeatureList, FeatureList);
	OFFSET(LookupList, LookupList);	
	} GSUBTbl;

#endif /* FORMAT_GSUB_H */
