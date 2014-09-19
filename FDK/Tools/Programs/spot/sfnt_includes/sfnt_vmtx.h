/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vmtx.h	1.1
 * Changed:    10/24/95 18:31:08
 ***********************************************************************/

/*
 * Vertical metrics table definition.
 */

#ifndef FORMAT_VMTX_H
#define FORMAT_VMTX_H

#define vmtx_VERSION VERSION(1,0)

typedef struct
	{
	uFWord advanceHeight;
	FWord topSideBearing;
	} LongVerMetrics;

typedef struct
	{
	LongVerMetrics *vMetrics;
	FWord *topSideBearing;
	} vmtxTbl;

#endif /* FORMAT_VMTX_H */
