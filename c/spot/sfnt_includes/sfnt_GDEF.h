/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * TrueType Open glyph definition table format definition.
 */

#ifndef SFNTGDEF_H
#define SFNTGDEF_H

/* --- Caret Values --- */
typedef struct
{
    uint16_t CaretValueFormat; /* =1 */
    uint16_t Coordinate;
} CaretValueFormat1;

typedef struct
{
    uint16_t CaretValueFormat; /* =2 */
    uint16_t CaretValuePoint;
} CaretValueFormat2;

typedef struct
{
    uint16_t CaretValueFormat; /* =3 */
    uint16_t Coordinate;
    OFFSET(DeviceTable, DeviceTable); /* => DeviceTable */
} CaretValueFormat3;

typedef struct
{
    uint16_t CaretValueFormat; /* =4 */
    uint16_t IdCaretValue;
} CaretValueFormat4;

typedef struct
{
    uint16_t PointCount;
    uint16_t *PointIndex; /* [PointCount] */
} AttachPoint;

typedef struct
{
    OFFSET(void *, Coverage); /* Coverage type */
    uint16_t GlyphCount;
    OFFSET_ARRAY(AttachPoint, AttachPoint);
} AttachList;

typedef struct
{
    uint16_t CaretCount;
    OFFSET_ARRAY(CaretValueFormat3, CaretValue); /* CaretValueFormat3 is the largest of the three; */
                                                 /* this gives enough memory when we allocate      */
                                                 /* an array of these things                       */
} LigGlyph;

typedef struct
{
    OFFSET(void *, Coverage); /* => Coverage */
    uint16_t LigGlyphCount;
    OFFSET_ARRAY(LigGlyph, LigGlyph);
} LigCaretList;

typedef struct
{
    uint16_t MarkSetTableFormat;
    uint16_t MarkSetCount;
    LOFFSET_ARRAY(void *, Coverage);
} MarkGlyphSetsDef;

typedef struct
{
    Fixed Version;
    OFFSET(void *, GlyphClassDef); /* Class Definition Table type */
    OFFSET(AttachList, AttachList);
    OFFSET(LigCaretList, LigCaretList);
    OFFSET(void *, MarkAttachClassDef);
    OFFSET(MarkGlyphSetsDef, MarkGlyphSetsDef);
    int hasMarkAttachClassDef;
} GDEFTbl;

#endif /* SFNTGDEF_H */
