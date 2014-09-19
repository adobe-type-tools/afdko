/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)OS_2.h	1.6
 * Changed:    11/20/98 16:03:40
 ***********************************************************************/

/*
 * OS/2 table format definition.
 */

#ifndef FORMAT_OS_2_H
#define FORMAT_OS_2_H

typedef struct
	{
	Card16 version;

	Card16 averageWidth;
	Card16 weightClass;
	Card16 widthClass;
	Card16 type;
	Card16 subscriptXSize;
	Card16 subscriptYSize;
	Int16 subscriptXOffset;
	Int16 subscriptYOffset;
	Card16 superscriptXSize;
	Card16 superscriptYSize;
	Int16 superscriptXOffset;
	Int16 superscriptYOffset;
	Card16 strikeoutSize;
	Int16 strikeoutPosition;
	Card16 familyClass;
	Card8 panose[10];
	Card32 charRange[4];
	Card8 vendor[4];
	Card16 selection;
	Card16 firstChar;
	Card16 lastChar;
	Int16 typographicAscent;
	Int16 typographicDescent;
	Int16 typographicLineGap;
	Card16 windowsAscent;
	Card16 windowsDescent;
	Card32 CodePageRange[2];	/* Version 1 */
	Int16 XHeight;				/* Version 2 */
	Int16 CapHeight;			/* Version 2 */
	Card16 DefaultChar;			/* Version 2 */
	Card16 BreakChar;			/* Version 2 */
	Int16 maxContext; 			/* Version 2 */
    Card16 usLowerOpticalPointSize;			/* Version 5 */
    Card16 usUpperOpticalPointSize;			/* Version 5 */
	} OS_2Tbl;

#endif /* FORMAT_OS_2_H */
