/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * SING table for glyphlets
 */

#ifndef FORMAT_SING_H
#define FORMAT_SING_H

#define SING_VERSION VERSION(1, 1)

#define SING_UNIQUENAMELEN 28
#define SING_MD5LEN 16

typedef struct
{
    uint16_t tableVersionMajor;
    uint16_t tableVersionMinor;
    uint16_t glyphletVersion;
    uint16_t permissions;
    uint16_t mainGID;
    uint16_t unitsPerEm;
    int16_t vertAdvance;
    int16_t vertOrigin;
    uint8_t uniqueName[SING_UNIQUENAMELEN];
    uint8_t METAMD5[SING_MD5LEN];
    uint8_t nameLength;
    uint8_t *baseGlyphName; /* name array */
} SINGTbl;

#endif /* FORMAT_SING_H */
