/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TrueType Open baseline table format definition.
 */

#ifndef FORMAT_BASE_H
#define FORMAT_BASE_H

#include "tto.h"

typedef struct
{
    uint16_t BaseTagCount;
    Tag *BaselineTag; /* [BaseTagCount] */
} BaseTagList;

typedef struct
{
    uint16_t BaseCoordFormat; /* =1 */
    int16_t Coordinate;
} BaseCoordFormat1;

typedef struct
{
    uint16_t BaseCoordFormat; /* =2 */
    int16_t Coordinate;
    GlyphId ReferenceGlyph;
    uint16_t BaseCoordPoint;
} BaseCoordFormat2;

typedef struct
{
    uint16_t BaseCoordFormat; /* =3 */
    int16_t Coordinate;
    OFFSET(DeviceTable, DeviceTable);
} BaseCoordFormat3;

typedef struct
{
    uint16_t BaseCoordFormat; /* =4 */
    uint16_t IdBaseCoord;
} BaseCoordFormat4;

typedef struct
{
    uint16_t DefaultIndex;
    uint16_t BaseCoordCount;
    OFFSET_ARRAY(void *, BaseCoord); /* [BaseCoordCount] */
} BaseValues;

typedef struct
{
    Tag FeatureTableTag;
    OFFSET(void *, MinCoord); /* From MinMax */
    OFFSET(void *, MaxCoord); /* From MinMax */
} FeatMinMaxRecord;

typedef struct
{
    OFFSET(void *, MinCoord);
    OFFSET(void *, MaxCoord);
    uint16_t FeatMinMaxCount;
    FeatMinMaxRecord *FeatMinMaxRecord; /* [FeatMinMaxCount] */
} MinMax;

typedef struct
{
    Tag BaseLangSysTag;
    OFFSET(MinMax, MinMax); /* From BaseScript */
} BaseLangSysRecord;

typedef struct
{
    OFFSET(BaseValues, BaseValues);
    OFFSET(MinMax, DefaultMinMax);
    uint16_t BaseLangSysCount;
    BaseLangSysRecord *BaseLangSysRecord; /* [BaseLangSysCount] */
} BaseScript;

typedef struct
{
    Tag BaseScriptTag;
    OFFSET(BaseScript, BaseScript); /* From BaseScriptList */
} BaseScriptRecord;

typedef struct
{
    uint16_t BaseScriptCount;
    BaseScriptRecord *BaseScriptRecord; /* [BaseScriptCount] */
} BaseScriptList;

typedef struct
{
    OFFSET(BaseTagList, BaseTagList);
    OFFSET(BaseScriptList, BaseScriptList);
} Axis;

typedef struct
{
    Fixed Version;
    OFFSET(Axis, HorizAxis);
    OFFSET(Axis, VertAxis);
} BASETbl;

#endif /* FORMAT_BASE_H */
