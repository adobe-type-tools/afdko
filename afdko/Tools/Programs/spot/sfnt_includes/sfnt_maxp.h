/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)maxp.h	1.1
 * Changed:    4/13/95 10:15:26
 ***********************************************************************/

/*
 * Maximum profile table format definition.
 */

#ifndef FORMAT_MAXP_H
#define FORMAT_MAXP_H

#define maxp_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 numGlyphs;
	Card16 maxPoints;
	Card16 maxContours;
	Card16 maxCompositePoints;
	Card16 maxCompositeContours;
	Card16 maxZones;
	Card16 maxTwilightPoints;
	Card16 maxStorage;
	Card16 maxFunctionDefs;
	Card16 maxInstructionDefs;
	Card16 maxStackElements;
	Card16 maxSizeOfInstructions;
	Card16 maxComponentElements;
	Card16 maxComponentDepth;
	} maxpTbl;

#endif /* FORMAT_MAXP_H */
