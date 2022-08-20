/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef FORMAT_BDAT_H
#define FORMAT_BDAT_H

#define BDAT_VERSION VERSION(1, 0)

#include "sfnt_sbit.h"

typedef da_DCL(uint8_t, StrikeType);

typedef StrikeType *StrikePtr;

/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in blocIndexSubTable_Format1
*/
typedef struct _bdatGlyphBitmap_Format1 {
    GlyphId _gid;
    sbitSmallGlyphMetrics Metrics;
    StrikeType data; /* bitmap image data: byte-aligned */
} bdatGlyphBitmap_Format1;

/* Similar to "fbit-7" image-data format, padded to byte boundary.
   All metric info is in blocIndexSubTable_Format1
*/
typedef struct _bdatGlyphBitmap_Format2 {
    GlyphId _gid;
    sbitSmallGlyphMetrics Metrics;
    StrikeType data; /* bitmap image data: bit-aligned */
} bdatGlyphBitmap_Format2;

#if 0
typedef struct _bdatGlyphBitmap_Format4 /* not used by Adobe */
{
    uint32_t whiteTreeOffset;
    uint32_t blackTreeOffset;
    uint32_t glyphDataOffset;
    uint8_t *whiteTree;
    uint8_t *blackTree;
    ;
    uint8_t *glyphdata;
} bdatGlyphBitmap_Format4;
#endif

typedef struct _bdatGlyphBitmap_Format5 /* PREFERRED format */
{
    /* Mono Metrics are in "bloc" portion */
    GlyphId _startgid, _endgid;
    uint32_t _imagesize; /* size in bytes */
    uint16_t _resY;      /* bits in em square */
    uint16_t _gridsq;    /* actual grid square ( sometimes less than _resY) */
    StrikeType data;   /* bit-aligned bitmap data, padded to byte boundary */
} bdatGlyphBitmap_Format5;

typedef struct _bdatGlyphBitmap_Format6 {
    GlyphId _gid;
    sbitBigGlyphMetrics Metrics;
    StrikeType data; /* bitmap image data: byte-aligned */
} bdatGlyphBitmap_Format6;

typedef struct _bdatGlyphBitmap_Format7 {
    GlyphId _gid;
    sbitBigGlyphMetrics Metrics;
    StrikeType data; /* bitmap image data: bit-aligned */
} bdatGlyphBitmap_Format7;

typedef union _bdatFormat {
    da_DCL(bdatGlyphBitmap_Format1 *, f1);
    da_DCL(bdatGlyphBitmap_Format2 *, f2);
    bdatGlyphBitmap_Format5 *f5;
    da_DCL(bdatGlyphBitmap_Format6 *, f6);
    da_DCL(bdatGlyphBitmap_Format7 *, f7);
} bdatFormat;

typedef struct _bdatFormats /* internal: for construction only */
{
    uint32_t _fileoffsetfromstart;
    uint32_t _bytelen;
    sbitBitmapDataFormats _dfmttype;
    uint8_t _index;
    uint32_t _num; /* number of entries in the _fmt */
    bdatFormat _fmt;
    /* Refer to this bdat's 'blocFormats' so that we can update the 'imageDataOffset' entry when we write out this bdat */
    uint16_t _mainblocsubtablearrayindex; /* index into the Main bloc SubtableArray */
    uint16_t _subtablearrayindex;         /*  index into _that_ array */
} bdatFormats;

typedef struct _bdatHeaderRecord {
#define BDAT_HEADER_VERSION 0x00020000
    Fixed version;
    uint16_t _numentries;
    DCL_ARRAY(bdatFormats, _formatlist);
} bdatHeaderRecord;

#endif /* FORMAT_BDAT_H */
