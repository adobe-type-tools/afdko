/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Glyph data table format definition.
 */

#ifndef FORMAT_GLYF_H
#define FORMAT_GLYF_H

typedef struct
{
    uint16_t *endPtsOfContours;
    uint16_t instructionLength;
    uint8_t *instructions;
    uint8_t *flags;
#define ONCURVE        (1 << 0)
#define XSHORT         (1 << 1)
#define YSHORT         (1 << 2)
#define REPEAT         (1 << 3)
#define SHORT_X_IS_POS (1 << 4)
#define NEXT_X_IS_ZERO (1 << 4)
#define SHORT_Y_IS_POS (1 << 5)
#define NEXT_Y_IS_ZERO (1 << 5)
    int16_t *xCoordinates;
    int16_t *yCoordinates;
} Simple;

typedef struct
{
    uint16_t flags;
#define ARG_1_AND_2_ARE_WORDS    (1 << 0)
#define ARGS_ARE_XY_VALUES       (1 << 1)
#define ROUND_XY_TO_GRID         (1 << 2)
#define WE_HAVE_A_SCALE          (1 << 3)
#define NON_OVERLAPPING          (1 << 4) /* Apparently obsolete */
#define MORE_COMPONENTS          (1 << 5)
#define WE_HAVE_AN_X_AND_Y_SCALE (1 << 6)
#define WE_HAVE_A_TWO_BY_TWO     (1 << 7)
#define WE_HAVE_INSTRUCTIONS     (1 << 8)
#define USE_MY_METRICS           (1 << 9)
    uint16_t glyphIndex;
    int16_t arg1;
    int16_t arg2;
    F2Dot14 transform[2][2];
    uint16_t instructionLength;
    uint8_t *instructions;
} Component;

typedef struct
{
    Component *component;
} Compound;

typedef struct
{
    int16_t numberOfContours;
    FWord xMin;
    FWord yMin;
    FWord xMax;
    FWord yMax;
    void *data;
} Glyph;

typedef struct
{
    Glyph *glyph;
} glyfTbl;

#endif /* FORMAT_GLYF_H */
