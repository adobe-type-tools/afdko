/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef FORMAT_SBIT_H
#define FORMAT_SBIT_H

#define SBIT_VERSION VERSION(1, 0)

typedef struct _sbitLineMetrics {
    int8_t ascender;
    int8_t descender;
    uint8_t widthMax;
    int8_t caretSlopeNumerator;
    int8_t caretSlopeDenominator;
    int8_t caretOffset;
    int8_t minOriginSB;  /* min of horiBearingX  */
                       /*       (vertBearingY) */
    int8_t minAdvanceSB; /* min of horiAdvance - horiBearingX + width  */
                       /*       (vertAdvance - vertBearing + height) */
    int8_t maxBeforeBL;  /* max of horiBearingX  */
                       /*       (vertBearingY) */
    int8_t minAfterBL;   /* min of horiBearingY - height */
                       /*       (vertBearingX - width) */
    int16_t padding;     /* long-word align */
} sbitLineMetrics;

#define SBITLINEMETRICS_SIZE (SIZEOF(sbitLineMetrics, ascender) +              \
                              SIZEOF(sbitLineMetrics, descender) +             \
                              SIZEOF(sbitLineMetrics, widthMax) +              \
                              SIZEOF(sbitLineMetrics, caretSlopeNumerator) +   \
                              SIZEOF(sbitLineMetrics, caretSlopeDenominator) + \
                              SIZEOF(sbitLineMetrics, caretOffset) +           \
                              SIZEOF(sbitLineMetrics, minOriginSB) +           \
                              SIZEOF(sbitLineMetrics, minAdvanceSB) +          \
                              SIZEOF(sbitLineMetrics, maxBeforeBL) +           \
                              SIZEOF(sbitLineMetrics, minAfterBL) +            \
                              SIZEOF(sbitLineMetrics, padding))

/* ....................................................... */
typedef enum {
    SBITIndexUnknown = 0,
    SBITIndexProportional = 1,
    SBITIndexMono,
    SBITIndexProportionalSmall,
    SBITIndexProportionalSparse,
    SBITIndexMonoSparse
} sbitBitmapIndexFormats;

typedef enum {
    SBITUnknown = 0,
    SBITProportionalSmallByteFormat = 1, /* small metrics & data, byte-aligned */
    SBITProportionalSmallBitFormat,      /* small metrics & data, bit-aligned */
    SBITProportionalCompressedFormat,    /* not used. metric info followed by compressed data */
    SBITMonoCompressedFormat,            /* just compressed data. Metrics in "bloc" */
    SBITMonoFormat,                      /* just bit-aligned data. Metrics in "bloc" */
    SBITProportionalBigByteFormat,       /* big metrics & byte-aligned data */
    SBITProportionalBigBitFormat,        /* big metrics & bit-aligned data */
    SBITComponentSmallFormat,            /* small metrics, component data */
    SBITComponentBigFormat               /* big metrics, component data */
} sbitBitmapDataFormats;

/* ....................GLYPH METRICS................................... */
typedef struct _sbitBigGlyphMetrics {
    uint8_t height;
    uint8_t width;
    int8_t horiBearingX;
    int8_t horiBearingY;
    uint8_t horiAdvance;
    int8_t vertBearingX;
    int8_t vertBearingY;
    uint8_t vertAdvance;
} sbitBigGlyphMetrics;

#define SBITBIGGLYPHMETRICS_SIZE (SIZEOF(sbitBigGlyphMetrics, height) +       \
                                  SIZEOF(sbitBigGlyphMetrics, width) +        \
                                  SIZEOF(sbitBigGlyphMetrics, horiBearingX) + \
                                  SIZEOF(sbitBigGlyphMetrics, horiBearingY) + \
                                  SIZEOF(sbitBigGlyphMetrics, horiAdvance) +  \
                                  SIZEOF(sbitBigGlyphMetrics, vertBearingX) + \
                                  SIZEOF(sbitBigGlyphMetrics, vertBearingY) + \
                                  SIZEOF(sbitBigGlyphMetrics, vertAdvance))

typedef struct _sbitSmallGlyphMetrics {
    uint8_t height;
    uint8_t width;
    int8_t bearingX;
    int8_t bearingY;
    uint8_t advance;
} sbitSmallGlyphMetrics;

#define SBITSMALLGLYPHMETRICS_SIZE (SIZEOF(sbitSmallGlyphMetrics, height) +   \
                                    SIZEOF(sbitSmallGlyphMetrics, width) +    \
                                    SIZEOF(sbitSmallGlyphMetrics, bearingX) + \
                                    SIZEOF(sbitSmallGlyphMetrics, bearingY) + \
                                    SIZEOF(sbitSmallGlyphMetrics, advance))

#endif /* FORMAT_SBIT_H */
