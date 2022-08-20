/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Multiple master  table format definition.
 */

#ifndef FORMAT_MMFX_H
#define FORMAT_MMFX_H

#define MMFX_VERSION VERSION(1, 0)

typedef struct _MMFXMetric {
    uint16_t id;    /* Metric id */
    int16_t length; /* Charstring length */
    int32_t index;  /* Charstring index */
} MMFXMetric;

typedef struct _MMFXTbl {
    Fixed version;
    uint16_t nMetrics;
    uint16_t offSize; /* 2 or 4 byte length indicator */
    int32_t *offset;  /* [nMetrics] */
    uint8_t *cstrs;   /* Charstrings */
} MMFXTbl;

#endif /* FORMAT_MMFX_H */
