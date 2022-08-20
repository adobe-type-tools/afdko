/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0.
This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Glyph positioning format definition.
 */

#ifndef FORMAT_GPOS_H
#define FORMAT_GPOS_H

enum {
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
#define ValueXPlacement (1 << 0)
#define ValueYPlacement (1 << 1)
#define ValueXAdvance   (1 << 2)
#define ValueYAdvance   (1 << 3)
#define ValueXPlaDevice (1 << 4)
#define ValueYPlaDevice (1 << 5)
#define ValueXAdvDevice (1 << 6)
#define ValueYAdvDevice (1 << 7)

/* MM extensions */
#define ValueXIdPlacement (1 << 8)
#define ValueYIdPlacement (1 << 9)
#define ValueXIdAdvance   (1 << 10)
#define ValueYIdAdvance   (1 << 11)

typedef struct
{
    int16_t XPlacement;
    int16_t YPlacement;
    int16_t XAdvance;
    int16_t YAdvance;
    Offset XPlaDevice; /* => Device */
    Offset YPlaDevice; /* => Device */
    Offset XAdvDevice; /* => Device */
    Offset YAdvDevice; /* => Device */
} ValueRecord;

/* --- Anchor Table --- */
typedef struct
{
    uint16_t AnchorFormat; /* =1 */
    int16_t XCoordinate;
    int16_t YCoordinate;
} AnchorFormat1;

typedef struct
{
    uint16_t AnchorFormat; /* =2 */
    int16_t XCoordinate;
    int16_t YCoordinate;
    uint16_t AnchorPoint;
} AnchorFormat2;

typedef struct
{
    uint16_t AnchorFormat; /* =3 */
    int16_t XCoordinate;
    int16_t YCoordinate;
    OFFSET(DeviceTable *, XDeviceTable);
    OFFSET(DeviceTable *, YDeviceTable);
} AnchorFormat3;

typedef struct
{
    uint16_t AnchorFormat; /* =4 */
    uint16_t XIdAnchor;
    uint16_t YIdAnchor;
} AnchorFormat4;

/* --- Mark Array --- */
typedef struct
{
    uint16_t Class;
    OFFSET(void *, MarkAnchor); /* => Anchor */
} MarkRecord;

typedef struct
{
    uint16_t MarkCount;
    MarkRecord *MarkRecord; /* [MarkCount] */
} MarkArray;

typedef struct
{
    OFFSET_ARRAY(void *, LigatureAnchor); /* => Anchor[ClassCount] */
} ComponentRecord;

typedef struct
{
    uint16_t ComponentCount;
    ComponentRecord *ComponentRecord; /* [ComponentCount] */
} LigatureAttach;

typedef struct
{
    uint16_t LigatureCount;
    OFFSET_ARRAY(LigatureAttach, LigatureAttach); /* => LigatureAttach[LigatureCount] */
} LigatureArray;

/* --- Single Adjustment (type 1) --- */
typedef struct
{
    uint16_t PosFormat;         /* =1 */
    OFFSET(void *, Coverage); /* => Coverage */
    uint16_t ValueFormat;
    ValueRecord Value;
} SinglePosFormat1;

typedef struct
{
    uint16_t PosFormat;         /* =2 */
    OFFSET(void *, Coverage); /* => Coverage */
    uint16_t ValueFormat;
    uint16_t ValueCount;
    ValueRecord *Value; /* [ValueCount] */
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
    uint16_t PairValueCount;
    PairValueRecord *PairValueRecord; /* [PairValueCount] */
} PairSet;

typedef struct
{
    uint16_t PosFormat;         /* =1 */
    OFFSET(void *, Coverage); /* => Coverage */
    uint16_t ValueFormat1;
    uint16_t ValueFormat2;
    uint16_t PairSetCount;
    OFFSET_ARRAY(PairSet, PairSet); /* [PairSetCount] */
} PosPairFormat1;

typedef struct
{
    ValueRecord Value1;
    ValueRecord Value2;
} Class2Record;

typedef struct
{
    Class2Record *Class2Record; /* [Class2Count] */
} Class1Record;

typedef struct
{
    uint16_t PosFormat;         /* =2 */
    OFFSET(void *, Coverage); /* => Coverage */
    uint16_t ValueFormat1;
    uint16_t ValueFormat2;
    OFFSET(void *, ClassDef1); /* => ClassDef */
    OFFSET(void *, ClassDef2); /* => ClassDef */
    uint16_t Class1Count;
    uint16_t Class2Count;
    Class1Record *Class1Record; /* [Class1Count] */
} PosPairFormat2;

/* --- Cursive Attachment (type 3) --- */
typedef struct
{
    OFFSET(void *, EntryAnchor); /* => Anchor */
    OFFSET(void *, ExitAnchor);  /* => Anchor */
} EntryExitRecord;

typedef struct
{
    uint16_t PosFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t EntryExitCount;
    EntryExitRecord *EntryExitRecord; /* [EntryExitCount] */
} CursivePosFormat1;

/* --- Mark To Base Attachment (type 4) --- */
typedef struct
{
    OFFSET(void *, BaseAnchor); /* => Anchor */
} AnchorRecord;

typedef struct
{
    OFFSET_ARRAY(void *, BaseAnchor); /* => Anchor[ClassCount] */
} BaseRecord;

typedef struct
{
    uint16_t BaseCount;
    BaseRecord *BaseRecord; /* [BaseCount] */
} BaseArray;

typedef struct
{
    uint16_t PosFormat;             /* =1 */
    OFFSET(void *, MarkCoverage); /* => Coverage */
    OFFSET(void *, BaseCoverage); /* => Coverage */
    uint16_t ClassCount;
    OFFSET(MarkArray, MarkArray); /* => MarkArray */
    OFFSET(BaseArray, BaseArray); /* => BaseArray */
} MarkBasePosFormat1;

typedef struct
{
    uint16_t PosFormat;                 /* =1 */
    OFFSET(void *, MarkCoverage);     /* => Coverage */
    OFFSET(void *, LigatureCoverage); /* => Coverage */
    uint16_t ClassCount;
    OFFSET(MarkArray, MarkArray);         /* => MarkArray */
    OFFSET(LigatureArray, LigatureArray); /* => LigatureArray */
} MarkToLigPosFormat1;

/* --- Contextual Substitution (type 7) --- */

typedef struct
{
    uint16_t SequenceIndex;
    uint16_t LookupListIndex;
} PosLookupRecord;

typedef struct
{
    uint16_t GlyphCount;
    uint16_t PosCount;
    GlyphId *Input;                   /* [GlyphCount - 1] */
    PosLookupRecord *PosLookupRecord; /* [PosCount] */
} PosRule;

typedef struct
{
    uint16_t PosRuleCount;
    OFFSET_ARRAY(PosRule, PosRule); /* [PosRuleCount] */
} PosRuleSet;

typedef struct
{
    uint16_t PosFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t PosRuleSetCount;
    OFFSET_ARRAY(PosRuleSet, PosRuleSet); /* [PosRuleSetCount] */
} ContextPosFormat1;

typedef struct
{
    uint16_t GlyphCount;
    uint16_t PosCount;
    uint16_t *Class;                    /* [GlyphCount - 1] */
    PosLookupRecord *PosLookupRecord; /* [PosCount] */
} PosClassRule;

typedef struct
{
    uint16_t PosClassRuleCnt;
    OFFSET_ARRAY(PosClassRule, PosClassRule); /* [PosClassRuleCnt] */
} PosClassSet;

typedef struct
{
    uint16_t PosFormat; /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, ClassDef);
    uint16_t PosClassSetCnt;
    OFFSET_ARRAY(PosClassSet, PosClassSet); /* [PosClassSetCnt] */
} ContextPosFormat2;

typedef struct
{
    uint16_t PosFormat; /* =3 */
    uint16_t GlyphCount;
    uint16_t PosCount;
    OFFSET_ARRAY(void *, CoverageArray); /* [GlyphCount] */
    PosLookupRecord *PosLookupRecord;    /* [PosCount] */
} ContextPosFormat3;

/* --- Chaining Contextual Substitution (type 8) --- */
typedef struct
{
    uint16_t BacktrackGlyphCount;
    GlyphId *Backtrack; /* [BacktrackGlyphCount] */
    uint16_t InputGlyphCount;
    GlyphId *Input; /* [InputGlyphCount -1] */
    uint16_t LookaheadGlyphCount;
    GlyphId *Lookahead; /* [LookaheadGlyphCount] */
    uint16_t PosCount;
    PosLookupRecord *PosLookupRecord; /* [PosCount] */
} ChainPosRule;

typedef struct
{
    uint16_t ChainPosRuleCount;
    OFFSET_ARRAY(ChainPosRule, ChainPosRule); /* [ChainPosRuleCount] */
} ChainPosRuleSet;

typedef struct
{
    uint16_t PosFormat; /* =1 */
    OFFSET(void *, Coverage);
    uint16_t ChainPosRuleSetCount;
    OFFSET_ARRAY(ChainPosRuleSet, ChainPosRuleSet); /* [ChainPosRuleSetCount]*/
} ChainContextPosFormat1;

typedef struct
{
    uint16_t BacktrackGlyphCount;
    uint16_t *Backtrack; /* [BacktrackGlyphCount] */
    uint16_t InputGlyphCount;
    uint16_t *Input; /* [InputGlyphCount - 1] */
    uint16_t LookaheadGlyphCount;
    uint16_t *Lookahead; /* [LookaheadGlyphCount] */
    uint16_t PosCount;
    PosLookupRecord *PosLookupRecord; /* [PosCount] */
} ChainPosClassRule;

typedef struct
{
    uint16_t ChainPosClassRuleCnt;
    OFFSET_ARRAY(ChainPosClassRule, ChainPosClassRule); /* [ChainPosClassRuleCnt]*/
} ChainPosClassSet;

typedef struct
{
    uint16_t PosFormat; /* =2 */
    OFFSET(void *, Coverage);
    OFFSET(void *, BackTrackClassDef);
    OFFSET(void *, InputClassDef);
    OFFSET(void *, LookAheadClassDef);
    uint16_t ChainPosClassSetCnt;
    OFFSET_ARRAY(ChainPosClassSet, ChainPosClassSet); /* [ChainPosClassSetCnt] */
} ChainContextPosFormat2;

typedef struct
{
    uint16_t PosFormat; /* =3 */
    uint16_t BacktrackGlyphCount;
    OFFSET_ARRAY(void *, Backtrack); /* [BacktrackGlyphCount] offsets to coverage tables */
    uint16_t InputGlyphCount;
    OFFSET_ARRAY(void *, Input); /* [InputGlyphCount] offsets to coverage tables */
    uint16_t LookaheadGlyphCount;
    OFFSET_ARRAY(void *, Lookahead); /* [LookaheadGlyphCount] offsets to coverage tables*/
    uint16_t PosCount;
    PosLookupRecord *PosLookupRecord; /* [PosCount] */
} ChainContextPosFormat3;

/* --- Extension Lookup (type 9) --- */
typedef struct
{
    uint16_t PosFormat; /* =1 */
    uint16_t ExtensionLookupType;
    uint32_t ExtensionOffset;
    void *subtable; /*pointer to the table in the overflow subtable*/
} ExtensionPosFormat1;

typedef struct
{
    Fixed Version;
    OFFSET(ScriptList, ScriptList);   /* => ScriptList */
    OFFSET(FeatureList, FeatureList); /* => FeatureList */
    OFFSET(LookupList, LookupList);   /* => LookupList */
} GPOSTbl;

#endif /* FORMAT_GPOS_H */
