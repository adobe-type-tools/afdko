/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)FNTP.h	1.1
 * Changed:    7/30/95 14:52:08
 ***********************************************************************/

/*
 * Font protection table format definition.
 */

#ifndef FORMAT_FNTP_H
#define FORMAT_FNTP_H

#define FNTP_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 flags;
	Card8 data[64];
	} FNTPTbl;

#endif /* FORMAT_FNTP_H */
