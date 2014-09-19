/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)ALMX.h	1.1
 * Changed:    7/14/95 18:31:59
 ***********************************************************************/

/*
 * Alternate metrics table format definition.
 */

#ifndef FORMAT_ALMX_H
#define FORMAT_ALMX_H

#define ALMX_VERSION VERSION(1,0)

typedef struct
	{
	Card16 glyphIdDelta;
	FWord *hAdv;	/* [nMasters] */
	FWord *hOrigX;	/* [nMasters] */
	FWord *vAdv;	/* [nMasters] */
	FWord *vOrigY;	/* [nMasters] */
	} ALMXMetrics;
#define ALMX_METR_SIZE(nMasters) (SIZEOF(ALMXMetrics, glyphIdDelta) + \
								  (SIZEOF(ALMXMetrics, hAdv[0]) + \
								   SIZEOF(ALMXMetrics, hOrigX[0]) + \
								   SIZEOF(ALMXMetrics, vAdv[0]) + \
								   SIZEOF(ALMXMetrics, vOrigY[0])) * nMasters)
typedef struct
	{
	Fixed version;
	Card16 flags;
	Card16 nMasters;
	GlyphId firstGlyph;
	GlyphId lastGlyph;
	Lookup lookup;
	} ALMXTbl;

#endif /* FORMAT_ALMX_H */
