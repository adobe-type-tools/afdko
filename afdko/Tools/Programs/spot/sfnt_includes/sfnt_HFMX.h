/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)HFMX.h	1.2
 * Changed:    7/14/95 18:27:21
 ***********************************************************************/

/*
 * Horizontal font metrics table format definition.
 */

#ifndef FORMAT_HFMX_H
#define FORMAT_HFMX_H

#define HFMX_VERSION VERSION(1,0)

typedef struct
	{
	Fixed Version;
	FWord *Ascent;
	FWord *Descent;
	FWord *LineGap;
	Int16 *CaretSlopeRise;
	Int16 *CaretSlopeRun;
	FWord *CaretOffset;
	} HFMXTbl;

#endif /* FORMAT_HFMX_H */
