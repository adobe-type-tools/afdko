/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)VFMX.h	1.1
 * Changed:    7/14/95 18:31:36
 ***********************************************************************/

/*
 * Vertical metrics table definition.
 */

#ifndef FORMAT_VFMX_H
#define FORMAT_VFMX_H

#define VFMX_VERSION VERSION(2,0)

typedef struct
	{
	FWord *vAdv;	/* [nMasters] */
	FWord *vOrigX;	/* [nMasters] */
	FWord *vOrigY;	/* [nMasters] */
	} VFMXMetrics;
#define VFMX_METR_SIZE(nMasters) \
	((SIZEOF(VFMXMetrics, vAdv[0]) + \
	  SIZEOF(VFMXMetrics, vOrigX[0]) + \
	  SIZEOF(VFMXMetrics, vOrigY[0])) * (nMasters))
								  
typedef struct
	{
	Fixed version;
	Card16 flags;
	Card16 nMasters;
	FWord *before;			/* [nMasters] */
	FWord *after;			/* [nMasters] */
	FWord *caretSlopeRise;	/* [nMasters] */
	FWord *caretSlopeRun;	/* [nMasters] */
	FWord *caretOffset;		/* [nMasters] */
	VFMXMetrics dflt;
	Lookup lookup;
	} VFMXTbl;

#endif /* FORMAT_VFMX_H */
