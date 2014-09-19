/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)kern.h	1.2
 * Changed:    8/10/95 15:41:35
 ***********************************************************************/

/*
 * Kern table format definition.
 */

#ifndef FORMAT_KERN_H
#define FORMAT_KERN_H

#define kern_VERSION VERSION(1,0)

/* ### Pair kerning */
typedef struct
	{
	GlyphId left;
	GlyphId right;
	FWord *value;
	} Pair;
#define PAIR_SIZE(n) (SIZEOF(Pair, left) + \
					  SIZEOF(Pair, right) + \
					  SIZEOF(Pair, value[0]) * (n))

typedef struct
	{
	Card16 nPairs;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Pair *pair;
	} Format0;
#define FORMAT0_HDR_SIZE (SIZEOF(Format0, nPairs) + \
						  SIZEOF(Format0, searchRange) + \
						  SIZEOF(Format0, entrySelector) + \
						  SIZEOF(Format0, rangeShift))

/* ### Class kerning format 2*/
typedef struct
	{
	GlyphId firstGlyph;
	Card16 nGlyphs;
	Card16 *classes;
	Card16 nClasses;	/* [Not in format] */
	} Class;
#define CLASS_SIZE(n) (SIZEOF(Class, firstGlyph) + \
					   SIZEOF(Class, nGlyphs) + \
					   SIZEOF(Class, classes[0]) * (n))

typedef struct
	{
	Card16 rowWidth;
	Card16 leftClassOffset;
	Card16 rightClassOffset;
	Card16 arrayOffset;
	FWord *array;
	Class left;
	Class right;
	} Format2;
#define FORMAT2_HDR_SIZE (SIZEOF(Format2, rowWidth) + \
						  SIZEOF(Format2, leftClassOffset) + \
						  SIZEOF(Format2, rightClassOffset) + \
						  SIZEOF(Format2, arrayOffset))


/* ### Class kerning format 3*/

typedef struct
	{
	Card16 glyphCount;
	Card8 kernValueCount;
	Card8 leftClassCount;
	Card8 rightClassCount;
	Card8 flags;
	FWord *kernValue;
	Card8 *leftClass;
	Card8 *rightClass;
	Card8 *kernIndex;
	} Format3;
	
#define FORMAT3_HDR_SIZE (SIZEOF(Format3, glyphCount) + \
						  SIZEOF(Format3, kernValueCount) + \
						  SIZEOF(Format3, leftClassCount) + \
						  SIZEOF(Format3, rightClassCount) + \
						  SIZEOF(Format3, flags) )

/* ### Apple format kern table */
typedef struct
	{
	Card32 length;
	Card16 coverage;
#define COVERAGE_FORMAT	0x00ff
	Card16 tupleIndex;
	void *format;
	} Subtable;
#define SUBTABLE_HDR_SIZE (SIZEOF(Subtable, length) + \
						   SIZEOF(Subtable, coverage) + \
						   SIZEOF(Subtable, tupleIndex))

typedef struct
	{
	Fixed version;
	Card32 nTables;
	DCL_ARRAY(Subtable, subtable);
	} kernTbl;
#define TBL_HDR_SIZE (SIZEOF(kernTbl, version) + \
					  SIZEOF(kernTbl, nTables))

/* ### Microsoft format kern table (really Apple's earlier design) */
typedef struct
	{
	Card16 version;
	Card16 length;
	Card16 coverage;
#define MS_COVERAGE_FORMAT	0xff00
	void *format;
	} MSSubtable;
#define MS_SUBTABLE_HDR_SIZE (SIZEOF(MSSubtable, version) + \
							  SIZEOF(MSSubtable, length) + \
							  SIZEOF(MSSubtable, coverage))

typedef struct
	{
	Card16 version;
	Card16 nTables;
	MSSubtable *subtable;
	} MSkernTbl;
#define MS_TBL_HDR_SIZE (SIZEOF(MSkernTbl, version) + \
						 SIZEOF(MSkernTbl, nTables))

#endif /* FORMAT_KERN_H */
