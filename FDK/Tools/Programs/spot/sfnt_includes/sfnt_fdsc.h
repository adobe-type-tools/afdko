/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fdsc.h	1.1
 * Changed:    4/13/95 10:15:21
 ***********************************************************************/

/*
 * Font descriptor table format definition.
 */

#ifndef FORMAT_FDSC_H
#define FORMAT_FDSC_H

#define fdsc_VERSION VERSION(1,0)

typedef struct
	{
	Card32 tag;
	Fixed value;
	} FontDescriptor;

typedef struct
	{
	Fixed version;
	Card32 nDescriptors;
	FontDescriptor *descriptor;
	} fdscTbl;

#endif /* FORMAT_FDSC_H */
