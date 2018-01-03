/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fvar.h	1.1
 * Changed:    4/13/95 10:15:22
 ***********************************************************************/

/*
 * Font variation table format definition.
 */

#ifndef FORMAT_FVAR_H
#define FORMAT_FVAR_H

#define fvar_VERSION VERSION(1,0)

typedef struct
	{
	Card32 axisTag;
	Fixed minValue;
	Fixed defaultValue;
	Fixed maxValue;
	Card16 flags;
	Card16 nameId;
	} Axis;
#define AXIS_SIZE (SIZEOF(Axis, axisTag) + \
				   SIZEOF(Axis, minValue) + \
				   SIZEOF(Axis, defaultValue) + \
				   SIZEOF(Axis, maxValue) + \
				   SIZEOF(Axis, flags) + \
				   SIZEOF(Axis, nameId))

typedef struct
	{
	Card16 nameId;
	Card16 flags;
    Card16 psNameId; /* This field may not be present in font */
	Fixed *coord;
    } Instance;
#define INSTANCE_SIZE(axes) (SIZEOF(Instance, nameId) + \
                            SIZEOF(Instance, flags) + \
                            SIZEOF(Instance, psNameId) + \
							 SIZEOF(Instance, coord[0]) * (axes)  )

typedef struct
	{
	Fixed version;
	Card16 offsetToData;
	Card16 countSizePairs;
	Card16 axisCount;
	Card16 axisSize;
	Card16 instanceCount;
	Card16 instanceSize;
	Axis *axis;
	Instance *instance;
	} fvarTbl;
#define HEADER_SIZE (SIZEOF(fvarTbl, version) + \
					 SIZEOF(fvarTbl, offsetToData) + \
					 SIZEOF(fvarTbl, countSizePairs) + \
					 SIZEOF(fvarTbl, axisCount) + \
					 SIZEOF(fvarTbl, axisSize) + \
					 SIZEOF(fvarTbl, instanceCount) + \
					 SIZEOF(fvarTbl, instanceSize))

#endif /* FORMAT_FVAR_H */
