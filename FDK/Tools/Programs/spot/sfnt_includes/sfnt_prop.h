/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)prop.h	1.1
 * Changed:    4/13/95 10:15:27
 ***********************************************************************/

/*
 * Properties table format definition.
 */

#ifndef FORMAT_PROP_H
#define FORMAT_PROP_H

#define prop_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 format;
	Card16 defaultProps;
	Lookup lookup;
	} propTbl;

#endif /* FORMAT_PROP_H */
