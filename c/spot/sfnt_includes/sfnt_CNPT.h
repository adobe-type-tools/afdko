/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CNPT.h	1.1
 * Changed:    4/13/95 10:15:15
 ***********************************************************************/

/*
 * Control point table format definition.
 */

#ifndef FORMAT_CNPT_H
#define FORMAT_CNPT_H

#define CNPT_VERSION VERSION(1,0)
	
typedef struct
	{
	Card16 firstPoint;
	Card16 nPoints;
	} Element0;
#define ELEMENT0_SIZE (SIZEOF(Element0, firstPoint) + \
					   SIZEOF(Element0, nPoints))

typedef struct
	{
	Element0 *index;
	} Format0;

typedef struct
	{
	GlyphId glyphId;
	Card16 firstPoint;
	Card16 nPoints;
	} Element1;
#define ELEMENT1_SIZE (SIZEOF(Element1, glyphId) + \
					   SIZEOF(Element1, firstPoint) + \
					   SIZEOF(Element1, nPoints))

typedef struct
	{
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Element1 *index;
	} Format1;
#define FORMAT1_SIZE (SIZEOF(Format1, searchRange) + \
					  SIZEOF(Format1, entrySelector) + \
					  SIZEOF(Format1, rangeShift))

typedef struct
	{
	Fixed version;
	Card16 format;
	Card16 flags;
	Card32 indexLength;
	GlyphId firstGlyph;
	Card16 nElements;
	void *formatSpecific;
	struct
		{
		Card32 cnt;
		CNPTPoint *point;
		} points;
	} CNPTTbl;

enum
	{
	CNPT_DENSE_FORMAT,
	CNPT_SPARSE_FORMAT
	};

#endif /* FORMAT_CNPT_H */
