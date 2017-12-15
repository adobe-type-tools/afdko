/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BASE.h	1.2
 * Changed:    3/18/98 16:43:19
 ***********************************************************************/

/*
 * TrueType Open baseline table format definition.
 */

#ifndef FORMAT_BASE_H
#define FORMAT_BASE_H

#include "tto.h"

typedef struct
	{
	Card16 BaseTagCount;
	Tag *BaselineTag;					/* [BaseTagCount] */
	} BaseTagList;

typedef struct
	{
	Card16 BaseCoordFormat;				/* =1 */
	Int16 Coordinate;
	} BaseCoordFormat1;

typedef struct
	{
	Card16 BaseCoordFormat;				/* =2 */
	Int16 Coordinate;
	GlyphId ReferenceGlyph;
	Card16 BaseCoordPoint;
	} BaseCoordFormat2;

typedef struct
	{
	Card16 BaseCoordFormat;				/* =3 */
	Int16 Coordinate;
	OFFSET(DeviceTable, DeviceTable);
	} BaseCoordFormat3;

typedef struct
	{
	Card16 BaseCoordFormat;				/* =4 */
	Card16 IdBaseCoord;
	} BaseCoordFormat4;

typedef struct
	{
	Card16 DefaultIndex;
	Card16 BaseCoordCount;
	OFFSET_ARRAY(void *, BaseCoord);	/* [BaseCoordCount] */
	} BaseValues;

typedef struct
	{
	Tag FeatureTableTag;
	OFFSET(void *, MinCoord);		   	/* From MinMax */
	OFFSET(void *, MaxCoord);		   	/* From MinMax */
	} FeatMinMaxRecord;

typedef struct
	{
	OFFSET(void *, MinCoord);
	OFFSET(void *, MaxCoord);
	Card16 FeatMinMaxCount;
	FeatMinMaxRecord *FeatMinMaxRecord;	/* [FeatMinMaxCount] */
	} MinMax;

typedef struct
	{
	Tag BaseLangSysTag;
	OFFSET(MinMax, MinMax);				/* From BaseScript */
	} BaseLangSysRecord;

typedef struct
	{
	OFFSET(BaseValues, BaseValues);
	OFFSET(MinMax, DefaultMinMax);
	Card16 BaseLangSysCount;
	BaseLangSysRecord *BaseLangSysRecord;	/* [BaseLangSysCount] */
	} BaseScript;

typedef struct
	{
	Tag BaseScriptTag;
	OFFSET(BaseScript, BaseScript);		/* From BaseScriptList */
	} BaseScriptRecord;

typedef struct
	{
	Card16 BaseScriptCount;
	BaseScriptRecord *BaseScriptRecord;	/* [BaseScriptCount] */
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
