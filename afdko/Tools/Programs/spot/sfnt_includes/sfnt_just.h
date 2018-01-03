/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)just.h	1.2
 * Changed:    7/14/95 18:29:32
 ***********************************************************************/

/*
 * Justification table format definition.
 */

#ifndef FORMAT_JUST_H
#define FORMAT_JUST_H

#define just_VERSION VERSION(1,0)

/* Ligature decompostion action */
typedef struct
	{
	GlyphId ligglyph;
	Fixed lowerLimit;
	Fixed upperLimit;
	Card16 order;
	Card16 nGlyphs;
	GlyphId *glyph;
	} DecompAction;
#define DECOMP_HDR_SIZE (SIZEOF(DecompAction, lowerLimit) + \
						 SIZEOF(DecompAction, upperLimit) + \
						 SIZEOF(DecompAction, order) + \
						 SIZEOF(DecompAction, nGlyphs))

/* Post compensation action header */
typedef struct
	{
	Card16 class;
	Card16 type;
	Card32 length;
	void *typeSpecific;
	} ActionHdr;
#define ACTION_HDR_SIZE (SIZEOF(ActionHdr, class) + \
						 SIZEOF(ActionHdr, type) + \
						 SIZEOF(ActionHdr, length))

/* Action types */
enum
	{
	Decomposition,
	UncondAddGlyph,
	CondAddGlyph,
	StretchGlyph,
	DuctileGlyph
	};

/* Post compensation action list */
typedef struct
	{
	Card32 nActions;
	ActionHdr *action;
	} PostcompAction;

/* Width delta pair */
typedef struct
	{
	Card32 class;
	Fixed beforeGrowLimit;
	Fixed beforeShrinkLimit;
	Fixed afterGrowLimit;
	Fixed afterShrinkLimit;
	Card16 growFlags;
#define FLAG_UNLIMITED	(1<<12)
	Card16 shrinkFlags;
	} WidthDeltaPair;
#define WDC_SIZE (SIZEOF(WidthDeltaPair, class) + \
				  SIZEOF(WidthDeltaPair, beforeGrowLimit) + \
				  SIZEOF(WidthDeltaPair, beforeShrinkLimit) + \
				  SIZEOF(WidthDeltaPair, afterGrowLimit) + \
				  SIZEOF(WidthDeltaPair, afterShrinkLimit) + \
				  SIZEOF(WidthDeltaPair, growFlags) + \
				  SIZEOF(WidthDeltaPair, shrinkFlags))
	
/* Width delta cluster */
typedef struct
	{
	Card32 nPairs;
	WidthDeltaPair *pair;
	} WidthDeltaCluster;

/* Direction header */
typedef struct
	{
	Card16 classOffset;
	Card16 wdcOffset;
	Card16 pcOffset;
	struct
		{
		Lookup lookup;
#define WDC_LOOKUP_INTL 256
#define WDC_LOOKUP_INCR 256
		DCL_ARRAY(WidthDeltaCluster, record);
		Card16 lookupSize;	/* Size of the lookup table (bytes) */
		Card16 dataSize;	/* Size of the wdc data (bytes) */
		} wdc;
	struct
		{
		Lookup lookup;
#define PC_LOOKUP_INTL 5
#define PC_LOOKUP_INCR 5
		DCL_ARRAY(PostcompAction, record);
		Card16 totalSize;	/* Size of the lookup table + pc data (bytes) */
		} pc;
	} DirectionHdr;
#define DIRECTION_HDR_SIZE (SIZEOF(DirectionHdr, classOffset) + \
							SIZEOF(DirectionHdr, wdcOffset) + \
							SIZEOF(DirectionHdr, pcOffset))

/* The justification table */
typedef struct
	{
	Fixed version;
	Card16 format;
	Card16 horizOffset;
	Card16 vertOffset;
	DirectionHdr horiz;
	DirectionHdr vert;
	} justTbl;
#define TBL_HDR_SIZE (SIZEOF(justTbl, version) + \
					  SIZEOF(justTbl, format) + \
					  SIZEOF(justTbl, horizOffset) + \
					  SIZEOF(justTbl, vertOffset))

#endif /* FORMAT_JUST_H */
