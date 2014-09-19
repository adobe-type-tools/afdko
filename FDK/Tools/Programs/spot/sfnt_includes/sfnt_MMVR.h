/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMVR.h	1.1
 * Changed:    4/13/95 10:15:17
 ***********************************************************************/

/*
 * Multiple master variation table format definition.
 */

#ifndef FORMAT_MMVR_H
#define FORMAT_MMVR_H

#define MMVR_VERSION VERSION(1,0)

typedef struct
	{
	Card32 Tag;
	Card16 Default;
	Card16 Scale;
	} Axis;

typedef struct
	{
	Fixed Version;
	Card16 Flags;
	Card16 AxisCount;
	Axis *axis;
	} MMVRTbl;

#endif /* FORMAT_MMVR_H */
