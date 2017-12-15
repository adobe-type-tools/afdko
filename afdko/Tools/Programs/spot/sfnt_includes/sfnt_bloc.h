/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)bloc.h	1.3
 * Changed:    10/12/95 15:02:14
 ***********************************************************************/


#ifndef FORMAT_BLOC_H
#define FORMAT_BLOC_H

#define BLOC_VERSION VERSION(1,0)

#include "sfnt_sbit.h"


typedef struct _blocBitmapSizeTable
	{
	  Card32 	indexSubTableArrayOffset; /* offset to corresp. index
						     subtable array from beginning
						     of "bloc" */
	  Card32 	indexTableSize;   /* length of corresp. index subtables and array */
	  Card32 	numberofIndexSubTables; /* number of index subtables.
						   There is a subtable for each range or format change */
	  Card32 	colorRef;         /* set to 0. ignore for now */
	  sbitLineMetrics  hori;
	  sbitLineMetrics  vert;
	  GlyphId 	startGlyphIndex;
	  GlyphId 	endGlyphIndex;
	  Card8  	ppemX;   /* target horizontal pixels/em */
	  Card8  	ppemY; 
	  Card8  	bitDepth;
	  Int8  	flags;
#define	  BLOC_FLAG_HORIZONTAL  (1<<0)
#define	  BLOC_FLAG_VERTICAL    (1<<1)
	  Card8  _index;
	} blocBitmapSizeTable;

#define BLOCBITMAPSIZETABLE_SIZE (SIZEOF(blocBitmapSizeTable, indexSubTableArrayOffset) + \
				  SIZEOF(blocBitmapSizeTable, indexTableSize) + \
				  SIZEOF(blocBitmapSizeTable, numberofIndexSubTables) + \
				  SIZEOF(blocBitmapSizeTable, colorRef) + \
				  SBITLINEMETRICS_SIZE + \
				  SBITLINEMETRICS_SIZE + \
				  SIZEOF(blocBitmapSizeTable, startGlyphIndex) + \
				  SIZEOF(blocBitmapSizeTable, endGlyphIndex) + \
				  SIZEOF(blocBitmapSizeTable, ppemX) + \
				  SIZEOF(blocBitmapSizeTable, ppemY) + \
				  SIZEOF(blocBitmapSizeTable, bitDepth) + \
				  SIZEOF(blocBitmapSizeTable, flags))



/* ...................BLOC SUBTABLES.................................... */

typedef struct _blocIndexSubHeader
	{
	  Card16  	indexFormat; /* sbitBitmapIndexFormats */
	  Card16  	imageFormat; /* sbitBitmapDataFormats  */
	  Card32 	imageDataOffset;   /*offset to corresponding image data from beginning of bdat table*/
	} blocIndexSubHeader;

#define BLOCINDEXSUBHEADER_SIZE (SIZEOF(blocIndexSubHeader, indexFormat) + \
				 SIZEOF(blocIndexSubHeader, imageFormat) + \
				 SIZEOF(blocIndexSubHeader, imageDataOffset))


/* format 1 has variable-length images of the same format for
   uncompressed PROPORTIONALLY-spaced glyphs */
typedef struct _blocIndexSubtable_Format1
	{
	  blocIndexSubHeader header;
	  Card32 _numoffsets;
	  DCL_ARRAY(Card32, offsetArray);  /* offsetArray[glyphIndex] + imageDataOffset ==> start of bitmapdata for glyph.
					      sizeOfArray = lastGlyph - firstGlyph + 1 */
	} blocIndexSubtable_Format1;

#define BLOCFORMAT1_SIZE(first, last)\
	(SIZEOF(blocIndexSubtable_Format1, header) + \
	 sizeof(Card32) * ((last) - (first) + 1))



/* format 2 has fixed-length images of the same format for
   MONO-spaced glyphs */
typedef struct _blocIndexSubtable_Format2
	{
	  blocIndexSubHeader 	header;
	  Card32  		imageSize; /* images may be compressed, bit-aligned or byte-aligned */
	  sbitBigGlyphMetrics 	Metrics; /* all glyphs share same metrics */
	} blocIndexSubtable_Format2;

#define BLOCFORMAT2_SIZE  (SIZEOF(blocIndexSubtable_Format2, header) + \
			   SIZEOF(blocIndexSubtable_Format2, imageSize) + \
			   SBITBIGGLYPHMETRICS_SIZE)



/* format 3 is similar to format 1, but with 16-bit offsets, for
   compressed PROPORTIONALLY-spaced glyphs. 
   Must be padded to a long-word boundary. */
typedef struct _blocIndexSubtable_Format3
	{
	  blocIndexSubHeader header;
	  Card32 _numoffsets;
	  DCL_ARRAY(Card16, offsetArray);
	} blocIndexSubtable_Format3;

#define BLOCFORMAT3_SIZE(first, last)\
	(SIZEOF(blocIndexSubtable_Format1, header) + \
	 sizeof(Card16) * ((last) - (first) + 1))



typedef struct _blocCodeOffsetPair
	{
	  GlyphId glyphCode;            /* code of glyph present */
	  Card16  offset;               /* location in "bdat" */
	} blocCodeOffsetPair;
#define BLOCCODEOFFSETPAIR_SIZE (SIZEOF(blocCodeOffsetPair, glyphCode) +\
				 SIZEOF(blocCodeOffsetPair, offset))

/* format 4 is for sparsely-embedded glyph data for 
   PROPORTIONAL metrics */
typedef struct _blocIndexSubtable_Format4
	{
	  blocIndexSubHeader 	header;
	  Card32 		numGlyphs;
	  DCL_ARRAY(blocCodeOffsetPair, glyphArray); /* one per glyph */
	} blocIndexSubtable_Format4;

#define BLOCFORMAT4_SIZE(numglyphs) \
	(SIZEOF(blocIndexSubtable_Format4, header) + \
	 SIZEOF(blocIndexSubtable_Format4, numGlyphs) + \
	 (BLOCCODEOFFSETPAIR_SIZE) * (numglyphs))



/* format 5 is for sparsely-embedded glyph data of
   fixed-sized, MONO-spaced metrics */
typedef struct _blocIndexSubtable_Format5
	{
	  blocIndexSubHeader 	header;
	  Card32 		imageSize;
	  sbitBigGlyphMetrics 	Metrics;
	  Card32 		numGlyphs;
	  DCL_ARRAY(blocCodeOffsetPair, glyphArray); /* one per glyph */
	} blocIndexSubtable_Format5;

#define BLOCFORMAT5_SIZE(numglyphs) \
	(SIZEOF(blocIndexSubtable_Format5, header) + \
	 SIZEOF(blocIndexSubtable_Format5, imageSize) + \
	 SBITBIGGLYPHMETRICS_SIZE + \
	 SIZEOF(blocIndexSubtable_Format5, numGlyphs) + \
	  (BLOCCODEOFFSETPAIR_SIZE) * (numglyphs))




/* ...................BLOC TABLE.................................... */

typedef union _blocFormat
	{
	  blocIndexSubtable_Format1  *f1;
	  blocIndexSubtable_Format2  *f2;
	  blocIndexSubtable_Format3  *f3;
	  blocIndexSubtable_Format4  *f4;
	  blocIndexSubtable_Format5  *f5;
	} blocFormat;

typedef struct _blocFormats /* internal: for construction only */
	{
	  Card32 _bytelen;
	  sbitBitmapIndexFormats _fmttype;
	  Card16 _index;
	  blocFormat _fmt;
	} blocFormats;



typedef struct _blocIndexSubTableArrayElt
	{
          GlyphId       firstGlyphIndex;
          GlyphId       lastGlyphIndex;
          Card32        additionalOffsetToIndexSubtable; /* add to indexSubTableArrayOffset to get offset from beginning of "bloc" table */
          blocFormats   _subtable;
	  Card16        _index; /* index in subtablearray */
	} blocIndexSubTableArrayElt;

#define BLOCINDEXSUBTABLEARRAYELT_SIZE (SIZEOF(blocIndexSubTableArrayElt, firstGlyphIndex) + \
					SIZEOF(blocIndexSubTableArrayElt, lastGlyphIndex) + \
					SIZEOF(blocIndexSubTableArrayElt, additionalOffsetToIndexSubtable))

typedef struct _blocIndexSubTableArray
        {
	  Card16 _numarrayelts;
	  DCL_ARRAY(blocIndexSubTableArrayElt, _elts);
	  Card16 _index; /* index in Main bloc SubTableArray */
        } blocIndexSubTableArray;


typedef struct _blocTableHeader
	{
#define BLOC_HEADER_VERSION           0x00020000 
	  Fixed 	version;
	  Card32 	numSizes;
	  DCL_ARRAY(blocBitmapSizeTable, bitmapSizeTable); /* array [numSizes] */
	  /* for construction:     array[numSizes].
	     one-to-one correspondence with bitmapSizeTable  */
	  DCL_ARRAY(blocIndexSubTableArray, _indexSubTableArray);
	} blocTableHeader;

#define BLOCTABLEHEADER_SIZE(numsizes) \
	(SIZEOF(blocTableHeader, version) + \
	 SIZEOF(blocTableHeader, numSizes) + \
	 BLOCBITMAPSIZETABLE_SIZE * (numsizes))



#endif /* FORMAT_BLOC_H */
