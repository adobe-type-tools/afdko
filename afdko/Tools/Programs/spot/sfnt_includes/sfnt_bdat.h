/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)bdat.h	1.4
 * Changed:    11/30/95 16:38:24
 ***********************************************************************/

#ifndef FORMAT_BDAT_H
#define FORMAT_BDAT_H

#define BDAT_VERSION VERSION(1,0)

#include "sfnt_sbit.h"

typedef da_DCL(Card8, StrikeType);

typedef StrikeType *StrikePtr;

/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in blocIndexSubTable_Format1
*/
typedef struct _bdatGlyphBitmap_Format1
	{
	  GlyphId _gid;
	  sbitSmallGlyphMetrics   Metrics;
	  StrikeType data;  /* bitmap image data: byte-aligned */
	} bdatGlyphBitmap_Format1;


/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in blocIndexSubTable_Format1
*/
typedef struct _bdatGlyphBitmap_Format2
	{
	  GlyphId _gid;
	  sbitSmallGlyphMetrics   Metrics;
	  StrikeType data; 	  /* bitmap image data: bit-aligned */
	} bdatGlyphBitmap_Format2;


#if 0
typedef struct _bdatGlyphBitmap_Format4 /* not used by Adobe */
	{
	  Card32  whiteTreeOffset;
	  Card32  blackTreeOffset;
	  Card32  glyphDataOffset;
	  Card8  *whiteTree;
	  Card8  *blackTree;;
	  Card8  *glyphdata;
	} bdatGlyphBitmap_Format4;
#endif

typedef struct _bdatGlyphBitmap_Format5  /* PREFERRED format */
	{
	  /* Mono Metrics are in "bloc" portion */
	  GlyphId _startgid, _endgid;
	  Card32  _imagesize; /* size in bytes */
	  Card16  _resY;  /* bits in em square */
	  Card16  _gridsq; /* actual grid square ( sometimes less than _resY) */
	  StrikeType data; /* bit-aligned bitmap data, padded to byte boundary */
	}bdatGlyphBitmap_Format5;

typedef struct _bdatGlyphBitmap_Format6
	{
	  GlyphId _gid;
	  sbitBigGlyphMetrics   Metrics;
	  StrikeType data;  /* bitmap image data: byte-aligned */
	} bdatGlyphBitmap_Format6;

typedef struct _bdatGlyphBitmap_Format7
	{
	  GlyphId _gid;
	  sbitBigGlyphMetrics   Metrics;
	  StrikeType data; 	  /* bitmap image data: bit-aligned */
	} bdatGlyphBitmap_Format7;


typedef union _bdatFormat
	{
	  da_DCL(bdatGlyphBitmap_Format1 *, f1);
	  da_DCL(bdatGlyphBitmap_Format2 *, f2);
	  bdatGlyphBitmap_Format5  *f5;
	  da_DCL(bdatGlyphBitmap_Format6 *, f6);
	  da_DCL(bdatGlyphBitmap_Format7 *, f7);
	} bdatFormat;

typedef struct _bdatFormats /* internal: for construction only */
	{
	  Card32 _fileoffsetfromstart;
	  Card32 _bytelen;
	  sbitBitmapDataFormats _dfmttype;
	  Card8 _index;
	  Card32 _num; /* number of entries in the _fmt */
	  bdatFormat _fmt;
	  /* Refer to this bdat's 'blocFormats' so that we can update the 'imageDataOffset' entry when we write out this bdat */
	  Card16 _mainblocsubtablearrayindex; /* index into the Main bloc SubtableArray */
	  Card16 _subtablearrayindex; /*  index into _that_ array */
	} bdatFormats;

typedef struct _bdatHeaderRecord
	{
#define BDAT_HEADER_VERSION             0x00020000
	  Fixed version;
	  Card16 _numentries;
	  DCL_ARRAY(bdatFormats, _formatlist);
	} bdatHeaderRecord;

#endif /* FORMAT_BDAT_H */
