/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Ligature caret table format definition.
 */

#ifndef FORMAT_LCAR_H
#define FORMAT_LCAR_H

#define lcar_VERSION VERSION(1, 0)

typedef struct
{
    GlyphId ligGlyph; /* [Not in format] */
    uint16_t cnt;
    int16_t *partial;
} LigCaretEntry;

typedef struct
{
    Fixed version;
    uint16_t format;
    Lookup lookup;
    DCL_ARRAY(LigCaretEntry, entry);
} lcarTbl;
#define TBL_HDR_SIZE (SIZEOF(lcarTbl, version) + \
                      SIZEOF(lcarTbl, format))

#endif /* FORMAT_LCAR_H */
