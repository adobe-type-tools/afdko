/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)tto.h	1.1
 * Changed:    10/12/95 14:59:09
 ***********************************************************************/

/*
 * Shared TrueType Open format definitions.
 */

#ifndef FORMAT_TTO_H
#define FORMAT_TTO_H

/* Special types */
typedef Card32 Tag;
/* In the file an Offset is a byte offset of data relative to some format
   component, normally the beginning of the record it belongs to. The 
   OFFSET macros allow a simple declaration of both the byte offset field and
   a structure containg the data for that offset. */
typedef Card16 Offset;
typedef Card32 LOffset;

#define OFFSET(type,name) \
	Offset name; \
	type _ ## name
#define OFFSET_ARRAY(type,name) \
	Offset *name; \
	type *_ ## name

#define LOFFSET(type,name) \
	LOffset name; \
	type _ ## name
#define LOFFSET_ARRAY(type,name) \
	LOffset *name; \
	type *_ ## name

/* --- ScriptList --- */
typedef struct
	{
	Offset LookupOrder;					/* => LookupOrder, Reserved=NULL */
	Card16 ReqFeatureIndex;
	Card16 FeatureCount;
	Card16 *FeatureIndex;				/* [FeatureCount] */
	} LangSys;

typedef struct
	{
	Tag LangSysTag;
	OFFSET(LangSys, LangSys);			/* From Script */
	} LangSysRecord;

typedef struct
	{
	OFFSET(LangSys, DefaultLangSys);
	Card16 LangSysCount;
	LangSysRecord *LangSysRecord;		/* [LangSysCount] */
	} Script;

typedef struct
	{
	Tag ScriptTag;
	OFFSET(Script, Script);				/* From ScriptList */
	} ScriptRecord;

typedef struct
	{
	Card16 ScriptCount;
	ScriptRecord *ScriptRecord;			/* [ScriptCount] */
	} ScriptList;

/* --- FeatureList --- */
typedef void *FeatureParam;				/* Fake type until MS specs this */
typedef struct
	{
	OFFSET(FeatureParam, FeatureParam);		/* =>FeatureParam, Reserved=NULL */
	Card16 LookupCount;
	Card16 *LookupListIndex;			/* [LookupCount] */
	} Feature;

typedef struct
	{
	Tag FeatureTag;
	OFFSET(Feature, Feature);			/* From FeatureList */
	} FeatureRecord;

typedef struct
	{
	Card16 FeatureCount;
	FeatureRecord *FeatureRecord;		/* [FeatureCount] */
	} FeatureList;

/* --- LookupList --- */
typedef struct
	{
	Card16 LookupType;
	Card16 LookupFlag;
#define RightToLeft			(1<<0)
#define IgnoreBaseGlyphs	(1<<1)
#define IgnoreLigatures		(1<<2)
#define IgnoreMarks			(1<<3)
#define UseMarkFilteringSet	(1<<4)
#define MarkAttachmentType	(0xFF00)
#define kNumNamedLookups  6

#define     DumpLookupSeen      (1<<8) /* our own flag for "spot" */

	Card16 SubTableCount;
	OFFSET_ARRAY(void *, SubTable);		/* [SubTableCount] */
	Card16 MarkFilteringSet;
	} Lookup;

typedef struct
	{
	Card16 LookupCount;
	OFFSET_ARRAY(Lookup, Lookup);		/* [LookupCount] */
	} LookupList;

/* --- Coverage --- */
typedef struct
	{
	Card16 CoverageFormat;				/* =1 */
	Card16 GlyphCount;
	GlyphId *GlyphArray;				/* [GlyphCount] */
	} CoverageFormat1;

typedef struct
	{
	GlyphId Start;
	GlyphId End;
	Card16 StartCoverageIndex;
	} RangeRecord;

typedef struct
	{
	Card16 CoverageFormat;				/* =2 */
	Card16 RangeCount;
	RangeRecord *RangeRecord;			/* [RangeCount] */
	} CoverageFormat2;

/* --- ClassDef --- */
typedef struct
	{
	Card16 ClassFormat;					/* =1 */
	GlyphId StartGlyph;
	Card16 GlyphCount;
	Card16 *ClassValueArray;			/* [GlyphCount] */
	} ClassDefFormat1;

typedef struct
	{
	GlyphId Start;
	GlyphId End;
	Card16 Class;
	} ClassRangeRecord;

typedef struct
	{
	Card16 ClassFormat;					/* =2 */
	Card16 ClassRangeCount;
	ClassRangeRecord *ClassRangeRecord;	/* [ClassRangeCount] */
	} ClassDefFormat2;

/* --- DeviceTable --- */
typedef struct
	{
	Card16 StartSize;
	Card16 EndSize;
	Card16 DeltaFormat;
#define DeltaBits2	1
#define DeltaBits4	2
#define DeltaBits8	3
	Card16 *DeltaValue; /* [((EndSize-StartSize+1)*(2^DeltaFormat)+15)/16] */
	} DeviceTable;

#endif /* FORMAT_TTO_H */
