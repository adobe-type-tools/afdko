/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hhea.h	1.1
 * Changed:    4/13/95 10:15:23
 ***********************************************************************/

/*
 * Horizontal header table format definition.
 */

#ifndef FORMAT_HHEA_H
#define FORMAT_HHEA_H

#define hhea_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	FWord ascender;
	FWord descender;
	FWord lineGap;
	uFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	Int16 caretSlopeRise;
	Int16 caretSlopeRun;
	Int16 caretOffset;
	Int16 reserved[4];
	Int16 metricDataFormat;
	Card16 numberOfLongHorMetrics;
	} hheaTbl;

#endif /* FORMAT_HHEA_H */
