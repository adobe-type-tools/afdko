/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)LTSH.h	1.1
 * Changed:    4/13/95 10:15:17
 ***********************************************************************/

/*
 * Linear threshold table format definition.
 */

#ifndef FORMAT_LTSH_H
#define FORMAT_LTSH_H

#define LTSH_VERSION 0

typedef struct
	{
	Card16 version;
	Card16 numGlyphs;
	Card8 *yPels;
	} LTSHTbl;

#endif /* FORMAT_LTSH_H */
