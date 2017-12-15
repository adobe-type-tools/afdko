/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)SING.h	1.1
 * Changed:    4/13/95 10:15:25
 ***********************************************************************/

/*
 * SING table for glyphlets
 */

#ifndef FORMAT_SING_H
#define FORMAT_SING_H

#define SING_VERSION VERSION(1,1)

#define SING_UNIQUENAMELEN 28
#define SING_MD5LEN 16

typedef struct
	{
	Card16 tableVersionMajor;	
	Card16 tableVersionMinor;
	Card16 glyphletVersion;
	Card16 permissions;
	Card16 mainGID;
	Card16 unitsPerEm;
	Int16 vertAdvance;
	Int16 vertOrigin;
	Card8 uniqueName[SING_UNIQUENAMELEN];
	Card8 METAMD5[SING_MD5LEN];
	Card8 nameLength;
	Card8 *baseGlyphName; /* name array */
	} SINGTbl;

#endif /* FORMAT_SING_H */
