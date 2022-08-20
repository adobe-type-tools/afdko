/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Horizontal device metrics table definition.
 */

#ifndef FORMAT_HDMX_H
#define FORMAT_HDMX_H

#define hdmx_VERSION VERSION(1, 0)

typedef struct
{
    uint8_t pixelsPerEm;
    uint8_t maxWidth;
    uint8_t *widths;
} DeviceRecord;

typedef struct
{
    uint16_t version;
    uint16_t nRecords;
    uint32_t recordSize;
    DeviceRecord *record;
} hdmxTbl;
#define TBL_HDR_SIZE (SIZEOF(hdmxTbl, version) +  \
                      SIZEOF(hdmxTbl, nRecords) + \
                      SIZEOF(hdmxTbl, recordSize))

#endif /* FORMAT_HDMX_H */
