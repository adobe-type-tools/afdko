/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hmtx.h	1.1
 * Changed:    4/13/95 10:15:23
 ***********************************************************************/

/*
 * Horizontal metrics table definition.
 */

#ifndef FORMAT_HMTX_H
#define FORMAT_HMTX_H

#define hmtx_VERSION VERSION(1,0)

typedef struct
	{
	uFWord advanceWidth;
	FWord lsb;
	} LongHorMetrics;

typedef struct
	{
	LongHorMetrics *hMetrics;
	FWord *leftSideBearing;
	} hmtxTbl;

#endif /* FORMAT_HMTX_H */
