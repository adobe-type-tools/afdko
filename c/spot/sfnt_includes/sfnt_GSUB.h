/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TrueType Open glyph substitution table format definition.
 */

#ifndef FORMAT_GSUB_H
#define FORMAT_GSUB_H

#include "tto.h"

/* --- Lookup types --- */
enum {
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
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    int16_t DeltaGlyphId;
} SingleSubstFormat1;

typedef struct
{
    uint16_t SubstFormat; /* =2 */
    OFFSET(void *, Coverage);
    uint16_t GlyphCount;
    GlyphId *Substitute; /* [GlyphCount] */
} SingleSubstFormat2;

/* --- Multiple Substitution (type 2) --- */
typedef struct
{
    uint16_t GlyphCount;
    GlyphId *Substitute; /* [GlyphCount] */
} Sequence;

typedef struct
{
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t SequenceCount;
    OFFSET_ARRAY(Sequence, Sequence); /* [SequenceCount] */
} MultipleSubstFormat1;

#define kMaxMultipleSub 100 /* the max glyphs we'll report when the MultipleSubstFormat1 is being referenced from a Context Sub.*/

/* --- Alternate Substitution (type 3) --- */
typedef struct
{
    uint16_t GlyphCount;
    GlyphId *Alternate; /* [GlyphCount] */
} AlternateSet;

typedef struct
{
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t AlternateSetCnt;
    OFFSET_ARRAY(AlternateSet, AlternateSet); /* [AlternateSetCnt] */
} AlternateSubstFormat1;

/* --- Ligature Substitution (type 4) --- */
typedef struct
{
    GlyphId LigGlyph;
    uint16_t CompCount;
    GlyphId *Component; /* [CompCount - 1] */
} Ligature;

typedef struct
{
    uint16_t LigatureCount;
    OFFSET_ARRAY(Ligature, Ligature); /* [LigatureCount] */
} LigatureSet;

typedef struct
{
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t LigSetCount;
    OFFSET_ARRAY(LigatureSet, LigatureSet); /* [LigSetCount] */
} LigatureSubstFormat1;

/* --- Contextual Substitution (type 5) --- */
typedef struct
{
    uint16_t SequenceIndex;
    uint16_t LookupListIndex;
} SubstLookupRecord;

typedef struct
{
    uint16_t GlyphCount;
    uint16_t SubstCount;
    GlyphId *Input;                       /* [GlyphCount - 1] */
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} SubRule;

typedef struct
{
    uint16_t SubRuleCount;
    OFFSET_ARRAY(SubRule, SubRule); /* [SubRuleCount] */
} SubRuleSet;

typedef struct
{
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t SubRuleSetCount;
    OFFSET_ARRAY(SubRuleSet, SubRuleSet); /* [SubRuleSetCount] */
} ContextSubstFormat1;

typedef struct
{
    uint16_t GlyphCount;
    uint16_t SubstCount;
    uint16_t *Class;                        /* [GlyphCount - 1] */
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} SubClassRule;

typedef struct
{
    uint16_t SubClassRuleCnt;
    OFFSET_ARRAY(SubClassRule, SubClassRule); /* [SubClassRuleCnt] */
} SubClassSet;

typedef struct
{
    uint16_t SubstFormat; /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, ClassDef);
    uint16_t SubClassSetCnt;
    OFFSET_ARRAY(SubClassSet, SubClassSet); /* [SubClassSetCnt] */
} ContextSubstFormat2;

typedef struct
{
    uint16_t SubstFormat; /* =3 */
    uint16_t GlyphCount;
    uint16_t SubstCount;
    OFFSET_ARRAY(void *, CoverageArray);  /* [GlyphCount] */
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} ContextSubstFormat3;

/* --- Chaining Contextual Substitution (type 6) --- */
typedef struct
{
    uint16_t BacktrackGlyphCount;
    GlyphId *Backtrack; /* [BacktrackGlyphCount] */
    uint16_t InputGlyphCount;
    GlyphId *Input; /* [InputGlyphCount -1] */
    uint16_t LookaheadGlyphCount;
    GlyphId *Lookahead; /* [LookaheadGlyphCount] */
    uint16_t SubstCount;
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} ChainSubRule;

typedef struct
{
    uint16_t ChainSubRuleCount;
    OFFSET_ARRAY(ChainSubRule, ChainSubRule); /* [ChainSubRuleCount] */
} ChainSubRuleSet;

typedef struct
{
    uint16_t SubstFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t ChainSubRuleSetCount;
    OFFSET_ARRAY(ChainSubRuleSet, ChainSubRuleSet); /* [ChainSubRuleSetCount]*/
} ChainContextSubstFormat1;

typedef struct
{
    uint16_t BacktrackGlyphCount;
    uint16_t *Backtrack; /* [BacktrackGlyphCount] */
    uint16_t InputGlyphCount;
    uint16_t *Input; /* [InputGlyphCount - 1] */
    uint16_t LookaheadGlyphCount;
    uint16_t *Lookahead; /* [LookaheadGlyphCount] */
    uint16_t SubstCount;
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} ChainSubClassRule;

typedef struct
{
    uint16_t ChainSubClassRuleCnt;
    OFFSET_ARRAY(ChainSubClassRule, ChainSubClassRule); /* [ChainSubClassRuleCnt]*/
} ChainSubClassSet;

typedef struct
{
    uint16_t SubstFormat; /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, BackTrackClassDef);
    OFFSET(void *, InputClassDef);
    OFFSET(void *, LookAheadClassDef);
    uint16_t ChainSubClassSetCnt;
    OFFSET_ARRAY(ChainSubClassSet, ChainSubClassSet); /* [ChainSubClassSetCnt] */
} ChainContextSubstFormat2;

typedef struct
{
    uint16_t SubstFormat; /* =3 */
    uint16_t BacktrackGlyphCount;
    OFFSET_ARRAY(void *, Backtrack); /* [BacktrackGlyphCount] */
    uint16_t InputGlyphCount;
    OFFSET_ARRAY(void *, Input); /* [InputGlyphCount] */
    uint16_t LookaheadGlyphCount;
    OFFSET_ARRAY(void *, Lookahead); /* [LookaheadGlyphCount] */
    uint16_t SubstCount;
    SubstLookupRecord *SubstLookupRecord; /* [SubstCount] */
} ChainContextSubstFormat3;

typedef struct
{
    uint16_t SubstFormat; /* =3 */
    OFFSET(void *, InputCoverage);
    uint16_t BacktrackGlyphCount;
    OFFSET_ARRAY(void *, Backtrack); /* [BacktrackGlyphCount] */
    uint16_t LookaheadGlyphCount;
    OFFSET_ARRAY(void *, Lookahead); /* [LookaheadGlyphCount] */
    uint16_t GlyphCount;
    GlyphId *Substitutions; /* [SubstCount] */
} ReverseChainContextSubstFormat1;

/* --- Extended Substitution (type 7) --- */
typedef struct
{
    uint16_t SubstFormat; /* =1 */
    uint16_t OverflowLookupType;
    uint32_t OverflowOffset;
    void *subtable; /*pointer to the table in the overflow subtable*/
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
