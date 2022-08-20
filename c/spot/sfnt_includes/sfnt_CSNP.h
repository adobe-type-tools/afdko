/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * ??? table format definition.
 */

#ifndef FORMAT_CSNP_H
#define FORMAT_CSNP_H

#define CSNP_VERSION VERSION(1, 0)

typedef struct
{
    uint8_t type;
    uint8_t nValues;
    int16_t *value;
} Hint;

typedef struct
{
    Fixed version;
    uint32_t flags;
    uint16_t nHints;
    Hint *hint;
} CSNPTbl;

#endif /* FORMAT_CSNP_H */
