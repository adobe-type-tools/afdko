/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)lookup.h	1.2
 * Changed:    7/14/95 17:44:19
 ***********************************************************************/

/*
 * Lookup table format definition.
 */

#ifndef FORMAT_LOOKUP_H
#define FORMAT_LOOKUP_H

/* 
 * Different lookup clients store different lookup DATA:
 *
 * Table        Data
 * -----        ----
 * ALMX			1 + 4 x nMasters (words)
 * ROTA			1 + 2 x nMasters (words)
 * VFMX			3 x nMasters (words)
 * bsln         1-word baseline value
 * just (wdc)   1-word offset to wdc (wdc base rel)
 * just (pca)   1-word offset to pca (pca base rel)
 * lcar         1-word offset to lcce (lcar rel)
 * mort			1-word glyphId
 * opbd         4 x 1-word optical bounds (left, top, right, bottom)
 * prop         1-word property
 *
 * Where:
 *      word    16 bits
 *      wdc     Width Delta Cluster
 *      pca     Post Compensation Action,
 *      lcce    Ligature Caret Class Entry
 *
 * In the following specification a lookup VALUE is expressed as either:
 *
 *      data    The actual lookup data
 *      offset  An offset to the lookup data
 *
 * , and number in brackets is the size of the value in words.
 *
 * Format 0
 * ========
 * Table        Value       Offset base
 * -----        -----       -----------
 * ALMX			data[<var>]
 * ROTA			data[<var>]
 * VFMX			data[<var>]
 * bsln         data[1]
 * just (wdc)   data[1]
 * just (pca)   data[1]
 * lcar         data[1]
 * mort			data[1]
 * opbd         data[4]
 * prop         data[1]
 *
 * Format 2
 * ========
 * ALMX			data[<var>]
 * ROTA			data[<var>]
 * VFMX			data[<var>]
 * bsln         data[1]
 * just (wdc)   data[1]
 * just (pca)   data[1]
 * lcar         data[1]
 * mort			data[1]
 * opbd         offset[1]   opbd
 * prop         data[1]
 *
 * Format 4
 * ========
 * ALMX			offset[2]	lookup
 * ROTA			offset[2]	lookup
 * VFMX			offset[2]	lookup
 * bsln         offset[1]   lookup
 * just (wdc)   offset[1]   lookup
 * just (pca)   offset[1]   lookup
 * lcar         offset[1]   lcar
 * mort			offset[1]	lookup
 * opbd         offset[1]   opbd
 * prop         offset[1]   lookup
 *
 * Format 6
 * ========
 * ALMX			data[<var>]
 * ROTA			data[<var>]
 * VFMX			data[<var>]
 * bsln         data[1]
 * just (wdc)   data[1]
 * just (pca)   data[1]
 * lcar         data[1]
 * mort			data[1]
 * opbd         offset[1]   opbd
 * prop         data[1]
 *
 * Format 8
 * ========
 * ALMX			data[<var>]
 * ROTA			data[<var>]
 * VFMX			data[<var>]
 * bsln         data[1]
 * just (wdc)   data[1]
 * just (pca)   data[1]
 * lcar         data[1]
 * mort			data[1]
 * opbd         data[4]
 * prop         data[1]
 *
 * Formats 0 and 8 require a contiguous range of glyphIds. This is unlikely in
 * practice so missing values are added to make the range contiguous.
 *
 * The lcar lcce and just pc offsets are lcar and just relative, respectively,
 * and therefore need adjusting once the lookup table size is known.
 */

typedef struct
	{
	Card16 format;
	Card16 *value;
	} Lookup0;

typedef struct
	{
	Card16 segmentSize;
	Card16 nSegments;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	} BinSearchHdr;
#define BIN_SEARCH_HDR_SIZE (SIZEOF(BinSearchHdr, segmentSize) + \
							 SIZEOF(BinSearchHdr, nSegments) + \
							 SIZEOF(BinSearchHdr, searchRange) + \
							 SIZEOF(BinSearchHdr, entrySelector) + \
							 SIZEOF(BinSearchHdr, rangeShift))

typedef struct
	{
	GlyphId lastGlyph;
	GlyphId firstGlyph;
	Card16 *value;
	} Segment2;
#define SEGMENT2_HDR_SIZE (SIZEOF(Segment2, lastGlyph) + \
						   SIZEOF(Segment2, firstGlyph))
#define SEGMENT2_SIZE (SEGMENT2_HDR_SIZE + \
					   SIZEOF(Segment2, value[0]))

typedef struct
	{
	Card16 format;
	BinSearchHdr search;
	Segment2 *segment;
	Card16 *data;		/* Only used by opbd */
	} Lookup2;

typedef struct
	{
	GlyphId lastGlyph;
	GlyphId firstGlyph;
	union
		{
		Card32 size32;
#define SHARED_DATA	(1<<31)	/* Flags single shared metrics data */
		Card16 size16;
		} offset;
	} Segment4;
#define SEGMENT4_OFFSET32_SIZE (SIZEOF(Segment4, lastGlyph) + \
								SIZEOF(Segment4, firstGlyph) + \
								SIZEOF(Segment4, offset.size32))

#define SEGMENT4_OFFSET16_SIZE (SIZEOF(Segment4, lastGlyph) + \
								SIZEOF(Segment4, firstGlyph) + \
								SIZEOF(Segment4, offset.size16))

typedef struct
	{
	Card16 format;
	BinSearchHdr search;
	Segment4 *segment;
	Card16 *data;
	} Lookup4;

typedef struct
	{
	GlyphId glyphId;
	Card16 *value;
	} Segment6;
#define SEGMENT6_HDR_SIZE (SIZEOF(Segment6, glyphId))
#define SEGMENT6_SIZE (SEGMENT6_HDR_SIZE + \
					   SIZEOF(Segment6, value[0]))

typedef struct
	{
	Card16 format;
	BinSearchHdr search;
	Segment6 *segment;
	Card16 *data;		/* Only used by opbd */
	} Lookup6;

typedef struct
	{
	Card16 format;
	GlyphId firstGlyph;
	Card16 nGlyphs;
	Card16 *value;
	} Lookup8;

#endif /* FORMAT_LOOKUP_H */
