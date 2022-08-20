/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Encoding table format definition.
 */

#ifndef FORMAT_ENCO_H
#define FORMAT_ENCO_H

#define ENCO_VERSION VERSION(1, 0)

typedef struct
{
    uint16_t format;
} Format0;
#define FORMAT0_SIZE SIZEOF(Format0, format)

typedef struct
{
    uint16_t format;
    uint16_t count;
    GlyphId *glyphId;
    uint8_t *code;
} Format1;
#define FORMAT1_SIZE(n) (SIZEOF(Format1, format) +      \
                         SIZEOF(Format1, count) +       \
                         (SIZEOF(Format1, glyphId[0]) + \
                          SIZEOF(Format1, code[0])) *   \
                             (n))

typedef struct
{
    uint16_t format;
    GlyphId glyphId[256];
} Format2;
#define FORMAT2_SIZE (SIZEOF(Format2, format) + \
                      SIZEOF(Format2, glyphId[0]) * 256)

typedef struct
{
    Fixed version;
    uint32_t *offset;
    DCL_ARRAY(void *, encoding);
} ENCOTbl;

enum {
    ENCO_STANDARD,
    ENCO_SPARSE,
    ENCO_DENSE
};

#endif /* FORMAT_ENCO_H */
