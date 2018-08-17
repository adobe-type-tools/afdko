/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Index to location table format definition.
 */

#ifndef FORMAT_LOCA_H
#define FORMAT_LOCA_H

typedef struct
{
    Card16 *offsets;
} Format0;

typedef struct
{
    Card32 *offsets;
} Format1;

typedef struct
{
    void *format;
} locaTbl;

#endif /* FORMAT_LOCA_H */
