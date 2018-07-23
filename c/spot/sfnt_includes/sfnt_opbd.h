/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)opbd.h	1.1
 * Changed:    4/13/95 10:15:27
 ***********************************************************************/

/*
 * Optical bound table format definition.
 */

#ifndef FORMAT_OPBD_H
#define FORMAT_OPBD_H

#define opbd_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 format;
	Lookup lookup;
	} opbdTbl;
#define TBL_HDR_SIZE (SIZEOF(opbdTbl, version) + \
					  SIZEOF(opbdTbl, format))

#endif /* FORMAT_OPBD_H */
