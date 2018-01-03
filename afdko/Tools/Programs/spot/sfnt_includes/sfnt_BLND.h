/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BLND.h	1.3
 * Changed:    8/10/95 15:35:24
 ***********************************************************************/

/*
 * Blend table format definition. [ADS Technical Note #5087]
 */

#ifndef FORMAT_BLND_H
#define FORMAT_BLND_H

#define BLND_VERSION 3	/* Support for design to weight vector conversion */

typedef struct
	{
	Card16 designCoord;
	Fixed normalizedValue;
	} Map;
#define MAP_SIZE (SIZEOF(Map, designCoord) + \
				  SIZEOF(Map, normalizedValue))

typedef struct
	{
	Card16 flags;
#define FLAG_MAP	(1<<0)	/* Axis has map infomation */
	Card16 minRange;
	Card16 maxRange;
	Card8 *type;
	Card8 *longLabel;
	Card8 *shortLabel;
	Card16 nMaps;
	Map *map;
	} AxisInfo;
#define AXIS_INFO_HDR_SIZE (SIZEOF(AxisInfo, flags) + \
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
	Card8 code;
	Card8 flags;	/* Unused */
	Card16 axis;
	Card16 nDeltas;
	Delta *delta;
	} SPOT_Style;
#define STYLE_HDR_SIZE (SIZEOF(Style, code) + \
						SIZEOF(Style, flags) + \
						SIZEOF(Style, axis) + \
						SIZEOF(Style, nDeltas))

typedef struct
	{
	Card16 *coord;
	Card32 offset;
	Int16 FONDId;
	Int16 NFNTId;
	} Instance;

/* Design to weight vector conversion */
typedef struct
	{
	Card16 CDVNum;
	Card16 CDVLength;
	Card16 NDVNum;
	Card16 NDVLength;
	Card16 lenBuildCharArray;
	Card8 *CDVSubr;
	Card8 *NDVSubr;
	} D2WV;

typedef struct
	{
	Card16 version;
	Card16 flags;
#define	FLAG_CHAMELEON	(1<<0)	/* Font is chameleon */
	Card16 nAxes;
	Card16 nMasters;
	Card16 languageId;	/* Unused */
	Card16 iRegular;
	Card16 nOffsets;
	Card32 *axisOffset;
	Card32 masterNameOffset;
	Card32 styleOffset;
	Card32 instanceOffset;
	Card32 instanceNameOffset;
	Card32 d2wvOffset;	/* != 0 if design to weight vector code in font */
	AxisInfo *axisInfo;
	DCL_ARRAY(Card8, masterNames);
	Card16 nStyles;
	SPOT_Style *style;
	Card16 nInstances;
	Instance *instance;
	DCL_ARRAY(Card8, instanceNames);
	D2WV d2wv;
	} BLNDTbl;

#endif /* FORMAT_BLND_H */
