/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Linear threshold table format definition.
 */

#ifndef FORMAT_LTSH_H
#define FORMAT_LTSH_H

#define LTSH_VERSION 0

typedef struct
{
    uint16_t version;
    uint16_t numGlyphs;
    uint8_t *yPels;
} LTSHTbl;

#endif /* FORMAT_LTSH_H */
