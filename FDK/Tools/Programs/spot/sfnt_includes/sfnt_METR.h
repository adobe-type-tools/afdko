/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)METR.h	1.1
 * Changed:    10/24/95 18:31:08
 ***********************************************************************/

/*
 * Metric table format definition.
 */

#ifndef FORMAT_METR_H
#define FORMAT_METR_H

#define METR_VERSION VERSION(1,0)

/* Per-glyph data. There is 1 width per master design in blend resource order.
 * The formula for the number of extrapolation limits is (2^n * n) yielding:
 * 
 * Axes  #limits
 *   1     	2
 *   2      8
 *   3     24
 *   4     64
 */
typedef struct
	{
	uFWord *width;	/* [nMasters] */
	Int16 *extrap;	/* (6.10 fixed) [nMasters] */
	} GlyphInfo;

/* Per-master design data */
typedef struct		
	{
	FWord capHeight;
	FWord xHeight;
	FWord stemV;
	struct
		{
		FWord left;
		FWord top;
	    FWord right;
		FWord bottom;
		} bbox;
	} MasterInfo;

typedef struct
	{
	Fixed version;
	Card16 flags;
	Byte8 fontName[34];	/* FontName (Pascal string) */
	Card16 nGlyphs;
	Card16 nMasters;
	FWord underlinePosition;
	FWord underlineThickness;
	GlyphInfo *glyph; 	/* [nGlyphs] */
	MasterInfo *master;	/* [nMasters] */
	} METRTbl;

#endif /* FORMAT_METR_H */
