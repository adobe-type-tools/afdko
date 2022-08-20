/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Shared TrueType Open format definitions.
 */

#ifndef FORMAT_TTO_H
#define FORMAT_TTO_H

/* Special types */
typedef uint32_t Tag;
/* In the file an Offset is a byte offset of data relative to some format
   component, normally the beginning of the record it belongs to. The
   OFFSET macros allow a simple declaration of both the byte offset field and
   a structure containing the data for that offset. */
typedef uint16_t Offset;
typedef uint32_t LOffset;

#define OFFSET(type, name) \
    Offset name;           \
    type _##name
#define OFFSET_ARRAY(type, name) \
    Offset *name;                \
    type *_##name

#define LOFFSET(type, name) \
    LOffset name;           \
    type _##name
#define LOFFSET_ARRAY(type, name) \
    LOffset *name;                \
    type *_##name

/* --- ScriptList --- */
typedef struct
{
    Offset LookupOrder; /* => LookupOrder, Reserved=NULL */
    uint16_t ReqFeatureIndex;
    uint16_t FeatureCount;
    uint16_t *FeatureIndex; /* [FeatureCount] */
} LangSys;

typedef struct
{
    Tag LangSysTag;
    OFFSET(LangSys, LangSys); /* From Script */
} LangSysRecord;

typedef struct
{
    OFFSET(LangSys, DefaultLangSys);
    uint16_t LangSysCount;
    LangSysRecord *LangSysRecord; /* [LangSysCount] */
} Script;

typedef struct
{
    Tag ScriptTag;
    OFFSET(Script, Script); /* From ScriptList */
} ScriptRecord;

typedef struct
{
    uint16_t ScriptCount;
    ScriptRecord *ScriptRecord; /* [ScriptCount] */
} ScriptList;

/* --- FeatureList --- */
typedef void *FeatureParam; /* Fake type until MS specs this */
typedef struct
{
    OFFSET(FeatureParam, FeatureParam); /* =>FeatureParam, Reserved=NULL */
    uint16_t LookupCount;
    uint16_t *LookupListIndex; /* [LookupCount] */
} Feature;

typedef struct
{
    Tag FeatureTag;
    OFFSET(Feature, Feature); /* From FeatureList */
} FeatureRecord;

typedef struct
{
    uint16_t FeatureCount;
    FeatureRecord *FeatureRecord; /* [FeatureCount] */
} FeatureList;

/* --- LookupList --- */
typedef struct
{
    uint16_t LookupType;
    uint16_t LookupFlag;
#define RightToLeft         (1 << 0)
#define IgnoreBaseGlyphs    (1 << 1)
#define IgnoreLigatures     (1 << 2)
#define IgnoreMarks         (1 << 3)
#define UseMarkFilteringSet (1 << 4)
#define MarkAttachmentType  (0xFF00)
#define kNumNamedLookups    6
#define DumpLookupSeen      (1 << 8) /* our own flag for "spot" */

    uint16_t SubTableCount;
    OFFSET_ARRAY(void *, SubTable); /* [SubTableCount] */
    uint16_t MarkFilteringSet;
} Lookup;

typedef struct
{
    uint16_t LookupCount;
    OFFSET_ARRAY(Lookup, Lookup); /* [LookupCount] */
} LookupList;

/* --- Coverage --- */
typedef struct
{
    uint16_t CoverageFormat; /* =1 */
    uint16_t GlyphCount;
    GlyphId *GlyphArray; /* [GlyphCount] */
} CoverageFormat1;

typedef struct
{
    GlyphId Start;
    GlyphId End;
    uint16_t StartCoverageIndex;
} RangeRecord;

typedef struct
{
    uint16_t CoverageFormat; /* =2 */
    uint16_t RangeCount;
    RangeRecord *RangeRecord; /* [RangeCount] */
} CoverageFormat2;

/* --- ClassDef --- */
typedef struct
{
    uint16_t ClassFormat; /* =1 */
    GlyphId StartGlyph;
    uint16_t GlyphCount;
    uint16_t *ClassValueArray; /* [GlyphCount] */
} ClassDefFormat1;

typedef struct
{
    GlyphId Start;
    GlyphId End;
    uint16_t Class;
} ClassRangeRecord;

typedef struct
{
    uint16_t ClassFormat; /* =2 */
    uint16_t ClassRangeCount;
    ClassRangeRecord *ClassRangeRecord; /* [ClassRangeCount] */
} ClassDefFormat2;

/* --- DeviceTable --- */
typedef struct
{
    uint16_t StartSize;
    uint16_t EndSize;
    uint16_t DeltaFormat;
#define DeltaBits2 1
#define DeltaBits4 2
#define DeltaBits8 3
    uint16_t *DeltaValue; /* [((EndSize-StartSize+1)*(2^DeltaFormat)+15)/16] */
} DeviceTable;

#endif /* FORMAT_TTO_H */
