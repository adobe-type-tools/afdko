/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)VORG.h	1.0
 * Changed:    06/02/00 
 ***********************************************************************/

/*
 * Vertical Origin table definition.
 */

#ifndef FORMAT_VORG_H
#define FORMAT_VORG_H

#define VORG_VERSION VERSION(1,0)

typedef struct
	{
	Card16 glyphIndex;
	Int16 vertOriginY;
	} vertOriginYMetric;

typedef struct
	{
	Card16 major, minor;
	Int16 defaultVertOriginY;
	Card16 numVertOriginYMetrics;
	vertOriginYMetric *vertMetrics;
	} VORGTbl;

#endif /* FORMAT_VMTX_H */
