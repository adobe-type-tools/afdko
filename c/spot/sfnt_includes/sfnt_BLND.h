/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Blend table format definition. [ADS Technical Note #5087]
 */

#ifndef FORMAT_BLND_H
#define FORMAT_BLND_H

#define BLND_VERSION 3 /* Support for design to weight vector conversion */

typedef struct
{
    uint16_t designCoord;
    Fixed normalizedValue;
} Map;
#define MAP_SIZE (SIZEOF(Map, designCoord) + \
                  SIZEOF(Map, normalizedValue))

typedef struct
{
    uint16_t flags;
#define FLAG_MAP (1 << 0) /* Axis has map information */
    uint16_t minRange;
    uint16_t maxRange;
    uint8_t *type;
    uint8_t *longLabel;
    uint8_t *shortLabel;
    uint16_t nMaps;
    Map *map;
} AxisInfo;
#define AXIS_INFO_HDR_SIZE (SIZEOF(AxisInfo, flags) +    \
                            SIZEOF(AxisInfo, minRange) + \
                            SIZEOF(AxisInfo, maxRange))

typedef struct
{
    Fixed start;
    Fixed delta;
} Delta;
#define DELTA_SIZE (SIZEOF(Delta, start) + \
                    SIZEOF(Delta, delta))

typedef struct
{
    uint8_t code;
    uint8_t flags; /* Unused */
    uint16_t axis;
    uint16_t nDeltas;
    Delta *delta;
} SPOT_Style;
#define STYLE_HDR_SIZE (SIZEOF(Style, code) +  \
                        SIZEOF(Style, flags) + \
                        SIZEOF(Style, axis) +  \
                        SIZEOF(Style, nDeltas))

typedef struct
{
    uint16_t *coord;
    uint32_t offset;
    int16_t FONDId;
    int16_t NFNTId;
} Instance;

/* Design to weight vector conversion */
typedef struct
{
    uint16_t CDVNum;
    uint16_t CDVLength;
    uint16_t NDVNum;
    uint16_t NDVLength;
    uint16_t lenBuildCharArray;
    uint8_t *CDVSubr;
    uint8_t *NDVSubr;
} D2WV;

typedef struct
{
    uint16_t version;
    uint16_t flags;
#define FLAG_CHAMELEON (1 << 0) /* Font is chameleon */
    uint16_t nAxes;
    uint16_t nMasters;
    uint16_t languageId; /* Unused */
    uint16_t iRegular;
    uint16_t nOffsets;
    uint32_t *axisOffset;
    uint32_t masterNameOffset;
    uint32_t styleOffset;
    uint32_t instanceOffset;
    uint32_t instanceNameOffset;
    uint32_t d2wvOffset; /* != 0 if design to weight vector code in font */
    AxisInfo *axisInfo;
    DCL_ARRAY(uint8_t, masterNames);
    uint16_t nStyles;
    SPOT_Style *style;
    uint16_t nInstances;
    Instance *instance;
    DCL_ARRAY(uint8_t, instanceNames);
    D2WV d2wv;
} BLNDTbl;

#endif /* FORMAT_BLND_H */
