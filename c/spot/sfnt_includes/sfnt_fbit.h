/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef FORMAT_FBIT_H
#define FORMAT_FBIT_H

#define FBIT_VERSION VERSION(1, 1)

typedef struct _fbitStdHeader {
    uint32_t _rsrcLength; /* FILLIN: length of resource data */
    uint16_t fontFlags;
#define FBIT_SUBSTITUTIONISON 0x0001 /* KanjiTalk font substitution is enabled */
#define FBIT_STATICFONT 0x2000
    uint32_t resourceType; /* Tag: 'fbit" */
    int16_t resourceID;
    uint16_t fdefVersion;
    uint16_t fdefID;
#define FBIT_FDEFID_DISKBASED 0x0007
#define FBIT_FDEFID_SFNTCOMPAT 0x002A
    uint32_t *fdefProc; /* 32-bit pointer */
    int16_t fondID;
    uint16_t searchPriority;
#define FBIT_STATIC_PRIORITY 0x0300
    uint32_t reserved0; /* set to 0 */
    uint16_t height;    /* glyph height */
    uint16_t width;
    int16_t style;
    uint16_t reserved1; /* ?? Bit depth ?? */
    uint16_t firstChar;
    uint16_t lastChar;
} fbitStdHeader;

typedef struct _fbitCharToOffset {
    uint16_t firstChar;
    uint16_t lastChar;
    uint16_t flags;  /* reserved. Set to 0 */
    uint32_t offset; /* format 5: offset to bit image from beginning of "bdat" */
} fbitCharToOffset;

/* "fbit"-42 structure for "sbit" format 5 */
typedef struct _sbit5fbit42Record {
    fbitStdHeader header;
    uint16_t flags;
    int16_t sfntID;
    int16_t fontScript;
    int16_t bdatFormat;     /* should be 5 */
    uint32_t bdatOffset;    /* offset to "bdat" in "sfnt" */
    uint32_t bdatImageSize; /* size of bit image per glyph */
    uint8_t **bdatRowData;  /* Handle: row-bitmap data from "bdat" */
    uint8_t bdatHeight;
    uint8_t bdatWidth;
    uint8_t top;  /* offset from top of bdat bitmap to fbit bitmap */
    uint8_t left; /* offset from left of bdat bitmap to fbit bitmap */
    uint16_t numRanges;
    DCL_ARRAY(fbitCharToOffset, ranges); /* range info. When substitution is */
                                         /* disabled, ranges[0] should       */
                                         /* contain offset to "missing"      */
                                         /* glyph. In this case, firstChar   */
                                         /* should be == 0                   */
    uint8_t *sfntName; /* Pascal-String containing font family name.           */
                     /* Should be same as unique subfamily ID name in "name" */
                     /* table                                                */
} sbit5fbit42Record;

/* from a message from Ariza-san  25-Jan-95 */
typedef struct _fbit42Record {
    fbitStdHeader header;
    uint32_t placeholder1;
    int16_t placeholder2;  /* == 0 */
    int16_t bdatFormat;    /* should be 5 */
    uint32_t bdatOffset;   /* offset base to be added to glyph offset */
    uint32_t placeholder3; /* == 0*/
    uint32_t placeholder4; /* == 0 */
    uint8_t bdatWidth;
    uint8_t bdatHeight;
    uint16_t placeholder5; /* == 0 */
    uint16_t numRanges;
    fbitCharToOffset *ranges; /* range info. When substitution is disabled,          */
                              /* ranges[0] should contain offset to "missing" glyph. */
                              /* In this case, firstChar should be == 0              */
    uint8_t *sfntName; /* Pascal-String containing font family name.                  */
                     /* Should be same as unique subfamily ID name in "name" table. */
} fbit42Record;

typedef struct _fbitKoreanStdHeader {
    uint32_t _rsrcLength; /* FILLIN: length of resource data */
    uint16_t fontFlags;
#define FBIT_KOREANFLAGS 0x8000
    uint32_t resourceType; /* Tag: 'fbit" */
    int16_t unused;
    uint16_t fdefVersion;
    int16_t resourceID;
    uint16_t fdefID;
#define FBIT_FDEFID_SFNTCOMPAT 0x002A
    uint32_t *fdefProc; /* 32-bit pointer */
    int16_t fondID;
    uint16_t searchPriority;
#define FBIT_STATIC_PRIORITY 0x0300
    uint16_t width;
    int16_t style;      /* 0  */
    uint16_t reserved1; /* 0x000 */
    uint16_t firstChar;
    uint16_t lastChar;
    int16_t fbithglcID;        /* hglc ID */
    int32_t fbithglcEntry;     /* ptr to hglc table entry */
    int16_t fbitHanjaflg;      /* Hanja flag */
    int16_t fbitHanjaComm[16]; /* Hanja fbit for common use */
} fbitKoreanStdHeader;

/* "fbit"-42 structure for "sbit" format 5 for Korean */
typedef struct _sbitKorean5fbit42Record {
    fbitKoreanStdHeader header;
    uint16_t flags;
    int16_t sfntID;
    int16_t fontScript;
    int16_t bdatFormat;     /* should be 5 */
    uint32_t bdatOffset;    /* offset to "bdat" in "sfnt" */
    uint32_t bdatImageSize; /* size of bit image per glyph */
    uint8_t **bdatRowData;  /* Handle: row-bitmap data from "bdat" */
    uint8_t bdatHeight;
    uint8_t bdatWidth;
    uint8_t top;  /* offset from top of bdat bitmap to fbit bitmap */
    uint8_t left; /* offset from left of bdat bitmap to fbit bitmap */
    uint16_t numRanges;
    DCL_ARRAY(fbitCharToOffset, ranges); /* range info. When substitution is disabled,          */
                                         /* ranges[0] should contain offset to "missing" glyph. */
                                         /* In this case, firstChar should be == 0              */
    uint8_t *sfntName; /* Pascal-String containing font family name.                 */
                     /* Should be same as unique subfamily ID name in "name" table */
} sbitKorean5fbit42Record;

#endif
