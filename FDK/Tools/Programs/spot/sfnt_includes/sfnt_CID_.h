/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CID_.h	1.1
 * Changed:    4/13/95 10:15:14
 ***********************************************************************/

/*
 * CID data table format defintion.
 */

#ifndef FORMAT_CID__H
#define FORMAT_CID__H

#define CID__VERSION VERSION(1,0)

typedef struct
	{
	Fixed Version;
	Card16 Flags;
#define CID_REARRANGED_FONT	(1<<0)
	Card16 CIDCount;
	Card32 TotalLength;
	Card32 AsciiLength;
	Card32 BinaryLength;
	Card16 FDCount;
	} CID_Tbl;
#define CID__HDR_SIZE (SIZEOF(CID_Tbl, Version) + \
					   SIZEOF(CID_Tbl, Flags) + \
					   SIZEOF(CID_Tbl, CIDCount) + \
					   SIZEOF(CID_Tbl, TotalLength) + \
					   SIZEOF(CID_Tbl, AsciiLength) + \
					   SIZEOF(CID_Tbl, BinaryLength) + \
					   SIZEOF(CID_Tbl, FDCount))

#endif /* FORMAT_CID__H */
