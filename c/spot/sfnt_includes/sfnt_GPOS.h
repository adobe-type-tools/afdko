/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GPOS.h	1.2
 * Changed:    7/6/98 16:42:14
 ***********************************************************************/

/*
 * Glyph positioning format definition.
 */

#ifndef FORMAT_GPOS_H
#define FORMAT_GPOS_H

enum
	{
	SingleAdjustType = 1,
	PairAdjustType,
	CursiveAttachType,
	MarkToBaseAttachType,
	MarkToLigatureAttachType,
	MarkToMarkAttachType,
	ContextPositionType,
	ChainingContextPositionType,
	ExtensionPositionType
	};

/* --- Shared Tables --- */

/* --- ValueRecord --- */
#define ValueXPlacement	(1<<0)
#define ValueYPlacement (1<<1)
#define ValueXAdvance	(1<<2)
#define ValueYAdvance	(1<<3)
#define ValueXPlaDevice (1<<4)
#define ValueYPlaDevice (1<<5)
#define ValueXAdvDevice (1<<6)
#define ValueYAdvDevice (1<<7)

/* MM extensions */
#define ValueXIdPlacement       (1<<8)
#define ValueYIdPlacement       (1<<9)
#define ValueXIdAdvance         (1<<10)
#define ValueYIdAdvance         (1<<11)


typedef struct
	{
	Int16 XPlacement;
	Int16 YPlacement;
	Int16 XAdvance;
	Int16 YAdvance;
	Offset XPlaDevice;					/* => Device */
	Offset YPlaDevice;					/* => Device */
	Offset XAdvDevice;					/* => Device */
	Offset YAdvDevice;					/* => Device */
	} ValueRecord;

/* --- Anchor Table --- */
typedef struct
	{
	Card16 AnchorFormat;				/* =1 */
	Int16 XCoordinate;
	Int16 YCoordinate;
	} AnchorFormat1;

typedef struct
	{
	Card16 AnchorFormat;				/* =2 */
	Int16 XCoordinate;
	Int16 YCoordinate;
	Card16 AnchorPoint;
	} AnchorFormat2;

typedef struct
	{
	Card16 AnchorFormat;				/* =3 */
	Int16 XCoordinate;
	Int16 YCoordinate;
	OFFSET(DeviceTable*,  XDeviceTable);
	OFFSET(DeviceTable*, YDeviceTable);
	} AnchorFormat3;

typedef struct
	{
	Card16 AnchorFormat;				/* =4 */
	Card16 XIdAnchor;
	Card16 YIdAnchor;
	} AnchorFormat4;


/* --- Mark Array --- */
typedef struct
	{
	Card16 Class;
	OFFSET(void *, MarkAnchor);					/* => Anchor */
	} MarkRecord;

typedef struct
	{
	Card16 MarkCount;
	MarkRecord *MarkRecord;				/* [MarkCount] */
	} MarkArray;

typedef struct
	{
	OFFSET_ARRAY(void *, LigatureAnchor);					/* => Anchor[ClassCount] */
	} ComponentRecord;

typedef struct
	{
	Card16 ComponentCount;
	ComponentRecord *ComponentRecord;				/* [ComponentCount] */
	} LigatureAttach;

typedef struct
	{
	Card16 LigatureCount;
	OFFSET_ARRAY(LigatureAttach, LigatureAttach);					/* => LigatureAttach[LigatureCount] */
	} LigatureArray;

/* --- Single Adjustment (type 1) --- */
typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, Coverage);					/* => Coverage */
	Card16 ValueFormat;
	ValueRecord Value;
	} SinglePosFormat1;

typedef struct
	{
	Card16 PosFormat;					/* =2 */
	OFFSET(void *, Coverage);					/* => Coverage */
	Card16 ValueFormat;
	Card16 ValueCount;
	ValueRecord *Value;					/* [ValueCount] */
	} SinglePosFormat2;

/* --- Pair Adjustment (type 2) --- */
typedef struct
	{
	GlyphId SecondGlyph;
	ValueRecord Value1;
	ValueRecord Value2;
	} PairValueRecord;

typedef struct
	{
	Card16 PairValueCount;
	PairValueRecord *PairValueRecord;	/* [PairValueCount] */
	} PairSet;

typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, Coverage);					/* => Coverage */
	Card16 ValueFormat1;
	Card16 ValueFormat2;
	Card16 PairSetCount;
	OFFSET_ARRAY(PairSet, PairSet);		/* [PairSetCount] */
	} PosPairFormat1;

typedef struct
	{
	ValueRecord Value1;
	ValueRecord Value2;
	} Class2Record;

typedef struct
	{
	Class2Record *Class2Record;			/* [Class2Count] */
	} Class1Record;

typedef struct
	{
	Card16 PosFormat;					/* =2 */
	OFFSET(void *, Coverage);					/* => Coverage */
	Card16 ValueFormat1;
	Card16 ValueFormat2;
	OFFSET(void *, ClassDef1);					/* => ClassDef */
	OFFSET(void *, ClassDef2);					/* => ClassDef */
	Card16 Class1Count;
	Card16 Class2Count;
	Class1Record *Class1Record;			/* [Class1Count] */
	} PosPairFormat2; 

/* --- Cursive Attachment (type 3) --- */
typedef struct
	{
	OFFSET(void *, EntryAnchor);					/* => Anchor */
	OFFSET(void *, ExitAnchor);					/* => Anchor */
	} EntryExitRecord;

typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 EntryExitCount;
	EntryExitRecord *EntryExitRecord;	/* [EntryExitCount] */
	} CursivePosFormat1;

/* --- Mark To Base Attachment (type 4) --- */
typedef struct
	{
	OFFSET(void *, BaseAnchor);					/* => Anchor */
	} AnchorRecord;

typedef struct
	{
	OFFSET_ARRAY(void *, BaseAnchor);					/* => Anchor[ClassCount] */
	} BaseRecord;

typedef struct
	{
	Card16 BaseCount;
	BaseRecord *BaseRecord;				/* [BaseCount] */
	} BaseArray;

typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, MarkCoverage);				/* => Coverage */
	OFFSET(void *, BaseCoverage);				/* => Coverage */
	Card16 ClassCount;
	OFFSET(MarkArray, MarkArray);					/* => MarkArray */
	OFFSET(BaseArray, BaseArray);					/* => BaseArray */
	} MarkBasePosFormat1;

typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, MarkCoverage);				/* => Coverage */
	OFFSET(void *, LigatureCoverage);				/* => Coverage */
	Card16 ClassCount;
	OFFSET(MarkArray, MarkArray);					/* => MarkArray */
	OFFSET(LigatureArray, LigatureArray);					/* => LigatureArray */
	} MarkToLigPosFormat1;



/* --- Contextual Substitution (type 7) --- */

typedef struct
	{
	Card16 SequenceIndex;
	Card16 LookupListIndex;
	} PosLookupRecord;

typedef struct
	{
	Card16 GlyphCount;
	Card16 PosCount;
	GlyphId *Input;						/* [GlyphCount - 1] */
	PosLookupRecord *PosLookupRecord;	/* [PosCount] */
	} PosRule;

typedef struct
	{
	Card16 PosRuleCount;
	OFFSET_ARRAY(PosRule, PosRule);		/* [PosRuleCount] */
	} PosRuleSet;

typedef struct
	{
	Card16 PosFormat;					/* =1 */
	OFFSET(void *, Coverage);
	Card16 PosRuleSetCount;
	OFFSET_ARRAY(PosRuleSet, PosRuleSet); /* [PosRuleSetCount] */
	} ContextPosFormat1;

typedef struct
	{
	Card16 GlyphCount;
	Card16 PosCount;
	Card16 *Class;						/* [GlyphCount - 1] */
	PosLookupRecord *PosLookupRecord;	/* [PosCount] */
	} PosClassRule;

typedef struct
	{
	Card16 PosClassRuleCnt;
	OFFSET_ARRAY(PosClassRule, PosClassRule);	/* [PosClassRuleCnt] */
	} PosClassSet;

typedef struct
	{
	Card16 PosFormat;					/* =2 */
	OFFSET(void *, Coverage);
	OFFSET(void *, ClassDef);
	Card16 PosClassSetCnt;
	OFFSET_ARRAY(PosClassSet, PosClassSet);	/* [PosClassSetCnt] */
	} ContextPosFormat2;

typedef struct
	{
	Card16 PosFormat;					/* =3 */
	Card16 GlyphCount;
	Card16 PosCount;
	OFFSET_ARRAY(void *, CoverageArray);		/* [GlyphCount] */
	PosLookupRecord *PosLookupRecord;	/* [PosCount] */
	} ContextPosFormat3;

/* --- Chaining Contextual Substitution (type 8) --- */
typedef struct
    {
    Card16 BacktrackGlyphCount;
    GlyphId *Backtrack;                             /* [BacktrackGlyphCount] */
    Card16 InputGlyphCount;
    GlyphId *Input;                                 /* [InputGlyphCount -1] */
    Card16 LookaheadGlyphCount;
    GlyphId *Lookahead;                             /* [LookaheadGlyphCount] */
    Card16 PosCount;
    PosLookupRecord *PosLookupRecord;           /* [PosCount] */
    } ChainPosRule;

typedef struct
    {
    Card16 ChainPosRuleCount;
    OFFSET_ARRAY(ChainPosRule, ChainPosRule);   /* [ChainPosRuleCount] */
    } ChainPosRuleSet;


typedef struct
    {
    Card16 PosFormat;                     /* =1 */
    OFFSET(void *, Coverage);
    Card16 ChainPosRuleSetCount;
    OFFSET_ARRAY(ChainPosRuleSet, ChainPosRuleSet); /* [ChainPosRuleSetCount]*/
    } ChainContextPosFormat1;


typedef struct
    {
    Card16 BacktrackGlyphCount;
    Card16 *Backtrack;                      /* [BacktrackGlyphCount] */
    Card16 InputGlyphCount;
    Card16 *Input;                          /* [InputGlyphCount - 1] */
    Card16 LookaheadGlyphCount;
    Card16 *Lookahead;                      /* [LookaheadGlyphCount] */
    Card16 PosCount;
    PosLookupRecord *PosLookupRecord;           /* [PosCount] */
    } ChainPosClassRule;

typedef struct
    {
    Card16 ChainPosClassRuleCnt;
    OFFSET_ARRAY(ChainPosClassRule, ChainPosClassRule); /* [ChainPosClassRuleCnt]*/
    } ChainPosClassSet;

typedef struct
    {
    Card16 PosFormat;                     /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, BackTrackClassDef);
    OFFSET(void *, InputClassDef);
    OFFSET(void *, LookAheadClassDef);
    Card16 ChainPosClassSetCnt;
    OFFSET_ARRAY(ChainPosClassSet, ChainPosClassSet); /* [ChainPosClassSetCnt] */
    } ChainContextPosFormat2;

typedef struct
    {
    Card16 PosFormat;                     /* =3 */
    Card16 BacktrackGlyphCount;
    OFFSET_ARRAY(void *, Backtrack);        /* [BacktrackGlyphCount] offsets to coverage tables */
    Card16 InputGlyphCount;
    OFFSET_ARRAY(void *, Input);            /* [InputGlyphCount] offsets to coverage tables */
    Card16 LookaheadGlyphCount;
    OFFSET_ARRAY(void *, Lookahead);        /* [LookaheadGlyphCount] offsets to coverage tables*/
    Card16 PosCount;
    PosLookupRecord *PosLookupRecord;   /* [PosCount] */
    } ChainContextPosFormat3;

/* --- Extension Lookup (type 9) --- */
typedef struct
	{
	Card16 PosFormat;					/* =1 */
	Card16 ExtensionLookupType;
	Card32 ExtensionOffset;
	void *subtable;					/*pointer to the table in the overflow subtable*/
	} ExtensionPosFormat1;

typedef struct
	{
	Fixed Version;
	OFFSET(ScriptList, ScriptList);					/* => ScriptList */
	OFFSET(FeatureList, FeatureList);					/* => FeatureList */
	OFFSET(LookupList, LookupList);					/* => LookupList */
	} GPOSTbl;

#endif /* FORMAT_GPOS_H */
