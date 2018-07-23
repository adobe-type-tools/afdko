/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vhea.h	1.2
 * Changed:    2/5/99 16:17:01
 ***********************************************************************/

/*
 * Vertical header table format definition.
 */

#ifndef FORMAT_VHEA_H
#define FORMAT_VHEA_H

#define vhea_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	FWord vertTypoAscender;
	FWord vertTypoDescender;
	FWord vertTypoLineGap;
	uFWord advanceHeightMax;
	FWord minTopSideBearing;
	FWord minBottomSideBearing;
	FWord yMaxExtent;
	Int16 caretSlopeRise;
	Int16 caretSlopeRun;
	Int16 caretOffset;
	Int16 reserved[4];
	Int16 metricDataFormat;
	Card16 numberOfLongVerMetrics;
	} vheaTbl;

#endif /* FORMAT_VHEA_H */
