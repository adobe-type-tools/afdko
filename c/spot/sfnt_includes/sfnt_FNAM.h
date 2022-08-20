/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * FOND name table format definition.
 */

#ifndef FORMAT_FNAM_H
#define FORMAT_FNAM_H

#define FNAM_VERSION VERSION(1, 0)

typedef struct
{
    uint8_t style;
    uint8_t *name;
} Client;

typedef struct
{
    uint16_t nClients; /* Temporary, not part of format */
    Client *client;
} Encoding;

typedef struct
{
    Fixed version;
    uint16_t nEncodings;
    uint16_t *offset;
    Encoding *encoding;
} FNAMTbl;

#endif /* FORMAT_FNAM_H */
