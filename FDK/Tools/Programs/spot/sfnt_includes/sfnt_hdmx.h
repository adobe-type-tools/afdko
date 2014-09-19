/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hdmx.h	1.1
 * Changed:    4/13/95 10:15:22
 ***********************************************************************/

/*
 * Horizontal device metrics table definition.
 */

#ifndef FORMAT_HDMX_H
#define FORMAT_HDMX_H

#define hdmx_VERSION VERSION(1,0)

typedef struct
	{
	Card8 pixelsPerEm;
	Card8 maxWidth;
	Card8 *widths;
	} DeviceRecord;

typedef struct
	{
	Card16 version;
	Card16 nRecords;
	Card32 recordSize;
	DeviceRecord *record;
	} hdmxTbl;
#define TBL_HDR_SIZE (SIZEOF(hdmxTbl, version) + \
					  SIZEOF(hdmxTbl, nRecords) + \
					  SIZEOF(hdmxTbl, recordSize))

#endif /* FORMAT_HDMX_H */
