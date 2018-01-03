/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)PFMX.h	1.1
 * Changed:    4/13/95 10:15:18
 ***********************************************************************/

/*
 * Global font information table format definition.
 */

#ifndef FORMAT_GLOB_H
#define FORMAT_GLOB_H

#define GLOB_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 flags;
#define GLOB_ITALIC         (1<<0)  /* Italic font */
#define GLOB_BOLD           (1<<1)  /* Bold font */
#define GLOB_SERIF          (1<<2)  /* Serif font */
#define GLOB_STD_ENC        (1<<6)  /* Font specifies StandardEncoding */
#define GLOB_FIXED_PITCH    (1<<7)  /* Fixed pitch font */
#define GLOB_ITALIC_AXIS    (1<<8)  /* Has italic axis */
#define GLOB_WEIGHT_AXIS    (1<<9)  /* Has weight axis */
#define GLOB_SERIF_AXIS     (1<<10) /* Has serif axis */
#define GLOB_CHAMELEON      (1<<14) /* Is chameleon font */
#define GLOB_INSTANCE_OK    (1<<15) /* Can make text instance */
	Card16 nMasters;
	Fixed matrix[6];				/* ???Not convinced this is needed */
	Fixed *italicAngle;				/* [nMasters] */
	FWord *bboxLeft;				/* [nMasters] */
	FWord *bboxBottom;				/* [nMasters] */
	FWord *bboxRight;				/* [nMasters] */
	FWord *bboxTop;					/* [nMasters] */
	FWord *capHeight;				/* [nMasters] */
	FWord *xHeight;					/* [nMasters] */
	FWord *underlinePosition;		/* [nMasters] */
	FWord *underlineThickness;		/* [nMasters] */
	FWord *dominantV;				/* [nMasters] */
	FWord *avgWidth;				/* [nMasters]. Windows ANSI encoding */
	FWord *maxWidth;				/* [nMasters]. Windows ANSI encoding */
	Card8 defaultChar;				/* Windows ANSI encoding */
	Card8 breakChar;				/* Windows ANSI encoding */
	Card16 unitsPerEm;
	Card16 macsfntId;
	Card16 winMenuNameOffset;
	Card8 winFileNamePrefix[5];
	Card8 *names;					/* String pool */
	} GLOBTbl;
#define WIN_MENU_NAME_OFFSET(n) \
	(SIZEOF(GLOBTbl, version) + \
	 SIZEOF(GLOBTbl, flags) + \
	 SIZEOF(GLOBTbl, nMasters) + \
	 SIZEOF(GLOBTbl, matrix) + \
	 (SIZEOF(GLOBTbl, italicAngle[0]) + \
	  SIZEOF(GLOBTbl, bboxLeft[0]) + \
	  SIZEOF(GLOBTbl, bboxBottom[0]) + \
	  SIZEOF(GLOBTbl, bboxRight[0]) + \
	  SIZEOF(GLOBTbl, bboxTop[0]) + \
	  SIZEOF(GLOBTbl, capHeight[0]) + \
	  SIZEOF(GLOBTbl, xHeight[0]) + \
	  SIZEOF(GLOBTbl, underlinePosition[0]) + \
	  SIZEOF(GLOBTbl, underlineThickness[0]) + \
	  SIZEOF(GLOBTbl, dominantV[0]) + \
	  SIZEOF(GLOBTbl, avgWidth[0]) + \
	  SIZEOF(GLOBTbl, maxWidth[0])) * (n) + \
	 SIZEOF(GLOBTbl, defaultChar) + \
	 SIZEOF(GLOBTbl, breakChar) + \
	 SIZEOF(GLOBTbl, unitsPerEm) + \
	 SIZEOF(GLOBTbl, macsfntId) + \
	 SIZEOF(GLOBTbl, winMenuNameOffset) + \
	 SIZEOF(GLOBTbl, winFileNamePrefix))

#endif /* FORMAT_GLOB_H */
