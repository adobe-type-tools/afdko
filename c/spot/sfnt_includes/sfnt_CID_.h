/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * CID data table format definition.
 */

#ifndef FORMAT_CID__H
#define FORMAT_CID__H

#define CID__VERSION VERSION(1, 0)

typedef struct
{
    Fixed Version;
    uint16_t Flags;
#define CID_REARRANGED_FONT (1 << 0)
    uint16_t CIDCount;
    uint32_t TotalLength;
    uint32_t AsciiLength;
    uint32_t BinaryLength;
    uint16_t FDCount;
} CID_Tbl;
#define CID__HDR_SIZE (SIZEOF(CID_Tbl, Version) +      \
                       SIZEOF(CID_Tbl, Flags) +        \
                       SIZEOF(CID_Tbl, CIDCount) +     \
                       SIZEOF(CID_Tbl, TotalLength) +  \
                       SIZEOF(CID_Tbl, AsciiLength) +  \
                       SIZEOF(CID_Tbl, BinaryLength) + \
                       SIZEOF(CID_Tbl, FDCount))

#endif /* FORMAT_CID__H */
