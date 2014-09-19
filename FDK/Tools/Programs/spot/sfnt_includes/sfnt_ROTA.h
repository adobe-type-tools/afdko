/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)ROTA.h	1.1
 * Changed:    7/14/95 18:31:45
 ***********************************************************************/

/*
 * Rotation table format definition.
 */

#ifndef FORMAT_ROTA_H
#define FORMAT_ROTA_H

#define ROTA_VERSION VERSION(1,0)

typedef struct
	{
	Card16 glyphIdDelta;
	FWord *hBslnShift;	/* [nMasters] */
	FWord *vBslnShift;	/* [nMasters] */
	} ROTAMetrics;
#define ROTA_METR_SIZE(nMasters) \
	(SIZEOF(ROTAMetrics, glyphIdDelta) + \
	 (SIZEOF(ROTAMetrics, hBslnShift[0]) + \
	  SIZEOF(ROTAMetrics, vBslnShift[0])) * nMasters)

typedef struct
	{
	Fixed version;
	Card16 flags;
	Card16 nMasters;
	GlyphId firstGlyph;
	GlyphId lastGlyph;
	Lookup lookup;
	} ROTATbl;

#endif /* FORMAT_ROTA_H */
