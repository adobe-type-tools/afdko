/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CSNP.h	1.1
 * Changed:    4/13/95 10:15:15
 ***********************************************************************/

/*
 * ??? table format definition.
 */

#ifndef FORMAT_CSNP_H
#define FORMAT_CSNP_H

#define CSNP_VERSION VERSION(1,0)

typedef struct
	{
	Card8 type;
	Card8 nValues;
	Int16 *value;
	} Hint;

typedef struct
	{
	Fixed version;
	Card32 flags;
	Card16 nHints;
	Hint *hint;
	} CSNPTbl;

#endif /* FORMAT_CSNP_H */
