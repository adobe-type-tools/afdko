/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fbit.h	1.4
 * Changed:    5/17/96 18:13:17
 ***********************************************************************/


#ifndef FORMAT_FBIT_H
#define FORMAT_FBIT_H

#define FBIT_VERSION VERSION(1,1)

typedef struct _fbitStdHeader 
	{
	  Card32 _rsrcLength;         /* FILLIN: length of resource data */
	  Card16 fontFlags;
#define FBIT_SUBSTITUTIONISON 0x0001  /* KanjiTalk font subst is enabled */
#define FBIT_STATICFONT	 0x2000
	  Card32 resourceType;     /* Tag: 'fbit" */
	  Int16  resourceID;
	  Card16 fdefVersion;
	  Card16 fdefID;
#define FBIT_FDEFID_DISKBASED  0x0007
#define FBIT_FDEFID_SFNTCOMPAT  0x002A
	  Card32 *fdefProc;        /* 32-bit pointer */
	  Int16  fondID;
	  Card16 searchPriority;
#define FBIT_STATIC_PRIORITY 0x0300
	  Card32 reserved0; 	   /* set to 0 */
	  Card16 height;           /* glyph height */
	  Card16 width;
	  Int16  style;
	  Card16 reserved1;	   /* ?? Bit depth ?? */
	  Card16 firstChar;
	  Card16 lastChar;
	} fbitStdHeader;


typedef struct _fbitCharToOffset
	{
	  Card16 firstChar;
	  Card16 lastChar;
	  Card16 flags;    	/* reserved. Set to 0 */
	  Card32 offset;	/* format 5: offset to bit image from beginning of "bdat" */
	} fbitCharToOffset;


/* "fbit"-42 structure for "sbit" format 5 */
typedef struct _sbit5fbit42Record
	{
	  fbitStdHeader  header;
	  Card16  flags;
	  Int16   sfntID;
	  Int16   fontScript;
	  Int16   bdatFormat; 		/* should be 5 */
	  Card32  bdatOffset; 		/* offset to "bdat" in "sfnt" */
	  Card32  bdatImageSize;	/* size of bit image per glyph */
	  Card8 **bdatRowData;         /* Handle: row-bitmap data from "bdat" */
	  Card8   bdatHeight;
	  Card8   bdatWidth;
	  Card8   top;        	/* offset from top of bdat bitmap to fbit bitmap */
	  Card8   left;        	/* offset from left of bdat bitmap to fbit bitmap */
	  Card16  numRanges;
	  DCL_ARRAY(fbitCharToOffset,  ranges);  /* range info. When substitution is disabled,
					 ranges[0] should contain offset to "missing" glyph.
					 In this case, firstChar should be == 0 */
	  Card8 *sfntName;         /* Pascal-String containing font family name.
				      Should be same as unique subfamily ID name in "name" table */
	} sbit5fbit42Record;


/* from a message from Ariza-san  25-Jan-95 */
typedef struct _fbit42Record
	{
	  fbitStdHeader  header;
	  Card32  placeholder1;
	  Int16   placeholder2;  /* == 0 */
	  Int16   bdatFormat; 		/* should be 5 */
	  Card32  bdatOffset; 		/* offset base to be added to glyph offset */
	  Card32  placeholder3;	/* == 0*/
	  Card32  placeholder4; /* == 0 */
	  Card8   bdatWidth;
	  Card8   bdatHeight;
	  Card16  placeholder5; /* == 0 */
	  Card16  numRanges;
	  fbitCharToOffset  *ranges;  /* range info. When substitution is disabled,
					 ranges[0] should contain offset to "missing" glyph.
					 In this case, firstChar should be == 0 */
	  Card8 *sfntName;         /* Pascal-String containing font family name.
				      Should be same as unique subfamily ID name in "name" table */
	} fbit42Record;


typedef struct _fbitKoreanStdHeader 
	{
	  Card32 _rsrcLength;         /* FILLIN: length of resource data */
	  Card16 fontFlags;
#define FBIT_KOREANFLAGS 0x8000
	  Card32 resourceType;     /* Tag: 'fbit" */
	  Int16  unused;
	  Card16 fdefVersion;
	  Int16  resourceID;
	  Card16 fdefID;
#define FBIT_FDEFID_SFNTCOMPAT  0x002A
	  Card32 *fdefProc;        /* 32-bit pointer */
	  Int16  fondID;
	  Card16 searchPriority;
#define FBIT_STATIC_PRIORITY 0x0300
	  Card16 width;
	  Int16  style; 		/* 0  */
	  Card16 reserved1;	   	/* 0x000 */
	  Card16 firstChar;
	  Card16 lastChar;
	  Int16  fbithglcID;  		/* hglc ID */
	  Int32  fbithglcEntry; 	/* ptr to hglc table entry */
	  Int16  fbitHanjaflg; 		/* Hanja flag */
	  Int16  fbitHanjaComm[16]; 	/* Hanja fbit for common use */
	} fbitKoreanStdHeader;


/* "fbit"-42 structure for "sbit" format 5 for Korean */
typedef struct _sbitKorean5fbit42Record
	{
	  fbitKoreanStdHeader  header;
	  Card16  flags;
	  Int16   sfntID;
	  Int16   fontScript;
	  Int16   bdatFormat; 		/* should be 5 */
	  Card32  bdatOffset; 		/* offset to "bdat" in "sfnt" */
	  Card32  bdatImageSize;	/* size of bit image per glyph */
	  Card8 **bdatRowData;         /* Handle: row-bitmap data from "bdat" */
	  Card8   bdatHeight;
	  Card8   bdatWidth;
	  Card8   top;        	/* offset from top of bdat bitmap to fbit bitmap */
	  Card8   left;        	/* offset from left of bdat bitmap to fbit bitmap */
	  Card16  numRanges;
	  DCL_ARRAY(fbitCharToOffset,  ranges);  /* range info. When substitution is disabled,
					 ranges[0] should contain offset to "missing" glyph.
					 In this case, firstChar should be == 0 */
	  Card8 *sfntName;         /* Pascal-String containing font family name.
				      Should be same as unique subfamily ID name in "name" table */
	} sbitKorean5fbit42Record;

#endif

