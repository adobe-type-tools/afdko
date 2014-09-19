/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sbit.h	1.2
 * Changed:    8/11/95 15:04:11
 ***********************************************************************/

#ifndef FORMAT_SBIT_H
#define FORMAT_SBIT_H

#define SBIT_VERSION VERSION(1,0)


typedef struct _sbitLineMetrics
	{
	  Int8 ascender;
	  Int8 descender;
	  Card8 widthMax;
	  Int8 caretSlopeNumerator;
	  Int8 caretSlopeDenominator;
	  Int8 caretOffset;
	  Int8 minOriginSB;  /* min of horiBearingX 
				(vertBearingY) */
	  Int8 minAdvanceSB; /* min of horiAdvance - horiBearingX + width
				(vertAdvance - vertBearing + height) */
	  Int8 maxBeforeBL;  /* max of horiBearingX
				(vertBearingY) */
	  Int8 minAfterBL;   /* min of horiBearingY - height
				(vertBearingX - width) */
	  Int16 padding;    /* long-word align */
	} sbitLineMetrics;

#define SBITLINEMETRICS_SIZE (SIZEOF(sbitLineMetrics, ascender) + \
			      SIZEOF(sbitLineMetrics, descender) + \
			      SIZEOF(sbitLineMetrics, widthMax) + \
			      SIZEOF(sbitLineMetrics, caretSlopeNumerator) + \
			      SIZEOF(sbitLineMetrics, caretSlopeDenominator) + \
			      SIZEOF(sbitLineMetrics, caretOffset) + \
			      SIZEOF(sbitLineMetrics, minOriginSB) + \
			      SIZEOF(sbitLineMetrics, minAdvanceSB) + \
			      SIZEOF(sbitLineMetrics, maxBeforeBL)+ \
			      SIZEOF(sbitLineMetrics, minAfterBL) + \
			      SIZEOF(sbitLineMetrics, padding))

/* ....................................................... */
typedef enum 
	{
	  SBITIndexUnknown=0,
	  SBITIndexProportional = 1,
	  SBITIndexMono,
	  SBITIndexProportionalSmall,
	  SBITIndexProportionalSparse,
	  SBITIndexMonoSparse
	} sbitBitmapIndexFormats;

typedef enum 
	{
	  SBITUnknown=0,
	  SBITProportionalSmallByteFormat = 1,  /* small metrics & data, byte-aligned */
	  SBITProportionalSmallBitFormat,	/* small metrics & data, bit-aligned */
	  SBITProportionalCompressedFormat, /* not used. metric info followd by compressed data */
	  SBITMonoCompressedFormat,	/* just compressed data. Metrics in "bloc" */
	  SBITMonoFormat,		/* just bit-aligned data. Metrics in "bloc" */
	  SBITProportionalBigByteFormat,	/* big metrics & byte-aligned data */
	  SBITProportionalBigBitFormat,	/* big metrics & bit-aligned data */
	  SBITComponentSmallFormat,		/* small metrics, component data */
	  SBITComponentBigFormat		/* big metrics, component data */
	} sbitBitmapDataFormats;


/* ....................GLYPH METRICS................................... */
typedef struct _sbitBigGlyphMetrics
	{
	  Card8 height;         
	  Card8 width;          
	  Int8  horiBearingX;   
	  Int8  horiBearingY;   
	  Card8 horiAdvance;
	  Int8 vertBearingX;   
	  Int8 vertBearingY;   
	  Card8 vertAdvance;
	} sbitBigGlyphMetrics;

#define SBITBIGGLYPHMETRICS_SIZE (SIZEOF(sbitBigGlyphMetrics, height) + \
				  SIZEOF(sbitBigGlyphMetrics, width) + \
				  SIZEOF(sbitBigGlyphMetrics, horiBearingX) + \
				  SIZEOF(sbitBigGlyphMetrics, horiBearingY) + \
				  SIZEOF(sbitBigGlyphMetrics, horiAdvance) + \
				  SIZEOF(sbitBigGlyphMetrics, vertBearingX) + \
				  SIZEOF(sbitBigGlyphMetrics, vertBearingY) + \
				  SIZEOF(sbitBigGlyphMetrics, vertAdvance))


typedef struct _sbitSmallGlyphMetrics
	{
	  Card8 height;
	  Card8 width;
	  Int8  bearingX;
	  Int8  bearingY;
	  Card8 advance;
	} sbitSmallGlyphMetrics;

#define SBITSMALLGLYPHMETRICS_SIZE (SIZEOF(sbitSmallGlyphMetrics, height) + \
				    SIZEOF(sbitSmallGlyphMetrics, width) + \
				    SIZEOF(sbitSmallGlyphMetrics, bearingX) + \
				    SIZEOF(sbitSmallGlyphMetrics, bearingY) + \
				    SIZEOF(sbitSmallGlyphMetrics, advance))

#endif /* FORMAT_SBIT_H */




