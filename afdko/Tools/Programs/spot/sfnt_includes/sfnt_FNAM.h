/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)FNAM.h	1.2
 * Changed:    8/10/95 15:35:24
 ***********************************************************************/

/*
 * FOND name table format definition.
 */

#ifndef FORMAT_FNAM_H
#define FORMAT_FNAM_H

#define FNAM_VERSION VERSION(1,0)

typedef struct
	{
	Card8 style;
	Card8 *name;
	} Client;

typedef struct
	{
	Card16 nClients;			/* Temporary, not part of format */
	Client *client;
	} Encoding;

typedef struct
	{
	Fixed version;
	Card16 nEncodings;
	Card16 *offset;
    Encoding *encoding;
	} FNAMTbl;

#endif /* FORMAT_FNAM_H */
