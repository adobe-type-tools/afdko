/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Name table format definition.
 */

#ifndef FORMAT_NAME_H
#define FORMAT_NAME_H

#define name_FORMAT 0

typedef struct
{
    uint16_t platformId;
    uint16_t scriptId;
    uint16_t languageId;
    uint16_t nameId;
    uint16_t length;
    uint16_t offset;
} NameRecord;
#define NAME_REC_SIZE (SIZEOF(NameRecord, platformId) + \
                       SIZEOF(NameRecord, scriptId) +   \
                       SIZEOF(NameRecord, languageId) + \
                       SIZEOF(NameRecord, nameId) +     \
                       SIZEOF(NameRecord, length) +     \
                       SIZEOF(NameRecord, offset))

typedef struct
{
    uint16_t length;
    uint16_t offset;
} LangTagRecord;
#define LANG_TAG_REC_SIZE (SIZEOF(LangTagRecord, length) + \
                           SIZEOF(LangTagRecord, offset))

typedef struct
{
    uint16_t format;
    uint16_t count;
    uint16_t stringOffset;
    DCL_ARRAY(NameRecord, record);
    uint16_t langTagCount;
    DCL_ARRAY(LangTagRecord, langTag);
    DCL_ARRAY(uint8_t, strings);
} nameTbl;
#define TBL_HDR_SIZE (SIZEOF(nameTbl, format) + \
                      SIZEOF(nameTbl, count) +  \
                      SIZEOF(nameTbl, stringOffset))

#define LANG_TAG_BASE_INDEX 0x8000

#endif /* FORMAT_NAME_H */
