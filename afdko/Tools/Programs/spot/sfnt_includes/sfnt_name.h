/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)name.h	1.2
 * Changed:    8/10/95 15:35:26
 ***********************************************************************/

/*
 * Name table format definition.
 */

#ifndef FORMAT_NAME_H
#define FORMAT_NAME_H

#define name_FORMAT 0

typedef struct
	{
	Card16 platformId;
	Card16 scriptId;
	Card16 languageId;
	Card16 nameId;
	Card16 length;
	Card16 offset;
	} NameRecord;
#define NAME_REC_SIZE (SIZEOF(NameRecord, platformId) + \
					   SIZEOF(NameRecord, scriptId) + \
					   SIZEOF(NameRecord, languageId) + \
					   SIZEOF(NameRecord, nameId) + \
					   SIZEOF(NameRecord, length) + \
					   SIZEOF(NameRecord, offset))

typedef struct
	{
	Card16 length;
	Card16 offset;
	} LangTagRecord;
#define LANG_TAG_REC_SIZE (SIZEOF(NameRecord, length) + \
					   SIZEOF(NameRecord, offset))

typedef struct
	{
	Card16 format;
	Card16 count;
	Card16 stringOffset;
	DCL_ARRAY(NameRecord, record);
	Card16 langTagCount;
	DCL_ARRAY(LangTagRecord, langTag);
	DCL_ARRAY(Card8, strings);
	} nameTbl;
#define TBL_HDR_SIZE (SIZEOF(nameTbl, format) + \
					  SIZEOF(nameTbl, count) + \
					  SIZEOF(nameTbl, stringOffset))


#define LANG_TAG_BASE_INDEX 0x8000

#endif /* FORMAT_NAME_H */
