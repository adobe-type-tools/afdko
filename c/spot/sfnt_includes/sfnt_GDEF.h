/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GDEF.h	1.2
 * Changed:    3/18/98 16:43:20
 ***********************************************************************/

/*
 * TrueType Open glyph definition table format definition.
 */

#ifndef SFNTGDEF_H
#define SFNTGDEF_H


/* --- Caret Values --- */
typedef struct
	{
	Card16 CaretValueFormat;			/* =1 */
	Card16 Coordinate;
	} CaretValueFormat1;

typedef struct
	{
	Card16 CaretValueFormat;			/* =2 */
	Card16 CaretValuePoint;
	} CaretValueFormat2;

typedef struct
	{
	Card16 CaretValueFormat;			/* =3 */
	Card16 Coordinate;
	OFFSET(DeviceTable, DeviceTable);					/* => DeviceTable */
	} CaretValueFormat3;

typedef struct
	{
	Card16 CaretValueFormat;			/* =4 */
	Card16 IdCaretValue;
	} CaretValueFormat4;

typedef struct
	{
	Card16 PointCount;
	Card16 *PointIndex;					/* [PointCount] */
	} AttachPoint;

typedef struct
	{
	OFFSET(void *, Coverage); /* Coverage type */
	Card16 GlyphCount;
	OFFSET_ARRAY(AttachPoint, AttachPoint);
	} AttachList;

typedef struct
	{
	Card16 CaretCount;
	OFFSET_ARRAY(CaretValueFormat3, CaretValue); /* CaretValueFormat3 is the largest of the three ;
													this gives enough memory when we allocate 
													an array of these things */
	} LigGlyph;

typedef struct
	{
	OFFSET(void *, Coverage);					/* => Coverage */
	Card16 LigGlyphCount;
	OFFSET_ARRAY(LigGlyph, LigGlyph);
	} LigCaretList;

typedef struct
	{
	Card16 MarkSetTableFormat;
	Card16 MarkSetCount;
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
	IntX	hasMarkAttachClassDef;
	} GDEFTbl;

#endif /* SFNTGDEF_H */
