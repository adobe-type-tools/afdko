/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)EBDT.h	1.2
 * Changed:    7/1/98 10:54:56
 ***********************************************************************/

#ifndef FORMAT_EBDT_H
#define FORMAT_EBDT_H

#define EBDT_VERSION VERSION(1,0)

#include "sfnt_sbit.h"

typedef da_DCL(Card8, StrikeType);

typedef StrikeType *StrikePtr;

/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in EBLCIndexSubTable_Format1
*/
typedef struct _EBDTGlyphBitmap_Format1
	{
	  GlyphId _gid;
	  sbitSmallGlyphMetrics   Metrics;
	  StrikeType data;  /* bitmap image data: byte-aligned */
	} EBDTGlyphBitmap_Format1;


/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in EBLCIndexSubTable_Format1
*/
typedef struct _EBDTGlyphBitmap_Format2
	{
	  GlyphId _gid;
	  sbitSmallGlyphMetrics   Metrics;
	  StrikeType data; 	  /* bitmap image data: bit-aligned */
	} EBDTGlyphBitmap_Format2;


typedef struct _EBDTGlyphBitmap_Format5  /* PREFERRED format */
	{
	  /* Mono Metrics are in "EBLC" portion */
	  GlyphId _startgid, _endgid;
	  Card32  _imagesize; /* size in bytes */
	  Card16  _resY;  /* bits in em square */
	  Card16  _gridsq; /* actual grid square ( sometimes less than _resY) */
	  StrikeType data; /* bit-aligned bitmap data, padded to byte boundary */
	}EBDTGlyphBitmap_Format5;

typedef struct _EBDTGlyphBitmap_Format6
	{
	  GlyphId _gid;
	  sbitBigGlyphMetrics   Metrics;
	  StrikeType data;  /* bitmap image data: byte-aligned */
	} EBDTGlyphBitmap_Format6;

typedef struct _EBDTGlyphBitmap_Format7
	{
	  GlyphId _gid;
	  sbitBigGlyphMetrics   Metrics;
	  StrikeType data; 	  /* bitmap image data: bit-aligned */
	} EBDTGlyphBitmap_Format7;

typedef struct _EBDTbdtComponent 
	{
	  GlyphId glyphCode;
	  Int8  xOffset;
	  Int8  yOffset;
	} EBDTbdtComponent;

/* for composite bitmaps */
typedef struct _EBDTGlyphBitmap_Format8
	{
	  GlyphId _gid;
	  sbitSmallGlyphMetrics   Metrics;
	  Card8 pad;
	  Card16  numComponents;
	  DCL_ARRAY(EBDTbdtComponent, componentArray);
	} EBDTGlyphBitmap_Format8;

typedef struct _EBDTGlyphBitmap_Format9
	{
	  GlyphId _gid;
	  sbitBigGlyphMetrics   Metrics;
	  Card16  numComponents;
	  DCL_ARRAY(EBDTbdtComponent, componentArray);
	} EBDTGlyphBitmap_Format9;



typedef union _EBDTFormat
	{
	  da_DCL(EBDTGlyphBitmap_Format1 *, f1);
	  da_DCL(EBDTGlyphBitmap_Format2 *, f2);
	  EBDTGlyphBitmap_Format5 *         f5;
	  da_DCL(EBDTGlyphBitmap_Format6 *, f6);
	  da_DCL(EBDTGlyphBitmap_Format7 *, f7);
	  da_DCL(EBDTGlyphBitmap_Format8 *, f8);
	  da_DCL(EBDTGlyphBitmap_Format9 *, f9);
	} EBDTFormat;

typedef struct _EBDTFormats /* internal: for construction only */
	{
	  Card32 _fileoffsetfromstart;
	  Card32 _bytelen;
	  sbitBitmapDataFormats _dfmttype;
	  Card8 _index;
	  Card32 _num; /* number of entries in the _fmt */
	  EBDTFormat _fmt;
	  /* Refer to this EBDT's 'EBLCFormats' so that we can update the 'imageDataOffset' entry when we write out this EBDT */
	  Card16 _mainEBLCsubtablearrayindex; /* index into the Main EBLC SubtableArray */
	  Card16 _subtablearrayindex; /*  index into _that_ array */
	} EBDTFormats;

typedef struct _EBDTHeaderRecord
	{
#define EBDT_HEADER_VERSION             0x00020000
	  Fixed version;
	  Card16 _numentries;
	  DCL_ARRAY(EBDTFormats, _formatlist);
	} EBDTHeaderRecord;

#endif /* FORMAT_EBDT_H */
