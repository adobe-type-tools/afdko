/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Multiple master variation table format definition.
 */

#ifndef FORMAT_MMVR_H
#define FORMAT_MMVR_H

#define MMVR_VERSION VERSION(1, 0)

typedef struct
{
    uint32_t Tag;
    uint16_t Default;
    uint16_t Scale;
} Axis;

typedef struct
{
    Fixed Version;
    uint16_t Flags;
    uint16_t AxisCount;
    Axis *axis;
} MMVRTbl;

#endif /* FORMAT_MMVR_H */
